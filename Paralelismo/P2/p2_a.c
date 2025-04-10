#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi/mpi.h>

int main(int argc, char *argv[])
{
    int i, n, count, numprocs, rank, total_count;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;

	//Inicializar el entorno de ejecución de MPI
	MPI_Init (&argc, &argv);	//Parámetros argc y argv permiten que MPI maneje argumentos de la línea de comandos.

	//Cada proceso en MPI tiene un identificador único(rango) y pertenece a un grupo llamado comunicador
	//MPI_COMM_WORLD representa todos los procesos en ejecución.

	//Obtener el número total de procesos en ejecución
	MPI_Comm_size (MPI_COMM_WORLD, &numprocs);

	//Obtiene el identificador único (rango) del proceso dentro del comunicador.
	//Cada proceso recibe un rank que va de 0 a numprocs - 1
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    while (1)
    {
    	if (!rank) {
    		printf("\nEnter the number of points: (0 quits) \n");
    		scanf("%d",&n);
        }

        // Enviar el valor de n a todos los procesos
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    	//Verificamos si debemos terminar, (si el usuario indicó 0 puntos)
        if (n == 0) break;

    	//Resetea el count
        count = 0;

    	//Cada proceso genera sus puntos y cuenta los que caen dentro del círculo
        for (i = rank; i < n; i+=numprocs) {
            // Get the random numbers between 0 and 1
			x = ((double) rand()) / ((double) RAND_MAX);
			y = ((double) rand()) / ((double) RAND_MAX);

			// Calculate the square root of the squares
			z = sqrt((x*x)+(y*y));

			// Check whether z is within the circle
			if(z <= 1.0)
				count++;
        }

    	// Reducir a total_count la suma de los count
    	MPI_Reduce(&count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        // Solo el proceso 0 calcula pi y muestra el resultado
        if (!rank) {
    		pi = ((double) total_count/(double) n)*4.0;
    		printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));

    	}
    }

	MPI_Finalize();

	return 0;
}