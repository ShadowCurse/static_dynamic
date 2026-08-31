#include "util.h"

_Thread_local sd_u32 tl_u32 = 5;
_Thread_local sd_u64 tl_u64[4] = {0xc0ffee, 0xc0ffee1, 0xc0ffee2, 0xc0ffee3};
_Thread_local sd_u64 tl_bss[8];

int main(int argc, char** argv) {
  (void)argv;

  ASSERT_EQ(sd_got.success, 0);
  ASSERT_EQ(sd_got.error, SD_ERROR_NO_BOUNCE_BINARY);
  ASSERT_EQ(get_thread_pointer(), 0);

  // This should set up TLS
  sd_c_tls_init((sd_u64*)(argv - 1));

  sd_u64 tp = get_thread_pointer();
  ASSERT(tp != 0);

  ASSERT_EQ(tl_u32,    5);
  ASSERT_EQ(tl_u64[0], 0xc0ffee);
  ASSERT_EQ(tl_u64[3], 0xc0ffee3);
  for (sd_u32 i = 0; i < 8; i += 1) {
    ASSERT_EQ(tl_bss[i], 0);
  }

  tl_u32 = 69;
  tl_u32 += 1;
  ASSERT_EQ(tl_u32, 70);

  tl_bss[1] = 0xbeef;
  ASSERT_EQ(tl_bss[1], 0xbeef);

  return 0;
}
