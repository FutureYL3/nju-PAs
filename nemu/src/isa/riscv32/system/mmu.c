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

#define PTESIZE 4
#define PTE_V (1UL << 0)
#define PTE_R (1UL << 1)
#define PTE_W (1UL << 2)
#define PTE_X (1UL << 3)
#define PTE_U (1UL << 4)
#define PTE_G (1UL << 5)
#define PTE_A (1UL << 6)
#define PTE_D (1UL << 7)

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  uint32_t page_dir_pa = cpu.satp << 12;
  void *page_dir_ptr = (void *)(uintptr_t) page_dir_pa;
  void *pte = page_dir_ptr + (vaddr >> 22) * PTESIZE;
  uint32_t pte_val = paddr_read((paddr_t)(uintptr_t) pte, 4);
  Assert((pte_val & PTE_V) == 1, "pte is not valid, pte value: 0x%x\n", pte_val);
  Assert((pte_val & PTE_R) == 0 && (pte_val & PTE_W) == 0 && (pte_val & PTE_X) == 0, "page dir entry does not points to the second level page table, pte value: 0x%x\n", pte_val);
  uint32_t page_table_pa = pte_val >> 10 << 12;
  void *page_table_ptr = (void *)(uintptr_t) page_table_pa;
  pte = page_table_ptr + ((vaddr >> 12) & 0x003ff) * PTESIZE;
  pte_val = paddr_read((paddr_t)(uintptr_t) pte, 4);
  Assert((pte_val & PTE_V) == 1, "pte is not valid, pte value: 0x%x\n", pte_val);
  Assert(!((pte_val & PTE_R) == 0 && (pte_val & PTE_W) == 0 && (pte_val & PTE_X) == 0), "page tale entry is not a leaf pte, pte value: 0x%x\n", pte_val);
  paddr_t ret = (pte_val >> 10 << 12) | (vaddr & 0x00000fff);
  Assert(ret == vaddr, "va != pa\n");
  return ret;
}
