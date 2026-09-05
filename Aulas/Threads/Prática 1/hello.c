#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

void *PrintHello(void *threadid){
    int id = *(int*)threadid;
    printf("Olá, mundo!. Thread id: %d\n", id);
    pthread_exit(NULL);
}

int main (int argc, char *argv[]){
    pthread_t threads[5];
    int thread_ids[5];
    int rc;

    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i;
        rc = pthread_create(&threads[i], NULL, PrintHello, &thread_ids[i]);

        if (rc){
            printf("ERRO; código de retorno é %d\n", rc);
            exit(-1);
            }
    }
    
    for (int k = 0; k < 5; k++) {
        pthread_join(threads[k], NULL);
    }
    pthread_exit(NULL);
}