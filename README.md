# HeatDistribution

Simulates steady-state heat on a square plate (48x48 by default), using OpenMP solvers and an optional Raylib window that shows the plate converge.

- **Compare solvers on the same problem.** Jacobi, Gauss-Seidel (red-black), and SOR (red-black) run on one grid, so iteration counts and times are directly comparable.
- **Watch it converge.** The window colors each cell from blue to red and shows time, iterations, and current error.
- **Try setups without recompiling.** Change the method, target error, thread count, edge temperatures, and equation from the keyboard.
- **Add a heat source.** Switch from Laplace to Poisson to heat an 8x8 block in the center of the plate.
- **Keep a record of each run.** Every converged run appends one row to `simulation_results.csv` in the working directory.

## Build and run

### Windows, with the window

The project targets Visual Studio 2022 (toolset v143) and the raylib 5.5.0 NuGet package.

1. Open `HeatSimulation_Parallel.sln` and restore NuGet packages.
2. Select **Release | x64**. Every configuration has OpenMP enabled; Release is the one to time.
3. Build, then run:

```bat
.\x64\Release\HeatSimulation_Parallel.exe
```

These Windows steps come from the project files and have not been run; see [Status and limits](#status-and-limits).

### Linux, headless

Needs GCC or Clang with OpenMP, and no Raylib. This build has no window.

```bash
make
./build/heat --no-gui --method SOR --threads 1 --threshold 0.00001
```

Use `make GRID_SIZE=512 BUILD_DIR=build/512` for another grid size.

## Usage

### Window

| Key | Action |
|-----|--------|
| `J` / `S` / `G` | Method: Jacobi, SOR, Gauss-Seidel |
| `Q` / `E` | Target error: multiply or divide by 10 |
| `.` or keypad `+`, `,` or keypad `-` | One thread more or fewer |
| Hold `Z` / `X` / `C` / `V` | Raise top, bottom, left, or right temperature (with `Shift`: lower) |
| `F` | Equation: Laplace or Poisson |
| `Enter` | Start the simulation |
| `R` | Back to the settings screen |

### Headless

```bat
HeatSimulation_Parallel.exe --no-gui --method SOR --threads 1 --threshold 0.00001
```

```text
[BENCHMARK] Method: SOR | Threads: 1 | Goal: 0.0000100000

[DONE] Converged in 443 iterations. Time: 0.0032s
```

| Option | Values | Default |
|--------|--------|---------|
| `--no-gui` | Run without a window | Window on |
| `--method` | `JACOBI`, `SOR`, `GAUSS`, `GAUSS-SEIDEL`, in any case | Jacobi |
| `--threads` | 1 or more; above the OpenMP maximum uses the maximum, with a warning | OpenMP maximum |
| `--threshold` | Above 0. Stop when the largest change in one sweep is at or below this | `0.001` |
| `--omega` | SOR relaxation factor, between 0 and 2, exclusive | `1.7` |
| `--equation` | `LAPLACE`, `POISSON`, in any case | Laplace |
| `--max-iter` | Give up after this many sweeps | `10000000` |
| `--help` | Show the options | |

Headless runs always use a top edge of 100 and the other edges at 0.

The exit code is `0` when the run converged, `1` when it reached `--max-iter`, and `2` for an invalid option or value, such as an unknown method. Only converged runs are added to `simulation_results.csv`, which git ignores.

## Tests

```bash
make test
```

- `tests/test_solvers.c` checks the solvers against exact solutions of the discrete Laplace and Poisson equations, hand-computed first sweeps, and iteration counts from before the parallel rewrite. It also checks that results are bit-identical at 1, 2, 3, and 8 threads.
- `tests/test_cli.sh` checks that each method name runs that method, and that invalid input exits with `2`, simulates nothing, and writes nothing.

On Windows, `tests.bat` runs each method at 1, 2, 4, 8, and 16 threads against the Release build, and fails if a method, an iteration count, or an exit code is wrong. Pass `/nopause` when running it from a script.

## Measured results

These figures come from `scripts/benchmark.sh` (`make benchmark`). Each is the median of 7 headless runs, on Linux with an Intel Core Ultra 7 355 (4 cores at up to 2.3 GHz and 4 at up to 1.7 GHz, as reported by `lscpu`) and GCC 16.2 `-O2`. Other work was running on the machine (load average 3 to 4), so differences of about 20% or less mean little. "Before" is commit `e4228a1`. They are not figures for the Visual Studio build, whose OpenMP runtime is different. Expect different times on other machines.

**On the default grid, more threads are slower.** Laplace, grid 48, threshold `0.00001`, omega `1.7`, in seconds. Thread count never changes the iterations or the result.

| Method | Iterations | 1 thread | 2 threads | 4 threads | 8 threads |
|--------|-----------:|---------:|----------:|----------:|----------:|
| SOR, before | 443 | 0.0050 | 0.0098 | 0.0162 | 0.0291 |
| SOR, now | 443 | 0.0032 | 0.0083 | 0.0080 | 0.0154 |
| Gauss-Seidel, before | 2194 | 0.0195 | 0.0470 | 0.0790 | 0.1500 |
| Gauss-Seidel, now | 2194 | 0.0173 | 0.0284 | 0.0374 | 0.0681 |
| Jacobi, before | 4076 | 0.0327 | 0.1021 | 0.1490 | 0.2740 |
| Jacobi, now | 4076 | 0.0255 | 0.0444 | 0.0346 | 0.0653 |

**Larger grids do benefit.** SOR, fixed number of sweeps, in microseconds per sweep. Speedup is against the same code at 1 thread.

| Grid | 1 thread | 2 threads | 4 threads | 8 threads | Best speedup | 8 threads, before |
|------|---------:|----------:|----------:|----------:|-------------:|------------------:|
| 48 | 6.9 | 11.7 | 17.7 | 33.8 | none | 67.9 |
| 128 | 43.5 | 50.5 | 40.7 | 45.4 | about none | 87.1 |
| 512 | 776.5 | 409.6 | 226.8 | 279.5 | 3.4x at 4 threads | 401.9 |
| 2048 | 13778.7 | 7251.1 | 3812.8 | 3029.8 | 4.5x at 8 threads | 4343.5 |

**Why the small grid does not speed up.** A SOR sweep of the 48x48 grid takes about 7 microseconds of work, while one barrier among 8 threads measured about 19 microseconds with GCC's default OpenMP settings and about 2.7 with `OMP_WAIT_POLICY=active`. Red-black solvers need two barriers per sweep, so the waiting costs more than the work it splits. This is inherent to a grid this small.

**What was fixed.** Each sweep used to start a parallel region and synchronize at least three times. A solve now keeps one region open and synchronizes once per color (once per sweep for Jacobi), swaps the Jacobi buffers instead of copying them, and visits only the cells of each color. Dynamic scheduling keeps the slower cores from holding the others back at each barrier.

**What was not the cause.** Padding the per-thread error values to separate cache lines and allocating the grid as one contiguous block made no measurable difference, so false sharing is not the problem here, and neither change was kept.

**To go further on Linux,** `OMP_WAIT_POLICY=active` makes threads spin at barriers instead of sleeping, which reduces the time at 2 and 4 threads in some runs but keeps those CPUs busy. It did not make the default grid faster than 1 thread.

## Status and limits

This is a small coursework-style project, not a general solver.

- **Not verified on Windows.** The Visual Studio build, `tests.bat`, and the window have not been run since these changes. The window code was only compiled against a stub of Raylib. The solvers, tests, and headless mode were built and run on Linux as described above.
- **Fixed problem setup.** `GRID_SIZE` (48) is a compile-time constant and `dx` is fixed at 0.1. The window cannot draw grids above 600 cells per side, because a cell would be under one pixel.
- **Window times are not comparable with headless times.** The window runs one solver step per frame at 60 FPS. It now records only the time inside the solver, but each step starts with cold caches and sleeping threads. Use `--no-gui` for timing.
- **Poisson colors saturate.** Colors clip at 100, while the Poisson run reached about 2278 at the center, so the heated area displays as flat red.
