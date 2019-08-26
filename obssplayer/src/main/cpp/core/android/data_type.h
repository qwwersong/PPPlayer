/*
 *	common data type define
 */

#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

#include <stdint.h>

typedef unsigned int		bool_t;

#define TIME_TYPE			uint32_t

#define THREAD_PARAM_TYPE		void *
#define THREAD_HANDLE_TYPE		int
#define THREAD_RETURN_TYPE		void *
#define THREAD_ID_TYPE			pthread_t

#define VPC_API

#ifndef NULL
	#define NULL	(void*)0
#endif

#define DYNAMIC_EXPORT
#define SDK_VERSION 7
#endif /*_DATA_TYPE_H*/
