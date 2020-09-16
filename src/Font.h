#pragma once

#include <stddef.h>

#include "Backends/Rendering.h"

typedef struct Font Font;

//Font* LoadFontFromData(const unsigned char *data, size_t data_size, size_t cell_width, size_t cell_height);
//Font* LoadFont(const char *font_filename, size_t cell_width, size_t cell_height);
Font* LoadBitmapFont(const char *bitmap_path, const char *metadata_path);
void DrawText(Font *font, RenderBackend_Surface *surface, int x, int y, unsigned long colour, const char *string);
void UnloadFont(Font *font);
