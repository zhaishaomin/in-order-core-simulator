/*
 * ECE 18-447, Spring 2013
 *
 * MIPS pipeline timing simulator
 *
 * Chris Fallin, 2012
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"

/* Support for inst_cache ,data_cache and branch predictor! */
//WRITTEN BY ZHAOSHAOMIN
/*defination of branch predictor*/
#define USE_BRANCH_PRED      1
#define GHR_LEN              8
#define NUM_PHT              256
#define BTB_INDEX            10
#define BTB_NUM              1024
#define RAS_NUM              8
//INST CACHE AND DATA CACHE
#define USE_INST_CACHE       1
#define USE_DATA_CACHE       1
#define INST_CACHE_WAY       4
#define INST_CACHE_SET       64
#define DATA_CACHE_WAY       8
#define DATA_CACHE_SET       256
#define CACHE_BLOCK_SIZE     32  
//memory latency,which means that once a miss occurs to inst cache or data cache,
//it need to get data from memory in at least 50 cpu cycles
#define MEM_LATENCY          50



/* Pipeline ops (instances of this structure) are high-level representations of
 * the instructions that actually flow through the pipeline. This struct does
 * not correspond 1-to-1 with the control signals that would actually pass
 * through the pieline. Rather, it carries the orginal instruction, operand
 * information and values as they are collected, and destination information. */
typedef struct Pipe_Op {
    /* PC of this instruction */
    uint32_t pc;
    /* raw instruction */
    uint32_t instruction;
    /* decoded opcode and subopcode fields */
    int opcode, subop;

    /* immediate value, if any, for ALU immediates */
    uint32_t imm16, se_imm16;
    /* shift amount */
    int shamt;

    /* register source values */
    int reg_src1, reg_src2; /* 0 -- 31 if this inst has register source(s), or
                               -1 otherwise */
    uint32_t reg_src1_value, reg_src2_value; /* values of operands from source
                                                regs */

    /* memory access information */
    int is_mem;       /* is this a load/store? */
    uint32_t mem_addr; /* address if applicable */
    int mem_write; /* is this a write to memory? */
    uint32_t mem_value; /* value loaded from memory or to be written to memory */

    /* register destination information */
    int reg_dst; /* 0 -- 31 if this inst has a destination register, -1
                    otherwise */
    uint32_t reg_dst_value; /* value to write into dest reg. */
    int reg_dst_value_ready; /* destination value produced yet? */

    /* branch information */
    int is_branch;        /* is this a branch? */
    uint32_t branch_dest; /* branch destination (if taken) */
    int branch_cond;      /* is this a conditional branch? */
    int branch_taken;     /* branch taken? (set as soon as resolved: in decode
                             for unconditional, execute for conditional) */
    int is_link;          /* jump-and-link or branch-and-link inst? */
    int link_reg;         /* register to place link into? */

} Pipe_Op;

/*defination of BTB states*/
typedef struct btb_state{
    uint32_t tag;
    uint32_t valid;
    uint32_t br_type;
    uint32_t target;
}btb_state;

extern  btb_state btb[BTB_NUM];
typedef struct btb_ret{
        uint32_t target;
        int hit;
        uint32_t br_type;
        }btb_ret;
        

//here i will use a function to operate the 8 least-significant-bit 
extern  int GHR;

//here i will use a function to read or update the PHT
extern  int PHT[NUM_PHT];
         
/*state of RAS:return address stack*/
typedef struct RAS_state{
        uint32_t pointer;
        uint32_t ras_entry[RAS_NUM];
        }RAS_state;
        
extern RAS_state RAS;
/* The pipe state represents the current state of the pipeline. It holds a
 * pointer to the op that is currently at the input of each stage. As stages
 * execute, they remove the op from their input (set the pointer to NULL) and
 * place an op at their output. If the pointer that represents a stage's output
 * is not null when that stage executes, then this represents a pipeline stall,
 * and the stage must not overwrite its output (otherwise an instruction would
 * be lost).
 */

typedef struct Pipe_State {
    /* pipe op currently at the input of the given stage (NULL for none) */
    Pipe_Op *decode_op, *execute_op, *mem_op, *wb_op;

    /* register file state */
    uint32_t REGS[32];
    uint32_t HI, LO;

    /* program counter in fetch stage */
    uint32_t PC;

    /* information for PC update (branch recovery). Branches should use this
     * mechanism to redirect the fetch stage, and flush the ops that came after
     * the branch as necessary. */
    int branch_recover; /* set to '1' to load a new PC */
    uint32_t branch_dest; /* next fetch will be from this PC */
    int branch_flush; /* how many stages to flush during recover? (1 = fetch, 2 = fetch/decode, ...) */

    /* multiplier stall info */
    int multiplier_stall; /* number of remaining cycles until HI/LO are ready */

    /* place other information here as necessary */

} Pipe_State;

/* global variable -- pipeline state */
extern Pipe_State pipe;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* helper: pipe stages can call this to schedule a branch recovery */
/* flushes 'flush' stages (1 = execute only, 2 = fetch/decode, ...) and then
 * sets the fetch PC to the given destination. */
void pipe_recover(int flush, uint32_t dest);

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();


/*addition to lab 6 of CMU 18-447

/*inst cache functions*/
uint32_t read_inst_cache(uint32_t addr);

/*data cache function*/
uint32_t read_data_cache(uint32_t addr);
void     write_data_cache(uint32_t addr,uint32_t value);

/*fun used to update GHR*/
void update_GHR(int actual_direction);

/*read PHT*/
uint32_t read_PHT(uint32_t PHT_index);
/*update PHT*/
void update_PHT(uint32_t PHT_index, uint32_t pre_state, int actual_direction );

/*fun about btb*/
void update_btb(uint32_t c_pc,uint32_t target,uint32_t br_type);
int hit_in_btb(uint32_t c_pc);
btb_ret access_btb(uint32_t c_pc);

/*fun about ras*/
void   push_ras(uint32_t push ,uint32_t target);
uint32_t  pop_ras(uint32_t pop);

/*initialize state of all branch predictor structures to 0*/
void initial_brp(); 

#endif
