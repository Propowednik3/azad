#ifndef _STREAMER_H_
#define _STREAMER_H_

#include "main.h"
#include "flv-demuxer.h"

enum TYPE_STRM_EVENT 
{
	STRM_EVENT_EXIT = 1,
	STRM_EVENT_GET_LIST,
	STRM_EVENT_NEXT_LIST,
	STRM_EVENT_PREV_LIST,
	STRM_EVENT_PLAY_LIST,
	STRM_EVENT_GET_NEXT_VIDEO_FRAME,
	STRM_EVENT_GET_NEXT_AUDIO_FRAME,
	STRM_EVENT_GET_VIDEO_PARAMS,
	STRM_EVENT_GET_VIDEO_CODEC,
	STRM_EVENT_GET_START_FRAME,
	STRM_EVENT_PLAY_POS,
	STRM_EVENT_PLAY_DONE,
	STRM_EVENT_GET_CLOSE_FILE,
	STRM_EVENT_GET_AUDIO_PARAMS,
	STRM_EVENT_GET_AUDIO_STREAM
};

typedef struct
{	
	char *Buffer;
	unsigned int BufferSize;
	unsigned int DataLength;
	unsigned char Type;	
	unsigned char KeyFrame;
} REMOTE_FRAME;

typedef struct
{	
	char Name[MAX_FILE_LEN];
	char ShowName[MAX_FILE_LEN];
	unsigned char Type;	
} REMOTE_FILE_LIST;

typedef struct
{	
	int Status;
	int Timer;
	int Current;
	int Selected;
	int Direct;
	int NewPos;
	int CurrentPos;
	int CurrentShow;
	int PlayStatus;
	int Offset;
	int AnimXPos; 
	int AnimYPos;
	char Path[MAX_PATH];
	unsigned int ID;	
	struct sockaddr_in  Address;
	unsigned int FileOpened;
	unsigned int FileLen;
	unsigned int FileCnt;
	REMOTE_FILE_LIST *FileList;
	unsigned int NewFileCnt;
	REMOTE_FILE_LIST *NewFileList;
	flv_demux_struct flv_info;
} REMOTE_STATUS_INFO;

extern char cThreadStreamerStatus;
extern unsigned int cStreamerExData[2];
extern REMOTE_STATUS_INFO	rsiRemoteStatus;

int RequestFileList(struct sockaddr_in *Address, unsigned int uiMode);
int NextRemoteFile(struct sockaddr_in *Address, unsigned int uiNewPos);
int PrevRemoteFile(struct sockaddr_in *Address);
int CloseRemoteFile(struct sockaddr_in *Address);
int PlayRemoteFile(struct sockaddr_in *Address, unsigned int uiNewPos, unsigned int uiWantStreams);
int ChangeRemoteFilePos(struct sockaddr_in *Address, unsigned int uiNewPos);
void * Streamer(void *pData);

#endif