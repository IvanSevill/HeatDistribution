#!/usr/bin/env bash
# Prints the Markdown tables used in README.md. Each figure is the median of RUNS runs.
# Usage: scripts/benchmark.sh  (RUNS, THREADS and SWEEP_GRIDS can be set in the environment)
set -eu

RUNS=${RUNS:-7}
THREADS=${THREADS:-1 2 4 8}
SWEEP_GRIDS=${SWEEP_GRIDS:-48 128 512 2048}
repo=$(cd "$(dirname "$0")/.." && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

build() {
    make -s -C "$repo" GRID_SIZE="$1" BUILD_DIR="$scratch/grid$1" "$scratch/grid$1/heat"
}

# median_time GRID args...: median of the "Time: Xs" field. Runs that stop at --max-iter
# exit with 1 on purpose, and they run in the scratch directory so no CSV is touched.
median_time() {
    local grid=$1
    shift
    for _ in $(seq "$RUNS"); do
        (cd "$scratch" && "$scratch/grid$grid/heat" --no-gui "$@" || true) |
            sed -n 's/.*Time: \([0-9.]*\)s.*/\1/p'
    done | sort -n | sed -n "$(((RUNS + 1) / 2))p"
}

header() {
    printf '| %s |' "$1"
    for t in $THREADS; do printf ' %s |' "$t thread(s)"; done
    printf '\n|%s|' "---"
    for _ in $THREADS; do printf -- '---:|'; done
    printf '\n'
}

build 48
echo "Time to converge, grid 48, Laplace, threshold 0.00001, omega 1.7 (seconds)"
echo
header "Method"
for method in SOR GAUSS JACOBI; do
    printf '| %s |' "$method"
    for t in $THREADS; do
        printf ' %s |' "$(median_time 48 --method "$method" --threads "$t" --threshold 0.00001)"
    done
    printf '\n'
done

echo
echo "Microseconds per sweep, fixed sweep count, SOR"
echo
header "Grid"
for grid in $SWEEP_GRIDS; do
    build "$grid"
    # About the same number of cell updates on every grid.
    sweeps=$((200000000 / (grid * grid)))
    printf '| %s |' "$grid"
    for t in $THREADS; do
        time=$(median_time "$grid" --method SOR --threads "$t" --threshold 1e-300 --max-iter "$sweeps")
        printf ' %s |' "$(awk -v time="$time" -v sweeps="$sweeps" 'BEGIN { printf "%.1f", time * 1e6 / sweeps }')"
    done
    printf '\n'
done
