#include <proc.h>
#include <elf.h>

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
	/* Only for single program in ramdisk */
	Elf_Ehdr *ehdr = (Elf_Ehdr *) &ramdisk_start;
	// check for magic number
	assert(*(uint32_t *)ehdr->e_ident == 0x464c457f); // for little endian check
	// check for ISA type
	assert(ehdr->e_machine == EXPECT_TYPE);
	// load to memory
	uint16_t phnum = ehdr->e_phnum;
	uint16_t phoff = ehdr->e_phoff;

	Elf_Phdr * phdr_table = (Elf_Phdr *) ((char *) &ramdisk_start + phoff);
	for (int i = 0; i < phnum; ++ i) {
		Elf_Phdr *phdr = &phdr_table[i];
		if (phdr->p_type == PT_LOAD) {
			void * vmem_addr = (void *) phdr->p_vaddr;
			size_t offset = phdr->p_offset;
			size_t filesz = phdr->p_filesz;
			size_t memsz = phdr->p_memsz;
			ramdisk_read(vmem_addr, offset, filesz);
			if (memsz > filesz) {
				void *fileend = (void *) ((char *) vmem_addr + filesz);
				memset(fileend, 0, memsz - filesz);
			}
		}
	}

	return (uintptr_t)ehdr->e_entry;	
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

