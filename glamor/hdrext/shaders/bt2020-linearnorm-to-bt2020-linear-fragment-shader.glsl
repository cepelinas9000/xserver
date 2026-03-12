#version 310 es
#extension GL_NV_uniform_buffer_std430_layout: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_io_blocks : enable

precision highp float;
precision highp int;

out vec4 frag_color;
uniform sampler2D sampler;

in vec2 fill_pos;

/* bt2020 to linear normalized to non-normalized */
void main() {	
    vec4 bt2020pq = texture(sampler, fill_pos).rgba;
    frag_color = vec4(bt2020pq.rgb * 10000.0, bt2020pq.a);
}

