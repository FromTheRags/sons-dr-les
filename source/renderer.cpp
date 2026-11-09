#include "renderer.h"
#include "assets.h"
#include <cstdio>
#include <algorithm>

namespace renderer {

static SDL_Surface* g_ecran = NULL;
static SDL_Surface* g_virtual_screen = NULL;
static bool g_fullscreen = false;

SDL_Surface* get_virtual_screen() {
    return g_virtual_screen;
}

// Pixel-format-aware scaling helper for fullscreen / resizing with aspect ratio preservation
static void blit_scaled(SDL_Surface *src, SDL_Surface *dst, const SDL_Rect &dst_rect) {
    if (!src || !dst) return;
    if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
    if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);

    SDL_PixelFormat *sfmt = src->format;
    SDL_PixelFormat *dfmt = dst->format;

    bool same_format = (sfmt->BitsPerPixel == 32 && dfmt->BitsPerPixel == 32 &&
                        sfmt->Rmask == dfmt->Rmask &&
                        sfmt->Gmask == dfmt->Gmask &&
                        sfmt->Bmask == dfmt->Bmask);

    if (same_format) {
        Uint32 *src_pixels = (Uint32*)src->pixels;
        Uint32 *dst_pixels = (Uint32*)dst->pixels;
        int src_pitch = src->pitch / 4;
        int dst_pitch = dst->pitch / 4;

        for (int y = 0; y < dst_rect.h; ++y) {
            int src_y = (y * src->h) / dst_rect.h;
            if (src_y >= src->h) src_y = src->h - 1;
            Uint32 *dst_row = dst_pixels + (dst_rect.y + y) * dst_pitch + dst_rect.x;
            Uint32 *src_row = src_pixels + src_y * src_pitch;

            for (int x = 0; x < dst_rect.w; ++x) {
                int src_x = (x * src->w) / dst_rect.w;
                if (src_x >= src->w) src_x = src->w - 1;
                dst_row[x] = src_row[src_x];
            }
        }
    } else {
        Uint32 *src_pixels = (Uint32*)src->pixels;
        int src_pitch = src->pitch / 4;

        for (int y = 0; y < dst_rect.h; ++y) {
            int src_y = (y * src->h) / dst_rect.h;
            if (src_y >= src->h) src_y = src->h - 1;
            Uint32 *src_row = src_pixels + src_y * src_pitch;

            for (int x = 0; x < dst_rect.w; ++x) {
                int src_x = (x * src->w) / dst_rect.w;
                if (src_x >= src->w) src_x = src->w - 1;
                Uint32 spix = src_row[src_x];

                Uint8 r = (Uint8)(((spix & sfmt->Rmask) >> sfmt->Rshift) << sfmt->Rloss);
                Uint8 g = (Uint8)(((spix & sfmt->Gmask) >> sfmt->Gshift) << sfmt->Gloss);
                Uint8 b = (Uint8)(((spix & sfmt->Bmask) >> sfmt->Bshift) << sfmt->Bloss);

                Uint32 dpix = ((r >> dfmt->Rloss) << dfmt->Rshift) |
                              ((g >> dfmt->Gloss) << dfmt->Gshift) |
                              ((b >> dfmt->Bloss) << dfmt->Bshift);

                if (dfmt->BitsPerPixel == 32) {
                    Uint32 *dst_pixels = (Uint32*)dst->pixels;
                    int dst_pitch = dst->pitch / 4;
                    dst_pixels[(dst_rect.y + y) * dst_pitch + (dst_rect.x + x)] = dpix;
                } else if (dfmt->BitsPerPixel == 16) {
                    Uint16 *dst_pixels = (Uint16*)dst->pixels;
                    int dst_pitch = dst->pitch / 2;
                    dst_pixels[(dst_rect.y + y) * dst_pitch + (dst_rect.x + x)] = (Uint16)dpix;
                }
            }
        }
    }

    if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
    if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
}

bool init_video() {
    SDL_WM_SetCaption("Sons Drôles (F11: Plein écran)", NULL);

    // Window icon
    SDL_Surface* icon = assets::get_image("sdl_icone.bmp");
    if (icon) {
        SDL_WM_SetIcon(icon, NULL);
    }

    // Physical display window
    g_ecran = SDL_SetVideoMode(VIRTUAL_W, VIRTUAL_H, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
    if (!g_ecran) {
        fprintf(stderr, "Impossible de configurer le mode vidéo SDL : %s\n", SDL_GetError());
        return false;
    }

    // Virtual 840x680 backbuffer
    g_virtual_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, VIRTUAL_W, VIRTUAL_H, 32,
        g_ecran->format->Rmask, g_ecran->format->Gmask, g_ecran->format->Bmask, g_ecran->format->Amask);

    if (!g_virtual_screen) {
        fprintf(stderr, "Impossible de créer la surface virtuelle de rendu\n");
        return false;
    }

    return true;
}

void cleanup_video() {
    if (g_virtual_screen) {
        SDL_FreeSurface(g_virtual_screen);
        g_virtual_screen = NULL;
    }
}

void present_screen() {
    if (!g_virtual_screen || !g_ecran) return;

    if (g_ecran->w == g_virtual_screen->w && g_ecran->h == g_virtual_screen->h &&
        g_ecran->format->BitsPerPixel == g_virtual_screen->format->BitsPerPixel) {
        SDL_BlitSurface(g_virtual_screen, NULL, g_ecran, NULL);
    } else {
        float scale = ((float)g_ecran->w / VIRTUAL_W < (float)g_ecran->h / VIRTUAL_H)
                    ? ((float)g_ecran->w / VIRTUAL_W)
                    : ((float)g_ecran->h / VIRTUAL_H);
        SDL_Rect dst;
        dst.w = (Uint16)(VIRTUAL_W * scale);
        dst.h = (Uint16)(VIRTUAL_H * scale);
        dst.x = (Sint16)((g_ecran->w - dst.w) / 2);
        dst.y = (Sint16)((g_ecran->h - dst.h) / 2);

        SDL_FillRect(g_ecran, NULL, 0x000000);
        blit_scaled(g_virtual_screen, g_ecran, dst);
    }
    SDL_Flip(g_ecran);
}

void toggle_fullscreen() {
    g_fullscreen = !g_fullscreen;
    Uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | (g_fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);
    int w = g_fullscreen ? 0 : VIRTUAL_W;
    int h = g_fullscreen ? 0 : VIRTUAL_H;
    g_ecran = SDL_SetVideoMode(w, h, 32, flags);
    if (!g_ecran) {
        g_fullscreen = false;
        g_ecran = SDL_SetVideoMode(VIRTUAL_W, VIRTUAL_H, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
    }
    present_screen();
}

void handle_resize(int w, int h) {
    if (w < 400) w = 400;
    if (h < 300) h = 300;
    g_ecran = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
    present_screen();
}

SDL_Rect transform_mouse_coords(int mx, int my) {
    SDL_Rect res = { -1, -1, 0, 0 };
    if (!g_ecran) return res;

    float scale = ((float)g_ecran->w / VIRTUAL_W < (float)g_ecran->h / VIRTUAL_H)
                ? ((float)g_ecran->w / VIRTUAL_W)
                : ((float)g_ecran->h / VIRTUAL_H);
    int dst_w = (int)(VIRTUAL_W * scale);
    int dst_h = (int)(VIRTUAL_H * scale);
    int dst_x = (g_ecran->w - dst_w) / 2;
    int dst_y = (g_ecran->h - dst_h) / 2;

    if (mx >= dst_x && mx < dst_x + dst_w && my >= dst_y && my < dst_y + dst_h) {
        res.x = (Sint16)((mx - dst_x) * (float)VIRTUAL_W / dst_w);
        res.y = (Sint16)((my - dst_y) * (float)VIRTUAL_H / dst_h);
    }
    return res;
}

void draw_text(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color color, int x, int y) {
    if (!dst || !font || !str || str[0] == '\0') return;
    SDL_Surface* txt = TTF_RenderUTF8_Blended(font, str, color);
    if (txt) {
        SDL_Rect r = { (Sint16)x, (Sint16)y, (Uint16)txt->w, (Uint16)txt->h };
        SDL_BlitSurface(txt, NULL, dst, &r);
        SDL_FreeSurface(txt);
    }
}

void draw_text_centered(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color color, int center_x, int top_y) {
    if (!dst || !font || !str || str[0] == '\0') return;
    SDL_Surface* txt = TTF_RenderUTF8_Blended(font, str, color);
    if (txt) {
        SDL_Rect r = { (Sint16)(center_x - txt->w / 2), (Sint16)top_y, (Uint16)txt->w, (Uint16)txt->h };
        SDL_BlitSurface(txt, NULL, dst, &r);
        SDL_FreeSurface(txt);
    }
}

void draw_button_text(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, SDL_Rect btn_rect) {
    if (!dst || !font || !str) return;
    SDL_Surface* txt = TTF_RenderUTF8_Shaded(font, str, fg, bg);
    if (txt) {
        SDL_Rect r = {
            (Sint16)(btn_rect.x + (btn_rect.w - txt->w) / 2),
            (Sint16)(btn_rect.y + (btn_rect.h - txt->h) / 2),
            (Uint16)txt->w,
            (Uint16)txt->h
        };
        SDL_BlitSurface(txt, NULL, dst, &r);
        SDL_FreeSurface(txt);
    }
}

void draw_rounded_box(SDL_Surface* dst, SDL_Rect rect, int radius, SDL_Color fill_color, SDL_Color border_color, int border_width) {
    if (!dst || rect.w <= 0 || rect.h <= 0) return;
    Uint32 fill_val = SDL_MapRGB(dst->format, fill_color.r, fill_color.g, fill_color.b);
    Uint32 border_val = SDL_MapRGB(dst->format, border_color.r, border_color.g, border_color.b);

    int r = radius;
    if (r * 2 > rect.w) r = rect.w / 2;
    if (r * 2 > rect.h) r = rect.h / 2;

    // Fill inner rects
    SDL_Rect r_mid = { (Sint16)(rect.x + r), rect.y, (Uint16)(rect.w - 2 * r), (Uint16)rect.h };
    SDL_FillRect(dst, &r_mid, fill_val);

    SDL_Rect r_left = { rect.x, (Sint16)(rect.y + r), (Uint16)r, (Uint16)(rect.h - 2 * r) };
    SDL_Rect r_right = { (Sint16)(rect.x + rect.w - r), (Sint16)(rect.y + r), (Uint16)r, (Uint16)(rect.h - 2 * r) };
    SDL_FillRect(dst, &r_left, fill_val);
    SDL_FillRect(dst, &r_right, fill_val);

    // Fill & stroke rounded corners
    if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);
    Uint32* pixels = (Uint32*)dst->pixels;
    int pitch = dst->pitch / 4;

    int corners[4][2] = {
        {rect.x + r, rect.y + r},
        {rect.x + rect.w - r - 1, rect.y + r},
        {rect.x + r, rect.y + rect.h - r - 1},
        {rect.x + rect.w - r - 1, rect.y + rect.h - r - 1}
    };
    int signs[4][2] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };

    int r_outer_sq = r * r;
    int r_inner_sq = (r > border_width && border_width > 0) ? (r - border_width) * (r - border_width) : 0;

    for (int c = 0; c < 4; ++c) {
        int cx = corners[c][0];
        int cy = corners[c][1];
        int sx = signs[c][0];
        int sy = signs[c][1];

        for (int dy = 0; dy <= r; ++dy) {
            for (int dx = 0; dx <= r; ++dx) {
                int dist_sq = dx * dx + dy * dy;
                if (dist_sq <= r_outer_sq) {
                    int px = cx + sx * dx;
                    int py = cy + sy * dy;
                    if (px >= 0 && px < dst->w && py >= 0 && py < dst->h) {
                        bool is_border = (border_width > 0 && dist_sq >= r_inner_sq);
                        pixels[py * pitch + px] = is_border ? border_val : fill_val;
                    }
                }
            }
        }
    }

    if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);

    // Straight border segments
    if (border_width > 0) {
        SDL_Rect top_b = { (Sint16)(rect.x + r), rect.y, (Uint16)(rect.w - 2 * r), (Uint16)border_width };
        SDL_Rect bot_b = { (Sint16)(rect.x + r), (Sint16)(rect.y + rect.h - border_width), (Uint16)(rect.w - 2 * r), (Uint16)border_width };
        SDL_Rect lft_b = { rect.x, (Sint16)(rect.y + r), (Uint16)border_width, (Uint16)(rect.h - 2 * r) };
        SDL_Rect rgt_b = { (Sint16)(rect.x + rect.w - border_width), (Sint16)(rect.y + r), (Uint16)border_width, (Uint16)(rect.h - 2 * r) };
        SDL_FillRect(dst, &top_b, border_val);
        SDL_FillRect(dst, &bot_b, border_val);
        SDL_FillRect(dst, &lft_b, border_val);
        SDL_FillRect(dst, &rgt_b, border_val);
    }
}

void draw_button_text_rounded(SDL_Surface* dst, TTF_Font* font, const char* str, SDL_Color fg, SDL_Color bg, SDL_Color border_col, SDL_Rect btn_rect, int radius) {
    draw_rounded_box(dst, btn_rect, radius, bg, border_col, 2);
    if (!font || !str || !dst) return;
    SDL_Surface* txt = TTF_RenderUTF8_Blended(font, str, fg);
    if (txt) {
        SDL_Rect r = {
            (Sint16)(btn_rect.x + (btn_rect.w - txt->w) / 2),
            (Sint16)(btn_rect.y + (btn_rect.h - txt->h) / 2),
            (Uint16)txt->w,
            (Uint16)txt->h
        };
        SDL_BlitSurface(txt, NULL, dst, &r);
        SDL_FreeSurface(txt);
    }
}

void draw_circle_outline(SDL_Surface* dst, int cx, int cy, int radius, SDL_Color color, int thickness) {
    if (!dst || radius <= 0) return;
    Uint32 col_val = SDL_MapRGB(dst->format, color.r, color.g, color.b);

    if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);
    Uint32* pixels = (Uint32*)dst->pixels;
    int pitch = dst->pitch / 4;

    int r_outer_sq = radius * radius;
    int r_inner_sq = (radius > thickness) ? (radius - thickness) * (radius - thickness) : 0;

    for (int dy = -radius; dy <= radius; ++dy) {
        int py = cy + dy;
        if (py < 0 || py >= dst->h) continue;
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = cx + dx;
            if (px < 0 || px >= dst->w) continue;
            int d_sq = dx * dx + dy * dy;
            if (d_sq <= r_outer_sq && d_sq >= r_inner_sq) {
                pixels[py * pitch + px] = col_val;
            }
        }
    }

    if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
}

void draw_victory_banner(SDL_Surface* dst, TTF_Font* font, int top_y) {
    const char* letters[] = {"V", "I", "C", "T", "O", "I", "R", "E"};
    SDL_Color letter_colors[] = {
        colors::Red, colors::Pink, colors::Orange, colors::Yellow,
        colors::AppleGreen, colors::Indigo, colors::Navy, colors::Purple
    };

    int num_letters = 8;
    int letter_widths[8];
    int total_width = 0;
    int spacing = 12;

    for (int k = 0; k < num_letters; ++k) {
        int w = 0, h = 0;
        TTF_SizeUTF8(font, letters[k], &w, &h);
        letter_widths[k] = w;
        total_width += w;
        if (k > 0) total_width += spacing;
    }

    int start_x = (VIRTUAL_W - total_width) / 2;
    int curr_x = start_x;

    for (int k = 0; k < num_letters; ++k) {
        SDL_Surface* txt = TTF_RenderUTF8_Blended(font, letters[k], letter_colors[k]);
        if (txt) {
            SDL_Rect r = { (Sint16)curr_x, (Sint16)top_y, (Uint16)txt->w, (Uint16)txt->h };
            SDL_BlitSurface(txt, NULL, dst, &r);
            SDL_FreeSurface(txt);
        }
        curr_x += letter_widths[k] + spacing;
    }
}

void draw_defeat_banner(SDL_Surface* dst, TTF_Font* font, int top_y) {
    const char* letters[] = {"D", "E", "F", "A", "I", "T", "E"};
    SDL_Color colors_list[] = {
        colors::Red, colors::Beige, colors::Red, colors::Beige,
        colors::Red, colors::Beige, colors::Red
    };

    int num_letters = 7;
    int letter_widths[7];
    int total_width = 0;
    int spacing = 12;

    for (int k = 0; k < num_letters; ++k) {
        int w = 0, h = 0;
        TTF_SizeUTF8(font, letters[k], &w, &h);
        letter_widths[k] = w;
        total_width += w;
        if (k > 0) total_width += spacing;
    }

    int start_x = (VIRTUAL_W - total_width) / 2;
    int curr_x = start_x;

    for (int k = 0; k < num_letters; ++k) {
        SDL_Surface* txt = TTF_RenderUTF8_Blended(font, letters[k], colors_list[k]);
        if (txt) {
            SDL_Rect r = { (Sint16)curr_x, (Sint16)top_y, (Uint16)txt->w, (Uint16)txt->h };
            SDL_BlitSurface(txt, NULL, dst, &r);
            SDL_FreeSurface(txt);
        }
        curr_x += letter_widths[k] + spacing;
    }
}

} // namespace renderer
