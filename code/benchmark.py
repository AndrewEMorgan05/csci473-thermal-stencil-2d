import argparse, csv, os, pathlib, re, subprocess, time
from check_correctness import compare

p = argparse.ArgumentParser()
p.add_argument('--iterations', type=int, required=True)
p.add_argument('--repeats', type=int, default=3)
p.add_argument('--sizes', type=int, nargs='+', default=[512, 1024, 2048])
p.add_argument('--mpiexec', default='mpiexec')
p.add_argument('--out', default='../data/benchmarks.csv')
a = p.parse_args()
if a.iterations < 0 or a.repeats < 1:
    p.error('iterations must be nonnegative and repeats >= 1')

base = pathlib.Path(__file__).resolve().parent
out = pathlib.Path(a.out).resolve()
out.parent.mkdir(parents=True, exist_ok=True)
pat = re.compile(r'T_total=([0-9.]+) s.*T_computation=([0-9.]+) s.*T_other=([0-9.]+) s')
rows = []

configs_by_impl = [
    ('serial', [(1, 1)]),
    ('pth', [(1, 1), (1, 2), (1, 4)]),
    ('omp', [(1, 1), (1, 2), (1, 4)]),
    ('mpi', [(1, 1), (2, 1), (4, 1)]),
    ('hybrid', [(1, 1), (1, 2), (2, 1), (2, 2)]),
]

def run(command, env=None):
    result = subprocess.run(command, cwd=base, env=env, text=True, capture_output=True, check=True)
    return result.stdout

for n in a.sizes:
    inp = f'grid_{n}.dat'
    run([str(base / 'make-2d'), str(n), str(n), inp])
    serial = f'serial_{n}.dat'

    for implementation, configs in configs_by_impl:
        for ranks, threads in configs:
            for trial in range(1, a.repeats + 1):
                target = f'output_{implementation}_{n}_{ranks}_{threads}.dat'
                exe = str(base / ('stencil-2d' if implementation == 'serial' else 'stencil-2d-' + implementation))
                command = [exe, '-t', str(a.iterations), '-i', inp, '-o', target, '-v', '1']

                run_env = os.environ.copy()
                if implementation in ('pth', 'omp', 'hybrid'):
                    command += ['-p', str(threads)]
                if implementation == 'omp':
                    run_env['OMP_NUM_THREADS'] = str(threads)
                if implementation in ('mpi', 'hybrid'):
                    command = [a.mpiexec, '-n', str(ranks)] + command

                stdout = run(command, env=run_env)
                match = pat.search(stdout)
                if not match:
                    raise RuntimeError(f'Could not parse timing: {stdout}')

                if implementation == 'serial':
                    pathlib.Path(base / target).replace(base / serial)
                    target = serial
                    diff = 0.0
                    correct = True
                else:
                    diff, edges, correct = compare(base / serial, base / target)

                record = dict(
                    implementation=implementation,
                    size=n,
                    iterations=a.iterations,
                    ranks=ranks,
                    threads=threads,
                    workers=ranks * threads,
                    trial=trial,
                    T_total=float(match[1]),
                    T_computation=float(match[2]),
                    T_other=float(match[3]),
                    max_abs_difference=diff,
                    correct=correct,
                )
                rows.append(record)
                print(record, flush=True)
                if not correct:
                    raise RuntimeError('Correctness check failed; stopping benchmark')

with open(out, 'w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

print('Saved', out)