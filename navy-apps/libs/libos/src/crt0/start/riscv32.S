.globl  _start
_start:
  move s0, zero
  # mv sp, a0     // set sp to address of argc, and sp will grow below it(no longer needed)
  jal call_main
