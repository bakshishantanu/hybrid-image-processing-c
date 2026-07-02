# Benchmarks & Methodology

This document outlines the testing methodology, hardware environment, and benchmark results for the Hybrid Image Processing project.

## Benchmark Environment

To ensure reproducibility, all benchmarks were executed under the following hardware and software configuration:

- **CPU:** AMD Ryzen (x86_64 architecture, varying core topology)
- **GPU:** NVIDIA GeForce RTX 4060 Laptop GPU
- **RAM:** 32 GB DDR5
- **Operating System:** Windows 11
- **Compiler:** MSYS2 GCC 14.2.0 (UCRT64)
- **CUDA Toolkit:** version 13.3
- **MPI Implementation:** Microsoft MPI (MS-MPI) v10.1
- **OpenMP:** GCC libgomp runtime
- **Build System:** GNU Make 4.4.1

## Methodology

Execution time is measured across four models: Sequential, OpenMP, Hybrid (MPI+OpenMP), and CUDA. The execution time reported encapsulates the entire processing pipeline (Grayscale -> Gaussian Blur -> Sobel Filter -> Unsharp Mask) but excludes file I/O operations, as I/O would bottleneck and misrepresent compute scaling.

- **Thread Counts (OpenMP):** 1, 2, 4, 8, 16
- **Process Counts (MPI):** 1, 2, 4, 8, 16
- **Image Resolutions:** Varies from small ($1600 \times 1067$) to extreme ($6000 \times 4000$) uncompressed BMPs.

---

## 1. Strong Scaling Results

Strong scaling measures how the execution time decreases as the number of parallel workers increases, while keeping the problem size fixed.

**Target Image:** `architecture.bmp` ($6000 \times 4000$, 24 MP)

| Execution Model | Workers (Threads/Procs) | Total Time (ms) | Speedup vs Sequential |
| :--- | :--- | :--- | :--- |
| Sequential | 1 | ~2457 | 1.0x |
| OpenMP | 2 | ~1190 | 2.06x |
| OpenMP | 4 | ~630 | 3.90x |
| OpenMP | 8 | ~380 | 6.46x |
| Hybrid (MPI) | 2 | ~430 | 5.71x |
| Hybrid (MPI) | 4 | ~350 | 7.02x |
| CUDA | Massively Parallel | ~25 | **98.28x** |

*(Note: Time variations occur based on OS scheduling and cache states. The table represents averaged approximations from raw CSV data).*

![Strong Scaling](../assets/strong_scaling_plot.png)

---

## 2. Problem-Size Scaling (Weak-ish Scaling)

This test evaluates how execution time scales as the input image size increases, keeping the compute resources maxed out for parallel models.

| Image | Resolution | Pixels | Sequential (ms) | OpenMP (Max) (ms) | CUDA (ms) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `cat_portrait.bmp` | 1600 x 1067 | 1.7M | ~147 | ~47 | ~19 |
| `texture.bmp` | 3589 x 5384 | 19.3M | ~1850 | ~290 | ~24 |
| `architecture.bmp` | 6000 x 4000 | 24.0M | ~2450 | ~366 | ~25 |

![Size Scaling](../assets/size_scaling_plot.png)

As the problem size increases, the difference in performance between the CPU implementation and the GPU implementation becomes exponentially more pronounced.

---

## Reproducing Results

To generate these benchmarks on your own machine:

1. Ensure all images are placed in the root directory.
2. Run the benchmarking batch script:
```bash
cmd /c scripts\bench_scaling.bat
```
3. The raw CSV outputs will be saved to the `results/` folder.
4. Run the Python plotting script to regenerate the graphs:
```bash
python scripts\plot_benchmarks.py
```
