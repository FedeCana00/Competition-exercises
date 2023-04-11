#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 3
#define RUNNERS 2
#define NONE -1
#define DELAY 1000

typedef enum {false, true} Boolean;

struct manager_t {
    pthread_mutex_t mutex;
    pthread_cond_t condWaitRunner, condWaitRefree, condResultRefree;

    int winner, ready, whoCaughtFlag;
} manager;

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    pthread_condattr_init(&condAttr);
    pthread_cond_init(&manager->condWaitRunner, &condAttr);
    pthread_cond_init(&manager->condWaitRefree, &condAttr);
    pthread_cond_init(&manager->condResultRefree, &condAttr);
    pthread_condattr_destroy(&condAttr);

    manager->winner = NONE;
    manager->whoCaughtFlag = NONE;
    manager->ready = 0;
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void refreeWaitsStart(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);

    while(manager->ready != RUNNERS)
        pthread_cond_wait(&manager->condWaitRefree, &manager->mutex);

    pthread_mutex_unlock(&manager->mutex);
}

void refreeStart(struct manager_t *manager){
    pthread_cond_broadcast(&manager->condWaitRunner);
}

int refreeResult(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);

    while(manager->winner == NONE)
        pthread_cond_wait(&manager->condResultRefree, &manager->mutex);

    pthread_mutex_unlock(&manager->mutex);

    return manager->winner;
}

void *refree(void *arg){
    printf("[REFREE] I'm on the track\n");
    refreeWaitsStart(&manager);
    printf("[REFREE] Ready, steady ...\n");
    refreeStart(&manager);
    printf("[REFREE] ... The winner is RUNNER %d\n", refreeResult(&manager));
    
    // microPause();
    return 0;
}

void runnerWaitsStart(struct manager_t *manager, int id){
    pthread_mutex_lock(&manager->mutex);

    while (manager->ready != RUNNERS){
        if(++manager->ready == RUNNERS)
            pthread_cond_signal(&manager->condWaitRefree);
        printf("[RUNNER %d] I'm ready to start\n", id);
        pthread_cond_wait(&manager->condWaitRunner, &manager->mutex);
    }
    
    pthread_mutex_unlock(&manager->mutex);
}

int gotYou(struct manager_t *manager, int id){
    pthread_mutex_lock(&manager->mutex);

    if(manager->winner != NONE){
        pthread_mutex_unlock(&manager->mutex);
        return 0;
    }

    manager->winner = id;
    pthread_cond_signal(&manager->condResultRefree);
    printf("[RUNNER %d] Wakey wakey\n", id);
    pthread_mutex_unlock(&manager->mutex);
    return 1;
}

int amISafe(struct manager_t *manager, int id){
    pthread_mutex_lock(&manager->mutex);

    if(manager->winner != NONE){
        pthread_mutex_unlock(&manager->mutex);
        return 0;
    }

    manager->winner = id;
    pthread_cond_signal(&manager->condResultRefree);
    printf("[RUNNER %d] Wakey wakey\n", id);
    pthread_mutex_unlock(&manager->mutex);
    return 1;
}

int catchFlag(struct manager_t *manager, int id){
    pthread_mutex_lock(&manager->mutex);

    if(manager->whoCaughtFlag == NONE){
        manager->whoCaughtFlag = id;
        printf("[RUNNER %d] I've catch the flag!\n", id);
        pthread_mutex_unlock(&manager->mutex);
        return 1;
    }
    
    pthread_mutex_unlock(&manager->mutex);
    return 0;
}

void *runner(void *arg){
    int id = (int) arg;
    runnerWaitsStart(&manager, id);
    printf("[RUNNER %d] I'm running as fast as I can\n", id);
    //microPause();
    if(catchFlag(&manager, id)){
        printf("[RUNNER %d] I'm running to the finish line\n", id);
        //microPause();
        if(amISafe(&manager, id))
            printf("[RUNNER %d] I'm safe\n", id);
    } else{
        printf("[RUNNER %d] I'm trying to catch you\n", id);
        //microPause();
        if(gotYou(&manager, id))
            printf("[RUNNER %d] I got you\n", id);
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

    for(i = 0; i < RUNNERS; i++)
        pthread_create(&thread, &threadAttr, runner, (void *) i);

    pthread_create(&thread, &threadAttr, refree, NULL);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}