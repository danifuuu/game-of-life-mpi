#include <unistd.h>

#include "misc_header.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_render.h>

void find_neighbours(MPI_Comm comm_grid, int my_rank, int np_y, int np_x, int *left, int *right, int *top, int *bottom, int *topleft, int *topright, int *bottomleft, int *bottomright);

void fill_buf(int rows, int cols, char *buffer);

int neighbourhood_sum(int outer_i, int outer_j);

void find_next_state(int i, int j, int sum);

void calculate_inner();

void calculate_outer();

void interchange_info(int np_y, int np_x, int left, int right, int top, int bottom, int topright, int topleft, int bottomright, int bottomleft, MPI_Comm comm_grid, int local_n_rows, int local_n_cols);

void print_global(char matrix[N_ROWS][N_COLS]);

void draw_board(SDL_Renderer *renderer, char global_matrix[N_ROWS][N_COLS]);

void game(MPI_Comm comm_grid, SDL_Renderer *r, int rank, int np_x, int np_y, int n_rows, int n_cols);