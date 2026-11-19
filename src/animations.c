#include "animations.h"
#include <math.h>

#define PI 3.14159265358979323846f

static float EaseOutCubic(float t) {
    float f = 1.0f - t;
    return 1.0f - (f * f * f);
}

static float EaseInQuad(float t) {
    return t * t;
}

void Anim_Init(AnimationController *anim) {
    if (!anim) return;
    anim->state = ANIM_STATE_CLOSED;
    anim->progress = 0.0f;
    anim->duration_open = 0.16f;   // 160ms snappy macOS open
    anim->duration_close = 0.12f;  // 120ms quick dismiss
    anim->cursor_timer = 0.0f;
}

void Anim_Update(AnimationController *anim, float dt) {
    if (!anim) return;

    // Advance cursor timer
    anim->cursor_timer += dt;
    if (anim->cursor_timer >= 1.0f) {
        anim->cursor_timer -= 1.0f;
    }

    switch (anim->state) {
        case ANIM_STATE_OPENING:
            anim->progress += dt / anim->duration_open;
            if (anim->progress >= 1.0f) {
                anim->progress = 1.0f;
                anim->state = ANIM_STATE_OPEN;
            }
            break;

        case ANIM_STATE_CLOSING:
            anim->progress -= dt / anim->duration_close;
            if (anim->progress <= 0.0f) {
                anim->progress = 0.0f;
                anim->state = ANIM_STATE_CLOSED;
            }
            break;

        case ANIM_STATE_OPEN:
            anim->progress = 1.0f;
            break;

        case ANIM_STATE_CLOSED:
            anim->progress = 0.0f;
            break;
    }
}

void Anim_StartOpen(AnimationController *anim) {
    if (!anim) return;
    anim->state = ANIM_STATE_OPENING;
    anim->cursor_timer = 0.0f; // Reset cursor to full bright on open
}

void Anim_StartClose(AnimationController *anim) {
    if (!anim) return;
    if (anim->state == ANIM_STATE_CLOSED) return;
    anim->state = ANIM_STATE_CLOSING;
}

float Anim_GetAlpha(const AnimationController *anim) {
    if (!anim) return 0.0f;
    if (anim->state == ANIM_STATE_CLOSED) return 0.0f;
    if (anim->state == ANIM_STATE_OPEN) return 1.0f;

    if (anim->state == ANIM_STATE_OPENING) {
        return EaseOutCubic(anim->progress);
    } else {
        return EaseInQuad(anim->progress);
    }
}

float Anim_GetScale(const AnimationController *anim) {
    if (!anim) return 1.0f;
    float alpha = Anim_GetAlpha(anim);
    // Scale ranges smoothly from 0.97 to 1.00
    return 0.97f + (0.03f * alpha);
}

float Anim_GetCursorAlpha(const AnimationController *anim) {
    if (!anim) return 0.0f;
    // macOS cursor pulse: crisp on for ~0.55s, smooth fade
    float t = anim->cursor_timer;
    if (t < 0.55f) {
        return 1.0f;
    } else if (t < 0.65f) {
        // Smooth fade out
        return 1.0f - ((t - 0.55f) / 0.10f);
    } else if (t < 0.90f) {
        return 0.0f;
    } else {
        // Smooth fade in
        return (t - 0.90f) / 0.10f;
    }
}

bool Anim_IsVisible(const AnimationController *anim) {
    if (!anim) return false;
    return (anim->state != ANIM_STATE_CLOSED);
}
