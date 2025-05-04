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
#include <stdio.h>

#define MIE_MASK    0x08        // bit3

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */ 
#ifdef CONFIG_ETRACE
  Log("Raised exception/interruption: The number is %d(" FMT_WORD "), at pc = " FMT_WORD "\n", NO, NO, epc);
#endif
  // printf("isa_raise_intr\n");
  cpu.mepc = epc;
  cpu.mcause = NO;
  /* store MIE in MPIE and zero MIE */
  word_t old_mstatus = cpu.mstatus;
  word_t old_mie_val = (old_mstatus >> 3) & 1;  // 获取 MIE 的值 (0 或 1)
  cpu.mstatus &= ~MIE_MASK;                     // 清除 MIE (bit 3)
  cpu.mstatus &= ~(1UL << 7);                   // 先清除 MPIE (bit 7)
  cpu.mstatus |= (old_mie_val << 7);            // 根据旧 MIE 的值设置 MPIE (bit 7)

  return cpu.mtvec;
}

#define IRQ_TIMER 0x80000007  // for riscv32

word_t isa_query_intr() {
  /* because we make timer intr directly connect to cpu's INTR pin, so its value determines whether we got a timer interrupt */
  if (cpu.INTR == true && cpu.mtvec != 0  // cpu.mtvec == 0 indicates that the mtvec has not been initialized yet
      && (cpu.mstatus & MIE_MASK) >> 3 == 1) { // also, cpu should be in open interrupt status

    cpu.INTR = false;
    // printf("Got IRQ_TIMER in NEMU\n");
    return IRQ_TIMER;
  }

  return INTR_EMPTY;
}
