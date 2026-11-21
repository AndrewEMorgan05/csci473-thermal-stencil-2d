CC = gcc
CFLAGS = -O2 -g -Wall -Wextra -Wstrict-prototypes -std=gnu99
LFLAGS = -lm

PROGS = print-2d make-2d stencil-2d

all: $(PROGS)

utilities.o: utilities.c utilities.h
	$(CC) $(CFLAGS) -c utilities.c

print-2d: utilities.o print-2d.c
	$(CC) $(CFLAGS) -o print-2d print-2d.c utilities.o $(LFLAGS)

make-2d: utilities.o make-2d.c
	$(CC) $(CFLAGS) -o make-2d make-2d.c utilities.o $(LFLAGS)

stencil-2d: utilities.o stencil-2d.c
	$(CC) $(CFLAGS) -o stencil-2d stencil-2d.c utilities.o $(LFLAGS)

clean:
	rm -f *.o core* $(PROGS)