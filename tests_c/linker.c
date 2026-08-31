#include "util.h"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  ASSERT(sd_got.success != 0);
  ASSERT(sd_got.dlopen != 0);
  ASSERT(sd_got.dlsym != 0);
  ASSERT(sd_got.dlclose != 0);
  ASSERT(sd_got.dlerror != 0);

  void* libc = sd_got.dlopen("libc.so.6", SD_RTLD_NOW);
  ASSERT(libc != 0);
  ASSERT(sd_got.dlsym(libc, "printf") != 0);

  ASSERT(sd_got.dlsym(libc, "blah") == 0);
  char* err = sd_got.dlerror();
  ASSERT(err != 0);
  ASSERT(sd_got.dlerror() == 0);

  ASSERT(sd_got.dlopen("blah.so", SD_RTLD_NOW) == 0);
  ASSERT(sd_got.dlerror() != 0);

  return 0;
}
