#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void arghf(const char* fmt, ...) __attribute__((noreturn)) __attribute__((format (printf, 1, 2)));

void arghf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	abort();
}

#define ASSERT(cond) do { if(!(cond)) { arghf("ASSERT(%s) failed in %s() in %s:%d\n", #cond, __func__, __FILE__, __LINE__); } } while(0)

#define AN(expr) do { ASSERT((expr) != 0); } while(0)
#define AZ(expr) do { ASSERT((expr) == 0); } while(0)
