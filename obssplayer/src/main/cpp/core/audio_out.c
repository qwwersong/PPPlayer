
#include <stdio.h>
#include "common.h"

int init_audio(fmp_context *ptx,media_stream *s, void *parm, int parm_size)
{
	tools_def *p = ptx->tools_start;
	int ret = 0;
	if(!s) return 0;

	if (s->out_node){
		uninit_audio(s);
	}
	s->out_node = 0;

	while (p)
	{
		if( p->p->category_id == AV_DEVICE)
		{
			if(p->p->available(AUDIO_RENDER))
			{
				s->out_node = p->p;
				s->out_device = p->p->create(0,0);
				if( s->out_device )
				{
					close_audio_device(s);
					ret = open_audio_device(s,parm,parm_size,ptx->low_latency_mode);
					if(!ret)
					{
						close_audio_device(s);
						s->out_device = 0;
					}
					if(ret)
						break;
				}
			}
		}
		p=p->next;
	}
	if(!s->out_device ){
		s->stream_status = STREAM_STATUS_STOP;
	}
	return s->out_device != NULL;
}

void uninit_audio(media_stream *s)
{
	if(!s) return;
	close_audio_device(s);
	if(s->out_node)
	{
		s->out_node->release(s->out_device);
	}
	s->out_node = 0;
}

int EqFrac(const fraction *a,const fraction *b)
{												
	if (a->den == b->den && a->num == b->num)	
		return 1;								
	if (!a->den) return b->den==0;				
	if (!b->den) return 0;						
	return b->den * a->num == a->den * b->num;
}

void audio_notify_callback(void *key, int msgid, TIME_TYPE audio_time,fraction *speed)
{
	media_stream *stream = (media_stream*)key;
	audio_device *dev = (audio_device*)stream->out_device;
//	static TIME_TYPE lasttime = 0;

	if( msgid == AUDIO_OUT_PKT )
	{
		int diff;
		TIME_TYPE time,old;

// 		TIME_TYPE as = vpc_gettime();
// 		printf("callback interval=%d\r\n", as - lasttime);
// 		lasttime = as;
		mutex_lock(dev->timer_section);
		
		if(!EqFrac(&dev->dev_speed,speed))
		{
			//dev must be change speed
			dev->dev_speed = *speed;
		}

		//update time
		time = (TIME_TYPE)vpc_gettime();
		old = dev->tick + ((time - dev->time_ref)*dev->dev_speed.num/dev->dev_speed.den);

		dev->tick = audio_time;
		diff = old - audio_time;
		if (abs(diff) < 500)
		{
			diff = (audio_time - old);
			diff >>=2;
			dev->tick = old + diff;
		}
		dev->time_ref = time;
		dev->pending_buffer--;
		mutex_unlock(dev->timer_section);
	}
	else if( msgid == AUDIO_OUT_FINISH)
	{
		//set stream status to STREAM_STATUS_STOP
		stream->stream_status = STREAM_STATUS_STOP;
	}
}

int open_audio_device( media_stream *stream,void *extern_parm, int parm_size,int moniter)
{
	audio_device *dev;
	audio_ext *ext;
	audio_out_spec out_spec={0};
	int ret = 0;

	if(!stream) return 0;
	dev = (audio_device*)stream->out_device;
	if(!dev) return 0;

	ext = (audio_ext*)stream->avext;

	//audio spec
	out_spec.freq = ext->freq;
	out_spec.format = ext->bits;
	out_spec.channels = ext->channels;

	dev->audio_msg = audio_notify_callback;
	dev->audio_key = stream;

	dev->dev_speed = stream->stream_speed;

	/*audio clock*/
	dev->timer_section = mutex_init();
	dev->tick = 0;
	dev->time_ref = 0;
	dev->pending_buffer = 0;

// 	if(!dev->extern_parm && parm_size)
// 		dev->extern_parm = vpc_mem_alloc(parm_size);
// 
// 	if(dev->extern_parm)
// 		memcpy(dev->extern_parm,extern_parm, parm_size);

	dev->extern_parm = extern_parm;

	dev->audio_moniter_mode = moniter;
    dev->audio_codec_id = stream->codec->codecid;
    
    if(dev->audio_codec_id == AAC_ID )
    {
        int samplerate,channels,sbr,ps;
        sscanf((char*)ext->extradata,"%d/%d/%d/%d",&samplerate,&channels,&sbr,&ps);
        if(sbr && !ps)
            dev->audio_codec_id = AACP_ID;
        else if(sbr && ps)
            dev->audio_codec_id = AACPP_ID;
    }
    /* dev don't need it now*/
    dev->audio_codec_id = 0;
    
	ret = dev->open_audio(dev,&out_spec);
	dev->spec = out_spec;
	return ret;
}

void close_audio_device(media_stream *stream)
{
	audio_device *dev = (audio_device*)stream->out_device;
	if(!dev) return;
	dev->close_audio(dev);
	if( dev->timer_section)
		mutex_uninit(dev->timer_section);
	dev->timer_section = 0;
	dev->pending_buffer = 0;
// 	if(dev->extern_parm)
// 		vpc_mem_free(dev->extern_parm);
	dev->extern_parm = 0;
}

int get_audio_buffer(audio_device *dev, ao_pkt *pkt)
{	
	if(!dev) return 0;
	return dev->get_audio_buffer(dev,pkt);
}

int push_audio_pkt(audio_device *dev, ao_pkt *pkt)
{
	if(!dev) return 0;
	return dev->push_audio_pkt(dev,pkt);
}

int get_audio_pending_buffer_size(audio_device *dev)
{
	if(!dev) return 0;
	return dev->pending_buffer;
}

void play_audio_pkt(audio_device *dev)
{
	if(!dev) return;
	dev->play_audio_pkt(dev);
	mutex_lock(dev->timer_section);
	dev->pending_buffer++;
	mutex_unlock(dev->timer_section);
}

void pause_audio( audio_device *dev)
{
	if(dev)
	dev->pause_audio(dev);
}

void reset_audio( audio_device *dev )
{
	if(dev)
	dev->reset_audio(dev);
}

void set_audio_mute(media_stream *s, int mute)
{
	audio_ext *ext = (audio_ext*)s->avext;
    ext->mute = mute;
}

void audio_flush_data(media_stream *s)
{
	audio_ext *ext = (audio_ext*)s->avext;
	memset( &ext->out_pkt,0, sizeof(ao_pkt));
	s->dc.out[0] = ext->decode_buf;
}


TIME_TYPE audio_get_timer_time(media_stream *stream)
{
	audio_device *dev = (audio_device*)stream->out_device;
	if(dev)
	{
		TIME_TYPE ret,now=(TIME_TYPE)vpc_gettime();
		mutex_lock(dev->timer_section);
		ret =  dev->tick + ((now - dev->time_ref)*dev->dev_speed.num/dev->dev_speed.den);
		mutex_unlock(dev->timer_section);
		return ret;
	}
	return 0;
}

void audio_reset_timer(media_stream *stream,int b_clear_base)
{
	audio_device *dev = (audio_device*)stream->out_device;
	if(dev)
	{
		TIME_TYPE now=(TIME_TYPE)vpc_gettime();
		mutex_lock(dev->timer_section);
		dev->time_ref = now;
		if(b_clear_base) dev->tick = 0;
		mutex_unlock(dev->timer_section);
	}
}
