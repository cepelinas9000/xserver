#ifndef __HDR_EXT_PRIV__
#define __HDR_EXT_PRIV__

#include <stdint.h>


/**
 * @brief HDR_impl_tf internal represantation
 */
typedef enum {
    HDR_impl_tf_linear = 0,
    HDR_impl_tf_pq     = 1,
    HDR_impl_tf_gamma  = 2,
    HDR_impl_passthough = 3,
} HDR_impl_tf;


typedef struct {

    float   tune_paramsf[4];
    int32_t tune_paramsi[4];


} HDRExtWindowPrivate;

#endif
