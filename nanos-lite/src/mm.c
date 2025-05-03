#include <memory.h>
#include <common.h>
#include <proc.h>

static void *pf = NULL;
bool map_exist(AddrSpace *as, void *va);

void* new_page(size_t nr_page) {
  void *ret = pf;
  pf = (void *) ((char *) pf + nr_page * PGSIZE);
  printf("allocated free page from %p to %p\n", (char *) ret, (char *) pf);
  // printf("address of pf is %p\n", &pf);
  return ret;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  /* apply for memory page */
  void *ret = new_page(n / PGSIZE);
  /* zero the page space */
  memset(ret, 0, n);

  return ret;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80

extern PCB *current;
/* The brk() system call handler. */
// mm_brk() sets the program break to the value specified by brk
int mm_brk(uintptr_t brk) {
  uintptr_t old_brk = current->max_brk;
  printf("old brk = %p, new brk = %p\n", old_brk, brk);
  if (brk <= old_brk) {
    return 0;           // only support scale
  }

  uintptr_t first_page = ROUNDUP(old_brk, PGSIZE);
  uintptr_t last_page  = ROUNDDOWN(brk - 1, PGSIZE);
  /* map */
  for (uintptr_t va = first_page; va <= last_page; va += PGSIZE) {
    // TODO: implement pte_is_valid()
    if (!map_exist(&(current->as), (void*) va)) {
      void *pa = new_page(1);
      map(&(current->as), (void*) va, pa, PTE_R | PTE_W);
    }
  }

  current->max_brk = brk;
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
