#include <criterion/criterion.h>

#include "../applyvulkanproperties.h"

#include "../hdrproto.h"

#include <vulkan/vulkan.h>

Test(applyvulkanproperter, HDR_vulkan_struct_header_packed ) {

    X11_HDR_VULKANSTRUCTCHAIN s;
    memset(&s,0,sizeof(s));

    s.current_lo_size =  42;
    s.current_hi_size = 1;

    s.total_lo_size = 43;
    s.total_hi_size = 1;

    s.structflags = 0xffff;
    s.vkStructure = 0x80000001;


    HDR_vulkan_struct_header_pakced *p = (HDR_vulkan_struct_header_pakced *)(&s);

    cr_expect_eq(p->current_size,s.current_lo_size + (s.current_hi_size << 16));
    cr_expect_eq(p->total_size,s.total_lo_size + (s.total_hi_size  << 16));


  cr_expect_eq(p->vkStructure,0x80000001);
  cr_expect_eq(p->struct_flags,0xffff);

}

Test(applyvulkanproperter, hdr_verify_and_get_size_of_single_chain){


    uint8_t buf[1024];

    union {
      VkSurfaceFormat2KHR s;
      X11_HDR_VULKANSTRUCTCHAIN x11;
      HDR_vulkan_struct_header_pakced px11;
    } to_test;

  to_test.s.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
  to_test.s.surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
  to_test.s.surfaceFormat.colorSpace =  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;


  /* check for sanity */
  cr_expect_eq(sizeof(to_test),sizeof(VkSurfaceFormat2KHR));

  to_test.px11.struct_flags = 0;
  to_test.px11.current_size = sizeof(VkSurfaceFormat2KHR) >> 2;
  to_test.px11.total_size = sizeof(VkSurfaceFormat2KHR) >> 2;

  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  memset(&to_test,0xaa,sizeof(to_test));
  memcpy(&to_test,buf,sizeof(VkSurfaceFormat2KHR));

  cr_expect_eq(to_test.s.sType,VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR);
  cr_expect_eq(to_test.s.surfaceFormat.format,VK_FORMAT_B8G8R8A8_UNORM);
  cr_expect_eq(to_test.s.surfaceFormat.colorSpace,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);


  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,1024) == (sizeof(VkSurfaceFormat2KHR)/4));

  // current_size > total_size
  to_test.px11.current_size = 8;
  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,1024) == 0);

  to_test.px11.current_size = 8;
  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,1024) == 0);

  /* too small struct sizes */
  for(int i=0;i<3;++i){
      to_test.px11.current_size =i;
      memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
      cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,1024) == 0);
  }

  /* 1st structure passes, 2nd one is too small to consider */
  to_test.px11.current_size =4;
  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,1024) == 0);

  to_test.px11.current_size =5;
  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,1024) == 0);


  to_test.px11.current_size =6;
  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,6) == 6);

  /* buffer too small */
  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR));
  cr_assert(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,5) == 0);


}

Test(applyvulkanproperter, hdr_verify_and_get_size_of_chained){


    uint8_t buf[1024];
    struct {
        union {
            VkSurfaceFormat2KHR s1_s;
            X11_HDR_VULKANSTRUCTCHAIN s1_x11;
            HDR_vulkan_struct_header_pakced s1_px11;
        };
        union {
            VkImageCompressionPropertiesEXT s2_s;
            X11_HDR_VULKANSTRUCTCHAIN s2_x11;
            HDR_vulkan_struct_header_pakced s2_px11;
        };

    } to_test;

  /* check for sanity */
  cr_expect_eq(sizeof(to_test),sizeof(VkSurfaceFormat2KHR) + sizeof(VkImageCompressionPropertiesEXT));

  to_test.s1_s.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
  to_test.s1_s.surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
  to_test.s1_s.surfaceFormat.colorSpace =  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

  to_test.s2_s.sType =  VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT;
  to_test.s2_s.imageCompressionFlags = 0x42424242;
  to_test.s2_s.imageCompressionFixedRateFlags = 0xbdbdbdbd;


  /* fill sizes */
  to_test.s1_px11.struct_flags = 0;
  to_test.s1_px11.current_size = sizeof(VkSurfaceFormat2KHR) / 4;
  to_test.s1_px11.total_size = (sizeof(VkSurfaceFormat2KHR) + sizeof(VkImageCompressionPropertiesEXT) ) / 4;


  to_test.s2_px11.struct_flags = 0;
  to_test.s2_px11.current_size = sizeof(VkImageCompressionPropertiesEXT)  / 4;
  to_test.s2_px11.total_size = sizeof(VkImageCompressionPropertiesEXT)  / 4;


  memcpy(buf,&to_test,sizeof(VkSurfaceFormat2KHR) + sizeof(VkImageCompressionPropertiesEXT));

  cr_assert_eq(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,256),(sizeof(VkSurfaceFormat2KHR) + sizeof(VkImageCompressionPropertiesEXT) ) / 4);

  cr_assert_eq(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,12),(sizeof(VkSurfaceFormat2KHR) + sizeof(VkImageCompressionPropertiesEXT) ) / 4);

  cr_assert_eq(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,13),(sizeof(VkSurfaceFormat2KHR) + sizeof(VkImageCompressionPropertiesEXT) ) / 4);


  memset(buf,0xff,sizeof(buf));
  cr_assert_eq(hdr_verify_and_get_size_of_single_chain((uint32_t*)buf,256),0);

}

