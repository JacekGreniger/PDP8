#ifndef PDP8_H
#define PDP8_H

typedef struct
{
	unsigned short PC;
	unsigned short AC;
	int L;
} cpu_t;

typedef struct
{
	unsigned short ram[4096];
	cpu_t cpu;
	uint16 SR;
	int readerFlag;
	int keycode;
	int teleprinterFlag;
	int ie; //interrupt enable
	int irq; //interrupt request
} pdp8_t;

extern pdp8_t pdp8;
uint16 MemoryRead(uint16 addr16);
void MemoryWrite(uint16 addr16, uint16 val16);
void MemoryReset();
void CPUReset();
void PrintRegisters();

typedef struct
{
	FILE * f;
	char filename[255];
	int status;
	int bytes_read;
	int eof;
	int read_request;
} tape_t;

extern tape_t tape;

#define IRQ_DELAY 1000
#endif