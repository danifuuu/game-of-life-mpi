#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_render.h>

void find_neighbours(MPI_Comm comm_grid, int my_rank, int np_y, int np_x, int *left, int *right, int *top, int *bottom, int *topleft, int *topright, int *bottomleft, int *bottomright);

int neighbourhood_sum(int outer_i, int outer_j);

void find_next_state(int i, int j, int sum);

void calculate_inner();

void calculate_outer();

void interchange_info(int np_y, int np_x, int left, int right, int top, int bottom, int topright, int topleft, int bottomright, int bottomleft, MPI_Comm comm_grid, int local_n_rows, int local_n_cols);

void game(MPI_Comm comm_grid, SDL_Renderer *r, int rank, int np_x, int np_y, int normal_cols, int max_cols, int normal_rows, int max_rows);