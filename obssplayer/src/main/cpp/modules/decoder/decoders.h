
#ifndef __DECODERS_H__
#define __DECODERS_H__

#ifdef __cplusplus
extern "C" {
#endif

	/*video CODEC*/
	void MP10_register(void *ptx);
	void xvid_register(void *ptx);
    void ffmpeg_h264_register(void *ptx);
	void H265_register(void *pc );

	/*audio CODEC*/
	void amrnb_register(void *ptx);
	void amrwb_register(void *ptx);


#ifdef __cplusplus
}
#endif

#endif //__DECODERS_H__