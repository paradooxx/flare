#include "search.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void Search_Init(SearchEngine *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(SearchEngine));
    engine->selected_index = 0;
    engine->scroll_offset = 0;
    engine->is_showing_suggestions = true;
}

void Search_Cleanup(SearchEngine *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(SearchEngine));
}

static void ToLowerStr(const char *src, char *dst, int maxLen) {
    int i = 0;
    while (src[i] && i < maxLen - 1) {
        dst[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dst[i] = '\0';
}

static void AddResult(SearchEngine *engine, const ApplicationItem *item, int score) {
    if (engine->result_count >= MAX_SEARCH_RESULTS) return;

    SearchResultItem *res = &engine->results[engine->result_count++];
    strncpy(res->title, item->name, sizeof(res->title) - 1);
    res->title[sizeof(res->title) - 1] = '\0';

    strncpy(res->subtitle, item->subtitle, sizeof(res->subtitle) - 1);
    res->subtitle[sizeof(res->subtitle) - 1] = '\0';

    strncpy(res->category_label, item->category, sizeof(res->category_label) - 1);
    res->category_label[sizeof(res->category_label) - 1] = '\0';

    strncpy(res->path, item->path, sizeof(res->path) - 1);
    res->path[sizeof(res->path) - 1] = '\0';

    res->is_command = item->is_command;
    res->match_score = score;
}

void Search_PopulateSuggestions(SearchEngine *engine, const ApplicationCache *apps) {
    if (!engine) return;
    engine->result_count = 0;
    engine->selected_index = 0;
    engine->scroll_offset = 0;
    engine->is_showing_suggestions = true;

    if (!apps || apps->count == 0) return;

    // Ordered list of preferred suggestions matching user UI
    const char *preferred[] = {
        "Quit All Apps",
        "Google Chrome",
        "Brave Browser",
        "Spotify",
        "Warp",
        "Command Prompt",
        "Create Reminder",
        "Task Manager",
        "Calculator",
        "Notepad",
        "Settings"
    };

    // First, add existing preferred suggestions
    for (size_t p = 0; p < sizeof(preferred) / sizeof(preferred[0]); p++) {
        for (int i = 0; i < apps->count; i++) {
            if (_stricmp(apps->items[i].name, preferred[p]) == 0) {
                AddResult(engine, &apps->items[i], 100);
                break;
            }
        }
    }

    // If we have fewer than 7 suggestions, fill with other installed apps
    for (int i = 0; i < apps->count && engine->result_count < 8; i++) {
        bool alreadyAdded = false;
        for (int r = 0; r < engine->result_count; r++) {
            if (strcmp(engine->results[r].title, apps->items[i].name) == 0) {
                alreadyAdded = true;
                break;
            }
        }
        if (!alreadyAdded) {
            AddResult(engine, &apps->items[i], 50);
        }
    }
}

static int CalculateMatchScore(const char *name, const char *sub, const char *qLower) {
    char nameLower[128];
    ToLowerStr(name, nameLower, sizeof(nameLower));

    // Exact match
    if (strcmp(nameLower, qLower) == 0) return 100;

    // Starts with query
    if (strncmp(nameLower, qLower, strlen(qLower)) == 0) {
        return 85;
    }

    // Word boundary start (e.g. "Chrome" in "Google Chrome")
    char padded[140];
    snprintf(padded, sizeof(padded), " %s", nameLower);
    char queryWord[70];
    snprintf(queryWord, sizeof(queryWord), " %s", qLower);
    if (strstr(padded, queryWord) != NULL) {
        return 70;
    }

    // Substring match
    if (strstr(nameLower, qLower) != NULL) {
        return 50;
    }

    // Acronym match (e.g. "gc" for "Google Chrome")
    if (strlen(qLower) >= 2 && strlen(qLower) <= 4) {
        char acronym[16] = { 0 };
        int acIdx = 0;
        bool nextIsInitial = true;
        for (size_t i = 0; i < strlen(name); i++) {
            if (isalnum((unsigned char)name[i])) {
                if (nextIsInitial && acIdx < 15) {
                    acronym[acIdx++] = (char)tolower((unsigned char)name[i]);
                    nextIsInitial = false;
                }
            } else {
                nextIsInitial = true;
            }
        }
        acronym[acIdx] = '\0';
        if (strncmp(acronym, qLower, strlen(qLower)) == 0) {
            return 65;
        }
    }

    // Subtitle match
    if (sub && strlen(sub) > 0) {
        char subLower[64];
        ToLowerStr(sub, subLower, sizeof(subLower));
        if (strstr(subLower, qLower) != NULL) {
            return 30;
        }
    }

    return 0;
}

void Search_Execute(SearchEngine *engine, const ApplicationCache *apps, const char *query) {
    if (!engine) return;

    if (!query || strlen(query) == 0) {
        engine->query[0] = '\0';
        Search_PopulateSuggestions(engine, apps);
        return;
    }

    strncpy(engine->query, query, sizeof(engine->query) - 1);
    engine->query[sizeof(engine->query) - 1] = '\0';
    engine->result_count = 0;
    engine->selected_index = 0;
    engine->scroll_offset = 0;
    engine->is_showing_suggestions = false;

    if (!apps || apps->count == 0) return;

    char qLower[64];
    ToLowerStr(query, qLower, sizeof(qLower));

    // Temporary storage for sorting
    typedef struct {
        int appIndex;
        int score;
    } ScoredMatch;

    ScoredMatch matches[MAX_SEARCH_RESULTS * 4];
    int matchCount = 0;

    for (int i = 0; i < apps->count; i++) {
        int score = CalculateMatchScore(apps->items[i].name, apps->items[i].subtitle, qLower);
        if (score > 0) {
            if (matchCount < (int)(sizeof(matches) / sizeof(matches[0]))) {
                matches[matchCount].appIndex = i;
                matches[matchCount].score = score;
                matchCount++;
            }
        }
    }

    // Sort by score descending (simple insertion sort for small arrays)
    for (int i = 1; i < matchCount; i++) {
        ScoredMatch key = matches[i];
        int j = i - 1;
        while (j >= 0 && matches[j].score < key.score) {
            matches[j + 1] = matches[j];
            j--;
        }
        matches[j + 1] = key;
    }

    // Populate top results
    int limit = matchCount < MAX_SEARCH_RESULTS ? matchCount : MAX_SEARCH_RESULTS;
    for (int i = 0; i < limit; i++) {
        AddResult(engine, &apps->items[matches[i].appIndex], matches[i].score);
    }
}

void Search_SelectNext(SearchEngine *engine) {
    if (!engine || engine->result_count <= 0) return;
    engine->selected_index = (engine->selected_index + 1) % engine->result_count;

    // Adjust scroll window
    if (engine->selected_index >= engine->scroll_offset + MAX_VISIBLE_ROWS) {
        engine->scroll_offset = engine->selected_index - MAX_VISIBLE_ROWS + 1;
    } else if (engine->selected_index < engine->scroll_offset) {
        engine->scroll_offset = engine->selected_index;
    }
}

void Search_SelectPrev(SearchEngine *engine) {
    if (!engine || engine->result_count <= 0) return;
    engine->selected_index--;
    if (engine->selected_index < 0) {
        engine->selected_index = engine->result_count - 1;
    }

    // Adjust scroll window
    if (engine->selected_index < engine->scroll_offset) {
        engine->scroll_offset = engine->selected_index;
    } else if (engine->selected_index >= engine->scroll_offset + MAX_VISIBLE_ROWS) {
        engine->scroll_offset = engine->selected_index - MAX_VISIBLE_ROWS + 1;
    }
}
