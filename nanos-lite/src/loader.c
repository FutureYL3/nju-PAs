#include <proc.h>
#include <elf.h>
#include <fs.h>
#include <common.h>
#include <memory.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_RISCV32__)
# define EXPECT_TYPE EM_RISCV
#else
# error Unsupported ISA
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

extern char ramdisk_start[];
extern char ramdisk_end[];

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80

static uintptr_t loader(PCB *pcb, const char *filename) {
  /* we do not use flags and mode */
	int fd = fs_open(filename, 0, 0);
  // printf("get fd = %d\n", fd);
	/* get ELF header */
  Elf_Ehdr ehdr = {};
  if (fs_read(fd, &ehdr, sizeof(Elf_Ehdr)) < 0) {
    panic("Failed to load program %s because can't open file", filename);
  }
  
	/* check for magic number */
	assert(*(uint32_t *)ehdr.e_ident == 0x464c457f); // for little endian check
	/* check for ISA type */
	assert(ehdr.e_machine == EXPECT_TYPE);

	/* get number of segments to be loaded and offset of Elf_Phdr */
	uint32_t phnum = ehdr.e_phnum;
	uint32_t phoff = ehdr.e_phoff;
  /* set file open offset to phoff */
  fs_lseek(fd, phoff, SEEK_SET);
  
  // printf("get phnum = %x, phoff = %x\n", phnum, phoff);

	// Elf_Phdr * phdr_table = (Elf_Phdr *) ((char *) &ramdisk_start + phoff);
	for (int i = 0; i < phnum; ++ i) {
    fs_lseek(fd, phoff + i * sizeof(Elf_Phdr), SEEK_SET);
		Elf_Phdr phdr = {};
    if (fs_read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
      panic("Failed to load program %s because can't read segment head %d", filename, i);
    }
		if (phdr.p_type == PT_LOAD) {
			void *vmem_addr = (void *) phdr.p_vaddr;
      void *cur_vaddr = vmem_addr;
			size_t offset = phdr.p_offset;
			size_t filesz = phdr.p_filesz;
			size_t memsz = phdr.p_memsz;
      size_t loadedsz = 0;
      /* handle pure bss segment */
      if (filesz == 0 && memsz > 0) {
        int bss_size = memsz;
        while (bss_size > 0) {
          void *newpg_pa = new_page(1);
          map(&(pcb->as), cur_vaddr, newpg_pa, PTE_R | PTE_W | PTE_X);
          memset(newpg_pa, 0, PGSIZE);
          bss_size -= PGSIZE;
          cur_vaddr += PGSIZE;
        }
        continue;
      }
      // char *buf = (char *) new_page(1);
      // char buf[filesz];
      // memset(buf, 0, PGSIZE);
      // printf("buffer ranges from %p to %p\n", buf, buf + filesz);
      // char buf[filesz];
      fs_lseek(fd, offset, SEEK_SET);
      while (loadedsz < filesz) {
        /* apply for new physical page */
        void *newpg_pa = new_page(1);
        /* map current vaddr to paddr */
        map(&(pcb->as), cur_vaddr, newpg_pa, PTE_R | PTE_W | PTE_X);
        
        size_t readlen = filesz - loadedsz >= PGSIZE ? PGSIZE : filesz - loadedsz;
        /* write to the physical page */
        if (fs_read(fd, newpg_pa, readlen) != readlen) {
          panic("Failed to load program %s: buffer read less than %d bytes", filename, readlen);
        }
        /* write to the physical page */
        // memcpy(newpg_pa, (void *) buf, readlen);

        // offset += readlen;
        loadedsz += readlen;

        /* zero the "memsz - filesz" part(if has any) */
        if (loadedsz >= filesz) {
          size_t exceed_bytes = readlen;
          int bss_size = memsz - filesz;
          if (bss_size > 0) {
            if (readlen != PGSIZE) {
              /* handle the already allocated page */
              memset(newpg_pa + exceed_bytes, 0, PGSIZE - exceed_bytes);
              bss_size -= (int) (PGSIZE - exceed_bytes);
            }
            /* handle the remaining "memsz - filesz" part(if has any) */
            while (bss_size > 0) {
              void *newpg_pa = new_page(1);
              cur_vaddr += PGSIZE;
              map(&(pcb->as), cur_vaddr, newpg_pa, PTE_R | PTE_W | PTE_X);
              memset(newpg_pa, 0, PGSIZE);
              bss_size -= PGSIZE;
            }
            break;
          }
        }

        cur_vaddr += readlen;
      }
      


      // memcpy(vmem_addr, (void *) buf, filesz);
      // // free(buf);
			// // ramdisk_read(vmem_addr, offset, filesz);
			// if (memsz > filesz) {
			// 	void *fileend = (void *) ((char *) vmem_addr + filesz);
			// 	memset(fileend, 0, memsz - filesz);
			// }
      // printf("vmem_addr = %p, offset = %x, filesz = %x, memsz = %x\n", vmem_addr, offset, filesz, memsz);
		}
	}

  fs_close(fd);

	return (uintptr_t) ehdr.e_entry;	
}

void naive_uload(PCB *pcb, const char *filename) {
  Log("Loading program: %s", filename);
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area kstack = RANGE(pcb->stack, pcb->stack + STACK_SIZE);
  Context *context = kcontext(kstack, entry, arg);
  pcb->cp = context;
}

#define NR_PAGE 8
#ifdef STACK_SIZE
#undef STACK_SIZE
#define STACK_SIZE NR_PAGE * PGSIZE
#endif
extern Area heap;
/* make sure that the argv[0] is always the executed filename, this is ensured by caller */
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  /* create addr space */
  protect(&(pcb->as));
  /* load the user program and get the entry */
  Log("Loading program: %s", filename);
  void (*entry)(void *) = (void (*)(void *)) loader(pcb, filename); 
  Log("Load program %s success", filename);
  /* create context in kernel stack */
  Area kstack = RANGE(pcb->stack, pcb->stack + STACK_SIZE);
  Context *context = ucontext(NULL, kstack, entry);
  /* apply for new stack memory */
  void *end = (void *) ((char *) new_page(NR_PAGE) + STACK_SIZE);
  printf("end is %p\n", (char *) end);
  /* set the passed arguments and environment variables */
  // printf("%s\n", filename);
  // printf("%p\n", *argv);
  // printf("%p\n", *envp);
  int argc = 0;
  char *last_end = (char *) end, *start;
  while (argv[argc] != NULL) {
    // printf("address of %s: %p\n", argv[argc], argv[argc]);
    size_t len = strlen(argv[argc]) + 1; // plus 1 for `\0`
    start = (char *) last_end - len;
    memcpy(start, argv[argc], len);
    last_end = start;
    ++argc;
  }
  char *argv_start = last_end;
  int envc = 0;
  /* avoid bad envp pointer address from libc */
  while (((uintptr_t) *envp < 0x88000000 && (uintptr_t) *envp > 0x80000000) && envp[envc] != NULL) {
    // printf("address of %s: %p\n", envp[envc], envp[envc]);
    size_t len = strlen(envp[envc]) + 1; // plus 1 for `\0`
    start = (char *) last_end - len;
    memcpy(start, envp[envc], len);
    last_end = start;
    ++envc;
  }
  char *envp_start = last_end;
  uintptr_t *p_end = (uintptr_t *) ROUNDDOWN(last_end, 4); // align for 4 bytes
  *(--p_end) = 0;
  while (envc--) {
    *(--p_end) = (uintptr_t) envp_start;
    envp_start += strlen((const char *) envp_start) + 1; // plus 1 for `\0`
  }
  *(--p_end) = 0;
  int count = argc;
  while (count--) {
    *(--p_end) = (uintptr_t) argv_start;
    argv_start += strlen((const char *) argv_start) + 1; // plus 1 for `\0`
  }
  *(--p_end) = argc;
  /* set GPRX to address of argc */
  context->GPRx = (uintptr_t) p_end;

  pcb->cp = context;
}

