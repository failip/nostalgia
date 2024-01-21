#pragma once

#include <webgpu/webgpu.hpp>

using namespace wgpu;

struct GLFWwindow;

class Engine {

    struct MyUniforms {
        //  4
        float tilemap_width;
        float tilemap_height;
        float number_of_layers;
        float _pad0;
        float time;
        float screen_width;
        float screen_height;
        float _pad;
    };

    public:
        bool on_init();
        void on_finish();
        void on_frame();
        bool is_running() const;
        Engine(const u_int32_t width, const u_int32_t height);

    private:
        Instance m_instance = nullptr;
        Surface m_surface = nullptr;
        Adapter m_adapter = nullptr;
        Device m_device = nullptr;
        Queue m_queue = nullptr;
        SwapChain m_swap_chain = nullptr;
        const TextureFormat m_swap_chain_format = TextureFormat::BGRA8Unorm;
        ShaderModule m_shader_module = nullptr;
        BindGroupLayout m_bind_group_layout = nullptr;
        Limits m_device_limits = {};
        RenderPipeline m_render_pipeline = nullptr;
        TextureView m_tilemap_texture_view = nullptr;
        Texture m_tilemap_texture = nullptr;
        TextureView m_tileset_texture_view = nullptr;
        Texture m_tileset_texture = nullptr;
        std::vector<float> m_point_data;
        std::vector<uint16_t> m_index_data;
        Buffer m_vertex_buffer = nullptr;
        Buffer m_index_buffer = nullptr;
        Buffer m_uniform_buffer = nullptr;
        BindGroup m_bind_group = nullptr;
        BindGroupDescriptor m_bind_group_descriptor = {};
        std::vector<BindGroupEntry> m_bindings;
        MyUniforms m_uniforms = {};
        uint32_t m_uniform_stride = 0;

        GLFWwindow* m_window = nullptr;

        u_int32_t m_width = 0;
        u_int32_t m_height = 0;

        bool init_window_and_device();
        bool init_swap_chain();
        bool init_render_pipeline();
        bool init_textures();
        bool init_geometries();
        bool init_buffers();
        bool init_bindings();
        void terminate_window_and_device();
        void terminate_swap_chain();
        void terminate_render_pipeline();
        void terminate_textures();
        void terminate_buffers();
        void terminate_bindings();

        void resize_screen(const u_int32_t width, const u_int32_t height);
};