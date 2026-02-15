#ifndef __HDR_EXT_PRIV__
#define __HDR_EXT_PRIV__

#include <stdint.h>


#include "screenint.h"

#include "hdrext.h"


#include "glamor_priv.h"

extern DevPrivateKeyRec HdrScrPrivateKeyRec;
#define HdrScrPrivateKey (&HdrScrPrivateKeyRec)

extern DevPrivateKeyRec HdrPixPrivateKeyRec;
#define HdrPixPrivateKey (&HdrPixPrivateKeyRec)

extern DevPrivateKeyRec HdrWinPrivateKeyRec;
#define HdrWinPrivateKey (&HdrWinPrivateKeyRec)


extern Atom HDR__X11HDR_SDR_PARAMS_atom;

/**
 * @brief HDR_impl_tf internal represantation
 */
typedef enum {
    HDR_impl_tf_linear = 0,
    HDR_impl_tf_pq     = 1,
    HDR_impl_tf_gamma  = 2,
    HDR_impl_passthough = 3,
} HDR_impl_tf;

typedef enum {
    HDR_colorspace_sRGB = 0,
    HDR_colorspace_BT2020,
    HDR_colorspace_custom,
} HDR_colorspace;

typedef enum {
    HDR_pixmap_SDR_OR_PASSTHROUGH = 0, /* is default state */
    HDR_pixmap_HDR, /* it is marker for known pixmap */
    HDR_pixmap_crt, /* this is CRT pixmap - last pixmap on pipeline */
    HDR_pixmap_intermiate, /* intermiadate (screen buffer ) pixmap */

} HDR_pixmap_purpose;

/**
 * @brief HDR_conversion_e describes which type conversion is necessary
 */
typedef enum {
    HDR_conversion_no_need,
    HDR_conversion_SDR_to_Intermiate
} HDR_conversion_e;
/**
 * @warning: keep same structure as in shader 
 **/
typedef struct {
    float u_whitepoint_ref; // this is relative from 0-1 (1 == 10 000 nits)
    float u_contrast;       // typically 0.0-2.0, 1.0 = no change
    float u_saturation;     // 0.0-2.0, 1.0 = no change
    float u_hue;            // 0.0-2π radians
    float u_brightness;     // typically -1.0 to 1.0, 0.0 = no change
    float u_gamma;          // gamma correction factor
    
    float pad1,pad2;

} HDR_SDRPARAMS_uniform_t;

typedef struct {
    HDR_SDRPARAMS_uniform_t sdr_params;
    GLuint sdr_params_u;

} HDRExtWindowPrivate;

typedef struct {

    HDR_pixmap_purpose purpose;
    int crt_num;

    HDR_impl_tf tf;
    HDR_colorspace colorspace;
    hdr_color_attributes colorspace_points;

} HDRPixmapPrivate;


static inline void HDR_SDRPARAMS_uniform_t_init(HDR_SDRPARAMS_uniform_t *s){
    s->u_whitepoint_ref = 100;
    s->u_contrast = 1.0f;
    s->u_saturation = 1.0f;
    s->u_hue = 1.0f;
    s->u_brightness = 1.0f;
    s->u_gamma = 1.0f;
}

/**
 * @brief HDR_SDRPARAMS_uniform_t_str return string from structure
 * @param s
 * @return
 */
static inline char* HDR_SDRPARAMS_uniform_t_str(HDR_SDRPARAMS_uniform_t *s){
    int len = snprintf(NULL,0,"%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
                s->u_whitepoint_ref, s->u_contrast, s->u_saturation,
                s->u_hue, s->u_brightness, s->u_gamma);

    char *str= (char *)malloc(len + 1);

    snprintf(str,len+1,"%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
             s->u_whitepoint_ref, s->u_contrast, s->u_saturation,
             s->u_hue, s->u_brightness, s->u_gamma);
    return str;
}

typedef struct {

    HDR_SDRPARAMS_uniform_t default_sdr_params;
    GLuint default_sdr_params_ubo;

    GLuint SDR8bit_to_BT2020_linear;
    GLuint BT2020_linear_to_crt_PQ;

    glamor_program SDR8bit_to_BT2020_linear_glamor;
    glamor_program BT2020_linear_to_crt_PQ_glamor;


    GLuint CRT_color_matrices[16];



} HDRScreenPrivateData;

/**
 * @brief InitializeSDRShaders
 * Compile necessary shaders for screen
 * @param pScreen
 */
void InitializeScreenShaders(ScreenPtr pScreen);

void FreeHDRScreenPrivateData(HDRScreenPrivateData *priv);

/**
 * @brief HDR_copy_find_hdr_glamor_program modifies prog variable for hdr
 * @param screen
 * @param glamor_priv
 * @param src
 * @param src_backing
 * @param dst
 * @param dst_backing
 * @param prog
 */
void HDR_copy_find_hdr_glamor_program(ScreenPtr screen,glamor_screen_private *glamor_priv, DrawablePtr src,PixmapPtr src_backing,DrawablePtr dst,PixmapPtr dst_backing,glamor_program **prog);


/**
 * @brief HDR_copy_find_hdr_glamor_program_set_parameters
 * @param screen
 * @param glamor_priv
 * @param src
 * @param src_backing
 * @param dst
 * @param dst_backing
 */
void HDR_copy_find_hdr_glamor_program_set_parameters(ScreenPtr screen,glamor_screen_private *glamor_priv, DrawablePtr src,PixmapPtr src_backing,DrawablePtr dst,PixmapPtr dst_backing,glamor_program *prog);

void dump_uniforms(GLuint program);

/**
 * @brief HDR_parseSDRParams_parsePropertyStr parse SDR params property items
 * it parses strings like "200.0,1.0,1.0,0.0,0.0,0.0" with aditional rules:
 *   if field have "default" - takes value from default
 *   if field have "*" - leave value untoched
 * @param input
 * @param defaults
 * @param inout inout because it can reuse previous values
 * @return
 */
bool HDR_parseSDRParams_parsePropertyStr(const char *input, HDR_SDRPARAMS_uniform_t *defaults, HDR_SDRPARAMS_uniform_t *inout);



void hdr_WindowUpdateSDRParams(ScreenPtr screen, WindowPtr pwindow, HDR_SDRPARAMS_uniform_t *sdrparams);


/**
 * @brief hdr_pick_conversion check what conversion is necessary
 * @param src
 * @param dst
 * @return
 */
HDR_conversion_e hdr_pick_conversion(DrawablePtr src,DrawablePtr dst);

/**
 * @brief hdr_setglprogram_params set opengl parameters
 * it calls glUniformBlockBinding, glBindBufferBase and etc to set necessary shader parameters
 * @param pScreen
 * @param src
 * @param dst
 * @param conv
 */
void hdr_setglprogram_params(ScreenPtr pScreen, glamor_program *prog, DrawablePtr src, DrawablePtr dst, HDR_conversion_e conv);

#endif
