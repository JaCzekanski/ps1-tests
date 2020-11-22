#include <stdio.h>
#include <stdlib.h>
#include <psxapi.h>
#include "common.h"
#include <twister.h>
#include "gte.h"

#define SAMPLE_STEP  25
uint32_t SEED = 0x00c0ffee;

int currentStep = SAMPLE_STEP;
int testNumber = 1;

void GTE_OP(uint8_t op, bool sf, bool lm, int tx, int vx, int mx) {
	union Command c;
	c._reg = 0;
	c._.cmd = op;
	c._.sf = sf;
	c._.lm = lm;
	c._.mvmvaTranslationVector = tx;
	c._.mvmvaMultiplyVector = vx;
	c._.mvmvaMultiplyMatrix = mx;

	_GTE_OP(c._reg);
}

char cursorAnimation() {
	static int frame = 0;
	static int state = 0;
	static char c = '-';
	
	if (frame++ % 10 == 0) {
		if (state == 0) c = '-';
		if (state == 1) c = '\\';
		if (state == 2) c = 'I';
		if (state == 3) c = '/';
		if (state == 4) c = '-';
		if (state == 5) c = '\\';
		if (state == 6) c = 'I';
		if (state == 7) c = '/';
		
		if (state++ == 7) state = 0;
	}
	return c;
}

/*
  00h  -          N/A (modifies similar registers than RTPS...)
  01h  RTPS   15  Perspective Transformation single
  0xh  -          N/A
  06h  NCLIP  8   Normal clipping
  0xh  -          N/A
  0Ch  OP(sf) 6   Outer product of 2 vectors
  0xh  -          N/A
  10h  DPCS   8   Depth Cueing single
  11h  INTPL  8   Interpolation of a vector and far color vector
  12h  MVMVA  8   Multiply vector by matrix and add vector (see below)
  13h  NCDS   19  Normal color depth cue single vector
  14h  CDP    13  Color Depth Que
  15h  -          N/A
  16h  NCDT   44  Normal color depth cue triple vectors
  1xh  -          N/A
  1Bh  NCCS   17  Normal Color Color single vector
  1Ch  CC     11  Color Color
  1Dh  -          N/A
  1Eh  NCS    14  Normal color single
  1Fh  -          N/A
  20h  NCT    30  Normal color triple
  2xh  -          N/A
  28h  SQR(sf)5   Square of vector IR
  29h  DCPL   8   Depth Cue Color light
  2Ah  DPCT   17  Depth Cueing triple (should be fake=08h, but isn't)
  2xh  -          N/A
  2Dh  AVSZ3  5   Average of three Z values
  2Eh  AVSZ4  6   Average of four Z values
  2Fh  -          N/A
  30h  RTPT   23  Perspective Transformation triple
  3xh  -          N/A
  3Dh  GPF(sf)5   General purpose interpolation
  3Eh  GPL(sf)5   General purpose interpolation with base
  3Fh  NCCT   39  Normal Color Color triple vector
  */

const char* getCommandName(int cmd) {
	switch (cmd) {
		case 0x01: return "RTPS";
		case 0x06: return "NCLIP";
		case 0x0C: return "OP";
		case 0x10: return "DPCS";
		case 0x11: return "INTPL";
		case 0x12: return "MVMVA";
		case 0x13: return "NCDS";
		case 0x14: return "CDP";
		case 0x16: return "NCDT";
		case 0x1B: return "NCCS";
		case 0x1C: return "CC";
		case 0x1E: return "NCS";
		case 0x20: return "NCT";
		case 0x28: return "SQR";
		case 0x29: return "DCPL";
		case 0x2A: return "DPCT";
		case 0x2D: return "AVSZ3";
		case 0x2E: return "AVSZ4";
		case 0x30: return "RTPT";
		case 0x3D: return "GPF";
		case 0x3E: return "GPL";
		case 0x3F: return "NCCT";
		default: return "---";
	}
}

int validCommands[] = {
	0x01, 
	0x06, 
	0x0c, 
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x16,
	0x1B,
	0x1C,
	0x1E,
	0x20,
	0x28,
	0x29,
	0x2A,
	0x2D,
	0x2E,
	0x30,
	0x3D,
	0x3E,
	0x3F,
	0x40
};


uint32_t VALUES[10] = {
	0x00000000,
	0xffffffff,
	0x7fffffff,
	0x80000000,
	0x7fff7fff,
	0x80008000,
	0x55555555,
	0xaaaaaaaa,
	0x00001000,
	0xd15ea5ed
};

uint32_t getRandomValue() {
	uint32_t selector = randomMT() % 31;
	if (selector < 10) return VALUES[selector];

	uint32_t rand =  randomMT();
	if (selector >= 20 && selector < 25) return rand&0x0000ffff;
	if (selector >= 25 && selector < 30) return rand&0xffff0000;
	return rand;
}

void fuzzCommand(int cmd, int samples) {
	EnterCriticalSection();
	printf("-------------- GTE 0x%02x %s (seed = 0x%08x)\n", cmd, getCommandName(cmd), SEED);
	for (int count = 0; count <samples; count++) {
		printf("Test %d\n", testNumber++);
		for (int i = 0; i<64; i++) {
			uint32_t v = getRandomValue();
			GTE_WRITE(i, v);
			printf("> r[%d] = 0x%08x\n", i, v);
		}

		if (cmd != 64) {
			uint32_t flags = randomMT();
			int sf = (flags & 1) == 1;
			int lm = (flags & 2) == 2;
			int tx = (flags & 0x0c) >> 2;
			int vx = (flags & 0x30) >> 4;
			int mx = (flags & 0xc0) >> 6;
			
			printf("GTE 0x%02x %s (sf=%d, lm=%d, tx=%d, vx=%d, mx=%d)\n", cmd, getCommandName(cmd), sf, lm, tx, vx, mx);
			GTE_OP(cmd, sf, lm, tx, vx, mx);
		}

		for (int i = 0; i<64; i++) {
			printf("< r[%d] = 0x%08x\n", i, GTE_READ(i));
		}
		printf("\n");
	}
	ExitCriticalSection();
}

int main() {
    init();
	printf("GTE fuzzer\n");

	int cmd = 1;
	int samples = 50;
	while (1) {
		clearFrameBuffer();
		if (BUTTON(PAD_UP)) {
			if (cmd > 0) cmd--;
		}
		if (BUTTON(PAD_DOWN)) {
			if (cmd < 64) cmd++;
		}
		if (BUTTON(PAD_LEFT)) {
			if (samples > currentStep) samples -= currentStep;
		}
		if (BUTTON(PAD_RIGHT)) {
			if (samples < 2000 - currentStep - 1) samples += currentStep;
		}

		if (BUTTON(PAD_SQUARE)) {
			cmd = 64;
		}
		if (BUTTON(PAD_TRIANGLE)) {
			if (currentStep == SAMPLE_STEP) currentStep = 1;
			else currentStep = SAMPLE_STEP;
		}
		if (BUTTON(PAD_CROSS)) {
			testNumber = 1;

			clearFrameBuffer();
			FntPrintf("\n\n\n\nSINGLE\nFuzzing cmd 0x%02x (%s) for %d samples\n", cmd, getCommandName(cmd), samples);
			display_frame();

			printf("==== SINGLE FUZZ  (seed = 0x%08x) ====\n", SEED);
			seedMT(SEED);
			fuzzCommand(cmd, samples);
			printf("==== END ====\n");
		}
		if (BUTTON(PAD_SELECT)) {
			testNumber = 1;

			printf("==== ALL CMD FUZZ (seed = 0x%08x) ====\n", SEED);
			for (int i = 0; i <= 64; i++) {
				cmd = i;
				clearFrameBuffer();
				FntPrintf("\n\n\n\nALL\nFuzzing cmd 0x%02x (%s) for %d samples\n", cmd, getCommandName(cmd), samples);
				display_frame();
				
				seedMT(SEED);
				fuzzCommand(cmd, samples);
			}
			printf("==== END ====\n");
		}
		if (BUTTON(PAD_START)) {
			testNumber = 1;

			printf("==== VALID CMD FUZZ (seed = 0x%08x) ====\n", SEED);
			for (int i = 0; i < sizeof(validCommands)/sizeof(int); i++) {
				cmd = validCommands[i];
				clearFrameBuffer();
				FntPrintf("\n\n\n\nVALID\nFuzzing cmd 0x%02x (%s) for %d samples\n", cmd, getCommandName(cmd), samples);
				display_frame();
				
				seedMT(SEED);
				fuzzCommand(cmd, samples);
			}
			printf("==== END ====\n");
		}
				
		FntPrintf("\n\n%c GTE Fuzzer\n", cursorAnimation());
		FntPrintf("Seed    = 0x%08x\n", SEED);
		FntPrintf("GTE_CMD = 0x%02x (%s)\n", cmd, getCommandName(cmd));
		FntPrintf("samples = %d\n", samples);

		FntPrintf("\n\n");
		FntPrintf("Left, Right - sample count\n");
		FntPrintf("Up, Down    - GTE command\n");
		FntPrintf("X           - Fuzz selected command\n");
		FntPrintf("Select      - Fuzz all commands\n\n");
		FntPrintf("Start       - Fuzz valid commands\n");
		display_frame();
	}

    return 0;
}