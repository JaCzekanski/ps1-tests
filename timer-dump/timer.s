#include <asm.h>
#include <inline_s.h>

.text

.globl DumpTimerValues
DumpTimerValues:

    li $a0, 0x1F801100 # Read address
    li $a1, 0x1F800000 # Write address
    li $a2, 0x1F80105A # SIO_CTRL

    li $v0, 64        # loop count

    # zero             # RTS on
    li $v1, (1<<5)     # RTS off


_loop:
    
    sh $zero, 0($a2)  # gpio HIGH

    # Read 4x timer value
    lh $t0, 0($a0)
    lh $t1, 0($a0)
    lh $t2, 0($a0)
    lh $t3, 0($a0)

    sh $v1, 0($a2)  # gpio LOW
    
    # Read 4x timer value
    lh $t4, 0($a0)
    lh $t5, 0($a0)
    lh $t6, 0($a0)
    lh $t7, 0($a0)
    
    # Save results to scratchpad
    sh $t0, 0($a1)
    sh $t1, 2($a1)
    sh $t2, 4($a1)
    sh $t3, 6($a1)
    sh $t4, 8($a1)
    sh $t5, 10($a1)
    sh $t6, 12($a1)
    sh $t7, 14($a1)

    addiu $a1, $a1, 16
    subu $v0, $v0, 1
    bnez $v0, _loop

    jr $ra
    nop
