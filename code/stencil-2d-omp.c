#include "utilities.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double stencil9(const double *grid, int i, int j, int cols) {
    return (grid[IDX(i - 1, j - 1, cols)] +
            grid[IDX(i - 1, j,     cols)] +
            grid[IDX(i - 1, j + 1, cols)] +
            grid[IDX(i,     j + 1, cols)] +
            grid[IDX(i + 1, j + 1, cols)] +
            grid[IDX(i + 1, j,     cols)] +
            grid[IDX(i + 1, j - 1, cols)] +
            grid[IDX(i,     j - 1, cols)] +
            grid[IDX(i,     j,     cols)]) / 9.0;
}

int main(int argc, char **argv) {
    StencilConfig cfg;
    if (parse_stencil_args(argc, argv, &cfg, 1) != 0) {
        return 1;
    }

    struct timespec total_start, total_end, comp_start, comp_end;
    clock_gettime(CLOCK_MONOTONIC, &total_start);

    int rows = 0, cols = 0;
    double *init_grid = NULL;
    if (read_grid_file(cfg.input_file, &rows, &cols, &init_grid) != 0) {
        return 1;
    }

    if (rows < 3 || cols < 3) {
        fprintf(stderr, "Error: grid must be at least 3x3\n");
        free_grid(init_grid);
        return 1;
    }

    int threads = 0;
    if (cfg.workers > 0) {
        threads = cfg.workers;
        omp_set_dynamic(0);
        omp_set_num_threads(threads);
    }

    double *a = allocate_grid(rows, cols);
    double *b = allocate_grid(rows, cols);
    if (!a || !b) {
        fprintf(stderr, "Error: failed to allocate working grids\n");
        free_grid(a);
        free_grid(b);
        free_grid(init_grid);
        return 1;
    }

    size_t grid_bytes = (size_t)rows * (size_t)cols * sizeof(double);
    memcpy(a, init_grid, grid_bytes);
    memcpy(b, init_grid, grid_bytes);
    free_grid(init_grid);

    int used_threads = 1;

    clock_gettime(CLOCK_MONOTONIC, &comp_start);
    for (int iter = 0; iter < cfg.iterations; iter++) {
        #pragma omp parallel for schedule(static)
        for (int i = 1; i < rows - 1; i++) {
            b[IDX(i, 0, cols)] = a[IDX(i, 0, cols)];
            b[IDX(i, cols - 1, cols)] = a[IDX(i, cols - 1, cols)];

            for (int j = 1; j < cols - 1; j++) {
                b[IDX(i, j, cols)] = stencil9(a, i, j, cols);
            }
        }

        if (iter == 0) {
            #pragma omp parallel
            {
                #pragma omp single
                used_threads = omp_get_num_threads();
            }
        }

        double *tmp = a;
        a = b;
        b = tmp;

        if (cfg.verbose == 2) {
            printf("Iteration %d\n", iter + 1);
            print_grid_formatted(a, rows, cols);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &comp_end);

    if (cfg.iterations == 0 && cfg.workers > 0) {
        used_threads = cfg.workers;
    }

    if (write_grid_file(cfg.output_file, rows, cols, a) != 0) {
        free_grid(a);
        free_grid(b);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &total_end);

    double t_comp = elapsed_seconds(&comp_start, &comp_end);
    double t_total = elapsed_seconds(&total_start, &total_end);
    double t_other = t_total - t_comp;

    if (cfg.verbose >= 1) {
        printf("[omp] threads=%d grid=%dx%d iterations=%d T_total=%.6f s T_computation=%.6f s T_other=%.6f s\n",
               used_threads, rows, cols, cfg.iterations, t_total, t_comp, t_other);
    }

    free_grid(a);
    free_grid(b);
    return 0;
}