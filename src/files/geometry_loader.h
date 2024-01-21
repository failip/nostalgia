#pragma once

#include <filesystem>

class GeometryLoader {
    public:
        using path = std::filesystem::path;
        static bool load_geometry(const path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData);
};