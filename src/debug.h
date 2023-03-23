#ifndef _DEBUG_H_
#define _DEBUG_H_


#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <math.h>
#include <assert.h> 

#define DBG_MODE_OFF 				0
#define DBG_MODE_TRACK_FUNC			1
#define DBG_MODE_MEMORY_INFO		2
#define DBG_MODE_DBG				8
#define DBG_MODE_MEMORY_ERROR		16
#define DBG_MODE_TRACK_MUTEX		32
#define DBG_MODE_TRACK_FUNC_THREAD	64
#define DBG_MODE_TRACK_LOCKED_MUTEX	128
#define DBG_MODE_LOG_FUNC			256
#define DBG_MODE_EXIT				512
#define DBG_MODE_LOG_MEM			1024
#define DBG_MODE_TRACK_LONG_MUTEX	2048

#define DBG_MAX_MUTEX_CNT	32
#define DBG_MAX_FUNC_CNT	1024
#define DBG_MAX_MEM_CNT		1024
#define DBG_MUTEX_TIMEOUT_MAX	50000
#define DBG_MUTEX_TIMEOUT_WARN	20

#define ENABLE_DBG 1

#if DBG
	#define dbg_printf(fmt, ...) printf(fmt, ##__VA_ARGS__)
	//#define log_print(Buff, argss) debug_fprintf(Buff, argss)
#else
	#define dbg_printf(fmt, ...) {}
	//#define log_print(Buff, argss) {}
#endif

#if ENABLE_DBG
	#define DBG_LOG_IN()		dbg_log_in(__FILE__, __func__, __LINE__)
	#define DBG_LOG_OUT()		dbg_log_out(__FILE__, __func__, __LINE__)
	#define DBG_INIT(x, y)		dbg_init(x, y)
	#define DBG_CLOSE() 		dbg_close()	
	#define DBG_ON(x) 			dbg_on(x)	
	#define DBG_OFF() 			dbg_off()		
	
	#define DBG_MALLOC(x) 		dbg_malloc(x, __FILE__, __func__, __LINE__)
	#define DBG_FREE(x) 		dbg_free(x, __FILE__, __func__, __LINE__)
	#define DBG_CALLOC(x, y) 	dbg_calloc(x, y, __FILE__, __func__, __LINE__)
	#define DBG_REALLOC(x, y) 	dbg_realloc(x, y, __FILE__, __func__, __LINE__)
	#define DBG_LOG_MEM() 		dbg_log_mem(__FILE__, __func__, __LINE__)
	#define DBG_TEST_MEM(x)		dbg_test_key(x, __FILE__, __func__, __LINE__)
	#define DBG_MAX_MEM()		dbg_max_mem()
	#define DBG_USE_MEM()		dbg_use_mem()
	
	#define DBG_SET_MUTEX(x) 			dbg_set_mutex(x)
	#define DBG_MUTEX_TRYLOCK(x)		dbg_mutex_trylock(x, __FILE__, __func__, __LINE__, #x)
	#define DBG_MUTEX_LOCK(x) 			dbg_mutex_lock(x, __FILE__, __func__, __LINE__, #x)
	#define DBG_MUTEX_UNLOCK(x) 		dbg_mutex_unlock(x, __FILE__, __func__, __LINE__, #x)	
	#define DBG_MUTEX_PRINT_LOCKED(x, y) 	dbg_mutex_print_locked(x, y) 
	#define DBG_POINT(x)				dbg_point(x,__LINE__)	
#else
	#define DBG_LOG_IN(); 		{}
	#define DBG_LOG_OUT(); 		{}
	#define DBG_INIT(x, y);		{}
	#define DBG_CLOSE(); 		{}	
	#define DBG_ON(x); 			{}	
	#define DBG_OFF(); 			{}	
		
	#define DBG_MALLOC(x) 		malloc(x)
	#define DBG_FREE(x) 		free(x)
	#define DBG_CALLOC(x, y) 	calloc(x, y)
	#define DBG_REALLOC(x, y) 	realloc(x, y)
	#define DBG_LOG_MEM(); 		{}	
	#define DBG_TEST_MEM(x);	{}
	#define DBG_MAX_MEM()		0
	#define DBG_USE_MEM()		0
	
	#define DBG_SET_MUTEX(x); 			{}	
	#define DBG_MUTEX_TRYLOCK(x) 		pthread_mutex_trylock(x)								
	#define DBG_MUTEX_LOCK(x) 			pthread_mutex_lock(x)
	#define DBG_MUTEX_UNLOCK(x) 		pthread_mutex_unlock(x)	
	#define DBG_MUTEX_PRINT_LOCKED(x, y) 	{}
	#define DBG_POINT(x)				{}
#endif

typedef struct dbg_mutex_info 
{
	pthread_mutex_t *mutex;
	unsigned int tid;
	unsigned int count;
	char filename[32];
	char funcname[32];
	char mutexname[32];
	int line; 
	char status;
	int64_t startlock_ms;
	int64_t endlock_ms;
} dbg_mutex_info;

typedef struct dbg_func_info 
{
	char filename[32];
	char funcname[64];
	int line; 
	int tid; 
	int stid; 
	char status;
} dbg_func_info;

typedef struct dbg_mem_info 
{ 
	char filename[32];
	char funcname[64];
	int line; 
	unsigned int size; 
	unsigned int number; 
	char status; 
	void *point;
} dbg_mem_info;

typedef struct dbg_point_info 
{
	unsigned int pid;
	unsigned int line;
	char		 mark;
	unsigned int info;
	int64_t		 time;
} dbg_point_info;

void dbg_death_signal(int signum);
int SendDbgUDPMessage(unsigned int Type, char *cData, int iLen, char *cAddr);
int dbg_init(unsigned int iMode, unsigned int uiMutexTime);
int dbg_close();
int dbg_log_in(char *pFileName, const char *pFuncName, int iLine);
int dbg_log_out(char *pFileName, const char *pFuncName, int iLine);
void dbg_on(unsigned int iMode);
void dbg_off();

void *dbg_calloc(size_t nmemb, size_t size, char *pFileName, const char *pFuncName, int iLine);
void *dbg_malloc(size_t size, char *pFileName, const char *pFuncName, int iLine);
void dbg_free(void *ptr, char *pFileName, const char *pFuncName, int iLine);
void *dbg_realloc(void *ptr, size_t size, char *pFileName, const char *pFuncName, int iLine);
int dbg_log_mem(char *pFileName, const char *pFuncName, int iLine);
int dbg_log_mem_short();
char dbg_test_key(void* mem, char *pFileName, const char *pFuncName, int iLine);
char dbgprintf(unsigned char cLevel, char *Buff, ...);
void dbg_point(int iInfo, int iLine);
void dbg_mark_cur_point(char cFlag);
uint32_t get_tick_ms();

void dbg_set_mutex(pthread_mutex_t *mutex);
int dbg_mutex_trylock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine, char *mutexname);
int dbg_mutex_lock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine, char *mutexname);
int dbg_mutex_unlock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine, char *mutexname);
void dbg_mutex_print_locked(unsigned int uiMutexTime, char cResult);
void dbg_func_print_entered();
void dbg_mem_print_allocated(int iLevel);
int dbg_mem_print_allocated_short();
void dbg_points_print();
void dbg_mem_test_allocated(char cShowOkRes);
unsigned int dbg_max_mem();
unsigned int dbg_use_mem();
int gettid();
unsigned int load_last_dbg_data(char *data, unsigned int len);

int iFileLog, iScreenLog, iMessageLog, iLocalMessageLog, iEmailLog;
char cFileLogName[256];
char cLogIP[256];
int64_t iLogEmailTimer;
char cLogMailAddress[64];
char cLogMailServer[64];
char cLogMailLogin[64];
char cLogMailPassword[64];
char cLogMailAuth[64];
unsigned int uiLogEmailPauseSize;
pthread_mutex_t 	dbg_mutex;

#endif