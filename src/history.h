#ifndef FLARE_HISTORY_H
#define FLARE_HISTORY_H

#include "applications.h"
#include <stdbool.h>

#define MAX_HISTORY_ITEMS 5

typedef struct {
    char name[MAX_APP_NAME];
    char subtitle[MAX_APP_SUBTITLE];
    char category[MAX_APP_CATEGORY];
    char path[MAX_APP_PATH];
    bool is_command;
} HistoryItem;

typedef struct {
    HistoryItem items[MAX_HISTORY_ITEMS];
    int count;
} HistoryManager;

void History_Init(HistoryManager *hist);
void History_Load(HistoryManager *hist, const ApplicationCache *apps);
void History_Save(const HistoryManager *hist);
void History_Add(HistoryManager *hist, const char *name, const char *path, const char *subtitle, const char *category, bool is_command);

#endif // FLARE_HISTORY_H
