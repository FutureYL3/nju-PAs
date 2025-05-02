#include <memory.h>
#include <common.h>

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

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
