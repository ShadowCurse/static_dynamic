#include "util.h"

typedef int   (*pthread_create_fn)(sd_u64*, void*, void* (*)(void*), void*);
typedef int   (*pthread_join_fn)(sd_u64, void**);
typedef int*  (*errno_location_fn)(void);
typedef int   (*open_fn)(const char*, int);
typedef void* (*malloc_fn)(sd_u64);
typedef void  (*free_fn)(void*);

_Thread_local sd_u32 tl_u32 = 5;
_Thread_local sd_u64 tl_u64[4] = {0xc0ffee, 0xc0ffee1, 0xc0ffee2, 0xc0ffee3};

static errno_location_fn g_errno;
static open_fn           g_open;
static malloc_fn         g_malloc;
static free_fn           g_free;
static sd_u64            main_tp;

static void* body(void* arg) {
  (void)arg;

  sd_u64 tp = get_thread_pointer();
  ASSERT(tp != 0);
  ASSERT(tp != main_tp);

  ASSERT_EQ(tl_u32, 5);
  ASSERT_EQ(tl_u64[0], 0xc0ffee);
  ASSERT_EQ(tl_u64[3], 0xc0ffee3);
  tl_u32 = 100;

  int* e = g_errno();
  ASSERT(e != 0);
  g_open("blah", 0);
  ASSERT_EQ(*e, 2) // ENOENT

  char* p = (char*)g_malloc(4096);
  ASSERT(p != 0);
  p[4095] = 1;
  ASSERT_EQ(p[4095], 1);
  g_free(p);

  return 0;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  ASSERT(sd_got.success != 0);

  void* libc = sd_got.dlopen("libc.so.6", SD_RTLD_NOW);
  ASSERT(libc != 0);

  pthread_create_fn create = (pthread_create_fn)sd_got.dlsym(libc, "pthread_create");
  pthread_join_fn   join   = (pthread_join_fn)sd_got.dlsym(libc, "pthread_join");
  g_errno  = (errno_location_fn)sd_got.dlsym(libc, "__errno_location");
  g_open   = (open_fn)          sd_got.dlsym(libc, "open");
  g_malloc = (malloc_fn)        sd_got.dlsym(libc, "malloc");
  g_free   = (free_fn)          sd_got.dlsym(libc, "free");
  ASSERT(create   != 0);
  ASSERT(join     != 0);
  ASSERT(g_errno  != 0);
  ASSERT(g_open   != 0);
  ASSERT(g_malloc != 0);
  ASSERT(g_free   != 0);

  main_tp = get_thread_pointer();
  tl_u32 = 7;

  sd_u64 tid = 0;
  ASSERT_EQ(create(&tid, 0, body, 0), 0);
  ASSERT_EQ(join(tid, 0), 0);

  ASSERT_EQ(tl_u32, 7);

  return 0;
}
