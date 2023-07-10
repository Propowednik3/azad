#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include "gpio.h"
#include "main.h"
#include "omx_client.h"
#include "tfp625a.h"

#define BUFFER_SIZE	256

#define CMD_GetImg 			0x01	//Getting fingerprint images for verification
#define CMD_Img2Tz 			0x02	//Feature extraction from fingerprint image
#define CMD_Match 			0x03	//Comparing two fingerprint features
#define CMD_Search 			0x04	//Fingerprint identification and comparison within all or specified partially registered fingerprint feature libraries
#define CMD_RegModel 		0x05 	//Combining 2-3 fingerprint features into a fingerprint registration template
#define CMD_StoreModel 		0x06	//Store registration template in FLASH
#define CMD_LoadChar 		0x07	//Read a template from FLASH into the cache
#define CMD_UpChar 			0x08	//Upload the feature template of the buffer to the
			//host computer
#define CMD_DownChar		0x09	//Download a feature template from the host computer to the buffer
#define CMD_Uplmage			0x0A	//Upload the fingerprint image of the buffer to the host computer
#define CMD_DeleteChar		0x0C	//Delete a feature from FLASH
#define CMD_Empty			0x0D	//Clear FLASH Fingerprint Database
#define CMD_SetSysPara		0x0E	//Set Module Parameters
#define CMD_ReadSysPara		0x0F	//Read Module Parameters
#define CMD_SetPwd			0x12	//Set Module Password
#define CMD_VfyPwd			0x13	//Verify Module Password
#define CMD_SetAddr			0x15	//Set Module Address
#define CMD_ReadINFPage		0x16	//Read information page content
#define CMD_WriteNotePad	0x18	//Write a 32-byte Notepad
#define CMD_ReadNotePad		0x19	//Read a 32-byte Notepad
#define CMD_HISearch		0x1B	//Search and identify quickly
#define CMD_TemplateNum		0x1D	//Read the number of templates in the database
#define CMD_ReadConList		0x1F	//Read available tags of templates in the database
#define CMD_Cancel			0x30	//Cancel instruction
#define CMD_AutoEnroll		0x31	//Automatic fingerprint Enrollment
#define CMD_AutoIdentify	0x32	//Automatic fingerprint Indentification
#define CMD_GetMinEmptylD	0xA0	//Get the minimum empty ID

#define RET_OK				0x00	//success
#define RET_InvalidPacket	0x01	//Invalid Packet
#define RET_NoFinger		0x02	//Sensor did not detect finger
#define RET_StorelmageFail	0x03	//Failed to save image in Image Buffer
#define RET_TooLowQuality	0x06	//Image quality is too poor to extract features
#define RET_TooFewPoint		0x07	//Too few feature points to extract features
#define RET_NotMatched		0x08	//Inconsistent fingerprint template matching
#define RET_Notldentified	0x09	//No matching fingerprints
#define RET_MergeFail		0x0A	//Merge feature failure
#define RET_InvalidTempID	0x0B	//Invalid template ID
#define RET_ReadTempFail	0x0C	//Failed to read template from database
#define RET_UpTempFail		0x0D	//Failed to upload template
#define RET_ModBusyErr		0x0E	//The module is busy to receive the data
			//packet now
#define RET_UplmgFail		0x0F	//Failure to upload image
#define RET_RemoveTempFail	0x10	//Failed to delete template from database
#define RET_RemoveAIIFail	0x11	//Failed to delete all templates from the database
#define RET_InvalidPwd		0x13	//Invalid password
#define RET_Invalidlmg		0x15	//There is no valid image data in Image Buffer
#define RET_InvalidMAddr		0x20	//Illegal module address
#define RET_NeedVfyPwd		0x21	//The password needs to be verified


int SendPacket(MODULE_INFO *miModule, unsigned char uiPacketType, char* pData, int iDataLen, int iTimeout)
{
	if ((iDataLen <= 0) || (iDataLen > 230)) return 0;
	uart_clear_port(miModule->InitParams[0]);
	unsigned char pBuff[12];
	memset(pBuff, 0, 12);
	pBuff[0] = 0xEF;
	pBuff[1] = 0x01;
	pBuff[2] = 0xFF;
	pBuff[3] = 0xFF;
	pBuff[4] = 0xFF;
	pBuff[5] = 0xFF;
	pBuff[6] = uiPacketType;
	iDataLen += 2;
	pBuff[7] = (iDataLen >> 8) & 0xFF;
	pBuff[8] = iDataLen & 0xFF;
	iDataLen -= 2;
	//memcpy(&pBuff[9], pData, iDataLen);
	int CRC = 0;
	int i;
	for (i = 0; i < iDataLen; i++) CRC += pData[i];
	CRC += pBuff[6] + pBuff[7] + pBuff[8];
	CRC &= 0x0000FFFF;
	i = iDataLen + 9;
	pBuff[9] = (CRC >> 8) & 0xFF;
	i++;
	pBuff[10] = CRC & 0xFF;
	i++;
	int ret = write_t(miModule->InitParams[0], (char*)pBuff, 9, iTimeout);
	if (ret != 9) return -1;
	ret = write_t(miModule->InitParams[0], (char*)pData, iDataLen, iTimeout);
	if (ret != iDataLen) return -2;
	ret = write_t(miModule->InitParams[0], (char*)&pBuff[9], 2, iTimeout);
	if (ret != 2) return -3;
	
	return 1;	
}

int SendCmdPacket(MODULE_INFO *miModule, unsigned char uiPacketType, char pData, int iTimeout)
{
	return SendPacket(miModule, uiPacketType, &pData, 1, iTimeout);
}

int RecvPacket(MODULE_INFO *miModule, unsigned char *uiPacketType, char* pData, int *iDataLen, int iTimeout)
{
	unsigned char pBuff[12];
	memset(pBuff, 0, 12);
	char cCRC[2];
	int ret, i;
	int64_t iLocalTimer = 0;
	get_ms(&iLocalTimer);
	int iWaitTime = iTimeout;
	char cFirstRun = 1;
	int iMaxLen = *iDataLen;
	*iDataLen = 0;
	int iTotalRecvLen = 0;
	int result = -1;
	//ret = read_t(miModule->InitParams[0], (char*)pBuff, 256, iWaitTime);
	//printf("### %i\n", ret);
	//for(i=0;i<ret;i++) printf("%i) %02x %i\n", i, pBuff[i], pBuff[i]);
	//return 0;
	while(cFirstRun || ((unsigned int)get_ms(&iLocalTimer) <= iWaitTime))
	{
		iWaitTime = iTimeout - (unsigned int)get_ms(&iLocalTimer);
		if (iWaitTime < 0) {result = 0; break;}
		cFirstRun = 0;
		ret = read_t(miModule->InitParams[0], (char*)pBuff, 1, iWaitTime);
		if (ret != 1) break;
		if (pBuff[0] != 0xEF) continue;		
		ret = read_t(miModule->InitParams[0], (char*)pBuff, 1, iWaitTime);
		if (ret != 1) break;
		if (pBuff[0] != 0x01) continue;
		if (iWaitTime < 1000) iWaitTime = 1000;
		ret = read_t(miModule->InitParams[0], (char*)pBuff, 4, iWaitTime);
		if (ret != 4) break;
		if ((pBuff[0] != 0xFF) && (pBuff[1] != 0xFF) && (pBuff[2] != 0xFF) && (pBuff[3] != 0xFF)) continue;
		ret = read_t(miModule->InitParams[0], (char*)pBuff, 3, iWaitTime);
		if (ret != 3) break;
		int iCRC = pBuff[0] + pBuff[1] + pBuff[2];		
		*uiPacketType = pBuff[0];		
		int iRecvLen = (pBuff[1] << 8) | pBuff[2];
		iRecvLen -= 2;
		if ((iRecvLen + iTotalRecvLen) > iMaxLen) 
		{
			dbgprintf(2, "Error recv packet, buffer small, %i > %i\n", (iRecvLen + iTotalRecvLen), iMaxLen);
			result = -2; 
			break;
		}
		ret = read_t(miModule->InitParams[0], (char*)&pData[iTotalRecvLen], iRecvLen, iWaitTime);
		if (ret != iRecvLen) break;
		for (i = 0; i < iRecvLen; i++) iCRC += pData[iTotalRecvLen+i];		
		iTotalRecvLen += iRecvLen;
		int iRecvCRC = 0;		
		ret = read_t(miModule->InitParams[0], cCRC, 2, iWaitTime);
		if (ret != 2) break;
		iRecvCRC = (cCRC[0] << 8) | cCRC[1];
		if (iCRC != iRecvCRC) {result = -3; break;}
		
		*iDataLen = iTotalRecvLen;
		if ((*uiPacketType) != 0x02) { result = 1; break;}
	}
	//printf("#### %i\n", result);
	return result;	
}

int RecvData(MODULE_INFO *miModule, char* pData, int *iDataLen, int iTimeout)
{
	unsigned char uiPacketType;
	return RecvPacket(miModule, &uiPacketType, pData, iDataLen, iTimeout);
}

int SendRequest(MODULE_INFO *miModule, char cReq, char cResp, int iTimeout)
{
	int ret = SendCmdPacket(miModule, 0x01, cReq, iTimeout);
	
	if (ret <= 0) return 0;
	char cBuff[6];
	int iLen = 6;
	ret = RecvData(miModule, cBuff, &iLen, iTimeout);
	/*if (iLen != 1)
	{
		int i;
		for (i = 0; i < iLen; i++) printf("\t%02x", cBuff[i]);
		printf("\n");
	}*/
	if (cBuff[0] == cResp) return 1; else return -1;
	return 0;
}

int RequestData(MODULE_INFO *miModule, char cReq, char *cBuff, int iLen, int iTimeout)
{
	int ret = SendCmdPacket(miModule, 0x01, cReq, iTimeout);
	
	if (ret <= 0) return 0;
	int iNeedLen = iLen;
	ret = RecvData(miModule, cBuff, &iLen, iTimeout);
	if (iNeedLen != iLen) return -2;	
	
	return 1;
}

int RequestDataEx(MODULE_INFO *miModule, char *pSendData, int iSendLen, char *cRecvData, int iRecvLen, int iTimeout)
{
	int ret = SendPacket(miModule, 0x01, pSendData, iSendLen, iTimeout);
	if (ret <= 0) return ret;
	int iNeedLen = iRecvLen;
	ret = RecvData(miModule, cRecvData, &iRecvLen, iTimeout);
	if (ret <= 0) return ret;
	if (iNeedLen != iRecvLen) return -10;
	return 1;
}

int GetFingerTemplates(MODULE_INFO *miModule)
{
	int result = 1;
	char cBuff[3];
	int iTemplCnt = 0;
	miModule->IO_flag[2] = 0;
	int ret = RequestData(miModule, CMD_TemplateNum, cBuff, 3, 100);	
	if (ret > 0) 
	{
		if (cBuff[0] == RET_OK)
		{
			iTemplCnt = (cBuff[1] << 8) | cBuff[2];
			dbgprintf(3, "GetFingerTemplates: %i\n", iTemplCnt);
			//memset(miModule->IO_data, 0, sizeof(FINGER_INFO)*FINGER_INFO_SIZE);
			char* pData = DBG_MALLOC(128);
			char cBlock[33];
			int i, n;
			for (i = 0; i < 4; i++)
			{
				cBuff[0] = CMD_ReadConList;
				cBuff[1] = i;
				ret = RequestDataEx(miModule, cBuff, 2, cBlock, 33, 200);	
				if (ret <= 0) 
				{
					dbgprintf(2, "Error read tags finger templates %i\n", ret);
					result = -1;
					break;
				} 
				else 
				{
					if (cBlock[0] == RET_OK)
					{
						memcpy(&pData[i*32], &cBlock[1], 32);
						dbgprintf(3, "Readed finger template %i\n", i);
					}
					else
					{
						dbgprintf(2, "Error read tags finger templates, resp: %i\n", cBlock[0]);
						result = -2;
						//break;
					}
				}
			}
			
			FINGER_INFO	*fing = (FINGER_INFO*)miModule->IO_data;				
			if (result == 1)
			{
				miModule->IO_flag[2] = iTemplCnt;
				int iCnt = 0;
				for (i = 0; i < 128; i++) 
					for (n = 0; n < 8; n++)
					{
						fing[iCnt].Filled = pData[i] & (1 << n) ? 1 : 0;
						//printf("%i) %i\n", i, pData[i]);
						iCnt++;
					}				
			}
			else
			{
				miModule->IO_flag[2] = 0;
				for (i = 0; i < 1024; i++) fing[i].Filled = 0;
			}
			DBG_FREE(pData);
		}
	}
	return result;
}

void TFP625A_lock(MODULE_INFO *miModule, int iRequest)
{
	DBG_MUTEX_LOCK(miModule->IO_mutex);
	while (miModule->IO_flag[0] || (!iRequest && miModule->IO_flag[1]))
	{
		if (iRequest && (miModule->IO_flag[1] == 0)) miModule->IO_flag[1] = 1;
		DBG_MUTEX_UNLOCK(miModule->IO_mutex);
		usleep(50);
		DBG_MUTEX_LOCK(miModule->IO_mutex);
	}
	miModule->IO_flag[0] = 1;
	if (iRequest && miModule->IO_flag[1]) miModule->IO_flag[1] = 0;
	DBG_MUTEX_UNLOCK(miModule->IO_mutex);
}

void TFP625A_unlock(MODULE_INFO *miModule)
{
	DBG_MUTEX_LOCK(miModule->IO_mutex);
	miModule->IO_flag[0] = 0;
	DBG_MUTEX_UNLOCK(miModule->IO_mutex);
}
	
int TFP625A_save_finger_info(MODULE_INFO *miModule)
{	
	char cPath[64];
	memset(cPath, 0, 64);
	snprintf(cPath, 64, "Settings/finger_info_%.4s.bin", (char*)&miModule->ID);
	omx_dump_data(cPath, (char*)miModule->IO_data, miModule->IO_size_data[0]); 
	return 1;
}

int TFP625A_init(MODULE_INFO *miModule)
{
	if (miModule->InitParams[0] > 0) 
	{
		dbgprintf(2,"Error TFP625A_init '%.4s', repeat init\n", (char*)&miModule->ID);
		return -1;
	}
	miModule->InitParams[0] = uart_init_port(miModule->Settings[1], 57600);
	if (miModule->InitParams[0] <= 0) 
	{
		dbgprintf(2,"Error TFP625A_init '%.4s', not open uart\n", (char*)&miModule->ID);
		return 0;
	}
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
		gpio_switch_on_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}
	
	uart_clear_port(miModule->InitParams[0]);	
	
	miModule->IO_mutex = DBG_MALLOC(sizeof(pthread_mutex_t));
	miModule->IO_flag = DBG_MALLOC(sizeof(int)*3);		
	miModule->IO_data = DBG_MALLOC(sizeof(FINGER_INFO)*FINGER_INFO_SIZE);
	miModule->IO_size_data = DBG_MALLOC(sizeof(int));
	miModule->IO_size_data[0] = sizeof(FINGER_INFO)*FINGER_INFO_SIZE;
	miModule->IO_flag[0] = 0;
	miModule->IO_flag[1] = 0;
	miModule->IO_flag[2] = 0;
	
	char cPath[64];
	memset(cPath, 0, 64);
	snprintf(cPath, 64, "Settings/finger_info_%.4s.bin", (char*)&miModule->ID);
	
	if (!omx_load_file_size(cPath, (char*)miModule->IO_data, miModule->IO_size_data[0]))
	{
		memset(miModule->IO_data, 0, miModule->IO_size_data[0]);
		omx_dump_data(cPath, (char*)miModule->IO_data, miModule->IO_size_data[0]); 
	}		
	
	pthread_mutex_init(miModule->IO_mutex, NULL); 

	char *param_table = DBG_MALLOC(512);
	int iPacketLen = 512;
	int result = 0;
	int ret;
	
	ret = SendRequest(miModule, CMD_ReadINFPage, RET_OK, 100);
	if (ret > 0) ret = RecvData(miModule, param_table, &iPacketLen, 300);
	else dbgprintf(2,"Error: no recv data on CMD: CMD_ReadINFPage\n");
	if ((ret > 0) && (param_table[126] == 0x12) && (param_table[127] == 0x34)) result = 1;
	
	GetFingerTemplates(miModule);
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);
		gpio_switch_off_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}	
	
	DBG_FREE(param_table);
	if (result == 0) 
	{
		TFP625A_close(miModule);
		dbgprintf(2,"Error TFP625A_init '%.4s', not init\n", (char*)&miModule->ID);		
		return 0;
	}
	
	return 1;
}

int TFP625A_close(MODULE_INFO *miModule)
{
	if (!miModule->InitParams[0]) return 0;
	
	close(miModule->InitParams[0]);
	
	TFP625A_save_finger_info(miModule);
	
	pthread_mutex_destroy(miModule->IO_mutex);
	if (miModule->IO_mutex) DBG_FREE(miModule->IO_mutex);
	if (miModule->IO_flag) DBG_FREE(miModule->IO_flag);
	if (miModule->IO_data) DBG_FREE(miModule->IO_data);
	if (miModule->IO_size_data) DBG_FREE(miModule->IO_size_data);
	miModule->InitParams[0] = 0;
	return 1;
}

int TFP625A_UpdateInfo(MODULE_INFO *miModule)
{		
	TFP625A_lock(miModule, 1);
	int ret = GetFingerTemplates(miModule);
	TFP625A_unlock(miModule);
	return ret;
}

int TFP625A_CleanDatabase(MODULE_INFO *miModule)
{		
	TFP625A_lock(miModule, 1);
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
		gpio_switch_on_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}
		
	int ret = SendRequest(miModule, CMD_Empty, RET_OK, 100);
	if (ret > 0)
	{
		DBG_MUTEX_LOCK(miModule->IO_mutex);
		memset(miModule->IO_data, 0, miModule->IO_size_data[0]);
		TFP625A_save_finger_info(miModule);
		DBG_MUTEX_UNLOCK(miModule->IO_mutex);
		dbgprintf(4, "TFP625A_CleanDatabase: clean '%.4s' OK\n", (char*)&miModule->ID);
	}
	else dbgprintf(2, "TFP625A_CleanDatabase: error clean, %i\n", ret);
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);
		gpio_switch_off_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}
	
	TFP625A_unlock(miModule);
	
	return 1;
}

int TFP625A_DeleteTemplate(MODULE_INFO *miModule, int iTemplateID)
{	
	if ((iTemplateID < 0) || (iTemplateID >= 1024)) return 0;
	
	printf("DeleteTemplate in1\n");
	TFP625A_lock(miModule, 1);
	printf("DeleteTemplate in2\n");
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
		gpio_switch_on_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}
		
	int result = 0;
	char pData[32];
	pData[0] = CMD_DeleteChar;
	pData[1] = (iTemplateID >> 8) & 0xFF;
	pData[2] = iTemplateID & 0xFF;
	pData[3] = 0;
	pData[4] = 1;
	unsigned char uiPacketType = 0;
	printf("TFP625A_DeleteTemplate '%i'\n", iTemplateID);
	
	if (SendPacket(miModule, 0x01, pData, 5, 100))
	{
		int iDataLen = 2;
		int ret = RecvPacket(miModule, &uiPacketType, pData, &iDataLen, 1000);
		if (ret > 0)
		{			
			if ((uiPacketType == 0x07) && (pData[0] == 00)) 
			{
				result = 1;
				DBG_MUTEX_LOCK(miModule->IO_mutex);
				FINGER_INFO *fing = (FINGER_INFO*)miModule->IO_data;
				memset(&fing[iTemplateID], 0, sizeof(FINGER_INFO));
				TFP625A_save_finger_info(miModule);
				DBG_MUTEX_UNLOCK(miModule->IO_mutex);
			}
		}
		printf("%i %i %i %i\n", uiPacketType, pData[0], pData[1], ret);
	}
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);
		gpio_switch_off_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}	
	
	TFP625A_unlock(miModule);
	printf("TFP625A_DeleteTemplate r '%i'\n", result);	
	return result;
}

int TFP625A_ChangeInfo(MODULE_INFO *miModule, int iTempNum, int iTempID, char *sTempInfo)
{
	if ((iTempNum < 0) || (iTempNum >= 1024)) return 0;
	
	DBG_MUTEX_LOCK(miModule->IO_mutex);
	
	FINGER_INFO *fing = (FINGER_INFO*)miModule->IO_data;
	fing[iTempNum].Filled = 1;
	fing[iTempNum].ID = iTempID;
	memset(fing[iTempNum].Info, 0, 32);
	if (strlen(sTempInfo) >= 32) 
		memcpy(fing[iTempNum].Info, sTempInfo, 31);
		else
		memcpy(fing[iTempNum].Info, sTempInfo, strlen(sTempInfo));
	TFP625A_save_finger_info(miModule);
		
	DBG_MUTEX_UNLOCK(miModule->IO_mutex);
		
	return 1;
}

int TFP625A_AutoEnroll(MODULE_INFO *miModule, int iTempNum, int iTempID, char *sTempInfo)
{
	if ((iTempNum < -1) || (iTempNum >= 1024)) return 0;
	
	TFP625A_lock(miModule, 1);
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
		gpio_switch_on_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}
	
	FINGER_INFO *fing = (FINGER_INFO*)miModule->IO_data;
	if (iTempNum == -1)
	{
		int i;
		for (i = 0; i < 1024; i++) 
			if (fing[i].Filled == 0)
			{
				iTempNum = i;
				break;
			}
	}
	
	if (iTempNum == -1)
	{
		dbgprintf(2, "Sensor full, break;");
		return 0;
	}
		
	int result = 0;
	char pData[32];
	pData[0] = CMD_AutoEnroll;
	pData[1] = (iTempNum >> 8) & 0xFF;
	pData[2] = iTempNum & 0xFF;
	pData[3] = 3;
	pData[4] = 0;
	pData[5] = 8;
	unsigned char uiPacketType;
	if (SendPacket(miModule, 0x01, pData, 6, 100))
	{
		int iDataLen = 32;
		while(RecvPacket(miModule, &uiPacketType, pData, &iDataLen, 5000) > 0)
		{
			if ((pData[0] == 0) && (pData[1] == 0x06) && (pData[2] == 0xF2))
			{
				result = 1;
				break;
			}
			/*int i;
			printf("%i) t:%i ", uiPacketType, iDataLen);
			for (i = 0; i < iDataLen; i++) printf("%02x ", pData[i]);
			printf("\n");
			iDataLen = 32;
			usleep(1000);*/
		}		
	}
	
	if (result)
	{
		fing[iTempNum].Filled = 1;
		fing[iTempNum].ID = iTempID;
		memset(fing[iTempNum].Info, 0, 32);
		if (strlen(sTempInfo) >= 32) 
			memcpy(fing[iTempNum].Info, sTempInfo, 31);
			else
			memcpy(fing[iTempNum].Info, sTempInfo, strlen(sTempInfo));
		TFP625A_save_finger_info(miModule);
	}
			
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);
		gpio_switch_off_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}	
	
	TFP625A_unlock(miModule);
	
	return result;
}

int TFP625A_AutoIdentify(MODULE_INFO *miModule)
{
	TFP625A_lock(miModule, 0);
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
		gpio_switch_on_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}
	
	//printf("TFP625A_AutoIdentify\n");
	char pData[32];
	pData[0] = CMD_AutoIdentify;
	pData[1] = 3;
	pData[2] = 0xFF;
	pData[3] = 0xFF;
	pData[4] = 0;
	pData[5] = 0;
	int iTemplateID = 0;
	int result = -2;
	int status_flag = 0;
	unsigned char uiPacketType;
	if (SendPacket(miModule, 0x01, pData, 6, 100))
	{
		int64_t iLocalTimer = 0;
		get_ms(&iLocalTimer);
		int iDataLen = 32;
		while((unsigned int)get_ms(&iLocalTimer) < 5400)
		{
			//int ret = RecvPacket(miModule, &uiPacketType, pData, &iDataLen, 500);
			if (RecvPacket(miModule, &uiPacketType, pData, &iDataLen, 5500) > 0)
			{
				switch(pData[0])
				{
					case 0x00: //Sucsess
						if (pData[1] == 0x05) //detected
						{
							iTemplateID = (pData[2] << 8) | pData[3];
							result = 1;
						}
						break;
					case 0x01: //substep				
						break;
					case 0x26: //timeout
						if (pData[1] == 0x01) result = 0;
						break;
					case 0x09: //not found
					case 0x24: //database empty
						if (pData[1] == 0x05) result = 3;
						miModule->Status[0] = -1;
						break;
					default:
						result = -1;
						dbgprintf(2, "Error response finger sensor: %02x, %02x\n", pData[0], pData[1]);
						break;
				}
				if (result != -2) break;
			}
			
			iDataLen = 32;
		}		
	}
	//printf("iTemplateID:%i\n", iTemplateID);
	if (result == 0)//not detected
	{
		miModule->Status[0] = 0;
		miModule->Status[1] = 0;
		miModule->Status[2] = 0;
		status_flag = 0;
	}
	if (result == 1)//detected
	{
		if ((iTemplateID >= 0) && (iTemplateID < 1024))
		{
			FINGER_INFO *fing = (FINGER_INFO*)miModule->IO_data;
			miModule->Status[0] = 1;
			miModule->Status[1] = iTemplateID;
			DBG_MUTEX_LOCK(miModule->IO_mutex);
			miModule->Status[2] = fing[iTemplateID].ID;
			DBG_MUTEX_UNLOCK(miModule->IO_mutex);
			status_flag = 0b111;			
		}
		else dbgprintf(2, "TFP625A_AutoIdentify: wrong template id:%i\n", iTemplateID);
	}
	if (result == 3)//not found
	{
		miModule->Status[0] = 2;
		miModule->Status[1] = 0;
		miModule->Status[2] = *(int*)&"#Unk";
		status_flag = 0b101;	
	}
	if (result < 0)//error
	{
		miModule->Status[0] = -1;
		miModule->Status[1] = 0;
		miModule->Status[2] = *(int*)&"#Err";
		status_flag = 0b101;	
	}
	
	if ((miModule->Settings[0] != 0) && (miModule->SubModule >= 0))
	{
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);
		gpio_switch_off_module(&miModule[miModule->SubModule]);
		if (miModule->Settings[4]) usleep(miModule->Settings[4]);						
	}	
	
	DBG_MUTEX_LOCK(miModule->IO_mutex);
	miModule->IO_flag[0] = 0;
	DBG_MUTEX_UNLOCK(miModule->IO_mutex);
	return status_flag;
}
