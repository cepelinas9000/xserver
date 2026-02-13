#include <dix-config.h>


#include "hdrext_priv.h"

#include "glamor_priv.h"

#include "bt2020-linear-to-crt-pq-fragment-shader-fp16.glsl.h"
#include "bt2020-linear-to-crt-pq-vertex-shader-fp16.glsl.h"
#include "sdr-fragment-shader-fp16.glsl.h"
#include "sdr-vertex-shader-fp16.glsl.h"


void FreeHDRScreenPrivateData(HDRScreenPrivateData *priv){

  glDeleteBuffers(1,&priv->default_sdr_params_ubo);
  glDeleteProgram(priv->SDR8bit_to_BT2020_linear);
  free(priv);

}
void InitializeScreenShaders(ScreenPtr pScreen){


  glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
  glamor_make_current(glamor_priv);


  HDRScreenPrivateData *priv = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);
  if (priv){
      FreeHDRScreenPrivateData(priv);
  }


  priv = calloc(1,sizeof(HDRScreenPrivateData));

  HDR_SDRPARAMS_uniform_t_init(&priv->default_sdr_params);

  glGenBuffers(1, &priv->default_sdr_params_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, priv->default_sdr_params_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(priv->default_sdr_params), &priv->default_sdr_params, GL_STATIC_DRAW);

  /* SDR to BT2020 linear */

  {
    GLuint fs = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER,(char *)SDR_FRAGMENT_SHADER_FP16_GLSL);
    GLuint vs= glamor_compile_glsl_prog(GL_VERTEX_SHADER,(char *)SDR_VERTEX_SHADER_FP16_GLSL);

    priv->SDR8bit_to_BT2020_linear = glCreateProgram();
    glAttachShader(priv->SDR8bit_to_BT2020_linear, vs);
    glAttachShader(priv->SDR8bit_to_BT2020_linear, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glamor_link_glsl_prog(pScreen,priv->SDR8bit_to_BT2020_linear,"SDR8bit_to_BT2020_linear");
    priv->SDR8bit_to_BT2020_linear_SDRPARAMS  = glGetUniformLocation(priv->SDR8bit_to_BT2020_linear,"SDRPARAMS");

  }

  /* BT2020 linear to colorspace with PQ transfer */
  {
    GLuint fs = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER,(char *)BT2020_LINEAR_TO_CRT_PQ_FRAGMENT_SHADER_FP16_GLSL);
    GLuint vs= glamor_compile_glsl_prog(GL_VERTEX_SHADER,(char *)BT2020_LINEAR_TO_CRT_PQ_VERTEX_SHADER_FP16_GLSL);

    priv->BT2020_linear_to_crt_PQ = glCreateProgram();
    glAttachShader(priv->BT2020_linear_to_crt_PQ, vs);
    glAttachShader(priv->BT2020_linear_to_crt_PQ, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glamor_link_glsl_prog(pScreen,priv->BT2020_linear_to_crt_PQ,"BT2020_linear_to_crt_PQ");
    priv->BT2020_linear_to_crt_PQ_HDRCRTPARAMS = glGetUniformLocation(priv->SDR8bit_to_BT2020_linear,"HDRCRTPARAMS");

  }

}

