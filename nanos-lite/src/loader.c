#include <proc.h>
#include <elf.h>
#include <fs.h>

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

static uintptr_t loader(PCB *pcb, const char *filename) {
  /* we do not use flags and mode */
	int fd = fs_open(filename, 0, 0);
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

	// Elf_Phdr * phdr_table = (Elf_Phdr *) ((char *) &ramdisk_start + phoff);
	for (int i = 0; i < phnum; ++ i) {
		Elf_Phdr phdr = {};
    if (fs_read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
      panic("Failed to load program %s because can't read segment head %d", filename, i);
    }
		if (phdr.p_type == PT_LOAD) {
			void * vmem_addr = (void *) phdr.p_vaddr;
			size_t offset = phdr.p_offset;
			size_t filesz = phdr.p_filesz;
			size_t memsz = phdr.p_memsz;
      char *buf = (char *) malloc(filesz);
      fs_lseek(fd, offset, SEEK_SET);
      if (fs_read(fd, (void *) buf, filesz) != filesz) {
        panic("Failed to load program %s because can't read total segment %d to buffer", filename, i);
      }
      memcpy(vmem_addr, (void *) buf, filesz);
      free(buf);
			// ramdisk_read(vmem_addr, offset, filesz);
			if (memsz > filesz) {
				void *fileend = (void *) ((char *) vmem_addr + filesz);
				memset(fileend, 0, memsz - filesz);
			}
		}
	}

	return (uintptr_t) ehdr.e_entry;	
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

