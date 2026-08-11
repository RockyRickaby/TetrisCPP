#include "text_renderer.hpp"
#include "fonts.hpp"

namespace TEngine::Text {
    // void destroy_bitmap_renderer(BitmapRenderer* bmr) {
    //     // delete bmr;
    // }

    void BitmapRenderer::destroy_bitmap_text(BitmapText text) {
        SDL_DestroyTexture(text.text_data);
    }
}