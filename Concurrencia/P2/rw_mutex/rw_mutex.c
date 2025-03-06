#include "rw_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


int rw_mutex_init(rw_mutex_t *m){
    if (m == NULL) {
        return -1;
    }
    if (pthread_mutex_init(&m->m, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&m->readers, NULL) != 0) {
        pthread_mutex_destroy(&m->m);
        return -1;
    }
    if (pthread_cond_init(&m->writers, NULL) != 0) {
        pthread_cond_destroy(&m->readers);
        pthread_mutex_destroy(&m->m);
        return -1;
    }
    m->active_readers = 0;
    m->writing = 0;

    return 0;
}

int rw_mutex_destroy(rw_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    if (pthread_mutex_destroy(&m->m) != 0) {
        return -1;
    }
    if (pthread_cond_destroy(&m->readers) != 0 || pthread_cond_destroy(&m->writers) != 0) {
        return -1;
    }

    return 0;
}

int rw_mutex_readlock(rw_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    pthread_mutex_lock(&m->m);

    // A reader must wait if there is an active writer
    while (m->writing) {
        pthread_cond_wait(&m->readers, &m->m);
    }
    m->active_readers++;

    pthread_mutex_unlock(&m->m);
    return 0;
}

int rw_mutex_writelock(rw_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    pthread_mutex_lock(&m->m);

    // A writer must wait if there are active readers or another active writer
    while (m->writing || m->active_readers > 0) {
        pthread_cond_wait(&m->writers, &m->m);
    }
    m->writing = 1;

    pthread_mutex_unlock(&m->m);

    return 0;
}

int rw_mutex_readunlock(rw_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    pthread_mutex_lock(&m->m);
    m->active_readers--;
    if (m->active_readers == 0) {
        // If no more readers, wake up a writer if waiting
        pthread_cond_signal(&m->writers);
    }
    pthread_mutex_unlock(&m->m);
    return 0;
}

int rw_mutex_writeunlock(rw_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    pthread_mutex_lock(&m->m);
    m->writing = 0;

    // Prioritize waiting writers, but if none, wake up all readers
    pthread_cond_signal(&m->writers);
    pthread_cond_broadcast(&m->readers);

    pthread_mutex_unlock(&m->m);
    return 0;
}
