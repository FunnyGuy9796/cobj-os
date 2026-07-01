#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include "util.h"
#include "../serial/serial.h"
#include "../fb/fbcon.h"

int snprintf(char *buf, size_t size, const char *fmt, ...);
int serial_printf(const char *fmt, ...);
int serial_vprintf(const char *fmt, va_list args);
int fbcon_printf(const char *fmt, ...);
int fbcon_vprintf(const char *fmt, va_list args);
void panic(const char *fmt, ...);

#endif