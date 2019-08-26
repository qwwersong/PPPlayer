
/*
 *	memory manager
 */

#include "common.h"
#include "android/data_type.h"

/*helper function*/
buffer* alloc_buffer_node()
{
	buffer *buffer_node = NULL;
	buffer_node = (buffer*)malloc( sizeof(buffer) );
	if( !buffer_node ) return 0;
	memset( buffer_node, 0, sizeof(buffer) );
	return buffer_node;
}

uint8_t* alloc_virtual_memory( uint32_t size )
{
	if( size>0)
	{
		uint8_t *memory_addr = tmpc_mem_alloc( size );
		if( memory_addr == NULL ) return NULL;
		memset( memory_addr, 0, size );
		return memory_addr;
	}
	return NULL;
}

bool_t free_a_buffer( buffer *p )
{
	if( !p ) return 0;
	if( p->data ) tmpc_mem_free ( p->data );
	free(p);
	return 1;
}

buffer * alloc_a_buffer(uint32_t size)
{
	buffer *p = alloc_buffer_node();
	if( !p) return 0;

	p->data = alloc_virtual_memory(size);
	if( !p->data )
	{
		free_a_buffer( p );
		return 0;
	}
	p->allocated_length = size;
	return p;
}

/*move read marker*/
bool_t adjust_read_marker(bits_context *bitctx)
{
	uint32_t need_copy_num;
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;
	buffer *p = alloc_a_buffer(bitctx->block_size);
	if( !p ){
		return 0;
	}

	/*previous buffer's data which need to read, copy to new buffer's bottom */
	need_copy_num = r->ref_buffer->allocated_length - r->index;
	memcpy( p->data + p->allocated_length-need_copy_num, r->ref_buffer->data+r->index, need_copy_num );

	/*join p to the queue*/
	p->next = f->ref_buffer->next;
	f->ref_buffer->next = p;

	/*change reader's position*/
	r->ref_buffer = p;
	r->index = r->index;/* just a joke*/

	bitctx->tag_of_queue = 0;
	return 1;
}

/*move fill marker,recommend use it!!!*/
bool_t adjust_fill_marker(bits_context *bitctx)
{
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;
	buffer *pre=0;
	buffer *p = alloc_a_buffer(bitctx->block_size);
	if( !p ){
		return 0;
	}

	/*previous buffer's data which need to read, copy to new buffer's top */
	memcpy( p->data , f->ref_buffer->data, f->index );
	pre = bitctx->start_buffer;
	while(pre)
	{
		if( pre->next == f->ref_buffer )
			break;
		pre = pre->next;
	}
	
	if( !pre ) return 0; //error

	/*join p to the queue*/
	p->next = f->ref_buffer;
	pre->next = p;

	/*change fill's position*/
	f->ref_buffer = p;
	f->index = f->index;/* just a joke*/

	bitctx->tag_of_queue = 0;
	return 1;
}
/*
void make_line(bits_context *bitctx, uint32_t need_size)
{
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;
	uint32_t left_size;
	if( f->ref_buffer != r->ref_buffer ) return;
	
	if ( f->index<r->index )
	{
		uint32_t copy_size;
		left_size = r->index;
		if( left_size < need_size ) return;
		copy_size = r->ref_buffer->allocated_length - r->index;
		memcpy( r->ref_buffer->data, r->ref_buffer->data+r->index, copy_size );
		r->index = 0;
		f->index = copy_size;
	}

}
*/

/*get current status of queue*/
bool_t IsFull(bits_context *bitctx)
{
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;

	if( f->ref_buffer == r->ref_buffer &&
		f->index      == r->index &&
		bitctx->tag_of_queue == 1)
	{
		return 1;
	}
	return 0;
}

bool_t IsEmpty(bits_context *bitctx)
{
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;
	if( f->ref_buffer == r->ref_buffer &&
		f->index      == r->index &&
		bitctx->tag_of_queue == 0 )
	{
		return 1;
	}
	return 0;
}

bool_t mmg_init_queue(bits_context *bitctx, uint32_t block_size )
{
	buffer *buffer_now;
	bitctx->block_size = block_size;
	bitctx->start_buffer = alloc_buffer_node();
	if( !bitctx->start_buffer ) return 0;

	if( !(buffer_now = alloc_a_buffer(bitctx->block_size)) ) return 0;

	/*alloc a buffer ready for data*/
	bitctx->start_buffer->next = buffer_now;
	bitctx->fill.ref_buffer = buffer_now;
	bitctx->read.ref_buffer = buffer_now;
	bitctx->fill.index      = 0;
	bitctx->read.index	    = 0;

	bitctx->total_bytes_num = 0;
	bitctx->frame_cnt       = 0;
	bitctx->tag_of_queue    = 0;
	bitctx->merge_memory	= 0;
	bitctx->merge_alloc_len	= 0;
	bitctx->tag_merge		= 0;
	return 1;
}

/*move the reader and fill to start position*/
void mmg_move_to_start(bits_context *bitctx)
{
	bitctx->tag_of_queue = 0;
	bitctx->fill.ref_buffer = bitctx->read.ref_buffer = bitctx->start_buffer->next;
	bitctx->fill.index = bitctx->read.index = 0;
	bitctx->frame_cnt = 0;
	bitctx->total_bytes_num = 0;
	bitctx->tag_merge = 0;
	if( bitctx->merge_memory ) free( bitctx->merge_memory );
	bitctx->merge_memory	= NULL;
	bitctx->merge_alloc_len	= 0;
}

void mmg_uninit_queue(bits_context *bitctx )
{
	buffer *pB = (buffer*)bitctx->start_buffer;
	buffer *pB2;
	while ( pB )
	{
		pB2 = pB->next;
		free_a_buffer( pB );
		pB = pB2;
	}
	bitctx->start_buffer	= 0;
	bitctx->fill.ref_buffer = bitctx->start_buffer;
	bitctx->read.ref_buffer = bitctx->start_buffer;

	bitctx->fill.index      = 0;
	bitctx->read.index	    = 0;
	if( bitctx->merge_memory ) free( bitctx->merge_memory);
	bitctx->merge_memory	= NULL;
	bitctx->merge_alloc_len	= 0;
	bitctx->tag_merge		=0;
}

/*save a buffer into the queue*/
bool_t mmg_put_buffer( bits_context *bitctx, uint8_t *buffer_data, uint32_t len )
{
	uint32_t	really_end	= 0;
	uint32_t	surplus		= len;
	uint32_t	avail_cnt, really_read;
	uint8_t *array_data[2];
	uint32_t array_len[2];
	marker *f,*r;
	int i;
	uint32_t data_len = len;

	if ( len < 1 || !bitctx->start_buffer ) return 0;

	f = &bitctx->fill;
	r = &bitctx->read;
	
	array_len[0]  = sizeof(data_len);
	array_len [1] = data_len;

	array_data[0] = (uint8_t*)&data_len;
	array_data[1] = buffer_data;


	/*there are two steps if save data into our memory*/
	/*@1,data len*/	
	/*@2,data*/

	for ( i=0;i<2;i++)
	{
		surplus		= array_len[i];
		len			= array_len[i];
		buffer_data = array_data[i];

		while( surplus  )
		{
			if( IsFull(bitctx) )
			{
				if( !adjust_fill_marker(bitctx) )
					return 0;
				continue;
			}
			really_end = f->ref_buffer->allocated_length;
			if( f->ref_buffer == r->ref_buffer &&
				f->index < r->index )
			{
				really_end = r->index;
			}
			
			avail_cnt   = really_end - f->index;
			really_read = surplus>avail_cnt?avail_cnt:surplus;
			
			memcpy( f->ref_buffer->data+f->index, buffer_data+len-surplus, really_read );
			
			f->index	+= really_read;
			surplus		-= really_read;
			bitctx->total_bytes_num	+= really_read;
			
			if( f->index >= bitctx->block_size )
			{
				f->index = 0;
				f->ref_buffer  = f->ref_buffer->next;
				if( !f->ref_buffer )  f->ref_buffer = bitctx->start_buffer->next;
			}
			
			bitctx->tag_of_queue = 1;
		}
	}
	bitctx->frame_cnt++;
	return 1;
}

/*get a buffer from queue*/
bool_t mmg_get_buffer( bits_context *bitctx, uint8_t *data_get, uint32_t *get_len )
{
	uint32_t need_read_num;
	uint32_t really_end;
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;
	marker old_read = *r;
	uint32_t des=0;

	/*for copy*/
	uint32_t left_bytes;
	uint32_t actual_read;

	int dectect_len=0;
	uint8_t *len_ptr = (uint8_t*)&dectect_len;

	if( r->ref_buffer == bitctx->start_buffer )
		return 0;

	*get_len = 0;

	while(1)
	{
		if( IsEmpty(bitctx) )
		{
			*r = old_read;
			return 0;
		}
		really_end = r->ref_buffer->allocated_length;
		if( f->ref_buffer == r->ref_buffer && f->index > r->index )
		{
			really_end = f->index;
		}

		if( *get_len == 0 ) /*don't know length ,continue to detect it */
		{
			for(;r->index < really_end;)
			{
				len_ptr[des++] = r->ref_buffer->data[r->index++];
				bitctx->tag_of_queue = 0;
				if( des == sizeof(dectect_len) )/*the length size is 4 bytes*/
				{
					*get_len =  dectect_len;
					des = 0;

					if( (bitctx->total_bytes_num - sizeof(dectect_len)) < *get_len )
					{
						*r = old_read;
						return 0;
					}
					need_read_num = *get_len;
					break;
				}
			}
		}
		else /*get length, copy data */
		{
			left_bytes  = really_end - r->index;
			actual_read = left_bytes>need_read_num?need_read_num:left_bytes;
			memcpy( data_get+des, r->ref_buffer->data+r->index, actual_read );
			bitctx->tag_of_queue = 0;

			des+=actual_read;
			need_read_num -= actual_read;
			r->index += actual_read;

			if ( des >= *get_len )
			{
				bitctx->total_bytes_num -= *get_len;
				bitctx->total_bytes_num -= sizeof(uint32_t);
				bitctx->frame_cnt--;
				return 1;
			}
		}

		if( r->index >= bitctx->block_size )
		{
			r->ref_buffer = r->ref_buffer->next;
			if( !r->ref_buffer ) r->ref_buffer = bitctx->start_buffer->next;
			r->index = 0;
		}
	}
	return 0;
}

bool_t mmg_begin_get_buffer2( bits_context *bitctx, uint8_t ** data, uint32_t *get_len )
{
	uint32_t need_read_num;
	uint32_t really_end;
	marker *f = &bitctx->fill;
	marker *r = &bitctx->read;
	marker old_read = *r;
	
	uint32_t des=0;
	int dectect_len=0;
	uint8_t *len_ptr = (uint8_t*)&dectect_len;

	/*for copy*/
	uint32_t left_bytes;
	uint32_t actual_read;

	bitctx->begin = *r;

	if( r->ref_buffer == bitctx->start_buffer ) return 0;

	*get_len = 0;

	while(1)
	{
		if( IsEmpty(bitctx) )
		{
			*r = old_read;
			return 0;
		}
		really_end = r->ref_buffer->allocated_length;
		if( f->ref_buffer == r->ref_buffer && f->index > r->index )
		{
			really_end = f->index;
		}

		if( *get_len == 0 ) /*don't know length ,continue to detect it */
		{
			for(;r->index < really_end;)
			{
				len_ptr[des++] = r->ref_buffer->data[r->index++];
				if( des == sizeof(dectect_len) )/*the length size is 4 bytes*/
				{
					*get_len = dectect_len;
					des = 0;

					if( (bitctx->total_bytes_num - sizeof(dectect_len)) < *get_len )
					{
						*r = old_read;
						return 0;
					}
					need_read_num = *get_len;
					bitctx->begin = *r; /*very important*/
					break;
				}
			}
		}
		else /*get length, copy data */
		{
			left_bytes  = really_end - r->index;
			actual_read = left_bytes>need_read_num?need_read_num:left_bytes;
//			memcpy( data_get+des, r->ref_buffer->data+r->index, actual_read );

			des+=actual_read;
			need_read_num -= actual_read;
			r->index += actual_read;

			if ( des >= *get_len )
			{
				bitctx->end = *r;
				*r = old_read;
				*data = merge_block( bitctx, *get_len );
				if( *data ) return 1;
				return 0;
			}
		}

		if( r->index >= bitctx->block_size )
		{
			r->ref_buffer = r->ref_buffer->next;
			if( !r->ref_buffer ) r->ref_buffer = bitctx->start_buffer->next;
			r->index = 0;
		}
	}
	return 0;
}

uint8_t* merge_block(bits_context *bitctx ,uint32_t merge_len)
{
	buffer *p;
	uint32_t cnt;
	uint32_t sp = 0;
	uint32_t buf_len = bitctx->end.ref_buffer->allocated_length;

	//debug
	/*static int times;
	static int total;
	total++;*/

	bitctx->tag_merge = 1;
	bitctx->cur_merge_len = merge_len;
	if ( bitctx->begin.ref_buffer == bitctx->end.ref_buffer &&
		 bitctx->end.index > bitctx->begin.index )
	{	
		cnt = bitctx->end.index - bitctx->begin.index;
		return bitctx->begin.ref_buffer->data+bitctx->begin.index;
	}

	if ( bitctx->merge_alloc_len<merge_len )
	{
		bitctx->merge_memory = (uint8_t*)realloc( bitctx->merge_memory,merge_len);
		bitctx->merge_alloc_len = merge_len;
	}
	/*copy memory into merge memory*/
	p=bitctx->begin.ref_buffer;
	cnt = 0;
	sp = bitctx->begin.index;
	do
	{
		int len = p->allocated_length - sp;
		memcpy( bitctx->merge_memory+cnt, p->data+sp, len );
		p=p->next;
		if( !p )
		{
			p = bitctx->start_buffer->next;
		}
		sp = 0;
		cnt += len;
	}while ( p != bitctx->end.ref_buffer );
	memcpy( bitctx->merge_memory+cnt, bitctx->end.ref_buffer->data, bitctx->end.index );
	cnt += bitctx->end.index;

	/*copy done*/
	mmg_end_get_buffer2(bitctx);
	return bitctx->merge_memory;
}

bool_t mmg_end_get_buffer2(bits_context *bitctx )
{
	if( bitctx->tag_merge )
	{
		bitctx->read = bitctx->end;
		bitctx->tag_of_queue = 0;
		bitctx->total_bytes_num -= bitctx->cur_merge_len;
		bitctx->total_bytes_num -= sizeof(uint32_t);
		bitctx->frame_cnt--;
		bitctx->tag_merge = 0;
	}
	return 1;
}

/*get the size of entire buffer*/
uint32_t mmg_get_buffer_size(bits_context *bitctx)
{
	return bitctx->total_bytes_num;
}

/*get frame count of entire buffer*/
uint32_t mmg_get_frame_count(bits_context *bitctx)
{
	return bitctx->frame_cnt;
}

/*skip some frame's data*/
void mmg_skip( bits_context *bitctx, int num )
{
	return;
}
