#include "tilemap_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

TilemapLoader::Tilemap TilemapLoader::load_tilemap(const std::filesystem::path &path) {
    std::ifstream file(path);
    json data = json::parse(file);

    uint32_t number_of_layers = data["layers"].size();
    uint32_t width = data["width"];
    uint32_t height = data["height"];

    std::vector<uint32_t> tilemap_data(width * height * number_of_layers);
    for (uint32_t i = 0; i < number_of_layers; i++) {
        std::vector<uint32_t> layer_data = data["layers"][i]["data"];
        auto layer_start = tilemap_data.begin() + (i * width * height);
        tilemap_data.insert(layer_start, layer_data.begin(), layer_data.end());
    }

    Tilemap tilemap = {tilemap_data, width, height, number_of_layers};
    return tilemap;
}