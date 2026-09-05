#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define N_ELEMENTOS 10000
#define N_THREADS 10
#define PARTITION (N_ELEMENTOS / N_THREADS)

// Struct para enviar todos os dados necessários para a cada Thread
typedef struct {
    int id;
    int front;
    int back;
    int soma;
    int *partition;
}ThreadData;

// Soma executada por cada Thread
void *SomadorParcial(void *agr){
    // Desempacota todos os dados respectivos à essa thread
    ThreadData *data = (ThreadData*)agr;
    
    // Somador
    while (data->front < data->back) {
        data->soma += data->partition[data->front];
        data->front++;
    }
    
    printf("ID: %d | Soma: %d\n", data->id, data->soma);
    pthread_exit(NULL);
}


int main (int argc, char *argv[]){
    int soma_final = 0;
    int rc;

    // Cria o array
    int *array = (int*)malloc(N_ELEMENTOS * sizeof(int));
    // Cria as N Threads    
    pthread_t *threads = (pthread_t*)malloc(N_THREADS * sizeof(pthread_t));
    // Cria N datas para N Threads
    ThreadData *dados_threads = (ThreadData*)malloc(N_THREADS * sizeof(ThreadData));

    // Fallback para falha na alocação
    if (array == NULL || threads == NULL || dados_threads == NULL) {
        printf("Erro de alocação.");
        exit(-1);
    }
    
    // Popula o array
    for (int k = 0; k < N_ELEMENTOS; k++) {
        array[k] = 1;
    }

    // Criação das Threads
    for (int i = 0; i < N_THREADS; i++) {
        // Salva os dados de cada Thread
        dados_threads[i].id = i;
        dados_threads[i].front = i * PARTITION;
        dados_threads[i].back = (i + 1) * PARTITION;
        dados_threads[i].soma = 0;
        dados_threads[i].partition = array;

        rc = pthread_create(&threads[i], NULL, SomadorParcial, &dados_threads[i]);

        if (rc){
            printf("ERRO; código de retorno é %d\n", rc);
            exit(-1);
            }
    }
    
    // A Thread principal aguarda o término das restantes
    for (int p = 0; p < N_THREADS; p++) {
        pthread_join(threads[p], NULL);
    }

    for (int u = 0; u < N_THREADS; u++) {
        soma_final += dados_threads[u].soma;
    }
    

    printf("Soma Final: %d\n", soma_final);
    pthread_exit(NULL);
}
