#include "dix-config.h"

#include "hdrext_priv.h"

#include "hdrext_utils.h"

#include "cglm/mat3.h"


#include "os/fmt.h"


static const float BT2020_PRIMARIES[4][2] = {
    {0.708f, 0.292f}, /* red */
    {0.170f, 0.797f}, /* green */
    {0.131, 0.046f}, /* blue */
    {0.3127f, 0.3290f} /* white point*/
};



static inline void xyY_to_XYZ(const float x,const  float y,const  float Y,  float  XYZ[3]){

  if(y == 0){
    XYZ[0] = XYZ[1] = XYZ[2] = 0;
  }

  XYZ[0] = (x*Y) / y;
  XYZ[1] = Y;
  XYZ[2] = ((1 - x - y)*XYZ[1]) / y;
}

static void get_rgb_to_xyz_matrix(const float redXY[2],const float greenXY[2],const float blueXY[2],const float whitepointXY[2], mat3 XYZ_out){

  float XYZ_colors[4][3];

  xyY_to_XYZ(redXY[0],redXY[1],1,XYZ_colors[0]);
  xyY_to_XYZ(greenXY[0],greenXY[1],1,XYZ_colors[1]);
  xyY_to_XYZ(blueXY[0],blueXY[1],1,XYZ_colors[2]);

  xyY_to_XYZ(whitepointXY[0],whitepointXY[1],1,XYZ_colors[3]);


  mat3 P;

  memcpy(&P[0], &XYZ_colors[0],sizeof(float)*3);
  memcpy(&P[1], &XYZ_colors[1],sizeof(float)*3);
  memcpy(&P[2], &XYZ_colors[2],sizeof(float)*3);


  mat3 P_inv;
  glm_mat3_inv(P,P_inv);

  vec3 S;

  glm_mat3_mulv(P_inv,XYZ_colors[3],S);


  mat3 S_diag =GLM_MAT3_ZERO_INIT;
  S_diag[0][0] = S[0];
  S_diag[1][1] = S[1];
  S_diag[2][2] = S[2];


  glm_mat3_mul(P,S_diag,XYZ_out);
}


void HDRutil_BT2020_matrix_colorspace(hdr_color_attributes *target_colorspace, float conv_matrix[3][3])
{

  mat3 src_to_xyz,dst_to_xyz,dst_to_xyzInv;

  get_rgb_to_xyz_matrix(BT2020_PRIMARIES[0],BT2020_PRIMARIES[1],BT2020_PRIMARIES[2],BT2020_PRIMARIES[3],src_to_xyz);
  get_rgb_to_xyz_matrix(target_colorspace->color_r,target_colorspace->color_g,target_colorspace->color_b,target_colorspace->white_point,dst_to_xyz);


  mat3 m3conv;

  glm_mat3_inv(dst_to_xyz,dst_to_xyzInv);

  glm_mat3_mul(dst_to_xyzInv,src_to_xyz,m3conv);

  memcpy(conv_matrix[0],m3conv[0],sizeof(float) * 3);
  memcpy(conv_matrix[1],m3conv[1],sizeof(float) * 3);
  memcpy(conv_matrix[2],m3conv[2],sizeof(float) * 3);


}

static inline void
_hdr_string_update_param_from_default(HDR_SDRPARAMS_uniform_t *u,int number,HDR_SDRPARAMS_uniform_t *d){
    switch (number) {
        case 0:
            u->u_whitepoint_ref  = d->u_whitepoint_ref;
        break;
        case 1:
            u->u_contrast  = d->u_contrast;
        break;
        case 2:
            u->u_saturation  = d->u_saturation;
        break;
        case 3:
            u->u_hue  = d->u_hue;
        break;
        case 4:
            u->u_brightness  = d->u_brightness;
        break;

        case 5:
            u->u_gamma = d->u_gamma;
        break;
    default:
        break;
    }

}
static inline bool
_hdr_string_update_param(HDR_SDRPARAMS_uniform_t *u,int number,float val){

    switch (number) {
        case 0: /* XXX: maybe limit by display ? */
            if (val < 0 || val > 10000){
              return false;
            }
            u->u_whitepoint_ref  = val;
        break;
        case 1:
            if (val < 0 || val > 2){
                return false;
            }
            u->u_contrast  = val;
        break;
        case 2:
            if (val < 0 || val > 2){
                return false;
            }
            u->u_saturation  = val;
        break;
        case 3:
            if (val < 0 || val > 2*M_PI){
                return false;
            }
            u->u_hue  = val;
        break;

        case 4:
            if (val < -1.0 || val > 1.0){
                return false;
            }
            u->u_brightness  = val;
        break;

        case 5:
            u->u_gamma = val;
        break;

    default:
        return false;
        break;
    }
    return true;

}

bool HDR_parseSDRParams_parsePropertyStr(const char *input, HDR_SDRPARAMS_uniform_t *defaults,HDR_SDRPARAMS_uniform_t *inout){

    HDR_SDRPARAMS_uniform_t old = *inout;

    char **token = xstrtokenize(input, ",");

    bool all_ok = true;
    int num_tokens = 0;

    if (!token) {
        all_ok =false;
    } else {
       for (; token[num_tokens]; num_tokens++) {

          char *endptr = NULL;
          float val = strtof(token[num_tokens],&endptr);
          if (endptr[0] != '\0'){
              //all_ok = false;
              if (strcmp(token[num_tokens],"default") == 0){
                _hdr_string_update_param_from_default(inout,num_tokens,defaults);
              } else if (strcmp(token[num_tokens],"*") == 0){

              } else {
                  all_ok = false;
              }
          } else {
           all_ok &= _hdr_string_update_param(inout,num_tokens,val);
          }
          free(token[num_tokens]);
      }
    }

    if (num_tokens != 6) {
        all_ok = false;
    }

    if (!all_ok){
      memcpy(inout,&old,sizeof(old));

      return false;
    }


  return all_ok;
}

