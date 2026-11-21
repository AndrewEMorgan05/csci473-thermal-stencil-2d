#include "utilities.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <input data file>\n", argv[0]);
        return 1;
    }

    int rows = 0, cols = 0;
    double *grid = NULL;

    if (read_grid_file(argv[1], &rows, &cols, &grid) != 0) {
        return 1;
    }

    printf("%s, %s, (%d x %d)\n", argv[0], argv[1], rows, cols);
    print_grid_formatted(grid, rows, cols);

    free_grid(grid);
    return 0;
}