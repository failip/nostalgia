#include "shader_loader.h"
#include <fstream>

wgpu::ShaderModule ShaderLoader::load_shader_module(const path &path, wgpu::Device device) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);

    wgpu::ShaderModuleWGSLDescriptor shader_code_descriptor;
    shader_code_descriptor.chain.next = nullptr;
    shader_code_descriptor.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    shader_code_descriptor.code = shaderSource.c_str();
    wgpu::ShaderModuleDescriptor shader_descriptor;
    shader_descriptor.nextInChain = &shader_code_descriptor.chain;
    return device.createShaderModule(shader_descriptor);
};
