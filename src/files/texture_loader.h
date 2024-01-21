#pragma once

#include <filesystem>
#include <webgpu/webgpu.hpp>

class TextureLoader {
    public:
        using path = std::filesystem::path;
        
        static wgpu::Texture load_texture(const path& path, wgpu::Device device, wgpu::TextureView* pTextureView = nullptr);
        static wgpu::Texture load_tilemap_as_texture(const path& path, wgpu::Device device, wgpu::TextureView* pTextureView = nullptr);
};