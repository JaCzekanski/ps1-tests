#pragma once
#include <sys/types.h>
#include <psxpad.h>
#include "stdint.h"

extern unsigned short buttons, prevButtons;

void init();
void display_frame();
void clearFrameBuffer();

bool BUTTON(uint16_t button);

void FntPos(int x, int y);
void FntChar(char c);
void FntPrintf(const char* format, ...);