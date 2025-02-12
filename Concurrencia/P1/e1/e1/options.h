#ifndef __OPTIONS_H__
#define __OPTIONS_H__

/*
 * options.c y options.h:
 * Gestionan la entrada de parámetros de línea de comandos para configurar
 * el número de hilos, tamaño del buffer, iteraciones, retraso, etc.
 */

struct options {
	int num_threads;
	int buffer_size;
	int iterations;
	int delay;
	int print_wait;
};

int read_options(int argc, char **argv, struct options *opt);


#endif
