#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void FntInit();
void FntPos(int x, int y);
void FntChar(char c);
void FntPrintf(const char* format, ...);

#ifdef __cplusplus
}
#endif