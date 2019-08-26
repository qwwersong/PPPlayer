#ifndef __RM_COMTYPE_H__
#define __RM_COMTYPE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define RM_MAX_ENUM_VALUE	0X7FFFFFFF

typedef void RM_VOID;
typedef unsigned char RM_U8;
typedef unsigned char RM_BYTE;
typedef signed char RM_S8;
typedef char RM_CHAR;
typedef unsigned short RM_U16;


#if defined _WIN32
typedef unsigned short RM_WCHAR;
typedef unsigned short* RM_PWCHAR;
#elif defined LINUX
typedef unsigned char RM_WCHAR;
typedef unsigned char* RM_PWCHAR;
#endif

#ifdef _WIN32
#define RM_TCHAR		TCHAR
#define RM_PTCHAR		TCHAR*
#else
typedef char TCHAR, *PTCHAR;
#define RM_TCHAR		char
#define RM_PTCHAR		char*
#endif // _WIN32

typedef signed short RM_S16;
typedef unsigned int RM_U32;
typedef int RM_S32;


#ifndef RM_SKIP64BIT
#ifdef _WIN32
typedef unsigned __int64  RM_U64;
typedef signed   __int64  RM_S64;
#else // WIN32
/** RM_U64 is a 64 bit unsigned quantity that is 64 bit word aligned */
typedef unsigned long long RM_U64;
/** RM_S64 is a 64 bit signed quantity that is 64 bit word aligned */
typedef signed long long RM_S64;
#endif // WIN32
#endif // RM_SKIP64BIT


typedef enum RM_BOOL {
    RM_FALSE = 0,
    RM_TRUE = !RM_FALSE,
	RM_BOOL_MAX = RM_MAX_ENUM_VALUE
} RM_BOOL;


typedef void* RM_PTR;
typedef const void* RM_CPTR;
typedef void* RM_HANDLE;
typedef char* RM_PCHAR;
typedef unsigned char* RM_PBYTE;

#ifdef _UNICODE
typedef unsigned short* RM_PTTCHAR;
typedef unsigned short RM_TTCHAR;
#else
typedef char* RM_PTTCHAR;
typedef char RM_TTCHAR;
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

typedef struct {
	RM_PBYTE	Buffer;		
	RM_U32		Length;		
	RM_S64		Time;		
	RM_PTR		UserData;   
} RM_CODECBUFFER;

typedef enum{
	RM_IMF_USERMEMOPERATOR		=0,	
	RM_IMF_PREALLOCATEDBUFFER	=1,	
	RM_IMF_MAX = RM_MAX_ENUM_VALUE
}RM_INIT_MEM_FlAG;

typedef struct{
	RM_U32						memflag;		
	RM_PTR						memData;		
	RM_U32						reserved1;		
	RM_U32						reserved2;		
}RM_CODEC_INIT_USERDATA;


#define RM_RETURN_OK				0x00000000
#define RM_RETURN_BASE				0X82100000
#define RM_RETURN_FAILED			0x82100001
#define RM_RETURN_MEM_ERROR			0x82100002
#define RM_RETURN_NOT_IMPLEMENT			0x82100003
#define RM_RETURN_INVALID_ARG			0x82100004
#define RM_RETURN_INPUT_NO_ENOUGH		0x82100005
#define RM_RETURN_DROPPEDFRAME		        0x82100006
#define RM_RETURN_WRONG_STATUS			0x82100007
#define RM_RETURN_WRONG_PARAM_ID		0x82100008
#define RM_RETURN_LICENSE_ERROR			0x82100009
#define RM_RETURN_FORCE_STOP                    0x8210000A

 
#define	RM_PID_BASE				0x42100000				
#define	RM_PID_HEADDATA			        (RM_PID_BASE | 0X0004)	
#define	RM_PID_FLUSH			        (RM_PID_BASE | 0X0005)	
#define	RM_PID_HeadInfo			        (RM_PID_BASE | 0X000B)	
#define	RM_PID_INPUTDATA		        (RM_PID_BASE | 0X000C)	
#define	RM_PID_CPUNUM			        (RM_PID_BASE | 0X0201)	
#define	RM_PID_FRAME_BUF_EX		        (RM_PID_BASE | 0X0204)	


typedef enum
{	
	RM_COLOR_YUV_PLANAR420			= 3,			
	RM_COLOR_TYPE_MAX			= RM_MAX_ENUM_VALUE
}  RM_COLOR_TYPE;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __RM_COMTYPE_H__
