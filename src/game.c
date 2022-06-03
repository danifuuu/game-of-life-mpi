#include <openmpi/mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "misc_header.h"
#include "game.h"
#include "utils.h"

#define N_NBR 8

char **next_gen;

void find_neighbours(MPI_Comm comm_grid, int my_rank, int np_y, int np_x, int *left, int *right, int *top, int *bottom, int *topleft, int *topright, int *bottomleft, int *bottomright)
{
    int disp = 1;
    int my_pos[2];
    int corner_pos[2];

    // Gracias a que nuestros procesos están organizados como un grid cartesiano, podemos hacer uso
    // de esta función para obtener nuestros vecinos, sin tener que preocuparnos de si somos la última
    // o primera fila. Se encarga automáticamente. Decimos en qué eje queremos obtener los vecinos, que
    // están siempre a un desplazamiento de 1 unidad respecto a nuestro proceso "base"
    MPI_Cart_shift(comm_grid, 0, disp, bottom, top);
    MPI_Cart_shift(comm_grid, 1, disp, left, right);

    // Para las esquinas tenemos que hacerlo distinto, no es tan fácil como hacer un shift
    // Calculamos las coordenadas de nuestro proceso, y las ajustamos para llegar a las esquina

    // Esquina superior derecha
    MPI_Cart_coords(comm_grid, my_rank, 2, my_pos);
    corner_pos[0] = my_pos[0] - 1;
    corner_pos[1] = (my_pos[1] + 1) % np_x;

    if (corner_pos[0] < 0)
        corner_pos[0] = np_y - 1;
    // dando unas coordenadas, obtenemos el rank
    MPI_Cart_rank(comm_grid, corner_pos, topright);

    // Esquina superior izquierda
    corner_pos[1] = my_pos[1] - 1;

    if (corner_pos[1] < 0)
        corner_pos[1] = np_x - 1;
    MPI_Cart_rank(comm_grid, corner_pos, topleft);

    // Esquina inferior derecha
    corner_pos[0] = (my_pos[0] + 1) % np_y;
    corner_pos[1] = (my_pos[1] + 1) % np_x;
    MPI_Cart_rank(comm_grid, corner_pos, bottomright);

    // Esquina inferior izquierda
    corner_pos[1] = my_pos[1] - 1;
    if (corner_pos[1] < 0)
        corner_pos[1] = np_x - 1;
    MPI_Cart_rank(comm_grid, corner_pos, bottomleft);

    int pene[2];
    int gordo[2];
    MPI_Cart_coords(comm_grid, *top, 2, pene);
    MPI_Cart_coords(comm_grid, *bottom, 2, gordo);

    // y,x
    // printf("soy  (rank :%d)y:%d,x:%d. Vecinos: top:%d,%d, bot:%d,%d\n", my_rank, my_pos[0], my_pos[1], pene[0], pene[1], gordo[0], gordo[1]);
}

void fill_buf(int rows, int cols, char *buffer, int rank, MPI_Comm comm_grid)
{
    int index = 0;

    // printf("LLenado de buffer de %d\n", rank);

    for (int i = 1; i <= local_n_rows; i++)
    {
        for (int j = 1; j <= local_n_cols; j++)
        {
            buffer[index++] = matrix[i][j];
            // printf("%c", buffer[index-1]);
        }
        // printf("\n");
    }
}

int neighbourhood_sum(int outer_i, int outer_j)
{
    int sum = 0;

    // Recorro todos mis vecinos
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            if (i || j) // true siempre que i y j sean distintos de 0 (que seria el caso de ser tu mismo)
                if (matrix[outer_i + i][outer_j + j] == '1')
                    sum++;
        }
    }
    return sum;
}

void find_next_state(int i, int j, int sum)
{
    // Si la célula está viva...
    if (matrix[i][j] == '1')
    {
        // Muere si la suma de sus vecinos vivos es distinta de 2 o 3
        if (sum == 2 || sum == 3)
        {
            next_gen[i][j] = '1';
        }
        else
        {
            next_gen[i][j] = '0';
        }
    }
    else
    { // Célula muerta
        // Sólo se convierte en viva si tiene exactamente 3 vecinos vivos
        if (sum == 3)
        {
            next_gen[i][j] = '1';
        }
        else
        {
            // En cualquier otro caso sigue muerta
            next_gen[i][j] = '0';
        }
    }
}

void calculate_inner()
{
    // Tenemos que quitar la matriz base por los bordes, solo nos interesa la parte que no tiene
    // interacción con otros procesos

    // por que 2? la primera para los intercambios, y la segunda está por fuera

    // el -1 igual. desde 2 hasta <= hago local_n -2 elementos.
    for (int i = 2; i <= local_n_rows - 1; i++)
    {
        for (int j = 2; j <= local_n_cols - 1; j++)
        {
            find_next_state(i, j, neighbourhood_sum(i, j));
        }
    }
}

void calculate_outer()
{
    for (int i = 1; i <= local_n_rows; i++)
    {
        for (int j = 1; j <= local_n_cols; j++)
        {
            // Solo si estamos en los extremos
            if (i == 1 || j == 1 || i == local_n_rows || j == local_n_cols)
            {
                // neighbourhood_sum(i, j);
                find_next_state(i, j, neighbourhood_sum(i, j));
            }
        }
    }
}

void interchange_info(int np_y, int np_x, int left, int right, int top, int bottom, int topright, int topleft, int bottomright, int bottomleft, MPI_Comm comm_grid, int local_n_rows, int local_n_cols)
{
    // printf("FILAS Y COLUMNAS: %d,%d\n", local_n_rows, local_n_cols);
    int row, col = 0;
    // Enviamos y recibimos primera y última columna a nuestros vecinos left y right. Vamos a usar como tag el NUMERO DE FILA

    // multiplicar filas por columnas me ayuda a saber de qué proceso tengo que recoger datos
    MPI_Request rq;

    for (row = 1; row <= local_n_rows; row++)
    {
        MPI_Isend(&(matrix[row][1]), 1, MPI_CHAR, left, row * 1, comm_grid, &rq);
        MPI_Recv(&(matrix[row][local_n_cols + 1]), 1, MPI_CHAR, right, row * 1, comm_grid, MPI_STATUS_IGNORE);

        MPI_Isend(&(matrix[row][local_n_cols]), 1, MPI_CHAR, right, row * local_n_cols, comm_grid, &rq);
        MPI_Recv(&(matrix[row][0]), 1, MPI_CHAR, left, row * local_n_cols, comm_grid, MPI_STATUS_IGNORE);
    }

    // Enviamos y recibimos primera y última fila a nuestros vecinos top y bottom. tag = número de columna
    for (col = 1; col <= local_n_cols; col++)
    {
        MPI_Isend(&(matrix[1][col]), 1, MPI_CHAR, top, col * 1, comm_grid, &rq);
        MPI_Recv(&(matrix[local_n_rows + 1][col]), 1, MPI_CHAR, bottom, col * 1, comm_grid, MPI_STATUS_IGNORE);

        MPI_Isend(&(matrix[local_n_rows][col]), 1, MPI_CHAR, bottom, col * local_n_rows, comm_grid, &rq);
        MPI_Recv(&(matrix[0][col]), 1, MPI_CHAR, top, col * local_n_rows, comm_grid, MPI_STATUS_IGNORE);
    }

    // Faltan las esquinas. Con los vecinos esquina sólo intercambiamos las esquinas

    MPI_Isend(&(matrix[1][1]), 1, MPI_CHAR, topleft, 0, comm_grid, &rq);
    MPI_Isend(&(matrix[1][local_n_cols]), 1, MPI_CHAR, topright, 0, comm_grid, &rq);
    MPI_Isend(&(matrix[local_n_rows][1]), 1, MPI_CHAR, bottomleft, 0, comm_grid, &rq);
    MPI_Isend(&(matrix[local_n_rows][local_n_cols]), 1, MPI_CHAR, bottomright, 0, comm_grid, &rq);

    MPI_Recv(&(matrix[local_n_rows + 1][local_n_cols + 1]), 1, MPI_CHAR, bottomright, MPI_ANY_TAG, comm_grid, MPI_STATUS_IGNORE);
    MPI_Recv(&(matrix[local_n_rows + 1][0]), 1, MPI_CHAR, bottomleft, MPI_ANY_TAG, comm_grid, MPI_STATUS_IGNORE);
    MPI_Recv(&(matrix[0][local_n_cols + 1]), 1, MPI_CHAR, topright, MPI_ANY_TAG, comm_grid, MPI_STATUS_IGNORE);
    MPI_Recv(&(matrix[0][0]), 1, MPI_CHAR, topleft, MPI_ANY_TAG, comm_grid, MPI_STATUS_IGNORE);
}

void print_global(char matrix[N_ROWS][N_COLS])
{
    printf("\n");
    for (int i = 1; i <= N_ROWS; i++)
    {
        for (int j = 1; j <= N_COLS; j++)
        {
            // printf("%c", matrix[i][j]);
            if (matrix[i][j] == '1')
                printf("*");
            else
                printf(" ");
        }
        printf("\n");
    }
}

void draw_board(SDL_Renderer *renderer, char global_matrix[N_ROWS][N_COLS])
{
    int height, width;

    SDL_GetRendererOutputSize(renderer, &width, &height);
    SDL_Rect rectangle;
    Uint8 red_channel, green_channel, blue_channel;

    for (int i = 0; i < height / CELL_SIZE; i++)
    {
        for (int j = 0; j < width / CELL_SIZE; j++)
        {
            if (global_matrix[i][j] == '1')
            {
                red_channel = 255;
                green_channel = 255;
                blue_channel = 255;
            }
            else
            {
                red_channel = 20;
                green_channel = 20;
                blue_channel = 20;
            }

            SDL_SetRenderDrawColor(renderer, red_channel, green_channel, blue_channel, 255);
            rectangle.x = j * CELL_SIZE;
            rectangle.y = i * CELL_SIZE;
            rectangle.h = CELL_SIZE - 1;
            rectangle.w = CELL_SIZE - 1;
            SDL_RenderDrawRect(renderer, &rectangle);
        }
    }
    SDL_RenderPresent(renderer);
    // SDL_Delay(200);
    usleep(200000);
}

void game(MPI_Comm comm_grid, SDL_Renderer *r, int rank, int np_x, int np_y, int normal_cols, int max_cols, int normal_rows, int max_rows)
{
    // matriz para ir guardando los cambios de la siguiente generación sin sobreescribir
    next_gen = allocate_memory(local_n_rows + 2, local_n_cols + 2);

    // vecinos
    int left,
        right, bottom, top, topleft, topright, bottomleft, bottomright;

    char **temp;
    char **global_matrix;

    find_neighbours(comm_grid, rank, np_y, np_x, &left, &right, &bottom, &top, &topleft, &topright, &bottomleft, &bottomright);

    // calculos para los que no se necesitan vecinos (matriz interna sin contar los bordes, es decir,
    //  primera fila y primera columna)
    // game loop
    for (int i = 0; i < MAX_GENERATIONS; i++)
    {

        // printf("En bucle pay\n");
        // Hacemos los cálculos que no tienen dependencias con vecinos
        calculate_inner();
        // printf("calculado inner pay\n");
        // Intercambiar información con nuestros vecinos
        interchange_info(np_y, np_x, left, right, top, bottom, topright, topleft, bottomright, bottomleft, comm_grid, local_n_rows, local_n_cols);
        // Una vez tenemos los valores de nuestros vecinos, calculamos lo que nos queda
        calculate_outer();
        // printf("NO SALE BIEN PAY\n");

        char buf[local_n_rows * local_n_cols];
        fill_buf(local_n_rows, local_n_cols, buf, rank, comm_grid);
        MPI_Request rq;
        MPI_Isend(buf, local_n_rows * local_n_cols, MPI_CHAR, 0, local_n_rows * local_n_cols, comm_grid, &rq);
        if (rank == 0)
        {
            char buffer[max_rows * max_cols];
            MPI_Status st;
            int elements;
            char global_matrix[N_ROWS][N_COLS];
            int size;
            MPI_Comm_size(comm_grid, &size);

            int del_rows;
            int del_cols;

            int pos[2]; // y,x
            int x, y;

            for (int a = 0; a < size; a++)
            {
                MPI_Recv(buffer, max_rows * max_cols, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, comm_grid, &st);
                elements = st.MPI_TAG;
                MPI_Cart_coords(comm_grid, st.MPI_SOURCE, 2, pos);

                y = pos[0] * normal_rows;
                x = pos[1] * normal_cols;

                if (elements == (normal_rows * normal_cols))
                {
                    del_rows = y + normal_rows;
                    del_cols = x + normal_cols;
                }
                else
                {
                    del_rows = y + max_rows;
                    del_cols = x + max_cols;
                }
                int index = 0;
                // printf("rank:%d, del_rows:%d, del_cols:%d, x:%d, y:%d\n", st.MPI_SOURCE, del_rows, del_cols, x, y);
                for (int j = y; j < del_rows; j++)
                {
                    for (int k = x; k < del_cols; k++)
                    {
                        global_matrix[j][k] = buffer[index++];
                    }
                }
            }

            // print_global(global_matrix);
            // printf("\n\n\n\n\n\n");
            draw_board(r, global_matrix);
        }

        temp = matrix;
        matrix = next_gen;
        next_gen = temp;

        MPI_Barrier(comm_grid);
    }
}
