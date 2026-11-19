#include "raylib.h"
#include "window.h"
#include "renderer.h"
#include "animations.h"
#include "keyboard.h"
#include "search.h"
#include "applications.h"
#include "history.h"
#include <stdio.h>

int main(void) {
    // 0. Enable Windows Per-Monitor High-DPI awareness (prevents OS bitmap scaling & blurring)
    Window_SetDpiAwareness();

    // 1. Configure raylib window flags
    // Undecorated borderless floating launcher, topmost, always running (2D optimized)
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | 
                   FLAG_WINDOW_TOPMOST | 
                   FLAG_WINDOW_ALWAYS_RUN);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Flare");
    SetTargetFPS(60);

    // 2. Initialize native Win32 window integration
    void *hwnd = GetWindowHandle();
    FlareWindow *win = Window_Create(WINDOW_WIDTH, WINDOW_HEIGHT, "Flare");
    if (!win) {
        CloseWindow();
        return 1;
    }
    win->hwnd = hwnd;

    Window_ApplyWin32Styles(win);
    Window_CenterOnActiveMonitor(win);
    Window_Show(win);
    Window_CheckFocusLost();

    // 3. Initialize subsystems
    AnimationController anim;
    Anim_Init(&anim);
    Anim_StartOpen(&anim);

    Renderer_Init();

    KeyboardState kb;
    Keyboard_Init(&kb);

    ApplicationCache apps;
    Applications_Init(&apps);
    Applications_BuildIndex(&apps);

    HistoryManager hist;
    History_Init(&hist);
    History_Load(&hist, &apps);

    SearchEngine search;
    Search_Init(&search);
    Search_PopulateSuggestions(&search, &apps, &hist);

    // Initial dynamic panel height based on suggestions
    int initialHeight = Renderer_CalculateHeight(&search);
    if (initialHeight != win->height) {
        win->height = initialHeight;
        SetWindowSize(win->width, initialHeight);
    }

    // Release startup initialization memory and DLL load working set
    Window_TrimMemory();

    // 4. Main application loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Check global hotkey (Alt + Space)
        if (Window_CheckHotkeyTriggered()) {
            if (anim.state == ANIM_STATE_OPEN || anim.state == ANIM_STATE_OPENING) {
                Anim_StartClose(&anim);
            } else {
                Keyboard_Clear(&kb);
                Search_PopulateSuggestions(&search, &apps, &hist);
                int targetH = Renderer_CalculateHeight(&search);
                if (win->height != targetH) {
                    win->height = targetH;
                    SetWindowSize(win->width, targetH);
                }
                Window_CenterOnActiveMonitor(win);
                Window_Show(win);
                Window_CheckFocusLost();
                Anim_StartOpen(&anim);
            }
        }

        // Process keyboard input and interaction when visible
        if (Anim_IsVisible(&anim)) {
            Keyboard_Update(&kb);

            // Escape key closes Flare
            if (kb.escape_pressed) {
                Anim_StartClose(&anim);
            }

            // Arrow keys navigate search results
            if (kb.arrow_down_pressed) {
                Search_SelectNext(&search);
            }
            if (kb.arrow_up_pressed) {
                Search_SelectPrev(&search);
            }

            // Enter opens the selected result
            if (kb.enter_pressed && search.result_count > 0) {
                if (search.selected_index >= 0 && search.selected_index < search.result_count) {
                    SearchResultItem *res = &search.results[search.selected_index];
                    History_Add(&hist, res->title, res->path, res->subtitle, res->category_label, res->is_command);
                    Applications_Execute(res->path, res->is_command, res->title);
                    Anim_StartClose(&anim);
                }
            }

            // Click outside or focus lost closes Flare
            if (anim.state == ANIM_STATE_OPEN) {
                if (Window_CheckFocusLost()) {
                    Anim_StartClose(&anim);
                }

                // Mouse click on result rows
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 mousePos = GetMousePosition();
                    float listY = 84.0f + 16.0f + 28.0f;
                    float rowH = 65.0f;
                    float rowW = (float)win->width - 28.0f;
                    int visibleCount = search.result_count - search.scroll_offset;
                    if (visibleCount > MAX_VISIBLE_ROWS) visibleCount = MAX_VISIBLE_ROWS;

                    for (int v = 0; v < visibleCount; v++) {
                        int idx = search.scroll_offset + v;
                        float ry = listY + (v * rowH);
                        Rectangle rowRec = { 14.0f, ry, rowW, rowH - 5.0f };
                        if (CheckCollisionPointRec(mousePos, rowRec)) {
                            search.selected_index = idx;
                            SearchResultItem *res = &search.results[idx];
                            History_Add(&hist, res->title, res->path, res->subtitle, res->category_label, res->is_command);
                            Applications_Execute(res->path, res->is_command, res->title);
                            Anim_StartClose(&anim);
                            break;
                        }
                    }
                }
            }

            // Update search state in real-time when query changed
            if (kb.text_changed) {
                Search_Execute(&search, &apps, &hist, kb.text);
            }

            // Dynamically adjust window height based on active search/suggestion count
            int targetHeight = Renderer_CalculateHeight(&search);
            if (targetHeight != win->height) {
                win->height = targetHeight;
                SetWindowSize(win->width, targetHeight);
            }
        }

        // Update animation timelines
        Anim_Update(&anim, dt);

        // Hide Win32 window once closing animation has finished
        if (anim.state == ANIM_STATE_CLOSED && win->is_visible) {
            Window_Hide(win);
            Window_TrimMemory();
        }

        // Low power mode when launcher is hidden (saves CPU cycles)
        if (anim.state == ANIM_STATE_CLOSED) {
            SetTargetFPS(20);
        } else {
            SetTargetFPS(60);
        }

        // Render frame
        BeginDrawing();
        Renderer_RenderFlare(win, &anim, &kb, &search);
        EndDrawing();

        // Release all startup and driver initialization RAM once initial frames settle
        static int s_settleFrames = 0;
        if (s_settleFrames < 3) {
            s_settleFrames++;
            if (s_settleFrames == 3) {
                Window_TrimMemory();
            }
        }
    }

    // 5. Cleanup and shutdown
    Renderer_Cleanup();
    Search_Cleanup(&search);
    Applications_Cleanup(&apps);
    Window_Destroy(win);
    CloseWindow();

    return 0;
}
