#include "playercore_platform.h"
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "cpu-features.h"
#include "os.h"

#undef  LOG
#include "logger.h"
#define LOG_TAG "playercore"

JavaVM *android_jvm = 0;

typedef struct android_mutex
{
	pthread_mutex_t pmt;
	pthread_mutexattr_t attr;
}android_mutex;


extern void Android_Audio_register(void*ptx);
extern void Android_Video_register(void *ptx);

void* vpc_mem_alloc(uint32_t len)
{
	void *p = malloc(len);
	if(p)memset(p,0,len);
	return p;
}

void  vpc_mem_free(void * data)
{
	if(data)
	{
		free(data);
	}
}

void vpc_delay(TIME_TYPE time)
{
//	usleep(time*1000);
	struct timeval tval; 
    tval.tv_sec = 0; 
    if(time<1000)
    {
        tval.tv_usec = ((int32_t)time)*1000;    
    }
    else
    {
        tval.tv_sec = time/1000;   
        tval.tv_usec = ((int32_t)time%1000)*1000;   
    }
    select(0,NULL,NULL,NULL,&tval);
}

int vpc_get_cpu_count()
{
	return android_getCpuCount();
}


TIME_TYPE vpc_gettime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL); /*get the time intervel from Jan 1, 1970*/
	return ((TIME_TYPE)tv.tv_sec) * 1000 + (TIME_TYPE)(tv.tv_usec/1000);
}

void *mutex_init()
{
	android_mutex *lock = (android_mutex *)vpc_mem_alloc(sizeof(android_mutex));
	if(lock)
	{
		int ret;
		ret = pthread_mutexattr_init(&lock->attr);
		ret = pthread_mutexattr_settype(&lock->attr, PTHREAD_MUTEX_NORMAL);
		ret = pthread_mutex_init(&lock->pmt, &lock->attr);
	}
	return lock;
}

void mutex_lock(void *lock)
{
	android_mutex *lock2 = (android_mutex *)lock;
	pthread_mutex_lock(&lock2->pmt);
}

void mutex_unlock(void *lock)
{
	android_mutex *lock2 = (android_mutex *)lock;
	pthread_mutex_unlock(&lock2->pmt);
}

void mutex_uninit(void *lock)
{
	if(lock)
	{
		android_mutex *lock2 = (android_mutex *)lock;
		pthread_mutex_destroy(&lock2->pmt);
		pthread_mutexattr_destroy(&lock2->attr);
		vpc_mem_free(lock);
	}
}

void *util_sem_init()
{
	sem_t *s = (sem_t *)vpc_mem_alloc(sizeof(sem_t));
	if(s)
	{
		int ret = sem_init(s, 0, 0);
	}
	return s;
}

int util_sem_wait(void *s)
{
	sem_t *sp = (sem_t*)s;
	if(sp)
	{
		sem_wait(sp);
	}
	return 1;
}

void util_sem_post(void *s)
{
	sem_t *sp = (sem_t*)s;
	if(sp)
	{
		sem_post(sp);
	}
}

void util_sem_uninit(void *s)
{
	sem_t *sp = (sem_t*)s;
	if(sp)
	{
		sem_destroy(sp);
		vpc_mem_free(sp);
	}
}

void vpc_load_all_module(void *ptx, REGISTERTOOLS reg_func)
{
	return;
}

void vpc_free_module(void *handle)
{
	return; //
}

typedef struct droid_player 
{
	pthread_t  	id[2];
	int			exit_flag[2];
	task_entry_t master;
	task_entry_t master_clean;
	
	task_entry_t slave;
	task_entry_t slave_clean;	
	void *ptx;
}droid_player;


void* master_linux(void *p)
{
	droid_player *phd = (droid_player*)p;
	while (!phd->exit_flag[0])
	{
		vpc_delay(phd->master(phd->ptx));
	}
	phd->master_clean(phd->ptx);
	return (void*)0xFF;
}

void* slave_linux(void *p)
{
	int sleep_time;
	droid_player *phd = (droid_player*)p;
	while (!phd->exit_flag[1])
	{
		sleep_time = phd->slave(phd->ptx);
		if(sleep_time ) vpc_delay(sleep_time);
	}
	phd->slave_clean(phd->ptx);
	return (void*)0xFF;
}

void *run_player(void *ptx,task_entry_t master, task_entry_t me,task_entry_t slave,task_entry_t se)
{
	droid_player *phd = (droid_player *)vpc_mem_alloc(sizeof(droid_player));
	if(!phd) return 0;
	phd->ptx = ptx;
	phd->master = master;
	phd->slave  = slave;
	phd->master_clean = me;
	phd->slave_clean  = se;
	pthread_create((pthread_t*)&phd->id[0],0,master_linux,phd);
	pthread_create((pthread_t*)&phd->id[1],0,slave_linux,phd);
	LOGD("begin run player");
	return phd;
}

//first wait slave exit
//then master exit
void  close_player(void *p)
{
	droid_player *phd = (droid_player*)p;
	if( phd )
	{
		phd->exit_flag[1] = 1;
		pthread_join(phd->id[1], NULL);
		
		phd->exit_flag[0] = 1;
		pthread_join(phd->id[0], NULL);		
		vpc_mem_free(phd);
		LOGD("^.^----------------close_player");
	}
}

THREAD_HANDLE_TYPE vpc_thread_create(THREAD_ID_TYPE *id, THREAD_RETURN_TYPE (VPC_API *run)(THREAD_PARAM_TYPE), THREAD_PARAM_TYPE param)
{	
	return !pthread_create((pthread_t*)id, 0, run, param);
}

void vpc_thread_destory(THREAD_HANDLE_TYPE *handle, THREAD_ID_TYPE id)
{	
	pthread_join(id, NULL);
	*handle = 0;
	LOGD("#------thread_destory");
}

void load_system_node(void *ptx)
{
	Android_Audio_register(ptx);
	Android_Video_register(ptx);
}

void vpc_debug_num(char *name ,int value)
{
	LOGD("%s=%d\r\n",name,value);
}

void vpc_debug_string(char *string)
{
	LOGD("%s\r\n",string);
}

void debug(const char *format, ... ) {
	va_list argptr;
	va_start(argptr, format);
	__android_log_vprint(ANDROID_LOG_ERROR, "playercore", format, argptr);
	va_end(argptr);
}

void vpc_printf(char *format,...)
{
	va_list argptr;
	va_start(argptr, format);
	__android_log_vprint(ANDROID_LOG_ERROR, "playercore", format, argptr);
	va_end(argptr);

// 	struct timeval tv;
// 	time_t	tCurTime;
// 	struct tm * ptmCurTime = 0;
// 	va_list ap;
// 	
// 	gettimeofday(&tv,0); /*get the time intervel from Jan 1, 1970*/
// 	tCurTime = time(0);
// 
// 	va_start(ap, fmt);
// 	ptmCurTime = localtime(&tCurTime);
// 	printf("[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",ptmCurTime->tm_year+1900,ptmCurTime->tm_mon+1,ptmCurTime->tm_mday,ptmCurTime->tm_hour,ptmCurTime->tm_min,ptmCurTime->tm_sec,tv.tv_usec/1000);
// 	vfprintf(stdout, fmt, ap);
// 	va_end(ap);
}


#ifdef __cplusplus
extern "C" {
#endif
int writelog(char* str)
{
	//LOGD(str);
	return 0;
}
#ifdef __cplusplus
}
#endif