
#include "common.h"

/************************************************************************/
//						internal function
/************************************************************************/

/*process audio or video*/
static int audio_stream_process(struct media_stream *s, TIME_TYPE ps, int eof);
static int video_stream_process(struct media_stream *s, TIME_TYPE ps, int eof);
static int video_decode_output_routine(struct media_stream *s, int direct_output);
static int video_send_decode_command(struct media_stream *stream);
static THREAD_RETURN_TYPE VPC_API video_decode_thread(void *p);

static int video_output(struct media_stream *s,int finish);
static void video_speed(struct media_stream *s, int step);
static void video_release_ext(struct media_stream *s);
static int video_render(media_stream *s, TIME_TYPE ps);
static void video_release(media_stream *s);

static int master_exit(fmp_context *ptx);
static int slave_exit(fmp_context *ptx);

static int audio_render(media_stream *s, TIME_TYPE ps);
static int audio_output(struct media_stream *s,int finish);
static void audio_speed(struct media_stream *s, int step);

static void audio_release_ext(struct media_stream *s);
static void audio_release(media_stream *s);

static TIME_TYPE smooth_media_time(struct media_stream *s, TIME_TYPE dts );
static int fill_sample(fmp_context *ptx, stream_pack *sp);
static int process_status(fmp_context*ptx);

static TIME_TYPE get_current_time(fmp_context *ptx);
static void reset_timer( fmp_context *ptx, uint32_t clear_base );
static void set_current_playspeed(fmp_context *ptx, int num,int den);

static void start_up_play(fmp_context *ptx);
static void *find_source_filter(fmp_context *ptx, const char *url, int av_format);
static int get_media_stream_buffer(struct media_stream *s, TIME_TYPE stop_ts,void (*handler)( struct media_stream *s) );

static void buffer_speed_reset(BufferSpeedComputer *speedComputer );
static int buffer_speed_append_sample(BufferSpeedComputer *speedComputer, int speed );

/* timer */
static void init_timer(user_timer *timer, int dur, void *key, int(*callback)(void *));
static void start_timer(user_timer *timer);
static void loop_timer(user_timer *timer);
static void stop_timer(user_timer *timer);

/*state machine*/
typedef int (*STATS_MAC_TAB)(fmp_context *ptx);

//static void connect_2_any(fmp_context *ptx, int req);
//static void buffer_2_any(fmp_context *ptx, int req);
//static void play_2_any(fmp_context *ptx, int req);
//static void pause_2_any(fmp_context *ptx, int req);
//static void seeking_2_any(fmp_context *ptx, int req);
//static void stop_2_any(fmp_context *ptx, int req);

/* 
#define		VPC_PS_UNKNOWN			0x00
#define 	VPC_PS_STOP				0x01
#define 	VPC_PS_PLAY				0x02
#define 	VPC_PS_PAUSE			0x03
*/

static int stop_2_play( fmp_context *ptx );
static int play_2_stop( fmp_context *ptx );
static int play_2_pause( fmp_context *ptx );
static int pause_2_stop( fmp_context *ptx );
static int pause_2_play( fmp_context *ptx );

static void do_internal_state_shift( fmp_context *ptx, int req);
static void do_pause(fmp_context *ptx);
static void do_seek(fmp_context *ptx);

static STATS_MAC_TAB state_func[VPC_PS_COUNT][VPC_PS_COUNT] =
{
	{0,0,0,0},
	{0,0,stop_2_play,0},
	{0,play_2_stop,0,play_2_pause},
	{0,pause_2_stop,pause_2_play,0},
};

#define BUFFER_CHECK_INTERVAL		3000 /* ms */
#define LOW_LATENCY_BUFFER_LIMIT	6000 /* ms */

//////////////////////////////////////////////////////////////////////////
/*
*	internal functions
*/
void RegisterTools(const void*ptx, const void*node_func, void*PluginsHandle)
{
	tools_def *now = 0;
	tools_def *pre,*pd;
	const node_boot_func *add_node = (const node_boot_func*)node_func;
	fmp_context *p = (fmp_context*)ptx;
	if(!p) return;

	/*alloc a new node*/
	now = (tools_def*)vpc_mem_alloc(sizeof(tools_def));
	if(!now) return;
	now->p				= node_func;
	now->module_handle	= PluginsHandle;
	
	/*find a exist node*/
	pre = 0;
	pd = p->tools_start;
	while (pd)
	{
		if(pd->p->sub_id == add_node->sub_id)
		{
			goto add_to_exist;
		}
		pre = pd;
		pd = pd->next;
	}

	now->next	= p->tools_start;
	p->tools_start	= now;
	return;

add_to_exist:
	while (pd)
	{
		if(add_node->priority > pd->p->priority || pd->p->sub_id != add_node->sub_id)
		{
			break;
		}
		pre = pd;
		pd = pd->next;
	}

	now->next = pd;
	if(pre){
		pre->next = now;
	}
	else{
		p->tools_start	= now;
	}
}

int is_unload_now(tools_def *p, tools_def *now)
{
	if( !p || !now ) return 1;
	while (p)
	{
		if(p->module_handle == now->module_handle )
			return 0;
		p=p->next;
	}
	return 1;
}

void UnRegisterTools(void*pc)
{
	tools_def *p;
	tools_def *now = 0;
	fmp_context *ptx = (fmp_context*)pc;
	if( !ptx ) return;

	while ( ptx->tools_start )
	{
		p=ptx->tools_start->next;
		now = ptx->tools_start;
		if( ptx->tools_start->module_handle )
		{
			if( is_unload_now(p,now) )
			{
				vpc_free_module(ptx->tools_start->module_handle);
			}
		}
		vpc_mem_free(ptx->tools_start);
		ptx->tools_start = p;
	}
}


/************************************************************************/
/*                         main API										*/
/************************************************************************/

/*
	param1, id
	param2, value
*/

void reset_player(fmp_context *ptx)
{
	int i=0;
	media_stream *s = 0;
	/*player*/
	ptx->seeked_pos			= 0;
	ptx->position			= 0;
	ptx->alreay_buffer_time = 0;
	ptx->buffer_full		= 0;
//	buffer_speed_reset(&ptx->buffer_speed);

	for (i=0;i<MAX_STREAM_NUM;i++)
	{
		s = ptx->stream[i];
		if(s)
		{
			s->wait_for_output = 0;
			s->recv_time = 0;
			s->use_time	 = 0;
			s->buffer_time = 0;
			s->prev_time_stamp = 0;
			s->diff = 0;

			s->flow_time = 0;

			s->output_offset = 0;
			s->skip_frame = skip_nothing;
			memset(&s->cur_pkt,0,sizeof(s->cur_pkt));
			memset(&s->dc,0, sizeof(s->dc));
			mmg_move_to_start( s->mempool);

			if(s->codec->flush)
				s->codec->flush(s->codec);

			if(s->media_style == TM_MEDIA_VIDEO){
				video_flush_img(s);
			}
			if(s->media_style == TM_MEDIA_AUDIO){
				audio_flush_data(s);
			}
		}
	}
}

void msg_gateway (void* key,int notify_id, long param1, long param2)
{
	fmp_context *ptx = (fmp_context*)key;
	if( !ptx) return;
	switch(notify_id)
	{
	case MSG_BY_PASS:
		ptx->notify_call_back(ptx->user_data, (int)param1, (unsigned int)param2);
		break;
	case MSG_AFTER_SEEK:
		{
			if( ptx->broken_continue )
			{
// 				if(ptx->vpc_cur_ps == VPC_PS_BUFFER)
// 					ptx->notify_call_back(ptx->user_data, VPC_START_BUFFER_DATA, 0);
// 				else if (ptx->vpc_cur_ps == VPC_PS_PLAY)
// 					ptx->notify_call_back(ptx->user_data, VPC_START_PLAY, 0);
// 				ptx->broken_continue = 0;
				break;
			}
			else {
				media_stream *s = ptx->stream[TM_MEDIA_VIDEO];
				if(s) s->i_arrived = 0;	
			}
			reset_player(ptx);
			ptx->seeked_pos = (TIME_TYPE)param1;
			if( param1 == 0 )
				ptx->seeked_pos = ptx->do_seek_point;
// 			ptx->do_seek_point = param1;//must be set.
			//printf("seeked time point=%d\r\n",ptx->seeked_pos);
			set_request_status(ptx, 0,Internal_Enter_Buffer );
		}
		break;
	case MSG_LIVE_RESET:
		{
			media_stream *sa,*sv = ptx->stream[TM_MEDIA_VIDEO];
			TIME_TYPE old = ptx->position + ptx->seeked_pos;
			if(sv) sv->i_arrived = 0;	

			sa = ptx->stream[TM_MEDIA_AUDIO];
			if(sa) reset_audio(sa->out_device);

			reset_player(ptx);
			ptx->seeked_pos = old;

			reset_timer(ptx,1);
			if( sa || sv )
				ptx->run_decode_process = 1;
            
            vpc_printf("player core reset\r\n");
		}
		break;
	case MSG_RECONNECT_SEEK:
		{
			ptx->broken_continue = 1;
		}
		break;
	case MSG_MEDIA_FMT:
		{
			if(init_media_fmt( ptx, (uint8_t*)param1, (uint32_t)param2)){
				set_request_status(ptx,0,Internal_Media_Got);
			}
		}
		break;
	case MSG_END_OF_FILE:
// 		if( ptx->vpc_cur_ps == VPC_PS_BUFFER )
// 			set_request_status(ptx, VPC_PS_BUFFER,VPC_PS_PLAY );
		break;
	case MSG_GET_BUF_TIME:
		{
			TIME_TYPE *p = (TIME_TYPE*)param1;
			if(p){
				player_status ps={0x00};

				compute_buffer_time(ptx);
                vpc_get_status(ptx, &ps);
                
                if (param2==1)
                {
                    *p = ps.cur_play_pos;//get cur play position
                }
                else
                {
                    *p = ptx->alreay_buffer_time ;//get buffer time
                }
				
			}
            break;
		}
    case MSG_GET_CORE_STATUS:
        {
            int *p = (int*)param1;
            if(p) *p = ptx->vpc_cur_ps;
            break;
        }
	default:
		break;
	}
}

 void generate_play_strategy(fmp_context*ptx)
 {
 	TIME_TYPE nbf = ptx->alreay_buffer_time;
 	int speed_num = 1;
 	int speed_den = 1;
 	if(nbf > 3000)
 	{
 		speed_num = 3;
 		speed_den = 2;
 	}
 	else if(nbf>1000)
 	{
 		speed_num = 6;
 		speed_den = 5;
 	}
 	else if(nbf>300)
 	{
 		speed_num = 11;
 		speed_den = 10;
 	}
 	else
 	{
 		speed_num = 1;
 		speed_den = 1;
 	}
 	set_current_playspeed(ptx,speed_num,speed_den);
 }

void video_handler( media_stream *s)
{
	//if it is video data,just put it into decoder
	decode_context	*pdc = &s->dc;
	do 
	{
		int consumed = s->codec->decode(s->codec,pdc);
		if(consumed<0){
			pdc->in_len = 0;
		}
		else if( consumed > 0 )
		{
			pdc->in_stream	+= consumed;
			pdc->in_len		-= consumed;
			if( pdc->in_len < 0 ) pdc->in_len =0;
		}
	} while(pdc->in_len);

}
int aa = 0;
void shrink_buffer( fmp_context *ptx, TIME_TYPE delete_buffer_ms )
{
//	int t = 0;
	TIME_TYPE video_sync_time = 0, audio_sync_time = 0;
	media_stream *vs, *as;
	vpc_printf("delete_buffer_ms=%d\r\n", delete_buffer_ms);
	vs = ptx->stream[TM_MEDIA_VIDEO];
	as = ptx->stream[TM_MEDIA_AUDIO];
	if( vs )
	{
		video_sync_time = vs->use_time + delete_buffer_ms;
		 if( video_sync_time > vs->recv_time ){
			 vpc_printf("video_sync_time > vs->recv_time, so cancel this sync\r\n");
			 return;
		 }
	 }
	 if( as )
	 {
		 audio_sync_time = as->use_time + delete_buffer_ms;
		 if( audio_sync_time > as->recv_time ){
			 vpc_printf("audio_sync_time > as->recv_time, so cancel this sync\r\n");
			 return;
		 }
	}
     aa = 0;
	 vpc_printf("begin reduce buffer time, current time is=%d\r\n", ptx->alreay_buffer_time);
	 if(vs) 
	 {
		 video_ext *vp	 = (video_ext*)vs->avext;

		 if(vp->video_decode_pending){
			 vpc_printf("video is decoding, cancel it!!!\r\n");
			 return;
		 }

		 mutex_lock(vp->decode_section);
		 get_media_stream_buffer(vs, video_sync_time, video_handler );
		 vs->decoded_bytes	= 0;
		 vs->wait_for_output = 0;
		 mutex_unlock(vp->decode_section);
	 }

	 if( as) 
	 {
		 audio_ext *ext = (audio_ext*)as->avext;
		 decode_context	*pdc = &as->dc;
		 get_media_stream_buffer(as, audio_sync_time, 0 );
		 as->decoded_bytes	= 0;
		 as->wait_for_output = 0;
		 pdc->out[0] = ext->decode_buf;
		 memset(&ext->out_pkt,0,sizeof(ext->out_pkt));
	 }
	 if( vs ){ //if no audio, must set video timer tick!
         decode_context	*pdc = &vs->dc;
		 video_reset_tick(vs, pdc->in_ts );
	 }
	 compute_buffer_time(ptx);
	 vpc_printf("reduce buffer time to=%d\r\n", ptx->alreay_buffer_time);

 }

/*render thread*/
int master_entry(fmp_context *ptx)
{
	/*need_cpu_cap -->sleep time*/
	media_stream *s;
	int i,need_cpu_cap = 10;

	/*state exchange*/
	process_status(ptx);
	
	/*check end of file*/
	if(ptx->check_end_of_file)
	{
		//check play finish
		int eof = 1;
		for (i=0;i<MAX_STREAM_NUM;i++)
		{
			s = ptx->stream[i];
			if(s && s->stream_status != STREAM_STATUS_STOP)
				eof = 0;
		}
		if(eof)
		{
			ptx->run_render_process = 0;
			ptx->run_decode_process = 0;
			set_request_status(ptx, 0,VPC_PS_STOP);
			ptx->notify_call_back(ptx->user_data, VPC_PLAY_FINISH, 0 );
			return 300;
		}
	}

	if( ptx->player_decode_mode_status == 1 && !ptx->wait_surface_ready ){
		/*it's time for opening video device*/
		media_stream *s = 0;
		for ( i=0;i<MAX_STREAM_NUM;i++)
		{
			s = ptx->stream[i];
			if(s && !s->out_device){
				ptx->notify_call_back(ptx->user_data, VPC_OPEN_AV_DEVICE, i);
			}
		}
		ptx->player_decode_mode_status = 2; //device is opened.

		if( ptx->vpc_cur_ps == VPC_PS_PAUSE ){
			do_pause(ptx);
		}
	}
	
	/*begin to render AV*/
	if ( ptx->player_decode_mode_status == 2 )
	{
//		int mm = ptx->urlinfo->moniter_mode;
		TIME_TYPE process_time = INVALID_TS;

		if( ptx->run_render_process ){
			process_time = get_current_time(ptx);
			//printf("process_time=%d\r\n",process_time);
		}
		
// 		if( mm ){
// 			generate_play_strategy(ptx);
// 		}

// 		if(1 )
// 		{
// 			static TIME_TYPE pre_ts = 0;
// 			static FILE *fp = 0;
// 			if(!fp)
// 			{
// 				fp = fopen("d:\\timesource.txt","w");
// 				pre_ts = vpc_gettime();
// 			}
// 			else
// 			{
// 				if( process_time == -1){
// 					process_time = -1;
// 				}
// 				fprintf(fp,"o=%d,t=%d\r\n",process_time,process_time - pre_ts);
// 				pre_ts = process_time;
// 				fflush(fp);
// 				if(ftell(fp)>1024*512)
// 				{
// 					fclose(fp);
// 					fp = 0;
// 				}
// 			}
// 		}
//		printf("now position=%d\r\n",ptx->position);

		need_cpu_cap = 20;
		for (i=0;i<MAX_STREAM_NUM;i++)
		{
			s = ptx->stream[i];
			if(s)
			{
				int ret = 1;
				mutex_lock(ptx->picture_lock);
				ret = s->render(s,process_time);
				mutex_unlock(ptx->picture_lock);
			}
		}
	}

	if( !ptx->run_render_process )
	{
		need_cpu_cap = 20;
		/*compute already buffer time*/
		compute_buffer_time(ptx);

		if( (ptx->vpc_cur_ps == VPC_PS_PLAY && ptx->player_decode_mode_status == 2 && 
			(ptx->play_step	!= Internal_Seeking)) && 
			((ptx->alreay_buffer_time > ptx->should_buffer_time) || 
			(ptx->source && ptx->source->eof(ptx->source))))
		{
			reset_timer(ptx,0);
			do_internal_state_shift(ptx, Internal_Playing);
			ptx->run_render_process = 1;
			need_cpu_cap = 0;
			start_timer( &ptx->buffer_moniter_timer);
			vpc_printf("start play, buffer time is %d ms.\r\n",ptx->alreay_buffer_time);
		}
	}
	return need_cpu_cap;
}

int master_exit(fmp_context *ptx)
{
	//delete stream
	int i;
	media_stream *s;
	for (i=0;i<MAX_STREAM_NUM;i++)
	{	
		s=ptx->stream[i];
		delete_stream(s);
		ptx->stream[i] =0;
	}
	ptx->stream_num = 0;
	return 1;
}

int estimate_buffer_pending(fmp_context *ptx)
{
	int pending = 0;
	int limit = ptx->should_buffer_time/2;

	media_stream *vs = ptx->stream[TM_MEDIA_VIDEO];
	media_stream *as = ptx->stream[TM_MEDIA_AUDIO];

	if( limit > 1500 ){
		limit = 1500;
	}

	if( vs && vs->buffer_time < limit ){
		pending++;
	}
	if( as && as->buffer_time < limit ){
		pending++;
	}
	return pending;
}

/*decode thread*/
int slave_entry(fmp_context *ptx)
{
	media_stream *s;
	int	data_warning = 0;
	int i, ret, need_cpu_cap = 16, both_full = 0;
	/* the last get data time*/
	mutex_lock(ptx->status_lock);

	if( ptx->run_decode_process )
	{
		int src_eof = 0;
		TIME_TYPE ps = INVALID_TS;

		// check if player have too much buffer
		if( ptx->low_latency_mode ){
			loop_timer( &ptx->buffer_moniter_timer);
		}

		if( ptx->run_render_process ){
			ps = get_current_time(ptx);
            //vpc_printf("get_current_time=%d\r\n", ps);
		}

		for (i=0;i<MAX_STREAM_NUM;i++)
		{
			s = ptx->stream[i];
			if(s)
			{
				ret = s->process(s,ps,0);
				if(ret == NO_MORE_DATA )
				{
					data_warning++;
					continue;
				}
				else{
					data_warning = 0;
				}
				
				if(ret != BUFFER_FULL){
					need_cpu_cap = 0; /*rest 0 ms*/
				}
                else{
                    both_full++;
                }
			}
            else{
                both_full++;
            }
		}
		
		src_eof = ptx->source->eof(ptx->source);

		/* if check network condition */
		if( !src_eof ){
			loop_timer(&ptx->check_net_timer);
		}

		if( data_warning > 0)
		{
			if( src_eof )
			{
				for (i=0;i<MAX_STREAM_NUM;i++)
				{
					s = ptx->stream[i];
					if(s)
					{
						if(s->process(s,ps,1) == NO_MORE_DATA )
							s->output(s,1);
					}
				}
				ptx->check_end_of_file = 1;
			}
			else if(  1||!low_level_buffer_ready(ptx) )
			{
				if( ptx->vpc_cur_ps == VPC_PS_PLAY ){
					if( ptx->play_step == Internal_Playing ){
						/* notify data source plugin player in mode*/
						if( ptx->source->customize && ptx->source->customize(ptx->source,"buffer",0,0) ){
							set_request_status(ptx, 0, Internal_Live_Buffer );
							vpc_printf("live stream, don't like buffer status.\r\n");
						}
						else{
							set_request_status(ptx, 0, Internal_Enter_Buffer );
						}
					}
				}
				need_cpu_cap = 50;
			}
			else{
				vpc_printf("data is not so quick in %d ms.\r\n", ret);
				need_cpu_cap = 30;
			}
		}

		if((ptx->player_decode_mode_status == 0) && (both_full == MAX_STREAM_NUM || ptx->check_end_of_file == 1) ){
			ptx->player_decode_mode_status = 1; //audio and video output buffer is full now
		}
	}
	else
	{
		//take a long time rest
		need_cpu_cap = 20;
	}
	mutex_unlock(ptx->status_lock);
	return need_cpu_cap;
}

int slave_exit(fmp_context *ptx)
{
	return 1;
}

void*  vpc_init(NOTIFY_CALLBACK func, void *user_data, void *platform_data)
{
	fmp_context *ptx = (fmp_context *)vpc_mem_alloc(sizeof(fmp_context));
	if( !ptx ) return 0;
	memset( ptx, 0, sizeof(fmp_context));
	
	//save platform data,notice it's just a pointer
	//UI must keep the data
	ptx->platform_data = platform_data;

	load_system_node(ptx);

	/*source demux*/
	ffmpeg_avformat_register(ptx);

	/*video CODEC*/
	ffmpeg_h264_register(ptx);
#ifndef ANDROID_VERSION
	H265_register(ptx);
#endif

	rtmp_stream_register(ptx);
	//srs_rtmp_stream_register(ptx);
	/*audio CODEC*/
	//amrnb_register(ptx);
	//amrwb_register(ptx);

	//http_svr_stream_register(ptx);
    
#ifndef __VPC_DLL__
//	mp4_register(ptx);
//	rtsp_register(ptx);
#ifdef ANDROID_VERSION
	aac_register(ptx);
#endif
//	xvid_register(ptx);
#endif
	
	vpc_load_all_module(ptx,RegisterTools);

	/*initial variable*/
	ptx->user_data			= user_data;
	ptx->notify_call_back	= func;
	ptx->vpc_cur_ps			= VPC_PS_STOP;
	do_internal_state_shift(ptx, Internal_Ready);

	/*initial mutex*/
	ptx->status_lock		= mutex_init();
	ptx->picture_lock		= mutex_init();
    ptx->req_queue_lock		= mutex_init();
	return (void *)ptx;
}

int check_network_status(void *key)
{
	fmp_context *ptx = (fmp_context*)key;
	compute_buffer_time(ptx);
	if( estimate_buffer_pending(ptx) ){
		ptx->notify_call_back(ptx->user_data, VPC_RECV_PENDING, 0);
		vpc_printf("...NOTIFY_RECV_PENDING..., Net it too low?\r\n");
	}
	return 6000;
}


/*
int check_reasonable_time(float t ){
	return (int)(2500*log(t+1));
}
*/

int player_buffer_moniter(void *key)
{
	fmp_context *ptx = (fmp_context*)key;
	int time = 2000;
	compute_buffer_time(ptx);

	if( ptx->should_buffer_time > time ){
		time = ptx->should_buffer_time;
	}

	if( ptx->buffer_full == 2 )
	{
		/* shrink data */
		shrink_buffer(ptx, ptx->alreay_buffer_time - time + 300 );
		ptx->buffer_full = 0;
		return 3000;
	}
	else
	{
		if( ptx->alreay_buffer_time > time ){
			ptx->buffer_full++;
		}
		else{
			ptx->buffer_full = 0;
		}

		if( ptx->buffer_full < 2 ){
			return 3000;
		}
		else{
			/* notify UI, player will shrink data after 2s*/
			ptx->notify_call_back( ptx->user_data, VPC_DELETE_FRAME, 0 );
			vpc_printf("player will shrink data after 2s\r\n");
			return 2000;
		}
	}
}

void vpc_start(void *pc,player_parameter*pp)
{
	fmp_context *ptx = (fmp_context *)pc;
	if( !ptx ) return;
	if(ptx->vpc_cur_ps == VPC_PS_STOP && pp )
	{
		int wait = 1, timeout_ = 0;;
		ptx->urlinfo = pp;
		ptx->should_buffer_time	= pp->should_buffer_time;

		init_timer( &ptx->check_net_timer, 6000, ptx, check_network_status);
		init_timer( &ptx->buffer_moniter_timer, 3000, ptx, player_buffer_moniter);

		reset_request_status(ptx);
		/*now, start a new play*/
		ptx->player = run_player(ptx, (task_entry_t)master_entry, (task_entry_t)master_exit,(task_entry_t)slave_entry,(task_entry_t)slave_exit);
		set_request_status_wait(ptx, 0, VPC_PS_PLAY, &wait );

		/* timeout check */
		while ( wait ){
			vpc_delay(10);
			timeout_ += 10;
			if( timeout_ > 3000 ){
				vpc_printf("vpc start timeout!\r\n");
				break;
			}
		}

		//http server mode, so don't need master and slave thread
        if (ptx->source_node != NULL) {
            if( ptx->source_node->sub_id == HTTPSVR_ID ){
                close_player(ptx->player);
                ptx->player = 0;
            }
        }
	}
	else
	{
        ptx->preload_mode = 0;
		set_request_status( ptx, 0,VPC_PS_PLAY );
	}
}

void vpc_surface_ready(void *pc, int ready){
	fmp_context *ptx = (fmp_context *)pc;
	if( ptx ){
		ptx->wait_surface_ready = !ready;
	}
}

int vpc_load(void *pc,player_parameter*pp)
{
    fmp_context *ptx = (fmp_context *)pc;
    if( !ptx ) return 0;
    
    if(ptx->vpc_cur_ps != VPC_PS_STOP){
        return 0;
    }
    
    //begin to start
    ptx->preload_mode = 1;
    vpc_start(ptx,pp);
    
    return 1;
}

int vpc_seek( void *pc,int ms )
{
	fmp_context *ptx = (fmp_context *)pc;
	int ret = 0;
	if( !ptx ) return 0;
	if( ptx->media_duration )
	{
		if(ms == 0)ms = 1;
		ptx->request_seek_pos = ms;
		ret = 1;
		set_request_status(ptx, 0, Internal_Seeking);
	}
	else
	{
		ptx->request_seek_pos = 0;
		ret = 0;
	}
	return ret;
}

// int vpc_redraw_frame(void *pc)
// {
// 	fmp_context *ptx = (fmp_context *)pc;
// 	media_stream *stream = 0;
// 	if(ptx){
//  		stream = ptx->stream[TM_MEDIA_VIDEO];
//  	}
//  	if(stream)
//  	{
//  		mutex_lock(ptx->picture_lock);
//  		video_render_img(stream,INVALID_TS,1);
//  		mutex_unlock(ptx->picture_lock);
//  		return 1;
//  	}
//  	return 0;
// }

void vpc_pause(void *pc, int wait)
{
	int sync = wait;
	fmp_context *ptx = (fmp_context *)pc;
	if( !ptx ) return;
    if(sync)
    {
        int ret = set_request_status_wait( ptx, 0,VPC_PS_PAUSE,&sync);
        while(sync && ret){
            vpc_delay(30);
        }
    }
    else{
		set_request_status( ptx, 0,VPC_PS_PAUSE);
    }
}

void vpc_stop(void *pc)
{
// 	int i;
// 	media_stream *s;
	fmp_context *ptx = (fmp_context *)pc;
	if( !ptx ) return;

	stop_timer( &ptx->check_net_timer );
	stop_timer( &ptx->buffer_moniter_timer );
	
	/*close source layer*/
	if(ptx->source){ 
		ptx->source->close(ptx->source);
	}
    video_send_decode_command(ptx->stream[TM_MEDIA_VIDEO]);

	/*close player thread*/
	close_player(ptx->player);
	ptx->player = 0;

	reset_request_status(ptx);

	if(ptx->source_node){
		ptx->source_node->release(ptx->source);
	}
	ptx->source = 0;
	ptx->source_node = 0;

	ptx->check_end_of_file = 0;
	ptx->position = 0;
	ptx->seeked_pos = 0;
	ptx->do_seek_point = 0;
    ptx->preload_mode = 0;

	ptx->player_decode_mode_status = 0;
	ptx->run_render_process = 0;
	ptx->run_decode_process = 0;
	do_internal_state_shift(ptx,Internal_Ready);
	ptx->vpc_cur_ps = VPC_PS_STOP; /*important*/

}

void vpc_open_av_device(void *pc, int style, void *parm, int parm_size)
{
	fmp_context *ptx = (fmp_context *)pc;
	media_stream *s = ptx->stream[style];
	if(s)
	{
		if(s->media_style == TM_MEDIA_AUDIO)
		{
			if(!s->out_device) 
			{
				init_audio(ptx,s,parm,parm_size);
			}
		}
		else if(s->media_style == TM_MEDIA_VIDEO)
		{
			if(!s->out_device){
				init_video(ptx,s,parm,parm_size);

				/* notify video device is opened */
				ptx->notify_call_back(ptx->user_data, VPC_PRE_PLAY,0);
			}
		}
	}
}

void vpc_get_media_info(void *pc, media_info* media)
{
	media_stream *audio ,*video;
	fmp_context *ptx = (fmp_context *)pc;
	if( !ptx || !media) return;

	memset( media, 0, sizeof(media_info) );

	audio = ptx->stream[TM_MEDIA_AUDIO];
	video = ptx->stream[TM_MEDIA_VIDEO];

	media->dwDuration = ptx->media_duration;

	if (video)
	{
		video_ext *ext = (video_ext *)video->avext;
		media->video_available = 1;
		media->videoWidth  = ext->width;
		media->videoHeight = ext->height;
		strcpy( media->sz_video_name, video->codecname );
	}

	if(audio) 
	{
		audio_ext *aext = (audio_ext *)audio->avext;
		media->audio_available = 1;
		media->freq		= aext->freq;
		media->format	= aext->bits;
		media->channels	= aext->channels;
		strcpy( media->sz_audio_name, audio->codecname );
	}
}

void vpc_get_status(void *pc,player_status*ps)
{
	fmp_context *ptx	= (fmp_context *)pc;
    media_stream *sa,*sv;
	if( !ptx ) return;
	memset( ps, 0, sizeof(player_status));
    
    sv = ptx->stream[TM_MEDIA_VIDEO];
    sa = ptx->stream[TM_MEDIA_AUDIO];
    
	if(sa){
		ps->a_kbps = sa->cur_kbps;
		ptx->position = sa->use_time;
	}

    if(sv){
		video_ext *ext = (video_ext*)sv->avext;
		ps->fps = ext->fps;
		ps->delivered = ext->delivered;
		ps->skipped   = ext->skipped;
		ps->v_kbps = sv->cur_kbps;
		if( ptx->position < sv->use_time ){
			ptx->position = sv->use_time;
		}
    }

 	ps->cur_play_pos = (uint32_t)(ptx->do_seek_point + (ptx->position));
	//vpc_printf("ptx->seeked_pos= %d, st =%d, ptx->position=%d\r\n",ptx->seeked_pos,st,ptx->position);
	if(ps->cur_play_pos < ptx->do_seek_point)
		ps->cur_play_pos = ptx->do_seek_point;

	/*unit is ms*/
    compute_buffer_time(ptx);
    ps->already_buffer_time = ptx->alreay_buffer_time;

	ps->already_data_time = ptx->got_data_time+ptx->do_seek_point;

	if( ptx->media_duration && ps->cur_play_pos>ptx->media_duration ){
		ps->cur_play_pos = ptx->media_duration;
	}
}

int  vpc_get_player_state(void *pc)
{
	fmp_context *ptx= (fmp_context *)pc;
	if(ptx)
	{
		int ps = ptx->vpc_cur_ps; 
		return ps;
	}
	return 0;
}

void vpc_audio_mute(void *pc, int mute)
{
	fmp_context *ptx	= (fmp_context *)pc;
	media_stream * audio = ptx->stream[TM_MEDIA_AUDIO];
	if(audio){ 
		set_audio_mute(audio,mute);
	}
// 	if( ptx->source->customize && ptx->source->customize(ptx->source,"buffer",0,0) ){
// 		set_request_status(ptx, 0, Internal_Live_Buffer );
// 		vpc_printf("live stream, don't like buffer status.\r\n");
// 	}
// 	else{
// 		set_request_status(ptx, 0, Internal_Enter_Buffer );
// 	}
}

void vpc_only_render_I_frame(void *pc, int flag)
{
	fmp_context *ptx	= (fmp_context *)pc;
	media_stream * video = ptx->stream[TM_MEDIA_VIDEO];
	if(video){
		video_ext *ext = (video_ext*)video->avext;
		ext->only_I_frame = flag;
	}
}

char* vpc_get_version(void *pc)
{
	return "0.0.1";
}

/*only one time*/
void vpc_quit(void*pc)
{
	fmp_context *ptx = (fmp_context *)pc;
	if( !ptx ) return;
	/*release mutex */
	mutex_uninit(ptx->status_lock);
	mutex_uninit(ptx->picture_lock);
    mutex_uninit(ptx->req_queue_lock);
	UnRegisterTools(pc);
	vpc_mem_free(ptx);
	/*good bye*/
}

char *status_tostring(int s);

/************************************************************************/
/*                            helper function                           */
/************************************************************************/

void reset_request_status(fmp_context *ptx)
{
	int i=0;
	req_msg msg = {VPC_PS_UNKNOWN};
   
    mutex_lock(ptx->req_queue_lock);
	for (;i<MAX_PLAYER_STATUS_QUEUE;i++)
	{
		msg = ptx->status_request_queue[i];
		if(msg.wait) *msg.wait = 0;
		ptx->status_request_queue[i].cur = VPC_PS_UNKNOWN;
		ptx->status_request_queue[i].req = VPC_PS_UNKNOWN;
		ptx->status_request_queue[i].wait = 0;
	}
	ptx->processed_index	= 0;
	ptx->request_index	= 0;
    mutex_unlock(ptx->req_queue_lock);
}

req_msg get_request_status(fmp_context*ptx)
{
	req_msg msg = {VPC_PS_UNKNOWN};
    mutex_lock(ptx->req_queue_lock);

	if( ptx->status_request_queue[ptx->processed_index].req )
	{
		msg = ptx->status_request_queue[ptx->processed_index];

        ptx->status_request_queue[ptx->processed_index].cur = VPC_PS_UNKNOWN;
		ptx->status_request_queue[ptx->processed_index].req = VPC_PS_UNKNOWN;
		ptx->status_request_queue[ptx->processed_index].wait = 0;
		ptx->processed_index++;
		if(ptx->processed_index>=MAX_PLAYER_STATUS_QUEUE)
		{
			ptx->processed_index = 0;
		}
	}

    mutex_unlock(ptx->req_queue_lock);
	return msg;
}

int set_request_status(fmp_context*ptx, int cur,int req)
{
	return set_request_status_wait(ptx, cur,req,0);
}

int set_request_status_wait(fmp_context*ptx, int cur,int req, int *wait)
{
    int ret = 0;
    mutex_lock(ptx->req_queue_lock);
	if( ptx->status_request_queue[ptx->request_index].req == VPC_PS_UNKNOWN )
	{
        ptx->status_request_queue[ptx->request_index].cur = cur;
		ptx->status_request_queue[ptx->request_index].req = req;
		ptx->status_request_queue[ptx->request_index].wait = wait;
		ptx->request_index++;
        
		if(ptx->request_index>=MAX_PLAYER_STATUS_QUEUE)
		{
			ptx->request_index = 0;
		}
        ret = 1;
	}
    else{
        vpc_printf("statue queue is full, request status %s is discarded\r\n",status_tostring(req));
        if( wait)
            *wait = 0;
    }
    mutex_unlock(ptx->req_queue_lock);
	return ret;
}

void compute_stream_kbps(media_stream *s, int consumed)
{
	int interval = 0;
	if( !s->decoded_bytes ){
		s->flow_time = s->use_time;
	}
	s->decoded_bytes += consumed;
	interval = s->use_time - s->flow_time;

	if( interval >= FLOW_COMPUTE_INTERVAL )
	{
		int32_t average = 0;
		int32_t	instant = s->decoded_bytes;
		instant *= 8;
		instant /= interval;
		s->average_bitrate[s->idx++] = instant;
		if( s->idx >= FLOW_SAMPLE_SIZES ){
			s->idx = 0;
		}
		s->cur_kbps = instant;

		/* reset for next second */
		s->flow_time = 0;
		s->decoded_bytes = 0;
	}
}

int video_send_decode_command(media_stream *stream )
{
    if( stream )
    {
        video_ext *vp	 = (video_ext*)stream->avext;
        util_sem_post(vp->video_semaphore);
    }
	return 1;
}

THREAD_RETURN_TYPE VPC_API video_decode_thread(void *p)
{
	media_stream *s = (media_stream*)p;
	video_ext *vp	 = (video_ext*)s->avext;
	while (1)
	{
		util_sem_wait(vp->video_semaphore);
		if(!vp->video_running) 
			break;
		mutex_lock(vp->decode_section);
		vp->routine_ret = vp->decode_routine(s,1);
		mutex_unlock(vp->decode_section);
		vp->video_decode_pending = 0;
	}
	return 0;
}

int low_level_buffer_ready( fmp_context *ptx )
{
	audio_device *dev = (audio_device*)ptx->stream[TM_MEDIA_AUDIO];
	if( dev ){
		return get_audio_pending_buffer_size(dev);
	}
	return 0;
}

int create_stream(fmp_context *ptx,media_stream **st, AV_FMT *fmt, int blocksize,int ext_size)
{
	int ret, codec_err = VPC_NO_AUDIO_CODEC;
	media_stream *s = vpc_mem_alloc(sizeof(media_stream));
	if(s)
	{
		*st = s;
		memset(s, 0, sizeof(*s));
		s->avext = vpc_mem_alloc(ext_size);

		if(!s->avext)
		{
			process_error(ptx,VPC_OUT_OF_MEMORY);
			return 0;
		}
		memset( s->avext, 0, ext_size);

		if(!(s->mempool = mmg_init_queue(blocksize)))
		{
			process_error(ptx,VPC_OUT_OF_MEMORY);
			return 0;
		}
		
		strcpy(s->codecname, fmt->codecname);
		s->stream_speed.num = s->stream_speed.den = 1;

		if(fmt->nType == TM_MEDIA_VIDEO)
			codec_err +=1;

		if( !(s->codec_node = find_codec( ptx,fmt->nFourCC)))
		{
			process_error(ptx,codec_err);
			return 0;
		}
		s->codec = s->codec_node->create(fmt->extradata,fmt->extrasize);
		if(!s->codec)
		{
			process_error(ptx,codec_err);
			return 0;
		}
		s->codec->codecid = fmt->nFourCC;
		ret = s->codec->open(s->codec,fmt->extradata,fmt->extrasize);
		if(!ret)
		{
			process_error(ptx,codec_err);
			return 0;
		}
		s->data_lock = mutex_init();
        if(fmt->extrasize>0 && fmt->extrasize<32)
        {
            if(fmt->nType == TM_MEDIA_AUDIO)
            {
				audio_ext *ax = (audio_ext*)s->avext;
				memcpy(ax->extradata,fmt->extradata,fmt->extrasize);
            }
        }
	}
	return 1;
}

void delete_stream(media_stream *s)
{
	if(!s) return;
	if(s->release_ext)
		s->release_ext(s);

	if(s->codec)
		s->codec->close(s->codec);

	if(s->codec_node)
		s->codec_node->release(s->codec);

	mmg_uninit_queue( s->mempool);

	/*close device*/
	if(s->release){
		s->release(s);
	}

	vpc_mem_free(s->avext);
	mutex_uninit(s->data_lock);
	vpc_mem_free(s);
}

int32_t init_media_fmt(fmp_context *ptx, uint8_t *info, uint32_t len )
{
    media_info media_spec={0x00};
	media_stream *s = 0;
	MEDIA_FMT *pMF = (MEDIA_FMT*)info;
//	int video_behind_audio = 0;
	int i,ret;

	if( pMF->cnt<1 )
	{
		process_error(ptx, VPC_MEDIA_SPEC_ERROR);
		return 0;
	}
//	if( ptx->stream[TM_MEDIA_AUDIO] && !ptx->stream[TM_MEDIA_VIDEO]){
//		video_behind_audio = 1;
//	}

	ptx->media_duration = pMF->file_length;

	for (i=0;i<pMF->cnt;i++)
	{
		if( pMF->fmt[i].nType == TM_MEDIA_AUDIO && !ptx->stream[TM_MEDIA_AUDIO] )
		{	
			audio_ext *ext = 0;
			ret = create_stream(ptx, &s, &pMF->fmt[i], 1024*4,sizeof(audio_ext));
			if(!ret)
			{
				delete_stream(s);
				continue;
			}

			s->media_style	= TM_MEDIA_AUDIO;
			s->process		= audio_stream_process;
			s->output		= audio_output;
			s->release		= audio_release;
			s->release_ext	= audio_release_ext;
			s->render		= audio_render;
			s->process_speed= audio_speed;

			ext = (audio_ext*)s->avext;
			ext->channels	= pMF->fmt[i].av.a.nChannels;
			ext->freq		= pMF->fmt[i].av.a.nSamplerate;
			ext->bits		= pMF->fmt[i].av.a.nBitsPerSample;/*need bitaudio_sp.nBytesPerSample*8*/
			ext->decode_buf = vpc_mem_alloc(8192);
			ext->decode_buf_size = 8192;
			ext->bytes_per_second = ext->channels*ext->freq*(ext->bits/8);

			/*set audio decoder buffer, maybe never use it*/
			s->dc.out[0] = ext->decode_buf;
			s->i_arrived = 1;
			ptx->stream[TM_MEDIA_AUDIO] = s;
			ptx->stream_num++;
            
            //open audio device now
			if(s && !s->out_device){
				ptx->notify_call_back(ptx->user_data, VPC_OPEN_AV_DEVICE, TM_MEDIA_AUDIO);
            }
		}
		if( pMF->fmt[i].nType == TM_MEDIA_VIDEO && !ptx->stream[TM_MEDIA_VIDEO] )
		{
			video_ext *ext = 0;
			ret = create_stream(ptx, &s, &pMF->fmt[i], 128*1024,sizeof(video_ext));
			if(!ret)
			{
				delete_stream(s);
				continue;
			}
			s->media_style	= TM_MEDIA_VIDEO;
			s->process		= video_stream_process;
			s->process_speed= video_speed;
			s->output		= video_output;
			s->release_ext	= video_release_ext;
			s->render		= video_render;
			s->release		= video_release;
			s->i_arrived	= 0;

			ext = (video_ext*)s->avext;
			ext->width  = pMF->fmt[i].av.v.nWidth;
			ext->height = pMF->fmt[i].av.v.nHeight;
            
			ext->widthAlign2 = makeAlgin2(ext->width);
			ext->heightAlign2 = makeAlgin2(ext->height);

// 			ext->buf_frame_cnt = 5;
// 			if(ptx->urlinfo->moniter_mode )
				ext->buf_frame_cnt = 3;

			//create a thread decode
			ext->decode_section = mutex_init();
			ext->video_running = 1;
			ext->video_semaphore = util_sem_init();
			ext->decode_routine = (int(*)(void*,int))video_decode_output_routine;
			ext->routine_ret = ALL_OK;
			ext->video_handle = vpc_thread_create(&ext->video_thread_id, video_decode_thread,s);

			ext->frame = vpc_mem_alloc(sizeof(Picture)*ext->buf_frame_cnt);	
			
			ext->avail = ext->buf_frame_cnt;
			ext->pre_frame = -1;
			ext->avail_section = mutex_init();
			ptx->stream[TM_MEDIA_VIDEO] = s;
			ptx->stream_num++;
		}

	}
	if( ptx->stream_num == 0 )
	{
		process_error(ptx, VPC_MEDIA_SPEC_ERROR);
		return 0;
	}

	vpc_get_media_info(ptx, &media_spec );
	ptx->notify_call_back( ptx->user_data, VPC_NOTIFY_MEDIA_INFO, (uint32_t)&media_spec );

	//if( video_behind_audio ){
		//ptx->player_decode_mode_status = 1;
	//}
		
	return 1;
}

const node_boot_func * find_codec(fmp_context *ptx, int32_t id)
{
	tools_def *p = ptx->tools_start;
	//bool_t find = 0;
	while (p)
	{
		if( p->p->category_id == CODEC_ID && p->p->sub_id == id)
		{
			if( p->p->available(id))
			{
				return p->p;
			}
		}
		p=p->next;
	}
	return 0;
}

void fill_packet_info(struct media_stream *s, uint8_t *buffer, int size)
{
	decode_context	*pdc = &s->dc;
	stream_pack		*psp = &s->cur_pkt;

	memcpy(psp, buffer, sizeof(stream_pack));
	psp->buffer		= buffer + sizeof(stream_pack);
	psp->buf_size	= size - sizeof(stream_pack);

	s->output_offset = 0;
	pdc->in_stream	= psp->buffer;
	pdc->in_len		= psp->buf_size;
	pdc->out_size	= 0;

	pdc->in_ts      = psp->dts;
	pdc->stream_time = psp->stream_time;

	pdc->out_ts     = psp->dts;
	s->use_time		= psp->stream_time;
	s->buffer_time = s->recv_time - s->use_time;
   // vpc_printf("use time=%d, bufftime=%d,psp->dts=%d\r\n",  psp->stream_time, s->buffer_time, psp->dts);
    
	psp->vtag.dt  	= vpc_gettime();
}

/* 
 *	return 0, no data
    return 1, get data
*/
int get_media_stream_buffer(struct media_stream *s, TIME_TYPE stop_ts, void (*handler)( struct media_stream *s) )
{
	decode_context	*pdc = &s->dc;
	stream_pack		*psp = &s->cur_pkt;
	uint8_t			*out_buffer = 0;
	int ret = 0, get_len, pkt_no = 0;
	if( 0&&handler ){
		/* just want to skip some frames, so don't decode every frame */
		mp_val last={0},curvalue={0};
		do
		{
			stream_pack * sp = 0;
			mutex_lock(s->data_lock);
			mmg_end_get_buffer2(s->mempool);
			//stream pack
			ret = mmg_access_next_chunk(s->mempool,&last,&curvalue);
			sp = (stream_pack*)curvalue.buf;
			mutex_unlock(s->data_lock);


			if(!ret){
                vpc_printf("warning, data time error\r\n");
				break;
			}
			else
			{
				vpc_printf("sp->stream_time=%d\r\n", sp->stream_time);
				fill_packet_info(s,curvalue.buf, curvalue.buf_size);
                //vpc_printf("current pdc stream stime is %d\r\n",pdc->stream_time);

				if( sp->frame_style == 'I')
				{
					handler(s);
					mutex_lock(s->data_lock);
					mmg_skip_to_here(s->mempool,&last);
					mutex_unlock(s->data_lock);

					memset(&last,0,sizeof(last));
					memset(&curvalue,0,sizeof(curvalue));
					vpc_printf("skip I frame, save cpu time\r\n");
				}
				else{
					last = curvalue;
				}

				if( pdc->stream_time >= stop_ts ) {
					vpc_printf("sp->stream_time >= stop_ts in preprocess \r\n");
					break;
				}
			}

		} while (1);
	}

	do
	{
        static TIME_TYPE prea = 0;
        TIME_TYPE now = vpc_gettime();
        if( !prea ) prea = vpc_gettime();
        if( now - prea < 1 ){
            now = 0;
        }
        prea = now;
		/*get new data pkt*/
		mutex_lock(s->data_lock);
		mmg_end_get_buffer2(s->mempool);
		ret = mmg_begin_get_buffer2(s->mempool, (uint8_t**)&out_buffer, (unsigned int *)&get_len);
		mutex_unlock(s->data_lock);
		if(!ret){	
			return 0;
		}
		fill_packet_info(s, out_buffer, get_len);
		if( pdc->stream_time >= stop_ts ) {
			return 1;
		}
		if( handler){
			handler(s);
		}

	}while(1);
	return 0;
}

int audio_stream_process(struct media_stream *s, TIME_TYPE ps, int eof )
{
	int ret = 0, get_len, pkt_no=1;
	decode_context	*pdc = &s->dc;
	stream_pack		*psp = &s->cur_pkt;
	audio_ext		*ext = (audio_ext*)s->avext;

	if(s->wait_for_output)
	{
		if(s->output(s,0) == BUFFER_FULL )
			return BUFFER_FULL;
		s->wait_for_output = 0;
	}

	if( pdc->out[0] != ext->decode_buf ){
		vpc_printf("error\r\n");
		pdc->out[0] = ext->decode_buf;
	}

	if( psp->dts + 500 < ps ){
		pkt_no = 10;
	}
	else if(psp->dts < ps ){
		pkt_no = 3;
	}
	
	while (pkt_no--)
	{
		if( pdc->in_len == 0 ) 
		{
			if( !get_media_stream_buffer(s,0, 0) )
				return NO_MORE_DATA;
#if 0
			/*get new data pkt*/
			mutex_lock(s->data_lock);
			mmg_end_get_buffer2(s->mempool);
			ret = mmg_get_buffer(s->mempool, (uint8_t*)psp, (unsigned int *)&get_len );
			if(!ret){	
				mutex_unlock(s->data_lock);
				return NO_MORE_DATA;
			}
			ret = mmg_begin_get_buffer2(s->mempool, (uint8_t**)&psp->buffer, (unsigned int *)&psp->buf_size);
			mutex_unlock(s->data_lock);

			s->output_offset = 0;

			/*it's time for change stream speed*/
			s->process_speed(s,0);//just set speed
			pdc->in_stream	= psp->buffer;
			pdc->in_len		= psp->buf_size;
			pdc->in_ts      = psp->ts;
			pdc->out_ts     = psp->ts;
			s->use_time		= psp->ts;
			psp->vtag.dt  	= vpc_gettime();
#endif 
		}

		/*begin to decode*/
		do 
		{
			int consumed = s->codec->decode(s->codec,pdc);
			if( consumed < 0 )
			{
				pdc->in_len = 0;
				return CODEC_ERROR;
			}
			
			if( consumed >0 )
			{
				compute_stream_kbps(s, consumed );

				pdc->in_stream	+= consumed;
				pdc->in_len		-= consumed;
				if( pdc->in_len < 0) pdc->in_len = 0;
			}
			
			//add or delete some data according to speed
			s->process_speed(s,1);
			s->wait_for_output = 1;

			/*output it to av buffer*/
			if (s->output(s,0) == BUFFER_FULL)
			{	
				return BUFFER_FULL;
			}
			s->wait_for_output = 0;

		} while( pdc->in_len > 0 );
	}
	return ALL_OK;
}

static int video_decide_skip_frame(struct media_stream *s, TIME_TYPE pkt_ts, TIME_TYPE ps )
{
	int ret = 1;
	int diff = ps - pkt_ts;
	s->skip_frame = skip_nothing;
    

	if( ps == INVALID_TS ){
		return ret;
	}
    
    //vpc_printf("++++++++++++++++++video_decide_skip_frame=%d\r\n", diff);

    /*
	if( diff> 1000 ){
        vpc_printf("skip to I frame, pkt ts =%d, ps=%d\r\n", pkt_ts, ps);
		s->skip_frame = skip_to_I_frame;
	}
	else */
    if( diff >200){
		s->skip_frame = skip_video_frame;
	}
	else if(diff > 80){
 		s->skip_frame = skip_show_frame;
	}
	return ret;
}

int video_decode_output_routine(media_stream *s, int direct_output)
{
//	int ret = 0;
	decode_context	*pdc = &s->dc;
//	stream_pack		*psp = &s->cur_pkt;
	if( pdc->in_len == 0 )
	{
		pdc->in_len = 0;
		vpc_printf("video bitstream pdc->in_len = 0\r\n");
	}
//	do 
	{
		int consumed = s->codec->decode(s->codec,pdc);
		if(consumed<0)
		{
			if(consumed == -1 ) //no more video YUV
				return NO_MORE_DATA;
			pdc->in_len = 0;
			return CODEC_ERROR;
		}

		if( consumed > 0 )
		{
			compute_stream_kbps(s, consumed);

			pdc->in_stream	+= consumed;
			pdc->in_len		-= consumed;
			if( pdc->in_len < 0 ) pdc->in_len =0;
		}

		//add or delete some data according to speed
		s->process_speed(s,1);

		if(!direct_output)
			s->wait_for_output = 1;

		/*output it to av buffer*/
		if (s->output(s,0) == BUFFER_FULL)
		{	
			return BUFFER_FULL;
		}
		s->wait_for_output = 0;
	}
//	} while(pdc->in_len);
	return ALL_OK;
}

int video_stream_process(struct media_stream *s, TIME_TYPE ps, int eof)
{
	int ret = 0, pkt_no=1;
	decode_context	*pdc = &s->dc;
	stream_pack		*psp = &s->cur_pkt;
	video_ext *vp	 = (video_ext*)s->avext;

	/* only in  multi-thread decode */
	if( vp->video_decode_pending )
		return ALL_OK;

	if(s->wait_for_output)
	{
		if(s->output(s,0) == BUFFER_FULL )
			return BUFFER_FULL;
		s->wait_for_output = 0;
	}
// 	{
// 		static FILE *fp = 0;
// 		if(!fp)
// 			fp = fopen("d:\\skipb.txt","w");
// 		fprintf(fp,"%d---%d\r\n",ps, psp->ts);
// 		fflush(fp);
// 	}
	pkt_no = video_decide_skip_frame(s, psp->dts, ps);
	while (pkt_no--)
	{

		if( pdc->in_len == 0 )
		{
			ret = get_media_stream_buffer(s, 0, 0);
			if( !ret )
			{
				if(!eof)
					return NO_MORE_DATA;
				else
					memset(psp,0,sizeof(*psp));
				
				// no more data
				pdc->in_stream	= 0;
				pdc->in_len		= 0;
				s->skip_frame = skip_nothing;
			}
#if 0
			/*get new data pkt*/
			mutex_lock(s->data_lock);
			mmg_end_get_buffer2(s->mempool);
			ret = mmg_get_buffer(s->mempool, (uint8_t*)psp, (unsigned int *)&get_len );
			if(!ret)
			{	
				mutex_unlock(s->data_lock);

				if(!eof)
					return NO_MORE_DATA;
				else
					memset(psp,0,sizeof(*psp));

				//need to flush video decoder!
			}
			ret = mmg_begin_get_buffer2(s->mempool, (uint8_t**)&psp->buffer, (unsigned int *)&psp->buf_size);
			mutex_unlock(s->data_lock);
			s->output_offset = 0;

			if(ret)
			{
				/*it's time for change stream speed*/
				s->process_speed(s,0);//just set speed
				pdc->in_ts      = psp->ts;
				s->use_time		= psp->ts;
				pdc->in_stream	= psp->buffer;
				pdc->in_len		= psp->buf_size;
				psp->vtag.dt    = vpc_gettime();
			}
			else
			{
			// psp buffer maybe 0, because of end of stream
				pdc->in_stream	= 0;
				pdc->in_len		= 0;
				s->skip_frame = skip_nothing;
			}
#endif //0
			if( vp->only_I_frame && psp->frame_style != 'I' && vp->already_ready > 10 ){
				 pdc->in_len = 0;
				 pkt_no++;
				 continue;
			}
            
			switch (s->skip_frame)
			{
			case skip_video_frame:
				{
					if( psp->skippable)
					{
						pdc->in_len = 0;
						s->skip_frame = skip_nothing;
						vpc_printf("skip video frame\r\n");
						return ALL_OK;
					}
				}
				break;
			case skip_to_I_frame:
				{
					if ( psp->frame_style != 'I' ){
						pdc->in_len = 0;
						pkt_no++;
						continue;
					}
					else{
						s->skip_frame = skip_nothing;
						return ALL_OK;
					}
				}
				break;
                case skip_show_frame:
                    vpc_printf("skip show frame\r\n");
                    break;
            default:
                    break;
			}
		}
		/* begin to decode and output */
		if( vpc_get_cpu_count() > 2 )
		{
            /*use multi-thread decode*/
			if( video_buffer_avail(s))
			{
				if(vp->routine_ret == NO_MORE_DATA || vp->routine_ret == CODEC_ERROR )
					return vp->routine_ret;
				vp->video_decode_pending = 1;
				video_send_decode_command(s);
				return ALL_OK;
			}
			else
			{
				return BUFFER_FULL;
			}
		}
		else
		{
			ret = video_decode_output_routine(s,0);
// 			if(ret != ALL_OK)
				return ret;
		}
	}
	return ALL_OK;
}

void video_speed(struct media_stream *s, int step)
{
	if(s->requst_change_speed && step==0) //change speed
	{
		s->stream_speed = s->request_speed;
		s->requst_change_speed = 0;
	}
}

int video_output(struct media_stream *s,int finish )
{
	int ret;
	decode_context	*pdc = &s->dc;
	video_ext		*ext = (video_ext *)s->avext;

	if(!finish)
	{
		if( !pdc->out[0]) 
			return ALL_OK;
		ext->delivered++;
		if(s->skip_frame == skip_nothing)
		{
			ret = video_push_image(s, pdc, pdc->out_ts, &s->cur_pkt.vtag );
			if(!ret) return BUFFER_FULL;
		}
		else
		{
			ext->skipped++;
			return ALL_OK;
		}
	}
	else
	{
		//end of video push
		ret = video_push_image(s,0,INVALID_TS,0);
	}

	return ALL_OK;
}

void video_release(media_stream *s)
{
	uninit_video(s);
}

void video_release_ext(struct media_stream *s)
{
	video_ext *ext = (video_ext*)s->avext;

	ext->video_running = 0;
	util_sem_post(ext->video_semaphore);
	vpc_thread_destory(&ext->video_handle, ext->video_thread_id);

	vpc_mem_free(ext->bufferbase);
	vpc_mem_free(ext->frame);
	vpc_mem_free(ext->sharpTempBuf);
	if(ext->avail_section)
	{
		mutex_uninit(ext->avail_section);
		ext->avail_section = 0;
	}
	if(ext->decode_section)
	{
		mutex_uninit(ext->decode_section);
		ext->decode_section = 0;
	}
	if(ext->video_semaphore)
	{
		util_sem_uninit(ext->video_semaphore);
		ext->video_semaphore = 0;
	}
	ext->bufferbase = 0;
	ext->frame = 0;
}

int video_render(media_stream *s, TIME_TYPE ps)
{	
// 	if(s->skip_frame)
// 		return 1;
	return video_render_img(s,ps,1/*s->skip_frame == skip_nothing*/);
}

int audio_render(media_stream *s, TIME_TYPE ps)
{
	if( ps != INVALID_TS ) 
		play_audio_pkt(s->out_device);
	return 1;
}

void send_audio_pkt(struct media_stream *s, TIME_TYPE base, ao_pkt *out)
{
	TIME_TYPE offset = s->output_offset;
	audio_ext *ext = (audio_ext*)s->avext;
	offset = 1000*offset/ext->bytes_per_second;
	offset = offset * s->stream_speed.num/ s->stream_speed.den;
	//push it to audio device
	out->ref_time = base+offset;
	out->speed = s->stream_speed;
	push_audio_pkt(s->out_device,out);
	memset(out,0,sizeof(*out));
}

void audio_speed(struct media_stream *s, int step)
{
	decode_context	*pdc = &s->dc;
	audio_ext *ext = (audio_ext*)s->avext;

	if(s->requst_change_speed && step==0) //change speed
	{
		ao_pkt *out = &ext->out_pkt;
		if(out->ao_fill)
			send_audio_pkt(s,s->cur_pkt.dts,out);
		out->ao_fill = 0;
		s->stream_speed = s->request_speed;
		s->requst_change_speed = 0;
	}
	if( step == 1 ) //convert data
	{
		if( s->stream_speed.den != s->stream_speed.num)
		{
			uint32_t format_bytes	= (ext->bits>>3)*ext->channels;
			uint32_t samples = pdc->out_size/format_bytes;		
			samples = samples*s->stream_speed.den/s->stream_speed.num;
			pdc->out_size = samples*format_bytes;
//			memset(pdc->out[0],0,pdc->out_size);
		}
	}
}


int audio_output(struct media_stream *s,int finish)
{

//	int ret = 0;
	audio_ext		*ext = (audio_ext*)s->avext;
	decode_context	*pdc = &s->dc;
	stream_pack		*psp = &s->cur_pkt;
	ao_pkt			*out = &ext->out_pkt;
	if(finish)
	{
		if(out->ao_fill)
			send_audio_pkt(s,psp->dts,out);
		push_audio_pkt(s->out_device,0);
		return ALL_OK;
	}
	while (pdc->out_size)
	{
		int used;
		if(!out->ao_buf && !get_audio_buffer(s->out_device,out))
		{
			return BUFFER_FULL;
		}
		used = out->ao_size - out->ao_fill;
		used = used > pdc->out_size?pdc->out_size:used;

		if( s->stream_speed.den == s->stream_speed.num )
		{
			if(ext->mute)
				memset(out->ao_buf + out->ao_fill,0, used);
			else 
				memcpy(out->ao_buf + out->ao_fill,pdc->out[0], used);
		}
		else
			memset(out->ao_buf + out->ao_fill,0, used);

		pdc->out[0]		+= used;
		pdc->out_size	-= used;
		out->ao_fill	+= used;
		s->output_offset+= used;

		if ( out->ao_fill >= out->ao_size)
		{
			send_audio_pkt(s,psp->dts,out);
		}
	}
	pdc->out[0] = ext->decode_buf;
	return ALL_OK;
}

void audio_release(media_stream *s)
{
	uninit_audio(s);
}

void audio_release_ext(struct media_stream *s)
{
	audio_ext *ext = (audio_ext*)s->avext;
	if(ext->decode_buf)
		vpc_mem_free(ext->decode_buf);
	ext->decode_buf = 0;
	ext->decode_buf_size = 0;
}

void process_error(fmp_context *ptx, int err)
{
	ptx->notify_call_back( ptx->user_data, err, 0);
}


TIME_TYPE smooth_media_time(struct media_stream *s, TIME_TYPE dts )
{
	TIME_TYPE retTime = 0;
	int av_diff = 0;

	if( !s->prev_time_stamp ){
		s->prev_time_stamp = dts;
	}

	av_diff = (int)(dts - s->prev_time_stamp);

	if( av_diff < 0){
		av_diff = av_diff;
	}

	if( abs(av_diff) > 500 ){//500 ms
		av_diff = s->diff;
	}
//	vpc_printf("av_diff=%d\r\n", av_diff);
// 	if( media == TM_MEDIA_AUDIO && av_diff == 0 ){
// 		vpc_printf("----------audio av_diff=%d\r\n",av_diff);
// 	}
// 
// 	if( media == TM_MEDIA_VIDEO && av_diff == 0 ){
// 		vpc_printf("----------video av_diff=%d\r\n",av_diff);
// 	}

	retTime = s->recv_time;
	s->recv_time += av_diff;
    //vpc_printf("stream time=%d, usetime =%d,previous dts =%d, current dts =%d, av_diff=%d\r\n", s->recv_time, s->use_time,s->prev_time_stamp, dts, av_diff);
    s->prev_time_stamp = (TIME_TYPE)dts;

	if( av_diff > 0 ){
		s->diff = av_diff;
	}
	return retTime;
}


int fill_sample(fmp_context *ptx, stream_pack *sp)
{
	media_stream *s=ptx->stream[sp->style];
	if(s)
	{
		if(s->i_arrived || (sp->frame_style=='I' ? ( s->i_arrived = 1 ) : 0))
		{
			//不需要精确定位
#if 0
			if(0 && sp->ts+ptx->seeked_pos < ptx->do_seek_point)
			{
				//it's a old data,discard it!
				if( sp->style == TM_MEDIA_VIDEO && sp->frame_style != 'B')
				{
					//if it is video data,just put it into decoder
					decode_context dc;
					dc.in_stream = sp->buffer;
					dc.in_len = sp->buf_size;
					dc.in_ts  = sp->ts;
					s->codec->decode(s->codec,&dc);
				}
				//s->stream_time_offset = sp->ts;
				return 0;
			}
#endif
			sp->stream_time = smooth_media_time(s, sp->dts);
            
			sp->vtag.rt = vpc_gettime();
			mutex_lock(s->data_lock);

			if( mmg_start_put(s->mempool, sizeof(stream_pack) + sp->buf_size ) )
			{
				mmg_put_data(s->mempool, (uint8_t*)sp,sizeof(stream_pack));
				mmg_put_data(s->mempool, (uint8_t*)sp->buffer,sp->buf_size );
				mmg_end_put(s->mempool);
			}
			mutex_unlock(s->data_lock);
			if( sp->frame_style == 'I' ){
				;//vpc_printf("sp->frame_style I Frame\r\n");
			}
// 			{
// 				static FILE *fp = 0;
// 				if(!fp)
// 				fp = fopen("d:\\d.txt","wb");
// 				if(sp->style == TM_MEDIA_VIDEO)
// 					fprintf(fp,"receive video data ts=%d\r\n", s->recv_time);
// 				if(sp->style == TM_MEDIA_AUDIO)
// 					fprintf(fp,"receive audio data ts=%d\r\n", s->recv_time);
// 			}

			
			
// 			if(sp->style == TM_MEDIA_VIDEO)
// 			{
/*				static FILE *fp = 0;*/
// 				uint8_t head[4]={0};
// 				uint8_t *ps = &sp->buf_size;
// 				if( !fp )
// 					fp = fopen("d:\\bbb.264","wb");
				
// 				if( sp->frame_style == 'I')
// 					head[0] = 1;
// 				head[1] = ps[2];
// 				head[2] = ps[1];
// 				head[3] = ps[0];
// 				fwrite(head,4,1,fp);
// 				fwrite(sp->buffer,sp->buf_size,1,fp);
// 				fflush(fp);
// 			}
			
		}
	}
	return 1;
}

void compute_buffer_time(fmp_context *ptx)
{
	int i = 0;
	media_stream *s = 0;
	TIME_TYPE already_buffer_time = ~0x0;
	for (i = 0;i < MAX_STREAM_NUM; i++ )
	{
		s = ptx->stream[i];
		if(s )
		{
            s->buffer_time = s->recv_time - s->use_time;
            if( already_buffer_time > s->buffer_time )
            {
                already_buffer_time = s->buffer_time;
                ptx->got_data_time = s->recv_time;
            }
		}
	}

	if( already_buffer_time == ~0x0){
		already_buffer_time = 0;
	}
	ptx->alreay_buffer_time = already_buffer_time;
}

char *status_tostring(int s)
{
    switch (s) {
        case VPC_PS_PLAY:
            return "VPC_PS_PLAY";
		case VPC_PS_PAUSE:
			return "VPC_PS_PAUSE";
		case VPC_PS_STOP:
			return "VPC_PS_STOP";
		default:
			return "VPC_PS_UNKNOWN";
    }
}

void do_internal_state_shift(fmp_context *ptx, int req )
{
	switch ( req )
	{
	case Internal_Ready:
		ptx->play_step = Internal_Ready;
		break;
	case Internal_Connecting:
		ptx->play_step = Internal_Connecting;
		break;
	case Internal_Media_Got:
		ptx->run_decode_process	= 1;
		ptx->play_step = Internal_Media_Got;
		break;
	case Internal_Enter_Buffer:
		{
			media_stream *audio = ptx->stream[TM_MEDIA_AUDIO];
			if(audio) pause_audio(audio->out_device);

			ptx->run_decode_process	= 1;
			ptx->run_render_process = 0;
			ptx->play_step = Internal_Enter_Buffer;
			vpc_printf("because of lack of data,enter buffer mode \r\n");
			/* notify buffer message*/
			if( ptx->low_latency_mode )
			{
				ptx->should_buffer_time *= 2;
				if( ptx->should_buffer_time > LOW_LATENCY_BUFFER_LIMIT){
					ptx->should_buffer_time = LOW_LATENCY_BUFFER_LIMIT;
				}
				vpc_printf("adjust the buffer time to %d.\r\n",ptx->should_buffer_time);
				ptx->notify_call_back( ptx->user_data, VPC_LIVE_BUFFER, 0);
			}
			else
			{
				ptx->notify_call_back( ptx->user_data, VPC_START_BUFFER_DATA, 0);
			}
		}
		break;
	case Internal_Live_Buffer:
		{
			media_stream *audio = ptx->stream[TM_MEDIA_AUDIO];
			if(audio) pause_audio(audio->out_device);

			ptx->run_decode_process	= 0;
			ptx->run_render_process = 0;
			ptx->play_step = Internal_Live_Buffer;
			if( ptx->should_buffer_time < LOW_LATENCY_BUFFER_LIMIT - 500 ){
				ptx->should_buffer_time += 500;
				vpc_printf("adjust the buffer time to %d.\r\n",ptx->should_buffer_time);
			}
			ptx->notify_call_back(ptx->user_data, VPC_LIVE_BUFFER, 0);
		}
		break;
	case Internal_Seeking:
		{
			if( ptx->play_step > Internal_Ready ){
				stop_timer(&ptx->check_net_timer);
				do_seek(ptx);
				reset_timer(ptx,1);
				ptx->play_step = Internal_Seeking;
			}
		}
		break;
	case Internal_Playing:
		ptx->play_step = Internal_Playing;
		ptx->notify_call_back(ptx->user_data, VPC_START_PLAY, 0);
		start_timer( &ptx->check_net_timer);
		break;
	default:
		break;
	}
}

int process_status(fmp_context*ptx)
{
	req_msg msg = {VPC_PS_UNKNOWN};
	int cur = ptx->vpc_cur_ps;
	if(!ptx) return VPC_PS_UNKNOWN;
	msg = get_request_status(ptx);
	//don't need change status!
	if( msg.req == cur || msg.req == VPC_PS_UNKNOWN )
	{
		if( msg.wait)
			*msg.wait = 0;
		return cur;
	}
	mutex_lock(ptx->status_lock);
 
	if( msg.req >= Internal_Ready )
	{
		do_internal_state_shift(ptx,msg.req);
		mutex_unlock(ptx->status_lock);
		return ptx->vpc_cur_ps;
	}
	
    //vpc_printf("req--%s <fact:%s>----->%s\r\n",status_tostring(msg.cur),status_tostring(cur),status_tostring(msg.req));
    
    if( !msg.cur ) msg.cur = cur;
	if( (msg.cur == cur) && state_func[cur][msg.req])
	{
		state_func[cur][msg.req](ptx);
	}
	mutex_unlock(ptx->status_lock);

	if( msg.wait)
		*msg.wait = 0;

	return ptx->vpc_cur_ps;
}

void reset_timer( fmp_context *ptx, uint32_t clear_base )
{
	media_stream *s = ptx->stream[TM_MEDIA_AUDIO];
	if(s&&s->out_device) audio_reset_timer(s,clear_base);

	s = ptx->stream[TM_MEDIA_VIDEO];
	if(s) video_reset_timer(s,clear_base);
}
 
void set_current_playspeed(fmp_context *pc, int num, int den)
{
 	fmp_context *ptx = (fmp_context *)pc;
 
 	int i;
 	fraction speed;
 	media_stream *s;
 	speed.num = num;
 	speed.den = den;
 	if( !ptx ) return;
 	for (i=0;i<MAX_STREAM_NUM;i++)
 	{	
 		s = ptx->stream[i];
 		if(s)
 		{
 			if( memcmp(&s->request_speed,&speed,sizeof(speed))){
 				s->request_speed = speed;
 				s->requst_change_speed = 1;
 			}
 		}
 	}
}


TIME_TYPE get_current_time(fmp_context *ptx)
{
// 	media_stream *s = ptx->stream[TM_MEDIA_VIDEO];
// 	if(s)
// 		return video_get_timer_value(s);
// 
// 	s = ptx->stream[TM_MEDIA_AUDIO];
// 	if(s)
// 		return audio_get_timer_time(s);
	
	media_stream *s =0;
	s = ptx->stream[TM_MEDIA_AUDIO];
	if(s&&s->out_device)
		return audio_get_timer_time(s);

	s = ptx->stream[TM_MEDIA_VIDEO];
	if(s&&s->out_device)
		return video_get_timer_value(s);

	return 0;
}

/*it's too long*/
/*
int seek_in_mempool(fmp_context *ptx, TIME_TYPE pos)
{
	int ret = 0;
	media_stream *s = 0;
	int get_len = 0;
	decode_context	*pdc;
	stream_pack		*psp;
	s = ptx->stream[TM_MEDIA_AUDIO];
	if(s)
	{
		pdc = &s->dc;
		psp = &s->cur_pkt;
		mutex_lock(s->data_lock);
		while (1)
		{
			mmg_end_get_buffer2(s->mempool);
			ret = mmg_get_buffer(s->mempool, (uint8_t*)psp, (unsigned int *)&get_len );
			if(!ret) 
			{
				mutex_unlock(s->data_lock);
				return 0;
			}
			ret = mmg_begin_get_buffer2(s->mempool, (uint8_t**)&psp->buffer, (unsigned int *)&psp->buf_size);

			pdc->in_stream	= psp->buffer;
			pdc->in_len		= psp->buf_size;
			s->use_time		= psp->dts;
			ptx->position	= s->use_time;
			psp->vtag.dt	= vpc_gettime();

			if( psp->dts + ptx->do_seek_point >= pos ){
				break;
			}
		}
		mutex_unlock(s->data_lock);
		pdc->out_size = 0;
		s->wait_for_output = 0;
		s->skip_frame = skip_nothing;
		s->output_offset = 0;
// 		ptx->run_process = 0;
		reset_audio(s->out_device);
		audio_flush_data(s);
	}

	s = ptx->stream[TM_MEDIA_VIDEO];
	if(s)
	{
		mp_val last_frame={0},now_frame={0};
		pdc = &s->dc;
		psp = &s->cur_pkt;
		mutex_lock(s->data_lock);
		mmg_end_get_buffer2(s->mempool);
		while (1)
		{
			stream_pack	*tmp;
			ret = mmg_access_next_chunk(s->mempool,&last_frame,&now_frame);
			if(!ret) 
			{
				mutex_unlock(s->data_lock);
				return 0;
			}
			tmp = (stream_pack*)now_frame.buf;
			if( tmp->dts + ptx->do_seek_point>=pos ){//find it!!!
				break;
			}
			if(psp->frame_style == 'I')
			{
				if( last_frame.buf )
					mmg_skip_to_here(s->mempool,&last_frame);
				memset(&last_frame,0,sizeof(last_frame));
			}
			else{
				last_frame = now_frame;
			}
		}
		
		while (1)
		{
			mmg_end_get_buffer2(s->mempool);
			ret = mmg_get_buffer(s->mempool, (uint8_t*)psp, (unsigned int *)&get_len );
			if( !ret )
			{
				mutex_unlock(s->data_lock);
				return 0;
			}
			ret = mmg_begin_get_buffer2(s->mempool, (uint8_t**)&psp->buffer, (unsigned int *)&psp->buf_size);

			pdc->in_stream = psp->buffer;
			pdc->in_len		= psp->buf_size;
			s->use_time		= psp->dts;
			ptx->position	= s->use_time;
			s->skip_frame	= 0;
			psp->vtag.dt	= vpc_gettime();

			if( psp->dts + ptx->do_seek_point>=pos ){
				break;
			}
			if(psp->frame_style != 'B'){
   				s->codec->decode(s->codec,pdc);
			}
		}
		mutex_unlock(s->data_lock);
		pdc->out_size = 0;
		s->wait_for_output = 0;
		s->skip_frame = skip_nothing;

		s->output_offset = 0;
// 		ptx->run_process = 0;
		video_flush_img(s);
	}
	return 1;
}
*/
void do_seek(fmp_context *ptx)
{
	media_stream *s = ptx->stream[TM_MEDIA_VIDEO];
	video_ext *vp = 0;
	if(s)
		vp	= (video_ext*)s->avext;
	if(vp )
		mutex_lock(vp->decode_section);

	if( ptx->request_seek_pos )
	{
		TIME_TYPE pos = ptx->request_seek_pos;
		TIME_TYPE cur_play_pos = ptx->do_seek_point + ptx->position;
		ptx->request_seek_pos = 0;
		compute_buffer_time(ptx);
        /*
		if( pos>cur_play_pos && pos < cur_play_pos+ptx->alreay_buffer_time){
 			if( seek_in_mempool(ptx,pos) )
			{
//  			set_request_status(ptx, VPC_PS_BUFFER );
// 				set_request_status(ptx, VPC_PS_PLAY );
				goto end_func;
			}
		}*/
//		vpc_printf("request seeked time point=%d\r\n",ptx->request_seek_pos);
		if(ptx->source->seek)
		{
            int result=-1;
            
			ptx->do_seek_point = pos;
			result = ptx->source->seek(ptx->source, pos);
            
            if(result)
			{
				media_stream /**video,*/* audio = ptx->stream[TM_MEDIA_AUDIO];
				if(audio) reset_audio(audio->out_device);
				ptx->run_decode_process	= 0;
				ptx->run_render_process	= 0;
				ptx->vpc_cur_ps = VPC_PS_PLAY;
				vpc_printf("do_seek_point=%d\r\n",ptx->do_seek_point);
			}
			else
            {
                ptx->do_seek_point = 0;
			}
		}
	}
end_func:
	if(vp )
		mutex_unlock(vp->decode_section);
}

void do_pause(fmp_context *ptx)
{
///	media_stream *s = 0;
	media_stream *audio = ptx->stream[TM_MEDIA_AUDIO];
	ptx->run_render_process = 0;
	if(audio) pause_audio(audio->out_device);
	if(ptx->source->pause){
		ptx->source->pause(ptx->source);
	}
}

int stop_2_play( fmp_context *ptx )
{
	ptx->run_render_process	= 0;
	ptx->run_decode_process	= 0;

	ptx->check_end_of_file	= 0;
	ptx->do_seek_point = 0;

	ptx->player_decode_mode_status  = 0;
	ptx->alreay_buffer_time = 0;

	/*start a play*/
	start_up_play(ptx);

	if(ptx->source){
		ptx->vpc_cur_ps = VPC_PS_PLAY;
		ptx->notify_call_back(ptx->user_data, VPC_CONNECT_SERVER, 0);
		do_internal_state_shift(ptx, Internal_Connecting);
	}
	return 1;
}

int play_2_stop( fmp_context *ptx )
{
	return 1;
}

int pause_2_stop( fmp_context *ptx )
{
	return 1;
}

int play_2_pause( fmp_context *ptx )
{
	if( ptx->player_decode_mode_status == 2 ){
		do_pause(ptx);
	}
	ptx->vpc_cur_ps = VPC_PS_PAUSE;
	return 1;
}

int pause_2_play( fmp_context *ptx )
{
	if(ptx->source)
		ptx->source->start(ptx->source,0);

	reset_timer(ptx,0);
	ptx->vpc_cur_ps = VPC_PS_PLAY;

	if(ptx->play_step >= Internal_Media_Got ){
		do_internal_state_shift(ptx, Internal_Playing);
		ptx->run_render_process = 1;
	}
	return 1;
}

/*/---------------------------

void connect_2_any(fmp_context *ptx, int req)
{
	if( req == VPC_PS_BUFFER )
	{
		ptx->vpc_cur_ps = VPC_PS_BUFFER;
		ptx->notify_call_back(ptx->user_data, VPC_START_BUFFER_DATA, 0);

		if( ptx->ready_for_run == ~0x0 )
		{
			if(ptx->source->pause){
				ptx->source->pause(ptx->source);
			}
			ptx->vpc_cur_ps = VPC_PS_PAUSE;
			ptx->ready_for_run = 0;
		}
	}
	else if( req == VPC_PS_PLAY){
		ptx->vpc_cur_ps = VPC_PS_CONNECT;
		ptx->ready_for_run = 0;
	}
	else if( req == VPC_PS_PAUSE ){
		ptx->ready_for_run = ~0x0;
	}
}

void buffer_2_any(fmp_context *ptx, int req)
{
	if(req == VPC_PS_PLAY )
	{
		if( !ptx->stream[TM_MEDIA_AUDIO] &&
			!ptx->stream[TM_MEDIA_VIDEO] )
		{
			ptx->notify_call_back(ptx->user_data,VPC_MEDIA_SPEC_ERROR,0);
			return;
		}

		reset_timer(ptx,0);
		ptx->pre_decode_mode = 1;
		ptx->run_process = 1;
		ptx->vpc_cur_ps = VPC_PS_PLAY;
	}
	else if(req == VPC_PS_PAUSE)
	{
		if(ptx->source->pause){
			ptx->source->pause(ptx->source);
		}
		ptx->vpc_cur_ps = VPC_PS_PAUSE;
	}
	else if( req == VPC_PS_SEEKING )
	{
		do_seek(ptx);
	}
}

void play_2_any(fmp_context *ptx, int req)
{
	int i;
	media_stream *audio = ptx->stream[TM_MEDIA_AUDIO];
	media_stream *s;
	if(req == VPC_PS_PAUSE)
	{
		ptx->run_process = 0;
		if(audio) pause_audio(audio->out_device);
		if(ptx->source->pause){
			ptx->source->pause(ptx->source);
		}
		ptx->vpc_cur_ps = VPC_PS_PAUSE;
	}
	else if( req == VPC_PS_BUFFER)
	{
		int t = ptx->ready_for_run;
		ptx->run_process = 0;
		if(audio) pause_audio(audio->out_device);
		ptx->vpc_cur_ps = VPC_PS_BUFFER;

		if( t == 2)
			ptx->notify_call_back(ptx->user_data, VPC_START_BUFFER_DATA, 0);

		if(t == 1)
			ptx->ready_for_run = 2;
	}
	else if( req == VPC_PS_SEEKING )
	{
		do_seek(ptx);
	}

	for (i=0;i<MAX_STREAM_NUM;i++)
	{	
		s = ptx->stream[i];
		if(s)
		{
			s->decoded_bytes = 0;
			memset(s->flow_anchor_bytes,0,sizeof(s->flow_anchor_bytes));
			memset(s->flow_anchor_time,0,sizeof(s->flow_anchor_time));
			s->flow_check_point = 0;
			s->avail_val = 0;
			s->idx = 0;
			s->cur_kbps = 0;

			if(s->media_style == TM_MEDIA_VIDEO)
			{
				video_ext *ext = s->avext;
				ext->fps = 0;
				ext->idx = 0;
				ext->avail_val = 0;
				memset(ext->showframes,0,sizeof(ext->showframes));
			}
		}
	}
}

void pause_2_any(fmp_context *ptx, int req)
{
	if( req == VPC_PS_PLAY )
	{
		media_stream *s = ptx->stream[TM_MEDIA_AUDIO];
		if(s && !s->out_device){
			ptx->notify_call_back(ptx->user_data, VPC_OPEN_AV_DEVICE, TM_MEDIA_AUDIO);
		}

		if( !ptx->stream[TM_MEDIA_AUDIO] &&!ptx->stream[TM_MEDIA_VIDEO] )
		{
			ptx->notify_call_back(ptx->user_data,VPC_MEDIA_SPEC_ERROR,0);
			return;
		}

		if(ptx->source)
			ptx->source->start(ptx->source,0);

		ptx->vpc_cur_ps = VPC_PS_PLAY;
		reset_timer(ptx,0);
		ptx->pre_decode_mode = 1;
		ptx->run_process = 1;
		ptx->ready_for_run = 2;
	}
	else if( req == VPC_PS_BUFFER )
	{
		if(ptx->source->pause){
			ptx->source->pause(ptx->source);
		}
		ptx->vpc_cur_ps = VPC_PS_PAUSE;
	}
	else if( req == VPC_PS_SEEKING )
	{
		if(ptx->source)
			ptx->source->start(ptx->source,0);

		do_seek(ptx);
	}
}

void seeking_2_any(fmp_context *ptx, int req)
{
	if(req == VPC_PS_BUFFER )
	{
		reset_timer(ptx,1);
		ptx->vpc_cur_ps = VPC_PS_BUFFER;
		ptx->notify_call_back(ptx->user_data, VPC_START_BUFFER_DATA, 0);
	}
	else if( req == VPC_PS_SEEKING )
	{
		do_seek(ptx);
	}
	else if( req == VPC_PS_PAUSE)
	{
		reset_timer(ptx,1);
		ptx->vpc_cur_ps = VPC_PS_PAUSE;
	}
	else if( req == VPC_PS_PLAY){
		ptx->vpc_cur_ps = VPC_PS_SEEKING;
	}
}

void stop_2_any(fmp_context *ptx, int req)
{
	if( req == VPC_PS_CONNECT )
	{
		ptx->run_process	  =0;
		ptx->check_end_of_file=0;
		ptx->do_seek_point = 0;

		ptx->pre_decode_mode  = 1;
		ptx->alreay_buffer_time=0;


		start_up_play(ptx);

		if(ptx->source){
			ptx->vpc_cur_ps = VPC_PS_CONNECT;
		}
		else{
			ptx->vpc_cur_ps = VPC_PS_STOP;
		}
	}
}

void any_2_suspend(fmp_context *ptx, int req)
{
	int i=0;
	media_stream *s;
	ptx->suspend_state = ptx->vpc_cur_ps;
	ptx->vpc_cur_ps = VPC_PS_SUSPEND;
	ptx->run_process = 0;
	//release av device
	for (i=0;i<MAX_STREAM_NUM;i++)
	{
		s= ptx->stream[i];
		if(s){
			s->release(s);
		}
	}
}

*///--------------------------

int is_facebac_url( fmp_context *ptx, const char *uri )
{
	static char proto_str[128];
	static char www[128];
	int port = 0;
	const char *org = uri;
	char *ptr;
	size_t proto_len = strspn(uri, URL_SCHEME_CHARS);
	if( proto_len )
	{
		int len = 0;
		if( uri[proto_len] != ':')
			return 0;
		proto_len += 3;//skip ://
		uri += proto_len;

		ptr = strstr(uri,"/");
		if(ptr)
		{
			len = ptr-uri;
			memcpy(www,uri,len);
			www[len] = '\0';
		}
		ptr = strstr(www,":");
		if( ptr )
		{
			*ptr = '\0';
			ptr++;
			port = atoi(ptr);
		}
		if(!port)
			port = 80;
	}

	return 1;
}

void *find_source_filter(fmp_context *ptx,const char *url,int av_format)
{
	tools_def *tools = NULL;
	const node_boot_func *func = NULL;
	if( !ptx ) return NULL;
		
	if( !is_facebac_url(ptx, url) ){
		return NULL;
	}

	ptx->low_latency_mode = 0;

	tools = ptx->tools_start;

	while (tools)
	{	
		func = tools->p;
		if( func->category_id == SOURCE_ID )
		{
			int ret = func->available((long)url);
			if( !ret )
			{
				switch (av_format)
				{
				case MED_FMT_HLS:
					if(func->sub_id == AVFORMAT_ID)
						ret = 1;
					break;
				}
			}
			if(ret)
			{
				//const char *desp = func->desc;
				ptx->source_node = tools->p;

				/* only rtmp need low latency mode */
				if( tools->p->sub_id == RTMP_ID ){
					ptx->low_latency_mode = 1;
					if( ptx->should_buffer_time > LOW_LATENCY_BUFFER_LIMIT ){
						ptx->should_buffer_time = LOW_LATENCY_BUFFER_LIMIT;
					}
				}

				return func->create(0,0);
			}
		}
		tools = tools->next;
	}
	return NULL;
}

void start_up_play(fmp_context *ptx)
{
	source_layer *source;
	if ( !ptx->urlinfo ) return;
	source = find_source_filter(ptx, (char*)ptx->urlinfo->play_url, ptx->urlinfo->format_selector );
	if( !source )
	{
		ptx->notify_call_back( ptx->user_data, VPC_NO_SOURCE_DEMUX,0 );
		return;
	}
	
	/*fill_sample is data push func*/
	source->open(source,ptx->urlinfo, fill_sample, ptx);

	/*set notify msg*/
	source->set_notify(source, msg_gateway, ptx);

	/*start run data source*/
 	source->start(source,ptx->urlinfo->start_pos);
	
	ptx->do_seek_point = ptx->urlinfo->start_pos;
	/*save source demux*/
	ptx->source = source;
}

void buffer_speed_reset(BufferSpeedComputer *speedComputer )
{
	speedComputer->mIn = 0;
	speedComputer->mSum = 0;
	speedComputer->mPrevious = -1;
	memset( speedComputer->mHistroySpeed, 0 , sizeof(speedComputer->mHistroySpeed) );
}

int buffer_speed_append_sample(BufferSpeedComputer *speedComputer, int speed )
{
	int average = 0;
	int diff = 0;

	/* instant */
	if( speedComputer->mPrevious < 0 ){
		speedComputer->mPrevious = speed;
	}
	diff = speed - speedComputer->mPrevious;
	speedComputer->mPrevious = speed;
	speedComputer->mSum += diff;

	speedComputer->mSum -= speedComputer->mHistroySpeed[speedComputer->mIn];
	speedComputer->mHistroySpeed[speedComputer->mIn] = diff;
	if( ++speedComputer->mIn >= HISTROY_SPEED_CNT ){
		speedComputer->mIn = 0;
	}

	return speedComputer->mSum;
}

void init_timer(user_timer *timer, int dur, void *key, int(*callback)(void *))
{
	timer->duration = dur;
	timer->active = 0;
	timer->start_time = 0;
	timer->key = key;
	timer->callback = callback;
}

void start_timer(user_timer *timer )
{
	timer->active = 1;
	timer->start_time = vpc_gettime();
}

void loop_timer(user_timer *timer)
{
	if( timer->active && (vpc_gettime() - timer->start_time) > timer->duration )
	{
		int ret = timer->callback(timer->key);
		if( ret ){
			timer->duration = ret;
			start_timer(timer);
		}
		else{
			stop_timer(timer);
		}
	}
}

void stop_timer(user_timer *timer)
{
	timer->active = 0;
	timer->start_time = 0;
}
