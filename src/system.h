#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "pthread2threadx.h"

typedef struct
{
	char *path;
	TX_EVENTER 	evnt;
    TX_EVENTER 	evnt_close;
    pthread_t threadFileIO;
	pthread_attr_t tattrFileIO;
	char *buffer1;
	char *buffer2;
	pthread_mutex_t buffmutex;
	char freebuffer;
	char flagstop;
	unsigned int datasize1;
	unsigned int datasize2;
	unsigned int buffersize;
	unsigned int minwritesize;
	FILE *filehandle;
	unsigned int filelen;
	char opened;
	time_t createtime;
} file_struct;

unsigned int GetTickCount();
int64_t get_ms(int64_t *previous_ms);
int64_t get_us(int64_t *previous_us);
file_struct* file_init(unsigned int uiBufferSize, unsigned int uiMinSizeWrite);
void file_deinit(file_struct* file_data);
FILE * file_open(char *filename, file_struct* file_data);
int file_close(FILE *g_file_handle, file_struct* file_data);
unsigned int file_write(char *data, unsigned int datalen, FILE* f, file_struct* file_data);
int Hex2Int(char *cString);
int Hex2IntLimit(char *cString, unsigned int cLen);
int GetProcessNameCount(const char* cchrptr_ProcessName);
#endif