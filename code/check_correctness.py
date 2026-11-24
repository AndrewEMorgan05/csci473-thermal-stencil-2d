import argparse, struct, subprocess, pathlib
import numpy as np

def read_grid(path):
    with open(path,'rb') as f:
        rows,cols=struct.unpack('ii',f.read(8))
        data=np.fromfile(f,dtype=np.float64,count=rows*cols)
    if data.size!=rows*cols: raise ValueError('Incomplete grid')
    return data.reshape(rows,cols)

def compare(reference,candidate,tol=1e-9):
    a,b=read_grid(reference),read_grid(candidate)
    if a.shape!=b.shape: raise ValueError(f'Shape mismatch: {a.shape} != {b.shape}')
    difference=float(np.max(np.abs(a-b)))
    edges=np.zeros(a.shape,dtype=bool);edges[[0,-1],:]=True;edges[:,[0,-1]]=True
    boundary_ok=bool(np.array_equal(a[edges],b[edges]))
    return difference,boundary_ok,difference<=tol and boundary_ok

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('reference');p.add_argument('candidate');p.add_argument('--tolerance',type=float,default=1e-9)
    args=p.parse_args();diff,edges,ok=compare(args.reference,args.candidate,args.tolerance)
    print(f'max_abs_difference={diff:.12g} boundaries_exact={edges} result={"PASS" if ok else "FAIL"}')
    raise SystemExit(0 if ok else 1)
