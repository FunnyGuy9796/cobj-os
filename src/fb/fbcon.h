#ifndef FBCON_H
#define FBCON_H

#include <stdint.h>
#include <stddef.h>

extern uint32_t bg_color;
extern uint32_t fg_color;

void fbcon_putchar(char c);
void fbcon_write(const char *str);
void fbcon_clear();

#endif