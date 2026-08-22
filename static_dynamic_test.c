#include "static_dynamic.h"
#include <stdio.h>
#include <sys/auxv.h>
#include "raylib.h"

#define RTLD_NOW 0x0002
typedef void* (*dlopen_fn)(const char *path, int flags);;
typedef void* (*dlsym_fn)(void *restrict handle, const char *restrict symbol);
typedef int   (*dlclose_fn)(void *handle);
typedef char* (*dlerror_fn)(void);

typedef int   (*printf_fn)(const char *restrict format, ...);
typedef float (*sinf_fn)(float);

thread_local u32   tl_a = 5;

int main(u64* argc_argv) {
  printf("musl-libc printf: argc_argv: %p", argc_argv);

  u64  argc = *argc_argv;
  u64* argv = argc_argv + 1;
  u64* got  = (u64*)*(argc_argv + argc);

  dlopen_fn  dlopen  = (dlopen_fn) got[0];
  dlsym_fn   dlsym   = (dlsym_fn)  got[1];
  dlclose_fn dlclose = (dlclose_fn)got[2];
  dlerror_fn dlerror = (dlerror_fn)got[3];

  void* libc   = dlopen("libc.so.6", RTLD_NOW);
  void *libm   = dlopen("libm.so.6", RTLD_NOW);
  void *raylib = dlopen("libraylib.so", RTLD_NOW);

  printf_fn printf2 = (printf_fn)dlsym(libc, "printf");

  printf2("dynamic libc argc %d\n", argc);
  for (u32 i = 0; i < argc - 1; i += 1) {
    printf2("argv[%d] %s\n", i, argv[i]);
  }
  printf2("thread local a %d\n", tl_a );

  printf2("libc.so is %p\n", libc);
  printf2("libm.so is %p\n", libm);
  printf2("raylib.so is %p\n", raylib);

  if (libm && raylib) {
    printf2("Demo time\n");

    float (*sinf)(float)                                   = dlsym(libm, "sinf");
    void  (*rlInitWindow)(int, int, const char*)           = dlsym(raylib, "InitWindow");
    void  (*rlSetTargetFPS)(int)                           = dlsym(raylib, "SetTargetFPS");
    bool  (*rlWindowShouldClose)(void)                     = dlsym(raylib, "WindowShouldClose");
    float (*rlGetFrameTime)(void)                          = dlsym(raylib, "GetFrameTime");
    void  (*rlBeginDrawing)(void)                          = dlsym(raylib, "BeginDrawing");
    void  (*rlClearBackground)(Color)                      = dlsym(raylib, "ClearBackground");
    void  (*rlDrawCircleV)(Vector2, float, Color)          = dlsym(raylib, "DrawCircleV");
    void  (*rlDrawText)(const char*, int, int, int, Color) = dlsym(raylib, "DrawText");
    void  (*rlEndDrawing)(void)                            = dlsym(raylib, "EndDrawing");
    void  (*rlCloseWindow)(void)                           = dlsym(raylib, "CloseWindow");

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
    dlclose(raylib);
  } else {
    printf2("Cannot load raylib, so no demo\n");
  }
  return 0;
}
