#include "renderer.h"
#include "assets.h"
#include <cstdio>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace renderer {

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static bool g_fullscreen = false;

bool init_video() {
    if (!SDL_CreateWindowAndRenderer("Sons Drôles", VIRTUAL_W, VIRTUAL_H, SDL_WINDOW_RESIZABLE, &g_window, &g_renderer)) {
        fprintf(stderr, "Erreur SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return false;
    }

    // Modern SDL 3 hardware letterboxing logical presentation!
    SDL_SetRenderLogicalPresentation(g_renderer, VIRTUAL_W, VIRTUAL_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    // Set Window Icon if present
    std::string icon_path = assets::get_asset_path("sdl_icone.bmp");
    if (!icon_path.empty()) {
        SDL_Surface* icon = SDL_LoadBMP(icon_path.c_str());
        if (icon) {
            SDL_SetWindowIcon(g_window, icon);
            SDL_DestroySurface(icon);
        }
    }

    return true;
}

void cleanup_video() {
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
}

SDL_Window* get_window() {
    return g_window;
}

SDL_Renderer* get_renderer() {
    return g_renderer;
}

void clear_screen() {
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
}

void present_screen() {
    if (!g_renderer) return;
    SDL_RenderPresent(g_renderer);
}

void toggle_fullscreen() {
    if (!g_window) return;
    g_fullscreen = !g_fullscreen;
    SDL_SetWindowFullscreen(g_window, g_fullscreen);
}

void draw_texture(SDL_Texture* tex, float x, float y, float w, float h) {
    if (!g_renderer || !tex) return;
    if (w <= 0.0f || h <= 0.0f) {
        float tex_w = 0, tex_h = 0;
        SDL_GetTextureSize(tex, &tex_w, &tex_h);
        w = tex_w;
        h = tex_h;
    }
    SDL_FRect dst = { x, y, w, h };
    SDL_RenderTexture(g_renderer, tex, NULL, &dst);
}

void draw_rect_filled(const SDL_FRect& rect, SDL_Color color) {
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(g_renderer, &rect);
}

void draw_rect_outline(const SDL_FRect& rect, SDL_Color color, float width) {
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    for (float i = 0; i < width; i += 1.0f) {
        SDL_FRect r = { rect.x + i, rect.y + i, rect.w - 2 * i, rect.h - 2 * i };
        SDL_RenderRect(g_renderer, &r);
    }
}

void draw_line(float x1, float y1, float x2, float y2, SDL_Color color) {
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(g_renderer, x1, y1, x2, y2);
}

void draw_circle_outline(float cx, float cy, float radius, SDL_Color color, float thickness) {
    if (!g_renderer || radius <= 0) return;
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);

    const int segments = 90;
    for (float t = 0; t < thickness; t += 1.0f) {
        float r = radius - t;
        float prev_x = cx + r;
        float prev_y = cy;
        for (int i = 1; i <= segments; ++i) {
            float angle = i * 2.0f * (float)M_PI / segments;
            float cur_x = cx + r * cosf(angle);
            float cur_y = cy + r * sinf(angle);
            SDL_RenderLine(g_renderer, prev_x, prev_y, cur_x, cur_y);
            prev_x = cur_x;
            prev_y = cur_y;
        }
    }
}

void draw_rounded_box(const SDL_FRect& rect, float radius, SDL_Color fill_color, SDL_Color border_color, float border_width) {
    if (!g_renderer || rect.w <= 0 || rect.h <= 0) return;

    float r = radius;
    if (r * 2.0f > rect.w) r = rect.w / 2.0f;
    if (r * 2.0f > rect.h) r = rect.h / 2.0f;

    // Fill interior
    SDL_FRect r_mid = { rect.x + r, rect.y, rect.w - 2.0f * r, rect.h };
    SDL_FRect r_left = { rect.x, rect.y + r, r, rect.h - 2.0f * r };
    SDL_FRect r_right = { rect.x + rect.w - r, rect.y + r, r, rect.h - 2.0f * r };

    draw_rect_filled(r_mid, fill_color);
    draw_rect_filled(r_left, fill_color);
    draw_rect_filled(r_right, fill_color);

    // Fill 4 corners with smooth fan of triangles/points
    SDL_SetRenderDrawColor(g_renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    float centers[4][2] = {
        {rect.x + r, rect.y + r},
        {rect.x + rect.w - r, rect.y + r},
        {rect.x + r, rect.y + rect.h - r},
        {rect.x + rect.w - r, rect.y + rect.h - r}
    };
    float start_angles[4] = { (float)M_PI, (float)(1.5 * M_PI), (float)(0.5 * M_PI), 0.0f };

    const int corner_segs = 16;
    for (int c = 0; c < 4; ++c) {
        float cx = centers[c][0];
        float cy = centers[c][1];
        float base_ang = start_angles[c];
        for (int i = 0; i < corner_segs; ++i) {
            float a1 = base_ang + (float)i * 0.5f * (float)M_PI / corner_segs;
            float a2 = base_ang + (float)(i + 1) * 0.5f * (float)M_PI / corner_segs;
            for (float cr = 0; cr <= r; cr += 1.0f) {
                SDL_RenderLine(g_renderer, cx + cr * cosf(a1), cy + cr * sinf(a1), cx + cr * cosf(a2), cy + cr * sinf(a2));
            }
        }
    }

    // Outer border
    if (border_width > 0.0f) {
        SDL_SetRenderDrawColor(g_renderer, border_color.r, border_color.g, border_color.b, border_color.a);
        // 4 straight border lines
        draw_rect_filled({ rect.x + r, rect.y, rect.w - 2.0f * r, border_width }, border_color);
        draw_rect_filled({ rect.x + r, rect.y + rect.h - border_width, rect.w - 2.0f * r, border_width }, border_color);
        draw_rect_filled({ rect.x, rect.y + r, border_width, rect.h - 2.0f * r }, border_color);
        draw_rect_filled({ rect.x + rect.w - border_width, rect.y + r, border_width, rect.h - 2.0f * r }, border_color);

        // 4 curved corner strokes
        for (int c = 0; c < 4; ++c) {
            float cx = centers[c][0];
            float cy = centers[c][1];
            float base_ang = start_angles[c];
            for (int i = 0; i < corner_segs; ++i) {
                float a1 = base_ang + (float)i * 0.5f * (float)M_PI / corner_segs;
                float a2 = base_ang + (float)(i + 1) * 0.5f * (float)M_PI / corner_segs;
                for (float bw = 0; bw < border_width; bw += 1.0f) {
                    float cur_r = r - bw;
                    SDL_RenderLine(g_renderer, cx + cur_r * cosf(a1), cy + cur_r * sinf(a1), cx + cur_r * cosf(a2), cy + cur_r * sinf(a2));
                }
            }
        }
    }
}

void draw_text(TTF_Font* font, const char* str, SDL_Color color, float x, float y) {
    if (!g_renderer || !font || !str || str[0] == '\0') return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, str, 0, color);
    if (surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
        if (tex) {
            SDL_FRect dst = { x, y, (float)surf->w, (float)surf->h };
            SDL_RenderTexture(g_renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
    }
}

void draw_text_centered(TTF_Font* font, const char* str, SDL_Color color, float center_x, float top_y) {
    if (!g_renderer || !font || !str || str[0] == '\0') return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, str, 0, color);
    if (surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
        if (tex) {
            SDL_FRect dst = { center_x - surf->w * 0.5f, top_y, (float)surf->w, (float)surf->h };
            SDL_RenderTexture(g_renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
    }
}

void draw_button_text(TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, const SDL_FRect& btn_rect) {
    draw_rect_filled(btn_rect, bg);
    if (!g_renderer || !font || !str) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, str, 0, fg);
    if (surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
        if (tex) {
            SDL_FRect dst = {
                btn_rect.x + (btn_rect.w - surf->w) * 0.5f,
                btn_rect.y + (btn_rect.h - surf->h) * 0.5f,
                (float)surf->w,
                (float)surf->h
            };
            SDL_RenderTexture(g_renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
    }
}

void draw_button_text_rounded(TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, SDL_Color border_col, const SDL_FRect& btn_rect, float radius) {
    draw_rounded_box(btn_rect, radius, bg, border_col, 2.0f);
    if (!g_renderer || !font || !str) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, str, 0, fg);
    if (surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
        if (tex) {
            SDL_FRect dst = {
                btn_rect.x + (btn_rect.w - surf->w) * 0.5f,
                btn_rect.y + (btn_rect.h - surf->h) * 0.5f,
                (float)surf->w,
                (float)surf->h
            };
            SDL_RenderTexture(g_renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
    }
}

void draw_victory_banner(TTF_Font* font, float top_y) {
    draw_text_centered(font, "VICTOIRE !", colors::Green, (float)VIRTUAL_W / 2.0f, top_y);
}

void draw_defeat_banner(TTF_Font* font, float top_y) {
    draw_text_centered(font, "DÉFAITE !", colors::Red, (float)VIRTUAL_W / 2.0f, top_y);
}

} // namespace renderer
