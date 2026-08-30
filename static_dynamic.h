// User API
// ------------------------------------------------

typedef unsigned short     sd_u16;
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
  SD_ERROR_CANNOT_INIT_TLS,
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

// Sets the thread pointer and reserves the static TLS block for the main
// thread. Usually this is done by the dynamic linker, but in case where
// loading of the dynamic linker failed (sd_got.success == 0), this function
// needs to be called instead.
//
// The function takes `sp` as an argument which is just 8 bytes before `argv`, so the call
// can look like:
// ```c
// sd_c_tls_init((sd_u64*)(argv - 1));
// ```
//
// Do not call this if dynamic loader has already performed all the setup, this
// will break loaded `libc`.
__attribute__((unused))
static int sd_c_tls_init(sd_u64* sp);

// Implementation
// ------------------------------------------------

// Copy all needed defines/structs from headers since not all compilers might
// provide them. All of these are part of the stable ABI, so they will not
// change over time.
// ------------------------------------------------

#if defined(__x86_64__)
// arch/x86_64/bits/syscall.h
  #define SD_SYS_openat     257
  #define SD_SYS_close        3
  #define SD_SYS_pread64     17
  #define SD_SYS_mmap         9
  #define SD_SYS_mprotect    10
  #define SD_SYS_munmap      11
  #define SD_SYS_arch_prctl 158
#elif defined(__aarch64__)
// arch/aarch64/bits/syscall.h
  #define SD_SYS_openat    56
  #define SD_SYS_close     57
  #define SD_SYS_pread64   67
  #define SD_SYS_mmap     222
  #define SD_SYS_munmap   215
  #define SD_SYS_mprotect 226
#endif

// arch/x86/include/uapi/asm/prctl.h
// ------------------------------------------------
#if defined(__x86_64__)
  #define SD_ARCH_SET_FS 0x1002
#endif

// include/sys/mman.h
// ------------------------------------------------
#define SD_PROT_NONE      0
#define SD_PROT_READ      1
#define SD_PROT_WRITE     2
#define SD_PROT_EXEC      4

// include/fcntl.h
// ------------------------------------------------
#define SD_AT_FDCWD (-100)
#define SD_O_RDONLY  00

// include/elf.h
// ------------------------------------------------
typedef sd_u16 sd_elf64_half;
typedef sd_u32 sd_elf64_word;
typedef sd_i32 sd_elf64_sword;
typedef sd_u64 sd_elf64_xword;
typedef sd_i64 sd_elf64_sxword;
typedef sd_u64 sd_elf64_addr;
typedef sd_u64 sd_elf64_off;
typedef sd_u16 sd_elf64_section;
#define SD_EI_NIDENT (16)

typedef struct {
  unsigned char e_ident[SD_EI_NIDENT];
  sd_elf64_half e_type;
  sd_elf64_half e_machine;
  sd_elf64_word e_version;
  sd_elf64_addr e_entry;
  sd_elf64_off  e_phoff;
  sd_elf64_off  e_shoff;
  sd_elf64_word e_flags;
  sd_elf64_half e_ehsize;
  sd_elf64_half e_phentsize;
  sd_elf64_half e_phnum;
  sd_elf64_half e_shentsize;
  sd_elf64_half e_shnum;
  sd_elf64_half e_shstrndx;
} sd_elf64_ehdr;

typedef struct {
  sd_elf64_word  p_type;
  sd_elf64_word  p_flags;
  sd_elf64_off   p_offset;
  sd_elf64_addr  p_vaddr;
  sd_elf64_addr  p_paddr;
  sd_elf64_xword p_filesz;
  sd_elf64_xword p_memsz;
  sd_elf64_xword p_align;
} sd_elf64_phdr;

typedef struct {
  sd_u64     a_type;
  union {
      sd_u64 a_val;
  } a_un;
} sd_elf64_auxv_t;

typedef struct {
  sd_elf64_sxword    d_tag;
  union {
      sd_elf64_xword d_val;
      sd_elf64_addr  d_ptr;
  } d_un;
} sd_elf64_dyn;

typedef struct {
  sd_elf64_word    st_name;
  unsigned char st_info;
  unsigned char st_other;
  sd_elf64_section st_shndx;
  sd_elf64_addr    st_value;
  sd_elf64_xword   st_size;
} sd_elf64_sym;

typedef struct {
  sd_elf64_addr   r_offset;
  sd_elf64_xword  r_info;
  sd_elf64_sxword r_addend;
} sd_elf64_rela;

#define SD_AT_NULL   0
#define SD_AT_PHDR   3
#define SD_AT_PHNUM  5
#define SD_AT_PAGESZ 6
#define SD_AT_BASE   7
#define SD_AT_ENTRY  9

#define SD_R_X86_64_GLOB_DAT     6
#define SD_R_AARCH64_GLOB_DAT 1025

#if defined(__x86_64__)
  #define SD_ARCH_GLOB_DAT SD_R_X86_64_GLOB_DAT
#elif defined(__aarch64__)
  #define SD_ARCH_GLOB_DAT SD_R_AARCH64_GLOB_DAT
#endif

#define SD_DT_NULL     0
#define SD_DT_NEEDED   1
#define SD_DT_STRTAB   5
#define SD_DT_SYMTAB   6
#define SD_DT_RELA     7
#define SD_DT_RELASZ   8
#define SD_DT_RELAENT  9
#define SD_DT_STRSZ   10
#define SD_DT_SYMENT  11

#define SD_PT_NULL    0
#define SD_PT_LOAD    1
#define SD_PT_DYNAMIC 2
#define SD_PT_INTERP  3
#define SD_PT_PHDR    6
#define SD_PT_TLS     7

#define SD_PF_X (1 << 0)
#define SD_PF_W (1 << 1)
#define SD_PF_R (1 << 2)

// include/sys/mman.h
// ------------------------------------------------
#define SD_MAP_PRIVATE    0x02
#define SD_MAP_FIXED      0x10
#define SD_MAP_ANONYMOUS  0x20
#define SD_MAP_NORESERVE  0x4000

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
  return (int)sd_syscall4(SD_SYS_openat, (sd_u64)SD_AT_FDCWD, (sd_u64)filename, (sd_u64)flags, (sd_u64)SD_O_RDONLY);
}
static inline int sd_close(int fd) {
  return (int)sd_syscall1(SD_SYS_close, (sd_u64)fd);
}
static inline sd_u64 sd_pread(int fd, void* buf, sd_u64 count, sd_u64 offset) {
  return (sd_u64)sd_syscall4(SD_SYS_pread64, (sd_u64)fd, (sd_u64)buf, (sd_u64)count, (sd_u64)offset);
}
static inline int sd_munmap(void* addr, sd_u64 length) {
  return (int)sd_syscall2(SD_SYS_munmap, (sd_u64)addr, (sd_u64)length);
}
static inline int sd_mprotect(void* addr, sd_u64 length, sd_i32 prot) {
  return (int)sd_syscall3(SD_SYS_mprotect, (sd_u64)addr, (sd_u64)length, (sd_u64)prot);
}
static inline void* sd_mmap(void *addr, sd_u64 length, int prot, int flags, int fd, sd_u64 offset) {
  return (void *)sd_syscall6(SD_SYS_mmap, (sd_u64)addr, (sd_u64)length, (sd_u64)prot, (sd_u64)flags,
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

static int sd_c_tls_init(sd_u64* sp) {
  sd_u64  argc = *sp;
  sd_u64* argv = sp + 1;
  sd_u64* envp = argv + argc + 1;
  while (*envp != 0) {
    envp += 1;
  }
  envp += 1;
  sd_elf64_auxv_t* auxv = (sd_elf64_auxv_t*)envp;

  sd_elf64_phdr* phdrs   = 0;
  sd_u32         n_phdrs = 0;
  while (auxv->a_type != SD_AT_NULL) {
    if (auxv->a_type == SD_AT_PHDR) {
      phdrs = (sd_elf64_phdr*)auxv->a_un.a_val;
    } else if (auxv->a_type == SD_AT_PHNUM) {
      n_phdrs = (sd_u32)auxv->a_un.a_val;
    }
    auxv += 1;
  }
  if (phdrs == 0) {
    return SD_ERROR_CANNOT_INIT_TLS;
  }

  sd_elf64_phdr* tls = 0;
  for (sd_u32 i = 0; i < n_phdrs; i += 1) {
    if (phdrs[i].p_type == SD_PT_TLS) {
      tls = &phdrs[i];
    }
  }
  // The program has no thread locals, so there is nothing to set up.
  if (tls == 0) {
    return SD_ERROR_NONE;
  }

  sd_u64 align  = tls->p_align < 1 ? 1 : tls->p_align;
  char*  image  = (char*)tls->p_vaddr;
  // The total memory size is tls->p_memsz, but only tls->p_filesz will be filled. The
  // rest will be zeroed by the kernel through `mmap`.
  sd_u32 filesz = (sd_u32)tls->p_filesz;

#if defined(__x86_64__)
  // Variant II: | TLS block | TCB |
  //                         ^
  //                         |
  //           with the thread pointer at the TCB.
  sd_u64 block = sd_align_up_64(tls->p_memsz, align);
  char*  area  = sd_mmap(0, block + align + sizeof(sd_u64), SD_PROT_READ | SD_PROT_WRITE,
                         SD_MAP_PRIVATE | SD_MAP_ANONYMOUS, -1, 0);
  if ((sd_i64)area < 0) {
    return SD_ERROR_CANNOT_INIT_TLS;
  }

  char*  base = (char*)sd_align_up_64((sd_u64)area, align);
  sd_u64 tp   = (sd_u64)(base + block);
  // The ABI TCB of this variant holds a single self referential pointer.
  *(sd_u64*)tp = tp;
  sd_memcpy(base, image, filesz);
  if (sd_syscall2(SD_SYS_arch_prctl, SD_ARCH_SET_FS, tp) != 0) {
    return SD_ERROR_CANNOT_INIT_TLS;
  }
#elif defined(__aarch64__)
  // Variant I: | TCB | TLS block |
  //             ^
  //             |
  //           with the thread pointer at the TCB.
  sd_u64 block_off = sd_align_up_64(2 * sizeof(sd_u64), align);
  char*  area      = sd_mmap(0, block_off + tls->p_memsz + align, SD_PROT_READ | SD_PROT_WRITE,
                             SD_MAP_PRIVATE | SD_MAP_ANONYMOUS, -1, 0);
  if ((sd_i64)area < 0) {
    return SD_ERROR_CANNOT_INIT_TLS;
  }

  char*  base = (char*)sd_align_up_64((sd_u64)area, align);
  sd_u64 tp   = (sd_u64)base;
  // The ABI TCB of this variant holds the DTV pointer and one reserved word.
  ((sd_u64*)tp)[0] = 0;
  ((sd_u64*)tp)[1] = 0;
  sd_memcpy(base + block_off, image, filesz);
  __asm__ __volatile__("msr tpidr_el0, %0" :: "r"(tp) : "memory");
#endif
  return SD_ERROR_NONE;
}

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
  sd_i32 file_fd   = sd_open(file_path, SD_O_RDONLY);
  if (file_fd < 0) {
    return SD_ERROR_NO_BOUNCE_BINARY;
  }

  sd_elf64_ehdr ehdr;
  if (sd_pread(file_fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
    sd_close(file_fd);
    return SD_ERROR_CANNOT_READ_BOUNCE_BINARY;
  }

  sd_elf64_phdr phdr;
  for (sd_i32 i = 0; i < ehdr.e_phnum; i += 1) {
    if (sd_pread(file_fd, &phdr, sizeof(phdr), ehdr.e_phoff + i * sizeof(phdr)) != sizeof(phdr)) {
      sd_close(file_fd);
      return SD_ERROR_CANNOT_READ_BOUNCE_BINARY;
    }
    if (phdr.p_type == SD_PT_INTERP) {
      if (sd_pread(file_fd, linker_path, phdr.p_filesz, phdr.p_offset) != phdr.p_filesz) {
        sd_close(file_fd);
        return SD_ERROR_CANNOT_READ_BOUNCE_BINARY;
      }
      break;
    }
  }
  sd_close(file_fd);

  if (phdr.p_type != SD_PT_INTERP) {
    return SD_ERROR_NO_LINKER_PATH;
  }

  return SD_ERROR_NONE;
}

// Pretend to be a kernel and mmap linker into the address space of this program
static sd_error sd_mmap_linker(char* linker_path, sd_u32 page_size, sd_u8** mmap, sd_u64* e_entry) {
  sd_i32 linker_file_fd = sd_open(linker_path, SD_O_RDONLY);
  if (linker_file_fd < 0) {
    return SD_ERROR_NO_LINKER;
  }

  sd_elf64_ehdr ehdr;
  if (sd_pread(linker_file_fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
    sd_close(linker_file_fd);
    return SD_ERROR_CANNOT_LOAD_LINKER;
  }

  const sd_u64 linker_phdrs_bytes = ehdr.e_phnum * sizeof(sd_elf64_phdr);
  sd_elf64_phdr*  linker_phdrs       = __builtin_alloca(linker_phdrs_bytes);
  if (sd_pread(linker_file_fd, linker_phdrs, linker_phdrs_bytes, ehdr.e_phoff) != linker_phdrs_bytes) {
    sd_close(linker_file_fd);
    return SD_ERROR_CANNOT_LOAD_LINKER;
  }

  sd_u64 min_va = ~0;
  sd_u64 max_va = 0;
  for (sd_elf64_phdr* i = linker_phdrs; i < linker_phdrs + ehdr.e_phnum; i += 1) {
    if (i->p_type == SD_PT_LOAD) {
      min_va =                    min_va < i->p_vaddr ? min_va : i->p_vaddr;
      max_va = (i->p_vaddr + i->p_memsz) < max_va     ? max_va : (i->p_vaddr + i->p_memsz);
    }
  }
  min_va    = sd_align_down_64(min_va, page_size);
  max_va    = sd_align_up_64(max_va, page_size);
  sd_u64 va = max_va - min_va;

  sd_u8* linker_mmap = sd_mmap((void*)min_va, va,
                               SD_PROT_NONE, SD_MAP_PRIVATE | SD_MAP_ANONYMOUS | SD_MAP_NORESERVE,
                               -1, 0);
  if ((sd_i64)linker_mmap < 0) {
    sd_close(linker_file_fd);
    return SD_ERROR_CANNOT_LOAD_LINKER;
  }
  sd_munmap(linker_mmap, va);

  for (sd_elf64_phdr* i = linker_phdrs; i < linker_phdrs + ehdr.e_phnum; i += 1) {
    if (i->p_type == SD_PT_LOAD) {
      sd_u64 off = sd_offset_from_alignment_64(i->p_vaddr, page_size);
      sd_u64 beg = (sd_u64)(linker_mmap + sd_align_down_64(i->p_vaddr, page_size));
      sd_i64 sz  = sd_align_up_64(i->p_memsz + off, page_size);

      sd_u8* map = sd_mmap((void*)beg, sz, SD_PROT_WRITE, SD_MAP_FIXED | SD_MAP_ANONYMOUS | SD_MAP_PRIVATE, -1, 0);
      if ((sd_i64)map < 0) {
        for (sd_elf64_phdr* j = linker_phdrs; i != j; j += 1) {
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
      if (i->p_flags & SD_PF_R) prot |= SD_PROT_READ;
      if (i->p_flags & SD_PF_W) prot |= SD_PROT_WRITE;
      if (i->p_flags & SD_PF_X) prot |= SD_PROT_EXEC;
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
  sd_elf64_auxv_t* orig_auxv = (sd_elf64_auxv_t*)orig_envp;
  // Restore `orig_envp` since it was used to get to the `orig_auxv`
  orig_envp = orig_argv + orig_argc + 1;

  // Read auxv to get ones pointing to entries we need to modify
  // also remember original phdrs since they will need to be copied
  sd_elf64_phdr*   orig_phdrs;
  sd_u32           orig_n_phdrs;
  sd_u32           page_size;
  sd_u32           n_auxv = 0;
  sd_elf64_auxv_t* orig_auxv_copy = orig_auxv;
  while (orig_auxv->a_type != SD_AT_NULL) {
    if (orig_auxv->a_type == SD_AT_PHDR) {
      orig_phdrs = (sd_elf64_phdr*)orig_auxv->a_un.a_val;
    } else if (orig_auxv->a_type == SD_AT_PHNUM) {
      orig_n_phdrs = (sd_u32)orig_auxv->a_un.a_val;
    } else if (orig_auxv->a_type == SD_AT_PAGESZ) {
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
  sd_u32         n_phdrs     = orig_n_phdrs + 2;
  sd_u32         phdrs_bytes = n_phdrs * sizeof(sd_elf64_phdr);
  sd_elf64_phdr* phdrs       = (sd_elf64_phdr*)sd_align_down_64((sd_u64)sp - phdrs_bytes, 8);

  sd_u64        n_dyn     = 9;
  sd_u32        dyn_bytes = n_dyn * sizeof(sd_elf64_dyn);
  sd_elf64_dyn* dyn       = (sd_elf64_dyn*)sd_align_down_64((sd_u64)phdrs - dyn_bytes, 8);

  // This specifies what library we want and what symbols from it we want.
  // The result will be placed in the `got`
  char   strtab_str[]   = "\000libc.so.6\000dlopen\000dlsym\000dlclose\000dlerror\000";
  sd_u32 strtab_str_len = sizeof(strtab_str) - 1;
  char*  strtab         = (char*)((sd_u64)dyn - strtab_str_len);

  sd_u32        n_sym        = 5;
  sd_u32        symtab_bytes = n_sym * sizeof(sd_elf64_sym);
  sd_elf64_sym* symtab       = (sd_elf64_sym*)sd_align_down_64((sd_u64)strtab - symtab_bytes, 8);

  sd_u32         n_rela     = sizeof(sd_got_t) / sizeof(void*);
  sd_u32         rela_bytes = n_rela * sizeof(sd_elf64_rela);
  sd_elf64_rela* rela       = (sd_elf64_rela*)sd_align_down_64((sd_u64)symtab - rela_bytes, 8);

  char* interp = (char*)((sd_u64)rela - linker_path_len);

  // Copy original phdrs as they are except with modification to the PT_PHDR
  // since they are now at a different address
  for (sd_u32 i = 0; i < orig_n_phdrs; i += 1) {
    if (orig_phdrs[i].p_type == SD_PT_PHDR) {
      phdrs[i] = (sd_elf64_phdr){
        .p_type   = SD_PT_PHDR,
        .p_flags  = SD_PF_R,
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
  phdrs[orig_n_phdrs] = (sd_elf64_phdr){
      .p_type   = SD_PT_DYNAMIC,
      .p_flags  = SD_PF_R | SD_PF_W,
      .p_offset = 0,
      .p_vaddr  = (sd_u64)dyn,
      .p_paddr  = 0,
      .p_filesz = n_dyn * sizeof(sd_elf64_dyn),
      .p_memsz  = n_dyn * sizeof(sd_elf64_dyn),
      .p_align  = 8,
  };
  phdrs[orig_n_phdrs + 1] = (sd_elf64_phdr){
      .p_type   = SD_PT_INTERP,
      .p_flags  = SD_PF_R,
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

  dyn[0] = (sd_elf64_dyn){ .d_tag = SD_DT_NEEDED,  .d_un.d_val = strtab_libc_off    };
  dyn[1] = (sd_elf64_dyn){ .d_tag = SD_DT_STRTAB,  .d_un.d_val = (sd_u64)strtab     };
  dyn[2] = (sd_elf64_dyn){ .d_tag = SD_DT_STRSZ,   .d_un.d_val = strtab_str_len     };
  dyn[3] = (sd_elf64_dyn){ .d_tag = SD_DT_SYMTAB,  .d_un.d_val = (sd_u64)symtab     };
  dyn[4] = (sd_elf64_dyn){ .d_tag = SD_DT_SYMENT,  .d_un.d_val = sizeof(sd_elf64_sym)  };
  dyn[5] = (sd_elf64_dyn){ .d_tag = SD_DT_RELA,    .d_un.d_val = (sd_u64)rela       };
  dyn[6] = (sd_elf64_dyn){ .d_tag = SD_DT_RELASZ,  .d_un.d_val = rela_bytes         };
  dyn[7] = (sd_elf64_dyn){ .d_tag = SD_DT_RELAENT, .d_un.d_val = sizeof(sd_elf64_rela) };
  dyn[8] = (sd_elf64_dyn){ .d_tag = SD_DT_NULL,    .d_un.d_val = 0                  };

  sd_memcpy(strtab, strtab_str, strtab_str_len);

  symtab[0] = (sd_elf64_sym){0};
  symtab[1] = (sd_elf64_sym){
    .st_name  = strtab_dlopen_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };
  symtab[2] = (sd_elf64_sym){
    .st_name  = strtab_dlsym_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };
  symtab[3] = (sd_elf64_sym){
    .st_name  = strtab_dlclose_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };
  symtab[4] = (sd_elf64_sym){
    .st_name  = strtab_dlerror_name_off,
    .st_info  = 0x12,
    .st_other = 0,
    .st_shndx = 0,
    .st_value = 0,
    .st_size  = 0
  };

  // Tell the linker where to write function pointers inside `got` table
  rela[0] = (sd_elf64_rela){
    .r_offset = (sd_u64)(&sd_got) + 0 * sizeof(sd_u64),
    .r_info   = (sd_u64)1 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };
  rela[1] = (sd_elf64_rela){
    .r_offset = (sd_u64)(&sd_got) + 1 * sizeof(sd_u64),
    .r_info   = (sd_u64)2 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };
  rela[2] = (sd_elf64_rela){
    .r_offset = (sd_u64)(&sd_got) + 2 * sizeof(sd_u64),
    .r_info   = (sd_u64)3 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };
  rela[3] = (sd_elf64_rela){
    .r_offset = (sd_u64)(&sd_got) + 3 * sizeof(sd_u64),
    .r_info   = (sd_u64)4 << 32 | SD_ARCH_GLOB_DAT ,
    .r_addend = 0
  };

  sd_memcpy(interp, linker_path, linker_path_len);

  // The stack needs to be 16 byte aligned, so calculate and subtract the
  // total number of bytes needed for everything so the resulting address can
  // be easily aligned
  sd_u32 auxv_bytes  = n_auxv * sizeof(sd_elf64_auxv_t);
  sd_u32 n_env       = (sd_u64*)orig_auxv - orig_envp - 1;
  sd_u32 env_bytes   = (n_env + 1) * sizeof(sd_u64);
  sd_u32 n_argv      = orig_argc;
  sd_u32 argv_bytes  = n_argv * sizeof(sd_u64) + sizeof(sd_u64);
  sd_u32 argc_bytes  = sizeof(sd_u64);
  sd_u32 total_bytes = auxv_bytes + env_bytes + argv_bytes + argc_bytes;

  sd_u64* argc          = (sd_u64*)sd_align_down_64((sd_u64)interp - total_bytes, 16);
  sd_u64* argv          = argc + 1;
  sd_u64* envp          = argv + n_argv + 1;
  sd_elf64_auxv_t* auxv = (sd_elf64_auxv_t*)(envp + n_env + 1);

  // Keep the arguments as they were passed to the original program but
  // add one additional argument which is a pointer to the `got` table
  *argc            = n_argv;
  sd_memcpy((char*)argv, (char*)orig_argv, n_argv * sizeof(sd_u64));
  argv[n_argv]     = 0x0;

  // Copy all environment vars as they were since they now need to be
  // at a different address
  sd_memcpy((char*)envp, (char*)orig_envp, n_env * sizeof(sd_u64));
  envp[n_env] = 0x0;

  while (orig_auxv->a_type != SD_AT_NULL) {
    if (orig_auxv->a_type == SD_AT_PHDR) {
      *auxv = (sd_elf64_auxv_t){ .a_type = SD_AT_PHDR,  .a_un.a_val = (sd_u64)phdrs            };
    } else if (orig_auxv->a_type == SD_AT_PHNUM) {
      *auxv = (sd_elf64_auxv_t){ .a_type = SD_AT_PHNUM, .a_un.a_val = (sd_u64)n_phdrs          };
    } else if (orig_auxv->a_type == SD_AT_ENTRY) {
      *auxv = (sd_elf64_auxv_t){ .a_type = SD_AT_ENTRY, .a_un.a_val = (sd_u64)&sd_stage2_entry };
    } else if (orig_auxv->a_type == SD_AT_BASE) {
      *auxv = (sd_elf64_auxv_t){ .a_type = SD_AT_BASE,  .a_un.a_val = (sd_u64)linker_map       };
    } else {
      *auxv = *orig_auxv;
    }
    orig_auxv += 1;
    auxv      += 1;
  }
  *auxv     = (sd_elf64_auxv_t){ .a_type = SD_AT_NULL,  .a_un.a_val = (sd_u64)0                };

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
