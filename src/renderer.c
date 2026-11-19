#include "renderer.h"
#include "raylib.h"
#include "applications.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Multi-size font system for crystal-clear typography
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
    GenTextureMipmaps(&f.texture);
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    return f;
}

void Renderer_Init(void) {
    const char *fontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    if (FileExists(fontPath)) {
        s_fontInput    = LoadCrispFont(fontPath, 19);
        s_fontTitle    = LoadCrispFont(fontPath, 15);
        s_fontSubtitle = LoadCrispFont(fontPath, 13);
        s_fontSection  = LoadCrispFont(fontPath, 12);
        s_fontFooter   = LoadCrispFont(fontPath, 11);
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
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);

    int idx = s_iconCacheCount++;
    strncpy(s_iconCache[idx].path, path, sizeof(s_iconCache[idx].path) - 1);
    s_iconCache[idx].path[sizeof(s_iconCache[idx].path) - 1] = '\0';
    s_iconCache[idx].texture = tex;
    s_iconCache[idx].loaded = true;
    return tex;
}

// Draw vector icon for system commands
static void DrawCommandIcon(float x, float y, float size, const char *title) {
    Rectangle bgRec = { x, y, size, size };

    // Dedicated calculator icon for math evaluations
    if (title && (title[0] == '=' || strstr(title, "Calc") != NULL)) {
        Color calcBg = (Color){ 240, 130, 25, 240 }; // Spotlight orange calculator accent
        DrawRectangleRounded(bgRec, 0.28f, 8, calcBg);
        float cx = x + size / 2.0f;
        float cy = y + size / 2.0f;
        float lineW = 9.0f;
        float lineH = 1.8f;
        DrawRectangleRounded((Rectangle){ cx - lineW/2.0f, cy - 2.5f, lineW, lineH }, 0.5f, 4, WHITE);
        DrawRectangleRounded((Rectangle){ cx - lineW/2.0f, cy + 2.0f, lineW, lineH }, 0.5f, 4, WHITE);
        return;
    }

    Color iconBg = (Color){ 42, 44, 52, 230 };
    Color iconBorder = (Color){ 255, 255, 255, 26 };
    DrawRectangleRounded(bgRec, 0.28f, 8, iconBg);
    DrawRectangleRoundedLines(bgRec, 0.28f, 8, 1.0f, iconBorder);

    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    Color fgColor = (Color){ 220, 224, 235, 235 };

    if (strstr(title, "Quit All") != NULL) {
        // Encircled X glyph
        DrawCircleLines((int)cx, (int)cy, 6.0f, fgColor);
        float d = 3.0f;
        DrawLineEx((Vector2){ cx - d, cy - d }, (Vector2){ cx + d, cy + d }, 1.5f, fgColor);
        DrawLineEx((Vector2){ cx + d, cy - d }, (Vector2){ cx - d, cy + d }, 1.5f, fgColor);
    } else if (strstr(title, "Reminder") != NULL) {
        // Checkbox glyph with checkmark
        Rectangle box = { cx - 5.5f, cy - 5.5f, 11.0f, 11.0f };
        DrawRectangleRoundedLines(box, 0.3f, 4, 1.2f, fgColor);
        Vector2 p1 = { cx - 3.0f, cy + 0.0f };
        Vector2 p2 = { cx - 0.5f, cy + 2.8f };
        Vector2 p3 = { cx + 3.8f, cy - 2.8f };
        DrawLineEx(p1, p2, 1.5f, fgColor);
        DrawLineEx(p2, p3, 1.5f, fgColor);
    } else if (strstr(title, "Lock") != NULL) {
        // Padlock glyph
        Rectangle lockBody = { cx - 4.5f, cy - 1.0f, 9.0f, 6.5f };
        DrawRectangleRounded(lockBody, 0.2f, 4, fgColor);
        DrawRing((Vector2){ cx, cy - 1.0f }, 2.5f, 4.0f, 180.0f, 360.0f, 16, fgColor);
    } else {
        // First letter badge
        char letter[2] = { title[0], '\0' };
        Vector2 lSize = MeasureTextEx(s_fontTitle, letter, 13.0f, 1.0f);
        DrawTextEx(s_fontTitle, letter, (Vector2){ cx - lSize.x / 2.0f, cy - lSize.y / 2.0f }, 13.0f, 1.0f, fgColor);
    }
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
    DrawRectangleRoundedLines((Rectangle){ 1, 1, w - 2, h - 2 }, 0.06f, 16, 1.0f, (Color){ 255, 255, 255, 28 });

    // Specular top highlight line
    DrawLineEx((Vector2){ 14.0f, 1.0f }, (Vector2){ w - 14.0f, 1.0f }, 1.0f, (Color){ 255, 255, 255, 30 });

    // 2. Search Input Header (Clean, no AI or Apple badges)
    float headerH = 56.0f;
    float textX = 22.0f;
    float textY = (headerH - 20.0f) / 2.0f;

    if (kb->length == 0) {
        const char *placeholder = "Search for apps and commands...";
        DrawTextEx(s_fontInput, placeholder, (Vector2){ textX, textY }, 19.0f, 1.0f, (Color){ 125, 130, 140, 200 });
    } else {
        DrawTextEx(s_fontInput, kb->text, (Vector2){ textX, textY }, 19.0f, 1.0f, (Color){ 245, 247, 252, 255 });
    }

    // Blinking Cursor
    float cursorX = textX;
    if (kb->length > 0) {
        Vector2 textMetrics = MeasureTextEx(s_fontInput, kb->text, 19.0f, 1.0f);
        cursorX = textX + textMetrics.x + 2.0f;
    }
    float cursorW = 2.0f;
    float cursorH = 20.0f;
    float cursorY = (headerH - cursorH) / 2.0f;
    Color cursorColor = (Color){ 215, 220, 230, (unsigned char)(cursorAlpha * 240.0f) };
    DrawRectangleRounded((Rectangle){ cursorX, cursorY, cursorW, cursorH }, 0.5f, 4, cursorColor);

    // Divider Line
    DrawLineEx((Vector2){ 0.0f, headerH }, (Vector2){ w, headerH }, 1.0f, (Color){ 255, 255, 255, 18 });

    // 3. Section Header ("Suggestions" or "Applications & Commands")
    float sectionY = headerH + 12.0f;
    const char *sectionTitle = search->is_showing_suggestions ? "Suggestions" : "Applications & Commands";
    DrawTextEx(s_fontSection, sectionTitle, (Vector2){ 22.0f, sectionY }, 12.0f, 1.0f, (Color){ 130, 134, 145, 215 });

    // 4. Search Results List
    float listY = sectionY + 22.0f;
    float rowH = 46.0f;
    float rowW = w - 16.0f;
    float rowX = 8.0f;

    Vector2 mousePos = GetMousePosition();

    if (search->result_count == 0 && !search->is_showing_suggestions) {
        char noRes[128];
        snprintf(noRes, sizeof(noRes), "No results found for \"%s\"", kb->text);
        Vector2 nSize = MeasureTextEx(s_fontTitle, noRes, 15.0f, 1.0f);
        DrawTextEx(s_fontTitle, noRes,
                   (Vector2){ (w - nSize.x) / 2.0f, listY + 50.0f },
                   15.0f, 1.0f, (Color){ 130, 135, 145, 200 });
    } else {
        int visibleCount = search->result_count - search->scroll_offset;
        if (visibleCount > MAX_VISIBLE_ROWS) visibleCount = MAX_VISIBLE_ROWS;

        for (int v = 0; v < visibleCount; v++) {
            int idx = search->scroll_offset + v;
            SearchResultItem *res = &search->results[idx];

            float ry = listY + (v * rowH);
            Rectangle rowRec = { rowX, ry, rowW, rowH - 2.0f };

            // Mouse hover selection
            if (CheckCollisionPointRec(mousePos, rowRec)) {
                search->selected_index = idx;
            }

            bool isSelected = (idx == search->selected_index);

            // Selection highlight pill
            if (isSelected) {
                Color selBg = (Color){ 255, 255, 255, 24 };
                DrawRectangleRounded(rowRec, 0.20f, 8, selBg);
            }

            // Left Icon (Clean without shortcut overlay arrow)
            float iconSize = 26.0f;
            float iconX = rowRec.x + 12.0f;
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

            // Title
            float titleX = iconX + iconSize + 12.0f;
            float titleY = rowRec.y + (rowRec.height - 15.0f) / 2.0f - 1.0f;
            DrawTextEx(s_fontTitle, res->title, (Vector2){ titleX, titleY }, 15.0f, 1.0f, (Color){ 245, 247, 252, 255 });

            // Subtitle (crisp, secondary tone)
            Vector2 titleMetrics = MeasureTextEx(s_fontTitle, res->title, 15.0f, 1.0f);
            float subX = titleX + titleMetrics.x + 10.0f;
            float subY = rowRec.y + (rowRec.height - 13.0f) / 2.0f;
            DrawTextEx(s_fontSubtitle, res->subtitle, (Vector2){ subX, subY }, 13.0f, 1.0f, (Color){ 135, 140, 150, 190 });

            // Right Category Label ("Application" or "Command")
            Vector2 catMetrics = MeasureTextEx(s_fontSubtitle, res->category_label, 13.0f, 1.0f);
            float catX = rowRec.x + rowRec.width - catMetrics.x - 14.0f;
            float catY = rowRec.y + (rowRec.height - 13.0f) / 2.0f;
            DrawTextEx(s_fontSubtitle, res->category_label, (Vector2){ catX, catY }, 13.0f, 1.0f, (Color){ 125, 130, 140, 180 });
        }
    }

    // 5. Clean Footer Navigation Hints (Standard clean text, zero '?' marks)
    float footerY = h - 22.0f;
    const char *hints = "Enter to open   |   Up / Down to navigate   |   Esc to close";
    Vector2 hMetrics = MeasureTextEx(s_fontFooter, hints, 11.0f, 1.0f);
    DrawTextEx(s_fontFooter, hints,
               (Vector2){ (w - hMetrics.x) / 2.0f, footerY },
               11.0f, 1.0f, (Color){ 110, 115, 125, 170 });
}
