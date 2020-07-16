#ifndef PDPSIM_H
#define PDPSIM_H

typedef enum
{
	SIM_OK,
	SIM_HALT,
	SIM_ILLEGAL_INSTRUCTION
} SIM_ERROR;

SIM_ERROR PDP_Simulate();

#endif