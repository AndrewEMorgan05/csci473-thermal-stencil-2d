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
