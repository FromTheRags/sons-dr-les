#ifndef COMMON_H
#define COMMON_H

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <unistd.h>
#else
    #include <unistd.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// Base virtual screen dimensions (matches original 2013 game layout)
const int VIRTUAL_W = 840;
const int VIRTUAL_H = 680;

// Game states
enum GameState {
    STATE_MENU,
    STATE_RULES,
    STATE_PLAYING,
    STATE_ROUND_OVER,
    STATE_EXIT
};

// Colors palette
namespace colors {
    const SDL_Color White       = {255, 255, 255, 255};
    const SDL_Color Black       = {0, 0, 0, 255};
    const SDL_Color Gold        = {255, 215, 0, 255};
    const SDL_Color Indigo      = {10, 206, 212, 255};
    const SDL_Color Green       = {128, 255, 23, 255};
    const SDL_Color Orange      = {255, 123, 0, 255};
    const SDL_Color Red         = {255, 40, 40, 255};
    const SDL_Color Yellow      = {255, 233, 0, 255};
    const SDL_Color Purple      = {165, 32, 175, 255};
    const SDL_Color Pink        = {255, 81, 255, 255};
    const SDL_Color Navy        = {30, 80, 255, 255};
    const SDL_Color AppleGreen  = {177, 255, 23, 255};
    const SDL_Color Beige       = {240, 213, 185, 255};
}

#endif // COMMON_H
