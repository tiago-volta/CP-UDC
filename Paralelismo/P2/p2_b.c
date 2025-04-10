#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi/mpi.h>

//Implementación de MPI_Reduce con un Flattree
int MPI_FlattreeColectiva(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

//Implementación de MPI_Bcast con Árbol Binomial
int MPI_BinomialColectiva(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);


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
        MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

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
    	MPI_FlattreeColectiva(&count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        // Solo el proceso 0 calcula pi y muestra el resultado
        if (!rank) {
    		pi = ((double) total_count/(double) n)*4.0;
    		printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));

    	}
    }

	MPI_Finalize();

	return 0;
}

//En esta función todos los procesos envían sus datos al proceso raíz,
//el cual los recibe en un bucle y hace la operación (en este caso, suma).

int MPI_FlattreeColectiva(const void *sendbuf, void *recvbuf, int count,
                          MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm){

    //Comprobamos si los punteros apuntan a un espacio válido de memoria
  	if (!sendbuf || !recvbuf) {
		fprintf(stderr, "MPI_FlattreeColectiva requiere 2 buffer uno de envío y otro de recepción\n");
		return MPI_ERR_BUFFER;
	}

	if (count < 0){
		fprintf(stderr, "MPI_FlattreeColectiva no admite count negativo\n");
		return MPI_ERR_COUNT;
	}

    // Solo aceptamos enteros
	if (datatype != MPI_INT) {
		fprintf(stderr, "MPI_FlattreeColectiva solo admite MPI_INT\n");
		return MPI_ERR_TYPE;
	}

	// Solo aceptamos suma
	if (op != MPI_SUM) {
		fprintf(stderr, "MPI_FlattreeColectiva solo admite MPI_SUM\n");
		return MPI_ERR_OP;
	}

    //Comprobamos si el comunicador se ha inicializado correctamente
	if (!comm) {
		fprintf(stderr, "MPI_FlattreeColectiva requiere un comunicador\n");
		return MPI_ERR_COMM;
	}

	int rank, size, error;

    if((error = MPI_Comm_size(comm, &size)) != MPI_SUCCESS){
		fprintf(stderr, "Error al obtener el tamaño del comunicador\n");
        return error;
    }

	if (root < 0 || root >= size){
		fprintf(stderr, "Parámetro root no válido\n");
		return MPI_ERR_ROOT;
	}

	if((error = MPI_Comm_rank(comm, &rank)) != MPI_SUCCESS){
		fprintf(stderr, "Error al obtener el rank del proceso\n");
		return error;
	}


    if (rank == root){
		int temp;

        // Copiar primero el valor local del root en la primera posición del array de enteros
        ((int*)recvbuf)[0] = ((int*)sendbuf)[0];

        // Recibir y acumular el buffer enviado por los demás procesos del comunicador,
        // si el root ejecuta el bucle antes de que todos los demás procesos no han hecho el send
        // no pasa nada, ya que, el root se quedará esperando en MPI_Recv, por ser esta bloqueante.
    	for (int i = 0; i < size - 1; i++) {		//No incluímos todos los procesos, ya que, el root nunca va a recibir.
    		error = MPI_Recv(&temp, count, datatype, MPI_ANY_SOURCE, 0, comm, MPI_STATUS_IGNORE);
            if (error != MPI_SUCCESS) {
               	fprintf(stderr, "Error al recibir datos de un proceso\n");
                return error;
            }
            ((int*)recvbuf)[0] += temp;

        }

    }else {
        error = MPI_Send(sendbuf, count, datatype, root, 0, comm);	// Enviar datos al root
        if (error != MPI_SUCCESS) {
            fprintf(stderr, "Error al enviar datos en el proceso %d\n", rank);
            return error;
        }
    }

	return MPI_SUCCESS;
}

//Esta función reparte un dato desde el proceso 0 al resto usando un árbol binomial.

int MPI_BinomialColectiva(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm){

  	if (!buffer){size
  		fprintf(stderr, "MPI_BinomialColectiva requiere un buffer\n");
        return MPI_ERR_BUFFER;
  	}

  	if (count < 0){
  		fprintf(stderr, "MPI_BinomialColectiva no admite count negativo\n");
    	return MPI_ERR_COUNT;
    }

	// Solo aceptamos enteros
	if (datatype != MPI_INT) {
		fprintf(stderr, "MPI_BinomialColectiva solo admite MPI_INT\n");
		return MPI_ERR_TYPE;
	}

    if (root != 0){
      	fprintf(stderr, "MPI_BinomialColectiva no admite root distinto de 0\n");
      	return MPI_ERR_ROOT;
    }

	if (!comm) {
		fprintf(stderr, "MPI_BinomialColectiva requiere un comunicador\n");
		return MPI_ERR_COMM;
	}

  	int rank, size, error, offset = 1;

	if((error = MPI_Comm_size(comm, &size)) != MPI_SUCCESS){
		fprintf(stderr, "Error al obtener el tamaño del comunicador en el proceso %d\n", rank);
		return error;
	}

    if((error = MPI_Comm_rank(comm, &rank)) != MPI_SUCCESS){
		fprintf(stderr, "Error al obtener el rank del proceso\n");
        return error;
    }

    while (offset < size) {
        int pareja;		//Rango de la pareja con la que un proceso se comunica

        //Los procesos con rango < 2^i−1 (offset), envían a los que están 'offset' posiciones más adelante (tu "pareja")
        if (rank < offset) {
            pareja = rank + offset;
            if (pareja < size) {	// Verificamos que no exceda el número total de procesos
                error = MPI_Send(buffer, count, datatype, pareja, 0, comm);
                if (error != MPI_SUCCESS) {
                  	fprintf(stderr, "Error al enviar datos en el proceso %d\n", rank);
            	  	return error;
                }
            }
        } else if (rank < 2 * offset) {		//Reciben los procesos que su rango está en [offset, 2*offset), para que no reciban procesos que no les toca
            pareja = rank - offset;
            if (pareja >= 0) {			// Verificamos que sea un proceso válido
                error = MPI_Recv(buffer, count, datatype, pareja, 0, comm, MPI_STATUS_IGNORE);
                if (error != MPI_SUCCESS) {
                    fprintf(stderr, "Error al recibir datos de un proceso\n");
                    return error;
                }
            }
        }

        offset*=2;	// Doblamos el valor del offset para el siguiente paso del árbol binomial
    }

  	return MPI_SUCCESS;
}
