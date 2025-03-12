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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <elf.h>


/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

#ifdef CONFIG_IRINGBUF
/* for iringbuf */
#define MAX_IRINGBUF_SIZE 20
char iringbuf[MAX_IRINGBUF_SIZE][200];
int iringbuf_cur_next = 0;
#endif

void device_update();
void check_for_wp_change();

#if CONFIG_FTRACE
FuncSymbol *funcSymbols = NULL;
FILE *ftrace_log = NULL;
int indent_count = 0;
int func_entry_count = 0;
void init_ftrace(const char *ftrace_elf, const char *ftrace_log_file) {
  FILE *fp = fopen(ftrace_elf, "rb");
  if (!fp)
    panic("Can not open ftrace_elf file\n"); 

  // 1. 读取 ELF 头部
  Elf32_Ehdr ehdr;
  if (fread(&ehdr, 1, sizeof(ehdr), fp) != sizeof(ehdr)) 
    panic("failed to read elf header\n");
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) 
    panic("Not an ELF file\n");
  
  // 2. 读取节区头表
  Elf32_Shdr *shdrs = malloc(ehdr.e_shentsize * ehdr.e_shnum);
  if (!shdrs) 
    panic("malloc section headers failed\n");
  
  fseek(fp, ehdr.e_shoff, SEEK_SET);
  if (fread(shdrs, ehdr.e_shentsize, ehdr.e_shnum, fp) != ehdr.e_shnum) 
    panic("fread section headers failed\n");

  // 3. 读取节区名称字符串表（用于解析各个节区名称）
  Elf32_Shdr shstr_hdr = shdrs[ehdr.e_shstrndx];
  char *shstrtab = malloc(shstr_hdr.sh_size);
  if (!shstrtab) 
    panic("malloc shstrtab failed\n");

  fseek(fp, shstr_hdr.sh_offset, SEEK_SET);
  if (fread(shstrtab, 1, shstr_hdr.sh_size, fp) != shstr_hdr.sh_size) 
    panic("fread shstrtab failed\n");
  
  // 4. 定位符号表节区 (.symtab) 以及它的关联字符串表 (.strtab)
  Elf32_Shdr *symtab_hdr = NULL;
  Elf32_Shdr *strtab_hdr = NULL;
  for (int i = 0; i < ehdr.e_shnum; i++) {
    char *sec_name = shstrtab + shdrs[i].sh_name;
    if (strcmp(sec_name, ".symtab") == 0) {
      symtab_hdr = &shdrs[i];
      // 符号表关联的字符串表索引存储在 sh_link 字段中
      strtab_hdr = &shdrs[symtab_hdr->sh_link];
      break;
    }
  }
  if (!symtab_hdr || !strtab_hdr) 
    panic("Could not find symbol table or its string table\n");
  
  // 5. 读取符号表数据到一个 Elf32_Sym 数组中
  int num_symbols = symtab_hdr->sh_size / symtab_hdr->sh_entsize;
  Elf32_Sym *symtab = malloc(symtab_hdr->sh_size);
  if (!symtab) 
    panic("malloc symtab failed\n");
  
  fseek(fp, symtab_hdr->sh_offset, SEEK_SET);
  if (fread(symtab, symtab_hdr->sh_entsize, num_symbols, fp) != num_symbols) 
    panic("fread symtab failed\n");
  
  // 6. 读取符号表对应的字符串表数据
  char *strtab = malloc(strtab_hdr->sh_size);
  if (!strtab) 
    panic("malloc strtab failed\n");

  fseek(fp, strtab_hdr->sh_offset, SEEK_SET);
  if (fread(strtab, 1, strtab_hdr->sh_size, fp) != strtab_hdr->sh_size) 
    panic("fread strtab failed\n");

  fclose(fp);

  // 7. 筛选出类型为函数（STT_FUNC）的符号条目
  // 第一遍统计符合条件的符号数量
  for (int i = 0; i < num_symbols; i++) {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC)
    func_entry_count++;
  }

  // 分配自定义结构体数组，用于存储函数符号
  funcSymbols = malloc(func_entry_count * sizeof(FuncSymbol));
  if (!funcSymbols) 
    panic("malloc funcSymbols failed\n");

  // 第二遍将符号条目存入自定义数组，并解析符号名称（复制字符串）
  int j = 0;
  for (int i = 0; i < num_symbols; i++) {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
      // 使用 st_name 字段作为偏移量，在符号字符串表中查找名称
      char *name = strdup(strtab + symtab[i].st_name);
      if (!name) 
        panic("strdup failed\n");
      
      funcSymbols[j].name = name;
      funcSymbols[j].value = symtab[i].st_value;
      funcSymbols[j].size = symtab[i].st_size;
      j++;
    }
  }

	// 打开日志文件
	ftrace_log = stdout;
	if (ftrace_log_file != NULL) {
		ftrace_log = fopen(ftrace_log_file, "w");
		if (!ftrace_log)
			panic("failed to open ftrace log file\n");
	}
	Log("Ftrace log is written to %s", ftrace_log_file ? ftrace_log_file : "stdout");
}
#endif

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

#ifdef CONFIG_IRINGBUF
	/* for iringbuf */
	snprintf(iringbuf[iringbuf_cur_next], sizeof(iringbuf[0]), "%s\n", _this->logbuf);
	iringbuf_cur_next = (iringbuf_cur_next + 1) % MAX_IRINGBUF_SIZE;
#endif

#ifdef CONFIG_WATCHPOINT
	/* scan all the watchpoints */
	check_for_wp_change();	
#endif
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
