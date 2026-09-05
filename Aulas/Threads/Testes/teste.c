#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* execute_thread(void* arg) {
    int value = *(int*)arg;
    printf("Thread em execução com o valor: %d\n", value);
    return NULL;
}

int main() {
    pthread_t thread_id;
    int arg = 42;

    if (pthread_create(&thread_id, NULL, execute_thread, &arg) != 0) {
        perror("Erro ao criar thread");
        return 1;
    }

    pthread_join(thread_id, NULL);

    printf("Thread realizada com sucesso!");
    return 0;
}