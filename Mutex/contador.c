#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

long contador = 0; 
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
void *inc(void *threadid){
  int i = 0; for(; i < 9000000; i++) { 
    pthread_mutex_trylock(&mymutex); 
    contador++; 
    pthread_mutex_unlock(&mymutex); }
}
void *dec(void *threadid){   
  int i = 0;
  for(; i < 9000000; i++) { 
    pthread_mutex_trylock(&mymutex); 
    contador--; 
    pthread_mutex_unlock(&mymutex); }
}
int main (int argc, char *argv[]){   
  pthread_t thread1, thread2;   
  pthread_create(&thread1, NULL, inc, NULL); 
  pthread_create(&thread2, NULL, dec, NULL); 
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL); 
  printf("Valor final do contador: %ld\n", contador);
  pthread_exit(NULL);
}