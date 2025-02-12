#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "op_count.h"
#include "options.h"

/*
* swap.c:
    Define una estructura buffer que almacena datos y su tamaño.
    Define una estructura args para pasar parámetros a los hilos.
    La función swap() es ejecutada por cada hilo y realiza intercambios aleatorios en el buffer compartido.
    start_threads() crea múltiples hilos que ejecutan swap(), espera a que terminen y luego ordena e imprime el buffer.

    El programa usa pthread para la concurrencia y getopt_long para procesar argumentos de línea de comandos.
 */

struct buffer {
    int *data;
    int size;
    pthread_mutex_t bufferMutex;
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

void *swap(void *ptr)
{
    struct args *args =  ptr;

    while(args->iterations--) {
        int i,j, tmp;

        i=rand() % args->buffer->size;
        j=rand() % args->buffer->size;

        pthread_mutex_lock(&args->buffer->bufferMutex);

        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
            args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

        tmp = args->buffer->data[i];
        if(args->delay) usleep(args->delay); // Force a context switch

        args->buffer->data[i] = args->buffer->data[j];
        if(args->delay) usleep(args->delay);

        args->buffer->data[j] = tmp;
        if(args->delay) usleep(args->delay);
        inc_count();

        pthread_mutex_unlock(&args->buffer->bufferMutex);

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

/*  Manejo del buffer y los hilos en swap.c:
    Se inicializa el buffer con valores secuenciales (buffer.data[i] = i;).
    Cada hilo accede aleatoriamente a posiciones del buffer e intercambia valores.
    Se usa usleep(delay); para introducir retardos y simular concurrencia real.
    Se espera a que todos los hilos terminen (pthread_join()).
    Se ordena el buffer final con qsort() y se imprime.
*/

void start_threads(struct options opt)
{
    int i;
    struct thread_info *threads;
    struct args *args;
    struct buffer buffer;

    srand(time(NULL));

    if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;

    for(i=0; i<buffer.size; i++)
        buffer.data[i]=i;

    if (pthread_mutex_init(&buffer.bufferMutex, NULL) != 0) {
        printf("Error initializing buffer mutex\n");
        exit(1);
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

    // Print the buffer
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    printf("iterations: %d\n", get_count());

    free(args);
    free(threads);
    free(buffer.data);

    //Es necesario este destroy si tengo el exit y ya es el final del programa?
    pthread_mutex_destroy(&buffer.bufferMutex);

    pthread_exit(NULL);
}

int main (int argc, char **argv)
{
    struct options opt;

    // Default values for the options
    opt.num_threads = 10;
    opt.buffer_size = 10;
    opt.iterations  = 100;
    opt.delay       = 10;

    read_options(argc, argv, &opt);

    start_threads(opt);

    exit (0);
}
