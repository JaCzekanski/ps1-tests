#pragma once
#include <sys/types.h>
#include <psxpad.h>
#include "stdint.h"

typedef uint8_t bool;
extern unsigned short buttons, prevButtons;

void init();
void display_frame();
void clearScreen();

bool BUTTON(uint16_t button);

void FntPos(int x, int y);
void FntChar(char c);
void FntPrintf(const char* format, ...);