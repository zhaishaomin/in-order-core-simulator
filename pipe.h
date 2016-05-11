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
#define PHT_INDEX_MAX        GHR+1
#define PHT_INDEX_MASK       0x000000ff
#define NUM_PHT              256
#define BTB_INDEX            10
#define BTB_NUM              1024
#define BTB_INDEX_MASK       0x000003ff
#define BTB_TAG_MASK         0xfffffffc
#define RAS_NUM              8
//INST CACHE AND DATA CACHE
#define USE_INST_CACHE       1
#define USE_DATA_CACHE       1
#define INST_CACHE_WAY       4
#define INST_CACHE_SET       64
#define IC_INDEX_MASK        0x000007e0  //using bits[10:5] of the data address
#define IC_TAG_MASK          0xfffff800  //using bits[31:11] of the data address
#define DATA_CACHE_WAY       8
#define DATA_CACHE_SET       256
#define DC_INDEX_MASK        0x00001fe0  //using bits[12:5] of the data address
#define DC_TAG_MASK          0xffffe000  //using bits[31:13] of the data address
#define CACHE_BLOCK_SIZE     5 //in binary used to gen index
#define CACHE_OFFSET_MASK    0x0000001c
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
/*defination of inst cache and data cache*/
typedef struct cache_block{
        uint32_t tag;
        uint32_t state; //0:invalid ,1:valid ,2:modified(only valid in data cache)
        uint32_t recency;
        uint32_t block[8];
        }cache_block;
        
extern cache_block inst_cache[INST_CACHE_SET][INST_CACHE_WAY] ;
extern uint32_t mem_ic_cycles;/*used to track how many cycles left before inst_block return from mem*/

extern cache_block data_cache[DATA_CACHE_SET][DATA_CACHE_WAY] ;
extern uint32_t mem_dc_cycles;/*used to track how many cycles left before data_block return from mem*/

extern uint32_t mem_stall_cycles;


typedef struct MSHR{
        int valid;
        int begin;
        int done;
        uint32_t addr;
        int is_load;
        }MSHR;
extern  MSHR   dc_mshr, ic_mshr;

//used by arbiter of mem to determine which cache miss go first
enum ic_dc_access_priority{ic_first,dc_first};//initialize to dc_first
extern  ic_dc_access_priority priority_state;

extern int ic_accessing_mem;//used to track whether a ic miss accesses mem! 1:ic_access_mem;
extern int valid_ic_accessing_mem;                                                                    //0:dc_access_mem.

enum    ic_fsm_state {tag_cmp,wait_block};
extern  ic_fsm_state ic_fsm;
extern  int ic_stall;//set by req_mem() ;reset by write_block_to_ic(); used by pipe_stage_fetch();

enum    dc_fsm_state{tag_cmp,wait_block,write_block};
extern  dc_fsm_state dc_fsm; 
extern  int dc_stall;//set by re_mem() ;reset by write_block_to_dc(); used by pipe_stage_mem();

/*defination of BTB states*/
typedef struct btb_state{
    uint32_t tag;
    uint32_t valid;
    CTRL_TYPE ctrl_type;
    uint32_t target;
}btb_state;

extern  btb_state btb[BTB_NUM];

//btb return this type of info
typedef struct btb_ret{
        uint32_t target;
        int hit;
        CTRL_TYPE br_type;
        }btb_ret;
        
/* pred info used for branch mispred recovery*/
typedef struct br_pred_info{
	uint32_t  target;
	int taken;
	CTRL_TYPE br_type;
}br_pred_info;

/* recovery info*/
typedef struct br_recovery{
	uint32_t dest_pc;
	int mispred;
	pht_cnt  cnt;
    CTRL_TYPE ctrl_type;
	uint32_t c_pc;
	int pred_taken;
}br_recovery;
enum pht_cnt {str_nt,wea_nt,wea_t,str_t};

enum CTRL_TYPE {br,jal,jalr,j,jr};

//here i will use a function to operate the 8 least-significant-bit 
extern  int GHR;

//here i will use a function to read or update the PHT
extern  pht_cnt  PHT[NUM_PHT];
         
/*state of RAS:return address stack*/
typedef struct RAS_state{
        uint32_t pointer;
        uint32_t ras_entry[RAS_NUM];
        }RAS_state;
        
extern RAS_state ras;
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

/*fun about mem access*/

/*check if mem_access has done,that mem_stall_cycles is 0 means done,then call write_block_to_xx according to ic_accessing_mem,
 if ic_accessing_mem is 0 ,then call write_block_to_dc(),if allocated addr is modified, call wb_block_to_mem before allocate new block*/ 
void mem_access_done();

/*used to check if there is a pending entry in either ic_mshr or dc_mshr,then req a access to mem if there is at least one valid entry!*/ 
void req_mem();

/*if mem has found what ic want to access,then this fun will be called */
void write_block_to_ic(); //called by mem_access_done()

/*if mem has found what dc want to access,then this fun will be called */
void write_block_to_dc();//called by mem_access_done()

/*if a block in dc which has been selected to be replaced by a new block is modified,it should be called before real allocation*/
void wb_block_to_mem();// called by write_block_to_dc()

/*fun used to update GHR*/
void update_GHR(int actual_direction);

uint32_t gen_pht_index(uint32_t pc);

/*read PHT*/
uint32_t PHT_pred(uint32_t PHT_index);
/*update PHT*/
void update_PHT(uint32_t PHT_index, pht_cnt pre_state, int actual_direction );

/*fun about btb*/
void update_btb(uint32_t c_pc,uint32_t target,uint32_t br_type,int btb_miss,int dest_mismatch);

btb_ret btb_pred(uint32_t c_pc);

/*prediction*/
br_pred_info  br_pred(uint32_t c_pc);
/*fun about ras*/
void   push_ras(uint32_t push ,uint32_t target);
uint32_t  pop_ras(uint32_t pop);
/* used for jalr inst*/
uint32_t  jalr_ras(uint32_t push_pc);

/*initialize state of all branch predictor structures to 0*/
void initial_brp(); 

/*initialize state of caches*/
void initial_caches();


#endif
