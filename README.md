# ps1-tests

## Tests

Name            | Description
----------------|------------
gpu/bandwidth   | Measure GPU/VRAM bandwidth
gpu/benchmark   | Simple GPU test to benchmark rasteriser
gpu/quad        | Semi-transparent polygon commands - for testing fill rules and transparency handling
gpu/transparency| Draws rectangles with 4 semi-transparent blending modes
gte-fuzz        | Executes GTE opcodes with random parameters, can be used to verify agains real console
spu/test        | Check SPU behaviour (data is lost randomly on 32bit access, ok on 16bit)
spu/stereo      | Play samples on first two voices 
timers          | Run Timer0,1,2 using various clock sources and sync modes and time them using busy loops and vblank interrupt

Note: Make sure your PS-EXE loaded does set default value for Stack Pointer - these .exes has SP set to 0.


## Download

[Latest release](https://github.com/JaCzekanski/ps1-tests/releases/latest)

## Build

```
docker run -it -v $(pwd):/build jaczekanski/psn00bsdk:latest make
```

Note: requires Docker
