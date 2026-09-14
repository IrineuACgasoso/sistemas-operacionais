// Compilação: gcc -O2 q5.c -o q5 -pthread | Execução: ./q5

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>

#define CORES 4 // Número de núcleos

// Estrutura para representar um elemento da lista encadeada pronta 
typedef struct Task {
    void (*function)(void*);
    void *arg;
    struct Task *next;
} Task;

// Fila Pronta
static Task *queue_start = NULL;
static Task *queue_end = NULL;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_empty_queue_cond = PTHREAD_COND_INITIALIZER;

// Controle de núcleos
static int empty_cores = CORES;
static pthread_mutex_t core_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty_cores_cond = PTHREAD_COND_INITIALIZER;

// Flag de controle de ciclo de vida do kernel (1 = ligado, 0 = desligando)
static volatile int sistema_ativo = 1;
static pthread_t thread_escalonador_id;

// Adiciona tarefas à lista pronta atomicamente
void agendar(void (*function)(void*), void *arg) {
    // Aloca a nova task
    Task *nova = malloc(sizeof(Task));
    nova->function = function;
    nova->arg = arg;
    nova->next = NULL;

    // Região Crítica da Fila
    pthread_mutex_lock(&queue_mutex);
    if (queue_end == NULL) {
        queue_start = nova; // Fila vazia
    }
    else {
        queue_end->next = nova; // Final da fila
    }
    queue_end = nova;

    // Notifica o escalonador que há trabalho
    pthread_cond_signal(&not_empty_queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Estrutura para passar a função e argumento para a worker thread
typedef struct {
    void (*function)(void*);
    void *arg;
} WorkerArgs;

// Simula a execução da tarefa em um core
static void *worker_thread(void *arg) {
    WorkerArgs *wargs = (WorkerArgs*)arg;

    // Executa a tarefa do usuário
    wargs->function(wargs->arg);
    free(wargs);

    // Região Crítica dos Núcleos
    pthread_mutex_lock(&core_mutex);
    empty_cores++;

    // Acorda o escalonador 
    pthread_cond_signal(&empty_cores_cond);
    pthread_mutex_unlock(&core_mutex);

    return NULL;
}

// Gerencia a atribuição de tarefas aos núcleos disponíveis
static void *escalonador(void *arg) {
    // Continua o laço enquanto o sistema estiver ligado OU existirem tarefas prontas
    while (sistema_ativo || queue_start != NULL) {
        // Bloqueio 1
        pthread_mutex_lock(&queue_mutex);

        // Dorme enquanto o sistema estiver ligado E não houverem tarefas prontas
        while (sistema_ativo && queue_start == NULL) {
            pthread_cond_wait(&not_empty_queue_cond, &queue_mutex);
        }
        
        // Se a ordem é para desligar o kernel E a fila está vazia, encerra o escalonador
        if (!sistema_ativo && queue_start == NULL) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        // Retira a tarefa da fila (FIFO)
        Task *tarefa = queue_start;
        queue_start = queue_start->next;
        if (queue_start == NULL) queue_end == NULL;
        pthread_mutex_unlock(&queue_mutex);

        // Bloqueio 2
        pthread_mutex_lock(&core_mutex);
        while (empty_cores == 0) {
            pthread_cond_wait(&empty_cores_cond, &core_mutex);
        }
        empty_cores--; // Consome um núcleo
        pthread_mutex_unlock(&core_mutex);

        // Prepara argumentos e envia para a worker thread
        WorkerArgs *wargs = malloc(sizeof(WorkerArgs));
        wargs->function = tarefa->function;
        wargs->arg = tarefa->arg;   
        free(tarefa); // Desaloca o nó

        pthread_t worker;
        pthread_create(&worker, NULL, worker_thread, wargs);
        pthread_detach(worker); // Desacopla para liberar recursos ao finalizar sem join
    }
    return NULL;
}

// Inicializa a thread do escalonador em segundo plano
void iniciar_kernel() {
    pthread_create(&thread_escalonador_id, NULL, escalonador, NULL);
}

// Encerra o kernel e garante a parada da thread do escalonador
void desligar_kernel() {
    pthread_mutex_lock(&queue_mutex);
    sistema_ativo = 0;
    // Acorda o escalonador caso ele esteja dormindo, para que ele possa encerrar
    pthread_cond_signal(&not_empty_queue_cond);
    pthread_mutex_unlock(&queue_mutex);

    pthread_join(thread_escalonador_id, NULL);
}