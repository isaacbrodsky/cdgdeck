// sdlutils.h
// Isaac Brodsky
// (not written on) 2012 AUGUST 6

#include "stdafx.h"

void apply_surface( int x, int y, SDL_Surface* source, SDL_Renderer* destination);
uint32_t get_pixel(SDL_Renderer *renderer, int x, int y);
void render_test_pattern(SDL_Renderer *renderer, int w, int h);
void render_text(SDL_Renderer *out, int x, int y, TTF_Font *font, SDL_Color col, const char *text);
