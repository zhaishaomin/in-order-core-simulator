/*extention of lab6 to support inst cache ,data cache and branch predictor*/

#ifndef _CACHE_BP_H_
#define _CACHE_BP_H_

#include "pipe.h"

/*addition to lab 6 of CMU 18-447

/*inst cache functions*/
uint32_t read_inst_cache(uint32_t addr)
{
	uint32_t index,tag,inst_word,offset;
	index=addr&IC_INDEX_MASK;
	index=index>>CACHE_BLOCK_SIZE;
	tag=addr&IC_TAG_MASK;
	offset=(addr&CACHE_OFFSET_MASK)>>2;
	if(ic_fsm==tag_cmp)
	{
		int  way=IC_NUM_WAY;
		for(int i=0;i<IC_NUM_WAY;i++)
		if(tag==inst_cache[index][i].tag)
		{
			way=i;
			break;
		}
		if(way==IC_NUM_WAY)
		  ic_stall=1;
        inst_word=inst_cache[index][way].block[offset];
	}
	if(ic_stall==1)
	{
         //allocate a mshr for ic miss
         ic_mshr.valid=1;
         ic_mshr.begin=0;
         ic_mshr.done=0;
         ic_mshr.addr=addr;
         //transform ic_state from tag_cmp to wait_block
         ic_fsm=wait_block;
    }
         
     return inst_word;
}
         

/*data cache function*/
uint32_t read_data_cache(uint32_t addr)
{
    uint32_t index,cache_data,tag,offset;
    index=addr&DC_INDEX_MASK;
    index=index>>CACHE_BLOCK_SIZE;
    tag=addr&DC_TAG_MASK;
    offset=(addr&CACHE_OFFSET_MASK)>>2;
    if(dc_fsm==tag_cmp)
    {
		int  way=DC_NUM_WAY;
		for(int i=0;i<DC_NUM_WAY;i++)
		if(tag==data_cache[index][i].tag)
		{
			way=i;
			break;
		}
		if(way==DC_NUM_WAY)
		  dc_stall=1;
        cache_data=data_cache[index][way].block[offset];
	}
	if(dc_stall==1)
	{
         //allocate a mshr for ic miss
         dc_mshr.valid=1;
         dc_mshr.begin=0;
         dc_mshr.done=0;
         dc_mshr.addr=addr;
         dc.mshr.is_load=1;
         //transform ic_state from tag_cmp to wait_block
         dc_fsm=wait_block;
    }
         
     return cache_data;
}
void     write_data_cache(uint32_t addr,uint32_t value)
{
    uint32_t index,cache_data,tag,offset;
    index=addr&DC_INDEX_MASK;
    index=index>>CACHE_BLOCK_SIZE;
    tag=addr&DC_TAG_MASK;
    offset=(addr&CACHE_OFFSET_MASK)>>2;
    if(dc_fsm==tag_cmp)
    {
		int  way=DC_NUM_WAY;
		for(int i=0;i<DC_NUM_WAY;i++)
		if(tag==data_cache[index][i].tag)
		{
			way=i;
			break;
		}
		if(way==DC_NUM_WAY)
		  dc_stall=1;
        else
          data_cache[index][way].block[offset]=value;
	}
	if(dc_stall==1)
	{
         //allocate a mshr for ic miss
         dc_mshr.valid=1;
         dc_mshr.begin=0;
         dc_mshr.done=0;
         dc_mshr.addr=addr;
         dc.mshr.is_load=0;
         //transform ic_state from tag_cmp to wait_block
         dc_fsm=wait_block;
    }
         
}
/*fun about mem access*/

/*check if mem_access has done,that mem_stall_cycles is 0 means done,then call write_block_to_xx according to ic_accessing_mem,
 if ic_accessing_mem is 0 ,then call write_block_to_dc(),if allocated addr is modified, call wb_block_to_mem before allocate new block*/ 
void mem_access_done()
{
     
 }
     
/*used to check if there is a pending entry in either ic_mshr or dc_mshr,then req a access to mem if there is at least one valid entry!*/ 
void req_mem()
{ 
   if(mem_stall_cycles==0)
   {
     if((ic_mshr.valid==1&&ic_mshr.begin==0)&&(dc_mshr.valid==1&&dc_mshr.begin==0))
     {
         if(priority_state==dc_first)
         {
           ic_accessing_mem=0;//dc miss go to mem
           ic_mshr.begin==1;
         }
         else  
         {
           ic_accessing_mem=1;//ic miss go to mem
           dc_mshr.begin==1;
         }
         valid_ic_accessing_mem=1;
         mem_stall_cycles=50;
     }
     else if((ic_mshr.valid==1&&ic_mshr.begin==0)||(dc_mshr.valid==1&&dc_mshr.begin==0))
     {
         if(ic_mshr.valid==1&&ic_mshr.begin==0)
         {
           ic_accessing_mem=1;//ic miss go to mem
           ic_mshr.begin=1;
         ]
         
         if(dc_mshr.valid==1&&dc_mshr.begin==0)
         {
           ic_accessing_mem=0;//dc miss go to mem
           dc_mshr.begin=1;
         }
         
           valid_ic_accessing_mem=1;  
           mem_stall_cycles=50;   
     }
   }
   else
     mem_stall_cycles--;
 }
 
/*if mem has found what ic want to access,then this fun will be called */
void write_block_to_ic() //called by mem_access_done()
{
     }
/*if mem has found what dc want to access,then this fun will be called */
void write_block_to_dc()//called by mem_access_done()
{
     }
/*if a block in dc which has been selected to be replaced by a new block is modified,it should be called before real allocation*/
void wb_block_to_mem()// called by write_block_to_dc()
{
     }
     
/*defination of block replace policy algorithm */
#ifdef  PSEUDO_RANDOM
void    pseudo_random(uint32_t hited_index,  uint32_t * new_block, uint32_t evicted_addr)
{       //note: the evicted_addr should be the block level addr such as 0xffff ffe0 means 32bytes or 8 words block
        uint32_t temp_block[8],addr_mem,write_back;
        if(data_cache[ hited_index].recency==0)
        {
            for(int i=0;i<8;i++)
            {
                 temp_block[i]=data_cache[ hited_index].block[0][i];//extract the evicted data
                 data_cache[ hited_index].block[0][i]=new_block[i]; //write missed data into the block
                
            }
            if(data_cache[ hited_index].state[0]==2)//modified !
            {
               for(int i=0;i<8;i++)
               {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
               }
            }
        }
        else if(data_cache[ hited_index].recency==1)
        {
            for(int i=0;i<8;i++)
            {
                 temp_block[i]=data_cache[ hited_index].block[1][i];//extract the evicted data
                 data_cache[ hited_index].block[1][i]=new_block[i]; //write missed data into the block
                
            }
            if(data_cache[ hited_index].state[1]==2)//modified !
            {
               for(int i=0;i<8;i++)
               {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
               }
            }
        }
        else if(data_cache[ hited_index].recency==2)
        {
            for(int i=0;i<8;i++)
            {
                 temp_block[i]=data_cache[ hited_index].block[2][i];//extract the evicted data
                 data_cache[ hited_index].block[2][i]=new_block[i]; //write missed data into the block
                
            }
            if(data_cache[ hited_index].state[2]==2)//modified !
            {
               for(int i=0;i<8;i++)
               {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]) ;  //write back the dirty data to mem if neccesary!
               }
            }
        }
        else// if(data_cache[ hited_index].recency==3)
        {
            for(int i=0;i<8;i++)
            {
                 temp_block[i]=data_cache[ hited_index].block[3][i];//extract the evicted data
                 data_cache[ hited_index].block[3][i]=new_block[i]; //write missed data into the block
                
            }
            if(data_cache[ hited_index].state[3]==2)//modified !
            {
               for(int i=0;i<8;i++)
               {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
               }
            }
        }
        //get the bits[1:0] of LFSR,which is a increment counter here and updated  every cycle!
        //update the random[1:0] according to the LFSR
        data_cache[ hited_index].recency=LFSR&ox00000003;
}
#elif   3_BIT_LRU
void    3_bit_lru_cache_hit(uint32_t hited_index, uint32_t hited_way )
{
        if(hited_way==0)
        {
            data_cache[hited_index].recency[1]=0;
            data_cache[hited_index].recency[0]=0;
        }
        else if(hited_way==1)
        {
            data_cache[hited_index].recency[1]=0;
            data_cache[hited_index].recency[0]=1;
        }
        else if(hited_way==2)
        {
            data_cache[hited_index].recency[2]=0;
            data_cache[hited_index].recency[1]=1;
        }
        else//hited_way==3
        {
            data_cache[hited_index].recency[2]=1;
            data_cache[hited_index].recency[1]=1;
        }
}
void    3_bit_lru_snoop_inv(uint32_t hited_index, uint32_t hited_way )
{
        if(hited_way==0)
        {
            data_cache[hited_index].recency[1]=1;
            data_cache[hited_index].recency[0]=1;
        }
        else if(hited_way==1)
        {
            data_cache[hited_index].recency[1]=1;
            data_cache[hited_index].recency[0]=0;
        }
        else if(hited_way==2)
        {
            data_cache[hited_index].recency[2]=1;
            data_cache[hited_index].recency[1]=0;
        }
        else//hited_way==3
        {
            data_cache[hited_index].recency[2]=0;
            data_cache[hited_index].recency[1]=0;
        }
        
}
void    3_bit_lru_cache_miss(uint32_t hited_index, uint32_t * new_block, uint32_t evicted_addr)
{
        if(data_cache[hited_index].recency[1]==1)
        {
            
            if(data_cache[hited_index].recency[0]=1)
            {
                 data_cache[hited_index].recency[1]=0;
                 data_cache[hited_index].recency[0]=0;
                 
                 for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[0][i];//extract the evicted data
                 data_cache[ hited_index].block[0][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[0]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
            }
            else
            {
                 data_cache[hited_index].recency[1]=0;
                 data_cache[hited_index].recency[0]=1;
                 for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[1][i];//extract the evicted data
                 data_cache[ hited_index].block[1][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[1]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
            }
        }
        else
        {
            if(data_cache[hited_index].recency[2]==1)
            {
                 data_cache[hited_index].recency[1]=0;
                 data_cache[hited_index].recency[0]=1;
                 for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[2][i];//extract the evicted data
                 data_cache[ hited_index].block[2][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[2]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
            }
            else
            {
                 data_cache[hited_index].recency[1]=1;
                 data_cache[hited_index].recency[0]=1;
                 
                 for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[3][i];//extract the evicted data
                 data_cache[ hited_index].block[3][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[3]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
            }
        }    
        
        
}
#elif   8_BIT_LRU
void    8_bit_lru_cache_hit(uint32_t hited_index, uint32_t hited_way )
{
        if(hited_way==0)
        {
             if(data_cache[hited_index].recency[0]==0)
             {
                  data_cache[hited_index].recency[0]=3;
                  data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;     
             }      
             else if(data_cache[hited_index].recency[0]==1)
             {
                  if(data_cache[hited_index].recency[1]!=0)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  }
                  if(data_cache[hited_index].recency[2]!=0)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  }
                  if(data_cache[hited_index].recency[3]!=0)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
                  }
                  data_cache[hited_index].recency[0]==3;
             }   
             else if(data_cache[hited_index].recency[0]==2)
             {
                  if(data_cache[hited_index].recency[1]=3)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  }
                  if(data_cache[hited_index].recency[2]=3)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  }
                  if(data_cache[hited_index].recency[3]=3)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
                  }
                  data_cache[hited_index].recency[0]==3;
                  
             } 
            
        }
        else if(hited_way==1)
        {
             if(data_cache[hited_index].recency[1]==0)
             {
                  data_cache[hited_index].recency[1]=3;
                  data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;     
             }      
             else if(data_cache[hited_index].recency[1]==1)
             {
                  if(data_cache[hited_index].recency[0]!=0)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  }
                  if(data_cache[hited_index].recency[2]!=0)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  }
                  if(data_cache[hited_index].recency[3]!=0)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
                  }
                  data_cache[hited_index].recency[1]==3;
             }   
             else if(data_cache[hited_index].recency[1]==2)
             {
                  if(data_cache[hited_index].recency[0]=3)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  }
                  if(data_cache[hited_index].recency[2]=3)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  }
                  if(data_cache[hited_index].recency[3]=3)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
                  }
                  data_cache[hited_index].recency[1]==3;
                  
             }
            
        }
        else if(hited_way==2)
        {
             if(data_cache[hited_index].recency[2]==0)
             {
                  data_cache[hited_index].recency[2]=3;
                  data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  data_cache[hited_index].recency[0]=data_cache[hited_index].recency[2]-1;
                  data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;     
             }      
             else if(data_cache[hited_index].recency[2]==1)
             {
                  if(data_cache[hited_index].recency[1]!=0)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  }
                  if(data_cache[hited_index].recency[0]!=0)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  }
                  if(data_cache[hited_index].recency[3]!=0)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
                  }
                  data_cache[hited_index].recency[2]==3;
             }   
             else if(data_cache[hited_index].recency[2]==2)
             {
                  if(data_cache[hited_index].recency[1]=3)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  }
                  if(data_cache[hited_index].recency[0]=3)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  }
                  if(data_cache[hited_index].recency[3]=3)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
                  }
                  data_cache[hited_index].recency[2]==3;
                  
             }
            
        }
        else//hited_way==3
        {
            if(data_cache[hited_index].recency[3]==0)
             {
                  data_cache[hited_index].recency[3]=3;
                  data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;     
             }      
             else if(data_cache[hited_index].recency[3]==1)
             {
                  if(data_cache[hited_index].recency[1]!=0)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  }
                  if(data_cache[hited_index].recency[2]!=0)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  }
                  if(data_cache[hited_index].recency[0]!=0)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  }
                  data_cache[hited_index].recency[3]==3;
             }   
             else if(data_cache[hited_index].recency[3]==2)
             {
                  if(data_cache[hited_index].recency[1]=3)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
                  }
                  if(data_cache[hited_index].recency[2]=3)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
                  }
                  if(data_cache[hited_index].recency[0]=3)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
                  }
                  data_cache[hited_index].recency[3]==3;
                  
             }
        }
        
}
void    8_bit_lru_snoop_inv(uint32_t hited_index, uint32_t hited_way )
{
        if(hited_way==0)
        {
             if(data_cache[hited_index].recency[0]==3)
             {
                  data_cache[hited_index].recency[0]=0;
                  data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;     
             }      
             else if(data_cache[hited_index].recency[0]==2)
             {
                  if(data_cache[hited_index].recency[1]!=3)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  }
                  if(data_cache[hited_index].recency[2]!=3)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  }
                  if(data_cache[hited_index].recency[3]!=3)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;
                  }
                  data_cache[hited_index].recency[0]==0;
             }   
             else if(data_cache[hited_index].recency[0]==1)
             {
                  if(data_cache[hited_index].recency[1]=1)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  }
                  if(data_cache[hited_index].recency[2]=1)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  }
                  if(data_cache[hited_index].recency[3]=1)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;
                  }
                  data_cache[hited_index].recency[0]==0;
                  
             }
        }
        else if(hited_way==1)
        {
             if(data_cache[hited_index].recency[1]==3)
             {
                  data_cache[hited_index].recency[1]=0;
                  data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;     
             }      
             else if(data_cache[hited_index].recency[1]==2)
             {
                  if(data_cache[hited_index].recency[0]!=3)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  }
                  if(data_cache[hited_index].recency[2]!=3)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  }
                  if(data_cache[hited_index].recency[3]!=3)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;
                  }
                  data_cache[hited_index].recency[1]==0;
             }   
             else if(data_cache[hited_index].recency[1]==1)
             {
                  if(data_cache[hited_index].recency[0]=1)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  }
                  if(data_cache[hited_index].recency[2]=1)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  }
                  if(data_cache[hited_index].recency[3]=1)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;
                  }
                  data_cache[hited_index].recency[1]==0;
                  
             }
        }
        else if(hited_way==2)
        {
             if(data_cache[hited_index].recency[2]==3)
             {
                  data_cache[hited_index].recency[2]=0;
                  data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;     
             }      
             else if(data_cache[hited_index].recency[2]==2)
             {
                  if(data_cache[hited_index].recency[1]!=3)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  }
                  if(data_cache[hited_index].recency[0]!=3)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  }
                  if(data_cache[hited_index].recency[3]!=3)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;
                  }
                  data_cache[hited_index].recency[2]==0;
             }   
             else if(data_cache[hited_index].recency[2]==1)
             {
                  if(data_cache[hited_index].recency[1]=1)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  }
                  if(data_cache[hited_index].recency[0]=1)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  }
                  if(data_cache[hited_index].recency[3]=1)
                  {
                      data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]+1;
                  }
                  data_cache[hited_index].recency[2]==0;
                  
             }
             
        }
        else // if(hited_way==3)
        {
             if(data_cache[hited_index].recency[3]==3)
             {
                  data_cache[hited_index].recency[3]=0;
                  data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;     
             }      
             else if(data_cache[hited_index].recency[3]==2)
             {
                  if(data_cache[hited_index].recency[1]!=3)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  }
                  if(data_cache[hited_index].recency[2]!=3)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  }
                  if(data_cache[hited_index].recency[0]!=3)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  }
                  data_cache[hited_index].recency[3]==0;
             }   
             else if(data_cache[hited_index].recency[3]==1)
             {
                  if(data_cache[hited_index].recency[1]=1)
                  {
                      data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]+1;
                  }
                  if(data_cache[hited_index].recency[2]=1)
                  {
                      data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]+1;
                  }
                  if(data_cache[hited_index].recency[0]=1)
                  {
                      data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]+1;
                  }
                  data_cache[hited_index].recency[3]==0;
                  
             }
             
        }     
        
}
void    8_bit_lru_cache_miss(uint32_t hited_index, uint32_t * new_block, uint32_t evicted_addr)
{
        if(data_cache[hited_index].recency[0]=0)
        {
              data_cache[hited_index].recency[0]=3;
              data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
              data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
              data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
              
              for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[0][i];//extract the evicted data
                 data_cache[ hited_index].block[0][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[0]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
        }
        else if(data_cache[hited_index].recency[1]=0)
        {
              data_cache[hited_index].recency[1]=3;
              data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
              data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
              data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
              
              for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[1][i];//extract the evicted data
                 data_cache[ hited_index].block[1][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[1]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
                
        }
        else if(data_cache[hited_index].recency[2]=0)
        {
              data_cache[hited_index].recency[2]=3;
              data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
              data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
              data_cache[hited_index].recency[3]=data_cache[hited_index].recency[3]-1;
              
              for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[2][i];//extract the evicted data
                 data_cache[ hited_index].block[2][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[2]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
                
        }
        else //if(data_cache[hited_index].recency[0]=0)
        {
              data_cache[hited_index].recency[3]=3;
              data_cache[hited_index].recency[1]=data_cache[hited_index].recency[1]-1;
              data_cache[hited_index].recency[2]=data_cache[hited_index].recency[2]-1;
              data_cache[hited_index].recency[0]=data_cache[hited_index].recency[0]-1;
              
              for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[3][i];//extract the evicted data
                 data_cache[ hited_index].block[3][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[3]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
        }
}
#else  
void    round_robin_cache_miss(uint32_t hited_index,  uint32_t * new_block, uint32_t evicted_addr)
{
        if(data_cache[hited_index].recency==0)
        {
             data_cache[hited_index].recency=1;
             
             for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[0][i];//extract the evicted data
                 data_cache[ hited_index].block[0][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[0]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
        }
        else if(data_cache[hited_index].recency==1)
        {
             data_cache[hited_index].recency=2;
             
             for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[1][i];//extract the evicted data
                 data_cache[ hited_index].block[1][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[1]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
        }
        else if(data_cache[hited_index].recency==2)
        {
             data_cache[hited_index].recency=3;
             
             for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[2][i];//extract the evicted data
                 data_cache[ hited_index].block[2][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[2]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
        }
        else // if(data_cache[hited_index].recency=3)
        {
             data_cache[hited_index].recency=0;
             
             for(int i=0;i<8;i++)
                {
                 temp_block[i]=data_cache[ hited_index].block[3][i];//extract the evicted data
                 data_cache[ hited_index].block[3][i]=new_block[i]; //write missed data into the block
                
                }
                if(data_cache[ hited_index].state[3]==2)//modified !
                {
                   for(int i=0;i<8;i++)
                   {
                   addr_mem=evicted_addr+i<<2;     //prepare the addr to mem 
                   mem_write_32(addr_mem,temp_block[i]);   //write back the dirty data to mem if neccesary!
                   }
                }
        }
}


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
			inst_cache[i].tag[j]=0;
			inst_cache[i].state[j]=0;
	
		}

	for(int i=0;i<DATA_CACHE_SET;i++)
	{	for(int j=0;j<DATA_CACHE_WAY;j++)
		{
			data_cache[i].tag[j]=0;
			data_cache[i].state[j]=0;
		
		}
		#ifdef  PSEUDO_RANDOM
        data_cache[i].recency=0;
        #elif   8_BIT_LRU
        for(int k=0:k<4;j++)
        {
            data_cache[i].recency[k]=k;
        }
        #elif   3_BIT_LRU
        for(int a=0; a<3; a++)
        {
            data_cache[i].recency[a]=0;
        }
        #else  //round robin
        data_cache[i].recency=0;
		
    }
}

#endif
