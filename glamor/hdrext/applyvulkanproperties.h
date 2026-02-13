#ifndef __applyvulkanproperties__h_
#define __applyvulkanproperties__h_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hdrext.h"

/**
 * @brief minimum possible vulkan structure size
 * minimum size is 12 bytes (sType,pNext,uint32_t) shoudn't be smaller structs
 */
#define HDR_ListOfVULKANSTRUCTCHAIN_MIN_LEN_IN_CARD32 3

/**
 * @brief hdr_verify_and_get_size_of_single_chain verify single CHain of ListOfVULKANSTRUCTCHAIN structure chain
 * It checks this and all children structures are in bounds
 * @param start
 * @param len_card32 - length in card32 units
 * @return start of next hdr chain or 0 if error
 * TODO: rename if
 */
size_t hdr_verify_and_get_size_of_single_chain(uint32_t *start, size_t len_card32);


/**
 * @brief hdr_very_all_chains verify incoming ListOfVULKANSTRUCTCHAINS
 * @param start
 * @param len_card32
 * @return true chains in
 */
bool hdr_verify_all_chains(uint32_t *start,size_t len_card32);

#endif
