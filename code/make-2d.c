#include "utilities.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <rows> <cols> <output_file>\n", argv[0]);
        return 1;
    }

    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    const char *out_file = argv[3];

    if (rows < 3 || cols < 3) {
        fprintf(stderr, "Error: rows and cols must be at least 3\n");
        return 1;
    }

    double *grid = allocate_grid(rows, cols);
    if (!grid) {
        fprintf(stderr, "Error: failed to allocate %d x %d grid\n", rows, cols);
        return 1;
    }

    /* Left and right walls = 1.0; top/bottom walls = 0.0; interior = 0.0 */
    for (int i = 0; i < rows; i++) {
        grid[IDX(i, 0, cols)] = 1.0;
        grid[IDX(i, cols - 1, cols)] = 1.0;
    }

    for (int j = 0; j < cols; j++) {
        grid[IDX(0, j, cols)] = 0.0;
        grid[IDX(rows - 1, j, cols)] = 0.0;
    }

    if (write_grid_file(out_file, rows, cols, grid) != 0) {
        free_grid(grid);
        return 1;
    }

    free_grid(grid);
    return 0;
}