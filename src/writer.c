#include "writer.h"
#include "main.h"
#include "pthread2threadx.h"
#include "network.h"
#include "audio.h"
#include "system.h"

#define TIME_BEFORE_CLOSE_IDLE	300000

int CloseWriter()
{
	int ret;
	tx_eventer_send_event(&writeevent_evnt, WRT_EVENT_COPY_FILE_EXIT);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadWriterStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);
	return 1;
}

int SendWriterMess(unsigned int uiConnectNum, unsigned int uiType, void* pData, unsigned int uiSize, struct sockaddr_in *Address)
{
	char ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	unsigned int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
		
	if (uiConnectNum < iConnMax)
	{
		//printf("[WRITER] Send %i, %i %i\n",uiConnectNum, uiType, uiSize);
		DBG_MUTEX_LOCK(&Connects_Info[uiConnectNum].Socket_Mutex);
		if ((Connects_Info[uiConnectNum].Status == CONNECT_STATUS_ONLINE) && 
			(Connects_Info[uiConnectNum].Type == CONNECT_SERVER) && 
			(Connects_Info[uiConnectNum].TraffType == TRAFFIC_TRANSFER_FILE) &&
			(Connects_Info[uiConnectNum].Addr.sin_addr.s_addr == Address->sin_addr.s_addr))
		{
			if (SendMessage(uiConnectNum, uiType, (char*)pData, uiSize, NULL, 0, &Connects_Info[uiConnectNum].Addr) > 0) ret++;
		}		
		DBG_MUTEX_UNLOCK(&Connects_Info[uiConnectNum].Socket_Mutex);			
	}
	if (ret == 0) dbgprintf(3,"Error connect num %i for SendWriterMess, MessageType:%i(%s)\n",uiConnectNum, uiType, getnamemessage(uiType));	
	return ret;
}

void * Writer(void *pData)
{
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "Writer");
	
	unsigned int uiEvent, ret, iLoop, n;
	TRANSFER_FILE_INFO tiStatus;
    memset(&tiStatus, 0, sizeof(tiStatus));	
	misc_buffer *mBuff = (misc_buffer*)pData;
	char *cCurrPath = NULL;
	char *cMainPath = mBuff->void_data;
	char *pDataPt;
	unsigned int uiDataLen;
	iLoop = 1;
	if (cMainPath) ret = strlen(cMainPath); else ret = 0;
	if ((ret == 0) || (ret > MAX_PATH))
	{
		dbgprintf(1, "Wrong path for Writer\n");
		iLoop = 0;
	}
	else
	{
		cCurrPath = (char*)DBG_MALLOC(MAX_PATH*2);
	}		
	
	DIRECTORY_INFO directoryNull;
	memset(&directoryNull, 0, sizeof(DIRECTORY_INFO));
	DIRECTORY_INFO *directoryInfo;
	memcpy(directoryNull.Path, cMainPath, MAX_PATH);
	
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadWriterStatus = 1;
	tfTransferFileStatus.Enabled = 1;
	tfTransferFileStatus.Status = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
										
	tx_eventer_add_event(&writeevent_evnt, WRT_EVENT_COPY_FILE_GET_ORDER);
	tx_eventer_add_event(&writeevent_evnt, WRT_EVENT_COPY_FILE_EXIT);
	int iChannel;
	char *pWriteBuffer = NULL;
	unsigned int uiWriteBufferSize = 0;
	unsigned int wrtd;
	unsigned int uiData[2];
	unsigned int uiCurrentFrame = 0;
	unsigned int uiFrameLen = 0;
	unsigned int uiRecvFileLen = 0;
	
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	int64_t previous_ms_2 = 0;
	get_ms(&previous_ms_2);
	
	
	if (iLoop)
	{
		uiWriteBufferSize = FILE_PACKET_BUFFER_SIZE;
		pWriteBuffer = (char*)DBG_MALLOC(uiWriteBufferSize);
	}
	
	while(iLoop)
	{
		uiEvent = 0;
		ret = 0;
		if (tiStatus.Status == 0)
		{
			iChannel = 0;
			uiEvent = 0;
			ret = tx_eventer_recv(&writeevent_evnt, &uiEvent, 1000, 0);
		}
		else
		{
			iChannel = 0;
			uiEvent = 0;
			tx_eventer_recv(&writeevent_evnt, &uiEvent, 0, 0);
			if (uiEvent == 0)
			{
				iChannel = 1;
				pthread_mutex_lock(&Connects_Info[tiStatus.ConnectNum].pevntIP->mutex);
				SendSignalType(tiStatus.ConnectNum, SIGNAL_WORK);
				//printf("SIGNAL_WORK %i %i\n", Connects_Info[tiStatus.ConnectNum].RecvedBytes,
					//			Connects_Info[tiStatus.ConnectNum].InDataSize);
				ret = tx_eventer_recv_data_prelocked(Connects_Info[tiStatus.ConnectNum].pevntIP, WRT_EVENT_COPY_FILE_DATA, (void**)&pDataPt, &uiDataLen, 1000);
			}
		}
		if (ret != 0)
		{
			//printf("[WRITER] Recv %i %i %i %i, %i\n",ret, tiStatus.Status, iChannel, uiEvent, uiDataLen);					
			if (iChannel == 0) 
			{
				if (uiEvent == WRT_EVENT_COPY_FILE_EXIT)
				{
					//printf("WRT_EVENT_COPY_FILE_EXIT\n");
					if (tiStatus.Status)
					{
						fclose(tiStatus.filehandle);
						SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
						DBG_MUTEX_LOCK(&stream_mutex);
						tfTransferFileStatus.Status = 0;
						DBG_MUTEX_UNLOCK(&stream_mutex);
						tiStatus.Status = 0;
					}
					break;
				}
				
				if (uiEvent == WRT_EVENT_COPY_FILE_GET_ORDER)
				{
					//printf("WRT_EVENT_COPY_FILE_GET_ORDER\n");
					DBG_MUTEX_LOCK(&stream_mutex);
					memcpy(&tiStatus, &tfTransferFileStatus, sizeof(tiStatus));
					DBG_MUTEX_UNLOCK(&stream_mutex);
					
					memset(cCurrPath, 0, MAX_PATH*2);
					directoryInfo = NULL;
					
					DBG_MUTEX_LOCK(&system_mutex);
					switch(tiStatus.Type)
					{
						case WRT_TYPE_ARCHIVE_FULL: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_NORM, 2); break;
						case WRT_TYPE_ARCHIVE_SLOW: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_SLOW, 2); break;
						case WRT_TYPE_ARCHIVE_DIFF: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_DIFF, 2); break;
						case WRT_TYPE_ARCHIVE_AUDIO: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_AUDIO, 2); break;
						case WRT_TYPE_ARCHIVE_STATUSES: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_STATUSES, 2); break;
						case WRT_TYPE_ARCHIVE_EVENTS: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_EVENTS, 2); break;
						case WRT_TYPE_ARCHIVE_ACTIONS: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_ACTIONS, 2); break;
						case WRT_TYPE_BACKUP_FULL: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_NORM, 1); break;
						case WRT_TYPE_BACKUP_SLOW: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_SLOW, 1); break;
						case WRT_TYPE_BACKUP_DIFF: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_DIFF, 1); break;
						case WRT_TYPE_BACKUP_AUDIO: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_AUDIO, 1); break;
						case WRT_TYPE_BACKUP_STATUSES: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_STATUSES, 1); break;
						case WRT_TYPE_BACKUP_EVENTS: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_EVENTS, 1); break;
						case WRT_TYPE_BACKUP_ACTIONS: directoryInfo = GetDirectoryInfoPoint(diDirList, iDirsCnt, DIRECTORY_TYPE_ACTIONS, 1); break;
						default: directoryInfo = &directoryNull; break;
					}
					if (!directoryInfo) 
					{
						directoryInfo = &directoryNull;
						dbgprintf(2, "Not found PATH setting for type: %i\n", tiStatus.Type);
						dbgprintf(3, "Using default path: '%s'\n", directoryInfo->Path);						
					}
					strcpy(cCurrPath, directoryInfo->Path);
					DBG_MUTEX_UNLOCK(&system_mutex);					
					
					n = strlen(cCurrPath);
					if (n && (cCurrPath[n - 1] != 47)) strcat(cCurrPath, "/");
					strcat(cCurrPath, tiStatus.Path);
					
					//printf("Create file '%s'\n", cCurrPath);
					CreateDirectoryFromPath(cCurrPath, 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
					tiStatus.filehandle = fopen(cCurrPath, "wb");
					if (!tiStatus.filehandle) 
					{
						DBG_MUTEX_LOCK(&stream_mutex);
						tfTransferFileStatus.Status = 0;
						DBG_MUTEX_UNLOCK(&stream_mutex);
						tiStatus.Status = 0;
						dbgprintf(2, "Error open file for copy(%i)%s\n",errno, strerror(errno));
					}
		
					if (tiStatus.Status)
					{
						uiRecvFileLen = 0;
						previous_ms = 0;
						get_ms(&previous_ms);
						SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_ACCEPT_ORDER, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
					}
					else SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
				}
			}
			else
			{
				if (uiDataLen > sizeof(unsigned int))
				{	
					uiFrameLen = uiDataLen - sizeof(unsigned int);
					int resl = 0;
					if (uiWriteBufferSize >= uiFrameLen)
					{
						memcpy(pWriteBuffer, &pDataPt[4], uiFrameLen);	
						memcpy(&uiCurrentFrame, pDataPt, sizeof(unsigned int)); 
						resl = 1;
					}	
					
					uiRecvFileLen += uiFrameLen;
					
					if (!tx_eventer_return_data(Connects_Info[tiStatus.ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
							dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);

					if (uiWriteBufferSize < uiFrameLen)
					{
						dbgprintf(2, "Error COPY_FILE_DATA big data %i >= %i\n",uiFrameLen, uiWriteBufferSize);	
						SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
					}
					if (uiRecvFileLen > tiStatus.Size) 
					{
						dbgprintf(2, "Error file length, %i(recv) > %i(want)\n",uiRecvFileLen, tiStatus.Size);	
						SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
					}
					
					if (resl)
					{
						
						//printf("WRITE data %i\n", uiFrameLen);	
						previous_ms_2 = 0;
						get_ms(&previous_ms_2);						
						wrtd = 0;
						do
						{	
							ret = fwrite(&pWriteBuffer[wrtd], 1, uiFrameLen - wrtd, tiStatus.filehandle);
							if (ret == 0) break;
							wrtd += ret;							
						} 
						while(uiFrameLen - wrtd);
						//printf("[WRITER] WRITE data Num:%i Len:%i Time:%i\n", uiCurrentFrame, uiFrameLen, (unsigned int)get_ms(&previous_ms_2));
						if (uiFrameLen == wrtd) 
						{
							uiData[0] = tiStatus.ID;
							uiData[1] = uiCurrentFrame;
							SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_DONE, (void*)uiData, sizeof(uiData[0])*2, &tiStatus.Address);
							resl = 1;
						} else resl = 0;
					} else SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
					
					if (resl == 0) 
					{
						fclose(tiStatus.filehandle);
						SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
						DBG_MUTEX_LOCK(&stream_mutex);
						tfTransferFileStatus.Status = 0;
						DBG_MUTEX_UNLOCK(&stream_mutex);
						tiStatus.Status = 0;
						dbgprintf(2, "Error writer, write data failed, cancel recv\n");
					}
					previous_ms = 0;
					get_ms(&previous_ms);					
				}
				else
				{
					//printf("[WRITER] TRANSFER DONE\n");
					if (tiStatus.Status)
					{
						if (uiRecvFileLen != tiStatus.Size) 
							dbgprintf(2, "Error file length, %i(recv) != %i(want)\n",uiRecvFileLen, tiStatus.Size);	
						fclose(tiStatus.filehandle);
						SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
						DBG_MUTEX_LOCK(&stream_mutex);
						tfTransferFileStatus.Status = 0;
						DBG_MUTEX_UNLOCK(&stream_mutex);
						tiStatus.Status = 0;
					}
				}
			}
		}
		else
		{
			if (tiStatus.Status)
			{
				if (get_ms(&previous_ms) > TIME_BEFORE_CLOSE_IDLE)
				{
					//printf("[WRITER] TIME_BEFORE_CLOSE_IDLE\n");
					fclose(tiStatus.filehandle);
					SendWriterMess(tiStatus.ConnectNum, TYPE_MESSAGE_COPY_FILE_REJECT, (void*)&tiStatus.ID, sizeof(tiStatus.ID), &tiStatus.Address);
					DBG_MUTEX_LOCK(&stream_mutex);
					tfTransferFileStatus.Status = 0;
					DBG_MUTEX_UNLOCK(&stream_mutex);
					tiStatus.Status = 0;
				}
			}
		}
	}
	
	if (iLoop) DBG_FREE(pWriteBuffer);
	DBG_FREE(cCurrPath);
	DBG_FREE(cMainPath);
	DBG_FREE(mBuff);
	
	DBG_MUTEX_LOCK(&system_mutex);
	tfTransferFileStatus.Enabled = 0;
	tfTransferFileStatus.Status = 0;
	cThreadWriterStatus = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_LOG_OUT();
	
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	
	return NULL;
}