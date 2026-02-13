#ifndef __HDR_EXT_PRIV__
#define __HDR_EXT_PRIV__

#include <stdint.h>


#include "screenint.h"


#include "glamor_priv.h"

extern DevPrivateKeyRec HdrScrPrivateKeyRec;
#define HdrScrPrivateKey (&HdrScrPrivateKeyRec)

extern DevPrivateKeyRec HdrPixPrivateKeyRec;
#define HdrPixPrivateKey (&HdrPixPrivateKeyRec)

extern DevPrivateKeyRec HdrWinPrivateKeyRec;
#define HdrWinPrivateKey (&HdrWinPrivateKeyRec)


/**
 * @brief HDR_impl_tf internal represantation
 */
typedef enum {
    HDR_impl_tf_linear = 0,
    HDR_impl_tf_pq     = 1,
    HDR_impl_tf_gamma  = 2,
    HDR_impl_passthough = 3,
} HDR_impl_tf;


/**
 * @warning: keep same structure as in shader 
 **/
typedef struct {
    float u_whitepoint_rel; // this is relative from 0-1 (1 == 10 000 nits)
    float u_contrast;       // typically 0.0-2.0, 1.0 = no change
    float u_saturation;     // 0.0-2.0, 1.0 = no change
    float u_hue;            // 0.0-2π radians
    float u_brightness;     // typically -1.0 to 1.0, 0.0 = no change
    float u_gamma;          // gamma correction factor
    
    float pad1,pad2

} HDR_SDRPARAMS_uniform_t;

typedef struct {

    float   tune_paramsf[4];
    int32_t tune_paramsi[4];


} HDRExtWindowPrivate;


static inline void HDR_SDRPARAMS_uniform_t_init(HDR_SDRPARAMS_uniform_t *s){
    s->u_whitepoint_rel = 0.02f; // 200 nits
    s->u_contrast = 1.0f;
    s->u_saturation = 1.0f;
    s->u_hue = 1.0f;
    s->u_brightness = 1.0f;
    s->u_gamma = 1.0f;
}

typedef struct {

    HDR_SDRPARAMS_uniform_t default_sdr_params;
    GLuint default_sdr_params_ubo;

    GLuint SDR8bit_to_BT2020_linear;
    GLuint BT2020_linear_to_crt_PQ;

    GLuint SDR8bit_to_BT2020_linear_SDRPARAMS;
    GLuint BT2020_linear_to_crt_PQ_HDRCRTPARAMS;

    GLuint CRT_color_matrices[16];


} HDRScreenPrivateData;

/**
 * @brief InitializeSDRShaders
 * Compile necessary shaders for screen
 * @param pScreen
 */
void InitializeScreenShaders(ScreenPtr pScreen);

void FreeHDRScreenPrivateData(HDRScreenPrivateData *priv);


#endif
