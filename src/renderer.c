#include "renderer.h"
#include "raylib.h"
#include "applications.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Multi-size font system for crystal-clear typography (+20% scaled)
static Font s_fontInput = { 0 };
static Font s_fontTitle = { 0 };
static Font s_fontSubtitle = { 0 };
static Font s_fontSection = { 0 };
static Font s_fontFooter = { 0 };
static bool s_hasCustomFonts = false;

// Icon Cache for extracted application textures
typedef struct {
    char path[260];
    Texture2D texture;
    bool loaded;
} CachedIcon;

#define MAX_CACHED_ICONS 128
static CachedIcon s_iconCache[MAX_CACHED_ICONS];
static int s_iconCacheCount = 0;

static Font LoadCrispFont(const char *fontPath, int size) {
    Font f = LoadFontEx(fontPath, size, NULL, 0);
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    return f;
}

void Renderer_Init(void) {
    const char *fontBold = "C:\\Windows\\Fonts\\seguisb.ttf";     // Segoe UI Semibold
    const char *fontRegular = "C:\\Windows\\Fonts\\segoeui.ttf";  // Segoe UI Regular

    const char *boldPath = FileExists(fontBold) ? fontBold : fontRegular;
    const char *regularPath = FileExists(fontRegular) ? fontRegular : fontBold;

    if (FileExists(boldPath) || FileExists(regularPath)) {
        s_fontInput    = LoadCrispFont(boldPath, 28);
        s_fontTitle    = LoadCrispFont(boldPath, 22);
        s_fontSubtitle = LoadCrispFont(regularPath, 17);
        s_fontSection  = LoadCrispFont(boldPath, 15);
        s_fontFooter   = LoadCrispFont(regularPath, 15);
        s_hasCustomFonts = true;
    } else {
        Font def = GetFontDefault();
        s_fontInput    = def;
        s_fontTitle    = def;
        s_fontSubtitle = def;
        s_fontSection  = def;
        s_fontFooter   = def;
        s_hasCustomFonts = false;
    }
}

void Renderer_Cleanup(void) {
    if (s_hasCustomFonts) {
        UnloadFont(s_fontInput);
        UnloadFont(s_fontTitle);
        UnloadFont(s_fontSubtitle);
        UnloadFont(s_fontSection);
        UnloadFont(s_fontFooter);
        s_hasCustomFonts = false;
    }

    for (int i = 0; i < s_iconCacheCount; i++) {
        if (s_iconCache[i].loaded) {
            UnloadTexture(s_iconCache[i].texture);
        }
    }
    s_iconCacheCount = 0;
}

static Texture2D GetOrLoadAppIcon(const char *path) {
    if (!path || strlen(path) == 0) return (Texture2D){ 0 };

    for (int i = 0; i < s_iconCacheCount; i++) {
        if (_stricmp(s_iconCache[i].path, path) == 0) {
            return s_iconCache[i].texture;
        }
    }

    if (s_iconCacheCount >= MAX_CACHED_ICONS) {
        return (Texture2D){ 0 };
    }

    int w = 0, h = 0;
    unsigned char *rgba = Applications_ExtractIconRGBA(path, &w, &h);
    if (!rgba || w <= 0 || h <= 0) return (Texture2D){ 0 };

    Image img = {
        .data = rgba,
        .width = w,
        .height = h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    Texture2D tex = LoadTextureFromImage(img);
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(img);

    int idx = s_iconCacheCount++;
    strncpy(s_iconCache[idx].path, path, sizeof(s_iconCache[idx].path) - 1);
    s_iconCache[idx].path[sizeof(s_iconCache[idx].path) - 1] = '\0';
    s_iconCache[idx].texture = tex;
    s_iconCache[idx].loaded = true;
    return tex;
}

// Draw vector icon for system commands (+20% scaled)
static void DrawCommandIcon(float x, float y, float size, const char *title) {
    Rectangle bgRec = { x, y, size, size };

    // Dedicated calculator icon for math evaluations
    if (title && (title[0] == '=' || strstr(title, "Calc") != NULL)) {
        Color calcBg = (Color){ 245, 140, 28, 255 }; // Spotlight orange calculator accent
        DrawRectangleRounded(bgRec, 0.28f, 8, calcBg);
        float cx = x + size / 2.0f;
        float cy = y + size / 2.0f;
        float lineW = 14.0f;
        float lineH = 2.8f;
        DrawRectangleRounded((Rectangle){ cx - lineW/2.0f, cy - 4.2f, lineW, lineH }, 0.5f, 4, WHITE);
        DrawRectangleRounded((Rectangle){ cx - lineW/2.0f, cy + 3.5f, lineW, lineH }, 0.5f, 4, WHITE);
        return;
    }

    Color iconBg = (Color){ 44, 46, 54, 240 };
    Color iconBorder = (Color){ 255, 255, 255, 32 };
    DrawRectangleRounded(bgRec, 0.28f, 8, iconBg);
    DrawRectangleRoundedLines(bgRec, 0.28f, 8, 1.0f, iconBorder);

    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    Color fgColor = (Color){ 225, 230, 240, 240 };

    if (strstr(title, "Quit All") != NULL) {
        // Encircled X glyph
        DrawCircleLines((int)cx, (int)cy, 9.0f, fgColor);
        float d = 4.5f;
        DrawLineEx((Vector2){ cx - d, cy - d }, (Vector2){ cx + d, cy + d }, 2.0f, fgColor);
        DrawLineEx((Vector2){ cx + d, cy - d }, (Vector2){ cx - d, cy + d }, 2.0f, fgColor);
    } else if (strstr(title, "Reminder") != NULL) {
        // Checkbox glyph with checkmark
        Rectangle box = { cx - 7.5f, cy - 7.5f, 15.0f, 15.0f };
        DrawRectangleRoundedLines(box, 0.28f, 4, 1.5f, fgColor);
        Vector2 p1 = { cx - 4.0f, cy + 0.0f };
        Vector2 p2 = { cx - 0.5f, cy + 3.8f };
        Vector2 p3 = { cx + 5.2f, cy - 3.8f };
        DrawLineEx(p1, p2, 2.0f, fgColor);
        DrawLineEx(p2, p3, 2.0f, fgColor);
    } else if (strstr(title, "Lock") != NULL) {
        // Padlock glyph
        Rectangle lockBody = { cx - 6.5f, cy - 1.0f, 13.0f, 9.5f };
        DrawRectangleRounded(lockBody, 0.22f, 4, fgColor);
        DrawRing((Vector2){ cx, cy - 1.0f }, 3.5f, 5.6f, 180.0f, 360.0f, 16, fgColor);
    } else if (strstr(title, "Recycle") != NULL || strstr(title, "Empty") != NULL) {
        // Trash bin glyph
        Rectangle binBody = { cx - 5.5f, cy - 1.0f, 11.0f, 10.0f };
        DrawRectangleRoundedLines(binBody, 0.2f, 4, 1.5f, fgColor);
        DrawLineEx((Vector2){ cx - 7.0f, cy - 3.0f }, (Vector2){ cx + 7.0f, cy - 3.0f }, 1.6f, fgColor);
        DrawLineEx((Vector2){ cx - 2.5f, cy - 5.0f }, (Vector2){ cx + 2.5f, cy - 5.0f }, 1.5f, fgColor);
    } else {
        // First letter badge
        char letter[2] = { title[0], '\0' };
        Vector2 lSize = MeasureTextEx(s_fontTitle, letter, 19.0f, 1.0f);
        DrawTextEx(s_fontTitle, letter, (Vector2){ cx - lSize.x / 2.0f, cy - lSize.y / 2.0f }, 19.0f, 1.0f, fgColor);
    }
}

int Renderer_CalculateHeight(const SearchEngine *search) {
    if (!search) return WINDOW_HEIGHT;

    int visibleCount = search->result_count;
    if (visibleCount > MAX_VISIBLE_ROWS) {
        visibleCount = MAX_VISIBLE_ROWS;
    }

    if (visibleCount > 0) {
        // Header (84) + section margins (16 + 28) = 128
        // visibleCount * rowH (65)
        // Bottom padding to footer hints = 67
        return 128 + (visibleCount * 65) + 67;
    }

    // 0 results state ("No results found for ...")
    if (!search->is_showing_suggestions) {
        return 280;
    }

    // Clean compact empty search bar
    return 110;
}

void Renderer_RenderFlare(const FlareWindow *win,
                          const AnimationController *anim,
                          const KeyboardState *kb,
                          SearchEngine *search) {
    if (!Anim_IsVisible(anim)) return;

    float cursorAlpha = Anim_GetCursorAlpha(anim);

    float w = (float)win->width;
    float h = (float)win->height;

    // 1. Draw clean floating search card background (no outer dark box)
    ClearBackground((Color){ 24, 25, 29, 255 });

    // Subtle frosted border
    DrawRectangleRoundedLines((Rectangle){ 1, 1, w - 2, h - 2 }, 0.038f, 16, 1.2f, (Color){ 255, 255, 255, 32 });

    // Specular top highlight line
    DrawLineEx((Vector2){ 24.0f, 1.5f }, (Vector2){ w - 24.0f, 1.5f }, 1.5f, (Color){ 255, 255, 255, 36 });

    // 2. Search Input Header with sleek vector search icon (+20% scaled)
    float headerH = 84.0f;

    // Search magnifying glass icon
    float iconCX = 38.0f;
    float iconCY = headerH / 2.0f;
    Color glassColor = (Color){ 145, 150, 165, 230 };
    DrawCircleLines((int)iconCX, (int)iconCY - 1, 8.5f, glassColor);
    DrawLineEx((Vector2){ iconCX + 5.8f, iconCY + 4.8f }, (Vector2){ iconCX + 12.5f, iconCY + 11.5f }, 2.5f, glassColor);

    float textX = 66.0f;
    float textY = (headerH - 28.0f) / 2.0f;

    if (kb->length == 0) {
        const char *placeholder = "Search apps, commands, or calculate math...";
        DrawTextEx(s_fontInput, placeholder, (Vector2){ textX + 4.0f, textY }, 28.0f, 1.0f, (Color){ 125, 130, 142, 190 });
    } else {
        DrawTextEx(s_fontInput, kb->text, (Vector2){ textX, textY }, 28.0f, 1.0f, (Color){ 248, 250, 255, 255 });
    }

    // Blinking Cursor
    float cursorX = textX;
    if (kb->length > 0) {
        Vector2 textMetrics = MeasureTextEx(s_fontInput, kb->text, 28.0f, 1.0f);
        cursorX = textX + textMetrics.x + 2.0f;
    }
    float cursorW = 2.5f;
    float cursorH = 30.0f;
    float cursorY = (headerH - cursorH) / 2.0f;
    Color cursorColor = (Color){ 220, 225, 235, (unsigned char)(cursorAlpha * 240.0f) };
    DrawRectangleRounded((Rectangle){ cursorX, cursorY, cursorW, cursorH }, 0.5f, 4, cursorColor);

    // Divider Line
    DrawLineEx((Vector2){ 0.0f, headerH }, (Vector2){ w, headerH }, 1.0f, (Color){ 255, 255, 255, 22 });

    // 3. Section Header ("SUGGESTIONS" or "APPLICATIONS & COMMANDS")
    float sectionY = headerH + 16.0f;
    const char *sectionTitle = search->is_showing_suggestions ? "SUGGESTIONS" : "APPLICATIONS & COMMANDS";
    DrawTextEx(s_fontSection, sectionTitle, (Vector2){ 30.0f, sectionY }, 15.0f, 1.0f, (Color){ 140, 145, 158, 220 });

    // 4. Search Results List (+20% scaled)
    float listY = sectionY + 28.0f;
    float rowH = 65.0f;
    float rowW = w - 28.0f;
    float rowX = 14.0f;

    Vector2 mousePos = GetMousePosition();

    if (search->result_count == 0 && !search->is_showing_suggestions) {
        char noRes[128];
        snprintf(noRes, sizeof(noRes), "No results found for \"%s\"", kb->text);
        Vector2 nSize = MeasureTextEx(s_fontTitle, noRes, 22.0f, 1.0f);
        DrawTextEx(s_fontTitle, noRes,
                   (Vector2){ (w - nSize.x) / 2.0f, listY + 70.0f },
                   22.0f, 1.0f, (Color){ 140, 145, 158, 200 });
    } else {
        int visibleCount = search->result_count - search->scroll_offset;
        if (visibleCount > MAX_VISIBLE_ROWS) visibleCount = MAX_VISIBLE_ROWS;

        for (int v = 0; v < visibleCount; v++) {
            int idx = search->scroll_offset + v;
            SearchResultItem *res = &search->results[idx];

            float ry = listY + (v * rowH);
            Rectangle rowRec = { rowX, ry, rowW, rowH - 5.0f };

            // Mouse hover selection
            if (CheckCollisionPointRec(mousePos, rowRec)) {
                search->selected_index = idx;
            }

            bool isSelected = (idx == search->selected_index);

            // Selection highlight pill with subtle border
            if (isSelected) {
                Color selBg = (Color){ 255, 255, 255, 28 };
                Color selBorder = (Color){ 255, 255, 255, 38 };
                DrawRectangleRounded(rowRec, 0.16f, 8, selBg);
                DrawRectangleRoundedLines(rowRec, 0.16f, 8, 1.0f, selBorder);
            }

            // Left Icon (+20% scaled: 40x40)
            float iconSize = 40.0f;
            float iconX = rowRec.x + 14.0f;
            float iconY = rowRec.y + (rowRec.height - iconSize) / 2.0f;

            if (res->is_command) {
                DrawCommandIcon(iconX, iconY, iconSize, res->title);
            } else {
                Texture2D iconTex = GetOrLoadAppIcon(res->path);
                if (iconTex.id > 0) {
                    Rectangle srcRec = { 0.0f, 0.0f, (float)iconTex.width, (float)iconTex.height };
                    Rectangle dstRec = { iconX, iconY, iconSize, iconSize };
                    DrawTexturePro(iconTex, srcRec, dstRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                } else {
                    DrawCommandIcon(iconX, iconY, iconSize, res->title);
                }
            }

            // Title (Segoe UI Semibold 22px)
            float titleX = iconX + iconSize + 16.0f;
            float titleY = rowRec.y + (rowRec.height - 22.0f) / 2.0f;
            Color titleColor = isSelected ? (Color){ 255, 255, 255, 255 } : (Color){ 240, 244, 250, 250 };
            DrawTextEx(s_fontTitle, res->title, (Vector2){ titleX, titleY }, 22.0f, 1.0f, titleColor);

            // Subtitle (crisp, secondary tone 17px)
            Vector2 titleMetrics = MeasureTextEx(s_fontTitle, res->title, 22.0f, 1.0f);
            float subX = titleX + titleMetrics.x + 14.0f;
            float subY = titleY + 3.0f;
            Color subColor = isSelected ? (Color){ 175, 182, 195, 220 } : (Color){ 135, 140, 150, 190 };
            DrawTextEx(s_fontSubtitle, res->subtitle, (Vector2){ subX, subY }, 17.0f, 1.0f, subColor);

            // Right Category Label ("Application", "Command", "Math" 16px)
            Vector2 catMetrics = MeasureTextEx(s_fontSubtitle, res->category_label, 16.0f, 1.0f);
            float catX = rowRec.x + rowRec.width - catMetrics.x - 18.0f;
            float catY = titleY + 3.0f;
            Color catColor = isSelected ? (Color){ 180, 185, 200, 220 } : (Color){ 130, 135, 148, 180 };
            DrawTextEx(s_fontSubtitle, res->category_label, (Vector2){ catX, catY }, 16.0f, 1.0f, catColor);
        }
    }

    // 5. Clean Footer Navigation Hints (Standard ASCII, zero '?' marks)
    float footerY = h - 30.0f;
    const char *hints = "Press Enter to open   |   Up / Down to navigate   |   Esc to close";
    Vector2 hMetrics = MeasureTextEx(s_fontFooter, hints, 15.0f, 1.0f);
    DrawTextEx(s_fontFooter, hints,
               (Vector2){ (w - hMetrics.x) / 2.0f, footerY },
               15.0f, 1.0f, (Color){ 120, 126, 138, 190 });
}
