#include "font.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

static SDL_Surface *font_surface = NULL;
static character_t *characters[UCHAR_MAX];

void load_character(char c, int x, int y, int width, int height) {
  int idx = tolower(c);
  character_t *character = (character_t *)malloc(sizeof(character_t));
  character->x = x;
  character->y = y;
  character->width = width;
  character->height = height;
  characters[idx] = character;
}

void font_init() {
  font_surface = SDL_LoadBMP_IO(
      SDL_IOFromConstMem(font_bitmap, sizeof(font_bitmap)), true);

  SDL_SetSurfaceColorKey(
      font_surface, true,
      SDL_MapRGB(SDL_GetPixelFormatDetails(font_surface->format),
                 SDL_GetSurfacePalette(font_surface), 255, 0, 220));

  load_character('A', 0, 0, 3, 5);
  load_character('B', 4, 0, 3, 5);
  load_character('C', 8, 0, 3, 5);
  load_character('D', 12, 0, 3, 5);
  load_character('E', 16, 0, 3, 5);
  load_character('F', 20, 0, 3, 5);
  load_character('G', 24, 0, 3, 5);
  load_character('H', 28, 0, 3, 5);
  load_character('I', 33, 0, 1, 5);
  load_character('J', 36, 0, 3, 5);
  load_character('K', 40, 0, 3, 5);
  load_character('L', 44, 0, 3, 5);
  load_character('M', 48, 0, 5, 5);

  load_character('N', 0, 6, 3, 5);
  load_character('O', 4, 6, 3, 5);
  load_character('P', 8, 6, 3, 5);
  load_character('Q', 12, 6, 3, 5);
  load_character('R', 16, 6, 3, 5);
  load_character('S', 20, 6, 3, 5);
  load_character('T', 24, 6, 3, 5);
  load_character('U', 28, 6, 3, 5);
  load_character('V', 32, 6, 3, 5);
  load_character('W', 36, 6, 5, 5);
  load_character('X', 42, 6, 3, 5);
  load_character('Y', 46, 6, 3, 5);
  load_character('Z', 50, 6, 3, 5);

  load_character('0', 0, 12, 3, 5);
  load_character('1', 4, 12, 3, 5);
  load_character('2', 8, 12, 3, 5);
  load_character('3', 12, 12, 3, 5);
  load_character('4', 16, 12, 3, 5);
  load_character('5', 20, 12, 3, 5);
  load_character('6', 24, 12, 3, 5);
  load_character('7', 28, 12, 3, 5);
  load_character('8', 32, 12, 3, 5);
  load_character('9', 36, 12, 3, 5);

  load_character('!', 41, 12, 1, 5);
  load_character('?', 44, 12, 3, 5);
  load_character('#', 48, 12, 5, 5);
  load_character(':', 55, 12, 1, 5);
}

void font_destroy() {
  if (font_surface != NULL) {
    SDL_DestroySurface(font_surface);
    font_surface = NULL;
  }
  for (int i = 0; i < UCHAR_MAX; ++i) {
    character_t *character = characters[i];
    if (!character)
      continue;

    free(character);
  }
}

void font_render_text(SDL_Surface *surface, const char *text, int x, int y, int size) {
  int length = strlen(text);

  int render_x = x;
  int render_y = y;

  for (int i = 0; i < length; ++i) {
    char char_name = tolower(text[i]);
    if (char_name == '\n') {
      render_y += 5 * size + size;
      render_x = x;
      continue;
    } else if (char_name == ' ') {
      render_x += 3 * size;
      continue;
    }

    character_t *character = characters[char_name];
    if (character == NULL)
      continue;

    int extra = (character->width == 1) ? 1 : 0;

    SDL_Rect src_rect = {character->x - extra, character->y,
                         character->width + extra, character->height};

    SDL_Rect dst_rect = {render_x, render_y, (character->width * size),
                         (character->height * size)};
    render_x += size;
    render_x += character->width * size;

    SDL_BlitSurfaceScaled(font_surface, &src_rect, surface, &dst_rect,
                          SDL_SCALEMODE_NEAREST);
  }
}
