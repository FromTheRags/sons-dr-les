#ifndef GAME_H
#define GAME_H

#include "common.h"

namespace game {

// Level progression (levels 1 to 4)
void load_progression();
void save_progression();
bool is_extreme_unlocked();
bool is_level_cleared(int level);

// Start a new round for the given level (1..5)
void start_level(int level);

// Game update per frame
void update(Uint32 delta_ms);

// Game rendering
void render_menu(SDL_Surface* dst);
void render_rules(SDL_Surface* dst);
void render_game(SDL_Surface* dst);
void render_round_over(SDL_Surface* dst);

// Handle clicks
void handle_menu_click(int mx, int my);
void handle_rules_click(int mx, int my);
void handle_game_click(int mx, int my);
void handle_round_over_click(int mx, int my);

// Replay sequence listener
void play_target_sequence();

// State transitions
GameState get_state();
void set_state(GameState state);

int get_current_level();
int get_replays_left();
int get_attempts_left();

} // namespace game

#endif // GAME_H
