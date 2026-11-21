#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>

/* Quinn Block Decomposition Macros (1D partitioning of n items across p workers) */
#define BLOCK_LOW(id, p, n)   ((id) * (n) / (p))
#define BLOCK_HIGH(id, p, n)  (BLOCK_LOW((id) + 1, (p), (n)) - 1)
#define BLOCK_SIZE(id, p, n)  (BLOCK_HIGH((id), (p), (n)) - BLOCK_LOW((id), (p), (n)) + 1)
#define BLOCK_OWNER(j, p, n)  (((p) * ((j) + 1) - 1) / (n))

/* Flat 2D indexing helper */
#define IDX(i, j, cols) ((size_t)(i) * (size_t)(cols) + (size_t)(j))

typedef struct {
    int iterations;
    char *input_file;
    char *output_file;
    int verbose;
    int workers;
} StencilConfig;

double *allocate_grid(int rows, int cols);
void free_grid(double *grid);

int read_grid_file(const char *filename, int *rows, int *cols, double **grid);
int write_grid_file(const char *filename, int rows, int cols, const double *grid);
void print_grid_formatted(const double *grid, int rows, int cols);

double get_wall_time(void);
int parse_stencil_args(int argc, char **argv, StencilConfig *cfg, int require_workers);
void print_stencil_usage(const char *prog_name, int require_workers);

#endif /* UTILITIES_H */