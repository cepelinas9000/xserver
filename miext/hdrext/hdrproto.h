#ifndef _HDRPROTO_H_
#define _HDRPROTO_H_


#include <X11/Xmd.h>

/**
 * @brief querty version
 */
#define XHDR_HDRQueryVersion 0

/**
 * Get supported surface list with
 */
#define XHDR_SupportedXlibreSurfaces 1
#define XHDR_ApplyVulkanProperties 2


/**
 * @brief version query same as for randr
 **/
typedef struct {
    CARD8   reqType;
    CARD8   HDRReqType;
    CARD16  length;
    CARD32  majorVersion;
    CARD32  minorVersion;
} xHDRQueryVersionReq;
#define sz_xHDRQueryVersionReq   12


typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  majorVersion;
    CARD32  minorVersion;
    CARD64  flags;
    CARD32  vkVersion;
    CARD32  pad;
} xHDRQueryVersionReply;
#define sz_xRRQueryVersionReply	32


typedef enum {
    GraphicsObjectE_SCREEN = 0,
    GraphicsObjectE_WINDOW,
    GraphicsObjectE_COLORMAP,
    GraphicsObjectE_DEVICE, /* xrandr provider id */
    GraphicsObjectE_COLORMAP_XRANDR_CRT,
    GraphicsObjectE_MAX = 255
} HDR_GraphicsObjectE;


typedef struct HDRGraphicObjectId {
    CARD16  flags_lo;
    CARD8   flags_hi;
    CARD8   GraphicsObjectE;
    CARD32  ObjectID;
} HDRGraphicObjectId;


typedef struct X11_HDR_VULKANSTRUCTCHAIN {
    CARD32 vkStructure;
    CARD16 structflags;
    CARD16 total_lo_size;
    CARD8  total_hi_size;
    CARD16 current_lo_size;
    CARD8  current_hi_size;
    CARD32 data[];
} X11_HDR_VULKANSTRUCTCHAIN;



typedef struct {
    CARD8   reqType;
    CARD8   HDRReqType;
    CARD16  length;
    CARD32  pad0;
    CARD64  flags;
    CARD32  msc;
    CARD32  pad;
    CARD16 objects_to_apply_size;
    CARD16 pad2;
    CARD32 pad3;

} xHDRApplyVulkanProperties;


typedef struct {
    CARD8 reqType;
    CARD8   HDRReqType;
    CARD16  length;
    CARD16   deviceIdType;
    CARD16   deviceIdLen;
} xQuerySupportedXlibreSurfaces;


typedef struct {
    CARD8  size;         /* structure size in CARD32 words */
    CARD8  pad1;
    CARD16 visualClass;
    CARD32 format;      /* Vkformat */
    CARD32 colorSpace;  /* VkColorSpaceKHR */
    CARD32 pad3;
    CARD64 flags;

} xSupportedXlibreSurfacesSurfaceType;


typedef struct {
    BYTE type;              /* X_Reply */
    BYTE pad1;             /* depends on reply type */
    CARD16 sequenceNumber;  /* of last request received by server */
    CARD32 length;          /* 4 byte quantities beyond size of GenericReply */

    CARD32 pad00;
    CARD32 pad01;
    CARD32 pad02;
    CARD32 pad03;
    CARD32 pad04;
    CARD32 pad05;

} xSupportedXlibreSurfacesReply;

#endif
