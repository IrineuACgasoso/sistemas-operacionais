#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
 
#define TAM_ARRAY 20
 
/* região critica compartilhada */
static int array_compartilhado[TAM_ARRAY];
 
/* Estado do controle*/
static pthread_mutex_t mutex_estado = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_leitores  = PTHREAD_COND_INITIALIZER; /* leitoras esperam aqui */
static pthread_cond_t  cond_escritores = PTHREAD_COND_INITIALIZER; /* escritoras esperam aqui */
 
static int leitores_ativos   = 0; /* quantas leitoras estao lendo agora */
static int escritor_ativo    = 0; /* 0 ou 1: existe escritora escrevendo agora */
static int escritores_esperando = 0; /* quantas escritoras estao na fila (garante prioridade) */
 
/* mutex pra organizar */
static pthread_mutex_t mutex_print = PTHREAD_MUTEX_INITIALIZER;
 
/* flag para as threads pararem */
static volatile sig_atomic_t executando = 1;
 
/*Funcoesde sincronizacao  */
 
static void iniciar_leitura(int id_leitora) {
    pthread_mutex_lock(&mutex_estado);
    /* uma leitora so pode entrar se nao houver escritora escrevendo E
       nao houver escritora esperando (isso da prioridade ao escritor) */
    while (executando &&(escritor_ativo || escritores_esperando > 0)) {
        pthread_cond_wait(&cond_leitores, &mutex_estado);
    }
    leitores_ativos++;
    pthread_mutex_unlock(&mutex_estado);
}
 
static void finalizar_leitura(int id_leitora) {
    pthread_mutex_lock(&mutex_estado);
    leitores_ativos--;
    /* se essa foi a ultima leitora saindo, acorda uma possivel escritora
       que esteja esperando */
    if (leitores_ativos == 0) {
        pthread_cond_signal(&cond_escritores);
    }
    pthread_mutex_unlock(&mutex_estado);
}
 
static void iniciar_escrita(int id_escritora) {
    pthread_mutex_lock(&mutex_estado);
    escritores_esperando++;
    /* uma escritora so entra quando nao houver ninguem (nem leitora nem
       outra escritora) usando a regiao critica */
    while (executando &&(escritor_ativo || leitores_ativos > 0)) {
        pthread_cond_wait(&cond_escritores, &mutex_estado);
    }
    escritores_esperando--;
    escritor_ativo = 1;
    pthread_mutex_unlock(&mutex_estado);
}
 
static void finalizar_escrita(int id_escritora) {
    pthread_mutex_lock(&mutex_estado);
    escritor_ativo = 0;
    /* prioridade ao escritor: se houver outra escritora esperando, ela
       acorda primeiro; so quando nao ha mais escritoras esperando é que
       as leitoras sao liberadas em bloco */
    if (escritores_esperando > 0) {
        pthread_cond_signal(&cond_escritores);
    } else {
        pthread_cond_broadcast(&cond_leitores);
    }
    pthread_mutex_unlock(&mutex_estado);
}
 
/* Threads leitoras e escritoras*/
 
typedef struct {
    int id;
    unsigned int seed; /* seed propria para rand_r, evita disputa por rand() global */
} ThreadArg;
 
static void *thread_leitora(void *arg) {
    ThreadArg *ta = (ThreadArg *)arg;
    while (executando) {
        iniciar_leitura(ta->id);
        if(!executando) { finalizar_leitura(ta->id); break;}
        int pos = rand_r(&ta->seed) % TAM_ARRAY;
        int valor = array_compartilhado[pos];
 
        pthread_mutex_lock(&mutex_print);
        printf("[Leitora %d] leu array[%d] = %d\n", ta->id, pos, valor);
        pthread_mutex_unlock(&mutex_print);
 
        finalizar_leitura(ta->id);
 
        /* pequena pausa para nao saturar a CPU e permitir intercalar com
           outras threads de forma visivel na saida */
        usleep((rand_r(&ta->seed) % 200 + 50) * 1000);
    }
    free(ta);
    return NULL;
}
 
static void *thread_escritora(void *arg) {
    ThreadArg *ta = (ThreadArg *)arg;
    while (executando) {
        iniciar_escrita(ta->id);
        if(!executando) { finalizar_escrita(ta->id); break;}
 
        int pos = rand_r(&ta->seed) % TAM_ARRAY;
        int valor = rand_r(&ta->seed) % 1000;
        array_compartilhado[pos] = valor;
 
        pthread_mutex_lock(&mutex_print);
        printf("[Escritora %d] escreveu array[%d] = %d\n", ta->id, pos, valor);
        pthread_mutex_unlock(&mutex_print);
 
        finalizar_escrita(ta->id);
 
        usleep((rand_r(&ta->seed) % 300 + 100) * 1000);
    }
    free(ta);
    return NULL;
}
 
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N_leitoras> <M_escritoras> [segundos]\n", argv[0]);
        return 1;
    }
 
    int N = atoi(argv[1]); /* leitoras */
    int M = atoi(argv[2]); /* escritoras */
    int segundos = (argc >= 4) ? atoi(argv[3]) : 0; /* 0 = roda ate Ctrl+C */
 
    if (N <= 0 || M <= 0) {
        fprintf(stderr, "N e M devem ser inteiros positivos.\n");
        return 1;
    }
 
    for (int i = 0; i < TAM_ARRAY; i++) {
        array_compartilhado[i] = 0;
    }
 
    pthread_t *leitoras = malloc(N * sizeof(pthread_t));
    pthread_t *escritoras = malloc(M * sizeof(pthread_t));
 
    for (int i = 0; i < N; i++) {
        ThreadArg *ta = malloc(sizeof(ThreadArg));
        ta->id = i;
        ta->seed = (unsigned int) time(NULL) ^ (i * 7919);
        pthread_create(&leitoras[i], NULL, thread_leitora, ta);
    }
    for (int i = 0; i < M; i++) {
        ThreadArg *ta = malloc(sizeof(ThreadArg));
        ta->id = i;
        ta->seed = (unsigned int) time(NULL) ^ (i * 104729 + 1);
        pthread_create(&escritoras[i], NULL, thread_escritora, ta);
    }
 
    if (segundos > 0) {
        sleep(segundos);
        executando = 0; /* sinaliza as threads para pararem o loop infinito */
        /* como as threads podem estar bloqueadas em cond_wait, cancelamos
           explicitamente para garantir que o programa termine */
        pthread_mutex_lock(&mutex_estado);
        pthread_cond_broadcast(&cond_leitores);
        pthread_cond_broadcast(&cond_escritores);
        pthread_mutex_unlock(&mutex_estado);
    }
 
    for (int i = 0; i < N; i++) pthread_join(leitoras[i], NULL);
    for (int i = 0; i < M; i++) pthread_join(escritoras[i], NULL);
 
    free(leitoras);
    free(escritoras);
 
    printf("Execucao finalizada.\n");
    return 0;
}
 
 