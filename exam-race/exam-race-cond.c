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
    pthread_mutex_t mutex;
    pthread_cond_t condStart, condEnd, runner;
    int ready, arrived, first, last;
} manager;

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutex, &mutexAttr);
    pthread_mutexattr_init(&mutexAttr);

    pthread_condattr_init(&condAttr);
    pthread_cond_init(&manager->runner, &condAttr);
    pthread_cond_init(&manager->condStart, &condAttr);
    pthread_cond_init(&manager->condEnd, &condAttr);
    pthread_condattr_init(&condAttr);

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
    pthread_mutex_lock(&manager->mutex);
    printf("[RUNNER %d] I'm READY\n", id);
    if(++manager->ready == N)
        pthread_cond_signal(&manager->condStart);
    
    while (manager->ready != N)
        pthread_cond_wait(&manager->runner, &manager->mutex);

    pthread_mutex_unlock(&manager->mutex);

    printf("[RUNNER %d] I'm RUNNING\n", id);
}

void runnerArrives(struct manager_t *manager, int id){
    pthread_mutex_lock(&manager->mutex);
    printf("[RUNNER %d] I'm ARRIVED %d\n", id, (manager->arrived + 1));
    if(++manager->arrived == N){
        manager->last = id;
        pthread_cond_signal(&manager->condEnd);
    } else if(manager->arrived == 1)
        manager->first = id;

    pthread_mutex_unlock(&manager->mutex);
}

void refreeWaitsStart(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);
    printf("[REFREE] I'm waiting\n");
    while(manager->ready != N)
        pthread_cond_wait(&manager->condStart, &manager->mutex);
    pthread_mutex_unlock(&manager->mutex);
}

void refreeStart(struct manager_t *manager){
    printf("[REFREE] GOOOOOO\n");
    pthread_cond_broadcast(&manager->runner);
}

void refreeResult(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);
    while(manager->arrived != N)
        pthread_cond_wait(&manager->condEnd, &manager->mutex);
    pthread_mutex_unlock(&manager->mutex);

    pthread_mutex_lock(&manager->mutex);
    printf("[REFREE] First %d, Last %d\n", manager->first, manager->last);
    pthread_mutex_unlock(&manager->mutex);
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