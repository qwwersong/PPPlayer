/*
 *	memory manager
 */

#include "common.h"

typedef struct mem_block 
{
	uint8_t		*mem_buf; /*memory start address*/
	int32_t	alloc_size;
	int32_t	use_size;
	struct mem_block *next;
}mem_block;

typedef struct bits_context 
{
	/*memory link list*/
	mem_block	*head;
	mem_block	*tail;
	int			block_size;
	int			frame_cnt;
	int			used;
	
	//
	mem_block *	in;
	int32_t	in_idx;
	
	//
	mem_block * out;
	int32_t	out_idx;
}bits_context;

/*helper function*/

static int	free_a_buffer( mem_block *p );
static mem_block * alloc_a_buffer(uint32_t size);

mem_block * alloc_a_buffer(uint32_t size)
{
	mem_block *blk = (mem_block*)vpc_mem_alloc(sizeof(mem_block));
	if( !blk ) return 0;
	memset( blk, 0, sizeof(mem_block));
	blk->mem_buf = vpc_mem_alloc(size);
	if( !blk->mem_buf )
	{
		free_a_buffer(blk);
		return 0;
	}
	blk->alloc_size = size;
	blk->use_size = size;
	return blk;
}

int free_a_buffer( mem_block *p )
{
	if( !p ) return 0;
	if( p->mem_buf ) vpc_mem_free ( p->mem_buf );
	vpc_mem_free(p);
	return 1;
}

void * mmg_init_queue(uint32_t block_size )
{
	mem_block *p = 0;
	bits_context * bitctx = vpc_mem_alloc(sizeof(bits_context));
	if(!bitctx) return 0;

	memset(bitctx,0,sizeof(bits_context));
	bitctx->block_size = block_size;

	p = alloc_a_buffer(block_size);
	if(!p){
		vpc_mem_free(bitctx);
		return 0;
	}

	bitctx->head = p;
	bitctx->tail = p;

	bitctx->in = p;
	bitctx->in_idx = 0;

	bitctx->out = p;
	bitctx->out_idx = 0;

	return bitctx;
}

/*move the reader and fill to start position*/
void mmg_move_to_start(void *bt)
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p = bitctx->head;
	while ( p ){
		p->use_size = p->alloc_size;
		p = p->next;
	}
	bitctx->in = bitctx->head;
	bitctx->in_idx = 0;

	bitctx->out = bitctx->head;
	bitctx->out_idx = 0;

	bitctx->frame_cnt = 0;
	bitctx->used = 0;
}

void mmg_uninit_queue(void *bt)
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p2,*p = bitctx->head;
	while ( p )
	{
		p2 = p->next;
		free_a_buffer( p );
		p = p2;
	}
	memset(bitctx,0,sizeof(*bitctx));

	vpc_mem_free(bitctx);
}

// int check_sum(void *bt, int *sum)
// {
// 	int t =1;
// 	bits_context *bitctx = (bits_context*)bt;
// 	mem_block *p = bitctx->head;
// 	while (p)
// 	{
// 		*sum += p->alloc_size;
// 		p = p->next;
// 		t++;
// 	}
// 	return t;
// }

/*save a buffer into the queue*/
int mmg_put_buffer(void *bt, uint8_t *buffer_data, uint32_t len )
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p	= bitctx->in;
	int blk_offset  = bitctx->in_idx;
	int len_align_4 = ((len+3)&(~3))+4;
	do 
	{
		if(!p)
		{
			//insert a new node into link list
			int size = bitctx->block_size;
			if(len_align_4 > size) size = len_align_4;
			p = alloc_a_buffer(size);
			bitctx->tail->next = p;
			bitctx->tail = p;
			if(!p){
				p = 0;
				return 0;
			}
// 			{
// 				int t,sum;
// 				static int bytes = 0;
// 				static int times = 0;
// 				bytes += size;
// 				times ++;
// 				vpc_printf("call times=%d, current size =%d, total size=%d\r\n", times, size, bytes);
// 				sum = 0;
// 				t = check_sum(bitctx,&sum);
// 				vpc_printf("check sum result =%d times, total size=%d\r\n", t, sum);
// 			}
		}

		if( blk_offset + len_align_4 > p->alloc_size )
		{
			//set the current block's use length
			p->use_size = blk_offset;

			//change to next node
			p = p->next;
			blk_offset = 0;
		}
		else
		{
			//data length
			memcpy(p->mem_buf+blk_offset,&len,4);
			blk_offset += 4;

			//data
			memcpy(p->mem_buf+blk_offset,buffer_data,len);
			blk_offset += ((len+3)&(~3));

			bitctx->frame_cnt++;

			/*save current fill information!!!*/
			bitctx->in = p;
			bitctx->in_idx = blk_offset;
			break;
		}

	} while (1);
	return 1;
}

int  mmg_start_put( void *bt, uint32_t len )
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p	= bitctx->in;
	int blk_offset  = bitctx->in_idx;
	int len_align_4 = ((len+3)&(~3))+4;
	do 
	{
		if(!p)
		{
			//insert a new node into link list
			int size = bitctx->block_size;
			if(len_align_4 > size) size = len_align_4;
			p = alloc_a_buffer(size);
			if(!p) return 0;
			bitctx->tail->next = p;
			bitctx->tail = p;
		}

		if( blk_offset + len_align_4 > p->alloc_size )
		{
			//set the current block's use length
			p->use_size = blk_offset;

			//change to next node
			p = p->next;
			blk_offset = 0;
		}
		else
		{
			//data length
			memcpy(p->mem_buf+blk_offset,&len,4);
			blk_offset += 4;
			/*save current fill information!!!*/
			bitctx->in = p;
			bitctx->in_idx = blk_offset;
			break;
		}

	} while (1);
	return 1;
}

int mmg_put_data(void *bt,uint8_t*data, int32_t len)
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p	= bitctx->in;
	int blk_offset  = bitctx->in_idx;
	//data
	memcpy(p->mem_buf+blk_offset,data,len);
	blk_offset += len;
	bitctx->in = p;
	bitctx->in_idx = blk_offset;
	return 1;
}

int mmg_end_put(void *bt)
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p	= bitctx->in;
	int blk_offset  = bitctx->in_idx;
	bitctx->in_idx = ((blk_offset+3)&(~3));
	bitctx->frame_cnt++;
	return 1;
}

int mmg_get_buffer(void *bt, uint8_t *data_get, uint32_t *get_len )
{
	bits_context *bitctx = (bits_context*)bt;
	uint8_t *pTemp = 0;
	uint32_t len = 0;
	if(mmg_begin_get_buffer2(bitctx,&pTemp,&len) )
	{
		memcpy(data_get,pTemp,len);
		*get_len = len;
		mmg_end_get_buffer2(bitctx);
		return 1;
	}
	return 0;
}

int mmg_begin_get_buffer2( void *bt, uint8_t **data, uint32_t *get_len )
{
	bits_context *bitctx = (bits_context*)bt;
	int length = 0;
	mem_block *p = bitctx->out;
	int blk_offset  = bitctx->out_idx;
	do 
	{
		if(p == bitctx->in && bitctx->in_idx == bitctx->out_idx ){
			return 0; //no data
		}
		if( bitctx->out_idx >= p->use_size )
		{
			mem_block *prev = p;
			prev->use_size = prev->alloc_size;

			//change to next node
			p = p->next;
			prev->next = 0;

			bitctx->tail->next = prev;
			bitctx->tail = prev;
			bitctx->head = p;

			bitctx->out_idx = 0;
			bitctx->out = p;
		}
		else
		{
			length = *(uint32_t*)(p->mem_buf+bitctx->out_idx);
			*get_len = length;
			*data = p->mem_buf+bitctx->out_idx+4;
			bitctx->out = p;
			bitctx->used = 1;
			break;
		}

	} while (1);
	return 1;
}

int mmg_end_get_buffer2(void *bt)
{
	bits_context *bitctx = (bits_context*)bt;
	if( bitctx->used )
	{
		int length = 0;
		mem_block *p = bitctx->out;
		length = *(uint32_t*)(p->mem_buf+bitctx->out_idx);
		bitctx->out_idx +=4;
		bitctx->out_idx += ((length+3)&(~3));
		bitctx->frame_cnt--;
		bitctx->used = 0;
	}
	return 1;
}

/*get frame count of entire buffer*/
uint32_t mmg_get_frame_count(void *bt)
{
	bits_context *bitctx = (bits_context*)bt;
	return bitctx->frame_cnt;
}

/*just access the memory pool, don't move data pointer*/
int mmg_access_next_chunk(void *bt, mp_val *last, mp_val *ret)
{
	bits_context *bitctx = (bits_context*)bt;
	mem_block *p = bitctx->out;
	int blk_offset  = bitctx->out_idx;
	
	if( last && last->inner_block )
	{
		p = last->inner_block;
		blk_offset = last->inner_idx;
	}
	do 
	{
		if(p == bitctx->in && bitctx->in_idx == blk_offset ){
			return 0; //no data
		}
		if( blk_offset >= p->use_size )
		{
			//change to next node
			p = p->next;
			blk_offset = 0;
		}
		else
		{
			ret->buf_size = *(uint32_t*)(p->mem_buf + blk_offset);
			ret->buf = p->mem_buf+blk_offset + 4;
			ret->inner_block = p;

			//compute next chunk position
			blk_offset += 4;
			blk_offset += ((ret->buf_size+3)&(~3));

			ret->inner_idx = blk_offset;
			break;
		}

	} while (1);
	return 1;
}

/*skip frame's data to mp_val *here*/
void mmg_skip_to_here( void *bt, mp_val *here )
{
	uint8_t *data;
	uint32_t len = 0;
	bits_context *bitctx = (bits_context*)bt;
	while(here)
	{
		if( mmg_begin_get_buffer2(bt,&data,&len) )
		{
			 mmg_end_get_buffer2(bt);
			 if(bitctx->out == here->inner_block && bitctx->out_idx == here->inner_idx &&
				 data == here->buf && len == here->buf_size ){
					 break;
			 }
		}
		else{
			break;
		}
	}
}