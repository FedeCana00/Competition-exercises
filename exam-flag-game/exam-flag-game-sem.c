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
    sem_t mutex;
    sem_t semWaitRunner, semWaitRefree, semResultRefree;

    int winner, ready, whoCaughtFlag;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->semWaitRunner, 0, 0);
    sem_init(&manager->semWaitRefree, 0, 0);
    sem_init(&manager->semResultRefree, 0, 0);

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
    sem_wait(&manager->semWaitRefree);
}

void refreeStart(struct manager_t *manager){
    sem_post(&manager->semWaitRunner);
    sem_post(&manager->semWaitRunner);
}

int refreeResult(struct manager_t *manager){
    sem_wait(&manager->semResultRefree);
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
    sem_wait(&manager->mutex);

    if(++manager->ready == RUNNERS)
        sem_post(&manager->semWaitRefree);
    printf("[RUNNER %d] I'm ready to start\n", id);
    sem_post(&manager->mutex);
    sem_wait(&manager->semWaitRunner);
}

int gotYou(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);

    if(manager->winner != NONE){
        sem_post(&manager->mutex);
        return 0;
    }

    manager->winner = id;
    sem_post(&manager->semResultRefree);
    printf("[RUNNER %d] Wakey wakey\n", id);
    sem_post(&manager->mutex);
    return 1;
}

int amISafe(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);

    if(manager->winner != NONE){
        sem_post(&manager->mutex);
        return 0;
    }

    manager->winner = id;
    sem_post(&manager->semResultRefree);
    printf("[RUNNER %d] Wakey wakey\n", id);
    sem_post(&manager->mutex);
    return 1;
}

int catchFlag(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);

    if(manager->whoCaughtFlag == NONE){
        manager->whoCaughtFlag = id;
        printf("[RUNNER %d] I've catch the flag!\n", id);
        sem_post(&manager->mutex);
        return 1;
    }
    
    sem_post(&manager->mutex);
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