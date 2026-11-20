#include "search.h"
#include "calc.h"
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

static bool HasNonWhitespace(const char *str) {
    if (!str) return false;
    while (*str) {
        if (!isspace((unsigned char)*str)) return true;
        str++;
    }
    return false;
}

static void UrlEncode(const char *src, char *dst, size_t maxLen) {
    if (!src || !dst || maxLen == 0) return;
    size_t d = 0;
    while (*src && d + 4 < maxLen) {
        unsigned char c = (unsigned char)*src;
        if (c == ' ') {
            dst[d++] = '+';
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[d++] = (char)c;
        } else {
            snprintf(dst + d, maxLen - d, "%%%02X", c);
            d += 3;
        }
        src++;
    }
    dst[d] = '\0';
}

static bool IsLikelyUrl(const char *str) {
    if (!str) return false;
    if (strncmp(str, "http://", 7) == 0 || strncmp(str, "https://", 8) == 0) return true;
    if (strncmp(str, "www.", 4) == 0) return true;
    if (strchr(str, ' ') != NULL) return false;

    // Check for common TLD endings
    const char *dot = strrchr(str, '.');
    if (dot && *(dot + 1) != '\0') {
        const char *tld = dot + 1;
        static const char *knownTlds[] = {
            "com", "org", "net", "edu", "gov", "io", "dev", "app",
            "co", "me", "info", "biz", "tv", "cc", "xyz", "online",
            "site", "tech", "store", "us", "uk", "ca", "de", "fr", "jp", "au"
        };
        for (size_t i = 0; i < sizeof(knownTlds) / sizeof(knownTlds[0]); i++) {
            if (_stricmp(tld, knownTlds[i]) == 0) {
                return true;
            }
        }
    }
    return false;
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

void Search_PopulateSuggestions(SearchEngine *engine, const ApplicationCache *apps, const HistoryManager *hist) {
    if (!engine) return;
    engine->result_count = 0;
    engine->selected_index = 0;
    engine->scroll_offset = 0;
    engine->is_showing_suggestions = true;

    // 1. Populate from the last 5 used apps in history
    if (hist && hist->count > 0) {
        int count = hist->count > MAX_HISTORY_ITEMS ? MAX_HISTORY_ITEMS : hist->count;
        for (int i = 0; i < count; i++) {
            if (engine->result_count >= MAX_SEARCH_RESULTS) break;

            SearchResultItem *res = &engine->results[engine->result_count++];
            strncpy(res->title, hist->items[i].name, sizeof(res->title) - 1);
            res->title[sizeof(res->title) - 1] = '\0';

            strncpy(res->subtitle, hist->items[i].subtitle, sizeof(res->subtitle) - 1);
            res->subtitle[sizeof(res->subtitle) - 1] = '\0';

            strncpy(res->category_label, "Recent", sizeof(res->category_label) - 1);
            res->category_label[sizeof(res->category_label) - 1] = '\0';

            strncpy(res->path, hist->items[i].path, sizeof(res->path) - 1);
            res->path[sizeof(res->path) - 1] = '\0';

            res->is_command = hist->items[i].is_command;
            res->match_score = 100 - i;
        }
    }

    // 2. If history has fewer than 5 items, fill up to 5 with installed apps
    if (apps && engine->result_count < 5) {
        for (int i = 0; i < apps->count && engine->result_count < 5; i++) {
            bool alreadyIn = false;
            for (int r = 0; r < engine->result_count; r++) {
                if (_stricmp(engine->results[r].title, apps->items[i].name) == 0) {
                    alreadyIn = true;
                    break;
                }
            }
            if (!alreadyIn) {
                AddResult(engine, &apps->items[i], 50);
            }
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

void Search_Execute(SearchEngine *engine, const ApplicationCache *apps, const HistoryManager *hist, const char *query) {
    if (!engine) return;

    if (!query || strlen(query) == 0 || !HasNonWhitespace(query)) {
        engine->query[0] = '\0';
        Search_PopulateSuggestions(engine, apps, hist);
        return;
    }

    strncpy(engine->query, query, sizeof(engine->query) - 1);
    engine->query[sizeof(engine->query) - 1] = '\0';
    engine->result_count = 0;
    engine->selected_index = 0;
    engine->scroll_offset = 0;
    engine->is_showing_suggestions = false;

    // 1. Evaluate mathematical expressions (e.g. 2+2, 15*6, sqrt(144), 50*20%)
    double mathVal = 0.0;
    char mathStr[64] = { 0 };
    if (Calc_Evaluate(query, &mathVal, mathStr, sizeof(mathStr))) {
        SearchResultItem *mathItem = &engine->results[engine->result_count++];
        snprintf(mathItem->title, sizeof(mathItem->title), "= %s", mathStr);
        snprintf(mathItem->subtitle, sizeof(mathItem->subtitle), "%s", query);
        snprintf(mathItem->category_label, sizeof(mathItem->category_label), "Calculation");
        snprintf(mathItem->path, sizeof(mathItem->path), "calc_copy:%s", mathStr);
        mathItem->is_command = true;
        mathItem->match_score = 999;
    }

    if (apps && apps->count > 0) {
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

    // 3. Fallback: If no application or calculation matched, provide Web Search fallback option
    if (engine->result_count == 0) {
        SearchResultItem *webItem = &engine->results[engine->result_count++];
        if (IsLikelyUrl(query)) {
            if (strncmp(query, "http://", 7) == 0 || strncmp(query, "https://", 8) == 0) {
                snprintf(webItem->path, sizeof(webItem->path), "%s", query);
            } else {
                snprintf(webItem->path, sizeof(webItem->path), "https://%s", query);
            }
            snprintf(webItem->title, sizeof(webItem->title), "Open \"%s\"", query);
            snprintf(webItem->subtitle, sizeof(webItem->subtitle), "Navigate to website in default browser");
        } else {
            char encoded[512] = { 0 };
            UrlEncode(query, encoded, sizeof(encoded));
            snprintf(webItem->path, sizeof(webItem->path), "https://www.google.com/search?q=%s", encoded);
            snprintf(webItem->title, sizeof(webItem->title), "Search the Web for \"%s\"", query);
            snprintf(webItem->subtitle, sizeof(webItem->subtitle), "Open in default browser");
        }
        snprintf(webItem->category_label, sizeof(webItem->category_label), "Web Search");
        webItem->is_command = true;
        webItem->match_score = 1;
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
