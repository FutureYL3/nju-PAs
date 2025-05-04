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

#define MIE_MASK    0x08        // bit3
#define MIE_ZERO    0xfffffff7
#define L7BITS_MASK 0x7f        // bit0-6

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */ 
#ifdef CONFIG_ETRACE
  Log("Raised exception/interruption: The number is %d(" FMT_WORD "), at pc = " FMT_WORD "\n", NO, NO, epc);
#endif

  cpu.mepc = epc;
  cpu.mcause = NO;
  /* store MIE in MPIE */
  word_t mpie = (cpu.mstatus & MIE_MASK) << 4;
  word_t new_l8bits = mpie + (cpu.mstatus & L7BITS_MASK);
  cpu.mstatus = (cpu.mstatus & 0xffffff00) + new_l8bits;
  /* zero MIE */
  cpu.mstatus &= MIE_ZERO;

  return cpu.mtvec;
}

#define IRQ_TIMER 0x80000007  // for riscv32

word_t isa_query_intr() {
  /* because we make timer intr directly connect to cpu's INTR pin, so its value determines whether we got a timer interrupt */
  if (cpu.INTR == true && cpu.mtvec != 0) {  // cpu.mtvec == 0 indicates that the mtvec has not been initialized yet
    cpu.INTR = false;
    return IRQ_TIMER;
  }

  return INTR_EMPTY;
}
