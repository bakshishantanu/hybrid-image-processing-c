# Image Processing Algorithms

This document describes the core algorithms implemented in the project and the reasoning behind their parallelization strategies. The documentation remains high-level, explaining *what* and *why*.

## 1. Grayscale Conversion

**Algorithm:**
The grayscale conversion transforms a 24-bit RGB image into a single-channel grayscale representation by applying the standard luminance formula:
`Y = 0.299 * R + 0.587 * G + 0.114 * B`

**Parallelization Strategy:**
This operation is embarrassingly parallel. Every pixel's luminance can be calculated independently without data dependencies.
- **OpenMP:** The nested loops traversing the image rows and columns are collapsed and parallelized.
- **CUDA:** A one-dimensional grid of threads processes the flat image array, with each thread handling a single pixel.

```cpp
// CUDA Kernel Launch Example
int blockSize = 256;
int numBlocks = (img->data_size + blockSize - 1) / blockSize;
RGB2GrayKernel<<<numBlocks, blockSize>>>(d_data, img->data_size);
```

## 2. Gaussian Blur (Separable Convolution)

**Algorithm:**
A standard 2D Gaussian Blur is computationally expensive: $O(K^2 \times N)$ where $K$ is the kernel size and $N$ is the number of pixels. To optimize this, we implemented a **separable convolution**. A 2D Gaussian kernel is mathematically equivalent to the outer product of two 1D Gaussian vectors. We blur the image horizontally first, and then blur the result vertically. This reduces the time complexity to $O(2K \times N)$.

**Parallelization Strategy:**
- **OpenMP:** The rows are processed in parallel during the horizontal pass, and columns (or rows depending on memory access patterns) are processed in parallel during the vertical pass.
- **MPI:** Requires a **Halo Exchange**. Because blurring requires accessing neighboring pixels, an MPI process must fetch overlapping row boundaries ("ghost zones" or "halos") from adjacent ranks before computing the blur.

```c
// MPI Halo Exchange Example (Vertical Pass)
MPI_Sendrecv(local_data, halo_size, MPI_UNSIGNED_CHAR, rank - 1, 0,
             halo_top, halo_size, MPI_UNSIGNED_CHAR, rank + 1, 0,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

## 3. Sobel Edge Detection

**Algorithm:**
The Sobel operator calculates the gradient magnitude of an image by convolving it with two $3 \times 3$ kernels (one for horizontal changes, $G_x$, and one for vertical changes, $G_y$). The final gradient is calculated as $\sqrt{G_x^2 + G_y^2}$.

**Parallelization Strategy:**
Like Gaussian blur, the Sobel filter requires a 1-pixel halo exchange in distributed environments (MPI). The computation itself is parallelized across threads or GPU blocks. The GPU implementation utilizes two distinct kernels to compute $G_x$ and $G_y$, followed by a third kernel to compute the magnitude.

## 4. Unsharp Masking

**Algorithm:**
Unsharp masking sharpens an image by subtracting a blurred version of the image from the original, creating an "edge mask", which is then added back to the original image:
`Sharpened = Original + (Original - Blurred) * Amount`

**Parallelization Strategy:**
The unsharp mask relies on the previously computed Gaussian blur output. The final blending operation is strictly element-wise (embarrassingly parallel), making it perfectly suited for `#pragma omp parallel for` (OpenMP) or element-wise CUDA kernels.

```c
// OpenMP Parallel Region Example
#pragma omp parallel for collapse(2) schedule(static)
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        // Element-wise blend logic
    }
}
```

## Memory and Load Balancing Considerations

- **Memory Complexity:** The filters operate out-of-place to prevent race conditions during convolution, requiring auxiliary buffers equal to the image size ($O(N)$ auxiliary space).
- **Load Balancing (MPI):** The image is sliced horizontally. Slices are evenly distributed based on `height / num_procs`. Any remainder rows are assigned to the last rank to ensure all pixels are processed.
