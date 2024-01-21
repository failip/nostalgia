#include "tilemap_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

TilemapLoader::Tilemap TilemapLoader::load_tilemap(const std::filesystem::path &path) {
    std::ifstream file(path);
    json data = json::parse(file);

    uint32_t number_of_layers = data["layers"].size();
    uint32_t width = data["width"];
    uint32_t height = data["height"];

    std::vector<uint32_t> layer1 = data["layers"][0]["data"];
    std::vector<uint32_t> layer2 = data["layers"][1]["data"];

    std::vector<uint32_t> layer;
    layer.reserve(layer1.size() + layer2.size());
    layer.insert(layer.end(), layer1.begin(), layer1.end());
    layer.insert(layer.end(), layer2.begin(), layer2.end());

    Tilemap tilemap = {layer, width, height, number_of_layers};
    return tilemap;
}