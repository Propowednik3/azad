
#include "signal.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "debug.h"
#include "network.h"
#include "audio.h"
#include <syslog.h>
#include <malloc.h>
#include "system.h"
#include "web.h"

#define DBG_MAX_BUFF_SIZE 	1024
#define DBG_KEY_BEGIN	 	0x01020408
#define DBG_KEY_END	 		0x10204080

#define DBG_POINTS_MAX	 	20

//#define DBG_LOCK(x);	{printf("%s\n",__func__);pthread_mutex_lock(x);printf("%s OK\n",__func__);}
//#define DBG_UNLOCK(x);	{pthread_mutex_unlock(x);printf("%s OK 2\n",__func__);}
#define DBG_LOCK(x);	pthread_mutex_lock(x);
#define DBG_UNLOCK(x);	pthread_mutex_unlock(x);

static int dbg_send_udp_message();
char* GetSignalType(char cNum, char cName);
char cFileLogStarted;
struct timespec 	dbg_nanotime;
char *dbg_buff;
unsigned int iUseMemory, iMaxUsedMemory, iThreadID, uiLogCnt;
unsigned int dbg_mode;
pthread_mutex_t *dbg_control_mutex;
unsigned int 	dbg_mutex_cnt, iLockCnt, dbg_func_cnt, dbg_mem_cnt, dbg_mem_cnt_cur, dbg_mem_num_cnt;
dbg_mutex_info	*dbg_mutex_data;
dbg_func_info 	*dbg_func_data;
dbg_mem_info 	*dbg_mem_data;
int dbg_sock_udp;
int dbg_broadcast;	
dbg_point_info  dbg_point_list[DBG_POINTS_MAX];
unsigned int 	dbg_points_cnt;
unsigned int 	dbg_MutexLongTime;
unsigned int 	dbg_log_saved;


void *thread_Restart()
{
	pthread_setname_np(pthread_self(), "restarter");
	
	printf("Closing LAN threads\n");
	DelLAN();
	printf("Closing WEB threads\n");
	web_stop();
	printf("\nRestart in emergency mode\n");
	if (uiStartMode & 0x0002) system("./azad debug safe pause &");
		else system("./azad safe pause &");
	exit(2);
}

void RestartInSafeMode()
{
	static pthread_t threadRestart;
	pthread_attr_t tattrRestart;
	pthread_attr_init(&tattrRestart);   
	pthread_attr_setdetachstate(&tattrRestart, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadRestart, &tattrRestart, thread_Restart, NULL);			
}

void dbg_death_signal(int signum)
{
	if (signum == SIGWINCH)
	{
		signal(signum, SIG_DFL);
		return;
	}
	
	setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog ("azad", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	syslog (LOG_DEBUG, "dbg_death_signal: TID: %d(%d), %i, %s - %s\n", 
				(unsigned int)pthread_self(), gettid(), signum, GetSignalType(signum, 0), GetSignalType(signum, 1));
	closelog ();

	if (pthread_mutex_trylock(&dbg_mutex) == 0) pthread_mutex_unlock(&dbg_mutex); else pthread_mutex_unlock(&dbg_mutex);
	
	if ((signum == SIGINT) || (signum == SIGTERM))
	{
		DBG_LOCK(&dbg_mutex);	
		iLogEmailTimer = 0;
		get_ms(&iLogEmailTimer);
		DBG_UNLOCK(&dbg_mutex);			
	}
	
	dbgprintf(1,"dbg_death_signal: TID: %d(%d), %i, %s - %s\n", 
				(unsigned int)pthread_self(), gettid(), signum, GetSignalType(signum, 0), GetSignalType(signum, 1));
				
	if ((signum == SIGTERM) ||
		(signum == SIGFPE) ||
		(signum == SIGKILL) ||
		(signum == SIGSEGV) ||
		(signum == SIGALRM) ||
		(signum == SIGSTKFLT) ||
		(signum == SIGCHLD) ||
		//(signum == SIGSTOP) ||
		(signum == SIGTSTP) ||
		(signum == SIGQUIT) ||
		(signum == SIGURG) ||
		(((signum == SIGINT) || (signum == SIGTERM)) && (cThreadMainStatus == 4) && (ENABLE_DBG == 2)) ||
		(signum == SIGIOT) ||
		(signum == SIGPIPE) ||		
		(signum == SIGPWR))
	{
		dbg_mem_test_allocated(1);
		dbg_func_print_entered();
		dbg_points_print();
		dbg_mutex_print_locked(0, 1);
		dbg_mem_print_allocated(1);
		char cFileName[256];
		char cFileLog[256];
		char cType[32];
		memset(cType, 0, 32);
		memset(cFileName, 0, 256);
		time_t rawtime;
		struct tm timeinfo;
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		if (signum == SIGTERM) strcpy(cType, "SIGTERM");
		if (signum == SIGFPE) strcpy(cType, "SIGFPE");
		if (signum == SIGKILL) strcpy(cType, "SIGKILL");
		if (signum == SIGSEGV) strcpy(cType, "SIGSEGV");
		if (signum == SIGALRM) strcpy(cType, "SIGALRM");
		if (signum == SIGSTKFLT) strcpy(cType, "SIGSTKFLT");
		if (signum == SIGCHLD) strcpy(cType, "SIGCHLD");
		if (signum == SIGSTOP) strcpy(cType, "SIGSTOP");
		if (signum == SIGTSTP) strcpy(cType, "SIGTSTP");
		if (signum == SIGQUIT) strcpy(cType, "SIGQUIT");
		if (signum == SIGURG) strcpy(cType, "SIGURG");
		if (signum == SIGINT) strcpy(cType, "SIGINT");
		if (signum == SIGIOT) strcpy(cType, "SIGIOT");
		if (signum == SIGPIPE) strcpy(cType, "SIGPIPE");
		if (signum == SIGPWR) strcpy(cType, "SIGPWR");
		sprintf(cFileName, "Crash_%s_%04i_%02i_%02i__%02i_%02i_%02i.log", cType, 
					timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);	
		DBG_LOCK(&dbg_mutex);
		memcpy(cFileLog, cFileLogName, 256);
		int saved = dbg_log_saved;
		DBG_UNLOCK(&dbg_mutex);
		if (saved == 0) 
		{
			CopyFile(cFileLog, cFileName, NULL, 1, 0, 0, 0, NULL, 20, 20, 0);
			DBG_LOCK(&dbg_mutex);
			dbg_log_saved = 1;
			DBG_UNLOCK(&dbg_mutex);			
		}
		if ((signum != SIGTERM) && (signum != SIGKILL) && (uiStartMode & 0x0008))
		{
			RestartInSafeMode();
			usleep(10000000);
		}
		signal(signum, SIG_DFL);
		exit(0);
		//if ((signum != SIGINT) && (signum != SIGTERM)) dbg_on(DBG_MODE_LOG_FUNC | DBG_MODE_TRACK_FUNC);// | DBG_MODE_EXIT
    }
	if (((signum == SIGINT) || (signum == SIGTERM)) && (cThreadMainStatus == 0))
	{
		System_Exit(NULL, 0);
		cThreadMainStatus = 4;
	}
	else
	{
		if (uiStartMode & 0x0008)
		{
			RestartInSafeMode();
			usleep(10000000);
		}
		signal(signum, SIG_DFL);
		exit(0);
	}
}  

int gettid()
{
	return (int)syscall(SYS_gettid);
}

unsigned int dbg_max_mem()
{
	unsigned int uiret = 0;
	DBG_LOCK(&dbg_mutex);
	uiret = iMaxUsedMemory;
	DBG_UNLOCK(&dbg_mutex);
	return uiret;	
}

unsigned int dbg_use_mem()
{
	unsigned int uiret = 0;
	DBG_LOCK(&dbg_mutex);
	uiret = iUseMemory;
	DBG_UNLOCK(&dbg_mutex);
	return uiret;	
}

unsigned int load_last_dbg_data(char *data, unsigned int len)
{
	unsigned int res = 0;
	DBG_LOCK(&dbg_mutex);
	FILE *f;
	if ((f = fopen(cFileLogName,"rb")) == NULL)
	{
		printf("Error open for save file:%s\n", cFileLogName);
		DBG_UNLOCK(&dbg_mutex);
		return 0;
	}
	if (fseek (f, 0L, SEEK_END) < 0) 
	{
		printf("Error seek file %s \n",cFileLogName);
		fclose(f);
		DBG_UNLOCK(&dbg_mutex);
		return 0;
	}  
	unsigned int uiPos = (unsigned int)ftell(f);
	if (uiPos == LONG_MAX) 
	{
		printf("Error tell file %s \n",cFileLogName);
		fclose(f);
		DBG_UNLOCK(&dbg_mutex);
		return 0;
	}
	if (uiPos < len) len = uiPos;
	uiPos -= len;
	if (fseek(f, uiPos, SEEK_SET) >= 0) 
	{
		res = fread(data, 1, len, f);
	}
	else 
	{
		res = 0;
		printf("Error seek file %s \n",cFileLogName);
	}	
	fclose(f);
	DBG_UNLOCK(&dbg_mutex);
	return res;
}
 
int64_t get_now_ms(int64_t *previous_ms) 
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	if ((*previous_ms) == 0)
	{
		*previous_ms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
		return 0;
	}
	return ((ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000) - (*previous_ms);
}

uint32_t get_tick_ms() 
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	return ((ts.tv_sec & 15) * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
}

char* GetSignalType(char cNum, char cName)
{
	switch(cNum)
	{
		case 1: if (cName == 0) return "SIGHUP"; else return "Hangup";
		case 2: if (cName == 0) return "SIGINT"; else return "Terminal interrupt";
		case 3: if (cName == 0) return "SIGQUIT"; else return "Terminal quit";
		case 4: if (cName == 0) return "SIGILL"; else return "Illegal instruction";
		case 5: if (cName == 0) return "SIGTRAP"; else return "Trace trap";
		case 6: if (cName == 0) return "SIGIOT"; else return "IOT Trap";
		case 7: if (cName == 0) return "SIGBUS"; else return "BUS error";
		case 8: if (cName == 0) return "SIGFPE"; else return "Floating point exception";
		case 9: if (cName == 0) return "SIGKILL"; else return "Kill(can't be caught or ignored)";
		case 10: if (cName == 0) return "SIGUSR1"; else return "User defined signal 1";
		case 11: if (cName == 0) return "SIGSEGV"; else return "Invalid memory segment access";
		case 12: if (cName == 0) return "SIGUSR2"; else return "User defined signal 2";
		case 13: if (cName == 0) return "SIGPIPE"; else return "Write on a pipe with no reader, Broken pipe";
		case 14: if (cName == 0) return "SIGALRM"; else return "Alarm clock";
		case 15: if (cName == 0) return "SIGTERM"; else return "Termination";
		case 16: if (cName == 0) return "SIGSTKFLT"; else return "Stack fault";
		case 17: if (cName == 0) return "SIGCHLD"; else return "Child process has stopped or exited, changed";
		case 18: if (cName == 0) return "SIGCONT"; else return "Continue executing, if stopped";
		case 19: if (cName == 0) return "SIGSTOP"; else return "Stop executing(can't be caught or ignored)";
		case 20: if (cName == 0) return "SIGTSTP"; else return "Terminal stop signal";
		case 21: if (cName == 0) return "SIGTTIN"; else return "Background process trying to read, from TTY";
		case 22: if (cName == 0) return "SIGTTOU"; else return "Background process trying to write, to TTY";
		case 23: if (cName == 0) return "SIGURG"; else return "Urgent condition on socket";
		case 24: if (cName == 0) return "SIGXCPU"; else return "CPU limit exceeded";
		case 25: if (cName == 0) return "SIGXFSZ"; else return "File size limit exceeded";
		case 26: if (cName == 0) return "SIGVTALRM"; else return "Virtual alarm clock";
		case 27: if (cName == 0) return "SIGPROF"; else return "Profiling alarm clock";
		case 28: if (cName == 0) return "SIGWINCH"; else return "Window size change";
		case 29: if (cName == 0) return "SIGIO"; else return "I/O now possible";
		case 30: if (cName == 0) return "SIGPWR"; else return "Power failure restart";
		default: return "Unknown";
	}
}
/*
char dbg_printf_ms(char *Buff, ...)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	int64_t previous_ms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
	
	return ((ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000) - (*previous_ms);
	
	
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);

    char timebuffer[256];
	char textbuffer[256];
        	
        memset(timebuffer, 0 , 256);
        sprintf(timebuffer,"%i.%i.%i %i:%i:%i >>> ",timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);

	memset(textbuffer, 0 , 256);
        
	va_list valist;
	va_start(valist, Buff);		
	vsprintf(textbuffer, Buff, valist);		
	va_end(valist);
	fputs(timebuffer, f);
	fputs(textbuffer, f);
	fputs("\n", f);
	
	fclose(f);
	return 1;
}*/

char dbgprintf(unsigned char cLevel, char *Buff, ...)
{
	char tmsbuffer[32];
	char timebuffer[256];
	char textbuffer[256];
	unsigned char cLevel2 = cLevel & 127;
	
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int64_t tms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
	memset(tmsbuffer, 0 , 32);
	snprintf(tmsbuffer, 32, "%i", (unsigned int)tms);		
	
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	memset(timebuffer, 0 , 256);
	snprintf(timebuffer, 255, " <<< %i.%i.%i %i:%i:%i >>> ", timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);		
	

	DBG_LOCK(&dbg_mutex);
	//if (ret != 0) DBG_UNLOCK(&dbg_mutex);
	if ((iFileLog >= cLevel2) || (iScreenLog >= cLevel2) || (iMessageLog >= cLevel2) || (iLocalMessageLog >= cLevel2) || (iEmailLog >= cLevel2))
	{		
		memset(textbuffer, 0 , 256);
		if (strlen(Buff) < 150)
		{
			va_list valist;
			va_start(valist, Buff);	
			vsnprintf(textbuffer, 255, Buff, valist);	
			va_end(valist);
		} 
		else
		{
			memcpy(textbuffer, Buff, 150);
			strcat(textbuffer, " ... VERY BIG LEN STRING\n");
		}
	}
	if (iFileLog >= cLevel2)
	{		
		if (cFileLogStarted == 0) 
		{
			//remove(cFileLogName); //Начать лог заново
			cFileLogStarted = 1;
		}
		FILE *f;
		if ((f = fopen(cFileLogName,"a")) == NULL)
		{
			printf("Error open for save file:%s\n", cFileLogName);
			return 0;
		}		
		fputs(timebuffer, f);		//fputs("\n", f);	
		fputs(textbuffer, f);		//fputs("\n", f);	
		if (uiLogCnt == 0)
		{
			if ((ftell(f)/1000000) > 10)
			{
				printf("SIZE FILE LOG BIG: clear\n");
				fclose(f);
				f = NULL;
				SendDbgUDPMessage(TYPE_MESSAGE_TEXT, "SIZE FILE LOG BIG, clear", strlen("SIZE FILE LOG BIG, clear"), cLogIP);
				remove(cFileLogName);
			}
		}
		if (f) fclose(f);		
		uiLogCnt++;
		if (uiLogCnt == 100) uiLogCnt = 0;
	}
	if (iScreenLog >= cLevel2) 
	{
		if (cLevel2 == 1) printf("\033[31m%i%s %s\033[0m", (unsigned int)tms, timebuffer, textbuffer); 
		if (cLevel2 == 2) printf("\033[33m%i%s %s\033[0m", (unsigned int)tms, timebuffer, textbuffer); 
		if (cLevel2 == 3) printf("%i%s %s", (unsigned int)tms, timebuffer, textbuffer);
		if (cLevel2 == 4) printf("\033[32m%i%s %s\033[0m", (unsigned int)tms, timebuffer, textbuffer); 
		if (cLevel2 > 4) printf("\033[36m%i%s %s\033[0m", (unsigned int)tms, timebuffer, textbuffer); 		
	}	
	if (iMessageLog >= cLevel2)
	{
		SendDbgUDPMessage(TYPE_MESSAGE_TEXT, textbuffer, strlen(textbuffer), cLogIP);		
	}	

	if (iLocalMessageLog >= cLevel2)
	{
		SendDbgUDPMessage(TYPE_MESSAGE_TEXT, textbuffer, strlen(textbuffer), "127.0.0.1");		
	}

	if ((iEmailLog >= cLevel2) || cLogMlList.ExtraTextSize)
	{
		if (iEmailLog >= cLevel2)
		{
			int textLength = strlen(textbuffer) + strlen(tmsbuffer) + strlen(timebuffer) + 2;	
			if ((cLogMlList.ExtraTextSize + textLength) < 1000000)
				cLogMlList.ExtraText = (char*)realloc(cLogMlList.ExtraText, cLogMlList.ExtraTextSize + textLength);
				else
				{
					if (textLength > 1000000) textLength = 1000000;
					if (cLogMlList.ExtraTextSize)
					{
						cLogMlList.ExtraTextSize = 1000000-textLength;
						memmove(cLogMlList.ExtraText, &cLogMlList.ExtraText[textLength], cLogMlList.ExtraTextSize);
					}
				}
			memset(&cLogMlList.ExtraText[cLogMlList.ExtraTextSize], 0, textLength);
			
			memcpy(&cLogMlList.ExtraText[cLogMlList.ExtraTextSize], tmsbuffer, strlen(tmsbuffer));
			cLogMlList.ExtraTextSize += strlen(tmsbuffer);
			memcpy(&cLogMlList.ExtraText[cLogMlList.ExtraTextSize], timebuffer, strlen(timebuffer));
			cLogMlList.ExtraTextSize += strlen(timebuffer);
			memcpy(&cLogMlList.ExtraText[cLogMlList.ExtraTextSize], textbuffer, strlen(textbuffer));
			cLogMlList.ExtraTextSize += strlen(textbuffer);
			memcpy(&cLogMlList.ExtraText[cLogMlList.ExtraTextSize], "\r\n", strlen("\r\n"));
			cLogMlList.ExtraTextSize += strlen("\r\n");
		}
		
		if ((unsigned int)get_ms(&iLogEmailTimer) > uiLogEmailPauseSize)
		{
			/*memset(cLogMlList.BodyText, 0, 64);
			if (strlen(textbuffer) >= 64) 
				memcpy(cLogMlList.BodyText, textbuffer, 63);
			else
				strcpy(cLogMlList.BodyText, textbuffer);
			cLogMlList.ExtraTextSize = 0;*/ 
			iLogEmailTimer = 0;
			get_ms(&iLogEmailTimer);
			DBG_UNLOCK(&dbg_mutex);
			SendMailFile(cLogMailLogin, cLogMailPassword, cLogMailServer, cLogMailAddress, cLogMailAuth, &cLogMlList);
			DBG_LOCK(&dbg_mutex);
			cLogMlList.ExtraTextSize = 0;
			free(cLogMlList.ExtraText);
			cLogMlList.ExtraText = NULL;
			DBG_UNLOCK(&dbg_mutex);
			return 1;	
		} //printf("Timer EMAIL LOG %i\n", (unsigned int)get_ms(&iLogEmailTimer));
	}
	
	DBG_UNLOCK(&dbg_mutex);
	return 1;
}

char dbgprintfw(unsigned char cLevel, char *Buff, ...)
{
	char timebuffer[256];
	char textbuffer[256];
	
	if ((iFileLog >= cLevel) || (iScreenLog >= cLevel))
	{
		time_t rawtime;
		struct tm timeinfo;
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);

		memset(timebuffer, 0 , 256);
		memset(textbuffer, 0 , 256);
		va_list valist;
		va_start(valist, Buff);		
		vsprintf(textbuffer, Buff, valist);		
		va_end(valist);
		sprintf(timebuffer,"%i.%i.%i %i:%i:%i >>> ",timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
	}
	if (iFileLog >= cLevel)
	{
		if (cFileLogStarted == 0) 
		{
			//remove(cFileLogName); //Начать лог заново
			cFileLogStarted = 1;
		}
		FILE *f;
		if ((f = fopen(cFileLogName,"a")) == NULL)
		{
			printf("Error open for save file:%s\n", cFileLogName);
			return 0;
		}		
		fputs(timebuffer, f);		//fputs("\n", f);	
		fputs(textbuffer, f);		//fputs("\n", f);	
		if (uiLogCnt == 0)
		{
			if ((ftell(f)/1000000) > 10)
			{
				fputs("SIZE FILE LOG BIG: restart log\n", f);
				fclose(f);
				f = NULL;
				remove(cFileLogName);
				//exit(0);
			}
		}
		fclose(f);
		uiLogCnt++;
		if (uiLogCnt == 100) uiLogCnt = 0;		
	} 
	if (iScreenLog >= cLevel) 
	{
		printf(textbuffer);
	}
	return 1;
}

int InitDbgLan()
{	
    int flags, n;
	dbg_broadcast = 0;
	
    if ((dbg_sock_udp=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)  
    {
		printf("[DBG] Error create udp socket\n");
		return 0;
    }       
    
	/*memset((char *) &my_addr_udp, 0, sizeof(my_addr_udp));     
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_port = htons(UDP_PORT);
    my_addr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if(bind(dbg_sock_udp , (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp) ) == -1)
    {
        dbgprintf(1 | 128,"Error bind udp socket\n");
        DBG_LOG_OUT();
        return 0;
    }  
	
	if (listen(sock, MAX_CONNECTIONS) == -1) 
	{
        dbgprintf(1 | 128,"Error listen tcp socket\n");
        return 0;
    }*/
    
	flags = fcntl(dbg_sock_udp, F_GETFL, 0);
	if (fcntl(dbg_sock_udp, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		printf("[DBG] error set udp nonblock\n");
		DBG_LOG_OUT();
        return 0;
	}
	
	if (setsockopt(dbg_sock_udp, SOL_SOCKET, SO_BROADCAST, &dbg_broadcast, sizeof(dbg_broadcast)) == -1) 
	{
        printf("[DBG] setsockopt (SO_BROADCAST)\n");
        return 0;
    }	
	n = 320*1024;
	if (setsockopt(dbg_sock_udp, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) 
	{
        printf("[DBG] setsockopt (SO_RCVBUF)\n");
        DBG_LOG_OUT();
        return 0;
    }		
	n = 320*1024;
	if (setsockopt(dbg_sock_udp, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)) == -1) 
	{
        printf("[DBG] setsockopt (SO_SNDBUF)\n");
        DBG_LOG_OUT();
        return 0;
    }
	return 1;
}

int SendDbgUDPMessage(unsigned int Type, char *cData, int iLen, char *cAddr)
{
	struct sockaddr_in LogAddress;
	LogAddress.sin_family 		= AF_INET;
	LogAddress.sin_port 		= htons(UDP_PORT);
	LogAddress.sin_addr.s_addr = inet_addr(cAddr);	
	
		
	unsigned int *pCRC;
	char *trOutBuffer = malloc(iLen+sizeof(TRANSFER_DATA)+sizeof(int));
	memset(trOutBuffer, 0, iLen+sizeof(TRANSFER_DATA)+sizeof(int));
	TRANSFER_DATA *trOutHeaderData;	
	trOutHeaderData = (TRANSFER_DATA*)trOutBuffer;
	trOutHeaderData->Key1 = TRANSFER_KEY1; 
	trOutHeaderData->Key2 = TRANSFER_KEY2; 
	trOutHeaderData->Key3 = TRANSFER_KEY3;
	trOutHeaderData->TypeMessage = Type;
	trOutHeaderData->SizeMessage = sizeof(TRANSFER_DATA) + iLen + sizeof(int);
	if (iLen == 0) trOutHeaderData->FlagMessage = FLAG_MESSAGE_EMPTY_DATA; 
	else 
	{
		trOutHeaderData->FlagMessage = FLAG_MESSAGE_EXIST_DATA;
		if (iLen != 0) memcpy(trOutBuffer + sizeof(TRANSFER_DATA), cData, iLen);
	}
	pCRC = (unsigned int*)(trOutBuffer + (trOutHeaderData->SizeMessage - sizeof(int)));
	*pCRC = CalcCRC(trOutBuffer, trOutHeaderData->SizeMessage - sizeof(int));	
	memset(&trOutHeaderData->Address, 0, sizeof(trOutHeaderData->Address));     
	trOutHeaderData->Address.sin_family = AF_INET;
	trOutHeaderData->Address.sin_port = htons(UDP_PORT);
		//printf("%i, %i\n",htonl(INADDR_dbg_broadcast),(int)Address);
	trOutHeaderData->Address.sin_addr.s_addr = LogAddress.sin_addr.s_addr;	
	//if (Type == TYPE_MESSAGE_MODULE_SET) omx_dump_data("packet.bin", trOutBuffer, trOutHeaderData->SizeMessage);			
		//printf("To send : %i/%i\n",trOutHeaderData->SizeMessage, ConnInfo->OutDataSize);		
	
	if ((LogAddress.sin_addr.s_addr == 0) && (dbg_broadcast == 0))
	{
		dbg_broadcast = 1;
		if (setsockopt(dbg_sock_udp, SOL_SOCKET, SO_BROADCAST, &dbg_broadcast, sizeof(dbg_broadcast)) == -1) 
			printf("[DBG] setsockopt (SO_BROADCAST) to 1\n");																		
	}
	if ((LogAddress.sin_addr.s_addr != 0) && (dbg_broadcast == 1))
	{
		dbg_broadcast = 0;
		if (setsockopt(dbg_sock_udp, SOL_SOCKET, SO_BROADCAST, &dbg_broadcast, sizeof(dbg_broadcast)) == -1) 
			printf("[DBG] setsockopt (SO_BROADCAST) to 0\n");																	
	}
	if (dbg_sock_udp)
	{
		if (sendto(dbg_sock_udp, trOutBuffer, trOutHeaderData->SizeMessage, 0, (struct sockaddr*) &LogAddress, sizeof(LogAddress)) != trOutHeaderData->SizeMessage)
			printf("[DBG] error sendto to %s\n", cAddr);
		//else printf("Send udp (%s:%d) %i\n", inet_ntoa(LogAddress.sin_addr), ntohs(LogAddress.sin_port), trOutHeaderData->SizeMessage);	
	}							
	return 1;
}

int CloseDbgLan()
{
	close(dbg_sock_udp);
	return 1;
}

int DbgCmpStr(char *cData1, char *cData2)
{
	int iData1Len = strlen(cData1);
	int iData2Len = strlen(cData2);
	int n;
	if (iData1Len != iData2Len) return 0;
	for (n = 0; n < iData1Len; n++)
		if (cData1[n] != cData2[n]) return 0;
	//DBG_LOG_OUT();
	return 1;
}

void dbg_point(int iInfo, int iLine)
{
	DBG_LOCK(&dbg_mutex);
	dbg_points_cnt++;
	if (dbg_points_cnt >= DBG_POINTS_MAX) dbg_points_cnt = 0;
	dbg_point_list[dbg_points_cnt].pid = pthread_self();
	dbg_point_list[dbg_points_cnt].line = iLine;
	dbg_point_list[dbg_points_cnt].mark = 0;
	dbg_point_list[dbg_points_cnt].info = iInfo;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	dbg_point_list[dbg_points_cnt].time = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
		
	DBG_UNLOCK(&dbg_mutex);
}

int dbg_init(unsigned int iMode, unsigned int uiMutexTime)
{			
	//if (iMode & DBG_MODE_LOG_FUNC)
	{
		char n;
		for (n = 1; n != 30; n++) 
			if ((n != SIGPIPE) && 
				(n != SIGCHLD) &&
				(n != SIGSTOP) &&
				(n != SIGCONT)) signal(n, dbg_death_signal);
						else signal(n, SIG_IGN);
	}
	iUseMemory = 0;
	iMaxUsedMemory = 0;
	iThreadID = 0;
	dbg_mode = iMode;
	dbg_buff = (char*)malloc(DBG_MAX_BUFF_SIZE);
	dbg_control_mutex = NULL;
	dbg_mutex_cnt = 0;
	dbg_mutex_data = NULL;
	iLockCnt = 0;
	dbg_func_cnt = 0;
	dbg_func_data = NULL;
	dbg_mem_cnt = 0;
	dbg_mem_num_cnt = 0;
	dbg_mem_cnt_cur = 0;
	dbg_mem_data = NULL;
	uiLogCnt = 0;
	cFileLogStarted = 0;
	dbg_log_saved = 0;
	
	dbg_MutexLongTime = uiMutexTime;
	
	pthread_mutex_init(&dbg_mutex, NULL);
	
	memset(dbg_point_list, 0, sizeof(dbg_point_info)*DBG_POINTS_MAX);
	dbg_points_cnt = DBG_POINTS_MAX;
	InitDbgLan();
	
	return 1;	
}

int dbg_close()
{
	dbg_off();
	
	CloseDbgLan();
	pthread_mutex_destroy(&dbg_mutex);
	free(dbg_buff);
	return 1;
}

void dbg_on(unsigned int iMode)
{
	DBG_LOCK(&dbg_mutex);
	dbg_mode = iMode;
	if (dbg_mode & DBG_MODE_TRACK_FUNC_THREAD) iThreadID = pthread_self();
	DBG_UNLOCK(&dbg_mutex);
}

void dbg_off()
{
	DBG_LOCK(&dbg_mutex);
	dbg_mode = DBG_MODE_OFF;
	iThreadID = 0;
	DBG_UNLOCK(&dbg_mutex);		
}

static int dbg_send_udp_message(char *pMessage)
{	
	int n;
	int i = strlen(pMessage);
	for (n = 0; n != i; n++) if (pMessage[n] == 9) pMessage[n] = 124;
	dbgprintfw(1,"[DBG] %s\n",pMessage);	
	return 1;
}

int dbg_log_in(char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);
	unsigned int uiTID = pthread_self();
	int i;	
	if (dbg_mode & DBG_MODE_LOG_FUNC) 
	{
		for (i = 0; i < dbg_func_cnt;i++) if (dbg_func_data[i].status == 0) break;
		//printf("%i %i %i %s %s\n", i, dbg_func_cnt, DBG_MAX_FUNC_CNT, pFileName, pFuncName);
		
		if (i == dbg_func_cnt)
		{
			if (dbg_func_cnt != DBG_MAX_FUNC_CNT)
			{
				dbg_func_cnt++;
				//dbg_func_cnt = DBG_MAX_FUNC_CNT;
				dbg_func_data = (dbg_func_info*)realloc(dbg_func_data, dbg_func_cnt*sizeof(dbg_func_info));
				if (dbg_func_data == NULL) printf("Error realloc\n");
				memset(&dbg_func_data[dbg_func_cnt-1], 0, sizeof(dbg_func_info));
				//memset(dbg_func_data, 0, dbg_func_cnt*sizeof(dbg_func_info));
				//printf("[DBG] DBG_MAX_FUNC_CNT++ %i\n",dbg_func_cnt);		
			}
			else 
			{
				printf("[DBG] !!!!!!!!! DBG_MAX_FUNC_CNT overfull: %s\n", pFuncName);				
				dbgprintfw(1,"[DBG] DBG_MAX_FUNC_CNT overfull: %s\n", pFuncName);		
				i--;
			}
		}// else dbgprintfw(1,"[DBG] DBG_MAX_FUNC_CNT exist\n");
		
		dbg_func_data[i].tid = uiTID;
		dbg_func_data[i].stid = gettid();		
		dbg_func_data[i].status = 1;
		dbg_func_data[i].line = iLine;
		memset(dbg_func_data[i].filename, 0, 32);
		memset(dbg_func_data[i].funcname, 0, 64);
		if (strlen(pFileName) < 32) strcpy(dbg_func_data[i].filename, pFileName);
			else memcpy(dbg_func_data[i].filename, pFileName, 31);
		if (strlen(pFuncName) < 64) strcpy(dbg_func_data[i].funcname, pFuncName);
			else memcpy(dbg_func_data[i].funcname, pFuncName, 63);		
	}

	if ((dbg_mode & DBG_MODE_TRACK_FUNC) || ((dbg_mode & DBG_MODE_TRACK_FUNC_THREAD) && (iThreadID == uiTID)))
	{		
		clock_gettime(CLOCK_REALTIME, &dbg_nanotime);	
		memset(dbg_buff, 0, DBG_MAX_BUFF_SIZE);
		
		sprintf(dbg_buff, "Thread:%i(%i) \t Time:%i \t File:'%s' \t Func In :'%s' \t Line:%d", uiTID, gettid(), (unsigned int)(dbg_nanotime.tv_nsec/1000000), pFileName, pFuncName, iLine);
		dbg_send_udp_message(dbg_buff);
	}
	DBG_UNLOCK(&dbg_mutex);	
	return 1;
}

int dbg_log_out(char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);
	
	unsigned int uiTID = pthread_self();
		
	if (dbg_mode & DBG_MODE_LOG_FUNC)
	{
		int i;
		for (i = 0; i < dbg_func_cnt;i++)
			if ((dbg_func_data[i].status == 1) &&
				(dbg_func_data[i].tid == uiTID) &&
				(strncmp(dbg_func_data[i].funcname, pFuncName, 64) == 0) 
				&& (strncmp(dbg_func_data[i].filename, pFileName, 32) == 0)
				)
			{
				dbg_func_data[i].status = 0;
				break;
			}
		if (i == dbg_func_cnt) 
		{
			dbgprintfw(1,"[DBG] dbg_log_out not found: %s\n", pFuncName);	
			DBG_UNLOCK(&dbg_mutex);
			dbg_func_print_entered();
			DBG_LOCK(&dbg_mutex);
			exit(0);
		}
	}
	
	if ((dbg_mode & DBG_MODE_TRACK_FUNC) || ((dbg_mode & DBG_MODE_TRACK_FUNC_THREAD) && (iThreadID == uiTID)))
	{		
		clock_gettime(CLOCK_REALTIME, &dbg_nanotime);	
		memset(dbg_buff, 0, DBG_MAX_BUFF_SIZE);
		
		sprintf(dbg_buff, "Thread:%i(%i) \t Time:%i \t File:'%s' \t Func Out:'%s' \t Line:%d", (unsigned int)pthread_self(), gettid(), (unsigned int)(dbg_nanotime.tv_nsec/1000000), pFileName, pFuncName, iLine);
		dbg_send_udp_message(dbg_buff);
	}
	if (dbg_mode & DBG_MODE_EXIT)
	{
		dbgprintfw(1,"DBG_MODE_EXIT\n");	
		DBG_UNLOCK(&dbg_mutex);
		exit(1);
	}
	DBG_UNLOCK(&dbg_mutex);	
	return 1;
}

static void dbg_set_key(void* mem, unsigned int iSize)
{
	unsigned int *pSize;
	pSize = (unsigned int*)mem;
	pSize[0] = iSize;
	pSize[1] = DBG_KEY_BEGIN;
	pSize = (unsigned int*)(mem + iSize - sizeof(unsigned int));
	pSize[0] = DBG_KEY_END;
}

char dbg_test_key(void* ptr, char *pFileName, const char *pFuncName, int iLine)
{
	if (!(dbg_mode & DBG_MODE_MEMORY_ERROR)) return 1;	
	unsigned int *pSize;
	unsigned int iSize;
	void* mem = ptr - (sizeof(unsigned int) * 2);
	pSize = (unsigned int*)mem;
	if (pSize[1] != DBG_KEY_BEGIN)
	{
		dbgprintfw(1,"[DBG] memory corrupted DBG_KEY_BEGIN, File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);
		return 0;
	}
	
	iSize = pSize[0];
	pSize = (unsigned int*)(mem + iSize - sizeof(unsigned int));
	if (pSize[0] != DBG_KEY_END)
	{
		dbgprintfw(1,"[DBG] memory corrupted DBG_KEY_END, File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);
		return 0;
	}
	return 1;
}

char dbg_test(void* ptr)
{
	if (!(dbg_mode & DBG_MODE_MEMORY_ERROR)) return 1;	
	unsigned int *pSize;
	unsigned int iSize;
	char res = 0;
	void* mem = ptr - (sizeof(unsigned int) * 2);
	pSize = (unsigned int*)mem;
	if (pSize[1] != DBG_KEY_BEGIN) res |= 1;
	
	iSize = pSize[0];
	pSize = (unsigned int*)(mem + iSize - sizeof(unsigned int));
	if (pSize[0] != DBG_KEY_END) res |= 2;
	return res;
}

void *dbg_calloc(size_t nmemb, size_t size, char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);	
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);
	
	void *mem;
	int mode = dbg_mode;
	if (dbg_mode & DBG_MODE_LOG_MEM) 
	{
		unsigned int iSize = nmemb * size + (sizeof(unsigned int) * 3);
		mem = malloc(iSize);
		if (mem == NULL) dbgprintfw(1,"[DBG] Error allocate memory, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);				
		dbg_mem_cnt_cur++;				
		memset(mem, 0, iSize);
		
		dbg_set_key(mem, iSize);
		
		iUseMemory += iSize;
		if (iUseMemory > iMaxUsedMemory) 
		{
			iMaxUsedMemory = iUseMemory;
			//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
		}		
	
		int i;
		for (i = 0; i != dbg_mem_cnt;i++) if (dbg_mem_data[i].point == NULL) break;
		if (i == dbg_mem_cnt)
		{
			if (dbg_mem_cnt != DBG_MAX_MEM_CNT)
			{
				dbg_mem_cnt++;
				dbg_mem_data = (dbg_mem_info*)realloc(dbg_mem_data, dbg_mem_cnt*sizeof(dbg_mem_info));
				if (dbg_mem_data == NULL) dbgprintfw(1,"[DBG] Error allocate memory 'realloc' internal\n");				
				memset(&dbg_mem_data[dbg_mem_cnt-1], 0, sizeof(dbg_mem_info));
				//printf("[DBG] DBG_MAX_FUNC_CNT++ %i\n",dbg_func_cnt);		
			}
			else 
			{
				dbgprintfw(1,"[DBG] DBG_MAX_MEM_CNT overfull: %s\n", pFuncName);		
				i--;
			}
		}// else dbgprintfw(1,"[DBG] DBG_MAX_MEM_CNT exist\n");
		dbg_mem_num_cnt++;
		dbg_mem_data[i].number = dbg_mem_num_cnt;
		dbg_mem_data[i].status = 1;
		dbg_mem_data[i].point = mem;
		dbg_mem_data[i].size = iSize;
		dbg_mem_data[i].line = iLine;
		memset(dbg_mem_data[i].filename, 0, 32);
		memset(dbg_mem_data[i].funcname, 0, 64);
		if (strlen(pFileName) < 32) strcpy(dbg_mem_data[i].filename, pFileName);
			else memcpy(dbg_mem_data[i].filename, pFileName, 31);
		if (strlen(pFuncName) < 64) strcpy(dbg_mem_data[i].funcname, pFuncName);
			else memcpy(dbg_mem_data[i].funcname, pFuncName, 63);		
	} 
	else 
	{
		mem = calloc(nmemb, size);
		iUseMemory += size;
		if (iUseMemory > iMaxUsedMemory) 
		{
			iMaxUsedMemory = iUseMemory;
			//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
		}
	}
	
	DBG_UNLOCK(&dbg_mutex);	
	
	dbg_log_mem_short();
	if (mode & DBG_MODE_LOG_MEM) 
		return (mem + (sizeof(unsigned int) * 2));
		else
		return mem;
}

void *dbg_malloc(size_t size, char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);	
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);
	
	void *mem;
	int mode = dbg_mode;
	if (dbg_mode & DBG_MODE_LOG_MEM) 
	{
		unsigned int iSize = size + (sizeof(unsigned int) * 3);
		mem = malloc(iSize);
		if (mem == NULL) dbgprintfw(1,"[DBG] Error allocate memory, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);				
		dbg_mem_cnt_cur++;				
		dbg_set_key(mem, iSize);
		
		iUseMemory += iSize;
		if (iUseMemory > iMaxUsedMemory) 
		{
			iMaxUsedMemory = iUseMemory;
			//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
		}
	
	
		int i;
		for (i = 0; i != dbg_mem_cnt;i++) if (dbg_mem_data[i].status == 0) break;
		if (i == dbg_mem_cnt)
		{
			if (dbg_mem_cnt != DBG_MAX_MEM_CNT)
			{
				dbg_mem_cnt++;
				dbg_mem_data = (dbg_mem_info*)realloc(dbg_mem_data, dbg_mem_cnt*sizeof(dbg_mem_info));
				if (dbg_mem_data == NULL) dbgprintfw(1,"[DBG] Error allocate memory 'realloc' internal\n");				
				memset(&dbg_mem_data[dbg_mem_cnt-1], 0, sizeof(dbg_mem_info));
				//printf("[DBG] DBG_MAX_FUNC_CNT++ %i\n",dbg_func_cnt);		
			}
			else 
			{
				dbgprintfw(1,"[DBG] DBG_MAX_MEM_CNT overfull: %s\n", pFuncName);		
				i--;
			}
		}// else dbgprintfw(1,"[DBG] DBG_MAX_MEM_CNT exist\n");
		dbg_mem_num_cnt++;
		dbg_mem_data[i].number = dbg_mem_num_cnt;
		dbg_mem_data[i].status = 1;
		dbg_mem_data[i].point = mem;
		dbg_mem_data[i].size = iSize;
		dbg_mem_data[i].line = iLine;
		memset(dbg_mem_data[i].filename, 0, 32);
		memset(dbg_mem_data[i].funcname, 0, 64);
		if (strlen(pFileName) < 32) strcpy(dbg_mem_data[i].filename, pFileName);
			else memcpy(dbg_mem_data[i].filename, pFileName, 31);
		if (strlen(pFuncName) < 64) strcpy(dbg_mem_data[i].funcname, pFuncName);
			else memcpy(dbg_mem_data[i].funcname, pFuncName, 63);		
	} 
	else 
	{
		mem = malloc(size);
		if (mem == NULL) dbgprintfw(1,"[DBG] Error allocate memory, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);		
		iUseMemory += size;
		if (iUseMemory > iMaxUsedMemory) 
		{
			iMaxUsedMemory = iUseMemory;
			//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
		}
	}
	
	DBG_UNLOCK(&dbg_mutex);	
	
	dbg_log_mem_short();
	
	if (mode & DBG_MODE_LOG_MEM) 
		return (mem + (sizeof(unsigned int) * 2));
		else
		return mem;
}


void dbg_free(void *ptr, char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);	
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);
	DBG_UNLOCK(&dbg_mutex);	
	
	if (ptr == NULL) 
	{
		dbgprintfw(1,"[DBG] MEMORY_FREE: memory point NULL, File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);
		return;
	}
	
	unsigned int *pSize = ptr - (sizeof(unsigned int) * 2);
	DBG_LOCK(&dbg_mutex);
	
	int mode = dbg_mode;
	
	if (dbg_mode & DBG_MODE_LOG_MEM)
	{
		dbg_test_key(ptr, pFileName, pFuncName, iLine);
		
		iUseMemory -= pSize[0];
		
		int i;
		for (i = 0; i != dbg_mem_cnt;i++)
		{
			//printf("%i, %i\n", (unsigned int)dbg_mem_data[i].point, (unsigned int)pSize);
			if (dbg_mem_data[i].point == pSize)
			{
				dbg_mem_data[i].status = 0;		
				dbg_mem_data[i].point = NULL;
				dbg_mem_data[i].size = 0;
				break;
			}
		}
		if (i == dbg_mem_cnt) 
		{
			dbgprintfw(1,"[DBG] dbg_log_mem free not found: File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);	
			DBG_UNLOCK(&dbg_mutex);
			dbg_mem_print_allocated(1);
			DBG_LOCK(&dbg_mutex);
			exit(0);
		}
		dbg_mem_cnt_cur--;	
	}
	else
	{
		iUseMemory -= malloc_usable_size(ptr);
	}
				
	DBG_UNLOCK(&dbg_mutex);	
	
	dbg_log_mem_short();
	
	if (mode & DBG_MODE_LOG_MEM)
	{
		pSize[0] = 0xFFFFFFFF;
		pSize[1] = 0xFFFFFFFF;
		free(ptr-(sizeof(unsigned int) * 2));
	} 
	else free(ptr);
}

void *dbg_realloc(void *ptr, size_t size, char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);	
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);
	
	void *mem;
	int mode = dbg_mode;
	unsigned int iSize;
			
	if (dbg_mode & DBG_MODE_LOG_MEM) 
	{
		if (ptr)
		{
			dbg_test_key(ptr, pFileName, pFuncName, iLine);
			
			iSize = size + (sizeof(unsigned int) * 3);
			mem = malloc(iSize);	
			if (mem == NULL) dbgprintfw(1,"[DBG] Error reallocate memory, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);		
			iUseMemory += iSize;
					
			unsigned int *pSize = ptr - (sizeof(unsigned int) * 2);
			iUseMemory -= pSize[0];
			if (iUseMemory > iMaxUsedMemory) 
			{
				iMaxUsedMemory = iUseMemory;
				//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
			}
			
			if (pSize[0] <= iSize) 
			{
				memcpy(mem, ptr - (sizeof(unsigned int) * 2), *pSize);
			}
			else
			{
				dbgprintfw(1,"[DBG] realloc to small size File:%s Func:%s Line:%i (%i >>> %i)\n", pFileName, pFuncName, iLine, pSize[0], iSize);
				memcpy(mem, ptr - (sizeof(unsigned int) * 2), iSize);
			}
			dbg_set_key(mem, iSize);
			free(pSize);
		}
		else 
		{
			iSize = size + (sizeof(unsigned int) * 3);
			mem = malloc(iSize);
			if (mem == NULL) dbgprintfw(1,"[DBG] Error reallocate memory, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);		
			dbg_mem_cnt_cur++;
			
			dbg_set_key(mem, iSize);
		
			iUseMemory += iSize;
			if (iUseMemory > iMaxUsedMemory) 
			{
				iMaxUsedMemory = iUseMemory;
				//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
			}
		}
	} 
	else 
	{
		iUseMemory -= malloc_usable_size(ptr);
		mem = realloc(ptr, size);
		if (mem == NULL) dbgprintfw(1,"[DBG] Error reallocate memory, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);		
		iUseMemory += size;
		if (iUseMemory > iMaxUsedMemory) 
		{
			iMaxUsedMemory = iUseMemory;
			//dbgprintfw(5,"[DBG] Inc MaxUsedMemory: %i,  %s, File:'%s' Func:'%s' Line:%d\n", iUseMemory, __func__, pFileName, pFuncName, iLine);
		}
	}
	
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] OUT %s, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);
	
	if (dbg_mode & DBG_MODE_LOG_MEM) 
	{
		int i;
		if (ptr != NULL)
		{
			for (i = 0; i < dbg_mem_cnt;i++)		
			{
				//printf("%i, %i\n", (unsigned int)dbg_mem_data[i].point, (unsigned int)(ptr-(sizeof(unsigned int) * 2)));			
				if (dbg_mem_data[i].point == (ptr-(sizeof(unsigned int) * 2)))
				{
					dbg_mem_data[i].status = 0;		
					dbg_mem_data[i].point = NULL;
					dbg_mem_data[i].size = 0;
					break;
				}
			}
			if (i == dbg_mem_cnt)
			{
				dbgprintfw(1,"[DBG] dbg_log_mem realloc not found: File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);	
				DBG_UNLOCK(&dbg_mutex);
				dbg_mem_print_allocated(1);
				DBG_LOCK(&dbg_mutex);
				exit(0);
			}
		}		
		for (i = 0; i != dbg_mem_cnt;i++) if (dbg_mem_data[i].point == NULL) break;
		if (i == dbg_mem_cnt)
		{
			if (dbg_mem_cnt < DBG_MAX_MEM_CNT)
			{
				dbg_mem_cnt++;
				dbg_mem_data = (dbg_mem_info*)realloc(dbg_mem_data, dbg_mem_cnt*sizeof(dbg_mem_info));
				if (dbg_mem_data == NULL) dbgprintfw(1,"[DBG] Error allocate memory 'realloc' internal\n");				
				memset(&dbg_mem_data[dbg_mem_cnt-1], 0, sizeof(dbg_mem_info));
				//printf("[DBG] DBG_MAX_FUNC_CNT++ %i\n",dbg_func_cnt);		
			}
			else 
			{
				dbgprintfw(1,"[DBG] DBG_MAX_MEM_CNT overfull: %s\n", pFuncName);		
				i--;
			}
		}// else dbgprintfw(1,"[DBG] DBG_MAX_MEM_CNT exist\n");
		dbg_mem_num_cnt++;
		dbg_mem_data[i].number = dbg_mem_num_cnt;
		dbg_mem_data[i].status = 1;		
		dbg_mem_data[i].point = mem;
		dbg_mem_data[i].size = iSize;
		dbg_mem_data[i].line = iLine;
		memset(dbg_mem_data[i].filename, 0, 32);
		memset(dbg_mem_data[i].funcname, 0, 64);
		if (strlen(pFileName) < 32) strcpy(dbg_mem_data[i].filename, pFileName);
			else memcpy(dbg_mem_data[i].filename, pFileName, 31);
		if (strlen(pFuncName) < 64) strcpy(dbg_mem_data[i].funcname, pFuncName);
			else memcpy(dbg_mem_data[i].funcname, pFuncName, 63);		
	}
	DBG_UNLOCK(&dbg_mutex);
	
	dbg_log_mem_short();
	if (mode & DBG_MODE_LOG_MEM) 
		return (mem + (sizeof(unsigned int) * 2));
		else
		return mem;
}

int dbg_log_mem(char *pFileName, const char *pFuncName, int iLine)
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d\n", __func__, pFileName, pFuncName, iLine);
	if (dbg_mode & DBG_MODE_MEMORY_INFO)
	{
		clock_gettime(CLOCK_REALTIME, &dbg_nanotime);	
		memset(dbg_buff, 0, DBG_MAX_BUFF_SIZE);
		
		sprintf(dbg_buff, "Thread:%i(%i) \t Time:%i \t File:'%s' \t Func:'%s' \t Line:%d \t UseMemory:%i \t MaxUsedMemory:%i", (unsigned int)pthread_self(),
					gettid(),(unsigned int)(dbg_nanotime.tv_nsec/1000000), pFileName, pFuncName, iLine, iUseMemory, iMaxUsedMemory);
		dbg_send_udp_message(dbg_buff);
	}
	DBG_UNLOCK(&dbg_mutex);	
	return 1;
}

int dbg_log_mem_short()
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & (DBG_MODE_MEMORY_INFO))
	{
		clock_gettime(CLOCK_REALTIME, &dbg_nanotime);	
		memset(dbg_buff, 0, DBG_MAX_BUFF_SIZE);
		
		sprintf(dbg_buff, "Cnt:%i \t Thread:%i(%i) \t Time:%i \t UseMemory:%i \t MaxUsedMemory:%i", dbg_mem_cnt_cur, (unsigned int)pthread_self(), gettid(),
					(unsigned int)(dbg_nanotime.tv_nsec/1000000), iUseMemory, iMaxUsedMemory);
		dbg_send_udp_message(dbg_buff);
	}
	DBG_UNLOCK(&dbg_mutex);	
	return 1;
}

void dbg_set_mutex(pthread_mutex_t *mutex)
{
	dbg_control_mutex = mutex;
}

/*int dbg_mutex_lock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine)
{
	int ret;	
	DBG_LOCK(&dbg_mutex);
	if ((dbg_mode & DBG_MODE_TRACK_MUTEX) && (dbg_control_mutex == mutex))
	{
		ret = pthread_mutex_trylock(mutex);
		if (ret == 0) 
		{
			dbgprintfw(1,"[DBG] MUTEX LOCK; File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);		
		}
		else
		{
			dbgprintfw(1,"[DBG] MUTEX LOCK (wait); File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);
			ret = pthread_mutex_lock(mutex);
			dbgprintfw(1,"[DBG] MUTEX LOCK (go); File:'%s' Func:'%s' Line:%d\n", pFileName, pFuncName, iLine);			
		}
	}
	else ret = pthread_mutex_lock(mutex);
	DBG_UNLOCK(&dbg_mutex);	
	
	return ret;
}*/

int pthread_mutex_lock_timeout(pthread_mutex_t *mutex, unsigned int timeout_ms)
{
	int ret;
	if (timeout_ms)
	{
		struct timespec   ts, ts2;	
		clock_gettime(CLOCK_REALTIME , &ts);
		/* Convert from timeval to timespec */
		ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
		ts.tv_sec += timeout_ms / 1000;
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec %= (1000 * 1000 * 1000);
		ret = pthread_mutex_timedlock(mutex, &ts);
		if (ret == 0)
		{
			clock_gettime(CLOCK_REALTIME , &ts2);	
			ret = ((ts.tv_sec - ts2.tv_sec) * 1000) + ((ts.tv_nsec - ts2.tv_nsec) / 1000000);			
		}
		else
		{
			if (ret == ETIMEDOUT) ret = 0;
			else printf("error lock %i\n", ret);
		}
	}
	else ret = pthread_mutex_lock(mutex);
	return ret;
}

int dbg_mutex_trylock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine, char *mutexname)
{
	DBG_LOCK(&dbg_mutex);	
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", __func__, pFileName, pFuncName, iLine, mutexname);
	DBG_UNLOCK(&dbg_mutex);
	
	int ret = pthread_mutex_trylock(mutex);
	if (ret == 0)
	{
		DBG_LOCK(&dbg_mutex);
		if ((dbg_mode & DBG_MODE_TRACK_MUTEX) && (dbg_control_mutex == mutex))
			dbgprintfw(1,"[DBG] MUTEX LOCK (TRY); Cnt: %i File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", iLockCnt, pFileName, pFuncName, iLine, mutexname);			
		if (mutex == NULL)
			dbgprintfw(1,"[DBG] MUTEX TRYLOCK ERROR: NULL; Cnt: %i File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", iLockCnt, pFileName, pFuncName, iLine, mutexname);			
		if (dbg_mode & DBG_MODE_TRACK_LOCKED_MUTEX)
		{
			int i;
			for (i = 0; i != dbg_mutex_cnt;i++)
				if (dbg_mutex_data[i].status == 0) break;
			if (i == dbg_mutex_cnt)
			{
				if (dbg_mutex_cnt != DBG_MAX_MUTEX_CNT)
				{
					dbg_mutex_cnt++;
					dbg_mutex_data = (dbg_mutex_info*)realloc(dbg_mutex_data, dbg_mutex_cnt*sizeof(dbg_mutex_info));
					memset(&dbg_mutex_data[dbg_mutex_cnt-1], 0, sizeof(dbg_mutex_info));
					//printf("[DBG] DBG_MAX_MUTEX_CNT++ %i\n",dbg_mutex_cnt);	
				}
				else 
				{
					dbgprintfw(1,"[DBG] DBG_MAX_MUTEX_CNT overfull\n");		
					i--;
				}
			}// else dbgprintfw(1,"[DBG] DBG_MAX_MUTEX_CNT exist\n");
			dbg_mutex_data[i].mutex = mutex;
			dbg_mutex_data[i].status = 2;
			dbg_mutex_data[i].line = iLine;
			dbg_mutex_data[i].tid = pthread_self();		
			dbg_mutex_data[i].count = iLockCnt;
			dbg_mutex_data[i].startlock_ms = 0;
			dbg_mutex_data[i].endlock_ms = 0;
			get_now_ms(&dbg_mutex_data[i].startlock_ms);
		
			memset(dbg_mutex_data[i].filename, 0, 32);
			memset(dbg_mutex_data[i].funcname, 0, 32);
			memset(dbg_mutex_data[i].mutexname, 0, 32);
			if (strlen(pFileName) < 32) strcpy(dbg_mutex_data[i].filename, pFileName);
				else memcpy(dbg_mutex_data[i].filename, pFileName, 31);
			if (strlen(pFuncName) < 32) strcpy(dbg_mutex_data[i].funcname, pFuncName);
				else memcpy(dbg_mutex_data[i].funcname, pFuncName, 31);
			if (strlen(mutexname) < 32) strcpy(dbg_mutex_data[i].mutexname, mutexname);
				else memcpy(dbg_mutex_data[i].mutexname, mutexname, 31);
		}
		iLockCnt++;
		DBG_UNLOCK(&dbg_mutex);	
	}
	return ret;
}

int dbg_mutex_lock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine, char *mutexname)
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", __func__, pFileName, pFuncName, iLine, mutexname);
	
	if ((dbg_mode & DBG_MODE_TRACK_MUTEX) && (dbg_control_mutex == mutex))
		dbgprintfw(1,"[DBG] MUTEX LOCK; Cnt: %i File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", iLockCnt, pFileName, pFuncName, iLine, mutexname);			
	if (mutex == NULL)
		dbgprintfw(1,"[DBG] MUTEX LOCK ERROR: NULL; Cnt: %i File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", iLockCnt, pFileName, pFuncName, iLine, mutexname);			
	int i;
	if (dbg_mode & DBG_MODE_TRACK_LOCKED_MUTEX)
	{
		unsigned int ctid = pthread_self();	
		for (i = 0; i != dbg_mutex_cnt;i++)
			if ((dbg_mutex_data[i].mutex == mutex) && (dbg_mutex_data[i].status == 2) && (dbg_mutex_data[i].tid == ctid)) 
			{
				dbgprintfw(1,"[DBG] MUTEX LOCK WARNING, DOUBLE LOCK; \nLOCKED:\n\tFile:'%s' Func:'%s' Line:%d Mutex:'%s' TID:%i\n\tLOCKING:\n\tFile:'%s' Func:'%s' Line:%d Mutex:'%s' TID:%i\n", 
									dbg_mutex_data[i].filename, dbg_mutex_data[i].funcname, dbg_mutex_data[i].line, dbg_mutex_data[i].mutexname, dbg_mutex_data[i].tid, 
									pFileName, pFuncName, iLine,  mutexname, ctid);
					
				break;
			}
		
		for (i = 0; i != dbg_mutex_cnt;i++)
			if (dbg_mutex_data[i].status == 0) break;
		if (i == dbg_mutex_cnt)
		{
			if (dbg_mutex_cnt != DBG_MAX_MUTEX_CNT)
			{
					dbg_mutex_cnt++;
					dbg_mutex_data = (dbg_mutex_info*)realloc(dbg_mutex_data, dbg_mutex_cnt*sizeof(dbg_mutex_info));
					memset(&dbg_mutex_data[dbg_mutex_cnt-1], 0, sizeof(dbg_mutex_info));
					//printf("[DBG] DBG_MAX_MUTEX_CNT++ %i\n",dbg_mutex_cnt);		
			}
			else 
			{
					dbgprintfw(1,"[DBG] DBG_MAX_MUTEX_CNT overfull\n");		
					i--;
			}
		}// else dbgprintfw(1,"[DBG] DBG_MAX_MUTEX_CNT exist\n");
		dbg_mutex_data[i].mutex = mutex;
		dbg_mutex_data[i].status = 1;
		dbg_mutex_data[i].line = iLine;
		dbg_mutex_data[i].tid = pthread_self();		
		dbg_mutex_data[i].count = iLockCnt;
		dbg_mutex_data[i].startlock_ms = 0;
		dbg_mutex_data[i].endlock_ms = 0;
		get_now_ms(&dbg_mutex_data[i].startlock_ms);
		
		memset(dbg_mutex_data[i].filename, 0, 32);
		memset(dbg_mutex_data[i].funcname, 0, 32);
		memset(dbg_mutex_data[i].mutexname, 0, 32);
		if (strlen(pFileName) < 32) strcpy(dbg_mutex_data[i].filename, pFileName);
			else memcpy(dbg_mutex_data[i].filename, pFileName, 31);
		if (strlen(pFuncName) < 32) strcpy(dbg_mutex_data[i].funcname, pFuncName);
			else memcpy(dbg_mutex_data[i].funcname, pFuncName, 31);
		if (strlen(mutexname) < 32) strcpy(dbg_mutex_data[i].mutexname, mutexname);
			else memcpy(dbg_mutex_data[i].mutexname, mutexname, 31);
	}
	iLockCnt++;
	DBG_UNLOCK(&dbg_mutex);
	
	int ret = pthread_mutex_lock(mutex);
	if (ret == 0) 
	{
		DBG_LOCK(&dbg_mutex);
		if (dbg_mode & DBG_MODE_TRACK_LOCKED_MUTEX)
		{
			dbg_mutex_data[i].status = 2;
			dbg_mutex_data[i].endlock_ms = get_now_ms(&dbg_mutex_data[i].startlock_ms);
		}
		DBG_UNLOCK(&dbg_mutex);
	}

	return ret;
}

int dbg_mutex_unlock(pthread_mutex_t *mutex, char *pFileName, const char *pFuncName, int iLine, char *mutexname)
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_DBG) dbgprintfw(1,"[DBG] %s, File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", __func__, pFileName, pFuncName, iLine, mutexname);
	
	if ((dbg_mode & DBG_MODE_TRACK_MUTEX) && (dbg_control_mutex == mutex))
		dbgprintfw(1,"[DBG] MUTEX UNLOCK; File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", pFileName, pFuncName, iLine, mutexname);		
	if (mutex == NULL)
		dbgprintfw(1,"[DBG] MUTEX UNLOCK ERROR: NULL; Cnt: %i File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", iLockCnt, pFileName, pFuncName, iLine, mutexname);			
	if (dbg_mode & DBG_MODE_TRACK_LOCKED_MUTEX)
	{
		unsigned int ctid = pthread_self();	
		int i;
		for (i = 0; i != dbg_mutex_cnt;i++)
			if ((dbg_mutex_data[i].mutex == mutex) && (dbg_mutex_data[i].status == 2) && (dbg_mutex_data[i].tid == ctid))
			{				
				dbg_mutex_data[i].status = 0;
				break;
			}
		if (i == dbg_mutex_cnt)
		{
			for (i = 0; i != dbg_mutex_cnt;i++)
				if ((dbg_mutex_data[i].mutex == mutex) && (dbg_mutex_data[i].status == 2))
				{
					dbgprintfw(1,"[DBG] MUTEX UNLOCK WARNING, DIFF TID; \nLOCKED:\n\tFile:'%s' Func:'%s' Line:%d Mutex:'%s' TID:%i\n\tUNLOCKED:\n\tFile:'%s' Func:'%s' Line:%d Mutex:'%s' TID:%i\n", 
									dbg_mutex_data[i].filename, dbg_mutex_data[i].funcname, dbg_mutex_data[i].line, dbg_mutex_data[i].mutexname, dbg_mutex_data[i].tid, 
									pFileName, pFuncName, iLine,  mutexname, ctid);
					dbg_mutex_data[i].status = 0;
					break;
				}
		}
		if (i == dbg_mutex_cnt) 
		{
			dbgprintfw(1,"[DBG] MUTEX UNLOCK FAILED, NOT LOCKED; File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", pFileName, pFuncName, iLine, mutexname);
		}
		else
		{
			if ((dbg_mode & DBG_MODE_TRACK_LONG_MUTEX)
				&&
				((unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms) >= dbg_MutexLongTime))
			{				
				dbgprintfw(1,"[DBG] MUTEX LONG UNLOCKED; Cnt: %i File:'%s' Func:'%s' Line:%d\n\tAddr:%i TID:%i, Mutex:'%s', LockedTime:%i, LockingTime:%i\n", 
														dbg_mutex_data[i].count, 
														dbg_mutex_data[i].filename, 
														dbg_mutex_data[i].funcname, 
														dbg_mutex_data[i].line, 
														(int)dbg_mutex_data[i].mutex,
														dbg_mutex_data[i].tid,
														dbg_mutex_data[i].mutexname,
														(unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms),
														(unsigned int)dbg_mutex_data[i].endlock_ms);
				dbgprintfw(1,"\t\t IN  %s, File:'%s' Func:'%s' Line:%d Mutex:'%s'\n", __func__, pFileName, pFuncName, iLine, mutexname);
			}
		}
	}
	DBG_UNLOCK(&dbg_mutex);	
	
	return pthread_mutex_unlock(mutex);
}

void dbg_mutex_print_locked(unsigned int uiMutexTime, char cResult)
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_TRACK_LOCKED_MUTEX)
	{
		int i;
		int ret = 0;
		
		for (i = 0; i != dbg_mutex_cnt;i++)
		{
			if ((ret == 0) && (dbg_mutex_data[i].status > 0) && ((unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms) >= uiMutexTime))
			{
				ret = 1;
				dbgprintfw(1,"[DBG] MUTEX NOW LOCKED >>>>>>>>>>\n");		
			}
			if ((dbg_mutex_data[i].status == 1) && ((unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms) >= uiMutexTime))
			{
				dbgprintfw(1,"[DBG] MUTEX NOW LOCKING; Cnt: %i File:'%s' Func:'%s' Line:%d\n\tAddr:%i TID:%i, Mutex:'%s', LockingTime:%i\n", 
														dbg_mutex_data[i].count, 
														dbg_mutex_data[i].filename, 
														dbg_mutex_data[i].funcname, 
														dbg_mutex_data[i].line, 
														(int)dbg_mutex_data[i].mutex,
														dbg_mutex_data[i].tid,
														dbg_mutex_data[i].mutexname,
														(unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms));
			}
			if ((dbg_mutex_data[i].status == 2) && ((unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms) >= uiMutexTime))
			{				
				dbgprintfw(1,"[DBG] MUTEX NOW LOCKED; Cnt: %i File:'%s' Func:'%s' Line:%d\n\tAddr:%i TID:%i, Mutex:'%s', LockedTime:%i, LockingTime:%i\n", 
														dbg_mutex_data[i].count, 
														dbg_mutex_data[i].filename, 
														dbg_mutex_data[i].funcname, 
														dbg_mutex_data[i].line, 
														(int)dbg_mutex_data[i].mutex,
														dbg_mutex_data[i].tid,
														dbg_mutex_data[i].mutexname,
														(unsigned int)get_now_ms(&dbg_mutex_data[i].startlock_ms),
														(unsigned int)dbg_mutex_data[i].endlock_ms);
			}
		}		
		//if (ret == 1) dbgprintfw(1," <<<<<<<<<<\n");
		if ((ret == 0) && cResult) dbgprintfw(1,"[DBG] MUTEX NOW NOT LOCKED\n");
	}
	DBG_UNLOCK(&dbg_mutex);	
}

void dbg_func_print_entered()
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_LOG_FUNC)
	{
		int i;
		//int ret = 0;
		dbgprintfw(1,"[DBG] FUNC NOW ENTERED >>>>>>>>>>\n");		
		for (i = 0; i != dbg_func_cnt;i++)
			if (dbg_func_data[i].status == 1)
			{
				dbgprintfw(1,"[DBG] FUNC NOW ENTERED; TID:%i(%i) File:'%s' Func:'%s' Line:%d\n", 
														dbg_func_data[i].tid, 
														dbg_func_data[i].stid, 
														dbg_func_data[i].filename, 
														dbg_func_data[i].funcname, 
														dbg_func_data[i].line);
				//ret = 1;								
			}
		//if (ret) dbgprintfw(1,"[DBG] FUNC NOW ENTERED <<<<<<<<<<\n"); else dbgprintfw(1," <<<<<<<<<<\n");
	}
	DBG_UNLOCK(&dbg_mutex);	
}

void dbg_points_print()
{
	DBG_LOCK(&dbg_mutex);
	int i, k = 1;
	dbgprintfw(1,"[DBG] POINTS >>>>>>>>>>\n");		
	for (i = dbg_points_cnt + 1; i < DBG_POINTS_MAX;i++)
	{
		dbgprintfw(1,"[DBG] %i): Time: %i Flag: %i Info: %i PID:%i Line:%d\n", 
			k, (unsigned int)dbg_point_list[i].time, dbg_point_list[i].mark, dbg_point_list[i].info, dbg_point_list[i].pid, dbg_point_list[i].line);
		k++;
	}
	for (i = 0; (i <= dbg_points_cnt) && (i < DBG_POINTS_MAX); i++)
	{
		if (i == dbg_points_cnt) dbgprintfw(1,"[DBG]>>%i): Time: %i Flag: %i Info: %i PID:%i Line:%d\n", 
			k, (unsigned int)dbg_point_list[i].time, dbg_point_list[i].mark, dbg_point_list[i].info, dbg_point_list[i].pid, dbg_point_list[i].line);
			else dbgprintfw(1,"[DBG] %i): Time: %i Flag: %i Info: %i PID:%i Line:%d\n", 
					k, (unsigned int)dbg_point_list[i].time, dbg_point_list[i].mark, dbg_point_list[i].info, dbg_point_list[i].pid, dbg_point_list[i].line);
		k++;
	}
	//dbgprintfw(1,"[DBG] POINTS <<<<<<<<<<\n");
	DBG_UNLOCK(&dbg_mutex);	
}

void dbg_mark_cur_point(char cFlag)
{
	DBG_LOCK(&dbg_mutex);
	dbg_point_list[dbg_points_cnt].mark = cFlag;
	DBG_UNLOCK(&dbg_mutex); 
}

void dbg_mem_print_allocated(int iLevel)
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_LOG_MEM)
	{ 
		int i;
		//int ret = 0;
		dbgprintfw(1,"[DBG] MEMORY NOW ALLOCATED cnt:%i total alloc:%i >>>>>>>>>>\n", dbg_mem_cnt_cur, iUseMemory);		
		for (i = 0; i != dbg_mem_cnt;i++)
			if (dbg_mem_data[i].size != 0)
			{
				dbgprintfw(iLevel,"[DBG] MEMORY NOW ALLOCATED; (%i) Size:%i File:'%s' Func:'%s' Line:%d\n", 
														dbg_mem_data[i].number, 
														dbg_mem_data[i].size, 
														dbg_mem_data[i].filename, 
														dbg_mem_data[i].funcname, 
														dbg_mem_data[i].line);
				//ret = 1;								
			}
		//if (ret) dbgprintfw(1,"[DBG] MEMORY NOW ALLOCATED <<<<<<<<<<\n"); else dbgprintfw(1," <<<<<<<<<<\n");
	}
	DBG_UNLOCK(&dbg_mutex);	
}

int dbg_mem_print_allocated_short()
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_LOG_MEM)
	{
		clock_gettime(CLOCK_REALTIME, &dbg_nanotime);	
		memset(dbg_buff, 0, DBG_MAX_BUFF_SIZE);
		
		sprintf(dbg_buff, "Cnt:%i \t Thread:%i \t Time:%i \t UseMemory:%i \t MaxUsedMemory:%i", dbg_mem_cnt_cur, (unsigned int)pthread_self(),
					(unsigned int)(dbg_nanotime.tv_nsec/1000000), iUseMemory, iMaxUsedMemory);
		dbg_send_udp_message(dbg_buff);
	}
	DBG_UNLOCK(&dbg_mutex);	
	return 1;
}

void dbg_mem_test_allocated(char cShowOkRes)
{
	DBG_LOCK(&dbg_mutex);
	if (dbg_mode & DBG_MODE_LOG_MEM)
	{ 
		int i, ret, ret2 = 0;
		//int ret = 0;
		for (i = 0; i != dbg_mem_cnt;i++)
		{
			ret = 0;
			if (dbg_mem_data[i].status == 1)
			{
				ret = dbg_test(dbg_mem_data[i].point + (sizeof(unsigned int) * 2));
				if (ret != 0)
				{
					ret2 |= ret;
					dbgprintfw(1,"[DBG] MEMORY CORRUPT DETECTED (Begin:%s;End:%s); Size:%i File:'%s' Func:'%s' Line:%d\n", 
														ret & 1 ? "Bad" : "Ok",
														ret & 2 ? "Bad" : "Ok",
														dbg_mem_data[i].size, 
														dbg_mem_data[i].filename, 
														dbg_mem_data[i].funcname, 
														dbg_mem_data[i].line);
				}
			}			
		}
		if ((ret2 == 0) && (cShowOkRes)) dbgprintfw(1,"[DBG] MEMORY OK <<<<<<<<<<\n");
	}
	DBG_UNLOCK(&dbg_mutex);
}



