@echo off
setlocal

REM ============================================================================
REM  Profiling with gprof
REM  Builds instrumented executables, runs them, collects profile output.
REM ============================================================================

set OUT_DIR=results
if not exist %OUT_DIR% mkdir %OUT_DIR%

set IMAGE=data\architecture.bmp
set G=%OUT_DIR%\_tmp_g.bmp
set B=%OUT_DIR%\_tmp_b.bmp
set S=%OUT_DIR%\_tmp_s.bmp
set U=%OUT_DIR%\_tmp_u.bmp

echo ========================================
echo  Profiling with gprof
echo ========================================
echo.

echo Building profiling executables (-pg -O2)...
make profile cu
if errorlevel 1 (
    echo.
    echo Build failed! Ensure 'make' and 'gcc' are in PATH.
    exit /b 1
)
echo.

echo [1/3] Profiling sequential execution on %IMAGE%...
image_seq_prof.exe %IMAGE% %G% %B% %S% %U%
if exist gmon.out (
    gprof image_seq_prof.exe gmon.out > %OUT_DIR%\profile_seq.txt
    echo   Saved to %OUT_DIR%\profile_seq.txt
    del gmon.out
) else (
    echo   Warning: gmon.out not generated
)

echo [2/3] Profiling OpenMP execution on %IMAGE%...
image_omp_prof.exe %IMAGE% %G% %B% %S% %U%
if exist gmon.out (
    gprof image_omp_prof.exe gmon.out > %OUT_DIR%\profile_omp.txt
    echo   Saved to %OUT_DIR%\profile_omp.txt
    del gmon.out
) else (
    echo   Warning: gmon.out not generated
)

echo [3/3] Profiling CUDA execution on %IMAGE%...
nsys profile --stats=true --force-overwrite=true -o %OUT_DIR%\profile_cu image_cu.exe %IMAGE% %G% %B% %S% %U% > %OUT_DIR%\profile_cu.txt 2>&1
if exist %OUT_DIR%\profile_cu.txt (
    echo   Saved to %OUT_DIR%\profile_cu.txt
) else (
    echo   Warning: nsys output not generated
)

REM Cleanup temp files
del /q %G% %B% %S% %U% 2>nul

echo.
echo Done! Profile outputs:
echo   %OUT_DIR%\profile_seq.txt
echo   %OUT_DIR%\profile_omp.txt
echo   %OUT_DIR%\profile_cu.txt
echo.
echo Review flat profile and call graph in those files.
echo Top hotspot functions should be OneDBlur, applySobel, or computeGradient.
