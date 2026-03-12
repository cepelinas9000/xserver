#version 310 es
#extension GL_NV_uniform_buffer_std430_layout: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_io_blocks : enable

precision highp float;
precision highp int;

out vec4 frag_color;
uniform sampler2D sampler;

in vec2 fill_pos;

// Color space conversion matrix (BT2020 to target) 
layout (std430, binding = 3) uniform HDRCRTPARAMS
{
    mat3 u_colorMatrix;

};

// Inverse PQ constants
const float PQ_M1 = 0.1593017578125;      // 2610.0 / 16384.0
const float PQ_M2 = 78.84375;             // 2523.0 / 4096.0 * 128.0
const float PQ_C1 = 0.8359375;            // 3424.0 / 4096.0
const float PQ_C2 = 18.8515625;           // 2413.0 / 4096.0 * 32.0
const float PQ_C3 = 18.6875;              // 2392.0 / 4096.0 * 32.0

// Normalize from 0-10000 nits to 0-1 range
vec3 normalizeFromNits(vec3 linear) {
    return linear / 10000.0;
}

// BT2020 PQ OETF (Perceptual Quantizer)
float linearToPQ(float normalizedLinear) {
    if (normalizedLinear <= 0.0) return 0.0;
    
    float y = pow(normalizedLinear, PQ_M1);
    float num = PQ_C1 + PQ_C2 * y;
    float den = 1.0 + PQ_C3 * y;
    float pq = pow(num / den, PQ_M2);

    return pq;
}

void main() {
    // Sample input (BT2020 linear, 0-10000 nits)
    vec3 bt2020Linear = texture(sampler, fill_pos).rgb;

    // Apply color space conversion (BT2020 to target)
    vec3 targetLinear = u_colorMatrix * bt2020Linear;
    
    // Normalize to 0-1 range
    vec3 normalized = normalizeFromNits(targetLinear);
    
    // Apply PQ transfer function to each channel
    vec3 pq = vec3(
        linearToPQ(normalized.r),
        linearToPQ(normalized.g),
        linearToPQ(normalized.b)
    );
    
    // Output PQ-encoded values
    frag_color = vec4(pq, 1.0);
}
