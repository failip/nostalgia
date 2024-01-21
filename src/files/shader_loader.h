#pragma once

#include <webgpu/webgpu.hpp>

#include <filesystem>

class ShaderLoader {
    public:
        using path = std::filesystem::path;
        static wgpu::ShaderModule load_shader_module(const path& path, wgpu::Device device);
};