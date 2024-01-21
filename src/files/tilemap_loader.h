#pragma once
#include <filesystem>

class TilemapLoader {
    public:

        struct Tilemap {
            std::vector<uint32_t> layer;
            uint32_t width;
            uint32_t height;
            uint32_t number_of_layers;
        };
        
        static Tilemap load_tilemap(const std::filesystem::path& path);
};