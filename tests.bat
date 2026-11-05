@echo off
:: Checks and benchmarks the Release build. Rows are added to simulation_results.csv
:: in the current directory. Pass /nopause to skip the final pause.
setlocal
set EXE_PATH=.\x64\Release\HeatSimulation_Parallel.exe
:: Target error of 10^-5
set THRESHOLD=0.00001
set OMEGA=1.7
set OUTPUT=%TEMP%\heat_tests_%RANDOM%.txt
set FAILED=0

if not exist "%EXE_PATH%" (echo %EXE_PATH% not found: build the Release x64 configuration first & exit /b 1)

echo ==================================================
echo   BENCHMARK AND CHECKS (Goal: %THRESHOLD%)
echo ==================================================

call :run_method SOR SOR 443
call :run_method GAUSS Gauss-Seidel 2194
call :run_method Gauss-Seidel Gauss-Seidel 2194
call :run_method Jacobi Jacobi 4076

echo.
echo [Unknown method must fail]
%EXE_PATH% --no-gui --method Nope > "%OUTPUT%" 2>&1
if errorlevel 1 (echo   OK) else (echo   FAILED: --method Nope did not return an error & set FAILED=1)

del "%OUTPUT%" 2>nul
echo.
echo ==================================================
if "%FAILED%"=="0" (echo   ALL CHECKS PASSED. See simulation_results.csv) else (echo   SOME CHECKS FAILED)
echo ==================================================
if /i not "%~1"=="/nopause" pause
exit /b %FAILED%

:: run_method <option value> <name printed by the program> <expected iterations>
:run_method
echo.
echo [METHOD: %~1]
echo ----------------------------------------------
for %%T in (1 2 4 8 16) do call :run_once %~1 %~2 %~3 %%T
exit /b 0

:run_once
%EXE_PATH% --no-gui --method %~1 --threads %~4 --threshold %THRESHOLD% --omega %OMEGA% > "%OUTPUT%" 2>&1
if errorlevel 1 (echo   FAILED: %~1 at %~4 threads returned an error & set FAILED=1)
findstr /C:"Method: %~2 " "%OUTPUT%" >nul || (echo   FAILED: %~1 did not run %~2 & set FAILED=1)
findstr /C:"Converged in %~3 iterations" "%OUTPUT%" || (echo   FAILED: %~1 at %~4 threads, expected %~3 iterations & set FAILED=1)
exit /b 0
