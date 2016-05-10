/*extention of lab6 to support inst cache ,data cache and branch predictor*/

#ifndef _CACHE_BP_H_
#define _CACHE_BP_H_

#include "pipe.h"

/*addition to lab 6 of CMU 18-447

/*inst cache functions*/
uint32_t read_inst_cache(uint32_t addr)
{
         if()
         }
         

/*data cache function*/
uint32_t read_data_cache(uint32_t addr)
void     write_data_cache(uint32_t addr,uint32_t value)

/*fun used to update GHR*/
void update_GHR(int actual_direction)
{
     if(actual_direction==1)
     GHR=(GHR<<1)|1;
     else
     GHR=GHR<<1;
}

uint32_t gen_pht_index(uint32_t pc)
{
         uint32_t pht_index;
         pht_index=GHR^(pc>>2);
         pht_index=pht_index&PHT_INDEX_MASK;
         return pht_index;
}
         
/*read PHT*/
uint32_t PHT_pred(uint32_t PHT_index)
{
         uint32_t taken;
         pht_cnt  temp_cnt;
         temp_cnt=PHT[PHT_index];
         if(temp_cnt==str_nt||temp_cnt==wea_nt)
         taken=0;
         else
         taken=1;
         reture taken;
}
         
/*update PHT*/
void update_PHT(uint32_t PHT_index, pht_cnt pre_state, int actual_direction )
{
	if(pre_state==str_nt&&actual_direction==1)

		PHT[PHT_index]=wea_nt;
	else if (pre_state==wea_nt)
	{
		if(actual_direction==1)
         PHT[PHT_index]=wea_t;
		else
		 PHT[PHT_index]=str_nt;
	}
	else if(pre_state==wea_t)
	{
		if(actual_direction==1)
         PHT[PHT_index]=str_t;
		else
		 PHT[PHT_index]=wea_nt;
    }
	else if(pre_state==str_t&&actual_direction==0)
        PHT[PHT_index]=wea_t;
}

/*fun about btb*/
void update_btb(uint32_t c_pc,uint32_t target,CTRL_TYPE br_type,int btb_miss,int dest_mismatch)
{
	uint32_t btb_index;
    btb_index=c_pc>>2;
	btb_index=btb_index&BTB_INDEX_MASK;

	if(btb_miss==1)
	{
		//write new entry into btb
		btb[btb_index].valid=1;
		btb[btb_index].tag=c_pc&BTB_TAG_MASK;
        btb[btb_index].ctrl_type=br_type;
        btb[btb_index].target=target;
	}
	else if(dest_mismatch==1)
	{
		btb[btb_index].ctrl_type=br_type;
        btb[btb_index].target=target;
	}
	
}


}
btb_ret btb_pred(uint32_t c_pc)
{
	btb_ret ret;
	uint32_t btb_index;
    uint32_t temp_tag,pc_tag,hit,target;
//	CTRL_TYPE ctrl_type;
	btb_index=c_pc>>2;
	btb_index=btb_index&BTB_INDEX_MASK;
	temp_tag=btb[btb_index].tag;
    pc_tag=pc_tag&BTB_TAG_MASK;
	hit=btb[btb_index].valid;
    ret.br_type=btb[btb_index].ctrl_type;
	target=btb[btb_index].target;
	//btb miss
	if(hit==0||pc_tag!=temp_tag)
	{
		ret.hit=0;
		ret.targer=c_pc+4;
	}
	else
	{
		ret.hit=1;
		ret.target=target;
	}
		

	return ret;
}

/*prediction*/
br_pred_info  br_pred(uint32_t c_pc)
{
	uint32_t pht_index,target;
	br_pred_info  pred_info;
	btb_ret  ret;
	int   taken;
	pht_index=gen_pht_index(c_pc);
    ret=btb_pred(c_pc);
    taken=PHT_pred(PHT_index);
	pred_info.br_type=ret.br_type;
	if(ret.hit==1)
	{
		if(ret.br_type==br||ret.br_type==j||ret.br_type==jal)
			pred_info.target=ret.target;
		else if(ret.br_type==jr)
            pred_info.target=pop_ras();
		else //ret.br_type==jalr
		{
			uint32_t temp_pc;
			temp_pc=c_pc+4;
			pred_info.target=jalr_ras(temp_pc);
		}
	}
	else
		pred_info.target=ret.target;
    
	if(ret.br_type==br)
     pred_info.taken=taken;
	else
	 pred_info.taken=1;
	
}



/*fun about ras*/
void   push_ras(uint32_t target)
{
	ras.pointer++;
	ras_entry[ras.pointer]=target;
}

uint32_t  pop_ras()
{
	uint32_t pop_pc;
	pop_pc=ras_entry[ras.pointer];
    ras.pointer--;
	return pop_pc;
}

uint32_t  jalr_ras(uint32_t push_pc)
{
	uint32_t pop_pc;
	pop=pop_ras();
	push_ras(push_pc);
}
/*initialize state of all branch predictor structures to 0*/
void initial_brp()
{
	 //initial GHR to 0
     GHR=0;
	 //initial all counters of pht to str_nt(00) 
	 pht_cnt temp_cnt;
	 temp_cnt=str_nt;
     for(int i=0;i<NUM_PHT;i++)
		 PHT[i]=temp_cnt;
	 //initial all btb_tags to 0
	 for(int i=0;i<BTB_NUM;i++)
	 {
		 btb[i].valid=0;
		 btb[i].ctrl_type=0;
		 btb[i].tag=0;
	 }
	 //initial RAS to 0
	 ras.pointer=0;
	 for(int i=0;i<RAS_NUM;i++)
		 ras_entry[i]=0;

 }
/*initialize state of caches*/
void initial_caches()
{
	for(int i=0;i<INST_CACHE_SET;i++)
		for(int j=0;j<INST_CACHE_WAY;j++)
		{
			inst_cache[i][j].tag=0;
			inst_cache[i][j].state=0;
			inst_cache[i][j].recency=0;
		}

	for(int i=0;i<DATA_CACHE_SET;i++)
		for(int j=0;j<DATA_CACHE_WAY;j++)
		{
			data_cache[i][j].tag=0;
			data_cache[i][j].state=0;
			data_cache[i][j].recency=0;
		}
}

