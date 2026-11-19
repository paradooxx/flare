#ifndef FLARE_ANIMATIONS_H
#define FLARE_ANIMATIONS_H

#include <stdbool.h>

typedef enum {
    ANIM_STATE_CLOSED,
    ANIM_STATE_OPENING,
    ANIM_STATE_OPEN,
    ANIM_STATE_CLOSING
} AnimState;

typedef struct {
    AnimState state;
    float progress;       // Normalized [0.0, 1.0]
    float duration_open;  // Seconds
    float duration_close; // Seconds
    float cursor_timer;   // Accumulator for blinking cursor
} AnimationController;

void Anim_Init(AnimationController *anim);
void Anim_Update(AnimationController *anim, float dt);
void Anim_StartOpen(AnimationController *anim);
void Anim_StartClose(AnimationController *anim);

// Evaluated animation properties
float Anim_GetAlpha(const AnimationController *anim);
float Anim_GetScale(const AnimationController *anim);
float Anim_GetCursorAlpha(const AnimationController *anim);
bool  Anim_IsVisible(const AnimationController *anim);

#endif // FLARE_ANIMATIONS_H
