#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int tarefa_pronta = 0;

void* consumidor(void* arg) {
    pthread_mutex_lock(&mutex);
    
    // SEMPRE use while para evitar acionamentos espúrios (spurious wakeups)
    while (!tarefa_pronta) {
        // Solta o mutex e dorme até receber o cond_signal
        pthread_cond_wait(&cond, &mutex);
    }
    printf("Consumidor: Dados recebidos! Processando...\n");
    int value = *(int*)arg;
    printf("%d\n", value);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* produtor(void* arg) {
    sleep(1); // Simula trabalho pesado
    
    pthread_mutex_lock(&mutex);
    tarefa_pronta = 1;
    *(int*)arg = 1000;
    printf("Produtor: Trabalho concluído. Notificando...\n");
    
    pthread_cond_signal(&cond); // Acorda o consumidor
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main() {
    pthread_t t_produtor, t_consumidor;
    int value = 0;

    // Criamos o consumidor primeiro para garantir que ele entre em espera antes do sinal
    pthread_create(&t_consumidor, NULL, consumidor, &value);
    pthread_create(&t_produtor, NULL, produtor, &value);

    // Aguarda o encerramento de ambas
    pthread_join(t_consumidor, NULL);
    pthread_join(t_produtor, NULL);

    // Destrói os recursos de sincronização
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}