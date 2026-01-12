#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "font.h"

#define CHARACTER_COUNT 255

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define GRID_FACTOR 6

#define GRID_WIDTH SCREEN_WIDTH / GRID_FACTOR
#define GRID_HEIGHT SCREEN_HEIGHT / GRID_FACTOR

static bool grid[(int)GRID_WIDTH][(int)GRID_HEIGHT] = {0};
static bool next_grid[(int)GRID_WIDTH][(int)GRID_HEIGHT] = {0};

static character_t *font_characters[CHARACTER_COUNT] = {0};
static SDL_Surface *font_surface;

static bool running = true;
static bool tick = false;
static bool lmbd = false;
static bool rmbd = false;

bool is_alive(int x, int y);
bool write_to_grid(bool v, int x, int y);
bool is_inbounds(int x, int y);

void render_grid(SDL_Surface *surface);
void render_ui(SDL_Surface *surface);

void render_text(SDL_Surface *surface, const char *string, int x, int y,
                 int size);

void init_font();
void deinit_font();

void set_char(char c, int x, int y, int width, int height);

int main(int argc, char **argv) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    printf("Failed to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window;
  SDL_Renderer *renderer;

  if (!SDL_CreateWindowAndRenderer("Conways Game of Life", SCREEN_WIDTH,
                                   SCREEN_HEIGHT, 0, &window, &renderer)) {
    printf("Failed to create window: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Texture *render_target =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, GRID_WIDTH, GRID_HEIGHT);
  SDL_SetTextureScaleMode(render_target, SDL_SCALEMODE_NEAREST);

  SDL_Texture *ui_render_target =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, GRID_WIDTH, GRID_HEIGHT);
  SDL_SetTextureScaleMode(ui_render_target, SDL_SCALEMODE_NEAREST);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  SDL_Surface *surface_target =
      SDL_CreateSurface(GRID_WIDTH, GRID_HEIGHT, SDL_PIXELFORMAT_RGB24);

  init_font();

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;

      case SDL_EVENT_KEY_DOWN:
        if (e.key.scancode == SDL_SCANCODE_SPACE) {
          tick = !tick;
        } else if (e.key.scancode == SDL_SCANCODE_R) {
          memset(grid, 0, sizeof(grid));
          tick = false;
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e.button.button == 1)
          lmbd = true;
        else if (e.button.button == 3)
          rmbd = true;
        break;

      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e.button.button == 1)
          lmbd = false;
        else if (e.button.button == 3)
          rmbd = false;
        break;
      }
    }

    memcpy(next_grid, grid, sizeof(grid));

    float mx;
    float my;
    SDL_GetMouseState(&mx, &my);
    if (lmbd) {
      write_to_grid(true, mx / GRID_FACTOR, my / GRID_FACTOR);
      for (int y = -1; y < 1; ++y) {
        for (int x = -1; x < 1; ++x) {
          if (x > GRID_WIDTH || x < 0 || y > GRID_HEIGHT || y < 0)
            continue;
          write_to_grid(true, (mx / GRID_FACTOR) + x, (my / GRID_FACTOR + y));
        }
      }

    } else if (rmbd) {
      for (int y = -1; y < 1; ++y) {
        for (int x = -1; x < 1; ++x) {
          write_to_grid(false, (mx / GRID_FACTOR) + x, (my / GRID_FACTOR + y));
        }
      }
    }
    if (tick) {
      for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
          bool alive = grid[x][y];
          bool neighbors[] = {is_alive(x + 1, y),     is_alive(x - 1, y),
                              is_alive(x, y + 1),     is_alive(x, y - 1),
                              is_alive(x + 1, y + 1), is_alive(x - 1, y + 1),
                              is_alive(x + 1, y - 1), is_alive(x - 1, y - 1)};
          int alive_neighbors = 0;
          for (int i = 0; i < 8; i++) {
            if (neighbors[i])
              alive_neighbors++;
          }

          if (alive && alive_neighbors == 2 && alive_neighbors == 3) {
            write_to_grid(true, x, y);
            continue;
          }

          if (alive && alive_neighbors < 2) {
            write_to_grid(false, x, y);
            continue;
          }

          if (alive && alive_neighbors > 3) {
            write_to_grid(false, x, y);
            continue;
          }

          if (!alive && alive_neighbors == 3) {
            write_to_grid(true, x, y);
            continue;
          }
        }
      }
    }

    memcpy(grid, next_grid, sizeof(grid));

    SDL_RenderClear(renderer);

    SDL_LockTextureToSurface(render_target, NULL, &surface_target);
    render_grid(surface_target);
    SDL_UnlockTexture(render_target);
    SDL_RenderTexture(renderer, render_target, NULL, NULL);

    SDL_LockTextureToSurface(ui_render_target, NULL, &surface_target);
    render_ui(surface_target);
    SDL_UnlockTexture(ui_render_target);
    SDL_RenderTexture(renderer, ui_render_target, NULL, NULL);

    SDL_RenderPresent(renderer);

    SDL_Delay(1000 / 60);
  }

  deinit_font();

  SDL_DestroyTexture(render_target);
  SDL_DestroyTexture(ui_render_target);

  SDL_DestroySurface(surface_target);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return 0;
}

void write_surface_pixel(SDL_Surface *surface, int x, int y, unsigned char r,
                         unsigned char g, unsigned char b, unsigned char a) {
  uint32_t pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format),
                               SDL_GetSurfacePalette(surface), r, g, b, a);
  uint32_t *pixels = (uint32_t *)surface->pixels;
  pixels[(y * surface->w) + x] = pixel;
}

void render_ui(SDL_Surface *surface) {
  SDL_ClearSurface(surface, 0, 0, 0, 0);
  float mx, my;
  SDL_GetMouseState(&mx, &my);
  for (int y = -1; y < 1; ++y) {
    for (int x = -1; x < 1; ++x) {
      if (x > GRID_WIDTH || x < 0 || y > GRID_HEIGHT || y < 0)
        continue;
      write_surface_pixel(surface, (mx / GRID_FACTOR) + x,
                          (my / GRID_FACTOR + y), 255, 255, 255, 100);
    }
  }
  if (!tick)
    render_text(surface, "Paused", 3, 3, 1);
}

void render_grid(SDL_Surface *surface) {
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      unsigned char color = (grid[x][y]) ? 255 : 0;
      write_surface_pixel(surface, x, y, color, color, color, 255);
    }
  }
}

bool is_alive(int x, int y) {
  if (!is_inbounds(x, y))
    return false;

  return grid[x][y];
}

bool write_to_grid(bool v, int x, int y) {
  if (!is_inbounds(x, y))
    return false;

  next_grid[x][y] = v;

  return true;
}

bool is_inbounds(int x, int y) {
  return x >= 0 && x <= GRID_WIDTH && y >= 0 && y <= GRID_HEIGHT;
}

void render_text(SDL_Surface *surface, const char *string, int x, int y,
                 int size) {
  int length = strlen(string);

  int render_x = x;
  int render_y = y;

  for (int i = 0; i < length; ++i) {
    char char_name = toupper(string[i]);
    if (char_name == '\n') {
      render_y += 5 * size + size;
      render_x = x;
      continue;
    } else if (char_name == ' ') {
      render_x += 3 * size;
      continue;
    }

    int extra = 0;
    character_t *character = font_characters[char_name];
    if (character == NULL)
      continue;
    if (character->width == 1)
      extra = 1;

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

void init_font() {
  font_surface = SDL_LoadBMP_IO(
      SDL_IOFromConstMem(font_bitmap, sizeof(font_bitmap)), true);

  SDL_SetSurfaceColorKey(
      font_surface, true,
      SDL_MapRGB(SDL_GetPixelFormatDetails(font_surface->format),
                 SDL_GetSurfacePalette(font_surface), 255, 0, 220));

  set_char('A', 0, 0, 3, 5);
  set_char('B', 4, 0, 3, 5);
  set_char('C', 8, 0, 3, 5);
  set_char('D', 12, 0, 3, 5);
  set_char('E', 16, 0, 3, 5);
  set_char('F', 20, 0, 3, 5);
  set_char('G', 24, 0, 3, 5);
  set_char('H', 28, 0, 3, 5);
  set_char('I', 33, 0, 1, 5);
  set_char('J', 36, 0, 3, 5);
  set_char('K', 40, 0, 3, 5);
  set_char('L', 44, 0, 3, 5);
  set_char('M', 48, 0, 5, 5);

  set_char('N', 0, 6, 3, 5);
  set_char('O', 4, 6, 3, 5);
  set_char('P', 8, 6, 3, 5);
  set_char('Q', 12, 6, 3, 5);
  set_char('R', 16, 6, 3, 5);
  set_char('S', 20, 6, 3, 5);
  set_char('T', 24, 6, 3, 5);
  set_char('U', 28, 6, 3, 5);
  set_char('V', 32, 6, 3, 5);
  set_char('W', 36, 6, 5, 5);
  set_char('X', 42, 6, 3, 5);
  set_char('Y', 46, 6, 3, 5);
  set_char('Z', 50, 6, 3, 5);

  set_char('0', 0, 12, 3, 5);
  set_char('1', 4, 12, 3, 5);
  set_char('2', 8, 12, 3, 5);
  set_char('3', 12, 12, 3, 5);
  set_char('4', 16, 12, 3, 5);
  set_char('5', 20, 12, 3, 5);
  set_char('6', 24, 12, 3, 5);
  set_char('7', 28, 12, 3, 5);
  set_char('8', 32, 12, 3, 5);
  set_char('9', 36, 12, 3, 5);

  set_char('!', 41, 12, 1, 5);
  set_char('?', 44, 12, 3, 5);
  set_char('#', 48, 12, 5, 5);
  set_char(':', 55, 12, 1, 5);
}

void deinit_font() {
  for (int i = 0; i < 255; ++i) {
    character_t *character = font_characters[i];
    if (!character)
      continue;

    free(character);
  }
}

void set_char(char c, int x, int y, int width, int height) {
  int idx = c;
  character_t *character = (character_t *)malloc(sizeof(character_t));
  character->x = x;
  character->y = y;
  character->width = width;
  character->height = height;
  font_characters[idx] = character;
}
