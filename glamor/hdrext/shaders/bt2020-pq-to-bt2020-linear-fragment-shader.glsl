#version 310 es
#extension GL_NV_uniform_buffer_std430_layout: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_io_blocks : enable

precision highp float;
precision highp int;

out vec4 frag_color;
uniform sampler2D sampler;

in vec2 fill_pos;

// Inverse PQ constants
const float PQ_M1 = 0.1593017578125;
const float PQ_M2 = 78.84375;
const float PQ_C1 = 0.8359375;
const float PQ_C2 = 18.8515625;
const float PQ_C3 = 18.6875;

float pq_to_linear_bt2020(float pq){
    float num   = max(pow(pq, 1.0 / PQ_M2) - PQ_C1,0.0);
    float denum = (PQ_C2 - PQ_C3* pow(pq,1.0/PQ_M2) );
    float linear_nits = pow(num / denum, 1.0/PQ_M1);
    return 10000.0*linear_nits;
}

/* bt2020 to linear non-normalized */
void main() {
    // bt2020 pq from [0..1]
    vec4 bt2020pq = texture(sampler, fill_pos).rgba;
    // Output PQ-encoded values
    frag_color = vec4(pq_to_linear_bt2020(bt2020pq.r),pq_to_linear_bt2020(bt2020pq.g),pq_to_linear_bt2020(bt2020pq.b), bt2020pq.a);
}

