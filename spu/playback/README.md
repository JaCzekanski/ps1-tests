# Playback binary SPU dump

1. Capture SPU dump using Avocado
2. Convert .bin to .h file `xxd -i spu-dump.bin` > spu-dump.h
3. `make clean && make`