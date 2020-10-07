//Group mumber: Yinghao Wang & Xinqiao Chu
//Our code base on our Lab3 coding.
#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct InstructionFields
{
	int opcode;
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	int funct;
	int imm16, imm32;
	int address;		// this is the 26 bit field from the J format
}Fields;

// some globe value
Pipe_Reg_IFtoDE Reg_IFtoDE;
Pipe_Reg_DEtoEX Reg_DEtoEX;
Pipe_Reg_EXtoMEM Reg_EXtoMEM;
Pipe_Reg_MEMtoWB Reg_MEMtoWB;
Pipe_Reg_MEMtoWB Reg_MEMtoWB_bak; // for forwarding
uint32_t instruction;
Fields field;
uint32_t memValue;
int stageCount = 0;
int stallFlag = 0;
// forwarding type
int forwardA = 0;
int forwardB = 0;

void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
	pipe_stage_fetch();
}


// detect forwarding
void detect_forwarding()
{
    forwardA = 0;
    forwardB = 0;
    //check EX hazard
    if(Reg_EXtoMEM.regWrite && Reg_EXtoMEM.rd != 0)
	{
        if(Reg_EXtoMEM.rd == Reg_DEtoEX.rs){
            forwardA = 0b10; 
        }else if(Reg_EXtoMEM.rd == Reg_DEtoEX.rt){
            forwardB = 0b10; 
        }
    }
    //check  MEM hazard
    if(Reg_MEMtoWB.regWrite && Reg_MEMtoWB.rd != 0 )
	{
         if(Reg_MEMtoWB.rd == Reg_DEtoEX.rs){
             forwardA = 0b01;
         }else if(Reg_MEMtoWB.rd == Reg_DEtoEX.rt){
             forwardB = 0b01;
         }
    }
   //stall check
    int check_stallFlag(uint32_t rs, uint32_t rt)
    {
        if(Reg_DEtoEX.control.memRead && (Reg_DEtoEX.rt == rs || Reg_DEtoEX.rt == rt))
            return 1;
        else
            return 0;
    }

}
void pipe_stage_wb()
{
	if(Reg_MEMtoWB.regWrite == 1){ // write data to dest register
        if(Reg_MEMtoWB.memToReg == 1){ // for lw instrucion
            CURRENT_STATE.REGS[Reg_MEMtoWB.rd] = Reg_MEMtoWB.memResult;
        }else{ // for R type or I type
            CURRENT_STATE.REGS[Reg_MEMtoWB.rd] = Reg_MEMtoWB.aluResult; // from alu result
        }
    }
}

uint32_t get_branchPC()
{
	uint32_t nextPc = Reg_EXtoMEM.PC; //ordinary situation
	// for branch
	nextPc += Reg_EXtoMEM.imm32*4;
	return nextPc;
}

uint32_t get_jmpPC()
{
	uint32_t nextPc = Reg_EXtoMEM.PC; //ordinary situation
	//address*4 means  need to shift left 2 , this address is 28bit
	nextPc = (nextPc & 0xF0000000)|(Reg_EXtoMEM.address*4);
	return nextPc;
}
void pipe_stage_mem()
{
	Reg_MEMtoWB_bak = Reg_MEMtoWB;
	// copy WB
	Reg_MEMtoWB.memToReg = Reg_EXtoMEM.memToReg;
	Reg_MEMtoWB.regWrite = Reg_EXtoMEM.regWrite;
	// copy alu result and rd
	Reg_MEMtoWB.aluResult = Reg_EXtoMEM.aluResult;
	Reg_MEMtoWB.rd = Reg_EXtoMEM.rd;

	if(Reg_EXtoMEM.branch == 1 && Reg_EXtoMEM.zeroFlag == 1){// for branch 
		CURRENT_STATE.PC = get_branchPC();
		Reg_IFtoDE.instruction = 0; // clear instruction
		memset(&Reg_DEtoEX.control, 0, sizeof(Controls)); // clear controls
	}else if(Reg_EXtoMEM.jump == 1){
		// address*4 means  need to shift left 2 , this address is 28bit
		CURRENT_STATE.PC = get_jmpPC();
		Reg_IFtoDE.instruction = 0; // clear instruction
		memset(&Reg_DEtoEX.control, 0, sizeof(Controls)); // clear controls
	}else if(Reg_EXtoMEM.memWrite == 1){  // load data from memory for sw
		mem_write_32(Reg_EXtoMEM.aluResult, Reg_EXtoMEM.rtVal); 
    }else if(Reg_EXtoMEM.memRead == 1){  // read data from memory for lw
        Reg_MEMtoWB.memResult = mem_read_32(Reg_EXtoMEM.aluResult);
    }
}
//0	and
// 1 or
//2	add
//6 sub
//7	less than
int get_ControlBit(int opcode, int funct)
{
	int control = 15; // not defined

	if (opcode == 0)
	{   //this  is R type
		switch (funct)
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
	}else{
		switch (opcode)
		{
		case 0x7: //bgtz
			control = 7;
			break;
		case 0x08: //addi
		case 0x09: //addiu
		case 0xf: //lui
		case 0x23: //lw
		case 0x2b: //sw
			control = 2;
			break;
		case 0xd: //ori
			control = 1;
			break;
		}
	}
	return control;
}
int isControlZero(Controls control)
{
	int sum = 0;
	int* array = (int*)&control;
	int i;
	for(i = 0; i < 10; i++){
		sum += array[i];
	}
	return sum == 0;
}
void pipe_stage_execute()
{
	if (isControlZero(Reg_DEtoEX.control))
	{
		memset(&Reg_EXtoMEM, 0, sizeof(Reg_EXtoMEM));
		return;  // if control is zero, exit function.
	}
	// copy WB 
	Reg_EXtoMEM.memToReg = Reg_DEtoEX.control.memToReg;
	Reg_EXtoMEM.regWrite = Reg_DEtoEX.control.regWrite;
	// copy M
	Reg_EXtoMEM.jump = Reg_DEtoEX.control.jump;
	Reg_EXtoMEM.branch = Reg_DEtoEX.control.branch;
	Reg_EXtoMEM.memRead = Reg_DEtoEX.control.memRead;
	Reg_EXtoMEM.memWrite = Reg_DEtoEX.control.memWrite;
	Reg_EXtoMEM.imm32 = Reg_DEtoEX.imm32; //pass imm32 for branche
	Reg_EXtoMEM.address = Reg_DEtoEX.address; // pass address for jmp
	// copy PC
	Reg_EXtoMEM.PC = Reg_DEtoEX.PC;
	// copy rtVal and rd
	Reg_EXtoMEM.rtVal = Reg_DEtoEX.rtVal;
	// choose destined register 
	if(Reg_DEtoEX.control.regDst == 1)
		Reg_EXtoMEM.rd = Reg_DEtoEX.rd;
	else
		Reg_EXtoMEM.rd = Reg_DEtoEX.rt;
	
	// get operator
	int control_bit = get_ControlBit(Reg_DEtoEX.opcode, Reg_DEtoEX.funct);

	int src1 = Reg_DEtoEX.rsVal;
	if(forwardA == 0b10) // value from exe stage
    {
        src1 = Reg_EXtoMEM.aluResult; // last alu result
    }else if(forwardA == 0b01) // value mem stage
	{
        if(Reg_MEMtoWB.memToReg == 1) // mem to reg
            src1 = Reg_MEMtoWB_bak.memResult; 
        else  // value from mem's aluresult
            src1 = Reg_MEMtoWB_bak.aluResult;
    }

	int src2 = Reg_DEtoEX.rtVal;
	if(forwardB == 0b10) // value from exe stage
    {
        src2 = Reg_EXtoMEM.aluResult; // from last exe alu value
    }else if(forwardB == 0b01){ // value from mem stage
        if(Reg_MEMtoWB.memToReg == 1)  
            src2 = Reg_MEMtoWB_bak.memResult;
        else // value from mem's aluresult
            src2 = Reg_MEMtoWB_bak.aluResult;
    }
	// make sure source2
	if (Reg_DEtoEX.control.ALUsrc == 1)
		src2 = Reg_DEtoEX.imm32;

	
	if(Reg_DEtoEX.opcode == 0xf) // lui
		src2 = src2<<16;

	if(Reg_DEtoEX.opcode == 0x7)
	{
		src2 = src1;
		src1 = 0;
	}
	int result = -1;
	switch (control_bit)
	{
	case 0:  // and
		result = src1 & src2;
		break;
	case 1: //ori
		result = src1 | src2;
		break;
	case 2: // add, addi,lw,sw, lui
		result = src1 + src2;
		break;
	case 6:  // sub,subu 
		result = src1 - src2;
		break;
	case 7: // slt, bgtz
		if (src1 < src2)
			result = 1;
		else
			result = 0;
		break;
	}

	Reg_EXtoMEM.zeroFlag = 0;
	Reg_EXtoMEM.aluResult = result;
	if (result == 0) // set zero flag
		Reg_EXtoMEM.zeroFlag = 1;
	if(Reg_DEtoEX.opcode == 0x7 && result)
		Reg_EXtoMEM.zeroFlag = 1;
}

void extract_instruction(uint32_t instruction)
{
	//function takes an instruction as an input, it will read all it's fields
	field.opcode = instruction >> 26 & 0x3f;

	field.rs = (instruction >> 21) & 0x1f; //0000000011111
	field.rt = (instruction >> 16) & 0x1f;

	// R type instrution also need rd, funct, shamt 
	field.rd = (instruction >> 11) & 0x1f;
	field.funct = instruction & 0x3f;

	//I-Type  instrution just need imm16/32
	field.imm16 = instruction & 0x0000ffff;
	field.imm32 = field.imm16;
	if (field.imm16 & 0x8000)
		field.imm32 = field.imm16 | 0xffff0000;

	// J-Type instrution just need address
	//This is used for j instruction and the address is 26bits.
	field.address = instruction & 0x3ffffff;
}
void fill_control(Controls* control)
{
	memset(control, 0, sizeof(Controls)); // clear control

	if (field.opcode == 0)
	{   //this  is R type
		control->regDst = 1;
		control->regWrite = 1;
		control->ALUOp1 = 1;
	}
	else
	{
		switch (field.opcode)
		{
		case 0x02://j
			control->jump = 1;
			break;
		case 0x7: //bgtz
			control->branch = 1;
			break;
		case 0x08: //addi
		case 0x09: //addiu
		case 0xf: // lui
		case 0xd: // ori
			control->ALUsrc = 1;
			control->ALUOp0 = 1;
			control->ALUOp1 = 1;
			control->regWrite = 1;
			break;
		case 0x23://lw
			control->ALUsrc = 1;
			control->memRead = 1;
			control->memToReg = 1;
			control->regWrite = 1;
			break;
		case 0x2b://sw
			control->ALUsrc = 1;
			control->memWrite = 1;
			break;
		}
	}
}

void pipe_stage_decode()
{
	if (Reg_IFtoDE.instruction == 0)
		return;
	extract_instruction(Reg_IFtoDE.instruction);

    // fill data
	Reg_DEtoEX.opcode = field.opcode;
	Reg_DEtoEX.funct = field.funct;
	Reg_DEtoEX.PC = Reg_IFtoDE.PC;
	Reg_DEtoEX.rs = field.rs;
	Reg_DEtoEX.rt = field.rt;
	Reg_DEtoEX.rd = field.rd;

	Reg_DEtoEX.rsVal = CURRENT_STATE.REGS[field.rs];
	Reg_DEtoEX.rtVal = CURRENT_STATE.REGS[field.rt];
	Reg_DEtoEX.imm32 = field.imm32;  // for I type
	Reg_DEtoEX.address = field.address; // for  j type

	// check stallFlag In DE/EX stage
	stallFlag += check_stallFlag(Reg_DEtoEX.rs, Reg_DEtoEX.rt);
	if (stallFlag) {
		memset(&Reg_DEtoEX.control, 0, sizeof(Controls));
	}
	else	// fill control code
	{
		fill_control(&Reg_DEtoEX.control);
		detect_forwarding();
	}
}

void pipe_stage_fetch() 
{
	if (stallFlag > 0) {
		stallFlag--;
		return; // if stallFlag, do nothing
	}
	uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
	if (instruction == 0 && ++stageCount == 5)
	{ // if there no instructions, need to wait 5 stags
		RUN_BIT = 0;
	}
	CURRENT_STATE.PC = CURRENT_STATE.PC + 4; // PC + 4
	Reg_IFtoDE.instruction = instruction; // copy instrction
	Reg_IFtoDE.PC = CURRENT_STATE.PC;
}
