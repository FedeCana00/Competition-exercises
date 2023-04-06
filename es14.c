#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 10
#define NOT_EXE 0
#define BLOCKED 1
#define EXE 2
#define DELAY 1000

typedef enum {false, true} Boolean;

struct process_t {
    sem_t sem;
    int statusA, statusB;
};

struct manager_t {
    sem_t semA, semB, semCD;
    sem_t mutex;
    sem_t semEnd;
    struct process_t processes[N];

    int countEnd, activeC, activeD, blockedC, blockedD;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->mutex, 0, 1);
    sem_init(&manager->semA, 0, 1);
    sem_init(&manager->semB, 0, 1);
    sem_init(&manager->semCD, 0, 1);
    sem_init(&manager->semEnd, 0, 0);

    for(int i = 0; i < N; i++){
        sem_init(&manager->processes[i].sem, 0, 0);
        manager->processes[i].statusA = NOT_EXE;
        manager->processes[i].statusB = NOT_EXE;
    }

    manager->countEnd = 0;
    manager->activeC = 0;
    manager->activeD = 0;
    manager->blockedC = 0;
    manager->blockedD = 0;
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
        if(i + 1 == N)
            printf("%d: A = %d, B = %d]\n", i, manager->processes[i].statusA, manager->processes[i].statusB);
        else
            printf("%d: A = %d, B = %d; ", i, manager->processes[i].statusA, manager->processes[i].statusB);
}

void A(struct manager_t *manager, int id){
    int v;

    sem_wait(&manager->mutex);
    sem_getvalue(&manager->semA, &v);
    if(!v){
        manager->processes[id].statusA = BLOCKED;
        sem_post(&manager->mutex);
        sem_wait(&manager->processes[id].sem);
    } else
        sem_post(&manager->mutex);

    sem_wait(&manager->semA);
    manager->processes[id].statusA = EXE;
    printf("[THREAD %d] I'm running A\n", id);
    microPause();
    printf("[THREAD %d] I've finished running A\n", id);

    for(int i = 0; i < N; i++)
        if(manager->processes[i].statusA == BLOCKED){
            sem_post(&manager->processes[i].sem);
            printf("[THREAD %d] I waked up %d in A\n", id, i);
            break;
        }

    sem_post(&manager->semA);
}

void B(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);
    // printStatus(manager);
    if(manager->processes[id - 1].statusB != EXE && id != 0){
        printf("[THREAD %d] I'm waiting in B\n", id);
        manager->processes[id].statusB = BLOCKED;
        sem_post(&manager->mutex);
        sem_wait(&manager->processes[id].sem);
    } else
        sem_post(&manager->mutex);

    sem_wait(&manager->semB);
    manager->processes[id].statusB = EXE;
    printf("[THREAD %d] I'm running B\n", id);
    microPause();
    printf("[THREAD %d] I've finished running B\n", id);

    if(manager->processes[id + 1].statusB == BLOCKED && id + 1 < N){
        sem_post(&manager->processes[id + 1].sem);
        printf("[THREAD %d] I waked up %d in B\n", id, id + 1);
    }
    // printStatus(manager);
    sem_post(&manager->semB);
}

void C(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);

    if(manager->activeC == 0 && manager->activeD == 0){
        manager->activeC++;
        sem_wait(&manager->semCD);
    } else if(manager->activeD){
        printf("[THREAD %d] I'm waiting to running C\n", id);
        manager->blockedC++;
        sem_post(&manager->mutex);

        sem_wait(&manager->semCD);

        sem_wait(&manager->mutex);
        manager->activeC++;
        manager->blockedC--;
    } else if(manager->activeC)
        manager->activeC++;

    sem_post(&manager->mutex);

    printf("[THREAD %d] I'm running C\n", id);
    microPause();
    printf("[THREAD %d] I've finished running C\n", id);

    sem_wait(&manager->mutex);
    if(--manager->activeC == 0 && manager->activeD)
        for(int i = 0; i < manager->blockedD; i++)
            sem_post(&manager->semCD);
    else if(manager->activeC == 0)
        sem_post(&manager->semCD);
    sem_post(&manager->mutex);
}

void D(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);

    if(manager->activeC == 0 && manager->activeD == 0){
        manager->activeD++;
        sem_wait(&manager->semCD);
    } else if(manager->activeC){
        printf("[THREAD %d] I'm waiting to running D\n", id);
        manager->blockedD++;
        sem_post(&manager->mutex);

        sem_wait(&manager->semCD);

        sem_wait(&manager->mutex);
        manager->activeD++;
        manager->blockedD--;
    } else if(manager->activeD)
        manager->activeD++;

    sem_post(&manager->mutex);

    printf("[THREAD %d] I'm running D\n", id);
    microPause();
    printf("[THREAD %d] I've finished running D\n", id);

    sem_wait(&manager->mutex);
    if(--manager->activeD == 0 && manager->activeC)
        for(int i = 0; i < manager->blockedC; i++)
            sem_post(&manager->semCD);
    else if(manager->activeD == 0)
        sem_post(&manager->semCD);
    sem_post(&manager->mutex);
}

void endCycle(struct manager_t *manager, int id){
    sem_wait(&manager->mutex);
    if(++manager->countEnd == N){
        for(int i = 0; i < N; i++)
            sem_post(&manager->semEnd);

        // reset before next cycle of execution
        manager->countEnd = 0;
    }
    sem_post(&manager->mutex);

    printf("[THREAD %d] I'm blocked at the end\n", id);
    sem_wait(&manager->semEnd);
    printf("[THREAD %d] I'm restarting at the end\n", id);
}

void *process(void *arg){
    int id = (int) arg;
    for(;;){
        printf("[THREAD %d] I'm here\n", id);
        A(&manager, id);
        B(&manager, id);
        if(rand() % 2 == 0)
            C(&manager, id);
        else
            D(&manager, id);
        endCycle(&manager, id);
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
        pthread_create(&thread, &threadAttr, process, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}