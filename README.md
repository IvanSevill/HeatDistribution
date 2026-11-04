# HeatDistribution

Simulates steady-state heat on a square 48x48 plate, using OpenMP solvers and an optional Raylib window that shows the plate converge.

- **Compare solvers on the same problem.** Jacobi, Gauss-Seidel (red-black), and SOR (red-black) run on one grid, so iteration counts and times are directly comparable.
- **Watch it converge.** The window colors each cell from blue to red and shows time, iterations, and current error.
- **Try setups without recompiling.** Change the method, target error, thread count, edge temperatures, and equation from the keyboard.
- **Add a heat source.** Switch from Laplace to Poisson to heat an 8x8 block in the center of the plate.
- **Keep a record of each run.** Every finished run appends one row to `simulation_results.csv` in the working directory.

## Build and run

The project targets Windows, using Visual Studio 2022 (toolset v143) and the raylib 5.5.0 NuGet package.

1. Open `HeatSimulation_Parallel.sln` and restore NuGet packages.
2. Select **Debug | x64**. It is the only configuration with OpenMP enabled.
3. Build, then run:

```bat
.\x64\Debug\HeatSimulation_Parallel.exe
```

These Windows steps come from the project files and have not been run; see [Status and limits](#status-and-limits).

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

[DONE] Converged in 443 iterations. Time: 0.0051s
```

| Option | Values | Default |
|--------|--------|---------|
| `--no-gui` | Run without a window | Window on |
| `--method` | `SOR`, `GAUSS`; anything else means Jacobi | Jacobi |
| `--threads` | 1 to the OpenMP maximum; other values are ignored | OpenMP maximum |
| `--threshold` | Stop when the largest change in one step is at or below this | `0.001` |
| `--omega` | SOR relaxation factor | `1.7` |
| `--equation` | `POISSON`; anything else means Laplace | Laplace |

Headless runs always use a top edge of 100 and the other edges at 0. `--method Gauss-Seidel` runs Jacobi without warning: use `GAUSS`.

`tests.bat` is a benchmark loop over Jacobi and SOR at 1, 2, 4, 8, and 16 threads. It contains no assertions.

## Measured results

Headless Laplace, threshold `0.00001`, omega `1.7`. Each time is the median of 7 runs.

| Method | Iterations | 1 thread | 2 threads | 4 threads | 8 threads |
|--------|-----------:|---------:|----------:|----------:|----------:|
| SOR | 443 | 0.0051 s | 0.0104 s | 0.0201 s | 0.0350 s |
| Gauss-Seidel | 2194 | 0.0230 s | 0.0486 s | 0.0816 s | 0.1794 s |
| Jacobi | 4076 | 0.0261 s | 0.0970 s | 0.1521 s | 0.3195 s |

**Source:** observed on 2026-11-04 on Linux with an Intel Core Ultra 7 355 (8 logical CPUs) and GCC 16.2 `-O2`. That build replaced Raylib with a stub, so these are not figures for the Visual Studio build. Expect different times on other machines.

**What this shows:** SOR needs about 9 times fewer iterations than Jacobi. On this small grid, adding threads made every method slower, so the run did not show any parallel speedup.

## Status and limits

This is a small coursework-style project, not a general solver.

- **Not verified on Windows.** The Visual Studio build and the window have not been run for this README. The solvers and headless mode were compiled and run on Linux as described above.
- **Windows only as shipped.** `main.cpp` uses MSVC's `_stricmp`, and the repository has no Linux build files. Building with real Raylib on Linux has not been tried.
- **Fixed problem size.** `GRID_SIZE` (48) is a compile-time constant and `dx` is fixed at 0.1.
- **Window times are not solver times.** The window runs one solver step per frame at 60 FPS, so its time mostly measures frame rate. Use `--no-gui` for timing. The one data row in the committed `simulation_results.csv` (917 iterations, 15.28 s) matches 917 frames at 60 FPS.
- **Poisson colors saturate.** Colors clip at 100, while the Poisson run reached about 2278 at the center, so the heated area displays as flat red.
- **Stale CSV header.** The committed `simulation_results.csv` header has no `Equation` column, so its data row has one field more than its header. A new file gets the correct header.
- **No automated tests.**
