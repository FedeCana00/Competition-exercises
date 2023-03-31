#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 4
#define DELAY 1000

typedef enum {false, true} Boolean;

struct manager_t {
    sem_t mutex;
    sem_t R1, R2;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->R1, 0, 1);
    sem_init(&manager->R2, 0, 1);
}

void microPause(int time) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % time + 1) * 1000000;
  nanosleep(&t, NULL);
}

void getR(struct manager_t *manager, int T, int id){
    printf("[THREAD %d] I want a resouce\n", id);

    int semValue;
    Boolean getResouce = false;

    for(int i = 0; i < T; i++){
        sem_wait(&manager->mutex);
        sem_getvalue(&manager->R1, &semValue);
        if(semValue != 0){
            sem_wait(&manager->R1);
            sem_post(&manager->mutex);
            printf("[THREAD %d] I'm using R1\n", id);
            microPause(DELAY);
            sem_post(&manager->R1);
            printf("[THREAD %d] I'm release R1\n", id);
            getResouce = true;
            break;
        } else {
            sem_getvalue(&manager->R2, &semValue);

            if(semValue != 0){
                sem_wait(&manager->R2);
                sem_post(&manager->mutex);
                printf("[THREAD %d] I'm using R2\n", id);
                microPause(DELAY);
                sem_post(&manager->R2);
                printf("[THREAD %d] I'm release R2\n", id);
                getResouce = true;
                break;
            }
        }
        sem_post(&manager->mutex);
    }

    if(!getResouce)
        printf("[THREAD %d] I didn't get any resource\n", id);
}

void *exeThread(void *arg){
    int T;
    for(;;){
        T = rand() % 100 + 1;
        getR(&manager, T, (int) arg);
        microPause(DELAY);
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

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, exeThread, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}