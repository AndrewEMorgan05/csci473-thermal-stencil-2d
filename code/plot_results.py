import argparse, pathlib
import pandas as pd
import matplotlib.pyplot as plt
p=argparse.ArgumentParser();p.add_argument('csv');p.add_argument('--out',default='../plots');a=p.parse_args()
out=pathlib.Path(a.out);out.mkdir(parents=True,exist_ok=True)
df=pd.read_csv(a.csv)
g=df.groupby(['implementation','size','ranks','threads','workers'],as_index=False)[['T_total','T_computation','T_other']].mean()
g['label']=g.apply(lambda r: f"{r['implementation']} {int(r['ranks'])}x{int(r['threads'])}",axis=1)
for size,sub in g.groupby('size'):
    fig,ax=plt.subplots(figsize=(11,5));x=range(len(sub))
    ax.bar(x,sub.T_computation,label='Computation');ax.bar(x,sub.T_other,bottom=sub.T_computation,label='Other')
    ax.set_xticks(list(x),sub.label,rotation=35,ha='right');ax.set_ylabel('Seconds');ax.set_title(f'Time breakdown: {size}x{size}');ax.legend();fig.tight_layout();fig.savefig(out/f'overhead_{size}.png');plt.close(fig)
    baseline=float(sub[sub.implementation=='serial'].T_total.iloc[0]);parallel=sub[sub.implementation!='serial'].copy()
    parallel['speedup']=baseline/parallel.T_total;parallel['efficiency']=parallel.speedup/parallel.workers
    for metric in ['speedup','efficiency']:
        fig,ax=plt.subplots(figsize=(10,5));ax.bar(parallel.label,parallel[metric]);ax.set_title(f'{metric.title()}: {size}x{size}');ax.tick_params(axis='x',rotation=35);fig.tight_layout();fig.savefig(out/f'{metric}_{size}.png');plt.close(fig)
fig,ax=plt.subplots(figsize=(8,5))
for label,sub in g.groupby('label'):
    ax.plot(sub['size'],sub['T_total'],marker='o',label=label)
ax.set_xlabel('Grid width/height');ax.set_ylabel('Total seconds');ax.set_title('Runtime vs grid size');ax.legend(fontsize=8);fig.tight_layout();fig.savefig(out/'runtime_vs_size.png');plt.close(fig)
print('Saved plots in',out)

# Heatmap from any existing binary grid output.
import struct, numpy as np
for file in sorted(pathlib.Path(a.csv).resolve().parent.parent.joinpath('code').glob('serial_*.dat')):
    with open(file,'rb') as f:
        nr,nc=struct.unpack('ii',f.read(8)); grid=np.fromfile(f,dtype=np.float64,count=nr*nc).reshape(nr,nc)
    fig,ax=plt.subplots(figsize=(6,5));im=ax.imshow(grid,cmap='coolwarm',vmin=0,vmax=1);fig.colorbar(im,ax=ax,label='Temperature');ax.set_title(file.stem);fig.tight_layout();fig.savefig(out/f'heatmap_{file.stem}.png');plt.close(fig)
