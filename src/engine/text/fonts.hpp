#pragma once

#include <SDL3/SDL.h>
#include "../sprites/sprites.hpp"

namespace TEngine::Text {
    using TextFlipMode = Sprites::SpriteFlipMode;

    // BitmapFont assumes all characters are in tiles of same width and height, where width == height.
    class BitmapFont final {
    public:
        static BitmapFont load_font(const std::filesystem::path& filepath, int tile_size, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color key);

        BitmapFont() = default;
        // move pre-configured atlas 
        // BitmapFont(Sprites::SpriteAtlas&& atlas, int tile_size);
        BitmapFont(BitmapFont&& other) noexcept;
 
        int tile_size() { return m_atlas.tile_width(); }
        SDL_Renderer* get_renderer(void) { return m_atlas.get_renderer(); }
        Sprites::SpriteAtlas& get_texture_atlas(void) { return m_atlas; }
        void draw_char(char c, float offset_x, float offset_y, float scale_width, float scale_height, float angle = 0, TextFlipMode flipmode = {});
        
        BitmapFont& operator=(BitmapFont other);
        friend void swap(BitmapFont& fst, BitmapFont& snd) {
            using std::swap;

            swap(fst.m_atlas, snd.m_atlas);
            // swap(fst.m_tile_size, snd.m_tile_size);
        }
    private:
        Sprites::SpriteAtlas m_atlas{};
    };

    // TODO - try to read TTF fonts and perhaps other formats at some point
    // NOTE - will need some extra planning before anything is done
    class TTFFont final {
    public:
        TTFFont() : m_renderer(nullptr) {}
        static TTFFont load_font(const std::filesystem::path& font_path, SDL_Renderer* renderer);
    private:
        TTFFont(SDL_Renderer* renderer) : m_renderer(renderer) {}

        SDL_Renderer* m_renderer = nullptr;

        struct TableRecord {
            std::uint8_t tag[4];
            std::uint32_t checksum;
            std::uint32_t offset32 = 0;
            std::uint32_t length;
        };
        struct Data {
            // 0x00010000 for TreType,
            // 0x4F54544F for CFF
            std::uint32_t sfntVersion;
            std::uint16_t numTables;
            // = 2 ^ floor(log2(numTables)) * 16
            std::uint16_t searchRange;
            // log2(searchRange/16)
            std::uint16_t entrySelector;
            // numTables * 16 - searchRange
            std::uint16_t rangeShift;

            struct CMAP {
                std::uint16_t version = 0;
                std::uint16_t numTables;
                struct EncodingRecord {
                    // platform == either 0 (Unicode) or 3 (Windows)
                    std::uint16_t platformID;
                    // encoding == either 
                    std::uint16_t encodingID;
                    std::uint32_t subtableOffset;
                };
            } cmap;
            struct HEAD {

            } head;
            struct HHEA {

            } hhea;
            struct HMTX {

            } hmtx;
            struct MAXP {

            } maxp;
            struct NAME {

            } name;
            struct OS2 {

            } os2;
            struct POST {

            } post;
            union {
                struct TTOutlines {
                    struct CVT {

                    } cvt;
                    struct FPGM {

                    } fpgm;
                    struct GLYF {

                    } glyf;
                    struct LOCA {

                    } loca;
                    struct PREP {

                    } prep;
                    struct GASP {

                    } gasp;
                } ttf_outlines;
                struct CCFOutlines {
                    struct CFF {

                    } cff;
                    struct CFF2 {

                    } cff2;
                    struct VORG {

                    } vorg;
                } ccf_outlines;
            } outlines;

            struct Advanced {
                struct BASE {

                } base;
                struct GDEF {

                } gdef;
                struct GPOS {

                } gpos;
                struct GSUB {

                } gsub;
                struct JSTF {

                } jstf;
                struct MATH {

                } math;
            };
        } m_ttf_data;
    };
}