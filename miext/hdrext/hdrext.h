#ifndef _HDREXT_H_
#define _HDREXT_H_
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdbool.h>

#include <X11/Xfuncproto.h>
#include <vulkan/vulkan.h>

#define HDREXT_NAME "HDRToadProto"

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
            uint16_t  struct_flags  :16;
            uint32_t  total_size    :24;
            uint32_t  current_size  :24;
        } __attribute__((packed));
    };

    uint64_t data[0];
}  __attribute__((packed)) HDR_vulkan_struct_header_pakced;



/**
 * @brief HDREXT_VULKANBASE_VERSION this is informative version
 * It doens't mean that we require this version!. It for client information that there are
 * supporting control structures defined in that version specification
 **/
#define HDREXT_VULKANBASE_VERSION VK_MAKE_API_VERSION(0,1,4,341)


void HDRExtensionInit(void);


#endif

