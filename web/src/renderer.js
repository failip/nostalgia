async function load_map() {
	const response = await fetch('resources/tilemaps/map.tmj');
	const map = await response.json();

	const number_of_layers = map.layers.length;
	const height = map.layers[0].height;
	const width = map.layers[0].width;

	const tile_data = new Uint32Array(height * width * number_of_layers);
	for (let i = 0; i < number_of_layers; i++) {
		const layer = map.layers[i];
		const data = layer.data;
		const layer_height = layer.height;
		const layer_width = layer.width;
		tile_data.set(data, i * layer_height * layer_width);
	}

	return {
		"data": tile_data,
		"height": height,
		"width": width,
		"number_of_layers": number_of_layers,
	}
}

async function initialize_webgpu() {
	const tilemap_shader = fetch('shader/tilemap.wgsl').then((response) => response.text());
	const sprite_shader = fetch('shader/sprite.wgsl').then((response) => response.text());
	const mix_shader = fetch('shader/mix.wgsl').then((response) => response.text());
	const tileset_texture_image = fetch('resources/textures/overworld.png').then((response) => response.blob());
	const sprite_texture_image = fetch('resources/textures/link.png').then((response) => response.blob());
	const map = load_map();
	const adapter = await navigator.gpu.requestAdapter();
	const device = await adapter.requestDevice();
	const canvas = document.getElementById('canvas');
	const context = canvas.getContext('webgpu');
	const device_pixel_ratio = window.devicePixelRatio;
	canvas.width = canvas.clientWidth * device_pixel_ratio;
	canvas.height = canvas.clientHeight * device_pixel_ratio;
	const presentation_format = navigator.gpu.getPreferredCanvasFormat();

	context.configure({
		device,
		format: presentation_format,
		alphaMode: 'premultiplied',
	});

	const tilemap_layout = device.createBindGroupLayout({
		entries: [
			{
				binding: 0,
				visibility: GPUShaderStage.FRAGMENT,
				buffer: {
					type: 'uniform',
				},
			},
			{
				binding: 1,
				visibility: GPUShaderStage.FRAGMENT,
				texture: {
					sampleType: 'float',
				},
			},
			{
				binding: 2,
				visibility: GPUShaderStage.FRAGMENT,
				texture: {
					sampleType: 'uint',
				},
			},
		],
	});

	const tilemap_pipeline_layout = device.createPipelineLayout({
		bindGroupLayouts: [tilemap_layout],
	});

	const tilemap_pipeline = device.createRenderPipeline({
		layout: tilemap_pipeline_layout,
		vertex: {
			module: device.createShaderModule({
				code: await tilemap_shader,
			}),
			entryPoint: 'vs_main',
		},
		fragment: {
			module: device.createShaderModule({
				code: await tilemap_shader,
			}),
			entryPoint: 'fs_main',
			targets: [
				{
					format: presentation_format,
				},
			],
		},
		primitive: {
			topology: 'triangle-list',
		},
	});

	const tileset_bitmap = await createImageBitmap(await tileset_texture_image);
	const tileset_texture = device.createTexture({
		size: {
			width: tileset_bitmap.width,
			height: tileset_bitmap.height,
		},
		format: 'rgba8unorm',
		usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST | GPUTextureUsage.RENDER_ATTACHMENT,
	});

	device.queue.copyExternalImageToTexture(
		{ source: tileset_bitmap },
		{ texture: tileset_texture },
		{ width: tileset_bitmap.width, height: tileset_bitmap.height },
	);

	const tilemap = new Uint32Array((await map).data);
	const tilemap_height = (await map).height;
	const tilemap_width = (await map).width;
	const number_of_layers = (await map).number_of_layers;

	const tilemap_texture = device.createTexture({
		size: {
			width: tilemap_width,
			height: tilemap_height * number_of_layers,
		},
		format: 'r32uint',
		usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.RENDER_ATTACHMENT,
	});

	device.queue.writeTexture(
		{
			texture: tilemap_texture,
		},
		tilemap,
		{
			bytesPerRow: tilemap_width * 4,
		},
		{
			width: tilemap_width,
			height: tilemap_height * number_of_layers,
		},
	);

	const tilemap_uniform_buffer = device.createBuffer({
		size: 12 * 4,
		usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
	});

	const screen_width = canvas.width;
	const screen_height = canvas.height;
	const tilemap_uniform_data = new Float32Array(
		[tilemap_width, tilemap_height, number_of_layers, 1.0,
			0.0, screen_width, screen_height, 1.0,
			0.0, 0.0, 1.0, 1.0]);

	const tilemap_bind_group = device.createBindGroup({
		layout: tilemap_pipeline.getBindGroupLayout(0),
		entries: [
			{
				binding: 0,
				resource: {
					buffer: tilemap_uniform_buffer,
				},
			},
			{
				binding: 1,
				resource: tileset_texture.createView(),
			},
			{
				binding: 2,
				resource: tilemap_texture.createView(),
			},
		],
	});

	device.queue.writeBuffer(tilemap_uniform_buffer, 0, tilemap_uniform_data.buffer, 0, tilemap_uniform_data.byteLength);

	const sprite_layout = device.createBindGroupLayout({
		entries: [
			{
				binding: 0,
				visibility: GPUShaderStage.FRAGMENT,
				texture: {
					sampleType: 'float',
				},
			},
			{
				binding: 1,
				visibility: GPUShaderStage.FRAGMENT,
				sampler: {
					type: 'filtering',
				},
			},
			{
				binding: 2,
				visibility: GPUShaderStage.FRAGMENT | GPUShaderStage.VERTEX,
				buffer: {
					type: 'uniform',
				},
			},
		],
	});

	const sprite_pipeline_layout = device.createPipelineLayout({
		bindGroupLayouts: [sprite_layout],
	});

	const sprite_pipeline = device.createRenderPipeline({
		layout: sprite_pipeline_layout,
		vertex: {
			module: device.createShaderModule({
				code: await sprite_shader,
			}),
			entryPoint: 'vs_main',
		},
		fragment: {
			module: device.createShaderModule({
				code: await sprite_shader,
			}),
			entryPoint: 'fs_main',
			targets: [
				{
					format: presentation_format,
				},
			],
		},
		primitive: {
			topology: 'triangle-list',
		},
	});

	const sprite_bitmap = await createImageBitmap(await sprite_texture_image);
	const sprite_texture = device.createTexture({
		size: {
			width: sprite_bitmap.width,
			height: sprite_bitmap.height,
		},
		format: 'rgba8unorm',
		usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST | GPUTextureUsage.RENDER_ATTACHMENT,
	});

	device.queue.copyExternalImageToTexture(
		{ source: sprite_bitmap },
		{ texture: sprite_texture },
		{ width: sprite_bitmap.width, height: sprite_bitmap.height },
	);

	const sprite_uniform_buffer = device.createBuffer({
		size: 8 * 4,
		usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
	});

	const sprite_uniform_data = new Float32Array([screen_width / 2.0, screen_height / 2.0, sprite_bitmap.width, sprite_bitmap.height, 0.0, 0.0, 0.0, 0.0]);

	const sprite_bind_group = device.createBindGroup({
		layout: sprite_pipeline.getBindGroupLayout(0),
		entries: [
			{
				binding: 0,
				resource: sprite_texture.createView(),
			},
			{
				binding: 1,
				resource: device.createSampler({
					magFilter: 'nearest',
					minFilter: 'nearest',
				}),
			},
			{
				binding: 2,
				resource: {
					buffer: sprite_uniform_buffer,
				},
			},
		],
	});

	function frame() {
		const command_encoder = device.createCommandEncoder();
		const texture_view = context.getCurrentTexture().createView();

		const tilemap_pass_descriptor = {
			colorAttachments: [
				{
					view: texture_view,
					clearValue: { r: 0.1, g: 0.1, b: 0.1, a: 1.0 },
					loadOp: 'clear',
					storeOp: 'store',
				},
			],
		};

		tilemap_uniform_data[4] = performance.now() / 1000.0;

		sprite_uniform_data[4] = (Math.sin(performance.now() / 5000.0) + 1.0) / 2.0 * ((screen_width) / 2.0 - sprite_bitmap.width);
		sprite_uniform_data[5] = (Math.cos(performance.now() / 5000.0) + 1.0) / 2.0 * ((screen_height / 2.0) - sprite_bitmap.height);

		device.queue.writeBuffer(tilemap_uniform_buffer, 0, tilemap_uniform_data.buffer, 0, tilemap_uniform_data.byteLength);
		device.queue.writeBuffer(sprite_uniform_buffer, 0, sprite_uniform_data.buffer, 0, sprite_uniform_data.byteLength);

		const pass_encoder = command_encoder.beginRenderPass(tilemap_pass_descriptor);
		pass_encoder.setPipeline(tilemap_pipeline);
		pass_encoder.setBindGroup(0, tilemap_bind_group);
		pass_encoder.draw(6);
		pass_encoder.setPipeline(sprite_pipeline);
		pass_encoder.setBindGroup(0, sprite_bind_group);
		pass_encoder.draw(6);
		pass_encoder.end();
		device.queue.submit([command_encoder.finish()]);
		requestAnimationFrame(frame);
	}

	requestAnimationFrame(frame);
};

initialize_webgpu();