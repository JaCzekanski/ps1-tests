#include <common.h>
#include <psxetc.h>
#include <string.h>

const int fontPosX = 960;
const int fontPosY = 0;

int fontCharX = 0;
int fontCharY = 0;

// TODO: refactor font handling to use SDK functions
void FntInit() {
    FntLoad(fontPosX, fontPosY);
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
		setClut(&t, fontPosX, 32);
		setRGB0(&t, 255, 255, 255);
		
		DrawPrim(&t);
	}

	fontCharX += 8;
	if (fontCharX > 1024) {
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
