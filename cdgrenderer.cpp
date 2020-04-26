#include "stdafx.h"
#include "cdg.h"
#include "cdgrenderer.h"
#include "png.h"

void cdg_render(const CDG &cdg, SDL_Renderer *out, bool maskBorder, int scaling)
{
	uint8_t pv, ph;
	uint8_t r, g, b, a;
	uint8_t border;

	cdg.getPointers(pv, ph);
	border = cdg.getBorderColor();

	if (scaling == 1)
	{
		//optimize for the case of scaling = 1
		for (int x = 0; x + ph < CDG_WIDTH; x++)
		{
			for (int y = 0; y + pv < CDG_HEIGHT; y++)
			{
				cdg.getColor(cdg.getPixel(x + ph, y + pv), r, g, b, a);
				SDL_SetRenderDrawColor(out, r, g, b, ~a);
				SDL_RenderDrawPoint(out, x, y);
			}
		}
	}
	else
	{
		for (int x = 0; x + ph < CDG_WIDTH; x++)
		{
			for (int y = 0; y + pv < CDG_HEIGHT; y++)
			{
				int lx = x * scaling;
				int ly = y * scaling;
				cdg.getColor(cdg.getPixel(x + ph, y + pv), r, g, b, a);
				SDL_SetRenderDrawColor(out, r, g, b, ~a);
				for (int xs = 0; xs < scaling; xs++)
				{
					for (int ys = 0; ys < scaling; ys++)
					{
						SDL_RenderDrawPoint(out, lx + xs, ly + ys);
					}
				}
			}
		}
	}

	if (maskBorder)
	{
		cdg.getColor(border, r, g, b, a);
		
		SDL_SetRenderDrawColor(out, r, g, b, ~a);
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = CDG_WIDTH * scaling;
		rect.h = ROW_MULT * scaling;
		SDL_RenderFillRect(out, &rect);
		rect.w = COL_MULT * scaling;
		rect.h = CDG_HEIGHT * scaling;
		SDL_RenderFillRect(out, &rect);
		rect.x = (CDG_WIDTH - COL_MULT) * scaling;
		SDL_RenderFillRect(out, &rect);
		rect.x = 0;
		rect.y = (CDG_HEIGHT - ROW_MULT) * scaling;
		rect.w = CDG_WIDTH * scaling;
		rect.h = ROW_MULT * scaling;
		SDL_RenderFillRect(out, &rect);
	}
}

//http://www.lemoda.net/c/write-png/
//what a mess
bool save_cdg_screen(const CDG &cdg, bool maskBorder, const char *file)
{
	uint8_t pv, ph;
	uint8_t r, g, b, a;
	uint8_t border;

	cdg.getPointers(pv, ph);
	border = cdg.getBorderColor();

	bool status = false;
	FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    int pixel_size = 4; //rgba
    
#ifdef WIN32
	bool fileIsOpen = !fopen_s(&fp, file, "wb");
#else
	fp = fopen(file, "wb");
	bool fileIsOpen = !fp;
#endif
	
    if (file && fileIsOpen)
	{
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr)
		{
			info_ptr = png_create_info_struct(png_ptr);
			if (info_ptr)
			{
				// Set up error handling.
				// what is this even?????????
				if (!setjmp(png_jmpbuf (png_ptr)))
				{
					//Set image attributes.
					png_set_IHDR(png_ptr,
								info_ptr,
								CDG_WIDTH,
								CDG_HEIGHT,
								8, //bpp depth
								PNG_COLOR_TYPE_RGBA,
								PNG_INTERLACE_NONE,
								PNG_COMPRESSION_TYPE_DEFAULT,
								PNG_FILTER_TYPE_DEFAULT);
		
					// Initialize rows of PNG.
	
					//int i = 255;
					row_pointers = (png_byte**) png_malloc(png_ptr, CDG_HEIGHT * sizeof(png_byte*));
					for (y = 0; y < CDG_HEIGHT; ++y)
					{
						png_byte *row = (png_byte*) png_malloc(png_ptr, CDG_WIDTH * pixel_size);
						row_pointers[y] = row;
						for (x = 0; x < CDG_WIDTH; ++x)
						{
							if (maskBorder && (x < COL_MULT
								|| x > CDG_WIDTH - COL_MULT
								|| y < ROW_MULT
								|| y > CDG_HEIGHT - ROW_MULT))
								cdg.getColor(border, r, g, b, a);
							else if (x + ph < CDG_WIDTH
								&& y + pv < CDG_HEIGHT)
								cdg.getColor(cdg.getPixel(x + ph, y + pv), r, g, b, a);
							else
								r = g = b = a = 0;
							*row++ = r;
							*row++ = g;
							*row++ = b;
							*row++ = (~a);
				/*
				//cool pattern this
				*row++ = x % 255; //r
				*row++ = y % 255; //g
				*row++ = (x * y) %255; //b
				*row++ = i--; //a
				if (i == 0)
					i = 255;*/
						}
			/*for (x = 0; x < CDG_WIDTH; ++x) {
				
				pixel_t * pixel = pixel_at (bitmap, x, y);
				*row++ = pixel->red;
				*row++ = pixel->green;
				*row++ = pixel->blue;
			}*/
					}
		
					// Write the image data to "fp".
	
					png_init_io(png_ptr, fp);
					png_set_rows(png_ptr, info_ptr, row_pointers);
					png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
		
					for (y = 0; y < CDG_HEIGHT; y++)
					{
					//something crashes around here
						png_free(png_ptr, row_pointers[y]);
					}
					png_free(png_ptr, row_pointers);
		
					status = true;
				}

				// I'm not sure if the png_destroy cleanup codes lines up
				//with what I have above
				png_destroy_info_struct(png_ptr, &info_ptr);
			}
    
			png_destroy_write_struct(&png_ptr, &info_ptr);
		}
	
		fclose(fp);
	}
	
    return status;
}