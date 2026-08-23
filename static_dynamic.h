#include <elf.h>          // Elf64_{Ehdr,Phdr,auxv_t}
#include <fcntl.h>        // O_{RDONLY,CLOEXEC}
#include <sys/mman.h>     // MAP_{FIXED,PRIVATE,ANONYMOUS}, PROT_{NONE,READ,WRITE,EXEC}
#include <sys/syscall.h>  // SYS_*

typedef signed long        i32;
typedef signed long long   i64;
typedef unsigned char      u8;
typedef unsigned           u32;
typedef unsigned long long u64;

#if defined(__x86_64__)
  static inline u64 syscall1(u64 n, u64 a0) {
    u64 result;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0) : "rcx", "r11", "memory");
    return result;
  }
  static inline u64 syscall2(u64 n, u64 a0, u64 a1) {
    u64 result;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1) : "rcx", "r11", "memory");
    return result;
  }
  static inline u64 syscall3(u64 n, u64 a0, u64 a1, u64 a2) {
    u64 result;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1), "d"(a2): "rcx", "r11", "memory");
    return result;
  }
  static inline u64 syscall4(u64 n, u64 a0, u64 a1, u64 a2, u64 a3) {
    u64 result;
    register u64 r10 __asm__("r10") = a3;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1), "d"(a2), "r"(r10) : "rcx", "r11", "memory");
    return result;
  }
  static inline u64 syscall6(u64 n, u64 a0, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5) {
    u64 result;
    register u64 r10 __asm__("r10") = a3;
    register u64 r8  __asm__("r8") = a4;
    register u64 r9  __asm__("r9") = a5;
    __asm__ __volatile__("syscall" : "=a"(result) : "a"(n), "D"(a0), "S"(a1), "d"(a2), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return result;
  }
#elif defined(__aarch64__)
  static inline u64 syscall1(u64 n, u64 a0) {
    register u64 x8 __asm__("x8") = n;
    register u64 x0 __asm__("x0") = a0;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
  }

  static inline u64 syscall2(u64 n, u64 a0, u64 a1) {
    register u64 x8 __asm__("x8") = n;
    register u64 x0 __asm__("x0") = a0;
    register u64 x1 __asm__("x1") = a1;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory");
    return x0;
  }

  static inline u64 syscall3(u64 n, u64 a0, u64 a1, u64 a2) {
    register u64 x8 __asm__("x8") = n;
    register u64 x0 __asm__("x0") = a0;
    register u64 x1 __asm__("x1") = a1;
    register u64 x2 __asm__("x2") = a2;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
  }

  static inline u64 syscall4(u64 n, u64 a0, u64 a1, u64 a2, u64 a3) {
    register u64 x8 __asm__("x8") = n;
    register u64 x0 __asm__("x0") = a0;
    register u64 x1 __asm__("x1") = a1;
    register u64 x2 __asm__("x2") = a2;
    register u64 x3 __asm__("x3") = a3;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
    return x0;
  }

  static inline u64 syscall6(u64 n, u64 a0, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5) {
    register u64 x8 __asm__("x8") = n;
    register u64 x0 __asm__("x0") = a0;
    register u64 x1 __asm__("x1") = a1;
    register u64 x2 __asm__("x2") = a2;
    register u64 x3 __asm__("x3") = a3;
    register u64 x4 __asm__("x4") = a4;
    register u64 x5 __asm__("x5") = a5;
    __asm__ __volatile__("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory");
    return x0;
  }
#endif

static inline int m_open(const char* filename, int flags) {
  return (int)syscall4(SYS_openat, (u64)AT_FDCWD, (u64)filename, (u64)flags, (u64)O_RDONLY);
}
static inline int m_close(int fd) {
  return (int)syscall1(SYS_close, (u64)fd);
}
static inline u64 m_pread(int fd, void* buf, size_t count, u64 offset) {
  return (u64)syscall4(SYS_pread64, (u64)fd, (u64)buf, (u64)count, (u64)offset);
}
static inline int m_munmap(void* addr, u64 length) {
  return (int)syscall2(SYS_munmap, (u64)addr, (u64)length);
}
static inline int m_mprotect(void* addr, u64 length, i32 prot) {
  return (int)syscall3(SYS_mprotect, (u64)addr, (u64)length, (u64)prot);
}
static inline void* m_mmap(void *addr, u64 length, int prot, int flags, int fd, u64 offset) {
  return (void *)syscall6(SYS_mmap, (u64)addr, (u64)length, (u64)prot, (u64)flags, (u64)fd, (u64)offset);
}

static u32 m_strlen(const char* s) {
  u32 result = 0;
  while(*s) { s += 1; result += 1; }
  return result;
}

static void m_memcpy(char* dst, char* src, u32 len) {
  for (u32 i = 0; i < len; i += 1) {
    dst[i] = src[i];
  }
}

static u64 align_up_64(u64 v, u64 a) {
  return (v + a - 1) & ~(a - 1);
}

static u64 align_down_64(u64 v, u64 a) {
  return v & ~(a - 1);
}

static void assert(bool v) {
  if (!v) *(char*)0 = 1;
}

extern int main(u64* argc_argv);

// Same thing as for the `_start` symbol.
__attribute__((used))
static void stage2_entry(void);

#if defined(__x86_64__)
  asm(
      ".type stage2_entry, @function\n"
      "stage2_entry:\n"
      "    movq %rsp, %rdi\n"
      "    call main\n"
      "    movq %rax, %rdi\n"
      "    movq $231, %rax\n" // 231 is SYS_exit_group
      "    syscall\n"
      "    ud2\n"
      ".size stage2_entry, .-stage2_entry\n"
  );
#elif defined(__aarch64__)
  asm(
      ".type stage2_entry, %function\n"
      "stage2_entry:\n"
      "    mov x0, sp\n"
      "    bl main\n"
      "    mov x8, #94\n" // 94 is SYS_exit_group
      "    svc #0\n"
      "    udf #0\n"
      ".size stage2_entry, .-stage2_entry\n"
  );
#endif

// Since there is no official way to get the location of the linker on the platform
// some tricks need to be used. In this case the trick is to simply expect for the most
// wide spread binary on the system to exist. In this case it is `/bin/sh`. Maybe there is
// a distribution which does not have `/bin/sh` or has it statically linked, but then ...
// you can't win them all..
static void read_linker_path(char* linker_path) {
  char* file_path = "/bin/sh";
  i32   file_fd   = m_open(file_path, O_RDONLY | O_CLOEXEC);

  Elf64_Ehdr ehdr;
  assert(m_pread(file_fd, &ehdr, sizeof(ehdr), 0) == sizeof(ehdr));

  Elf64_Phdr phdr;
  for (i32 i = 0; i < ehdr.e_phnum; i += 1) {
    assert(m_pread(file_fd, &phdr, sizeof(phdr), ehdr.e_phoff + i * sizeof(phdr)) == sizeof(phdr));
    if (phdr.p_type == PT_INTERP) {
      assert(m_pread(file_fd, linker_path, phdr.p_filesz, phdr.p_offset) == phdr.p_filesz);
      break;
    }
  }
  m_close(file_fd);
}

// Pretend to be a kernel and mmap linker into the address space of this program
static void mmap_linker(char* linker_path, u8** mmap, u64* e_entry) {
  i32 linker_file_fd = m_open(linker_path, O_RDONLY | O_CLOEXEC);

  Elf64_Ehdr ehdr;
  assert(m_pread(linker_file_fd, &ehdr, sizeof(ehdr), 0) == sizeof(ehdr));

  const u64   linker_phdrs_bytes = ehdr.e_phnum * sizeof(Elf64_Phdr);
  Elf64_Phdr* linker_phdrs       = __builtin_alloca(linker_phdrs_bytes);
  assert(m_pread(linker_file_fd, linker_phdrs, linker_phdrs_bytes, ehdr.e_phoff) == linker_phdrs_bytes);

  u64 min_va = ~0;
  u64 max_va = 0;
  for (Elf64_Phdr* i = linker_phdrs; i < linker_phdrs + ehdr.e_phnum; i += 1) {
    if (i->p_type == PT_LOAD) {
      min_va =                    min_va < i->p_vaddr ? min_va : i->p_vaddr;
      max_va = (i->p_vaddr + i->p_memsz) < max_va     ? max_va : (i->p_vaddr + i->p_memsz);
    }
  }
  min_va = align_down_64(min_va, 4096);
  max_va = align_up_64(max_va, 4096);
  u64 va = max_va - min_va;

  u8* linker_mmap = m_mmap((void*)min_va, va,
                          PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                          -1, 0);
  assert(0 < (i64)linker_mmap);
  m_munmap(linker_mmap, va);

  for (Elf64_Phdr* i = linker_phdrs; i < linker_phdrs + ehdr.e_phnum; i += 1) {
    if (i->p_type == PT_LOAD) {
      u64 off = i->p_vaddr & (4096 - 1);
      u64 beg = (u64)(linker_mmap + align_down_64(i->p_vaddr, 4096));
      i64 sz  = align_up_64(i->p_memsz + off, 4096);

      u8* map = m_mmap((void*)beg, sz, PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      assert(0 < (i64)map);

      assert(m_pread(linker_file_fd, map + off, i->p_filesz, i->p_offset) == i->p_filesz);
      i32 prot = 0;
      if (i->p_flags & PF_R) prot |= PROT_READ;
      if (i->p_flags & PF_W) prot |= PROT_WRITE;
      if (i->p_flags & PF_X) prot |= PROT_EXEC;
      m_mprotect(map, sz, prot);
    }
  }
  m_close(linker_file_fd);

  *mmap    = linker_mmap;
  *e_entry = ehdr.e_entry;
}

void entry(u64* sp) {
  char linker_path [128] = {0};
  read_linker_path(linker_path);
  u32 linker_path_len = m_strlen(linker_path);

  u8* linker_map;
  u64 e_entry;
  mmap_linker(linker_path, &linker_map, &e_entry);

  // The permanent data is stored on the stack as tightly as possible.
  // The layout will look like this:
  //
  // | ...
  // | original argc
  // | original argc
  // |---------------
  // | new phdrs
  // | new dyn
  // | new strtab
  // | new symtab
  // | new rela
  // | new got
  // | new interp
  // |---------------
  // | new auxv
  // | new envp
  // | new argv
  // | new argc
  // |---------------
  // | actual stack for this function call, for the linker and for the future `main`
  // | ...
  void* original_sp = (void*)sp;
  u64  orig_argc = *(u64*)original_sp;
  u64* orig_argv = (u64*)(original_sp + sizeof(u64));
  u64* orig_envp = orig_argv + orig_argc + 1;
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
  u32           orig_n_phdrs;
  u32           n_auxv         = 0;
  Elf64_auxv_t* orig_auxv_copy = orig_auxv;
  while (orig_auxv->a_type != AT_NULL) {
    if (orig_auxv->a_type == AT_PHDR) {
      orig_phdrs = (Elf64_Phdr*)orig_auxv->a_un.a_val;
    } else if (orig_auxv->a_type == AT_PHNUM) {
      orig_n_phdrs = (u32)orig_auxv->a_un.a_val;
    }
    orig_auxv += 1;
    n_auxv    += 1;
  }
  n_auxv   += 1;
  orig_auxv = orig_auxv_copy;

  // Start setting up the required structures starting from the original sp
  // in backwards direction. This way all data is tightly packed just bellow original sp
  // wasting minimum amount of space.
  u32         n_phdrs     = orig_n_phdrs + 2;
  u32         phdrs_bytes = n_phdrs * sizeof(Elf64_Phdr);
  Elf64_Phdr* phdrs       = (Elf64_Phdr*)align_down_64((u64)original_sp - phdrs_bytes, 8);

  u64        n_dyn     = 9;
  u32        dyn_bytes = n_dyn * sizeof(Elf64_Dyn);
  Elf64_Dyn* dyn       = (Elf64_Dyn*)align_down_64((u64)phdrs - dyn_bytes, 8);

  // This specifies what library we want and what symbols from it we want.
  // The result will be placed in the `got`
  char  strtab_str[]   = "\000libc.so.6\000dlopen\000dlsym\000dlclose\000dlerror\000";
  u32   strtab_str_len = sizeof(strtab_str) - 1;
  char* strtab         = (char*)((u64)dyn - strtab_str_len);

  u32        n_sym        = 5;
  u32        symtab_bytes = n_sym * sizeof(Elf64_Sym);
  Elf64_Sym* symtab       = (Elf64_Sym*)align_down_64((u64)strtab - symtab_bytes, 8);

  u32         n_rela     = 4;
  u32         rela_bytes = n_rela * sizeof(Elf64_Rela);
  Elf64_Rela* rela       = (Elf64_Rela*)align_down_64((u64)symtab - rela_bytes, 8);

  u32  n_got     = 4;
  u32  got_bytes = n_got * sizeof(u64);
  u64* got       = (u64*)align_down_64((u64)rela - got_bytes, 8);

  char* interp = (char*)((u64)got - linker_path_len);

  // Copy original phdrs as they are except with modification to the PT_PHDR
  // since they are now at a different address
  for (u32 i = 0; i < orig_n_phdrs; i += 1) {
    if (orig_phdrs[i].p_type == PT_PHDR) {
      phdrs[i] = (Elf64_Phdr){
        .p_type   = PT_PHDR,
        .p_flags  = PF_R,
        .p_offset = 0,
        .p_vaddr  = (u64)phdrs,
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
      .p_vaddr  = (u64)dyn,
      .p_paddr  = 0,
      .p_filesz = n_dyn * sizeof(Elf64_Dyn),
      .p_memsz  = n_dyn * sizeof(Elf64_Dyn),
      .p_align  = 8,
  };
  phdrs[orig_n_phdrs + 1] = (Elf64_Phdr){
      .p_type   = PT_INTERP,
      .p_flags  = PF_R,
      .p_offset = 0,
      .p_vaddr  = (u64)interp,
      .p_paddr  = 0,
      .p_filesz = linker_path_len,
      .p_memsz  = linker_path_len,
      .p_align  = 1,
  };

  // These are offsets into the `strtab_str`
  u32 strtab_libc_off         =  1;
  u32 strtab_dlopen_name_off  = 11;
  u32 strtab_dlsym_name_off   = 18;
  u32 strtab_dlclose_name_off = 24;
  u32 strtab_dlerror_name_off = 32;

  dyn[0] = (Elf64_Dyn){ .d_tag = DT_NEEDED,  .d_un.d_val = strtab_libc_off            };
  dyn[1] = (Elf64_Dyn){ .d_tag = DT_STRTAB,  .d_un.d_val = (u64)strtab                };
  dyn[2] = (Elf64_Dyn){ .d_tag = DT_STRSZ,   .d_un.d_val = strtab_str_len             };
  dyn[3] = (Elf64_Dyn){ .d_tag = DT_SYMTAB,  .d_un.d_val = (u64)symtab                };
  dyn[4] = (Elf64_Dyn){ .d_tag = DT_SYMENT,  .d_un.d_val = sizeof(Elf64_Sym)          };
  dyn[5] = (Elf64_Dyn){ .d_tag = DT_RELA,    .d_un.d_val = (u64)rela                  };
  dyn[6] = (Elf64_Dyn){ .d_tag = DT_RELASZ,  .d_un.d_val = n_got * sizeof(Elf64_Rela) };
  dyn[7] = (Elf64_Dyn){ .d_tag = DT_RELAENT, .d_un.d_val = sizeof(Elf64_Rela)         };
  dyn[8] = (Elf64_Dyn){ .d_tag = DT_NULL,    .d_un.d_val = 0                          };

  m_memcpy(strtab, strtab_str, strtab_str_len);

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
  #define ARCH_GLOB_DAT R_X86_64_GLOB_DAT
#elif defined(__aarch64__)
  #define ARCH_GLOB_DAT R_AARCH64_GLOB_DAT
#endif

  rela[0] = (Elf64_Rela){
    .r_offset = (u64)got + 0 * sizeof(u64),
    .r_info   = (u64)1 << 32 | ARCH_GLOB_DAT,
    .r_addend = 0
  };
  rela[1] = (Elf64_Rela){
    .r_offset = (u64)got + 1 * sizeof(u64),
    .r_info   = (u64)2 << 32 | ARCH_GLOB_DAT,
    .r_addend = 0
  };
  rela[2] = (Elf64_Rela){
    .r_offset = (u64)got + 2 * sizeof(u64),
    .r_info   = (u64)3 << 32 | ARCH_GLOB_DAT,
    .r_addend = 0
  };
  rela[3] = (Elf64_Rela){
    .r_offset = (u64)got + 3 * sizeof(u64),
    .r_info   = (u64)4 << 32 | ARCH_GLOB_DAT,
    .r_addend = 0
  };

  m_memcpy(interp, linker_path, linker_path_len);

  u32 auxv_bytes     = n_auxv * sizeof(Elf64_auxv_t);
  Elf64_auxv_t* auxv = (Elf64_auxv_t*)align_down_64((u64)interp - auxv_bytes, 8);

  u32  n_env     = (u64*)orig_auxv - orig_envp - 1;
  u32  env_bytes = (n_env + 1) * sizeof(u64);
  u64* envp      = (u64*)align_down_64((u64)auxv - env_bytes, 8);

  u32 n_argv     = orig_argc + 1;
  u32 argv_bytes = n_argv * sizeof(u64) + sizeof(u64);
  u64* argv      = (u64*)align_down_64((u64)envp - argv_bytes, 8);

  u32 argc_bytes = sizeof(u64);
  u64* argc      = argv - 1;

  // Keep the arguments as they were passed to the original program but
  // add one additional argument which is a pointer to the `got` table
  *argc            = n_argv;
  m_memcpy((char*)argv, (char*)orig_argv, n_argv * sizeof(u64));
  argv[n_argv - 1] = (u64)got;
  argv[n_argv]     = 0x0;

  // Copy all environment vars as they were since they now need to be
  // at a different address
  m_memcpy((char*)envp, (char*)orig_envp, n_env * sizeof(u64));
  envp[n_env] = 0x0;

  while (orig_auxv->a_type != AT_NULL) {
    if (orig_auxv->a_type == AT_PHDR) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_PHDR,  .a_un.a_val = (u64)phdrs         };
    } else if (orig_auxv->a_type == AT_PHNUM) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_PHNUM, .a_un.a_val = n_phdrs            };
    } else if (orig_auxv->a_type == AT_ENTRY) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_ENTRY, .a_un.a_val = (u64)&stage2_entry };
    } else if (orig_auxv->a_type == AT_BASE) {
      *auxv = (Elf64_auxv_t){ .a_type = AT_BASE,  .a_un.a_val = (u64)linker_map    };
    } else {
      *auxv = *orig_auxv;
    }
    orig_auxv += 1;
    auxv      += 1;
  }
  *auxv =     (Elf64_auxv_t){ .a_type = AT_NULL,  .a_un.a_val = (u64)0             };

  u64 linker_entry = (u64)(linker_map + e_entry);

#if defined(__x86_64__)
  __asm__ __volatile__(
    "mov %1,%%rsp\n"
    "jmpq *%0\n"
    :
    : "S"(linker_entry), "d"((u64)argc)
    : "memory");
#elif defined(__aarch64__)
  __asm__ __volatile__(
      "mov sp, %1\n"
      "br %0\n"
      :
      : "r"(linker_entry), "r"((u64)argc)
      : "memory");
#endif

  __builtin_unreachable();
}

// Define _start as a top level assembly block because on aarch64 gcc does not
// respect the `__naked__` attribute and still adds the prolog to the function
// that messes up the stack pointer.
// On x86_64 this is not an issue, but do same thing just to be consistent
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
      "    call entry\n"
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
    "    ldr x2, =entry\n"
    "    blr x2\n"
    ".size _start, .-_start\n"
  );
#endif
