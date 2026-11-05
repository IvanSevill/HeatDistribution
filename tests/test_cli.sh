#!/usr/bin/env bash
# Black-box tests of the headless command line. Usage: test_cli.sh path/to/heat
set -u

heat=$(realpath "$1")
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
cd "$scratch" || exit 1
failures=0

# expect_run EXIT_CODE PATTERN args...: exit code and text in stdout+stderr.
expect_run() {
    local code=$1 pattern=$2 output actual
    shift 2
    output=$("$heat" "$@" 2>&1)
    actual=$?
    if [ "$actual" -ne "$code" ] || ! grep -q -- "$pattern" <<<"$output"; then
        failures=$((failures + 1))
        echo "FAIL: heat $* -> exit $actual, wanted $code and '$pattern'"
        echo "$output" | tail -3
    fi
}

# expect_rejected PATTERN args...: exit 2, nothing simulated, nothing recorded.
expect_rejected() {
    local pattern=$1
    shift
    rm -f simulation_results.csv
    expect_run 2 "$pattern" --no-gui "$@"
    if [ -e simulation_results.csv ]; then
        failures=$((failures + 1))
        echo "FAIL: heat --no-gui $* wrote a CSV"
    fi
    if "$heat" --no-gui "$@" 2>/dev/null | grep -q BENCHMARK; then
        failures=$((failures + 1))
        echo "FAIL: heat --no-gui $* started a simulation"
    fi
}

args=(--no-gui --threads 2 --threshold 0.00001)

expect_run 0 'Method: Jacobi .*' "${args[@]}"
expect_run 0 'Converged in 4076 iterations' "${args[@]}"
for name in Jacobi JACOBI jacobi; do
    expect_run 0 'Method: Jacobi ' "${args[@]}" --method "$name"
done
for name in Gauss-Seidel gauss-seidel GAUSS gauss; do
    expect_run 0 'Method: Gauss-Seidel ' "${args[@]}" --method "$name"
    expect_run 0 'Converged in 2194 iterations' "${args[@]}" --method "$name"
done
for name in SOR sor; do
    expect_run 0 'Method: SOR ' "${args[@]}" --method "$name"
    expect_run 0 'Converged in 443 iterations' "${args[@]}" --method "$name"
done
expect_run 0 'Converged in 575 iterations' "${args[@]}" --method SOR --equation POISSON
expect_run 0 'Converged in 443 iterations' "${args[@]}" --method SOR --equation laplace

expect_rejected "invalid value 'Gauss_Seidel' for --method" --method Gauss_Seidel
expect_rejected "invalid value 'Nope' for --method" --method Nope
expect_rejected "invalid value 'Poison' for --equation" --equation Poison
expect_rejected "invalid value '0' for --threads" --threads 0
expect_rejected "invalid value 'many' for --threads" --threads many
expect_rejected "invalid value '4x' for --threads" --threads 4x
expect_rejected "invalid value '0' for --threshold" --threshold 0
expect_rejected "invalid value 'abc' for --threshold" --threshold abc
expect_rejected "invalid value '2.5' for --omega" --omega 2.5
expect_rejected "invalid value '0' for --max-iter" --max-iter 0
expect_rejected "invalid value '99999999999999999999' for --max-iter" --max-iter 99999999999999999999
expect_rejected "unknown option '--methd'" --methd SOR
expect_rejected "--method needs a value" --method

expect_run 0 'above the OpenMP maximum' --no-gui --threads 100000 --threshold 0.1

# A run that gives up says so, fails, and records nothing.
rm -f simulation_results.csv
expect_run 1 'No convergence after 10 iterations' --no-gui --threshold 0.00001 --max-iter 10
if [ -e simulation_results.csv ]; then
    failures=$((failures + 1))
    echo "FAIL: a run that did not converge wrote a CSV"
fi

# The CSV gets the current header and rows with the same number of fields.
rm -f simulation_results.csv
expect_run 0 'Converged' --no-gui --threads 2 --threshold 0.1 --method SOR --equation POISSON
if ! head -1 simulation_results.csv | grep -q '^Timestamp,Equation,Method,Grid_Size,Threads,Omega,Threshold,Iterations,Final_Error,Execution_Time_Sec$'; then
    failures=$((failures + 1))
    echo "FAIL: unexpected CSV header"
fi
if ! sed -n 2p simulation_results.csv | grep -q ',Poisson,SOR,48,2,1.70,'; then
    failures=$((failures + 1))
    echo "FAIL: unexpected CSV row: $(sed -n 2p simulation_results.csv)"
fi
if [ "$(awk -F, '{print NF}' simulation_results.csv | sort -u | wc -l)" -ne 1 ]; then
    failures=$((failures + 1))
    echo "FAIL: CSV rows and header have different field counts"
fi

expect_run 0 'Usage:' --help

if [ "$failures" -gt 0 ]; then
    echo "test_cli: $failures checks failed"
    exit 1
fi
echo "test_cli: all checks passed"
