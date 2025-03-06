#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "rw_mutex.h"
#include "options.h"



// Structure representing a shared buffer
struct buffer {
    int counter;
    rw_mutex_t counter_mutex;
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

// Thread function for readers
void *reader(void *arg) {
    struct args *thread_args = (struct args *)arg;

    for (int i = 0; i < thread_args->iterations; i++) {
        rw_mutex_readlock(&thread_args->buffer->counter_mutex);
        printf("Reader %d: Counter = %d\n", thread_args->thread_num, thread_args->buffer->counter);
        rw_mutex_readunlock(&thread_args->buffer->counter_mutex);
        usleep(thread_args->delay); // Delay between reads
    }
    return NULL;
}

// Thread function for writers
void *writer(void *arg) {
    struct args *thread_args = (struct args *)arg;

    for (int i = 0; i < thread_args->iterations; i++) {
        rw_mutex_writelock(&thread_args->buffer->counter_mutex);
        thread_args->buffer->counter++;
        printf("Writer %d: Incremented counter to %d\n", thread_args->thread_num, thread_args->buffer->counter);
        rw_mutex_writeunlock(&thread_args->buffer->counter_mutex);
        usleep(thread_args->delay); // Delay between writes
    }
    return NULL;
}

// Function to start threads
void start_threads(struct options opt) {

    struct thread_info *threads;    //Pointer to an array of thread_info structures
    struct args *args;              //Pointer to an array of arg structures
    struct buffer shared_buffer;           //Local variable that holds the shared data array and its size

    // Initialize read-write mutex
    if (rw_mutex_init(&shared_buffer.counter_mutex) != 0) {
        printf("Error initializing rw_mutex\n");
        return;
    }
    shared_buffer.counter = 0;

    int total_threads = opt.num_readers + opt.num_writers;
    threads = malloc(sizeof(struct thread_info) * total_threads);
    args = malloc(sizeof(struct args) * total_threads);

    // Create reader threads
    for (int i = 0; i < opt.num_readers; i++) {
        args[i].thread_num = i;
        args[i].delay = opt.delay;
        args[i].iterations = opt.iterations;
        args[i].buffer = &shared_buffer;

        if (pthread_create(&threads[i].thread_id, NULL, reader, &args[i]) != 0) {
            printf("Error creating reader thread %d\n", i);
            return;
        }
        threads[i].thread_num = i;
    }

    int index = 0;
    // Create writer threads
    for (int i = 0; i < opt.num_writers; i++) {
        index = opt.num_readers + i;
        args[index].thread_num = i;
        args[index].delay = opt.delay;
        args[index].iterations = opt.iterations;
        args[index].buffer = &shared_buffer;

        if (pthread_create(&threads[index].thread_id, NULL, writer, &args[index]) != 0) {
            printf("Error creating writer thread %d\n", i);
            return;
        }
        threads[index].thread_num = i;
    }

    // Wait for all threads to finish
    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i].thread_id, NULL);
    }

    // Cleanup
    rw_mutex_destroy(&shared_buffer.counter_mutex);
    free(threads);
    free(args);
}

// Función principal
int main(int argc, char **argv) {
    struct options opt;

    //Predefined values
    opt.num_readers = 5;
    opt.num_writers = 2;
    opt.iterations = 100;
    opt.delay = 10;

    // Parse command-line options
    read_options(argc, argv, &opt);

    // Start threads with given options
    start_threads(opt);

    return 0;
}
