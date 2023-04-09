#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 10
#define CAKES 10
#define EMPTY 0
#define FULL 1
#define NOT_BLOCKED 0
#define BLOCKED 1
#define DELAY 1000

typedef enum {false, true} Boolean;

struct manager_t {
    sem_t mutex, semClerkPrepare, semClerkSell, semCook, semCustomer;
    int cake[10];
    int nextCakeSlot;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->semClerkPrepare, 0, 0);
    sem_init(&manager->semClerkSell, 0, 0);
    sem_init(&manager->semCook, 0, 1);
    sem_init(&manager->semCustomer, 0, 0);

    for(int i = 0; i < CAKES; i++)
        manager->cake[i] = EMPTY;

    manager->nextCakeSlot = 0;
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void prinfStatus(struct manager_t *manager){
    printf("[");
    for(int i = 0; i < CAKES; i++)
        if(i == CAKES - 1)
            printf("%d: %s]\n", i, manager->cake[i] == EMPTY ? "EMPTY" : "FULL");
        else
            printf("%d: %s, ", i, manager->cake[i] == EMPTY ? "EMPTY" : "FULL");
}

void customer(struct manager_t *manager, int id){
    sem_post(&manager->semClerkSell);
    printf("[CUSTOMER %d] I'm inside the shop\n", id);

    sem_wait(&manager->semCustomer);
    printf("[CUSTOMER %d] I bought the cake\n", id);
}

void *exeCustomer(void *arg){
    int id = (int) arg;
    for(;;){
       customer(&manager, id);
       microPause();
    }

    return 0;
}

void prepare(struct manager_t *manager){
    sem_wait(&manager->semClerkPrepare);
    printf("[CLERK] I'm preparing the cake %d\n", manager->nextCakeSlot - 1);
    microPause();
}

void sell(struct manager_t *manager){
    sem_wait(&manager->semClerkSell);
    sem_wait(&manager->mutex);
    printf("[CLERK] I'm selling the cake %d\n", manager->nextCakeSlot - 1);
    microPause();
    if(manager->nextCakeSlot-- == CAKES){
        printf("[CLERK] I waked up the cook\n");
        sem_post(&manager->semCook);
    }
    manager->cake[manager->nextCakeSlot] = EMPTY;
    prinfStatus(manager);
    sem_post(&manager->semCustomer);
    sem_post(&manager->mutex);
}

void *exeClerk(void *arg){
    for(;;){
       prepare(&manager);
       sell(&manager);
    }

    return 0;
}

void cook(struct manager_t *manager){
    sem_wait(&manager->semCook);

    printf("[COOK] I'm preparing the cake %d\n", manager->nextCakeSlot);
    microPause();

    sem_wait(&manager->mutex);
    printf("[COOK] Cake completed\n");
    manager->cake[manager->nextCakeSlot++] = FULL;
    prinfStatus(manager);
    sem_post(&manager->semClerkPrepare);
    sem_post(&manager->mutex);

    sem_post(&manager->semCook);

    sem_wait(&manager->mutex);
    if(manager->nextCakeSlot == CAKES){
        printf("[COOK] I'm waiting to free cake slots\n");
        sem_wait(&manager->semCook);
    }
    sem_post(&manager->mutex);
}

void *exeCook(void *arg){
    for(;;){
        cook(&manager);
        // microPause();
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

    pthread_create(&thread, &threadAttr, exeCook, NULL);
    pthread_create(&thread, &threadAttr, exeClerk, NULL);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, exeCustomer, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N + 2; i++)
        pthread_join(thread, NULL);

    return 0;
}