#ifndef __RW_MUTEX_H__
#define __RW_MUTEX_H__

#include <pthread.h>

typedef struct rw_mutex_t {
    pthread_mutex_t m;      // Protect the access to the internal variables
    pthread_cond_t readers; // Condition to wake up readers
    pthread_cond_t writers; // Condition to wake up writers
    int active_readers;     // Number of active readers
    int writing;     // If there is active writers (0/1)
} rw_mutex_t;

int rw_mutex_init(rw_mutex_t *m);
int rw_mutex_destroy(rw_mutex_t *m);

int rw_mutex_readlock(rw_mutex_t *m);
int rw_mutex_writelock(rw_mutex_t *m);
int rw_mutex_readunlock(rw_mutex_t *m);
int rw_mutex_writeunlock(rw_mutex_t *m);

#endif