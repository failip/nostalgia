struct Uniforms {
	tilemap_width: f32,
	tilemap_height: f32,
	tilemap_number_of_layers: f32,
	scaling: f32,
	time: f32,
	screen_width: f32,
	screen_height: f32,
	pad_: f32,
	offset_x: f32,
	offset_y: f32,
	pad__: f32,
	pad___: f32
};

@group(0) @binding(0) var<uniform> uUniforms: Uniforms;
@group(0) @binding(1) var uTileset: texture_2d<f32>;
@group(0) @binding(2) var uTilemap: texture_2d<u32>;

struct VertexInput {
	@location(0) position: vec2f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) uv: vec2f,
};

@vertex
fn vs_main(
  @builtin(vertex_index) VertexIndex : u32
) -> VertexOutput {
	const uv = array<vec2<f32>, 6>(
        vec2<f32>(1.0, 1.0),
        vec2<f32>(0.0, 1.0),
        vec2<f32>(1.0, 0.0),
        vec2<f32>(1.0, 0.0),
        vec2<f32>(0.0, 1.0),
        vec2<f32>(0.0, 0.0)
    );

	var pos = array<vec2<f32>, 6>(
		vec2<f32>(-1.0, -1.0),
		vec2<f32>(1.0, -1.0),
		vec2<f32>(-1.0, 1.0),
		vec2<f32>(-1.0, 1.0),
		vec2<f32>(1.0, -1.0),
		vec2<f32>(1.0, 1.0)
	);

	var out: VertexOutput;
	var position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);
	out.position = position * 0.5;
	out.uv = uv[VertexIndex];
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
  	let number_of_layers = i32(uUniforms.tilemap_number_of_layers);
  	let width = u32(uUniforms.tilemap_width);
	let height = u32(uUniforms.tilemap_height);
	let scaling = uUniforms.scaling;
	let offset = vec2(uUniforms.offset_x, uUniforms.offset_y);

	var color = vec3f(0.0, 0.0, 0.0);

	// if (in.position.x > uUniforms.tilemap_width * 16.0) {
	// 	return vec4f(0.0, 0.0, 0.0, 1.0);
	// }

	// if (in.position.y > uUniforms.tilemap_height * 16.0) {
	// 	return vec4f(0.0, 0.0, 0.0, 1.0);
	// }

	let inverted_uv = vec2(1.0 - in.uv.x, in.uv.y);
	let pixel_position = in.position.xy / 2.0;

	for (var i = 0; i < number_of_layers; i++) {
		let layer_offset = vec2i(0, i * 30);

		// Color by position
		let texture_coord = vec2u((pixel_position + offset)) % vec2u(16, 16);
		let tilemap_coord = vec2i((pixel_position + offset)) / vec2i(16, 16) + layer_offset;
		let tilemap_data = textureLoad(uTilemap, tilemap_coord, 0).r;
		
		if (tilemap_data == u32(0)) {
			continue;
		}

		let texture_coord_one_dimensional = tilemap_data - 1;

		let texture_coord_two_dim = vec2u(texture_coord_one_dimensional % width, 
										texture_coord_one_dimensional / width);

		let offset_texture_coord = texture_coord_two_dim * 16 + texture_coord;

		let texture_color = textureLoad(uTileset, offset_texture_coord, 0);
		color = mix(color, texture_color.rgb, texture_color.a);
	}
  
  return vec4<f32>(color, 1.0);
//   return vec4<f32>(0.2, 0.2, 0.2, 1.0);	
}