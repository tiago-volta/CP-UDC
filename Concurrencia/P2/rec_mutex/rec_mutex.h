#ifndef __REC_MUTEX_H__
#define __REC_MUTEX_H__

#include <pthread.h>

typedef struct rec_mutex_t {
    pthread_mutex_t m;  //The main mutex, it's not the recursive mutex, it's main goal is to protect the locker and count variables
    pthread_cond_t c;   //Condition for the mutex
    pthread_t locker;   //Which thread got the mutex
    int count;  //How many locks the mutex have
} rec_mutex_t;

int rec_mutex_init(rec_mutex_t *m);
int rec_mutex_destroy(rec_mutex_t *m);

int rec_mutex_lock(rec_mutex_t *m);
int rec_mutex_unlock(rec_mutex_t *m);
int rec_mutex_trylock(rec_mutex_t *m); // 0 if sucessful, -1 if already locked

#endif