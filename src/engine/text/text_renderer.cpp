#include "text_renderer.hpp"
#include "fonts.hpp"

namespace TEngine::Text {
    BitmapRenderer init_bitmap_renderer(SDL_Renderer* renderer) {
        const auto& jf = BitmapFonts::JoustFont::instance(renderer);

        return BitmapRenderer(renderer);
    }

    // void destroy_bitmap_renderer(BitmapRenderer* bmr) {
    //     // delete bmr;
    // }

    void destroy_bitmap_text(BitmapText text) {
        SDL_DestroyTexture(text.text_data);
    }
}