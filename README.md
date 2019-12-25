# ps1-tests

Collection of PlayStation 1 tests for emulator development and hardware verification

## Tests

### CPU

Name                     | Description
-------------------------|------------
code-in-scratchpad       | **(Not finished)** Check whether code execution from Scratchpad is possible 

### DMA

Name                     | Description
-------------------------|------------
otc-test                 | DMA Channel 6 (OTC aka Ordering Table clear) unit tests

### GPU

Name                     | Description
-------------------------|------------
bandwidth                | Measure GPU/VRAM bandwidth
benchmark                | GPU test to benchmark rasterizer for various commands
quad                     | Semi-transparent polygon commands - for testing fill rules and transparency handling
transparency             | Draws rectangles with 4 semi-transparent blending modes
triangle                 | Draws Gouroud shaded equilateral triangle 
lines                    | Draws lines using different modes - for verifying Bresenham implementation, color blending, polyline handling
rectangles               | Draws all combinations of Rectangle commands
texture-overflow         | Draws textured rectangle with UV overflowing VRAM width
mask-bit                 | Check Mask bit behavior during VRAM copy operations
gp0-e1                   | Check if GP0_E1, GPUSTAT and polygon render uses the same register internally
vram-to-vram-overlap     | Test GP0(80) VRAM-VRAM copy behaviour in overlapping rects

### GTE

Name                     | Description
-------------------------|------------
gte-fuzz                 | Executes GTE opcodes with random parameters, can be used to verify against real console

### SPU

Name                     | Description
-------------------------|------------
test                 | Check SPU behavior (data is lost randomly on 32bit access, ok on 16bit)
stereo               | Play samples on first two voices 

### Timer

Name                     | Description
-------------------------|------------
timers                   | Run Timer0,1,2 using various clock sources and sync modes and time them using busy loops and vblank interrupt

Note: Make sure your PS-EXE loaded does set default value for Stack Pointer - these .exes has SP set to 0.

## Tools

Name                 | Description
---------------------|------------
diffvram             | Diff two images and write diff png if image contents aren't exactly the same

## Download

[Latest release](https://github.com/JaCzekanski/ps1-tests/releases/latest)

## Examples

<img src="gpu/benchmark/screenshot.png" height="480">
<img src="gpu/lines/vram.png" height="256">
<img src="gpu/transparency/vram.png" height="256">
<img src="gpu/rectangles/vram.png" height="256">
<img src="gpu/texture-overflow/vram.png" height="256">
<img src="gpu/vram-to-vram-overlap/vram.png" height="256">

## Build

```
docker run -it -v $(pwd):/build jaczekanski/psn00bsdk:latest make
```
