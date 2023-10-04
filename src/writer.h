#ifndef _WRITER_H_
#define _WRITER_H_

#include "main.h"
#include "pthread2threadx.h"

enum WRT_EVENT 
{
	WRT_EVENT_COPY_FILE_GET_ORDER = 1,
	WRT_EVENT_COPY_FILE_REJECT,
	WRT_EVENT_COPY_FILE_ACCEPT,
	WRT_EVENT_COPY_FILE_BUSY,
	WRT_EVENT_COPY_FILE_DONE,
	WRT_EVENT_COPY_FILE_RELEASE,
	WRT_EVENT_COPY_FILE_DATA,
	WRT_EVENT_COPY_FILE_EXIT
};

enum WRT_TYPE 
{
	WRT_TYPE_OTHER = 0,
	WRT_TYPE_ARCHIVE_FULL,
	WRT_TYPE_ARCHIVE_SLOW,
	WRT_TYPE_ARCHIVE_DIFF,
	WRT_TYPE_ARCHIVE_AUDIO,
	WRT_TYPE_ARCHIVE_STATUSES,
	WRT_TYPE_ARCHIVE_EVENTS,
	WRT_TYPE_ARCHIVE_ACTIONS,
	WRT_TYPE_BACKUP_FULL,
	WRT_TYPE_BACKUP_SLOW,
	WRT_TYPE_BACKUP_DIFF,
	WRT_TYPE_BACKUP_AUDIO,
	WRT_TYPE_BACKUP_STATUSES,
	WRT_TYPE_BACKUP_EVENTS,
	WRT_TYPE_BACKUP_ACTIONS	
};

typedef struct
{
   unsigned int ID;
   unsigned int Type;
   unsigned int Size;
   int	Enabled;
   int	Status;
   char Path[MAX_PATH];
   unsigned int ConnectNum;
   struct sockaddr_in  Address;
   FILE *filehandle;
} TRANSFER_FILE_INFO;

unsigned int cThreadWriterStatus;
TRANSFER_FILE_INFO tfTransferFileStatus;
TX_EVENTER writeevent_evnt;

int CloseWriter();
void * Writer(void *pData);

#endif