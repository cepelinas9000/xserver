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

#include "dix_priv.h"
#include "property_priv.h"

#include "xacestr.h"
#include "xace.h"
#include "Xext/namespace/hooks.h"

#include "propertyst.h"

#include <X11/Xatom.h>

#include "os/fmt.h"

#include "namespace/namespace.h"

DevPrivateKeyRec HdrScrPrivateKeyRec;
DevPrivateKeyRec HdrPixPrivateKeyRec;
DevPrivateKeyRec HdrWinPrivateKeyRec;


Atom HDR__X11HDR_SDR_PARAMS_atom;


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

    HDRExtWindowPrivate *priv = dixLookupPrivate(&pWindow->devPrivates, HdrWinPrivateKey);

    if (priv->sdr_params_u){

        glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
        glamor_make_current(glamor_priv);
        glDeleteBuffers(1,&priv->sdr_params_u);
    }


}

static void
HDRPixmapDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, PixmapPtr pPixmap)
{

}


/**
 * @brief Hook_HDR_property_filter it only checks if HDR__X11HDR_SDR_PARAMS_atom have correct structure
 * @param pcbl
 * @param unused
 * @param calldata
 */
static void
Hook_HDR_property_filter(CallbackListPtr *pcbl, void *unused, void *calldata){


    XacePropertyAccessRec *param = (XacePropertyAccessRec*)(calldata);

    if ( (*param->ppProp)->propertyName != HDR__X11HDR_SDR_PARAMS_atom ){
        return;
    }

    if (param->access_mode == DixReadAccess){
      return;
    }

    if ( (*param->ppProp)->type != XA_STRING){
        param->status = BadAccess;
        return;
    }

    /* there is "default" (inherits from root window) value and string with 6 fp numbers "u_whitepoint_ref,u_contrast,u_saturation,u_hue,u_brightness,u_gamma" */

    HDRScreenPrivateData *priv_screen = dixLookupPrivate(&param->pWin->drawable.pScreen->devPrivates, HdrScrPrivateKey);


    char *val = (char *)(param->ppProp[0]->data);

    if (strcmp(val,"default") == 0 ){


      free(param->ppProp[0]->data);

      param->ppProp[0]->data = HDR_SDRPARAMS_uniform_t_str(&priv_screen->default_sdr_params);
      param->ppProp[0]->size = strlen(param->ppProp[0]->data) + 1;
    }

    HDR_SDRPARAMS_uniform_t sdr_params;
    memset(&sdr_params,0,sizeof(sdr_params));


  bool ret = HDR_parseSDRParams_parsePropertyStr(param->ppProp[0]->data,&priv_screen->default_sdr_params,&sdr_params);

  if (ret == false){
        param->status = BadAccess;
        return;
  }

  hdr_WindowUpdateSDRParams(param->pWin->drawable.pScreen,param->pWin,&sdr_params);
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


    if (!dixRegisterPrivateKey(&HdrWinPrivateKeyRec, PRIVATE_WINDOW, sizeof(HDRExtWindowPrivate)))
        return FALSE;


    HDR__X11HDR_SDR_PARAMS_atom = dixAddAtom("__X11HDR_SDR_PARAMS");




    dixScreenHookClose(pScreen, HDRCloseScreen);
    dixScreenHookWindowDestroy(pScreen, HDRWindowDestroy);
    dixScreenHookPixmapDestroy(pScreen, HDRPixmapDestroy);

    XaceRegisterCallback(XACE_PROPERTY_ACCESS,Hook_HDR_property_filter,NULL); /* XXX: it should be callback like in RR for property update */

    InitializeScreenShaders(pScreen);

    return TRUE;
}

void
HDRExtensionInit(ScreenPtr pScreen){


    HDRExtensionSetup(pScreen);

    AddExtension(HDREXT_NAME, 0, 0,
                 ProcHDRDispatch, ProcHDRDispatch,
                 NULL, StandardMinorOpcode);


    /* XXX: investigate why we setting screen output before get edid& colorimetry*/

    {
      hdr_color_attributes crt0;
      crt0.color_r[0] = 0.708;
      crt0.color_r[1] = 0.292;
      crt0.color_g[0] = 0.170;
      crt0.color_g[1] = 0.797;
      crt0.color_b[0] = 0.131;
      crt0.color_b[1] = 0.046;

      crt0.white_point[0]=0.3127;
      crt0.white_point[1]=0.3290;


      crt0.max_britness_nits = 400;
      HdrSetColorMatrix(pScreen,0,&crt0);
    }
}

void HdrSetColorMatrix(ScreenPtr pScreen,int crtnum, hdr_color_attributes *hdr_color_attrs)
{
  struct {
  float u_colorMatrix[3][4];
} data;

  memset(&data,0,sizeof(data));

  float m[3][3];

  HDRutil_BT2020_matrix_colorspace(hdr_color_attrs,m);

  memcpy(data.u_colorMatrix[0],m[0],3*sizeof(float));
  memcpy(data.u_colorMatrix[1],m[1],3*sizeof(float));
  memcpy(data.u_colorMatrix[2],m[2],3*sizeof(float));

  HDRScreenPrivateData *priv = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);
  BUG_RETURN_MSG(priv == NULL,"HdrSetColorMatrix called without initialized screen private");

    glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
    glamor_make_current(glamor_priv);


  if (priv->CRT_color_matrices[crtnum] != 0){
        glDeleteBuffers(1,&priv->CRT_color_matrices[crtnum]);
  }


  glGenBuffers(1, &priv->CRT_color_matrices[crtnum]);
  glBindBuffer(GL_UNIFORM_BUFFER, priv->CRT_color_matrices[crtnum]);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);



}

void HdrFlagPixmap_CRT(PixmapPtr pixmap, int crtnum)
{
  HDRPixmapPrivate *priv = dixLookupPrivate(&pixmap->devPrivates, HdrPixPrivateKey);
  priv->purpose = HDR_pixmap_crt;
  priv->crt_num = crtnum;


}

void HdrFlagPixmap_INTERMEDIATE(PixmapPtr pixmap)
{
  HDRPixmapPrivate *priv = dixLookupPrivate(&pixmap->devPrivates, HdrPixPrivateKey);
  priv->purpose = HDR_pixmap_intermiate;
}

void HdrFlagPixmap_SetPurpose(PixmapPtr pixmap, bool set_hdr)
{
  HDRPixmapPrivate *priv = dixLookupPrivate(&pixmap->devPrivates, HdrPixPrivateKey);

    if (priv->purpose == HDR_pixmap_intermiate || priv->purpose == HDR_pixmap_crt){
        return;
    }

  priv->purpose = set_hdr ? HDR_pixmap_HDR : HDR_pixmap_SDR_OR_PASSTHROUGH;
}

void HdrFillDefaultProperties(ScreenPtr pScreen, WindowPtr wnd)
{
  HDRScreenPrivateData *priv_screen = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);

  if (priv_screen == NULL){
      return;
  }


}

HDR_conversion_e hdr_pick_conversion(DrawablePtr src,DrawablePtr dst){

  /* XXX: todo */
  if (dst->depth == 64 || dst->depth == 30) {

    if (src->depth == 30 || src->depth == 24 || src->depth == 16 || src->depth == 8 || src->depth == 4 || src->depth == 1){
        return HDR_conversion_SDR_to_Intermiate;
    }

  }

  return HDR_conversion_no_need;
}

