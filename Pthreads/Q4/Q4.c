#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#define DIMENSIONS 8192
#define INTERACTIONS 1000

// Define a matriz A, o vetor B e o duplo buffer
double matriz_A[DIMENSIONS][DIMENSIONS];
double vector_B[DIMENSIONS];

double previous_x[DIMENSIONS];
double new_x[DIMENSIONS];

// Barreira de sincronização
pthread_barrier_t barrier;

typedef struct
{
    int thread_id;
    int start_idx;
    int end_idx;
} ThreadData;


void* solve_jacobi(void* arg) 
{
    ThreadData *data = (ThreadData*)arg;

    for (int k = 0; k < INTERACTIONS; k++) {
        // Calcula as icógnitas respectivas dessa thread
        for (int i = data->start_idx; i < data->end_idx; i++) {
                double soma = 0.0;
                for (int j = 0; j < DIMENSIONS; j++) {
                    if (i != j) {
                        soma += matriz_A[i][j] * previous_x[j];
                    }
                }
                new_x[i] = (vector_B[i] - soma) / matriz_A[i][i];
            }
            // Espera todas as threads terminarem de calcular new_x
            pthread_barrier_wait(&barrier);

            // Copia o valor novo para o buffer antigo
            for (int i = data->start_idx; i < data->end_idx; i++) {
                previous_x[i] = new_x[i];
            }
            
            // Espera a atualização dos buffers antigos
            pthread_barrier_wait(&barrier);
        }
        pthread_exit(NULL);   
    }

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <numero_de_threads>\n", argv[0]);
        return -1;
    }

    int numero_threads = atoi(argv[1]);
    if (numero_threads <= 0 || numero_threads > DIMENSIONS) {
        printf("Número inválido de threads. Válidos entre 1 e %d.\n", DIMENSIONS);
        return(-1);
    }
    
    // Inicia o sistema
    for (int i = 0; i < DIMENSIONS; i++) {
        vector_B[i] = DIMENSIONS + 1.0;
        previous_x[i] = 1.0;
        for (int j = 0; j < DIMENSIONS; j++) {
            if (i == j) matriz_A[i][j] = DIMENSIONS * 2.0;
            else matriz_A[i][j] = 1.0;            
        }
    }

    pthread_barrier_init(&barrier, NULL, numero_threads);

    pthread_t threads[numero_threads];
    ThreadData thread_data[numero_threads];

    int itens_base = DIMENSIONS / numero_threads;
    int remainder = DIMENSIONS % numero_threads;
    int current_idx = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < numero_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start_idx = current_idx;

        int count = itens_base + (i < remainder ? 1:0);
        thread_data[i].end_idx = current_idx + count;
        current_idx = thread_data[i].end_idx;

        pthread_create(&threads[i], NULL, solve_jacobi, &thread_data[i]);
    }

    // Espera as threads terminarem
    for (int i = 0; i < numero_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    // Encerra o contador
    gettimeofday(&end, NULL);

    double execution_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Threads: %d | Tempo de execucao: %.6f segundos\n", numero_threads, execution_time);
    printf("Resultado x[0]: %f | x[%d]: %f\n", previous_x[0], DIMENSIONS - 1, previous_x[DIMENSIONS - 1]);

    pthread_barrier_destroy(&barrier);
    return 0;
    

    

    

}

