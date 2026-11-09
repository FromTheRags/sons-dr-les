#include "game.h"
#include "assets.h"
#include "audio.h"
#include "renderer.h"
#include <algorithm>
#include <cstdio>
#include <random>

namespace game {

static GameState g_state = STATE_MENU;
static int g_current_level = 1;

// Progression: cleared[0..3] corresponds to levels 1..4
static int g_levels_cleared[4] = {0, 0, 0, 0};

// Round state
static int g_target_sequence[8];     // Sounds 1..8 in target playback order
static int g_case_to_sound[8];       // Sound (1..8) associated with each tile (0..7)
static int g_selected_cases[8];      // Ordered tiles clicked by the player
static int g_selected_sounds[8];     // Corresponding sound for each click
static int g_click_count = 0;        // Number of tiles currently chosen (0..8)

static int g_replays_left = 2;       // 2 replay listens allowed per round
static int g_attempts_left = 5;      // 5 complete combination attempts allowed before defeat
static int g_time_left_sec = 120;    // 2 minutes countdown
static Uint32 g_last_tick = 0;
static Uint32 g_timer_accum_ms = 0;

static bool g_round_victory = false;
static bool g_round_defeat = false;

// Feedback banner timer when a combination fails
static Uint32 g_feedback_timer_ms = 0;

// Sequence playback state at beginning of level or upon replay
static bool g_playing_sequence = false;
static int g_seq_playback_step = 0;
static Uint32 g_seq_step_timer = 0;
static const Uint32 SEQ_INTERVAL_MS = 600;

void load_progression() {
    std::string path = assets::get_asset_path("data/log.data");
    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        path = assets::get_asset_path("log.data");
        f = fopen(path.c_str(), "r");
    }

    if (f) {
        if (fscanf(f, " %d %d %d %d", &g_levels_cleared[0], &g_levels_cleared[1],
                   &g_levels_cleared[2], &g_levels_cleared[3]) != 4) {
            g_levels_cleared[0] = g_levels_cleared[1] = g_levels_cleared[2] = g_levels_cleared[3] = 0;
        }
        fclose(f);
    }
}

void save_progression() {
    std::string path = "data/log.data";
    FILE* f = fopen(path.c_str(), "w");
    if (!f) {
        path = "log.data";
        f = fopen(path.c_str(), "w");
    }

    if (f) {
        fprintf(f, " %d %d %d %d\n", g_levels_cleared[0], g_levels_cleared[1],
                g_levels_cleared[2], g_levels_cleared[3]);
        fclose(f);
    }
}

bool is_extreme_unlocked() {
    return g_levels_cleared[0] == 1 && g_levels_cleared[1] == 1 &&
           g_levels_cleared[2] == 1 && g_levels_cleared[3] == 1;
}

bool is_level_cleared(int level) {
    if (level >= 1 && level <= 4) {
        return g_levels_cleared[level - 1] == 1;
    }
    return false;
}

GameState get_state() {
    return g_state;
}

void set_state(GameState state) {
    g_state = state;
}

int get_current_level() {
    return g_current_level;
}

int get_replays_left() {
    return g_replays_left;
}

int get_attempts_left() {
    return g_attempts_left;
}

void start_level(int level) {
    g_current_level = level;
    g_click_count = 0;
    g_replays_left = 2;       // 2 replays allowed
    g_attempts_left = 5;      // 5 complete combination attempts
    g_time_left_sec = 120;
    g_timer_accum_ms = 0;
    g_last_tick = SDL_GetTicks();
    g_round_victory = false;
    g_round_defeat = false;
    g_feedback_timer_ms = 0;

    for (int i = 0; i < 8; ++i) {
        g_selected_cases[i] = -1;
        g_selected_sounds[i] = 0;
    }

    // Stop background music as requested: concentration on game sounds!
    audio::stop_music();

    // 1. Configure target sequence
    if (level == 1 || level == 2) {
        // Standard ordered sequence 1..8
        for (int i = 0; i < 8; ++i) {
            g_target_sequence[i] = i + 1;
        }
    } else {
        // Levels 3, 4, 5: Random permutation of 1..8
        std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8};
        std::mt19937 rng((unsigned int)time(NULL) + level * 100);
        std::shuffle(nums.begin(), nums.end(), rng);
        for (int i = 0; i < 8; ++i) {
            g_target_sequence[i] = nums[i];
        }
    }

    // 2. Configure tile-to-sound mapping
    if (level == 1) {
        const int map1[8] = {3, 7, 5, 1, 8, 2, 4, 6};
        for (int i = 0; i < 8; ++i) g_case_to_sound[i] = map1[i];
    } else if (level == 2) {
        const int map2[8] = {4, 8, 3, 1, 5, 2, 6, 7};
        for (int i = 0; i < 8; ++i) g_case_to_sound[i] = map2[i];
    } else if (level == 3) {
        const int map3[8] = {7, 2, 1, 5, 4, 6, 3, 8};
        for (int i = 0; i < 8; ++i) g_case_to_sound[i] = map3[i];
    } else if (level == 4) {
        const int map4[8] = {5, 6, 8, 3, 4, 2, 7, 1};
        for (int i = 0; i < 8; ++i) g_case_to_sound[i] = map4[i];
    } else {
        // Extreme mode: completely randomized tile mapping
        std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8};
        std::mt19937 rng((unsigned int)time(NULL) + 9999);
        std::shuffle(nums.begin(), nums.end(), rng);
        for (int i = 0; i < 8; ++i) g_case_to_sound[i] = nums[i];
    }

    // Launch initial sequence preview
    play_target_sequence();
    g_state = STATE_PLAYING;
}

void play_target_sequence() {
    audio::stop_all_sfx();
    g_playing_sequence = true;
    g_seq_playback_step = 0;
    g_seq_step_timer = SDL_GetTicks();
}

void update(Uint32 delta_ms) {
    if (g_state == STATE_PLAYING) {
        // Update feedback banner timer
        if (g_feedback_timer_ms > 0) {
            if (delta_ms >= g_feedback_timer_ms) {
                g_feedback_timer_ms = 0;
            } else {
                g_feedback_timer_ms -= delta_ms;
            }
        }

        // 1. Manage target sequence playback
        if (g_playing_sequence) {
            Uint32 now = SDL_GetTicks();
            if (now - g_seq_step_timer >= SEQ_INTERVAL_MS) {
                if (g_seq_playback_step < 8) {
                    int sound_id = g_target_sequence[g_seq_playback_step];
                    audio::play_game_sound(g_current_level, sound_id);
                    g_seq_playback_step++;
                    g_seq_step_timer = now;
                } else {
                    g_playing_sequence = false;
                }
            }
            return;
        }

        // 2. Countdown timer (120s)
        g_timer_accum_ms += delta_ms;
        while (g_timer_accum_ms >= 1000) {
            g_timer_accum_ms -= 1000;
            if (g_time_left_sec > 0) {
                g_time_left_sec--;
                if (g_time_left_sec == 0) {
                    // Time out: defeat!
                    g_round_defeat = true;
                    g_state = STATE_ROUND_OVER;
                }
            }
        }
    }
}

void render_menu(SDL_Surface* dst) {
    SDL_Surface* bg = assets::get_image("menu_background.bmp");
    if (bg) {
        SDL_BlitSurface(bg, NULL, dst, NULL);
    }

    // Dynamic music pulsation effect (animated in real-time at 110 BPM)
    Uint32 ticks = SDL_GetTicks();
    float beat = (sinf(ticks * 0.0115f) + 1.0f) * 0.5f; // 0.0 to 1.0

    // Pulsing acoustic soundwaves expanding from center (420, 355)
    int cx = VIRTUAL_W / 2;
    int cy = 355;
    int r1 = 80 + (int)(16.0f * beat);
    int r2 = 175 + (int)(26.0f * beat);
    int r3 = 285 + (int)(36.0f * beat);
    int r4 = 405 + (int)(46.0f * beat);

    SDL_Color pulse_col1 = {255, 60, 120, 255};
    SDL_Color pulse_col2 = {0, 210, 255, 255};
    SDL_Color pulse_col3 = {255, 215, 0, 255};
    SDL_Color pulse_col4 = {180, 50, 220, 255};

    renderer::draw_circle_outline(dst, cx, cy, r1, pulse_col1, 2);
    renderer::draw_circle_outline(dst, cx, cy, r2, pulse_col2, 2);
    renderer::draw_circle_outline(dst, cx, cy, r3, pulse_col3, 2);
    renderer::draw_circle_outline(dst, cx, cy, r4, pulse_col4, 1);

    // Pulsing audio equalizer spectrum visualizer along bottom
    for (int b = 0; b < 24; ++b) {
        float phase = b * 0.38f;
        int bar_h = (int)(4.0f + 16.0f * fabsf(sinf(ticks * 0.010f + phase)));
        SDL_Rect bar = { (Sint16)(120 + b * 25), (Sint16)(668 - bar_h), 14, (Uint16)bar_h };
        SDL_Color bar_col = (b % 3 == 0) ? colors::AppleGreen : ((b % 3 == 1) ? colors::Yellow : colors::Pink);
        SDL_FillRect(dst, &bar, SDL_MapRGB(dst->format, bar_col.r, bar_col.g, bar_col.b));
    }

    TTF_Font* f_badge = assets::get_font("Graduate-Regular.ttf", 22);

    // Rounded Badges for completed levels
    const SDL_Rect badge_pos[4] = {
        {220, 114, 160, 36},
        {635, 114, 160, 36},
        {220, 389, 160, 36},
        {635, 389, 160, 36}
    };

    for (int lvl = 1; lvl <= 4; ++lvl) {
        if (is_level_cleared(lvl)) {
            renderer::draw_button_text_rounded(dst, f_badge, " GAGNÉ ! ", colors::Black, colors::Gold, colors::White, badge_pos[lvl - 1], 16);
        }
    }

    // Mode Extreme button if unlocked
    if (is_extreme_unlocked()) {
        SDL_Surface* btn_ext = assets::get_image("mode_extreme.bmp");
        if (btn_ext) {
            SDL_Rect r_ext = {272, 275, (Uint16)btn_ext->w, (Uint16)btn_ext->h};
            SDL_BlitSurface(btn_ext, NULL, dst, &r_ext);
        }
    }

    // Help/Rules icon button at bottom right (733, 574)
    SDL_Surface* btn_help = assets::get_image("aide_icon.bmp");
    if (btn_help) {
        SDL_Rect r_help = {733, 574, (Uint16)btn_help->w, (Uint16)btn_help->h};
        SDL_BlitSurface(btn_help, NULL, dst, &r_help);
    }
}

void render_rules(SDL_Surface* dst) {
    SDL_Surface* bg = assets::get_image("rules_background.bmp");
    if (bg) {
        SDL_BlitSurface(bg, NULL, dst, NULL);
    }

    // Return button at top left (10, 10)
    SDL_Surface* b_ret = assets::get_image("btn_retour.bmp");
    if (b_ret) {
        SDL_Rect r_ret = {10, 10, (Uint16)b_ret->w, (Uint16)b_ret->h};
        SDL_BlitSurface(b_ret, NULL, dst, &r_ret);
    }
}

void render_game(SDL_Surface* dst) {
    // Fill background with dark navy
    SDL_FillRect(dst, NULL, SDL_MapRGB(dst->format, 20, 24, 34));

    // Tile dimensions: 4 columns x 2 rows
    // Col widths: 210 each. Row heights: 340 each.
    TTF_Font* font_num = assets::get_font("Graduate-Regular.ttf", 68);

    for (int t = 0; t < 8; ++t) {
        int col = t % 4;
        int row = t / 4;
        int tx = col * 210;
        int ty = row * 340;

        // Check if tile t is selected by the player
        int order_index = -1;
        for (int k = 0; k < g_click_count; ++k) {
            if (g_selected_cases[k] == t) {
                order_index = k + 1;
                break;
            }
        }

        SDL_Rect dst_rect = { (Sint16)tx, (Sint16)ty, 209, 339 };

        if (order_index == -1) {
            // Unrevealed tile
            SDL_Surface* surf = assets::get_image("case_interrogation.bmp");
            if (surf) {
                SDL_BlitSurface(surf, NULL, dst, &dst_rect);
            }
        } else {
            // Revealed tile
            char name[64];
            snprintf(name, sizeof(name), "case_sound_%d.bmp", t + 1);
            SDL_Surface* surf = assets::get_image(name);
            if (surf) {
                SDL_BlitSurface(surf, NULL, dst, &dst_rect);
            }

            // Draw player's click order number (1..8)
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%d", order_index);
            renderer::draw_text_centered(dst, font_num, num_str, colors::Gold, tx + 105, ty + 240);
        }
    }

    // Grid divider lines
    SDL_Rect vline1 = {210, 0, 1, 680};
    SDL_Rect vline2 = {420, 0, 1, 680};
    SDL_Rect vline3 = {630, 0, 1, 680};
    SDL_Rect hline  = {0, 340, 840, 1};

    SDL_FillRect(dst, &vline1, SDL_MapRGB(dst->format, 50, 65, 90));
    SDL_FillRect(dst, &vline2, SDL_MapRGB(dst->format, 50, 65, 90));
    SDL_FillRect(dst, &vline3, SDL_MapRGB(dst->format, 50, 65, 90));
    SDL_FillRect(dst, &hline,  SDL_MapRGB(dst->format, 50, 65, 90));

    // Return button at top left (0, 0, 60, 60)
    SDL_Surface* b_ret = assets::get_image("btn_retour.bmp");
    if (b_ret) {
        SDL_Rect r_ret = {0, 0, (Uint16)b_ret->w, (Uint16)b_ret->h};
        SDL_BlitSurface(b_ret, NULL, dst, &r_ret);
    }

    // Replay button at top right (765, 0) with badge indicating remaining replays
    TTF_Font* f_badge_mini = assets::get_font("Graduate-Regular.ttf", 20);
    if (g_replays_left > 0) {
        SDL_Surface* b_rep = assets::get_image("btn_replay.bmp");
        if (b_rep) {
            SDL_Rect r_rep = {765, 0, (Uint16)b_rep->w, (Uint16)b_rep->h};
            SDL_BlitSurface(b_rep, NULL, dst, &r_rep);
        }
        // Badge text showing replays left (e.g., "x2" or "x1")
        char rep_str[16];
        snprintf(rep_str, sizeof(rep_str), "x%d", g_replays_left);
        SDL_Rect r_rep_badge = {720, 12, 42, 32};
        renderer::draw_button_text(dst, f_badge_mini, rep_str, colors::Black, colors::Green, r_rep_badge);
    } else {
        SDL_Surface* b_crx = assets::get_image("croix.bmp");
        if (b_crx) {
            SDL_Rect r_crx = {765, 0, (Uint16)b_crx->w, (Uint16)b_crx->h};
            SDL_BlitSurface(b_crx, NULL, dst, &r_crx);
        }
        SDL_Rect r_rep_badge = {720, 12, 42, 32};
        renderer::draw_button_text(dst, f_badge_mini, "x0", colors::White, colors::Red, r_rep_badge);
    }

    // Bottom info bar overlay
    TTF_Font* f_info = assets::get_font("Graduate-Regular.ttf", 24);

    // Timer display (bottom left)
    char time_str[32];
    int m = g_time_left_sec / 60;
    int s = g_time_left_sec % 60;
    snprintf(time_str, sizeof(time_str), "TEMPS: %d:%02d", m, s);
    SDL_Rect r_time_bg = {5, 638, 205, 38};
    SDL_FillRect(dst, &r_time_bg, SDL_MapRGB(dst->format, 15, 20, 30));
    renderer::draw_text(dst, f_info, time_str, colors::Orange, 12, 642);

    // Remaining combination attempts display (bottom right: 5 combinations allowed)
    char comb_str[32];
    snprintf(comb_str, sizeof(comb_str), "ESSAIS: %d/5", g_attempts_left);
    SDL_Rect r_comb_bg = {620, 638, 215, 38};
    SDL_FillRect(dst, &r_comb_bg, SDL_MapRGB(dst->format, 15, 20, 30));
    SDL_Color comb_color = (g_attempts_left <= 2) ? colors::Red : colors::AppleGreen;
    renderer::draw_text(dst, f_info, comb_str, comb_color, 632, 642);

    // Sequence playback notification banner
    if (g_playing_sequence) {
        SDL_Rect r_banner = {160, 280, 520, 80};
        SDL_FillRect(dst, &r_banner, SDL_MapRGB(dst->format, 20, 30, 55));
        SDL_Rect r_b_out = {160, 280, 520, 3};
        SDL_FillRect(dst, &r_b_out, SDL_MapRGB(dst->format, 255, 215, 0));
        TTF_Font* f_seq = assets::get_font("Graduate-Regular.ttf", 26);
        renderer::draw_text_centered(dst, f_seq, "ÉCOUTE DE LA SÉQUENCE CIBLE...", colors::Gold, 420, 292);
        char note_str[64];
        snprintf(note_str, sizeof(note_str), "Réécoutes restantes : %d/2", g_replays_left);
        TTF_Font* f_sub = assets::get_font("PatrickHand-Regular.ttf", 22);
        renderer::draw_text_centered(dst, f_sub, note_str, colors::White, 420, 326);
    }
    // Wrong combination feedback banner (visible for 2s after an incorrect 8-combination)
    else if (g_feedback_timer_ms > 0) {
        SDL_Rect r_err_banner = {140, 280, 560, 80};
        SDL_FillRect(dst, &r_err_banner, SDL_MapRGB(dst->format, 60, 15, 25));
        SDL_Rect r_err_border = {140, 280, 560, 3};
        SDL_FillRect(dst, &r_err_border, SDL_MapRGB(dst->format, 255, 60, 80));
        TTF_Font* f_err = assets::get_font("Graduate-Regular.ttf", 26);
        renderer::draw_text_centered(dst, f_err, "COMBINAISON INCORRECTE !", colors::Yellow, 420, 292);
        char tries_str[64];
        snprintf(tries_str, sizeof(tries_str), "Grille réinitialisée. Il vous reste %d/5 essais !", g_attempts_left);
        TTF_Font* f_sub = assets::get_font("PatrickHand-Regular.ttf", 22);
        renderer::draw_text_centered(dst, f_sub, tries_str, colors::White, 420, 326);
    }
}

void render_round_over(SDL_Surface* dst) {
    // 1. Render game board in background
    render_game(dst);

    // 2. Dark translucent overlay box in center
    SDL_Rect modal = {100, 70, 640, 460};
    SDL_FillRect(dst, &modal, SDL_MapRGB(dst->format, 25, 30, 45));

    // Outer border
    SDL_Rect border_top = {100, 70, 640, 4};
    SDL_Rect border_bot = {100, 526, 640, 4};
    SDL_Rect border_lft = {100, 70, 4, 460};
    SDL_Rect border_rgt = {736, 70, 4, 460};
    SDL_FillRect(dst, &border_top, SDL_MapRGB(dst->format, 255, 215, 0));
    SDL_FillRect(dst, &border_bot, SDL_MapRGB(dst->format, 255, 215, 0));
    SDL_FillRect(dst, &border_lft, SDL_MapRGB(dst->format, 255, 215, 0));
    SDL_FillRect(dst, &border_rgt, SDL_MapRGB(dst->format, 255, 215, 0));

    // 3. Victory or Defeat Banner
    TTF_Font* f_banner = assets::get_font("Graduate-Regular.ttf", 72);
    if (g_round_victory) {
        renderer::draw_victory_banner(dst, f_banner, 110);
    } else {
        renderer::draw_defeat_banner(dst, f_banner, 110);
    }

    // 4. Prompt question
    TTF_Font* f_prompt = assets::get_font("PatrickHand-Regular.ttf", 36);
    renderer::draw_text_centered(dst, f_prompt, "Veux-tu rejouer cette manche ?", colors::White, 420, 240);

    // 5. OUI and NON rounded buttons
    TTF_Font* f_btn = assets::get_font("Graduate-Regular.ttf", 34);
    SDL_Rect btn_oui = {200, 340, 160, 70};
    SDL_Rect btn_non = {480, 340, 160, 70};

    renderer::draw_button_text_rounded(dst, f_btn, "OUI", colors::Black, colors::Green, colors::White, btn_oui, 18);
    renderer::draw_button_text_rounded(dst, f_btn, "NON", colors::White, colors::Red, colors::White, btn_non, 18);
}

void handle_menu_click(int mx, int my) {
    if (mx < 0 || my < 0) return;

    // Check Help icon at (733, 574)
    if (mx >= 733 && mx <= 840 && my >= 574 && my <= 680) {
        g_state = STATE_RULES;
        return;
    }

    // Check Mode Extreme at (272, 275, w=295, h=139) if unlocked
    if (is_extreme_unlocked()) {
        if (mx >= 272 && mx <= 567 && my >= 275 && my <= 414) {
            start_level(5);
            return;
        }
    }

    // Level 1: Top-left
    if (mx >= 0 && mx <= 405 && my >= 95 && my <= 330) {
        start_level(1);
        return;
    }

    // Level 2: Top-right
    if (mx >= 435 && mx <= 840 && my >= 95 && my <= 330) {
        start_level(2);
        return;
    }

    // Level 3: Bottom-left
    if (mx >= 0 && mx <= 405 && my >= 370 && my <= 620) {
        start_level(3);
        return;
    }

    // Level 4: Bottom-right
    if (mx >= 435 && mx <= 840 && my >= 370 && my <= 620) {
        start_level(4);
        return;
    }
}

void handle_rules_click(int mx, int my) {
    if (mx < 0 || my < 0) return;

    // Return button at top left (0..70, 0..70)
    if (mx >= 0 && mx <= 70 && my >= 0 && my <= 70) {
        g_state = STATE_MENU;
    }
}

void handle_game_click(int mx, int my) {
    if (mx < 0 || my < 0) return;
    if (g_playing_sequence) return; // Ignore input while hearing sequence

    // 1. Return button at top left (0..60, 0..60) -> Back to main menu
    if (mx >= 0 && mx <= 60 && my >= 0 && my <= 60) {
        audio::stop_all_sfx();
        audio::start_music(); // Resume music when returning to menu
        g_state = STATE_MENU;
        return;
    }

    // 2. Replay button at top right (720..840, 0..60)
    if (mx >= 720 && mx <= 840 && my >= 0 && my <= 60) {
        if (g_replays_left > 0) {
            g_replays_left--;
            play_target_sequence();
        } else {
            audio::play_buzzer();
        }
        return;
    }

    // 3. Grid tiles (8 tiles: 4 cols x 2 rows)
    int col = mx / 210;
    int row = my / 340;
    if (col >= 0 && col < 4 && row >= 0 && row < 2) {
        int tile_index = row * 4 + col;

        // Check if tile is already selected
        int found_order = -1;
        for (int i = 0; i < g_click_count; ++i) {
            if (g_selected_cases[i] == tile_index) {
                found_order = i;
                break;
            }
        }

        if (found_order == -1) {
            // New tile selected!
            if (g_click_count < 8) {
                g_selected_cases[g_click_count] = tile_index;
                int sound_id = g_case_to_sound[tile_index];
                g_selected_sounds[g_click_count] = sound_id;
                g_click_count++;

                audio::play_game_sound(g_current_level, sound_id);

                // If all 8 tiles selected, check the combination!
                if (g_click_count == 8) {
                    bool match = true;
                    for (int k = 0; k < 8; ++k) {
                        if (g_selected_sounds[k] != g_target_sequence[k]) {
                            match = false;
                            break;
                        }
                    }

                    if (match) {
                        // VICTORY!
                        g_round_victory = true;
                        if (g_current_level >= 1 && g_current_level <= 4) {
                            g_levels_cleared[g_current_level - 1] = 1;
                            save_progression();
                        }
                        g_state = STATE_ROUND_OVER;
                    } else {
                        // Not matching: decrement combination attempts (up to 5 allowed)
                        g_attempts_left--;
                        if (g_attempts_left > 0) {
                            // Don't defeat directly! Give the player another chance
                            audio::play_buzzer();
                            g_feedback_timer_ms = 2200; // Display wrong combo feedback
                            // Reset current tiles so the player can try their next series
                            g_click_count = 0;
                            for (int i = 0; i < 8; ++i) {
                                g_selected_cases[i] = -1;
                                g_selected_sounds[i] = 0;
                            }
                        } else {
                            // 5 attempts exhausted: DEFEAT
                            g_round_defeat = true;
                            g_state = STATE_ROUND_OVER;
                        }
                    }
                }
            }
        } else if (found_order == g_click_count - 1) {
            // Player clicked on the very last selected tile -> UNDO
            g_selected_cases[g_click_count - 1] = -1;
            g_selected_sounds[g_click_count - 1] = 0;
            g_click_count--;
            audio::play_undo();
        } else {
            // Player clicked on an already selected tile that is NOT the last -> Error buzzer
            audio::play_buzzer();
        }
    }
}

void handle_round_over_click(int mx, int my) {
    if (mx < 0 || my < 0) return;

    // OUI button at (200, 340, 160, 70) -> Replay this round (music stays off)
    if (mx >= 200 && mx <= 360 && my >= 340 && my <= 410) {
        start_level(g_current_level);
        return;
    }

    // NON button at (480, 340, 160, 70) -> Back to main menu (resume background music)
    if (mx >= 480 && mx <= 640 && my >= 340 && my <= 410) {
        audio::start_music();
        g_state = STATE_MENU;
        return;
    }
}

} // namespace game
