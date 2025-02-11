
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "op_count.h"
#include "options.h"


struct buffer {
    int *data;
    int size;
    pthread_mutex_t *mutexes; //se cambia a dos mutexes
};

struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

struct args {
    int				thread_num;       // application defined thread #
    int				delay;			  // delay between operations
    int				iterations;
    struct buffer	*buffer;		  // Shared buffer
};

struct print_args {
    struct buffer *buffer;
    int delay;
    volatile int * running; // Controla la ejecución del hilo de impresión
    pthread_mutex_t *running_mutex; // Mutex para proteger `running`
};

void *swap(void *ptr)
{
    struct args *args =  ptr;
    srand(time(NULL) ^ pthread_self()); // Seed para evitar valores idénticos en hilos distintos

    while(args->iterations>0) {
        int i,j, tmp;
        i=rand() % args->buffer->size;
        j=rand() % args->buffer->size;

        if (i==j) continue; //no damos swap si son iguales

        // Asegurar un orden en los bloqueos para evitar interbloqueos, evitando la espera circular
        if (i > j) {
            pthread_mutex_lock(&args->buffer->mutexes[j]);
            pthread_mutex_lock(&args->buffer->mutexes[i]);
        } else {
            pthread_mutex_lock(&args->buffer->mutexes[i]);
            pthread_mutex_lock(&args->buffer->mutexes[j]);
        }

        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
            args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

        tmp = args->buffer->data[i];
        if(args->delay) usleep(args->delay); // Force a context switch

        args->buffer->data[i] = args->buffer->data[j];
        if(args->delay) usleep(args->delay);

        args->buffer->data[j] = tmp;
        if(args->delay) usleep(args->delay);

        pthread_mutex_unlock(&args->buffer->mutexes[j]); //desbloqueamos los mutexes en orden inverso
        pthread_mutex_unlock(&args->buffer->mutexes[i]);

        args->iterations--; // Reducimos las iteraciones SOLO si hubo swap

        inc_count();


    }
    return NULL;
}

int cmp(int *e1, int *e2) {
    if(*e1==*e2) return 0;
    if(*e1<*e2) return -1;
    return 1;
}

void print_buffer(struct buffer buffer) {
    int i;

    for (i = 0; i < buffer.size; i++)
        printf("%i ", buffer.data[i]);
    printf("\n");
}

void *print_periodically(void *ptr) {
    struct print_args *args = (struct print_args *)ptr;

    while (1) {
        sleep(args->delay);

        pthread_mutex_lock(args->running_mutex);
        if (!*(args->running)) {  // Revisamos si debemos detenernos
            pthread_mutex_unlock(args->running_mutex);
            break;
        }
        pthread_mutex_unlock(args->running_mutex);

        // Imprimir contenido del buffer sin bloquear todo el array
        printf("buffer content: ");
        for (int i = 0; i < args->buffer->size; i++) {
            pthread_mutex_lock(&args->buffer->mutexes[i]);
            printf("%d ", args->buffer->data[i]);
            pthread_mutex_unlock(&args->buffer->mutexes[i]);
        }
        printf("\n");
    }
    return NULL;
}

//para modificar el valor de running
void stop_printing_thread(volatile int *running, pthread_mutex_t *running_mutex) {
    pthread_mutex_lock(running_mutex);
    *running = 0;
    pthread_mutex_unlock(running_mutex);
}

void start_threads(struct options opt)
{
    int i;
    struct thread_info *threads;;
    struct print_args p_args;
    struct args *args;
    struct buffer buffer;
    pthread_t print_thread;
    volatile int running = 1;
    pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

    srand(time(NULL));

    if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;

    //ahora con dos mutexes
    buffer.mutexes = malloc(opt.buffer_size * sizeof(pthread_mutex_t));
    if (buffer.mutexes == NULL) {
        printf("Not enough memory\n");
        free(buffer.data);
        exit(1);
    }

    for (i = 0; i < buffer.size; i++) {
        buffer.data[i] = i;
        pthread_mutex_init(&buffer.mutexes[i], NULL); // Initialize mutexes
    }

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads);
    args = malloc(sizeof(struct args) * opt.num_threads);

    if (threads == NULL || args==NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    printf("Buffer before: ");
    print_buffer(buffer);

    //Thread que imprime periodicamente el contenido del array
    p_args.buffer = &buffer;
    p_args.delay = opt.delay;
    p_args.running = &running;
    p_args.running_mutex = &running_mutex;
    if (pthread_create(&print_thread, NULL, print_periodically, &p_args) != 0) {
        printf("Could not create print thread\n");
        exit(1);
    }

    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].thread_num = i;
        args[i].thread_num = i;
        args[i].buffer     = &buffer;
        args[i].delay      = opt.delay;
        args[i].iterations = opt.iterations;

        if ( 0 != pthread_create(&threads[i].thread_id, NULL,
                     swap, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    // Detener hilo de impresión y esperar a que finalice
    stop_printing_thread(&running, &running_mutex);
    pthread_join(print_thread, NULL);

    // Print the buffer
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    printf("iterations: %d\n", get_count());

    //destruimos los mutexes
    for (i = 0; i < buffer.size; i++)
        pthread_mutex_destroy(&buffer.mutexes[i]);

    free(args);
    free(threads);
    free(buffer.data);
    free(buffer.mutexes);

    pthread_exit(NULL);
}

int main (int argc, char **argv)
{
    struct options opt;

    // Default values for the options
    opt.num_threads = 10;
    opt.buffer_size = 10;
    opt.iterations  = 10;
    opt.delay       = 10;

    read_options(argc, argv, &opt);

    start_threads(opt);

    exit (0);
}