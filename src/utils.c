#include <stdlib.h>
#include "misc_header.h"

char **allocate_memory(int rows, int cols)
{
    char **matrix;
    matrix = (char **)malloc(rows * sizeof(char *));
    for (int i = 0; i < rows; i++)
        matrix[i] = (char *)malloc(cols * sizeof(char));

    return matrix;
}
