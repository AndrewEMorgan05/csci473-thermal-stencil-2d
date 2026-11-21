#include "utilities.h"

static void copy_boundaries(const double *src, double *dst, int rows, int cols) {
    for (int j = 0; j < cols; j++) {
        dst[IDX(0, j, cols)] = src[IDX(0, j, cols)];
        dst[IDX(rows - 1, j, cols)] = src[IDX(rows - 1, j, cols)];
    }
    for (int i = 0; i < rows; i++) {
        dst[IDX(i, 0, cols)] = src[IDX(i, 0, cols)];
        dst[IDX(i, cols - 1, cols)] = src[IDX(i, cols - 1, cols)];
    }
}

int main(int argc, char **argv) {
    StencilConfig cfg;
    if (parse_stencil_args(argc, argv, &cfg, 0) != 0) {
        return 1;
    }

    double t_start = get_wall_time();

    int rows = 0, cols = 0;
    double *x = NULL;
    if (read_grid_file(cfg.input_file, &rows, &cols, &x) != 0) {
        return 1;
    }

    double *newx = allocate_grid(rows, cols);
    if (!newx) {
        fprintf(stderr, "Error: failed to allocate buffer grid\n");
        free_grid(x);
        return 1;
    }
    copy_boundaries(x, newx, rows, cols);

    if (cfg.verbose >= 1) {
        printf("[serial] grid=%dx%d, iterations=%d, in=%s, out=%s\n",
               rows, cols, cfg.iterations, cfg.input_file, cfg.output_file);
    }
    if (cfg.verbose == 2) {
        printf("Iteration 0\n");
        print_grid_formatted(x, rows, cols);
        printf("\n");
    }

    double t_comp_start = get_wall_time();

    for (int iter = 1; iter <= cfg.iterations; iter++) {
        for (int i = 1; i < rows - 1; i++) {
            for (int j = 1; j < cols - 1; j++) {
                newx[IDX(i, j, cols)] =
                    (x[IDX(i - 1, j - 1, cols)] + x[IDX(i - 1, j, cols)] + x[IDX(i - 1, j + 1, cols)] +
                     x[IDX(i,     j + 1, cols)] + x[IDX(i + 1, j + 1, cols)] + x[IDX(i + 1, j, cols)] +
                     x[IDX(i + 1, j - 1, cols)] + x[IDX(i,     j - 1, cols)] + x[IDX(i,     j, cols)]) / 9.0;
            }
        }

        double *tmp = x;
        x = newx;
        newx = tmp;

        if (cfg.verbose == 2) {
            printf("Iteration %d\n", iter);
            print_grid_formatted(x, rows, cols);
            printf("\n");
        }
    }

    double t_comp_end = get_wall_time();

    if (write_grid_file(cfg.output_file, rows, cols, x) != 0) {
        free_grid(x);
        free_grid(newx);
        return 1;
    }

    double t_end = get_wall_time();

    if (cfg.verbose >= 1) {
        double t_total = t_end - t_start;
        double t_comp = t_comp_end - t_comp_start;
        double t_other = t_total - t_comp;
        printf("[serial] T_total=%.6f s, T_computation=%.6f s, T_other=%.6f s\n",
               t_total, t_comp, t_other);
    }

    free_grid(x);
    free_grid(newx);
    return 0;
}