/*
 * TITLE: Variables swap
 * SUBTITLE: Practical 1
 * AUTHOR 1: Tiago da Costa Teixeira Veloso e Volta LOGIN 1: tiago.velosoevolta
 * AUTHOR 2: Pablo Herrero Díaz  LOGIN 2: pablo.herrero.diaz
 * GROUP: 2.3
 * DATE: 13 / 02 / 2025
 */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "op_count.h"
#include "options.h"
#include "rec_mutex.h"

// Structure representing a shared buffer
struct buffer {
    int *data;    //array of integers
    int size;     //size of the buffer
    rec_mutex_t *positionsMutexs;    //Array of mutexes, one mutex for each buffer position
};

struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

struct args {
    int				thread_num;       // application defined thread #
    int				delay;			  // delay between operations
    int				iterations;       // number of iterations
    struct buffer	*buffer;		  // Shared buffer
};

// Function executed by each thread, swapping elements in the shared buffer
void *swap(void *ptr)
{
    struct args *args =  ptr;

    while(args->iterations--) {
        int i,j, tmp;

        i=rand() % args->buffer->size;
        j=rand() % args->buffer->size;


        //Block with recursive mutexes
        if (i < j) {
            rec_mutex_lock(&args->buffer->positionsMutexs[i]);
            rec_mutex_lock(&args->buffer->positionsMutexs[j]);
        } else {
            rec_mutex_lock(&args->buffer->positionsMutexs[j]);
            rec_mutex_lock(&args->buffer->positionsMutexs[i]);
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

        //Unlock the mutexes
        rec_mutex_unlock(&args->buffer->positionsMutexs[j]);
        rec_mutex_unlock(&args->buffer->positionsMutexs[i]);
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

// Function to initialize and start threads
void start_threads(struct options opt)
{
    int i;    //Auxiliary variable for loops
    struct thread_info *threads;    //Pointer to an array of thread_info structures
    struct args *args;              //Pointer to an array of arg structures
    struct buffer buffer;           //Local variable that holds the shared data array and its size

    srand(time(NULL));    // Seed random number generator

    if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;

    // Allocate a mutex for each buffer position
    buffer.positionsMutexs = malloc(buffer.size * sizeof(rec_mutex_t));
    if (buffer.positionsMutexs == NULL) {
        printf("Out of memory for positions mutexes\n");
        free(buffer.data);
        exit(1);
    }
    // Initialize buffer data and mutexes
    for (i = 0; i < buffer.size; i++) {
        buffer.data[i]=i;
        if (rec_mutex_init(&buffer.positionsMutexs[i]) != 0) {
            printf("Error initializing mutex for position %d\n", i);
            exit(1);
        }
    }

    printf("creating %d threads\n", opt.num_threads);

    // Allocate memory for the info structure of each thread and its respective arguments structure
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

        if (pthread_create(&threads[i].thread_id, NULL, swap, &args[i]) != 0) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish execution
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    // Print sorted buffer after operations
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    printf("iterations: %d\n", get_count());

    // Destroy mutexes and free memory
    for (i = 0; i < buffer.size; i++) {
        rec_mutex_destroy(&buffer.positionsMutexs[i]);
    }
    free(buffer.positionsMutexs);
    free(args);
    free(threads);
    free(buffer.data);

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

    // Read options from command line arguments
    read_options(argc, argv, &opt);

    // Start the thread operations
    start_threads(opt);

    exit (0);
}
