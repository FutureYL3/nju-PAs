#include <memory.h>
#include <common.h>
#include <proc.h>

static void *pf = NULL;

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
bool first = true;
/* The brk() system call handler. */
// mm_brk() sets the program break to the value specified by brk
int mm_brk(uintptr_t brk) {
  uintptr_t old_brk = current->max_brk;
  if (brk > old_brk) {
    if (first) {
      // 找到 brk 落在哪一页
      uintptr_t old_page = ROUNDDOWN(old_brk, PGSIZE);
      uintptr_t new_pg = ROUNDDOWN(brk, PGSIZE);
      // 遍历所有页
      for (uintptr_t va = old_page; va <= new_pg; va += PGSIZE) {
        void *pa = new_page(1);
        map(&(current->as), (void *) va, pa, PTE_R | PTE_W);
      }

      current->max_brk = new_pg + PGSIZE;
      first = false;
    }
    else {
      uintptr_t new_pg = ROUNDDOWN(brk, PGSIZE);
      for (uintptr_t va = old_brk; va <= new_pg; va += PGSIZE) {
        void *pa = new_page(1);
        map(&(current->as), (void *) va, pa, PTE_R | PTE_W);
      }

      current->max_brk = new_pg + PGSIZE;
    }
  }

  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
