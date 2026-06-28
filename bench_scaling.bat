@echo off
setlocal enabledelayedexpansion

REM ============================================================================
REM  Scaling Benchmark Suite
REM  Generates CSV data for strong scaling and problem-size scaling analysis.
REM ============================================================================

set OUT_DIR=results
if not exist %OUT_DIR% mkdir %OUT_DIR%

set G=%OUT_DIR%\_tmp_g.bmp
set B=%OUT_DIR%\_tmp_b.bmp
set S=%OUT_DIR%\_tmp_s.bmp
set U=%OUT_DIR%\_tmp_u.bmp

REM ============================================================================
REM  1. Strong Scaling (fixed image: architecture.bmp, vary parallelism)
REM ============================================================================
set IMAGE=architecture.bmp
set CSV=%OUT_DIR%\strong_scaling.csv

echo model,image,width,height,pixels,gray_ms,blur_ms,sobel_ms,unsharp_ms,total_ms,threads,processes> %CSV%

echo.
echo ========================================
echo  Strong Scaling: %IMAGE%
echo ========================================

echo [1/3] Sequential...
image_seq.exe %IMAGE% %G% %B% %S% %U% --csv>> %CSV%

echo [2/3] OpenMP (varying threads)...
for %%T in (1 2 4 8) do (
    echo   OMP_NUM_THREADS=%%T
    set OMP_NUM_THREADS=%%T
    image_omp.exe %IMAGE% %G% %B% %S% %U% --csv>> %CSV%
)

echo [3/3] Hybrid MPI+OpenMP (varying processes)...
for %%P in (1 2 4) do (
    echo   mpiexec -n %%P
    mpiexec -n %%P image_hb.exe %IMAGE% %G% %B% %S% %U% --csv>> %CSV%
)

echo.
echo Strong scaling results: %CSV%

REM ============================================================================
REM  2. Problem Size Scaling (all images, fixed config)
REM ============================================================================
set CSV2=%OUT_DIR%\size_scaling.csv

echo model,image,width,height,pixels,gray_ms,blur_ms,sobel_ms,unsharp_ms,total_ms,threads,processes> %CSV2%

echo.
echo ========================================
echo  Problem Size Scaling
echo ========================================

echo [Sequential]
for %%I in (cat_portrait.bmp satellite.bmp texture.bmp cityscape.bmp architecture.bmp synthetic_noise.bmp) do (
    if exist %%I (
        echo   %%I
        image_seq.exe %%I %G% %B% %S% %U% --csv>> %CSV2%
    )
)

echo [OpenMP - max threads]
for %%I in (cat_portrait.bmp satellite.bmp texture.bmp cityscape.bmp architecture.bmp synthetic_noise.bmp) do (
    if exist %%I (
        echo   %%I
        image_omp.exe %%I %G% %B% %S% %U% --csv>> %CSV2%
    )
)

echo.
echo Size scaling results: %CSV2%

REM ============================================================================
REM  Cleanup temp files
REM ============================================================================
del /q %G% %B% %S% %U% 2>nul

echo.
echo Done! All results saved to %OUT_DIR%\
