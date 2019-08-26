/*
*	memory manager,basic idea is FIFO queue
*/
#ifndef  _MEMORY_MANAGER_H
#define  _MEMORY_MANAGER_H

#include <stdint.h>

/*a single memory zone, the actual size is allocated_length*/

typedef struct mp_val{
	//user data
	uint8_t		*buf;
	int		buf_size;

	//don't access them
	void	*inner_block;
	uint32_t inner_idx;
}mp_val;

/*function prototype*/
#ifdef __cplusplus
extern "C"
{
#endif

	void  *mmg_init_queue(uint32_t block_size );
	void mmg_uninit_queue(void *bitctx );
	
	/*save*/

	//1 way
	int  mmg_put_buffer( void *bitctx, uint8_t *buffer_data, uint32_t len );

	//2 way
	int  mmg_start_put( void *bitctx, uint32_t len );
	int mmg_put_data(void *bitctx,uint8_t*data, int32_t len);
	int mmg_end_put(void *bitctx);


	/*@1,slow*/
	int mmg_get_buffer( void *bitctx, uint8_t *data_get, uint32_t *get_len );

	/*@2,faster*/
	int mmg_begin_get_buffer2(void *bitctx, uint8_t ** data, uint32_t *get_len );
	int mmg_end_get_buffer2(void *bitctx );

	/*move the reader and fill to start position*/
	void mmg_move_to_start(void *bitctx);

	/*get the size of entire buffer*/
	uint32_t mmg_get_buffer_size(void *bitctx);
	
	/*get frame count of entire buffer*/
	uint32_t mmg_get_frame_count(void *bitctx);
	
//--------------------------------------------------------
//					   new interface
//--------------------------------------------------------
	/*just access the memory pool, don't move data pointer*/
	int mmg_access_next_chunk(void *bitctx, mp_val *last, mp_val *ret);
	
	/*skip frame's data to mp_val *here*/
	void mmg_skip_to_here( void *bitctx, mp_val *here );

#ifdef __cplusplus
}
#endif

#endif /*_MEMORY_MANAGER_H*/


