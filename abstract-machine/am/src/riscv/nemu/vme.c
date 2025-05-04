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
      // printf("0x%p\n", (char *) va);
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

#define PTESIZE   4
#define PPN_MASK  ((1u << 22) - 1)    // 22 位 PPN 掩码

/* helper function: check whether va has mapped to a physical page */
bool map_exist(AddrSpace *as, void *va) {
  // 1) 计算一级页表索引、PDE 地址
  uint32_t idx1   = (uintptr_t) va >> 22;
  uint32_t *pgdir = (uint32_t *) as->ptr;
  uint32_t *pde   = &(pgdir[idx1]);
  // 2) 如果还没创建二级页表，则映射不存在，返回false
  if (!(*pde & PTE_V)) {
    return false;
  }
  // 3) 如果存在PDE，则取出二级页表物理基址进一步检查
  uint32_t raw_pde = *pde;
  //   ——先右移 10 丢掉权限位，再掩码，只留下面22位
  uint32_t pd_ppn  = (raw_pde >> 10) & PPN_MASK;
  uint32_t *pt     = (uint32_t *)(pd_ppn << 12);
  // 4) 计算二级页表索引、PTE 地址
  uint32_t idx2 = ((uintptr_t) va >> 12) & 0x3ff;
  uint32_t *pte = &(pt[idx2]);
  // 5) 如果PTE不合法，则表明目前还没有映射，返回false
  if (!(*pte & PTE_V)) {
    return false;
  }
  // 6) 通过了所有检查，表明存在，返回true
  return true;
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  // 1) 计算一级页表索引、PDE 地址
  uint32_t idx1   = (uintptr_t) va >> 22;
  uint32_t *pgdir = (uint32_t *) as->ptr;
  uint32_t *pde   = &(pgdir[idx1]);

  // 2) 如果还没创建二级页表，就分配并安装一个新的
  if (!(*pde & PTE_V)) {
    void *new_pt = pgalloc_usr(PGSIZE);
    uint32_t ppn  = ((uintptr_t) new_pt >> 12) & PPN_MASK;
    *pde = (ppn << 10) | PTE_V;   // 只标记 valid，R/W/X 都留 0
  }

  // 3) 无论是刚刚创建，还是原本就有，都要取出二级页表物理基址
  uint32_t raw_pde = *pde;
  //   ——先右移 10 丢掉权限位，再掩码，只留下面22位
  uint32_t pd_ppn  = (raw_pde >> 10) & PPN_MASK;
  uint32_t *pt     = (uint32_t *)(pd_ppn << 12);

  // 4) 计算二级页表索引、PTE 地址
  uint32_t idx2 = ((uintptr_t) va >> 12) & 0x3ff;
  uint32_t *pte = &(pt[idx2]);

  // 5) 安装最终的叶子 PTE：先掩码再移位
  uint32_t ppn_pa = ((uintptr_t) pa >> 12) & PPN_MASK;
  *pte = (ppn_pa << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
}

#define MIE   0x08
#define MPIE  0x80

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  /* create the context */
  Context *context = (Context *) ((char *) kstack.end - sizeof(Context));
  /* set the entry */
  context->mepc = (uintptr_t) entry; // do not need to cooperate with `c->mepc += 4;`
  /* set the addr space ptr */
  context->pdir = as->ptr;
  /* difftest */
  context->mstatus = 0x1800; // to pass difftest
  /* open interrupt */
  context->mstatus |= MIE;
  context->mstatus |= MPIE;

  
  return context;
}
