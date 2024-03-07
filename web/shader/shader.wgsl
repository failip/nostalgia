struct Uniforms {
	tilemap_width: f32,
	tilemap_height: f32,
	tilemap_number_of_layers: f32,
	scaling: f32,
	time: f32,
	screen_width: f32,
	screen_height: f32,
	pad__: f32,
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
	@location(0) color: vec3f,
	@location(1) texcoord: vec2f,
};

@vertex
fn vs_main(
  @builtin(vertex_index) VertexIndex : u32
) -> VertexOutput {
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
	out.texcoord = position.xy * 0.5 + 0.5;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
  	let number_of_layers = i32(uUniforms.tilemap_number_of_layers);
  	let width = u32(uUniforms.tilemap_width);
	let height = u32(uUniforms.tilemap_height);
	let scaling = uUniforms.scaling;

	var color = vec3f(0.0, 0.0, 0.0);

	if (in.position.x > uUniforms.tilemap_width * 16.0 * scaling) {
		return vec4f(0.0, 0.0, 0.0, 1.0);
	}

	if (in.position.y > uUniforms.tilemap_height * 16.0 * scaling) {
		return vec4f(0.0, 0.0, 0.0, 1.0);
	}

	for (var i = 0; i < number_of_layers; i++) {
		let layer_offset = vec2i(0, i * 30);

		// Color by position
		let texture_coord = vec2u(in.position.xy / scaling) % vec2u(16, 16);
		let tilemap_coord = vec2i(in.position.xy / scaling) / vec2i(16, 16) + layer_offset;
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
  
  return vec4f(color, 1.0);
}