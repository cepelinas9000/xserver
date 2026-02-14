#include <dix-config.h>

#include "hdrext.h"

#include "colormap.h"
#include "privates.h"

#include "extension.h"
#include "extnsionst.h"

#include "request_priv.h"

#include "hdrproto.h"

#include "screenint_priv.h"
#include "screen_hooks_priv.h"

#include "hdrext_priv.h"
#include "hdrext_utils.h"

DevPrivateKeyRec HdrScrPrivateKeyRec;
DevPrivateKeyRec HdrPixPrivateKeyRec;
DevPrivateKeyRec HdrWinPrivateKeyRec;


static int
ProcHDRQueryVersion(ClientPtr client){


    REQUEST(xHDRQueryVersionReq);
    REQUEST_SIZE_MATCH(xHDRQueryVersionReq);


    if (stuff->majorVersion != 0 && stuff->minorVersion != 1){
        return BadValue;
    }


  xHDRQueryVersionReply reply = {
      .majorVersion = 0,
      .minorVersion = 1,
      .vkVersion = HDREXT_VULKANBASE_VERSION,
      .flags = 0,
  };


    return X_SEND_REPLY_SIMPLE(client, reply);
}

static bool add_vksurface_format_colorspace(x_rpcbuf_t *rpc_buf,VkFormat format,VkColorSpaceKHR colorspace,uint16_t visual){

    xSupportedXlibreSurfacesSurfaceType *s = x_rpcbuf_reserve(rpc_buf,sizeof(xSupportedXlibreSurfacesSurfaceType));
    if (s == NULL){
        return false;
    }

    s->size = sizeof(xSupportedXlibreSurfacesSurfaceType) / 4;
    s->format = format;
    s->colorSpace = colorspace;
    s->visualClass= visual;

  return true;
}
static int
ProcHDRSupportedXlibreSurfaces(ClientPtr client){

    /* XXX: currently only static list */
    REQUEST(xQuerySupportedXlibreSurfaces);
    REQUEST_AT_LEAST_EXTRA_SIZE(xQuerySupportedXlibreSurfaces,4);


    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    xSupportedXlibreSurfacesReply reply;


    /* for example see: https://pixfmtdb.emersion.fr/VK_FORMAT_R16G16B16A16_SFLOAT */

    /*  VK_FORMAT_A2B10G10R10_UNORM_PACK32 <>GL_RGBA,GL_UNSIGNED_INT_2_10_10_10_REV <> DRM_FORMAT_ABGR2101010 */
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_A2B10G10R10_UINT_PACK32,VK_COLOR_SPACE_PASS_THROUGH_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_A2B10G10R10_UINT_PACK32,VK_COLOR_SPACE_BT2020_LINEAR_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_A2B10G10R10_UINT_PACK32,VK_COLOR_SPACE_HDR10_ST2084_EXT,HDRColor);


    /*  VK_FORMAT_R16G16B16A16_SFLOAT <>DRM_FORMAT_ABGR16161616F */
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_PASS_THROUGH_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_BT2020_LINEAR_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_HDR10_ST2084_EXT,HDRColor);

    /*  VK_FORMAT_R16G16B16A16_SFLOAT <>DRM_FORMAT_ABGR16161616F ( note for little endian architecture)*/
/*
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_PASS_THROUGH_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_BT2020_LINEAR_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_HDR10_ST2084_EXT,HDRColor);
*/

    /*  VK_FORMAT_R16G16B16A16_UNORM <>DRM_FORMAT_ABGR16161616 (this is one integer, note for little endian architecture )*/
/*
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_UNORM,VK_COLOR_SPACE_PASS_THROUGH_EXT);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_UNORM,VK_COLOR_SPACE_BT2020_LINEAR_EXT);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_UNORM,VK_COLOR_SPACE_HDR10_ST2084_EXT);
*/

    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R8G8B8A8_UNORM,VK_COLOR_SPACE_PASS_THROUGH_EXT,HDRColor);

    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R8G8B8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,TrueColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R8G8B8A8_SRGB,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,TrueColor);

  return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);


}

static int
ProcHDRApplyVulkanProperties(ClientPtr client){


    return BadRequest;
}

static int
ProcHDRDispatch(ClientPtr client)
{
    REQUEST(xReq);

    UpdateCurrentTimeIf();

    /* no swapped clients currently */
    if (client->swapped){
        return BadImplementation;
    }



    switch (stuff->data) {
        case XHDR_HDRQueryVersion         : return ProcHDRQueryVersion(client); break;
        case XHDR_SupportedXlibreSurfaces : return ProcHDRSupportedXlibreSurfaces(client); break;
        case XHDR_ApplyVulkanProperties   : return ProcHDRApplyVulkanProperties(client); break;
    }

    return BadRequest;
}

static void HDRWindowDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, WindowPtr pWindow);
static void HDRPixmapDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, PixmapPtr pPixmap);

static void
HDRCloseScreen(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unused){

    dixScreenUnhookPostClose(pScreen, HDRCloseScreen);
    dixScreenUnhookWindowDestroy(pScreen, HDRWindowDestroy);
    dixScreenUnhookPixmapDestroy(pScreen, HDRPixmapDestroy);

    HDRScreenPrivateData *priv = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);
    if (priv){
        FreeHDRScreenPrivateData(priv);
    }


}

static void
HDRWindowDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, WindowPtr pWindow)
{

}

static void
HDRPixmapDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, PixmapPtr pPixmap)
{

}

static bool
HDRExtensionSetup(ScreenPtr pScreen)
{
    if (!dixRegisterPrivateKey(&HdrScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey)) {
        return TRUE;
     } else {
        dixSetPrivate(&pScreen->devPrivates,HdrScrPrivateKey,NULL);
     }

    if (!dixRegisterPrivateKey(&HdrPixPrivateKeyRec, PRIVATE_PIXMAP, sizeof(HDRPixmapPrivate)))
        return FALSE;


    if (!dixRegisterPrivateKey(&HdrWinPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;


    dixScreenHookClose(pScreen, HDRCloseScreen);
    dixScreenHookWindowDestroy(pScreen, HDRWindowDestroy);
    dixScreenHookPixmapDestroy(pScreen, HDRPixmapDestroy);



    InitializeScreenShaders(pScreen);

    return TRUE;
}

void
HDRExtensionInit(ScreenPtr pScreen){


    HDRExtensionSetup(pScreen);


    AddExtension(HDREXT_NAME, 0, 0,
                 ProcHDRDispatch, ProcHDRDispatch,
                 NULL, StandardMinorOpcode);

}

void HdrSetColorMatrix(ScreenPtr pScreen,int crtnum, hdr_color_attributes *hdr_color_attrs)
{
  float m[3][3];
  HDRutil_BT2020_matrix_colorspace(hdr_color_attrs,m);

  HDRScreenPrivateData *priv = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);
  BUG_RETURN_MSG(priv == NULL,"HdrSetColorMatrix called without initialized screen private");

  if (priv->CRT_color_matrices[crtnum] != 0){
        glDeleteBuffers(1,&priv->CRT_color_matrices[crtnum]);
  }

  glGenBuffers(1, &priv->CRT_color_matrices[crtnum]);
  glBindBuffer(GL_UNIFORM_BUFFER, priv->CRT_color_matrices[crtnum]);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(m), m, GL_STATIC_DRAW);


}

void HdrFlagPixmap_CRT(PixmapPtr pixmap)
{
  HDRPixmapPrivate *priv = dixLookupPrivate(&pixmap->devPrivates, HdrPixPrivateKey);
  priv->purpose = HDR_pixmap_crt;


}

void HdrFlagPixmap_INTERMEDIATE(PixmapPtr pixmap)
{
  HDRPixmapPrivate *priv = dixLookupPrivate(&pixmap->devPrivates, HdrPixPrivateKey);
  priv->purpose = HDR_pixmap_intermiate;
}
