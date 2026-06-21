/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <dix-config.h>

#include <unistd.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"
#include "include/syncsdk.h"
#include "os/client_priv.h"
#include "Xext/randr/randrstr_priv.h"

#include "dri3_priv.h"
#include "Xext/sync/syncsrv.h"
#include "xlibreEXT/dri3xlibreproto.h"
#include "xf86.h"

#include <xace.h>
#include <protocol-versions.h>
#include <drm_fourcc.h>
#include "dixstruct_priv.h"

#include "propertyst.h"
#include "property_priv.h"

#include <X11/Xatom.h>

static Bool
dri3_screen_can_one_point_one(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    if (dri3 && dri3->info && dri3->info->version >= 1 &&
        dri3->info->fd_from_pixmap)
        return TRUE;

    return FALSE;
}

static Bool
dri3_screen_can_one_point_two(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    if (dri3 && dri3->info && dri3->info->version >= 2 &&
        dri3->info->pixmap_from_fds && dri3->info->fds_from_pixmap &&
        dri3->info->get_formats && dri3->info->get_modifiers &&
        dri3->info->get_drawable_modifiers)
        return TRUE;

    return FALSE;
}

static Bool
dri3_screen_can_one_point_four(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    return dri3 &&
        dri3->info &&
        dri3->info->version >= 4 &&
        dri3->info->import_syncobj;
}

static int
proc_dri3_query_version(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3QueryVersionReq);
    X_REQUEST_FIELD_CARD32(majorVersion);
    X_REQUEST_FIELD_CARD32(minorVersion);

    xDRI3QueryVersionReply reply = {
        .majorVersion = SERVER_DRI3_MAJOR_VERSION,
        .minorVersion = SERVER_DRI3_MINOR_VERSION
    };

    DIX_FOR_EACH_SCREEN({
        if (!dri3_screen_can_one_point_one(walkScreen)) {
            reply.minorVersion = 0;
            break;
        }
        if (!dri3_screen_can_one_point_two(walkScreen)) {
            reply.minorVersion = 1;
            break;
        }
        if (!dri3_screen_can_one_point_four(walkScreen)) {
            reply.minorVersion = 2;
            break;
        } else {
            reply.minorVersion = 4;
            break;
        }
    });

    DIX_FOR_EACH_GPU_SCREEN({
        if (!dri3_screen_can_one_point_one(walkScreen)) {
            reply.minorVersion = 0;
            break;
        }
        if (!dri3_screen_can_one_point_two(walkScreen)) {
            reply.minorVersion = 1;
            break;
        }
        if (!dri3_screen_can_one_point_four(walkScreen)) {
            reply.minorVersion = 2;
            break;
        } else {
            reply.minorVersion = 4;
            break;
        }
    });

    /* From DRI3 proto:
     *
     * The client sends the highest supported version to the server
     * and the server sends the highest version it supports, but no
     * higher than the requested version.
     */

    reply.majorVersion = 1;
    reply.minorVersion = 5;

    if (reply.majorVersion > stuff->majorVersion ||
        (reply.majorVersion == stuff->majorVersion &&
         reply.minorVersion > stuff->minorVersion)) {
        reply.majorVersion = stuff->majorVersion;
        reply.minorVersion = stuff->minorVersion;
    }

    X_REPLY_FIELD_CARD32(majorVersion);
    X_REPLY_FIELD_CARD32(minorVersion);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
dri3_send_open_reply(ClientPtr client, int fd)
{
    xDRI3OpenReply reply = {
        .nfd = 1,
    };

    if (WriteFdToClient(client, fd, TRUE) < 0) {
        close(fd);
        return BadAlloc;
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
proc_dri3_open(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3OpenReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(provider);

    RRProviderPtr provider;
    DrawablePtr drawable;
    ScreenPtr screen;
    int fd;
    int status;

    status = dixLookupDrawable(&drawable, stuff->drawable, client, 0, DixGetAttrAccess);
    if (status != Success)
        return status;

    if (stuff->provider == None)
        provider = NULL;
    else if (!RRProviderType) {
        return BadMatch;
    } else {
        VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);
        if (drawable->pScreen != provider->pScreen)
            return BadMatch;
    }
    screen = drawable->pScreen;

    /* XXX: it only handles default case */
    dri3_client_private_ptr client_priv = dri3_get_client_private(client);
    if (client_priv && client_priv->active_screen && provider == NULL){
       screen = client_priv->active_screen;
    }
    status = dri3_open(client, screen, provider, &fd);
    if (status != Success)
        return status;

    if (client->ignoreCount == 0)
        return dri3_send_open_reply(client, fd);

    return Success;
}

static int
proc_dri3_pixmap_from_buffer(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3PixmapFromBufferReq);
    X_REQUEST_FIELD_CARD32(pixmap);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(size);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD16(stride);

    int fd;
    DrawablePtr drawable;
    PixmapPtr pixmap;
    CARD32 stride, offset;
    int rc;

    SetReqFds(client, 1);
    LEGAL_NEW_RESOURCE(stuff->pixmap, client);
    rc = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->drawable;
        return rc;
    }

    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }

    if (stuff->width > 32767 || stuff->height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        DepthPtr depth = drawable->pScreen->allowedDepths;
        int i;
        for (i = 0; i < drawable->pScreen->numDepths; i++, depth++)
            if (depth->depth == stuff->depth)
                break;
        if (i == drawable->pScreen->numDepths) {
            client->errorValue = stuff->depth;
            return BadValue;
        }
    }

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    offset = 0;
    stride = stuff->stride;
    rc = dri3_pixmap_from_fds(&pixmap,
                              drawable->pScreen, 1, &fd,
                              stuff->width, stuff->height,
                              &stride, &offset,
                              stuff->depth, stuff->bpp,
                              DRM_FORMAT_MOD_INVALID);
    close (fd);
    if (rc != Success)
        return rc;

    pixmap->drawable.id = stuff->pixmap;

    /* security creation/labeling check */
    rc = XaceHookResourceAccess(client, stuff->pixmap, X11_RESTYPE_PIXMAP,
                  pixmap, X11_RESTYPE_NONE, NULL, DixCreateAccess);

    if (rc != Success) {
        dixDestroyPixmap(pixmap, 0);
        return rc;
    }
    if (!AddResource(stuff->pixmap, X11_RESTYPE_PIXMAP, (void *) pixmap))
        return BadAlloc;

    return Success;
}

static int
proc_dri3_buffer_from_pixmap(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3BufferFromPixmapReq);
    X_REQUEST_FIELD_CARD32(pixmap);

    int rc;
    int fd;
    PixmapPtr pixmap;

    rc = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, X11_RESTYPE_PIXMAP,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->pixmap;
        return rc;
    }

    xDRI3BufferFromPixmapReply reply = {
        .nfd = 1,
        .width = pixmap->drawable.width,
        .height = pixmap->drawable.height,
        .depth = pixmap->drawable.depth,
        .bpp = pixmap->drawable.bitsPerPixel,
    };

    fd = dri3_fd_from_pixmap(pixmap, &reply.stride, &reply.size);
    if (fd < 0)
        return BadPixmap;

    if (WriteFdToClient(client, fd, TRUE) < 0) {
        close(fd);
        return BadAlloc;
    }

    X_REPLY_FIELD_CARD32(size);
    X_REPLY_FIELD_CARD16(width);
    X_REPLY_FIELD_CARD16(height);
    X_REPLY_FIELD_CARD16(stride);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
proc_dri3_fence_from_fd(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3FenceFromFDReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(fence);

    DrawablePtr drawable;
    int fd;
    int status;

    SetReqFds(client, 1);
    LEGAL_NEW_RESOURCE(stuff->fence, client);

    status = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    status = SyncCreateFenceFromFD(client, drawable, stuff->fence,
                                   fd, stuff->initially_triggered);

    return status;
}

static int
proc_dri3_fd_from_fence(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3FDFromFenceReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(fence);

    xDRI3FDFromFenceReply reply = {
        .nfd = 1,
    };
    DrawablePtr drawable;
    int fd;
    int status;
    SyncFence *fence;

    status = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;
    status = SyncVerifyFence(&fence, stuff->fence, client, DixWriteAccess);
    if (status != Success)
        return status;

    fd = SyncFDFromFence(client, drawable, fence);
    if (fd < 0)
        return BadMatch;

    if (WriteFdToClient(client, fd, FALSE) < 0)
        return BadAlloc;

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
proc_dri3_get_supported_modifiers(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3GetSupportedModifiersReq);
    X_REQUEST_FIELD_CARD32(window);

    WindowPtr window;
    ScreenPtr pScreen;
    CARD64 *window_modifiers = NULL;
    CARD64 *screen_modifiers = NULL;
    CARD32 nwindowmodifiers = 0;
    CARD32 nscreenmodifiers = 0;
    int status;

    status = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (status != Success)
        return status;
    pScreen = window->drawable.pScreen;

    dri3_get_supported_modifiers(pScreen, &window->drawable,
                                 stuff->depth, stuff->bpp,
                                 &nwindowmodifiers, &window_modifiers,
                                 &nscreenmodifiers, &screen_modifiers);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD64s(&rpcbuf, window_modifiers, nwindowmodifiers);
    x_rpcbuf_write_CARD64s(&rpcbuf, screen_modifiers, nscreenmodifiers);

    free(window_modifiers);
    free(screen_modifiers);

    xDRI3GetSupportedModifiersReply reply = {
        .numWindowModifiers = nwindowmodifiers,
        .numScreenModifiers = nscreenmodifiers,
    };

    X_REPLY_FIELD_CARD32(numWindowModifiers);
    X_REPLY_FIELD_CARD32(numScreenModifiers);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
proc_dri3_pixmap_from_buffers(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3PixmapFromBuffersReq);
    X_REQUEST_FIELD_CARD32(pixmap);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD32(stride0);
    X_REQUEST_FIELD_CARD32(offset0);
    X_REQUEST_FIELD_CARD32(stride1);
    X_REQUEST_FIELD_CARD32(offset1);
    X_REQUEST_FIELD_CARD32(stride2);
    X_REQUEST_FIELD_CARD32(offset2);
    X_REQUEST_FIELD_CARD32(stride3);
    X_REQUEST_FIELD_CARD32(offset3);
    X_REQUEST_FIELD_CARD64(modifier);

    int fds[4];
    CARD32 strides[4], offsets[4];
    ScreenPtr screen;
    WindowPtr window;
    PixmapPtr pixmap;
    int rc;
    int i;

    SetReqFds(client, stuff->num_buffers);
    LEGAL_NEW_RESOURCE(stuff->pixmap, client);
    rc = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->window;
        return rc;
    }
    screen = window->drawable.pScreen;

    if (!stuff->width || !stuff->height || !stuff->bpp || !stuff->depth) {
        client->errorValue = 0;
        return BadValue;
    }

    if (stuff->width > 32767 || stuff->height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        DepthPtr depth = screen->allowedDepths;
        int j;
        for (j = 0; j < screen->numDepths; j++, depth++)
            if (depth->depth == stuff->depth)
                break;
        if (j == screen->numDepths) {
            client->errorValue = stuff->depth;
            return BadValue;
        }
    }

    if (!stuff->num_buffers || stuff->num_buffers > 4) {
        client->errorValue = stuff->num_buffers;
        return BadValue;
    }

    for (i = 0; i < stuff->num_buffers; i++) {
        fds[i] = ReadFdFromClient(client);
        if (fds[i] < 0) {
            while (--i >= 0)
                close(fds[i]);
            return BadValue;
        }
    }

    strides[0] = stuff->stride0;
    strides[1] = stuff->stride1;
    strides[2] = stuff->stride2;
    strides[3] = stuff->stride3;
    offsets[0] = stuff->offset0;
    offsets[1] = stuff->offset1;
    offsets[2] = stuff->offset2;
    offsets[3] = stuff->offset3;

    rc = dri3_pixmap_from_fds(&pixmap, screen,
                              stuff->num_buffers, fds,
                              stuff->width, stuff->height,
                              strides, offsets,
                              stuff->depth, stuff->bpp,
                              stuff->modifier);

    for (i = 0; i < stuff->num_buffers; i++)
        close (fds[i]);

    if (rc != Success)
        return rc;

    pixmap->drawable.id = stuff->pixmap;

    /* security creation/labeling check */
    rc = XaceHookResourceAccess(client, stuff->pixmap, X11_RESTYPE_PIXMAP,
                  pixmap, X11_RESTYPE_NONE, NULL, DixCreateAccess);

    if (rc != Success) {
        dixDestroyPixmap(pixmap, 0);
        return rc;
    }
    if (!AddResource(stuff->pixmap, X11_RESTYPE_PIXMAP, (void *) pixmap))
        return BadAlloc;

    return Success;
}

static int
proc_dri3_buffers_from_pixmap(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3BuffersFromPixmapReq);
    X_REQUEST_FIELD_CARD32(pixmap);

    int rc;
    int fds[4];
    int num_fds;
    uint32_t strides[4], offsets[4];
    uint64_t modifier;
    int i;
    PixmapPtr pixmap;

    rc = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, X11_RESTYPE_PIXMAP,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->pixmap;
        return rc;
    }

    num_fds = dri3_fds_from_pixmap(pixmap, fds, strides, offsets, &modifier);
    if (num_fds == 0)
        return BadPixmap;

    for (i = 0; i < num_fds; i++) {
        if (WriteFdToClient(client, fds[i], TRUE) < 0) {
            while (i--)
                close(fds[i]);
            return BadAlloc;
        }
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)strides, num_fds);
    x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)offsets, num_fds);

    xDRI3BuffersFromPixmapReply reply = {
        .nfd = num_fds,
        .width = pixmap->drawable.width,
        .height = pixmap->drawable.height,
        .depth = pixmap->drawable.depth,
        .bpp = pixmap->drawable.bitsPerPixel,
        .modifier = modifier,
    };

    X_REPLY_FIELD_CARD16(width);
    X_REPLY_FIELD_CARD16(height);
    X_REPLY_FIELD_CARD64(modifier);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static bool drm_device_matches(ScreenPtr scr,uint32_t drmMajor,uint32_t drmMinor){
    rrScrPrivPtr pScrPriv = rrGetScrPriv(scr);
    RRProviderPtr pp = pScrPriv->provider;

    if ( pp->drmDevice){
        return pp->drmMajor == drmMajor && pp->drmMinor == drmMinor;
    }

    return false;
}
static int
proc_dri3_set_drm_device_in_use(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3SetDRMDeviceInUseReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD32(drmMajor);
    X_REQUEST_FIELD_CARD32(drmMinor);

    WindowPtr window;
    int status;

    status = dixLookupWindow(&window, stuff->window, client,
                             DixGetAttrAccess);
    if (status != Success)
        return status;

    /* TODO Eventually we should use this information to have
     * DRI3GetSupportedModifiers return device-specific modifiers, but for now
     * we will ignore it until multi-device support is more complete.
     * Otherwise we can't advertise support for DRI3 1.4.
     */



    /* XXX cepelinas9000: this is not correct implementation - it only set enough to return selected device when dri3_open called */
    /* XXX cepelinas9000: this is only for drm devices (modesetting for now) */
    /* XXX cepelinas9000: probaly glx provider should be set too  */


    if (drm_device_matches(window->drawable.pScreen,stuff->drmMajor,stuff->drmMinor)){
        dri3_client_private_ptr ptr = dri3_get_or_create_client_private(client);
        ptr->active_screen  = window->drawable.pScreen;
        return Success;
    }

    ScreenPtr secondary;
    xorg_list_for_each_entry(secondary,&window->drawable.pScreen->secondary_list, secondary_head) {
        if (!secondary->is_offload_secondary){
            continue;
        }


        if (drm_device_matches(secondary,stuff->drmMajor,stuff->drmMinor)){
            dri3_client_private_ptr ptr = dri3_get_or_create_client_private(client);
            ptr->active_screen = secondary;
            return Success;
        }

    }

    return BadMatch;
}

static int
proc_dri3_import_syncobj(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3ImportSyncobjReq);
    X_REQUEST_FIELD_CARD32(syncobj);
    X_REQUEST_FIELD_CARD32(drawable);

    DrawablePtr drawable;
    ScreenPtr screen;
    int fd;
    int status;

    SetReqFds(client, 1);
    LEGAL_NEW_RESOURCE(stuff->syncobj, client);

    status = dixLookupDrawable(&drawable, stuff->drawable, client,
                               M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;

    screen = drawable->pScreen;

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    return dri3_import_syncobj(client, screen, stuff->syncobj, fd);
}

static int
proc_dri3_free_syncobj(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3FreeSyncobjReq);
    X_REQUEST_FIELD_CARD32(syncobj);

    struct dri3_syncobj *syncobj;
    int status;

    status = dixLookupResourceByType((void **) &syncobj, stuff->syncobj,
                                     dri3_syncobj_type, client, DixWriteAccess);
    if (status != Success)
        return status;

    FreeResource(stuff->syncobj, RT_NONE);
    return Success;
}


static void write_gpu_UserPreferencesGPUDevice(x_rpcbuf_t *rpcbuf,ScreenPtr scr,ClientPtr client,bool prefered){

    xDRI3GetDeviceUserPreferencesGPUDevice reply = { 0 };


    rrScrPrivPtr pScrPriv = rrGetScrPriv(scr);
    RRProviderPtr pp = pScrPriv->provider;

    dri3_screen_priv_ptr        ds = dri3_screen_priv(scr);
    const dri3_screen_info_rec  *info = ds->info;

    const char *vendor = "";
    const char *friendly_name = "";

    if (info->vendor_library){
        vendor = info->vendor_library(client,scr,pScrPriv->provider);
    } else {
       /* guess vendor from dirver name, other driver can have callback only nvidia propertary  */

       ScrnInfoPtr scrn = xf86ScreenToScrn(scr);
       vendor = scrn->confScreen->device->driver;
    }

    reply.vendor_name_len = bytes_to_int32(strlen(vendor));

    reply.provider = pp->id;
    reply.provider_name_len = bytes_to_int32(pScrPriv->provider->nameLength+1);

    if (pp->drmDevice){
        reply.drmMajor = pp->drmMajor;
        reply.drmMinor = pp->drmMinor;
    } else {
        reply.struct_flags |= xDRI3GetDeviceUserPreferences_struct_flag_NOTDRM;
    }

    if (scr->current_primary == NULL){ /* primary screen - mark as desktop render, XXX: maybe someday for cases when there "no penalty" allow multiple devices */
        reply.usage_flags |=   xDRI3GetDeviceUserPreferences_flag_DESKTOPRENDER;
    }


    ScrnInfoPtr pScrn = xf86ScreenToScrn(scr);

    BusRec br = xf86EntityGetBusAddress(pScrn->entityList[0]);

    char device_location_buf[64] = {0};

    if (br.type == BUS_PCI){
        int device_location_buf_len = 0;
        device_location_buf_len = snprintf(device_location_buf,63,"pci-%04x:%02x:%02x.%d",br.id.pci->domain,br.id.pci->bus,br.id.pci->dev,br.id.pci->func);
        reply.device_location_len = bytes_to_int32(device_location_buf_len + 1);
    } else if (br.type == BUS_PLATFORM){
       int device_location_buf_len = 0;
       device_location_buf_len = snprintf(device_location_buf,63,"%s",xf86EntityBusRecGetPlatformBus(&br));
       if (strncmp(device_location_buf,"pci:",4)== 0){
          device_location_buf[3]='-';
       }
       for(int i=4;i<device_location_buf_len;++i){
           if (device_location_buf[i] == ':' || device_location_buf[i] == '.') {
               device_location_buf[i] = '_';
           }
       }
       reply.device_location_len = bytes_to_int32(device_location_buf_len + 1);
       
    }


    if (pp->friendlyName){
        friendly_name = pp->friendlyName;
        reply.device_friendly_name_len = bytes_to_int32(strlen(pp->friendlyName));

    } else {
        friendly_name = pScrPriv->provider->name;
        reply.device_friendly_name_len =bytes_to_int32(pScrPriv->provider->nameLength+1); /* use as provider name */
    }



    reply.vendor_name_offset = bytes_to_int32(sizeof(xDRI3GetDeviceUserPreferencesGPUDevice));
    reply.provider_name_offset = reply.vendor_name_offset + reply.vendor_name_len;
    reply.device_location_offset = reply.provider_name_offset + reply.provider_name_len;
    reply.device_friendly_name_offset = reply.device_location_offset + reply.device_location_len;

    reply.length = bytes_to_int32(sizeof(xDRI3GetDeviceUserPreferencesGPUDevice))  + reply.vendor_name_len + reply.provider_name_len + reply.device_location_len + reply.device_friendly_name_len;

    if (prefered) {
       reply.prefer |= xDRI3GetDeviceUserPreferences_prefer_PREFER;
    }

    X_REPLY_FIELD_CARD32(length);
    X_REPLY_FIELD_CARD32(struct_flags);
    X_REPLY_FIELD_CARD32(provider);

    X_REPLY_FIELD_CARD32(drmMajor);
    X_REPLY_FIELD_CARD32(drmMinor);

    X_REPLY_FIELD_CARD16(vendor_name_len);
    X_REPLY_FIELD_CARD16(vendor_name_offset);

    X_REPLY_FIELD_CARD16(provider_name_len);
    X_REPLY_FIELD_CARD16(provider_name_offset);

    X_REPLY_FIELD_CARD16(device_location_len);
    X_REPLY_FIELD_CARD16(device_location_offset);

    X_REPLY_FIELD_CARD16(device_friendly_name_len);
    X_REPLY_FIELD_CARD16(device_friendly_name_offset);

    X_REPLY_FIELD_CARD32(pad); /* XXX pad XXX */

    X_REPLY_FIELD_CARD64(usage_flags);
    X_REPLY_FIELD_CARD64(prefer);


    x_rpcbuf_write_binary_pad(rpcbuf,&reply,sizeof(xDRI3GetDeviceUserPreferencesGPUDevice));

    x_rpcbuf_write_string_0t_pad(rpcbuf,vendor);
    x_rpcbuf_write_string_0t_pad(rpcbuf,pScrPriv->provider->name);
    x_rpcbuf_write_string_0t_pad(rpcbuf,device_location_buf);
    x_rpcbuf_write_string_0t_pad(rpcbuf,friendly_name);


}



/* here for now */
static int
proc_dri3_get_device_user_preferences(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3GetDeviceUserPreferencesReq);
    X_REQUEST_FIELD_CARD32(flags);
    X_REQUEST_FIELD_CARD32(drawable);


    DrawablePtr drawable;

    int status = dixLookupDrawable(&drawable, stuff->drawable, client, 0, DixGetAttrAccess);
    if (status != Success)
        return status;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    ScreenPtr p = drawable->pScreen;


    PropertyPtr pProp;
    Atom prop_atom = dixAddAtom("XLIBRE_X11_XGPU");
    int rc= dixLookupProperty(&pProp, p->root, prop_atom, serverClient, DixReadAccess);
    /* XXX: handle if property deleted */
    ScreenPtr secondary;


    int prefferd_xgpu =0;

    int num_secondaries = 0;
     xorg_list_for_each_entry(secondary, &p->secondary_list, secondary_head) {
         if (!secondary->is_offload_secondary){
             continue;
         }

         num_secondaries++;
     }

    if (pProp->type == XA_INTEGER && pProp->format == 8){
        CARD8 *prop_data = (CARD8*)pProp->data;
        if (prop_data[0] >= 0 && prop_data[0] < (num_secondaries+1)){
            prefferd_xgpu = prop_data[0];
        }

    }

    int num_devices = 1;



    write_gpu_UserPreferencesGPUDevice(&rpcbuf,p,client,prefferd_xgpu == 0);

    xorg_list_for_each_entry(secondary, &p->secondary_list, secondary_head) {
        if (!secondary->is_offload_secondary){
            continue;
        }
        write_gpu_UserPreferencesGPUDevice(&rpcbuf,secondary,client,prefferd_xgpu == num_devices );
        num_devices++;

    }

    xDRI3GetDeviceUserPreferencesReply reply = {
        .num_devices = num_devices,
    };

    X_REPLY_FIELD_CARD32(num_devices);


    return X_SEND_REPLY_WITH_RPCBUF(client,reply,rpcbuf);
;
}


int
proc_dri3_dispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (!client->local)
        return BadMatch;

    switch (stuff->data) {
        case X_DRI3QueryVersion:
            return proc_dri3_query_version(client);
        case X_DRI3Open:
            return proc_dri3_open(client);
        case X_DRI3PixmapFromBuffer:
            return proc_dri3_pixmap_from_buffer(client);
        case X_DRI3BufferFromPixmap:
            return proc_dri3_buffer_from_pixmap(client);
        case X_DRI3FenceFromFD:
            return proc_dri3_fence_from_fd(client);
        case X_DRI3FDFromFence:
            return proc_dri3_fd_from_fence(client);

        /* v1.2 */
        case xDRI3GetSupportedModifiers:
            return proc_dri3_get_supported_modifiers(client);
        case xDRI3PixmapFromBuffers:
            return proc_dri3_pixmap_from_buffers(client);
        case xDRI3BuffersFromPixmap:
            return proc_dri3_buffers_from_pixmap(client);

        /* v1.3 */
        case xDRI3SetDRMDeviceInUse:
            return proc_dri3_set_drm_device_in_use(client);

        /* v1.4 */
        case xDRI3ImportSyncobj:
            return proc_dri3_import_syncobj(client);
        case xDRI3FreeSyncobj:
            return proc_dri3_free_syncobj(client);

        /* v1.5 XLibre*/
        case xDRI3GetDeviceUserPreferences:
            return proc_dri3_get_device_user_preferences(client);

        default:
            return BadRequest;
    }
}
