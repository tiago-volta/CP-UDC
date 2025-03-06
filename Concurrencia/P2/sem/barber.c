#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "options.h"
#include "sem.h"

// Structure representing a shared buffer
struct buffer {
    sem_t customers;
    sem_t barbers;
    sem_t free_seats_sem;    //Sem that acts like a mutex to protect free_seats
    int free_seats;
    int done;                //No more clients expected
};

// Structure representing thread information
struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

// Structure representing arguments passed to threads
struct args {
    int				thread_num;       // application defined thread #
    int				delay;			  // delay between operations (only used by the barder)
    struct buffer	*buffer;		  // Shared buffer
};

void *barber_thread(void *ptr) {
    struct args *args =  ptr;
    while (1) {

        //Waits for a client
        sem_p(&args->buffer->customers);

        //Check if we should exit
        if (args->buffer->done) {
            break;
        }

        sem_p(&args->buffer->free_seats_sem);   //Block to increase the counter
        args->buffer->free_seats ++;
        sem_v(&args->buffer->barbers);          //Signal up a client

        sem_v(&args->buffer->free_seats_sem);   //Unlock the counter

        // Simulation of the hair cut
        printf("Barbero %d: Cortando el pelo...\n", args->thread_num);
        usleep(args->delay);
    }
    return NULL;
}



void *customer_thread(void *ptr) {
    struct args *args =  ptr;
    sem_p(&args->buffer->free_seats_sem);       //Block counter
    if (args->buffer->free_seats > 0) {
        args->buffer->free_seats --;            //Occupied chair
        sem_v(&args->buffer->customers);        //Incremet the clients

        sem_v(&args->buffer->free_seats_sem);   //Unlock the counter

        sem_p(&args->buffer->barbers);          //Waits for a free barber

        // Simulation of the hair cut
        printf("Cliente %d: Le están cortando el pelo...\n", args->thread_num);
        usleep(args->delay);
    }else {
        sem_v(&args->buffer->free_seats_sem);
        printf("Cliente %d: No hay sillas, se va.\n", args->thread_num);
    }
    return NULL;
}

// Function to initialize and start threads
void start_threads(struct options opt)
{
    int i;    //Auxiliary variable for loops
    struct thread_info *customer_threads, *barber_threads;    //Pointer to an array of thread_info structures
    struct args *customer_args,*barber_args;                  //Pointer to an array of arg structures
    struct buffer buffer;                                     //Local variable that represents the shared buffer with the semaphores, the number of free seats and the flag

    sem_init(&buffer.customers, 0);         //Initially there is no clients waiting
    sem_init(&buffer.barbers, 0);           //Initially the barber is sleeping
    sem_init(&buffer.free_seats_sem, 1);    //Sem that acts like a mutex to protect free_seats
    buffer.free_seats = opt.seats;
    buffer.done = 0;

    printf("creando %d hilos de barberos y %d hilos de clientes\n", opt.barbers,opt.customers);

    //Allocate memory for the info structure of each thread and its respective arguments structure,
    //There will be (customers + barbers) threads in total
    customer_threads = malloc(sizeof(struct thread_info) * opt.customers);
    barber_threads = malloc(sizeof(struct thread_info) * opt.barbers);
    customer_args = malloc(sizeof(struct args) * opt.customers);
    barber_args = malloc(sizeof(struct args) * opt.barbers);


    if (customer_threads == NULL || barber_threads==NULL || customer_args==NULL || barber_args==NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    //Creation of the barber threads
    for (i = 0; i < opt.barbers; i++) {
        barber_threads[i].thread_num = i;
        barber_args[i].thread_num = i;
        barber_args[i].delay = opt.cut_time;
        barber_args[i].buffer = &buffer;
        if (pthread_create(&barber_threads[i].thread_id, NULL, barber_thread, &barber_args[i]) != 0) {
            printf("Could not create the barber thread #%d", i);
            exit(1);
        }
    }


    //Creation of the customers threads
    for (i = 0; i < opt.customers; i++) {
        customer_threads[i].thread_num = i;
        customer_args[i].thread_num = i;
        customer_args[i].delay = opt.cut_time;
        customer_args[i].buffer = &buffer;
        if (pthread_create(&customer_threads[i].thread_id, NULL, customer_thread, &customer_args[i]) != 0) {
            printf("Could not create the customer thread #%d", i);
            exit(1);
        }
    }


    // Wait for all customers to finish
    for (i = 0; i < opt.customers; i++) {
        pthread_join(customer_threads[i].thread_id, NULL);
    }

    // Indicate barbers to stop
    buffer.done = 1;
    for (i = 0; i < opt.barbers; i++) {
        sem_v(&buffer.customers);
    }

    // Some barbers wait for the others, because, without it, the program could free memory, in free(barber_threads),
    // with the threads runnig yet, this will cause an access to freed memory, which would yield undefined behavior.
    for (i = 0; i < opt.barbers; i++) {
        pthread_join(barber_threads[i].thread_id, NULL);
    }

    // Liberar recursos
    free(barber_threads);
    free(customer_threads);
    free(barber_args);
    free(customer_args);
    sem_destroy(&buffer.customers);
    sem_destroy(&buffer.barbers);
    sem_destroy(&buffer.free_seats_sem);

    pthread_exit(NULL);
}


int main (int argc, char **argv)
{
    struct options opt;

    // Default values for the options
    opt.barbers = 5;
    opt.customers = 100;
    opt.cut_time  = 1000;
    opt.seats = 5;

    read_options(argc, argv, &opt);

    start_threads(opt);

    exit (0);
}
