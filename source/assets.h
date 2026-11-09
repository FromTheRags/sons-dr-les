#ifndef ASSETS_H
#define ASSETS_H

#include "common.h"
#include <string>

namespace assets {

std::string get_asset_path(const char* name);
SDL_Texture* get_texture(const std::string& filename);
TTF_Font* get_font(const std::string& filename, int point_size);
void cleanup();

} // namespace assets

#endif // ASSETS_H
