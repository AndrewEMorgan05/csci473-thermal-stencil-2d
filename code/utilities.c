#define _POSIX_C_SOURCE 200809L
#include "utilities.h"
#include <time.h>
#include <unistd.h>

double *allocate_grid(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    return (double *)calloc((size_t)rows * (size_t)cols, sizeof(double));
}

void free_grid(double *grid) {
    free(grid);
}

/* Binary format: [int rows][int cols][rows * cols doubles] */
int read_grid_file(const char *filename, int *rows, int *cols, double **grid) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening input file");
        return -1;
    }

    if (fread(rows, sizeof(int), 1, fp) != 1 ||
        fread(cols, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error reading grid dimensions from %s\n", filename);
        fclose(fp);
        return -1;
    }

    if (*rows <= 0 || *cols <= 0) {
        fprintf(stderr, "Invalid grid dimensions (%d x %d) in %s\n", *rows, *cols, filename);
        fclose(fp);
        return -1;
    }

    *grid = allocate_grid(*rows, *cols);
    if (!*grid) {
        fprintf(stderr, "Memory allocation failed for %d x %d grid\n", *rows, *cols);
        fclose(fp);
        return -1;
    }

    size_t total = (size_t)(*rows) * (size_t)(*cols);
    if (fread(*grid, sizeof(double), total, fp) != total) {
        fprintf(stderr, "Error reading grid values from %s\n", filename);
        free_grid(*grid);
        *grid = NULL;
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int write_grid_file(const char *filename, int rows, int cols, const double *grid) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening output file");
        return -1;
    }

    if (fwrite(&rows, sizeof(int), 1, fp) != 1 ||
        fwrite(&cols, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error writing grid dimensions to %s\n", filename);
        fclose(fp);
        return -1;
    }

    size_t total = (size_t)rows * (size_t)cols;
    if (fwrite(grid, sizeof(double), total, fp) != total) {
        fprintf(stderr, "Error writing grid data to %s\n", filename);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

void print_grid_formatted(const double *grid, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%6.2f", grid[IDX(i, j, cols)]);
            if (j < cols - 1) {
                printf(" | ");
            }
        }
        printf("\n");
    }
}

double get_wall_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void print_stencil_usage(const char *prog_name, int require_workers) {
    if (require_workers) {
        fprintf(stderr, "Usage: %s -t <num_iters> -i <in> -o <out> -p <workers> [-v <0|1|2>]\n", prog_name);
    } else {
        fprintf(stderr, "Usage: %s -t <num_iters> -i <in> -o <out> [-v <0|1|2>]\n", prog_name);
    }
}

int parse_stencil_args(int argc, char **argv, StencilConfig *cfg, int require_workers) {
    cfg->iterations = -1;
    cfg->input_file = NULL;
    cfg->output_file = NULL;
    cfg->verbose = 0;
    cfg->workers = 1;

    int opt;
    while ((opt = getopt(argc, argv, "t:n:i:o:v:p:")) != -1) {
        switch (opt) {
            case 't':
            case 'n':
                cfg->iterations = atoi(optarg);
                break;
            case 'i':
                cfg->input_file = optarg;
                break;
            case 'o':
                cfg->output_file = optarg;
                break;
            case 'v':
                cfg->verbose = atoi(optarg);
                break;
            case 'p':
                cfg->workers = atoi(optarg);
                break;
            default:
                print_stencil_usage(argv[0], require_workers);
                return -1;
        }
    }

    if (cfg->iterations < 0 || !cfg->input_file || !cfg->output_file) {
        print_stencil_usage(argv[0], require_workers);
        return -1;
    }
    if (require_workers && cfg->workers <= 0) {
        fprintf(stderr, "Error: -p <workers> must be >= 1\n");
        return -1;
    }
    if (cfg->verbose < 0 || cfg->verbose > 2) {
        fprintf(stderr, "Error: -v must be 0, 1, or 2\n");
        return -1;
    }
    return 0;
}