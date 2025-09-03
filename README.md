# Lowpass Filter

Reads a PNG image, applies a vertical 1-2-1 low-pass filter, and writes the filtered image.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Usage

```bash
./build/lowpass input.png output.png
```
