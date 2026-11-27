CC = gcc
MPICC = mpicc
CFLAGS = -O2 -g -Wall -Wextra -Wstrict-prototypes -std=gnu99
OMPFLAGS = -fopenmp
PTHFLAGS = -pthread
LFLAGS = -lm

PROGS = print-2d make-2d stencil-2d stencil-2d-pth stencil-2d-omp stencil-2d-mpi stencil-2d-hybrid

.PHONY: all clean

all: $(PROGS)

utilities.o: utilities.c utilities.h
	$(CC) $(CFLAGS) -c utilities.c

print-2d: utilities.o print-2d.c
	$(CC) $(CFLAGS) -o print-2d print-2d.c utilities.o $(LFLAGS)

make-2d: utilities.o make-2d.c
	$(CC) $(CFLAGS) -o make-2d make-2d.c utilities.o $(LFLAGS)

stencil-2d: utilities.o stencil-2d.c
	$(CC) $(CFLAGS) -o stencil-2d stencil-2d.c utilities.o $(LFLAGS)

stencil-2d-pth: stencil-2d-pth.c utilities.c utilities.h
	$(CC) $(CFLAGS) $(PTHFLAGS) -o stencil-2d-pth stencil-2d-pth.c utilities.c $(LFLAGS)

stencil-2d-omp: stencil-2d-omp.c utilities.c utilities.h
	$(CC) $(CFLAGS) $(OMPFLAGS) -o stencil-2d-omp stencil-2d-omp.c utilities.c $(LFLAGS)

stencil-2d-mpi: stencil-2d-mpi.c utilities.c utilities.h
	$(MPICC) $(CFLAGS) -o stencil-2d-mpi stencil-2d-mpi.c utilities.c $(LFLAGS)

stencil-2d-hybrid: stencil-2d-hybrid.c utilities.c utilities.h
	$(MPICC) $(CFLAGS) $(OMPFLAGS) -o stencil-2d-hybrid stencil-2d-hybrid.c utilities.c $(LFLAGS)

clean:
	rm -f *.o core* $(PROGS)