#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void GetHistoryFilePath(char *outPath, size_t maxLen) {
    char appData[MAX_PATH];
    if (GetEnvironmentVariableA("APPDATA", appData, sizeof(appData)) > 0) {
        char dir[MAX_PATH];
        snprintf(dir, sizeof(dir), "%s\\Flare", appData);
        CreateDirectoryA(dir, NULL);
        snprintf(outPath, maxLen, "%s\\recent_apps.txt", dir);
    } else {
        snprintf(outPath, maxLen, "recent_apps.txt");
    }
}

void History_Init(HistoryManager *hist) {
    if (!hist) return;
    memset(hist, 0, sizeof(HistoryManager));
}

void History_Save(const HistoryManager *hist) {
    if (!hist) return;

    char filePath[MAX_PATH];
    GetHistoryFilePath(filePath, sizeof(filePath));

    FILE *f = fopen(filePath, "w");
    if (!f) return;

    for (int i = 0; i < hist->count && i < MAX_HISTORY_ITEMS; i++) {
        const HistoryItem *it = &hist->items[i];
        fprintf(f, "%s|%s|%s|%s|%d\n",
                it->name,
                it->path,
                it->subtitle,
                it->category,
                it->is_command ? 1 : 0);
    }
    fclose(f);
}

void History_Load(HistoryManager *hist, const ApplicationCache *apps) {
    if (!hist) return;
    hist->count = 0;

    char filePath[MAX_PATH];
    GetHistoryFilePath(filePath, sizeof(filePath));

    FILE *f = fopen(filePath, "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f) && hist->count < MAX_HISTORY_ITEMS) {
            // Trim newline
            size_t len = strlen(line);
            while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
                line[--len] = '\0';
            }
            if (len == 0) continue;

            char *p1 = strchr(line, '|');
            if (!p1) continue;
            *p1 = '\0';
            char *name = line;

            char *p2 = strchr(p1 + 1, '|');
            if (!p2) continue;
            *p2 = '\0';
            char *path = p1 + 1;

            char *p3 = strchr(p2 + 1, '|');
            if (!p3) continue;
            *p3 = '\0';
            char *subtitle = p2 + 1;

            char *p4 = strchr(p3 + 1, '|');
            if (!p4) continue;
            *p4 = '\0';
            char *category = p3 + 1;
            int is_cmd = atoi(p4 + 1);

            HistoryItem *it = &hist->items[hist->count++];
            strncpy(it->name, name, sizeof(it->name) - 1);
            it->name[sizeof(it->name) - 1] = '\0';

            strncpy(it->path, path, sizeof(it->path) - 1);
            it->path[sizeof(it->path) - 1] = '\0';

            strncpy(it->subtitle, subtitle, sizeof(it->subtitle) - 1);
            it->subtitle[sizeof(it->subtitle) - 1] = '\0';

            strncpy(it->category, category, sizeof(it->category) - 1);
            it->category[sizeof(it->category) - 1] = '\0';

            it->is_command = (is_cmd != 0);
        }
        fclose(f);
    }

    // If history has fewer than 5 items, seed with common installed apps
    if (hist->count < MAX_HISTORY_ITEMS && apps && apps->count > 0) {
        const char *defaults[] = {
            "Brave Browser",
            "Google Chrome",
            "Notepad",
            "Command Prompt",
            "Calculator",
            "Task Manager",
            "File Explorer"
        };
        for (size_t d = 0; d < sizeof(defaults) / sizeof(defaults[0]) && hist->count < MAX_HISTORY_ITEMS; d++) {
            // Check if already in history
            bool exists = false;
            for (int h = 0; h < hist->count; h++) {
                if (_stricmp(hist->items[h].name, defaults[d]) == 0) {
                    exists = true;
                    break;
                }
            }
            if (exists) continue;

            // Find in apps cache
            for (int a = 0; a < apps->count; a++) {
                if (_stricmp(apps->items[a].name, defaults[d]) == 0) {
                    HistoryItem *it = &hist->items[hist->count++];
                    strncpy(it->name, apps->items[a].name, sizeof(it->name) - 1);
                    it->name[sizeof(it->name) - 1] = '\0';

                    strncpy(it->path, apps->items[a].path, sizeof(it->path) - 1);
                    it->path[sizeof(it->path) - 1] = '\0';

                    strncpy(it->subtitle, apps->items[a].subtitle, sizeof(it->subtitle) - 1);
                    it->subtitle[sizeof(it->subtitle) - 1] = '\0';

                    strncpy(it->category, apps->items[a].category, sizeof(it->category) - 1);
                    it->category[sizeof(it->category) - 1] = '\0';

                    it->is_command = apps->items[a].is_command;
                    break;
                }
            }
        }
        // Save initial seeded history
        History_Save(hist);
    }
}

void History_Add(HistoryManager *hist, const char *name, const char *path, const char *subtitle, const char *category, bool is_command) {
    if (!hist || !name || !path) return;

    // Check if item already exists in history; if so, remove it to move to front
    int existingIdx = -1;
    for (int i = 0; i < hist->count; i++) {
        if (_stricmp(hist->items[i].name, name) == 0 || _stricmp(hist->items[i].path, path) == 0) {
            existingIdx = i;
            break;
        }
    }

    if (existingIdx >= 0) {
        // Shift items after existingIdx to the left
        for (int i = existingIdx; i < hist->count - 1; i++) {
            hist->items[i] = hist->items[i + 1];
        }
        hist->count--;
    } else if (hist->count >= MAX_HISTORY_ITEMS) {
        // Drop the oldest item
        hist->count = MAX_HISTORY_ITEMS - 1;
    }

    // Shift all items right to make room at index 0 (Most Recently Used)
    for (int i = hist->count; i > 0; i--) {
        hist->items[i] = hist->items[i - 1];
    }

    // Insert at index 0
    HistoryItem *it = &hist->items[0];
    memset(it, 0, sizeof(HistoryItem));
    strncpy(it->name, name, sizeof(it->name) - 1);
    it->name[sizeof(it->name) - 1] = '\0';

    strncpy(it->path, path, sizeof(it->path) - 1);
    it->path[sizeof(it->path) - 1] = '\0';

    if (subtitle) {
        strncpy(it->subtitle, subtitle, sizeof(it->subtitle) - 1);
        it->subtitle[sizeof(it->subtitle) - 1] = '\0';
    }
    if (category) {
        strncpy(it->category, category, sizeof(it->category) - 1);
        it->category[sizeof(it->category) - 1] = '\0';
    }
    it->is_command = is_command;
    hist->count++;

    // Persist immediately to disk
    History_Save(hist);
}
