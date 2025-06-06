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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

#if CONFIG_FTRACE
extern FuncSymbol *funcSymbols;
extern FILE *ftrace_log;
extern int indent_count;
extern int func_entry_count;

void ftrace_call_write(word_t pc, word_t dnpc) {
  /* find function name */
  int i;
  char *func_name = NULL;
  for (i = 0; i < func_entry_count; ++ i) {
    if (dnpc >= funcSymbols[i].value && dnpc < funcSymbols[i].value + funcSymbols[i].size) {
      func_name = funcSymbols[i].name;
      break;
    }
  }
  if (i == func_entry_count) { Log("can not find function call at pc = " FMT_WORD "\n", pc); return; }
  char log_str[200];
  char *p = log_str;
  for (int j = 0; j < indent_count; ++ j)  p += sprintf(p, "  ");
  sprintf(p, "call [%s@" FMT_WORD "]", func_name, funcSymbols[i].value);

  fprintf(ftrace_log, FMT_WORD ": %s\n", pc, log_str);
  indent_count++;
}

void ftrace_ret_write(word_t pc) {
	indent_count--;
  /* find function name */
  int i;
  char *func_name = NULL;
  for (i = 0; i < func_entry_count; ++ i) {
    if (pc >= funcSymbols[i].value && pc < funcSymbols[i].value + funcSymbols[i].size) {
      func_name = funcSymbols[i].name;
      break;
    }
  }
  if (i == func_entry_count) { Log("can not find function call at pc = " FMT_WORD "\n", pc); return; }
  char log_str[200];
  char *p = log_str;
  for (int j = 0; j < indent_count; ++ j)  p += sprintf(p, "  ");
  sprintf(p, "ret  [%s]", func_name);

  fprintf(ftrace_log, FMT_WORD ": %s\n", pc, log_str);
}
#endif

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_N, // none
	TYPE_J, TYPE_B, TYPE_R, TYPE_SI
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while (0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while (0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while (0)
#define immJ() do { *imm = SEXT((BITS(i, 30, 21) + (BITS(i, 20, 20) << 10) + (BITS(i, 19, 12) << 11) + (BITS(i, 31, 31) << 19)) << 1, 21); } while (0)
#define immB() do { *imm = SEXT((BITS(i, 11, 8) + (BITS(i, 30, 25) << 4) + (BITS(i, 7, 7) << 10) + (BITS(i, 31, 31) << 11)) << 1, 13); } while (0)
#define immSI() do { *imm = BITS(i, 24, 20); } while (0)

#define MIE_MASK    0x08      // bit 3
#define MPIE_MASK   0x80      // bit 7

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I:  src1R();          immI();  break;
    case TYPE_U:                    immU();  break;
    case TYPE_S:  src1R(); src2R(); immS();  break;
		case TYPE_J:  							  	immJ();  break;
		case TYPE_B:  src1R(); src2R(); immB();  break;
		case TYPE_R:  src1R(); src2R();				   break;
		case TYPE_SI: src1R();					immSI(); break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__;  \
}

  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 0110111", lui    , U , R(rd) = imm);
  INSTPAT("??????? ????? ????? 010 ????? 0000011", lw     , I , R(rd) = Mr(src1 + imm, 4));
	INSTPAT("??????? ????? ????? 001 ????? 0000011", lh     , I , R(rd) = SEXT(Mr(src1 + imm, 2), 16));
	INSTPAT("??????? ????? ????? 101 ????? 0000011", lhu    , I , R(rd) = (word_t) Mr(src1 + imm, 2));
	INSTPAT("??????? ????? ????? 000 ????? 0000011", lb     , I , R(rd) = SEXT(Mr(src1 + imm, 1), 8));
	INSTPAT("??????? ????? ????? 100 ????? 0000011", lbu    , I , R(rd) = (word_t) Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 010 ????? 0100011", sw     , S , Mw(src1 + imm, 4, src2));
	INSTPAT("??????? ????? ????? 001 ????? 0100011", sh     , S , Mw(src1 + imm, 2, BITS(src2, 15, 0)));
	INSTPAT("??????? ????? ????? 000 ????? 0100011", sb     , S , Mw(src1 + imm, 1, BITS(src2, 7, 0)));
	INSTPAT("??????? ????? ????? 000 ????? 0010011", addi   , I , R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? ??? ????? 0010111", auipc  , U , R(rd) = s->pc + imm);	
#if CONFIG_FTRACE
	INSTPAT("??????? ????? ????? ??? ????? 1101111", jal    , J , s->dnpc = s->pc + imm; R(rd) = s->snpc; if (rd == 1)  ftrace_call_write(s->pc, s->dnpc));
	INSTPAT("??????? ????? ????? 000 ????? 1100111", jalr   , I , s->dnpc = (imm + src1) & ~1; R(rd) = s->snpc; if (rd == 1)  ftrace_call_write(s->pc, s->dnpc); else if (rd == 0 && BITS(s->isa.inst.val, 19, 15) == 1 && imm == 0)  ftrace_ret_write(s->pc));
#else
	INSTPAT("??????? ????? ????? ??? ????? 1101111", jal    , J , s->dnpc = s->pc + imm; R(rd) = s->snpc);
	INSTPAT("??????? ????? ????? 000 ????? 1100111", jalr   , I , s->dnpc = (imm + src1) & ~1; R(rd) = s->snpc);
#endif
	INSTPAT("??????? ????? ????? 000 ????? 1100011", beq    , B , if (src1 == src2)  s->dnpc = imm + s->pc);
	INSTPAT("??????? ????? ????? 001 ????? 1100011", bne    , B , if (src1 != src2)  s->dnpc = imm + s->pc);
	INSTPAT("??????? ????? ????? 100 ????? 1100011", blt    , B , if ((int32_t) src1 < (int32_t) src2)  s->dnpc = imm + s->pc);
	INSTPAT("??????? ????? ????? 110 ????? 1100011", bltu   , B , if (src1 < src2)  s->dnpc = imm + s->pc);
	INSTPAT("??????? ????? ????? 101 ????? 1100011", bge    , B , if ((int32_t) src1 >= (int32_t) src2)  s->dnpc = imm + s->pc);
	INSTPAT("??????? ????? ????? 111 ????? 1100011", bgeu   , B , if (src1 >= src2)  s->dnpc = imm + s->pc);
	INSTPAT("0100000 ????? ????? 000 ????? 0110011", sub    , R , R(rd) = src1 - src2);
	INSTPAT("??????? ????? ????? 111 ????? 0010011", andi   , I , R(rd) = src1 & imm);
	INSTPAT("??????? ????? ????? 110 ????? 0010011", ori    , I , R(rd) = src1 | imm);
	INSTPAT("??????? ????? ????? 100 ????? 0010011", xori   , I , R(rd) = src1 ^ imm);
	INSTPAT("??????? ????? ????? 011 ????? 0010011", sltiu  , I , if (src1 < imm) R(rd) = 1; else R(rd) = 0);
	INSTPAT("??????? ????? ????? 010 ????? 0010011", slti   , I , if ((int32_t) src1 < (int32_t) imm) R(rd) = 1; else R(rd) = 0);
	INSTPAT("0000000 ????? ????? 000 ????? 0110011", add    , R , R(rd) = src1 + src2);
	INSTPAT("0000000 ????? ????? 011 ????? 0110011", sltu   , R , if (src1 < src2) R(rd) = 1; else R(rd) = 0);
	INSTPAT("0000000 ????? ????? 010 ????? 0110011", slt    , R , if ((int32_t) src1 < (int32_t) src2) R(rd) = 1; else R(rd) = 0);
	INSTPAT("0000000 ????? ????? 100 ????? 0110011", xor    , R , R(rd) = src1 ^ src2);
	INSTPAT("0000000 ????? ????? 111 ????? 0110011", and    , R , R(rd) = src1 & src2);
	INSTPAT("0000000 ????? ????? 110 ????? 0110011", or     , R , R(rd) = src1 | src2);
	INSTPAT("0000000 ????? ????? 001 ????? 0110011", sll    , R , R(rd) = src1 << BITS(src2, 4, 0));
	INSTPAT("0000000 ????? ????? 101 ????? 0110011", srl    , R , R(rd) = src1 >> BITS(src2, 4, 0));
	INSTPAT("0100000 ????? ????? 101 ????? 0110011", sra    , R , R(rd) = (int32_t) src1 >> BITS(src2, 4, 0));
	INSTPAT("0100000 ????? ????? 101 ????? 0010011", srai   , SI, R(rd) = (int32_t) src1 >> imm);
	INSTPAT("0000000 ????? ????? 001 ????? 0010011", slli   , SI, R(rd) = src1 << imm);
	INSTPAT("0000000 ????? ????? 101 ????? 0010011", srli   , SI, R(rd) = src1 >> imm);
	INSTPAT("0000001 ????? ????? 000 ????? 0110011", mul    , R , R(rd) = (int64_t)(int32_t) src1 * (int64_t)(int32_t) src2);
	INSTPAT("0000001 ????? ????? 001 ????? 0110011", mulh   , R , R(rd) = ((int64_t)(int32_t) src1 * (int64_t)(int32_t) src2) >> 32);
	INSTPAT("0000001 ????? ????? 011 ????? 0110011", mulhu  , R , R(rd) = ((uint64_t) src1 * (uint64_t) src2) >> 32);
	INSTPAT("0000001 ????? ????? 010 ????? 0110011", mulhsu , R , R(rd) = (((int64_t)(int32_t) src1) * ((int64_t)(uint32_t) src2)) >> 32);
	INSTPAT("0000001 ????? ????? 100 ????? 0110011", div    , R , if (src2 == 0) R(rd) = (int32_t) -1; else if ((int32_t) src1 == INT32_MIN && (int32_t) src2 == -1) R(rd) = (int32_t) INT32_MIN; else R(rd) = (int32_t) src1 / (int32_t) src2);
	INSTPAT("0000001 ????? ????? 101 ????? 0110011", divu   , R , if (src2 != 0) R(rd) = src1 / src2; else R(rd) = (word_t) -1);
	INSTPAT("0000001 ????? ????? 110 ????? 0110011", rem    , R , if (src2 == 0) R(rd) = src1; else if ((int32_t) src1 == INT32_MIN && (int32_t) src2 == -1) R(rd) = 0; else R(rd) = (int32_t) src1 % (int32_t) src2);
	INSTPAT("0000001 ????? ????? 111 ????? 0110011", remu   , R , if (src2 != 0) R(rd) = src1 % src2; else R(rd) = src1);


	INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I, do { if (imm == 0x305) {if (rd != 0) R(rd) = cpu.mtvec; cpu.mtvec = src1;} else if (imm == 0x300) {if (rd != 0) R(rd) = cpu.mstatus; cpu.mstatus = src1;} else if (imm == 0x341) {if (rd != 0) R(rd) = cpu.mepc; cpu.mepc = src1;} else if (imm == 0x342) {if (rd != 0) R(rd) = cpu.mcause; cpu.mcause = src1;} else if (imm == 0x180) {if (rd != 0) R(rd) = cpu.satp; cpu.satp = src1;} else if (imm == 0x340) {if (rd != 0) R(rd) = cpu.mscratch; cpu.mscratch = src1;} else panic("unknown csr %x at pc = " FMT_WORD "\n", imm, s->pc); } while (0));
	INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs  , I, do { if (imm == 0x305) {if (src1 != 0) cpu.mtvec |= src1; R(rd) = cpu.mtvec;} else if (imm == 0x300) {if (src1 != 0) cpu.mstatus |= src1; R(rd) = cpu.mstatus;} else if (imm == 0x341) {if (src1 != 0) cpu.mepc |= src1; R(rd) = cpu.mepc;} else if (imm == 0x342) {if (src1 != 0) cpu.mcause |= src1; R(rd) = cpu.mcause;} else if (imm == 0x180) {if (src1 != 0) cpu.satp |= src1; R(rd) = cpu.satp;} else if (imm == 0x340) {R(rd) = cpu.mscratch; if (src1 != 0) cpu.mscratch |= src1;} else panic("unknown csr %x at pc = " FMT_WORD "\n", imm, s->pc); } while (0));
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
 	INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , N, s->dnpc = isa_raise_intr(R(17) == -1 ? -1 : 1, s->pc));  // R(17) is $a7, -1 is yield, 1 is syscall
	INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , N, s->dnpc = cpu.mepc; word_t old_mstatus = cpu.mstatus; word_t mpie_val = (old_mstatus >> 7) & 1; cpu.mstatus &= ~MIE_MASK; cpu.mstatus |= (mpie_val << 3); cpu.mstatus |= MPIE_MASK);
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
