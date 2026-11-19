#include "applications.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static void AddItem(ApplicationCache *cache, const char *name, const char *subtitle,
                    const char *category, const char *path, bool is_command) {
    if (!cache || !name || !path) return;

    // Check for duplicate name
    for (int i = 0; i < cache->count; i++) {
        if (_stricmp(cache->items[i].name, name) == 0) {
            return;
        }
    }

    if (cache->count >= cache->capacity) {
        int newCap = cache->capacity == 0 ? 64 : cache->capacity * 2;
        ApplicationItem *newItems = (ApplicationItem*)realloc(cache->items, newCap * sizeof(ApplicationItem));
        if (!newItems) return;
        cache->items = newItems;
        cache->capacity = newCap;
    }

    ApplicationItem *item = &cache->items[cache->count++];
    strncpy(item->name, name, MAX_APP_NAME - 1);
    item->name[MAX_APP_NAME - 1] = '\0';

    strncpy(item->subtitle, subtitle ? subtitle : "Application", MAX_APP_SUBTITLE - 1);
    item->subtitle[MAX_APP_SUBTITLE - 1] = '\0';

    strncpy(item->category, category ? category : "Application", MAX_APP_CATEGORY - 1);
    item->category[MAX_APP_CATEGORY - 1] = '\0';

    strncpy(item->path, path, MAX_APP_PATH - 1);
    item->path[MAX_APP_PATH - 1] = '\0';

    item->is_command = is_command;
}

static bool ShouldIgnoreShortcut(const char *name) {
    char lower[256];
    int len = (int)strlen(name);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (int i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)name[i]);
    }
    lower[len] = '\0';

    const char *blacklist[] = {
        "uninstall", "remove", "setup", "help", "readme",
        "documentation", "license", "website", "homepage",
        "manual", "release notes", "configuration", "install"
    };

    for (size_t i = 0; i < sizeof(blacklist) / sizeof(blacklist[0]); i++) {
        if (strstr(lower, blacklist[i]) != NULL) {
            return true;
        }
    }
    return false;
}

static void ScanDirectory(ApplicationCache *cache, const char *dir, const char *suite) {
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            const char *subSuite = suite;
            if (strcmp(fd.cFileName, "Programs") != 0 &&
                strcmp(fd.cFileName, "Accessories") != 0 &&
                strcmp(fd.cFileName, "System Tools") != 0) {
                subSuite = fd.cFileName;
            }
            ScanDirectory(cache, fullPath, subSuite);
        } else {
            const char *ext = strrchr(fd.cFileName, '.');
            if (ext && (_stricmp(ext, ".lnk") == 0 || _stricmp(ext, ".url") == 0)) {
                char cleanName[MAX_APP_NAME];
                int nameLen = (int)(ext - fd.cFileName);
                if (nameLen >= MAX_APP_NAME) nameLen = MAX_APP_NAME - 1;
                strncpy(cleanName, fd.cFileName, nameLen);
                cleanName[nameLen] = '\0';

                if (ShouldIgnoreShortcut(cleanName)) continue;

                if (_stricmp(cleanName, "Brave") == 0) {
                    strcpy(cleanName, "Brave Browser");
                }

                const char *subtitle = (suite && strlen(suite) > 0) ? suite : "Application";
                AddItem(cache, cleanName, subtitle, "Application", fullPath, false);
            }
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

void Applications_Init(ApplicationCache *cache) {
    if (!cache) return;
    cache->items = NULL;
    cache->count = 0;
    cache->capacity = 0;
    cache->is_indexed = false;
}

void Applications_Cleanup(ApplicationCache *cache) {
    if (!cache) return;
    if (cache->items) {
        free(cache->items);
        cache->items = NULL;
    }
    cache->count = 0;
    cache->capacity = 0;
    cache->is_indexed = false;
}

void Applications_BuildIndex(ApplicationCache *cache) {
    if (!cache) return;

    // Built-in Commands - Clean labels without Apple or AI text
    AddItem(cache, "Quit All Apps", "System", "Command", "cmd_quit_all_apps", true);
    AddItem(cache, "Create Reminder", "Reminders", "Command", "cmd_create_reminder", true);
    AddItem(cache, "Lock PC", "System", "Command", "cmd_lock_pc", true);
    AddItem(cache, "Empty Recycle Bin", "System", "Command", "cmd_empty_bin", true);

    // Built-in Windows Accessories & System Applications
    AddItem(cache, "Task Manager", "System", "Application", "taskmgr.exe", false);
    AddItem(cache, "Notepad", "Accessories", "Application", "notepad.exe", false);
    AddItem(cache, "Calculator", "Accessories", "Application", "calc.exe", false);
    AddItem(cache, "Command Prompt", "System", "Application", "cmd.exe", false);
    AddItem(cache, "PowerShell", "System", "Application", "powershell.exe", false);
    AddItem(cache, "Settings", "System", "Application", "start ms-settings:", true);
    AddItem(cache, "File Explorer", "System", "Application", "explorer.exe", false);
    AddItem(cache, "Snipping Tool", "Accessories", "Application", "snippingtool.exe", false);
    AddItem(cache, "Paint", "Accessories", "Application", "mspaint.exe", false);

    // Scan Common and User Start Menu folders
    ScanDirectory(cache, "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs", "");

    char appData[MAX_PATH];
    if (GetEnvironmentVariableA("APPDATA", appData, sizeof(appData)) > 0) {
        char userPrograms[MAX_PATH];
        snprintf(userPrograms, sizeof(userPrograms), "%s\\Microsoft\\Windows\\Start Menu\\Programs", appData);
        ScanDirectory(cache, userPrograms, "");
    }

    cache->is_indexed = true;
}

static BOOL CALLBACK CloseTopLevelWindow(HWND hwnd, LPARAM lParam) {
    (void)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;

    char title[128];
    GetWindowTextA(hwnd, title, sizeof(title));
    if (strlen(title) == 0) return TRUE;

    char className[64];
    GetClassNameA(hwnd, className, sizeof(className));
    if (strcmp(className, "Shell_TrayWnd") == 0 ||
        strcmp(className, "Progman") == 0 ||
        strcmp(className, "WorkerW") == 0 ||
        strcmp(className, "Windows.UI.Core.CoreWindow") == 0) {
        return TRUE;
    }

    PostMessageA(hwnd, WM_CLOSE, 0, 0);
    return TRUE;
}

bool Applications_Execute(const char *path, bool is_command, const char *name) {
    if (!path) return false;
    (void)name;

    if (is_command) {
        if (strcmp(path, "cmd_quit_all_apps") == 0) {
            EnumWindows(CloseTopLevelWindow, 0);
            return true;
        } else if (strcmp(path, "cmd_lock_pc") == 0) {
            LockWorkStation();
            return true;
        } else if (strcmp(path, "cmd_empty_bin") == 0) {
            SHEmptyRecycleBinA(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI);
            return true;
        } else if (strcmp(path, "cmd_create_reminder") == 0) {
            ShellExecuteA(NULL, "open", "ms-todo:", NULL, NULL, SW_SHOWNORMAL);
            return true;
        } else if (strncmp(path, "start ", 6) == 0) {
            ShellExecuteA(NULL, "open", path + 6, NULL, NULL, SW_SHOWNORMAL);
            return true;
        }
    }

    HINSTANCE res = ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)res > 32);
}

// Fast binary parser to resolve .lnk shortcuts to pure target .exe (removes shortcut arrow)
static bool GetLnkTarget(const char *lnkPath, char *outTarget, int maxLen) {
    FILE *f = fopen(lnkPath, "rb");
    if (!f) return false;

    unsigned char header[76];
    if (fread(header, 1, 76, f) < 76) { fclose(f); return false; }

    unsigned int sig = 0;
    memcpy(&sig, header, 4);
    if (sig != 0x0000004C) { fclose(f); return false; }

    unsigned int flags = 0;
    memcpy(&flags, header + 0x14, 4);

    // If HasLinkTargetIDList (bit 0)
    if (flags & 0x01) {
        unsigned short idListSize = 0;
        if (fread(&idListSize, 1, 2, f) < 2) { fclose(f); return false; }
        fseek(f, (long)idListSize, SEEK_CUR);
    }

    // If HasLinkInfo (bit 1)
    if (flags & 0x02) {
        long linkInfoStart = ftell(f);
        unsigned int linkInfoSize = 0;
        if (fread(&linkInfoSize, 1, 4, f) < 4) { fclose(f); return false; }

        unsigned int localBasePathOffset = 0;
        fseek(f, linkInfoStart + 0x10, SEEK_SET);
        if (fread(&localBasePathOffset, 1, 4, f) < 4) { fclose(f); return false; }

        if (localBasePathOffset > 0 && localBasePathOffset < linkInfoSize) {
            fseek(f, linkInfoStart + (long)localBasePathOffset, SEEK_SET);
            if (fgets(outTarget, maxLen, f)) {
                fclose(f);
                return true;
            }
        }
    }
    fclose(f);
    return false;
}

unsigned char* Applications_ExtractIconRGBA(const char *path, int *out_w, int *out_h) {
    if (!path || !out_w || !out_h) return NULL;
    *out_w = 0;
    *out_h = 0;

    // Resolve .lnk shortcut to actual .exe to obtain clean unbadged high-res icon
    const char *iconPath = path;
    char targetPath[MAX_PATH] = { 0 };
    const char *ext = strrchr(path, '.');
    if (ext && _stricmp(ext, ".lnk") == 0) {
        if (GetLnkTarget(path, targetPath, sizeof(targetPath))) {
            if (GetFileAttributesA(targetPath) != INVALID_FILE_ATTRIBUTES) {
                iconPath = targetPath;
            }
        }
    }

    HICON hIcon = NULL;

    // First try extracting directly from target .exe for clean native icon without shortcut overlay
    if (iconPath != path) {
        ExtractIconExA(iconPath, 0, &hIcon, NULL, 1);
    }

    // Fallback to ShellFileInfo
    if (!hIcon) {
        SHFILEINFOA sfi;
        ZeroMemory(&sfi, sizeof(sfi));
        DWORD_PTR hr = SHGetFileInfoA(iconPath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
        if (!hr || !sfi.hIcon) {
            hr = SHGetFileInfoA(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
            if (!hr || !sfi.hIcon) return NULL;
        }
        hIcon = sfi.hIcon;
    }

    ICONINFO ii;
    if (!GetIconInfo(hIcon, &ii)) {
        DestroyIcon(hIcon);
        return NULL;
    }

    BITMAP bm;
    GetObject(ii.hbmColor, sizeof(bm), &bm);
    int w = bm.bmWidth;
    int h = bm.bmHeight;
    if (w <= 0 || h <= 0) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        DestroyIcon(hIcon);
        return NULL;
    }

    HDC hdc = GetDC(NULL);
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    unsigned char *pixels = (unsigned char*)malloc(w * h * 4);
    if (!pixels) {
        ReleaseDC(NULL, hdc);
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        DestroyIcon(hIcon);
        return NULL;
    }

    GetDIBits(hdc, ii.hbmColor, 0, h, pixels, &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
    DestroyIcon(hIcon);

    bool hasAlpha = false;
    for (int i = 0; i < w * h; i++) {
        if (pixels[i * 4 + 3] > 0) {
            hasAlpha = true;
            break;
        }
    }

    // Convert BGRA to RGBA
    for (int i = 0; i < w * h; i++) {
        unsigned char b = pixels[i * 4 + 0];
        unsigned char g = pixels[i * 4 + 1];
        unsigned char r = pixels[i * 4 + 2];
        unsigned char a = pixels[i * 4 + 3];
        if (!hasAlpha) {
            a = (r > 0 || g > 0 || b > 0) ? 255 : 0;
        }
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }

    *out_w = w;
    *out_h = h;
    return pixels;
}
