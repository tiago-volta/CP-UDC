#include "rec_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//We don't use the mutex because only one thread is creating the mutex so only one will access to it until this function comes to an end.
int rec_mutex_init(rec_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    if (pthread_mutex_init(&m->m, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&m->c, NULL) != 0) {
        pthread_mutex_destroy(&m->m);
        return -1;
    }

    m->locker = (pthread_t)0;
    m->count = 0;

    return 0;
}

//PRECD: There is no threads using the mutex at this time
int rec_mutex_destroy(rec_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }
    if (pthread_mutex_destroy(&m->m) != 0) {
        return -1;
    }
    if (pthread_cond_destroy(&m->c) != 0) {
        return -1;
    }

    return 0;
}

int rec_mutex_lock(rec_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }

    pthread_t self = pthread_self();
    pthread_mutex_lock(&m->m);

    if (pthread_equal (m->locker, self)) {  //We compare the thread which got the mutex and the parameter thread, at first m->locker is 0 so is going through the else
        m->count++;
    }else {
        while (m->count > 0) {  //The actual thread waits until its frees the mutex, here is where the real block between threads takes place.
            pthread_cond_wait(&m->c, &m->m);
        }
        m->locker = self;
        m->count = 1;
    }

    pthread_mutex_unlock(&m->m);

    return 0;
};

int rec_mutex_unlock(rec_mutex_t *m) {
    if (m == NULL) {
        return -1;
    }

    pthread_t self = pthread_self();
    pthread_mutex_lock(&m->m);

    if (pthread_equal (m->locker, self)) {
        m->count--;
        if (m->count == 0) {
            m->locker = (pthread_t)0;
            pthread_cond_signal(&m->c);
        }
    }else{
        pthread_mutex_unlock(&m->m);
        return -1;
    }

    pthread_mutex_unlock(&m->m);
    return 0;
};

int rec_mutex_trylock(rec_mutex_t *m) {    // 0 if sucessful, -1 if already locked
    if (m == NULL) {
        return -1;
    }

    pthread_t self = pthread_self();
    int result = 0;

    pthread_mutex_lock(&m->m);

    if (pthread_equal (m->locker, self)) {  //Parameter thread = thread which got the mutex
        m->count++;
    }else if (m->count == 0) {  //If no other thread owns the mutex
        m->locker = self;
        m->count = 1;
    }else {     //Mutex is busy whit other thread
        result = -1;
    }

    pthread_mutex_unlock(&m->m);
    return result;
};
