#include "utilities.h"
#include <mpi.h>
#include <string.h>
#include <omp.h>

static double stencil(const double *a, int i, int j, int cols) {
    return (a[IDX(i-1,j-1,cols)] + a[IDX(i-1,j,cols)] + a[IDX(i-1,j+1,cols)] +
            a[IDX(i,j+1,cols)] + a[IDX(i+1,j+1,cols)] + a[IDX(i+1,j,cols)] +
            a[IDX(i+1,j-1,cols)] + a[IDX(i,j-1,cols)] + a[IDX(i,j,cols)]) / 9.0;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size; MPI_Comm_rank(MPI_COMM_WORLD,&rank); MPI_Comm_size(MPI_COMM_WORLD,&size);
    StencilConfig cfg; int ok = parse_stencil_args(argc,argv,&cfg,1)==0;
    int all_ok; MPI_Allreduce(&ok,&all_ok,1,MPI_INT,MPI_MIN,MPI_COMM_WORLD);
    if (!all_ok) { MPI_Finalize(); return 1; }
    double start=MPI_Wtime(), comp=0;
    int rows=0,cols=0; double *global=NULL;
    if(rank==0 && read_grid_file(cfg.input_file,&rows,&cols,&global)!=0) ok=0;
    MPI_Bcast(&ok,1,MPI_INT,0,MPI_COMM_WORLD);
    if(!ok){MPI_Finalize();return 1;}
    MPI_Bcast(&rows,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&cols,1,MPI_INT,0,MPI_COMM_WORLD);
    if(rows<3 || cols<3 || size>rows-2) {
        if(rank==0) fprintf(stderr,"Error: grid must be >=3x3 and MPI ranks <= interior rows\n");
        if(rank==0) free_grid(global);
        MPI_Finalize(); return 1;
    }
    int *counts=malloc((size_t)size*sizeof(int)), *displs=malloc((size_t)size*sizeof(int));
    int interior=rows-2;
    for(int r=0;r<size;r++) {
        int low=BLOCK_LOW(r,size,interior), high=BLOCK_LOW(r+1,size,interior);
        counts[r]=(high-low)*cols; displs[r]=(low+1)*cols;
    }
    int localrows=counts[rank]/cols;
    int threads=cfg.workers;
    if(threads<1) threads=1;
    omp_set_dynamic(0); omp_set_num_threads(threads);
    double *a=allocate_grid(localrows+2,cols), *b=allocate_grid(localrows+2,cols);
    if(!counts || !displs || !a || !b) MPI_Abort(MPI_COMM_WORLD,2);
    MPI_Scatterv(global,counts,displs,MPI_DOUBLE,a+cols,localrows*cols,MPI_DOUBLE,0,MPI_COMM_WORLD);
    /* Fixed left/right walls for both buffers. */
    for(int i=1;i<=localrows;i++) {
        b[IDX(i,0,cols)]=a[IDX(i,0,cols)];
        b[IDX(i,cols-1,cols)]=a[IDX(i,cols-1,cols)];
    }
    if(rank==0) {
        memcpy(a,global,(size_t)cols*sizeof(double));
        memcpy(b,a,(size_t)cols*sizeof(double));
    }
    if(rank==size-1) {
        memcpy(a+(size_t)(localrows+1)*cols,global+(size_t)(rows-1)*cols,(size_t)cols*sizeof(double));
        memcpy(b+(size_t)(localrows+1)*cols,a+(size_t)(localrows+1)*cols,(size_t)cols*sizeof(double));
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double cs=MPI_Wtime();
    for(int t=0;t<cfg.iterations;t++) {
        MPI_Sendrecv(a+cols,cols,MPI_DOUBLE,rank-1>=0?rank-1:MPI_PROC_NULL,10,
                     a,cols,MPI_DOUBLE,rank-1>=0?rank-1:MPI_PROC_NULL,11,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Sendrecv(a+(size_t)localrows*cols,cols,MPI_DOUBLE,rank+1<size?rank+1:MPI_PROC_NULL,11,
                     a+(size_t)(localrows+1)*cols,cols,MPI_DOUBLE,rank+1<size?rank+1:MPI_PROC_NULL,10,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        #pragma omp parallel for schedule(static)
        for(int i=1;i<=localrows;i++) for(int j=1;j<cols-1;j++) b[IDX(i,j,cols)]=stencil(a,i,j,cols);
        double *tmp=a;a=b;b=tmp;
        if(cfg.verbose==2) {
            MPI_Gatherv(a+cols,localrows*cols,MPI_DOUBLE,global,counts,displs,MPI_DOUBLE,0,MPI_COMM_WORLD);
            if(rank==0){printf("Iteration %d\n",t+1);print_grid_formatted(global,rows,cols);}
        }
    }
    comp=MPI_Wtime()-cs;
    MPI_Gatherv(a+cols,localrows*cols,MPI_DOUBLE,global,counts,displs,MPI_DOUBLE,0,MPI_COMM_WORLD);
    if(rank==0) ok=write_grid_file(cfg.output_file,rows,cols,global)==0;
    MPI_Bcast(&ok,1,MPI_INT,0,MPI_COMM_WORLD);
    double total=MPI_Wtime()-start, maxcomp, maxtotal;
    MPI_Reduce(&comp,&maxcomp,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
    MPI_Reduce(&total,&maxtotal,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
    if(rank==0 && cfg.verbose>=1) printf("[hybrid] ranks=%d threads=%d grid=%dx%d iterations=%d T_total=%.6f s T_computation=%.6f s T_other=%.6f s\n",size,threads,rows,cols,cfg.iterations,maxtotal,maxcomp,maxtotal-maxcomp);
    free_grid(a);free_grid(b);free_grid(global);free(counts);free(displs);
    MPI_Finalize();return ok?0:1;
}
