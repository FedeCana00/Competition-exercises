#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 20
#define R_SIZE 10
#define NULL_TIMEOUT -1
#define DELAY 1000

typedef enum {false, true} Boolean;

struct R {
    sem_t sem;
    Boolean isAllocated;
};

struct manager_t {
    sem_t mutex;
    sem_t semClient[N];
    int clientTimeout[N];
    struct R resources[R_SIZE];
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);

    int i;
    for(i = 0; i < N; i++){
        sem_init(&manager->semClient[i], 0, 0);
        manager->clientTimeout[i] = NULL_TIMEOUT;
    }

    for(i = 1; i < R_SIZE; i++){
        manager->resources[i].isAllocated = false;
        sem_init(&manager->resources[i].sem, 0, 1);
    }
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

int checkResources(struct manager_t *manager){
    for(int i = 1; i < R_SIZE; i++)
        if(!manager->resources[i].isAllocated){
            manager->resources[i].isAllocated = true;
            sem_wait(&manager->resources[i].sem);
            return i;
        }

    return 0;
}

void printStatus(struct manager_t *manager){
    printf("RES: [");
    for(int i = 1; i < R_SIZE; i++)
        if(i == R_SIZE - 1)
            printf("%d]\n", manager->resources[i].isAllocated);
        else
            printf("%d, ", manager->resources[i].isAllocated);
}

int request(struct manager_t *manager, int t, int p){
    sem_wait(&manager->mutex);

    int r = checkResources(manager);
    if(r){
        printf("[THREAD %d] I've the resource %d\n", p, r);
        printStatus(manager);
        sem_post(&manager->mutex);
        return r;
    }

    manager->clientTimeout[p] = t;
    printf("[THREAD %d] I'm waiting the resource\n", p);
    sem_post(&manager->mutex);
    sem_wait(&manager->semClient[p]);

    sem_wait(&manager->mutex);
    r =checkResources(manager);

    if(r == 0)
        printf("[THREAD %d] My timeout is expired\n", p);
    else
        printf("[THREAD %d] I've the resource %d\n", p, r);
    printStatus(manager);

    sem_post(&manager->mutex);
    return r;
}

void release(struct manager_t *manager, int id, int indexResource){
    sem_wait(&manager->mutex);
    
    sem_post(&manager->resources[indexResource].sem);
    manager->resources[indexResource].isAllocated = false;

    for(int i = 0; i < N; i++)
        if(manager->clientTimeout[i] != NULL_TIMEOUT){
            sem_post(&manager->semClient[i]);
            manager->clientTimeout[i] = NULL_TIMEOUT;
            break;
        }

    printf("[THREAD %d] I've relased the resource %d\n", id, indexResource);
    printStatus(manager);

    sem_post(&manager->mutex);
}

void tick(struct manager_t *manager){
    sem_wait(&manager->mutex);
    for(int i = 0; i < N; i++)
        if(manager->clientTimeout[i] != NULL_TIMEOUT){
            if(manager->clientTimeout[i] == 0){
                manager->clientTimeout[i] = NULL_TIMEOUT;
                sem_post(&manager->semClient[i]);
            } else
                manager->clientTimeout[i]--;
        }
    sem_post(&manager->mutex);

    microPause();
}

void *client(void *arg){
    int id = (int) arg;
    for(;;){
        microPause();
        int indexResource = request(&manager, rand() % 10 + 1, id);
        if(indexResource){
            microPause();
            release(&manager, id, indexResource);
        }
        microPause();
    }

    return 0;
}

void *exeClock(void *arg){
    for(;;)
        tick(&manager);

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

    pthread_create(&thread, &threadAttr, exeClock, NULL);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, client, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N + 1; i++)
        pthread_join(thread, NULL);

    return 0;
}