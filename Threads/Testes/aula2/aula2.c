#include <stdio.h>
#include <stdlib.h>                /* PRODUTOR - CONSUMIDOR */
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h> /* For O_CREAT, O_EXCL */
#include <sys/stat.h> /* For mode constants */

/* #define LIMIT 10000 */
#define LIMIT 10000 
#define BSIZE 10   /* Tamanho do buffer */
#define PRODS 100  /* Número máximo de elementos que serão produzidos (opcional) */

const char *sem_vazios = "/sem_vazios";
const char *sem_cheios = "/sem_cheios";
const char *sem_mex = "/sem_mex";

typedef struct {
	int buf[BSIZE]; /* Array (buffer) de tamanho BSIZE (10) */
	int nextin;    /* Posição onde próximo elemento produzido será inserido */
	int nextout;   /* posição de onde o próximo elemento consumido será retirado */
	sem_t *vazios;  /* Semáforo: Controle das posições vazias */
	sem_t *cheios;  /* Semáforo: Controle das posições preenchidas */
	pthread_mutex_t *mex;     /* Semáforo: Controle da Exclusão Mútua */
} buffer_t;

void delay() {
  for (int m=0; m<=LIMIT;m++) {}
}

void initbuffer(buffer_t * b)
{
	b->vazios = sem_open(sem_vazios, O_CREAT, 0600, BSIZE); /* Inicialmente todas as posições vazias */
	b->cheios = sem_open(sem_cheios, O_CREAT, 0600, 0); /* Nenhum elemento foi produzido ainda */

    pthread_mutex_init(&b->mex, NULL);

  	b->nextin = 0;
  	b->nextout = 0;
}

double randomn;

void producer(void *arg) {
    buffer_t *b = (buffer_t*) arg;
		
	for (int i = 0; i < PRODS; i++) {
		sem_wait(b->vazios);

        // PRÉ
        pthread_mutex_lock(&b->mex);

        // --- REGIÃO CRÍTICA ---
		b->buf[b->nextin++] = i; /* Insere elemento produzido no buffer */
        b->nextin = (b->nextin + 1) % BSIZE;
		printf("Produzido %d\n", i); /* Imprime elemento produzido */

        // PÓS
        pthread_mutex_unlock(&b->mex);

        sem_post(b->cheios);

	}
    return NULL;
}

void consumer(void *arg) {
    buffer_t *b = (buffer_t*)arg;
	int item; /* Elemento que será produzido */
	int total = 0; 

	for (int i = 0; i < PRODS; i++) {
        sem_wait(b->cheios);

        // PRÉ
        pthread_mutex_lock(&b->mex);

        // --- REGIÃO CRÍTICA ---
        item = b->buf[b->nextout]; /* item recebe elemento consumido */
        b->nextout = (b->nextout + 1) % BSIZE;
        total += item;
		printf("Consumido       %d\n", item); /* imprime elemento consumido - item */
        
        // PÓS
        pthread_mutex_unlock(&b->mex);

        sem_post(b->vazios);

	}
	printf("Total consumido  %d\n", total); /* imprime elemento consumido - item */
    return NULL;
}

int main()
{
	pthread_t thprod, thcons;
	pthread_attr_t attr;
	buffer_t buffer;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	
	initbuffer(&buffer); /* Inicializa o buffer */
	pthread_create(&thprod, &attr, (void *)producer, (void *) &buffer); /*Cria uma thread produtor*/
	pthread_create(&thcons, &attr, (void *)consumer, (void *) &buffer); /*Cria uma thread consumidor*/
	
	pthread_join(thprod, NULL);
  	pthread_join(thcons, NULL);
  	return 0;
}