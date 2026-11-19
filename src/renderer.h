#ifndef FLARE_RENDERER_H
#define FLARE_RENDERER_H

#include "window.h"
#include "animations.h"
#include "keyboard.h"
#include "search.h"

void Renderer_Init(void);
void Renderer_Cleanup(void);

// Main rendering routine for Flare search launcher
void Renderer_RenderFlare(const FlareWindow *win,
                          const AnimationController *anim,
                          const KeyboardState *kb,
                          SearchEngine *search);

#endif // FLARE_RENDERER_H
