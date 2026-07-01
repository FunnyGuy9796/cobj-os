#ifndef KB_H
#define KB_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_ESC,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_LSHIFT, KEY_RSHIFT,
    KEY_LCTRL, KEY_RCTRL,
    KEY_LALT, KEY_RALT,
    KEY_CAPSLOCK
} keycode_t;

typedef struct {
    bool shift;
    bool ctrl;
    bool alt;
    bool caps;
} kbd_mods_t;

typedef struct {
    keycode_t code;
    char ch;
    bool pressed;
    kbd_mods_t mods;
} kbd_event_t;

void kbd_handle_scancode(uint8_t sc);
kbd_mods_t kbd_getmods();
kbd_event_t kbd_getevent();
bool kbd_hasevent();
char kbd_getchar();
bool kbd_haschar();

#endif