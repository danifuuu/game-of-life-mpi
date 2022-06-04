#ifndef MISC_HEADER_H
#define MISC_HEADER_H

#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_render.h>

extern char **matrix;
extern int local_n_rows;
extern int local_n_cols;

#define N_ROWS 64
#define N_COLS 64
#define CELL_SIZE 10
#define MAX_GENERATIONS 2000

char **allocate_memory(int rows, int cols);

#endif
