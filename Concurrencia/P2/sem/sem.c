#include "sem.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int sem_init(sem_t *s, int value) {
    if (s == NULL || value < 0) {
        return -1;
    }
    if (pthread_mutex_init(&s->mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&s->cond, NULL) != 0) {
        pthread_mutex_destroy(&s->mutex);
        return -1;
    }
    s->count = value;
    return 0;
}

int sem_destroy(sem_t *s) {
    if (s == NULL) {
        return -1;
    }
    if (pthread_mutex_destroy(&s->mutex) != 0 || pthread_cond_destroy(&s->cond) != 0) {
        return -1;
    }
    return 0;
}

int sem_p(sem_t *s) {   //Block the semaphore, same as sem_wait
    if (s == NULL) {
        return -1;
    }
    pthread_mutex_lock(&s->mutex);

    while (s->count == 0) {
        pthread_cond_wait(&s->cond, &s->mutex);
    }
    s->count--;

    pthread_mutex_unlock(&s->mutex);
    return 0;
}

int sem_v(sem_t *s) {   //Free the semaphore, same as sem_post
    if (s == NULL) {
        return -1;
    }
    pthread_mutex_lock(&s->mutex);

    s->count++;
    pthread_cond_signal(&s->cond);

    pthread_mutex_unlock(&s->mutex);
    return 0;
}

int sem_tryp(sem_t *s) { // 0 on sucess, -1 if already locked
    if (s == NULL) {
        return -1;
    }
    pthread_mutex_lock(&s->mutex);

    if (s->count == 0) {
        return -1;
    }
    s->count--;

    pthread_mutex_unlock(&s->mutex);
    return 0;
}
