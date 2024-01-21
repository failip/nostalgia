struct VertexInput {
	@location(0) position: vec2f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(1) texcoord: vec2f,
};

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
	color: vec3f,
	time: f32,
};

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
@group(0) @binding(1) var uTexture: texture_2d<f32>;
@group(0) @binding(2) var uTilemap: texture_2d<u32>;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = vec4f(in.position.x, in.position.y, 0.0, 1.0);
	out.color = in.color;
	out.texcoord = in.position.xy * 0.5 + 0.5;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

	// Color by position
	let texture_coord = vec2u(in.position.xy) % vec2u(16, 16);
	let tilemap_coord = vec2i(in.position.xy) / vec2i(16, 16);

	let number_of_layers = u32(2);

	let texture_coord_one_dimensional = textureLoad(uTilemap, tilemap_coord, 0).r - 1;

	let width = u32(40);
	let height = u32(30);

	let texture_coord_two_dim = vec2u(texture_coord_one_dimensional % width, 
									  texture_coord_one_dimensional / width);

	let offset_texture_coord = texture_coord_two_dim * 16 + texture_coord;

	let color = textureLoad(uTexture, offset_texture_coord, 0).rgb;

	// Gamma-correction
	// let corrected_color = pow(color, vec3f(2.2));
	return vec4f(color, 1.0);
}