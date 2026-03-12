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
  float u_brightness;     // typically -1.0 to 1.0, 0.0 = no change . ok
  float u_gamma;         // gamma correction factor 1 - no change, goes from 0.1 - 3

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

// https://www.shadertoy.com/view/XdcXzn

mat4 brightnessMatrix( float brightness )
{
    return mat4( 1, 0, 0, 0,
                 0, 1, 0, 0,
                 0, 0, 1, 0,
                 brightness, brightness, brightness, 1 );
}

mat4 contrastMatrix( float contrast )
{
        float t = ( 1.0 - contrast ) / 2.0;

    return mat4( contrast, 0, 0, 0,
                 0, contrast, 0, 0,
                 0, 0, contrast, 0,
                 t, t, t, 1 );

}

mat4 saturationMatrix( float saturation )
{
    vec3 luminance = vec3( 0.3086, 0.6094, 0.0820 );

    float oneMinusSat = 1.0 - saturation;

    vec3 red = vec3( luminance.x * oneMinusSat );
    red+= vec3( saturation, 0, 0 );

    vec3 green = vec3( luminance.y * oneMinusSat );
    green += vec3( 0, saturation, 0 );

    vec3 blue = vec3( luminance.z * oneMinusSat );
    blue += vec3( 0, 0, saturation );

    return mat4( red,     0,
                 green,   0,
                 blue,    0,
                 0, 0, 0, 1 );
}


// https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786

vec3 hueShift( vec3 color, float hueAdjust ){

    const vec3  kRGBToYPrime = vec3 (0.299, 0.587, 0.114);
    const vec3  kRGBToI      = vec3 (0.596, -0.275, -0.321);
    const vec3  kRGBToQ      = vec3 (0.212, -0.523, 0.311);

    const vec3  kYIQToR     = vec3 (1.0, 0.956, 0.621);
    const vec3  kYIQToG     = vec3 (1.0, -0.272, -0.647);
    const vec3  kYIQToB     = vec3 (1.0, -1.107, 1.704);

    float   YPrime  = dot (color, kRGBToYPrime);
    float   I       = dot (color, kRGBToI);
    float   Q       = dot (color, kRGBToQ);
    float   hue     = atan (Q, I);
    float   chroma  = sqrt (I * I + Q * Q);

    hue += hueAdjust;

    Q = chroma * sin (hue);
    I = chroma * cos (hue);

    vec3    yIQ   = vec3 (YPrime, I, Q);

    return vec3( dot (yIQ, kYIQToR), dot (yIQ, kYIQToG), dot (yIQ, kYIQToB) );

}
vec3 rec709_to_bt2020_linear(vec3 inputColor){

  if (u_hue != 0.0){
    inputColor = hueShift(inputColor, u_hue);
  }

  vec4 i4c = vec4(inputColor,1.0);

  inputColor =    (brightnessMatrix( u_brightness ) *
                  contrastMatrix( u_contrast ) *
                  saturationMatrix( u_saturation ) *
                  i4c).rgb;


  inputColor = clamp(inputColor,0,1);

  inputColor = pow(inputColor, vec3(1.0/u_gamma));


  vec3 linearColor = vec3(linearize_rec709(inputColor.r),linearize_rec709(inputColor.g),linearize_rec709(inputColor.b));


  vec3 bt2020Color = linearColor * sRGB_TO_BT2020;

  return bt2020Color * u_whitepoint_ref;

}
