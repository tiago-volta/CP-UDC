#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "op_count.h"
#include "options.h"

volatile int running = 1; // Controla la ejecución del hilo de impresión
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger `running`
pthread_mutex_t iterations_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger `iterations`
int global_iterations; // Global iterations counter

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
    int             thread_num;       // application defined thread #
    int             delay;            // delay between operations
    struct buffer   *buffer;          // Shared buffer
    struct control_vars *ctrl_vars;   // Variables de control compartidas
};

struct control_vars {
    int running;                     // Controla la ejecución del hilo de impresión
    pthread_mutex_t running_mutex;   // Mutex para proteger 'running'
    int global_iterations;           // Contador global de iteraciones
    pthread_mutex_t iterations_mutex;// Mutex para proteger 'global_iterations'
};

struct print_args {
    struct buffer *buffer;
    int delay;
    struct control_vars *ctrl_vars; // Control variables
};

void *swap(void *ptr)
{
    struct args *args = (struct args *)ptr;
    srand(time(NULL) ^ pthread_self()); // Seed para evitar valores idénticos en hilos distintos

    while (1) {
        pthread_mutex_lock(&args->ctrl_vars->iterations_mutex);
        if (args->ctrl_vars->global_iterations <= 0) {
            pthread_mutex_unlock(&args->ctrl_vars->iterations_mutex);
            break;
        }
        args->ctrl_vars->global_iterations--;
        pthread_mutex_unlock(&args->ctrl_vars->iterations_mutex);

        int i, j, tmp;
        i = rand() % args->buffer->size;
        j = rand() % args->buffer->size;

        if (i == j) {
            // Revertir decremento si no se realiza swap
            pthread_mutex_lock(&args->ctrl_vars->iterations_mutex);
            args->ctrl_vars->global_iterations++;
            pthread_mutex_unlock(&args->ctrl_vars->iterations_mutex);
            continue;
        }

        // Bloqueo ordenado para prevenir deadlocks
        if (i > j) {
            if (pthread_mutex_lock(&args->buffer->mutexes[j]) != 0) continue;
            if (pthread_mutex_lock(&args->buffer->mutexes[i]) != 0) {
                pthread_mutex_unlock(&args->buffer->mutexes[j]);
                continue;
            }
        } else {
            if (pthread_mutex_lock(&args->buffer->mutexes[i]) != 0) continue;
            if (pthread_mutex_lock(&args->buffer->mutexes[j]) != 0) {
                pthread_mutex_unlock(&args->buffer->mutexes[i]);
                continue;
            }
        }

        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
               args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

        tmp = args->buffer->data[i];
        if (args->delay) usleep(args->delay); // Single delay

        args->buffer->data[i] = args->buffer->data[j];
        args->buffer->data[j] = tmp;

        pthread_mutex_unlock(&args->buffer->mutexes[j]); // Unlock in reverse order
        pthread_mutex_unlock(&args->buffer->mutexes[i]);

        inc_count();
    }
    return NULL;
}

int cmp(int *e1, int *e2) {
    if (*e1 == *e2) return 0;
    if (*e1 < *e2) return -1;
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

        pthread_mutex_lock(&args->ctrl_vars->running_mutex);
        if (!args->ctrl_vars->running) {
            pthread_mutex_unlock(&args->ctrl_vars->running_mutex);
            break;
        }
        pthread_mutex_unlock(&args->ctrl_vars->running_mutex);

        // Imprimir el contenido del buffer
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

//Función para detener el thread de impresión
void stop_printing_thread(struct control_vars *ctrl_vars) {
    pthread_mutex_lock(&ctrl_vars->running_mutex);
    ctrl_vars->running = 0;
    pthread_mutex_unlock(&ctrl_vars->running_mutex);
}

void start_threads(struct options opt)
{
    int i;
    struct thread_info *threads;
    struct print_args p_args;
    struct args *args;
    struct buffer buffer;
    pthread_t print_thread;

    // Crear e inicializar las variables de control
    struct control_vars ctrl_vars;
    ctrl_vars.running = 1;
    ctrl_vars.global_iterations = opt.iterations;
    pthread_mutex_init(&ctrl_vars.running_mutex, NULL);
    pthread_mutex_init(&ctrl_vars.iterations_mutex, NULL);p_args.ctrl_vars = &ctrl_vars;

    srand(time(NULL));

    if ((buffer.data = malloc(opt.buffer_size * sizeof(int))) == NULL) {
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

    if (threads == NULL || args == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    printf("Buffer before: ");
    print_buffer(buffer);

    //Thread que imprime periodicamente el contenido del array
    p_args.buffer = &buffer;
    p_args.delay = opt.delay;
    p_args.ctrl_vars = &ctrl_vars;
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
        args[i].ctrl_vars  = &ctrl_vars;

        if (0 != pthread_create(&threads[i].thread_id, NULL, swap, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    // Detener hilo de impresión y esperar a que finalice
    stop_printing_thread(&ctrl_vars);
    pthread_join(print_thread, NULL);

    // Print the buffer
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);

    printf("iterations: %d\n", get_count());

    //destruimos los mutexes
    for (i = 0; i < buffer.size; i++)
        pthread_mutex_destroy(&buffer.mutexes[i]);
    pthread_mutex_destroy(&ctrl_vars.running_mutex);
    pthread_mutex_destroy(&ctrl_vars.iterations_mutex);


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