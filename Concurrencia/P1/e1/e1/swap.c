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

struct buffer {
    int *data;      //Pointer to the buffer (an integer array)
    int size;       //The size (number of elements) of the buffer
    pthread_mutex_t bufferMutex;    //Mutex to protect the array accesses
};

struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

struct args {
    int				thread_num;       // application defined thread #
    int				delay;			  // delay between operations (in microseconds) to force context switches
    int				iterations;       //
    struct buffer	*buffer;		  // Pointer to the Shared buffer
};

//Swap function executed by each thread
void *swap(void *ptr)
{
    struct args *args =  ptr;   //Cast the argument to our args structure

    //Each thread performs 'iterations' swaps
    while(args->iterations--) {
        int i,j, tmp;

        //Choose two random indexes within the buffer range
        i=rand() % args->buffer->size;
        j=rand() % args->buffer->size;

        pthread_mutex_lock(&args->buffer->bufferMutex); //Lock the mutex to ensure exclusive access to the buffer while swapping

        //Print swap details
        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
            args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

        //Swap the values at positions i and j
        tmp = args->buffer->data[i];
        if(args->delay) usleep(args->delay); // Simulate a context switch, in this case is only a sleep time for the current thread, because the current one locked this critical section

        args->buffer->data[i] = args->buffer->data[j];
        if(args->delay) usleep(args->delay);

        args->buffer->data[j] = tmp;
        if(args->delay) usleep(args->delay);

        //Increment the global operation counter
        inc_count();

        pthread_mutex_unlock(&args->buffer->bufferMutex); //Unlock the mutex so that the other threads can access the buffer
    }
    return NULL;
}

//Comparison function used by qsort
int cmp(int *e1, int *e2) {
    if(*e1==*e2) return 0;
    if(*e1<*e2) return -1;
    return 1;
}

// Function to print the entire buffer
void print_buffer(struct buffer buffer) {
    int i;

    for (i = 0; i < buffer.size; i++)
        printf("%i ", buffer.data[i]);
    printf("\n");
}

/*
 * This function initializes a shared buffer with sequential integers, creates a specified number
 * of swap threads that perform random swaps on the buffer, waits for all threads to finish,
 * sorts the final buffer, prints the final state along with the total number of swap operations,
 * cleans up allocated resources, and finally terminates the thread using pthread_exit().
 */

void start_threads(struct options opt)
{
    int i;    //Auxiliary variable for loops
    struct thread_info *threads;    //Pointer to an array of thread_info structures
    struct args *args;              //Pointer to an array of arg structures
    struct buffer buffer;           //Local variable that holds the shared data array and its size

    srand(time(NULL));

    //Allocate memory for the buffer's data array
    if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;  //Set the buffer size

    //Initialize the buffer with sequential values
    for(i=0; i<buffer.size; i++)
        buffer.data[i]=i;

    //Initialize the mutex that will protect the entire buffer during the swap operations
    if (pthread_mutex_init(&buffer.bufferMutex, NULL) != 0) {
        printf("Error initializing buffer mutex\n");
        exit(1);
    }

    printf("creating %d threads\n", opt.num_threads);

    //Allocate memory for the arrays of threads information and threads arguments
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

        //Initialize arguments for each thread
        args[i].thread_num = i;
        args[i].buffer     = &buffer;
        args[i].delay      = opt.delay;
        args[i].iterations = opt.iterations;    //Number of swaps each thread should perfom

        //Create the thread that will execute the swap function
        if ( 0 != pthread_create(&threads[i].thread_id, NULL,
                     swap, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    // Print the total number of swap operations performed.
    printf("iterations: %d\n", get_count());

    free(args);
    free(threads);
    free(buffer.data);

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
