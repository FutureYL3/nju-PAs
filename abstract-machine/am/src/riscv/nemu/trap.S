#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15) f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

#define CONTEXT_SIZE  ((32 + 3 + 1) * XLEN)
#define OFFSET_SP     ( 2 * XLEN)
#define OFFSET_CAUSE  (32 * XLEN)
#define OFFSET_STATUS (33 * XLEN)
#define OFFSET_EPC    (34 * XLEN)
#define OFFSET_NP     (35 * XLEN)

.align 3
.globl __am_asm_trap
__am_asm_trap:
  # switch to kernal stack if needed
  csrrw sp, mscratch, sp   // (1) atomically exchange sp and mscratch
  bnez sp, save_context    // (2) take the branch if we trapped from user
  csrr sp, mscratch        // (3) if we trapped from kernel, restore the original sp

save_context:
  addi sp, sp, -CONTEXT_SIZE
  # store the register and csr, make room for general register usage
  MAP(REGS, PUSH)

  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  # store original sp and np
  csrr t0, mscratch          # Read mscratch into t0
  beqz t0, store_kernel_sp_np   # If mscratch is 0, branch to store current sp (kernel sp)
  STORE t0, OFFSET_SP(sp)
  li t1, 0
  STORE t1, OFFSET_NP(sp)      // if mscratch is not 0, then next privilege is user
  j after_store_sp_np

store_kernel_sp_np:
  addi t0, sp, CONTEXT_SIZE
  STORE t0, OFFSET_SP(sp)
  li t1, 1
  STORE t1, OFFSET_NP(sp)      // if mscratch is 0, then next privilege is kernal

after_store_sp_np:
  # make ksp(mscratch) = 0 to support re-entry of CTE
  csrw mscratch, 0

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1
  # handle interruption
  mv a0, sp
  jal __am_irq_handle

  # make sp point at returned context to achieve context switch and multi program
  mv sp, a0
  
  # Check if next privilege is user (OFFSET_NP(sp) == 0) before restoring context
  LOAD t0, OFFSET_NP(sp)           # Load the value at OFFSET_NP(sp) into t0
  bnez t0, skip_restore_mscratch   # If t0 is not zero (next privilege is kernel), skip restoring mscratch

  # If next privilege is user, restore user sp to mscratch for potential re-entry
  addi t1, sp, CONTEXT_SIZE        # Calculate the original user sp before trap (current sp + CONTEXT_SIZE)
  csrw mscratch, t1                # Write the calculated user sp back to mscratch

skip_restore_mscratch:
  # restore the context
  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  MAP(REGS, POP)

  addi sp, sp, CONTEXT_SIZE
  # switch sp to c->sp
  LOAD sp, -(34 * XLEN)(sp)
  # return to mepc to continue executing another process or current process
  mret
