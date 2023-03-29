#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

/* for sched_param.c */
#include <sched.h>

#define N 10

typedef enum {false, true} Boolean;

typedef struct {
    char *message;
    char *threadName;
} Envelope;

struct manager_t {
    pthread_mutex_t mutex;
    pthread_cond_t condSend, condReceive;

    Envelope envelopes[N];
    int envIndex; // next empty cell
} manager;

void printStatus(struct manager_t *manager){
    printf("[");
    for(int i = 0; i < N; i++)
        printf("(i = %d, msg = %s, from %s)", i, manager->envelopes[i].message, manager->envelopes[i].threadName);
    printf("]\n");
}

Envelope *getEmptyEnv(struct manager_t *manager){
    if(manager->envIndex < N)
        return &manager->envelopes[manager->envIndex];
    
    return NULL;
}

Envelope getEnv(struct manager_t *manager){
    Envelope env = manager->envelopes[0]; // get firtst item (FIFO)

    // move array to left
    for(int i = 1; i < manager->envIndex; i++)
        manager->envelopes[i - 1] = manager->envelopes[i];

    // clear last envelope
    manager->envelopes[--manager->envIndex].message = "(null)";
    manager->envelopes[manager->envIndex].threadName = "(null)";

    return env; 
}

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    pthread_condattr_init(&condAttr);
    pthread_cond_init(&manager->condSend, &condAttr);
    pthread_cond_init(&manager->condReceive, &condAttr);
    pthread_condattr_destroy(&condAttr);

    manager->envIndex = 0;
}

void receive(struct manager_t *manager, char *threadName){
    pthread_mutex_lock(&manager->mutex);

    while(manager->envIndex <= 0)
        pthread_cond_wait(&manager->condReceive, &manager->mutex);

    printf("Receiver %s started to work - evnIndex = %d\n", threadName, manager->envIndex);

    Envelope env = getEnv(manager);

    printf("Thread %s receive msg = %s from %s\n", threadName, env.message, env.threadName);

    printStatus(manager);

    pthread_cond_signal(&manager->condSend);

    pthread_mutex_unlock(&manager->mutex);
}

void send(struct manager_t *manager, char *threadName){
    pthread_mutex_lock(&manager->mutex);

    while(manager->envIndex >= N)
        pthread_cond_wait(&manager->condSend, &manager->mutex);

    printf("Sender %s started to work\n", threadName);

    Envelope *e = getEmptyEnv(manager);
    e->message = "MEX";
    e->threadName = threadName;

    manager->envIndex++;

    printf("Thread %s send msg = %s\n", threadName, e->message);

    printStatus(manager);

    pthread_cond_signal(&manager->condReceive);

    pthread_mutex_unlock(&manager->mutex);
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % 10 + 1) * 1000000;
  nanosleep(&t, NULL);
}

void *receiver(void *arg){
    for(;;){
        receive(&manager, arg);
        microPause();
    }
    return 0;
}

void *sender(void *arg){
    for(;;){
        send(&manager, arg);
        microPause();
    }
    return 0;
}

int main (int argc, char **argv) {
    pthread_attr_t lowAttr, mediumAttr, highAttr;
    struct sched_param highPolicy, mediumPolicy, lowPolicy;
    pthread_t thread;

    // init struct
    initManager(&manager);
    // init random numbers
    srand(555);

    pthread_attr_setdetachstate(&lowAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&mediumAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&highAttr, PTHREAD_CREATE_DETACHED);

    pthread_attr_init(&lowAttr);
    pthread_attr_setschedpolicy(&lowAttr, SCHED_FIFO);
    lowPolicy.sched_priority = 2;
    pthread_attr_setschedparam(&lowAttr, &lowPolicy);  
    // NTPL defaults to INHERIT_SCHED!!!
    // pthread_attr_setinheritsched(&lowAttr, PTHREAD_EXPLICIT_SCHED);

    pthread_attr_init(&mediumAttr);
    pthread_attr_setschedpolicy(&mediumAttr, SCHED_FIFO);
    mediumPolicy.sched_priority = 1;
    pthread_attr_setschedparam(&mediumAttr, &mediumPolicy);  
    // NTPL defaults to INHERIT_SCHED!!!
    // pthread_attr_setinheritsched(&mediumAttr, PTHREAD_EXPLICIT_SCHED);

    pthread_attr_init(&highAttr);
    pthread_attr_setschedpolicy(&highAttr, SCHED_FIFO);
    highPolicy.sched_priority = 0;
    pthread_attr_setschedparam(&highAttr, &highPolicy);  
    // NTPL defaults to INHERIT_SCHED!!!
    // pthread_attr_setinheritsched(&highAttr, PTHREAD_EXPLICIT_SCHED);

    pthread_create(&thread, &lowAttr, receiver, (void *)"R2");
    pthread_create(&thread, &mediumAttr, receiver, (void *)"R1");
    pthread_create(&thread, &highAttr, receiver, (void *)"R0");

    pthread_create(&thread, &lowAttr, sender, (void *)"S2");
    pthread_create(&thread, &mediumAttr, sender, (void *)"S1");
    pthread_create(&thread, &highAttr, sender, (void *)"S0");

    pthread_attr_destroy(&lowAttr);
    pthread_attr_destroy(&mediumAttr);
    pthread_attr_destroy(&highAttr);

    sleep(10);

    return 0;
}