#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

typedef enum {false, true} Boolean;

struct R {
    int a, b;
} resource;

struct manager_t {
    pthread_mutex_t mutexReset, mutexA, mutexB;
    pthread_cond_t condAB, condReset;

    Boolean isResetWait;
    int countABWaiting;
} manager;

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutexReset, &mutexAttr);
    pthread_mutex_init(&manager->mutexA, &mutexAttr);
    pthread_mutex_init(&manager->mutexB, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    pthread_condattr_init(&condAttr);
    pthread_cond_init(&manager->condAB, &condAttr);
    pthread_cond_init(&manager->condReset, &condAttr);
    pthread_condattr_destroy(&condAttr);

    manager->isResetWait = false;
    manager->countABWaiting = 0;
}

void initR(struct R *resource){
    resource->a = 0;
    resource->b = 0;
}

void procA(struct manager_t *manager, struct R *resource){
    pthread_mutex_lock(&manager->mutexA);

    pthread_mutex_lock(&manager->mutexReset);
    if(manager->isResetWait) {
        manager->countABWaiting++;
        if(manager->countABWaiting == 1)
            pthread_cond_signal(&manager->condReset);
        pthread_mutex_unlock(&manager->mutexReset);
        
        while(manager->isResetWait)
            pthread_cond_wait(&manager->condAB, &manager->mutexA);
    }
    pthread_mutex_unlock(&manager->mutexReset);

    printf("Resource a value = %d\n", ++resource->a);

    pthread_mutex_unlock(&manager->mutexA);
}

void procB(struct manager_t *manager, struct R *resource){
    pthread_mutex_lock(&manager->mutexB);

    pthread_mutex_lock(&manager->mutexReset);
    if(manager->isResetWait){
        manager->countABWaiting++;
        if(manager->countABWaiting == 1)
            pthread_cond_signal(&manager->condReset);

        while(manager->isResetWait)
            pthread_cond_wait(&manager->condAB, &manager->mutexB);
    }
    pthread_mutex_unlock(&manager->mutexReset);

    printf("Resource b value = %d\n", ++resource->b);
    
    pthread_mutex_unlock(&manager->mutexB);
}

void reset(struct manager_t *manager, struct R *resource){
    pthread_mutex_lock(&manager->mutexReset);
    manager->isResetWait = true;
    printf("Thread reset is WAITING!\n");
    while(manager->countABWaiting)
        pthread_cond_wait(&manager->condReset, &manager->mutexReset);

    printf("Resource a value = %d, b value = %d\n", resource->a = 0, resource->b = 0);
    pthread_cond_broadcast(&manager->condAB);
    manager->countABWaiting = 0;
    manager->isResetWait = false;
    pthread_mutex_unlock(&manager->mutexReset);
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % 10 + 1) * 1000000;
  nanosleep(&t, NULL);
}

void *exeProcA(void *arg){
    for(;;){
        procA(&manager, &resource);
        microPause();
    }
    return 0;
}

void *exeProcB(void *arg){
    for(;;){
        procB(&manager, &resource);
        microPause();
    }
    return 0;
}

void *exeReset(void *arg){
    for(;;){
        reset(&manager, &resource);
        microPause();
    }
    return 0;
}

int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;

    // init struct
    initManager(&manager);
    initR(&resource);
    // init random numbers
    srand(555);

    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    pthread_attr_init(&threadAttr);

    pthread_create(&thread, &threadAttr, exeProcA, (void *)"A1");
    pthread_create(&thread, &threadAttr, exeProcA, (void *)"A2");
    pthread_create(&thread, &threadAttr, exeProcA, (void *)"A3");
    pthread_create(&thread, &threadAttr, exeProcB, (void *)"B1");
    pthread_create(&thread, &threadAttr, exeProcB, (void *)"B2");
    pthread_create(&thread, &threadAttr, exeProcB, (void *)"B3");
    pthread_create(&thread, &threadAttr, exeReset, (void *)"R");

    pthread_attr_destroy(&threadAttr);

    sleep(10);

    return 0;
}