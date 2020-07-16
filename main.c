#include "stdio.h"
#include "string.h"
#include "conio.h"
#include <process.h>
#include <direct.h>
#include "ctype.h"
#include "defs.h"
#include "iodev.h"
#include "pdpsim.h"
#include "disasm.h"
#include "config.h"
#include "pdp8.h"

#if defined(_WIN32)
#include "windows.h"
#endif

int debug_break = 0;

tape_t tape;
FILE * logfile = NULL;

#define BREAKPOINT_MAX 16
uint16 breakpoints_arr[BREAKPOINT_MAX];
int breakpoints_num = 0;

#define TRACE_BUFFER_LEN 5000

cpu_t traceBuffer[TRACE_BUFFER_LEN];
int trace_count = 0;

void TraceBuffer_Add()
{
	memmove(&traceBuffer[0], &traceBuffer[1], sizeof(cpu_t)*(TRACE_BUFFER_LEN - 1));	
	memcpy(&traceBuffer[TRACE_BUFFER_LEN - 1], &pdp8.cpu, sizeof(cpu_t));
	if (trace_count < TRACE_BUFFER_LEN)
	{
		trace_count++;
	}
}

void TraceBuffer_Reset()
{
	memset(traceBuffer, 0, sizeof(cpu_t)*TRACE_BUFFER_LEN);
}

int keyboard_ctrl_d = 0;

void Keyboard()
{
	if (_kbhit())
	{
		int key = _getch();
		if (key == 4) //Ctrl-D
		{
			keyboard_ctrl_d = 1;
		}
		else
		{
			pdp8.readerFlag = 1;
			pdp8.keycode = key;
		}
	}
	else if (pdp8.ie == 1) //interrupt enabled
	{
		if (0 == pdp8.irq) //interrupt not yet requested
		{
			if (pdp8.readerFlag || pdp8.teleprinterFlag)
			{
				pdp8.irq = IRQ_DELAY;
			}
		}
	}
}


void ParseCommand_Trace(char * cmdTok_p)
{
	int i;
	int num = TRACE_BUFFER_LEN;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p != NULL)
	{
		sscanf(cmdTok_p, "%d", &num);
		if (num >= TRACE_BUFFER_LEN)
		{
			num = TRACE_BUFFER_LEN;
		}
	}

	num = TRACE_BUFFER_LEN - num;

	if (trace_count < TRACE_BUFFER_LEN)
	{
		num = TRACE_BUFFER_LEN - trace_count;
	}

	printf("NOTE: registers state BEFORE instruction execution\n");
	for (i = num; i < TRACE_BUFFER_LEN; i++)
	{
		printf("\n(%d) ", i - TRACE_BUFFER_LEN + 1);
		printf("PC=%04ho ", traceBuffer[i].PC);
		printf("AC=%04ho ", traceBuffer[i].AC);
		printf("L=%d  ", traceBuffer[i].L);
		Disassemble(traceBuffer[i].PC);
	}
}

void ParseCommand_Tape(char * cmdTok_p)
{
	char * filename;
	FILE * f;
	filename = strtok(NULL, " =,");

	if (filename == NULL)
	{
		if (tape.status)
		{
			printf("Tape: %s, bytes read=%d %s\n", tape.filename, tape.bytes_read, tape.eof?"(end of file)":"");
		}
		else
		{
			printf("Tape not opened yet\n");
		}
	}
	else if (strcmp(filename, "CLOSE")==0)
	{
		if (tape.status)
		{
			fclose(tape.f);
			tape.status = 0;
			tape.filename[0];
		}
		else
		{
			printf("Cannot close tape\n");
		}
	}
	else
	{
		if (tape.status)
		{
			printf("Tape %s closed\n", tape.filename);
			fclose(tape.f);
		}
		f = fopen(filename, "rb");
		if (f != NULL)
		{
			strcpy(tape.filename, filename);
			tape.status = 1;
			tape.f = f;
			tape.bytes_read = 0;
			tape.eof = 0;
			printf("Tape %s opened\n", tape.filename);
		}
		else
		{
			printf("Error opening file: %s\n", filename);
		}
	}
}

void ParseCommand_Examine(char * cmdTok_p)
{
	uint16 addr16;
	uint16 size = 1;
	int i;
	int res;
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("e=addr12 [N] where N is number of words to read\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &addr16);
	if (res != 1)
	{
		printf("error in address, must be in octal format\n");
		return;
	}
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p != NULL)
	{
		if (strncmp(cmdTok_p, "0", 1) == 0)
		{
			cmdTok_p++;
			res = sscanf(cmdTok_p, "%ho", &size);

		}
		else
		{
			res = sscanf(cmdTok_p, "%hd", &size);
		}
		if (res != 1)
		{
			printf("error in size , must be in octal or decimal format\n");
			return;
		}
	}
	for (i = 0; i < size; i++)
	{
		printf("%04ho: %04ho\n", addr16, MemoryRead(addr16));
		addr16++;
	}
}

void ParseCommand_ExamineWithDisassembly(char * cmdTok_p)
{
	uint16 addr16;
	uint16 size = 1;
	int i;
	int res;
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("mr=addr16 [N] where N is number of words to read\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &addr16);
	if (res != 1)
	{
		printf("error in address, must be in octal format\n");
		return;
	}
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p != NULL)
	{
		if (strncmp(cmdTok_p, "0", 1) == 0)
		{
			cmdTok_p++;
			res = sscanf(cmdTok_p, "%ho", &size);

		}
		else
		{
			res = sscanf(cmdTok_p, "%hd", &size);
		}
		if (res != 1)
		{
			printf("error in size , must be in octal or decimal format\n");
			return;
		}
	}
	for (i = 0; i < size; i++)
	{
		printf("%04ho: %04ho ", addr16, MemoryRead(addr16));
		Disassemble(addr16);
		addr16 += 1;
	}
}


void ParseCommand_Deposit(char * cmdTok_p)
{
	uint16 addr16;
	uint16 value16;
	int res;

	/* Get address from commands string */
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("Syntax error\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &addr16);
	if (res != 1)
	{
		printf("error in address, must be in octal format\n");
		return;
	}

	/* Get value from commands string */
	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("Syntax error\n");
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &value16);
	if (res != 1)
	{
		printf("Error in value, must be in octal format\n");
		return;
	}

	MemoryWrite(addr16, value16);
}

void ParseCommand_RegWrite(char * cmdTok_p)
{
	uint16 reg = 0;
	uint16 val16;
	int res;

	if (strcmp(cmdTok_p, "PC") == 0)
	{
		reg = 1;
	}
	else if (strcmp(cmdTok_p, "AC") == 0)
	{
		reg = 2;
	}
	else if (strcmp(cmdTok_p, "L") == 0)
	{
		reg = 3;
	}

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		switch (reg)
		{
		case 1:
			printf("PC=%04ho\n", pdp8.cpu.PC);
			break;
		case 2:
			printf("AC=%04ho\n", pdp8.cpu.AC);
			break;
		case 3:
			printf("L=%d\n", pdp8.cpu.L);
			break;
		default:
			break;
		}
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &val16);
	if (res != 1)
	{
		printf("error in value, must be in octal format\n");
		return;
	}
	val16 &= 07777;

	switch (reg)
	{
	case 1:
		pdp8.cpu.PC = val16;
		break;
	case 2:
		pdp8.cpu.AC = val16;
		break;
	case 3:
		val16 &= 1;
		pdp8.cpu.L = val16;
		break;
	default:
		break;
	}
}

void ParseCommand_SRReg(char * cmdTok_p)
{
	uint16 reg = 0;
	uint16 val16;
	int res;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("SR=%04ho\n", pdp8.SR);
		return;
	}
	res = sscanf(cmdTok_p, "%ho", &val16);
	if (res != 1)
	{
		printf("error in value, must be in octal format\n");
		return;
	}

	pdp8.SR = val16 & 07777;
}

int ParseCommand_OpenScript(char * cmdTok_p, FILE ** inifile)
{
	/* Open script file */
	if (*inifile != NULL)
	{
		printf("Script file already opened");
		return 0;
	}
	cmdTok_p = strtok(NULL, " =,");
	*inifile = fopen(cmdTok_p, "r");
	if (*inifile == NULL)
	{
		printf("Cannot open script file: %s\n", cmdTok_p);
		return 0;
	}

	return 1;
}

void ParseCommand_LoadBinary(char * cmdTok_p)
{
	FILE * f;
	char filename[255];
	uint16 load_address;
	int c;
	int bytes_read = 0;
	uint16 load_address_save;
	uint16 new_pc;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("load filename load_address [newPC]\n");
		return;
	}
	strcpy(filename, cmdTok_p);

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("load filename load_address [newPC]\n");
		return;
	}
	if (1 != sscanf(cmdTok_p, "%ho", &load_address))
	{
		printf("load filename load_address [newPC]\n");
		return;
	}
	load_address_save = load_address;

	f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("Cannot open %s\n", filename);
		return;
	}

	while ((c = fgetc(f)) >= 0)
	{
		bytes_read++;
		//FIX IT
		MemoryWrite(load_address++, c);
	}
	fclose(f);

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p)
	{
		if (1 == sscanf(cmdTok_p, "%ho", &new_pc))
		{
			pdp8.cpu.PC = new_pc;
		}
		else
		{
			printf("Syntax error: PC not set\n");
		}
	}

	printf("File %s: loaded %d bytes into 0%ho\n", filename, bytes_read, load_address_save);
}

void ParseCommand_BinaryTapeLoader(char * cmdTok_p)
{
	FILE * f;
	char filename[255];
	uint16 load_address;
	int c;
	int bytes_read = 0;
	int write_to_memory = 0;
	uint16 data;
	FILE *dump;

	cmdTok_p = strtok(NULL, " =,");
	if (cmdTok_p == NULL)
	{
		printf("BTL filename\n");
		return;
	}
	strcpy(filename, cmdTok_p);

	f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("Cannot open %s\n", filename);
		return;
	}


	dump = fopen("dump.do", "wb");
	if (dump == NULL)
	{
		printf("Cannot create dump.do\n");
		return;
	}

	while ((c = fgetc(f)) >= 0)
	{
		if (c == 0200)
		{
			//leader
			if (write_to_memory)
			{
				printf("done\n");
				break;
			}
		}
		else if (0100 == (c & 0100))
		{
			load_address = (c & 077) << 6;
			c = fgetc(f);
			load_address |= (c & 077);
			printf("Address = %ho\n", load_address);
			write_to_memory = 1;
		}
		else if (write_to_memory)
		{
			data = (c & 077) << 6;
			c = fgetc(f);
			data |= (c & 077);
			fprintf(dump, "deposit %ho %ho\n", load_address, data);
			MemoryWrite(load_address++, data);
		}
	}
	fclose(dump);
	fclose(f);
}

void ParseCommand_Breakpoint(char * cmdTok_p)
{
	int i;
	uint16 address;
	int breakpoint_found = 0;

	cmdTok_p = strtok(NULL, " =,");

	if (cmdTok_p == NULL)
	{
		for (i = 0; i < breakpoints_num; i++)
		{
			printf("breakpoint %d: %04ho\n", i, breakpoints_arr[i]);
		}
		return;
	}

	if (1 != sscanf(cmdTok_p, "%ho", &address))
	{
		printf("wrong breakpoint address\n");
		return;
	}

	for (i = 0; i < breakpoints_num; i++)
	{
		if (breakpoints_arr[i] == address)
		{
			//remove breakpoint
			--breakpoints_num;
			for (; i < breakpoints_num; i++)
			{
				breakpoints_arr[i] = breakpoints_arr[i + 1];
			}
			printf("breakpoint removed\n");
			return;
		}
	}
	if (breakpoints_num + 1 < BREAKPOINT_MAX)
	{
		breakpoints_arr[breakpoints_num++] = address;
		printf("breakpoint set\n");
	}
}


void strtoupper(char * st)
{
	while (*st)
	{
		*st = toupper(*st);
		st++;
	}
}

void main(int argc, char ** argv)
{
	char cmdStr[255];
	char *cmdTok_p;
#if 0
	char mnemonic[64];
	char par1[64];
	char par2[64];
#endif
	int i = 0;
	char * c;
	FILE * inifile = NULL;

	printf("PDP8 sim %s %s\n\n", __DATE__, __TIME__);

	if (argc > 1)
	{
		inifile = fopen(argv[1], "r");
		if (inifile == NULL)
		{
			printf("Cannot open %s\n", argv[1]);
		}
	}
	else
	{
		inifile = fopen("pdp8.ini", "r");
	}

	tape.status = 0;
	tape.filename[0];
	tape.eof = 0;
	tape.read_request = 0;
	MemoryReset();
	CPUReset();
	TraceBuffer_Reset();

	while (1)
	{
		printf("pdp8> ");
		
		if (inifile == NULL)
		{
			gets(cmdStr);
		}
		else
		{
			//get command from file
			if (NULL == fgets(cmdStr, 255, inifile))
			{
				// end of ini file
				fclose(inifile);
				inifile = NULL;
				continue;
			}
			//fgets copies newline to the the buffer
			if (strrchr(cmdStr, 13))
			{
				*strrchr(cmdStr, 13) = 0;
			}
			if (strrchr(cmdStr, 10))
			{
				*strrchr(cmdStr, 10) = 0;
			}
			printf("%s\n", cmdStr);
		}

		strtoupper(cmdStr);
		if (c = strchr(cmdStr, ';'))
		{
			*c = 0;
		}
			
		cmdTok_p = strtok(cmdStr, " =,");
		if (cmdTok_p == NULL)
		{
			continue;
		}

		/* Quit */
		if (strncmp(cmdTok_p, "Q", 1) == 0)
		{
			if (tape.status)
			{
				fclose(tape.f);
			}
			return;
		}
		/* Breakpoint set/print/remove */
		else if (strncmp(cmdTok_p, "BR", 2) == 0)
		{
			ParseCommand_Breakpoint(cmdTok_p);
		}
		else if (strcmp(cmdTok_p, "DIR") == 0)
		{
			system("DIR");
		}
		else if (strcmp(cmdTok_p, "CD") == 0)
		{
			cmdTok_p = strtok(NULL, "\n");
			if (cmdTok_p == NULL)
			{
				continue;
			}
			_chdir(cmdTok_p);
		}
		/* Reset everything */
		else if (strcmp(cmdTok_p, "RESET") == 0)
		{
			MemoryReset();
			CPUReset();
			TraceBuffer_Reset();
			if (tape.status)
			{
				fclose(tape.f);
				tape.status = 0;
				tape.filename[0];
			}
			breakpoints_num = 0;
		}
		/* Examine */
		else if (strncmp(cmdTok_p, "E", 1) == 0)
		{
			ParseCommand_Examine(cmdTok_p);
		}
		/* Examine */
		else if (strncmp(cmdTok_p, "DIS", 3) == 0)
		{
			ParseCommand_ExamineWithDisassembly(cmdTok_p);
		}
		/* Execute script file */
		else if ((strcmp(cmdTok_p, "DO") == 0) && (inifile == NULL))
		{
			int res;
			res = ParseCommand_OpenScript(cmdTok_p, &inifile);
		}
		/* Deposit */
		else if (strncmp(cmdTok_p, "D", 1) == 0)
		{
			ParseCommand_Deposit(cmdTok_p);
		}
		/* Print trace buffer */
		else if (strcmp(cmdTok_p, "TRACE") == 0)
		{
			ParseCommand_Trace(cmdTok_p);
		}
		/* Tape open/info/close */
		else if (strncmp(cmdTok_p, "T", 1) == 0)
		{
			ParseCommand_Tape(cmdTok_p);
		}
		/* Load binary file to memory */
		else if (strncmp(cmdTok_p, "LOAD", strlen(cmdTok_p)) == 0)
		{
			ParseCommand_LoadBinary(cmdTok_p);
		}
		/* Binary tape loader to memory */
		else if (strcmp(cmdTok_p, "BTL") == 0)
		{
			ParseCommand_BinaryTapeLoader(cmdTok_p);
		}
		/* Print all register */
		else if (strcmp(cmdTok_p, "R") == 0)
		{
			PrintRegisters();
		}
		/* Print/set Status Register */
		else if (strcmp(cmdTok_p, "SR") == 0)
		{
			ParseCommand_SRReg(cmdTok_p);
		}
		/* Print/set CPU registers */
		else if (strcmp(cmdTok_p, "AC") == 0 ||
			strcmp(cmdTok_p, "PC") == 0 ||
			strcmp(cmdTok_p, "L") == 0)
		{
			ParseCommand_RegWrite(cmdTok_p);
		}
		/* Print help */
		else if (strncmp(cmdTok_p, "H", 1) == 0)
		{
			printf("PDP8 sim %s %s\n\n", __DATE__, __TIME__);
			printf(" BReakpoint [ADDR] - breakpoint list/set\n");
			printf(" RESET - clear memory/registers, close tape files\n");
			printf(" TRACE [num]- program execution trace\n");
			printf(" Examine ADDR [N] - examine memory [N-words]\n");
			printf(" DISassembly ADDR [N] - examine and disassembly memory [N-instructions]\n");
			printf(" Deposit ADDR VALUE - deposit value in memory\n");
			printf(" PC/AC/L[=NEW_OCTAL_VALUE] - read/write CPU register\n");
			printf(" SR [=NEW_OCTAL_VALUE] - read/write SR register\n");
			printf(" Tape [filename/CLOSE] - status/open/close tape image file\n");
			printf(" Step [number of steps] - step CPU simulator\n");
			printf(" Go [new PC] - run CPU simulator\n");
			printf(" Load filename load_address [new PC] - load binary file into address\n");
			printf(" BTL filename - binary tape loader\n");
			printf(" DO filename - execute script\n");
			printf(" DIR - list content of current directory\n");
			printf(" CD directory - change current directory\n");
			printf(" Quit\n");
			printf("\n");
		}
		/* CPU step */
		else if (strncmp(cmdTok_p, "STEP", strlen(cmdTok_p)) == 0)
		{
			SIM_ERROR simError;
			int steps = 1;
			uint16 addr16;

			cmdTok_p = strtok(NULL, " =,");
			if ((cmdTok_p == NULL) || (sscanf(cmdTok_p, "%d", &steps) != 1))
			{
				steps = 1; //error converting string to int
			}

			while (steps--)
			{
				addr16 = pdp8.cpu.PC;

				for (i = 0; i < breakpoints_num; i++)
				{
					if (breakpoints_arr[i] == pdp8.cpu.PC)
					{
						printf("breakpoint hit at %04ho\n", pdp8.cpu.PC);
					}
				}

				TraceBuffer_Add();
				printf("%04ho: %04ho ", addr16, MemoryRead(addr16));
				Disassemble(addr16);
				simError = PDP_Simulate();
				PrintRegisters();

				if (simError == SIM_HALT)
				{
					printf("HALT instruction\n");
					break;
				}
			}
		}
		/* CPU run */
		else if (strncmp(cmdTok_p, "G", 1) == 0)
		{
			SIM_ERROR simError;
			uint16 addr16;
			int breakpoint_hit = 0;

			cmdTok_p = strtok(NULL, " =,");
			if (cmdTok_p)
			{
				if (1 == sscanf(cmdTok_p, "%ho", &addr16))
				{
					pdp8.cpu.PC = addr16;
				}
				else
				{
					printf("Syntax error: go [new PC]\n");
				}
			}

			printf("Running CPU, PC=%ho, Ctrl-A to simulate Ctrl-C, Ctrl-D to break\n", pdp8.cpu.PC);
			while (!breakpoint_hit)
			{	
				if (pdp8.ie > 1)
				{
					--pdp8.ie;
				}
				else if (pdp8.ie == 1)
				{
					if (pdp8.irq > 1)
					{
						--pdp8.irq;
					}
					else if (pdp8.irq == 1)
					{
						//raise interrupt
						pdp8.ie = 0;
						pdp8.irq = 0;
						MemoryWrite(0, pdp8.cpu.PC); //save return address from interrupt
						pdp8.cpu.PC = 1;

						//check for breakpoint
						for (i = 0; i < breakpoints_num; i++)
						{
							if (breakpoints_arr[i] == pdp8.cpu.PC)
							{
								printf("breakpoint hit at %06ho\n", pdp8.cpu.PC);
								breakpoint_hit = 1;
								continue;
							}
						}
					}
				}

				if (debug_break)
				{
					printf("DEBUG_BREAK\n");
					debug_break = 0;
					break;
				}

				addr16 = pdp8.cpu.PC;

				Keyboard();
				if (keyboard_ctrl_d)
				{
					printf("\nProgram execution stopped by user\n");
					keyboard_ctrl_d = 0;
					break;
				}

				TraceBuffer_Add();
				simError = PDP_Simulate();

				for (i = 0; i < breakpoints_num; i++)
				{
					if (breakpoints_arr[i] == pdp8.cpu.PC)
					{
						printf("breakpoint hit at %06ho\n", pdp8.cpu.PC	);
						breakpoint_hit = 1;
					}
				}

				if (simError == SIM_HALT)
				{
					printf("HALT instruction\n");
					break;
				}
				else if (simError == SIM_ILLEGAL_INSTRUCTION)
				{
#ifdef CPU_ILLEGAL_INSTRUCTION_INFO
					//printf("ERROR: illegal instruction at %06ho: %06ho\n", addr16, MemoryRead(addr16));
#endif //CPU_ILLEGAL_INSTRUCTION_INFO
					continue;
				}
			}
			PrintRegisters();
		}
		else
		{
			printf("???\n");
		}

	}
}