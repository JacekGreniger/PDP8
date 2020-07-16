#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "defs.h"
#include "pdp8.h"

int Dism_MRI(uint16 pc)
{
	uint16 opcode;
	int indirect;
	uint16 addr;
	uint16 currWord = MemoryRead(pc);

	opcode = (currWord >> 9) & 7;
	indirect = !!(currWord & 0400);
	if (currWord & 0200) /* current page */
	{
		addr = (pc & 07600) | (currWord & 0177);
	}
	else
	{
		addr = currWord & 0177;
	}


	switch (opcode)
	{
	case 0: //	AND
		printf("AND %c%ho\n", indirect ? '*' : ' ', addr);
		break;

	case 1: //	TAD
		printf("TAD %c%ho\n", indirect ? '*' : ' ', addr);
		break;

	case 2: //	ISZ
		printf("ISZ %c%ho\n", indirect ? '*' : ' ', addr);
		break;

	case 3: //	DCA
		printf("DCA %c%ho\n", indirect ? '*' : ' ', addr);
		break;

	case 4: //	JMS
		printf("JMS %c%ho\n", indirect ? '*' : ' ', addr);
		break;

	case 5: //	JMP
		printf("JMP %c%ho\n", indirect ? '*' : ' ', addr);
		break;

	default:
		return 0;
	}
	return 1;
}


void Dism_Group1(uint16 currWord)
{
	//sequence 1
	if (currWord & 0200) //CLA
	{
		printf("CLA ");
	}
	if (currWord & 0100) //CLL
	{
		printf("CLL ");
	}

	//sequence 2
	if (currWord & 040) //CMA, complement AC
	{
		printf("CMA ");
	}
	if (currWord & 020) //CML, complement L
	{
		printf("CML ");
	}

	//sequence 3
	if (currWord & 001) //IAC, increment AC
	{
		printf("IAC ");
	}

	//sequence 4
	if (currWord & 010) //RAR
	{
		if (currWord & 02) //twice
		{
			printf("RTR ");
		}
		else
		{
			printf("RAR ");
		}
	}
	if (currWord & 4) //RAL
	{
		if (currWord & 2) //twice
		{
			printf("RTL ");
		}
		else
		{
			printf("RAL ");
		}
	}

	if ((currWord & 016) == 2) //BSW when bits 8 and 9 are zeros
	{
		printf("BSW");
	}

	if (!(currWord & 00777))
	{
		printf("NOP");
	}

	printf("\n");
}

void Dism_Group2(uint16 currWord)
{
	//sequence 1 (OR group)
	if (!(currWord & 010))
	{
		if (currWord & 0100)
		{
			printf("SMA ");
		}
		if (currWord & 0040)
		{
			printf("SZA ");
		}
		if (currWord & 0020) //SNL, skip on nonzero L
		{
			printf("SNL ");
		}
	}
	else //AND group
	{
		if (currWord & 0100) //SPA, skip on plus AC
		{
			printf("SPA ");

		}
		if (currWord & 0040) //SNA, skip on nonzero AC
		{
			printf("SNA ");
		}
		if (currWord & 0020) //SZL, skip on zero L
		{
			printf("SZL ");
		}
		if (!(currWord & 0160)) //SKP skip always
		{
			printf("SKP ");
		}
	}

	//sequence 2
	if (currWord & 0200) //CLA
	{
		printf("CLA ");
	}

	//sequence 3
	if (currWord & 4) //OSR
	{
		printf("OSR ");
	}
	if (currWord & 2) //HLT
	{
		printf("HLT ");
	}

	printf("\n");
}

void Dism_IOT(uint16 currWord)
{
	switch (currWord & 07777)
	{
	//teleprinter
	case 06040: //set teleprinter flag
		printf("SPF\n");
		break;
	case 06041: //skip if teleprinter flag set
		printf("TSF\n");
		break;
	case 06042: //clear teleprinter flag
		printf("TCF\n");
		break;
	case 06044: //load printer buffer and print
		printf("TPC\n");
		break;
	case 06046: //load printer buffer and print, clear flag
		printf("TLS\n");
		break;
	case 06001: //interrupt enable
		printf("ION\n");
		break;
	case 06002: //interrupt disable
		printf("IOF\n");
		break;

	default:
		printf("IOT %o\n", currWord & 0777);
		break;
	}
}

void Disassemble(uint16 pc)
{
	uint16 currWord;

	/* Fetch instruction */
	currWord = MemoryRead(pc);

	if (((currWord & 07000) >= 0) && ((currWord & 07000) <= 05000))
	{
		Dism_MRI(pc);
	}
	else if ((currWord & 07400) == 07000)
	{
		Dism_Group1(currWord);
	}
	else if ((currWord & 07400) == 07400)
	{
		Dism_Group2(currWord);
	}
	else if ((currWord & 07000) == 06000)
	{
		Dism_IOT(currWord);
	}
}

