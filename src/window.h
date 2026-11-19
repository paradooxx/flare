#ifndef FLARE_WINDOW_H
#define FLARE_WINDOW_H

#include <stdbool.h>

// Flare Window Dimensions - Modern launcher proportions (Spotlight/Raycast scale)
#define WINDOW_WIDTH     800
#define WINDOW_HEIGHT    540
#define WINDOW_RADIUS    16.0f

typedef struct {
    void *hwnd;
    int width;
    int height;
    bool is_visible;
} FlareWindow;

// Window lifecycle & DPI
void Window_SetDpiAwareness(void);
FlareWindow* Window_Create(int width, int height, const char *title);
void Window_Destroy(FlareWindow *win);

// Native Win32 operations
void Window_ApplyWin32Styles(FlareWindow *win);
void Window_CenterOnActiveMonitor(FlareWindow *win);
void Window_Show(FlareWindow *win);
void Window_Hide(FlareWindow *win);
void Window_Toggle(FlareWindow *win);
bool Window_IsFocused(const FlareWindow *win);

// Event polling from subclassed Win32 messages
bool Window_CheckHotkeyTriggered(void);
bool Window_CheckFocusLost(void);

#endif // FLARE_WINDOW_H
