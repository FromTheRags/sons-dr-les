#ifndef COMMON_H
#define COMMON_H

#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#include <cstdio>
extern "C" {
    FILE * __cdecl __iob_func(void)
    {
        return __acrt_iob_func(0);
    }
}
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#ifndef _SDL_main_h
#define _SDL_main_h
#endif
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#if defined(__APPLE__)
    #include <SDL/SDL.h>
    #include <SDL/SDL_ttf.h>
#elif defined(__linux__) || defined(__unix__)
    #if __has_include(<SDL/SDL.h>)
        #include <SDL/SDL.h>
    #else
        #include <SDL.h>
    #endif
    #if __has_include(<SDL/SDL_ttf.h>)
        #include <SDL/SDL_ttf.h>
    #else
        #include <SDL_ttf.h>
    #endif
#else
    #if __has_include(<SDL/SDL.h>)
        #include <SDL/SDL.h>
    #else
        #include <SDL.h>
    #endif
    #if __has_include(<SDL/SDL_ttf.h>)
        #include <SDL/SDL_ttf.h>
    #else
        #include <SDL_ttf.h>
    #endif
#endif

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
