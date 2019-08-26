#ifndef __OS_H__
#define __OS_H__


#include "data_type.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef int(*task_entry_t)(void *ptx);
	typedef void (*REGISTERTOOLS)(const void*pc, const void*node_func, void*PluginsHandle);
	typedef	void (*REGISTER_MODULE)(void *pc, void* pluginHandle, REGISTERTOOLS regfunc,int version);


	DYNAMIC_EXPORT void* vpc_mem_alloc(uint32_t len);
	DYNAMIC_EXPORT void  vpc_mem_free(void * data);

	DYNAMIC_EXPORT void vpc_delay(TIME_TYPE time);
	DYNAMIC_EXPORT TIME_TYPE vpc_gettime();
	
	DYNAMIC_EXPORT int vpc_get_cpu_count();

	DYNAMIC_EXPORT void *mutex_init();
	DYNAMIC_EXPORT void mutex_lock(void *lock);
	DYNAMIC_EXPORT void mutex_unlock(void *lock);
	DYNAMIC_EXPORT void mutex_uninit(void *lock);

	DYNAMIC_EXPORT void *util_sem_init();
	DYNAMIC_EXPORT int util_sem_wait(void *s);
	DYNAMIC_EXPORT void util_sem_post(void *s);
	DYNAMIC_EXPORT void util_sem_uninit(void *s);
	
	DYNAMIC_EXPORT THREAD_HANDLE_TYPE vpc_thread_create(THREAD_ID_TYPE *p_tid, THREAD_RETURN_TYPE (VPC_API *run)(THREAD_PARAM_TYPE), THREAD_PARAM_TYPE param);
	DYNAMIC_EXPORT void vpc_thread_destory(THREAD_HANDLE_TYPE *handle, THREAD_ID_TYPE tid);

	void vpc_load_all_module(void *ptx, REGISTERTOOLS reg_func);
	void vpc_free_module(void *handle);

	void load_system_node(void *ptx);
	void *run_player(void *ptx,task_entry_t master, task_entry_t me, task_entry_t slave,task_entry_t se);
	void  close_player(void *p);

	DYNAMIC_EXPORT void vpc_debug_num(char *name ,int value);
	DYNAMIC_EXPORT void vpc_debug_string(char *string);

	DYNAMIC_EXPORT void vpc_printf(char *fmt,...);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif //__OS_H__