/**
  Xlibre specific dri3 protocol extensions
**/
#ifndef _DRI3_XLIBRE_PROTO_H_
#define _DRI3_XLIBRE_PROTO_H_

#include <X11/Xmd.h>
#include <X11/extensions/dri3proto.h>


/* v1.5 */

/**  \brief xDRI3GetDeviceUserPreferences get render devices "preferences"
 *  This request return list of known X provider devices with aditional flags. The main purpose is for multi gpu system to choice appropriate device automatic and consistent way
 *  Aditionally this would enable support envirioment variables like `X11_PREFER_VENDOR`/`X11_PREFER_XGPU` for 'simple' gpu selection.
**/


#define xDRI3GetDeviceUserPreferences 12

typedef struct {
    CARD8       reqType;
    CARD8       dri3ReqType;
    CARD16      length;
    CARD32      flags;
    CARD32      drawable;
} xDRI3GetDeviceUserPreferencesReq;

/**
 reply to xDRI3GetDeviceUserPreferences call.
 It responds with xDRI3GetDeviceUserPreferencesReply and with num_devices*(xDRI3GetDeviceUserPreferencesGPUDevice+strings)
*/
typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  num_devices; //! number of gpu devices
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} xDRI3GetDeviceUserPreferencesReply;

/**
 * \brief xDRI3GetDeviceUserPreferencesGPUDevice single device response (part of xDRI3GetDeviceUserPreferencesReply)
 * It provider enough information to set appropriate gpu device (ex. XID name string - it for NVIDIA propertary driver)
 *
 * Response is following:
 * [ xDRI3GetDeviceUserPreferencesGPUDevice + [vendor_name x card32 size units] + [provider_name x card32 size units] + [device_location_len x card32 size units] + [device_friendly_name x card32 size units]
 */

typedef struct {
  CARD32 length;                     /** record length in 4 bytes units including strings **/
  CARD32 struct_flags;               /** structure flags **/

  CARD32 provider;

  CARD32 drmMajor;                   /** it can be '0' if it is not drm device **/
  CARD32 drmMinor;                   /** it can be '0' if it is not drm device **/

  CARD16 vendor_name_len;            /** vendor name length in 4 bytes chunks, string always ends with \0 **/
  CARD16 vendor_name_offset;         /** vender name offset starting from structure beggining **/

  CARD16 provider_name_len;          /** provider name length in 4 bytes chunks, string always end with \0 (value of xid provider) **/
  CARD16 provider_name_offset;       /** provider name offset starting from structure beggining **/

  CARD16 device_location_len;        /** device location length, it format pci-XXXX_XX_XX_X (https://docs.mesa3d.org/envvars.html#envvar-DRI_PRIME format) or similar **/
  CARD16 device_location_offset;     /** device location offset, starting from structure beggining **/

  CARD16 device_friendly_name_len;   /** for example :"NVIDIA GeForce RTX 4080 SUPER/PCIe/SSE2 @ pci-XXXX_XX_XX_X" */
  CARD16 device_friendly_name_offset;/** device_friendly_name offset, starting from structure beggining */

  /* EXT_device_persistent_id - in theory worth, but from quick glance only in recent nvidia drivers*/
  CARD16 device_uuid_len_card8;
  CARD16 device_uuid_offset;         /** offset in card32 units **/

  CARD16 device_driver_uuid_len_card8;
  CARD16 device_driver_uuid_offset; /** offset in card32 units **/

  CARD32 pad;
  CARD64 usage_flags;              /** device usage flags */
  CARD64 prefer;                   /** device preference flags */
} xDRI3GetDeviceUserPreferencesGPUDevice;

#define sz_xDRI3GetDeviceUserPreferencesGPUDevice	64
#define szmin_xDRI3GetDeviceUserPreferencesGPUDevice (sz_xDRI3GetDeviceUserPreferencesGPUDevice + 16)

#define xDRI3GetDeviceUserPreferences_struct_flag_NOTDRM 1   //! drmMajor, drmMinor doesn't contains /dev/dri/* node minor:major numbers

#define xDRI3GetDeviceUserPreferences_flag_DESKTOPRENDER 1 //! There are no/little penalty to process windows pixmaps on this device (for example you can run compositor here)
#define xDRI3GetDeviceUserPreferences_flag_XLIBRE_RENDER 2 //! device uses glamor render or similar (code fully controlled and contained by XLibre)

#define xDRI3GetDeviceUserPreferences_prefer_PREFER 1      //! this device is prefered for graphics context

#endif
