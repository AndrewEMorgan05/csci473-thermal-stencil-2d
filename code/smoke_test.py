import pathlib,subprocess
from check_correctness import compare
root=pathlib.Path(__file__).resolve().parent
def run(args):subprocess.run(args,cwd=root,check=True)
for n in [5,16]:
    inp=f'test_{n}.dat';ref=f'test_serial_{n}.dat'
    run(['./make-2d',str(n),str(n),inp])
    run(['./stencil-2d','-t','10','-i',inp,'-o',ref])
    for ranks,threads,mode in [(1,1,'mpi'),(2,1,'mpi'),(1,1,'hybrid'),(1,2,'hybrid'),(2,1,'hybrid'),(2,2,'hybrid')]:
        target=f'test_{mode}_{n}_{ranks}_{threads}.dat'
        cmd=['mpiexec','-n',str(ranks),f'./stencil-2d-{mode}','-t','10','-i',inp,'-o',target]
        if mode=='hybrid':cmd+=['-p',str(threads)]
        run(cmd)
        diff,edges,ok=compare(root/ref,root/target)
        print(n,mode,ranks,threads,'PASS' if ok else 'FAIL',diff,flush=True)
        if not ok:raise SystemExit(1)
