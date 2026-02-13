#version 310 es
#extension GL_EXT_scalar_block_layout : require

precision highp float;
precision highp int;

// Input texture (BT2020 linear, 0-10000 nits)
uniform sampler2D u_inputTexture;

// Color space conversion matrix (BT2020 to target) 
layout (std430, binding = 0) uniform HDRCRTPARAMS
{
    mat3 u_colorMatrix;
    
    float pad1,pad2,pad3;
};
// Inverse PQ constants
const float PQ_M1 = 0.1593017578125;      // 2610.0 / 16384.0
const float PQ_M2 = 78.84375;             // 2523.0 / 4096.0 * 128.0
const float PQ_C1 = 0.8359375;            // 3424.0 / 4096.0
const float PQ_C2 = 18.8515625;           // 2413.0 / 4096.0 * 32.0
const float PQ_C3 = 18.6875;              // 2392.0 / 4096.0 * 32.0

// Output to 16-bit floating point texture
layout(location = 0) out vec4 o_fragColor;

in vec2 v_texcoord;


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
    vec3 bt2020Linear = texture(u_inputTexture, v_texcoord).rgb;
    
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
    o_fragColor = vec4(pq, 1.0);
}

