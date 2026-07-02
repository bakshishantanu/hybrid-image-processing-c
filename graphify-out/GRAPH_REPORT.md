# Graph Report - .  (2026-07-02)

## Corpus Check
- 2 files · ~9,110 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 109 nodes · 157 edges · 20 communities (19 shown, 1 thin omitted)
- Extraction: 91% EXTRACTED · 9% INFERRED · 0% AMBIGUOUS · INFERRED: 14 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_CUDA Gaussian Blur|CUDA Gaussian Blur]]
- [[_COMMUNITY_BMP IO & Header|BMP I/O & Header]]
- [[_COMMUNITY_Entry Points & Headers|Entry Points & Headers]]
- [[_COMMUNITY_CUDA Unsharp Mask|CUDA Unsharp Mask]]
- [[_COMMUNITY_CUDA Sobel Filter|CUDA Sobel Filter]]
- [[_COMMUNITY_Sequential Gaussian Blur|Sequential Gaussian Blur]]
- [[_COMMUNITY_Hybrid MPI+OMP Blur|Hybrid MPI+OMP Blur]]
- [[_COMMUNITY_OpenMP Gaussian Blur|OpenMP Gaussian Blur]]
- [[_COMMUNITY_Sequential Sobel Filter|Sequential Sobel Filter]]
- [[_COMMUNITY_OpenMP Sobel Filter|OpenMP Sobel Filter]]
- [[_COMMUNITY_Sequential Unsharp Mask|Sequential Unsharp Mask]]
- [[_COMMUNITY_Hybrid MPI+OMP Unsharp|Hybrid MPI+OMP Unsharp]]
- [[_COMMUNITY_OpenMP Unsharp Mask|OpenMP Unsharp Mask]]
- [[_COMMUNITY_Sequential Grayscale|Sequential Grayscale]]
- [[_COMMUNITY_Hybrid MPI+OMP Grayscale|Hybrid MPI+OMP Grayscale]]
- [[_COMMUNITY_OpenMP Grayscale|OpenMP Grayscale]]
- [[_COMMUNITY_Roofline Performance Model|Roofline Performance Model]]

## God Nodes (most connected - your core abstractions)
1. `main()` - 8 edges
2. `BMP_Open()` - 6 edges
3. `BMP_GaussianBlur_Hybrid()` - 5 edges
4. `BMP_Sobel_Hybrid()` - 5 edges
5. `BMP_UnsharpMask_Hybrid()` - 5 edges
6. `main()` - 5 edges
7. `BMP_GaussianBlur()` - 4 edges
8. `BMP_GaussianBlur()` - 4 edges
9. `cleanUp()` - 4 edges
10. `BMP_Save()` - 4 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `BMP_GaussianBlur_Hybrid()`  [INFERRED]
  main_hb.c → bmpblur_hb.c
- `main()` --calls--> `BMP_Gray_Hybrid()`  [INFERRED]
  main_hb.c → bmpgray_hb.c
- `main()` --calls--> `BMP_Sobel_Hybrid()`  [INFERRED]
  main_hb.c → sobelfilter_hb.c
- `main()` --calls--> `BMP_UnsharpMask_Hybrid()`  [INFERRED]
  main_hb.c → unsharp_hb.c
- `main()` --calls--> `BMP_Sobel()`  [INFERRED]
  main_cu.cu → sobelfilter_cu.cu

## Import Cycles
- None detected.

## Communities (20 total, 1 thin omitted)

### Community 0 - "CUDA Gaussian Blur"
Cohesion: 0.17
Nodes (10): BMP_GaussianBlur(), BMP_Image, __global__, generateKernel(), OneDBlurKernel(), BMP_Gray(), BMP_Image, __global__ (+2 more)

### Community 1 - "BMP I/O & Header"
Cohesion: 0.35
Nodes (10): BMP_Header, BMP_Destroy(), BMP_Open(), BMP_Save(), BMP_Image, checkHeader(), cleanUp(), FILE (+2 more)

### Community 2 - "Entry Points & Headers"
Cohesion: 0.33
Nodes (4): applyLocalSobel(), BMP_Sobel_Hybrid(), BMP_Image, computeLocalGradient()

### Community 3 - "CUDA Unsharp Mask"
Cohesion: 0.38
Nodes (6): BMP_UnsharpMask(), BMP_Image, __global__, generateKernel(), UnsharpMaskKernel(), UnsharpOneDBlurKernel()

### Community 4 - "CUDA Sobel Filter"
Cohesion: 0.40
Nodes (5): applySobelKernel(), BMP_Sobel(), computeGradientKernel(), BMP_Image, __global__

### Community 5 - "Sequential Gaussian Blur"
Cohesion: 0.70
Nodes (4): BMP_GaussianBlur(), BMP_Image, generateKernel(), OneDBlur()

### Community 6 - "Hybrid MPI+OMP Blur"
Cohesion: 0.60
Nodes (4): BMP_GaussianBlur_Hybrid(), BMP_Image, generateKernel(), OneDLocalBlur()

### Community 7 - "OpenMP Gaussian Blur"
Cohesion: 0.70
Nodes (4): BMP_GaussianBlur(), BMP_Image, generateKernel(), OneDBlur()

### Community 8 - "Sequential Sobel Filter"
Cohesion: 0.80
Nodes (4): applySobel(), BMP_Sobel(), BMP_Image, computeGradient()

### Community 9 - "OpenMP Sobel Filter"
Cohesion: 0.80
Nodes (4): applySobel(), BMP_Sobel(), BMP_Image, computeGradient()

### Community 10 - "Sequential Unsharp Mask"
Cohesion: 0.60
Nodes (4): BMP_UnsharpMask(), BMP_Image, generateKernel(), OneDBlur()

### Community 11 - "Hybrid MPI+OMP Unsharp"
Cohesion: 0.60
Nodes (4): BMP_UnsharpMask_Hybrid(), BMP_Image, generateKernel(), OneDLocalBlur()

### Community 12 - "OpenMP Unsharp Mask"
Cohesion: 0.60
Nodes (4): BMP_UnsharpMask(), BMP_Image, generateKernel(), OneDBlur()

### Community 13 - "Sequential Grayscale"
Cohesion: 0.67
Nodes (3): BMP_Gray(), BMP_Image, RGB2Gray()

### Community 14 - "Hybrid MPI+OMP Grayscale"
Cohesion: 0.67
Nodes (3): BMP_Gray_Hybrid(), BMP_Image, RGB2Gray()

### Community 15 - "OpenMP Grayscale"
Cohesion: 0.67
Nodes (3): BMP_Gray(), BMP_Image, RGB2Gray()

## Knowledge Gaps
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `main()` connect `BMP I/O & Header` to `Entry Points & Headers`, `Hybrid MPI+OMP Unsharp`, `Hybrid MPI+OMP Blur`, `Hybrid MPI+OMP Grayscale`?**
  _High betweenness centrality (0.076) - this node is a cross-community bridge._
- **Why does `main()` connect `CUDA Gaussian Blur` to `CUDA Unsharp Mask`, `CUDA Sobel Filter`?**
  _High betweenness centrality (0.041) - this node is a cross-community bridge._
- **Are the 7 inferred relationships involving `main()` (e.g. with `BMP_GaussianBlur_Hybrid()` and `BMP_Destroy()`) actually correct?**
  _`main()` has 7 INFERRED edges - model-reasoned connections that need verification._
- **Are the 2 inferred relationships involving `BMP_Open()` (e.g. with `main()` and `main()`) actually correct?**
  _`BMP_Open()` has 2 INFERRED edges - model-reasoned connections that need verification._