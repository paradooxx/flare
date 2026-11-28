#include "window.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

#define FLARE_HOTKEY_ID 1

typedef HRESULT (WINAPI *pfnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

typedef enum {
    FLARE_PROCESS_DPI_UNAWARE = 0,
    FLARE_PROCESS_SYSTEM_DPI_AWARE = 1,
    FLARE_PROCESS_PER_MONITOR_DPI_AWARE = 2
} FLARE_PROCESS_DPI_AWARENESS;

typedef BOOL (WINAPI *pfnSetProcessDpiAwarenessContext)(HANDLE);
typedef HRESULT (WINAPI *pfnSetProcessDpiAwareness)(FLARE_PROCESS_DPI_AWARENESS);
typedef BOOL (WINAPI *pfnSetProcessDPIAware)(void);

void Window_SetDpiAwareness(void) {
    // 1. Try Windows 10 Creators Update+ Per-Monitor V2 (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == -4)
    HMODULE hUser = LoadLibraryA("user32.dll");
    if (hUser) {
        pfnSetProcessDpiAwarenessContext setCtx = 
            (pfnSetProcessDpiAwarenessContext)(void*)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
        if (setCtx) {
            if (setCtx((HANDLE)-4)) {
                return;
            }
        }
    }

    // 2. Try Windows 8.1+ Per-Monitor V1 via shcore.dll
    HMODULE hShcore = LoadLibraryA("shcore.dll");
    if (hShcore) {
        pfnSetProcessDpiAwareness setAwareness = 
            (pfnSetProcessDpiAwareness)(void*)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (setAwareness) {
            if (SUCCEEDED(setAwareness(FLARE_PROCESS_PER_MONITOR_DPI_AWARE))) {
                return;
            }
        }
    }

    // 3. Fallback to Windows Vista/7 SetProcessDPIAware
    if (hUser) {
        pfnSetProcessDPIAware setLegacy = 
            (pfnSetProcessDPIAware)(void*)GetProcAddress(hUser, "SetProcessDPIAware");
        if (setLegacy) {
            setLegacy();
        }
    }
}

static WNDPROC s_origWndProc = NULL;
static bool s_hotkeyTriggered = false;
static bool s_focusLost = false;

static LRESULT CALLBACK FlareSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_HOTKEY:
            if (wParam == FLARE_HOTKEY_ID) {
                s_hotkeyTriggered = true;
                return 0;
            }
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                s_focusLost = true;
            }
            break;

        case WM_KILLFOCUS:
            s_focusLost = true;
            break;
    }

    if (s_origWndProc) {
        return CallWindowProc(s_origWndProc, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

FlareWindow* Window_Create(int width, int height, const char *title) {
    (void)title;
    FlareWindow *win = (FlareWindow*)calloc(1, sizeof(FlareWindow));
    if (!win) return NULL;

    win->width = width;
    win->height = height;
    win->is_visible = true;
    win->hwnd = NULL;
    return win;
}

void Window_Destroy(FlareWindow *win) {
    if (!win) return;
    if (win->hwnd) {
        HWND hwnd = (HWND)win->hwnd;
        UnregisterHotKey(hwnd, FLARE_HOTKEY_ID);
        if (s_origWndProc) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)s_origWndProc);
            s_origWndProc = NULL;
        }
    }
    free(win);
}

void Window_TrimMemory(void) {
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
}

void Window_ApplyWin32Styles(FlareWindow *win) {
    if (!win || !win->hwnd) return;
    HWND hwnd = (HWND)win->hwnd;

    // 1. Tool window (no taskbar icon), topmost
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOOLWINDOW;    // Hide from taskbar and Alt+Tab
    exStyle &= ~WS_EX_APPWINDOW;
    exStyle |= WS_EX_TOPMOST;      // Always on top
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // 2. Enable Windows 11 native rounded corners & dark mode
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        pfnDwmSetWindowAttribute pSetAttr = 
            (pfnDwmSetWindowAttribute)(void*)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (pSetAttr) {
            BOOL darkMode = TRUE;
            pSetAttr(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

            // Native Windows 11 rounded corners on the floating window
            int cornerPref = DWMWCP_ROUND;
            pSetAttr(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
        }
    }

    // 3. Subclass window procedure
    s_origWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)FlareSubclassProc);

    // 4. Register Global Hotkey: Alt + Space
    if (!RegisterHotKey(hwnd, FLARE_HOTKEY_ID, MOD_ALT | 0x4000, VK_SPACE)) {
        RegisterHotKey(hwnd, FLARE_HOTKEY_ID, MOD_ALT, VK_SPACE);
    }

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void Window_CenterOnActiveMonitor(FlareWindow *win) {
    if (!win || !win->hwnd) return;
    HWND hwnd = (HWND)win->hwnd;

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(hMonitor, &mi)) {
        mi.rcWork.left = 0;
        mi.rcWork.top = 0;
        mi.rcWork.right = GetSystemMetrics(SM_CXSCREEN);
        mi.rcWork.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int workWidth = mi.rcWork.right - mi.rcWork.left;
    int workHeight = mi.rcWork.bottom - mi.rcWork.top;

    int posX = mi.rcWork.left + (workWidth - win->width) / 2;
    int posY = mi.rcWork.top + (int)(workHeight * 0.20f);

    SetWindowPos(hwnd, HWND_TOPMOST, posX, posY, win->width, win->height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void Window_Show(FlareWindow *win) {
    if (!win || !win->hwnd) return;
    HWND hwnd = (HWND)win->hwnd;

    s_focusLost = false;

    ShowWindow(hwnd, SW_SHOW);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    win->is_visible = true;
}

void Window_Hide(FlareWindow *win) {
    if (!win || !win->hwnd) return;
    HWND hwnd = (HWND)win->hwnd;

    ShowWindow(hwnd, SW_HIDE);
    win->is_visible = false;
}

void Window_Toggle(FlareWindow *win) {
    if (!win) return;
    if (win->is_visible) {
        Window_Hide(win);
    } else {
        Window_CenterOnActiveMonitor(win);
        Window_Show(win);
    }
}

bool Window_IsFocused(const FlareWindow *win) {
    if (!win || !win->hwnd) return false;
    HWND fg = GetForegroundWindow();
    return (fg == (HWND)win->hwnd);
}

bool Window_CheckHotkeyTriggered(void) {
    bool triggered = s_hotkeyTriggered;
    s_hotkeyTriggered = false;
    return triggered;
}

bool Window_CheckFocusLost(void) {
    bool lost = s_focusLost;
    s_focusLost = false;
    return lost;
}
