#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <elf.h>
#include <link.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

// Custom gnu_hash_table (not in standard headers)
struct gnu_hash_table {
  uint32_t nbuckets;
  uint32_t symoffset;
  uint32_t bloom_size;
  uint32_t bloom_shift;
  uint64_t bloom[0];
};

// Get RIP (instruction pointer) for position-independent base calculation
static inline uint64_t get_rip(void) {
    uint64_t rip;
    asm volatile ("lea 0(%%rip), %0" : "=r" (rip));
    return rip;
}

// Utility functions (adapted from original)
size_t _strlen(const char* str) {
  size_t len = 0;
  while (str[len] != 0) len++;
  return len;
}

void _strcat(char* dest, const char* src) {
  char* ptr = dest + _strlen(dest);
  while (*src != 0) *ptr++ = *src++;
  *ptr = 0;
}

void _to_hex(uint64_t value, char* buffer) {  // Adapted for 64-bit
  const char hex_chars[] = "0123456789ABCDEF";
  buffer[0] = '0'; buffer[1] = 'x';
  for (int i = 15; i >= 0; i--) {
    buffer[2 + (15 - i)] = hex_chars[(value >> (i * 4)) & 0xF];
  }
  buffer[18] = 0;
}

void _to_dec(uint64_t value, char* buffer) {
  if (value == 0) {
    buffer[0] = '0'; buffer[1] = 0; return;
  }
  char temp[32];
  int i = 0;
  while (value > 0) {
    temp[i++] = (value % 10) + '0';
    value /= 10;
  }
  int j = 0;
  while (i > 0) {
    buffer[j++] = temp[--i];
  }
  buffer[j] = 0;
}

int _strcmp(const char* s1, const char* s2) {
  while (*s1 && *s1 == *s2) { s1++; s2++; }
  return *s1 - *s2;
}

char* _strstr(const char* hay, const char* needle) {
  int len = _strlen(needle);
  if (len == 0) return (char*)hay;
  for (; *hay; hay++) {
    if (_strcmp(hay, needle) == 0) return (char*)hay;
  }
  return NULL;
}

long _atoi(const char* s) {
  long n = 0;
  while (*s >= '0' && *s <= '9') {
    n = n * 10 + *s - '0';
    s++;
  }
  return n;
}

// Parse field from space-separated line (0-indexed)
char* get_field(char* line, int n) {
  for (int i = 0; i < n; i++) {
    while (*line && *line != ' ') line++;
    while (*line == ' ') line++;
  }
  return line;
}

// ELF parsing functions
Elf64_Phdr *elf_get_phdr(void *base, Elf64_Word type) {
  if (!base) return NULL;
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;
  Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)base + ehdr->e_phoff);
  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type == type) return &phdr[i];
  }
  return NULL;
}

Elf64_Dyn *elf_get_dyn(void *base, Elf64_Sxword tag) {
  if (!base) return NULL;
  Elf64_Phdr *dynamic = elf_get_phdr(base, PT_DYNAMIC);
  if (dynamic) {
    Elf64_Dyn *entry = (Elf64_Dyn *)((char *)base + dynamic->p_vaddr);
    while (entry->d_tag != DT_NULL) {
      if (entry->d_tag == tag) return entry;
      entry++;
    }
  }
  return NULL;
}

// Get module base using GOT and link_map
void *get_module_handle(const char *module) {
  uint64_t rip = get_rip();
  void *base = (void *)(rip & ~0xFFFUL); // page align down
  int attempts = 0;
  while (*(uint32_t *)base != 0x464c457f && attempts < 1024 && (uintptr_t)base > 0x100000) {
    base = (void *)((uintptr_t)base - 0x1000);
    attempts++;
  }
  if (*(uint32_t *)base != 0x464c457f) return NULL;

  Elf64_Dyn *got = elf_get_dyn(base, DT_PLTGOT);
  if (!got) return NULL;
  uint64_t *ptrs = (uint64_t *)got->d_un.d_ptr;
  struct link_map *map = (struct link_map *)ptrs[1]; // Standard for glibc
  while (map) {
    if (module == NULL || _strstr(map->l_name, module)) return (void *)map->l_addr;
    map = map->l_next;
  }
  return NULL;
}

// GNU hash function
uint32_t gnu_hash(const char *name) {
  uint32_t h = 5381;
  for (; *name; name++) h = (h << 5) + h + *name;
  return h;
}

// Lookup symbol using GNU hash
void *gnu_lookup(const char *name, void *hash_tbl, const Elf64_Sym *symtab, const char *strtab) {
  struct gnu_hash_table *hashtab = (struct gnu_hash_table *)hash_tbl;
  if (hashtab->bloom_size == 0) return NULL;
  uint32_t namehash = gnu_hash(name);
  uint32_t nbuckets = hashtab->nbuckets;
  uint32_t symoffset = hashtab->symoffset;
  uint32_t bloom_size = hashtab->bloom_size;
  uint32_t bloom_shift = hashtab->bloom_shift;
  uint64_t *bloom = hashtab->bloom;
  uint64_t word = bloom[(namehash / 64) % bloom_size];
  uint64_t mask = 1LL << (namehash % 64) | 1LL << ((namehash >> bloom_shift) % 64);
  if ((word & mask) != mask) return NULL;
  uint32_t *buckets = (uint32_t *)(bloom + bloom_size);
  uint32_t *chain = buckets + nbuckets;
  uint32_t symix = buckets[namehash % nbuckets];
  if (symix < symoffset) return NULL;
  for (;;) {
    const char *symname = strtab + symtab[symix].st_name;
    uint32_t hash = chain[symix - symoffset];
    if ((namehash | 1) == (hash | 1) && _strcmp(name, symname) == 0) {
      return (void *)symtab[symix].st_value;
    }
    if (hash & 1) break;
    symix++;
  }
  return NULL;
}

// Get proc address
void *get_proc_address(void *module, const char *func_name) {
  if (!module) return NULL;
  Elf64_Dyn *strtab = elf_get_dyn(module, DT_STRTAB);
  Elf64_Dyn *symtab = elf_get_dyn(module, DT_SYMTAB);
  if (!strtab || !symtab) return NULL;
  const char *strs = (const char *)strtab->d_un.d_ptr;
  Elf64_Sym *syms = (Elf64_Sym *)symtab->d_un.d_ptr;
  Elf64_Dyn *hash = elf_get_dyn(module, DT_GNU_HASH);
  if (hash) {
    return gnu_lookup(func_name, (void *)hash->d_un.d_ptr, syms, strs);
  }
  return NULL; // Fall back if no GNU hash
}

// Shellcode entry
void ShellcodeEntry(void) {
  char libc_s[] = "libc.so.6";

  void *hLibc = get_module_handle(libc_s);
  if (hLibc == NULL) {
    char msg[] = "Failed to find libc\n";
    write(1, msg, _strlen(msg));
    return;
  }

  typedef pid_t (*FP_GetPID)(void);
  typedef int (*FP_GetPrio)(int, int);
  typedef int (*FP_Open)(const char*, int, mode_t);
  typedef ssize_t (*FP_Read)(int, void*, size_t);
  typedef int (*FP_Close)(int);
  typedef ssize_t (*FP_Readlink)(const char*, char*, size_t);
  typedef int (*FP_Gettimeofday)(struct timeval*, void*);
  typedef long (*FP_Sysconf)(int);
  typedef struct tm* (*FP_Gmtime)(const time_t*);
  typedef ssize_t (*FP_Write)(int, const void*, size_t);

  FP_GetPID _GetPID = (FP_GetPID)get_proc_address(hLibc, "getpid");
  FP_GetPrio _GetPrio = (FP_GetPrio)get_proc_address(hLibc, "getpriority");
  FP_Open _Open = (FP_Open)get_proc_address(hLibc, "open");
  FP_Read _Read = (FP_Read)get_proc_address(hLibc, "read");
  FP_Close _Close = (FP_Close)get_proc_address(hLibc, "close");
  FP_Readlink _Readlink = (FP_Readlink)get_proc_address(hLibc, "readlink");
  FP_Gettimeofday _Gettimeofday = (FP_Gettimeofday)get_proc_address(hLibc, "gettimeofday");
  FP_Sysconf _Sysconf = (FP_Sysconf)get_proc_address(hLibc, "sysconf");
  FP_Gmtime _Gmtime = (FP_Gmtime)get_proc_address(hLibc, "gmtime");
  FP_Write _Write = (FP_Write)get_proc_address(hLibc, "write");

  if (!_GetPID || !_Open || !_Read || !_Close || !_Readlink || !_Gettimeofday || !_Sysconf || !_Gmtime || !_Write) {
    char msg[] = "Failed to resolve functions\n";
    _Write(1, msg, _strlen(msg));
    return;
  }

  char buffer[2048];
  buffer[0] = 0;
  char tmp[64];

  char newline[] = "\n";
  char space[] = " ";
  char t_pid[] = "PID: ";
  _strcat(buffer, t_pid);
  _to_hex(_GetPID(), tmp);
  _strcat(buffer, tmp);
  _strcat(buffer, newline);

  char t_cmd[] = "CMD: ";
  _strcat(buffer, t_cmd);
  char cmd_path[] = "/proc/self/cmdline";
  int fd = _Open(cmd_path, O_RDONLY, 0);
  if (fd > 0) {
    char cmd[512];
    ssize_t len = _Read(fd, cmd, 512);
    _Close(fd);
    for (ssize_t i = 0; i < len - 1; i++) {
      if (cmd[i] == 0) cmd[i] = ' ';
    }
    if (len > 0) cmd[len - 1] = 0;
    _strcat(buffer, cmd);
  }
  _strcat(buffer, newline);

  char t_path[] = "Path: ";
  _strcat(buffer, t_path);
  char exe_path[] = "/proc/self/exe";
  char pathBuf[260];
  ssize_t path_len = _Readlink(exe_path, pathBuf, 260);
  if (path_len > 0) pathBuf[path_len] = 0;
  else pathBuf[0] = 0;
  _strcat(buffer, pathBuf);
  _strcat(buffer, newline);

  char t_prio[] = "Priority: ";
  _strcat(buffer, t_prio);
  _to_hex(_GetPrio(PRIO_PROCESS, 0), tmp);
  _strcat(buffer, tmp);
  _strcat(buffer, newline);

  char t_date[] = "Created: ";
  char dot[] = ".";
  char colon[] = ":";
  _strcat(buffer, t_date);
  struct timeval tv;
  _Gettimeofday(&tv, NULL);
  time_t current = tv.tv_sec;
  long clk_tck = _Sysconf(_SC_CLK_TCK);
  char stat_path[] = "/proc/self/stat";
  fd = _Open(stat_path, O_RDONLY, 0);
  if (fd > 0) {
    char line[1024];
    ssize_t len = _Read(fd, line, 1024);
    _Close(fd);
    if (len > 0) line[len] = 0;
    char* field = get_field(line, 21); // 0-indexed, field 22 is starttime
    long start_jiffies = _atoi(field);
    char uptime_path[] = "/proc/uptime";
    fd = _Open(uptime_path, O_RDONLY, 0);
    if (fd > 0) {
      char upline[64];
      len = _Read(fd, upline, 64);
      _Close(fd);
      if (len > 0) upline[len] = 0;
      long uptime = _atoi(upline);
      time_t creation = current - uptime + (start_jiffies / clk_tck);
      struct tm *st = _Gmtime(&creation);
      _to_dec(st->tm_year + 1900, tmp); _strcat(buffer, tmp); _strcat(buffer, dot);
      _to_dec(st->tm_mon + 1, tmp); _strcat(buffer, tmp); _strcat(buffer, dot);
      _to_dec(st->tm_mday, tmp); _strcat(buffer, tmp); _strcat(buffer, space);
      _to_dec(st->tm_hour, tmp); _strcat(buffer, tmp); _strcat(buffer, colon);
      _to_dec(st->tm_min, tmp); _strcat(buffer, tmp);
    }
  }
  _strcat(buffer, newline);
  _strcat(buffer, newline);

  char t_mod[] = "Modules:\n";
  char arrow[] = " -> ";
  _strcat(buffer, t_mod);
  uint64_t rip2 = get_rip();
  void *base2 = (void *)(rip2 & ~0xFFFUL);
  int attempts2 = 0;
  while (*(uint32_t *)base2 != 0x464c457f && attempts2 < 1024 && (uintptr_t)base2 > 0x100000) {
    base2 = (void *)((uintptr_t)base2 - 0x1000);
    attempts2++;
  }
  if (*(uint32_t *)base2 != 0x464c457f) {
    char msg[] = "Failed to find ELF header for modules\n";
    _Write(1, msg, _strlen(msg));
    return;
  }
  Elf64_Dyn *got2 = elf_get_dyn(base2, DT_PLTGOT);
  if (!got2) {
    char msg[] = "Failed to find GOT for modules\n";
    _Write(1, msg, _strlen(msg));
    return;
  }
  uint64_t *ptrs2 = (uint64_t *)got2->d_un.d_ptr;
  struct link_map *map = (struct link_map *)ptrs2[1];
  int count = 0;
  while (map && count < 8) {
    if (map->l_name && map->l_name[0]) _strcat(buffer, map->l_name);
    _strcat(buffer, arrow);
    _to_hex(map->l_addr, tmp);
    _strcat(buffer, tmp);
    _strcat(buffer, newline);
    map = map->l_next;
    count++;
  }
  _strcat(buffer, newline);

  char t_env[] = "Env(start):\n";
  _strcat(buffer, t_env);
  char env_path[] = "/proc/self/environ";
  fd = _Open(env_path, O_RDONLY, 0);
  if (fd > 0) {
    char env[512];
    ssize_t len = _Read(fd, env, 150);
    _Close(fd);
    if (len > 0) env[len] = 0;
    _strcat(buffer, env);
  }

  _Write(1, buffer, _strlen(buffer));
}

void EndShellCode(void) {}

int main() {
  unsigned char* start = (unsigned char*)ShellcodeEntry;
  unsigned char* end = (unsigned char*)EndShellCode;
  size_t size = end - start;
  if (size == 0 || size > 20000) size = 4096;
  void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    printf("mmap failed\n");
    return 1;
  }
  memcpy(mem, start, size);
  printf("Shellcode Size: %zu bytes\n", size);
  printf("Running...\n");
  ((void(*)())mem)();
  munmap(mem, size);
  return 0;
}