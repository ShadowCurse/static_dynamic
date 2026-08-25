#include "static_dynamic.h"
#include <stdio.h>
#include <sys/auxv.h>
#include "raylib.h"

typedef int   (*printf_fn)(const char *restrict format, ...);
typedef float (*sinf_fn)(float);

_Thread_local sd_u32 tl_a = 5;

static inline int my_write(sd_u64 fd, void* buf, sd_u64 len) {
  return (int)sd_syscall3(SYS_write, fd, (sd_u64)buf, (sd_u64)len);
}

int main(int argc, char** argv) {
#if defined(SD_MUSL)
  if (!sd_got.success) {
    printf("error during linker loading: %d\n", sd_got.error);
    return 1;
  }
  printf("sd_got dlopen:  %p\n", sd_got.dlopen);
  printf("sd_got dlsym:   %p\n", sd_got.dlsym);
  printf("sd_got dlclose: %p\n", sd_got.dlclose);
  printf("sd_got dlerror: %p\n", sd_got.dlerror);
#else
  if (!sd_got.success) {
    my_write(0, "error during linker loading, aborting\n", 38);
    return 1;
  }
#endif

  void* libc   = sd_got.dlopen("libc.so.6", SD_RTLD_NOW);
  void *libm   = sd_got.dlopen("libm.so.6", SD_RTLD_NOW);
  void *raylib = sd_got.dlopen("libraylib.so", SD_RTLD_NOW);

  printf_fn printf2 = (printf_fn)sd_got.dlsym(libc, "printf");

  printf2("dynamic libc argc %d\n", argc);
  for (sd_u32 i = 0; i < argc; i += 1) {
    printf2("argv[%d] %s\n", i, argv[i]);
  }
  printf2("thread local a %d\n", tl_a );

  printf2("libc.so is %p\n", libc);
  printf2("libm.so is %p\n", libm);
  printf2("raylib.so is %p\n", raylib);

  void *libfoo = sd_got.dlopen("libfoo.so", SD_RTLD_NOW);
  printf2("libfoo: %s\n", sd_got.dlerror());

  if (libm && raylib) {
    printf2("Demo time\n");

    float (*sinf)(float)                                   = sd_got.dlsym(libm, "sinf");
    void  (*rlInitWindow)(int, int, const char*)           = sd_got.dlsym(raylib, "InitWindow");
    void  (*rlSetTargetFPS)(int)                           = sd_got.dlsym(raylib, "SetTargetFPS");
    bool  (*rlWindowShouldClose)(void)                     = sd_got.dlsym(raylib, "WindowShouldClose");
    float (*rlGetFrameTime)(void)                          = sd_got.dlsym(raylib, "GetFrameTime");
    void  (*rlBeginDrawing)(void)                          = sd_got.dlsym(raylib, "BeginDrawing");
    void  (*rlClearBackground)(Color)                      = sd_got.dlsym(raylib, "ClearBackground");
    void  (*rlDrawCircleV)(Vector2, float, Color)          = sd_got.dlsym(raylib, "DrawCircleV");
    void  (*rlDrawText)(const char*, int, int, int, Color) = sd_got.dlsym(raylib, "DrawText");
    void  (*rlEndDrawing)(void)                            = sd_got.dlsym(raylib, "EndDrawing");
    void  (*rlCloseWindow)(void)                           = sd_got.dlsym(raylib, "CloseWindow");

    rlInitWindow(1280, 720, "test");
    rlSetTargetFPS(60);

    float   t            = 0.0f;
    float   d            = 1.0f;
    float   left_border  = 10.0f;
    float   right_border = 1260.0f;
    Vector2 p            = {left_border, 200.0f};

    while (!rlWindowShouldClose()) {
      float dt = rlGetFrameTime();

      t += dt;
      p.x += d * 200.0f * dt;
      if (right_border < p.x) {
        d  *= -1.0f;
        p.x = right_border;
      }
      if (p.x < left_border) {
        d  *= -1.0f;
        p.x = left_border;
      }
      p.y = sinf(20.0f * t) + 700.0;

      rlBeginDrawing();
        rlClearBackground(BLACK);
        rlDrawCircleV(p, 10.0f, RED);
        rlDrawText("static_dynamic demo", 120.0f, 250.0f, 100.0f, RED);
      rlEndDrawing();
    }
    rlCloseWindow();
    printf2("End of the demo\n");
    sd_got.dlclose(raylib);
  } else {
    printf2("Cannot load raylib, so no demo\n");
  }
  return 0;
}
