#include "common.h"
#include "assets.h"
#include "audio.h"
#include "renderer.h"
#include "game.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    srand((unsigned int)time(NULL));

    // Initialize SDL core & TTF
    if (TTF_Init() == -1) {
        fprintf(stderr, "Erreur TTF_Init : %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0) {
        fprintf(stderr, "Erreur SDL_Init : %s\n", SDL_GetError());
        TTF_Quit();
        return EXIT_FAILURE;
    }

    // Initialize video & renderer
    if (!renderer::init_video()) {
        fprintf(stderr, "Échec de l'initialisation vidéo.\n");
        SDL_Quit();
        TTF_Quit();
        return EXIT_FAILURE;
    }

    // Initialize audio system & preload sound effects
    if (!audio::init()) {
        fprintf(stderr, "Avertissement : le système audio n'a pas pu être initialisé.\n");
    } else {
        audio::load_sounds();
        audio::start_music();
    }

    // Load saved levels progression
    game::load_progression();

    SDL_Surface* v_screen = renderer::get_virtual_screen();

    // Automated screenshot generation for documentation
    bool do_screenshot = false;
    for (int i = 1; i < argc; ++i) {
        if (strstr(argv[i], "screenshot")) do_screenshot = true;
    }
#if defined(_WIN32)
    if (strstr(GetCommandLineA(), "screenshot")) do_screenshot = true;
#endif

    if (do_screenshot) {
        // Create directory if not exists
#if defined(_WIN32)
        CreateDirectoryA("Demo", NULL);
#endif
        // Screenshot 1: Menu
        game::set_state(STATE_MENU);
        game::render_menu(v_screen);
        int ret1 = SDL_SaveBMP(v_screen, "Demo/menu.bmp");

        // Screenshot 2: In-Game (Level 1)
        game::start_level(1);
        game::render_game(v_screen);
        int ret2 = SDL_SaveBMP(v_screen, "Demo/ingame.bmp");

        // Screenshot 3: Rules
        game::set_state(STATE_RULES);
        game::render_rules(v_screen);
        int ret3 = SDL_SaveBMP(v_screen, "Demo/rules.bmp");

        FILE* logf = fopen("Demo/screenshot_log.txt", "w");
        if (logf) {
            fprintf(logf, "Saved: menu=%d, ingame=%d, rules=%d\n", ret1, ret2, ret3);
            fclose(logf);
        }

        audio::quit();
        assets::cleanup();
        renderer::cleanup_video();
        TTF_Quit();
        SDL_Quit();
#if defined(_WIN32)
        _exit(0);
#else
        return 0;
#endif
    }

    bool running = true;
    SDL_Event event;
    Uint32 last_time = SDL_GetTicks();

    while (running) {
        Uint32 current_time = SDL_GetTicks();
        Uint32 delta_ms = current_time - last_time;
        last_time = current_time;

        // Poll SDL events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_VIDEORESIZE:
                renderer::handle_resize(event.resize.w, event.resize.h);
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_F11 ||
                    (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))) {
                    renderer::toggle_fullscreen();
                } else if (event.key.keysym.sym == SDLK_p) {
                    audio::toggle_pause();
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (game::get_state() == STATE_MENU) {
                        running = false;
                    } else {
                        audio::stop_all_sfx();
                        audio::start_music();
                        game::set_state(STATE_MENU);
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP: {
                SDL_Rect click = renderer::transform_mouse_coords(event.button.x, event.button.y);
                if (click.x >= 0 && click.y >= 0) {
                    GameState state = game::get_state();
                    if (state == STATE_MENU) {
                        game::handle_menu_click(click.x, click.y);
                    } else if (state == STATE_RULES) {
                        game::handle_rules_click(click.x, click.y);
                    } else if (state == STATE_PLAYING) {
                        game::handle_game_click(click.x, click.y);
                    } else if (state == STATE_ROUND_OVER) {
                        game::handle_round_over_click(click.x, click.y);
                    }
                }
                break;
            }

            default:
                break;
            }
        }

        // Update logic
        game::update(delta_ms);

        // Render current state to virtual backbuffer
        GameState state = game::get_state();
        if (state == STATE_MENU) {
            game::render_menu(v_screen);
        } else if (state == STATE_RULES) {
            game::render_rules(v_screen);
        } else if (state == STATE_PLAYING) {
            game::render_game(v_screen);
        } else if (state == STATE_ROUND_OVER) {
            game::render_round_over(v_screen);
        }

        // Flip & present virtual screen with letterbox scaling
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

#if defined(_WIN32)
    // Avoid MSVC/MinGW 64-bit SDL 1.2 atexit crash (0xC0000005)
    _exit(0);
#else
    return EXIT_SUCCESS;
#endif
}

#if defined(_WIN32) && (defined(__MINGW32__) || defined(__MINGW64__))
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return main(__argc, __argv);
}
#endif
