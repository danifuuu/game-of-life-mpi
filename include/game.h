void find_neighbours(MPI_Comm comm_grid, int my_rank, int np_y, int np_x, int *left, int *right, int *top, int *bottom, int *topleft, int *topright, int *bottomleft, int *bottomright);

int neighbourhood_sum(int outer_i, int outer_j);

void find_next_state(int i, int j, int sum);

void calculate_inner();

void calculate_outer();

void interchange_info(int np_y, int np_x, int left, int right, int top, int bottom, int topright, int topleft, int bottomright, int bottomleft, MPI_Comm comm_grid);

void game(MPI_Comm comm_grid, int rank, int np_x, int np_y, int MAX_GENERATIONS);