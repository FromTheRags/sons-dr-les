#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"

namespace renderer {

bool init_video();
void cleanup_video();

SDL_Surface* get_virtual_screen();

void present_screen();
void toggle_fullscreen();
void handle_resize(int w, int h);
SDL_Rect transform_mouse_coords(int mx, int my);

// Text drawing helpers
void draw_text(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color color, int x, int y);
void draw_text_centered(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color color, int center_x, int top_y);
void draw_button_text(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, SDL_Rect btn_rect);

// Rounded button & box drawing
void draw_rounded_box(SDL_Surface* dst, SDL_Rect rect, int radius, SDL_Color fill_color, SDL_Color border_color, int border_width = 2);
void draw_button_text_rounded(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, SDL_Color border_col, SDL_Rect btn_rect, int radius = 14);
void draw_circle_outline(SDL_Surface* dst, int cx, int cy, int radius, SDL_Color color, int thickness = 2);

// Animated/styled banners
void draw_victory_banner(SDL_Surface* dst, TTF_Font* font, int top_y);
void draw_defeat_banner(SDL_Surface* dst, TTF_Font* font, int top_y);

} // namespace renderer

#endif // RENDERER_H
