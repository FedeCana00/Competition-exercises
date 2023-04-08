#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 10
#define NOT_BLOCKED 0
#define BLOCKED 1
#define DELAY 1000

typedef enum {false, true} Boolean;

struct process_t {
    pthread_cond_t condClient;
    int status;
    float amountWithdraw;
};

struct manager_t {
    pthread_mutex_t mutex;
    struct process_t processes[N];
    float amount;
    int suspendedProcesses;
} manager;

void initManager(struct manager_t *manager){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&manager->mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    pthread_condattr_init(&condAttr);
    for(int i = 0; i < N; i++){
        pthread_cond_init(&manager->processes[i].condClient, &condAttr);
        manager->processes[i].status = NOT_BLOCKED;
        manager->processes[i].amountWithdraw = 0;
    }
    pthread_condattr_destroy(&condAttr);

    manager->amount = 0;
    manager->suspendedProcesses = 0;
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
            printf("%d: %.2f]\n", i, manager->processes[i].amountWithdraw);
        else
            printf("%d: %.2f, ", i, manager->processes[i].amountWithdraw);
}

void suspendProcess(struct manager_t *manager, int p, float amount){
    printf("[THREAD %d] I'm going to suspend (Withdraw = %.2f - Bank account = %.2f\n", p, amount, manager->amount);
    manager->processes[p].status = BLOCKED;
    manager->processes[p].amountWithdraw = amount;
    manager->suspendedProcesses++;
    printStatus(manager);
    pthread_cond_wait(&manager->processes[p].condClient, &manager->mutex);
    printf("[THREAD %d] I waked up\n", p);
    manager->suspendedProcesses--;
    manager->processes[p].status = NOT_BLOCKED;
    manager->processes[p].amountWithdraw = 0;
    printStatus(manager);
}

void withdraw(struct manager_t *manager, int id, float amount){
    pthread_mutex_lock(&manager->mutex);
    if(manager->suspendedProcesses){
        for(int i = 0; i < id; i++)
            if(manager->processes[i].status == BLOCKED && manager->processes[i].amountWithdraw <= manager->amount){
                pthread_cond_signal(&manager->processes[i].condClient);
                suspendProcess(manager, id, amount);
                break;
            }
    } 
    
    if(manager->amount < amount)
        while (manager->amount < amount)
            suspendProcess(manager, id, amount);

    manager->amount = manager->amount - amount;
    printf("[THREAD %d] Withdraw -%.2f - Bank account %.2f\n", id, amount, manager->amount);
    pthread_mutex_unlock(&manager->mutex);
        
}

void deposit(struct manager_t *manager, int id, float amount){
    pthread_mutex_lock(&manager->mutex);
    manager->amount = manager->amount + amount;
    printf("[THREAD %d] Deposit +%.2f - Bank account %.2f\n", id, amount, manager->amount);
    pthread_mutex_unlock(&manager->mutex);
}

void *process(void *arg){
    int id = (int) arg;
    float amount = rand() % 1000 + 1;
    for(;;){
        if(rand() % 2 == 0)
            withdraw(&manager, id, amount);
        else
            deposit(&manager, id, amount);
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