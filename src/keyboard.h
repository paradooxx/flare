#ifndef FLARE_KEYBOARD_H
#define FLARE_KEYBOARD_H

#include <stdbool.h>

#define MAX_QUERY_LENGTH 256

typedef struct {
    char text[MAX_QUERY_LENGTH];
    int length;
    int cursor_pos;
    bool text_changed;
    bool escape_pressed;
    bool enter_pressed;
    bool arrow_up_pressed;
    bool arrow_down_pressed;
} KeyboardState;

void Keyboard_Init(KeyboardState *kb);
void Keyboard_Update(KeyboardState *kb);
void Keyboard_Clear(KeyboardState *kb);
void Keyboard_SetText(KeyboardState *kb, const char *str);

#endif // FLARE_KEYBOARD_H
