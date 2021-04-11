#include <asm.h>
#include <inline_s.h>

.text

.globl asm_write_32
asm_write_32:
    sw $a1, 0($a0)

    jr $ra
    nop

.globl asm_write_16
asm_write_16:
    sh $a1, 0($a0)

    jr $ra
    nop

.globl asm_write_8
asm_write_8:
    sb $a1, 0($a0)

    jr $ra
    nop

