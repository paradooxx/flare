#include "keyboard.h"
#include "raylib.h"
#include <string.h>
#include <ctype.h>

void Keyboard_Init(KeyboardState *kb) {
    if (!kb) return;
    memset(kb, 0, sizeof(KeyboardState));
}

void Keyboard_Clear(KeyboardState *kb) {
    if (!kb) return;
    kb->text[0] = '\0';
    kb->length = 0;
    kb->cursor_pos = 0;
    kb->text_changed = true;
}

void Keyboard_SetText(KeyboardState *kb, const char *str) {
    if (!kb || !str) return;
    strncpy(kb->text, str, MAX_QUERY_LENGTH - 1);
    kb->text[MAX_QUERY_LENGTH - 1] = '\0';
    kb->length = (int)strlen(kb->text);
    kb->cursor_pos = kb->length;
    kb->text_changed = true;
}

void Keyboard_Update(KeyboardState *kb) {
    if (!kb) return;

    kb->text_changed = false;
    kb->escape_pressed = IsKeyPressed(KEY_ESCAPE);
    kb->enter_pressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    kb->arrow_up_pressed = IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP);
    kb->arrow_down_pressed = IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN);

    bool ctrl_down = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    // Ctrl + A: Clear all text (or select all)
    if (ctrl_down && IsKeyPressed(KEY_A)) {
        Keyboard_Clear(kb);
        return;
    }

    // Ctrl + Backspace: Delete previous word
    if (ctrl_down && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) {
        if (kb->length > 0) {
            // Trim trailing spaces
            while (kb->length > 0 && isspace((unsigned char)kb->text[kb->length - 1])) {
                kb->length--;
            }
            // Trim word
            while (kb->length > 0 && !isspace((unsigned char)kb->text[kb->length - 1])) {
                kb->length--;
            }
            kb->text[kb->length] = '\0';
            kb->cursor_pos = kb->length;
            kb->text_changed = true;
        }
        return;
    }

    // Standard Backspace
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (kb->length > 0) {
            kb->length--;
            kb->text[kb->length] = '\0';
            kb->cursor_pos = kb->length;
            kb->text_changed = true;
        }
    }

    // Printable character input
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126 && kb->length < MAX_QUERY_LENGTH - 1) {
            kb->text[kb->length] = (char)key;
            kb->length++;
            kb->text[kb->length] = '\0';
            kb->cursor_pos = kb->length;
            kb->text_changed = true;
        }
        key = GetCharPressed();
    }
}
