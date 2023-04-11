#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 20
#define NOBODY -1
#define MALE 0
#define FEMALE 1
#define MAX_INSIDE 5
#define DELAY 1000

typedef enum {false, true} Boolean;

struct manager_t {
    sem_t mutex;
    sem_t turnstile, semWait;

    int countInside, genderInside, genederWaiting;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->turnstile, 0, 1);
    sem_init(&manager->semWait, 0, 0);

    manager->countInside = 0;
    manager->genderInside = NOBODY;
    manager->genederWaiting = NOBODY;
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % DELAY + 1) * 1000000;
  nanosleep(&t, NULL);
}

void male(struct manager_t *manager, int id){
    sem_wait(&manager->turnstile);

    if(manager->genderInside == NOBODY){
        manager->genderInside = MALE;
    } else if(manager->genderInside == FEMALE || manager->countInside == MAX_INSIDE){
        manager->genederWaiting = MALE;
        printf("[MALE %d] I'm waiting to enter\n", id);
        sem_wait(&manager->semWait);
        manager->genderInside = MALE;
        manager->genederWaiting = NOBODY;
    }

    sem_wait(&manager->mutex);
    manager->countInside++;
    sem_post(&manager->mutex);

    sem_post(&manager->turnstile);

    printf("[MALE %d] I'm inside\n", id);
    microPause();

    printf("[MALE %d] I'm exiting\n", id);

    sem_wait(&manager->mutex);
    if(manager->countInside-- == MAX_INSIDE && manager->genederWaiting == MALE)
        sem_post(&manager->semWait);
    else if(manager->countInside == 0)
        sem_post(&manager->semWait);
    sem_post(&manager->mutex);
}

void female(struct manager_t *manager, int id){
    sem_wait(&manager->turnstile);

    if(manager->genderInside == NOBODY){
        manager->genderInside = FEMALE;
    } else if(manager->genderInside == MALE || manager->countInside == MAX_INSIDE){
        manager->genederWaiting = FEMALE;
        printf("[FEMALE %d] I'm waiting to enter\n", id);
        sem_wait(&manager->semWait);
        manager->genderInside = FEMALE;
        manager->genederWaiting = NOBODY;
    }

    sem_wait(&manager->mutex);
    manager->countInside++;
    sem_post(&manager->mutex);

    sem_post(&manager->turnstile);

    printf("[FEMALE %d] I'm inside\n", id);
    microPause();

    printf("[FEMALE %d] I'm exiting\n", id);

    sem_wait(&manager->mutex);
    if(manager->countInside-- == MAX_INSIDE && manager->genederWaiting == FEMALE)
        sem_post(&manager->semWait);
    else if(manager->countInside == 0)
        sem_post(&manager->semWait);
    sem_post(&manager->mutex);
}

void *exeMale(void *arg){
    int id = (int) arg;
    for(;;){
        male(&manager, id);
        microPause();
    }

    return 0;
}


void *exeFemale(void *arg){
    int id = (int) arg;
    for(;;){
       female(&manager, id);
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

    for(i = 0; i < N; i++)
        if(rand() % 2)
            pthread_create(&thread, &threadAttr, exeMale, (void *) i);
        else
            pthread_create(&thread, &threadAttr, exeFemale, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}