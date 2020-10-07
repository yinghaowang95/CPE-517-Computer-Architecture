/*
 * author: Yinghao Wang
 * date: Oct 13, 2019
 */
#include <stdio.h>
#include "shell.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


typedef struct CPUControl
{
	int regDst;
	int ALUOp0;
	int ALUOp1;
	int ALUsrc;

	int branch;
	int jump;

	int memRead;
	int memWrite;
	
	int memToReg;
	int regWrite;

}Controls;

typedef struct InstructionFields
{
	int opcode;
	int rs;
	int rt;
	int rd;
	int funct;
	int imm16, imm32;
	int address;		// this is the 26 bit field from the J format
}Fields;

typedef struct ALUResult
{
	int result;
	int zero;
} ALUResult;


uint32_t instruction;
Controls control;
Fields field;
ALUResult aluResult;
uint32_t memValue;
extern CPU_State CURRENT_STATE;

uint32_t get_instruction()
{
    // get a instruction
    instruction = mem_read_32(CURRENT_STATE.PC);
    // update PC
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    return instruction;
}

void extract_instruction()
{
	//function takes an instruction as an input, it will read all it's fields
	field.opcode = instruction>>26 & 0x3f;
	field.rs = (instruction>>21) & 0x1f;
	field.rt = (instruction>>16) & 0x1f;

	// R type instrution also need rd, funct, shamt 
	field.rd = (instruction>>11) & 0x1f;
	field.funct = instruction & 0x3f;

	//I-Type  instrution just need imm16/32
	field.imm16 = instruction & 0x0000ffff;
	field.imm32 = field.imm16;
	if(field.imm16 & 0x8000)
		field.imm32 = field.imm16|0xffff0000;

	// J-Type instrution just need address
	//This is used for j instruction and the address is 26bits.
	field.address = instruction & 0x3ffffff;
}
void fill_control()
{
	memset(&control,0, sizeof(control)); // clear control

	if(field.opcode == 0)
	{   //this  is R type
		control.regDst = 1;
		control.regWrite = 1;
		control.ALUOp1 = 1;
	}else
	{	
		switch(field.opcode)
		{
			case 0x02://j
				control.jump = 1;
			break;
			case 0x7: //bgtz
				control.branch = 1;
			break;
			case 0x08: //addi
			case 0x09: //addiu
				control.ALUsrc = 1;
				control.ALUOp0 = 1;
				control.ALUOp1 = 1;
				control.regWrite = 1;
			break;
			case 0x23://lw
			control.ALUsrc = 1;
			control.memRead = 1;
			control.memToReg = 1;
			control.regWrite = 1;
			break;
			case 0x2b://sw
			control.ALUsrc = 1;
			control.memWrite = 1;
			break;
		}
	}
}
void decode_instrcuction()
{
	extract_instruction();
	fill_control();
}

//0	and
//2	add
//6 sub
//7	less than
int get_ControlBit()
{
	int control;
	
	if(field.opcode == 0)
	{   //this  is R type
		switch (field.funct)
        {
        case 0x20: //add
        case 0x21: //addu
            control = 2;
            break;
        case 0x22: //sub
        case 0x23: //subu
            control = 6;
            break;
        case 0x24: //and
            control = 0;
            break;
        }
	}else
	{	
		switch(field.opcode)
		{
			case 0x7: //bgtz
				control = 7;
			break;
			case 0x08: //addi
			case 0x09: //addiu
			case 0x23: //lw
			case 0x2b://sw
				control = 2;
			break;
		}
	}
	return control;
}
//0	and
//2	add
//6 sub
//7	less than	
void execute_ALU()
{
	int control_bit = get_ControlBit();

	int src1 = CURRENT_STATE.REGS[field.rs];
	int src2 = CURRENT_STATE.REGS[field.rt];
	if(control.ALUsrc == 1)
		src2 = field.imm32;
	
	switch(control_bit)
	{
		case 0:  // and
			aluResult.result = src1 & src2;
		break;
		case 2: // add,addi , lw, sw
			aluResult.result = src1 + src2;
		break;
		case 6:  // sub,subu 
			aluResult.result = src1 - src2;
		break;
		case 7: // slt, slti
			if(src1 < src2)
				aluResult.result = 1;
			else
				aluResult.result = 0;
		break;
	}
	if(aluResult.result == 0)
		aluResult.zero = 1;	
}
uint32_t get_nextPC()
{
	uint32_t nextPc = CURRENT_STATE.PC + 4; //ordinary situation

	if(control.branch == 1 && aluResult.zero == 1)// for branch
	{ 
		nextPc += field.imm32*4;
	}else if(control.jump == 1)//here is jump instrction
	{// address*4 means  need to shift left 2 , this address is 28bit
		nextPc = (nextPc & 0xF0000000)|(field.address*4);
	} 
	return nextPc;
}
void execute_MEM()
{
	 if(control.memWrite == 1){  // check for sw
		mem_write_32(aluResult.result, CURRENT_STATE.REGS[field.rt]);
	}else if(control.memRead == 1){  // check for lw
		memValue = mem_read_32(aluResult.result); //read data from memory
	}
}

void execute_updateRegs()
{
	if(control.regWrite == 1)
	{
		int dest = field.rd;
		if(control.regDst == 0) // if execute a r type instruction
		{
			dest = field.rt;
		}
		
		NEXT_STATE.REGS[dest] = aluResult.result; // write back reg with alu result

		if(control.memToReg == 1){
			NEXT_STATE.REGS[dest] = memValue; // write back reg with mem result
		}

	}
}
void process_instruction()
{
	if(get_instruction() == 0){
		RUN_BIT = 0;
		return;
	}
	decode_instrcuction();
	execute_ALU();
	NEXT_STATE.PC = get_nextPC();
	execute_MEM();
	execute_updateRegs();
}
