CSCI 473 - 2D Thermal Diffusion 9-Point Stencil
Team: Andrew E. Morgan, Byron

Build:
  cd code
  make

Clean:
  cd code
  make clean

Run Serial Baseline:
  ./make-2d 16 16 ../data/init_16x16.dat
  ./stencil-2d -t 25 -i ../data/init_16x16.dat -o ../data/out_16x16.dat -v 1
  ./print-2d ../data/out_16x16.dat


make-2d.c (make-2d): Generates initial binary .dat grids with fixed left/right heat walls (1.0) and 0.0 top/bottom/interior values for any specified row and column dimensions.

print-2d.c (print-2d): Reads a binary .dat grid file and prints its formatted temperature values to the terminal so you can inspect initial or output grids.

stencil-2d.c (stencil-2d): Runs the single-threaded serial 9-point stencil heat diffusion simulation for a given number of iterations while preserving fixed boundaries and supporting -v 0/1/2 output modes.

stencil-2d-pth.c (stencil-2d-pth): Implements the shared-memory Pthreads version (-p <threads>), partitioning interior rows with BLOCK_LOW and synchronizing iterations using a pthread_barrier_t.

stencil-2d-omp.c (stencil-2d-omp): Implements the shared-memory OpenMP version (-p <threads> or OMP_NUM_THREADS), parallelizing interior row updates across threads with double buffering.

stencil-2d-mpi.c (stencil-2d-mpi): Implements the distributed-memory MPI version (mpirun -np <ranks>), scattering interior rows across ranks, exchanging top/bottom ghost rows via MPI_Sendrecv, and gathering results back to rank 0.

stencil-2d-hybrid.c (stencil-2d-hybrid): Combines MPI ranks with OpenMP threads (-p <threads>) to compute row blocks across ranks and thread loops locally within each rank.

utilities.h & utilities.c: Provide shared CLI argument parsing, binary .dat grid file read/write helpers, grid memory allocation/freeing, wall-clock timing helpers, formatted grid printing, and IDX / BLOCK_LOW macros.

Makefile: Builds all seven C executables (print-2d, make-2d, stencil-2d, stencil-2d-pth, stencil-2d-omp, stencil-2d-mpi, stencil-2d-hybrid) and cleans build artifacts via make clean.

check_correctness.py: Compares a candidate .dat output against a reference .dat file to verify matching grid dimensions, exact boundary values, and max absolute difference within tolerance (default 1e-9).

smoke_test.py: Runs quick automated correctness checks on small grids (5x5 and 16x16) comparing parallel outputs against the serial baseline.

benchmark.py: Runs repeated timed benchmarks across grid sizes and worker configurations, checks correctness against serial as it runs, and writes T_total, T_computation, and T_other to CSV.

plot_results.py: Reads the benchmark CSV to generate overhead breakdown charts, speedup charts, efficiency charts, runtime-vs-size plots, and grid heatmaps.

data/: Holds binary .dat input/output grids and benchmark CSV output (ignored by Git).

presentation/: Stores the 10-minute presentation slides (PDF) and demo materials for submission.