/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"

typedef unsigned char uint8;
typedef unsigned short uint16;

/* Enumeration for different APEX pipeline stages */
typedef enum PIPELINE_STAGES {
    FETCH = 0,
    DECODE_RENAME1,
    RENAME2_DISPATCH,
	EXECUTE_IU,
	EXECUTE_MU,
	EXECUTE_BU,
	EXECUTE_LOAD_STORE,
    WRITEBACK_IU,
	WRITEBACK_MU,
	WRITEBACK_BU,
	WRITEBACK_LOAD
} PIPELINE_STAGES;

/* Enumeration for different FU types */
typedef enum CPU_FU_TYPE {
    IU = 0,
    MU,
    BU,
    FU_MAX
} CPU_FU_TYPE;

/* Enumeration for allocation status */
typedef enum ALLOCATION_STATUS {
    UN_ALLOCATED = 0,
    ALLOCATED
} ALLOCATION_STATUS;

/* Enumeration for data validity status */
typedef enum VALIDITY_STATUS {
    INVALID = 0,
    VALID
} VALIDITY_STATUS;

/* Enumeration for renamed status */
typedef enum RENAMED_STATUS {
    NOT_RENAMED = 0,
    RENAMED
} RENAMED_STATUS;

/* Enumeration for load store bit */
typedef enum LOAD_STORE_BIT {
    LOAD = 0,
    STORE
} LOAD_STORE_BIT;

/* Enumeration for different APEX CPU commands */
typedef enum CPU_COMMAND_TYPE {
    SIMULATE = 0,
    DISPLAY,
    SINGLE_STEP,
    SHOW_MEM,
    COMMAND_MAX
} CPU_COMMAND_TYPE;

/* Structure to hold APEX CPU commands and related data */
typedef struct CPU_COMMAND {
    CPU_COMMAND_TYPE cmd;
    int data;
} CPU_COMMAND;

/* Structure for data forwarding latches*/
typedef struct DATA_FORWARDING_LATCH {
    int reg_id;
    int ready;
    int data;
    int z_flag;
    int p_flag;
} DATA_FORWARDING_LATCH;

/* Structure for architecture registers*/
typedef struct ARCH_REG {
    int value;
    int z_flag;
    int p_flag;
} ARCH_REG;

/* Bit field for waiting bit vector of pyhsical registers*/
typedef union WBV {
    uint8 BYTE;
    struct BIT {
        uint8 W0: 1;
        uint8 W1: 1;
        uint8 W2: 1;
        uint8 W3: 1;
        uint8 W4: 1;
        uint8 W5: 1;
        uint8 W6: 1;
        uint8 W7: 1;
    } BIT;
} WBV;

/* Structure for pyhsical registers*/
typedef struct PHYS_REG {
    int value;
    ALLOCATION_STATUS al;
	VALIDITY_STATUS status;
    RENAMED_STATUS renamed;
    WBV wbv;
    int z_flag;
    int p_flag;
} PHYS_REG;

/* Format of an LSQ Entry */
typedef struct LSQ_Entry {
	ALLOCATION_STATUS al;
	LOAD_STORE_BIT ls_bit;
	VALIDITY_STATUS mem_valid;
    int mem_address;
    int dest;
    VALIDITY_STATUS data_ready;
    int src1_tag;
    int src1_src;
    int src1_value;
    int rd;
	int pc;
	int rob_id;
	unsigned int cycle;
	char opcode_str[128];
} LSQ_Entry;

/* Format of LSQ */
typedef struct LSQ {
    int num_of_entries;
    int front;
    int rear;
    int size;
    LSQ_Entry *entries;
} LSQ;

/* Format of an IQ Entry */
typedef struct IQ_Entry {
	ALLOCATION_STATUS al;
	int fu_type;
    int literal;
    VALIDITY_STATUS src1_ready;
    int src1_tag;
    int src1_src;
    int src1_value;
    VALIDITY_STATUS src2_ready;
    int src2_tag;
    int src2_src;
    int src2_value;
    int dest;
    int rd;
    int z_flag;
    int p_flag;
    int lsq_id;
    int rob_id;
    int btb_id;
	int pc;
	unsigned int cycle;
	char opcode_str[128];
} IQ_Entry;

/* Format of IQ */
typedef struct IQ {
    int num_of_entries;
    int size;
    IQ_Entry *entries;
} IQ;

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int p1;
    int rs2;
    int p2;
    int rd;
    int pd;
    int imm;
    int rs1_value;
    int p1_value;
    int rs2_value;
    int p2_value;
    int has_insn;
    int btb_id;
} CPU_Stage;

/* Model of FU stage latch */
typedef struct FU_Stage
{
	int has_insn;
	int iq_id;
	int rob_id;
	int delay;
	int change_control;
	int misprediction;
	IQ_Entry iq_entry;
	DATA_FORWARDING_LATCH latch;
} FU_Stage;

/* Model of FU stage latch */
typedef struct MEM_FU_Stage
{
	int has_insn;
	int rob_id;
	int lsq_id;
	int delay;
	LSQ_Entry lsq_entry;
	DATA_FORWARDING_LATCH latch;
} MEM_FU_Stage;

typedef struct RENAME_TABLE
{
	int slot_id;
	int src_bit;
} RENAME_TABLE;

typedef struct ROB_Entry
{
	int pc;
	int cycle;
	char opcode_str[128];
	int arch_address;
	int phy_address;
	int result;
	int svalue;
	int sval_valid;
	int excodes;
	int status;
	int itype;

}ROB_Entry;

typedef struct ROB
{
	int num_of_entries;
	int front;
	int rear;
	int size;
	ROB_Entry *entries;
} ROB;

typedef struct BTB_Entry
{
	int al;
	int tag;
	int type;
	int resolved;
	int target;
	int history;
	int prediction;
}BTB_Entry;

typedef struct BTB
{
	int num_of_entries;
	int size;
	BTB_Entry *entries;
} BTB;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    CPU_COMMAND *command;                       /* CPU command type and related data */
    int pc;                                     /* Current program counter */
    unsigned int clock;                                  /* Clock cycles elapsed */
    int insn_completed;                         /* Instructions retired */
    ARCH_REG arch_regs[REG_FILE_SIZE];          /* Architecture register file */
    PHYS_REG phys_regs[PHYS_REG_FILE_SIZE];     /* Pyhsical register file */
    RENAME_TABLE rename_table[REG_FILE_SIZE];          /* Rename table */
    ARCH_REG backup_arch_regs[REG_FILE_SIZE];          /* Architecture register file */
    PHYS_REG backup_phys_regs[PHYS_REG_FILE_SIZE];     /* Pyhsical register file */
    RENAME_TABLE backup_rename_table[REG_FILE_SIZE];          /* Rename table */
    int misprediction;
    int misprediction_clock;
    IQ iq;
    LSQ lsq;
    ROB rb;
    BTB btb;
    uint8 stall;                        		/* Stalling status as per scoreboarding */
    uint8 d_stall;                        		/* Stalling status as per scoreboarding */
    int code_memory_size;                       /* Number of instruction in the input file */
    APEX_Instruction *code_memory;              /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE];          /* Data Memory */
    int single_step;                            /* Wait for user input after every cycle */
    int zero_flag;                              /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int positive_flag;                          /* {TRUE, FALSE} Used by BP and BNP to branch */
    int fetch_from_next_cycle;

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode_rename1;
    CPU_Stage rename2_dispatch;
    FU_Stage execute_iu;
    FU_Stage execute_mu;
    FU_Stage execute_bu;
    MEM_FU_Stage execute_load_store;
    FU_Stage writeback_iu;
    FU_Stage writeback_mu;
    FU_Stage writeback_bu;
    MEM_FU_Stage writeback_load;
    /* Debug data */
    CPU_Stage debug_fetch;
    CPU_Stage debug_decode_rename1;
    CPU_Stage debug_rename2_dispatch;
    FU_Stage debug_execute_iu;
    FU_Stage debug_execute_mu;
    FU_Stage debug_execute_bu;
    MEM_FU_Stage debug_execute_load_store;
    FU_Stage debug_writeback_iu;
    FU_Stage debug_writeback_mu;
    FU_Stage debug_writeback_bu;
    MEM_FU_Stage debug_writeback_load;
} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *commands[]);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
CPU_COMMAND * process_cpu_commands(char const *commands[]);
#endif
