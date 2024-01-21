#ifndef RESOURCE_DIR
#define RESOURCE_DIR "resources"
#endif

#include "engine.h"
#include "../files/shader_loader.h"
#include "../files/geometry_loader.h"
#include "../files/texture_loader.h"

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}

bool Engine::on_init()  {
    if (!init_window_and_device()) return false;
    if (!init_swap_chain()) return false;
    if (!init_render_pipeline()) return false;
    if (!init_textures()) return false;
    if (!init_geometries()) return false;
    if (!init_buffers()) return false;
    if (!init_bindings()) return false;
    return true;
}

void Engine::on_finish() {
    terminate_bindings();
    terminate_buffers();
    terminate_textures();
    terminate_render_pipeline();
    terminate_swap_chain();
    terminate_window_and_device();
}

void Engine::on_frame() {
    glfwPollEvents();

    m_uniforms.time = static_cast<float>(glfwGetTime());
    m_queue.writeBuffer(m_uniform_buffer, offsetof(MyUniforms, time), &m_uniforms.time, sizeof(MyUniforms::time));

    TextureView nextTexture = m_swap_chain.getCurrentTextureView();
    if (!nextTexture) {
        std::cerr << "Cannot acquire next swap chain texture" << std::endl;
        return;
    }

    CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Command Encoder";
    CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);
    
    RenderPassDescriptor renderPassDesc;

    RenderPassColorAttachment renderPassColorAttachment{};
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    renderPassColorAttachment.clearValue = Color{ 0.05, 0.05, 0.05, 1.0 };
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;
    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

    renderPass.setPipeline(m_render_pipeline);

    renderPass.setVertexBuffer(0, m_vertex_buffer, 0, m_point_data.size() * sizeof(float));
    renderPass.setIndexBuffer(m_index_buffer, IndexFormat::Uint16, 0, m_index_data.size() * sizeof(uint16_t));

    uint32_t dynamicOffset = 0;

    // Set binding group
    dynamicOffset = 0 * m_uniform_stride;
    renderPass.setBindGroup(0, m_bind_group, 1, &dynamicOffset);
    uint32_t index_count = (uint32_t)m_index_data.size();
    renderPass.drawIndexed(index_count, 1, 0, 0, 0);

    // Set binding group with a different uniform offset
    // dynamicOffset = 1 * m_uniform_stride;
    // renderPass.setBindGroup(0, m_bind_group, 1, &dynamicOffset);
    // renderPass.drawIndexed(index_count, 1, 0, 0, 0);

    renderPass.end();
    
    nextTexture.release();

    CommandBufferDescriptor cmdBufferDescriptor{};
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    m_queue.submit(1, &command);

    m_swap_chain.present();
    // Check for pending error callbacks
    m_device.tick();
    // break;
}

bool Engine::is_running() const {
    return !glfwWindowShouldClose(m_window);
}

Engine::Engine(const u_int32_t width, const u_int32_t height) : m_width(width), m_height(height) {

}

bool Engine::init_window_and_device() {
    m_instance = createInstance(InstanceDescriptor{});

    if (!m_instance) {
        std::cerr << "Could not create instance!" << std::endl;
        return false;
    }

    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, "nostalgia", NULL, NULL);
    if (!m_window) {
        std::cerr << "Could not open window!" << std::endl;
        return false;
    }

    glfwSetWindowUserPointer(m_window, this);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        Engine* engine = (Engine*)glfwGetWindowUserPointer(window);
        engine->resize_screen(width, height);
    });

    m_surface = glfwGetWGPUSurface(m_instance, m_window);
    RequestAdapterOptions adapter_options{};
    adapter_options.compatibleSurface = m_surface;
    m_adapter = m_instance.requestAdapter(adapter_options);

    SupportedLimits supported_limits;
    m_adapter.getLimits(&supported_limits);

    RequiredLimits required_limits = Default;
    required_limits.limits.maxVertexAttributes = 2;
    required_limits.limits.maxVertexBuffers = 1;
    required_limits.limits.maxBufferSize = 15 * 5 * sizeof(float);
    required_limits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
    required_limits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
    required_limits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
    required_limits.limits.maxInterStageShaderComponents = 10;
    required_limits.limits.maxBindGroups = 1;
    required_limits.limits.maxUniformBuffersPerShaderStage = 1;
    required_limits.limits.maxUniformBufferBindingSize = 16 * 4;
    required_limits.limits.maxTextureDimension1D = 4096;
    required_limits.limits.maxTextureDimension2D = 4096;
    required_limits.limits.maxSampledTexturesPerShaderStage = 5;
    // Extra limit requirement
    required_limits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

    DeviceDescriptor device_descriptor{};
    device_descriptor.label = "GPU";
    device_descriptor.requiredFeaturesCount = 0;
    device_descriptor.requiredLimits = &required_limits;
    device_descriptor.defaultQueue.label = "default";
    m_device = m_adapter.requestDevice(device_descriptor);
    // Get device limits
    SupportedLimits device_supported_limits;
    m_device.getLimits(&device_supported_limits);
    m_device_limits = device_supported_limits.limits;

    // Add an error callback for more debug info
    auto h = m_device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
        std::cout << "Device error: type " << type;
        if (message) std::cout << " (message: " << message << ")";
        std::cout << std::endl;
        });

    m_queue = m_device.getQueue();

    return true;
}

void Engine::terminate_window_and_device() {
    m_surface.release();
    m_queue.release();
    m_device.release();
    m_adapter.release();
    m_instance.release();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Engine::init_swap_chain() {
    SwapChainDescriptor swap_chain_descriptor = {};
    swap_chain_descriptor.width = m_width;
    swap_chain_descriptor.height = m_height;
    swap_chain_descriptor.usage = TextureUsage::RenderAttachment;
    swap_chain_descriptor.format = m_swap_chain_format;
    swap_chain_descriptor.presentMode = PresentMode::Fifo;
    m_swap_chain = m_device.createSwapChain(m_surface, swap_chain_descriptor);
    return m_swap_chain != nullptr;
}

void Engine::terminate_swap_chain() {
    m_swap_chain.release();
}

void Engine::terminate_render_pipeline() {
    m_render_pipeline.release();
    m_shader_module.release();
    m_bind_group_layout.release();
}

bool Engine::init_render_pipeline() {
    RenderPipelineDescriptor pipeline_descriptor;

    std::filesystem::path path = std::filesystem::path(RESOURCE_DIR "/shaders/shader.wgsl");
    m_shader_module = ShaderLoader::load_shader_module(path, m_device);

    // Vertex fetch
    std::vector<VertexAttribute> vertex_attributes(2);

    // Position attribute
    vertex_attributes[0].shaderLocation = 0;
    vertex_attributes[0].format = VertexFormat::Float32x2;
    vertex_attributes[0].offset = 0;

    // Color attribute
    vertex_attributes[1].shaderLocation = 1;
    vertex_attributes[1].format = VertexFormat::Float32x3;
    vertex_attributes[1].offset = 2 * sizeof(float);

    VertexBufferLayout vertex_buffer_layout;
    vertex_buffer_layout.attributeCount = (uint32_t)vertex_attributes.size();
    vertex_buffer_layout.attributes = vertex_attributes.data();
    vertex_buffer_layout.arrayStride = 5 * sizeof(float);
    vertex_buffer_layout.stepMode = VertexStepMode::Vertex;

    pipeline_descriptor.vertex.bufferCount = 1;
    pipeline_descriptor.vertex.buffers = &vertex_buffer_layout;

    pipeline_descriptor.vertex.module = m_shader_module;
    pipeline_descriptor.vertex.entryPoint = "vs_main";
    pipeline_descriptor.vertex.constantCount = 0;
    pipeline_descriptor.vertex.constants = nullptr;

    pipeline_descriptor.primitive.topology = PrimitiveTopology::TriangleList;
    pipeline_descriptor.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipeline_descriptor.primitive.frontFace = FrontFace::CCW;
    pipeline_descriptor.primitive.cullMode = CullMode::None;

    FragmentState fragment_state;
    pipeline_descriptor.fragment = &fragment_state;
    fragment_state.module = m_shader_module;
    fragment_state.entryPoint = "fs_main";
    fragment_state.constantCount = 0;
    fragment_state.constants = nullptr;

    BlendState blend_state{};
    blend_state.color.srcFactor = BlendFactor::SrcAlpha;
    blend_state.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blend_state.color.operation = BlendOperation::Add;
    blend_state.alpha.srcFactor = BlendFactor::Zero;
    blend_state.alpha.dstFactor = BlendFactor::One;
    blend_state.alpha.operation = BlendOperation::Add;

    ColorTargetState color_target;
    color_target.format = m_swap_chain_format;
    color_target.blend = &blend_state;
    color_target.writeMask = ColorWriteMask::All;

    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;
    
    pipeline_descriptor.depthStencil = nullptr;

    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.multisample.mask = ~0u;
    pipeline_descriptor.multisample.alphaToCoverageEnabled = false;

    std::vector<BindGroupLayoutEntry> binding_layout_entries(3, Default);
    // Create binding layout
    BindGroupLayoutEntry& bindingLayout = binding_layout_entries[0];
    bindingLayout.binding = 0;
    bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    bindingLayout.buffer.type = BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
    // Make this binding dynamic so we can offset it between draw calls
    bindingLayout.buffer.hasDynamicOffset = true;

    BindGroupLayoutEntry& texture_binding_layout = binding_layout_entries[1];
    texture_binding_layout.binding = 1;
    texture_binding_layout.visibility = ShaderStage::Fragment;
    texture_binding_layout.texture.sampleType = TextureSampleType::Float;
    texture_binding_layout.texture.viewDimension = TextureViewDimension::_2D;

    BindGroupLayoutEntry& tilemap_binding_layout = binding_layout_entries[2];
    tilemap_binding_layout.binding = 2;
    tilemap_binding_layout.visibility = ShaderStage::Fragment;
    tilemap_binding_layout.texture.sampleType = TextureSampleType::Uint;
    tilemap_binding_layout.texture.viewDimension = TextureViewDimension::_2D;

    // Create a bind group layout
    BindGroupLayoutDescriptor bind_group_layout_descriptor;
    bind_group_layout_descriptor.entryCount = (uint32_t)binding_layout_entries.size();
    bind_group_layout_descriptor.entries = binding_layout_entries.data();
    m_bind_group_layout = m_device.createBindGroupLayout(bind_group_layout_descriptor);

    // Create the pipeline layout
    PipelineLayoutDescriptor pipeline_layout_descriptor;
    pipeline_layout_descriptor.bindGroupLayoutCount = 1;
    pipeline_layout_descriptor.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bind_group_layout;
    PipelineLayout layout = m_device.createPipelineLayout(pipeline_layout_descriptor);
    pipeline_descriptor.layout = layout;

    m_render_pipeline = m_device.createRenderPipeline(pipeline_descriptor);

    return m_render_pipeline != nullptr;
}

bool Engine::init_textures() {

    std::filesystem::path tileset_path = std::filesystem::path(RESOURCE_DIR "/textures/overworld.png");
    m_tileset_texture = TextureLoader::load_texture(tileset_path, m_device, &m_tileset_texture_view);
    if (!m_tileset_texture) {
        std::cerr << "Could not load texture!" << std::endl;
        return false;
    }

    std::filesystem::path tilemap_path = std::filesystem::path(RESOURCE_DIR "/tilemaps/map.tmj");
    m_tilemap_texture = TextureLoader::load_tilemap_as_texture(tilemap_path, m_device, &m_tilemap_texture_view);
    if (!m_tilemap_texture) {
        std::cerr << "Could not load tilemap texture!" << std::endl;
        return false;
    }

    return true;
}

bool Engine::init_geometries() {

    std::filesystem::path geometry_path = std::filesystem::path(RESOURCE_DIR "/geometries/webgpu.txt");
    bool success = GeometryLoader::load_geometry(geometry_path, m_point_data, m_index_data);
    if (!success) {
        std::cerr << "Could not load geometry!" << std::endl;
        return false;
    }
    return true;
}

bool Engine::init_buffers() {

    // Create vertex buffer
    BufferDescriptor buffer_description;
    buffer_description.size = m_point_data.size() * sizeof(float);
    buffer_description.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    buffer_description.mappedAtCreation = false;
    m_vertex_buffer = m_device.createBuffer(buffer_description);
    m_queue.writeBuffer(m_vertex_buffer, 0, m_point_data.data(), buffer_description.size);
    
    // Create index buffer
    buffer_description.size = m_index_data.size() * sizeof(float);
    buffer_description.usage = BufferUsage::CopyDst | BufferUsage::Index;
    buffer_description.mappedAtCreation = false;
    m_index_buffer = m_device.createBuffer(buffer_description);
    m_queue.writeBuffer(m_index_buffer, 0, m_index_data.data(), buffer_description.size);

    m_uniform_stride = ceilToNextMultiple(
        (uint32_t)sizeof(MyUniforms),
        (uint32_t)m_device_limits.minUniformBufferOffsetAlignment
    );
    // The buffer will contain 2 values for the uniforms plus the space in between
    // (NB: stride = sizeof(MyUniforms) + spacing)
    buffer_description.size = m_uniform_stride + sizeof(MyUniforms);
    buffer_description.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
    buffer_description.mappedAtCreation = false;
    m_uniform_buffer = m_device.createBuffer(buffer_description);

    m_uniforms.time = 1.0f;
    m_queue.writeBuffer(m_uniform_buffer, 0, &m_uniforms, sizeof(MyUniforms));

    return true;
}

bool Engine::init_bindings() {

    m_bindings = std::vector<BindGroupEntry>(3);
    m_bindings[0].binding = 0;
    m_bindings[0].buffer = m_uniform_buffer;
    m_bindings[0].offset = 0;
    m_bindings[0].size = sizeof(MyUniforms);

    m_bindings[1].binding = 1;
    m_bindings[1].textureView = m_tileset_texture_view;

    m_bindings[2].binding = 2;
    m_bindings[2].textureView = m_tilemap_texture_view;

    m_bind_group_descriptor.layout = m_bind_group_layout;
    m_bind_group_descriptor.entryCount = (uint32_t)m_bindings.size();
    m_bind_group_descriptor.entries = m_bindings.data();
    m_bind_group = m_device.createBindGroup(m_bind_group_descriptor);

    return true;
}

void Engine::terminate_buffers() {
    m_vertex_buffer.destroy();
    m_vertex_buffer.release();
    m_index_buffer.destroy();
    m_index_buffer.release();
    m_uniform_buffer.destroy();
    m_uniform_buffer.release();
}

void Engine::terminate_bindings() {
    m_bind_group.release();
}

void Engine::resize_screen(const u_int32_t width, const u_int32_t height) {
    terminate_swap_chain();

    m_width = width / 2;
    m_height = height / 2;

    init_swap_chain();
}

void Engine::terminate_textures() {
    m_tilemap_texture_view.release();
    m_tilemap_texture.destroy();
    m_tilemap_texture.release();
    m_tileset_texture_view.release();
    m_tileset_texture.destroy();
    m_tileset_texture.release();
}
