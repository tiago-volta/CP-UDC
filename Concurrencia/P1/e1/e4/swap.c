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
#include <stdbool.h>
#include "op_count.h"
#include "options.h"

/*
* swap.c:
    Defines a buffer structure that stores data and its size.
    Defines an args structure to pass parameters to the threads.
    The swap() function is executed by each thread and performs random swaps in the shared buffer.
    start_threads() creates multiple threads that execute swap(), waits for them to finish, and then sorts and prints the buffer.

    The program uses pthread for concurrency and getopt_long to process command line arguments.
 */

// Structure representing a shared buffer
struct buffer {
    int *data;  // array of integers
    int size;  // size of the buffer
    pthread_mutex_t *positionsMutexs;    // Array of mutexes, one for each position
    pthread_mutex_t iterMutex;           // Mutex for iteration control
    int globalIter;                      // Global iteration counter
    bool stopIter;                       // Flag to stop iterations
};

// Structure representing thread information
struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

// Structure representing arguments passed to threads
struct args {
    int				thread_num;       // application defined thread #
    int				delay;			  // delay between operations
    struct buffer	*buffer;		  // Shared buffer
};

void *swap(void *ptr)
{
    struct args *args =  ptr;

    while(1) {
        int i,j, tmp;

        // Lock access to global iteration counter
        pthread_mutex_lock(&args->buffer->iterMutex);
        if (args->buffer->globalIter <= 0) {
            pthread_mutex_unlock(&args->buffer->iterMutex);
            break; // Exit if no iterations left
        }
        args->buffer->globalIter--; // Decrement global iteration counter

        // If counter reaches 0, signal the printer thread to stop
        if (args->buffer->globalIter == 0) {
            args->buffer->stopIter = true;
        }
        pthread_mutex_unlock(&args->buffer->iterMutex);

        // Randomly select two positions in the buffer
        i=rand() % args->buffer->size;
        j=rand() % args->buffer->size;

        //Avoid deadlock by acquiring resources in a consistent order, thus, we will avoid a circular wait between threads
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

        // Print the swap operation
        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
            args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

        // Swap the elements
        tmp = args->buffer->data[i];
        if(args->delay) usleep(args->delay); // Force a context switch

        args->buffer->data[i] = args->buffer->data[j];
        if(args->delay) usleep(args->delay);

        args->buffer->data[j] = tmp;
        if(args->delay) usleep(args->delay);
        inc_count();

        // Unlock the mutexes
        pthread_mutex_unlock(&args->buffer->positionsMutexs[i]);
        if (i != j) {
            pthread_mutex_unlock(&args->buffer->positionsMutexs[j]);
        }

    }
    return NULL;
}

// Comparison function for sorting
int cmp(int *e1, int *e2) {
    if(*e1==*e2) return 0;
    if(*e1<*e2) return -1;
    return 1;
}

// Function to print the buffer content
void print_buffer(struct buffer buffer) {
    int i;

    for (i = 0; i < buffer.size; i++)
        printf("%i ", buffer.data[i]);
    printf("\n");
}

// Function executed by the printer thread to periodically print the buffer
void *print_periodic (void *ptr) {
    struct args *args = ptr;

    while (1){
        usleep(args->delay);

        pthread_mutex_lock(&args->buffer->iterMutex);
        if (args->buffer->stopIter) {  // Check if we should stop
            pthread_mutex_unlock(&args->buffer->iterMutex);
            break;
        }
        pthread_mutex_unlock(&args->buffer->iterMutex);

        // Lock each position in the buffer
        for (int i = 0; i < args->buffer->size; i++) {
        	pthread_mutex_lock(&args->buffer->positionsMutexs[i]);
        }
        // Print the buffer content
        printf("Buffer: ");
        for (int i = 0; i < args->buffer->size; i++) {
          	printf("%d ", args->buffer->data[i]);
        }
        // Unlock each position in the buffer
        for (int i = 0; i < args->buffer->size; i++) {
          	pthread_mutex_unlock(&args->buffer->positionsMutexs[i]);
        }
        printf("\n");
    }

    return NULL;
}

// Function to initialize and start threads
void start_threads(struct options opt)
{
     int i;    //Auxiliary variable for loops
    struct thread_info *threads;    //Pointer to an array of thread_info structures
    struct args *args;              //Pointer to an array of arg structures
    struct buffer buffer;           //Local variable that holds the shared data array and its size

    srand(time(NULL));

    // Allocate memory for the buffer
    if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;          // Store the buffer size in the local variables
    buffer.globalIter = opt.iterations;     // Initialize global iteration counter
    buffer.stopIter = false;                // Initialize stop flag

    // Allocate a mutex for each buffer position
    buffer.positionsMutexs = malloc(buffer.size * sizeof(pthread_mutex_t));
    if (buffer.positionsMutexs == NULL) {
        printf("Out of memory for positions mutexes\n");
        free(buffer.data);
        exit(1);
    }
    // Initialize buffer data and mutexes
    for (i = 0; i < buffer.size; i++) {
        buffer.data[i]=i;
        if (pthread_mutex_init(&buffer.positionsMutexs[i], NULL) != 0) {
            printf("Error initializing mutex for position %d\n", i);
            exit(1);
        }
    }
    //Initialize the iteration mutex
    if (pthread_mutex_init(&buffer.iterMutex, NULL) != 0) {
        printf("Error initializing iter_mutex\n");
        exit(1);
    }

    printf("creating %d threads\n", opt.num_threads);

    //Allocate memory for the info structure of each thread and its respective arguments structure,
    //the last position will be the printer thread
    threads = malloc(sizeof(struct thread_info) * (opt.num_threads + 1));
    args = malloc(sizeof(struct args) * (opt.num_threads + 1));

    if (threads == NULL || args==NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    printf("Buffer before: ");
    print_buffer(buffer);

    /* --- Create the printer thread (using the extra element) --- */

    // Initialize the printer thread's arguments.
    // Here we use the 'delay' field to hold the print interval (opt.print_wait).
    args[opt.num_threads].thread_num = opt.num_threads;  // Identifier for the printer thread
    args[opt.num_threads].buffer     = &buffer;          // Assign the shared buffer to the printer thread's arguments
    args[opt.num_threads].delay      = opt.print_wait;   // Print interval (in microseconds)

    if (pthread_create(&threads[opt.num_threads].thread_id, NULL, print_periodic, &args[opt.num_threads]) != 0) {
        printf("Could not create printer thread\n");
        exit(1);
    }

    /* --- Create the swap threads --- */


    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].thread_num = i;
        args[i].thread_num = i;
        args[i].buffer     = &buffer;
        args[i].delay      = opt.delay;

        if ( 0 != pthread_create(&threads[i].thread_id, NULL,
                     swap, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    //flag was stablished to true in the last swap so printer thread can wait
    pthread_join(threads[opt.num_threads].thread_id, NULL);

    // Print the buffer
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    printf("iterations: %d\n", get_count());

    // Destroy the mutexes and free memory
    pthread_mutex_destroy(&buffer.iterMutex);
    for (i = 0; i < buffer.size; i++) {
        pthread_mutex_destroy(&buffer.positionsMutexs[i]);
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
    opt.iterations  = 100;
    opt.delay       = 10;
    opt.print_wait  = 1;

    // Read options from command line arguments
    read_options(argc, argv, &opt);

    // Start the thread operations
    start_threads(opt);

    exit (0);
}
