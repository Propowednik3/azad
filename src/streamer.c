#include "streamer.h"
#include "main.h"
#include "pthread2threadx.h"
#include "network.h"
#include "flv-demuxer.h"
#include "flv-muxer.h"
#include "omx_client.h"
#include "writer.h"
#include "system.h"

#define TIME_BEFORE_NEW_OPEN	1000
#define TIME_BEFORE_CLOSE_IDLE	300000
#define MAX_AUDIO_FRAMES_IN_PACKET	30
#define MAX_VIDEO_FRAME_SIZE		65536
#define STRING_LAST					"Последние"
#define STRING_FULL					"Полные"
#define STRING_SLOW					"Медленные"
#define STRING_DIFF					"Изменения"
#define STRING_AUDIO				"Аудио"
#define STRING_ARCHIV				"Архив"
#define STRING_ARCHIV_FULL			"Архив Полные"
#define STRING_ARCHIV_SLOW			"Архив Медленные"
#define STRING_ARCHIV_DIFF			"Архив Изменения"
#define STRING_ARCHIV_AUDIO			"Архив Аудио"


void SortFileList(REMOTE_FILE_LIST **pList, int iFileCnt)
{
	if (!iFileCnt) return;
	DBG_LOG_IN();
	
	REMOTE_FILE_LIST *pFileList = *pList;	
	REMOTE_FILE_LIST *pOutList = (REMOTE_FILE_LIST*)DBG_MALLOC(sizeof(REMOTE_FILE_LIST)*iFileCnt);
	memset(pOutList, 0, sizeof(REMOTE_FILE_LIST)*iFileCnt);
	
	int iOutCnt = 0;
	int n, iCnt;
	
	int iNewCnt = 0;
	for (n = 0; n < iFileCnt; n++) if (pFileList[n].Type == 1) iNewCnt++;
	LIST_FILES_TYPE *pFiles = (LIST_FILES_TYPE*)DBG_MALLOC(sizeof(LIST_FILES_TYPE)*iNewCnt);
	memset(pFiles, 0, sizeof(LIST_FILES_TYPE)*iNewCnt);
	////////////////////////////////////////
	iCnt = 0;
	for (n = 0; n < iFileCnt; n++) 
		if (pFileList[n].Type == 1) 
		{
			memcpy(pFiles[iCnt].ShowName, pFileList[n].Name, MAX_FILE_LEN);
			memcpy(pFiles[iCnt].Name, pFileList[n].ShowName, MAX_FILE_LEN);
			iCnt++;
		}
	SortFiles(pFiles, iNewCnt);
	for (n = 0; n < iNewCnt; n++) 
	{
		memcpy(pOutList[iOutCnt].ShowName, pFiles[pFiles[n].Sort].Name, MAX_FILE_LEN);
		memcpy(pOutList[iOutCnt].Name, pFiles[pFiles[n].Sort].ShowName, MAX_FILE_LEN);
		pOutList[iOutCnt].Type = 1;
		iOutCnt++;
	}
	DBG_FREE(pFiles);
	/////////////////////////////////////
	iNewCnt = 0;
	for (n = 0; n < iFileCnt; n++) if (pFileList[n].Type == 0) iNewCnt++;
	pFiles = (LIST_FILES_TYPE*)DBG_MALLOC(sizeof(LIST_FILES_TYPE)*iNewCnt);
	memset(pFiles, 0, sizeof(LIST_FILES_TYPE)*iNewCnt);
	
	iCnt = 0;
	for (n = 0; n < iFileCnt; n++) 
		if (pFileList[n].Type == 0) 
		{
			memcpy(pFiles[iCnt].ShowName, pFileList[n].Name, MAX_FILE_LEN);
			memcpy(pFiles[iCnt].Name, pFileList[n].ShowName, MAX_FILE_LEN);
			iCnt++;
		}
	SortFiles(pFiles, iNewCnt);
	for (n = 0; n < iNewCnt; n++) 
	{
		memcpy(pOutList[iOutCnt].ShowName, pFiles[pFiles[n].Sort].Name, MAX_FILE_LEN);
		memcpy(pOutList[iOutCnt].Name, pFiles[pFiles[n].Sort].ShowName, MAX_FILE_LEN);
		pOutList[iOutCnt].Type = 0;
		iOutCnt++;
	}
	DBG_FREE(pFiles);
	
	/////////////////////////////////////
	iNewCnt = 0;
	for (n = 0; n < iFileCnt; n++) if (pFileList[n].Type > 1) iNewCnt++;
	pFiles = (LIST_FILES_TYPE*)DBG_MALLOC(sizeof(LIST_FILES_TYPE)*iNewCnt);
	memset(pFiles, 0, sizeof(LIST_FILES_TYPE)*iNewCnt);
	
	iCnt = 0;
	for (n = 0; n < iFileCnt; n++) 
		if (pFileList[n].Type > 1) 
		{
			memcpy(pFiles[iCnt].ShowName, pFileList[n].Name, MAX_FILE_LEN);
			memcpy(pFiles[iCnt].Name, pFileList[n].ShowName, MAX_FILE_LEN);
			iCnt++;
		}
	SortFiles(pFiles, iNewCnt);
	for (n = 0; n < iNewCnt; n++) 
	{		
		memcpy(pOutList[iOutCnt].ShowName, pFiles[pFiles[n].Sort].Name, MAX_FILE_LEN);
		memcpy(pOutList[iOutCnt].Name, pFiles[pFiles[n].Sort].ShowName, MAX_FILE_LEN);
		pOutList[iOutCnt].Type = 2;
		iOutCnt++;
	}
	DBG_FREE(pFiles);

	DBG_FREE(pFileList);
	*pList = pOutList;
		
	DBG_LOG_OUT();
}

int RequestFileList(struct sockaddr_in *Address, unsigned int uiMode)
{
	SendTCPMessage(TYPE_MESSAGE_REQUEST_FILELIST, (char*)&uiMode, sizeof(uiMode), NULL, 0, Address);
	return 1;
}

int NextRemoteFile(struct sockaddr_in *Address, unsigned int uiNewPos)
{
	SendTCPMessage(TYPE_MESSAGE_NEXT_FILELIST, (char*)&uiNewPos, sizeof(uiNewPos), NULL, 0, Address);
	return 1;
}

/*int PlayRemoteFile(struct sockaddr_in *Address, unsigned int uiNewPos, unsigned int uiWantStreams)
{
	unsigned int uiData[2];
	uiData[0] = uiNewPos;
	uiData[1] = uiWantStreams;
	SendTCPMessage(TYPE_MESSAGE_GET_PLAY_FILE, (char*)&uiData, sizeof(unsigned int) * 2, NULL, 0, Address);
	return 1;
}*/

int CloseRemoteFile(struct sockaddr_in *Address)
{
	SendTCPMessage(TYPE_MESSAGE_GET_CLOSE_FILE, NULL, 0, NULL, 0, Address);
	return 1;
}

int PrevRemoteFile(struct sockaddr_in *Address)
{
	SendTCPMessage(TYPE_MESSAGE_PREV_FILELIST, NULL, 0, NULL, 0, Address);
	return 1;
}

int ChangeRemoteFilePos(struct sockaddr_in *Address, unsigned int uiNewPos)
{
	SendTCPMessage(TYPE_MESSAGE_CHANGE_PLAY_FILE_POS, (char*)&uiNewPos, sizeof(unsigned int), NULL, 0, Address);
	return 1;
}

int SendStreamData(unsigned int uiType, void* pData1, unsigned int uiSize1, void* pData2, unsigned int uiSize2)
{
	int s;
	char ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
		
	for (s = 1; s < iConnMax; s++)
	{
		DBG_MUTEX_LOCK(&Connects_Info[s].Socket_Mutex);
		if ((Connects_Info[s].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[s].Type == CONNECT_SERVER) && (Connects_Info[s].TraffType == TRAFFIC_REMOTE_FILE))
		{
			if (SendMessage(s, uiType, (char*)pData1, uiSize1, (char*)pData2, uiSize2, &Connects_Info[s].Addr) > 0) ret++;
		}		
		DBG_MUTEX_UNLOCK(&Connects_Info[s].Socket_Mutex);			
	}
	if (ret == 0) dbgprintf(3,"No clients for SendStreamData MessageType:%i(%s)\n",uiType, getnamemessage(uiType));	
	return ret;
}

int ResetFileList(int iCaptureAccess, int iBackUpAccess, REMOTE_FILE_LIST **rFileList, unsigned int *uiFileCnt)
{
	int iCnt = *uiFileCnt;
	REMOTE_FILE_LIST *flist = *rFileList;
	
	if (iCnt) DBG_FREE(flist);
	iCnt = 0;
	if (iCaptureAccess) iCnt++;
	if (iBackUpAccess) iCnt++;
	flist = (REMOTE_FILE_LIST*)DBG_MALLOC(sizeof(REMOTE_FILE_LIST)*iCnt);
	memset(flist, 0, sizeof(REMOTE_FILE_LIST)*iCnt);
	int ret = 0;
	if (iCaptureAccess)
	{
		strcpy(flist[ret].Name, STRING_LAST);
		strcpy(flist[ret].ShowName, STRING_LAST);
		flist[ret].Type = 1;
		ret++;
	}
	if (iBackUpAccess)
	{
		strcpy(flist[ret].Name, STRING_ARCHIV);
		strcpy(flist[ret].ShowName, STRING_ARCHIV);
		flist[ret].Type = 1;
		ret++;
	}
	DBG_MUTEX_LOCK(&system_mutex);
	int i;
	for (i = 0; i < iDirsCnt; i++)
	{
		if (diDirList[i].RemoteAccess)
		{
			iCnt++;
			flist = (REMOTE_FILE_LIST*)DBG_REALLOC(flist, sizeof(REMOTE_FILE_LIST)*iCnt);
			memset(&flist[iCnt-1], 0, sizeof(REMOTE_FILE_LIST));
			strcpy(flist[ret].Name, diDirList[i].Name);
			strcpy(flist[ret].ShowName, diDirList[i].Name);
			flist[ret].Type = 1;
			ret++;
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	*rFileList = flist;
	*uiFileCnt = iCnt;
	return ret;
}

int GetFileList(char* cPathCurrent, REMOTE_FILE_LIST **rFileList, unsigned int *uiFileCnt)
{
	//printf("Path: %s\n", cPathCurrent);
	unsigned int uiCnt = 0;
	REMOTE_FILE_LIST *flist = NULL;
	
	char cSubDir[MAX_PATH];
	DIR *dir;
	DIR *subdir;
	struct dirent *dp;
	int n, len;
	dir = opendir(cPathCurrent);
	if (dir != NULL)
	{
		while((dp=readdir(dir)) != NULL)
		{
			n = 1;
			len = strlen(dp->d_name);			
			if (((len == 1) && (dp->d_name[0] == 46))
				||
				((len == 2) && (dp->d_name[0] == 46) && (dp->d_name[1] == 46))) n = 0;
			//printf("%s %i %i\n", dp->d_name, len, SearchStrInDataCaseIgn(dp->d_name, len, len - 4, ".FLV"));
			if ((strlen(dp->d_name) < MAX_FILE_LEN) && (n == 1))
			{			
				uiCnt++;
				flist = (REMOTE_FILE_LIST*)DBG_REALLOC(flist, uiCnt*sizeof(REMOTE_FILE_LIST));
				memset(&flist[uiCnt-1], 0, sizeof(REMOTE_FILE_LIST));
				if (strlen(dp->d_name) < MAX_FILE_LEN) 
				{
					strcpy(flist[uiCnt-1].Name, dp->d_name);
					n = MAX_FILE_LEN - 1;
					utf8_to_cp866(dp->d_name, strlen(dp->d_name), flist[uiCnt-1].ShowName, (unsigned int*)&n);
				}
				if ((strlen(cPathCurrent) + strlen(dp->d_name) + 1) < MAX_PATH)
				{
					memset(cSubDir, 0, MAX_PATH);				
					strcpy(cSubDir, cPathCurrent);
					strcat(cSubDir, "/");
					strcat(cSubDir, dp->d_name);
					subdir = opendir(cSubDir);
					if (subdir != NULL)
					{
						flist[uiCnt-1].Type = 1;
						closedir(subdir);
					}
					else
					{
						if ((errno != ENOTDIR) || ((len > 4) && (SearchStrInDataCaseIgn(dp->d_name, len, len - 4, ".FLV") != (len - 3))))
						{
							flist[uiCnt-1].Type = 2;
						}
					}
				} else flist[uiCnt-1].Type = 3;
			}
		}
		closedir(dir);			
	} else return 0;
	
	if (uiCnt)
	{
		if (*uiFileCnt) DBG_FREE(*rFileList);
		*uiFileCnt = uiCnt;
		SortFileList(&flist, uiCnt);
		*rFileList = flist;
	}
	return uiCnt;
}

int BackwardPath(char *cPath)
{
	int i;
	int iLen = strlen(cPath);
	for (i = iLen - 2; i >= 0; i--) if (cPath[i] == 47) break;
	if (i < 0) return 0;
	i++;
	memset(&cPath[i], 0, iLen - i);
	return 1;
}

void * Streamer(void *pData)
{
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "Streamer");
	
	misc_buffer *mBuff = (misc_buffer*)pData;
	int iCaptureAccess = mBuff->uidata[0];
	int iBackUpAccess = mBuff->uidata[1];
	int iCaptureAccessDef = mBuff->clock;
	
	char* pP = mBuff->void_data;
	char *cPathCaptureFull = &pP[0];
	char *cPathCaptureSlow = &pP[256];
	char *cPathCaptureDiff = &pP[512];
	char *cPathCaptureAudio = &pP[768];
	
	pP = mBuff->void_data2;
	char *cPathBackUpFull = &pP[0];
	char *cPathBackUpSlow = &pP[256];
	char *cPathBackUpDiff = &pP[512];
	char *cPathBackUpAudio = &pP[768];
	
	char cPathCurrent[MAX_PATH];
	char cShowPathCurrent[MAX_PATH];
	
	unsigned int uiPathLevel = 0;
	unsigned int uiPathType = 0;
	unsigned int uiPlayStatus = 0;
	unsigned int iNumVFrame = 0;
	unsigned int iNumAFrame = 0;
		
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	
	unsigned int uiEvent, ret, uiExData, uiReqStreams;
	unsigned int uiFileCnt = 0;
	unsigned int uiAudLimit = 0;
	unsigned int uiData[2];
	unsigned int uiBusyStatus = 0;
	REMOTE_FILE_LIST *rFileList = NULL;
	//REMOTE_FRAME rfFrame;
	//memset(&rfFrame, 0, sizeof(rfFrame));
	
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadStreamerStatus = 1;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	flv_demux_struct flv_info;
	memset(&flv_info, 0, sizeof(flv_demux_struct));
	
	omx_start_packet StartPack;
    memset(&StartPack, 0, sizeof(omx_start_packet));
    StartPack.CodecInfoLen = 0;
    StartPack.BufferCodecInfo = NULL;
	StartPack.BufferStartSize = 0;
    StartPack.BufferStartFrame = NULL;
		
	memset(&StartPack.VideoParams, 0, sizeof(StartPack.VideoParams)); 
	StartPack.VideoParams.video_codec = AV_CODEC_ID_H264;
	StartPack.VideoParams.video_width32 = 640;
	StartPack.VideoParams.video_height16 = 480;
	StartPack.VideoParams.video_width = 640;
	StartPack.VideoParams.video_height = 480;
	StartPack.VideoParams.video_frame_rate = 5;
	StartPack.VideoParams.video_intra_frame = 60;		
	StartPack.VideoParams.video_pixel_format = AV_PIX_FMT_YUV420P;
	StartPack.VideoParams.video_frame_size = ((StartPack.VideoParams.video_width * StartPack.VideoParams.video_height16 * 3)/2);	
	StartPack.VideoParams.Filled = 1;
	
	AudioCodecInfo tCodecInfo;
	memset(&tCodecInfo, 0, sizeof(tCodecInfo)); 
	tCodecInfo.audio_channels = 1;	
	tCodecInfo.audio_codec = AV_CODEC_ID_AAC;
	tCodecInfo.audio_profile = FF_PROFILE_UNKNOWN;
	//tCodecInfo.audio_sample_rate = miModule->Settings[3]; 
	//tCodecInfo.audio_bit_rate = miModule->Settings[4];
		
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_EXIT);	
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_LIST);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_PREV_LIST);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_NEXT_LIST);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_PLAY_LIST);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_START_FRAME);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_NEXT_VIDEO_FRAME);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_NEXT_AUDIO_FRAME);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_VIDEO_PARAMS);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_VIDEO_CODEC);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_PLAY_POS);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_CLOSE_FILE);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_AUDIO_PARAMS);
	tx_eventer_add_event(&strmevent_evnt, STRM_EVENT_GET_AUDIO_STREAM);	

	while(1)
	{
		uiEvent = 0;
		ret = tx_eventer_recv(&strmevent_evnt, &uiEvent, 1000, 0);
		if (ret != 0)
		{
			if (uiEvent == STRM_EVENT_EXIT) break;
			if (uiEvent == STRM_EVENT_GET_LIST)
			{
				uiData[1] = 0;
				DBG_MUTEX_LOCK(&stream_mutex);
				uiExData = cStreamerExData[0];
				DBG_MUTEX_UNLOCK(&stream_mutex);				
				if (uiPlayStatus == 0)
				{
					if (uiExData && iCaptureAccessDef && iCaptureAccess)
					{
						do
						{
							uiExData = 0;
							uiPathLevel = 0;
							memset(cPathCurrent, 0, MAX_PATH);
							memset(cShowPathCurrent, 0, MAX_PATH);
							ResetFileList(iCaptureAccess, iBackUpAccess, &rFileList, &uiFileCnt);
							
							uiPathType = 1;
							memset(cPathCurrent, 0, MAX_PATH);
							memset(cShowPathCurrent, 0, MAX_PATH);
							//strcat(cPathCurrent, "/");
							if ((strlen(cPathCurrent) + 5) >= MAX_PATH) break;
							
							strcpy(cShowPathCurrent, STRING_LAST);
							strcat(cShowPathCurrent, "/");
							if (iCaptureAccessDef == 1) 
							{
								memcpy(cPathCurrent, cPathCaptureFull, strlen(cPathCaptureFull));							
								strcat(cPathCurrent, DIR_NORM);
								strcat(cShowPathCurrent, DIR_NORM);
							}
							if (iCaptureAccessDef == 2) 
							{
								memcpy(cPathCurrent, cPathCaptureSlow, strlen(cPathCaptureSlow));							
								strcat(cPathCurrent, DIR_SLOW);
								strcat(cShowPathCurrent, DIR_SLOW);
							}
							if (iCaptureAccessDef == 3) 
							{
								memcpy(cPathCurrent, cPathCaptureDiff, strlen(cPathCaptureDiff));							
								strcat(cPathCurrent, DIR_DIFF);
								strcat(cShowPathCurrent, DIR_DIFF);
							}
							if (iCaptureAccessDef == 4) 
							{
								memcpy(cPathCurrent, cPathCaptureAudio, strlen(cPathCaptureAudio));							
								strcat(cPathCurrent, DIR_AUD);
								strcat(cShowPathCurrent, DIR_AUD);
							}
							strcat(cPathCurrent, "/");
							strcat(cShowPathCurrent, "/");
							
							if (!GetFileList(cPathCurrent, &rFileList, &uiFileCnt)) break;
							
							uiPathLevel++;
							int clk;
							int mx = -1;
							for (clk = 0; clk < uiFileCnt; clk++) 
							{
								if (rFileList[clk].Type != 1)
								{
									mx = -1;
									break;
								}
								if ((rFileList[clk].Type == 1) && (mx < clk)) mx = clk;
							}
							if (mx < 0) 
							{
								uiExData = 1;									
								break;
							}
							
							if ((strlen(cPathCurrent) + strlen(rFileList[mx].Name) + 1) >= MAX_PATH) break;
									
							strcat(cPathCurrent, rFileList[mx].Name);
							strcat(cShowPathCurrent, rFileList[mx].ShowName);
							strcat(cPathCurrent, "/");
							strcat(cShowPathCurrent, "/");
							
							if (!GetFileList(cPathCurrent, &rFileList, &uiFileCnt)) break;
							
							uiPathLevel++;
							mx = -1;
							for (clk = 0; clk < uiFileCnt; clk++) 
							{
								if (rFileList[clk].Type != 1)
								{
									mx = -1;
									uiExData = 1;
									break;
								}								
								if ((rFileList[clk].Type == 1) && (mx < clk)) mx = clk;
							}
							if (mx < 0) break;
							if ((strlen(cPathCurrent) + strlen(rFileList[mx].Name) + 1) >= MAX_PATH) break;
									
							strcat(cPathCurrent, rFileList[mx].Name);
							strcat(cShowPathCurrent, rFileList[mx].ShowName);
							strcat(cPathCurrent, "/");
							strcat(cShowPathCurrent, "/");
							
							if (!GetFileList(cPathCurrent, &rFileList, &uiFileCnt)) break; 
							
							uiPathLevel++;
							uiExData = 1;
							uiData[1] = 3;
						} while(0);
					}
					
					if ((uiExData == 0) || (iCaptureAccessDef == 0))
					{
						uiPathLevel = 0;
						memset(cPathCurrent, 0, MAX_PATH);
						memset(cShowPathCurrent, 0, MAX_PATH);
						ResetFileList(iCaptureAccess, iBackUpAccess, &rFileList, &uiFileCnt);
					}
				}
				uiData[0] = uiFileCnt;
				SendStreamData(TYPE_MESSAGE_FILELIST, uiData, sizeof(unsigned int)*2, rFileList, uiFileCnt*sizeof(REMOTE_FILE_LIST));
				SendStreamData(TYPE_MESSAGE_FILEPATH, cShowPathCurrent, strlen(cShowPathCurrent), NULL, 0);
			}
			if (uiEvent == STRM_EVENT_PREV_LIST)
			{
				uiData[1] = 2;
				if (uiPathLevel == 1)
				{
					uiPathLevel = 0;
					memset(cPathCurrent, 0, MAX_PATH);
					memset(cShowPathCurrent, 0, MAX_PATH);
					ResetFileList(iCaptureAccess, iBackUpAccess, &rFileList, &uiFileCnt);
				} 
				else 
				{
					uiPathLevel--;
					BackwardPath(cPathCurrent);	
					BackwardPath(cShowPathCurrent);	
					
					if (!GetFileList(cPathCurrent, &rFileList, &uiFileCnt))
					{
						uiPathLevel = 0;
						memset(cPathCurrent, 0, MAX_PATH);
						memset(cShowPathCurrent, 0, MAX_PATH);
						ResetFileList(iCaptureAccess, iBackUpAccess, &rFileList, &uiFileCnt);	
						uiData[1] = 0;
					}
				}
				uiData[0] = uiFileCnt;
				SendStreamData(TYPE_MESSAGE_FILELIST, uiData, sizeof(unsigned int)*2, rFileList, uiFileCnt*sizeof(REMOTE_FILE_LIST));
				SendStreamData(TYPE_MESSAGE_FILEPATH, cShowPathCurrent, strlen(cShowPathCurrent), NULL, 0);
			}
			if (uiEvent == STRM_EVENT_NEXT_LIST)
			{	
				uiData[1] = 1;
				DBG_MUTEX_LOCK(&stream_mutex);
				uiExData = cStreamerExData[0];
				DBG_MUTEX_UNLOCK(&stream_mutex);	
				if (uiPathLevel == 0)
				{
					if (uiExData > 1) uiPathType = 3;
					if ((uiExData == 0) && iCaptureAccess) uiPathType = 1;
					if (((uiExData == 0) && !iCaptureAccess && iBackUpAccess) ||
						((uiExData == 1) && iCaptureAccess && iBackUpAccess))
							uiPathType = 2;		
					memset(cPathCurrent, 0, MAX_PATH);
					memset(cShowPathCurrent, 0, MAX_PATH);
					if (uiPathType == 11) 
					{
						memcpy(cPathCurrent, cPathCaptureFull, strlen(cPathCaptureFull));
						strcpy(cShowPathCurrent, STRING_FULL);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 12) 
					{
						memcpy(cPathCurrent, cPathCaptureSlow, strlen(cPathCaptureSlow));
						strcpy(cShowPathCurrent, STRING_SLOW);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 13) 
					{
						memcpy(cPathCurrent, cPathCaptureDiff, strlen(cPathCaptureDiff));
						strcpy(cShowPathCurrent, STRING_DIFF);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 14) 
					{
						memcpy(cPathCurrent, cPathCaptureAudio, strlen(cPathCaptureAudio));
						strcpy(cShowPathCurrent, STRING_AUDIO);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 21)
					{
						memcpy(cPathCurrent, cPathBackUpFull, strlen(cPathBackUpFull));
						strcpy(cShowPathCurrent, STRING_ARCHIV_FULL);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 22)
					{
						memcpy(cPathCurrent, cPathBackUpSlow, strlen(cPathBackUpSlow));
						strcpy(cShowPathCurrent, STRING_ARCHIV_SLOW);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 23)
					{
						memcpy(cPathCurrent, cPathBackUpDiff, strlen(cPathBackUpDiff));
						strcpy(cShowPathCurrent, STRING_ARCHIV_DIFF);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 24)
					{
						memcpy(cPathCurrent, cPathBackUpAudio, strlen(cPathBackUpAudio));
						strcpy(cShowPathCurrent, STRING_ARCHIV_AUDIO);
						strcat(cShowPathCurrent, "/");
					}
					if (uiPathType == 3)
					{
						if (iCaptureAccess) uiExData--;
						if (iBackUpAccess) uiExData--;
						DBG_MUTEX_LOCK(&system_mutex);
						int i, cn, n;
						cn = 0;
						n = -1;
						for (i = 0; i < iDirsCnt; i++) 
						{
							if (diDirList[i].RemoteAccess) 
							{
								if (cn == uiExData) { n = i; break; }
								cn++;
							}
						}
						if (n != -1)
						{
							memcpy(cPathCurrent, diDirList[n].Path, strlen(diDirList[n].Path));
							strcpy(cShowPathCurrent,  diDirList[n].Name);
							strcat(cShowPathCurrent, "/");
							n = strlen(cPathCurrent);
							if (n && (cPathCurrent[n - 1] != 47)) cPathCurrent[n] = 47;					
						}
						DBG_MUTEX_UNLOCK(&system_mutex);
					}
				}
				else
				{
					if ((uiExData <= uiFileCnt) && (rFileList[uiExData].Type == 1) && ((strlen(cPathCurrent) + strlen(rFileList[uiExData].Name) + 1) < MAX_PATH))
					{
						strcat(cPathCurrent, rFileList[uiExData].Name);
						strcat(cPathCurrent, "/");
						strcat(cShowPathCurrent, rFileList[uiExData].ShowName);
						strcat(cShowPathCurrent, "/");
					}					
				}
				uiPathLevel++;
				if (!GetFileList(cPathCurrent, &rFileList, &uiFileCnt))
				{
					if (uiPathLevel == 1)
					{
						memset(cPathCurrent, 0, MAX_PATH);
						memset(cShowPathCurrent, 0, MAX_PATH);
						ResetFileList(iCaptureAccess, iBackUpAccess, &rFileList, &uiFileCnt);	
					} 
					else 
					{
						BackwardPath(cPathCurrent);
						BackwardPath(cShowPathCurrent);
					}
					uiPathLevel--;
					uiData[0] = 0;
					//uiData[1] = 0;
					SendStreamData(TYPE_MESSAGE_FILELIST, uiData, sizeof(unsigned int)*2, NULL, 0);
					SendStreamData(TYPE_MESSAGE_FILEPATH, cShowPathCurrent, strlen(cShowPathCurrent), NULL, 0);
				}
				else
				{
					uiData[0] = uiFileCnt;
					SendStreamData(TYPE_MESSAGE_FILELIST, uiData, sizeof(unsigned int)*2, rFileList, uiFileCnt*sizeof(REMOTE_FILE_LIST));
					SendStreamData(TYPE_MESSAGE_FILEPATH, cShowPathCurrent, strlen(cShowPathCurrent), NULL, 0);
				}
			}
			
			if (uiEvent == STRM_EVENT_PLAY_POS)
			{
				DBG_MUTEX_LOCK(&stream_mutex);
				uiExData = cStreamerExData[0];
				DBG_MUTEX_UNLOCK(&stream_mutex);
				flv_dm_set_pos(&flv_info, uiExData);
				//printf("SET POS %i %i\n", uiExData, flv_info.CurrentScrollFrame);
				SendStreamData(TYPE_MESSAGE_NEW_FILE_POS, &flv_info.CurrentScrollFrame, sizeof(flv_info.CurrentScrollFrame), NULL, 0);				
			}
				
			if (uiEvent == STRM_EVENT_PLAY_LIST)
			{
				//printf("STRM_EVENT_PLAY_LIST %i %i\n", (unsigned int)get_ms(&previous_ms), TIME_BEFORE_NEW_OPEN);
				if ((unsigned int)get_ms(&previous_ms) > TIME_BEFORE_NEW_OPEN)
				{
					previous_ms = 0;
					get_ms(&previous_ms);
					
					DBG_MUTEX_LOCK(&stream_mutex);
					uiExData = cStreamerExData[0];
					uiReqStreams = cStreamerExData[1];
					DBG_MUTEX_UNLOCK(&stream_mutex);				
					
					if ((uiExData <= uiFileCnt) && (rFileList[uiExData].Type == 0) && ((strlen(cPathCurrent) + strlen(rFileList[uiExData].Name) + 1) < MAX_PATH))
					{
						iNumVFrame = 0;
						iNumAFrame = 0;
						if (flv_info.Status)
						{
							//SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_DATACORRUPT, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
							//SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
							flv_dm_close(&flv_info, &StartPack);
						}
						memset(&flv_info, 0, sizeof(flv_demux_struct));
						strcpy(flv_info.Name, rFileList[uiExData].ShowName);
						strcpy(flv_info.Path, cPathCurrent);
						strcat(flv_info.Path, rFileList[uiExData].Name);
						strcpy(flv_info.ShowPath, cShowPathCurrent);
						strcat(flv_info.ShowPath, rFileList[uiExData].ShowName);
						ret = flv_dm_open(&flv_info, &StartPack, &tCodecInfo, uiReqStreams);
						if (ret)
						{
							//printf("!!!! START SIZE %i %i %i\n", StartPack.StartFrameLen, flv_info.AudioEnabled, flv_info.VideoEnabled);
							if (flv_info.Status)
							{
								if (flv_info.AudioEnabled || flv_info.VideoEnabled)
								{
									//printf("TYPE_MESSAGE_OPENED_FILE %i\n", flv_info.Status);
									SendStreamData(TYPE_MESSAGE_OPENED_FILE, (char*)&flv_info, sizeof(flv_demux_struct), NULL, 0);
								}
								else
								{
									SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);	
									flv_dm_close(&flv_info, &StartPack);
									memset(&flv_info, 0, sizeof(flv_demux_struct));
									dbgprintf(2, "Error flv_dm_open no media(%i)\n", uiExData);
								}
							}
							else 
							{
								SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
								dbgprintf(2, "Error flv_dm_open (%i)\n", uiExData);
							}
						}
						else
						{
							SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
							dbgprintf(2, "Error flv_dm_open 2 (%i)\n", ret);
						}
					} else dbgprintf(2, "Error open file from remote file list (%i)\n", uiExData);
				}
			}

			if ((uiEvent == STRM_EVENT_GET_VIDEO_PARAMS) || (uiEvent == STRM_EVENT_GET_VIDEO_CODEC) || (uiEvent == STRM_EVENT_GET_START_FRAME))
			{
				//printf("STRM_EVENT_GET_VIDEO_PARAMS\n");
				if (flv_info.Status)
				{
					unsigned int uiCurSize = flv_info.Frame_Lenght - flv_info.Frame_Sended;
					unsigned int uiCurLen;
					if (uiCurSize > VIDEO_CODER_BUFFER_SIZE) 
							uiCurLen = VIDEO_CODER_BUFFER_SIZE;
							else
							uiCurLen = uiCurSize;
					//printf(">>S %i %i %i %i\n", iNumVFrame, flv_info.Frame_Sended, &flv_info.Frame[flv_info.Frame_Sended], uiCurLen);
									
					ret = SendVideoFrame(iNumVFrame, &flv_info.Frame[flv_info.Frame_Sended], VIDEO_CODER_BUFFER_SIZE, 
														uiCurLen, 
														OMX_BUFFERFLAG_STARTTIME, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);										
					flv_info.Frame_Sended += uiCurLen;
					iNumVFrame++;
					if (ret > 0)
					{
						if (uiEvent == STRM_EVENT_GET_VIDEO_PARAMS) flv_info.Status |= 2;
						if (uiEvent == STRM_EVENT_GET_VIDEO_CODEC) flv_info.Status |= 4;
						if (uiEvent == STRM_EVENT_GET_START_FRAME) flv_info.Status |= 8;						
					}
					//printf("STRM_EVENT_GET_VIDEO_PARAMS %i\n", flv_info.Status);
				}
				else
				{
					SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_DATACORRUPT , &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);
					//SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
					dbgprintf(2, "Error read video params from flv file (file closed)\n");
				}
			}
			
			if (uiEvent == STRM_EVENT_GET_AUDIO_PARAMS)
			{
				//printf("STRM_EVENT_GET_AUDIO_PARAMS\n");
				if (flv_info.Status)
				{
					//printf("Send audio param %i\n", flv_info.Status);
					ret = SendAudioFrame(iNumAFrame, FLAG_AUDIO_CODEC_INFO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);
					if (ret > 0) flv_info.Status |= 16;
				}
				else
				{
					//printf("Send STOP_AUDIO1 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);									
					SendAudioFrame(iNumAFrame, FLAG_STOP_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);
					//SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
					dbgprintf(2, "Error read audio params from flv file (file closed)\n");
				}
			}
			
			if (uiEvent == STRM_EVENT_GET_NEXT_AUDIO_FRAME)
			{
				//printf("STRM_EVENT_GET_NEXT_AUDIO_FRAME\n");
				if (flv_info.Status)
				{
					uiAudLimit = MAX_AUDIO_FRAMES_IN_PACKET;
					if ((flv_info.Status & 17) != 17)
					{
						dbgprintf(2, "Resend audio param %i\n", flv_info.Status);
						ret = SendAudioFrame(iNumAFrame, FLAG_AUDIO_CODEC_INFO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);														
					}
					else
					{
						ret = 0;
						DBG_MUTEX_LOCK(&system_mutex);
						if (uiBusyStatus && (uiThreadCopyStatus == 0) && (tfTransferFileStatus.Status == 0)) ret = 1;
						uiBusyStatus = uiThreadCopyStatus | tfTransferFileStatus.Status;
						DBG_MUTEX_UNLOCK(&system_mutex);
						if (ret) SendStreamData(TYPE_MESSAGE_FREE_FILE, NULL, 0, NULL, 0);
						
						do
						{
							if (uiBusyStatus)
							{
								dbgprintf(4, "Send BUSY_FILE %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);									
								if (flv_info.AudioEnabled) SendAudioFrame(iNumAFrame, FLAG_DONE_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
								if (flv_info.VideoEnabled) SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_EOS, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
								SendStreamData(TYPE_MESSAGE_BUSY_FILE, NULL, 0, NULL, 0);
								break;						
							}
							
							if (flv_info.Reworked == 0) ret = 1;
								else ret = flv_dm_read_frame(&flv_info, &StartPack);
							
							if (ret == 1)
							{	
								if ((flv_info.Frame_Type != FLV_TAG_TYPE_META) &&
									(flv_info.Frame_Type != FLV_TAG_TYPE_AUDIO) &&
									(flv_info.Frame_Type != FLV_TAG_TYPE_VIDEO))
									{
										dbgprintf(2, "Wrong FLV frame type %i\n", flv_info.Frame_Type);
										break;
									}
								if (flv_info.Frame_Type == FLV_TAG_TYPE_META)
								{
									flv_info.Reworked = 1;
								}
								
								if (flv_info.Frame_Type == FLV_TAG_TYPE_AUDIO)
								{
									if (uiAudLimit) 
									{
										uiAudLimit--;
										//printf("Send Audio frame 1 %i %i\n", flv_info.Frame_Lenght, flv_info.CurrentByte);
										SendAudioFrame(iNumAFrame, uiAudLimit ? FLAG_NEXT_AUDIO_FRAME : (FLAG_NEXT_AUDIO_FRAME | FLAG_DONE_AUDIO_FRAME), flv_info.Frame, flv_info.Frame_Lenght, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);
										if (!flv_info.VideoEnabled)
											SendStreamData(TYPE_MESSAGE_NEW_FILE_POS, &flv_info.CurrentScrollFrame, sizeof(flv_info.CurrentScrollFrame), NULL, 0);							
										if (uiAudLimit == 0) break;
									}
									flv_info.Reworked = 1;
									iNumAFrame++;
									if ((flv_info.TotalFrames - 1) == flv_info.CurrentFrame)
									{
										//printf("Send DONE_AUDIO1 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);									
										if (flv_info.AudioEnabled) SendAudioFrame(iNumAFrame, FLAG_DONE_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
										if (flv_info.VideoEnabled) SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_EOS, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
										SendStreamData(TYPE_MESSAGE_DONE_FILE, NULL, 0, NULL, 0);
									}
								}
								
								if (flv_info.Frame_Type == FLV_TAG_TYPE_VIDEO) break;
							}
							else
							{
								//printf("Send DONE_AUDIO2 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);									
								if (flv_info.VideoEnabled) 
										SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_EOS, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
								if (flv_info.AudioEnabled) 
										SendAudioFrame(iNumAFrame, FLAG_DONE_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
								SendStreamData(TYPE_MESSAGE_DONE_FILE, NULL, 0, NULL, 0);
								if (ret != 0) dbgprintf(2, "Error read frame from flv file 1 (%i)\n", ret);
								break;						
							}
						} while(1);
					}
				}
				else
				{
					//printf("Send STOP_AUDIO2 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);									
					if (flv_info.AudioEnabled)
						SendAudioFrame(iNumAFrame, FLAG_STOP_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
					//SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_DATACORRUPT , &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
					//SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
					dbgprintf(2, "Error read audio frame from flv file (file closed)\n");
				}				
			}
			
			if (uiEvent == STRM_EVENT_GET_NEXT_VIDEO_FRAME)
			{
				//printf("STRM_EVENT_GET_NEXT_VIDEO_FRAME 0\n");					
				if (flv_info.Status)
				{
					if ((flv_info.Status & 15) != 15)
					{
						dbgprintf(2, "Resend start video frame %i\n", flv_info.Status);
						unsigned int uiCurSize = flv_info.Frame_Lenght - flv_info.Frame_Sended;
						unsigned int uiCurLen;
						if (uiCurSize > VIDEO_CODER_BUFFER_SIZE) 
							uiCurLen = VIDEO_CODER_BUFFER_SIZE;
							else
							uiCurLen = uiCurSize;
						ret = SendVideoFrame(iNumVFrame, &flv_info.Frame[flv_info.Frame_Sended], VIDEO_CODER_BUFFER_SIZE, 
														uiCurLen, 
														OMX_BUFFERFLAG_STARTTIME, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);										
						flv_info.Frame_Sended += uiCurLen;
						iNumVFrame++;
					}
					else
					{
						ret = 0;
						DBG_MUTEX_LOCK(&system_mutex);
						if (uiBusyStatus && (uiThreadCopyStatus == 0) && (tfTransferFileStatus.Status == 0)) ret = 1;
						uiBusyStatus = uiThreadCopyStatus | tfTransferFileStatus.Status;
						DBG_MUTEX_UNLOCK(&system_mutex);
						if (ret) SendStreamData(TYPE_MESSAGE_FREE_FILE, NULL, 0, NULL, 0);
						
						do
						{
							if (uiBusyStatus)
							{
								//printf("Send BUSY_FILE2 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);
								if (flv_info.AudioEnabled) SendAudioFrame(iNumAFrame, FLAG_DONE_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
								if (flv_info.VideoEnabled) SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_EOS, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
								SendStreamData(TYPE_MESSAGE_BUSY_FILE, NULL, 0, NULL, 0);
								break;						
							}
							
							//printf("flv_dm_read_frame %i\n", flv_info.Reworked);
							if ((flv_info.TotalFrames - 1) == flv_info.CurrentFrame) ret = 0;
							else
							{
								if (flv_info.Reworked == 0) ret = 1;
									else ret = flv_dm_read_frame(&flv_info, &StartPack);
							}
							
							if (ret == 1)
							{
								if (flv_info.Frame_Type == FLV_TAG_TYPE_META)
								{
									flv_info.Reworked = 1;
								}
								
								if (flv_info.Frame_Type == FLV_TAG_TYPE_AUDIO)
								{
									if (flv_info.AudioEnabled)
									{
										if (uiAudLimit)
										{
											uiAudLimit--;
											//printf("Send Audio frame 2 %i %i %i %i\n", uiAudLimit, flv_info.Frame_Lenght, flv_info.CurrentByte, uiAudLimit ? FLAG_NEXT_AUDIO_FRAME : (FLAG_NEXT_AUDIO_FRAME | FLAG_DONE_AUDIO_FRAME));
											//printf("Send Audio frame 3 %i %i %i %i\n", uiAudLimit,
											//uiAudLimit ? FLAG_NEXT_AUDIO_FRAME : (FLAG_NEXT_AUDIO_FRAME | FLAG_DONE_AUDIO_FRAME),
											//FLAG_NEXT_AUDIO_FRAME, (FLAG_NEXT_AUDIO_FRAME | FLAG_DONE_AUDIO_FRAME));
											SendAudioFrame(iNumAFrame, uiAudLimit ? FLAG_NEXT_AUDIO_FRAME : (FLAG_NEXT_AUDIO_FRAME | FLAG_DONE_AUDIO_FRAME), flv_info.Frame, flv_info.Frame_Lenght, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
										}
									}// else printf("Skip audio frame %i\n", flv_info.Frame_Lenght);
									flv_info.Reworked = 1;
									iNumAFrame++;
								}
								
								if (flv_info.Frame_Type == FLV_TAG_TYPE_VIDEO)
								{
									//printf("Send Video frame %i %i\n", flv_info.Frame_Lenght, flv_info.Frame_MaxSize);
									unsigned int uiCurSize = flv_info.Frame_Lenght - flv_info.Frame_Sended;
									unsigned int uiCurLen;
									if (uiCurSize > VIDEO_CODER_BUFFER_SIZE) 
										uiCurLen = VIDEO_CODER_BUFFER_SIZE;
										else
										uiCurLen = uiCurSize;
									//printf(">>> %i %i %i %i\n", iNumVFrame, flv_info.Frame_Sended, &flv_info.Frame[flv_info.Frame_Sended], uiCurLen);
									SendVideoFrame(iNumVFrame, &flv_info.Frame[flv_info.Frame_Sended], VIDEO_CODER_BUFFER_SIZE, 
																	uiCurLen, 
																	OMX_BUFFERFLAG_TIMESTAMPINVALID, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);										
									flv_info.Frame_Sended += uiCurLen;		
									if (flv_info.Frame_Sended == flv_info.Frame_Lenght) flv_info.Reworked = 1;
									
									SendStreamData(TYPE_MESSAGE_NEW_FILE_POS, &flv_info.CurrentScrollFrame, sizeof(flv_info.CurrentScrollFrame), NULL, 0);							
									iNumVFrame++;
									break;
								}
							}
							else
							{
								//printf("Send DONE_AUDIO4 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);								
								if (flv_info.AudioEnabled) SendAudioFrame(iNumAFrame, FLAG_DONE_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
								if (flv_info.VideoEnabled) SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_EOS, &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
								SendStreamData(TYPE_MESSAGE_DONE_FILE, NULL, 0, NULL, 0);
								if (ret != 0) dbgprintf(2, "Error read frame from flv file 2 (%i)\n", ret);
								break;						
							}
						} while(1);						
					}
				}
				else
				{
					SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_DATACORRUPT , &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
					//SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
					dbgprintf(2, "Error read video frame from flv file (file closed)\n");
				}	
				//printf("STRM_EVENT_GET_NEXT_VIDEO_FRAME 1\n");
			}
			if (uiEvent == STRM_EVENT_GET_CLOSE_FILE)
			{
				if (flv_info.Status)
				{
					//printf("STRM_EVENT_GET_CLOSE_FILE\n");
					//printf("Send STOP_AUDIO3 %i %i\n", flv_info.AudioEnabled, flv_info.VideoEnabled);
					if (flv_info.AudioEnabled) SendAudioFrame(iNumAFrame, FLAG_STOP_AUDIO, flv_info.Frame, 0, &tCodecInfo, TRAFFIC_REMOTE_AUDIO);									
					if (flv_info.VideoEnabled) SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_DATACORRUPT , &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
					SendStreamData(TYPE_MESSAGE_CLOSED_FILE, NULL, 0, NULL, 0);	
					flv_dm_close(&flv_info, &StartPack);					
				}
				else
				{
					//SendVideoFrame(iNumVFrame, flv_info.Frame, flv_info.Frame_MaxSize, 0, OMX_BUFFERFLAG_DATACORRUPT , &StartPack.VideoParams, &StartPack, TRAFFIC_REMOTE_VIDEO);				
					//SendStreamData(TYPE_MESSAGE_ERROR_FILE, NULL, 0, NULL, 0);
					//dbgprintf(2, "Error read frame from flv file (file closed)\n");
				}
			}
		}
		else
		{
			if (flv_info.Status)
			{
				if (get_ms(&previous_ms) > TIME_BEFORE_CLOSE_IDLE)
				{
					flv_dm_close(&flv_info, &StartPack);
					SendStreamData(TYPE_MESSAGE_CLOSED_FILE, NULL, 0, NULL, 0);
				}
			}
		}
	}

	if (flv_info.Status) flv_dm_close(&flv_info, &StartPack);
	
	DBG_FREE(mBuff->void_data);
	DBG_FREE(mBuff->void_data2);
	DBG_FREE(mBuff);
	if (uiFileCnt) DBG_FREE(rFileList);
	
	DBG_LOG_OUT();
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadStreamerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	
	return NULL;
}