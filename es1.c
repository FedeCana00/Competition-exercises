#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NOBODY_SEQUENCE 0
#define A_SEQUENCE 1
#define B_SEQUENCE 2
#define C_SEQUENCE 3
#define D_SEQUENCE 4
#define E_SEQUENCE 5
#define D_OR_E_SEQUENCE 6

typedef enum {false, true} Boolean;

// Structure that holds semaphores, counters, etc.
struct manager_t {
    sem_t semA, semB, semC, semD, semE, mutex;
    int countA, countB, countC, countD, countE;

    int activeB;
    int state;
} manager;

void initManager(struct manager_t *manager){
    manager->countA = 0; manager->countB = 0; manager->countC = 0;
    manager->countD = 0; manager->countE = 0; manager->state = 0;
    manager->activeB = 0;
    
    sem_init(&manager->mutex, 0, 1);

    sem_init(&manager->semA, 0, 0);
    sem_init(&manager->semB, 0, 0);
    sem_init(&manager->semC, 0, 0);
    sem_init(&manager->semD, 0, 0);
    sem_init(&manager->semE, 0, 0);
}

void startA(struct manager_t *manager){
    sem_wait(&manager->mutex);
    if (manager->state == NOBODY_SEQUENCE) {
        manager->state = A_SEQUENCE;
        sem_post(&manager->semA);
    } else
        manager->countA++;

    sem_post(&manager->mutex);
    sem_wait(&manager->semA);
}

void endA(struct manager_t *manager){
    sem_wait(&manager->mutex);

    manager->state = B_SEQUENCE;

    // wake up all Bs
    while (manager->countB) {
        manager->countB--;
        manager->activeB++;
        sem_post(&manager->semB);
    }

    sem_post(&manager->mutex);
}

void startB(struct manager_t *manager){
    sem_wait(&manager->mutex);
    // pthread_mutex_lock(&manager->mutex);

    if(manager->state == B_SEQUENCE){
        manager->activeB++;
        sem_post(&manager->semB); // preventive block
        // pthread_cond_signal(&manager->condB, &manager->mutex);
    } else {
        manager->countB++;
    }

    sem_post(&manager->mutex);
    sem_wait(&manager->semB);
}

void wakeUpAorC(struct manager_t *manager){
    if(manager->countA){
        manager->countA--;
        manager->state = A_SEQUENCE;
        sem_post(&manager->semA);
    } else if(manager->countC){
        manager->countC--;
        manager->state = C_SEQUENCE;
        sem_post(&manager->semC);
    } else
        manager->state = NOBODY_SEQUENCE;
}

void endB(struct manager_t *manager){
    sem_wait(&manager->mutex);

    manager->activeB--;

    if(manager->activeB == 0){
        wakeUpAorC(manager);
    }

    sem_post(&manager->mutex);
}

void startC(struct manager_t *manager){
    sem_wait(&manager->mutex);

    if (manager->state == C_SEQUENCE) {
        sem_post(&manager->semC);
    } else
        manager->countC++;

    sem_post(&manager->mutex);
    sem_wait(&manager->semC);
}

void endC(struct manager_t *manager){
    sem_wait(&manager->mutex);

    if(manager->countD){
        manager->countD++;
        manager->state = D_SEQUENCE;
        sem_post(&manager->semD);
    } else if(manager->countE){
        manager->countE++;
        manager->state = E_SEQUENCE;
        sem_post(&manager->semE);
    } else
        manager->state = C_SEQUENCE;

    sem_post(&manager->mutex);
}

void startD(struct manager_t *manager) {
  sem_wait(&manager->mutex);

    if (manager->state == D_OR_E_SEQUENCE) {
        manager->state = D_SEQUENCE;
        sem_post(&manager->semD);
    } else
        manager->countD++;

    sem_post(&manager->mutex);
    sem_wait(&manager->semD);
}

void endD(struct manager_t *manager) {
    sem_wait(&manager->mutex);

    wakeUpAorC(manager);

    sem_post(&manager->mutex);
}

void startE(struct manager_t *manager){
    sem_wait(&manager->mutex);
    if (manager->state == D_OR_E_SEQUENCE) {
        manager->state = E_SEQUENCE;
        sem_post(&manager->semE);
    } else
        manager->countE++;

    sem_post(&manager->mutex);
    sem_wait(&manager->semE);
}

void endE(struct manager_t *manager)
{
  sem_wait(&manager->mutex);

  wakeUpAorC(manager);

  sem_post(&manager->mutex);
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand()% 10 + 1)*1000000;
  nanosleep(&t,NULL);
}

void *A(void *arg) {
  for (;;) {
    startA(&manager);
    putchar(*(char *)arg);
    endA(&manager);
    microPause();
  }
  return 0;
}

void *B(void *arg) {
  for (;;) {
    startB(&manager);
    putchar(*(char *)arg);
    endB(&manager);
    microPause();
  }
  return 0;
}

void *C(void *arg) {
  for (;;) {
    startC(&manager);
    putchar(*(char *)arg);
    endC(&manager);
    microPause();
  }
  return 0;
}

void *D(void *arg) {
  for (;;) {
    startD(&manager);
    putchar(*(char *)arg);
    endD(&manager);
    microPause();
  }
  return 0;
}

void *E(void *arg) {
  for (;;) {
    startE(&manager);
    putchar(*(char *)arg);
    endE(&manager);
    microPause();
  }
  return 0;
}

int main() {
    pthread_attr_t a;
    pthread_t p;


    initManager(&manager);
    // init random numbers
    srand(555);

    pthread_attr_init(&a);

    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, A, (void *)"a");
    pthread_create(&p, &a, A, (void *)"A");

    pthread_create(&p, &a, B, (void *)"B");
    pthread_create(&p, &a, B, (void *)"b");
    pthread_create(&p, &a, B, (void *)"x");

    pthread_create(&p, &a, C, (void *)"C");
    pthread_create(&p, &a, C, (void *)"c");

    pthread_create(&p, &a, D, (void *)"D");
    pthread_create(&p, &a, D, (void *)"d");

    pthread_create(&p, &a, E, (void *)"E");
    pthread_create(&p, &a, E, (void *)"e");

    pthread_attr_destroy(&a);

    // wait ten secons before terminate all threads
    sleep(10);

    return 0;
}