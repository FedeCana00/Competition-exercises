#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define NONE -1
#define DELAY 1000

typedef enum {false, true} Boolean;

struct manager_t {
    sem_t runner;
    sem_t semStart, semEnd, mutex;
    int ready, arrived, first, last;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->semStart, 0, 0);
    sem_init(&manager->semEnd, 0, 0);
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->runner, 0, 0);

    manager->ready = 0;
    manager->arrived = 0;
    manager->first = NONE;
    manager->last = NONE;
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void runnerWaitsStart(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);
    printf("[RUNNER %d] I'm READY\n", id);
    if(++manager->ready == N)
        sem_post(&manager->semStart);
    sem_post(&manager->mutex);

    sem_wait(&manager->runner);

    printf("[RUNNER %d] I'm RUNNING\n", id);
}

void runnerArrives(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);
    printf("[RUNNER %d] I'm ARRIVED %d\n", id, (manager->arrived + 1));
    if(++manager->arrived == N){
        manager->last = id;
        sem_post(&manager->semEnd);
    } else if(manager->arrived == 1)
        manager->first = id;

    sem_post(&manager->mutex);
}

void refreeWaitsStart(struct manager_t *manager){
    printf("[REFREE] I'm waiting\n");
    sem_wait(&manager->semStart);
}

void refreeStart(struct manager_t *manager){
    printf("[REFREE] GOOOOOO\n");

    for(int i = 0; i < N; i++)
        sem_post(&manager->runner);
}

void refreeResult(struct manager_t *manager){
    sem_wait(&manager->semEnd);

    sem_wait(&manager->mutex);
    printf("[REFREE] First %d, Last %d\n", manager->first, manager->last);
    sem_post(&manager->mutex);
}

void *refree(void *arg){
    printf("[REFREE] I'm on the track\n");
    refreeWaitsStart(&manager);
    printf("[REFREE] Ready, steady ...\n");
    refreeStart(&manager);
    refreeResult(&manager);

    return 0;
}

void *runner(void *arg){
    int id = (int) arg;
    runnerWaitsStart(&manager, id);
    microPause();
    runnerArrives(&manager, id);
    printf("[RUNNER %d] I'm going HOME\n", id);

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
        pthread_create(&thread, &threadAttr, runner, (void *) i);

    pthread_create(&thread, &threadAttr, refree, NULL);


    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N + 1; i++)
        pthread_join(thread, NULL);

    return 0;
}