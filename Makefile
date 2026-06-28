CC ?= gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lm
OMP_FLAGS = -fopenmp

# Microsoft MPI paths (set MSMPI_INC and MSMPI_LIB64 env vars, or override here)
MPI_CFLAGS = -I"$(MSMPI_INC)"
MPI_LDFLAGS = -L"$(MSMPI_LIB64)" -lmsmpi

HEADERS = bmpheader.h bmpimage.h bmpfile.h bmpfunctions.h

SEQ_SRC = main.c bmpfile.c bmpgray.c bmpblur.c sobelfilter.c unsharp.c
OMP_SRC = main.c bmpfile.c bmpgray_omp.c bmpblur_omp.c sobelfilter_omp.c unsharp_omp.c
HB_SRC  = main_hb.c bmpfile.c bmpgray_hb.c bmpblur_hb.c sobelfilter_hb.c unsharp_hb.c
PM_SRC  = perf_model.c

# ==============================================================================
# Build Targets
# ==============================================================================

.PHONY: all seq omp hybrid perf_model profile clean bench bench-scaling

all: image_seq.exe image_omp.exe image_hb.exe

seq: image_seq.exe
omp: image_omp.exe
hybrid: image_hb.exe

image_seq.exe: $(SEQ_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SEQ_SRC) $(LDFLAGS)

image_omp.exe: $(OMP_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o $@ $(OMP_SRC) $(LDFLAGS)

image_hb.exe: $(HB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(OMP_FLAGS) $(MPI_CFLAGS) -o $@ $(HB_SRC) $(MPI_LDFLAGS) $(LDFLAGS)

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
	./image_seq.exe cat_portrait.bmp cat_portrait_gray.bmp cat_portrait_blur.bmp cat_portrait_sobel.bmp cat_portrait_unsharp.bmp
	@echo ""
	@echo "=== OpenMP ==="
	./image_omp.exe cat_portrait.bmp cat_portrait_gray.bmp cat_portrait_blur.bmp cat_portrait_sobel.bmp cat_portrait_unsharp.bmp
	@echo ""
	@echo "=== Hybrid (1 process) ==="
	mpiexec -n 1 ./image_hb.exe cat_portrait.bmp cat_portrait_gray.bmp cat_portrait_blur.bmp cat_portrait_sobel.bmp cat_portrait_unsharp.bmp

bench-scaling: all
	cmd /c bench_scaling.bat

# ==============================================================================
# Clean
# ==============================================================================

clean:
	rm -f image_seq.exe image_omp.exe image_hb.exe perf_model.exe
	rm -f image_seq_prof.exe image_omp_prof.exe
	rm -f gmon.out
	rm -f *.obj
