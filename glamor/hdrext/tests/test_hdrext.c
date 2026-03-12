#include <criterion/criterion.h>

#include <criterion/new/assert.h>

#include "hdrext/hdrext_priv.h"

void
FatalError(const char *f, ...)
{
    abort();
}


Test(HDRWindowPropert, Simple ) {

    HDR_SDRPARAMS_uniform_t t_defaults;
    HDR_SDRPARAMS_uniform_t t1;

    HDR_SDRPARAMS_uniform_t_init(&t_defaults);
    t_defaults.u_gamma= 99;

    memset(&t1,0,sizeof(t1));


  bool ret = HDR_parseSDRParams_parsePropertyStr("1.1,1.2,1.3,1.4,0,1.6",&t_defaults,&t1);
  cr_expect(eq(int,ret,true));

  cr_expect(epsilon_eq(t1.u_whitepoint_ref,1.1,0.01));
  cr_expect(epsilon_eq(t1.u_contrast,1.2,0.001));
  cr_expect(epsilon_eq(t1.u_saturation,1.3,0.001));
  cr_expect(epsilon_eq(t1.u_hue,1.4,0.001));
  cr_expect(epsilon_eq(t1.u_brightness,0,0.001));
  cr_expect(epsilon_eq(t1.u_gamma,1.6,0.001));


  ret = HDR_parseSDRParams_parsePropertyStr("400,*,*,*,*,default",&t_defaults,&t1);

  cr_expect(eq(int,ret,true));

  cr_expect(epsilon_eq(t1.u_whitepoint_ref,400,0.01));
  cr_expect(epsilon_eq(t1.u_contrast,1.2,0.001));
  cr_expect(epsilon_eq(t1.u_saturation,1.3,0.001));
  cr_expect(epsilon_eq(t1.u_hue,1.4,0.001));
  cr_expect(epsilon_eq(t1.u_brightness,0,0.001));
  cr_expect(epsilon_eq(t1.u_gamma,99,0.001));


}


Test(HDRWindowPropert, invalidInput ) {
    HDR_SDRPARAMS_uniform_t t_defaults;
    HDR_SDRPARAMS_uniform_t t1;

    HDR_SDRPARAMS_uniform_t_init(&t_defaults);
    t1 = t_defaults;

    bool ret = HDR_parseSDRParams_parsePropertyStr(",,,,,",&t_defaults,&t1);
    cr_expect(eq(int,ret,false));

    ret = HDR_parseSDRParams_parsePropertyStr("420,,,,,",&t_defaults,&t1);
    cr_expect(eq(int,ret,false));

    cr_expect(epsilon_eq(t1.u_whitepoint_ref,t_defaults.u_whitepoint_ref,0.01));


}
