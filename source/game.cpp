#include "game.h"
#include "assets.h"
#include "audio.h"
#include "renderer.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>

namespace game {

static GameState g_state = STATE_MENU;
static int g_current_level = 1;
static bool g_level_cleared[4] = { false, false, false, false };

// Sequence playback state
static bool g_playing_sequence = false;
static int g_sequence_index = 0;
static Uint32 g_sequence_timer_ms = 0;
const Uint32 SEQUENCE_INTERVAL_MS = 600;

// Game round data
static int g_case_to_sound[8];
static int g_target_sequence[8];
static int g_selected_cases[8];
static int g_selected_sounds[8];
static int g_click_count = 0;

static int g_replays_left = 2;
static int g_attempts_left = 5;
static int g_time_left_sec = 120;
static Uint32 g_timer_accum_ms = 0;

// Round over state
static bool g_round_victory = false;
static bool g_round_defeat = false;
static Uint32 g_feedback_timer_ms = 0;

GameState get_state() { return g_state; }
void set_state(GameState state) { g_state = state; }
int get_current_level() { return g_current_level; }
int get_replays_left() { return g_replays_left; }
int get_attempts_left() { return g_attempts_left; }

void load_progression() {
    std::string path = assets::get_asset_path("data/log.data");
    if (path.empty()) path = "data/log.data";

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        f = fopen("log.data", "rb");
    }
    if (f) {
        int val = 0;
        for (int i = 0; i < 4; ++i) {
            if (fscanf(f, "%d", &val) == 1) {
                g_level_cleared[i] = (val == 1);
            }
        }
        fclose(f);
    }
}

void save_progression() {
    std::string path = "data/log.data";
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        f = fopen("log.data", "wb");
    }
    if (f) {
        for (int i = 0; i < 4; ++i) {
            fprintf(f, "%d\n", g_level_cleared[i] ? 1 : 0);
        }
        fclose(f);
    }
}

bool is_extreme_unlocked() {
    return g_level_cleared[0] && g_level_cleared[1] && g_level_cleared[2] && g_level_cleared[3];
}

bool is_level_cleared(int level) {
    if (level < 1 || level > 4) return false;
    return g_level_cleared[level - 1];
}

void play_target_sequence() {
    g_playing_sequence = true;
    g_sequence_index = 0;
    g_sequence_timer_ms = 0;
}

void start_level(int level) {
    g_current_level = level;
    g_click_count = 0;
    g_replays_left = 2;
    g_attempts_left = 5;
    g_time_left_sec = 120;
    g_timer_accum_ms = 0;
    g_round_victory = false;
    g_round_defeat = false;
    g_feedback_timer_ms = 0;

    for (int i = 0; i < 8; ++i) {
        g_selected_cases[i] = -1;
        g_selected_sounds[i] = -1;
    }

    if (level >= 1 && level <= 4) {
        for (int i = 0; i < 8; ++i) {
            g_case_to_sound[i] = i + 1;
        }

        std::vector<int> pool;
        for (int s = 1; s <= 8; ++s) {
            pool.push_back(s);
        }
        for (int i = 0; i < 8; ++i) {
            int r_idx = rand() % pool.size();
            g_target_sequence[i] = pool[r_idx];
            pool.erase(pool.begin() + r_idx);
        }
    } else if (level == 5) {
        std::vector<int> theme_pool;
        for (int s = 1; s <= 8; ++s) {
            theme_pool.push_back(s);
        }
        for (int i = 0; i < 8; ++i) {
            int r_idx = rand() % theme_pool.size();
            g_case_to_sound[i] = theme_pool[r_idx];
            theme_pool.erase(theme_pool.begin() + r_idx);
        }

        std::vector<int> seq_pool;
        for (int s = 1; s <= 8; ++s) {
            seq_pool.push_back(s);
        }
        for (int i = 0; i < 8; ++i) {
            int r_idx = rand() % seq_pool.size();
            g_target_sequence[i] = seq_pool[r_idx];
            seq_pool.erase(seq_pool.begin() + r_idx);
        }
    }

    audio::stop_music();
    g_state = STATE_PLAYING;
    play_target_sequence();
}

void update(Uint32 delta_ms) {
    if (g_state == STATE_PLAYING) {
        if (g_feedback_timer_ms > 0) {
            if (delta_ms >= g_feedback_timer_ms) {
                g_feedback_timer_ms = 0;
            } else {
                g_feedback_timer_ms -= delta_ms;
            }
        }

        if (g_playing_sequence) {
            g_sequence_timer_ms += delta_ms;
            if (g_sequence_timer_ms >= SEQUENCE_INTERVAL_MS) {
                g_sequence_timer_ms -= SEQUENCE_INTERVAL_MS;
                if (g_sequence_index < 8) {
                    int sound_id = g_target_sequence[g_sequence_index];
                    int snd_lvl = (g_current_level == 5) ? 4 : g_current_level;
                    audio::play_game_sound(snd_lvl, sound_id);
                    g_sequence_index++;
                } else {
                    g_playing_sequence = false;
                }
            }
            return;
        }

        g_timer_accum_ms += delta_ms;
        while (g_timer_accum_ms >= 1000) {
            g_timer_accum_ms -= 1000;
            if (g_time_left_sec > 0) {
                g_time_left_sec--;
                if (g_time_left_sec == 0) {
                    g_round_defeat = true;
                    g_state = STATE_ROUND_OVER;
                }
            }
        }
    }
}

void render_menu() {
    SDL_Texture* bg = assets::get_texture("menu_background.bmp");
    if (bg) {
        renderer::draw_texture(bg, 0, 0, (float)VIRTUAL_W, (float)VIRTUAL_H);
    }

    Uint64 ticks = SDL_GetTicks();
    float beat = (sinf((float)ticks * 0.0115f) + 1.0f) * 0.5f;

    // Pulsing acoustic soundwaves
    float cx = (float)VIRTUAL_W / 2.0f;
    float cy = 355.0f;
    float r1 = 80.0f + 16.0f * beat;
    float r2 = 175.0f + 26.0f * beat;
    float r3 = 285.0f + 36.0f * beat;
    float r4 = 405.0f + 46.0f * beat;

    SDL_Color pulse_col1 = {255, 60, 120, 255};
    SDL_Color pulse_col2 = {0, 210, 255, 255};
    SDL_Color pulse_col3 = {255, 215, 0, 255};
    SDL_Color pulse_col4 = {180, 50, 220, 255};

    renderer::draw_circle_outline(cx, cy, r1, pulse_col1, 2.0f);
    renderer::draw_circle_outline(cx, cy, r2, pulse_col2, 2.0f);
    renderer::draw_circle_outline(cx, cy, r3, pulse_col3, 2.0f);
    renderer::draw_circle_outline(cx, cy, r4, pulse_col4, 1.0f);

    // Pulsing audio equalizer spectrum visualizer along bottom
    for (int b = 0; b < 24; ++b) {
        float phase = (float)b * 0.38f;
        float bar_h = 4.0f + 16.0f * fabsf(sinf((float)ticks * 0.010f + phase));
        SDL_FRect bar = { 120.0f + (float)b * 25.0f, 668.0f - bar_h, 14.0f, bar_h };
        SDL_Color bar_col = (b % 3 == 0) ? colors::AppleGreen : ((b % 3 == 1) ? colors::Yellow : colors::Pink);
        renderer::draw_rect_filled(bar, bar_col);
    }

    TTF_Font* f_badge = assets::get_font("Graduate-Regular.ttf", 22);

    const SDL_FRect badge_pos[4] = {
        {220.0f, 114.0f, 160.0f, 36.0f},
        {635.0f, 114.0f, 160.0f, 36.0f},
        {220.0f, 389.0f, 160.0f, 36.0f},
        {635.0f, 389.0f, 160.0f, 36.0f}
    };

    for (int lvl = 1; lvl <= 4; ++lvl) {
        if (is_level_cleared(lvl)) {
            renderer::draw_button_text_rounded(f_badge, " GAGNÉ ! ", colors::Black, colors::Gold, colors::White, badge_pos[lvl - 1], 16.0f);
        }
    }

    if (is_extreme_unlocked()) {
        SDL_Texture* btn_ext = assets::get_texture("mode_extreme.bmp");
        if (btn_ext) {
            renderer::draw_texture(btn_ext, 272.0f, 275.0f);
        }
    }

    SDL_Texture* btn_help = assets::get_texture("aide_icon.bmp");
    if (btn_help) {
        renderer::draw_texture(btn_help, 733.0f, 574.0f);
    }

    // Subtle ambient pulsating halo around the Help/Rules circular icon (center 776, 626)
    float h_cx = 776.0f;
    float h_cy = 626.0f;
    float h_r = 38.0f + 2.5f * beat;
    SDL_Color h_pulse_col = {220, 160, 255, (Uint8)(70 + 40 * beat)};
    renderer::draw_circle_outline(h_cx, h_cy, h_r, h_pulse_col, 1.5f);
}

void render_rules() {
    SDL_Texture* bg = assets::get_texture("rules_background.bmp");
    if (bg) {
        renderer::draw_texture(bg, 0, 0, (float)VIRTUAL_W, (float)VIRTUAL_H);
    }

    SDL_Texture* b_ret = assets::get_texture("btn_retour.bmp");
    if (b_ret) {
        renderer::draw_texture(b_ret, 10.0f, 10.0f);
    }
}

void render_game() {
    renderer::draw_rect_filled({0, 0, (float)VIRTUAL_W, (float)VIRTUAL_H}, {20, 24, 38, 255});

    SDL_Texture* img_inter = assets::get_texture("case_interrogation.bmp");
    TTF_Font* font_num = assets::get_font("Graduate-Regular.ttf", 64);

    for (int i = 0; i < 8; ++i) {
        float tx = (float)((i % 4) * 210);
        float ty = (float)((i / 4) * 340);

        int order_index = -1;
        for (int k = 0; k < g_click_count; ++k) {
            if (g_selected_cases[k] == i) {
                order_index = k + 1;
                break;
            }
        }

        if (order_index == -1) {
            if (img_inter) {
                renderer::draw_texture(img_inter, tx, ty, 210.0f, 340.0f);
            }
        } else {
            int sound_id = g_case_to_sound[i];
            char cname[64];
            snprintf(cname, sizeof(cname), "case_sound_%d.bmp", sound_id);
            SDL_Texture* c_img = assets::get_texture(cname);
            if (c_img) {
                renderer::draw_texture(c_img, tx, ty, 210.0f, 340.0f);
            }

            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%d", order_index);
            renderer::draw_text_centered(font_num, num_str, colors::Gold, tx + 105.0f, ty + 240.0f);
        }
    }

    // Grid dividers
    renderer::draw_line(210.0f, 0, 210.0f, 680.0f, {50, 65, 90, 255});
    renderer::draw_line(420.0f, 0, 420.0f, 680.0f, {50, 65, 90, 255});
    renderer::draw_line(630.0f, 0, 630.0f, 680.0f, {50, 65, 90, 255});
    renderer::draw_line(0, 340.0f, 840.0f, 340.0f, {50, 65, 90, 255});

    // Top buttons
    SDL_Texture* b_ret = assets::get_texture("btn_retour.bmp");
    if (b_ret) {
        renderer::draw_texture(b_ret, 0, 0);
    }

    TTF_Font* f_badge_mini = assets::get_font("Graduate-Regular.ttf", 20);
    if (g_replays_left > 0) {
        SDL_Texture* b_rep = assets::get_texture("btn_replay.bmp");
        if (b_rep) {
            renderer::draw_texture(b_rep, 765.0f, 0);
        }
        char rep_str[16];
        snprintf(rep_str, sizeof(rep_str), "x%d", g_replays_left);
        renderer::draw_button_text(f_badge_mini, rep_str, colors::Black, colors::Green, {720.0f, 12.0f, 42.0f, 32.0f});
    } else {
        SDL_Texture* b_crx = assets::get_texture("croix.bmp");
        if (b_crx) {
            renderer::draw_texture(b_crx, 765.0f, 0);
        }
        renderer::draw_button_text(f_badge_mini, "x0", colors::White, colors::Red, {720.0f, 12.0f, 42.0f, 32.0f});
    }

    // Bottom info bar overlay
    TTF_Font* f_info = assets::get_font("Graduate-Regular.ttf", 24);

    char time_str[32];
    int m = g_time_left_sec / 60;
    int s = g_time_left_sec % 60;
    snprintf(time_str, sizeof(time_str), "TEMPS: %d:%02d", m, s);
    renderer::draw_rect_filled({5.0f, 638.0f, 205.0f, 38.0f}, {15, 20, 30, 255});
    renderer::draw_text(f_info, time_str, colors::Orange, 12.0f, 642.0f);

    char comb_str[32];
    snprintf(comb_str, sizeof(comb_str), "ESSAIS: %d/5", g_attempts_left);
    renderer::draw_rect_filled({620.0f, 638.0f, 215.0f, 38.0f}, {15, 20, 30, 255});
    SDL_Color comb_color = (g_attempts_left <= 2) ? colors::Red : colors::AppleGreen;
    renderer::draw_text(f_info, comb_str, comb_color, 632.0f, 642.0f);

    if (g_playing_sequence) {
        renderer::draw_rect_filled({160.0f, 280.0f, 520.0f, 80.0f}, {20, 30, 55, 240});
        renderer::draw_rect_filled({160.0f, 280.0f, 520.0f, 3.0f}, {255, 215, 0, 255});
        TTF_Font* f_seq = assets::get_font("Graduate-Regular.ttf", 26);
        renderer::draw_text_centered(f_seq, "ÉCOUTE DE LA SÉQUENCE CIBLE...", colors::Gold, 420.0f, 292.0f);
        char note_str[64];
        snprintf(note_str, sizeof(note_str), "Réécoutes restantes : %d/2", g_replays_left);
        TTF_Font* f_sub = assets::get_font("PatrickHand-Regular.ttf", 22);
        renderer::draw_text_centered(f_sub, note_str, colors::White, 420.0f, 326.0f);
    } else if (g_feedback_timer_ms > 0) {
        renderer::draw_rect_filled({140.0f, 280.0f, 560.0f, 80.0f}, {60, 15, 25, 240});
        renderer::draw_rect_filled({140.0f, 280.0f, 560.0f, 3.0f}, {255, 60, 80, 255});
        TTF_Font* f_err = assets::get_font("Graduate-Regular.ttf", 26);
        renderer::draw_text_centered(f_err, "COMBINAISON INCORRECTE !", colors::Yellow, 420.0f, 292.0f);
        char tries_str[64];
        snprintf(tries_str, sizeof(tries_str), "Grille réinitialisée. Il vous reste %d/5 essais !", g_attempts_left);
        TTF_Font* f_sub = assets::get_font("PatrickHand-Regular.ttf", 22);
        renderer::draw_text_centered(f_sub, tries_str, colors::White, 420.0f, 326.0f);
    }
}

void render_round_over() {
    render_game();

    // Dark modal overlay
    renderer::draw_rect_filled({0, 0, (float)VIRTUAL_W, (float)VIRTUAL_H}, {0, 0, 0, 190});
    renderer::draw_rounded_box({100.0f, 70.0f, 640.0f, 460.0f}, 24.0f, {24, 28, 48, 255}, {255, 215, 0, 255}, 4.0f);

    TTF_Font* f_banner = assets::get_font("Graduate-Regular.ttf", 72);
    if (g_round_victory) {
        renderer::draw_victory_banner(f_banner, 110.0f);
    } else {
        renderer::draw_defeat_banner(f_banner, 110.0f);
    }

    TTF_Font* f_prompt = assets::get_font("PatrickHand-Regular.ttf", 36);
    renderer::draw_text_centered(f_prompt, "Veux-tu rejouer cette manche ?", colors::White, 420.0f, 240.0f);

    TTF_Font* f_btn = assets::get_font("Graduate-Regular.ttf", 34);
    renderer::draw_button_text_rounded(f_btn, "OUI", colors::Black, colors::Green, colors::White, {200.0f, 340.0f, 160.0f, 70.0f}, 18.0f);
    renderer::draw_button_text_rounded(f_btn, "NON", colors::White, colors::Red, colors::White, {480.0f, 340.0f, 160.0f, 70.0f}, 18.0f);
}

void handle_menu_click(float mx, float my) {
    if (mx < 0 || my < 0) return;

    // Check Help/Rules circular icon at bottom right
    float h_dx = mx - 776.0f;
    float h_dy = my - 626.0f;
    if ((h_dx * h_dx + h_dy * h_dy <= 45.0f * 45.0f) || (mx >= 720.0f && mx <= 840.0f && my >= 574.0f && my <= 680.0f)) {
        g_state = STATE_RULES;
        return;
    }

    if (is_extreme_unlocked()) {
        if (mx >= 272.0f && mx <= 567.0f && my >= 275.0f && my <= 414.0f) {
            start_level(5);
            return;
        }
    }

    if (mx >= 0 && mx <= 405.0f && my >= 95.0f && my <= 330.0f) {
        start_level(1);
        return;
    }
    if (mx >= 435.0f && mx <= 840.0f && my >= 95.0f && my <= 330.0f) {
        start_level(2);
        return;
    }
    if (mx >= 0 && mx <= 405.0f && my >= 365.0f && my <= 590.0f) {
        start_level(3);
        return;
    }
    if (mx >= 435.0f && mx <= 840.0f && my >= 365.0f && my <= 590.0f) {
        start_level(4);
        return;
    }
}

void handle_rules_click(float mx, float my) {
    if (mx < 0 || my < 0) return;
    if (mx >= 0 && mx <= 70.0f && my >= 0 && my <= 70.0f) {
        g_state = STATE_MENU;
    }
}

void handle_game_click(float mx, float my) {
    if (mx < 0 || my < 0) return;
    if (g_playing_sequence) return;

    if (mx >= 0 && mx <= 60.0f && my >= 0 && my <= 60.0f) {
        audio::stop_all_sfx();
        audio::start_music();
        g_state = STATE_MENU;
        return;
    }

    if (mx >= 720.0f && mx <= 840.0f && my >= 0 && my <= 60.0f) {
        if (g_replays_left > 0) {
            g_replays_left--;
            play_target_sequence();
        } else {
            audio::play_buzzer();
        }
        return;
    }

    int col = (int)(mx / 210.0f);
    int row = (int)(my / 340.0f);
    if (col >= 0 && col < 4 && row >= 0 && row < 2) {
        int tile_index = row * 4 + col;

        int found_order = -1;
        for (int i = 0; i < g_click_count; ++i) {
            if (g_selected_cases[i] == tile_index) {
                found_order = i;
                break;
            }
        }

        if (found_order == -1) {
            if (g_click_count < 8) {
                g_selected_cases[g_click_count] = tile_index;
                int sound_id = g_case_to_sound[tile_index];
                g_selected_sounds[g_click_count] = sound_id;
                g_click_count++;

                int snd_lvl = (g_current_level == 5) ? 4 : g_current_level;
                audio::play_game_sound(snd_lvl, sound_id);

                if (g_click_count == 8) {
                    bool match = true;
                    for (int k = 0; k < 8; ++k) {
                        if (g_selected_sounds[k] != g_target_sequence[k]) {
                            match = false;
                            break;
                        }
                    }

                    if (match) {
                        if (g_current_level >= 1 && g_current_level <= 4) {
                            g_level_cleared[g_current_level - 1] = true;
                            save_progression();
                        }
                        g_round_victory = true;
                        g_round_defeat = false;
                        g_state = STATE_ROUND_OVER;
                    } else {
                        g_attempts_left--;
                        audio::play_buzzer();

                        if (g_attempts_left <= 0) {
                            g_round_defeat = true;
                            g_round_victory = false;
                            g_state = STATE_ROUND_OVER;
                        } else {
                            g_feedback_timer_ms = 2000;
                            g_click_count = 0;
                            for (int k = 0; k < 8; ++k) {
                                g_selected_cases[k] = -1;
                                g_selected_sounds[k] = -1;
                            }
                        }
                    }
                }
            }
        } else if (found_order == g_click_count - 1) {
            audio::play_undo();
            g_selected_cases[g_click_count - 1] = -1;
            g_selected_sounds[g_click_count - 1] = -1;
            g_click_count--;
        }
    }
}

void handle_round_over_click(float mx, float my) {
    if (mx < 0 || my < 0) return;

    if (mx >= 200.0f && mx <= 360.0f && my >= 340.0f && my <= 410.0f) {
        start_level(g_current_level);
        return;
    }

    if (mx >= 480.0f && mx <= 640.0f && my >= 340.0f && my <= 410.0f) {
        audio::stop_all_sfx();
        audio::start_music();
        g_state = STATE_MENU;
        return;
    }
}

} // namespace game
