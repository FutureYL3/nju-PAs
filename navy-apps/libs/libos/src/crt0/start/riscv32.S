.globl  _start
_start:
  move s0, zero
  mv sp, a0 // set sp to address of argc, and it will grow below it
  jal call_main
