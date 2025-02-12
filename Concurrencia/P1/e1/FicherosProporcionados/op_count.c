#include <pthread.h>

/*
*   Uso de pthread_mutex_t en op_count.c:
    Protege la variable count correctamente con un mutex.
    Esto previene condiciones de carrera cuando varios hilos incrementan el contador.
 */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int count = 0;

void inc_count() {
    pthread_mutex_lock(&m);
    count ++;
    pthread_mutex_unlock(&m);
}


int get_count() {
    int cnt;
    pthread_mutex_lock(&m);
    cnt = count;
    pthread_mutex_unlock(&m);

    return cnt;
}