#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 10
#define DELAY 1000
#define BARBERS 3
#define SOFA_SIT 4

typedef enum {false, true} Boolean;

struct entrance_t {
    sem_t sofa;
    sem_t door;
} entrace;

struct manager_t {
    struct entrance_t entrace;
    sem_t barbers[BARBERS];
    sem_t cashier;
    sem_t mutex;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->entrace.sofa, 0, BARBERS);
    sem_init(&manager->entrace.door, 0, SOFA_SIT);

    for(int i = 0; i < BARBERS; i++)
        sem_init(&manager->barbers[i], 0, 1);

    sem_init(&manager->cashier, 0, 1);
    sem_init(&manager->mutex, 0, 1);
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void enter(struct manager_t *manager, int id){
    printf("[CUSTOMER %d] I'm waiting outside\n", id);
    sem_wait(&manager->entrace.door);
    printf("[CUSTOMER %d] I'm waiting on sofa\n", id);
    sem_wait(&manager->entrace.sofa);
}

void haircut(struct manager_t *manager, int id){
    sem_post(&manager->entrace.door);
    printf("[CUSTOMER %d] I'm getting up\n", id);

    sem_wait(&manager->mutex);
    int i;
    for(i = 0; i < BARBERS; i++){
        int semValue;
        sem_getvalue(&manager->barbers[i], &semValue);

        if(semValue != 0){
            printf("[CUSTOMER %d] I having a haircut -> BARBER %d\n", id, i);
            sem_wait(&manager->barbers[i]);
            break;
        }
    }

    sem_post(&manager->mutex);
    microPause();
    sem_post(&manager->barbers[i]);
    sem_post(&manager->entrace.sofa);
}

void payment(struct manager_t *manager, int id){
    sem_wait(&manager->cashier);
    printf("[CUSTOMER %d] I'm paying\n", id);
    microPause();
    sem_post(&manager->cashier);
    printf("[CUSTOMER %d] I payed\n", id);
}

void *customer(void *arg){
    enter(&manager, (int) arg);
    haircut(&manager, (int) arg);
    payment(&manager, (int) arg);
    microPause();
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
        pthread_create(&thread, &threadAttr, customer, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}