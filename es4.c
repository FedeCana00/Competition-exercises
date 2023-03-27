#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NO_MOVE -1
#define PAPER 0
#define ROCK 1
#define SCISSOR 2

typedef enum {false, true} Boolean;

char *moves[3] = {"paper", "rock", "scissor"};

struct manager_t {
    sem_t semPlayers, semReferee, mutex;

    int player1Move, player2Move, countMoves;
} manager;

void initManager(struct manager_t *manager){
    sem_init(&manager->semPlayers, 0, 0);
    sem_init(&manager->semReferee, 0, 0);
    sem_init(&manager->mutex, 0, 1);

    // inititialeze empty move
    manager->player1Move = NO_MOVE;
    manager->player2Move = NO_MOVE;

    manager->countMoves = 0;
}

void referee(struct manager_t *manager){
    // wake up all players
    sem_post(&manager->semPlayers);
    sem_post(&manager->semPlayers);

    sem_wait(&manager->semReferee);

    if(manager->player1Move == NO_MOVE || manager->player2Move == NO_MOVE)
        printf("Error occurs during players move!\n");
    else if(manager->player1Move == manager->player2Move)
        printf("The two players drew!\n");
    else if(manager->player1Move == PAPER){
        if(manager->player2Move == ROCK)
            printf("Player 1 won!\n");
        else
            printf("Player 2 won!\n");
    } else if(manager->player1Move == ROCK){
        if(manager->player2Move == PAPER)
            printf("Player 2 won!\n");
        else
            printf("Player 1 won!\n");
    } else if(manager->player1Move == SCISSOR){
        if(manager->player2Move == ROCK)
            printf("Player 2 won!\n");
        else
            printf("Player 1 won!\n");
    }

    printf("Press ENTER to play again...\n");
    getchar();
    // clean player moves
    manager->player1Move = NO_MOVE;
    manager->player2Move = NO_MOVE;
    manager->countMoves = 0;
}

void player1(struct manager_t *manager){
    sem_wait(&manager->semPlayers);

    manager->player1Move = rand() % 3;
    printf("Player1 move is -> %s\n", moves[manager->player1Move]);

    sem_wait(&manager->mutex);
    if(++manager->countMoves == 2)
        sem_post(&manager->semReferee);
    sem_post(&manager->mutex);
}

void player2(struct manager_t *manager){
    sem_wait(&manager->semPlayers);

    manager->player2Move = rand() % 3;
    printf("Player2 move is -> %s\n", moves[manager->player2Move]);

    sem_wait(&manager->mutex);
    if(++manager->countMoves == 2)
        sem_post(&manager->semReferee);
    sem_post(&manager->mutex);
}

void microPause(void) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % 10 + 1) * 1000000;
  nanosleep(&t, NULL);
}

void *exePlayer2(void *args){
    for(;;){
        player2(&manager);
        microPause();
    }
    return 0;
}

void *exePlayer1(void *args){
    for(;;){
        player1(&manager);
        microPause();
    }
    return 0;
}

void *exeReferee(void *args){
    for(;;){
        referee(&manager);
        microPause();
    }
    return 0;
}

int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;

    // init struct
    initManager(&manager);
    // init random numbers
    srand(555);

    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    pthread_attr_init(&threadAttr);

    pthread_create(&thread, &threadAttr, exePlayer1, NULL);
    pthread_create(&thread, &threadAttr, exePlayer2, NULL);
    pthread_create(&thread, &threadAttr, exeReferee, NULL);

    pthread_attr_destroy(&threadAttr);

    sleep(10);

    return 0;
}