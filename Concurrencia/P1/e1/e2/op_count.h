#ifndef __OP_COUNT_H__
#define __OP_COUNT_H__

/*
 * op_count.c y op_count.h:
 * Manejan una variable global count protegida por un mutex
 * para contar las operaciones realizadas por los hilos.
 */

void inc_count();
int get_count();

#endif