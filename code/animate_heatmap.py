import argparse
import pathlib
import struct
import matplotlib.animation as animation
import matplotlib.pyplot as plt
import numpy as np


def read_grid(path: pathlib.Path) -> np.ndarray:
    with open(path, "rb") as f:
        header = f.read(8)
        if len(header) != 8:
            raise ValueError(f"Invalid header in {path}")
        rows, cols = struct.unpack("ii", header)
        data = np.fromfile(f, dtype=np.float64, count=rows * cols)
    if data.size != rows * cols:
        raise ValueError(f"Incomplete grid in {path}: expected {rows * cols}, got {data.size}")
    return data.reshape(rows, cols)


def build_frames(files: list[pathlib.Path], steps: int) -> list[tuple[str, np.ndarray]]:
    grids = [read_grid(p) for p in files]
    if len(grids) > 1:
        base_shape = grids[0].shape
        for p, g in zip(files, grids):
            if g.shape != base_shape:
                raise ValueError(f"Shape mismatch in {p}: {g.shape} != {base_shape}")
        return [(p.stem, g) for p, g in zip(files, grids)]

    # Single grid passed: interpolate smoothly from a wall-only boundary state to the grid
    target = grids[0]
    rows, cols = target.shape
    start = np.zeros((rows, cols), dtype=np.float64)
    start[:, 0] = target[:, 0]
    start[:, -1] = target[:, -1]
    start[0, :] = target[0, :]
    start[-1, :] = target[-1, :]

    frames: list[tuple[str, np.ndarray]] = []
    for k in range(steps + 1):
        alpha = k / max(steps, 1)
        frame = (1.0 - alpha) * start + alpha * target
        # Keep exact outer boundaries from target
        frame[:, 0] = target[:, 0]
        frame[:, -1] = target[:, -1]
        frame[0, :] = target[0, :]
        frame[-1, :] = target[-1, :]
        frames.append((f"{files[0].stem} ({int(round(alpha * 100))}%)", frame))
    return frames


def main() -> None:
    parser = argparse.ArgumentParser(description="Create an animated heatmap GIF from .dat grid files.")
    parser.add_argument("inputs", nargs="+", help="One or more .dat grid files (in order for multi-frame animation)")
    parser.add_argument("--out", default="../data/diffusion_animation.gif", help="Output .gif path (default: ../data/diffusion_animation.gif)")
    parser.add_argument("--fps", type=int, default=6, help="Frames per second (default: 6)")
    parser.add_argument("--steps", type=int, default=20, help="Interpolation steps when given a single .dat file (default: 20)")
    args = parser.parse_args()

    in_paths = [pathlib.Path(p) for p in args.inputs]
    out_path = pathlib.Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    frames = build_frames(in_paths, args.steps)
    vmin = min(float(np.min(g)) for _, g in frames)
    vmax = max(float(np.max(g)) for _, g in frames)
    if np.isclose(vmin, vmax):
        vmin, vmax = 0.0, 1.0

    fig, ax = plt.subplots(figsize=(6, 5))
    im = ax.imshow(frames[0][1], cmap="coolwarm", vmin=vmin, vmax=vmax, interpolation="nearest")
    cbar = fig.colorbar(im, ax=ax, label="Temperature")
    title = ax.set_title(frames[0][0])
    ax.set_xlabel("Column")
    ax.set_ylabel("Row")
    fig.tight_layout()

    def update(idx: int):
        label, grid = frames[idx]
        im.set_data(grid)
        title.set_text(label)
        return [im, title]

    interval_ms = int(1000 / max(args.fps, 1))
    anim = animation.FuncAnimation(fig, update, frames=len(frames), interval=interval_ms, blit=False)

    if out_path.suffix.lower() == ".gif":
        anim.save(out_path, writer="pillow", fps=args.fps)
    else:
        anim.save(out_path, fps=args.fps)

    plt.close(fig)
    print(f"Saved animation to {out_path}")


if __name__ == "__main__":
    main()