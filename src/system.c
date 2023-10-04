#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "system.h"
#include "debug.h"
#include "pthread2threadx.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h> // for opendir(), readdir(), closedir()
#include <sys/stat.h> // for stat()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define PROC_DIRECTORY "/proc/"
#define CASE_SENSITIVE    1
#define CASE_INSENSITIVE  0
#define EXACT_MATCH       1
#define INEXACT_MATCH     0


int IsNumeric(const char* ccharptr_CharacterList)
{
    for ( ; *ccharptr_CharacterList; ccharptr_CharacterList++)
        if (*ccharptr_CharacterList < '0' || *ccharptr_CharacterList > '9')
            return 0; // false
    return 1; // true
}

int GetProcessNameCount(const char* cchrptr_ProcessName)
{
    char chrarry_CommandLinePath[100]  ;
    char chrarry_NameOfProcess[300]  ;
    char* chrptr_StringToCompare = NULL ;
    struct dirent* de_DirEntity = NULL ;
    DIR* dir_proc = NULL;
	int result = 0;

    dir_proc = opendir(PROC_DIRECTORY) ;
    if (dir_proc == NULL)
    {
        perror("Couldn't open the " PROC_DIRECTORY " directory") ;
        return (pid_t) -2 ;
    }
	
    while ( (de_DirEntity = readdir(dir_proc)) )
    {
        if (de_DirEntity->d_type == DT_DIR)
        {
            if (IsNumeric(de_DirEntity->d_name))
            {
				memset(chrarry_CommandLinePath, 0, 100);
                strcpy(chrarry_CommandLinePath, PROC_DIRECTORY) ;
                strcat(chrarry_CommandLinePath, de_DirEntity->d_name) ;
                strcat(chrarry_CommandLinePath, "/cmdline") ;
				//printf("####%s\n", chrarry_CommandLinePath);
                FILE* fd_CmdLineFile = fopen (chrarry_CommandLinePath, "rt") ;  // open the file for reading text
                if (fd_CmdLineFile)
                {
					memset(chrarry_NameOfProcess, 0, 300);
                    fscanf(fd_CmdLineFile, "%s", chrarry_NameOfProcess) ; // read from /proc/<NR>/cmdline
                    fclose(fd_CmdLineFile);  // close the file prior to exiting the routine

                    if (strrchr(chrarry_NameOfProcess, '/'))
                        chrptr_StringToCompare = strrchr(chrarry_NameOfProcess, '/') +1 ;
                    else
                        chrptr_StringToCompare = chrarry_NameOfProcess ;

                    //printf("%s\t", strcmp(chrptr_StringToCompare, cchrptr_ProcessName) == 0 ? "+" : "-" );
					//printf("Process name: %s\t", chrarry_NameOfProcess);
                    //printf("Pure Process name: %s\n", chrptr_StringToCompare);

                    if (strcmp(chrptr_StringToCompare, cchrptr_ProcessName) == 0) result++;					
                }
            }
        }
    }
    closedir(dir_proc) ;
    return result;
}

unsigned int GetTickCount()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int64_t tms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
	return (unsigned int)tms;
}

int64_t get_ms(int64_t *previous_ms)
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

int64_t get_us(int64_t *previous_us)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	if ((*previous_us) == 0)
	{
		*previous_us = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000;
		return 0;
	}
	return ((ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000) - (*previous_us);
}

void file_send_signal_write(file_struct* file_data)
{
	tx_eventer_send_event(&file_data->evnt, 210);
}

void* thread_file_writer(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "file_writer");
	
	file_struct* file_data = (file_struct*)pData;
	char iloop = 1;
	unsigned int ret = 0;
	unsigned int uizero = 0;
	unsigned int *idatasize = &uizero;
	unsigned int diff = 0;	
	char cGetWait = 0;
	char *buffer = NULL;
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	tx_eventer_add_event(&file_data->evnt, 210);	
			
	while(iloop)
	{
		DBG_MUTEX_LOCK(&file_data->buffmutex);
		if (file_data->flagstop > 0)
		{			
			if (file_data->freebuffer == 0) 
			{
				idatasize =  &file_data->datasize2;
				buffer = file_data->buffer2;
				if ((file_data->datasize2 == 0) && (file_data->datasize1 != 0) && ((file_data->datasize1 >= file_data->minwritesize) || (file_data->flagstop >= 2)))
				{
					file_data->freebuffer = 1;
					idatasize = &file_data->datasize1;
					buffer = file_data->buffer1;			
				}
			}
			else
			{
				idatasize = &file_data->datasize1;
				buffer = file_data->buffer1;
				if ((file_data->datasize1 == 0) && (file_data->datasize2 != 0) && ((file_data->datasize2 >= file_data->minwritesize) || (file_data->flagstop >= 2)))
				{
					file_data->freebuffer = 0;
					idatasize = &file_data->datasize2;
					buffer = file_data->buffer2;
				}
			}
			if ((*idatasize == 0) && (file_data->flagstop == 2))	
			{
				file_data->flagstop = 0;
				idatasize = &uizero;
				tx_eventer_send_event(&file_data->evnt_close, 220);
			}
		}
		DBG_MUTEX_UNLOCK(&file_data->buffmutex);
		//*printf("idatasize %i\n",*idatasize);			
		if (*idatasize != 0)
		{
			//printf("thread write %i\n",*idatasize);
			diff = (unsigned int)get_ms(&previous_ms);
			ret = fwrite(buffer, 1, *idatasize, file_data->filehandle);
			diff = (unsigned int)get_ms(&previous_ms) - diff;
			//printf(">>> %i %i\n", *idatasize, diff);
			if (diff > 1000) 
			{
				DBG_MUTEX_LOCK(&file_data->buffmutex);
				dbgprintf(3,"LONG WRITE FILE: time:%d len:%i data1:%i data2:%i\n",diff, ret, file_data->datasize1, file_data->datasize2);			
				DBG_MUTEX_UNLOCK(&file_data->buffmutex);
			}
			if (ret != *idatasize)
			{
				*idatasize -= ret;
				memmove(buffer, &buffer[ret], *idatasize);				
			} 
			else *idatasize = 0; 
		}
		else
		{
			DBG_MUTEX_LOCK(&file_data->buffmutex);
			if (file_data->flagstop < 2) cGetWait = 1;
			DBG_MUTEX_UNLOCK(&file_data->buffmutex);		
		}
		
		DBG_MUTEX_LOCK(&file_data->buffmutex);				
		if ((file_data->datasize1 == 0) && (file_data->datasize2 == 0) && (file_data->flagstop == 3)) {iloop = 0; cGetWait = 0;}
		DBG_MUTEX_UNLOCK(&file_data->buffmutex);			
		if (cGetWait) 
		{
			tx_eventer_recv_event(&file_data->evnt, 210, TX_WAIT_FOREVER);
			cGetWait = 0;
		}
	}
	tx_eventer_send_event(&file_data->evnt_close, 230);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return NULL;
}

file_struct* file_init(unsigned int uiBufferSize, unsigned int uiMinSizeWrite)
{
	file_struct* file_data = (file_struct*)DBG_MALLOC(sizeof(file_struct));
	memset(file_data, 0, sizeof(file_struct));
	
	tx_eventer_create(&file_data->evnt, 1);
	tx_eventer_create(&file_data->evnt_close, 0);
	pthread_mutex_init(&file_data->buffmutex, NULL);
	file_data->buffer1 = (char*)DBG_MALLOC(uiBufferSize);
	file_data->buffer2 = (char*)DBG_MALLOC(uiBufferSize);
	file_data->buffersize = uiBufferSize;
	file_data->freebuffer = 0;
	file_data->datasize1 = 0;
	file_data->datasize2 = 0;
	file_data->flagstop = 0;
	file_data->minwritesize = uiMinSizeWrite;
	pthread_attr_init(&file_data->tattrFileIO);   
	pthread_attr_setdetachstate(&file_data->tattrFileIO, PTHREAD_CREATE_DETACHED);
	pthread_create(&file_data->threadFileIO, &file_data->tattrFileIO, thread_file_writer, (void*)file_data);	

	return file_data;
}

void file_deinit(file_struct* file_data)
{
	DBG_MUTEX_LOCK(&file_data->buffmutex);
	file_data->flagstop = 3; // stop
	file_data->minwritesize = 0;
	DBG_MUTEX_UNLOCK(&file_data->buffmutex);	
	tx_eventer_add_event(&file_data->evnt_close, 230);				
	file_send_signal_write(file_data);
	tx_eventer_recv_event(&file_data->evnt_close, 230, TX_WAIT_FOREVER);
	pthread_attr_destroy(&file_data->tattrFileIO);
	pthread_mutex_destroy(&file_data->buffmutex);
	DBG_FREE(file_data->buffer1);
	DBG_FREE(file_data->buffer2);
	tx_eventer_delete(&file_data->evnt);
	tx_eventer_delete(&file_data->evnt_close);
	
	DBG_FREE(file_data);	
}

FILE * file_open(char *filename, file_struct* file_data)
{
	if (NULL == filename) return 0;
    FILE *g_file_handle = fopen(filename, "wb");
	if (g_file_handle) 
	{
		//if (setvbuf(*g_file_handle, NULL, _IOFBF, 10485760) != 0) perror("Error setvbuf"); else perror("Ok setvbuf");
		if (file_data) 
		{
			int len = strlen(filename);
			file_data->path = (char*)DBG_MALLOC(len + 1);
			memcpy(file_data->path, filename, len);
			file_data->path[len] = 0;
			
			file_data->filelen = 0;	
			file_data->opened = 1;			
			time(&file_data->createtime);
			DBG_MUTEX_LOCK(&file_data->buffmutex);
			file_data->filehandle = g_file_handle;
			file_data->flagstop = 1;
			DBG_MUTEX_UNLOCK(&file_data->buffmutex);
			file_send_signal_write(file_data);	
		}
		return g_file_handle;
	}
	return NULL;
}

int file_close(FILE *g_file_handle, file_struct* file_data)
{
	if (NULL == g_file_handle) return 0;
	
	if (file_data) 
	{
		tx_eventer_add_event(&file_data->evnt_close, 220);	
		DBG_MUTEX_LOCK(&file_data->buffmutex);
		file_data->flagstop = 2;
		DBG_MUTEX_UNLOCK(&file_data->buffmutex);	
		file_send_signal_write(file_data);	
		tx_eventer_recv_event(&file_data->evnt_close, 220, TX_WAIT_FOREVER);
	}
    fclose(g_file_handle);
	//printf("closed\n");
	if (file_data) 
	{
		DBG_MUTEX_LOCK(&file_data->buffmutex);
		file_data->filehandle = NULL;
		DBG_MUTEX_UNLOCK(&file_data->buffmutex);
		file_data->filelen = 0;
		file_data->opened = 0;
		DBG_FREE(file_data->path);
	}	
	return 1;
}

unsigned int file_write(char *data, unsigned int datalen, FILE* f, file_struct* file_data)
{
	int ret = 0;
	unsigned int uiWaited;
	
	if (file_data == NULL) 
	{
		ret = fwrite(data, 1, datalen, f);
		return ret;
	}
	file_data->filelen += datalen;
	uiWaited = 0;
	do
	{
		if (ret)
		{
			if (uiWaited == 0) dbgprintf(2,"buffer overfull: waiting ...\n");
			usleep(100000);			
			ret = 0;
			uiWaited++;
		}
		DBG_MUTEX_LOCK(&file_data->buffmutex);
		if (file_data->freebuffer == 0)
		{
			if ((file_data->datasize1 + datalen) > file_data->buffersize) ret = 1;
			else
			{
				memcpy(&file_data->buffer1[file_data->datasize1], data, datalen);
				file_data->datasize1 += datalen;
			}
		}
		else
		{
			if ((file_data->datasize2 + datalen) > file_data->buffersize) ret = 1;
			else
			{
				memcpy(&file_data->buffer2[file_data->datasize2], data, datalen);
				file_data->datasize2 += datalen;
			}
		}
		DBG_MUTEX_UNLOCK(&file_data->buffmutex);	
	} while(ret);
	if (uiWaited > 1) dbgprintf(2,"buffer overfull: waited %ims\n", uiWaited * 100);
	//printf("write %i %i\n", datalen, file_data->filelen);
	file_send_signal_write(file_data);
	
	return datalen;
}

int Hex2Int(char *cString)
{
	int n, ret;
	char cStr[32];
	int len = strlen(cString);
	if (len > 8) return - 2;
	memset(cStr, 0, 32);
	if (len & 1) 
	{
		cStr[0] = 48;
		strcpy(&cStr[1],cString);
		len++;
	} else strcpy(cStr,cString);
	ret = 0;
	for(n = 0; n < len; n++)
	{
		ret <<= 4;
		if (cStr[n] > 96) cStr[n] = cStr[n] - 32;
		if (((cStr[n] < 48) || (cStr[n] > 90)) || ((cStr[n] >57) && (cStr[n] <65))) return - 1;
		if (cStr[n] > 57) cStr[n] = cStr[n] - 55; else cStr[n] = cStr[n] - 48;
		ret |= cStr[n];
	}
	return ret;
}

int Hex2IntLimit(char *cString, unsigned int cLen)
{
	int n, ret;
	char cStr[32];
	if (cLen > 31) return - 2;
	if (cLen == 0) return 0;
	memset(cStr, 0, 32);
	memcpy(cStr,cString, cLen);
	ret = 0;
	for(n = 0; n != cLen; n++)
	{
		ret <<= 4;
		if (cStr[n] > 96) cStr[n] = cStr[n] - 32;
		if (((cStr[n] < 48) || (cStr[n] > 90)) || ((cStr[n] >57) && (cStr[n] <65))) return - 1;
		if (cStr[n] > 57) cStr[n] = cStr[n] - 55; else cStr[n] = cStr[n] - 48;
		ret |= cStr[n];
	}
	return ret;
}



