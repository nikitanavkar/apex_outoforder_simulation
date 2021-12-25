/*
 * apex_macros.h
 * Contains APEX cpu pipeline macros
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _MACROS_H_
#define _MACROS_H_

#define FALSE 0x0
#define TRUE 0x1

/* Integers */
#define DATA_MEMORY_SIZE 4096

/* Size of integer register file */
#define REG_FILE_SIZE 17
#define PHYS_REG_FILE_SIZE 26
#define HIDDEN_PHY_REG_FILE_SIZE 6
#define ROB_SIZE 16
#define IQ_SIZE	8
#define LSQ_SIZE 6
#define BTB_SIZE 30

/* Numeric OPCODE identifiers for instructions */
#define OPCODE_ADD 0x0
#define OPCODE_SUB 0x1
#define OPCODE_MUL 0x2
#define OPCODE_DIV 0x3
#define OPCODE_AND 0x4
#define OPCODE_OR 0x5
#define OPCODE_XOR 0x6
#define OPCODE_MOVC 0x7
#define OPCODE_LOAD 0x8
#define OPCODE_STORE 0x9
#define OPCODE_BZ 0xa
#define OPCODE_BNZ 0xb
#define OPCODE_HALT 0xc
#define OPCODE_ADDL 0xd
#define OPCODE_SUBL 0xe
#define OPCODE_BP 0x11
#define OPCODE_BNP 0x12
#define OPCODE_CMP 0x13
#define OPCODE_JUMP 0x14
#define OPCODE_NOP 0x15
#define OPCODE_JALR 0x16
#define OPCODE_RET 0x17

/* Set this flag to 1 to enable debug messages */
#define ENABLE_DEBUG_MESSAGES 0

/* Set this flag to 1 to enable single-step mode */
#define ENABLE_SINGLE_STEP 1

/* Set this flag to 0 to disable single-step mode */
#define DIABLE_SINGLE_STEP 0

/* Number of pipeline stages */
#define NUM_OF_PIPELINE_STAGES 12

/* Base program cunter value */
#define PC_START 4000

/* Instruction size in code memory */
#define INSTRUCTION_SIZE 4

/* Flag to put the pipeline in stalling stage */
#define STALL_CPU 1

/* Flag to put the pipeline in execution stage */
#define CONTINUE_EXEC 0

/* X-macro to set the register status to invalid */
#define SET_REGISTER_STATUS(word,nbit)   ((word) |=  (1<<(nbit)))

/* X-macro to set the register status to valid */
#define CLEAR_REGISTER_STATUS(word,nbit) ((word) &=  ~(1<<(nbit)))

/* X-macro to get the register status bit value */
#define CHECK_REGISTER_STATUS(word,nbit) ((word) &    (1<<(nbit)))

#endif
