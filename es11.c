#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define DELAY 1000

typedef enum {false, true} Boolean;

struct sem_reporting_t{
    sem_t sem;
    Boolean isBlocked;
};

struct manager_t {
    sem_t semR, mutex;
    struct sem_reporting_t semReporting[N];
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->semR, 0, 1);
    sem_init(&manager->mutex, 0, 1);

    for(int i = 0; i < N; i++){
        sem_init(&manager->semReporting[i].sem, 0, 0);
        manager->semReporting[i].isBlocked = false;
    }
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

/*
*   Wake up the first reporting thread suspended.
*/
void reportA(struct manager_t *manager){
    for(int i = 0; i < N; i++)
        if(manager->semReporting[i].isBlocked){
            sem_post(&manager->semReporting[i].sem);
            break;
        }
}

/*
*   Consume the resource.
*/
void headA(struct manager_t *manager, int id){
    int v;
    sem_wait(&manager->mutex);

    printf("[THREAD %d REPORTED] I'm ready to consume the resource\n", id);
    sem_getvalue(&manager->semR, &v);
    if(v){
        sem_wait(&manager->semR);
        sem_post(&manager->mutex);
    } else {
        sem_post(&manager->mutex);
        sem_wait(&manager->semR);
    }
    printf("[THREAD %d REPORTED] I consumed the resource\n", id);

}

void *exeReported(void *arg){
    for(;;){
        headA(&manager, (int) arg);
        reportA(&manager);
        microPause();
    }
    
    return 0;
}

/*
 * Block reporting thread on his specific semaphore. 
*/
void blockReporting(struct manager_t *manager, int id, int v){
    manager->semReporting[id].isBlocked = true;
    sem_post(&manager->mutex);
    if(v)
        printf("[THREAD %d REPORTING] I'm going to suspend (semR = %d)\n", id, v);
    else
        printf("[THREAD %d REPORTING] I'm going to suspend (semR = %d but other threads are waiting before me)\n", id, v);
    sem_wait(&manager->semReporting[id].sem);
    manager->semReporting[id].isBlocked = false;
    printf("[THREAD %d REPORTING] Waked up\n", id);
    sem_wait(&manager->mutex);
}

void reporting(struct manager_t *manager, int id){
    int v;
    sem_wait(&manager->mutex);

    sem_getvalue(&manager->semR, &v);
    if(v)
        blockReporting(manager, id, v);
    else { // check if there are other threads blocked
        int count = 0;
        for(int i = 0; i < N; i++)
            if(manager->semReporting[i].isBlocked){
                count++;
                break;
            }

        if(count)
            blockReporting(manager, id, v);
    }

    sem_post(&manager->semR);
    printf("[THREAD %d REPORTING] I created a new resource\n", id);

    sem_post(&manager->mutex);
}

void *exeReporting(void *arg){
    for(;;){
        reporting(&manager, (int) arg);
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

    for(i = 0; i < N; i++){
        pthread_create(&thread, &threadAttr, exeReported, (void *) i);
        pthread_create(&thread, &threadAttr, exeReporting, (void *) i);
    }

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N + 1; i++)
        pthread_join(thread, NULL);

    return 0;
}