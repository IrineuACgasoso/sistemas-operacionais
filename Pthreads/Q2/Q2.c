// Compilação: gcc -O2 q2.c -o q2 -pthread | Execução: ./q2
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_LINES 7

// Códigos de escape ANSI para as cores de exibição do painel
const char *colors[NUM_LINES] = { 
    "\033[40;37m", // Linha 1: Preto
    "\033[41;37m", // Linha 2: Vermelho
    "\033[42;30m", // Linha 3: Verde
    "\033[43;30m", // Linha 4: Amarelo
    "\033[44;37m", // Linha 5: Azul
    "\033[45;37m", // Linha 6: Roxo
    "\033[46;30m"  // Linha 7: Ciano
};


#define NAME_SIZE 35
#define FILE_NAME_SIZE 50

// Mutex para controlar quem pode entrar na sala
pthread_mutex_t mutex_lines[NUM_LINES];
// Mutex de controle para quem pode alterar a tela
pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct {
    int thread_id;
    char filename[FILE_NAME_SIZE];
}   ThreadData;

// Função executada por cada thread para leitura do arquivo e impressão na tela
void *ProcessFile(void *arg) 
{
    // Desempacota a struct
    ThreadData *data = (ThreadData *)arg;

    // Abre o arquivo respectivo desta thread
    FILE *file = fopen(data->filename, "r");
    if (file == NULL) {
        pthread_exit(NULL);
    }

    char name[NAME_SIZE];
    int consultorio;

    // Lê os dados do arquivo no formato "<nome_paciente> <numero_consultorio>"
    // Usa %19s para prevenir estouro de buffer (Buffer Overflow)
    while (fscanf(file, "%19s %d", name, &consultorio) != EOF)  {
        int line = consultorio - 1;
        
        // Limita acesso inválido no vetor
        if (line >= 0 && line < NUM_LINES) {
            // Bloqueia o consultório atual
            pthread_mutex_lock(&mutex_lines[line]);
            // ÁREA CRÍTICA
            // Impede a escrita global exceto para esta thread
            pthread_mutex_lock(&screen_mutex); 
            printf("\033[%d;1H\033[2K", line + 4); // Move o cursor e limpa linha
            printf("%sPaciente %-15s Consultório %d\033[0m", colors[line], name, consultorio);
            fflush(stdout);
            pthread_mutex_unlock(&screen_mutex);
            
            // Cooldown antes da próxima atualização na tela
            sleep(3);
            
            // Libera o consultório para o próximo paciente
            pthread_mutex_unlock(&mutex_lines[line]);
        }
    }
    fclose(file);
    pthread_exit(NULL);
}
    


int main (int argc, char *argv[]) {
    // Inicia o Mutex
    for (int i = 0; i < NUM_LINES; i++) {
        pthread_mutex_init(&mutex_lines[i], NULL);
    }
    // Limpa a tela e move o cursor para o início da linha
    printf("\033[2J\033[1;1H"); 
    printf("=========================\n");
    printf("        ATENDIMENTO      \n");
    printf("=========================\n");

    // Nomes iniciais no painel
    const char *iniciais[] = {"Gabro", "Feldspato", "Cherte", "Riebeck", "Esker", "Gneiss", "Corneana"};
    for (int k = 0; k < NUM_LINES; k++) {
        printf("%sPaciente %-15s Consultório %d\033[0m\n", colors[k], iniciais[k], k + 1);
    }
    fflush(stdout);
    
    // Alocação dinâmica dos threads e de suas structs
    pthread_t *threads = (pthread_t*)malloc(NUM_LINES * sizeof(pthread_t));
    ThreadData *dados_threads = (ThreadData*)malloc(NUM_LINES * sizeof(ThreadData));

    // Fallback para falha na alocação
    if (threads == NULL || dados_threads == NULL) {
        printf("Erro de alocação.");
        exit(-1);
    }

    // Criação das N threads leitoras de arquivo
    for (int y = 0; y < NUM_LINES; y++) {
        dados_threads[y].thread_id = y;
        sprintf(dados_threads[y].filename, "txt/pacientes_%d.txt", y + 1);
        pthread_create(&threads[y], NULL, ProcessFile, &dados_threads[y]);
    }
    
    // A Thread principal aguarda o término das restantes
    for (int p = 0; p < NUM_LINES; p++) {
        pthread_join(threads[p], NULL);
    }

    // Posiciona o cursor abaixo do painel
    printf("\033[%d;1H\n", NUM_LINES + 5);

    // Liberação dos recursos do SO
    for (int u = 0; u < NUM_LINES; u++) {
        pthread_mutex_destroy(&mutex_lines[u]);
    }
    pthread_mutex_destroy(&screen_mutex);

    // Desalocação da memória
    free(threads);
    free(dados_threads);
}
