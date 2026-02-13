#version 310 es
#extension GL_EXT_scalar_block_layout : require

precision highp float;
precision highp sampler2D;


// Input texture (8-bit sRGB)
uniform sampler2D u_inputTexture;

layout (std430, binding = 0) uniform SDRPARAMS
{
  // Color adjustment parameters
  float u_whitepoint_rel; // 1.0 - 10000nits
  float u_contrast;     // typically 0.0-2.0, 1.0 = no change
  float u_saturation;   // 0.0-2.0, 1.0 = no change
  float u_hue;          // 0.0-2π radians
  float u_brightness;   // typically -1.0 to 1.0, 0.0 = no change
  float u_gamma;        // gamma correction factor

  float pad1,pad2;
};

// Output to 16-bit floating point texture
layout(location = 0) out vec4 o_fragColor;

in vec2 v_texcoord;

// sRGB to linear conversion
vec3 sRGBToLinear(vec3 srgb) {
    return mix(
        srgb / 12.92,
        pow((srgb + 0.055) / 1.055, vec3(2.4)),
        step(vec3(0.04045), srgb)
    );
}

// Apply gamma correction
vec3 applyGamma(vec3 linearColor, float gamma) {
    return pow(linearColor, vec3(1.0 / gamma));
}

// Hue rotation matrix
mat3 hueRotationMatrix(float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    vec3 weights = vec3(0.299, 0.587, 0.114); // BT.709 luma weights
    
    return mat3(
        cosA + (1.0 - cosA) * weights.r,
        (1.0 - cosA) * weights.r - sinA * weights.g,
        (1.0 - cosA) * weights.r + sinA * weights.b,
        
        (1.0 - cosA) * weights.g + sinA * weights.r,
        cosA + (1.0 - cosA) * weights.g,
        (1.0 - cosA) * weights.g - sinA * weights.r,
        
        (1.0 - cosA) * weights.b - sinA * weights.r,
        (1.0 - cosA) * weights.b + sinA * weights.g,
        cosA + (1.0 - cosA) * weights.b
    );
}

// sRGB linear to BT.2020 linear conversion matrix
const mat3 sRGB_TO_BT2020 = mat3(
    0.627403,  0.329283,  0.043313,
    0.069097,  0.919540,  0.011363,
    0.016391,  0.088013,  0.895596
);

void main() {
    // Sample input texture (8-bit sRGB)
    vec3 inputColor = texture(u_inputTexture, v_texcoord).rgb;
    
    // 1. Apply gamma correction to input
    vec3 gammaCorrected = applyGamma(inputColor, u_gamma);
    
    // 2. Convert sRGB to linear
    vec3 linearColor = sRGBToLinear(gammaCorrected);
    
    // 3. Apply brightness
    linearColor += u_brightness;
    linearColor = max(linearColor, 0.0);
    
    // 4. Apply contrast
    float avgLuminance = dot(linearColor, vec3(0.2126, 0.7152, 0.0722));
    linearColor = mix(vec3(avgLuminance), linearColor, u_contrast);
    
    // 5. Apply saturation
    float luma = dot(linearColor, vec3(0.2126, 0.7152, 0.0722));
    linearColor = mix(vec3(luma), linearColor, u_saturation);
    
    // 6. Apply hue rotation
    mat3 hueMat = hueRotationMatrix(u_hue);
    linearColor = hueMat * linearColor;
    
    // 7. Convert to BT.2020 linear colorspace
    vec3 bt2020Color = sRGB_TO_BT2020 * linearColor;
    
    // Ensure non-negative values for 16FP output
    bt2020Color = max(bt2020Color * u_whitepoint_rel, 0.0);

    // Output to 16-bit floating point texture
    o_fragColor = vec4(bt2020Color, 1.0);
}