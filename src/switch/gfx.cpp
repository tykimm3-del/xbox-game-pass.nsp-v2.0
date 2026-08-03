#include "gfx.hpp"

#include <SDL2/SDL_image.h>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <cmath>
#include <cstdio>

namespace gnx::gfx {

namespace {
constexpr int kFontPx[5] = {24, 38, 54, 100, 30};

SDL_Texture* g_background = nullptr;
int g_background_slot = 1;

void destroy_background() {
    if (g_background) {
        SDL_DestroyTexture(g_background);
        g_background = nullptr;
    }
}

void load_background(SDL_Renderer* renderer) {
    destroy_background();
    if (!renderer) return;
    if (g_background_slot <= 0) return;

    char path[160] = {};
#ifdef __SWITCH__
    std::snprintf(path, sizeof(path),
                  "sdmc:/switch/xbox/backgrounds/background_%02d.png",
                  g_background_slot);
#else
    std::snprintf(path, sizeof(path),
                  "backgrounds/background_%02d.png",
                  g_background_slot);
#endif
    g_background = IMG_LoadTexture(renderer, path);

#ifdef __SWITCH__
    if (!g_background) {
        std::snprintf(path, sizeof(path),
                      "romfs:/backgrounds/background_%02d.png",
                      g_background_slot);
        g_background = IMG_LoadTexture(renderer, path);
    }
#endif
}
}

bool Gfx::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                 SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() != 0) return false;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP);

    // SDL_Init already brought up the video subsystem; just make the window.
    if (!create_window_renderer()) return false;
    load_background(renderer_);

#ifdef __SWITCH__
    PlFontData shared_font;
    if (R_SUCCEEDED(plGetSharedFontByType(&shared_font,
                                          PlSharedFontType_KO))) {
        font_data_ = shared_font.address;
        for (int i = 0; i < 5; ++i) {
            SDL_RWops* rw =
                SDL_RWFromConstMem(shared_font.address, shared_font.size);
            fonts_[i] = TTF_OpenFontRW(rw, 1, kFontPx[i]);
        }
    }
#else
    const char* path = "/System/Library/Fonts/Helvetica.ttc";
    for (int i = 0; i < 5; ++i) fonts_[i] = TTF_OpenFont(path, kFontPx[i]);
#endif
    return fonts_[0] != nullptr;
}

void Gfx::shutdown() {
    destroy_background();
    for (auto& [key, cached] : text_cache_)
        SDL_DestroyTexture(cached.texture);
    text_cache_.clear();
    for (TTF_Font*& handle : fonts_)
        if (handle) TTF_CloseFont(handle), handle = nullptr;
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

bool Gfx::create_window_renderer() {
    window_ = SDL_CreateWindow("green-nx", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, kWidth, kHeight,
                               SDL_WINDOW_SHOWN);
    if (!window_) return false;
    renderer_ = SDL_CreateRenderer(
        window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) return false;
    SDL_RenderSetLogicalSize(renderer_, kWidth, kHeight);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    return true;
}

bool Gfx::resume() {
    // Re-init SDL video after the native streaming renderer releases it.
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return false;
    if (!create_window_renderer()) return false;
    load_background(renderer_);
    return true;
}

void Gfx::suspend() {
    destroy_background();
    for (auto& [key, cached] : text_cache_)
        SDL_DestroyTexture(cached.texture);
    text_cache_.clear();
    if (renderer_) SDL_DestroyRenderer(renderer_), renderer_ = nullptr;
    if (window_) SDL_DestroyWindow(window_), window_ = nullptr;
    // Destroying the window is not enough: SDL's mesa/EGL backend keeps the
    // default nwindow bound until the whole video subsystem is torn down. Fully
    // quit it so deko3d can become the window's buffer producer. SDL_Init took
    // one ref at startup, so this single Quit actually shuts the subsystem down.
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

TTF_Font* Gfx::font(FontSize size) {
    return fonts_[static_cast<int>(size)];
}

void Gfx::begin_frame() {
    SDL_SetRenderDrawColor(renderer_, kBg.r, kBg.g, kBg.b, 255);
    SDL_RenderClear(renderer_);
    if (g_background) {
        SDL_Rect destination = {0, 0, kWidth, kHeight};
        SDL_RenderCopy(renderer_, g_background, nullptr, &destination);
    }
}

void Gfx::set_background_slot(int slot) {
    if (slot < 0) slot = 0;
    if (slot > 10) slot = 10;
    g_background_slot = slot;
    load_background(renderer_);
}

void Gfx::reload_background() {
    load_background(renderer_);
}

void Gfx::end_frame() {
    SDL_RenderPresent(renderer_);
    trim_text_cache();
}

void Gfx::fill(const SDL_Rect& rect, Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
}

void Gfx::frame(const SDL_Rect& rect, Color color, int thickness) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; ++i) {
        SDL_Rect outline = {rect.x - i, rect.y - i, rect.w + 2 * i,
                            rect.h + 2 * i};
        SDL_RenderDrawRect(renderer_, &outline);
    }
}

SDL_Texture* Gfx::render_text(const std::string& utf8, FontSize size,
                              Color color, int* width, int* height) {
    std::string key = std::to_string(static_cast<int>(size)) + "|" +
                      std::to_string(color.r) + "," +
                      std::to_string(color.g) + "," +
                      std::to_string(color.b) + "|" + utf8;
    auto found = text_cache_.find(key);
    if (found != text_cache_.end()) {
        found->second.last_used = SDL_GetTicks();
        *width = found->second.width;
        *height = found->second.height;
        return found->second.texture;
    }

    SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended(font(size), utf8.c_str(), sdl_color);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    *width = surface->w;
    *height = surface->h;
    SDL_FreeSurface(surface);
    if (texture)
        text_cache_[key] = {texture, *width, *height, SDL_GetTicks()};
    return texture;
}

int Gfx::text(const std::string& utf8, int x, int y, FontSize size,
              Color color) {
    if (utf8.empty()) return 0;
    int width = 0, height = 0;
    SDL_Texture* texture = render_text(utf8, size, color, &width, &height);
    if (!texture) return 0;
    SDL_Rect destination = {x, y, width, height};
    SDL_RenderCopy(renderer_, texture, nullptr, &destination);
    return width;
}

int Gfx::text_centered(const std::string& utf8, int cx, int y, FontSize size,
                       Color color) {
    return text(utf8, cx - text_width(utf8, size) / 2, y, size, color);
}

int Gfx::text_width(const std::string& utf8, FontSize size) {
    int width = 0, height = 0;
    TTF_SizeUTF8(font(size), utf8.c_str(), &width, &height);
    return width;
}

SDL_Texture* Gfx::texture_from_memory(const void* data, size_t size) {
    SDL_RWops* rw = SDL_RWFromConstMem(data, size);
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void Gfx::draw_texture(SDL_Texture* texture, const SDL_Rect& destination) {
    SDL_RenderCopy(renderer_, texture, nullptr, &destination);
}

void Gfx::spinner(int cx, int y, Uint32 ticks) {
    // 3 pulsing 14x14 squares, 30px apart (redesign card 1c).
    for (int i = 0; i < 3; ++i) {
        float phase = std::sin((ticks / 200.0f) - i * 0.9f);
        Uint8 alpha = static_cast<Uint8>(64 + 191 * (phase > 0 ? phase : 0));
        SDL_Rect dot = {cx - 37 + i * 30, y, 14, 14};
        fill(dot, {kText.r, kText.g, kText.b, alpha});
    }
}

void Gfx::trim_text_cache() {
    if (text_cache_.size() < 512) return;
    Uint32 now = SDL_GetTicks();
    for (auto it = text_cache_.begin(); it != text_cache_.end();) {
        if (now - it->second.last_used > 5000) {
            SDL_DestroyTexture(it->second.texture);
            it = text_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace gnx::gfx
