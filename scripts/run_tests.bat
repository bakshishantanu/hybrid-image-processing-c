@echo off
setlocal

REM ============================================================================
REM  Automated Verification Script
REM  Runs Seq, OMP, Hybrid, and CUDA models on a small test image and compares 
REM  their binary outputs pixel-by-pixel to ensure exact correctness.
REM ============================================================================

set IMAGE=data\architecture.bmp
set OUT_DIR=results
if not exist %OUT_DIR% mkdir %OUT_DIR%

set T_GRAY=%OUT_DIR%\_t_gray.bmp
set T_BLUR=%OUT_DIR%\_t_blur.bmp
set T_SOBEL=%OUT_DIR%\_t_sobel.bmp
set T_UNSHARP=%OUT_DIR%\_t_unsharp.bmp

set REF_GRAY=%OUT_DIR%\_ref_gray.bmp
set REF_BLUR=%OUT_DIR%\_ref_blur.bmp
set REF_SOBEL=%OUT_DIR%\_ref_sobel.bmp
set REF_UNSHARP=%OUT_DIR%\_ref_unsharp.bmp

echo Building all models...
make all
if errorlevel 1 (
    echo WARNING: 'make' failed or is not installed. Will attempt to run tests using existing executables if present.
)
echo.

echo [1/4] Generating reference outputs using Sequential Model...
image_seq.exe %IMAGE% %REF_GRAY% %REF_BLUR% %REF_SOBEL% %REF_UNSHARP%
echo.

echo [2/4] Testing OpenMP...
image_omp.exe %IMAGE% %T_GRAY% %T_BLUR% %T_SOBEL% %T_UNSHARP%
fc /b %REF_GRAY% %T_GRAY% >nul || echo ERROR: OMP Gray mismatch
fc /b %REF_BLUR% %T_BLUR% >nul || echo ERROR: OMP Blur mismatch
fc /b %REF_SOBEL% %T_SOBEL% >nul || echo ERROR: OMP Sobel mismatch
fc /b %REF_UNSHARP% %T_UNSHARP% >nul || echo ERROR: OMP Unsharp mismatch
echo.

echo [3/4] Testing Hybrid MPI+OpenMP...
mpiexec -n 2 image_hb.exe %IMAGE% %T_GRAY% %T_BLUR% %T_SOBEL% %T_UNSHARP%
fc /b %REF_GRAY% %T_GRAY% >nul || echo ERROR: HB Gray mismatch
fc /b %REF_BLUR% %T_BLUR% >nul || echo ERROR: HB Blur mismatch
fc /b %REF_SOBEL% %T_SOBEL% >nul || echo ERROR: HB Sobel mismatch
fc /b %REF_UNSHARP% %T_UNSHARP% >nul || echo ERROR: HB Unsharp mismatch
echo.

echo [4/4] Testing CUDA...
if exist image_cu.exe (
    image_cu.exe %IMAGE% %T_GRAY% %T_BLUR% %T_SOBEL% %T_UNSHARP%
    fc /b %REF_GRAY% %T_GRAY% >nul || echo ERROR: CUDA Gray mismatch
    fc /b %REF_BLUR% %T_BLUR% >nul || echo ERROR: CUDA Blur mismatch
    fc /b %REF_SOBEL% %T_SOBEL% >nul || echo ERROR: CUDA Sobel mismatch
    fc /b %REF_UNSHARP% %T_UNSHARP% >nul || echo ERROR: CUDA Unsharp mismatch
) else (
    echo CUDA skipped: image_cu.exe not found.
)
echo.

del /q %T_GRAY% %T_BLUR% %T_SOBEL% %T_UNSHARP% 2>nul
del /q %REF_GRAY% %REF_BLUR% %REF_SOBEL% %REF_UNSHARP% 2>nul

echo Done! If no "ERROR" messages appeared, all implementations match the sequential reference exactly.
