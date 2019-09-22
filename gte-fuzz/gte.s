#include <asm.h>
#include <inline_s.h>
#include <gtereg_s.h>

.text

FlushCache:
	li $t1, 0x44
	li $t2, 0xa0
	jr $t2


.globl GTE_READ
# uint32_t GTE_READ(uint8_t reg);
GTE_READ:
	addi $sp, -4
	sw $ra, 0($sp)


	# reg < 32 - data
	# reg >= 32 - control

	sub $t0, $a0, 32
	bgez $t0, read_control
	nop

    /*
	00 f8 02 48
	   ^^
	   gte reg number
	
	1. andi reg with 31
	2. << 3
	3. load ins addr
	4. store byte ins + 1
	*/

# ###############################
read_data:
	and $a0, 31
	sll $a0, 3

	la $t0, ins_mfc
	sb $a0, 1($t0)
	nop
	nop
	nop

	jal FlushCache
	nop

ins_mfc:
	mfc2 $v0, $31 # to r2 (v0) read xx <-- replaced by us
	nop
	nop
	j exitGTE_READ
	nop


# #############################
read_control:
	sub $a0, 32
	and $a0, 31
	sll $a0, 3

	la $t0, ins_cfc
	sb $a0, 1($t0)
	nop
	nop
	nop
	
	jal FlushCache
	nop

ins_cfc:
	cfc2 $v0, $31
	nop
	nop
	j exitGTE_READ
	nop



exitGTE_READ:
	lw $ra, 0($sp)
	addi $sp, 4
	
	jr $ra
	nop

	



.globl GTE_WRITE
# void GTE_WRITE(uint8_t reg, uint32_t value);
GTE_WRITE:
	addi $sp, -4
	sw $ra, 0($sp)

	sub $t0, $a0, 32
	bgez $t0, write_control
	nop

# /////////////////////////////////
write_data:
	and $a0, 31
	sll $a0, 3

	la $t0, ins_mtc
	sb $a0, 1($t0)
	nop
	nop
	nop

	jal FlushCache
	nop

ins_mtc:
	mtc2 $a1, $31
	nop
	nop
	j exitGTE_WRITE
	nop


# ///////////////////////////////
write_control:
	sub $a0, 32
	and $a0, 31
	sll $a0, 3

	la $t0, ins_ctc
	sb $a0, 1($t0)
	nop
	nop
	nop
	
	jal FlushCache
	nop

ins_ctc:
	ctc2 $a1, $31
	nop
	nop
	j exitGTE_WRITE
	nop



exitGTE_WRITE:
	lw $ra, 0($sp)
	addi $sp, 4
	
	jr $ra
	nop


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
