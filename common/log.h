#pragma once
#include <stdio.h>

#define DEBUG

#ifdef DEBUG
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif