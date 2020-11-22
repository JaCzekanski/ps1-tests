#include <common.h>
#include "common.h"
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <stdio.h>
#include <string.h>

#define SCR_W 320
#define SCR_H 240

static DISPENV disp;
static DRAWENV draw;

char padBuffer[2][34];
unsigned short buttons = 0xffff, prevButtons = 0xffff;

const int fontPosX = 320;
const int fontPosY = 0;

int fontCharX = 0;
int fontCharY = 0;

void init() {		
    ResetGraph(0);
    SetDefDispEnv(&disp, 0, 0, SCR_W, SCR_H);
    SetDefDrawEnv(&draw, 0, 0, SCR_W, SCR_H);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
    SetDispMask(1);

	InitGeom();

	InitPAD(padBuffer[0], 34, padBuffer[1], 34);
	StartPAD();
	ChangeClearPAD(0);

	FntLoad(fontPosX, fontPosY);
}

void display_frame()
{
	VSync(0);

	prevButtons = buttons;
	buttons = ((PADTYPE*)padBuffer[0])->btn;

	fontCharX = 0;
	fontCharY = 0;
}


bool BUTTON(uint16_t button) {
	return (buttons & button) == 0 && ((prevButtons & button) != 0);
}


void clearFrameBuffer() {
	fillRect(0, 0, SCR_W, SCR_H, 0, 0, 0);
}

void FntPos(int x, int y) {
	fontCharX = x;
	fontCharY = y;
}

void FntChar(char c) {
	if (c == '\n') {
		fontCharX = 0;
		fontCharY += 8;
		return;
	}
	if (c >= ' ') {
		DR_TPAGE e;
		unsigned short texpage = getTPage(/* bits */ 0, /* semi transparency */ 0, fontPosX, fontPosY);
		setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ 1, texpage);
		DrawPrim(&e);

		if (c >= 96) {
			 c &= ~(1<<5); // To lower
		}
		char pos = c - 33;
		int u = pos%16;
		int v = pos/16;

		SPRT_8 t;
		setSprt8(&t);
		setXY0(&t, fontCharX, fontCharY);
		setUV0(&t, u*8, v*8);
		setClut(&t, 320, 32);
		setRGB0(&t, 255, 255, 255);
		
		DrawPrim(&t);
	}

	fontCharX += 8;
	if (fontCharX > SCR_W) {
		fontCharX = 0;
		fontCharY += 8;
	}
}

void FntPrintf(const char* format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);

	int len = strlen(buffer);
	for (int i = 0; i<len; i++) FntChar(buffer[i]);

	va_end(args);
}
