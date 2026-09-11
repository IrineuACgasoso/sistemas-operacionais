// Compilação: gcc -O2 q5.c -o q5 -pthread | Execução: ./q5

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>

#define CORES 4

// Estrutura para representar um elemento da lista pronta 
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


void agendar(void (*function)(void*), void *arg) {
    // Aloca a nova task
    Task *nova = malloc(sizeof(Task));
    nova->function = function;
    nova->arg = arg;
    nova->next = NULL;

    // Região Crítica da Fila
    pthread_mutex_lock(&queue_mutex);
    if (queue_end == NULL) {
        queue_start = nova;
    }
    else {
        queue_end->next = nova;
    }
    queue_end = nova;

    // Notifica o escalonador 
    pthread_cond_signal(&not_empty_queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}


typedef struct {
    void (*function)(void*);
    void *arg;
} WorkerArgs;

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


static void *escalonador(void *arg) {
}