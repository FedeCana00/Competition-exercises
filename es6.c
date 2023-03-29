#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define DELAY 5
#define THINKING 0
#define EATING 1

typedef enum {false, true} Boolean;

struct manager_t {
    sem_t fork[N];
} manager;

void initManager(struct manager_t *manager){
    for(int i = 0; i < N; i++)
        sem_init(&manager->fork[i], 0, 1);
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void eat(struct manager_t *manager, int id){
    if(id == 0){
        sem_wait(&manager->fork[id]);
        sem_wait(&manager->fork[id + 1 % N]);
    } else {
        sem_wait(&manager->fork[id + 1 % N]);
        sem_wait(&manager->fork[id]);
    }

    printf("[THREAD %d] I'm eating (fork %d and fork %d)\n", id, id, id + 1 % N);
    microPause();

    sem_post(&manager->fork[id + 1 % N]);
    sem_post(&manager->fork[id]);

    printf("[THREAD %d] STOP eating\n", id);
}

void think(struct manager_t *manager, int id){
    printf("[THREAD %d] I'm thinking\n", id);
}

void *philosopher(void *arg){
    for(;;){
        think(&manager, (int) arg);
        microPause();
        eat(&manager, (int) arg);
        microPause();
    }
    return 0;
}

int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;
    int i;

    // init struct
    initManager(&manager);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, philosopher, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}