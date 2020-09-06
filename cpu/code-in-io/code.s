#include <asm.h>
#include <inline_s.h>

.extern FlushCache
FlushCache:
	li $t1, 0x44
	li $t2, 0xa0
	jr $t2
	nop

.globl testCode
testCode:
	jr $ra
	nop
