#ifndef FLARE_SEARCH_H
#define FLARE_SEARCH_H

#include <stdbool.h>
#include "applications.h"
#include "history.h"

#define MAX_SEARCH_RESULTS 24
#define MAX_VISIBLE_ROWS    7

typedef struct {
    char title[128];
    char subtitle[64];
    char category_label[32]; // "Application" or "Command"
    char path[512];
    bool is_command;
    int match_score;
} SearchResultItem;

typedef struct {
    char query[256];
    SearchResultItem results[MAX_SEARCH_RESULTS];
    int result_count;
    int selected_index;
    int scroll_offset;
    bool is_showing_suggestions;
} SearchEngine;

void Search_Init(SearchEngine *engine);
void Search_Cleanup(SearchEngine *engine);
void Search_PopulateSuggestions(SearchEngine *engine, const ApplicationCache *apps, const HistoryManager *hist);
void Search_Execute(SearchEngine *engine, const ApplicationCache *apps, const HistoryManager *hist, const char *query);
void Search_SelectNext(SearchEngine *engine);
void Search_SelectPrev(SearchEngine *engine);

#endif // FLARE_SEARCH_H
