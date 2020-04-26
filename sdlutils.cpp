// sdlutils.cpp
// Isaac Brodsky
// (not written on) 2012 AUGUST 6

#include "stdafx.h"

void apply_surface(int x, int y, SDL_Surface* source, SDL_Renderer* renderer) {
	// Probably inefficient to create so many textures
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, source);
	
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = source->w;
	dest.h = source->h;
	SDL_RenderCopy(renderer, texture, NULL, &dest);
	
	SDL_DestroyTexture(texture);
}

uint32_t get_pixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(uint32_t *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

void render_test_pattern(SDL_Renderer *renderer, int w, int h) {
	SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = w;
	rect.h = h;
	SDL_RenderFillRect(renderer, &rect);
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
	for (int i = 0; i < 5; i++)
		SDL_RenderDrawLine(renderer, 2 + i, 2, w - 3, h - 3 - i);
}

void render_text(SDL_Renderer *out, int x, int y, TTF_Font *font, SDL_Color col, const char *text) {
	if (font == NULL)
		return;

	SDL_Surface *temp = TTF_RenderText_Blended(font, text, col);
	apply_surface(x, y, temp, out);
	SDL_FreeSurface(temp);
}