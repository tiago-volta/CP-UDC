#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "op_count.h"
#include "options.h"

/*
 * swap.c:
 *   - Defines a 'buffer' structure that holds the data array, its size, and a mutex.
 *   - Defines an 'args' structure used to pass parameters to threads.
 *   - The swap() function is executed by each swap thread and randomly exchanges values
 *     in the shared array.
 *   - A printer thread is added to periodically print the contents of the array.
 *   - The start_threads() function creates all threads (both swap threads and the printer thread),
 *     waits for them to finish, sorts and prints the final array.
 */

struct buffer {
    int *data;
    int size;
    pthread_mutex_t *positionsMutexs, iterMutex;    //Array de mutexs, tendremos un mutex por cada posición
    bool stopIter;
};

struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

struct args {
    int				thread_num;       // application defined thread # (numero de hilo)
    int				delay;			  // delay between operations (en micro)
    int				iterations;        // Número de intercambios que realizará el hilo
    struct buffer	*buffer;		  // Shared buffer
};

void *swap(void *ptr)
{
    struct args *args =  ptr;

    while(args->iterations--) {
        int i,j, tmp;

        i=rand() % args->buffer->size;
        j=rand() % args->buffer->size;

        /*
        * Evitamos el interbloqueo realizando una reserva ordenada de recursos
        * así evitaremos una espera circular entre los threads.
        */

        if (i != j) {
            if (i < j) {
                pthread_mutex_lock(&args->buffer->positionsMutexs[i]);
                pthread_mutex_lock(&args->buffer->positionsMutexs[j]);
            } else {
                pthread_mutex_lock(&args->buffer->positionsMutexs[j]);
                pthread_mutex_lock(&args->buffer->positionsMutexs[i]);
            }
        } else {
            pthread_mutex_lock(&args->buffer->positionsMutexs[i]); // Bloquear solo una vez si i == j
        }

        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
            args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

        tmp = args->buffer->data[i];
        if(args->delay) usleep(args->delay); // Force a context switch

        args->buffer->data[i] = args->buffer->data[j];
        if(args->delay) usleep(args->delay);

        args->buffer->data[j] = tmp;
        if(args->delay) usleep(args->delay);
        inc_count();

        pthread_mutex_unlock(&args->buffer->positionsMutexs[i]);
        if (i != j) {
            pthread_mutex_unlock(&args->buffer->positionsMutexs[j]);
        }
    }
    return NULL;
}

//Compare function for qsort
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


void *print_periodic (void *ptr) {
    struct args *args = ptr;

    while (1){
        usleep(args->delay);

        pthread_mutex_lock(&args->buffer->iterMutex);
        if (args->buffer->stopIter) {  // Revisamos si debemos detenernos
            pthread_mutex_unlock(&args->buffer->iterMutex);
            break;
        }
        pthread_mutex_unlock(&args->buffer->iterMutex);

        for (int i = 0; i < args->buffer->size; i++) {
        	 pthread_mutex_lock(&args->buffer->positionsMutexs[i]);
        }
        printf("Buffer: ");
        for (int i = 0; i < args->buffer->size; i++) {
          printf("%d ", args->buffer->data[i]);
        }
        for (int i = 0; i < args->buffer->size; i++) {
          pthread_mutex_unlock(&args->buffer->positionsMutexs[i]);
        }
        printf("\n");
    }

    return NULL;
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
    int i;    //Auxiliary variable for loops
    struct thread_info *threads;    //Pointer to an array of thread_info structures
    struct args *args;    //Pointer to an array of arg structures
    struct buffer buffer;    //Local variable that holds the shared data array and its size

    srand(time(NULL));

    if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {    //Memory allocated for the buffer which is an integer array
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;    //Store de buffer size in the local variables
    buffer.stopIter = false;

    /* Reservar e inicializar el array de mutex para cada posición */
    buffer.positionsMutexs = malloc(buffer.size * sizeof(pthread_mutex_t));
    if (buffer.positionsMutexs == NULL) {
        printf("Out of memory for positions mutexes\n");
        free(buffer.data);
        exit(1);
    }
    for (i = 0; i < buffer.size; i++) {
        buffer.data[i]=i;
        if (pthread_mutex_init(&buffer.positionsMutexs[i], NULL) != 0) {
            printf("Error initializing mutex for position %d\n", i);
            exit(1);
        }
    }

    if (pthread_mutex_init(&buffer.iterMutex, NULL) != 0) {
        printf("Error initializing iter_mutex\n");
        exit(1);
    }

    printf("creating %d threads\n", opt.num_threads);

    /*Allocate memory for the info structure of each thread and its respective arguments structure,
    the last position will be the printer thread*/
    threads = malloc(sizeof(struct thread_info) * (opt.num_threads + 1));
    args = malloc(sizeof(struct args) * (opt.num_threads + 1));

    if (threads == NULL || args==NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    //Initial buffer state
    printf("Buffer before: ");
    print_buffer(buffer);

    /* --- Create the printer thread (using the extra element) --- */

    // Initialize the printer thread's arguments.
    // Here we use the 'delay' field to hold the print interval (opt.print_wait).
    args[opt.num_threads].thread_num = opt.num_threads;  // Identifier for the printer thread
    args[opt.num_threads].buffer     = &buffer;
    args[opt.num_threads].delay      = opt.print_wait;  // Print interval (in microseconds)
    args[opt.num_threads].iterations = 0;              // Not used by the printer thread

    if (pthread_create(&threads[opt.num_threads].thread_id, NULL, print_periodic, &args[opt.num_threads]) != 0) {
        printf("Could not create printer thread\n");
        exit(1);
    }

    /* --- Create the swap threads --- */

    for (i = 0; i < opt.num_threads; i++) {
        threads[i].thread_num = i;

        args[i].thread_num = i;
        args[i].buffer     = &buffer;
        args[i].delay      = opt.delay;
        args[i].iterations = opt.iterations;

        if (pthread_create(&threads[i].thread_id, NULL, swap, &args[i]) != 0) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    // Señalar al hilo de impresión que debe terminar
    pthread_mutex_lock(&buffer.iterMutex);
    buffer.stopIter = true;
    pthread_mutex_unlock(&buffer.iterMutex);
    pthread_join(threads[opt.num_threads].thread_id, NULL);

    // Print the buffer
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    printf("iterations: %d\n", get_count());

    free(args);
    free(threads);
    free(buffer.data);

    pthread_mutex_destroy(&buffer.iterMutex);

    // Destruir los mutex y liberar memoria
    for (i = 0; i < buffer.size; i++) {
        pthread_mutex_destroy(&buffer.positionsMutexs[i]);
    }
    free(buffer.positionsMutexs);

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
    opt.print_wait  = 1;  // Default print interval: 1 microsecond

    read_options(argc, argv, &opt);

    start_threads(opt);

    exit (0);
}
