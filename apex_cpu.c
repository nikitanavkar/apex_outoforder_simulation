/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apex_cpu.h"
#include "apex_macros.h"

/* Nvariable to store the preveous stalling stage of CPU*/
static int prev_stage = CONTINUE_EXEC;
/* Variable to monitor if halt instruction was fetched while in stalled stage*/
static int last_halt = FALSE;

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

/* Prints the current instruction from fetch stage in debug messages
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JALR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_JUMP:
        {
            printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_NOP:
        {
            printf("%s ", stage->opcode_str);
            break;
        }

        case OPCODE_RET:
        {
        	printf("%s,R%d ", stage->opcode_str, stage->rs1);
            break;
        }
    }
}

/* Prints the current instruction of IU, BU or MU in execute or
 * writeback stage in debug messages
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_instruction1(const APEX_CPU *cpu, const FU_Stage *stage)
{
    switch (stage->iq_entry.fu_type)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_CMP:
        {
        	if(stage->iq_entry.src1_src == 0)
        	{
        		if(stage->iq_entry.src2_src == 0)
        		{
                    printf("%s,P%d,R%d,R%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest,
                    		stage->iq_entry.src1_tag, stage->iq_entry.src2_tag);
        		}
        		else
        		{
                    printf("%s,P%d,R%d,P%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest,
                    		stage->iq_entry.src1_tag, cpu->rb.entries[stage->iq_entry.src2_tag].phy_address);
        		}
        	}
        	else
        	{
        		if(stage->iq_entry.src2_src == 0)
        		{
                    printf("%s,P%d,P%d,R%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest,
                    		cpu->rb.entries[stage->iq_entry.src1_tag].phy_address, stage->iq_entry.src2_tag);
        		}
        		else
        		{
                    printf("%s,P%d,P%d,P%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest,
                    		cpu->rb.entries[stage->iq_entry.src1_tag].phy_address, cpu->rb.entries[stage->iq_entry.src2_tag].phy_address);
        		}
        	}
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,P%d,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest, stage->iq_entry.literal);
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JALR:
        {
    		if(stage->iq_entry.src1_src == 0)
    		{
            	printf("%s,P%d,R%d,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest, stage->iq_entry.src1_tag,
            			stage->iq_entry.literal);
    		}
    		else
    		{
            	printf("%s,P%d,P%d,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.dest, cpu->rb.entries[stage->iq_entry.src1_tag].phy_address,
            			stage->iq_entry.literal);
    		}
            break;
        }

        case OPCODE_STORE:
        {
        	if(stage->iq_entry.src1_src == 0)
        	{
        		if(stage->iq_entry.src2_src == 0)
        		{
                    printf("%s,R%d,R%d,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.src1_tag,
                    		stage->iq_entry.src2_tag, stage->iq_entry.literal);
        		}
        		else
        		{
                    printf("%s,R%d,P%d,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.src1_tag,
                    		cpu->rb.entries[stage->iq_entry.src2_tag].phy_address, stage->iq_entry.literal);
        		}
        	}
        	else
        	{
        		if(stage->iq_entry.src2_src == 0)
        		{
                    printf("%s,P%d,R%d,#%d ", stage->iq_entry.opcode_str, cpu->rb.entries[stage->iq_entry.src1_tag].phy_address,
                    		stage->iq_entry.src2_tag, stage->iq_entry.literal);
        		}
        		else
        		{
                    printf("%s,P%d,P%d,#%d ", stage->iq_entry.opcode_str, cpu->rb.entries[stage->iq_entry.src1_tag].phy_address,
                    		cpu->rb.entries[stage->iq_entry.src2_tag].phy_address, stage->iq_entry.literal);
        		}
        	}
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            printf("%s,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.literal);
            break;
        }

        case OPCODE_JUMP:
        {
    		if(stage->iq_entry.src1_src == 0)
    		{
    			printf("%s,R%d,#%d ", stage->iq_entry.opcode_str, stage->iq_entry.src1_tag, stage->iq_entry.literal);
    		}
    		else
    		{
    			printf("%s,P%d,#%d ", stage->iq_entry.opcode_str, cpu->rb.entries[stage->iq_entry.src1_tag].phy_address, stage->iq_entry.literal);
    		}
            break;
        }

        case OPCODE_RET:
        {
    		if(stage->iq_entry.src1_src == 0)
    		{
    			printf("%s,R%d ", stage->iq_entry.opcode_str, stage->iq_entry.src1_tag);
    		}
    		else
    		{
    			printf("%s,P%d ", stage->iq_entry.opcode_str, cpu->rb.entries[stage->iq_entry.src1_tag].phy_address);
    		}
            break;
        }

        case OPCODE_NOP:
        {
            printf("NOP ");
            break;
        }

        case OPCODE_HALT:
        {
            printf("HALT");
            break;
        }
    }
}

/* Prints the current instruction of L/D FU in execute or
 * writeback stage in debug messages
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_instruction2(const APEX_CPU *cpu, const MEM_FU_Stage *stage)
{
    switch (stage->lsq_entry.ls_bit)
    {
        case 0:
        {
        	printf("%s,P%d,#%d ", stage->lsq_entry.opcode_str, stage->lsq_entry.dest, stage->lsq_entry.mem_address);
            break;
        }

        case 1:
        {
        	if(stage->lsq_entry.src1_src == 0)
        	{
        		printf("%s,R%d,#%d ", stage->lsq_entry.opcode_str, stage->lsq_entry.src1_tag, stage->lsq_entry.mem_address);
        	}
        	else
        	{
        		printf("%s,P%d,#%d ", stage->lsq_entry.opcode_str, cpu->rb.entries[stage->lsq_entry.src1_tag].phy_address, stage->lsq_entry.mem_address);
        	}
            break;
        }
    }
}

/* Prints the current instruction in rename2/dispatch stage
 * in debug messages
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_instruction3(const APEX_CPU *cpu, const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_CMP:
        {
        	if(cpu->rename_table[stage->rs1].src_bit == 0)
        	{
        		if(cpu->rename_table[stage->rs2].src_bit == 0)
        		{
                    printf("%s,P%d,R%d,R%d ", stage->opcode_str, stage->pd, stage->p1, stage->p2);
        		}
        		else
        		{
                    printf("%s,P%d,R%d,P%d ", stage->opcode_str, stage->pd, stage->p1, cpu->rb.entries[stage->p2].phy_address);
        		}
        	}
        	else
        	{
        		if(cpu->rename_table[stage->rs2].src_bit == 0)
        		{
                    printf("%s,P%d,P%d,R%d ", stage->opcode_str, stage->pd, cpu->rb.entries[stage->p1].phy_address, stage->p2);
        		}
        		else
        		{
                    printf("%s,P%d,P%d,P%d ", stage->opcode_str, stage->pd, cpu->rb.entries[stage->p1].phy_address,
                    		cpu->rb.entries[stage->p2].phy_address);
        		}
        	}
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,P%d,#%d ", stage->opcode_str, stage->pd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JALR:
        {
    		if(cpu->rename_table[stage->rs1].src_bit == 0)
    		{
                printf("%s,P%d,R%d,#%d ", stage->opcode_str, stage->pd, stage->p1, stage->imm);
    		}
    		else
    		{
                printf("%s,P%d,P%d,#%d ", stage->opcode_str, stage->pd, cpu->rb.entries[stage->p1].phy_address, stage->imm);
    		}
            break;
        }

        case OPCODE_STORE:
        {
        	if(cpu->rename_table[stage->rs1].src_bit == 0)
        	{
        		if(cpu->rename_table[stage->rs2].src_bit == 0)
        		{
                    printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->p1, stage->p2, stage->imm);
        		}
        		else
        		{
                    printf("%s,R%d,P%d,#%d ", stage->opcode_str, stage->p1, cpu->rb.entries[stage->p2].phy_address, stage->imm);
        		}
        	}
        	else
        	{
        		if(cpu->rename_table[stage->rs2].src_bit == 0)
        		{
                    printf("%s,P%d,R%d,#%d ", stage->opcode_str, cpu->rb.entries[stage->p1].phy_address, stage->p2, stage->imm);
        		}
        		else
        		{
                    printf("%s,P%d,P%d,#%d ", stage->opcode_str, cpu->rb.entries[stage->p1].phy_address,
                    		cpu->rb.entries[stage->p2].phy_address, stage->imm);
        		}
        	}
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_JUMP:
        {
    		if(cpu->rename_table[stage->rs1].src_bit == 0)
    		{
                printf("%s,R%d,#%d ", stage->opcode_str, stage->p1, stage->imm);
    		}
    		else
    		{
                printf("%s,P%d,#%d ", stage->opcode_str, cpu->rb.entries[stage->p1].phy_address, stage->imm);
    		}
            break;
        }

        case OPCODE_RET:
        {
    		if(cpu->rename_table[stage->rs1].src_bit == 0)
    		{
                printf("%s,R%d ", stage->opcode_str, stage->p1);
    		}
    		else
    		{
                printf("%s,P%d ", stage->opcode_str, cpu->rb.entries[stage->p1].phy_address);
    		}
            break;
        }

        case OPCODE_HALT:
        case OPCODE_NOP:
        {
        	printf("%s ", stage->opcode_str);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const APEX_CPU *cpu)
{
    for(int i = 1; i <= NUM_OF_PIPELINE_STAGES; i++)
    {
    	int cur_stage = i - 1;
    	switch(cur_stage)
    	{
    	case FETCH:
			{
		        if(cpu->debug_fetch.has_insn)
		        {
		            int ins_num = (cpu->debug_fetch.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at FETCH______STAGE --->          (I%d: %02d) ", i, ins_num, cpu->debug_fetch.pc);
		            print_instruction(&cpu->debug_fetch);
		        }
		        else
		        {
		            printf("%d.  Instruction at FETCH______STAGE --->          EMPTY", i);
		        }
				break;
			}
    	case DECODE_RENAME1:
			{
		        if(cpu->debug_decode_rename1.has_insn)
		        {
		            int ins_num = (cpu->debug_decode_rename1.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at DECODE___RENAME1 --->          (I%d: %02d) ", i, ins_num, cpu->debug_decode_rename1.pc);
		            print_instruction3(cpu, &cpu->debug_decode_rename1);
		        }
		        else
		        {
		            printf("%d.  Instruction at DECODE___RENAME1 --->          EMPTY", i);
		        }
				break;
			}
    	case RENAME2_DISPATCH:
			{
		        if(cpu->debug_rename2_dispatch.has_insn)
		        {
		            int ins_num = (cpu->debug_rename2_dispatch.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at RENAME2_DISPATCH --->          (I%d: %02d) ", i, ins_num, cpu->debug_rename2_dispatch.pc);
		            print_instruction3(cpu, &cpu->debug_rename2_dispatch);
		        }
		        else
		        {
		            printf("%d.  Instruction at RENAME2_DISPATCH --->          EMPTY", i);
		        }
				break;
			}
    	case EXECUTE_IU:
			{
		        if(cpu->debug_execute_iu.has_insn)
		        {
		            int ins_num = (cpu->debug_execute_iu.iq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at EXECUTE_______IU --->          (I%d: %02d) ", i, ins_num, cpu->debug_execute_iu.iq_entry.pc);
		            print_instruction1(cpu, &cpu->debug_execute_iu);
		        }
		        else
		        {
		            printf("%d.  Instruction at EXECUTE_______IU --->          EMPTY", i);
		        }
				break;
			}
    	case EXECUTE_MU:
			{
		        if(cpu->debug_execute_mu.has_insn)
		        {
		            int ins_num = (cpu->debug_execute_mu.iq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at EXECUTE_______MU --->          (I%d: %02d) ", i, ins_num, cpu->debug_execute_mu.iq_entry.pc);
		            print_instruction1(cpu, &cpu->debug_execute_mu);
		        }
		        else
		        {
		            printf("%d.  Instruction at EXECUTE_______MU --->          EMPTY", i);
		        }
				break;
			}
    	case EXECUTE_BU:
			{
		        if(cpu->debug_execute_bu.has_insn)
		        {
		            int ins_num = (cpu->debug_execute_bu.iq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at EXECUTE_______BU --->          (I%d: %02d) ", i, ins_num, cpu->debug_execute_bu.iq_entry.pc);
		            print_instruction1(cpu, &cpu->debug_execute_bu);
		        }
		        else
		        {
		            printf("%d.  Instruction at EXECUTE_______BU --->          EMPTY", i);
		        }
				break;
			}
    	case EXECUTE_LOAD_STORE:
			{
		        if(cpu->debug_execute_load_store.has_insn)
		        {
		            int ins_num = (cpu->debug_execute_load_store.lsq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at EXECUTE_LD_STORE --->          (I%d: %02d) ", i, ins_num, cpu->debug_execute_load_store.lsq_entry.pc);
		            print_instruction2(cpu, &cpu->debug_execute_load_store);
		        }
		        else
		        {
		            printf("%d.  Instruction at EXECUTE_LD_STORE --->          EMPTY", i);
		        }
				break;
			}
    	case WRITEBACK_IU:
			{
		        if(cpu->debug_writeback_iu.has_insn)
		        {
		            int ins_num = (cpu->debug_writeback_iu.iq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at WRITEBACK_____IU --->          (I%d: %02d) ", i, ins_num, cpu->debug_writeback_iu.iq_entry.pc);
		            print_instruction1(cpu, &cpu->debug_writeback_iu);
		        }
		        else
		        {
		            printf("%d.  Instruction at WRITEBACK_____IU --->          EMPTY", i);
		        }
				break;
			}
    	case WRITEBACK_MU:
			{
		        if(cpu->debug_writeback_mu.has_insn)
		        {
		            int ins_num = (cpu->debug_writeback_mu.iq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d.  Instruction at WRITEBACK_____MU --->          (I%d: %02d) ", i, ins_num, cpu->debug_writeback_mu.iq_entry.pc);
		            print_instruction1(cpu, &cpu->debug_writeback_mu);
		        }
		        else
		        {
		            printf("%d.  Instruction at WRITEBACK_____MU --->          EMPTY", i);
		        }
				break;
			}
    	case WRITEBACK_BU:
			{
		        if(cpu->debug_writeback_bu.has_insn)
		        {
		            int ins_num = (cpu->debug_writeback_bu.iq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d. Instruction at WRITEBACK_____BU --->          (I%d: %02d) ", i, ins_num, cpu->debug_writeback_bu.iq_entry.pc);
		            print_instruction1(cpu, &cpu->debug_writeback_bu);
		        }
		        else
		        {
		            printf("%d. Instruction at WRITEBACK_____BU --->          EMPTY", i);
		        }
				break;
			}
    	case WRITEBACK_LOAD:
			{
		        if(cpu->debug_writeback_load.has_insn)
		        {
		            int ins_num = (cpu->debug_writeback_load.lsq_entry.pc - PC_START) / INSTRUCTION_SIZE;
		            printf("%d. Instruction at WRITEBACK___LOAD --->          (I%d: %02d) ", i, ins_num, cpu->debug_writeback_load.lsq_entry.pc);
		            print_instruction2(cpu, &cpu->debug_writeback_load);
		        }
		        else
		        {
		            printf("%d. Instruction at WRITEBACK__LD_ST --->          EMPTY", i);
		        }
				break;
			}
    	}
        printf("\n");
    }
}

/* Debug function which prints the Issue Queue entries that
 * are allocated
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_iq(const APEX_CPU *cpu)
{
    printf("\n=============== STATE OF ISSUE QUEUE ==========\n\n");
    printf("Id   Allocation  FU   Literal   Src1-Ready   Src1-Tag   Src1-Value   Src2-Ready   Src2-Tag   Src2-Value   Dest   LSQ   Cycle   Instruction\n");
    for(int i = 0; i < IQ_SIZE; i++)
    {
    	if(cpu->iq.entries[i].al == ALLOCATED)
    	{
    		int ins_num = (cpu->iq.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
            printf("%d    %d           %02d   %-4d      %d            %02d         %-6d       %d            %02d         %-6d       %02d     %-2d    %-3d     I%02d\n",
            		i, cpu->iq.entries[i].al, cpu->iq.entries[i].fu_type, cpu->iq.entries[i].literal, cpu->iq.entries[i].src1_ready, cpu->iq.entries[i].src1_tag,
					cpu->iq.entries[i].src1_value, cpu->iq.entries[i].src2_ready, cpu->iq.entries[i].src2_tag, cpu->iq.entries[i].src2_value,
					cpu->iq.entries[i].dest, cpu->iq.entries[i].lsq_id, cpu->iq.entries[i].cycle, ins_num);
    	}
    }
}

/* Debug function which prints the Load Store Queue entries
 * from head -> tail
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_lsq(const APEX_CPU *cpu)
{
    printf("\n=============== STATE OF LOAD STORE QUEUE ==========\n\n");
    printf("Id     Allocation  L/S   Memory-Valid   Memory-Address   Dest   Data-Ready   Src1-Tag   Src1-Value     Cycle   Instruction\n");

    if(cpu->lsq.size == 0)
    {
        return;
    }
    if(cpu->lsq.rear >= cpu->lsq.front)
    {
        for(int i = cpu->lsq.front; i <= cpu->lsq.rear; i++)
        {
    		int ins_num = (cpu->lsq.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
            printf("%d      %d           %d     %d              %-4d             %02d     %d            %02d         %-6d         %-3d     I%d\n",
            		i, cpu->lsq.entries[i].al, cpu->lsq.entries[i].ls_bit, cpu->lsq.entries[i].mem_valid, cpu->lsq.entries[i].mem_address, cpu->lsq.entries[i].dest,
					cpu->lsq.entries[i].data_ready, cpu->lsq.entries[i].src1_tag, cpu->lsq.entries[i].src1_value, cpu->lsq.entries[i].cycle, ins_num);
        }
    }
    else
    {
        for(int i = cpu->lsq.front; i < LSQ_SIZE; i++)
        {
    		int ins_num = (cpu->lsq.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
            printf("%d      %d           %d     %d              %-4d             %02d     %d            %02d         %-6d         %-3d     I%d\n",
            		i, cpu->lsq.entries[i].al, cpu->lsq.entries[i].ls_bit, cpu->lsq.entries[i].mem_valid, cpu->lsq.entries[i].mem_address, cpu->lsq.entries[i].dest,
					cpu->lsq.entries[i].data_ready, cpu->lsq.entries[i].src1_tag, cpu->lsq.entries[i].src1_value, cpu->lsq.entries[i].cycle, ins_num);
        }
        for(int i = 0; i <= cpu->lsq.rear; i++)
        {
    		int ins_num = (cpu->lsq.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
            printf("%d      %d           %d     %d              %-4d             %02d     %d            %02d         %-6d         %-3d     I%d\n",
            		i, cpu->lsq.entries[i].al, cpu->lsq.entries[i].ls_bit, cpu->lsq.entries[i].mem_valid, cpu->lsq.entries[i].mem_address, cpu->lsq.entries[i].dest,
					cpu->lsq.entries[i].data_ready, cpu->lsq.entries[i].src1_tag, cpu->lsq.entries[i].src1_value, cpu->lsq.entries[i].cycle, ins_num);
        }
    }
}

/* Debug function which prints the physical register file
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_phy_reg_file(const APEX_CPU *cpu)
{
    printf("\n=============== STATE OF PHYSICAL REGISTER FILE ==========\n\n");
    printf("RegId   Allocation   Status   Value   Renamed   z-flag   p-flag   W0   W1   W2   W3   W4   W5   W6   W7\n");
    for(int i = 0; i < PHYS_REG_FILE_SIZE; i++)
    {
    	if(cpu->phys_regs[i].al == UN_ALLOCATED)
    	{
    		continue;
    	}
        printf("P%02d     %d            %d        %-6d  %d         %d        %d        %d    %d    %d    %d    %d    %d    %d    %d\n",
        		i, cpu->phys_regs[i].al, cpu->phys_regs[i].status, cpu->phys_regs[i].value, cpu->phys_regs[i].renamed, cpu->phys_regs[i].z_flag,
				cpu->phys_regs[i].p_flag, cpu->phys_regs[i].wbv.BIT.W0, cpu->phys_regs[i].wbv.BIT.W1, cpu->phys_regs[i].wbv.BIT.W2,
				cpu->phys_regs[i].wbv.BIT.W3, cpu->phys_regs[i].wbv.BIT.W4, cpu->phys_regs[i].wbv.BIT.W5, cpu->phys_regs[i].wbv.BIT.W6,
				cpu->phys_regs[i].wbv.BIT.W7);
    }
}

/* Debug function which prints the architecuture register file
 * in combination with rename table
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_arch_reg_file(const APEX_CPU *cpu)
{
    printf("\n=============== STATE OF ARCHITECTURAL REGISTER FILE / RENAME TABLE ==========\n\n");
    printf("RegId     Value    z-flag    p-flag    Src_Bit     Slot_Id\n");
    for(int i = 0; i < REG_FILE_SIZE; i++)
    {
    	if(i == REG_FILE_SIZE - 1)
    	{
    		printf("CC        %-6d   %d         %d         %-2d           %-2d\n", cpu->arch_regs[i].value, cpu->arch_regs[i].z_flag, cpu->arch_regs[i].p_flag,
    					cpu->rename_table[i].src_bit, cpu->rename_table[i].slot_id);
    	}
    	else
    	{
    		printf("R%02d       %-6d   %d         %d         %-2d           %-2d\n", i, cpu->arch_regs[i].value, cpu->arch_regs[i].z_flag, cpu->arch_regs[i].p_flag,
    					cpu->rename_table[i].src_bit, cpu->rename_table[i].slot_id);
    	}
    }
}

/* Debug function which prints the reorder buffer from
 * head->tail
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_rob(const APEX_CPU *cpu)
{
    printf("\n===================== STATE OF Reorder Buffer ================\n\n");
    printf("ROBId   Status  Sval_Valid   Excodes     Instruction     OPCode    PhysicalReg     ArchitectureReg     Result\n");
    if(cpu->rb.size == 0)
    {
        return;
    }
    if(cpu->rb.rear >= cpu->rb.front)
    {
        for(int i = cpu->rb.front; i <= cpu->rb.rear; i++)
        {
        	int ins_num = (cpu->rb.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
        	printf("%02d      %d       %d            %04d        I%02d             %02d        P%02d             R%02d                 %-6d\n", i, cpu->rb.entries[i].status, cpu->rb.entries[i].sval_valid, cpu->rb.entries[i].excodes,
        			ins_num, cpu->rb.entries[i].itype, cpu->rb.entries[i].phy_address, cpu->rb.entries[i].arch_address, cpu->rb.entries[i].result);
        }
    }
    else
    {
        for(int i = cpu->rb.front; i < ROB_SIZE; i++)
        {
        	int ins_num = (cpu->rb.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
        	printf("%02d      %d       %d            %04d        I%02d             %02d        P%02d             R%02d                 %-6d\n", i, cpu->rb.entries[i].status, cpu->rb.entries[i].sval_valid, cpu->rb.entries[i].excodes,
        			ins_num, cpu->rb.entries[i].itype, cpu->rb.entries[i].phy_address, cpu->rb.entries[i].arch_address, cpu->rb.entries[i].result);
        }
        for(int i = 0; i <= cpu->rb.rear; i++)
        {
        	int ins_num = (cpu->rb.entries[i].pc - PC_START) / INSTRUCTION_SIZE;
        	printf("%02d      %d       %d            %04d        I%02d             %02d        P%02d             R%02d                 %-6d\n", i, cpu->rb.entries[i].status, cpu->rb.entries[i].sval_valid, cpu->rb.entries[i].excodes,
        			ins_num, cpu->rb.entries[i].itype, cpu->rb.entries[i].phy_address, cpu->rb.entries[i].arch_address, cpu->rb.entries[i].result);
        }
    }
}

/* Debug function which prints the data memory
 *
 * Note: You can edit this function to print in more detail
 */
static void 
print_data_memory(const APEX_CPU *cpu)
{
    printf("\n==================== STATE OF DATA MEMORY =====================\n\n");
    for(int i = 0; i < DATA_MEMORY_SIZE; i++)
    {
        printf("|      MEM[%04d]      |      Data Value = %d      \n", i, cpu->data_memory[i]);
    }
}

/* Debug function which prints the debug messages in each
 * cycle based on the CPU command
 *
 * Note: You can edit this function to print in more detail
 */
static int 
print_debug_info(const APEX_CPU *cpu)
{
    switch (cpu->command->cmd)
    {
        case SIMULATE:
            {
                if(cpu->clock >= cpu->command->data)
                {
                    print_phy_reg_file(cpu);
                    print_arch_reg_file(cpu);
                    print_iq(cpu);
                    print_lsq(cpu);
                    print_rob(cpu);
                    print_data_memory(cpu);
                    printf("\nAPEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                    return TRUE;
                }
            }
            break;

        case DISPLAY:
            {
                if(cpu->clock >= cpu->command->data)
                {
                    print_phy_reg_file(cpu);
                    print_arch_reg_file(cpu);
                    print_iq(cpu);
                    print_lsq(cpu);
                    print_rob(cpu);
                    print_data_memory(cpu);
                    printf("\nAPEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                    return TRUE;
                }
                else
                {
                    printf("\n....................... CLOCK CYCLE %d ........................\n\n", cpu->clock+1);
                    print_stage_content(cpu);
                    print_phy_reg_file(cpu);
                    print_arch_reg_file(cpu);
                    print_iq(cpu);
                    print_lsq(cpu);
                    print_rob(cpu);
                }
            }
            break;
    
        case SHOW_MEM:
            /* Do nothing because data at given memory location
             * is printed at the end of the program 
             */
            break;
    
        case SINGLE_STEP:
            {
                printf("\n....................... CLOCK CYCLE %d ........................\n\n", cpu->clock+1);
                print_stage_content(cpu);
                print_phy_reg_file(cpu);
                print_arch_reg_file(cpu);
                print_iq(cpu);
                print_lsq(cpu);
                print_rob(cpu);
            }
            break;
    
        default:
            break;
    }
    return 0;
}

/* Function to get the free physical register available
 *
 */
static int
get_free_physical_register(APEX_CPU *cpu)
{
	int ret = -1;
	for(int i = 0; i < PHYS_REG_FILE_SIZE - HIDDEN_PHY_REG_FILE_SIZE; i++)
	{
		if(cpu->phys_regs[i].al == UN_ALLOCATED)
		{
			ret = i;
			break;
		}
	}
	return ret;
}

/* Function to get the free hidden physical register
 *
 */
static int
get_free_hidden_physical_register(APEX_CPU *cpu)
{
	int ret = -1;
	for(int i = PHYS_REG_FILE_SIZE - HIDDEN_PHY_REG_FILE_SIZE; i < PHYS_REG_FILE_SIZE; i++)
	{
		if(cpu->phys_regs[i].al == UN_ALLOCATED)
		{
			ret = i;
			break;
		}
	}
	return ret;
}

/* Function to allocate the free physical register
 *
 */
static void
allocate_physical_register(APEX_CPU *cpu, uint8 reg)
{
	cpu->phys_regs[reg].al = ALLOCATED;
	cpu->phys_regs[reg].renamed = NOT_RENAMED;
	cpu->phys_regs[reg].status = INVALID;
	cpu->phys_regs[reg].value = 0;
	cpu->phys_regs[reg].p_flag = 0;
	cpu->phys_regs[reg].z_flag = 0;
	cpu->phys_regs[reg].wbv.BYTE = 0;
}

/* Function to get the free issue queue entry
 *
 */
static int
get_free_issue_queue_entry(APEX_CPU *cpu)
{
	int ret = -1;
	for(int i = 0; i < IQ_SIZE; i++)
	{
		if(cpu->iq.entries[i].al == UN_ALLOCATED)
		{
			ret = i;
			break;
		}
	}
	return ret;
}

/* Function to get the free lsq entry
 *
 */
static int
get_free_load_store_queue_entry(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->lsq.size < LSQ_SIZE)
	{
		cpu->lsq.rear = (cpu->lsq.rear + 1) % LSQ_SIZE;
		id = cpu->lsq.rear;
		cpu->lsq.size++;
	}
	return id;
}

/* Function to get the free rob entry
 *
 */
static int
get_free_rob_entry(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->rb.size < ROB_SIZE)
	{
		cpu->rb.rear = (cpu->rb.rear + 1) % ROB_SIZE;
		id = cpu->rb.rear;
		cpu->rb.size++;
	}
	return id;
}

/* Function to update the src1 of the issue queue entry
 *
 */
static void
update_src1(int reg, int id, APEX_CPU *cpu)
{
	cpu->iq.entries[id].src1_src = cpu->rename_table[cpu->rename2_dispatch.rs1].src_bit;
	if(cpu->rename_table[cpu->rename2_dispatch.rs1].src_bit == 0)
	{
		cpu->iq.entries[id].src1_ready = VALID;
		cpu->iq.entries[id].src1_value = cpu->arch_regs[cpu->rename2_dispatch.rs1].value;
		cpu->iq.entries[id].z_flag = cpu->arch_regs[cpu->rename2_dispatch.rs1].z_flag;
		cpu->iq.entries[id].p_flag = cpu->arch_regs[cpu->rename2_dispatch.rs1].p_flag;
	}
	else
	{
		if(cpu->phys_regs[cpu->rb.entries[reg].phy_address].status == VALID)
		{
			cpu->iq.entries[id].src1_ready = VALID;
			cpu->iq.entries[id].src1_value = cpu->phys_regs[cpu->rb.entries[reg].phy_address].value;
			cpu->iq.entries[id].z_flag = cpu->phys_regs[cpu->rb.entries[reg].phy_address].z_flag;
			cpu->iq.entries[id].p_flag = cpu->phys_regs[cpu->rb.entries[reg].phy_address].p_flag;
		}
		else
		{
			if(cpu->execute_iu.latch.reg_id == reg && cpu->execute_iu.latch.ready == VALID)
			{
				cpu->iq.entries[id].src1_ready = VALID;
				cpu->iq.entries[id].src1_value = cpu->execute_iu.latch.data;
				cpu->iq.entries[id].z_flag = cpu->execute_iu.latch.z_flag;
				cpu->iq.entries[id].p_flag = cpu->execute_iu.latch.p_flag;
			}
			else if(cpu->execute_mu.latch.reg_id == reg && cpu->execute_mu.latch.ready == VALID)
			{
				cpu->iq.entries[id].src1_ready = VALID;
				cpu->iq.entries[id].src1_value = cpu->execute_mu.latch.data;
				cpu->iq.entries[id].z_flag = cpu->execute_iu.latch.z_flag;
				cpu->iq.entries[id].p_flag = cpu->execute_iu.latch.p_flag;
			}
			else if(cpu->execute_load_store.latch.reg_id == reg && cpu->execute_load_store.latch.ready == VALID)
			{
				cpu->iq.entries[id].src1_ready = VALID;
				cpu->iq.entries[id].src1_value = cpu->execute_load_store.latch.data;
				cpu->iq.entries[id].z_flag = cpu->execute_iu.latch.z_flag;
				cpu->iq.entries[id].p_flag = cpu->execute_iu.latch.p_flag;
			}
			else
			{
				cpu->iq.entries[id].src1_ready = INVALID;
				cpu->phys_regs[cpu->rb.entries[reg].phy_address].wbv.BYTE |=  1<<(id);
			}
		}
	}
}

/* Function to update the src2 of the issue queue entry
 *
 */
static void
update_src2(int reg, int id, APEX_CPU *cpu)
{
	cpu->iq.entries[id].src2_src = cpu->rename_table[cpu->rename2_dispatch.rs2].src_bit;
	if(cpu->rename_table[cpu->rename2_dispatch.rs2].src_bit == 0)
	{
		cpu->iq.entries[id].src2_ready = VALID;
		cpu->iq.entries[id].src2_value = cpu->arch_regs[cpu->rename2_dispatch.rs2].value;
	}
	else
	{
		if(cpu->phys_regs[cpu->rb.entries[reg].phy_address].status == VALID)
		{
			cpu->iq.entries[id].src2_ready = VALID;
			cpu->iq.entries[id].src2_value = cpu->phys_regs[cpu->rb.entries[reg].phy_address].value;
		}
		else
		{
			if(cpu->execute_iu.latch.reg_id == reg && cpu->execute_iu.latch.ready == VALID)
			{
				cpu->iq.entries[id].src2_ready = VALID;
				cpu->iq.entries[id].src2_value = cpu->execute_iu.latch.data;
			}
			else if(cpu->execute_mu.latch.reg_id == reg && cpu->execute_mu.latch.ready == VALID)
			{
				cpu->iq.entries[id].src2_ready = VALID;
				cpu->iq.entries[id].src2_value = cpu->execute_mu.latch.data;
			}
			else if(cpu->execute_load_store.latch.reg_id == reg && cpu->execute_load_store.latch.ready == VALID)
			{
				cpu->iq.entries[id].src2_ready = VALID;
				cpu->iq.entries[id].src2_value = cpu->execute_load_store.latch.data;
			}
			else
			{
				cpu->iq.entries[id].src2_ready = INVALID;
				cpu->phys_regs[cpu->rb.entries[reg].phy_address].wbv.BYTE |=  1<<(id);
			}
		}
	}
}

/* Function to update the alloted issue queue entry
 *
 */
static void
update_issue_queue_entry(int id, int lid, int rob_id, APEX_CPU *cpu)
{
	int p1 = cpu->rename2_dispatch.p1;
	int p2 = cpu->rename2_dispatch.p2;
	cpu->iq.entries[id].al = ALLOCATED;
	cpu->iq.size++;
	cpu->iq.entries[id].cycle = cpu->clock;
	cpu->iq.entries[id].pc = cpu->rename2_dispatch.pc;
	cpu->iq.entries[id].dest = cpu->rename2_dispatch.pd;
	cpu->iq.entries[id].fu_type = cpu->rename2_dispatch.opcode;
	cpu->iq.entries[id].literal = cpu->rename2_dispatch.imm;
	cpu->iq.entries[id].src1_tag = p1;
	cpu->iq.entries[id].src2_tag = p2;
	cpu->iq.entries[id].lsq_id = lid;
	cpu->iq.entries[id].rob_id = rob_id;
	cpu->iq.entries[id].btb_id = cpu->rename2_dispatch.btb_id;
	cpu->iq.entries[id].rd = cpu->rename2_dispatch.rd;
	memcpy(cpu->iq.entries[id].opcode_str, cpu->rename2_dispatch.opcode_str, 128);
	switch(cpu->rename2_dispatch.opcode)
	{
		case OPCODE_ADD:
		case OPCODE_SUB:
		case OPCODE_MUL:
		case OPCODE_DIV:
		case OPCODE_AND:
		case OPCODE_OR:
		case OPCODE_XOR:
		case OPCODE_CMP:
		case OPCODE_STORE:
		{
			update_src1(p1, id, cpu);
			update_src2(p2, id, cpu);
			break;
		}

        case OPCODE_LOAD:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JUMP:
        case OPCODE_JALR:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
        	update_src1(p1, id, cpu);
        	cpu->iq.entries[id].src2_ready = VALID;
        	cpu->iq.entries[id].src2_tag = -1;
        	cpu->iq.entries[id].src2_value = 0;
        	if(cpu->rename2_dispatch.opcode == OPCODE_BZ)
        	{
        		//printf("\nBZ before dispatch: src1_ready = %d, zflag = %d, pflag = %d\n", cpu->iq.entries[id].src1_ready, cpu->iq.entries[id].z_flag, cpu->iq.entries[id].p_flag);
        	}
            break;
        }

        case OPCODE_MOVC:
        case OPCODE_HALT:
        case OPCODE_NOP:
        {
        	cpu->iq.entries[id].src1_ready = VALID;
        	cpu->iq.entries[id].src1_tag = -1;
        	cpu->iq.entries[id].src1_value = 0;
        	cpu->iq.entries[id].src2_ready = VALID;
        	cpu->iq.entries[id].src2_tag = -1;
        	cpu->iq.entries[id].src2_value = 0;
            break;
        }

        case OPCODE_RET:
        {
        	cpu->iq.entries[id].src1_ready = VALID;
        	cpu->iq.entries[id].src1_tag = cpu->rename2_dispatch.p1;
        	cpu->iq.entries[id].src1_value = cpu->rename2_dispatch.p1_value;
        	cpu->iq.entries[id].src2_ready = VALID;
        	cpu->iq.entries[id].src2_tag = -1;
        	cpu->iq.entries[id].src2_value = 0;
        	break;
        }
	}
}

/* Function to update the alloted load store queue entry
 *
 */
static void
update_load_store_queue_entry(int lid, int rob_id, APEX_CPU *cpu)
{
	cpu->lsq.entries[lid].al = ALLOCATED;
	cpu->lsq.entries[lid].cycle = cpu->clock;
	cpu->lsq.entries[lid].pc = cpu->rename2_dispatch.pc;
	cpu->lsq.entries[lid].dest = cpu->rename2_dispatch.pd;
	cpu->lsq.entries[lid].src1_tag = cpu->rename2_dispatch.p1;
	cpu->lsq.entries[lid].data_ready = INVALID;
	cpu->lsq.entries[lid].mem_valid = INVALID;
	cpu->lsq.entries[lid].rob_id = rob_id;
	cpu->lsq.entries[lid].rd = cpu->rename2_dispatch.rd;
	memcpy(cpu->lsq.entries[lid].opcode_str, cpu->rename2_dispatch.opcode_str, 128);
	if(cpu->rename2_dispatch.opcode == OPCODE_LOAD)
	{
		cpu->lsq.entries[lid].ls_bit = 0;
	}
	else
	{
		cpu->lsq.entries[lid].ls_bit = 1;
		cpu->lsq.entries[lid].src1_src = cpu->rename_table[cpu->rename2_dispatch.rs1].src_bit;
		if(cpu->rename_table[cpu->rename2_dispatch.rs1].src_bit == 0)
		{
			cpu->lsq.entries[lid].data_ready = VALID;
			cpu->lsq.entries[lid].src1_value = cpu->arch_regs[cpu->rename2_dispatch.rs2].value;
		}
		else
		{
			if(cpu->phys_regs[cpu->rb.entries[cpu->lsq.entries[lid].src1_tag].phy_address].status == VALID)
			{
				cpu->lsq.entries[lid].data_ready = VALID;
				cpu->lsq.entries[lid].src1_value = cpu->phys_regs[cpu->rb.entries[cpu->lsq.entries[lid].src1_tag].phy_address].value;
			}
			else
			{
				if(cpu->execute_iu.latch.reg_id == cpu->lsq.entries[lid].src1_tag && cpu->execute_iu.latch.ready == VALID)
				{
					cpu->lsq.entries[lid].data_ready = VALID;
					cpu->lsq.entries[lid].src1_value = cpu->execute_iu.latch.data;
				}
				else if(cpu->execute_mu.latch.reg_id == cpu->lsq.entries[lid].src1_tag && cpu->execute_mu.latch.ready == VALID)
				{
					cpu->lsq.entries[lid].data_ready = VALID;
					cpu->lsq.entries[lid].src1_value = cpu->execute_mu.latch.data;
				}
				else if(cpu->execute_load_store.latch.reg_id == cpu->lsq.entries[lid].src1_tag && cpu->execute_load_store.latch.ready == VALID)
				{
					cpu->lsq.entries[lid].data_ready = VALID;
					cpu->lsq.entries[lid].src1_value = cpu->execute_load_store.latch.data;
				}
				else
				{
					cpu->lsq.entries[lid].data_ready = INVALID;
				}
			}
		}
	}
}

/* Function to update the alloted rob entry
 *
 */
static void
update_rob_entry(int rob_id, APEX_CPU *cpu)
{
	cpu->rb.entries[rob_id].status = INVALID;
	cpu->rb.entries[rob_id].phy_address = cpu->rename2_dispatch.pd;
	cpu->rb.entries[rob_id].arch_address = cpu->rename2_dispatch.rd;
	cpu->rb.entries[rob_id].excodes = -1;
	cpu->rb.entries[rob_id].itype = cpu->rename2_dispatch.opcode;
	cpu->rb.entries[rob_id].pc = cpu->rename2_dispatch.pc;
	memcpy(cpu->rb.entries[rob_id].opcode_str, cpu->rename2_dispatch.opcode_str, 128);
	cpu->rb.entries[rob_id].cycle = cpu->clock;
	cpu->rb.entries[rob_id].sval_valid = INVALID;
	cpu->rb.entries[rob_id].svalue = -1;
	cpu->rb.entries[rob_id].result = -1;

	switch(cpu->rename2_dispatch.opcode)
	{
		case OPCODE_ADD:
		case OPCODE_SUB:
		case OPCODE_MUL:
		case OPCODE_DIV:
		case OPCODE_AND:
		case OPCODE_OR:
		case OPCODE_XOR:
		case OPCODE_CMP:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
		{
			if(cpu->rename_table[cpu->rename2_dispatch.rd].src_bit == 1)
			{
				cpu->phys_regs[cpu->rb.entries[cpu->rename_table[cpu->rename2_dispatch.rd].slot_id].phy_address].renamed = RENAMED;
			}
			cpu->rename_table[cpu->rename2_dispatch.rd].src_bit = 1;
			cpu->rename_table[cpu->rename2_dispatch.rd].slot_id = rob_id;
			cpu->arch_regs[REG_FILE_SIZE - 1].value = 0;
			cpu->arch_regs[REG_FILE_SIZE - 1].p_flag = 0;
			cpu->arch_regs[REG_FILE_SIZE - 1].z_flag = 0;
			cpu->rename_table[REG_FILE_SIZE - 1].src_bit = 1;
			cpu->rename_table[REG_FILE_SIZE - 1].slot_id = rob_id;
			break;
		}

		case OPCODE_LOAD:
        case OPCODE_JALR:
        case OPCODE_MOVC:
		{
			if(cpu->rename_table[cpu->rename2_dispatch.rd].src_bit == 1)
			{
				cpu->phys_regs[cpu->rb.entries[cpu->rename_table[cpu->rename2_dispatch.rd].slot_id].phy_address].renamed = RENAMED;
			}
			cpu->rename_table[cpu->rename2_dispatch.rd].src_bit = 1;
			cpu->rename_table[cpu->rename2_dispatch.rd].slot_id = rob_id;
			break;
		}

		case OPCODE_STORE:
        case OPCODE_JUMP:
        case OPCODE_RET:
        case OPCODE_HALT:
        case OPCODE_NOP:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BZ:
        case OPCODE_BNZ:
		{
			cpu->rb.entries[rob_id].phy_address = -1;
			cpu->rb.entries[rob_id].arch_address = -1;
			break;
		}
	}
}

/* Function to retire rob entries from the head
 *
 */
static int rob_retirement_logic(APEX_CPU *cpu)
{
	if(cpu->rb.size > 0 && cpu->rb.entries[cpu->rb.front].excodes == -1)
	{
		if(cpu->rb.entries[cpu->rb.front].itype == OPCODE_HALT)
		{
			if(cpu->execute_iu.has_insn == 0 && cpu->execute_mu.has_insn == 0 && cpu->execute_bu.has_insn == 0 && cpu->execute_load_store.has_insn == 0 &&
			   cpu->iq.size == 0 && cpu->lsq.size == 0)
			{
				cpu->rb.front = (cpu->rb.front + 1) % ROB_SIZE;
				cpu->rb.size--;
				return 1;
			}
		}
		else if(cpu->rb.entries[cpu->rb.front].status == VALID &&
		  (cpu->rb.entries[cpu->rb.front].itype == OPCODE_ADD  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_SUB  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_MUL  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_DIV  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_AND  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_OR   ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_XOR  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_MOVC ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_LOAD ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_ADDL ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_SUBL ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_CMP  ||
		   cpu->rb.entries[cpu->rb.front].itype == OPCODE_JALR))
		{
			if(cpu->phys_regs[cpu->rb.entries[cpu->rb.front].phy_address].renamed != RENAMED)
			{
				cpu->arch_regs[cpu->rb.entries[cpu->rb.front].arch_address].value = cpu->phys_regs[cpu->rb.entries[cpu->rb.front].phy_address].value;
				cpu->arch_regs[cpu->rb.entries[cpu->rb.front].arch_address].z_flag = cpu->phys_regs[cpu->rb.entries[cpu->rb.front].phy_address].z_flag;
				cpu->arch_regs[cpu->rb.entries[cpu->rb.front].arch_address].p_flag = cpu->phys_regs[cpu->rb.entries[cpu->rb.front].phy_address].p_flag;
				cpu->rename_table[cpu->rb.entries[cpu->rb.front].arch_address].src_bit = 0;
				cpu->rename_table[cpu->rb.entries[cpu->rb.front].arch_address].slot_id = cpu->rb.entries[cpu->rb.front].arch_address;

				if(cpu->rb.entries[cpu->rb.front].itype == OPCODE_ADD  ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_SUB  ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_MUL  ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_DIV  ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_AND  ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_OR   ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_XOR  ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_ADDL ||
				   cpu->rb.entries[cpu->rb.front].itype == OPCODE_SUBL)
				{
					cpu->arch_regs[REG_FILE_SIZE - 1] = cpu->arch_regs[cpu->rb.entries[cpu->rb.front].arch_address];
					cpu->rename_table[REG_FILE_SIZE - 1].src_bit = 0;
					cpu->rename_table[REG_FILE_SIZE - 1].slot_id = REG_FILE_SIZE - 1;
				}
			}
			cpu->rb.front = (cpu->rb.front + 1) % ROB_SIZE;
			cpu->rb.size--;
		}
		else if(cpu->rb.entries[cpu->rb.front].status == VALID)
		{
			cpu->rb.front = (cpu->rb.front + 1) % ROB_SIZE;
			cpu->rb.size--;
		}
	}
	return 0;
}

/* Function to flush the instruction during misprediction or
 * first time resolution of the branch
 *
 */
static void flush_instructions_after_branch(APEX_CPU *cpu, int clock, int target)
{
	cpu->pc = target;
	cpu->decode_rename1.has_insn = FALSE;
	switch(cpu->rename2_dispatch.opcode)
	{
		case OPCODE_ADD:
		case OPCODE_SUB:
		case OPCODE_MUL:
		case OPCODE_DIV:
		case OPCODE_AND:
		case OPCODE_OR:
		case OPCODE_XOR:
		case OPCODE_CMP:
	    case OPCODE_ADDL:
	    case OPCODE_SUBL:
	    case OPCODE_LOAD:
	    case OPCODE_MOVC:
		{
			cpu->phys_regs[cpu->rename2_dispatch.pd].al = UN_ALLOCATED;
			cpu->phys_regs[cpu->rename2_dispatch.pd].renamed = NOT_RENAMED;
			cpu->phys_regs[cpu->rename2_dispatch.pd].status = INVALID;
			cpu->phys_regs[cpu->rename2_dispatch.pd].value = 0;
			cpu->phys_regs[cpu->rename2_dispatch.pd].p_flag = 0;
			cpu->phys_regs[cpu->rename2_dispatch.pd].z_flag = 0;
			cpu->phys_regs[cpu->rename2_dispatch.pd].wbv.BYTE = 0;
			break;
		}

		case OPCODE_JALR:
		{
			cpu->phys_regs[cpu->rename2_dispatch.pd].al = UN_ALLOCATED;
			cpu->phys_regs[cpu->rename2_dispatch.pd].renamed = NOT_RENAMED;
			cpu->phys_regs[cpu->rename2_dispatch.pd].status = INVALID;
			cpu->phys_regs[cpu->rename2_dispatch.pd].value = 0;
			cpu->phys_regs[cpu->rename2_dispatch.pd].p_flag = 0;
			cpu->phys_regs[cpu->rename2_dispatch.pd].z_flag = 0;
			cpu->phys_regs[cpu->rename2_dispatch.pd].wbv.BYTE = 0;
			cpu->btb.entries[cpu->rename2_dispatch.btb_id].al = UN_ALLOCATED;
			break;
		}

		case OPCODE_BZ:
		case OPCODE_BNZ:
		case OPCODE_BP:
		case OPCODE_BNP:
		case OPCODE_JUMP:
		{
			cpu->btb.entries[cpu->rename2_dispatch.btb_id].al = UN_ALLOCATED;
			break;
		}

		case OPCODE_RET:
		{
			// Do nothing
			break;
		}
	}
	cpu->rename2_dispatch.has_insn = FALSE;
	cpu->stall = 0;
	cpu->d_stall = 0;
	prev_stage = 0;

	for(int i = 0; i < IQ_SIZE; i++)
	{
		if(cpu->iq.entries[i].al == ALLOCATED && cpu->iq.entries[i].cycle <= cpu->clock && cpu->iq.entries[i].cycle > clock)
		{
			switch(cpu->iq.entries[i].fu_type)
			{
				case OPCODE_ADD:
				case OPCODE_SUB:
				case OPCODE_MUL:
				case OPCODE_DIV:
				case OPCODE_AND:
				case OPCODE_OR:
				case OPCODE_XOR:
				case OPCODE_CMP:
			    case OPCODE_ADDL:
			    case OPCODE_SUBL:
			    case OPCODE_MOVC:
				{
					cpu->phys_regs[cpu->iq.entries[i].dest].al = UN_ALLOCATED;
					cpu->phys_regs[cpu->iq.entries[i].dest].renamed = NOT_RENAMED;
					cpu->phys_regs[cpu->iq.entries[i].dest].status = INVALID;
					cpu->phys_regs[cpu->iq.entries[i].dest].value = 0;
					cpu->phys_regs[cpu->iq.entries[i].dest].p_flag = 0;
					cpu->phys_regs[cpu->iq.entries[i].dest].z_flag = 0;
					cpu->phys_regs[cpu->iq.entries[i].dest].wbv.BYTE = 0;
					cpu->rename_table[cpu->iq.entries[i].rd].src_bit = cpu->backup_rename_table[cpu->iq.entries[i].rd].src_bit;
					cpu->rename_table[cpu->iq.entries[i].rd].slot_id = cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id;
					if(cpu->backup_rename_table[cpu->iq.entries[i].rd].src_bit == 1)
					{
						cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id].phy_address] = cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id].phy_address];
					}
					else
					{
						cpu->arch_regs[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id];
					}

					break;
				}

				case OPCODE_JALR:
				{
					cpu->phys_regs[cpu->iq.entries[i].dest].al = UN_ALLOCATED;
					cpu->phys_regs[cpu->iq.entries[i].dest].renamed = NOT_RENAMED;
					cpu->phys_regs[cpu->iq.entries[i].dest].status = INVALID;
					cpu->phys_regs[cpu->iq.entries[i].dest].value = 0;
					cpu->phys_regs[cpu->iq.entries[i].dest].p_flag = 0;
					cpu->phys_regs[cpu->iq.entries[i].dest].z_flag = 0;
					cpu->phys_regs[cpu->iq.entries[i].dest].wbv.BYTE = 0;

					cpu->rename_table[cpu->iq.entries[i].rd].src_bit = cpu->backup_rename_table[cpu->iq.entries[i].rd].src_bit;
					cpu->rename_table[cpu->iq.entries[i].rd].slot_id = cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id;
					if(cpu->backup_rename_table[cpu->iq.entries[i].rd].src_bit == 1)
					{
						cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id].phy_address] = cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id].phy_address];
					}
					else
					{
						cpu->arch_regs[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->iq.entries[i].rd].slot_id];
					}
					cpu->btb.entries[cpu->iq.entries[i].btb_id].al = UN_ALLOCATED;
					break;
				}

				case OPCODE_BZ:
				case OPCODE_BNZ:
				case OPCODE_BP:
				case OPCODE_BNP:
				case OPCODE_JUMP:
				{
					cpu->btb.entries[cpu->iq.entries[i].btb_id].al = UN_ALLOCATED;
					break;
				}

				case OPCODE_RET:
				{
					// Do nothing
					break;
				}
			}
			cpu->iq.entries[i].al = UN_ALLOCATED;
			cpu->iq.size--;
		}
	}

	for(int i = 0; i < LSQ_SIZE; i++)
	{
		if(cpu->lsq.entries[i].al == ALLOCATED && cpu->lsq.entries[i].cycle <= cpu->clock && cpu->lsq.entries[i].cycle > clock)
		{
			cpu->lsq.entries[i].al = UN_ALLOCATED;
			cpu->lsq.size--;
		}
	}

	if(cpu->lsq.size > 0)
	{
		if(cpu->lsq.rear >= cpu->lsq.front)
		{
	    	while(cpu->lsq.size > 0 && (cpu->lsq.entries[cpu->lsq.rear].cycle > clock && cpu->lsq.entries[cpu->lsq.rear].cycle <= cpu->clock))
	    	{
	    		cpu->lsq.entries[cpu->lsq.rear].al = UN_ALLOCATED;
	    		if(cpu->lsq.entries[cpu->lsq.rear].ls_bit == 0)
	    		{
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].al = UN_ALLOCATED;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].renamed = NOT_RENAMED;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].status = INVALID;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].value = 0;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].p_flag = 0;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].z_flag = 0;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].wbv.BYTE = 0;

					cpu->rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].src_bit = cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].src_bit;
					cpu->rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id = cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id;
					if(cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].src_bit == 1)
					{
						cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id].phy_address] =
								cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id].phy_address];
					}
					else
					{
						cpu->arch_regs[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id];
					}
	    		}
	    		cpu->lsq.rear--;
	    		cpu->lsq.size--;
	    	}
		}
		else
		{
	    	while(cpu->lsq.size > 0 && (cpu->lsq.entries[cpu->lsq.rear].cycle > clock && cpu->lsq.entries[cpu->lsq.rear].cycle <= cpu->clock))
	    	{
	    		cpu->lsq.entries[cpu->lsq.rear].al = UN_ALLOCATED;
	    		if(cpu->lsq.entries[cpu->lsq.rear].ls_bit == 0)
	    		{
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].al = UN_ALLOCATED;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].renamed = NOT_RENAMED;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].status = INVALID;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].value = 0;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].p_flag = 0;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].z_flag = 0;
					cpu->phys_regs[cpu->lsq.entries[cpu->lsq.rear].dest].wbv.BYTE = 0;

					cpu->rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].src_bit = cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].src_bit;
					cpu->rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id = cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id;
					if(cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].src_bit == 1)
					{
						cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id].phy_address] =
								cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id].phy_address];
					}
					else
					{
						cpu->arch_regs[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->lsq.entries[cpu->lsq.rear].rd].slot_id];
					}
	    		}
	    		if(cpu->lsq.rear > 0)
	    		{
		    		cpu->lsq.rear--;
		    		cpu->lsq.size--;
	    		}
	    		else
	    		{
	    			cpu->lsq.rear = LSQ_SIZE - 1;
		    		cpu->lsq.size--;
	    		}
	    	}
		}
	}


	if(cpu->rb.size > 0)
	{
	    if(cpu->rb.rear >= cpu->rb.front)
	    {
	    	while(cpu->rb.size > 0 && (cpu->rb.entries[cpu->rb.rear].cycle > clock && cpu->rb.entries[cpu->rb.rear].cycle <= cpu->clock))
	    	{
	    		printf("ROB entry deleted from rear = %d\n", cpu->rb.rear);
	    		cpu->rb.rear--;
	    		cpu->rb.size--;
	    	}
	    }
	    else
	    {
	    	while(cpu->rb.size > 0 && (cpu->rb.entries[cpu->rb.rear].cycle > clock && cpu->rb.entries[cpu->rb.rear].cycle <= cpu->clock))
	    	{
	    		if(cpu->rb.rear > 0)
	    		{
	    			printf("ROB entry deleted from rear = %d\n", cpu->rb.rear);
		    		cpu->rb.rear--;
		    		cpu->rb.size--;
	    		}
	    		else
	    		{
	    			printf("ROB entry deleted from rear = %d\n", cpu->rb.rear);
	    			cpu->rb.rear = ROB_SIZE - 1;
		    		cpu->rb.size--;
	    		}
	    	}
	    }
	}

	if(cpu->execute_iu.has_insn == 1 && cpu->execute_iu.iq_entry.cycle > clock && cpu->execute_iu.iq_entry.cycle <= cpu->clock)
	{
		if(cpu->execute_iu.iq_entry.fu_type == OPCODE_ADD   ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_SUB   ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_AND   ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_OR    ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_XOR   ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_CMP   ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_ADDL  ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_SUBL  ||
		   cpu->execute_iu.iq_entry.fu_type == OPCODE_MOVC)
		   {
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].al = UN_ALLOCATED;
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].renamed = NOT_RENAMED;
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].status = INVALID;
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].value = 0;
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].p_flag = 0;
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].z_flag = 0;
				cpu->phys_regs[cpu->execute_iu.iq_entry.dest].wbv.BYTE = 0;

				cpu->rename_table[cpu->execute_iu.iq_entry.rd].src_bit = cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].src_bit;
				cpu->rename_table[cpu->execute_iu.iq_entry.rd].slot_id = cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].slot_id;
				if(cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].src_bit == 1)
				{
					cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].slot_id].phy_address] =
							cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].slot_id].phy_address];
				}
				else
				{
					cpu->arch_regs[cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->execute_iu.iq_entry.rd].slot_id];
				}
		   }
		   cpu->execute_iu.has_insn = 0;
	}

	if(cpu->writeback_iu.has_insn == 1 && cpu->writeback_iu.iq_entry.cycle > clock && cpu->writeback_iu.iq_entry.cycle <= cpu->clock)
	{
		if(cpu->writeback_iu.iq_entry.fu_type == OPCODE_ADD   ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_SUB   ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_AND   ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_OR    ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_XOR   ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_CMP   ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_ADDL  ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_SUBL  ||
		   cpu->writeback_iu.iq_entry.fu_type == OPCODE_MOVC)
		   {
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].al = UN_ALLOCATED;
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].renamed = NOT_RENAMED;
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].status = INVALID;
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].value = 0;
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].p_flag = 0;
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].z_flag = 0;
				cpu->phys_regs[cpu->writeback_iu.iq_entry.dest].wbv.BYTE = 0;

				cpu->rename_table[cpu->writeback_iu.iq_entry.rd].src_bit = cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].src_bit;
				cpu->rename_table[cpu->writeback_iu.iq_entry.rd].slot_id = cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].slot_id;
				if(cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].src_bit == 1)
				{
					cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].slot_id].phy_address] =
							cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].slot_id].phy_address];
				}
				else
				{
					cpu->arch_regs[cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->writeback_iu.iq_entry.rd].slot_id];
				}
		   }
		   cpu->writeback_iu.has_insn = 0;
	}

	if(cpu->execute_mu.has_insn == 1 && cpu->execute_mu.iq_entry.cycle > clock && cpu->execute_mu.iq_entry.cycle <= cpu->clock)
	{
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].al = UN_ALLOCATED;
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].renamed = NOT_RENAMED;
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].status = INVALID;
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].value = 0;
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].p_flag = 0;
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].z_flag = 0;
		cpu->phys_regs[cpu->execute_mu.iq_entry.dest].wbv.BYTE = 0;

		cpu->rename_table[cpu->execute_mu.iq_entry.rd].src_bit = cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].src_bit;
		cpu->rename_table[cpu->execute_mu.iq_entry.rd].slot_id = cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].slot_id;
		if(cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].src_bit == 1)
		{
			cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].slot_id].phy_address] = cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].slot_id].phy_address];
		}
		else
		{
			cpu->arch_regs[cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->execute_mu.iq_entry.rd].slot_id];
		}
		cpu->execute_mu.has_insn = 0;
	}

	if(cpu->writeback_mu.has_insn == 1 && cpu->writeback_mu.iq_entry.cycle > clock && cpu->writeback_mu.iq_entry.cycle <= cpu->clock)
	{
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].al = UN_ALLOCATED;
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].renamed = NOT_RENAMED;
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].status = INVALID;
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].value = 0;
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].p_flag = 0;
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].z_flag = 0;
		cpu->phys_regs[cpu->writeback_mu.iq_entry.dest].wbv.BYTE = 0;

		cpu->rename_table[cpu->writeback_mu.iq_entry.rd].src_bit = cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].src_bit;
		cpu->rename_table[cpu->writeback_mu.iq_entry.rd].slot_id = cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].slot_id;
		if(cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].src_bit == 1)
		{
			cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].slot_id].phy_address] =
					cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].slot_id].phy_address];
		}
		else
		{
			cpu->arch_regs[cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->writeback_mu.iq_entry.rd].slot_id];
		}
		cpu->writeback_mu.has_insn = 0;
	}

	if(cpu->execute_load_store.has_insn == 1 && cpu->execute_load_store.lsq_entry.cycle > clock
	   && cpu->execute_load_store.lsq_entry.cycle <= cpu->clock)
	{
		if(cpu->execute_load_store.lsq_entry.ls_bit == 0)
		{
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].al = UN_ALLOCATED;
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].renamed = NOT_RENAMED;
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].status = INVALID;
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].value = 0;
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].p_flag = 0;
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].z_flag = 0;
			cpu->phys_regs[cpu->execute_load_store.lsq_entry.dest].wbv.BYTE = 0;

			cpu->rename_table[cpu->execute_load_store.lsq_entry.rd].src_bit = cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].src_bit;
			cpu->rename_table[cpu->execute_load_store.lsq_entry.rd].slot_id = cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].slot_id;
			if(cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].src_bit == 1)
			{
				cpu->phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].slot_id].phy_address] =
						cpu->backup_phys_regs[cpu->rb.entries[cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].slot_id].phy_address];
			}
			else
			{
				cpu->arch_regs[cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].slot_id] = cpu->backup_arch_regs[cpu->backup_rename_table[cpu->execute_load_store.lsq_entry.rd].slot_id];
			}
		}

		cpu->execute_load_store.has_insn = 0;
	}

	if(cpu->execute_bu.has_insn == 1 && cpu->execute_bu.iq_entry.cycle > clock && cpu->execute_bu.iq_entry.cycle <= cpu->clock)
	{
		cpu->execute_bu.has_insn = 0;
	}
}

/* Function to check the dependency of register to register
 * instructions
 *
 */
static void
check_r_to_r_instruction_dependency(APEX_CPU *cpu)
{
	int id, rob_id;
	id = get_free_issue_queue_entry(cpu);
	rob_id = get_free_rob_entry(cpu);
	if(id == -1 || rob_id == -1)
	{
		if(cpu->rename2_dispatch.opcode == OPCODE_HALT)
		{
			printf("Issue queue is full at HALT\n");
		}
		cpu->d_stall = 1;
	}
	else
	{
		update_issue_queue_entry(id, -1, rob_id, cpu);
		update_rob_entry(rob_id, cpu);
		cpu->d_stall = 0;
	}
}

/* Function to check the dependency of LOAD STORE instructions
 *
 */
static void
check_load_store_instruction_dependency(APEX_CPU *cpu)
{
	int id, lid, rob_id;
	id = get_free_issue_queue_entry(cpu);
	lid = get_free_load_store_queue_entry(cpu);
	rob_id = get_free_rob_entry(cpu);
	if(id == -1 || lid == -1 || rob_id == -1)
	{
		cpu->d_stall = 1;
	}
	else
	{
		update_issue_queue_entry(id, lid, rob_id, cpu);
		update_load_store_queue_entry(lid, rob_id, cpu);
		update_rob_entry(rob_id, cpu);
		cpu->d_stall = 0;
	}
}

/* Function to get the free btb entry
 *
 */
static int get_free_btb_entry(APEX_CPU *cpu)
{
	int id = -1;
	for(int i = 0; i < BTB_SIZE; i++)
	{
		if(cpu->btb.entries[i].al == UN_ALLOCATED)
		{
			id = i;
			cpu->btb.entries[i].al = ALLOCATED;
			cpu->btb.entries[i].history = -1;
			cpu->btb.entries[i].prediction = 1;
			cpu->btb.entries[i].resolved = 0;
			cpu->btb.entries[i].tag = cpu->decode_rename1.pc;
			cpu->btb.entries[i].type = cpu->decode_rename1.opcode;
			cpu->btb.entries[i].target = -1;
			break;
		}
	}
	return id;
}

/* Function to check for any unresolved branch instruction
 * in the pipeline
 *
 */
static int check_unresolved_btb_entry(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->execute_bu.has_insn == 1)
	{
		return cpu->execute_bu.iq_entry.btb_id;
	}

	if(cpu->writeback_bu.has_insn == 1)
	{
		return cpu->writeback_bu.iq_entry.btb_id;
	}

	for(int i = 0; i < IQ_SIZE; i++)
	{
		if(cpu->iq.entries[i].al == ALLOCATED &&
		  (cpu->iq.entries[i].fu_type == OPCODE_JUMP ||
		   cpu->iq.entries[i].fu_type == OPCODE_JALR ||
		   cpu->iq.entries[i].fu_type == OPCODE_BP ||
		   cpu->iq.entries[i].fu_type == OPCODE_BNP ||
		   cpu->iq.entries[i].fu_type == OPCODE_BZ ||
		   cpu->iq.entries[i].fu_type == OPCODE_BNZ))
		{
			id = i;
			break;
		}
	}
	return id;
}

/* Function to check for btb entries in parallel with fetch
 *
 */
static int check_btb_entries(APEX_CPU *cpu, int pc)
{
	int id = -1;
	for(int i = 0; i < BTB_SIZE; i++)
	{
		if(cpu->btb.entries[i].al == ALLOCATED && cpu->btb.entries[i].tag == pc)
		{
			id = i;
			break;
		}
	}
	return id;
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;
    int id;
    if(cpu->stall)
    {
        if(!prev_stage)
        {
            /* Store current PC in fetch latch */

            cpu->fetch.pc = cpu->pc;

            /* Index into code memory using this pc and copy all instruction fields
            * into fetch latch  */
            current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
            strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
            cpu->fetch.opcode = current_ins->opcode;
            cpu->fetch.rd = current_ins->rd;
            cpu->fetch.rs1 = current_ins->rs1;
            cpu->fetch.rs2 = current_ins->rs2;
            cpu->fetch.imm = current_ins->imm;
            cpu->fetch.btb_id = -1;
            printf("Fetch at address = %d\n", cpu->pc);
            /* Update PC for next instruction */
            cpu->pc += INSTRUCTION_SIZE;

            id = check_btb_entries(cpu, cpu->pc);
    		cpu->fetch.btb_id = id;
        	if(id >= 0)
        	{
        		if(cpu->btb.entries[id].resolved == 1)
        		{
        			if(cpu->btb.entries[id].history == 1)
        			{
            			cpu->pc = cpu->btb.entries[id].target;
            			cpu->btb.entries[id].prediction = 1;
            			cpu->btb.entries[id].resolved = 0;
            			cpu->fetch_from_next_cycle = TRUE;
        			}
        			else
        			{
        				cpu->btb.entries[id].prediction = 0;
            			cpu->btb.entries[id].resolved = 0;
        			}
        		}
        	}
            /* Copy data from fetch latch to debug fetch latch*/
            cpu->debug_fetch = cpu->fetch;
            
            /* Stop fetching new instructions if HALT is fetched */
            if (cpu->fetch.opcode == OPCODE_HALT)
            {
                last_halt = TRUE;
            }
        }
        else
        {
            /* Copy data from fetch latch to debug fetch latch*/
        	cpu->debug_fetch = cpu->fetch;
        }
    }
    else
    {
        if(prev_stage)
        {
            /* Copy data from fetch latch to decode latch*/
            cpu->decode_rename1 = cpu->fetch;
            /* Copy data from fetch latch to debug fetch latch*/
            cpu->debug_fetch = cpu->fetch;
            if(last_halt)
            {
                last_halt = FALSE;
                cpu->fetch.has_insn = FALSE;
                return;
            }
        }
        else
        {
            if (cpu->fetch.has_insn)
            {
                /* This fetches new branch target instruction from next cycle */
                if (cpu->fetch_from_next_cycle == TRUE)
                {
                    cpu->fetch_from_next_cycle = FALSE;
                    cpu->debug_fetch.has_insn = FALSE;
                    /* Skip this cycle*/
                    return;
                }

                /* Store current PC in fetch latch */
                cpu->fetch.pc = cpu->pc;

                /* Index into code memory using this pc and copy all instruction fields
                * into fetch latch  */
                current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
                strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
                cpu->fetch.opcode = current_ins->opcode;
                cpu->fetch.rd = current_ins->rd;
                cpu->fetch.rs1 = current_ins->rs1;
                cpu->fetch.rs2 = current_ins->rs2;
                cpu->fetch.imm = current_ins->imm;
                cpu->fetch.btb_id = -1;
                printf("Fetch at address = %d\n", cpu->pc);
                cpu->pc += INSTRUCTION_SIZE;

                id = check_btb_entries(cpu, cpu->pc);
        		cpu->fetch.btb_id = id;
            	if(id >= 0)
            	{
            		printf("BTB Hit\n");
            		if(cpu->btb.entries[id].resolved == 1)
            		{
            			if(cpu->btb.entries[id].history == 1)
            			{
                			cpu->pc = cpu->btb.entries[id].target;
                			cpu->btb.entries[id].prediction = 1;
                			cpu->btb.entries[id].resolved = 0;
                			cpu->fetch_from_next_cycle = TRUE;
            			}
            			else
            			{
            				cpu->btb.entries[id].prediction = 0;
                			cpu->btb.entries[id].resolved = 0;
            			}
            		}
            	}

                /* Copy data from fetch latch to decode latch*/
                cpu->decode_rename1 = cpu->fetch;

                /* Copy data from fetch latch to debug fetch latch*/
                cpu->debug_fetch = cpu->fetch;

                /* Stop fetching new instructions if HALT is fetched */
                if (cpu->fetch.opcode == OPCODE_HALT)
                {
                    cpu->fetch.has_insn = FALSE;
                }
            }
            else
            {
            	cpu->debug_fetch = cpu->fetch;
            }
        }
    }
    prev_stage = cpu->stall;
}

/*
 * Decode/Rename1 Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode_rename1(APEX_CPU *cpu)
{
	int reg = -1;
	if(cpu->d_stall == 1)
	{
		cpu->stall = 1;
        /* Copy data from decode latch to debug decode latch*/
        cpu->debug_decode_rename1 = cpu->decode_rename1;
        return;
	}
	else
	{
		cpu->stall = 0;
	}
    if (cpu->decode_rename1.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode_rename1.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_DIV:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
            	reg = get_free_physical_register(cpu);
            	if(reg >= 0)
            	{
            		cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
                	cpu->decode_rename1.p2 = cpu->rename_table[cpu->decode_rename1.rs2].slot_id;
                	cpu->decode_rename1.pd = reg;
                	allocate_physical_register(cpu, reg);
                	cpu->stall = 0;
            	}
            	else
            	{
            		cpu->stall = 1;
            	}
                break;
            }

			case OPCODE_CMP:
            {
            	reg = get_free_hidden_physical_register(cpu);
            	if(reg >= 0)
            	{
            		cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
                	cpu->decode_rename1.p2 = cpu->rename_table[cpu->decode_rename1.rs2].slot_id;
                	cpu->decode_rename1.pd = reg;
                	cpu->stall = 0;
            	}
            	else
            	{
            		cpu->stall = 1;
            	}
                break;
            }

			case OPCODE_STORE:
            {
            	cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
            	cpu->decode_rename1.p2 = cpu->rename_table[cpu->decode_rename1.rs2].slot_id;
            	cpu->stall = 0;
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_LOAD:
            {
            	reg = get_free_physical_register(cpu);
            	if(reg >= 0)
            	{
                	cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
                	cpu->decode_rename1.pd = reg;
                	allocate_physical_register(cpu, reg);
                	cpu->stall = 0;
            	}
            	else
            	{
            		cpu->stall = 1;
            	}
                break;
            }

            case OPCODE_MOVC:
            {
            	reg = get_free_physical_register(cpu);
            	if(reg >= 0)
            	{
                	cpu->decode_rename1.pd = reg;
                	allocate_physical_register(cpu, reg);
                	cpu->stall = 0;
            	}
            	else
            	{
            		cpu->stall = 1;
            	}
                break;
            }

            case OPCODE_JALR:
            {
            	reg = get_free_physical_register(cpu);
            	if(reg >= 0)
            	{
                	cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
                	cpu->decode_rename1.pd = reg;
                	allocate_physical_register(cpu, reg);
                	cpu->stall = 0;

                	if(cpu->decode_rename1.btb_id < 0)
                	{
                		cpu->decode_rename1.btb_id = get_free_btb_entry(cpu);
                	}
            	}
            	else
            	{
            		cpu->stall = 1;
            	}
                break;
            }

            case OPCODE_BZ:
            case OPCODE_BNZ:
            case OPCODE_BP:
            case OPCODE_BNP:
            {
            	if(cpu->decode_rename1.btb_id < 0)
            	{
            		cpu->decode_rename1.btb_id = get_free_btb_entry(cpu);
            	}
            	cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
            	break;
            }

            case OPCODE_JUMP:
            {
            	if(cpu->decode_rename1.btb_id < 0)
            	{
            		cpu->decode_rename1.btb_id = get_free_btb_entry(cpu);
            	}
            	cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
            	cpu->stall = 0;
                break;
            }

            case OPCODE_RET:
            {
            	printf("RET rs1 src = %d, slot_id = %d, status = %d\n", cpu->rename_table[cpu->decode_rename1.rs1].src_bit,
            			cpu->rename_table[cpu->decode_rename1.rs1].slot_id, cpu->phys_regs[cpu->rb.entries[cpu->rename_table[cpu->decode_rename1.rs1].slot_id].phy_address].status);
            	if(cpu->rename_table[cpu->decode_rename1.rs1].src_bit == 0)
            	{
            		cpu->decode_rename1.p1 = cpu->decode_rename1.rs1;
            		cpu->decode_rename1.p1_value = cpu->arch_regs[cpu->decode_rename1.rs1].value;
            		cpu->stall = 0;
            	}
            	else
            	{
            		if(cpu->phys_regs[cpu->rb.entries[cpu->rename_table[cpu->decode_rename1.rs1].slot_id].phy_address].status == VALID)
            		{
            			cpu->decode_rename1.p1 = cpu->rename_table[cpu->decode_rename1.rs1].slot_id;
            			cpu->decode_rename1.p1_value = cpu->phys_regs[cpu->rb.entries[cpu->rename_table[cpu->decode_rename1.rs1].slot_id].phy_address].value;
            			cpu->stall = 0;
            		}
            		else
            		{
            			cpu->stall = 1;
            		}
            	}
            	break;
            }
        }
        /* Copy data from decode latch to debug decode latch*/
        cpu->debug_decode_rename1 = cpu->decode_rename1;
        if(!cpu->stall)
        {
            /* Copy data from decode latch to execute latch*/
            cpu->rename2_dispatch = cpu->decode_rename1;
            cpu->decode_rename1.has_insn = FALSE;
        }
    }
    else
    {
    	cpu->debug_decode_rename1 = cpu->decode_rename1;
    }
}

/*
 * Rename2/Dispatch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_rename2_dispatch(APEX_CPU *cpu)
{
    if (cpu->rename2_dispatch.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->rename2_dispatch.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_DIV:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_CMP:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_MOVC:
            case OPCODE_HALT:
            case OPCODE_NOP:
            {
            	check_r_to_r_instruction_dependency(cpu);
                break;
            }

            case OPCODE_BP:
            case OPCODE_BNP:
            case OPCODE_BZ:
            case OPCODE_BNZ:
            case OPCODE_JALR:
            case OPCODE_JUMP:
            {
            	if(cpu->rename2_dispatch.opcode == OPCODE_BZ)
            	{
            		printf("BZ RD2 ROB ID = %d, for CCR = %d", cpu->rename2_dispatch.p1, cpu->rename2_dispatch.rs1);
            	}
            	if(check_unresolved_btb_entry(cpu) >= 0)
            	{
            		cpu->d_stall = 1;
            	}
            	else
            	{
            		check_r_to_r_instruction_dependency(cpu);
            	}
            	break;
            }

            case OPCODE_RET:
            {
            	cpu->pc = cpu->rename2_dispatch.p1_value;
            	cpu->fetch_from_next_cycle = TRUE;
            	cpu->decode_rename1.has_insn = FALSE;
            	break;
            }
			
            case OPCODE_STORE:
            case OPCODE_LOAD:
            {
                check_load_store_instruction_dependency(cpu);
                break;
            }
        }
        /* Copy data from decode latch to debug decode latch*/
        cpu->debug_rename2_dispatch = cpu->rename2_dispatch;
        if(cpu->d_stall == 0)
        {
        	if(cpu->rename2_dispatch.opcode == OPCODE_JUMP ||
        	   cpu->rename2_dispatch.opcode == OPCODE_JALR ||
			   cpu->rename2_dispatch.opcode == OPCODE_BP   ||
			   cpu->rename2_dispatch.opcode == OPCODE_BNP  ||
			   cpu->rename2_dispatch.opcode == OPCODE_BZ  ||
			   cpu->rename2_dispatch.opcode == OPCODE_BNZ)
        	{
        		memcpy(cpu->backup_arch_regs, cpu->arch_regs, sizeof(ARCH_REG) * REG_FILE_SIZE);
        		memcpy(cpu->backup_phys_regs, cpu->phys_regs, sizeof(PHYS_REG) * PHYS_REG_FILE_SIZE);
        		memcpy(cpu->backup_rename_table, cpu->rename_table, sizeof(RENAME_TABLE) * REG_FILE_SIZE);
        	}
            /* Copy data from decode latch to execute latch*/
            cpu->rename2_dispatch.has_insn = FALSE;
        }
    }
    else
    {
    	cpu->debug_rename2_dispatch = cpu->rename2_dispatch;
    }
}

/*
 * Function to get the next available IU instruction
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
get_next_available_iu_instruction(APEX_CPU *cpu)
{
	int id = -1;
	unsigned int min = 0xFFFFFFFF;
	for(int i = 0; i < IQ_SIZE; i++)
	{
		if(cpu->iq.entries[i].al == ALLOCATED &&
		  (cpu->iq.entries[i].fu_type == OPCODE_ADD   ||
		   cpu->iq.entries[i].fu_type == OPCODE_SUB   ||
		   cpu->iq.entries[i].fu_type == OPCODE_AND   ||
		   cpu->iq.entries[i].fu_type == OPCODE_OR    ||
		   cpu->iq.entries[i].fu_type == OPCODE_XOR   ||
		   cpu->iq.entries[i].fu_type == OPCODE_CMP   ||
		   cpu->iq.entries[i].fu_type == OPCODE_ADDL  ||
		   cpu->iq.entries[i].fu_type == OPCODE_SUBL  ||
		   cpu->iq.entries[i].fu_type == OPCODE_LOAD  ||
		   cpu->iq.entries[i].fu_type == OPCODE_STORE ||
		   cpu->iq.entries[i].fu_type == OPCODE_NOP  ||
		   cpu->iq.entries[i].fu_type == OPCODE_HALT ||
		   cpu->iq.entries[i].fu_type == OPCODE_MOVC) &&
		   cpu->iq.entries[i].src1_ready == VALID &&
		   cpu->iq.entries[i].src2_ready == VALID)
		{
			if(cpu->iq.entries[i].cycle < min)
			{
				id = i;
				min = cpu->iq.entries[i].cycle;
			}
		}
	}
	return id;
}

/*
 * Function to get the next available MU instruction
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
get_next_available_mu_instruction(APEX_CPU *cpu)
{
	int id = -1;
	unsigned int min = 0xFFFFFFFF;
	for(int i = 0; i < IQ_SIZE; i++)
	{
		if(cpu->iq.entries[i].al == ALLOCATED &&
		  (cpu->iq.entries[i].fu_type == OPCODE_MUL ||
		   cpu->iq.entries[i].fu_type == OPCODE_DIV) &&
		   cpu->iq.entries[i].src1_ready == VALID &&
		   cpu->iq.entries[i].src2_ready == VALID)
		{
			if(cpu->iq.entries[i].cycle < min)
			{
				id = i;
				min = cpu->iq.entries[i].cycle;
			}
		}
	}
	return id;
}

/*
 * Function to get the next available Load Store instruction
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
get_next_available_load_store_instruction(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->lsq.size > 0)
	{
		if(cpu->lsq.entries[cpu->lsq.front].al == ALLOCATED &&
		   ((cpu->lsq.entries[cpu->lsq.front].ls_bit == 0 && cpu->lsq.entries[cpu->lsq.front].mem_valid == VALID) ||
		    (cpu->lsq.entries[cpu->lsq.front].ls_bit == 1 && cpu->lsq.entries[cpu->lsq.front].mem_valid == VALID &&
		     cpu->lsq.entries[cpu->lsq.front].data_ready == VALID)))
		{
			id = cpu->lsq.front;
			//cpu->lsq.front = (cpu->lsq.front + 1) % LSQ_SIZE;
			//cpu->lsq.size--;
		}
	}
	return id;
}

/*
 * Function to get the next available BU instruction
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
get_next_available_bu_instruction(APEX_CPU *cpu)
{
	int id = -1;
	unsigned int min = 0xFFFFFFFF;
	for(int i = 0; i < IQ_SIZE; i++)
	{
		if(cpu->iq.entries[i].al == ALLOCATED &&
		  (cpu->iq.entries[i].fu_type == OPCODE_BZ   ||
		   cpu->iq.entries[i].fu_type == OPCODE_BNZ  ||
		   cpu->iq.entries[i].fu_type == OPCODE_BP   ||
		   cpu->iq.entries[i].fu_type == OPCODE_BNP  ||
		   cpu->iq.entries[i].fu_type == OPCODE_JUMP ||
		   cpu->iq.entries[i].fu_type == OPCODE_JALR ||
		   cpu->iq.entries[i].fu_type == OPCODE_RET) &&
		   cpu->iq.entries[i].src1_ready == VALID)
		{
			if(cpu->iq.entries[i].cycle < min)
			{
				id = i;
				min = cpu->iq.entries[i].cycle;
			}
		}
	}
	return id;
}

/*
 * Function to execute Load Store FU
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
execute_load_store(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->execute_load_store.has_insn == 1)
	{
		if(cpu->execute_load_store.delay == 2)
		{
			cpu->lsq.entries[cpu->execute_load_store.lsq_id].al = UN_ALLOCATED;
			cpu->lsq.front = (cpu->execute_load_store.lsq_id + 1) % LSQ_SIZE;
			cpu->lsq.size--;
		}
		cpu->execute_load_store.delay--;

		if(cpu->execute_load_store.delay == 0)
		{
			if(cpu->execute_load_store.lsq_entry.ls_bit == 1)
			{
				cpu->data_memory[cpu->execute_load_store.lsq_entry.mem_address] = cpu->execute_load_store.lsq_entry.src1_value;
				cpu->execute_load_store.latch.data = cpu->execute_load_store.lsq_entry.src1_value;
			}
			else
			{
				cpu->execute_load_store.latch.data = cpu->data_memory[cpu->execute_load_store.lsq_entry.mem_address];
			}
			cpu->execute_load_store.latch.ready = VALID;
			cpu->debug_execute_load_store = cpu->execute_load_store;
			cpu->writeback_load = cpu->execute_load_store;
			cpu->execute_load_store.has_insn = 0;

			id = get_next_available_load_store_instruction(cpu);
			if(id >= 0)
			{
				cpu->execute_load_store.has_insn = 1;
				cpu->execute_load_store.delay = 2;
				cpu->execute_load_store.lsq_id = id;
				cpu->execute_load_store.lsq_entry = cpu->lsq.entries[id];
				cpu->execute_load_store.latch.ready = 0;
				cpu->execute_load_store.latch.reg_id = cpu->execute_load_store.lsq_entry.rob_id;
			}
		}
		else
		{
			cpu->debug_execute_load_store = cpu->execute_load_store;
		}
	}
	else
	{
		cpu->debug_execute_load_store = cpu->execute_load_store;
		id = get_next_available_load_store_instruction(cpu);
		if(id >= 0)
		{
			cpu->execute_load_store.has_insn = 1;
			cpu->execute_load_store.delay = 2;
			cpu->execute_load_store.lsq_id = id;
			cpu->execute_load_store.lsq_entry = cpu->lsq.entries[id];
			cpu->execute_load_store.latch.ready = 0;
			cpu->execute_load_store.latch.reg_id = cpu->execute_load_store.lsq_entry.rob_id;
		}
	}
}

/*
 * Function to execute IU FU
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
execute_iu(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->execute_iu.has_insn == 1)
	{
		cpu->iq.entries[cpu->execute_iu.iq_id].al = UN_ALLOCATED;
		cpu->iq.size--;
		cpu->execute_iu.delay--;
		if(cpu->execute_iu.delay == 0)
		{
			switch(cpu->execute_iu.iq_entry.fu_type)
			{
				case OPCODE_ADD:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value + cpu->execute_iu.iq_entry.src2_value;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
				case OPCODE_SUB:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value - cpu->execute_iu.iq_entry.src2_value;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
				case OPCODE_AND:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value & cpu->execute_iu.iq_entry.src2_value;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
				case OPCODE_OR:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value | cpu->execute_iu.iq_entry.src2_value;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
				case OPCODE_XOR:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value ^ cpu->execute_iu.iq_entry.src2_value;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
				case OPCODE_CMP:
				{
	                if (cpu->execute_iu.iq_entry.src1_value == cpu->execute_iu.iq_entry.src2_value)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.iq_entry.src1_value > cpu->execute_iu.iq_entry.src2_value)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					break;
				}

		        case OPCODE_STORE:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src2_value + cpu->execute_iu.iq_entry.literal;
					cpu->execute_iu.latch.ready = VALID;
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}

		        case OPCODE_LOAD:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value + cpu->execute_iu.iq_entry.literal;
					cpu->execute_iu.latch.ready = VALID;
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}

		        case OPCODE_ADDL:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value + cpu->execute_iu.iq_entry.literal;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}

		        case OPCODE_SUBL:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.src1_value - cpu->execute_iu.iq_entry.literal;
					cpu->execute_iu.latch.ready = VALID;
	                if (cpu->execute_iu.latch.data == 0)
	                {
	                	cpu->execute_iu.latch.z_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.z_flag = FALSE;
	                }
	                if (cpu->execute_iu.latch.data > 0)
	                {
	                	cpu->execute_iu.latch.p_flag = TRUE;
	                }
	                else
	                {
	                	cpu->execute_iu.latch.p_flag = FALSE;
	                }
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}

		        case OPCODE_MOVC:
				{
					cpu->execute_iu.latch.data = cpu->execute_iu.iq_entry.literal;
					cpu->execute_iu.latch.ready = VALID;
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
		        case OPCODE_HALT:
		        case OPCODE_NOP:
				{
					cpu->writeback_iu = cpu->execute_iu;
					break;
				}
			}
			cpu->debug_execute_iu = cpu->execute_iu;
			cpu->execute_iu.has_insn = 0;
			id = get_next_available_iu_instruction(cpu);
			if(id >= 0)
			{
				cpu->execute_iu.has_insn = 1;
				cpu->execute_iu.delay = 1;
				cpu->execute_iu.iq_id = id;
				cpu->execute_iu.iq_entry = cpu->iq.entries[id];
				cpu->execute_iu.latch.ready = 0;
				cpu->execute_iu.latch.reg_id = cpu->execute_iu.iq_entry.rob_id;
			}
		}
		else
		{
			cpu->debug_execute_iu = cpu->execute_iu;
		}
	}
	else
	{
		cpu->debug_execute_iu = cpu->execute_iu;
		id = get_next_available_iu_instruction(cpu);
		if(id >= 0)
		{
			cpu->execute_iu.has_insn = 1;
			cpu->execute_iu.delay = 1;
			cpu->execute_iu.iq_id = id;
			cpu->execute_iu.iq_entry = cpu->iq.entries[id];
			cpu->execute_iu.latch.ready = 0;
			cpu->execute_iu.latch.reg_id = cpu->execute_iu.iq_entry.rob_id;
		}
	}
}

/*
 * Function to execute MU FU
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
execute_mu(APEX_CPU *cpu)
{
	int id = -1;
	if(cpu->execute_mu.has_insn == 1)
	{
		if(cpu->execute_mu.delay == 4)
		{
			cpu->iq.entries[cpu->execute_mu.iq_id].al = UN_ALLOCATED;
			cpu->iq.size--;
		}

		cpu->execute_mu.delay--;
		if(cpu->execute_mu.delay == 0)
		{
			switch(cpu->execute_mu.iq_entry.fu_type)
			{
				case OPCODE_MUL:
				{
					cpu->execute_mu.latch.data = cpu->execute_mu.iq_entry.src1_value * cpu->execute_mu.iq_entry.src2_value;
					break;
				}

				case OPCODE_DIV:
				{
					cpu->execute_mu.latch.data = cpu->execute_mu.iq_entry.src1_value / cpu->execute_mu.iq_entry.src2_value;
					break;
				}
			}
            if (cpu->execute_mu.latch.data == 0)
            {
            	cpu->execute_mu.latch.z_flag = TRUE;
            }
            else
            {
            	cpu->execute_mu.latch.z_flag = FALSE;
            }
            if (cpu->execute_mu.latch.data > 0)
            {
            	cpu->execute_mu.latch.p_flag = TRUE;
            }
            else
            {
            	cpu->execute_mu.latch.p_flag = FALSE;
            }
			cpu->execute_mu.latch.ready = 1;
			cpu->debug_execute_mu = cpu->execute_mu;
			cpu->writeback_mu = cpu->execute_mu;
			cpu->execute_mu.has_insn = 0;
			id = get_next_available_mu_instruction(cpu);
			if(id >= 0)
			{
				cpu->execute_mu.has_insn = 1;
				cpu->execute_mu.delay = 4;
				cpu->execute_mu.iq_id = id;
				cpu->execute_mu.iq_entry = cpu->iq.entries[id];
				cpu->execute_mu.latch.ready = 0;
				cpu->execute_mu.latch.reg_id = cpu->execute_mu.iq_entry.rob_id;
			}
		}
		else
		{
			cpu->debug_execute_mu = cpu->execute_mu;
		}
	}
	else
	{
		cpu->debug_execute_mu = cpu->execute_mu;
		id = get_next_available_mu_instruction(cpu);
		if(id >= 0)
		{
			cpu->execute_mu.has_insn = 1;
			cpu->execute_mu.delay = 4;
			cpu->execute_mu.iq_id = id;
			cpu->execute_mu.iq_entry = cpu->iq.entries[id];
			cpu->execute_mu.latch.ready = 0;
			cpu->execute_mu.latch.reg_id = cpu->execute_mu.iq_entry.rob_id;
		}
	}
}

/*
 * Function to execute BU FU
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
execute_bu(APEX_CPU *cpu)
{
	int id;
	if(cpu->execute_bu.has_insn == 1)
	{
		cpu->iq.entries[cpu->execute_bu.iq_id].al = UN_ALLOCATED;
		cpu->iq.size--;
		cpu->execute_bu.delay--;
		if(cpu->execute_bu.delay == 0)
		{
			switch(cpu->execute_bu.iq_entry.fu_type)
			{
				case OPCODE_BZ:
				case OPCODE_BNZ:
				case OPCODE_BP:
				case OPCODE_BNP:
				{
					printf("BZ src1 = %d, z-flag = %d, p-flag = %d\n", cpu->execute_bu.iq_entry.src1_tag, cpu->execute_bu.iq_entry.z_flag, cpu->execute_bu.iq_entry.p_flag);
					if((cpu->execute_bu.iq_entry.fu_type == OPCODE_BZ  && cpu->execute_bu.iq_entry.z_flag == TRUE) ||
					   (cpu->execute_bu.iq_entry.fu_type == OPCODE_BNZ && cpu->execute_bu.iq_entry.z_flag == FALSE)||
					   (cpu->execute_bu.iq_entry.fu_type == OPCODE_BP  && cpu->execute_bu.iq_entry.p_flag == TRUE) ||
					   (cpu->execute_bu.iq_entry.fu_type == OPCODE_BNP && cpu->execute_bu.iq_entry.p_flag == FALSE))
					{
						printf("BZ branch taken\n");
						if(cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history == -1)
						{
							cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.pc + cpu->execute_bu.iq_entry.literal;
							cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 1;
							cpu->execute_bu.change_control = 1;
							cpu->execute_bu.misprediction = 0;
						}
						else
						{
							if(cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].prediction == 0)
							{
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.pc + cpu->execute_bu.iq_entry.literal;
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 1;
								// Need to take the branch
								cpu->execute_bu.change_control = 1;
								cpu->execute_bu.misprediction = 0;
							}
							else
							{
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.pc + cpu->execute_bu.iq_entry.literal;
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 1;
								// Branch already taken
								cpu->execute_bu.change_control = 0;
								cpu->execute_bu.misprediction = 0;
							}
						}
					}
					else
					{
						if(cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history == -1)
						{
							cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.pc + cpu->execute_bu.iq_entry.literal;
							cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 0;
							cpu->execute_bu.change_control = 0;
							cpu->execute_bu.misprediction = 0;
						}
						else
						{
							if(cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].prediction == 0)
							{
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.pc + cpu->execute_bu.iq_entry.literal;
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 0;
								// Branch already not taken
								cpu->execute_bu.change_control = 0;
								cpu->execute_bu.misprediction = 0;
							}
							else
							{
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.pc + cpu->execute_bu.iq_entry.literal;
								cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 0;
								// Branch already taken (mispredicted)
								cpu->execute_bu.change_control = 0;
								cpu->execute_bu.misprediction = 1;
							}
						}
					}

					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].resolved = 1;
					break;
				}

				case OPCODE_JUMP:
				{
					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.src1_value + cpu->execute_bu.iq_entry.literal;
					cpu->execute_bu.misprediction = 0;
					if(cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history == -1)
					{
						cpu->execute_bu.change_control = 1;
					}
					else
					{
						cpu->execute_bu.change_control = 0;
					}
					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 1;
					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].resolved = 1;
					break;
				}

				case OPCODE_JALR:
				{
					cpu->execute_bu.latch.data = cpu->execute_bu.iq_entry.pc + INSTRUCTION_SIZE;
					printf("ExecuteBU JALR latch data = %d", cpu->execute_bu.latch.data);
					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].target = cpu->execute_bu.iq_entry.src1_value + cpu->execute_bu.iq_entry.literal;
					printf("JALR src1_value = %d, literal = %d \n", cpu->execute_bu.iq_entry.src1_value, cpu->execute_bu.iq_entry.literal);

					if(cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history == -1)
					{
						cpu->execute_bu.change_control = 1;
					}
					else
					{
						cpu->execute_bu.change_control = 0;
					}
					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].history = 1;
					cpu->btb.entries[cpu->execute_bu.iq_entry.btb_id].resolved = 1;
					break;
				}

				case OPCODE_RET:
				{
					break;
				}
			}

			cpu->execute_bu.latch.ready = 1;
			cpu->writeback_bu = cpu->execute_bu;
			cpu->debug_execute_bu = cpu->execute_bu;
			cpu->execute_bu.has_insn = 0;
			id = get_next_available_bu_instruction(cpu);
			if(id >= 0)
			{
				cpu->execute_bu.has_insn = 1;
				cpu->execute_bu.delay = 1;
				cpu->execute_bu.iq_id = id;
				cpu->execute_bu.iq_entry = cpu->iq.entries[id];
				cpu->execute_bu.latch.ready = 0;
				cpu->execute_bu.latch.reg_id = cpu->execute_bu.iq_entry.rob_id;
			}
		}
		else
		{
			cpu->debug_execute_bu = cpu->execute_bu;
		}
	}
	else
	{
		cpu->debug_execute_bu = cpu->execute_bu;
		id = get_next_available_bu_instruction(cpu);
		if(id >= 0)
		{
			cpu->execute_bu.has_insn = 1;
			cpu->execute_bu.delay = 1;
			cpu->execute_bu.iq_id = id;
			cpu->execute_bu.iq_entry = cpu->iq.entries[id];
			cpu->execute_bu.latch.ready = 0;
			cpu->execute_bu.latch.reg_id = cpu->execute_bu.iq_entry.rob_id;
		}
	}
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
	execute_iu(cpu);
	execute_mu(cpu);
	execute_bu(cpu);
	execute_load_store(cpu);
}

/*
 * Function to Writeback/Forward IU results
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
writeback_iu(APEX_CPU *cpu)
{
	cpu->debug_writeback_iu = cpu->writeback_iu;
	if(cpu->writeback_iu.has_insn == 1)
	{
		if(cpu->writeback_iu.iq_entry.fu_type == OPCODE_HALT || cpu->writeback_iu.iq_entry.fu_type == OPCODE_NOP)
		{
			cpu->rb.entries[cpu->writeback_iu.iq_entry.rob_id].status = VALID;
		}
		else if(cpu->writeback_iu.latch.ready == 1)
		{
			cpu->writeback_iu.latch.ready = 0;
			if(cpu->writeback_iu.iq_entry.fu_type == OPCODE_ADD   ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_SUB   ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_OR    ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_XOR   ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_CMP   ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_ADDL  ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_SUBL  ||
			   cpu->writeback_iu.iq_entry.fu_type == OPCODE_MOVC)
		    {
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_iu.latch.reg_id].phy_address].value  = cpu->writeback_iu.latch.data;
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_iu.latch.reg_id].phy_address].z_flag = cpu->writeback_iu.latch.z_flag;
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_iu.latch.reg_id].phy_address].p_flag = cpu->writeback_iu.latch.p_flag;
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_iu.latch.reg_id].phy_address].status = VALID;
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_iu.latch.reg_id].phy_address].wbv.BYTE = 0;
				for(int i = 0; i < IQ_SIZE; i++)
				{
					if(cpu->iq.entries[i].al == ALLOCATED)
					{
						if(cpu->iq.entries[i].src1_ready == INVALID && cpu->iq.entries[i].src1_tag == cpu->writeback_iu.latch.reg_id)
						{
							cpu->iq.entries[i].src1_ready = VALID;
							cpu->iq.entries[i].src1_value = cpu->writeback_iu.latch.data;
							cpu->iq.entries[i].z_flag = cpu->writeback_iu.latch.z_flag;
							cpu->iq.entries[i].p_flag = cpu->writeback_iu.latch.p_flag;
						}
						if(cpu->iq.entries[i].src2_ready == INVALID && cpu->iq.entries[i].src2_tag == cpu->writeback_iu.latch.reg_id)
						{
							cpu->iq.entries[i].src2_ready = VALID;
							cpu->iq.entries[i].src2_value = cpu->writeback_iu.latch.data;
						}
					}
				}

				for(int i = 0; i < LSQ_SIZE; i++)
				{
					if(cpu->lsq.entries[i].al == ALLOCATED)
					{
						if(cpu->lsq.entries[i].data_ready == INVALID && cpu->lsq.entries[i].src1_tag == cpu->writeback_iu.latch.reg_id)
						{
							cpu->lsq.entries[i].data_ready = VALID;
							cpu->lsq.entries[i].src1_value = cpu->writeback_iu.latch.data;
						}
					}
				}
				cpu->rb.entries[cpu->writeback_iu.latch.reg_id].status = VALID;
				cpu->rb.entries[cpu->writeback_iu.latch.reg_id].result = cpu->writeback_iu.latch.data;
		    }
			else if(cpu->writeback_iu.iq_entry.fu_type == OPCODE_LOAD ||
				    cpu->writeback_iu.iq_entry.fu_type == OPCODE_STORE)
			{
				printf("IU WB: lsq_id = %d\n", cpu->writeback_iu.iq_entry.lsq_id);
				for(int i = 0; i < LSQ_SIZE; i++)
				{
					if(cpu->lsq.entries[i].al == ALLOCATED && i == cpu->writeback_iu.iq_entry.lsq_id)
					{
						printf("IU WB: Match Found = %d \n", i);
						cpu->lsq.entries[i].mem_valid = VALID;
						cpu->lsq.entries[i].mem_address = cpu->writeback_iu.latch.data;
						break;
					}
				}
			}
			else
			{
				cpu->rb.entries[cpu->writeback_iu.latch.reg_id].status = VALID;
			}
		}
		cpu->writeback_iu.has_insn = 0;
	}
}

/*
 * Function to Writeback/Forward MU results
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
writeback_mu(APEX_CPU *cpu)
{
	cpu->debug_writeback_mu = cpu->writeback_mu;
	if(cpu->writeback_mu.has_insn == 1)
	{
		if(cpu->writeback_mu.latch.ready == 1)
		{
			cpu->writeback_mu.latch.ready = 0;

			cpu->phys_regs[cpu->rb.entries[cpu->writeback_mu.latch.reg_id].phy_address].value  = cpu->writeback_mu.latch.data;
			cpu->phys_regs[cpu->rb.entries[cpu->writeback_mu.latch.reg_id].phy_address].z_flag = cpu->writeback_mu.latch.z_flag;
			cpu->phys_regs[cpu->rb.entries[cpu->writeback_mu.latch.reg_id].phy_address].p_flag = cpu->writeback_mu.latch.p_flag;
			cpu->phys_regs[cpu->rb.entries[cpu->writeback_mu.latch.reg_id].phy_address].status = VALID;
			cpu->phys_regs[cpu->rb.entries[cpu->writeback_mu.latch.reg_id].phy_address].wbv.BYTE = 0;

			for(int i = 0; i < IQ_SIZE; i++)
			{
				if(cpu->iq.entries[i].al == ALLOCATED)
				{
					if(cpu->iq.entries[i].src1_ready == INVALID && cpu->iq.entries[i].src1_tag == cpu->writeback_mu.latch.reg_id)
					{
						cpu->iq.entries[i].src1_ready = VALID;
						cpu->iq.entries[i].src1_value = cpu->writeback_mu.latch.data;
					}
					if(cpu->iq.entries[i].src2_ready == INVALID && cpu->iq.entries[i].src2_tag == cpu->writeback_mu.latch.reg_id)
					{
						cpu->iq.entries[i].src2_ready = VALID;
						cpu->iq.entries[i].src2_value = cpu->writeback_mu.latch.data;
					}
				}
			}
			for(int i = 0; i < LSQ_SIZE; i++)
			{
				if(cpu->lsq.entries[i].al == ALLOCATED)
				{
					if(cpu->lsq.entries[i].data_ready == INVALID && cpu->lsq.entries[i].src1_tag == cpu->writeback_mu.latch.reg_id)
					{
						cpu->lsq.entries[i].data_ready = VALID;
						cpu->lsq.entries[i].src1_value = cpu->writeback_mu.latch.data;
					}
				}
			}
			cpu->rb.entries[cpu->writeback_mu.latch.reg_id].status = VALID;
			cpu->rb.entries[cpu->writeback_mu.latch.reg_id].result = cpu->writeback_mu.latch.data;
		}
		cpu->writeback_mu.has_insn = 0;
	}
}

/*
 * Function to Writeback/Forward Load/Store FU results
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
writeback_load(APEX_CPU *cpu)
{
	cpu->debug_writeback_load = cpu->writeback_load;
	if(cpu->writeback_load.has_insn == 1)
	{
		if(cpu->writeback_load.latch.ready == 1)
		{
			cpu->writeback_load.latch.ready = 0;

			if(cpu->writeback_load.lsq_entry.ls_bit == 0)
			{
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_load.latch.reg_id].phy_address].value  = cpu->writeback_load.latch.data;
				cpu->phys_regs[cpu->rb.entries[cpu->writeback_load.latch.reg_id].phy_address].status = VALID;

				for(int i = 0; i < IQ_SIZE; i++)
				{
					if(cpu->iq.entries[i].al == ALLOCATED)
					{
						if(cpu->iq.entries[i].src1_ready == INVALID && cpu->iq.entries[i].src1_tag == cpu->execute_load_store.latch.reg_id)
						{
							cpu->iq.entries[i].src1_ready = VALID;
							cpu->iq.entries[i].src1_value = cpu->execute_load_store.latch.data;
						}
						if(cpu->iq.entries[i].src2_ready == INVALID && cpu->iq.entries[i].src2_tag == cpu->execute_load_store.latch.reg_id)
						{
							cpu->iq.entries[i].src2_ready = VALID;
							cpu->iq.entries[i].src2_value = cpu->execute_load_store.latch.data;
						}
					}
				}
				for(int i = 0; i < LSQ_SIZE; i++)
				{
					if(cpu->lsq.entries[i].al == ALLOCATED)
					{
						if(cpu->lsq.entries[i].data_ready == INVALID && cpu->lsq.entries[i].src1_tag == cpu->execute_load_store.latch.reg_id)
						{
							cpu->lsq.entries[i].data_ready = VALID;
							cpu->lsq.entries[i].src1_value = cpu->execute_load_store.latch.data;
						}
					}
				}
			}

			cpu->rb.entries[cpu->writeback_load.latch.reg_id].status = VALID;
			cpu->rb.entries[cpu->writeback_load.latch.reg_id].sval_valid = VALID;
			cpu->rb.entries[cpu->writeback_load.latch.reg_id].svalue = cpu->writeback_load.lsq_entry.mem_address;
			cpu->rb.entries[cpu->writeback_load.latch.reg_id].result = cpu->writeback_load.latch.data;
		}
		cpu->writeback_load.has_insn = 0;
	}
}

/*
 * Function to Writeback/Forward BU results
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
writeback_bu(APEX_CPU *cpu)
{
	cpu->debug_writeback_bu = cpu->writeback_bu;
	if(cpu->writeback_bu.has_insn == 1)
	{
		if(cpu->writeback_bu.latch.ready == 1)
		{
			cpu->writeback_bu.latch.ready = 0;
			switch(cpu->writeback_bu.iq_entry.fu_type)
			{
				case OPCODE_BZ:
				case OPCODE_BNZ:
				case OPCODE_BP:
				case OPCODE_BNP:
				{
					if(cpu->writeback_bu.change_control == 1)
					{
						flush_instructions_after_branch(cpu, cpu->writeback_bu.iq_entry.cycle, cpu->btb.entries[cpu->writeback_bu.iq_entry.btb_id].target);
						//cpu->fetch_from_next_cycle = TRUE;
					}
					if(cpu->writeback_bu.misprediction == 1)
					{
						// Recover from mis prediction
						flush_instructions_after_branch(cpu, cpu->writeback_bu.iq_entry.cycle, cpu->writeback_bu.iq_entry.pc + INSTRUCTION_SIZE);
						//cpu->fetch_from_next_cycle = TRUE;
						cpu->misprediction = 1;
						cpu->writeback_bu.misprediction = 0;
						cpu->misprediction_clock = cpu->writeback_bu.iq_entry.cycle;
					}
					break;
				}

				case OPCODE_JUMP:
				{
					if(cpu->writeback_bu.change_control == 1)
					{
						flush_instructions_after_branch(cpu, cpu->writeback_bu.iq_entry.cycle, cpu->btb.entries[cpu->writeback_bu.iq_entry.btb_id].target);
						//cpu->fetch_from_next_cycle = TRUE;
					}
					break;
				}

				case OPCODE_JALR:
				{

					if(cpu->writeback_bu.change_control == 1)
					{
						flush_instructions_after_branch(cpu, cpu->writeback_bu.iq_entry.cycle, cpu->btb.entries[cpu->writeback_bu.iq_entry.btb_id].target);
					}

					cpu->phys_regs[cpu->rb.entries[cpu->writeback_bu.latch.reg_id].phy_address].value  = cpu->writeback_bu.latch.data;
					cpu->phys_regs[cpu->rb.entries[cpu->writeback_bu.latch.reg_id].phy_address].status = VALID;

					cpu->rb.entries[cpu->writeback_bu.latch.reg_id].result = cpu->writeback_bu.latch.data;
					for(int i = 0; i < IQ_SIZE; i++)
					{
						if(cpu->iq.entries[i].al == ALLOCATED)
						{
							if(cpu->iq.entries[i].src1_ready == INVALID && cpu->iq.entries[i].src1_tag == cpu->writeback_bu.latch.reg_id)
							{
								cpu->iq.entries[i].src1_ready = VALID;
								cpu->iq.entries[i].src1_value = cpu->writeback_bu.latch.data;
							}
							if(cpu->iq.entries[i].src2_ready == INVALID && cpu->iq.entries[i].src2_tag == cpu->writeback_bu.latch.reg_id)
							{
								cpu->iq.entries[i].src2_ready = VALID;
								cpu->iq.entries[i].src2_value = cpu->writeback_bu.latch.data;
							}
						}
					}
					for(int i = 0; i < LSQ_SIZE; i++)
					{
						if(cpu->lsq.entries[i].al == ALLOCATED)
						{
							if(cpu->lsq.entries[i].data_ready == INVALID && cpu->lsq.entries[i].src1_tag == cpu->writeback_bu.latch.reg_id)
							{
								cpu->lsq.entries[i].data_ready = VALID;
								cpu->lsq.entries[i].src1_value = cpu->writeback_bu.latch.data;
							}
						}
					}
					break;
				}

				case OPCODE_RET:
				{
					break;
				}
			}
		}
		cpu->debug_writeback_bu = cpu->writeback_bu;
		cpu->rb.entries[cpu->writeback_bu.latch.reg_id].status = VALID;
		cpu->writeback_bu.has_insn = 0;
	}
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_writeback(APEX_CPU *cpu)
{
	writeback_bu(cpu);
	writeback_mu(cpu);
	writeback_load(cpu);
	writeback_iu(cpu);
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *commands[])
{
    APEX_CPU *cpu;

    if (!commands[0])
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = PC_START;
    memset(cpu->phys_regs, 0, sizeof(PHYS_REG) * PHYS_REG_FILE_SIZE);
    memset(cpu->arch_regs, 0, sizeof(ARCH_REG) * REG_FILE_SIZE);
    //memset(cpu->rename_table, -1, sizeof(RENAME_TABLE) * REG_FILE_SIZE);
    for(int i = 0; i < PHYS_REG_FILE_SIZE; i++)
    {
    	cpu->rename_table[i].slot_id = i;
    	cpu->rename_table[i].src_bit = 0;
    }
    cpu->stall = 0x0;
    cpu->d_stall = 0x0;
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);

    cpu->command = process_cpu_commands(commands+1);
    if(!cpu->command)
    {
        free(cpu);
        return NULL;
    }
    if(cpu->command->cmd == SINGLE_STEP)
    {
        cpu->single_step = ENABLE_SINGLE_STEP;
    }
    else
    {
        cpu->single_step = DIABLE_SINGLE_STEP;
    }

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(commands[0], &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    cpu->iq.num_of_entries = IQ_SIZE;
    cpu->iq.size = 0;
    cpu->iq.entries = calloc(IQ_SIZE, sizeof(IQ_Entry));
    if(!cpu->iq.entries)
    {
        free(cpu);
        return NULL;
    }
    memset(cpu->iq.entries, 0, IQ_SIZE * sizeof(IQ_Entry));

    cpu->lsq.num_of_entries = LSQ_SIZE;
    cpu->lsq.entries = calloc(LSQ_SIZE, sizeof(LSQ_Entry));
    if(!cpu->lsq.entries)
    {
        free(cpu);
        return NULL;
    }
    memset(cpu->lsq.entries, 0, LSQ_SIZE * sizeof(LSQ_Entry));
    cpu->lsq.front = 0;
    cpu->lsq.rear = -1;
    cpu->lsq.size = 0;

    cpu->rb.num_of_entries = ROB_SIZE;
    cpu->rb.size = 0;
    cpu->rb.front = 0;
    cpu->rb.rear = -1;
    cpu->rb.entries = calloc(ROB_SIZE, sizeof(ROB_Entry));
    if(!cpu->rb.entries)
    {
        free(cpu);
        return NULL;
    }
    memset(cpu->rb.entries, 0, ROB_SIZE * sizeof(ROB_Entry));

    cpu->btb.num_of_entries = BTB_SIZE;
    cpu->btb.size = 0;
    cpu->btb.entries = calloc(BTB_SIZE, sizeof(BTB_Entry));
    if(!cpu->btb.entries)
    {
        free(cpu);
        return NULL;
    }
    memset(cpu->btb.entries, 0, BTB_SIZE * sizeof(BTB_Entry));
    /* To start fetch stage */

    cpu->misprediction = 0;
    cpu->misprediction_clock = -1;

    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        APEX_writeback(cpu);
        APEX_execute(cpu);
        APEX_rename2_dispatch(cpu);
        APEX_decode_rename1(cpu);
        APEX_fetch(cpu);
        if(print_debug_info(cpu))
        {
            break;
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_phy_reg_file(cpu);
            print_arch_reg_file(cpu);
        }
        if(rob_retirement_logic(cpu))
        {
            /* Halt in writeback stage */
            if(cpu->command->cmd == DISPLAY)
            {
                print_debug_info(cpu); 
                print_phy_reg_file(cpu);
                print_arch_reg_file(cpu);
                print_iq(cpu);
                print_lsq(cpu);
                print_rob(cpu);
                print_data_memory(cpu);
            }
            else if(cpu->command->cmd == SIMULATE)
            {
                print_phy_reg_file(cpu);
                print_arch_reg_file(cpu);
                print_iq(cpu);
                print_lsq(cpu);
                print_rob(cpu);
                print_data_memory(cpu);
            }
            else if(cpu->command->cmd == SHOW_MEM)
            {
                printf("|      MEM[%04d]      |      Data Value = %d      \n", cpu->command->data, cpu->data_memory[cpu->command->data]);
            }
            else if(cpu->command->cmd == SINGLE_STEP)
            {
                print_debug_info(cpu);
                print_phy_reg_file(cpu);
                print_arch_reg_file(cpu);
                print_iq(cpu);
                print_lsq(cpu);
                print_rob(cpu);
                print_data_memory(cpu);
            }
            printf("\nAPEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock+1, cpu->insn_completed);
            break;
        }
        if (cpu->single_step)
        {
			printf("\nPress any key to advance CPU Clock or <q> to quit:\n");
			scanf("%c", &user_prompt_val);

			if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
			{
				print_phy_reg_file(cpu);
				print_arch_reg_file(cpu);
				print_iq(cpu);
				print_lsq(cpu);
				print_rob(cpu);
				print_data_memory(cpu);
				printf("\nAPEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
				break;
			}
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}
