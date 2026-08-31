#include "util.h"

_Thread_local sd_u32 tl_u32 = 5;

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  // Assuming linker loading failed
  ASSERT_EQ(sd_got.success, 0);
  ASSERT_EQ(get_thread_pointer(), 0);

  // Since `sd_c_tls_init` was not called, expect `SIGSEGV`
  ASSERT_EQ(tl_u32, 5);

  return 1;
}
