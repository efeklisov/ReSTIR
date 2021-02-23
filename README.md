# ReSTIR
An implementation of [spatiotemporal reservoir resampling](https://research.nvidia.com/publication/2020-07_Spatiotemporal-reservoir-resampling) algorithm with c++/Vulkan.

![demo](demo.png)

# System requirements
- GNU/Linux distribution
- GPU that supports Vulkan Raytracing
- GPU driver that supports finalized raytracing API

# Build Prerequisites
- Clang version with C++20 support
- GNU Parallel and glslangValidator in PATH for shader compilation

# Build

```
mkdir build
cd build
cmake ..
make
```

# Run

Just double-click the executable

# Controlls

- W/A/S/D - strafe
- Q/E - turn left/right
- Backspace/Enter - levitate downwards/upwards

# CLI options
Default is `./neo -m ReSTIR -N 1 -M 4`

-  -h,--help                   Print this help message and exit
-  -N,--N UINT                 Number of samples per pixel
-  -m,--method TEXT            Which shader to use
-  -c,--capture                Capture screenshot
-  -o,--offline                Quit after rendering the screenshot
-  -a,--accumulate             Stitch frames together
-  -f,--frames UINT            Number of frames to concatenate
-  -t,--tolerance UINT         Number of frames before capturing
-  -M,--M UINT                 M value for RIS
-  -i,--immediate              Unlock FPS

# EXTRA
`shaders/extra` folder contains several other shaders for debug and comparison. You may want to test the perfomance and quality with other explicit sampling strategies:

```
$ time ./neo -m ReSTIR -N 1 -M 4 -ocf 16
$ time ./neo -m extra/shadowrays_const -N 8 -ocf 16
```
