#include "assets.h"
#include "renderer.h"
#include <map>
#include <cstdio>

namespace assets {

static std::map<std::string, SDL_Texture*> g_textures;
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

SDL_Texture* get_texture(const std::string& filename) {
    auto it = g_textures.find(filename);
    if (it != g_textures.end()) {
        return it->second;
    }

    SDL_Renderer* ren = renderer::get_renderer();
    if (!ren) return NULL;

    std::string path = get_asset_path(filename.c_str());
    SDL_Surface* surf = SDL_LoadBMP(path.c_str());
    if (!surf) {
        fprintf(stderr, "Avertissement : impossible de charger l'image BMP : %s (chemin résolu: %s)\n",
                filename.c_str(), path.c_str());
        return NULL;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_DestroySurface(surf);

    if (tex) {
        g_textures[filename] = tex;
    }
    return tex;
}

TTF_Font* get_font(const std::string& filename, int point_size) {
    char key[256];
    snprintf(key, sizeof(key), "%s_%d", filename.c_str(), point_size);

    auto it = g_fonts.find(key);
    if (it != g_fonts.end()) {
        return it->second;
    }

    std::string path = get_asset_path(filename.c_str());
    TTF_Font* font = TTF_OpenFont(path.c_str(), (float)point_size);
    if (!font) {
        fprintf(stderr, "Avertissement : impossible de charger la police TTF : %s (%s)\n",
                filename.c_str(), SDL_GetError());
        return NULL;
    }

    g_fonts[key] = font;
    return font;
}

void cleanup() {
    for (auto& pair : g_textures) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    g_textures.clear();

    for (auto& pair : g_fonts) {
        if (pair.second) {
            TTF_CloseFont(pair.second);
        }
    }
    g_fonts.clear();
}

} // namespace assets
