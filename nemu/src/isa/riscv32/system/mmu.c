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
  /* get page table directory address */
  uint32_t root_ppn = cpu.satp & 0x003fffff;   // 22 bit
  paddr_t page_dir_pa = (paddr_t) root_ppn << 12;
  /* get the corresponding pte address */
  paddr_t pte_pa = page_dir_pa + (vaddr >> 22) * PTESIZE;
  /* get pte's value */
  uint32_t pte_val = paddr_read(pte_pa, 4);
  Assert((pte_val & PTE_V) == 1, "pte is not valid, pte value: 0x%x\n", pte_val);
  Assert((pte_val & PTE_R) == 0 && (pte_val & PTE_W) == 0 && (pte_val & PTE_X) == 0, 
    "page dir entry does not points to the second level page table, pte value: 0x%x, vaddr value: 0x%x\n", pte_val, vaddr);
  /* get second page table address */
  paddr_t page_table_pa = pte_val >> 10 << 12;
  /* get the corresponding pte address */
  pte_pa = page_table_pa + ((vaddr >> 12) & 0x003ff) * PTESIZE;
  /* get pte's value */
  pte_val = paddr_read(pte_pa, 4);
  Assert((pte_val & PTE_V) == 1, "pte is not valid, pte value: 0x%x\n", pte_val);
  Assert(!((pte_val & PTE_R) == 0 && (pte_val & PTE_W) == 0 && (pte_val & PTE_X) == 0), 
    "page tale entry is not a leaf pte, pte value: 0x%x\n", pte_val);
  /* get the translated physical address */
  paddr_t ret = (pte_val >> 10 << 12) | (vaddr & 0x00000fff);
  Assert(ret == vaddr, "va != pa\n");
  return ret;
}
