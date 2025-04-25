#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

#define PTESIZE 4

void map(AddrSpace *as, void *va, void *pa, int prot) {
  /* get the address of the page directory entry */
  void *pgdir_entry = as->ptr + ((uintptr_t) va >> 22) * PTESIZE;
  /* get the value of the page directory entry */
  uint32_t *pgdir_entry_val = (uint32_t *) pgdir_entry;
  if ((*pgdir_entry_val & PTE_V) == 0) {  // indicates that the second level page table is not created yet
    /* create the second level page table of this page dir entry */
    void *pgtable = pgalloc_usr(PGSIZE);
    /* set this page dir entry points to the second level page table, and set neccessary status bits, left r,w,x all 0 to indicate that this entry is not leaf */
    *pgdir_entry_val = (uint32_t) ((uintptr_t) pgtable >> 12 << 10 | PTE_V);
    printf("%d\n", *pgdir_entry_val & PTE_V);
    /* get the second level page table entry, 0x003ff is identical to 00000000001111111111*/
    void *pgtable_entry = pgtable + (((uintptr_t) va >> 12) & 0x003ff) * PTESIZE;
    /* get the value of second level page table entry */
    uint32_t *pgtable_entry_val = (uint32_t *) pgtable_entry;
    /* set second level page table entry points to the real physical page, and set default prot */
    *pgtable_entry_val = (uint32_t) ((uintptr_t) pa >> 12 << 10 | PTE_V | PTE_R | PTE_W | PTE_X);
  }
  else if ((*pgdir_entry_val & PTE_R) == 0 && (*pgdir_entry_val & PTE_W) == 0 && (*pgdir_entry_val & PTE_X) == 0) {
    /* get the second level page table entry, 0x003ff is identical to 00000000001111111111*/
    void *pgtable_entry = (void *) ((*((uint32_t *) pgdir_entry) >> 10 << 12) + (((uintptr_t) va >> 12) & 0x003ff) * PTESIZE);
    /* get the value of second level page table entry */
    uint32_t *pgtable_entry_val = (uint32_t *) pgtable_entry;
    /* set second level page table entry points to the real physical page, and set default prot */
    *pgtable_entry_val = (uint32_t) ((uintptr_t) pa >> 12 << 10 | PTE_V | PTE_R | PTE_W | PTE_X);
  }
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  /* create the context */
  Context *context = (Context *) ((char *) kstack.end - sizeof(Context));
  /* set the kernal thread entry */
  context->mepc = (uintptr_t) entry - 4; // cooperate with `c->mepc += 4;`
  /* difftest */
  context->mstatus = 0x1800; // to pass difftest
  
  return context;
}
