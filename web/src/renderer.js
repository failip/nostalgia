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
	const shader = fetch('shader/shader.wgsl').then((response) => response.text());
	const texture = fetch('resources/textures/overworld.png').then((response) => response.blob());
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

	const layout = device.createBindGroupLayout({
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

	const pipeline_layout = device.createPipelineLayout({
		bindGroupLayouts: [layout],
	});

	const pipeline = device.createRenderPipeline({
		layout: pipeline_layout,
		vertex: {
			module: device.createShaderModule({
				code: await shader,
			}),
			entryPoint: 'vs_main',
		},
		fragment: {
			module: device.createShaderModule({
				code: await shader,
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

	const tileset_bitmap = await createImageBitmap(await texture);
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

	const uniform_buffer = device.createBuffer({
		size: 8 * 4,
		usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
	});

	const screen_width = canvas.width;
	const screen_height = canvas.height;
	const uniform_data = new Float32Array([tilemap_width, tilemap_height, number_of_layers, window.devicePixelRatio, 0.0, screen_width, screen_height, 1.0]);

	const bind_group = device.createBindGroup({
		layout: pipeline.getBindGroupLayout(0),
		entries: [
			{
				binding: 0,
				resource: {
					buffer: uniform_buffer,
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

	device.queue.writeBuffer(uniform_buffer, 0, uniform_data.buffer, 0, uniform_data.byteLength);

	function frame() {
		const command_encoder = device.createCommandEncoder();
		const texture_view = context.getCurrentTexture().createView();

		const render_pass_descriptor = {
			colorAttachments: [
				{
					view: texture_view,
					clearValue: { r: 0.1, g: 0.1, b: 0.1, a: 1.0 },
					loadOp: 'clear',
					storeOp: 'store',
				},
			],
		};

		const pass_encoder = command_encoder.beginRenderPass(render_pass_descriptor);
		pass_encoder.setPipeline(pipeline);
		pass_encoder.setBindGroup(0, bind_group);
		pass_encoder.draw(6);
		pass_encoder.end();

		device.queue.submit([command_encoder.finish()]);
		requestAnimationFrame(frame);
	}

	requestAnimationFrame(frame);
};

initialize_webgpu();