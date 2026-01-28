#ifndef __applyvulkanproperties__h_
#define __applyvulkanproperties__h_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hdrext.h"

#define HDR_ListOfVULKANSTRUCTCHAIN_MIN_LEN_IN_CARD64 2

/**
 * @brief hdr_verify_and_get_size_of_single_chain verify single ListOfVULKANSTRUCTCHAIN structure chain
 * This function checks for size fields to be in bounds
 * @param start
 * @param len
 * @return start of next hdr chain or 0 if error
 * TODO: rename if
 */
size_t hdr_verify_and_get_size_of_single_chain(uint64_t *start,size_t len);


#endif
