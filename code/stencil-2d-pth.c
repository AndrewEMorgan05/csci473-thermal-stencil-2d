#include "utilities.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int tid;
    int num_threads;
    int rows;
    int cols;
    int iterations;
    int verbose;
    double **a;
    double **b;
    pthread_barrier_t *barrier;
} ThreadArgs;

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

static void *worker_fn(void *arg) {
    ThreadArgs *ctx = (ThreadArgs *)arg;
    int interior = ctx->rows - 2;
    int row_start = 1 + BLOCK_LOW(ctx->tid, ctx->num_threads, interior);
    int row_end   = 1 + BLOCK_LOW(ctx->tid + 1, ctx->num_threads, interior);

    for (int iter = 0; iter < ctx->iterations; iter++) {
        double *src = *ctx->a;
        double *dst = *ctx->b;

        for (int i = row_start; i < row_end; i++) {
            dst[IDX(i, 0, ctx->cols)] = src[IDX(i, 0, ctx->cols)];
            dst[IDX(i, ctx->cols - 1, ctx->cols)] = src[IDX(i, ctx->cols - 1, ctx->cols)];

            for (int j = 1; j < ctx->cols - 1; j++) {
                dst[IDX(i, j, ctx->cols)] = stencil9(src, i, j, ctx->cols);
            }
        }

        pthread_barrier_wait(ctx->barrier);

        if (ctx->tid == 0) {
            double *tmp = *ctx->a;
            *ctx->a = *ctx->b;
            *ctx->b = tmp;

            if (ctx->verbose == 2) {
                printf("Iteration %d\n", iter + 1);
                print_grid_formatted(*ctx->a, ctx->rows, ctx->cols);
            }
        }

        pthread_barrier_wait(ctx->barrier);
    }

    return NULL;
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

    int num_threads = cfg.workers > 0 ? cfg.workers : 1;
    int interior = rows - 2;
    if (num_threads > interior) {
        num_threads = interior;
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

    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    ThreadArgs *args = malloc((size_t)num_threads * sizeof(ThreadArgs));
    pthread_barrier_t barrier;

    if (!threads || !args || pthread_barrier_init(&barrier, NULL, (unsigned)num_threads) != 0) {
        fprintf(stderr, "Error: failed to initialize Pthreads resources\n");
        free(threads);
        free(args);
        free_grid(a);
        free_grid(b);
        return 1;
    }

    double *cur_a = a;
    double *cur_b = b;

    clock_gettime(CLOCK_MONOTONIC, &comp_start);
    for (int t = 0; t < num_threads; t++) {
        args[t].tid = t;
        args[t].num_threads = num_threads;
        args[t].rows = rows;
        args[t].cols = cols;
        args[t].iterations = cfg.iterations;
        args[t].verbose = cfg.verbose;
        args[t].a = &cur_a;
        args[t].b = &cur_b;
        args[t].barrier = &barrier;

        if (pthread_create(&threads[t], NULL, worker_fn, &args[t]) != 0) {
            fprintf(stderr, "Error: pthread_create failed on thread %d\n", t);
            return 1;
        }
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &comp_end);

    pthread_barrier_destroy(&barrier);
    free(threads);
    free(args);

    if (write_grid_file(cfg.output_file, rows, cols, cur_a) != 0) {
        free_grid(a);
        free_grid(b);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &total_end);

    double t_comp = elapsed_seconds(&comp_start, &comp_end);
    double t_total = elapsed_seconds(&total_start, &total_end);
    double t_other = t_total - t_comp;

    if (cfg.verbose >= 1) {
        printf("[pth] threads=%d grid=%dx%d iterations=%d T_total=%.6f s T_computation=%.6f s T_other=%.6f s\n",
               num_threads, rows, cols, cfg.iterations, t_total, t_comp, t_other);
    }

    free_grid(a);
    free_grid(b);
    return 0;
}