#include <criterion/criterion.h>

#include "../applyvulkanproperties.h"

#include "../hdrproto.h"

#include <vulkan/vulkan.h>

Test(applyvulkanproperter, test_1 ) {

    X11_HDR_VULKANSTRUCTCHAIN s;
    memset(&s,0,sizeof(s));

    s.current_lo_size =  42;
    s.total_lo_size = 43;

    HDR_vulkan_struct_header_pakced *p = (HDR_vulkan_struct_header_pakced *)(&s);

   // printf("%d\n",p->current_size);

    cr_expect_eq(p->current_size,s.current_lo_size);


}

