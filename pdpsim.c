#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "pdpsim.h"
#include "pdp8.h"

/*
http://www4.wittenberg.edu/academics/mathcomp/shelburne/comp255/notes/PDP8Overview.htm
http://homepage.cs.uiowa.edu/~jones/pdp8/man/index.html
http://www.ict.griffith.edu.au/~sdrew/cit1507/m4/m4_t2.htm
*/

int DecodeMRI(uint16 currWord)
{
	uint16 opcode;
	int indirect;
	uint16 addr;
	uint16 val;

	opcode = (currWord >> 9) & 7;
	indirect = !!(currWord & 0400);
	if (currWord & 0200) /* current page */
	{
		addr = (pdp8.cpu.PC & 07600) | (currWord & 0177);
	}
	else /* zero page */
	{
		addr = currWord & 0177;
	}


	if (indirect && ((addr >= 010) && (addr <= 017)))
	{
		val = MemoryRead(addr);
		val += 1;
		val &= 07777;
		MemoryWrite(addr, val);
	}

	pdp8.cpu.PC++;

	switch (opcode)
	{
	case 0: //	AND
		if (indirect)
		{
			pdp8.cpu.AC = pdp8.cpu.AC & MemoryRead(MemoryRead(addr));
		}
		else
		{
			pdp8.cpu.AC = pdp8.cpu.AC & MemoryRead(addr);
		}
		break;

	case 1: //	TAD
		if (indirect)
		{
			pdp8.cpu.AC = pdp8.cpu.AC + MemoryRead(MemoryRead(addr));
		}
		else
		{
			pdp8.cpu.AC = pdp8.cpu.AC + MemoryRead(addr);
		}

		if (pdp8.cpu.AC & 010000)
		{
			pdp8.cpu.L ^= 1; //If carry out then complement Link
		}

		pdp8.cpu.AC &= 07777;
		break;

	case 2: //	ISZ
		if (indirect)
		{
			val =  MemoryRead(MemoryRead(addr));
			val += 1;
			val &= 07777;
			MemoryWrite(MemoryRead(addr), val);
		}
		else
		{
			val = MemoryRead(addr);
			val += 1;
			val &= 07777;
			MemoryWrite(addr, val);
		}

		if (0 == val) //skip next instruction
		{
			pdp8.cpu.PC++;
		}

		break;

	case 3: //	DCA, Deposit Accumulator to memory and Clear accumulator
		if (indirect)
		{
			MemoryWrite(MemoryRead(addr), pdp8.cpu.AC);
		}
		else
		{
			MemoryWrite(addr, pdp8.cpu.AC);
		}

		pdp8.cpu.AC = 0;
		break;

	case 4: //	JMS, JuMP to Subroutine
		if (indirect)
		{
			addr = MemoryRead(addr);
			MemoryWrite(addr, pdp8.cpu.PC);
			addr += 1;
			addr &= 07777;
			pdp8.cpu.PC = addr;
		}
		else
		{
			MemoryWrite(addr, pdp8.cpu.PC);
			addr += 1;
			addr &= 07777;
			pdp8.cpu.PC = addr;
		}
		break;

	case 5: //	JMP
		if (indirect)
		{
			addr = MemoryRead(addr);
			pdp8.cpu.PC = addr;
		}
		else
		{
			pdp8.cpu.PC = addr;
		}
		break;

	default:
		return 0;
	}
	return 1;
}


void ExecuteGroup1(uint16 currWord)
{
	pdp8.cpu.PC++;

	//sequence 1
	if (currWord & 0200) //CLA
	{
		pdp8.cpu.AC = 0;
	}
	if (currWord & 0100) //CLL
	{
		pdp8.cpu.L = 0;
	}

	//sequence 2
	if (currWord & 040) //CMA, complement AC
	{
		pdp8.cpu.AC ^= 07777;
	}
	if (currWord & 020) //CML, complement L
	{
		pdp8.cpu.L ^= 1;
	}

	//sequence 3
	if (currWord & 001) //IAC, increment AC
	{
		pdp8.cpu.AC += 1;
		if (pdp8.cpu.AC & 010000)
		{
			pdp8.cpu.L ^= 1;
		}
		//pdp8.cpu.L = !!(pdp8.cpu.AC & 010000);
		pdp8.cpu.AC &= 07777;
	}

	//sequence 4
	if (currWord & 010) //RAR
	{
		if (currWord & 2) //twice
		{
			pdp8.cpu.AC |= pdp8.cpu.L << 12;
			pdp8.cpu.L = !!(pdp8.cpu.AC & 1);
			pdp8.cpu.AC >>= 1;
			pdp8.cpu.AC &= 07777;

			pdp8.cpu.AC |= pdp8.cpu.L << 12;
			pdp8.cpu.L = !!(pdp8.cpu.AC & 1);
			pdp8.cpu.AC >>= 1;
			pdp8.cpu.AC &= 07777;
		}
		else
		{
			pdp8.cpu.AC |= pdp8.cpu.L << 12;
			pdp8.cpu.L = !!(pdp8.cpu.AC & 1);
			pdp8.cpu.AC >>= 1;
			pdp8.cpu.AC &= 07777;
		}
	}
	if (currWord & 4) //RAL
	{
		if (currWord & 2) //twice
		{
			pdp8.cpu.AC <<= 1;
			pdp8.cpu.AC |= pdp8.cpu.L;
			pdp8.cpu.L = !!(pdp8.cpu.AC & 010000);
			pdp8.cpu.AC &= 07777;

			pdp8.cpu.AC <<= 1;
			pdp8.cpu.AC |= pdp8.cpu.L;
			pdp8.cpu.L = !!(pdp8.cpu.AC & 010000);
			pdp8.cpu.AC &= 07777;
		}
		else
		{
			pdp8.cpu.AC <<= 1;
			pdp8.cpu.AC |= pdp8.cpu.L;
			pdp8.cpu.L = !!(pdp8.cpu.AC & 010000);
			pdp8.cpu.AC &= 07777;
		}
	}

	if ((currWord & 016) == 2) //BSW when bits 8 and 9 are zeros
	{
		pdp8.cpu.AC = ((pdp8.cpu.AC & 07700) >> 6) | ((pdp8.cpu.AC & 00077) << 6);
	}

	if (!(currWord & 00777))
	{
		//printf("NOP");
	}

	//printf("\n");
}

SIM_ERROR ExecuteGroup2(uint16 currWord)
{
	int skip = 0;
	SIM_ERROR err = SIM_OK;

	//after fetch move PC to next instruction
	pdp8.cpu.PC++;

	//sequence 1 (OR group)
	if (!(currWord & 010))
	{
		if (currWord & 0100) //SMA, skip on minus AC
		{
			if (pdp8.cpu.AC & 04000)
			{
				skip = 1;
			}
		}
		if (currWord & 0040) //SZA, skip on zero AC
		{
			if (0 == pdp8.cpu.AC)
			{
				skip = 1;
			}
		}
		if (currWord & 0020) //SNL, skip on nonzero L
		{
			if (pdp8.cpu.L)
			{
				skip = 1;
			}
		}
	}
	else //AND group
	{
		int cond = 0;

		if (currWord & 0100) //SPA, skip on plus AC
		{
			cond = 4;
			if (!(pdp8.cpu.AC & 04000))
			{
				skip = 4;
			}

		}
		if (currWord & 0040) //SNA, skip on nonzero AC
		{
			cond |= 2;
			if (pdp8.cpu.AC)
			{
				skip |= 2;
			}
		}
		if (currWord & 0020) //SZL, skip on zero L
		{
			cond |= 1;
			if (!pdp8.cpu.L)
			{
				skip |= 1;
			}
		}
		skip = !!(skip == cond);

		if (!(currWord & 0160)) //SKP skip always
		{
			skip = 1;
		}
	}

	if (skip)
	{
		pdp8.cpu.PC++;
	}

	//sequence 2
	if (currWord & 0200) //CLA
	{
		pdp8.cpu.AC = 0;
	}

	//sequence 3
	if (currWord & 4) //OSR
	{
		pdp8.cpu.AC |= pdp8.SR;
	}
	if (currWord & 2) //HLT
	{
		err = SIM_HALT;
	}

	return err;
}

SIM_ERROR IOT(uint16 currWord)
{
	pdp8.cpu.PC++;

	switch (currWord & 07777)
	{
	//teleprinter
	case 06041: //TSF, skip if teleprinter flag set
		if (pdp8.teleprinterFlag)
		{
			pdp8.cpu.PC++;
		}
		break;
	case 06042: //TCF, clear teleprinter flag
		pdp8.teleprinterFlag = 0;
		break;
	case 06044: //TPC, load printer buffer and print
		if ((pdp8.cpu.AC & 0177) > 0)
		{
			printf("%c", pdp8.cpu.AC & 0177);
		}
		pdp8.teleprinterFlag = 1;
		break;
	case 06046: //TLS, load printer buffer and print, clear flag and set it again
		if ((pdp8.cpu.AC & 0177) > 0)
		{
			printf("%c", pdp8.cpu.AC & 0177);
		}
		//TODO: teleprinter flag should be cleared and set again after some time to simulate real real during printing char
		pdp8.teleprinterFlag = 1;
		break;
	case 06001: //ION
		pdp8.ie = 2;
		break;
	case 06002: //IOF
		pdp8.ie = 0;
		break;

		/* Keyboard reader */
	case 06031: //KSF, skip if reader flag set
		if (pdp8.readerFlag)
		{
			pdp8.cpu.PC++;
		}
		break;
	case 06032: //KCC
		pdp8.cpu.AC = 0;
		pdp8.readerFlag = 0;
		break;
	case 06034: //KRS
		pdp8.cpu.AC = pdp8.keycode;
		break;
	case 06036: //KRB
		pdp8.cpu.AC = pdp8.keycode;
		pdp8.readerFlag = 0;
		break;
	default:
		return SIM_ILLEGAL_INSTRUCTION;
	}

	return SIM_OK;
}

SIM_ERROR PDP_Simulate()
{
	uint16 currWord;
	SIM_ERROR err;

	/* Fetch instruction */
	currWord = MemoryRead(pdp8.cpu.PC);

	if (((currWord & 07000) >= 0) && ((currWord & 07000) <= 05000))
	{
		(void)DecodeMRI(currWord);
		return SIM_OK;
	}
	else if ((currWord & 07400) == 07000)
	{
		ExecuteGroup1(currWord);
		return SIM_OK;
	}
	else if ((currWord & 07400) == 07400)
	{
		err = ExecuteGroup2(currWord);
		return err;
	}
	else if ((currWord & 07000) == 06000)
	{
		err = IOT(currWord);
		return err;
	}

	return SIM_ILLEGAL_INSTRUCTION;
}
