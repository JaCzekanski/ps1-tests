#include <asm.h>
#include <inline_s.h>

.extern FlushCache
FlushCache:
	li $t1, 0x44
	li $t2, 0xa0
	jr $t2
	nop

getPc:
	move $v0, $ra
	jr $ra
	nop

.globl codeRam
codeRam:
	addiu $sp, -4
	sw $ra, 0($sp)

	la $t0, getPc
	jalr $ra, $t0 # Get PC 
	nop

	lw $ra, 0($sp)
	addiu $sp, 4
	jr $ra
	nop

.globl codeScratchpad
codeScratchpad:
	addiu $sp, -4
	sw $ra, 0($sp)

	la $t0, getPc
	jalr $ra, $t0 # Get PC 
	nop

	lw $ra, 0($sp)
	addiu $sp, 4
	jr $ra
	nop
	