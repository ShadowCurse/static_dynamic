#include "../static_dynamic.h"

#if defined(__x86_64__)
  #define SYS_write      1
  #define SYS_exigroup 231
#elif defined(__aarch64__)
  #define SYS_write      64
  #define SYS_exigroup 94
#endif

#define ASSERT(cond)  {                                                  \
                        if (!(cond)) panic(__FILE__, __LINE__, 0, 0, 0); \
                      }
#define ASSERT_EQ(a, b) {                                                                 \
                          sd_u64 a_u64 = (sd_u64)(a);                                     \
                          sd_u64 b_u64 = (sd_u64)(b);                                     \
                          if (a_u64 != b_u64) panic(__FILE__, __LINE__, a_u64, b_u64, 1); \
                        }

static void my_puts(const char* s) {
  sd_syscall3(SYS_write, 2, (sd_u64)s, (sd_u64)sd_strlen(s));
}

// `base` is 10 or 16, and 16 gets a `0x` prefix.
static void putnum(sd_u64 v, sd_u32 base) {
  char digits[20];
  sd_u32 n = 0;
  do {
    digits[n] = "0123456789abcdef"[v % base];
    v /= base;
    n += 1;
  } while (v != 0);

  char buf[24];
  sd_u32 i = 0;
  if (base == 16) {
    buf[i] = '0'; i += 1;
    buf[i] = 'x'; i += 1;
  }
  while (n != 0) {
    n -= 1;
    buf[i] = digits[n];
    i += 1;
  }
  sd_syscall3(SYS_write, 2, (sd_u64)buf, (sd_u64)i);
}

__attribute__((noreturn))
static void panic(const char* file, sd_u32 line, sd_u64 got, sd_u64 want, int have_values) {
  my_puts(file);
  my_puts(":");
  putnum(line, 10);
  if (have_values) {
    my_puts(" expected ");
    putnum(want, 16);
    my_puts(", got ");
    putnum(got, 16);
  }
  my_puts(" assertion failed\n");
  sd_syscall1(SYS_exigroup, 1);
  __builtin_unreachable();
}

__attribute__((unused))
static sd_u64 get_thread_pointer(void) {
#if defined(__x86_64__)
  sd_u64 tp = 0;
  sd_syscall2(SD_SYS_arch_prctl, 0x1003 /* ARCH_GEFS */, (sd_u64)&tp);
  return tp;
#elif defined(__aarch64__)
  sd_u64 tp;
  __asm__ __volatile__("mrs %0, tpidr_el0" : "=r"(tp));
  return tp;
#endif
}
