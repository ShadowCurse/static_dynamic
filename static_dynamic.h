#include <elf.h>          // Elf64_{Ehdr,Phdr,auxv_t}
#include <fcntl.h>        // O_{RDONLY,CLOEXEC}
#include <sys/mman.h>     // MAP_{FIXED,PRIVATE,ANONYMOUS}, PROT_{NONE,READ,WRITE,EXEC}
#include <sys/syscall.h>  // SYS_*

typedef signed long        sd_i32;
typedef signed long long   sd_i64;
typedef unsigned char      sd_u8;
typedef unsigned           sd_u32;
typedef unsigned long long sd_u64;

#define SD_RTLD_LAZY 0x0001
#define SD_RTLD_NOW  0x0002
typedef void* (*sd_dlopen_fn)(const char *path, int flags);;
typedef void* (*sd_dlsym_fn)(void *restrict handle, const char *restrict symbol);
typedef int   (*sd_dlclose_fn)(void *handle);
typedef char* (*sd_dlerror_fn)(void);

typedef enum sd_u64 {
  SD_ERROR_NONE,
  SD_ERROR_NO_BOUNCE_BINARY,
  SD_ERROR_CANNOT_READ_BOUNCE_BINARY,
  SD_ERROR_NO_LINKER_PATH,
  SD_ERROR_NO_LINKER,
  SD_ERROR_CANNOT_LOAD_LINKER,
} sd_error;

typedef union {
  struct {
    sd_dlopen_fn  dlopen;
    sd_dlsym_fn   dlsym;
    sd_dlclose_fn dlclose;
    sd_dlerror_fn dlerror;
  };
  struct {
    sd_u64        success;
    sd_error      error;
    sd_u64        _padding1;
    sd_u64        _padding2;
  };
} sd_got_t;

// This global acts a role of a `got` table (hence the name), so any references to the `got`
// down bellow are references for this global. Here the linker will write
// function pointers from the loaded `libc`.
sd_got_t sd_got;

extern int main(int argc, char** argv);

// ------------------------------------------------

#if defined(__x86_64__)
  static inline sd_u64 sd_syscall1(sd_u64 n, sd_u64 a0) {
    sd_u64 result;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0) : "rcx", "r11", "memory");
    return result;
  }
  static inline sd_u64 sd_syscall2(sd_u64 n, sd_u64 a0, sd_u64 a1) {
    sd_u64 result;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1) : "rcx", "r11", "memory");
    return result;
  }
  static inline sd_u64 sd_syscall3(sd_u64 n, sd_u64 a0, sd_u64 a1, sd_u64 a2) {
    sd_u64 result;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1), "d"(a2): "rcx", "r11",
                         "memory");
    return result;
  }
  static inline sd_u64 sd_syscall4(sd_u64 n, sd_u64 a0, sd_u64 a1, sd_u64 a2, sd_u64 a3) {
    sd_u64 result;
    register sd_u64 r10 __asm__("r10") = a3;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1), "d"(a2), "r"(r10) :
                         "rcx", "r11", "memory");
    return result;
  }
  static inline sd_u64 sd_syscall6(sd_u64 n, sd_u64 a0, sd_u64 a1, sd_u64 a2, sd_u64 a3, sd_u64 a4, sd_u64 a5) {
    sd_u64 result;
    register sd_u64 r10 __asm__("r10") = a3;
    register sd_u64 r8  __asm__("r8") = a4;
    register sd_u64 r9  __asm__("r9") = a5;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1), "d"(a2), "r"(r10), "r"(r8),
                         "r"(r9) : "rcx", "r11", "memory");
    return result;
  }
#elif defined(__aarch64__)
  static inline sd_u64 sd_syscall1(sd_u64 n, sd_u64 a0) {
    register sd_u64 x8 __asm__("x8") = n;
    register sd_u64 x0 __asm__("x0") = a0;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
  }

  static inline sd_u64 sd_syscall2(sd_u64 n, sd_u64 a0, sd_u64 a1) {
    register sd_u64 x8 __asm__("x8") = n;
    register sd_u64 x0 __asm__("x0") = a0;
    register sd_u64 x1 __asm__("x1") = a1;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory");
    return x0;
  }

  static inline sd_u64 sd_syscall3(sd_u64 n, sd_u64 a0, sd_u64 a1, sd_u64 a2) {
    register sd_u64 x8 __asm__("x8") = n;
    register sd_u64 x0 __asm__("x0") = a0;
    register sd_u64 x1 __asm__("x1") = a1;
    register sd_u64 x2 __asm__("x2") = a2;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
  }

  static inline sd_u64 sd_syscall4(sd_u64 n, sd_u64 a0, sd_u64 a1, sd_u64 a2, sd_u64 a3) {
    register sd_u64 x8 __asm__("x8") = n;
    register sd_u64 x0 __asm__("x0") = a0;
    register sd_u64 x1 __asm__("x1") = a1;
    register sd_u64 x2 __asm__("x2") = a2;
    register sd_u64 x3 __asm__("x3") = a3;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
    return x0;
  }

  static inline sd_u64 sd_syscall6(sd_u64 n, sd_u64 a0, sd_u64 a1, sd_u64 a2, sd_u64 a3, sd_u64 a4, sd_u64 a5) {
    register sd_u64 x8 __asm__("x8") = n;
    register sd_u64 x0 __asm__("x0") = a0;
    register sd_u64 x1 __asm__("x1") = a1;
    register sd_u64 x2 __asm__("x2") = a2;
    register sd_u64 x3 __asm__("x3") = a3;
    register sd_u64 x4 __asm__("x4") = a4;
    register sd_u64 x5 __asm__("x5") = a5;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory");
    return x0;
  }
#endif

static inline int sd_open(const char* filename, int flags) {
  return (int)sd_syscall4(SYS_openat, (sd_u64)AT_FDCWD, (sd_u64)filename, (sd_u64)flags, (sd_u64)O_RDONLY);
}
static inline int sd_close(int fd) {
  return (int)sd_syscall1(SYS_close, (sd_u64)fd);
}
static inline sd_u64 sd_pread(int fd, void* buf, size_t count, sd_u64 offset) {
  return (sd_u64)sd_syscall4(SYS_pread64, (sd_u64)fd, (sd_u64)buf, (sd_u64)count, (sd_u64)offset);
}
static inline int sd_munmap(void* addr, sd_u64 length) {
  return (int)sd_syscall2(SYS_munmap, (sd_u64)addr, (sd_u64)length);
}
static inline int sd_mprotect(void* addr, sd_u64 length, sd_i32 prot) {
  return (int)sd_syscall3(SYS_mprotect, (sd_u64)addr, (sd_u64)length, (sd_u64)prot);
}
static inline void* sd_mmap(void *addr, sd_u64 length, int prot, int flags, int fd, sd_u64 offset) {
  return (void *)sd_syscall6(SYS_mmap, (sd_u64)addr, (sd_u64)length, (sd_u64)prot, (sd_u64)flags,
                             (sd_u64)fd, (sd_u64)offset);
}

// Compilers can decide to replace this function with a stdlib `strlen` call if
// optimizations are enabled. This will make compilation fail when `nostdlib`
// flag is used since linker will not find `strlen`.
#if defined(__clang__)
  __attribute__((no_builtin))
#elif defined(__GNUC__)
  __attribute__((optimize("no-tree-loop-distribute-patterns")))
#endif
static sd_u32 sd_strlen(const char* s) {
  sd_u32 result = 0;
  while(*s) { s += 1; result += 1; }
  return result;
}

static void sd_memcpy(char* dst, char* src, sd_u32 len) {
  for (sd_u32 i = 0; i < len; i += 1) {
    dst[i] = src[i];
  }
}

static sd_u64 sd_align_up_64(sd_u64 v, sd_u64 a) {
  return (v + a - 1) & ~(a - 1);
}

static sd_u64 sd_align_down_64(sd_u64 v, sd_u64 a) {
  return v & ~(a - 1);
}

static sd_u64 sd_offset_from_alignment_64(sd_u64 v, sd_u64 a) {
  return v & (a - 1);
}

// Do this weird definition to silence "used but not defined" warnings. The
// reason for defining a function with top level `asm` block is same as for the
// `_start` symbol bellow.
extern void sd_stage2_entry(void);

#if defined(__x86_64__)
  asm(
      ".local sd_stage2_entry\n"
      ".type sd_stage2_entry, @function\n"
      "sd_stage2_entry:\n"
      "    movq (%rsp), %rdi\n"
      "    leaq 8(%rsp), %rsi\n"
      "    call main\n"
      "    movq %rax, %rdi\n"
      "    movq $231, %rax\n" // 231 is SYS_exit_group
      "    syscall\n"
      "    ud2\n"
      ".size sd_stage2_entry, .-sd_stage2_entry\n"
  );
#elif defined(__aarch64__)
  asm(
      ".local sd_stage2_entry\n"
      ".type sd_stage2_entry, %function\n"
      "sd_stage2_entry:\n"
      "    ldr x0, [sp]\n"
      "    add x1, sp, #8\n"
      "    bl main\n"
      "    mov x8, #94\n" // 94 is SYS_exit_group
      "    svc #0\n"
      "    udf #0\n"
      ".size sd_stage2_entry, .-sd_stage2_entry\n"
  );
#endif

// Since there is no official way to get the location of the linker on the platform
// some tricks need to be used. In this case the trick is to simply expect for the most
// wide spread binary on the system to exist. In this case it is `/bin/sh`. Maybe there is
// a distribution which does not have `/bin/sh` or has it statically linked, but then ...
// you can't win them all..
static sd_error sd_read_linker_path(char* linker_path) {
  char*  file_path = "/bin/sh";
  sd_i32 file_fd   = sd_open(file_path, O_RDONLY | O_CLOEXEC);
  if (file_fd < 0) {
    return SD_ERROR_NO_BOUNCE_BINARY;
  }

  Elf64_Ehdr ehdr;
  if (sd_pread(file_fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
    sd_close(file_fd);
    return SD_ERROR_CANNOT_READ_BOUNCE_BINARY;
  }

  Elf64_Phdr phdr;
  for (sd_i32 i = 0; i < ehdr.e_phnum; i += 1) {
    if (sd_pread(file_fd, &phdr, sizeof(phdr), ehdr.e_phoff + i * sizeof(phdr)) != sizeof(phdr)) {
      sd_close(file_fd);
      return SD_ERROR_CANNOT_READ_BOUNCE_BINARY;
    }
    if (phdr.p_type == PT_INTERP) {
      if (sd_pread(file_fd, linker_path, phdr.p_filesz, phdr.p_offset) != phdr.p_filesz) {
        sd_close(file_fd);
        return SD_ERROR_CANNOT_READ_BOUNCE_BINARY;
      }
      break;
    }
  }
  sd_close(file_fd);

  if (phdr.p_type != PT_INTERP) {
    return SD_ERROR_NO_LINKER_PATH;
  }

  return SD_ERROR_NONE;
}

// Pretend to be a kernel and mmap linker into the address space of this program
static sd_error sd_mmap_linker(char* linker_path, sd_u32 page_size, sd_u8** mmap, sd_u64* e_entry) {
  sd_i32 linker_file_fd = sd_open(linker_path, O_RDONLY | O_CLOEXEC);
  if (linker_file_fd < 0) {
    return SD_ERROR_NO_LINKER;
  }

  Elf64_Ehdr ehdr;
  if (sd_pread(linker_file_fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
    sd_close(linker_file_fd);
    return SD_ERROR_CANNOT_LOAD_LINKER;
  }

  const sd_u64 linker_phdrs_bytes = ehdr.e_phnum * sizeof(Elf64_Phdr);
  Elf64_Phdr*  linker_phdrs       = __builtin_alloca(linker_phdrs_bytes);
  if (sd_pread(linker_file_fd, linker_phdrs, linker_phdrs_bytes, ehdr.e_phoff) != linker_phdrs_bytes) {
    sd_close(linker_file_fd);
    return SD_ERROR_CANNOT_LOAD_LINKER;
  }

  sd_u64 min_va = ~0;
  sd_u64 max_va = 0;
  for (Elf64_Phdr* i = linker_phdrs; i < linker_phdrs + ehdr.e_phnum; i += 1) {
    if (i->p_type == PT_LOAD) {
      min_va =                    min_va < i->p_vaddr ? min_va : i->p_vaddr;
      max_va = (i->p_vaddr + i->p_memsz) < max_va     ? max_va : (i->p_vaddr + i->p_memsz);
    }
  }
  min_va    = sd_align_down_64(min_va, page_size);
  max_va    = sd_align_up_64(max_va, page_size);
  sd_u64 va = max_va - min_va;

  sd_u8* linker_mmap = sd_mmap((void*)min_va, va,
                               PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                               -1, 0);
  if ((sd_i64)linker_mmap < 0) {
    sd_close(linker_file_fd);
    return SD_ERROR_CANNOT_LOAD_LINKER;
  }
  sd_munmap(linker_mmap, va);

  for (Elf64_Phdr* i = linker_phdrs; i < linker_phdrs + ehdr.e_phnum; i += 1) {
    if (i->p_type == PT_LOAD) {
      sd_u64 off = sd_offset_from_alignment_64(i->p_vaddr, page_size);
      sd_u64 beg = (sd_u64)(linker_mmap + sd_align_down_64(i->p_vaddr, page_size));
      sd_i64 sz  = sd_align_up_64(i->p_memsz + off, page_size);

      sd_u8* map = sd_mmap((void*)beg, sz, PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if ((sd_i64)map < 0) {
        for (Elf64_Phdr* j = linker_phdrs; i != j; j += 1) {
            sd_u64 off = sd_offset_from_alignment_64(i->p_vaddr, page_size);
            sd_u64 beg = (sd_u64)(linker_mmap + sd_align_down_64(i->p_vaddr, page_size));
            sd_i64 sz  = sd_align_up_64(i->p_memsz + off, page_size);
            sd_munmap((void*)beg, sz);
        }

        sd_close(linker_file_fd);
        return SD_ERROR_CANNOT_LOAD_LINKER;
      }

      if (sd_pread(linker_file_fd, map + off, i->p_filesz, i->p_offset) != i->p_filesz) {
        sd_close(linker_file_fd);
        return SD_ERROR_CANNOT_LOAD_LINKER;
      }

      sd_i32 prot = 0;
      if (i->p_flags & PF_R) prot |= PROT_READ;
      if (i->p_flags & PF_W) prot |= PROT_WRITE;
      if (i->p_flags & PF_X) prot |= PROT_EXEC;
      sd_mprotect(map, sz, prot);
    }
  }
  sd_close(linker_file_fd);

  *mmap    = linker_mmap;
  *e_entry = ehdr.e_entry;

  return SD_ERROR_NONE;
}


void sd_stage2_bypass(sd_u64* sp, sd_error e) {
    sd_got.success = 0;
    sd_got.error   = e;
#if defined(SD_MUSL)
    // musl exposes these even though not as a part of official API. Calling
    // this takes care of all the musl init and calling of the actual `main`
    extern int __libc_start_main(int (*)(int, char**, char**), int, char**,
                                  void (*)(void), void (*)(void), void*);
    extern void _init(void) __attribute__((weak));
    extern void _fini(void) __attribute__((weak));

    sd_u64  argc = *(sd_u64*)sp;
    sd_u64* argv = (sd_u64*)(sp + 1);
    __libc_start_main((int (*)(int, char**, char**))main,
                      (int)argc, (char**)argv,
                      _init, _fini, 0);
#else
  #if defined(__x86_64__)
    __asm__ __volatile__(
      "mov %1,%%rsp\n"
      "jmpq *%0\n"
      :
      : "S"(sd_stage2_entry), "d"((sd_u64)sp)
      : "memory");
  #elif defined(__aarch64__)
    __asm__ __volatile__(
        "mov sp, %1\n"
        "br %0\n"
        :
        : "r"(sd_stage2_entry), "r"((sd_u64)sp)
        : "memory");
  #endif
#endif
  __builtin_unreachable();
}

// stage1 performs the search for the dynamic linker path, loads the linker in the virtual
// address space of this process, prepares needed data for the linker and lastly jumps into it.
//
// Since the data for the linker needs to be stored permanently somewhere, we just store it
// on the program stack outside any function frame. The stack allocation happen in the `_start`.
//
// The layout of memory will look like this:
//
// | ...
// | original argc
// | original argc
// |--------------- <- This where `sp` argument points to. This is an original `sp` given to us by the kernel.
// |                   It will also be used if linker loading fails in any way.
// | new phdrs
// | new dyn
// | new strtab
// | new symtab
// | new rela
// | new interp
// |---------------
// | new auxv
// | new envp
// | new argv
// | new argc
// |--------------- <- This is where `sp` will point to when we jump to the linker. This way all data above
// |                   `sp` will remain untouched.
void sd_stage1_entry(sd_u64* sp) {
  sd_u64  orig_argc = *(sd_u64*)sp;
  sd_u64* orig_argv = (sd_u64*)(sp + 1);
  sd_u64* orig_envp = orig_argv + orig_argc + 1;
  while (*orig_envp != 0) {
    orig_envp += 1;
  }
  orig_envp += 1;
  Elf64_auxv_t* orig_auxv = (Elf64_auxv_t*)orig_envp;
  // Restore `orig_envp` since it was used to get to the `orig_auxv`
  orig_envp = orig_argv + orig_argc + 1;

  // Read auxv to get ones pointing to entries we need to modify
  // also remember original phdrs since they will need to be copied
  Elf64_Phdr*   orig_phdrs;
  sd_u32        orig_n_phdrs;
  sd_u32        page_size;
  sd_u32        n_auxv = 0;
  Elf64_auxv_t* orig_auxv_copy = orig_auxv;
  while (orig_auxv->a_type != AT_NULL) {
    if (orig_auxv->a_type == AT_PHDR) {
      orig_phdrs = (Elf64_Phdr*)orig_auxv->a_un.a_val;
    } else if (orig_auxv->a_type == AT_PHNUM) {
      orig_n_phdrs = (sd_u32)orig_auxv->a_un.a_val;
    } else if (orig_auxv->a_type == AT_PAGESZ) {
      page_size = (sd_u32)orig_auxv->a_un.a_val;
    }
    orig_auxv += 1;
    n_auxv    += 1;
  }
  n_auxv   += 1;
  orig_auxv = orig_auxv_copy;

  char linker_path [128] = {0};
  sd_error e = sd_read_linker_path(linker_path);
  if (e != SD_ERROR_NONE) {
    sd_stage2_bypass(sp, e);
  }
  sd_u32 linker_path_len = sd_strlen(linker_path);

  sd_u8* linker_map;
  sd_u64 e_entry;
  e = sd_mmap_linker(linker_path, page_size, &linker_map, &e_entry);
  if (e != SD_ERROR_NONE) {
    sd_stage2_bypass(sp, e);
  }

  // Start setting up the required structures starting from the original sp
  // in backwards direction. This way all data is tightly packed just bellow original sp
  // wasting minimum amount of space.
  sd_u32      n_phdrs     = orig_n_phdrs + 2;
  sd_u32      phdrs_bytes = n_phdrs * sizeof(Elf64_Phdr);
  Elf64_Phdr* phdrs       = (Elf64_Phdr*)sd_align_down_64((sd_u64)sp - phdrs_bytes, 8);

  sd_u64         n_dyn     = 9;
  sd_u32         dyn_bytes = n_dyn * sizeof(Elf64_Dyn);
  Elf64_Dyn* dyn           = (Elf64_Dyn*)sd_align_down_64((sd_u64)phdrs - dyn_bytes, 8);

  // This specifies what library we want and what symbols from it we want.
  // The result will be placed in the `got`
  char   strtab_str[]   = "\000libc.so.6\000dlopen\000dlsym\000dlclose\000dlerror\000";
  sd_u32 strtab_str_len = sizeof(strtab_str) - 1;
  char*  strtab         = (char*)((sd_u64)dyn - strtab_str_len);

  sd_u32     n_sym        = 5;
  sd_u32     symtab_bytes = n_sym * sizeof(Elf64_Sym);
  Elf64_Sym* symtab       = (Elf64_Sym*)sd_align_down_64((sd_u64)strtab - symtab_bytes, 8);

  sd_u32      n_rela     = sizeof(sd_got_t) / sizeof(void*);
  sd_u32      rela_bytes = n_rela * sizeof(Elf64_Rela);
  Elf64_Rela* rela       = (Elf64_Rela*)sd_align_down_64((sd_u64)symtab - rela_bytes, 8);

  char* interp = (char*)((sd_u64)rela - linker_path_len);

  // Copy original phdrs as they are except with modification to the PT_PHDR
  // since they are now at a different address
  for (sd_u32 i = 0; i < orig_n_phdrs; i += 1) {
    if (orig_phdrs[i].p_type == PT_PHDR) {
      phdrs[i] = (Elf64_Phdr){
        .p_type   = PT_PHDR,
        .p_flags  = PF_R,
        .p_offset = 0,
        .p_vaddr  = (sd_u64)phdrs,
        .p_paddr  = 0,
        .p_filesz = phdrs_bytes,
        .p_memsz  = phdrs_bytes,
        .p_align  = 8,
      };
    } else {
      phdrs[i] = orig_phdrs[i];
    }
  }
  // Add new PT_DYNAMIC and PT_INTERP phdrs which the linker will use to get us
  // functions from libc
  phdrs[orig_n_phdrs] = (Elf64_Phdr){
      .p_type   = PT_DYNAMIC,
      .p_flags  = PF_R | PF_W,
      .p_offset = 0,
      .p_vaddr  = (sd_u64)dyn,
      .p_paddr  = 0,
      .p_filesz = n_dyn * sizeof(Elf64_Dyn),
      .p_memsz  = n_dyn * sizeof(Elf64_Dyn),
      .p_align  = 8,
  };
  phdrs[orig_n_phdrs + 1] = (Elf64_Phdr){
      .p_type   = PT_INTERP,
      .p_flags  = PF_R,
      .p_offset = 0,
      .p_vaddr  = (sd_u64)interp,
      .p_paddr  = 0,
      .p_filesz = linker_path_len,
      .p_memsz  = linker_path_len,
      .p_align  = 1,
  };

  // These are offsets into the `strtab_str`
  sd_u32 strtab_libc_off         =  1;
  sd_u32 strtab_dlopen_name_off  = 11;
  sd_u32 strtab_dlsym_name_off   = 18;
  sd_u32 strtab_dlclose_name_off = 24;
  sd_u32 strtab_dlerror_name_off = 32;

  dyn[0] = (Elf64_Dyn){ .d_tag = DT_NEEDED,  .d_un.d_val = strtab_libc_off    };
  dyn[1] = (Elf64_Dyn){ .d_tag = DT_STRTAB,  .d_un.d_val = (sd_u64)strtab     };
  dyn[2] = (Elf64_Dyn){ .d_tag = DT_STRSZ,   .d_un.d_val = strtab_str_len     };
  dyn[3] = (Elf64_Dyn){ .d_tag = DT_SYMTAB,  .d_un.d_val = (sd_u64)symtab     };
  dyn[4] = (Elf64_Dyn){ .d_tag = DT_SYMENT,  .d_un.d_val = sizeof(Elf64_Sym)  };
  dyn[5] = (Elf64_Dyn){ .d_tag = DT_RELA,    .d_un.d_val = (sd_u64)rela       };
  dyn[6] = (Elf64_Dyn){ .d_tag = DT_RELASZ,  .d_un.d_val = rela_bytes         };
  dyn[7] = (Elf64_Dyn){ .d_tag = DT_RELAENT, .d_un.d_val = sizeof(Elf64_Rela) };
  dyn[8] = (Elf64_Dyn){ .d_tag = DT_NULL,    .d_un.d_val = 0                  };

  sd_memcpy(strtab, strtab_str, strtab_str_len);

  symtab[0] = (Elf64_Sym){0};
  symtab[1] = (Elf64_Sym){
    .st_name  = strtab_dlopen_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };
  symtab[2] = (Elf64_Sym){
    .st_name  = strtab_dlsym_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };
  symtab[3] = (Elf64_Sym){
    .st_name  = strtab_dlclose_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };
  symtab[4] = (Elf64_Sym){
    .st_name  = strtab_dlerror_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };

#if defined(__x86_64__)
  #define SD_ARCH_GLOB_DAT R_X86_64_GLOB_DAT
#elif defined(__aarch64__)
  #define SD_ARCH_GLOB_DAT  R_AARCH64_GLOB_DAT
#endif

  // Tell the linker where to write function pointers inside `got` table
  rela[0] = (Elf64_Rela){
    .r_offset = (sd_u64)(&sd_got) + 0 * sizeof(sd_u64),
    .r_info   = (sd_u64)1 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };
  rela[1] = (Elf64_Rela){
    .r_offset = (sd_u64)(&sd_got) + 1 * sizeof(sd_u64),
    .r_info   = (sd_u64)2 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };
  rela[2] = (Elf64_Rela){
    .r_offset = (sd_u64)(&sd_got) + 2 * sizeof(sd_u64),
    .r_info   = (sd_u64)3 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };
  rela[3] = (Elf64_Rela){
    .r_offset = (sd_u64)(&sd_got) + 3 * sizeof(sd_u64),
    .r_info   = (sd_u64)4 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };

  sd_memcpy(interp, linker_path, linker_path_len);

  // The stack needs to be 16 byte aligned, so calculate and subtract the
  // total number of bytes needed for everything so the resulting address can
  // be easily aligned
  sd_u32 auxv_bytes  = n_auxv * sizeof(Elf64_auxv_t);
  sd_u32 n_env       = (sd_u64*)orig_auxv - orig_envp - 1;
  sd_u32 env_bytes   = (n_env + 1) * sizeof(sd_u64);
  sd_u32 n_argv      = orig_argc;
  sd_u32 argv_bytes  = n_argv * sizeof(sd_u64) + sizeof(sd_u64);
  sd_u32 argc_bytes  = sizeof(sd_u64);
  sd_u32 total_bytes = auxv_bytes + env_bytes + argv_bytes + argc_bytes;

  sd_u64* argc       = (sd_u64*)sd_align_down_64((sd_u64)interp - total_bytes, 16);
  sd_u64* argv       = argc + 1;
  sd_u64* envp       = argv + n_argv + 1;
  Elf64_auxv_t* auxv = (Elf64_auxv_t*)(envp + n_env + 1);

  // Keep the arguments as they were passed to the original program but
  // add one additional argument which is a pointer to the `got` table
  *argc            = n_argv;
  sd_memcpy((char*)argv, (char*)orig_argv, n_argv * sizeof(sd_u64));
  argv[n_argv]     = 0x0;

  // Copy all environment vars as they were since they now need to be
  // at a different address
  sd_memcpy((char*)envp, (char*)orig_envp, n_env * sizeof(sd_u64));
  envp[n_env] = 0x0;

  while (orig_auxv->a_type != AT_NULL) {
    if (orig_auxv->a_type == AT_PHDR) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_PHDR,  .a_un.a_val = (sd_u64)phdrs            };
    } else if (orig_auxv->a_type == AT_PHNUM) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_PHNUM, .a_un.a_val = n_phdrs                  };
    } else if (orig_auxv->a_type == AT_ENTRY) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_ENTRY, .a_un.a_val = (sd_u64)&sd_stage2_entry };
    } else if (orig_auxv->a_type == AT_BASE) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_BASE,  .a_un.a_val = (sd_u64)linker_map       };
    } else {
      *auxv = *orig_auxv;
    }
    orig_auxv += 1;
    auxv      += 1;
  }
  *auxv =     (Elf64_auxv_t){ .a_type = AT_NULL,  .a_un.a_val = (sd_u64)0                };

  sd_u64 linker_entry = (sd_u64)(linker_map + e_entry);

#if defined(__x86_64__)
  __asm__ __volatile__(
    "mov %1,%%rsp\n"
    "jmpq *%0\n"
    :
    : "S"(linker_entry), "d"((sd_u64)argc)
    : "memory");
#elif defined(__aarch64__)
  __asm__ __volatile__(
      "mov sp, %1\n"
      "br %0\n"
      :
      : "r"(linker_entry), "r"((sd_u64)argc)
      : "memory");
#endif

  __builtin_unreachable();
}

// Define _start as a top level assembly block because on aarch64 compilers
// does not respect the `__naked__` attribute and still adds the prolog to the
// function that messes up the stack pointer. On x86_64 this is not an issue,
// but do same thing just to be consistent
__attribute__((used))
void _start(void);

#if defined(__x86_64__)
  asm(
      ".global _start\n"
      ".type _start, @function\n"
      "_start:\n"
      "    movq %rsp, %rdi\n"
      "    movq %rdx, %rsi\n"
      "    subq $8192, %rsp\n"
      "    call sd_stage1_entry\n"
      ".size _start, .-_start\n"
  );
#elif defined(__aarch64__)
  asm(
    ".global _start\n"
    ".type _start, %function\n"
    "_start:\n"
    "    mov x1, x0\n"
    "    mov x0, sp\n"
    "    sub sp, sp, #8192\n"
    "    ldr x2, =sd_stage1_entry\n"
    "    blr x2\n"
    ".size _start, .-_start\n"
  );
#endif
