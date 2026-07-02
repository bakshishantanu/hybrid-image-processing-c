# Project Architecture & Workflow Diagrams

This document illustrates the core architectural concepts of the Hybrid Image Processing pipeline. Understanding these workflows is essential for grasping how the system extracts performance across different hardware topologies.

## 1. Image Processing Compute Pipeline

The core pipeline is identical across all models. The filters run in a strict sequence because each stage depends on the exact output of the preceding stage. By executing these sequentially but parallelizing the *internal workload* of each filter, we maximize throughput.

```mermaid
flowchart LR
    subgraph Input["Input Stage"]
        A["24-bit BMP Image"]
    end

    subgraph Pipeline["Image Processing Pipeline"]
        B["Grayscale\nRGB → Luminance"]
        C["Gaussian Blur\nSeparable Convolution"]
        D["Sobel Edge Detection\nGradient Magnitude"]
        E["Unsharp Mask\nEdge Enhancement"]
    end

    subgraph Output["Output Stage"]
        F["Processed BMP"]
    end

    A -->|"Load Image"| B
    B -->|"Intermediate Buffer"| C
    C -->|"Blurred Image"| D
    D -->|"Edge Map"| E
    E -->|"Write Output"| F

    class A,F io
    class B,C,D,E stage

```

## 2. Distributed Memory: MPI Halo Exchange Workflow

The most complex implementation in this project is the Hybrid MPI+OpenMP model. Because processes operate in isolated memory spaces (distributed memory), they cannot simply read neighboring pixels from a shared array when performing convolutions like Gaussian Blur or Sobel Edge Detection. 

To solve this, we implement a **Halo Exchange** (or Ghost Zone swapping). Before a convolution, adjacent ranks exchange their boundary rows.

```mermaid
sequenceDiagram
    participant R0 as Rank 0 (Master)
    participant R1 as Rank 1 (Worker)
    participant R2 as Rank 2 (Worker)

    Note over R0: Load full BMP into Host RAM
    R0->>R1: MPI_Scatterv (Row Chunk 1)
    R0->>R2: MPI_Scatterv (Row Chunk 2)

    Note over R0,R2: Parallel Compute: Grayscale (No dependencies)

    Note over R0,R2: Halo Exchange Required for Convolution

    par R0 <-> R1 Exchange
        R0->>R1: MPI_Sendrecv (Send Bottom Row -> R1 Top Halo)
        R1->>R0: MPI_Sendrecv (Send Top Row -> R0 Bottom Halo)
    and R1 <-> R2 Exchange
        R1->>R2: MPI_Sendrecv (Send Bottom Row -> R2 Top Halo)
        R2->>R1: MPI_Sendrecv (Send Top Row -> R1 Bottom Halo)
    end

    Note over R0,R2: Parallel Compute: Blur & Sobel

    R1->>R0: MPI_Gatherv (Processed Chunk 1)
    R2->>R0: MPI_Gatherv (Processed Chunk 2)
    Note over R0: Construct Final Array & Save
```
*Why this diagram adds value: It instantly demonstrates to a technical recruiter that you understand distributed computing paradigms, synchronization, and how to handle data dependencies across process boundaries.*

## 3. Massively Parallel: CUDA Execution & Memory Model

The CUDA implementation drastically outperforms the CPU models by leveraging massive thread-level parallelism. 

The diagram below illustrates the actual lifecycle of a typical filter operation (e.g., Gaussian Blur) within our pipeline. To allow for intermediate disk saves, the host explicitly manages `cudaMemcpy` operations before and after each filter, while the GPU handles the heavy arithmetic intensity via a highly structured Grid and Block execution hierarchy.

```mermaid
graph TD
    subgraph Host [Host: System RAM]
        A[Host Image Buffer]
    end

    subgraph Device1 [Device: GPU VRAM — Input]
        B[Device Buffer: d_src]
    end

    subgraph KernelExec [CUDA Execution Hierarchy]
        Grid[Grid: numBlocks] --> Block[Block: 16x16 threads]
        Block --> Thread[Thread: Process Pixel]
    end

    subgraph Device2 [Device: GPU VRAM — Output]
        D[Device Buffer: d_dst]
    end

    Sync((Sync))

    subgraph HostEnd [Host: System RAM]
        F[Save Intermediate Result]
    end

    A -->|cudaMemcpy H2D| B
    B -.->|Global Read| KernelExec
    KernelExec -.->|Global Write| D
    D -->|cudaDeviceSynchronize| Sync
    Sync -->|cudaMemcpy D2H| A
    A --> F

    class A host
    class F host
    class B,D device
    class Grid,Block,Thread kernel
    class Sync sync
```
*Why this diagram adds value: It proves you understand the intricate execution model of CUDA (Host vs. Device distinction, manual memory transfers, and the Thread/Block/Grid mapping).*
