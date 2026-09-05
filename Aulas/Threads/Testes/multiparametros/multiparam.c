#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int a;
    int b;
} Operandos;

void* somar(void* arg) {
    Operandos* op = (Operandos*)arg;
    int* resultado = malloc(sizeof(int)); // Deve ser na heap para não desalocar na saída da função
    *resultado = op->a + op->b;
    
    return (void*)resultado;
}

int main() {
    pthread_t thread;
    Operandos dados = {15, 27};
    int* res;

    pthread_create(&thread, NULL, somar, &dados);
    
    // O segundo argumento do join captura o ponteiro retornado pela thread
    pthread_join(thread, (void**)&res);
    
    printf("Resultado retornado: %d\n", *res);
    free(res); // Libera a memória alocada dentro da thread
    return 0;
}