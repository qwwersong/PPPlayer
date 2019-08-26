
#ifndef __DEMUXS_H__
#define __DEMUXS_H__

#ifdef __cplusplus
extern "C" {
#endif

	/*all demuxs, include mp4 demux, rtsp demux, netiq demux*/
 	void rtsp_register(void *ptx);
    void mp4_register(void *pc);
	void ffmpeg_avformat_register(void *pc);
	//rtmp live demuxer
	void rtmp_stream_register(void *pc);
	void srs_rtmp_stream_register(void *pc);
	void http_svr_stream_register(void *pc);

#ifdef __cplusplus
}
#endif


#endif