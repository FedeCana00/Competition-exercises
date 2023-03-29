#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NUM_THREADS 25

typedef enum {false, true} Boolean;

struct manager_t {
    pthread_mutex_t mutex;
    pthread_cond_t condResult;
} manager;

struct URN {
    int vote0, vote1;
};

struct URN urn = {.vote0 = 0, .vote1 = 0};

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    pthread_condattr_init(&condAttr);
    pthread_cond_init(&manager->condResult, &condAttr);
    pthread_condattr_destroy(&condAttr);
}

void toVote(struct manager_t *manager, int vote){
    pthread_mutex_lock(&manager->mutex);
    if(vote == 0)
        urn.vote0++;
    else
        urn.vote1++;

    if(urn.vote0 > NUM_THREADS / 2 || urn.vote1 > NUM_THREADS / 2){
        pthread_cond_broadcast(&manager->condResult);
        printf("Free the threads!\n");
    }
    printf("Vote 0 = %d, vote 1 = %d\n", urn.vote0, urn.vote1);
    pthread_mutex_unlock(&manager->mutex);
}

int result(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);
    printf("Start waiting\n");
    while (urn.vote0 < NUM_THREADS / 2 && urn.vote1 < NUM_THREADS / 2)
        pthread_cond_wait(&manager->condResult, &manager->mutex);
    pthread_mutex_unlock(&manager->mutex);

    return urn.vote0 > NUM_THREADS / 2 ? 0 : 1;
}

void *thread(void *arg) {
    int vote = rand() % 2;
    toVote(&manager, vote);
    if (vote == result(&manager))  
        printf("I WIN!\n");
    else 
        printf("I LOSE!\n");
    pthread_exit(0);
}

int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t th;

    // init struct
    initManager(&manager);
    // init random numbers
    srand(555);

    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    pthread_attr_init(&threadAttr);

    for(int i = 0; i < NUM_THREADS; i++)
        pthread_create(&th, &threadAttr, thread, NULL);

    pthread_attr_destroy(&threadAttr);

    sleep(10);

    return 0;
}