#include <asm.h>
#include <inline_s.h>
#include <gtereg_s.h>

.text

FlushCache:
	li $t1, 0x44
	li $t2, 0xa0
	jr $t2

.global GTE_INIT
.type GTE_INIT, @function
GTE_INIT:
	addiu	$sp, -4
	sw		$ra, 0($sp)

	jal		EnterCriticalSection
	nop

	mfc0	$v0, $12				# Get SR
	lui		$v1, 0x4000				# Set bit to enable cop2
	or		$v0, $v1
	mtc0	$v0, $12				# Set new SR

	jal		ExitCriticalSection
	nop

	lw		$ra, 0($sp)
	addiu	$sp, 4
	jr		$ra
	nop

.globl GTE_READ
# uint32_t GTE_READ(uint8_t reg);
GTE_READ:
	andi $a0, 63
	sll $a0, 4 # *4
	la $t0, read_jump_table
	addu $t0, $a0
	jr $t0
	nop

read_jump_table:
	mfc2 $v0, $0; nop; jr $ra; nop
	mfc2 $v0, $1; nop; jr $ra; nop
	mfc2 $v0, $2; nop; jr $ra; nop
	mfc2 $v0, $3; nop; jr $ra; nop
	mfc2 $v0, $4; nop; jr $ra; nop
	mfc2 $v0, $5; nop; jr $ra; nop
	mfc2 $v0, $6; nop; jr $ra; nop
	mfc2 $v0, $7; nop; jr $ra; nop
	mfc2 $v0, $8; nop; jr $ra; nop
	mfc2 $v0, $9; nop; jr $ra; nop
	mfc2 $v0, $10; nop; jr $ra; nop
	mfc2 $v0, $11; nop; jr $ra; nop
	mfc2 $v0, $12; nop; jr $ra; nop
	mfc2 $v0, $13; nop; jr $ra; nop
	mfc2 $v0, $14; nop; jr $ra; nop
	mfc2 $v0, $15; nop; jr $ra; nop
	mfc2 $v0, $16; nop; jr $ra; nop
	mfc2 $v0, $17; nop; jr $ra; nop
	mfc2 $v0, $18; nop; jr $ra; nop
	mfc2 $v0, $19; nop; jr $ra; nop
	mfc2 $v0, $20; nop; jr $ra; nop
	mfc2 $v0, $21; nop; jr $ra; nop
	mfc2 $v0, $22; nop; jr $ra; nop
	mfc2 $v0, $23; nop; jr $ra; nop
	mfc2 $v0, $24; nop; jr $ra; nop
	mfc2 $v0, $25; nop; jr $ra; nop
	mfc2 $v0, $26; nop; jr $ra; nop
	mfc2 $v0, $27; nop; jr $ra; nop
	mfc2 $v0, $28; nop; jr $ra; nop
	mfc2 $v0, $29; nop; jr $ra; nop
	mfc2 $v0, $30; nop; jr $ra; nop
	mfc2 $v0, $31; nop; jr $ra; nop
	cfc2 $v0, $0; nop; jr $ra; nop
	cfc2 $v0, $1; nop; jr $ra; nop
	cfc2 $v0, $2; nop; jr $ra; nop
	cfc2 $v0, $3; nop; jr $ra; nop
	cfc2 $v0, $4; nop; jr $ra; nop
	cfc2 $v0, $5; nop; jr $ra; nop
	cfc2 $v0, $6; nop; jr $ra; nop
	cfc2 $v0, $7; nop; jr $ra; nop
	cfc2 $v0, $8; nop; jr $ra; nop
	cfc2 $v0, $9; nop; jr $ra; nop
	cfc2 $v0, $10; nop; jr $ra; nop
	cfc2 $v0, $11; nop; jr $ra; nop
	cfc2 $v0, $12; nop; jr $ra; nop
	cfc2 $v0, $13; nop; jr $ra; nop
	cfc2 $v0, $14; nop; jr $ra; nop
	cfc2 $v0, $15; nop; jr $ra; nop
	cfc2 $v0, $16; nop; jr $ra; nop
	cfc2 $v0, $17; nop; jr $ra; nop
	cfc2 $v0, $18; nop; jr $ra; nop
	cfc2 $v0, $19; nop; jr $ra; nop
	cfc2 $v0, $20; nop; jr $ra; nop
	cfc2 $v0, $21; nop; jr $ra; nop
	cfc2 $v0, $22; nop; jr $ra; nop
	cfc2 $v0, $23; nop; jr $ra; nop
	cfc2 $v0, $24; nop; jr $ra; nop
	cfc2 $v0, $25; nop; jr $ra; nop
	cfc2 $v0, $26; nop; jr $ra; nop
	cfc2 $v0, $27; nop; jr $ra; nop
	cfc2 $v0, $28; nop; jr $ra; nop
	cfc2 $v0, $29; nop; jr $ra; nop
	cfc2 $v0, $30; nop; jr $ra; nop
	cfc2 $v0, $31; nop; jr $ra; nop

.globl GTE_WRITE
# void GTE_WRITE(uint8_t reg, uint32_t value);
GTE_WRITE:
	andi $a0, 63
	sll $a0, 4
	la $t0, write_jump_table
	addu $t0, $a0
	jr $t0
	nop

write_jump_table:
    mtc2 $a1, $0; nop; jr $ra; nop
    mtc2 $a1, $1; nop; jr $ra; nop
    mtc2 $a1, $2; nop; jr $ra; nop
    mtc2 $a1, $3; nop; jr $ra; nop
    mtc2 $a1, $4; nop; jr $ra; nop
    mtc2 $a1, $5; nop; jr $ra; nop
    mtc2 $a1, $6; nop; jr $ra; nop
    mtc2 $a1, $7; nop; jr $ra; nop
    mtc2 $a1, $8; nop; jr $ra; nop
    mtc2 $a1, $9; nop; jr $ra; nop
    mtc2 $a1, $10; nop; jr $ra; nop
    mtc2 $a1, $11; nop; jr $ra; nop
    mtc2 $a1, $12; nop; jr $ra; nop
    mtc2 $a1, $13; nop; jr $ra; nop
    mtc2 $a1, $14; nop; jr $ra; nop
    mtc2 $a1, $15; nop; jr $ra; nop
    mtc2 $a1, $16; nop; jr $ra; nop
    mtc2 $a1, $17; nop; jr $ra; nop
    mtc2 $a1, $18; nop; jr $ra; nop
    mtc2 $a1, $19; nop; jr $ra; nop
    mtc2 $a1, $20; nop; jr $ra; nop
    mtc2 $a1, $21; nop; jr $ra; nop
    mtc2 $a1, $22; nop; jr $ra; nop
    mtc2 $a1, $23; nop; jr $ra; nop
    mtc2 $a1, $24; nop; jr $ra; nop
    mtc2 $a1, $25; nop; jr $ra; nop
    mtc2 $a1, $26; nop; jr $ra; nop
    mtc2 $a1, $27; nop; jr $ra; nop
    mtc2 $a1, $28; nop; jr $ra; nop
    mtc2 $a1, $29; nop; jr $ra; nop
    mtc2 $a1, $30; nop; jr $ra; nop
    mtc2 $a1, $31; nop; jr $ra; nop
    ctc2 $a1, $0; nop; jr $ra; nop
    ctc2 $a1, $1; nop; jr $ra; nop
    ctc2 $a1, $2; nop; jr $ra; nop
    ctc2 $a1, $3; nop; jr $ra; nop
    ctc2 $a1, $4; nop; jr $ra; nop
    ctc2 $a1, $5; nop; jr $ra; nop
    ctc2 $a1, $6; nop; jr $ra; nop
    ctc2 $a1, $7; nop; jr $ra; nop
    ctc2 $a1, $8; nop; jr $ra; nop
    ctc2 $a1, $9; nop; jr $ra; nop
    ctc2 $a1, $10; nop; jr $ra; nop
    ctc2 $a1, $11; nop; jr $ra; nop
    ctc2 $a1, $12; nop; jr $ra; nop
    ctc2 $a1, $13; nop; jr $ra; nop
    ctc2 $a1, $14; nop; jr $ra; nop
    ctc2 $a1, $15; nop; jr $ra; nop
    ctc2 $a1, $16; nop; jr $ra; nop
    ctc2 $a1, $17; nop; jr $ra; nop
    ctc2 $a1, $18; nop; jr $ra; nop
    ctc2 $a1, $19; nop; jr $ra; nop
    ctc2 $a1, $20; nop; jr $ra; nop
    ctc2 $a1, $21; nop; jr $ra; nop
    ctc2 $a1, $22; nop; jr $ra; nop
    ctc2 $a1, $23; nop; jr $ra; nop
    ctc2 $a1, $24; nop; jr $ra; nop
    ctc2 $a1, $25; nop; jr $ra; nop
    ctc2 $a1, $26; nop; jr $ra; nop
    ctc2 $a1, $27; nop; jr $ra; nop
    ctc2 $a1, $28; nop; jr $ra; nop
    ctc2 $a1, $29; nop; jr $ra; nop
    ctc2 $a1, $30; nop; jr $ra; nop
    ctc2 $a1, $31; nop; jr $ra; nop


.globl _GTE_OP
# void _GTE_OP(uint32_t op)
_GTE_OP:
	addi $sp, -4
	sw $ra, 0($sp)

	li $t0, 0x1FFFFFF
	and $a0, $t0
	
	li $t0, 0x4a000000
	or $a0, $t0

	la $t0, ins_cop2
	sw $a0, 0($t0)
	nop
	nop
	nop

	jal FlushCache
	nop

ins_cop2:
	cop2 0xffffff
	nop
	nop

	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; 

	lw $ra, 0($sp)
	addi $sp, 4
	
	jr $ra
	nop
