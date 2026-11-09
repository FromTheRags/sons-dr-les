#include "common.h"
#include <SDL3/SDL_main.h>
#include "assets.h"
#include "audio.h"
#include "renderer.h"
#include "game.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    srand((unsigned int)time(NULL));

    // Initialize SDL3 Core & TTF
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "Erreur SDL_Init : %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (!TTF_Init()) {
        fprintf(stderr, "Erreur TTF_Init : %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Initialize video & renderer with logical letterbox presentation
    if (!renderer::init_video()) {
        fprintf(stderr, "Échec de l'initialisation vidéo.\n");
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Initialize audio system
    if (!audio::init()) {
        fprintf(stderr, "Avertissement : le système audio n'a pas pu être initialisé.\n");
    } else {
        audio::load_sounds();
        audio::start_music();
    }

    // Load saved levels progression
    game::load_progression();

    bool running = true;
    SDL_Event event;
    Uint64 last_time = SDL_GetTicks();

    while (running) {
        Uint64 current_time = SDL_GetTicks();
        Uint32 delta_ms = (Uint32)(current_time - last_time);
        last_time = current_time;

        // Poll SDL3 events
        while (SDL_PollEvent(&event)) {
            // Convert window mouse coordinates to 840x680 logical coordinates
            SDL_ConvertEventToRenderCoordinates(renderer::get_renderer(), &event);

            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_F11 ||
                    (event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT)) ||
                    event.key.key == SDLK_F) {
                    renderer::toggle_fullscreen();
                } else if (event.key.key == SDLK_P) {
                    audio::toggle_pause();
                } else if (event.key.key == SDLK_ESCAPE) {
                    if (game::get_state() == STATE_MENU) {
                        running = false;
                    } else {
                        audio::stop_all_sfx();
                        audio::start_music();
                        game::set_state(STATE_MENU);
                    }
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP: {
                float mx = event.button.x;
                float my = event.button.y;
                GameState state = game::get_state();
                if (state == STATE_MENU) {
                    game::handle_menu_click(mx, my);
                } else if (state == STATE_RULES) {
                    game::handle_rules_click(mx, my);
                } else if (state == STATE_PLAYING) {
                    game::handle_game_click(mx, my);
                } else if (state == STATE_ROUND_OVER) {
                    game::handle_round_over_click(mx, my);
                }
                break;
            }

            default:
                break;
            }
        }

        // Update logic
        game::update(delta_ms);

        // Render current state
        renderer::clear_screen();

        GameState state = game::get_state();
        if (state == STATE_MENU) {
            game::render_menu();
        } else if (state == STATE_RULES) {
            game::render_rules();
        } else if (state == STATE_PLAYING) {
            game::render_game();
        } else if (state == STATE_ROUND_OVER) {
            game::render_round_over();
        }

        // Present hardware rendered frame
        renderer::present_screen();

        // 60 FPS cap
        SDL_Delay(16);
    }

    // Cleanup resources
    audio::quit();
    assets::cleanup();
    renderer::cleanup_video();
    TTF_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}
