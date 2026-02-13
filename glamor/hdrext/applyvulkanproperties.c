#include "applyvulkanproperties.h"

size_t hdr_verify_and_get_size_of_single_chain(uint32_t *start, size_t len_card32)
{

    size_t offset = 0;
    bool cont = false;
    do {

        if ((len_card32-offset) < HDR_ListOfVULKANSTRUCTCHAIN_MIN_LEN_IN_CARD32){
            return 0;
        }

        HDR_vulkan_struct_header_pakced  * restrict px11 = (HDR_vulkan_struct_header_pakced  * restrict)&start[offset];

        /* check minimum size constraint */
        if ( (px11->current_size < HDR_ListOfVULKANSTRUCTCHAIN_MIN_LEN_IN_CARD32) || ((px11->total_size < HDR_ListOfVULKANSTRUCTCHAIN_MIN_LEN_IN_CARD32)) ){
            return 0;
        }

        /* check if total_size more than current_size field and structure in buffer */
        if ((px11->total_size < px11->current_size) || ((offset + px11->current_size) > len_card32)) {
            return 0;
        };

        offset+=px11->current_size;

        cont = px11->total_size > px11->current_size;

    } while ((offset < len_card32) && (cont));

    return offset;
}

bool hdr_verify_all_chains(uint32_t *start, size_t len_card32)
{

  size_t offset = 0;
  do {

    size_t ret = hdr_verify_and_get_size_of_single_chain(&start[offset],len_card32 - offset);

    if (ret == 0) {
        return false;
    }

  offset+=ret;
  } while (offset < len_card32);
  return true;
}
