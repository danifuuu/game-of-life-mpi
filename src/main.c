#include <openmpi/mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "game.h"
#include "misc_header.h"

char **matrix;
int local_n_rows;
int local_n_cols;
int max_buf;

void calculate_rows_cols(int *vector_n_rows, int *vector_n_cols, int size, int np_x, int np_y)
{
    for (int i = 0; i < size; i++)
    {
        vector_n_rows[i] = N_ROWS / np_y;
        vector_n_cols[i] = N_COLS / np_x;
    }
}

int main(int argc, char const *argv[])
{
    int size;
    int rank;

    int *vector_n_rows;
    int *vector_n_cols;
    SDL_Renderer *renderer;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Comprobación de las dimensaiones
    if (rank == 0)
    {
        if ((N_ROWS * N_COLS) % size != 0)
        {
            fprintf(stderr, "Número inválido de procesos, abortando\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    // Vamos a preparar nuestros procesos procesos para tratarlos como si se tratara de un grid o una
    // disposición cartesiana. Es infinitamente más fácil comunicarnos entre procesos de esta forma
    // gracias a las primitivas que nos ofrece MPI. Vamos a estar tratando con una topología 2D
    int dims[2] = {0, 0}; // dejando 0,0 la primitiva escoge la mejor disposición de procesos en 2D

    MPI_Dims_create(size, 2, dims);

    int my_pos[2];

    // Nuestras 2 dimensiones son periódicas. Si no lo fueran, una topología 2d se comportaría o sería
    // como un rectángulo. Al activar o setear la periodicidad, nuestra topología tiene el comportamiento de
    // un toroide. E.g. los procesos de la primera fila son vecinos con los de la última fila (sin que nosotros)
    // controlemos nada explícitamente. https://stackoverflow.com/questions/19200836/mpi-cart-create-period-argument
    int period[2] = {1, 1};
    // creamos un nuevo comunicador para nuestra topología
    MPI_Comm comm_grid;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, period, 0, &comm_grid);
    MPI_Cart_coords(comm_grid, rank, 2, my_pos);

    // número de procesos por cada eje o dimensión
    // [y, x]
    int np_y = dims[0];
    int np_x = dims[1];

    // Proceso con rank 0 organiza el trabajo
    if (rank == 0)
    {
        // calculamos los bloques de trabajo de cada proceso
        vector_n_cols = (int *)malloc(size * sizeof(int));
        vector_n_rows = (int *)malloc(size * sizeof(int));

        printf("np_x: %d,np_y: %d\n", np_x, np_y);
        calculate_rows_cols(vector_n_rows, vector_n_cols, size, np_x, np_y);

        // prepara gráficos
        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
        SDL_Window *window = SDL_CreateWindow(
            "Juego de la vida de Conway",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            N_COLS * CELL_SIZE,
            N_ROWS * CELL_SIZE,
            flags);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    // llamada colectiva, se sincronizan todos los procesos. Cada uno recibe el número de filas
    // y columnas de el bloque con el que tiene que trabajar
    MPI_Scatter(vector_n_rows, 1, MPI_INT, &local_n_rows, 1, MPI_INT, 0, comm_grid);
    MPI_Scatter(vector_n_cols, 1, MPI_INT, &local_n_cols, 1, MPI_INT, 0, comm_grid);

    // Vamos a reservar 2 filas y 2 columnas extras para los intercambios
    // con los vecinos
    matrix = allocate_memory(local_n_rows + 2, local_n_cols + 2);

    // cada proceso inicializa de manera aleatoria su bloque
    // dejamos las primeras y últimas filas y columnas vacias (para lo que mencionaba
    // antes de los intercambios)
    srand(getpid());
    for (int i = 1; i <= local_n_rows; i++)
    {
        for (int j = 1; j <= local_n_cols; j++)
        {
            if (rand() % 2)
                matrix[i][j] = '1';
            else
                matrix[i][j] = '0';
        }
    }

    game(comm_grid, renderer, rank, np_x, np_y, local_n_rows, local_n_cols);

    MPI_Finalize();

    return 0;
}
