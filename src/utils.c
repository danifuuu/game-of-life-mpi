#include <stdlib.h>

char **allocate_memory(int rows, int cols)
{
    char **matrix;
    matrix = (char **)malloc(rows * sizeof(char *));
    for (int i = 0; i < rows; i++)
        matrix[i] = (char *)malloc(cols * sizeof(char));

    return matrix;
}

int get_max(int *values, int n)
{
    int max = values[0];

    for (int i = 1; i < n; i++)
    {
        if (values[i] > max)
            max = values[i];
    }

    return max;
}