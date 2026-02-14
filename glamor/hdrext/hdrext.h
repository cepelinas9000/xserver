#ifndef _HDREXT_H_
#define _HDREXT_H_

#include <stdbool.h>
#include "screenint.h"

#include <X11/Xfuncproto.h>
#include <vulkan/vulkan.h>

#define HDREXT_NAME "HDRProto"

#include <stddef.h>
#include <stdint.h>

/**
 * Wrapped (decoded) vulkan structure header from ListOfVULKANSTRUCTCHAIN
 */
typedef struct HDR_vulkan_struct_header_packed {
    uint32_t   vkStructure;
    union {
        uint64_t pVoid;
        struct {
            uint16_t  struct_flags;
            union {
                uint8_t   total_size_f[3];
                uint32_t  total_size    :24;
            };

            union {
                uint8_t   current_size_f[3];
                uint32_t  current_size    :24;
            };

        } __attribute__((packed));
    };

    uint64_t data[0];
}  __attribute__((packed)) HDR_vulkan_struct_header_pakced;

/**
 * @brief hdr_monitr_colorimetry
 * this struct hold enough colorimetry information calculate transformation matrix
 */
typedef struct {

    float white_point[2];
    float color_r[2];
    float color_g[2];
    float color_b[2];

    float max_britness_nits;
} hdr_color_attributes;

/**
 * @brief HDREXT_VULKANBASE_VERSION this is informative version
 * It doens't mean that we require this version!. It for client information that there are
 * supporting control structures defined in that version specification
 **/
#define HDREXT_VULKANBASE_VERSION VK_MAKE_API_VERSION(0,1,4,341)


void HDRExtensionInit(ScreenPtr pScreen);

/**
 * @brief HdrSetColorMatrix set internal shader color matrix
 * @param pScreen
 * @param hdr_color_attrs
 */
_X_EXPORT
void HdrSetColorMatrix(ScreenPtr pScreen,int crtnum,hdr_color_attributes *hdr_color_attrs);

#endif

