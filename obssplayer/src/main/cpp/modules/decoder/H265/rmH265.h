#ifndef __RM_H265_H__
#define __RM_H265_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "rmComType.h"

#define RM_HEVC_BASE                            0x80001000
#define RM_INDEX_HEVCDEC                     RM_HEVC_BASE | 0x00000500
#define RM_INDEX_HEVCENC                     RM_HEVC_BASE | 0x00000600
#define	RM_PID_VIDEO_BASE					 RM_HEVC_BASE | RM_PID_BASE        
#define	RM_PID_VIDEO_VIDEOMEMOP		 (RM_PID_VIDEO_BASE | 0X0003)									
#define RM_PID_REF_ERROR                 (RM_PID_VIDEO_BASE | 0X0004)
#define RM_PID_RPS_ERROR                 (RM_PID_VIDEO_BASE | 0X0005)
#define RM_PID_REF_PIC_ERROR             (RM_PID_VIDEO_BASE | 0x0006)
#define RM_PID_NALU_ERROR                (RM_PID_VIDEO_BASE | 0x0007)	
#define RM_PID_SLICEHEADER_ERROR         (RM_PID_VIDEO_BASE | 0x0008)
#define RM_PID_PPS_ERROR                 (RM_PID_VIDEO_BASE | 0x0009)
#define RM_PID_SPS_ERROR                 (RM_PID_VIDEO_BASE | 0x000A)
#define	RM_PID_VIDEO_OUTPUTMODE		 (RM_PID_VIDEO_BASE | 0X000B)
#define RM_PID_SPS_NULL                  (RM_PID_VIDEO_BASE | 0X000C)
#define RM_PID_VPS_ERROR                 (RM_PID_VIDEO_BASE | 0X000D)
#define RM_PID_ENTRY_ERROR               (RM_PID_VIDEO_BASE | 0X000E)
#define RM_PID_SLIST_ERROR               (RM_PID_VIDEO_BASE | 0X0010)
#define RM_PID_PPS_NULL                  (RM_PID_VIDEO_BASE | 0X0011)



typedef struct
{
	RM_PBYTE 					Buffer[3];		
	RM_S32							Stride[3];		
	RM_COLOR_TYPE   		    ColorType;		
	RM_S64				            Time;			
	RM_PTR				            UserData;       
} RM_VIDEO_BUFFER;

#define     RM_FM1_FLAG      0x2
#define     RM_FM2_FLAG      0x10
#define     RM_FM3_FLAG      0x20

typedef enum
{
	RM_VIDRO_NONE                     =0,
	RM_VIDEO_FRAME_I                = 1,   
	RM_VIDEO_FRAME_P                = 2,  
	RM_VIDEO_FRAME_B                = 3,  
	RM_VIDEO_FRAME_S                = 4,  
	RM_VIDEO_FRAME_NULL             = 5,
    RM_VIDEO_FRAMETYPE_MAX			= RM_MAX_ENUM_VALUE
}
RM_VIDEO_FRAMETYPE;

typedef struct
{
	RM_S32				Width;		 
	RM_S32				Height;		 
	RM_VIDEO_FRAMETYPE	Type;		
} RM_VIDEO_FORMAT;


typedef struct
{
	RM_VIDEO_FORMAT		Format;		
	RM_U32							UsedLens;	
	RM_U32							Flag;		
	RM_U32							Resevered;	
} RM_VIDEO_BASICINFO;


typedef struct
{
	RM_U32 (* Open) (RM_HANDLE * phDec);
	RM_U32 (* Process)(RM_HANDLE hDec, 
		               RM_PTR * pOutBuffer, 
			           RM_VIDEO_BASICINFO * pOutInfo
			           );
	RM_U32 (* Set) (RM_HANDLE hDec, RM_S32 uParamID, RM_PTR pData);
	RM_U32 (* Get) (RM_HANDLE hDec, RM_S32 uParamID, RM_PTR pData);
	RM_U32 (* Close) (RM_HANDLE hDec);
} RM_VIDEO_CODECAPI;

/* Decoder Setting ID */
#define RM_ERR_HEVCDEC_BASE                RM_RETURN_BASE | RM_INDEX_HEVCDEC
#define RM_PID_HEVCDEC_BASE                RM_PID_BASE | RM_INDEX_HEVCDEC         
#define RM_PID_HEVCDEC_END_FLUSH           RM_PID_HEVCDEC_BASE | 0x0002
#define RM_PID_HEVCDEC_FASTMODE            RM_PID_HEVCDEC_BASE | 0x0003   

/* Encoder Setting ID */
#define RM_PID_HEVCENC_BASE                RM_PID_BASE | RM_INDEX_HEVCENC
#define RM_PID_HEVCENC_BITRATE             RM_PID_HEVCENC_BASE | 0x0001
#define RM_PID_HEVCENC_FRAMERATE           RM_PID_HEVCENC_BASE | 0x0002
#define RM_PID_HEVCENC_WIDTH               RM_PID_HEVCENC_BASE | 0x0003
#define RM_PID_HEVCENC_HEIGHT              RM_PID_HEVCENC_BASE | 0x0004
#define RM_PID_HEVCENC_KEY_FRAME_I         RM_PID_HEVCENC_BASE | 0x0005
/*
HEVC Decoder interface
*/
RM_S32 rmGetDecHandle (RM_VIDEO_CODECAPI * pDecHandle);

/*
HEVC Encoder interface
*/ 
RM_S32 rmGetEncHandle(RM_VIDEO_CODECAPI *pEncHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __RM_H265_H__
