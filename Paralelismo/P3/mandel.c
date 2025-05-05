/*The Mandelbrot set is a fractal that is defined as the set of points c
in the complex plane for which the sequence z_{n+1} = z_n^2 + c
with z_0 = 0 does not tend to infinity.*/

/*This code computes an image of the Mandelbrot set.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi/mpi.h>


#define DEBUG 1

#define          X_RESN  1024  /* x resolution (Image width) */
#define          Y_RESN  1024  /* y resolution (Image height) */

/* Define los límites del plano complejo que se va a visualizar. */
#define           X_MIN  -2.0
#define           X_MAX   2.0
#define           Y_MIN  -2.0
#define           Y_MAX   2.0

/* The maximum number of iterations to check whether a point belongs to the set or not.
 * More iterations -> more detailed image & higher computational cost */
#define   maxIterations  1000

typedef struct complextype
{
  float real, imag;
} Compl;

//Calculates the execution duration in seconds, passing as arguments sometimes measured with gettimeofday
static inline double get_seconds(struct timeval t_ini, struct timeval t_end)
{
  return (t_end.tv_usec - t_ini.tv_usec) / 1E6 +
         (t_end.tv_sec - t_ini.tv_sec);
}


int main (int argc, char *argv[])
{

  /* Mandelbrot variables */
  int i, j, k;
  Compl   z, c;
  float   lengthsq, temp;
  int *vres = NULL,   //matriz global, solo para el proceso 0
  *localRes = NULL;  //matriz local de cada proceso

  //variables para la descomposicion de la matriz en filas
  int blocksize,  //numero de filas por proceso
  remainingRows,  //numero de filas restantes
  startRow,       //fila inicial para cada proceso
  localRows;      //numero de filas que le toca a cada proceso

  //variabes para el GatherVl
  int *receivedIntsCount = NULL,  //Array para almacenar el número de elementos que cada proceso envía
  *receivedOffset = NULL;       //Array para almacenar los desplazamientos de cada proceso

  /* Timestamp variables */
  struct timeval  ti, tf;
  double commTime = 0.0, calcTime = 0.0;

  //variables MPI
  int numprocs, rank;

  //Inicializar el entorno de ejecución de MPI
  MPI_Init (&argc, &argv);	//Parámetros argc y argv permiten que MPI maneje argumentos de la línea de comandos.

  //Cada proceso en MPI tiene un identificador único(rango) y pertenece a un grupo llamado comunicador
  //MPI_COMM_WORLD representa todos los procesos en ejecución.

  //Obtener el número total de procesos en ejecución
  MPI_Comm_size (MPI_COMM_WORLD, &numprocs);

  //Obtiene el identificador único (rango) del proceso dentro del comunicador.
  //Cada proceso recibe un rank que va de 0 a numprocs - 1
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  //calcular el número de filas por proceso
  blocksize = Y_RESN / numprocs;      //numero de filas por proceso
  remainingRows = Y_RESN % numprocs;  //numero de filas sobrantes

  startRow = rank * blocksize + (rank < remainingRows ? rank : remainingRows);    //calculo del indice de la fila inicial para cada proceso
  localRows = blocksize + (rank < remainingRows ? 1 : 0);   //numero de filas que le toca a cada proceso, si rank < remainingRows, le toca una fila más

  /*Usamos localres como array 1D para mantener la memoria contigua, eficiente,
  y lista para MPI. Lo tratamos como 2D mediante indexación manual con i * X_RESN + j*/

  //asignar memoria solo para las filas locales, filas de la matriz de cada proceso
  localRes = (int*) malloc(localRows * X_RESN * sizeof(int));
  if (!localRes) {
    fprintf(stderr, "Error allocating memory\n");
    MPI_Finalize();
    return 1;
  }

  //asignamos memoria para la matriz global, pero solo para el proceso 0 que es el unico que la imprime
  if (!rank) {
    vres = (int *) malloc(Y_RESN * X_RESN * sizeof(int));
    receivedIntsCount = malloc(numprocs * sizeof(int));  //cantidad de elementos que cada proceso envia al root
    receivedOffset = malloc(numprocs * sizeof(int));  //desplazamiento donde los datos de cada proceso deben colocarse en el buffer final
    //manejo de errores
    if (!vres || !receivedIntsCount || !receivedOffset){
      fprintf(stderr, "Error allocating memory\n");
      free(localRes);
      MPI_Finalize();
      return 1;
    }

    //Almacenamos los valores para pasárselos a gatherv
    int previousEnd;   //almacena el indice final del ultimo bloque recibido por el root
    for (i = 0; i < numprocs; i++) {
      receivedIntsCount[i] = (blocksize + (i < remainingRows ? 1 : 0)) * X_RESN;   //numero de filas para cada proceso
      previousEnd = receivedOffset[i-1] + receivedIntsCount[i-1];   //suma el desplazamiento donde comienzan los datos del bloque anterior con el tamaño del bloque anterior
      receivedOffset[i] = (i == 0) ? 0 : previousEnd;   //si es el primer bloque, el desplazamiento es 0, si no, es previousEnd
    }
  }

  /***************************CÁLCULO*****************************/

  /* Start measuring time */
  gettimeofday(&ti, NULL);

  /* Calculate and draw points (los bucles recorren cada píxel de la imagen.)*/
  for(i=0; i < localRows; i++){
    for(j=0; j < X_RESN; j++){
      z.real = z.imag = 0.0;
      c.real = X_MIN + j * (X_MAX - X_MIN)/X_RESN;
      c.imag = Y_MAX - (startRow + i) * (Y_MAX - Y_MIN)/Y_RESN;
      k = 0;

      do{    /* iterate for pixel color */
        temp = z.real*z.real - z.imag*z.imag + c.real;
        z.imag = 2.0*z.real*z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real*z.real+z.imag*z.imag;
        k++;
      } while (lengthsq < 4.0 && k < maxIterations);  //Si la magnitud de z se mantiene menor que 2 (longitud al cuadrado < 4) y no supera las maxIterations, se asume que el punto está en el conjunto.

      if (k >= maxIterations) localRes[i * X_RESN + j] = 0;  //Guarda el resultado: si se alcanzaron todas las iteraciones, se considera parte del conjunto (valor 0), si no, se guarda k, el número de iteraciones hasta que "explotó".
      else localRes[i * X_RESN + j] = k;
    }
  }

  /* End measuring time */
  gettimeofday(&tf, NULL);
  calcTime = get_seconds(ti, tf);

  /***************************COMUNICACIÓN*****************************/

  gettimeofday(&ti, NULL);

  //El root recibe de cada proceso una cantidad variable de elementos (indicados en receivedIntsCount) y los coloca en el buffer vres en las posiciones indicadas por receivedOffset.
  //localRes es la matriz local de cada proceso
  //local_rows * X_RESN es el numero de elementos que envía cada proceso
  //vres es la matriz global que recibe el proceso 0
  MPI_Gatherv(localRes, localRows * X_RESN, MPI_INT,
              vres, receivedIntsCount, receivedOffset,
              MPI_INT, 0, MPI_COMM_WORLD);

  gettimeofday(&tf, NULL);

  commTime = get_seconds(ti, tf); //Tiempo de comunicación = Tiempo de espera (para que los procesos lentos en la etapa de cálculo hagan la función de gatherv) + comunicación real

  //Imprimimos por la salida de error porque la salida estándar la vamos a redirigir a un fichero
  fprintf(stderr, "Proceso %d - Cálculo: %lf s, Comunicación: %lf s\n",
        rank, calcTime, commTime);


  /* Print result out */
  if(!rank && DEBUG ) {
    for(i=0;i<Y_RESN;i++) {
      for(j=0;j<X_RESN;j++)
              printf("%3d ", vres[i * X_RESN + j]);
      printf("\n");
    }
    free(vres);
    free(receivedIntsCount);
    free(receivedOffset);
  }

  free(localRes);
  MPI_Finalize();
  return 0;
}