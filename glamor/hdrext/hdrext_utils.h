#ifndef __HDREXT_UTILS_H_
#define __HDREXT_UTILS_H_

/**
 * utility functions
 * matrix operations based on https://github.com/recp/cglm
 */

#include "hdrext.h"


/** @brief matrix operations
 * @{ */


void HDRutil_BT2020_matrix_colorspace(hdr_color_attributes *target_colorspace, float conv_matrix[3][3]);



/** @} */

#endif
