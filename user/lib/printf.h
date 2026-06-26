#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include "util.h"

int ksnprintf(char *buf, size_t size, const char *fmt, ...);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list args);

#endif