#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"

namespace renderer {

bool init_video();
void cleanup_video();

SDL_Window* get_window();
SDL_Renderer* get_renderer();

void clear_screen();
void present_screen();
void toggle_fullscreen();

// Drawing primitives
void draw_texture(SDL_Texture* tex, float x, float y, float w = 0, float h = 0);
void draw_rect_filled(const SDL_FRect& rect, SDL_Color color);
void draw_rect_outline(const SDL_FRect& rect, SDL_Color color, float width = 1.0f);
void draw_line(float x1, float y1, float x2, float y2, SDL_Color color);
void draw_circle_outline(float cx, float cy, float radius, SDL_Color color, float thickness = 2.0f);
void draw_rounded_box(const SDL_FRect& rect, float radius, SDL_Color fill_color, SDL_Color border_color, float border_width = 2.0f);

// Text & Button helpers
void draw_text(TTF_Font* font, const char* str, SDL_Color color, float x, float y);
void draw_text_centered(TTF_Font* font, const char* str, SDL_Color color, float center_x, float top_y);
void draw_button_text(TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, const SDL_FRect& btn_rect);
void draw_button_text_rounded(TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, SDL_Color border_col, const SDL_FRect& btn_rect, float radius = 14.0f);

// Banners
void draw_victory_banner(TTF_Font* font, float top_y);
void draw_defeat_banner(TTF_Font* font, float top_y);

} // namespace renderer

#endif // RENDERER_H
