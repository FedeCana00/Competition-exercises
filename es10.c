#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 40
#define DELAY 1000
#define RQ 0
#define RA 1
#define RB 2
#define R2 3

typedef enum {false, true} Boolean;

struct sem_private_t {
    sem_t sem;
    int count;
};

struct manager_t {
    sem_t semA, semB, mutex;
    struct sem_private_t semPriotity[4];
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->semA, 0, 1);
    sem_init(&manager->semB, 0, 1);
    sem_init(&manager->mutex, 0, 1);

    for(int i = 0; i < 4; i++){
        sem_init(&manager->semPriotity[i].sem, 0, 0);
        manager->semPriotity[i].count = 0;
    }
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void request(struct manager_t *manager, int id, int priority){
    printf("[THREAD %d] I'm requesting R%d\n", id, priority);
    manager->semPriotity[priority].count++;
    sem_wait(&manager->semPriotity[priority].sem);
    manager->semPriotity[priority].count--;
}

void getA(struct manager_t *manager, int id){
    sem_wait(&manager->semA);
    printf("[THREAD %d] I'm using resource A\n", id);
    microPause();
    printf("[THREAD %d] I released A\n", id);
    sem_post(&manager->semA);
}

void getB(struct manager_t *manager, int id){
    sem_wait(&manager->semB);
    printf("[THREAD %d] I'm using resource B\n", id);
    microPause();
    printf("[THREAD %d] I released B\n", id);
    sem_post(&manager->semB);
}

void reqQ(struct manager_t *manager, int id){
    request(manager, id, RQ);
    sem_wait(&manager->mutex);

    int v;
    sem_getvalue(&manager->semA, &v);
    if(v) {
        sem_post(&manager->mutex);
        getA(manager, id);
    } else {
        sem_post(&manager->mutex);
        getB(manager, id);
    }
}

void reqA(struct manager_t *manager, int id){
    request(manager, id, RA);

    sem_wait(&manager->mutex);

    getA(manager, id);

    sem_post(&manager->mutex);
}

void reqB(struct manager_t *manager, int id){
    request(manager, id, RB);

    sem_wait(&manager->mutex);

    getB(manager, id);

    sem_post(&manager->mutex);
}
void req2(struct manager_t *manager, int id){
    request(manager, id, R2);

    sem_wait(&manager->mutex);

    getA(manager, id);
    getB(manager, id);

    sem_post(&manager->mutex);
}

void *exeThread(void *arg){
    for(;;){
        int value = rand() % 4;

        switch(value){
            case RQ:
                reqQ(&manager, (int) arg);
                break;
            case RA:
                reqA(&manager, (int) arg);
                break;
            case RB:
                reqB(&manager, (int) arg);
                break;
            case R2:
                req2(&manager, (int) arg);
                break;
            default:
                printf("Something went wrong...\n");
        }
        microPause();
    }
    
    return 0;
}

void handling(struct manager_t *manager){
    sem_wait(&manager->mutex);
    int sa, sb;
    sem_getvalue(&manager->semA, &sa);
    sem_getvalue(&manager->semB, &sb);

    if(manager->semPriotity[RQ].count && (sa || sb)){
        sem_post(&manager->semPriotity[RQ].sem);
    } else if(manager->semPriotity[RA].count && sa){
        sem_post(&manager->semPriotity[RA].sem);
    } else if(manager->semPriotity[RB].count && sb){
        sem_post(&manager->semPriotity[RB].sem);
    } else if(manager->semPriotity[R2].count && sa && sb){
        sem_post(&manager->semPriotity[R2].sem);
    }

    sem_post(&manager->mutex);
}

void *handler(void *arg){
    for(;;)
        handling(&manager);

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

    pthread_create(&thread, &threadAttr, handler, NULL);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, exeThread, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N + 1; i++)
        pthread_join(thread, NULL);

    return 0;
}