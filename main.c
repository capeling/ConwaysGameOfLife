#include <stdio.h>
#include <math.h>

#include <SDL3/SDL.h>

#include "font.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define GRID_FACTOR 4

#define GRID_WIDTH SCREEN_WIDTH / GRID_FACTOR
#define GRID_HEIGHT SCREEN_HEIGHT / GRID_FACTOR

#define FPS_LIMIT 60
#define PHYSICS_LIMIT 15

static bool grid[(int)GRID_WIDTH][(int)GRID_HEIGHT] = {0};
static bool next_grid[(int)GRID_WIDTH][(int)GRID_HEIGHT] = {0};

static bool running = true;
static bool tick = false;
static bool lmbd = false;
static bool rmbd = false;

static int brush_size = 2;

bool is_alive(int x, int y);
bool write_to_grid(bool v, int x, int y);
bool is_inbounds(int x, int y);

void render_grid(SDL_Surface *surface);
void render_ui(SDL_Surface *surface);

int main() {
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

  font_init();

  double now = SDL_GetTicks();
  double last = SDL_GetTicks();
  double wait = 1000.0 / PHYSICS_LIMIT;
  while (running) {
    last = now;
    now = SDL_GetTicks();
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

      case SDL_EVENT_MOUSE_WHEEL:
        if (e.wheel.y > 0)
          brush_size += 1;

        if (e.wheel.y < 0)
          brush_size -= 1;
        
        brush_size = fmin(5, fmax(1, brush_size));
        break;
      }
    }

    memcpy(next_grid, grid, sizeof(grid));

    float mx;
    float my;
    SDL_GetMouseState(&mx, &my);
    if (lmbd) {
      write_to_grid(true, mx / GRID_FACTOR, my / GRID_FACTOR);
      for (int y = -brush_size; y < brush_size; ++y) {
        for (int x = -brush_size; x < brush_size; ++x) {
          float mgx = mx / GRID_FACTOR + x;
          float mgy = my / GRID_FACTOR + y;
          if (!is_inbounds(mgx, mgy))
            continue;
          write_to_grid(true, mgx, mgy);
        }
      }

    } else if (rmbd) {
      for (int y = -brush_size; y < brush_size; ++y) {
        for (int x = -brush_size; x < brush_size; ++x) {
          float mgx = mx / GRID_FACTOR + x;
          float mgy = my / GRID_FACTOR + y;
          if (!is_inbounds(mgx, mgy))
            continue;
          write_to_grid(false, mgx, mgy);
        }
      }
    }
    wait += now - last;
    if (tick && wait > 1000.0 / PHYSICS_LIMIT) {
      for (int y = GRID_HEIGHT; y >= 0; --y) {
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
      wait = 0.0;
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

    SDL_Delay(1000 / FPS_LIMIT);
  }

  font_destroy();

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
  for (int y = -brush_size; y < brush_size; ++y) {
    for (int x = -brush_size; x < brush_size; ++x) {
      float mgx = mx / GRID_FACTOR + x;
      float mgy = my / GRID_FACTOR + y;
      if (!is_inbounds(mgx, mgy))
        continue;
      write_surface_pixel(surface, mgx,
                          mgy, 255, 255, 255, 100);
    }
  }
  if (!tick)
    font_render_text(surface, "\nPaused", 3, 3, 1);

  char* string;
  SDL_asprintf(&string, "Brush Size: %d", brush_size);
  font_render_text(surface, string, 3, 3, 1);
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
  if (!is_inbounds(x, y)) {
    return false;
  }

  return grid[x][y];
}

bool write_to_grid(bool v, int x, int y) {
  if (!is_inbounds(x, y))
    return false;

  next_grid[x][y] = v;

  return true;
}

bool is_inbounds(int x, int y) {
  return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}
