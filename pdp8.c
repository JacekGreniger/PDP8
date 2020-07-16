#include "stdio.h"
#include "conio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "pdp8.h"

pdp8_t pdp8;

uint16 MemoryRead(uint16 addr16)
{
	return pdp8.ram[addr16 & 0x0fff];
}


void MemoryWrite(uint16 addr16, uint16 val16)
{
	pdp8.ram[addr16 & 0x0fff] = val16&0x0fff;
}

/* Reset memory content */
void MemoryReset()
{
	memset(pdp8.ram, 0, 4096);
}

void CPUReset()
{
	pdp8.cpu.AC = 0;
	pdp8.cpu.PC = 0;
	pdp8.cpu.L = 0;
	pdp8.SR = 0;
	pdp8.readerFlag = 0;
	pdp8.teleprinterFlag = 0;
	pdp8.ie = 0;
	pdp8.irq = 0;
	pdp8.readerFlag = 0;
}

void PrintRegisters()
{
#if 0
	char mnemonic[64];
	char par1[64];
	char par2[64];
#endif

	printf("PC=%04ho ", pdp8.cpu.PC);
	printf("AC=%04ho ", pdp8.cpu.AC);
	printf("L=%d\n", pdp8.cpu.L);
#if 0
	if (Disassemble(R[7], mnemonic, par1, par2)>0)
	{
		printf("  %s %s%c %s\n", mnemonic, par1, par2[0] ? ',' : ' ', par2);
	}
#endif
}
