# APEX Pipeline Simulator v2.0
Implementation of simulator for Out-Of-Order Pipeline with IQ, LSQ, Register Renaming, ROB, and multiple FUs.

## Group Members:
### 1) 
### Name : Akash Jaiswal
### Email: ajaiswa3@binghamton.edu
### BU: B00849859
## Feature Implemented : Issue Queue/Load Store Queue/ Out of order changes

### 2) 
### Name : Nikita Shriram Navkar
### Email: nnavkar1@binghamton.edu
### BU: B00888741
## Feature Implemented : Reorder buffer and simulator commands

### 3) 
### Name : Mihir R Dalvi
### Email: mdalvi3@binghamton.edu
### BU: B00863242
## Feature Implemented : Branch prediction 

## Notes:

 - This code is a simple implementation template of a working 5-Stage APEX In-order Pipeline
 - Implementation is in `C` language
 - Stages: Fetch -> Decode -> Execute -> Memory -> Writeback
 - You can read, modify and build upon given code-base to add other features as required in project description
 - You are also free to write your own implementation from scratch
 - All the stages have latency of one cycle
 - There is a single functional unit in Execute stage which perform all the arithmetic and logic operations
 - Logic to check data dependencies has not be included
 - Includes logic for `ADD`, `LOAD`, `BZ`, `BNZ`,  `MOVC` and `HALT` instructions
 - On fetching `HALT` instruction, fetch stage stop fetching new instructions
 - When `HALT` instruction is in commit stage, simulation stops
 - You can modify the instruction semantics as per the project description

## Files:

 - `Makefile`
 - `file_parser.c` - Functions to parse input file
 - `apex_cpu.h` - Data structures declarations
 - `apex_cpu.c` - Implementation of APEX cpu
 - `apex_macros.h` - Macros used in the implementation
 - `main.c` - Main function which calls APEX CPU interface
 - `input.asm` - Sample input file

## How to compile and run

 Go to terminal, `cd` into project directory and type:
```
 make
```
 Run as follows:
```
 ./apex_sim <input_file_name> <command> <sub-command>
 1) Simulate command 
    ./apex_sim input.asm simulate 50
    Simulate for 50 cycles and then show State of Unified Physical Register File and Data Memory at the end of 50 cycles or at the end of program (whichever comes first).
 2) Display command 
    ./apex_sim input.asm display 10
    Simulate for 10 cycles and then show Instruction Flow as well as State of Unified Physical Register File and Data Memory at the end of 10 cycles or at the end of program (whichever comes first).
 3) Single Step command 
    ./apex_sim input.asm single_step
    Proceed one cycle and display all the states shown above, but DO NOT display State of Unified Physical Register File and Data Memory in each cycle (Note: Display State of Unified Physical Register File and Data Memory only at the end)
 4) Show Memory command 
    ./apex_sim input.asm show_mem
    Second command line argument is “show_mem” which displays the content of a specific memory location, with the address of the memory location specific as an argument to this command.
```

## Author

 - Copyright (C) Gaurav Kothari (gkothar1@binghamton.edu)
 - State University of New York, Binghamton

## Bugs

 - Please contact your TAs for any assistance or query
 - Report bugs at: gkothar1@binghamton.edu