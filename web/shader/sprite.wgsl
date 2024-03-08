@group(0) @binding(0) var uSprite: texture_2d<f32>;
@group(0) @binding(1) var uSpriteSampler: sampler;
@group(0) @binding(2) var<uniform> uUniforms: Uniforms;

struct VertexOutput {
	@builtin(position) position: vec4f,
    @location(0) uv: vec2<f32>
};

struct Uniforms {
    screen_width: f32,
    screen_height: f32,
    sprite_width: f32,
    sprite_height: f32,
    position_x: f32,
    position_y: f32,
    pad_0: f32,
    pad_1: f32
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

	let pos = array<vec2<f32>, 6>(
        vec2<f32>(  -1.0 + (uUniforms.position_x + uUniforms.sprite_width) * 2.0 / uUniforms.screen_width, 
                    1.0 - (uUniforms.position_y + uUniforms.sprite_height) * 2.0 / uUniforms.screen_height),
        vec2<f32>(  -1.0 + (uUniforms.position_x) * 2.0 / uUniforms.screen_width, 
                    1.0 - (uUniforms.position_y + uUniforms.sprite_height) * 2.0 / uUniforms.screen_height),
        vec2<f32>(  -1.0 + (uUniforms.position_x + uUniforms.sprite_width) * 2.0 / uUniforms.screen_width, 
                    1.0 - (uUniforms.position_y) * 2.0 / uUniforms.screen_height),
        vec2<f32>(  -1.0 + (uUniforms.position_x + uUniforms.sprite_width) * 2.0 / uUniforms.screen_width, 
                    1.0 - (uUniforms.position_y) * 2.0 / uUniforms.screen_height),
        vec2<f32>(  -1.0 + (uUniforms.position_x) * 2.0 / uUniforms.screen_width, 
                    1.0 - (uUniforms.position_y + uUniforms.sprite_height) * 2.0 / uUniforms.screen_height),
        vec2<f32>(  -1.0 + (uUniforms.position_x) * 2.0 / uUniforms.screen_width, 
                    1.0 - (uUniforms.position_y) * 2.0 / uUniforms.screen_height)
	);

    var out: VertexOutput;
    var position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);
    out.position = position;
    out.uv = uv[VertexIndex];
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(uSprite, uSpriteSampler, in.uv);
    if color.a == 0.0 {
        discard;
    };
    return color;
}