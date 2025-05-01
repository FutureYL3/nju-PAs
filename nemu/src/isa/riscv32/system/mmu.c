/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <debug.h>

#define PTE_V (1UL << 0)
#define PTE_R (1UL << 1)
#define PTE_W (1UL << 2)
#define PTE_X (1UL << 3)
#define PTE_U (1UL << 4)
#define PTE_G (1UL << 5)
#define PTE_A (1UL << 6)
#define PTE_D (1UL << 7)

#define PTESIZE   4
#define PPN_MASK  ((1u << 22) - 1)

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  // 1) 取出一级页表的物理基址
  uint32_t root_ppn    = cpu.satp & PPN_MASK;       // 只保留低 22 位
  paddr_t page_dir_pa  = (paddr_t) root_ppn << 12;   // <<12 得到字节地址

  // 2) 计算一级页表（PDE）所在的物理地址，并读取它
  uint32_t idx1        = vaddr >> 22;               // 虚拟地址高 10 位
  paddr_t  pde_pa      = page_dir_pa + idx1 * PTESIZE;
  uint32_t pde_val     = paddr_read(pde_pa, 4);
  Assert((pde_val & PTE_V) != 0, "PDE must be valid! vaddr=0x%x, pde_pa=0x%x, pde_val=0x%x",
         vaddr, pde_pa, pde_val);
  Assert((pde_val & (PTE_R|PTE_W|PTE_X)) == 0,
         "PDE should not have R|W|X bits! vaddr=0x%x, pde_pa=0x%x, pde_val=0x%x",
         vaddr, pde_pa, pde_val);

  // 3) 从 PDE 中提取二级页表的 PPN→物理基址
  uint32_t pd_ppn      = (pde_val >> 10) & PPN_MASK; // 【先>>10再掩码】丢权限，只留 22 位
  paddr_t  page_table_pa = (paddr_t) pd_ppn << 12;    // 恢复成字节地址

  // 4) 计算二级页表（PTE）所在的物理地址，并读取它
  uint32_t idx2        = (vaddr >> 12) & 0x3ff;      // 中间 10 位
  paddr_t  pte_pa      = page_table_pa + idx2 * PTESIZE;
  uint32_t pte_val     = paddr_read(pte_pa, 4);
  Assert((pte_val & PTE_V) != 0, "PTE must be valid! vaddr=0x%x, pte_pa=0x%x, pte_val=0x%x",
         vaddr, pte_pa, pte_val);
  Assert((pte_val & (PTE_R|PTE_W|PTE_X)) != 0,
         "Leaf PTE must have at least one of R|W|X bits! vaddr=0x%x, pte_pa=0x%x, pte_val=0x%x",
         vaddr, pte_pa, pte_val);

  // 5) 从 PTE 中提取物理页帧号，合成最终物理地址
  uint32_t pt_ppn      = (pte_val >> 10) & PPN_MASK; // 同样先 >>10 再掩码
  paddr_t  pa_base     = (paddr_t) pt_ppn << 12;     // 页基址
  paddr_t  pa          = pa_base | (vaddr & 0xfff); // 加上页内偏移

  Assert(pa == vaddr, "Direct mapping check failed: pa=0x%x, va=0x%x, satp=0x%x",
         pa, vaddr, cpu.satp);
  return pa;
}
