# ps1-tests

Collection of PlayStation 1 tests for emulator development and hardware verification

## Tests

Name                 | Description
---------------------|------------
gpu/bandwidth        | Measure GPU/VRAM bandwidth
gpu/benchmark        | Simple GPU test to benchmark rasteriser
gpu/quad             | Semi-transparent polygon commands - for testing fill rules and transparency handling
gpu/transparency     | Draws rectangles with 4 semi-transparent blending modes
gpu/triangle         | Draws Gouroud shaded equilateral triangle 
gpu/lines            | Draws lines using different modes - for verifying Bresenham implementation, color blending, polyline handling
gpu/rectangles       | Draws all combinations of Rectangle commands
gpu/texture-overflow | Draws textured rectangle with UV overflowing VRAM width
gpu/mask-bit         | Check Mask bit behavior during VRAM copy operations
gpu/gp0-e1           | Check if GP0_E1, GPUSTAT and polygon render uses the same register internally
gte-fuzz             | Executes GTE opcodes with random parameters, can be used to verify against real console
spu/test             | Check SPU behavior (data is lost randomly on 32bit access, ok on 16bit)
spu/stereo           | Play samples on first two voices 
timers               | Run Timer0,1,2 using various clock sources and sync modes and time them using busy loops and vblank interrupt

Note: Make sure your PS-EXE loaded does set default value for Stack Pointer - these .exes has SP set to 0.

## Tools

Name                 | Description
---------------------|------------
diffvram             | Diff two images and write diff png if image contents aren't exactly the same

## Download

[Latest release](https://github.com/JaCzekanski/ps1-tests/releases/latest)

## Examples

<img src="gpu/lines/vram.png" height="256">
<img src="gpu/transparency/vram.png" height="256">
<img src="gpu/rectangles/vram.png" height="256">
<img src="gpu/texture-overflow/vram.png" height="256">

## Build

```
docker run -it -v $(pwd):/build jaczekanski/psn00bsdk:latest make
```

Note: requires Docker
