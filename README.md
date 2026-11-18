# CPE-2600: Mandelbrot Multiprocessing and Multithreading

**Assignments:** Lab 11 - Multiprocessing; Lab 12 - Multithreading  
**Course:** CPE-2600 Systems Programming, Fall 2026  
**Student:** [Your full name]  
**Section:** [Your section number]

## Project overview

This C project generates images of the Mandelbrot set and uses the results to examine the performance of multiprocessing and multithreading. It includes two executables:

- **`mandel`** generates a single JPEG image of the Mandelbrot set.
- **`mandelmovie`** creates a sequence of progressively zoomed JPEG frames. It uses `fork()` to process different frames concurrently and POSIX threads (`pthread`) to divide the computation of each frame into separate image regions.

The generated frames can be combined into a zoom movie with FFmpeg. The project also benchmarks the effects of different process and thread counts on total execution time.

## Source files

| File | Purpose |
| --- | --- |
| `mandel.c` | Computes Mandelbrot iterations, converts iteration counts to grayscale colors, and implements the single-image command-line program. |
| `mandel.h` | Declares the image-computation functions, including `compute_image_rows()`. |
| `mandelmovie.c` | Generates zoom frames with child processes and multiple threads per image. |
| `jpegrw.c` | Allocates and manages image buffers and reads/writes JPEG files with libjpeg. |
| `jpegrw.h` | Declares the image structure and image/JPEG helper functions. |
| `makefile` | Builds the `mandel` and `mandelmovie` executables. |
| `mandel.mp4` | Example rendered Mandelbrot zoom movie. |

## Requirements and compilation

The program targets Linux or a compatible Unix-like environment with GCC, POSIX processes/threads, and libjpeg.

On Ubuntu or Debian, install the prerequisites:

```bash
sudo apt update
sudo apt install build-essential libjpeg-dev ffmpeg
```

Build both programs from the repository directory:

```bash
make
```

The included Makefile provides `mandel` and `mandelmovie` targets. To remove compiled output:

```bash
make clean
```

The programs can also be compiled manually:

```bash
gcc -Wall -O2 -o mandel mandel.c jpegrw.c -ljpeg
gcc -Wall -O2 -DMANDEL_LIBRARY -pthread -o mandelmovie mandelmovie.c mandel.c jpegrw.c -ljpeg -lm
```

The `-DMANDEL_LIBRARY` definition excludes the `main()` in `mandel.c` when its image-calculation functions are linked into `mandelmovie`. The `-pthread` compiler/linker flag is recommended for the threaded target.

## Lab 11: Multiprocessing

### Implementation

The movie generator uses `fork()` to create child processes. Each child is assigned one frame, calculates its image, writes a JPEG file, frees its image buffer, and exits. The parent limits the number of concurrently active children to the value supplied with `-P`. It uses `wait()` to free a process slot and waits for all remaining children before terminating.

The default movie has **50 frames**. Every new frame reduces the scale around a fixed center point, creating a zoom effect:

```text
scale_for_frame_i = initial_scale / (1.10 ^ i)
```

This work can be divided among processes because each frame is calculated independently.

### Process-count benchmark

For the Lab 11 process-only comparison, runtime was measured while varying the number of child processes. The process-versus-runtime plot is shown below:

![Number of Processes vs Execution Time](Screenshot%202026-11-17%20211230.png)

The graph shows a sharp improvement when moving from one to several processes. The benefit becomes smaller as the number of processes increases, and the highest process count does not necessarily produce the fastest execution. For the comparable single-thread configurations in the complete benchmark table below, runtime decreased from **26.067 s** at 1 process to **3.473 s** at 20 processes, then increased to **4.371 s** at 50 processes.

## Lab 12: Multithreading

### Implementation

Lab 12 adds the `-T` option to control the number of POSIX threads used to calculate **each frame**, while preserving the Lab 11 multiprocessing capability.

1. A child process allocates an image buffer and determines the frame's zoomed coordinate boundaries.
2. `createThreads()` allocates arrays of thread IDs and `thread_data_t` structures.
3. Each thread receives a different, non-overlapping range of image rows, along with the image pointer, coordinate bounds, and maximum iteration count.
4. Threads call `compute_image_rows()` and independently calculate pixel colors for their assigned rows.
5. The child calls `pthread_join()` for every created thread before saving the completed JPEG.
6. The parent process waits until all child processes have finished.

Each thread writes to a separate portion of the image buffer, so no mutex is needed for pixel calculation. The program supports the required range of **1 through 20 threads**; additional counts were also included in the benchmark.

### Runtime results

The table below shows the measured execution time **in seconds** for each process/thread combination. The process count varies by row and the thread count by column. **Lower is better.**

| Processes / Threads | 1 | 2 | 5 | 10 | 20 | 50 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **1** | 26.067 | 15.193 | 8.364 | 5.521 | 4.299 | 3.846 |
| **2** | 12.825 | 8.154 | 5.305 | 3.616 | 3.417 | 3.428 |
| **5** | 6.253 | 3.940 | 3.199 | 3.282 | 3.098 | 3.289 |
| **10** | 3.599 | 3.158 | 2.997 | 3.181 | 3.148 | 3.212 |
| **20** | 3.473 | 3.282 | 3.254 | **2.698** | 3.220 | 3.598 |
| **50** | 4.371 | 3.211 | 3.653 | 3.525 | 3.576 | 3.213 |

#### Runtime heatmap

![Mandelbrot Runtime Heatmap](Screenshot%202026-11-17%20212816.png)

### Discussion of results

**Which improved runtime more: multiprocessing or multithreading?**

Both techniques substantially reduced runtime compared with the 1-process, 1-thread baseline of **26.067 seconds**. In this benchmark, multiprocessing generally had the greater effect when each technique was varied on its own:

- **Increasing only processes:** 20 processes with 1 thread took **3.473 s**, about **7.51 times faster** than the baseline.
- **Increasing only threads:** 1 process with 20 threads took **4.299 s**, about **6.06 times faster** than the baseline.

A likely explanation is that independently processing entire frames makes good use of multiple CPU cores, while increasing the thread count within an individual frame eventually incurs overhead for creating, scheduling, and joining threads. The precise balance also depends on available CPU cores, memory, and JPEG-writing overhead.

**Was there a sweet spot?**

Yes. The fastest observed combination was **20 processes and 10 threads per image**, at **2.698 seconds**. This is approximately **9.66 times faster** than the 26.067-second baseline, or about a **89.6% reduction in elapsed time**.

Increasing concurrency past that point did not consistently help. For example, at 20 processes, changing from 10 to 50 threads increased runtime from **2.698 s** to **3.598 s**. Similarly, increasing from 20 to 50 processes with only one thread increased runtime from **3.473 s** to **4.371 s**. These observations suggest diminishing returns and possible CPU scheduling or resource-contention overhead.

Timing results can vary with system load and hardware. The table reports the recorded measurements rather than repeated-run averages.

## Running the programs

### Generate one image

```bash
./mandel
```

This creates `mandel.jpg`. For a custom image:

```bash
./mandel -x -0.7435 -y 0.1314 -s 0.01 -W 1000 -H 1000 -m 1000 -o zoom.jpg
```

| Option | Meaning | Default |
| --- | --- | --- |
| `-x` | Center X coordinate | `0` |
| `-y` | Center Y coordinate | `0` |
| `-s` | Horizontal scale | `4` |
| `-W` | Image width (pixels) | `1000` |
| `-H` | Image height (pixels) | `1000` |
| `-m` | Maximum iterations per point | `1000` |
| `-o` | Output filename | `mandel.jpg` |
| `-h` | Display help | — |

### Generate a zoom sequence

```bash
./mandelmovie -i 50 -P 5 -T 10
```

This command generates 50 frames using up to 5 concurrently active processes and 10 threads per frame. Output files are named `mandel_0000.jpg`, `mandel_0001.jpg`, and so on.

| Option | Meaning | Default |
| --- | --- | --- |
| `-i` | Number of frames | `50` |
| `-P` | Maximum concurrent child processes | `1` |
| `-T` | Threads per frame | `1` |
| `-m` | Maximum iterations per point | `1000` |
| `-x` | Zoom center X | `-0.7435` |
| `-y` | Zoom center Y | `0.1314` |
| `-s` | Initial scale | `0.01` |
| `-W` | Image width (pixels) | `1000` |
| `-H` | Image height (pixels) | `1000` |

**Note:** `-P` and `-T` are uppercase. The process and thread counts must both be greater than zero.

### Measure runtime

```bash
/usr/bin/time -p ./mandelmovie -i 50 -P 20 -T 10
```

The reported `real` value is the wall-clock execution time. For fair comparisons, keep the frame count, image dimensions, iteration limit, zoom center, and initial scale the same between runs.

## Creating and watching the movie

The program saves JPEG frames rather than a finished video. To assemble them into an MP4 at 10 frames per second:

```bash
ffmpeg -framerate 10 -i mandel_%04d.jpg -c:v libx264 -pix_fmt yuv420p mandel.mp4
```

To play the generated video:

```bash
ffplay mandel.mp4
```

To preview frames directly:

```bash
ffplay -framerate 10 -i mandel_%04d.jpg
```

The repository also includes an example movie: [mandel.mp4](mandel.mp4).

## Notes

- Generated `mandel_*.jpg` frames may be recreated when the program runs again.
- The number of processes determines concurrent frame calculations; it does **not** change the total number of frames requested with `-i`.
- Multiprocessing and threading overhead mean that larger counts will not always improve runtime.
- Generated JPEG frames and compiled object files normally belong in `.gitignore`. The final demonstration movie is included in this repository for grading, as required by Lab 11.

## Code acknowledgements

The starter code contains acknowledgements for external examples used as a foundation:

- [FIU COP4610 Mandelbrot project example](https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html)
- [libjpeg example from tspi.at](https://www.tspi.at/2020/03/20/libjpegexample.html)
