//Group mumber: Yinghao Wang and Xinqiao Chu
#ifndef _PIPE_H_
#define _PIPE_H_
#include "shell.h"
#include "stdbool.h"
#include <limits.h>

typedef struct Pipe_Reg_IFtoDE {  
	uint32_t instruction; 
	uint32_t PC;          //pc + 4
} Pipe_Reg_IFtoDE;

typedef struct CPUControl
{
	// for EX 
	int regDst;
	int ALUOp0;
	int ALUOp1;
	int ALUsrc;

	// for M
	int jump;
	int branch;
	int memRead;
	int memWrite;

	// for WB
	int memToReg;
	int regWrite;

}Controls;

typedef struct Pipe_Reg_DEtoEX {
	int opcode;
	int funct;
	uint32_t PC;          //pc + 4
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	
	uint32_t rsVal;
	uint32_t rtVal;
	int imm32;
	int address;

	Controls control;

} Pipe_Reg_DEtoEX;

typedef struct Pipe_Reg_EXtoMEM {
	uint32_t rtVal;
	uint32_t rd;
	uint32_t PC;    //calculate new PC for branch or jmp
	// for M
	int jump;
	int branch;
	int memRead;
	int memWrite;

	// for WB
	int memToReg;
	int regWrite;

	int zeroFlag;
	int aluResult;

} Pipe_Reg_EXtoMEM;

typedef struct Pipe_Reg_MEMtoWB {
	// for WB
	int memToReg;
	int regWrite;
	uint32_t aluResult;
	uint32_t memResult;
	uint32_t rd;
} Pipe_Reg_MEMtoWB;

typedef struct CPU_State {
	/* register file state */
	uint32_t REGS[MIPS_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */
	int FLAG_V;        /* flag V */
	int FLAG_C;        /* flag C */

	/* program counter in fetch stage */
	uint32_t PC;
	
} CPU_State;

int RUN_BIT;

/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

/* called during simulator startup */
//void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif
