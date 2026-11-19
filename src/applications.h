#ifndef FLARE_APPLICATIONS_H
#define FLARE_APPLICATIONS_H

#include <stdbool.h>

#define MAX_APP_NAME     128
#define MAX_APP_SUBTITLE 64
#define MAX_APP_CATEGORY 32
#define MAX_APP_PATH     512

typedef struct {
    char name[MAX_APP_NAME];
    char subtitle[MAX_APP_SUBTITLE];
    char category[MAX_APP_CATEGORY]; // "Application" or "Command"
    char path[MAX_APP_PATH];
    bool is_command;
} ApplicationItem;

typedef struct {
    ApplicationItem *items;
    int count;
    int capacity;
    bool is_indexed;
} ApplicationCache;

void Applications_Init(ApplicationCache *cache);
void Applications_Cleanup(ApplicationCache *cache);
void Applications_BuildIndex(ApplicationCache *cache);

// Execute an application or system command
bool Applications_Execute(const char *path, bool is_command, const char *name);

// Extract icon pixels (pure Win32 RGBA buffer, caller frees with free())
unsigned char* Applications_ExtractIconRGBA(const char *path, int *out_w, int *out_h);

#endif // FLARE_APPLICATIONS_H
