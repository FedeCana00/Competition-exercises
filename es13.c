#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define EMPTY -1
#define DELAY 1000

typedef enum {false, true} Boolean;

typedef struct mes{
    int value;
    Boolean receiver1Read;
    Boolean receiver2Read;
};

struct manager_t {
    sem_t mutex, empty1, empty2, full, semR2;
    struct mes buffer[N];
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->empty1, 0, 0);
    sem_init(&manager->empty2, 0, 0);
    sem_init(&manager->full, 0, N);
    sem_init(&manager->semR2, 0, 0);

    for(int i = 0; i < N; i++){
        manager->buffer[i].value = EMPTY;
        manager->buffer[i].receiver1Read = false;
        manager->buffer[i].receiver2Read = false;
    }
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void printStatus(struct manager_t *manager){
    printf("[");
    for(int i = 0; i < N; i++)
        if(i == N - 1)
            printf("%d]\n", manager->buffer[i].value);
        else
            printf("%d, ", manager->buffer[i].value);
}

void removeResource(struct manager_t *manager, int position){
    int i;
    for(i = position; i < N - 1; i++){
        manager->buffer[i].value = manager->buffer[i + 1].value;
        manager->buffer[i].receiver1Read = manager->buffer[i + 1].receiver1Read;
        manager->buffer[i].receiver2Read = manager->buffer[i + 1].receiver2Read;
    }

    manager->buffer[i].value = EMPTY;
    manager->buffer[i].receiver1Read = false;
    manager->buffer[i].receiver2Read = false;

    sem_post(&manager->full);
}

void receive1(struct manager_t *manager){
    sem_wait(&manager->empty1);
    sem_wait(&manager->mutex);

    int msg = EMPTY;
    for(int i = 0; i < N; i++)
        if(manager->buffer[i].value != EMPTY && !manager->buffer[i].receiver1Read){
            msg = manager->buffer[i].value;
            if(manager->buffer[i].receiver2Read)
                removeResource(manager, i);
            else
                manager->buffer[i].receiver1Read = true;
            break;
        }

    printf("[RECEIVER 1] I've read = %d\n", msg);
    printStatus(manager);

    sem_post(&manager->semR2); // say to receiver to read the message
    sem_post(&manager->mutex);
}

void receive2(struct manager_t *manager){
    sem_wait(&manager->empty2);
    sem_wait(&manager->semR2);
    sem_wait(&manager->mutex);

    int msg = EMPTY;
    int i;
    for(i = 0; i < N; i++)
        if(manager->buffer[i].value != EMPTY && !manager->buffer[i].receiver2Read){
            msg = manager->buffer[i].value;
            if(manager->buffer[i].receiver1Read)
                removeResource(manager, i);
            else
                manager->buffer[i].receiver2Read = true;
            break;
        }

    printf("[RECEIVER 2] I've read = %d\n", msg);
    printStatus(manager);

    sem_post(&manager->mutex);
}

void send(struct manager_t *manager, int id){
    sem_wait(&manager->full);
    printf("[SENDER %d] I'm ready to send a new message\n", id);
    sem_wait(&manager->mutex);

    int msg = rand() % 10000;
    for(int i = 0; i < N; i++)
        if(manager->buffer[i].value == EMPTY){
            manager->buffer[i].value = msg;
            printf("[SENDER %d] I've send msg = %d\n", id, msg);
            printStatus(manager);
            sem_post(&manager->empty2);
            sem_post(&manager->empty1);
            break;
        }

    sem_post(&manager->mutex);
}

void *exeReceiver1(void *arg){
    for(;;){
        receive1(&manager);
        microPause();
    }

    return 0;
}

void *exeReceiver2(void *arg){
    for(;;){
        receive2(&manager);
        microPause();
    }

    return 0;
}

void *sender(void *arg){
    for(;;){
        send(&manager, (int) arg);
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

    pthread_create(&thread, &threadAttr, exeReceiver1, NULL);
    pthread_create(&thread, &threadAttr, exeReceiver2, NULL);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, sender, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N + 2; i++)
        pthread_join(thread, NULL);

    return 0;
}