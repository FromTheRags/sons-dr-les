#include "assets.h"
#include <map>
#include <cstdio>

namespace assets {

static std::map<std::string, SDL_Surface*> g_images;
static std::map<std::string, TTF_Font*> g_fonts;

std::string get_asset_path(const char* name) {
    if (!name || name[0] == '\0') return "";

    const char* prefixes[] = {
        "assets/",
        "assets/images/",
        "assets/sounds/",
        "assets/fonts/",
        "../assets/",
        "../assets/images/",
        "../assets/sounds/",
        "../assets/fonts/",
        ""
    };

    for (const char* prefix : prefixes) {
        std::string cand = std::string(prefix) + name;
        FILE* f = fopen(cand.c_str(), "rb");
        if (f) {
            fclose(f);
            return cand;
        }
    }

    return std::string(name);
}

SDL_Surface* get_image(const std::string& filename) {
    auto it = g_images.find(filename);
    if (it != g_images.end()) {
        return it->second;
    }

    std::string path = get_asset_path(filename.c_str());
    SDL_Surface* surf = SDL_LoadBMP(path.c_str());
    if (!surf) {
        fprintf(stderr, "Avertissement : impossible de charger l'image BMP : %s (chemin résolu: %s)\n",
                filename.c_str(), path.c_str());
        return NULL;
    }

    g_images[filename] = surf;
    return surf;
}

TTF_Font* get_font(const std::string& filename, int point_size) {
    std::string key = filename + "@" + std::to_string(point_size);
    auto it = g_fonts.find(key);
    if (it != g_fonts.end()) {
        return it->second;
    }

    std::string path = get_asset_path(filename.c_str());
    TTF_Font* font = TTF_OpenFont(path.c_str(), point_size);
    if (!font) {
        fprintf(stderr, "Avertissement : impossible de charger la police : %s (chemin résolu: %s) : %s\n",
                filename.c_str(), path.c_str(), TTF_GetError());
        return NULL;
    }

    g_fonts[key] = font;
    return font;
}

void cleanup() {
    for (auto& pair : g_images) {
        if (pair.second) {
            SDL_FreeSurface(pair.second);
        }
    }
    g_images.clear();

    for (auto& pair : g_fonts) {
        if (pair.second) {
            TTF_CloseFont(pair.second);
        }
    }
    g_fonts.clear();
}

} // namespace assets
