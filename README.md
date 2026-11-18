<<<<<<< HEAD
Builds a movie of the Mandelbrot set
=======
# Mandelbrot Image and Zoom Movie Generator

This project generates images of the **Mandelbrot set** in C. It includes a program for creating a single JPEG image and a second program that creates a sequence of progressively zoomed images using **processes (`fork`)** and **POSIX threads (`pthread`)**.

The movie generator can also be used to compare execution times for different combinations of processes and threads.

## Project Files

| File | Description |
| --- | --- |
| `mandel.c` | Calculates the Mandelbrot set, converts iteration counts to grayscale pixel values, and provides the single-image program. |
| `mandel.h` | Declares the image computation functions, including row-based computation. |
| `mandelmovie.c` | Creates a zoom sequence using child processes and multiple threads per image. |
| `jpegrw.c` | Creates and manages image buffers and reads/writes JPEG files using libjpeg. |
| `jpegrw.h` | Defines the `imgRawImage` structure and JPEG/image helper functions. |
| `README.md` | Project documentation. |

## Requirements

- A Unix-like environment supporting `fork()`, `wait()`, and POSIX threads (for example, Linux)
- GCC or a compatible C compiler
- **libjpeg** headers and library
- The POSIX threads and math libraries (linked by the commands below)
- Optional: **FFmpeg** and **ffplay** for creating or viewing a video

On Ubuntu/Debian, the compiler and JPEG dependency can be installed with:

```bash
sudo apt install build-essential libjpeg-dev
```

To use the video commands below, also install FFmpeg:

```bash
sudo apt install ffmpeg
```

## Compilation

From the directory containing the `.c` and `.h` files, run:

### Single-image program

```bash
gcc -Wall -O2 -o mandel mandel.c jpegrw.c -ljpeg
```

### Zoom movie program

```bash
gcc -Wall -O2 -DMANDEL_LIBRARY -o mandelmovie mandelmovie.c mandel.c jpegrw.c -pthread -lm -ljpeg
```

**Important:** The `-DMANDEL_LIBRARY` flag excludes the `main()` function in `mandel.c` when compiling `mandelmovie`, preventing two `main()` definitions from conflicting.

If your repository already has a Makefile that correctly builds both targets, you can use that instead.

## Generating a Single Image

Run with the default settings:

```bash
./mandel
```

This creates `mandel.jpg` in the current directory.

### Single-image options

| Option | Description | Default |
| --- | --- | --- |
| `-x` | X-coordinate of the image center | `0` |
| `-y` | Y-coordinate of the image center | `0` |
| `-s` | Horizontal span of the region | `4` |
| `-W` | Image width in pixels | `1000` |
| `-H` | Image height in pixels | `1000` |
| `-m` | Maximum iterations per point | `1000` |
| `-o` | Output JPEG filename | `mandel.jpg` |
| `-h` | Display usage/help text | — |

Example: generate a custom image centered near a detailed boundary of the Mandelbrot set.

```bash
./mandel -x -0.7435 -y 0.1314 -s 0.01 -W 1000 -H 1000 -m 1000 -o zoom.jpg
```

## Generating a Mandelbrot Zoom Sequence

Run the movie generator with its defaults:

```bash
./mandelmovie
```

The program creates **50 JPEG frames**, named `mandel_0000.jpg`, `mandel_0001.jpg`, and so on, in the current directory.

Each frame zooms further into the same center point. For frame index `i`, the scale is computed as:

```text
current_scale = initial_scale / (1.10 ^ i)
```

### Movie generator options

| Option | Description | Default |
| --- | --- | --- |
| `-i` | Total number of images/frames to generate | `50` |
| `-P` | Maximum number of concurrently active child processes | `1` |
| `-T` | Number of threads used per image | `1` |
| `-m` | Maximum iterations per point | `1000` |
| `-x` | X-coordinate of zoom center | `-0.7435` |
| `-y` | Y-coordinate of zoom center | `0.1314` |
| `-s` | Initial scale | `0.01` |
| `-W` | Image width in pixels | `1000` |
| `-H` | Image height in pixels | `1000` |

**Note:** Process count uses an uppercase **`-P`**, while thread count uses an uppercase **`-T`**.

Example: generate 50 frames using up to 5 concurrent processes, with 10 threads per image.

```bash
./mandelmovie -i 50 -P 5 -T 10
```

Example: generate 10 smaller frames for a quick test.

```bash
./mandelmovie -i 10 -P 2 -T 5 -W 500 -H 500
```

## How Parallel Processing Works

1. The parent process loops through the requested frame indices.
2. For each frame, it uses `fork()` to start a child process. The parent uses `wait()` to limit the number of concurrently active children to the value of `-P`.
3. Each child allocates its own image buffer and calculates the scale for its frame.
4. The child creates `-T` POSIX threads. Each thread computes a distinct range of image rows through `compute_image_rows()`.
5. After all its threads have joined, the child writes its JPEG file, releases its memory, and exits.
6. The parent waits for remaining children before terminating.

This approach combines **multiprocessing across frames** with **multithreading within each frame**. Pixel colors are grayscale values derived from the number of Mandelbrot iterations at each point.

## Watching the Zoom as a Video

The C program produces **individual JPEG images**, not a video file. FFmpeg can assemble the frames into a video:

```bash
ffmpeg -framerate 10 -i mandel_%04d.jpg -c:v libx264 -pix_fmt yuv420p mandelbrot.mp4
```

This creates `mandelbrot.mp4` at 10 frames per second. The output frame rate can be changed by adjusting `-framerate`.

To play the video:

```bash
ffplay mandelbrot.mp4
```

You can also preview the image sequence directly without first exporting an MP4:

```bash
ffplay -framerate 10 -i mandel_%04d.jpg
```

## Performance Benchmarking

The program accepts different process and thread counts, making it possible to compare execution times. For example:

```bash
time ./mandelmovie -i 50 -P 5 -T 10
```

For a controlled comparison, keep the image count, image dimensions, center, scale, and maximum iterations the same across test runs. Test combinations such as:

```text
Processes: 1, 2, 5, 10, 20, 50
Threads:   1, 2, 5, 10, 20, 50
```

This produces **36 configurations**. Record the elapsed execution time for each configuration in a table or CSV file. A heatmap can highlight the fastest configuration, while a line chart can show execution time versus thread count with one line per process count.

**Interpretation:** Lower elapsed time is better. Increasing process or thread counts may improve performance up to a point, but excessive concurrency can introduce scheduling, memory, and disk-I/O overhead. The elapsed time also includes JPEG generation and file writing, not just Mandelbrot calculations. For more reliable comparisons, run each configuration multiple times and report the average or median.

### Adding Benchmark Graphs to This README

Export your Excel heatmap and graph as PNG images, store them in an `images/` folder in your repository, and then add the following Markdown where you want the images to appear:

```markdown
IMAGES GO HERE:


## Notes

- The generator writes frames to the current working directory. Running it again with the same frame indices overwrites those files.
- The thread count and process count must both be greater than zero; the movie generator checks these values.
- More processes means more simultaneously active image-generating children, not more total frames. The total number of frames is controlled by `-i`.
- This project targets a Unix-like environment; the `fork()`-based movie generator is not directly portable to native Windows without an appropriate compatibility environment.

## Code References

The source files include acknowledgments of example code used as a starting point:

- Mandelbrot implementation: [FIU COP4610 project example](https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html)
- JPEG utility implementation: [libjpeg example by tspi.at](https://www.tspi.at/2020/03/20/libjpegexample.html)
>>>>>>> 79981a1 (Refactor code structure for improved readability and maintainability)
