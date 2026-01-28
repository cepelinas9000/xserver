#include <dix-config.h>

#include "hdrext.h"

#include "privates.h"

#include "extension.h"
#include "extnsionst.h"

#include "request_priv.h"

#include "hdrproto.h"

#include "hdrext_priv.h"

/*
static DevPrivateKeyRec HDRScrPrivateKeyRec;
#define HDRScrPrivateKey (&HDRScrPrivateKeyRec)

static DevPrivateKeyRec HDRPixPrivateKeyRec;
#define HDRPixPrivateKey (&HDRPixPrivateKeyRec)

static DevPrivateKeyRec HDRWinPrivateKeyRec;
#define HDRWinPrivateKey (&HDRWinPrivateKeyRec)


static DevPrivateKeyRec HDRClientPrivateKeyRec;
#define HDRClientPrivateKey (&HDRClientPrivateKeyRec)
*/

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
        case XHDR_HDRQueryVersion      : return ProcHDRQueryVersion(client); break;
        case XHDR_ApplyVulkanProperties: return ProcHDRApplyVulkanProperties(client); break;
    }

    return BadRequest;
}

void
HDRExtensionInit(void){

    AddExtension(HDREXT_NAME, 0, 0,
                 ProcHDRDispatch, ProcHDRDispatch,
                 NULL, StandardMinorOpcode);



}
