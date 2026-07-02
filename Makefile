CC ?= gcc
CFLAGS = -O2 -Wall -Iinclude
LDFLAGS = -lm
OMP_FLAGS = -fopenmp

# Microsoft MPI paths (set MSMPI_INC and MSMPI_LIB64 env vars, or override here)
MPI_CFLAGS = -I"$(subst \,/,$(MSMPI_INC))"
MPI_LDFLAGS = -L"$(subst \,/,$(MSMPI_LIB64))" -lmsmpi

HEADERS = include/bmpheader.h include/bmpimage.h include/bmpfile.h include/bmpfunctions.h

SEQ_SRC = src/core/main.c src/core/bmpfile.c src/seq/bmpgray.c src/seq/bmpblur.c src/seq/sobelfilter.c src/seq/unsharp.c
OMP_SRC = src/core/main.c src/core/bmpfile.c src/omp/bmpgray_omp.c src/omp/bmpblur_omp.c src/omp/sobelfilter_omp.c src/omp/unsharp_omp.c
HB_SRC  = src/core/main_hb.c src/core/bmpfile.c src/mpi/bmpgray_hb.c src/mpi/bmpblur_hb.c src/mpi/sobelfilter_hb.c src/mpi/unsharp_hb.c
CU_SRC  = src/core/main_cu.cu src/core/bmpfile.c src/cuda/bmpgray_cu.cu src/cuda/bmpblur_cu.cu src/cuda/sobelfilter_cu.cu src/cuda/unsharp_cu.cu
PM_SRC  = src/core/perf_model.c

NVCC = nvcc
CUDA_ARCH = -gencode arch=compute_75,code=sm_75 \
            -gencode arch=compute_86,code=sm_86 \
            -gencode arch=compute_89,code=sm_89
NVCC_FLAGS = -O2 -Xcompiler -Wall $(CUDA_ARCH) -Iinclude -ccbin "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.43.34808\bin\Hostx64\x64\cl.exe"

# ==============================================================================
# Build Targets
# ==============================================================================

.PHONY: all seq omp hybrid cu perf_model profile clean bench bench-scaling

all: image_seq.exe image_omp.exe image_hb.exe

seq: image_seq.exe
omp: image_omp.exe
hybrid: image_hb.exe
cu: image_cu.exe

image_seq.exe: $(SEQ_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SEQ_SRC) $(LDFLAGS)

image_omp.exe: $(OMP_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o $@ $(OMP_SRC) $(LDFLAGS)

image_hb.exe: $(HB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(OMP_FLAGS) $(MPI_CFLAGS) -o $@ $(HB_SRC) $(MPI_LDFLAGS) $(LDFLAGS)

image_cu.exe: $(CU_SRC) $(HEADERS)
	$(NVCC) $(NVCC_FLAGS) -o $@ $(CU_SRC)

perf_model: perf_model.exe
perf_model.exe: $(PM_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(PM_SRC) $(LDFLAGS)

# ==============================================================================
# Profiling (gprof)
# ==============================================================================

profile: $(SEQ_SRC) $(OMP_SRC) $(HEADERS)
	$(CC) -pg $(CFLAGS) -o image_seq_prof.exe $(SEQ_SRC) $(LDFLAGS)
	$(CC) -pg $(CFLAGS) $(OMP_FLAGS) -o image_omp_prof.exe $(OMP_SRC) $(LDFLAGS)
	@echo "Profiling executables built: image_seq_prof.exe, image_omp_prof.exe"
	@echo "Run them, then: gprof image_seq_prof.exe gmon.out > profile.txt"

# ==============================================================================
# Quick Benchmark
# ==============================================================================

bench: all
	@echo "=== Sequential ==="
	./image_seq.exe data/cat_portrait.bmp data/cat_portrait_gray.bmp data/cat_portrait_blur.bmp data/cat_portrait_sobel.bmp data/cat_portrait_unsharp.bmp
	@echo ""
	@echo "=== OpenMP ==="
	./image_omp.exe data/cat_portrait.bmp data/cat_portrait_gray.bmp data/cat_portrait_blur.bmp data/cat_portrait_sobel.bmp data/cat_portrait_unsharp.bmp
	@echo ""
	@echo "=== Hybrid (1 process) ==="
	mpiexec -n 1 ./image_hb.exe data/cat_portrait.bmp data/cat_portrait_gray.bmp data/cat_portrait_blur.bmp data/cat_portrait_sobel.bmp data/cat_portrait_unsharp.bmp
	@echo ""
	@echo "=== CUDA ==="
	./image_cu.exe data/cat_portrait.bmp data/cat_portrait_gray.bmp data/cat_portrait_blur.bmp data/cat_portrait_sobel.bmp data/cat_portrait_unsharp.bmp

bench-scaling: all
	cmd /c scripts\bench_scaling.bat

# ==============================================================================
# Clean
# ==============================================================================

clean:
	rm -f image_seq.exe image_omp.exe image_hb.exe image_cu.exe perf_model.exe
	rm -f image_seq_prof.exe image_omp_prof.exe
	rm -f gmon.out
	rm -f *.obj
