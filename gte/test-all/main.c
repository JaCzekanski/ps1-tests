#include <stdio.h>
#include <stdlib.h>
#include <psxapi.h>
#include "common.h"
#include "gte.h"

void runTests();
int main() {
    initVideo(320, 240);
    printf("\ngte/test-all\n");
	printf("Auto test all GTE valid opcodes/registers\n");
	
	GTE_INIT();

	EnterCriticalSection();
	runTests();
	ExitCriticalSection();

	printf("Done.\n");

	while (1) {
		VSync(1);
	}

    return 0;
}