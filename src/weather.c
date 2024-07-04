// LoadFromWeb.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "debug.h"
#include "system.h"

#include "weather.h"
//#include "WinSock2.h"

//#pragma  comment (lib, "ws2_32")

#define SOCKET_ERROR 	-1
#define SD_RECEIVE 		0

int WSearchStrInData(char *Data, int DataLen, int Pos, char *Str)
{
	int StrLen = strlen(Str);
	int Flag1 = 0;
	int n;
	for (n = Pos; n != DataLen; n++)
	{
		if (Str[Flag1] == Data[n]) Flag1++; 
		else
		{
			Flag1 = 0;
			if (Str[Flag1] == Data[n]) Flag1++;
		}
		if (Flag1 == StrLen) return ((n - StrLen) + 1);
	}
	return 0;
}

int SearchStrInDataToStr(char *Data, int DataLen, int Pos, int *NewPos, char *Str, char *BeforeStr)
{
	int StrLen1 = strlen(Str);
	int StrLen2 = strlen(BeforeStr);
	int Flag1 = 0;
	int Flag2 = 0;
	int n;	
	for (n = Pos; n != DataLen; n++)
	{
		if (Str[Flag1] == Data[n]) Flag1++; 
		else
		{
			Flag1 = 0;
			if (Str[Flag1] == Data[n]) Flag1++;
		}
		if (BeforeStr[Flag2] == Data[n]) Flag2++; 
		else 
		{
			Flag2 = 0;
			if (BeforeStr[Flag2] == Data[n]) Flag2++;
		}
		if (Flag1 == StrLen1) {*NewPos = ((n - StrLen1) + 1);return 1;}
		if (Flag2 == StrLen2) return 0;
	}
	return 0;
}

int Char2Hex(char *cOutString, unsigned char *cInString, unsigned int cLen, char cDirect)
{
	int n, ret, cnt;
	char Template[20];
	memset(Template, 0, 20);
	strcpy(Template, "0123456789ABCDEF");
	if (cLen == 0) return 0;
	ret = 0;
	memset(cOutString, 0, (cLen * 2)+1);
	if (cDirect == 0) cnt = 0; else cnt = cLen - 1;
	for(n = 0; n != cLen; n++)
	{
		cOutString[ret] = Template[cInString[cnt] >> 4];
		ret++;
		cOutString[ret] = Template[cInString[cnt] & 15];		
		ret++;	
		if (cDirect == 0) cnt++; else cnt--;
	}
	return ret;
}

int Str2Int(char *cString)
{
	int n,i;
	int ret;
	char cStr[32];
	if (strlen(cString) > 31) return 0;
	memset(cStr, 0, 32);
	strcpy(cStr,cString);
	ret = 0;
	i = 1;
	for(n = 0; n != (int)strlen(cString); n++)
	{
		if ((cStr[n] > 47) && (cStr[n] < 58))
		{
			ret *= 10;
			cStr[n] -= 48;			
			ret += cStr[n];	
		}
		if (cStr[n] == 45) i *= -1;
	}
	return ret*i;
}

int Str2IntLimit(char *cString, unsigned int cLen)
{
	int i;
	unsigned int n;
	int ret;
	char cStr[32];
	if (cLen > 31) return 0;
	if (cLen == 0) return 0;
	memset(cStr, 0, 32);
	memcpy(cStr,cString, cLen);
	ret = 0;
	i = 1;
	for(n = 0; n < cLen; n++)
	{
		if ((cStr[n] > 47) && (cStr[n] < 58))
		{
			ret *= 10;
			cStr[n] -= 48;			
			ret += cStr[n];	
		}
		if (cStr[n] == 45) i *= -1;
	}
	return ret*i;
}

float Str2FloatLimit(char *cString, unsigned int cLen)
{
	int i,m,v;
	unsigned int n;
	float ret, res;
	char cStr[32];
	if (cLen > 31) return - 2;
	if (cLen == 0) return 0;
	memset(cStr, 0, 32);
	memcpy(cStr,cString, cLen);
	i = 0;
	m = 0;
	ret = 0;
	res = 0;
	v = 1;
	for(n = 0; n < cLen; n++)
	{
		if ((cStr[n] > 47) && (cStr[n] < 58))
		{
			ret *= 10;
			if (i != 0) m++;
			cStr[n] -= 48;
			ret += cStr[n];
		}
		else
		{
			if (cStr[n] == 45) v *= -1;
			if ((cStr[n] == 44) || (cStr[n] == 46))
			{
				i++;			
				res = ret;
				ret = 0;
				m = 0;
			}
		}
	}	
	if (i != 0)
	{
		while (m != 0)
		{
			ret /= 10;
			m--;
		}
		res += ret;
	}
	else res = ret;
	return res*v;
}

float Str2Float(char *cString)
{
	int i,n,m,v;
	float ret, res;
	char cStr[32];
	if (strlen(cString) > 31) return - 2;
	memset(cStr, 0, 32);
	strcpy(cStr,cString);
	i = 0;
	m = 0;
	ret = 0;
	res = 0;
	v = 1;
	for(n = 0; n != (int)strlen(cString); n++)
	{
		if ((cStr[n] > 47) && (cStr[n] < 58))
		{
			ret *= 10;
			if (i != 0) m++;
			cStr[n] -= 48;
			ret += cStr[n];
		}
		else
		{
			if (cStr[n] == 45) v *= -1;
			if ((cStr[n] == 44) || (cStr[n] == 46))
			{
				i++;			
				res = ret;
				ret = 0;
				m = 0;
			}
		}
	}	
	if (i != 0)
	{
		while (m != 0)
		{
			ret /= 10;
			m--;
		}
		res += ret;
	}
	else res = ret;
	return res*v;
}

int GetValueFromXml(char *cXmlData, int iXmlSize, char *Table, char *Field, char *Value, char *Res, int ResSize, int iStartPos)
{
	int iNewPos = 0;
	int iNewPos2 = 0;
	char SrchStr[2] = {34,0};
	char cBuff[64];
	char cBuff2[64];
	memset(cBuff,0,64);
	strcpy(cBuff,"<");
	strcat(cBuff,Table);
	strcat(cBuff," ");
	if (iStartPos == 0) iStartPos = WSearchStrInData(cXmlData, iXmlSize, iStartPos, cBuff);
	if (iStartPos != 0)
	{
		memset(cBuff,0,64);
		strcpy(cBuff,"</");
		strcat(cBuff,Table);
		strcat(cBuff,">");
		memset(cBuff2,0,64);
		strcpy(cBuff2,"<");
		strcat(cBuff2,Field);
		strcat(cBuff2," ");
		if (SearchStrInDataToStr(cXmlData, iXmlSize, iStartPos, &iNewPos, cBuff2, cBuff) == 1)
		{
			//iNewPos++;
			memset(cBuff2,0,64);
			strcpy(cBuff2," ");
			strcat(cBuff2,Value);
			strcat(cBuff2,"=");	
			if (SearchStrInDataToStr(cXmlData, iXmlSize, iNewPos, &iNewPos, cBuff2, ">") == 1)
			{
				iNewPos+= strlen(cBuff2);
				if (SearchStrInDataToStr(cXmlData, iXmlSize, iNewPos, &iNewPos, SrchStr, ">") == 1)
				{
					iNewPos++;
					if (SearchStrInDataToStr(cXmlData, iXmlSize, iNewPos, &iNewPos2, SrchStr, ">") == 1)
					{						
						iNewPos2 -= iNewPos;
						if (ResSize > iNewPos2)
						{
							memset(Res, 0, ResSize);
							memcpy(Res, cXmlData + iNewPos, iNewPos2);
							return 1;
						}
					}
				}			
			}				
		}
	}
	return 0;
}

int ConvertXml(char *cXmlData, int iXmlSize, WeatherStruct**wConvertData, WeatherRise *RiseData)
{
	WeatherStruct *wData;
	*wConvertData = NULL;
	int iCnt;
	//char SrchStr[2] = {34,0};
	char cBuff[64], cBuff2[64];
	//int iTimePos = 0;
	iCnt = 0;
	int i = 0;
	i = WSearchStrInData(cXmlData, iXmlSize, 0, "<sun ");
	if (i != 0)
	{
		if (GetValueFromXml(cXmlData, iXmlSize, "sun", "sun", "rise", cBuff, 64, i) == 1)
		{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff,10);
				RiseData->FromDate = (int)Str2Int(cBuff2);				
		}
		if (GetValueFromXml(cXmlData, iXmlSize, "sun", "sun", "rise", cBuff, 64, i) == 1)
		{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff+11,8);
				RiseData->FromTime = (int)Str2Int(cBuff2);				
		}
		if (GetValueFromXml(cXmlData, iXmlSize, "sun", "sun", "set", cBuff, 64, i) == 1)
		{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff,10);
				RiseData->ToDate = (int)Str2Int(cBuff2);				
		}
		if (GetValueFromXml(cXmlData, iXmlSize, "sun", "sun", "set", cBuff, 64, i) == 1)
		{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff+11,8);
				RiseData->ToTime = (int)Str2Int(cBuff2);				
		}
	}
	i = WSearchStrInData(cXmlData, iXmlSize, 0, "<forecast>");
	while (i != 0)
	{
		i = WSearchStrInData(cXmlData, iXmlSize, i, "<time ");
		if (i != 0)
		{
			iCnt++;
			*wConvertData = (WeatherStruct*)DBG_REALLOC(*wConvertData, iCnt*sizeof(WeatherStruct));
			wData = *wConvertData;
			iCnt--;
			memset(&wData[iCnt], 0, sizeof(WeatherStruct));
			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "time", "from", cBuff, 64, i) == 1)
			{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff,10);
				wData[iCnt].FromDate = (int)Str2Int(cBuff2);				
			}
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "time", "from", cBuff, 64, i) == 1)
			{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff+11,8);
				wData[iCnt].FromTime = (int)Str2Int(cBuff2);				
			}
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "time", "to", cBuff, 64, i) == 1)
			{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff,10);
				wData[iCnt].ToDate = (int)Str2Int(cBuff2);				
			}
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "time", "to", cBuff, 64, i) == 1)
			{
				memset(cBuff2, 0, 64);
				memcpy(cBuff2, cBuff+11,8);
				wData[iCnt].ToTime = (int)Str2Int(cBuff2);				
			}
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "symbol", "var", cBuff, 64, i) == 1)
				wData[iCnt].Symbol = Hex2Int(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "symbol", "name", cBuff, 64, i) == 1)
				if (strlen(cBuff) < 32) strcpy(wData[iCnt].SymbolName,cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "precipitation", "value", cBuff, 64, i) == 1)
				wData[iCnt].PrecipitationValue = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "precipitation", "type", cBuff, 64, i) == 1)
				if (strlen(cBuff) < 32) strcpy(wData[iCnt].PrecipitationType,cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "windDirection", "deg", cBuff, 64, i) == 1)
				wData[iCnt].WindDirection = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "windSpeed", "mps", cBuff, 64, i) == 1)
				wData[iCnt].WindSpeed = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "value", cBuff, 64, i) == 1)
				wData[iCnt].Temperature = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "max", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureMax = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "min", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureMin = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "pressure", "value", cBuff, 64, i) == 1)
				wData[iCnt].Pressure = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "humidity", "value", cBuff, 64, i) == 1)
				wData[iCnt].Humidity = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "clouds", "all", cBuff, 64, i) == 1)
				wData[iCnt].Clouds = Str2Float(cBuff);			
			iCnt++;
			i++;
		}
	}
	return iCnt;
}

int ConvertDayXml(char *cXmlData, int iXmlSize, WeatherDayStruct**wConvertData)
{
	WeatherDayStruct *wData;
	*wConvertData = NULL;
	int iCnt;
	//char SrchStr[2] = {34,0};
	char cBuff[64];// cBuff2[64];
	//int iTimePos = 0;
	iCnt = 0;
	int i = WSearchStrInData(cXmlData, iXmlSize, 0, "<forecast>");
	while (i != 0)
	{
		i = WSearchStrInData(cXmlData, iXmlSize, i, "<time ");
		if (i != 0)
		{
			iCnt++;
			*wConvertData = (WeatherDayStruct*)DBG_REALLOC(*wConvertData, iCnt*sizeof(WeatherDayStruct));
			wData = *wConvertData;
			iCnt--;
			memset(&wData[iCnt], 0, sizeof(WeatherDayStruct));
			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "time", "day", cBuff, 64, i) == 1)
				wData[iCnt].Date = (int)Str2Int(cBuff);	
			//printf("%s  %i\n",cBuff, wData[iCnt].Date);
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "symbol", "var", cBuff, 64, i) == 1)
				wData[iCnt].Symbol = Hex2Int(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "symbol", "name", cBuff, 64, i) == 1)
				if (strlen(cBuff) < 32) strcpy(wData[iCnt].SymbolName,cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "precipitation", "value", cBuff, 64, i) == 1)
				wData[iCnt].PrecipitationValue = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "precipitation", "type", cBuff, 64, i) == 1)
				if (strlen(cBuff) < 32) strcpy(wData[iCnt].PrecipitationType,cBuff);		
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "windDirection", "deg", cBuff, 64, i) == 1)
				wData[iCnt].WindDirection = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "windSpeed", "mps", cBuff, 64, i) == 1)
				wData[iCnt].WindSpeed = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "day", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureDay = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "max", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureMax = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "min", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureMin = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "eve", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureEve = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "morn", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureMorn = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "temperature", "night", cBuff, 64, i) == 1)
				wData[iCnt].TemperatureNight = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "pressure", "value", cBuff, 64, i) == 1)
				wData[iCnt].Pressure = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "humidity", "value", cBuff, 64, i) == 1)
				wData[iCnt].Humidity = Str2Float(cBuff);			
			if (GetValueFromXml(cXmlData, iXmlSize, "time", "clouds", "all", cBuff, 64, i) == 1)
				wData[iCnt].Clouds = Str2Float(cBuff);			
			iCnt++;
			i++;
		}
	}
	return iCnt;
}

void DumpData(char *Buff, int Len, char *Lable)
{
	FILE *f;
	if ((f = fopen(Lable,"wb+")) == NULL)
	{
		printf("Ошибка создания файла дампа");
		return;
	}
	
	fwrite(Buff, 1, Len, f);
	fclose(f);
	return;	
}

int DownloadXml(char *cServer, char *cPath, char **cBuffer)
{
	char cHeader[2048];
	char *cHeaderRecv, *cHeaderRecvClk;
	char SrchStr[4] = {13,10,13,0};
	struct sockaddr_in m_sin;
	struct sockaddr *m_s;
	m_s = (void*)&m_sin;
	int	v_socket;
	//WSADATA WsaDat;
	int ret;
	struct hostent *remoteHost;
	struct in_addr addr;

	//ret = WSAStartup(MAKEWORD(2,0), &WsaDat);
	
	
	//printf("\tIPv4 Address #%d: %s\n", 0, cServer);
	

	remoteHost = gethostbyname(cServer);
	if (remoteHost == NULL) 
	{
		dbgprintf(2,"gethostbyname(%s)\n", cServer);
		return 0;//WSAGetLastError();
	}

	int i = 0;
	int iDataSize = 0;
	int iFileSize = 0;
	int iNeedSize;
	int m;
	//u_long *ulTmp = (u_long *) (remoteHost->h_addr_list[0]);
	//addr.s_addr = *ulTmp;
	memcpy(&addr.s_addr, remoteHost->h_addr_list[0], sizeof(addr.s_addr));
    //printf("\tIPv4 Address #%d: %s\n",0, inet_ntoa(addr));

	memset(cHeader, 0, 2048);
	//GET
	strcpy(cHeader, "GET ");
	strcat(cHeader, cPath);
	strcat(cHeader, " HTTP/1.1\r\n");
	//HOST
	strcat(cHeader, "Host: ");
	strcat(cHeader, cServer);
	strcat(cHeader, "\r\n");
	//CONNECTION
	strcat(cHeader, "Connection: keep-alive\r\n");
	//ACCEPT
	strcat(cHeader, "Accept: */*\r\n");
	//USER-AGENT
	strcat(cHeader, "User-Agent: Mozilla/5.0\r\n");
	//Accept-Encoding
	strcat(cHeader, "Accept-Encoding: gzip,deflate,sdch\r\n");
	//Accept-Language
	strcat(cHeader, "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n");
	strcat(cHeader, "\r\n");
	//printf(cHeader);

	m_sin.sin_family = AF_INET;
	m_sin.sin_port = htons(80);
	m_sin.sin_addr = addr;
	
	v_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if ((v_socket == SOCKET_ERROR)) return 0;//WSAGetLastError();
	dbgprintf(4,"connect\n");
	ret = connect(v_socket, m_s, sizeof(m_sin));
	if ((ret == SOCKET_ERROR)) return 0;//WSAGetLastError();
	dbgprintf(4,"send %i\n", strlen(cHeader));
	ret = send(v_socket, cHeader, strlen(cHeader)+1, 0);
	if ((ret == SOCKET_ERROR)) return 0;//WSAGetLastError();
	cHeaderRecv = (char*)DBG_MALLOC(32768);
	memset(cHeaderRecv,0,32768);
	iNeedSize = 32768;
	cHeaderRecvClk = cHeaderRecv;
	i = 0;
	while(iDataSize < iNeedSize)
	{
		dbgprintf(4,"recv\n");
		ret = recv(v_socket, cHeaderRecvClk, 2000, 0);
		if (ret == SOCKET_ERROR)
		{
			//if (WSAGetLastError() != 10035) return WSAGetLastError();  else sleep(1);
		}
		else
		{
			//printf("\n%s\n",cHeaderRecvClk);
			iDataSize += ret;
			cHeaderRecvClk += ret;
			//printf("Size :%i\n", iDataSize);
			if ((iFileSize == 0) && (iDataSize >=300))
			{
				i = WSearchStrInData(cHeaderRecv, iDataSize, i, SrchStr);				
				if (i == 0) break;
				i += 4;
				SrchStr[2] = 0;
				SrchStr[3] = 0;
				//printf("i :%i\n", i);
				m = WSearchStrInData(cHeaderRecv, iDataSize, i, SrchStr);
				//printf("m :%i\n", m);
				memset(cHeader, 0, 2048);
				if ((m-i) < 10) 
				{
					memcpy(cHeader, cHeaderRecv + i, m - i);
					iFileSize = Hex2Int(cHeader);
					iNeedSize = iFileSize + m+1;
				}
			}
		}
	}
	if (iFileSize != 0)
	{
		*cBuffer = (char*)DBG_MALLOC(iFileSize);
		memcpy(*cBuffer, cHeaderRecv + m + 2, iFileSize);
	}
	DBG_FREE(cHeaderRecv);
	
	shutdown(v_socket, SD_RECEIVE);
	close(v_socket);
	//WSACleanup(); 
    
	return iFileSize;
}

int DownloadFileNB(char *cServer, char *cPath, char **cBuffer)
{
	DBG_LOG_IN();
	char cHeader[2048];
	char *cBufferRecv, *cBufferRecvClk;
	char SrchDblEnter[4] = {13,10,13,0};
	char SrchSnglEnter[4] = {13,10,0,0};
	struct sockaddr_in m_sin;
	struct sockaddr *m_s;
	m_s = (void*)&m_sin;
	int	v_socket;
	int ret;
	struct hostent *remoteHost;
	struct in_addr addr;

	remoteHost = gethostbyname(cServer);
	if (remoteHost == NULL) 
	{
		dbgprintf(1,"DownloadFileNB: error gethostbyname\n");	
		DBG_LOG_OUT();
		return 0;
	}
	memcpy(&addr.s_addr, remoteHost->h_addr_list[0], sizeof(addr.s_addr));
    //addr.s_addr = inet_addr("172.16.2.81");
	
	int i = 0;
	int iDataSize = 0;
	int iFileSize = 0;
	int iNeedSize;
	int m;	
	
	memset(cHeader, 0, 2048);
	strcpy(cHeader, "GET ");
	strcat(cHeader, cPath);
	strcat(cHeader, " HTTP/1.1\r\n");
	strcat(cHeader, "Host: ");
	strcat(cHeader, cServer);
	strcat(cHeader, "\r\n");
	strcat(cHeader, "Connection: keep-alive\r\n");
	strcat(cHeader, "Accept: */*\r\n");
	strcat(cHeader, "User-Agent: Mozilla/5.0\r\n");
	strcat(cHeader, "Accept-Encoding: gzip,deflate,sdch\r\n");
	strcat(cHeader, "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n");
	strcat(cHeader, "\r\n");
	//printf("Request: '%s'",cHeader);
	m_sin.sin_family = AF_INET;
	m_sin.sin_port = htons(80);
	m_sin.sin_addr = addr;
	
	v_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if ((v_socket == SOCKET_ERROR)) 
	{		
		dbgprintf(1,"DownloadFileNB: error socket\n");
		DBG_LOG_OUT();
		return 0;
	}
	int flags = fcntl(v_socket, F_GETFL, 0);
	if (fcntl(v_socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(1,"DownloadFileNB: error set nonblock\n");
		DBG_LOG_OUT();
		return 0;
	}
	fd_set rfds, wfds;
	struct timeval tv;
	
	int max_fd = -1;
	socklen_t err_len;
	int error;
	
	if (v_socket > max_fd) max_fd = v_socket;	
	
	//printf("connect\n");
	ret = connect(v_socket, m_s, sizeof(m_sin));
	if (ret != 0) 
	{
		if (errno == EINPROGRESS) 
		{
			//printf("connecting\n");
			FD_ZERO(&wfds);
			FD_ZERO(&rfds);
			FD_SET(v_socket, &wfds);
			FD_SET(v_socket, &rfds);
			tv.tv_sec = 10;
			tv.tv_usec = 0;
	
			error = select(max_fd + 1, &rfds, &wfds, NULL, &tv);
			if (error > 0)
			{
				if (FD_ISSET(v_socket, &wfds) || FD_ISSET(v_socket, &rfds))
				{
					err_len = sizeof(error);
					if ((getsockopt(v_socket, SOL_SOCKET, SO_ERROR, &error, &err_len) < 0) || (error != 0))
					{
						dbgprintf(1,"DownloadFileNB: error connect2 (errno:%i, %s)\n", errno, strerror(errno));
						shutdown(v_socket, SD_RECEIVE);
						close(v_socket);
						DBG_LOG_OUT();
						return 0;
					}
				}
			}
			else
			{
				if (error < 0) dbgprintf(1,"DownloadFileNB: error connect3 (errno:%i, %s)\n", errno, strerror(errno));
				close(v_socket);
				DBG_LOG_OUT();				
				return 0;
			}
			//printf("connected\n");
		}
		else		
		{
			dbgprintf(1,"DownloadFileNB: error connect (errno:%i, %s)\n", errno, strerror(errno));
			DBG_LOG_OUT();
			return 0;
		}				
	}// else printf("connected\n");
	
	int iSended = 0;
	int iNeedSend = strlen(cHeader);
	
	while(iSended < iNeedSend)
	{
		ret = send(v_socket, &cHeader[iSended], iNeedSend - iSended, 0);
		if (ret <= 0) 
		{
			if ((ret == 0) || (errno != EAGAIN))
			{
				if (ret == 0)dbgprintf(2,"DownloadFileNB: error send2 (closed)\n");
					else dbgprintf(2,"DownloadFileNB: error send2 (errno:%i, %s)\n", errno, strerror(errno));
				shutdown(v_socket, SD_RECEIVE);
				close(v_socket);
				DBG_LOG_OUT();
				return 0;
			}
			FD_ZERO(&wfds);
			FD_SET(v_socket, &wfds);
			tv.tv_sec = 10;
			tv.tv_usec = 0;
	
			error = select(max_fd + 1, NULL, &wfds, NULL, &tv);
			if (error == 0)
			{
				dbgprintf(2,"DownloadFileNB: error send (timout)\n");
				shutdown(v_socket, SD_RECEIVE);
				close(v_socket);
				DBG_LOG_OUT();				
				return 0;
			}
		} else iSended += ret;
	}
		
		//printf("send %i\n", strlen(cHeader));
	
	
	int iBufferSize = 65536;
	iNeedSize = iBufferSize;
	cBufferRecv = (char*)DBG_MALLOC(iBufferSize);
	memset(cBufferRecv,0,iBufferSize);
	cBufferRecvClk = cBufferRecv;
	i = 0;
	while(iDataSize < iNeedSize)
	{		
		ret = recv(v_socket, cBufferRecvClk, 16384, 0);
		if (ret <= 0)
		{
			if ((ret == 0) || (errno != EAGAIN))
			{
				if (ret == 0) dbgprintf(2,"DownloadFileNB: error recv (closed)\n");
					else dbgprintf(2,"DownloadFileNB: error recv (errno:%i, %s)\n", errno, strerror(errno));
				DBG_FREE(cBufferRecv);	
				shutdown(v_socket, SD_RECEIVE);
				close(v_socket);
				DBG_LOG_OUT();
				return 0;
			}
			FD_ZERO(&rfds);
			FD_SET(v_socket, &rfds);
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			error = select(max_fd + 1, &rfds, NULL, NULL, &tv);
			if (error == 0)
			{
				dbgprintf(2,"DownloadFileNB: error recv (timeout)\n");
				DBG_FREE(cBufferRecv);	
				shutdown(v_socket, SD_RECEIVE);
				close(v_socket);
				DBG_LOG_OUT();
				return 0;
			}			
		}
		else
		{
			//printf("recv :%i(Total:%i >> %i)\n", ret, iDataSize , iNeedSize);
			iDataSize += ret;
			cBufferRecvClk += ret;
			
			if ((iFileSize == 0) && (iDataSize >=300))
			{	
				//printf("BODY: '%s'\n", cBufferRecv);
				i = WSearchStrInData(cBufferRecv, iDataSize, i, "Content-Length:");				
				if (i != 0)
				{
					i += strlen("Content-Length:\0");
					m = WSearchStrInData(cBufferRecv, iDataSize, i, SrchSnglEnter);
					if ((m == 0) || ((m-i)>25)) 
					{
						dbgprintf(2,"DownloadFileNB: error find SglEnter1\n");			
						break;
					}
					iFileSize = Str2IntLimit(&cBufferRecv[i], m-i);
				}
				
				m = WSearchStrInData(cBufferRecv, iDataSize, 0, SrchDblEnter);				
				if (m == 0) 
				{
					dbgprintf(2,"DownloadFileNB: error find DblEnter\n");			
					break;
				}
				i = m + 4;					
				if (iFileSize == 0)
				{
					m = WSearchStrInData(cBufferRecv, iDataSize, i, SrchSnglEnter);
					if ((m == 0) || ((m-i)>25)) 
					{
						dbgprintf(2,"DownloadFileNB: error find SglEnter2\n");			
						break;
					}
					iFileSize = Hex2IntLimit(&cBufferRecv[i], m-i);
					i = m + 2;
				}
				//printf("iFileSize %i\n", iFileSize);
				iNeedSize = iFileSize + i;
			}
		}
		if ((iDataSize < iNeedSize) && ((iBufferSize - iDataSize) < 16384))
		{
			iBufferSize += 65536;
			cBufferRecv = (char*)DBG_REALLOC(cBufferRecv, iBufferSize);
		}
	}
	if (iFileSize != 0)
	{
		*cBuffer = (char*)DBG_MALLOC(iFileSize);
		memcpy(*cBuffer, cBufferRecv + i, iFileSize);
	}
	DBG_FREE(cBufferRecv);
	//printf("iFileSize %i\n", iFileSize);
	
	shutdown(v_socket, SD_RECEIVE);
	close(v_socket);
	//WSACleanup(); 
    DBG_LOG_OUT();
	return iFileSize;
}
