#extension GL_NV_uniform_buffer_std430_layout: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_io_blocks : enable

precision highp float;

layout (std430, binding = 3) uniform SDRPARAMS
{
  // Color adjustment parameters,  keep in sync with C structure
  float u_whitepoint_ref; // 1.0 - 10000nits
  float u_contrast;           // typically 0.0-2.0, 1.0 = no change
  float u_saturation;       // 0.0-2.0, 1.0 = no change
  float u_hue;                 // 0.0-2π radians
  float u_brightness;     // typically -1.0 to 1.0, 0.0 = no change
  float u_gamma;         // gamma correction factor

  float pad1,pad2;
};


// sRGB linear to BT.2020 linear conversion matrix
const mat3 sRGB_TO_BT2020 = mat3(
    0.627403,  0.329283,  0.043313,
    0.069097,  0.919540,  0.011363,
    0.016391,  0.088013,  0.895596
);

float linearize_rec709(float color){

    if (color <  0.081){
       return color / 4.5;
    } else {
        return pow((color + 0.099) / 1.099,(1.0/0.45));
    }
}


vec3 rec709_to_bt2020_linear(vec3 inputColor){
     vec3 linearColor = vec3(linearize_rec709(inputColor.r),linearize_rec709(inputColor.g),linearize_rec709(inputColor.b));
     vec3 bt2020Color = linearColor *  sRGB_TO_BT2020;

     return bt2020Color * u_whitepoint_ref;

}
