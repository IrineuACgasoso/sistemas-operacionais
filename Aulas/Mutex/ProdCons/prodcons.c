#include <stdio.h>
#include <pthread.h> 
#include <stdlib.h>

#define BUFFER_SIZE 10
#define NUM_ITEMS 200

int buff[BUFFER_SIZE];  /* buffer size = 10; */
int items = 0; // number of items in the buffer.
int first = 0;
int last = 0; 

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
 
void *producer();
void *consumer();

int main() {
  pthread_t consumer_thread[3]; 
  pthread_t producer_thread; 
  pthread_create(&consumer_thread,NULL,consumer,NULL);
  pthread_create(&producer_thread,NULL,producer,NULL);
  pthread_join(producer_thread,NULL);  
  pthread_join(consumer_thread,NULL);
}

void put(int i){
  pthread_mutex_lock(&mutex);
  while(items == BUFFER_SIZE) {
    pthread_cond_wait(&empty, &mutex);
  }
  buff[last] = i;
  printf("pos %d: ", last);
  items++; last++;
  if(last==BUFFER_SIZE) { last = 0; } 
  if(items == 1) { 
    pthread_cond_signal(&fill); }
  pthread_mutex_unlock(&mutex); 
}
void *producer() {
  int i = 0;
  printf("Produtor\n");
  for(i=0;i<NUM_ITEMS * 3; i++) {
    put(i);
    printf("Produzi %d \n",i);
  }
  pthread_exit(NULL);
}

int get(){
  int result;
  pthread_mutex_lock(&mutex);
 while(items == 0){
    pthread_cond_wait(&fill, &mutex);
  }
  result = buff[first];
  printf("pos %d: ", first);
  items--; first++;
  if(first==BUFFER_SIZE) { first = 0; }
  if(items == BUFFER_SIZE - 1){
    pthread_cond_signal(&empty);}
  pthread_mutex_unlock(&mutex);
  return result;
}
void *consumer() {
  int i,v;
  printf("Consumidor\n");
  for (i=0;i<NUM_ITEMS;i++) {
    v = get();
    printf("Consumi %d  \n",v);
  }
  pthread_exit(NULL);
}
