#ifndef CDG_RENDERER_H
#define CDG_RENDERER_H

#include "stdafx.h"
#include "cdg.h"

extern void cdg_render(const CDG &cdg, SDL_Renderer *out, bool maskBorder = true, int scaling = 1);
extern bool save_cdg_screen(const CDG &cdg, bool maskBorder, const char *file);

#endif