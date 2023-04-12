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
    pthread_mutex_t mutex;
    pthread_cond_t condClerkPrepare, condClerkSell, condCook, condCustomer;
    int cake[10];
    int nextCakeSlot, customerInside;
} manager;

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    pthread_condattr_init(&condAttr);
    pthread_cond_init(&manager->condClerkPrepare, &condAttr);
    pthread_cond_init(&manager->condClerkSell, &condAttr);
    pthread_cond_init(&manager->condCook, &condAttr);
    pthread_cond_init(&manager->condCustomer, &condAttr);
    pthread_condattr_destroy(&condAttr);

    for(int i = 0; i < CAKES; i++)
        manager->cake[i] = EMPTY;

    manager->nextCakeSlot = 0;
    manager->customerInside = 0;
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
    pthread_mutex_lock(&manager->mutex);
    manager->customerInside++;
    pthread_cond_signal(&manager->condClerkSell);
    printf("[CUSTOMER %d] I'm inside the shop\n", id);

    pthread_cond_wait(&manager->condCustomer, &manager->mutex);
    printf("[CUSTOMER %d] I bought the cake\n", id);
    manager->customerInside--;
    pthread_mutex_unlock(&manager->mutex);
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
    pthread_mutex_lock(&manager->mutex);
    while(manager->nextCakeSlot - 1 == 0)
        pthread_cond_wait(&manager->condClerkPrepare, &manager->mutex);
    printf("[CLERK] I'm preparing the cake %d\n", manager->nextCakeSlot - 1);
    pthread_mutex_unlock(&manager->mutex);

    microPause();
}

void sell(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);
    while(!manager->customerInside)
        pthread_cond_wait(&manager->condClerkSell, &manager->mutex);
    printf("[CLERK] I'm selling the cake %d\n", manager->nextCakeSlot - 1);
    microPause();
    if(manager->nextCakeSlot-- == CAKES){
        printf("[CLERK] I waked up the cook\n");
        pthread_cond_signal(&manager->condCook);
    }
    manager->cake[manager->nextCakeSlot] = EMPTY;
    prinfStatus(manager);
    pthread_cond_signal(&manager->condCustomer);
    pthread_mutex_unlock(&manager->mutex);
}

void *exeClerk(void *arg){
    for(;;){
       prepare(&manager);
       sell(&manager);
    }

    return 0;
}

void cook(struct manager_t *manager){
    pthread_mutex_lock(&manager->mutex);
    while(manager->nextCakeSlot == CAKES){
        printf("[COOK] I'm waiting to free cake slots\n");
        pthread_cond_wait(&manager->condCook, &manager->mutex);
    }
    pthread_mutex_unlock(&manager->mutex);

    printf("[COOK] I'm preparing the cake %d\n", manager->nextCakeSlot);
    microPause();

    pthread_mutex_lock(&manager->mutex);
    printf("[COOK] Cake completed\n");
    manager->cake[manager->nextCakeSlot++] = FULL;
    prinfStatus(manager);
    pthread_cond_signal(&manager->condClerkPrepare);
    pthread_mutex_unlock(&manager->mutex);
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