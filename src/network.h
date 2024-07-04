#ifndef _NETWORK_H_
#define _NETWORK_H_


#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
//#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>

#include "bcm_host.h"
#include "main.h"
#include "pthread2threadx.h"

//#include "pthread2threadx.h"

#define UDP_PORT 8888
#define TCP_PORT 8888

#define FALSE				0
#define TRUE				1

#define SERVICE_TRAFFIC_BUFFER_SIZE 	131072
#define FVIDEO_TRAFFIC_BUFFER_SIZE 		4194304
#define PVIDEO_TRAFFIC_BUFFER_SIZE 		1048576
#define AUDIO_TRAFFIC_BUFFER_SIZE 		1048576
#define FILE_TRAFFIC_BUFFER_SIZE		196608
#define FILE_PACKET_BUFFER_SIZE 		FILE_TRAFFIC_BUFFER_SIZE / 3
#define UDP_TRAFFIC_BUFFER_SIZE 		4524288
#define CONNECT_TIMEOUT					60
#define RECV_FRAME_TIMEOUT_MS			4000
#define RECV_DATA_TIMEOUT_MS			1000
#define TCP_PACKET_SIZE					1024
#define UDP_PACKET_MAX_SEND				1
#define TCP_LOST_BLOCK_SIZE				128000

#define FLAG_VIDEO_CODEC_INFO			0x0001
#define FLAG_START_VIDEO_FRAME			0x0002
#define FLAG_NEXT_VIDEO_FRAME			0x0004
#define FLAG_SUB_VIDEO_FRAME			0x0008
#define FLAG_VIDEO_STREAM				0x0010
#define FLAG_AUDIO_CODEC_INFO			0x0020
#define FLAG_NEXT_AUDIO_FRAME			0x0040
#define FLAG_AUDIO_STREAM				0x0080
#define FLAG_SUB_AUDIO_FRAME			0x0100
#define FLAG_STOP_AUDIO					0x0200
#define FLAG_VIDEO_PARAMS				0x0400
#define FLAG_PAUSE_AUDIO				0x0800
#define FLAG_NEW_AUDIO					0x1000
#define FLAG_DONE_AUDIO					0x2000
#define FLAG_DONE_AUDIO_FRAME			0x4000
#define FLAG_DATA_CORRUPT				0x8000

#define TRANSFER_KEY1 0x12345678
#define TRANSFER_KEY2 0x87654321
#define TRANSFER_KEY3 0x18273645

//#define MAX_STREAM_INFO 4
#define MAX_CONNECTIONS						128

#define MESSAGES_MAX_CNT 256
#define MESSAGES_MAX_LEN 256

enum TYPE_MESSAGE 
{ 
	TYPE_MESSAGE_EMPTY,
	TYPE_MESSAGE_SKIP,
	TYPE_MESSAGE_CLOSE,
	TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO,
	TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA,
	TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME,
	TYPE_MESSAGE_START_VIDEO_FRAME_DATA,
	TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME,
	TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA,
	TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA,
	TYPE_MESSAGE_ALARM_SET,
	TYPE_MESSAGE_ALARM_RESET,
	TYPE_MESSAGE_REQUEST_VIDEO_STREAM,
	TYPE_MESSAGE_STOP_VIDEO_STREAM,
	TYPE_MESSAGE_REQUEST_DATETIME,
	TYPE_MESSAGE_DATETIME,
	TYPE_MESSAGE_REQUEST_MODULE_LIST,
	TYPE_MESSAGE_MODULE_LIST,
	TYPE_MESSAGE_MODULE_SET,
	TYPE_MESSAGE_REQUEST_AUDIO_STREAM,
	TYPE_MESSAGE_STOP_AUDIO_STREAM,
	TYPE_MESSAGE_REQUEST_AUDIO_CODEC_INFO,
	TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA,
	TYPE_MESSAGE_REQUEST_NEXT_AUDIO_FRAME,
	TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA,
	TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA,
	TYPE_MESSAGE_MODULE_STATUS_CHANGED,
	TYPE_MESSAGE_MODULE_STATUS,
	TYPE_MESSAGE_REQUEST_SYSTEM_INFO,
	TYPE_MESSAGE_SYSTEM_SIGNAL,	
	TYPE_MESSAGE_SYSTEM_INFO,
	TYPE_MESSAGE_DEVICE_STARTED,
	TYPE_MESSAGE_REQUEST_SECURITYLIST,
	TYPE_MESSAGE_SECURITYLIST,
	TYPE_MESSAGE_CHANGED_SECURITYLIST,
	TYPE_MESSAGE_GET_CHANGE_SMARTCARD, 
	TYPE_MESSAGE_SET_CHANGE_SMARTCARD,
	TYPE_MESSAGE_REQUEST_VIDEO_PARAMS,
	TYPE_MESSAGE_VIDEO_PARAMS,
	TYPE_MESSAGE_TEXT,
	TYPE_MESSAGE_REQUEST_MODULE_STATUS,
	TYPE_MESSAGE_DEVICE_STOPED,
	TYPE_MESSAGE_TRAFFIC_TYPE_AUDIO,
	TYPE_MESSAGE_TRAFFIC_TYPE_FULL_VIDEO,
	TYPE_MESSAGE_TRAFFIC_TYPE_PREV_VIDEO,
	TYPE_MESSAGE_TRAFFIC_TYPE_SERVICE,
	TYPE_MESSAGE_TRAFFIC_TYPE_TRANSFER_FILE,
	TYPE_MESSAGE_FORCE_DATETIME,
	TYPE_MESSAGE_MODULE_CHANGED,
	TYPE_MESSAGE_DISPLAY_CONTENT,	
	TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_VIDEO,
	TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_AUDIO,
	
	TYPE_MESSAGE_FILELIST_LOW_GRID = 1000,
	TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_FILE,
	TYPE_MESSAGE_REQUEST_FILELIST,
	TYPE_MESSAGE_FILELIST,
	TYPE_MESSAGE_NEXT_FILELIST,
	TYPE_MESSAGE_PREV_FILELIST,
	TYPE_MESSAGE_GET_PLAY_FILE,
	TYPE_MESSAGE_OPENED_FILE,
	TYPE_MESSAGE_CLOSED_FILE,
	TYPE_MESSAGE_CHANGE_PLAY_FILE_POS,
	TYPE_MESSAGE_DONE_FILE,
	TYPE_MESSAGE_BUSY_FILE,
	TYPE_MESSAGE_FREE_FILE,	
	TYPE_MESSAGE_GET_CLOSE_FILE,
	TYPE_MESSAGE_NEW_FILE_POS,
	TYPE_MESSAGE_ERROR_FILE,
	TYPE_MESSAGE_FILEPATH,
	//TYPE_MESSAGE_BUSY_FILE,
	//TYPE_MESSAGE_FREE_FILE,
	TYPE_MESSAGE_FILELIST_HIGH_GRID,
	
	TYPE_MESSAGE_COPY_FILE_LOW_GRID = 1100,
	TYPE_MESSAGE_COPY_FILE_GET_ORDER,
	TYPE_MESSAGE_COPY_FILE_ACCEPT_ORDER,
	TYPE_MESSAGE_COPY_FILE_DATA,
	TYPE_MESSAGE_COPY_FILE_DONE,
	TYPE_MESSAGE_COPY_FILE_RELEASE,
	TYPE_MESSAGE_COPY_FILE_BUSY,
	TYPE_MESSAGE_COPY_FILE_REJECT,	
	TYPE_MESSAGE_COPY_FILE_HIGH_GRID
};

enum TYPE_EVENT 
{
	EVENT_VIDEO_CODEC_INFO_DATA	= 1,
	EVENT_START_VIDEO_FRAME_DATA,
	EVENT_NEXT_VIDEO_FRAME_DATA,
	EVENT_SUB_VIDEO_FRAME_DATA,
	EVENT_AUDIO_CODEC_INFO_DATA,
	EVENT_NEXT_AUDIO_FRAME_DATA,
	EVENT_SUB_AUDIO_FRAME_DATA,
	EVENT_SKIP_AUDIO_FRAME_DATA,
	EVENT_SYNC_AUDIO_CODEC_INFO_DATA,
	EVENT_SYNC_NEXT_AUDIO_FRAME_DATA,
	EVENT_SYNC_SUB_AUDIO_FRAME_DATA,
	EVENT_START,
	EVENT_STOP,
	EVENT_VIDEO,
	EVENT_AUDIO,
	EVENT_SYNC_VIDEO_CODEC_INFO_DATA,
	EVENT_SYNC_START_VIDEO_FRAME_DATA,
	EVENT_SYNC_NEXT_VIDEO_FRAME_DATA,
	EVENT_SYNC_SUB_VIDEO_FRAME_DATA,
	EVENT_SYNC_COPY_VIDEO_DATA,
	EVENT_SYNC_COPY_AUDIO_DATA,
	EVENT_NEED_VIDEO_DATA,
	EVENT_NEED_AUDIO_DATA,
	EVENT_VIDEO_INFO_DATA,
	EVENT_SYNC_VIDEO_INFO_DATA,
	EVENT_SYNC_VIDEO_PARAMS,
	EVENT_VIDEO_PARAMS,
	EVENT_STOP_REC,
	EVENT_STOP_COPY_REC,
	EVENT_SPLIT_EVENTS,
	EVENT_SPLIT_ACTIONS,
};

enum COPY_EVENT 
{
	EVENT_ACCEPT_COPY_ORDER	= 1,
	EVENT_REJECT_COPY_ORDER,
	EVENT_BUSY_COPY_ORDER
};

enum TYPE_DATA 
{
	TYPE_DATA_VIDEO,
	TYPE_DATA_AUDIO
};

enum TYPE_SIGNAL 
{
	SIGNAL_SEND = 1,
	SIGNAL_CLOSE,
	SIGNAL_WORK
};

enum CONNECT_STATUS 
{
	CONNECT_STATUS_OFFLINE,
	CONNECT_STATUS_ONLINE
};

enum CONNECT_TYPE 
{
	CONNECT_SERVER, 
	CONNECT_CLIENT
};

enum TRAFFIC_TYPE 
{
	TRAFFIC_UNKNOWN, 
	TRAFFIC_FULL_VIDEO,
	TRAFFIC_PREV_VIDEO,
	TRAFFIC_AUDIO,
	TRAFFIC_SERVICE,
	TRAFFIC_REMOTE_FILE,
	TRAFFIC_REMOTE_VIDEO,
	TRAFFIC_REMOTE_AUDIO,
	TRAFFIC_TRANSFER_FILE
};

enum FLAG_MESSAGE 
{
	FLAG_MESSAGE_EMPTY_DATA,
	FLAG_MESSAGE_EXIST_DATA
};

typedef struct
{
	unsigned int 		Key1;
	unsigned int 		Key2;
	unsigned int 		Key3;
	unsigned int 		SizeMessage;
	unsigned int		TypeMessage;
	char				FlagMessage;
	struct sockaddr_in  Address;	
} TRANSFER_DATA;

typedef struct
{
   int	Width;
   int	Height;
} VIDEO_INFO;

typedef struct
{
   int	Number;
   int	LenData;
   int  Flag;
   unsigned int CRC;
} VIDEO_FRAME_INFO;

typedef struct
{
   int	Number;
   int	LenData;
   unsigned int Flag;
   unsigned int CRC;
} AUDIO_FRAME_INFO;

typedef struct
{
   int	Number;
   int	SubNumber;
   int	LenData;
   int  Position;
} VIDEO_SUBFRAME_INFO;

typedef struct
{
   int	Number;
   int	SubNumber;
   int	LenData;
   int  Position;
} AUDIO_SUBFRAME_INFO;

typedef struct
{
	int					Socket;
	unsigned int		ID;
	struct sockaddr_in 	Addr;
	int 				Status;
	unsigned int		Type;
	unsigned int		TraffType;
	unsigned int		SendedBytes;
	unsigned int		RecvedBytes;
	int					Timer;
	unsigned int		NeedData;	
	unsigned int		DataNum;	
	unsigned int		DataPos;	
	unsigned int		InBufferSize;	
	char				*InBuffer;
	int					InDataSize;
	unsigned int		OutBufferSize;	
	char				*OutBuffer;
	int					OutDataSize;
	int					ClientPort;
	TX_EVENTER 			*pevntIP;
	pthread_mutex_t 	Socket_Mutex;
	pthread_mutex_t 	Pair_Mutex;
	int					socketpair[2];
	char				DateConnect[64];
} CONNECT_INFO;

typedef struct
{
	char if_name[IFNAMSIZ];
    struct sockaddr if_addr;
    struct sockaddr if_dstaddr;
    struct sockaddr if_broadaddr;
    struct sockaddr if_netmask;
    struct sockaddr if_hwaddr;
    short           if_flags;
    int             if_ifindex;
    int             if_metric;
    int             if_mtu;
    struct ifmap    if_map;
    char            if_slave[IFNAMSIZ];
    char            if_newname[IFNAMSIZ];    
} eth_config;

extern int iMessagesListCnt;
extern char *cMessagesList;
extern char iMessageListChanged;

extern unsigned int 		Connects_Count;
extern unsigned int 		Connects_Max_Active;
extern CONNECT_INFO 		*Connects_Info;

//char *PacketBuffer;

extern unsigned int iAlarmEvents;
extern unsigned int uiRecvDataCnt;
extern unsigned int uiSendDataCnt;

char* getnamemessage(int TypeMessage);
int GetLocalNetIf(eth_config *ifr);
int GetLocalNetIfFromDhcpcd(char *ipaddress, char *routers, char *mask, char *domain_name_servers, unsigned int ui_max_len);
int GetLocalAddr(struct sockaddr_in *Address);
int GetBroadcastAddr(struct sockaddr_in *Address);
void LoadPrintDump(char *cPath);
void PrintNanos(char *cLabel);
int InitLAN();
void DelLAN();
int Init_Servers(void);
unsigned int CalcCRC(char *buffer, unsigned int iLength);
unsigned int CalcCRC2(unsigned char *buffer, unsigned int iLength);
void CloseAllConnects(char cServer, unsigned int iFlag, char cFull);
int CloseConnectID(unsigned int uiID);
int TCP_Client(struct sockaddr_in *m_sin, int cTypeConnect, unsigned int iFlag, char cJoinToExist, unsigned int *uiID);
//void* TrafWorker(void *pData);
//void* Recv_Server(void *pData);
//void* Send_Server(void *pData);
void GetTransferDataCnt(unsigned int *pRecv, unsigned int *pSend);
void GetConnBufferStatus(unsigned int *pRecv, unsigned int *pSend);
char TestIncomeConnects();
char TestOutcomeConnects();
int SearchKeyInData(char *cData, int iDataLen, int iPos);
int ReWorkTraffic(int Num);
int SendMessage(unsigned int ConnNum, unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen, struct sockaddr_in *Address);
int SendTCPMessage(unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen, struct sockaddr_in *Address);
int SendUDPMessage(unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen, struct sockaddr_in *Address);
int SendBroadCastMessage(char ucBroadCastTCP, unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen);
void CloseConnect(int iNum, int self);
//void SendSignalSend();
char SendSubFrames(char *Buffer, int LenData, unsigned int iNumFrame, int iClientNum);
int SendVideoFrame(unsigned int iNumFrame, char* Buffer, int BufferSize, int LenData, int Flag, void* tVideoInfo, void* StartPack, int iTraffType);
int RecvVideoFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* StartPack, char cStream, unsigned int ConnectNum, unsigned int ConnectID);
int SendAudioFrame(unsigned int iNumFrame, unsigned int iFlag, char* Buffer, int LenData, void* pData, int iTraffType);
int RecvAudioFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* pData, char cStream, unsigned int ConnectNum, unsigned int ConnectID);
int ItsNetNeed(int iType, int iTraffType, int iFlags, char cCalc);
int GetLocalModule(unsigned int iID);
int SendMailFile(char *cLogin, char *cPassword, char *cServer, char *cFromAddress, char *cAuth, MAIL_INFO* cMlList);
int DownloadAddress(char *cPath, char **pBuffer, unsigned int *uiLen);
int DownloadFile(char *cPath, char *cSavePath);
void SendSignalType(unsigned int uiConnNum, char cType);
void * thread_WEB_income(void* pData);

#endif