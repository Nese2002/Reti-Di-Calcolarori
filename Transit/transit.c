#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

typedef struct {
    int id;
    int time;
} Train;

int total_trains = 0;
int crossing_trains = 0;
int waiting_trains = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t station;

void stampa() {
    printf("Total trains: %d\nCrossing trains: %d\nWaiting trains: %d\n", total_trains, crossing_trains, waiting_trains);
    printf("\n");
}

void* train_func(void* arg) {
    Train* train = (Train*)arg;

    pthread_mutex_lock(&mutex);
    waiting_trains++;
    stampa();
    pthread_mutex_unlock(&mutex);

    sem_wait(&station);

    pthread_mutex_lock(&mutex);
    waiting_trains--;
    crossing_trains++;
    stampa();
    pthread_mutex_unlock(&mutex);

    usleep(train->time*1000);

    pthread_mutex_lock(&mutex);
    crossing_trains--;
    total_trains++;
    stampa();
    pthread_mutex_unlock(&mutex);

    sem_post(&station);

    free(train);
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 5){
        printf("Inserire i valori per N, T, TMin e TMax\n");
        return -1;
    }
    int N = atoi(argv[1]);
    int T = atoi(argv[2]);
    int TMin = atoi(argv[3]);
    int TMax = atoi(argv[4]);

    sem_init(&station, 0, N);

    while(1) {
        usleep((rand() % (TMax - TMin + 1) + TMin)*1000); 

        Train* train = malloc(sizeof(Train));
        train->id = total_trains + crossing_trains + waiting_trains;
        train->time = T;

        pthread_t thread;
        pthread_create(&thread, NULL, train_func, train);
        pthread_detach(thread); 
    }

    return 0;
}