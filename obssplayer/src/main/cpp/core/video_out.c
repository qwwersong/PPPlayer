
#include "common.h"


//////////////////////////////////////////////////////////////////////////

int makeAlgin2(int value)
{
#if 0
	int n1 = value;
	--n1;
	n1 |= n1 >> 16;
	n1 |= n1 >> 8;
	n1 |= n1 >> 4;
	n1 |= n1 >> 2;
	n1 |= n1 >> 1;
	++n1;
	return n1;	
#else
	return value;
#endif
}

void video_insert_ts(timestamp_reorder* t, TIME_TYPE time)
{
	int prev;
	TIME_TYPE tmp;
	int slot = t->enter;

	if(t->time_array[slot] != INVALID_TS )
		return;

	while (1)
	{
		prev  = (slot - 1 + MAX_TIME_SIZE)%MAX_TIME_SIZE;
		tmp   = t->time_array[prev]; 
		
		if( tmp ==  INVALID_TS || time > tmp)
		{
			break;
		}
		else if( time == tmp)
		{
			slot = -1; //don't save it.
			break;
		}
		else
		{
			t->time_array[slot] = tmp;
			slot = prev;
		}
	}
	/*inert a new ts*/
	if( slot != -1)
	{
		t->time_array[slot] = time;
		if(++t->enter >= MAX_TIME_SIZE )
			t->enter = 0;
	}
}

TIME_TYPE video_get_ts(timestamp_reorder *t )
{
	TIME_TYPE ret = t->time_array[t->leave];
	t->time_array[t->leave] = INVALID_TS;
	if(ret != INVALID_TS){
		if(++t->leave >= MAX_TIME_SIZE )
			t->leave = 0;
	}
	else{
		ret = 0;
	}
	return ret;
}

void video_flush_ts(timestamp_reorder* t)
{
	t->enter = 0;
	t->leave = 0;
	memset(t->time_array,INVALID_TS,sizeof(t->time_array));

}

//////////////////////////////////////////////////////////////////////////

int init_video(fmp_context *ptx, media_stream *s,void *parm, int parm_size)
{
	tools_def *p = ptx->tools_start;
	int ret = 0;
	if(!s) return 0;

	if (s->out_node){
		uninit_video(s);
	}
	s->out_node = 0;

	while ( p && !ret )
	{
		if( p->p->category_id == AV_DEVICE )
		{
			if(p->p->available(VIDEO_RENDER))
			{
                mutex_lock(ptx->picture_lock);
				s->out_node = p->p;
				s->out_device = p->p->create(0,0);
				if( s->out_device )
				{
					close_video_device(s);
					ret = open_video_device(s,parm,parm_size,ptx->low_latency_mode);
					if(!ret) 
					{
						close_video_device(s);
						s->out_device = 0;
					}
				}
				else{
					vpc_printf("no video device!!!\r\n");
				}
                mutex_unlock(ptx->picture_lock);
			}
		}
		p=p->next;
	}
	if(!s->out_device ){
		s->stream_status = STREAM_STATUS_STOP;
	}
	return s->out_device != NULL;
}

void uninit_video(media_stream *s)
{
	if(!s) return;
	close_video_device(s);
	if(s->out_node){
		s->out_node->release(s->out_device);
	}
	s->out_node = 0;
}

int open_video_device(media_stream *stream, void *extern_parm, int parm_size, int moniter)
{
	video_device *dev;
	video_ext *ext;
	if(!stream) return 0;
	dev = (video_device*)stream->out_device;
	if(!dev) return 0;

	ext = (video_ext*)stream->avext;
  	ext->timer_section = mutex_init();

// 	if(!dev->extern_parm && parm_size)
// 		dev->extern_parm = vpc_mem_alloc(parm_size);

// 	if(dev->extern_parm)
// 		memcpy(dev->extern_parm,extern_parm, parm_size);
	dev->extern_parm = extern_parm;

	dev->video_moniter_mode = moniter;
    dev->video_codec_id = stream->codec->codecid;
 	return dev->open_video(dev,ext->width,ext->height);
}

void close_video_device(media_stream *stream)
{
   	video_ext *ext;
	video_device *dev = (video_device*)stream->out_device;
	if(!dev) return;
	dev->close_video(dev);
	ext = (video_ext*)stream->avext;
    if( ext->timer_section)
	{
		mutex_uninit(ext->timer_section);
		ext->timer_section = 0;
	}
// 	if(dev->extern_parm) vpc_mem_free(dev->extern_parm);
	dev->extern_parm = 0;
	return;
}

static int clip_uint8(int a)
{
	if (a&(~255))
		return (-a)>>31;
	else
		return a;
}

int sharp_func_M(unsigned char *pSource,int para_i,int width,int height,int stride_diff,unsigned char *temp_array)
{
	int para0,para4,i,j,tempy,a,b,c;
	unsigned char *p,*tmp;
	unsigned char tmp0_0,tmp0_1,tmp1_0,tmp1_1;
	int stride = stride_diff + width;

	p=pSource;
	para0=-para_i;
	para4=10*para_i;
	memcpy(temp_array,p,width);
	p+=stride;
	for(i=1;i<height-1;i++)
	{
		tmp=temp_array+width;
		memcpy(tmp,p,width);
		p++;
		tmp0_0=temp_array[0];
		tmp0_1=temp_array[1];
		tmp1_0=temp_array[0+width];
		tmp1_1=temp_array[1+width];
		a=tmp0_0+tmp1_0+(int)(*(p+stride-1));
		b= tmp0_1+ tmp1_1+(int)(*(p+stride));
		for(j=1;j<width-1;j++)
		{
			c=temp_array[j+1]+temp_array[width+j+1]+*(p+stride+1);
			tempy=para0*(a+b+c)+para4*temp_array[width+j];
			a=b;
			b=c;
			tempy=(int)(*p+(tempy>>8));
			*p++=clip_uint8(tempy);
		}
		p+= stride_diff+1;
		tmp=temp_array+width;
		memcpy(temp_array,tmp,width);
	}
	return 0;
}

int video_push_image(media_stream *stream, decode_context*pdc, TIME_TYPE ts,video_tag *vt)
{
//	video_device *dev= (video_device*)stream->out_device;
	video_ext *vp	 = (video_ext*)stream->avext;
	Picture *pFrame  = 0;
	int32_t  size,height,i;
	uint8_t **src;
	int *stride;
	if(!pdc)
	{
		/*end of video stream*/
		if(vp->avail >= vp->buf_frame_cnt-1)
			stream->stream_status = STREAM_STATUS_STOP;
		return 1;
	}

	src = pdc->out;
	stride= pdc->stride;

	if(!pdc->width || !pdc->height )
		return 1;


	if(pdc->width != vp->width ||
		pdc->height != vp->height )
	{
		vp->width = pdc->width;
		vp->height= pdc->height;
		vp->widthAlign2 = makeAlgin2(vp->width);
		vp->heightAlign2 = makeAlgin2(vp->height);
		if(vp->bufferbase)
			vpc_mem_free(vp->bufferbase);
		if(vp->sharpTempBuf)
			vpc_mem_free(vp->sharpTempBuf);
		//
		vp->bufferbase = 0;
		mutex_lock(vp->avail_section);
		vp->avail = vp->buf_frame_cnt;
		mutex_unlock(vp->avail_section);
		vp->pre_frame = -1;
		vp->in = 0;
		vp->out = 0;
	}


	//	if( !src[0] || !stride[0] ) return 1;
	height = vp->height; //src  height/144
	size = height * stride[0];

	if(!vp->bufferbase)
	{
		if( src[0] && stride[0] && size )
		{
            
			size = vp->widthAlign2 * vp->heightAlign2*3/2;
			vp->bufferbase = vpc_mem_alloc(size*vp->buf_frame_cnt);
			if( !vp->bufferbase) return 0;
			vp->sharpTempBuf = vpc_mem_alloc(2*vp->width);

			for(i=0;i<vp->buf_frame_cnt;i++)
			{
				//y
				uint8_t *p = vp->bufferbase+size*i;
				vp->frame[i].src[0]= p;

				//u
				p+= vp->widthAlign2*vp->heightAlign2;
				
				memset(p,128,vp->widthAlign2*vp->heightAlign2/2);
				vp->frame[i].src[1]= p;

				//v
				p+= vp->widthAlign2*vp->heightAlign2/4;
				vp->frame[i].src[2]= p;

				vp->frame[i].ts = INVALID_TS;
			}
		}
	}

	//then put current into frame buffer
	size = height * stride[0];
	if( vp->avail<= 0 )
		return 0;
	pFrame = &vp->frame[vp->in];
	vp->in++;
	mutex_lock(vp->avail_section);
	vp->avail--;
	mutex_unlock(vp->avail_section);
	if(vp->in>=vp->buf_frame_cnt)
	{
		vp->in = 0;
	}
	if( src[0] && size )
	{
        uint8_t *pd;
        const uint8_t*ps;
		pd = pFrame->src[0];
		ps = src[0];

		i=vp->height;
		do{
			memcpy(pd, ps, vp->width);
			pd += vp->widthAlign2;
			ps +=stride[0];
		}while(--i);

		//sharp y
		//sharp_func_M(pFrame->src[0],16/*para_i*/,vp->width,vp->height, vp->widthAlign2 - vp->width, vp->sharpTempBuf);
		
		pd = pFrame->src[1];
		ps = src[1];
		i=vp->height/2;
		
		do{
			memcpy(pd, ps, vp->width/2);
			pd += vp->widthAlign2/2;
			ps += stride[1];
		}while (--i);
		
		pd = pFrame->src[2];
		ps = src[2];
		i=vp->height/2;
		do{
			memcpy(pd, ps, vp->width/2);
			pd += vp->widthAlign2/2;
			ps += stride[2];
		}while (--i);
		pFrame->stride = vp->widthAlign2;
	}
	pFrame->vtag = *vt;
	pFrame->vtag.pt = vpc_gettime();
	pFrame->ts = ts;
	vp->already_ready++;
	return 1;
}

int	 video_buffer_avail(media_stream *stream)
{
	video_ext *vp	 = (video_ext*)stream->avext;
	return vp->avail;
}


int video_render_img(media_stream *stream,TIME_TYPE ps, int bShow)
{
	video_device *dev= (video_device*)stream->out_device;
	video_ext *vp	 = (video_ext*)stream->avext;
	Picture *pFrame = 0;
	int got = 0;
	if(!dev || !vp )
        return 0;

	if(ps == INVALID_TS)
	{
		if(vp->pre_frame != -1)
			pFrame = &vp->frame[vp->pre_frame];
		else
			pFrame = &vp->frame[vp->out];
        
		if( !pFrame )
			return 0;
	}
	else
	{
		pFrame = &vp->frame[vp->out];
		if(pFrame->ts == INVALID_TS )
			return 0;
        
        if( pFrame->ts > ps + EXCEED_RANGE )
        {
            video_reset_tick(stream, pFrame->ts);
            vpc_printf("video ts exceed too much, pFrame->ts=%d, ps=%d\r\n", pFrame->ts, ps);
        }
        else if( pFrame->ts >ps + EARLY_TOO_MUCH )
        {
            //too early to show it
            //printf("early too much , pFrame->ts=%lld, ps=%lld\r\n",pFrame->ts,ps);
            return 0;
        }
        
        if( pFrame->ts + EXCEED_RANGE < ps)
        {
            video_reset_tick(stream, pFrame->ts);
			vpc_printf("video ts behind too much, pFrame->ts=%d, ps=%d\r\n", pFrame->ts, ps);
        }
        else if( pFrame->ts + DELAY_TOO_MUCH < ps)
        {
            //printf("delay too much , pFrame->ts=%lld, ps=%lld\r\n",pFrame->ts,ps);
            //too late to show it
            vp->skipped++;
            //if( stream->codec_node->sub_id != H265_ID ){
            //bShow = 0;
            got = -1; // too late.
            //}
        }
	}
// 	{
// 		static FILE *fp = 0;
// 		if(!fp)
// 		{
// 			fp = fopen("d:\\render_time.txt","w");
// 		}
// 		if(fp)
// 		{
// 			fprintf(fp,"v=%d\r\n",pFrame->ts);
// 			fflush(fp);
// 
// 		}
// 	}
	if( pFrame->src[0] && pFrame->stride && bShow )
	{
		int stride[3];
		stride[0] = pFrame->stride;
		stride[1] = pFrame->stride>>1;
		stride[2] = pFrame->stride>>1;
        
		dev->render_img(dev,pFrame->src,stride, vp->width,vp->height );
		
		if( ps != INVALID_TS )
		{
			//get fps
			int p = vp->idx;
			int e = p;
			vp->showframes[p] = vpc_gettime();
			vp->avail_val++;
			if( vp->avail_val >= FPS_FRAMES)
				vp->avail_val = FPS_FRAMES;

			vp->idx++;
			if(vp->idx >= FPS_FRAMES)
				vp->idx = 0;

			e = (p-vp->avail_val+1);
			if(e<0)
				e += FPS_FRAMES;

			if(e!=p)
			{
				int diff = vp->showframes[p] - vp->showframes[e];
				if(diff)
					vp->fps = ((vp->avail_val)*1000.0)/diff;
			}
		}

		//OK,yuv-rgb,and render it
// 		yuv420_2_rgb565((void *)(*(outp)),
// 			pFrame->src[0],
// 			pFrame->src[1],
// 			pFrame->src[2],
// 			width,
// 			height,
// 			pFrame->stride,
// 			pFrame->stride>>1,
// 			make_it_2_power(width)<<1,
// 			yuv2rgb565_table,
// 			0);
		got = 1;
	}
	pFrame->vtag.st = vpc_gettime();

	if(0)
	{
		vpc_printf("rec-->dec=%d,dec->push=%d,push->show=%d,rec-->show=%d\r\n",
			pFrame->vtag.dt - pFrame->vtag.rt,
			pFrame->vtag.pt - pFrame->vtag.dt,
			pFrame->vtag.st - pFrame->vtag.pt,
			pFrame->vtag.st - pFrame->vtag.rt);
	}
	if( ps == INVALID_TS)
		return got;

	//*************************
	//vpc_printf("++++++++++++++++++++++=render a frame,out=%d,ts=%d, ps=%d\r\n",vp->out,pFrame->ts ,ps);
	pFrame->ts = INVALID_TS;
	if(vp->pre_frame == -1)
	{
		mutex_lock(vp->avail_section);
		vp->avail--;
		mutex_unlock(vp->avail_section);
	}
	vp->pre_frame = vp->out;
	vp->out++;
	if(vp->out >= vp->buf_frame_cnt)
	{
		vp->out = 0;
	}
	mutex_lock(vp->avail_section);
	vp->avail++;
	mutex_unlock(vp->avail_section);
	return got;
}

void video_flush_img(media_stream *stream)
{
	int i;
	video_ext *vp	 = (video_ext*)stream->avext;
	if( !vp ) return;

	vp->in = 0;
	vp->out = 0;
	mutex_lock(vp->avail_section);
	vp->avail = vp->buf_frame_cnt;
	mutex_unlock(vp->avail_section);
	vp->pre_frame = -1;

	for (i=0;i<vp->buf_frame_cnt;i++)
	{
		if(vp->frame)
		vp->frame[i].ts = INVALID_TS;
	}
	vp->fps = 0;
	vp->skipped = 0;
	vp->delivered = 0;
	vp->routine_ret = ALL_OK;
}

TIME_TYPE video_get_timer_value(media_stream *stream)
{
	video_ext *ext= (video_ext*)stream->avext;
	TIME_TYPE ret = 0;
	ret = vpc_gettime();
//    printf("ret %d-%x\r\n",ret,ret);
	mutex_lock(ext->timer_section);
	ext->tick += ((ret-ext->time_ref)*stream->stream_speed.num/stream->stream_speed.den);
	ext->time_ref = ret;
//    printf("ext->tick=%d-%x\r\n",ext->tick,ext->tick);
    mutex_unlock(ext->timer_section);
	return ext->tick;
}

void video_reset_tick(media_stream *stream, TIME_TYPE tick)
{
	video_ext* ext= (video_ext*)stream->avext;
	ext->tick = tick;
	ext->time_ref = vpc_gettime();
}

void video_reset_timer(media_stream *stream, int b_clear_up)
{
	video_ext* ext= (video_ext*)stream->avext;
	if( b_clear_up ) 
		ext->tick = 0;
	ext->time_ref = vpc_gettime();
}
