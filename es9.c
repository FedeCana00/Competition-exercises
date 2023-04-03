#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define INT_NULL -1
#define DELAY 1000
#define A 'A'
#define B 'B'
#define NO_CLASS 'N'

typedef enum {false, true} Boolean;
typedef int T;

struct manager_t {
    sem_t semR[N];
    sem_t semReceiver;
    sem_t mutex;
    sem_t semA, semB;
    T buffer[N];
    int count;
    Boolean isFirst;
    char whoIsWriting;
} manager;

void initManager(struct manager_t *manager){
    for(int i = 0; i < N; i++){
        sem_init(&manager->semR[i], 0 , 1);
        manager->buffer[i] = INT_NULL;
    }

    sem_init(&manager->semReceiver, 0, 0);
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->semA, 0, 1);
    sem_init(&manager->semB, 0, 1);

    manager->count = 0;
    manager->isFirst = true;
    manager->whoIsWriting = NO_CLASS;
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void entranceA(struct manager_t *manager, int id){
    sem_wait(&manager->semA);

    if(!manager->isFirst && manager->whoIsWriting == B)
        sem_wait(&manager->semA);

    printf("[THREAD %dA] I'm waiting the MUTEX\n", id);
    sem_wait(&manager->mutex);
    printf("[THREAD %dA] I HAVE the MUTEX\n", id);

    if(manager->isFirst)
        manager->whoIsWriting = A;
    manager->isFirst = false;

    sem_post(&manager->mutex);
    sem_post(&manager->semA);
}

void entranceB(struct manager_t *manager, int id){
    sem_wait(&manager->semB);

    if(!manager->isFirst && manager->whoIsWriting == A)
            sem_wait(&manager->semB);

    printf("[THREAD %dB] I'm waiting the MUTEX\n", id);
    sem_wait(&manager->mutex);
    printf("[THREAD %dB] I HAVE the MUTEX\n", id);

    if(manager->isFirst)
        manager->whoIsWriting = B;
    manager->isFirst = false;

    sem_post(&manager->mutex);
    sem_post(&manager->semB);
}

void send(struct manager_t *manager, int id, char class){
    int v;
    sem_getvalue(&manager->semR[id], &v);
    if(v == 0){
        printf("[THREAD %d%c] I've already WRITE\n", id, class);
        return;
    }

    printf("[THREAD %d%c] I'm waiting the resource\n", id, class);
    sem_wait(&manager->semR[id]);

    T msg = rand() % 10000;
    manager->buffer[id] = msg;

    printf("[THREAD %d%c] I've inserted %d\n", id, class, msg);

    sem_wait(&manager->mutex);
    manager->count++;
    sem_post(&manager->mutex);

    if(manager->count == N){
        printf("[THREAD %d%c] I'm waking up the receiver %d\n", id, class);
        sem_post(&manager->semReceiver);
    }

    printf("[THREAD %d%c] I'm waiting to restart %d\n", id, class);
}

void receive(struct manager_t *manager){
    sem_wait(&manager->semReceiver);

    printf("[RECEIVER] I read [");
    for(int i = 0; i < N; i++){
        printf("%d", manager->buffer[i]);
        if(i != N - 1)
            printf(", ");
        else
            printf("]\n");

        manager->buffer[i] = INT_NULL;
    }

    manager->count = 0;
    manager->isFirst = true;
    manager->whoIsWriting = NO_CLASS;

    for(int i = 0; i < N; i++)
        sem_post(&manager->semR[i]);

    int v;
    sem_getvalue(&manager->semA, &v);
    if(v == 0)
        sem_post(&manager->semA);
    sem_getvalue(&manager->semB, &v);
    if(v == 0)
        sem_post(&manager->semB);
}

void *senderA(void *arg){
    for(;;){
        entranceA(&manager, (int) arg);
        send(&manager, (int) arg, 'A');
        microPause();
    }
    return 0;
}

void *senderB(void *arg){
    for(;;){
        entranceB(&manager, (int) arg);
        send(&manager, (int) arg, 'B');
        microPause();
    }
    return 0;
}

void *receiver(void *arg){
    for(;;)
        receive(&manager);
    
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
        pthread_create(&thread, &threadAttr, senderA, (void *) i);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, senderB, (void *) i);

    pthread_create(&thread, &threadAttr, receiver, NULL);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < (N * 2) + 1; i++)
        pthread_join(thread, NULL);

    return 0;
}