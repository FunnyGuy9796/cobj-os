#include "kb.h"
#include "../fb/fbcon.h"

#define KBD_BUF_SIZE 256

static volatile kbd_event_t kbd_buf[KBD_BUF_SIZE];
static volatile size_t kbd_head = 0;
static volatile size_t kbd_tail = 0;
static volatile kbd_mods_t mods = { 0 };

static const char scancode_ascii[] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0 /*ctrl*/, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0 /*lshift*/, '\\', 'z','x','c','v','b','n','m',',','.','/',
    0 /*rshift*/, '*', 0 /*alt*/, ' ', 0 /*capslock*/,
};

static const char scancode_ascii_shifted[] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|', 'Z','X','C','V','B','N','M','<','>','?',
    0, '*', 0, ' ', 0,
};

static keycode_t scancode_keycode(uint8_t code) {
    switch (code) {
        case 0x1c:
            return KEY_ENTER;

        case 0x0e:
            return KEY_BACKSPACE;

        case 0x0f:
            return KEY_TAB;

        case 0x01:
            return KEY_ESC;

        case 0x2a:
            return KEY_LSHIFT;

        case 0x36:
            return KEY_RSHIFT;

        case 0x1d:
            return KEY_LCTRL;

        case 0x38:
            return KEY_LALT;

        case 0x3a:
            return KEY_CAPSLOCK;

        default:
            return KEY_NONE;
    }
}

static void kbd_push(kbd_event_t ev) {
    size_t next = (kbd_head + 1) % KBD_BUF_SIZE;

    if (next != kbd_tail) {
        kbd_buf[kbd_head] = ev;
        kbd_head = next;
    }
}

static void update_mods(keycode_t code, bool pressed) {
    switch (code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT: {
            mods.shift = pressed;

            break;
        }

        case KEY_LCTRL: {
            mods.ctrl = pressed;

            break;
        }

        case KEY_LALT: {
            mods.alt = pressed;

            break;
        }

        case KEY_CAPSLOCK: {
            if (pressed)
                mods.caps = !mods.caps;

            break;
        }
        
        default:
            break;
    }
}

void kbd_handle_scancode(uint8_t sc) {
    bool released = sc & 0x80;
    uint8_t code = sc & 0x7f;
    keycode_t special = scancode_keycode(code);

    if (special != KEY_NONE)
        update_mods(special, !released);

    kbd_event_t ev = {0};

    ev.pressed = !released;
    ev.mods = mods;

    if (special != KEY_NONE) {
        ev.code = special;
        ev.ch = 0;
    } else if (code < sizeof(scancode_ascii) && scancode_ascii[code]) {
        ev.code = KEY_CHAR;

        bool use_shifted = mods.shift;
        char base = scancode_ascii[code];
        bool is_letter = (base >= 'a' && base <= 'z');

        if (is_letter && mods.caps)
            use_shifted = !use_shifted;

        ev.ch = use_shifted ? scancode_ascii_shifted[code] : scancode_ascii[code];
    } else
        return;

    kbd_push(ev);
}

kbd_mods_t kbd_getmods() {
    return mods;
}

kbd_event_t kbd_getevent() {
    while (kbd_head == kbd_tail)
        __asm__ volatile ("hlt");

    kbd_event_t ev = kbd_buf[kbd_tail];

    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;

    return ev;
}

bool kbd_hasevent() {
    return kbd_head != kbd_tail;
}

char kbd_getchar() {
    for (;;) {
        kbd_event_t ev = kbd_getevent();

        if (!ev.pressed)
            continue;

        if (ev.code == KEY_CHAR)
            return ev.ch;

        if (ev.code == KEY_ENTER)
            return '\n';

        if (ev.code == KEY_BACKSPACE)
            return '\b';

        if (ev.code == KEY_TAB)
            return '\t';
    }
}

bool kbd_haschar() {
    return kbd_hasevent();
}