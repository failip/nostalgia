#include "texture_loader.h"
#include <stb_image.h>
#include "tilemap_loader.h"

wgpu::Texture TextureLoader::load_texture(const path &path, wgpu::Device device, wgpu::TextureView *pTextureView)
{
    using namespace wgpu;
    int width, height, channels;

    unsigned char *pixelData = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
    if (nullptr == pixelData) {
        return nullptr;
    }

    TextureDescriptor textureDesc;
    textureDesc.dimension = TextureDimension::_2D;
    textureDesc.format = TextureFormat::RGBA8Unorm; // by convention for bmp, png and jpg file. Be careful with other formats.
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.size = { (unsigned int)width, (unsigned int)height, 1 };
    textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    Texture texture = device.createTexture(textureDesc);

    if (pTextureView) {
        TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect = TextureAspect::All;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
        textureViewDesc.dimension = TextureViewDimension::_2D;
        textureViewDesc.format = textureDesc.format;
        *pTextureView = texture.createView(textureViewDesc);
    }

    // Upload data to the GPU texture
    ImageCopyTexture destination;
    destination.texture = texture;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 };
    TextureDataLayout source;
    source.offset = 0;
    source.bytesPerRow = 4 * width;
    source.rowsPerImage = height;
    Queue queue = device.getQueue();
    queue.writeTexture(destination, pixelData, width * height * 4, source, textureDesc.size);

    queue.release();

    stbi_image_free(pixelData);

    return texture;
}

wgpu::Texture TextureLoader::load_tilemap_as_texture(const path &path, wgpu::Device device, wgpu::TextureView *pTextureView) {
    using namespace wgpu;

    TilemapLoader::Tilemap tilemap = TilemapLoader::load_tilemap(path);
    int width = tilemap.width;
    int height = tilemap.height;

    TextureDescriptor textureDesc;
    textureDesc.dimension = TextureDimension::_2D;
    textureDesc.format = TextureFormat::R32Uint;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.size = { (unsigned int)width, (unsigned int)height, 1 };
    textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    Texture texture = device.createTexture(textureDesc);

    if (pTextureView) {
        TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect = TextureAspect::All;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
        textureViewDesc.dimension = TextureViewDimension::_2D;
        textureViewDesc.format = textureDesc.format;
        *pTextureView = texture.createView(textureViewDesc);
    }

    // Upload data to the GPU texture
    ImageCopyTexture destination;
    destination.texture = texture;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 };
    TextureDataLayout source;
    source.offset = 0;
    source.bytesPerRow = 4 * width;
    source.rowsPerImage = height;
    Queue queue = device.getQueue();
    queue.writeTexture(destination, tilemap.layer.data(), width * height * 4, source, textureDesc.size);

    queue.release();

    return texture;
}
