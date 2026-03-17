@echo off
:: Ajusta a Release si quieres la máxima velocidad real del ejecutable
set EXE_PATH=.\x64\Debug\HeatSimulation_Parallel.exe
:: Subimos la precisión a 10^-6 para que tarde más
set THRESHOLD=0.00001
set OMEGA=1.7

echo ==================================================
echo   HIGH PRECISION BENCHMARK (Goal: %THRESHOLD%)
echo ==================================================

for %%M in (Jacobi SOR) do (
    echo.
    echo [METHOD: %%M]
    echo ----------------------------------------------
    
    for %%T in (1 2 4 8 16) do (
        %EXE_PATH% --no-gui --method %%M --threads %%T --threshold %THRESHOLD% --omega %OMEGA%
    )
)

echo.
echo ==================================================
echo   FINISHED! Check your simulation_results.csv
echo ==================================================
pause