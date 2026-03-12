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
#include "colormap_priv.h"

#include "xacestr.h"
#include "xace.h"
#include "Xext/namespace/hooks.h"

#include "propertyst.h"

#include <X11/Xatom.h>

#include "os/fmt.h"

#include "namespace/namespace.h"

#include "applyvulkanproperties.h"

DevPrivateKeyRec HdrScrPrivateKeyRec;
DevPrivateKeyRec HdrPixPrivateKeyRec;
DevPrivateKeyRec HdrWinPrivateKeyRec;
DevPrivateKeyRec HdrColorMapPrivateKeyRec;


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
    memset(&reply,0,sizeof(xSupportedXlibreSurfacesReply));

    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,TrueColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_B8G8R8A8_SRGB,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,TrueColor);

    /* for example see: https://pixfmtdb.emersion.fr/VK_FORMAT_R16G16B16A16_SFLOAT */

    /*  VK_FORMAT_A2B10G10R10_UNORM_PACK32 <>GL_RGBA,GL_UNSIGNED_INT_2_10_10_10_REV <> DRM_FORMAT_ABGR2101010 */

    //add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_A2B10G10R10_UINT_PACK32,VK_COLOR_SPACE_BT2020_LINEAR_EXT,HDRColor);
    //add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_A2B10G10R10_UINT_PACK32,VK_COLOR_SPACE_HDR10_ST2084_EXT,HDRColor);


    /*  VK_FORMAT_R16G16B16A16_SFLOAT <>DRM_FORMAT_ABGR16161616F */
  //add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_PASS_THROUGH_EXT,HDRColor);

    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_HDR10_ST2084_EXT,HDRColor);
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R16G16B16A16_SFLOAT,VK_COLOR_SPACE_BT2020_LINEAR_EXT,HDRColor);

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

/*
    add_vksurface_format_colorspace(&rpcbuf,VK_FORMAT_R8G8B8A8_UNORM,VK_COLOR_SPACE_PASS_THROUGH_EXT,HDRColor);
*/


  return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);


}


static int ApplyVulkanProperties_onColorMap(ClientPtr client, ColormapPtr pmap, uint32_t *chains,uint32_t sizeleft){

    size_t chain_size = hdr_verify_and_get_size_of_single_chain(chains,sizeleft) * 4;

    /* XXX: maybe support VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR and next in chain VkHdrMetadataEXT for VK_COLOR_SPACE_PASS_THROUGH_EXT */
    HDR_vulkan_struct_header_pakced *h = (HDR_vulkan_struct_header_pakced *)chains;


    if (h->vkStructure == VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR){

        if (chain_size != sizeof(VkSurfaceFormat2KHR)){
            return BadMatch;
        }

        VkSurfaceFormat2KHR *s = (VkSurfaceFormat2KHR*)(h);

        HDRColormapPrivate *priv_cmap = dixLookupPrivate(&pmap->devPrivates, HdrColorMapPrivateKey);

        switch (s->surfaceFormat.colorSpace) {
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
                priv_cmap->colorspace = HDR_colorspace_BT2020;
                priv_cmap->tf = HDR_impl_tf_linear_norm_bt2020;
            break;

            case VK_COLOR_SPACE_HDR10_ST2084_EXT:
                priv_cmap->colorspace = HDR_colorspace_BT2020;
                priv_cmap->tf = HDR_impl_tf_pq;
            break;

            default:
                priv_cmap->colorspace = HDR_colorspace_passthough;
                priv_cmap->tf = HDR_impl_tf_linear;

            break;
        }

      return Success;
    }

    return BadMatch;

}

/*
static int
ApplyVulkanProperties_onPixmap(ClientPtr client,PixmapPtr ppixmap,HDRPixmapPrivate *priv,uint32_t *chains,uint32_t sizeleft){

    size_t chain_size = hdr_verify_and_get_size_of_single_chain(chains,sizeleft) * 4;

    HDR_vulkan_struct_header_pakced *h = (HDR_vulkan_struct_header_pakced *)chains;


    if (h->vkStructure == VK_STRUCTURE_TYPE_HDR_METADATA_EXT){
        if (chain_size != sizeof(VkHdrMetadataEXT)){
            return BadMatch;
        }

        VkHdrMetadataEXT *s = (VkHdrMetadataEXT*)(h);


        / ** XXX: set metadata header ** /
        return Success;
    }

  return BadMatch;
}
*/

static int
ProcHDRApplyVulkanProperties(ClientPtr client){


    REQUEST_AT_LEAST_EXTRA_SIZE(xHDRApplyVulkanProperties,sizeof(HDRGraphicObjectId)+ sizeof(HDR_vulkan_struct_header_pakced));
    REQUEST(xHDRApplyVulkanProperties);

    if ( stuff->objects_to_apply_size != 1){
        return BadValue; /* for now only thing */
    }

    if ( ((stuff->objects_to_apply_size * sizeof(HDRGraphicObjectId) >> 2)) >= client->req_len ) {
        return BadLength;
    }

    HDRGraphicObjectId *o= (HDRGraphicObjectId *)( &stuff[1]);
    void *chain_start = (HDRGraphicObjectId *)( &o[stuff->objects_to_apply_size ] );

    const size_t chains_size_in_cards32= client->req_len  - ( (ptrdiff_t) ( chain_start - (void*)stuff)>>2);
    if (!hdr_verify_all_chains(chain_start,chains_size_in_cards32)) {
        return BadRequest;
    }


    if (o->GraphicsObjectE == GraphicsObjectE_COLORMAP){
        ColormapPtr pmap;
        int rc = dixLookupResourceByType((void **) &pmap, o->ObjectID, X11_RESTYPE_COLORMAP,
                                 client, DixWriteAccess);

        if (rc != Success){
            return rc;
        }

        if (pmap ==  NULL){
            return BadRequest;
        }

        if (pmap->class != HDRColor){
            return BadColor;
        }

        return ApplyVulkanProperties_onColorMap(client,pmap,chain_start,chains_size_in_cards32);

    }

    if (o->GraphicsObjectE == GraphicsObjectE_WINDOW){

        WindowPtr pwindow;
        int rc = dixLookupResourceByType((void **) &pwindow, o->ObjectID, X11_RESTYPE_WINDOW,
                                 client, DixWriteAccess);

        if (rc != Success){
            return rc;
        }

        if (pwindow ==  NULL){
            return BadRequest;
        }

      return Success;
    }

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
static void HDRImportPixmap(CallbackListPtr *pcbl, ScreenPtr pScreen, pXorgScreenHDRImportPixmapProcPtr_params_S pParams);


static void
HDRCloseScreen(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unused){

    dixScreenUnhookPostClose(pScreen, HDRCloseScreen);
    dixScreenUnhookWindowDestroy(pScreen, HDRWindowDestroy);
    dixScreenUnhookPixmapDestroy(pScreen, HDRPixmapDestroy);
    dixScreenUnhookHDRImportPixmap(pScreen,HDRImportPixmap);
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

    HDRPixmapPrivate *priv = dixLookupPrivate(&pPixmap->devPrivates, HdrPixPrivateKey);
    /* XXX: implement freeing when there will be other colorspace support */
}


/* XXX: don't forget other colorspaces */
static void HDRImportPixmap(CallbackListPtr *pcbl, ScreenPtr pScreen, pXorgScreenHDRImportPixmapProcPtr_params_S pParams){
    HDRPixmapPrivate   *pixmap_priv   = dixLookupPrivate(&pParams->pPixmap->devPrivates, HdrPixPrivateKey);
    HDRColormapPrivate *colormap_priv = dixLookupPrivate(&pParams->pCmap->devPrivates, HdrColorMapPrivateKey);


    pixmap_priv->purpose =  HDR_pixmap_HDR;
    pixmap_priv->tf = colormap_priv->tf;
    pixmap_priv->colorspace = colormap_priv->colorspace;
    memcpy(&pixmap_priv->colorspace_points,&colormap_priv->colorspace_points,sizeof(hdr_color_attributes));

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


    if (!dixRegisterPrivateKey(&HdrColorMapPrivateKeyRec,PRIVATE_COLORMAP,sizeof(HDRColormapPrivate))){
        return FALSE;
    }

    HDR__X11HDR_SDR_PARAMS_atom = dixAddAtom("__X11HDR_SDR_PARAMS");


    dixScreenHookClose(pScreen, HDRCloseScreen);
    dixScreenHookWindowDestroy(pScreen, HDRWindowDestroy);
    dixScreenHookPixmapDestroy(pScreen, HDRPixmapDestroy);
    dixScreenHookHDRImportPixmap(pScreen, HDRImportPixmap);

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


    /* XXX: investigate why we setting screen output before get edid & colorimetry*/

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

