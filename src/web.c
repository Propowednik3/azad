#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include "web.h"
#include "flv-muxer.h"
#include "weather.h"
#include <zlib.h>
#include "weather_func.h"
#include <openssl/md5.h>
#include "rtsp.h"
#include "main.h"
#include "gpio.h"
#include "omx_client.h"
#include "system.h"
#include "widgets.h"
#include "tfp625a.h"

#define URL_MARK     	"http:"
#define CLIENT_PORT 	"client_port="
#define INTERLEAVED 	"interleaved="
#define SESSION 		"Session: "
#define AUTH_MARK 		"Authorization: Digest "
#define URI_MARK 		"uri=\""
#define USERNAME_MARK 	"username=\""
#define RESPONSE_MARK 	"response=\""
#define CNONCE_MARK 	"cnonce=\""
#define NC_MARK 		"nc="

#define WEB_MESSAGES_MAX_CNT	10
#define WEB_MESSAGES_MAX_LEN	128
#define WEB_PAGE_MAX_LEN		20
#define WEB_HISTORY_MAX_CNT		128
#define WEB_AUTH_LOCK_TIME		10

static pthread_t threadWEB_income;
pthread_attr_t tattrWEB_income;

static pthread_t threadWEB_io;
pthread_attr_t tattrWEB_io;

static pthread_t threadWEB_file;
pthread_attr_t tattrWEB_file;

char cWEB_init_done;
int iWebMessListCnt;
char *cWebMessList;
int iCurModulePage;
int iCurAlarmPage;
int iCurSoundPage;
int iCurMailPage;
int iCurStreamPage;
int iCurStreamTypeFilter;
int iCurStrmTypePage;
int iCurWidgetPage;
int iCurDirectoryPage;
int iCurMnlActionsPage;
int iCurEvntActionsPage;
int iCurKeyPage;
int iCurIrCodePage;
int iCurCamRectPage;
int iCurSkipIrCodePage;
int iCurAlienKeyPage;
int iCurSkipEventPage;
int iCurUserPage;
int iCurManualPage;
int iCurModStatusPage;
int iCurModStatusAreaFilter;
int iCurModStatusTypeFilter;
int iCurModStatusEnabledFilter;
int iCurModuleAreaFilter;
int iCurModuleTypeFilter;
int iCurModuleEnabledFilter;
unsigned int uiCurEvntActIdFilter;
char cCurEvntActEnFilter;
char cCurEvntActGroupFilter[130];
int iCurRadioPage;
int iCurConnectPage;
int iCurConnectPage2;
char cLastLeftPath[MAX_FILE_LEN];
char cLastRightPath[MAX_FILE_LEN];
int iCurViewSize;
int iCurViewerPage;
char cLastViewerFile[MAX_FILE_LEN];
misc_buffer mbFileBuffer;
char cExplorerBusy;
int iCurHistoryPage;
int iCurMessagesPage;

long int WEB_Auth_lock_time;

int cThreadWebStatus;
int Web_Pair[2];
	
int iCurExplLeftPath;	
int iExplorerLeftPage;
unsigned int uiExplorerLeftCnt;
DIRECTORY_INFO *diExplorerLeft;

int iCurExplRightPath;
int iExplorerRightPage;
unsigned int uiExplorerRightCnt;
DIRECTORY_INFO *diExplorerRight;
char cYouTubeLastUrl[MAX_FILE_LEN];
char cYouTubeLastType;

void * thread_WEB_file(void* pData);
void * thread_WEB_income(void* pData);
void * thread_WEB_io(void* pData);
void WEB_GetMessageList(char *msg_tx);
int WEB_refresh_dir(unsigned int uiDirect, char* cPath, DIRECTORY_INFO **diDir, unsigned int *uiDirCnt);


void web_start(unsigned int uiPort)
{
	cWEB_init_done = 0;
	iCurModulePage = 0;
	iCurAlarmPage = 0;
	iCurSoundPage = 0;
	iCurMailPage = 0;
	iCurStreamPage = 0;
	iCurStreamTypeFilter = -1;
	iCurStrmTypePage = 0;
	iCurWidgetPage = 0;
	iCurDirectoryPage = 0;
	iCurMnlActionsPage = 0;
	iCurEvntActionsPage = 0;
	iCurKeyPage = 0;
	iCurConnectPage = 0;
	iCurConnectPage2 = 0;
	iCurIrCodePage = 0;
	iCurCamRectPage = 0;
	iCurSkipIrCodePage = 0;
	iCurAlienKeyPage = 0;
	iCurSkipEventPage = 0;
	iCurUserPage = 0;
	iCurManualPage = 0;
	iCurModStatusPage = 0;
	iCurModStatusEnabledFilter = 1;
	iCurModStatusAreaFilter = 1;
	iCurModStatusTypeFilter = MODULE_TYPE_MAX;
	iCurModuleEnabledFilter = 2;
	iCurModuleAreaFilter = 1;
	iCurModuleTypeFilter = MODULE_TYPE_MAX;
	uiCurEvntActIdFilter = 0;
	cCurEvntActEnFilter = 2;
	memset(cCurEvntActGroupFilter, 0, 130);
	iCurRadioPage = 0;
	iCurHistoryPage = 0;
	iCurExplLeftPath = 0;
	iExplorerLeftPage = 0;
	uiExplorerLeftCnt = 0;
	diExplorerLeft = NULL;
	iCurExplRightPath = 0;
	iExplorerRightPage = 0;
	uiExplorerRightCnt = 0;
	diExplorerRight = NULL;
	iWebHistoryCnt = 0;
	iCurMessagesPage = 0;
	whWebHistory = NULL;
	iCurViewSize = WEB_MAX_SIZE_FILE_VIEW;
	iCurViewerPage = 0;
	cExplorerBusy = 0;
	memset(cLastLeftPath, 0, MAX_FILE_LEN);
	memset(cLastRightPath, 0, MAX_FILE_LEN);
	memset(cLastViewerFile, 0, MAX_FILE_LEN);
	memset(&mbFileBuffer, 0, sizeof(mbFileBuffer));
	memset(cYouTubeLastUrl, 0, MAX_FILE_LEN);	
	cYouTubeLastType = 18;
	
	WEB_refresh_dir(iCurExplLeftPath, cLastLeftPath, &diExplorerLeft, &uiExplorerLeftCnt);
	WEB_refresh_dir(iCurExplRightPath, cLastRightPath, &diExplorerRight, &uiExplorerRightCnt);
	
	pthread_mutex_init(&WEB_explorer_mutex, NULL);
	pthread_mutex_init(&WEB_req_mutex, NULL);
	
	pthread_attr_init(&tattrWEB_io);   
	pthread_attr_setdetachstate(&tattrWEB_io, PTHREAD_CREATE_DETACHED);
	
	pthread_attr_init(&tattrWEB_file);   
	pthread_attr_setdetachstate(&tattrWEB_file, PTHREAD_CREATE_DETACHED);
	
	pthread_mutex_init(&WEB_mutex, NULL);
	
	DBG_MUTEX_LOCK(&WEB_mutex);		
	cThreadWebStatus = 0;
	Web_Pair[0] = 0;
	Web_Pair[1] = 0;		
	DBG_MUTEX_UNLOCK(&WEB_mutex);	
	
	unsigned int *cSettings = (unsigned int*)DBG_MALLOC(sizeof(unsigned int)*10);
	cSettings[0] = uiPort;
	pthread_attr_init(&tattrWEB_income);   
	pthread_attr_setdetachstate(&tattrWEB_income, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadWEB_income, &tattrWEB_income, thread_WEB_income, (void*)cSettings); 
	
	cWEB_init_done = 1;
	iWebMessListCnt = 0;
	cWebMessList = NULL;
}

int web_stop()
{
	if (cWEB_init_done == 0) return 0;
	//pthread_mutex_destroy(&WEB_mutex);
	DBG_MUTEX_LOCK(&WEB_mutex);
	if (Web_Pair[0] != 0)
	{
		char cType = SIGNAL_CLOSE;
		int rv = write(Web_Pair[0], &cType, 1);
		if (rv < 1) dbgprintf(4, "web_stop: write socketpair signal %i (errno:%i, %s)\n", cType, errno, strerror(errno));	
	}
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	int ret;
	do
	{
		DBG_MUTEX_LOCK(&WEB_mutex);
		ret = cThreadWebStatus;
		DBG_MUTEX_UNLOCK(&WEB_mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);
	
	if (uiExplorerLeftCnt) 
	{
		uiExplorerLeftCnt = 0;
		DBG_FREE(diExplorerLeft);
		diExplorerLeft = NULL;
	}
	if (uiExplorerRightCnt) 
	{
		uiExplorerRightCnt = 0;
		DBG_FREE(diExplorerRight);
		diExplorerRight = NULL;
	}
	WEB_ClearWebHistory();
	
	pthread_attr_destroy(&tattrWEB_income);
	pthread_attr_destroy(&tattrWEB_io);	
	pthread_attr_destroy(&tattrWEB_file);	
	pthread_mutex_destroy(&WEB_mutex);	
	pthread_mutex_destroy(&WEB_explorer_mutex);
	pthread_mutex_destroy(&WEB_req_mutex);
	return 1;
}

int WEB_load_limit_file_in_buff(char *filename, misc_buffer * buffer, unsigned int uiLen) 
{
	int32_t pos;
	FILE *fp;
	  
	buffer->data_size = 0;
	buffer->void_data = NULL;
	fp = fopen (filename, "rb");
	if (!fp) 
	{
		dbgprintf(4,"Error open file %s \n",filename);
		return 0;
	}
	if (fseek (fp, 0L, SEEK_END) < 0) 
	{
		dbgprintf(4,"Error seek file %s \n",filename);
		fclose (fp);
		return 0;
	}
	pos = ftell (fp);
	if (pos == LONG_MAX) 
	{
		dbgprintf(4,"Error tell file %s \n",filename);
		fclose (fp);
		return 0;
	}
	if (pos > uiLen)
	{
	    fclose (fp);
		return -1;
	}
	buffer->data_size = pos;
	fseek (fp, 0L, SEEK_SET);
	buffer->void_data = DBG_MALLOC(buffer->data_size);
	fread (buffer->void_data, 1, buffer->data_size, fp);
	fclose (fp);
	return 1;
}

int WEB_pages_list(char *msg_tx, char *cPageName, int iSubPages, int iCurPage, char *cParams)
{
	char *buff1 = (char*)DBG_MALLOC(256);
	char *buffout = (char*)DBG_MALLOC(8192);
	int n, i, b, i2, b2;
	
    memset(buffout, 0, 8192);
	strcat(buffout, "<p style='color:#00ff00'>");
	i = 0;
	b = 0;
	i2 = 0;
	b2 = 0;
	for (n = 0; n < iSubPages; n++)
	{
		b++;
		if (b == 10)
		{
			b = 0;
			i = 1;
		}
		b2++;
		if (b2 == 100)
		{
			b2 = 0;
			i2 = 1;
		}				
		if ((n == 0) || (n == (iSubPages-1)) || (n == iCurPage) 
			|| ((n >= (iCurPage - 5)) && (n <= (iCurPage + 5)))
			|| ((i == 1) && (n >= (iCurPage - 50)) && (n <= (iCurPage + 50)))
			|| ((i2 == 1) && (n >= (iCurPage - 500)) && (n <= (iCurPage + 500))))
		{
			i = 0;
			i2 = 0;
			memset(buff1, 0, 256);
			if (n == iCurPage) sprintf(buff1, "<a href=\"/%s/list?Page=%i%s\"> [%i] </a>", cPageName, n, cParams, n + 1);
				else sprintf(buff1, "<a href=\"/%s/list?Page=%i%s\"> %i </a>", cPageName, n, cParams, n + 1);
			strcat(buffout, buff1);
		}		
	}		
	strcat(buffout, "</p>\r\n");
	strcat(msg_tx, buffout);
	
	DBG_FREE(buffout);
	DBG_FREE(buff1);
	return 1;
}

void WEB_sort_dirs(DIRECTORY_INFO *diDir, unsigned int uiDirCnt)
{
	DBG_LOG_IN();
	
	int i, n;
	int iSortNum = 0;
	int iDirNum = -1;
	char cFirstName[MAX_FILE_LEN + 1];
	char cNextName[MAX_FILE_LEN + 1];
	int iFirstType;
	char *cSortTmp = (char*)DBG_MALLOC(uiDirCnt);
	
	memset(cSortTmp, 0, uiDirCnt);
	for (i = 0; i < uiDirCnt; i++) diDir[i].Sort = -1;
	for (iSortNum = 0; iSortNum < uiDirCnt; iSortNum++)
	{
		iDirNum = -1;
		memset(cFirstName, 255, MAX_FILE_LEN);
		cFirstName[MAX_FILE_LEN] = 0;
		iFirstType = 0;
		for (i = 0; i < uiDirCnt; i++)
		{
			if (cSortTmp[i] == 0)
			{	
				memcpy(cNextName, diDir[i].Path, MAX_FILE_LEN);
				cNextName[MAX_FILE_LEN] = 0;
				UpperText(cNextName);
				
				for (n = 0; n < MAX_FILE_LEN; n++)
				{
					if ((iFirstType <= diDir[i].Type) && (cFirstName[n] > cNextName[n]))
					{
						iDirNum = i;
						memcpy(cFirstName, cNextName, MAX_FILE_LEN);
						cFirstName[MAX_FILE_LEN] = 0;
						//UpperText(cFirstName);
						iFirstType = diDir[i].Type;
						break;
					}
					if ((iFirstType >= diDir[i].Type) && (cFirstName[n] < cNextName[n])) break;
					if ((cFirstName[n] == 0) && (cNextName[n] == 0))
					{
						iDirNum = i;
						break;
					}
				} 
				if (n == MAX_FILE_LEN) iDirNum = i;					
			}
		}
		diDir[iSortNum].Sort = iDirNum;
		cSortTmp[iDirNum] = 1;
	}
	if (cSortTmp) DBG_FREE(cSortTmp);
	DBG_LOG_OUT();
}

int WEB_refresh_dir(unsigned int uiDirect, char* cPath, DIRECTORY_INFO **diDir, unsigned int *uiDirCnt)
{
	int res = 0;
	DIRECTORY_INFO *di = *diDir;
	unsigned int uiCnt = *uiDirCnt;
	char cDir[MAX_FILE_LEN];
	char cSubDir[MAX_FILE_LEN];
	memset(cDir, 0, MAX_FILE_LEN);
	
	if (uiCnt != 0)
	{
		DBG_FREE(di);
		di = NULL;
		uiCnt = 0;
		res = 0;
	}
	DBG_MUTEX_LOCK(&system_mutex);
	if (uiDirect < iDirsCnt)
	{
		if (strlen(diDirList[uiDirect].Path) < MAX_FILE_LEN) 
		{
			res = 1;
			strcpy(cDir, diDirList[uiDirect].Path);
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (res)
	{
		strcat(cDir, "/");
		strcat(cDir, cPath);
		DIR *dir;
		DIR *subdir;
		struct dirent *dp;
		int n;
		dir = opendir(cDir);
		if (dir != NULL)
		{
			while((dp=readdir(dir)) != NULL)
			{
				n = 1;
				if (((strlen(dp->d_name) == 1) && (dp->d_name[0] == 46))
					||
					((strlen(dp->d_name) == 2) && (dp->d_name[0] == 46) && (dp->d_name[1] == 46))) n = 0;
					
				if ((strlen(dp->d_name) < MAX_FILE_LEN) && (n == 1))
				{			
					uiCnt++;
					di = (DIRECTORY_INFO*)DBG_REALLOC(di, uiCnt*sizeof(DIRECTORY_INFO));
					memset(&di[uiCnt-1], 0, sizeof(DIRECTORY_INFO));
					strcpy(di[uiCnt-1].Path, dp->d_name);
					memset(cSubDir, 0, MAX_FILE_LEN);
					strcpy(cSubDir, cDir);
					strcat(cSubDir, "/");
					strcat(cSubDir, dp->d_name);
					subdir = opendir(cSubDir);
					if (subdir != NULL)
					{
						di[uiCnt-1].Type = 1;
						closedir(subdir);
					}
					else
					{
						if (errno != ENOTDIR) di[uiCnt-1].Type = 2;
					}
				}
			}
			closedir(dir);			
		} else res = 0;
    }
	
	WEB_sort_dirs(di, uiCnt);
	
	*diDir = di;
	*uiDirCnt = uiCnt;
	return res;
}

int WEB_decode_url(char *cUrl, int iLenUrl, char *OutBuffer, unsigned int iLenBuffer)
{
	int n, i;
	i = 0;
	for (n = 0; n < iLenUrl; n++)
	{
		if (i >= iLenBuffer) break;
		
		if (cUrl[n] == '%')
		{
			if ((n + 2) >= iLenUrl) break;
			n++;
			OutBuffer[i] = Hex2IntLimit(&cUrl[n], 2);
			n++;
		}
		else
		{
			if (cUrl[n] != '+') OutBuffer[i] = cUrl[n];
				else OutBuffer[i] = ' ';
		}
		i++;
	}
	
	return i;
}

int WEB_decode_url_to_src(char *cUrl, int iLenUrl)
{
	int n, i;
	i = 0;
	if (iLenUrl == 0) return 0;
	char *cbuff = (char*)DBG_MALLOC(iLenUrl);
	memset(cbuff, 0, iLenUrl);
	
	for (n = 0; n < iLenUrl; n++)
	{
		if (i >= iLenUrl) break;
		if (cUrl[n] == '%')
		{
			if ((n + 2) >= iLenUrl) break;
			n++;
			cbuff[i] = Hex2IntLimit(&cUrl[n], 2);
			n++;
		}		
		else
		{
			if (cUrl[n] != '+') cbuff[i] = cUrl[n];
				else cbuff[i] = ' ';
		}
		i++;
	}
	memset(cUrl, 0, iLenUrl);
	memcpy(cUrl, cbuff, i);
	DBG_FREE(cbuff);
	return i;
}

char* WEB_get_AudioCodecName(unsigned int uiCodecNum)
{
	switch(uiCodecNum)
	{
		case 0: return "AAC";
		case 1: return "MP3";
		case 2: return "Speex";
		case 3: return "PCM";
		case 4: return "ADPCM";
		case 5: return "NELLYMOSER";
		case 6: return "MP2";
		case 7: return "WMA v1";
		case 8: return "WMA v2";
		case 9: return "WMA voice";
		case 10: return "WMA pro";
		case 11: return "WMA lossless";
		case 12: return "MP1";
		case 13: return "MPEG2TS";
		case 14: return "AC3";
		case 15: return "DTS";
		case 16: return "VORBIS";
		case 17: return "WAVPACK";
		case 18: return "EAC3";
		default: return "UNKNOWN";
	}
	return "UNKNOWN";
}

int WEB_get_url(char *msg_rx, char *OutBuffer, int iLenBuffer)
{
	int n, ret;
	int msg_len = strlen(msg_rx);
	ret = SearchData("GET ", 4, msg_rx, msg_len, 0);
	if (ret != 0) return 0;
		
	memset(OutBuffer, 0, iLenBuffer);	
	for (n = 0; n < msg_len; n++) 
		if (msg_rx[n] < 32) break;
	ret = SearchData(" HTTP", 5, msg_rx, msg_len, 0);
	if (ret >= n) return 0;
	if ((ret - 4) >= iLenBuffer) return 0;
	memcpy(OutBuffer, &msg_rx[4], ret - 4);
	//printf("WEB_get_url '%s'\n", OutBuffer);
	return 1;
}

int WEB_get_param_from_url(char *cUrl, unsigned int uiUrlLen, char *param_name, char *param_value, int value_len)
{
	memset(param_value, 0, value_len);
	int msg_len = uiUrlLen;
	char *msg;
	char param[64];
	int n, ret;
	if (msg_len == 0) 
	{
		dbgprintf(2, "WEB_get_param_from_url: income str null lenght\n");
		return -1;
	}	
	if (strlen(param_name) >= 64)
	{
		dbgprintf(2, "WEB_get_param_from_url: big param lenght (%i)'%s'\n", strlen(param_name), param_name);
		return -1;
	}	
	
	for (n = 1; n < msg_len; n++) 
		if (cUrl[n] == '?') break;
	if (n < (msg_len - 1)) 
	{
		msg = &cUrl[n + 1]; 
		msg_len = uiUrlLen - (n + 1);
	}
	else msg = cUrl;
	memset(param, 0, 64);
	strcpy(param, param_name);
	strcat(param, "=");
	ret = SearchData(param, strlen(param), msg, msg_len, 0);
	if (ret == -1) 
	{
		dbgprintf(5, "WEB_get_param_from_url: param not finded '%s'\n", param);
		return -1;
	}
	if ((ret != 0) && (msg[ret-1] != '?') && (msg[ret-1] != '&'))  
	{
		dbgprintf(2, "WEB_get_param_from_url: income str bad\n");
		return -1;
	}
	ret += strlen(param);
	
	n = SearchData("&", 1, msg, msg_len, ret);
	if (n == -1) n = msg_len;
	
	if ((n - ret) >= value_len) 
	{
		dbgprintf(2, "WEB_get_param_from_url: big len param value :%i maxlen: %i\n", n - ret, value_len);
		return -1;
	}
	
	memcpy(param_value, &msg[ret], n - ret);
	//printf("WEB_get_param_from_url: '%s'='%s'\n", param_name, param_value);
	return n - ret;
}

int uncompress_gzip(void *src, int srcLen, void *dst, int dstLen) 
{
    z_stream strm  = {0};
    strm.total_in  = strm.avail_in  = srcLen;
    strm.total_out = strm.avail_out = dstLen;
    strm.next_in   = (Bytef *) src;
    strm.next_out  = (Bytef *) dst;

    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    int err = -1;
    int ret = -1;

    err = inflateInit2(&strm, (15 + 32)); //15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
    if (err == Z_OK) {
        err = inflate(&strm, Z_FINISH);
        if (err == Z_STREAM_END) {
            ret = strm.total_out;
        }
        else {
             inflateEnd(&strm);
             return err;
        }
    }
    else {
        inflateEnd(&strm);
        return err;
    }

    inflateEnd(&strm);
    return ret;
}

int WEB_get_block(unsigned int uiNum, char *cBuffer, unsigned int uiLen, char cSplitter, unsigned int *uiStartPos, unsigned int *uiBlockLen)
{
	*uiStartPos = 0;
	*uiBlockLen = 0;
	int n = 0;
	int cnt = 0;	
	
	while((uiNum != cnt) && (n < uiLen))
		for(;n < uiLen; n++) 
			if (cBuffer[n] == cSplitter) {n++; cnt++; break;}
	if (uiNum != cnt) return 0;
	if (n != uiLen) 
	{
		*uiStartPos = n;
		for(;n < uiLen; n++) 
			if (cBuffer[n] == cSplitter) {n++; break;}
		if (n != uiLen) *uiBlockLen = n - 1 - (*uiStartPos); else *uiBlockLen = uiLen - (*uiStartPos);
	} else *uiBlockLen = uiLen - (*uiStartPos);
	return 1;
}

char WEB_test_youtube_content_H264(unsigned int itag)
{
	if (itag == 18) return 1;
	if (itag == 22) return 1;
	if (itag == 37) return 1;
	if (itag == 82) return 1;
	if (itag == 83) return 1;
	if (itag == 84) return 1;
	if (itag == 85) return 1;	
	return 0;
}

char* WEB_get_youtube_content_name(unsigned int itag)
{
	if (itag == 5) return "flv; 400x240; 25 fps; FLV1 H.263; MP3";
	if (itag == 6) return "flv; 450x270";
	if (itag == 13) return "3gp";
	if (itag == 17) return "3gp; 176x144; 12 fps; MPEG-4; AAC";
	if (itag == 18) return "mp4; 640x360; 25 fps; AVC (MPEG4 H.264); AAC";
	if (itag == 22) return "mp4; 1280x720; 25 fps; AVC (MPEG4 H.264); AAC";
	if (itag == 34) return "flv; 640x360";
	if (itag == 35) return "flv; 854x480";
	if (itag == 36) return "3gp; 320x240; 25 fps; MPEG4 H.263;AAC";
	if (itag == 37) return "mp4; 1920x1080";
	if (itag == 38) return "mp4; 4096x3072";
	if (itag == 43) return "webm; 640x360; 24.194 fps; VP8; Vorbis";
	if (itag == 44) return "webm; 854x480";
	if (itag == 45) return "webm; 1280x720";
	if (itag == 46) return "webm; 1920x1080";
	if (itag == 82) return "mp4; 640x360 [3D]; 25 fps; AVC (MPEG4 H.264); AAC";
	if (itag == 83) return "mp4; 854x480 [3D]";
	if (itag == 84) return "mp4; 1280x720 [3D]; 25 fps; AVC (MPEG4 H.264); AAC";
	if (itag == 85) return "mp4; 1920x1080p [3D]";
	if (itag == 100) return "webm; 640x360 [3D]; 24.194 fps; VP8; AAC";
	if (itag == 101) return "webm; 854x480 [3D]";
	if (itag == 102) return "webm; 1280x720 [3D]";
	if (itag == 92) return "mp4; 320x240; HLS";
	if (itag == 93) return "mp4; 640x360; HLS";
	if (itag == 94) return "mp4; 854x480; HLS";
	if (itag == 95) return "mp4; 1280x720; HLS";
	if (itag == 96) return "mp4; 1920x1080; HLS";
	if (itag == 132) return "mp4; 320x240; HLS";
	if (itag == 151) return "mp4; *x72";
	if (itag == 133) return "mp4; 320x240; DASH video";
	if (itag == 134) return "mp4; 640x360; DASH video";
	if (itag == 135) return "mp4; 854x480; DASH video";
	if (itag == 136) return "mp4; 1280x720; DASH video";
	if (itag == 137) return "mp4; 1920x1080; DASH video";
	if (itag == 138) return "mp4; *x2160 (not fix); DASH video";
	if (itag == 160) return "mp4; 176x144; DASH video";
	if (itag == 264) return "mp4; 176x1440; DASH video";
	if (itag == 298) return "mp4; 1280x720; 60 fps; DASH video H.264";
	if (itag == 299) return "mp4; 1920x1080; 60 fps; DASH video H.264";
	if (itag == 266) return "mp4; *x2160; 60 fps; DASH video H.264;Dash MP4 Audio";
	if (itag == 139) return "m4a; Stereo, 44.1 KHz 48 Kbps; AAC [DASH audio]";
	if (itag == 140) return "m4a; Stereo, 44.1 KHz 128 Kbps; AAC [DASH audio]";
	if (itag == 141) return "m4a; Stereo, 44.1 KHz 256 Kbps; AAC [DASH audio]";
	if (itag == 167) return "webm; 640x360; VP8 DASH video";
	if (itag == 168) return "webm; 854x480; VP8 DASH video";
	if (itag == 169) return "webm; 1280x720; VP8 DASH video";
	if (itag == 170) return "webm; 1920x1080; VP8 DASH video";
	if (itag == 218) return "webm; 854x480; VP8 DASH video";
	if (itag == 219) return "webm; 854x480; VP8 DASH video";
	if (itag == 219) return "webm; *x144; VP9 DASH video";
	if (itag == 242) return "webm; 320x240; VP8 DASH video";
	if (itag == 243) return "webm; 640x360; VP8 DASH video";
	if (itag == 244) return "webm; 854x480; VP8 DASH video";
	if (itag == 245) return "webm; 854x480; VP8 DASH video";
	if (itag == 246) return "webm; 854x480; VP8 DASH video";
	if (itag == 247) return "webm; 1280x720; VP8 DASH video";
	if (itag == 248) return "webm; 1920x1080; VP8 DASH video";
	if (itag == 271) return "webm; 176x1440; VP8 DASH video";
	if (itag == 272) return "webm; *x2160; VP8 DASH video";
	if (itag == 302) return "webm; *x2160; 60 fps; VP9 DASH video";
	if (itag == 303) return "webm; 1920x1080; 60 fps; VP9 DASH video";
	if (itag == 308) return "webm; 176x1440; 60 fps; VP9 DASH video";
	if (itag == 313) return "webm; *x2160; 60 fps; VP9 DASH video";
	if (itag == 315) return "webm; *x2160; 60 fps; VP9 DASH video";
	if (itag == 171) return "webm; Stereo, 44.1 KHz 128 Kbps; AAC [DASH audio]";
	if (itag == 172) return "webm; Stereo, 44.1 KHz 256 Kbps; AAC [DASH audio]";
	return NULL;
}

int WEB_get_msg_type(char *msg_rx, int iLen, unsigned int *uiType, unsigned int *uiAction, unsigned int *uiRequestId, int *pPage, int *pPage2, int *iValue, char *strValue, float *flValue)
{
	int ret;
	int msg_len;
	char *msg = (char*)DBG_MALLOC(iLen);
	char param_value[2048];			
	
	*uiAction = WEB_ACT_NOTHING; 
	*uiType = WEB_MSG_UNKNOWN;
	*uiRequestId = 0;
	
	if (WEB_get_url(msg_rx, msg, iLen) == 0) 
	{
		DBG_FREE(msg);
		return 0;
	}
	//WEB_decode_url_to_src(msg, iLen);
	
	msg_len = iLen;
	ret = strcmp(msg, MARK_START);
	if (ret == 0) *uiType = WEB_MSG_START;
	ret = strcmp(msg, MARK_GET_ICO);
	if (ret == 0) *uiType = WEB_MSG_GET_ICO;	
	
	ret = SearchData(MARK_MANUAL, strlen(MARK_MANUAL), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MANUAL;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, strlen(msg), "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
	}
	ret = SearchData(MARK_LOG, strlen(MARK_LOG), msg, msg_len, 0);
	if (ret == 0) *uiType = WEB_MSG_LOG;
	ret = SearchData(MARK_LOGOUT, strlen(MARK_LOGOUT), msg, msg_len, 0);
	if (ret == 0) *uiType = WEB_MSG_LOGOUT;
	ret = SearchData(MARK_MEDIA, strlen(MARK_MEDIA), msg, msg_len, 0);
	if (ret == 0) *uiType = WEB_MSG_MEDIA;
	ret = SearchData(MARK_MENU, strlen(MARK_MENU), msg, msg_len, 0);
	if (ret == 0) *uiType = WEB_MSG_MENU;
	
	if (*uiType != WEB_MSG_UNKNOWN)
	{
		ret = SearchData(MARK_GET_INFO, strlen(MARK_GET_INFO), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_GET_INFO;
		ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_PLAY;
		ret = SearchData(MARK_PREV, strlen(MARK_PREV), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_PREV;
		ret = SearchData(MARK_PAGE, strlen(MARK_PAGE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_PAGE;
			if (WEB_get_param_from_url(msg, msg_len, "num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "pos", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value); else iValue[1] = -1;
		}
		ret = SearchData(MARK_SHOW, strlen(MARK_SHOW), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_SHOW;
			if (WEB_get_param_from_url(msg, strlen(msg), "mode", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
	}
	
	ret = SearchData(MARK_CONTROL, strlen(MARK_CONTROL), msg, msg_len, 0);
	if (ret == 0)
	{
		*uiType = WEB_MSG_CONTROL;
		
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0)
		{
			*uiAction = WEB_ACT_REFRESH;
			if (WEB_get_param_from_url(msg, strlen(msg), "direct", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);				
		}
		ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_PLAY;
		ret = SearchData(MARK_PAUSE, strlen(MARK_PAUSE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_PAUSE;
		ret = SearchData(MARK_STOP, strlen(MARK_STOP), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_STOP;		
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_CHANGE;
			if (WEB_get_param_from_url(msg, strlen(msg), "position", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, strlen(msg), "type", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);		
		}
		ret = SearchData(MARK_EXECUTE, strlen(MARK_EXECUTE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_EXECUTE;
			if (WEB_get_param_from_url(msg, strlen(msg), "mode", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_SHOW, strlen(MARK_SHOW), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_SHOW;
			if (WEB_get_param_from_url(msg, strlen(msg), "mode", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_NEXT, strlen(MARK_NEXT), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_NEXT;
			if (WEB_get_param_from_url(msg, msg_len, "id", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_PREV, strlen(MARK_PREV), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_PREV;
		ret = SearchData(MARK_SET_VOLUME, strlen(MARK_SET_VOLUME), msg, msg_len, 0);
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "volpos", param_value, 2048) >= 0) 
			{
				*uiAction = WEB_ACT_SET_VOLUME;
				iValue[0] = Str2Int(param_value);
			}
		}
		ret = SearchData(MARK_SET_PLAY_POS, strlen(MARK_SET_PLAY_POS), msg, msg_len, 0);
		if (ret > 0)
		{
			if (WEB_get_param_from_url(msg, msg_len, "playpos", param_value, 2048) >= 0) 
			{
				*uiAction = WEB_ACT_SET_PLAY_POS;
				iValue[0] = Str2Int(param_value);
			}
		}
	}	
	
	ret = SearchData(MARK_YOUTUBE, strlen(MARK_YOUTUBE), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_YOUTUBE;
		ret = SearchData(MARK_OPEN, strlen(MARK_OPEN), msg, msg_len, 0);
		if (ret > 0)
		{
			*uiAction = WEB_ACT_OPEN;
			if (WEB_get_param_from_url(msg, msg_len, "url", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "replay", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[1] = 1; else iValue[1] = 0;
			}
		}
		ret = SearchData(MARK_SHOW, strlen(MARK_SHOW), msg, msg_len, 0);
		if (ret > 0)
		{
			*uiAction = WEB_ACT_SHOW;
			if (WEB_get_param_from_url(msg, msg_len, "type", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "url", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "replay", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[1] = 1; else iValue[1] = 0;
			}
		}
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_REFRESH;
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_LOAD, strlen(MARK_LOAD), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_LOAD;
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
	}	
	
	ret = SearchData(MARK_HISTORY, strlen(MARK_HISTORY), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_HISTORY;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_DELETE;
	}
	
	ret = SearchData(MARK_CONNECTS, strlen(MARK_CONNECTS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_CONNECTS;
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{		
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
			iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Code", param_value, 2048) >= 0) 
			iValue[1] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_SYSTEM, strlen(MARK_SYSTEM), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_SYSTEM;
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_CHANGE;
		ret = SearchData(MARK_CLOSE, strlen(MARK_CLOSE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_CLOSE;
		ret = SearchData(MARK_EXIT, strlen(MARK_EXIT), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_EXIT;
		ret = SearchData(MARK_REBOOT, strlen(MARK_REBOOT), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_REBOOT;
		ret = SearchData(MARK_OFF, strlen(MARK_OFF), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_EXIT;
		ret = SearchData(MARK_UPDATE, strlen(MARK_UPDATE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_UPDATE;	
		ret = SearchData(MARK_RESTART, strlen(MARK_RESTART), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_RESTART;	
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_REFRESH;	
		ret = SearchData(MARK_STOP, strlen(MARK_STOP), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_STOP;
			if (WEB_get_param_from_url(msg, msg_len, "type", param_value, 2048) >= 0) 
			iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "mode", param_value, 2048) >= 0) 
			iValue[1] = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_SAVE;
			if (WEB_get_param_from_url(msg, msg_len, "DateDay", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[0] = Str2Int(param_value);
				if (iValue[0] < 0) iValue[0] = 0;
			} else iValue[0] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "DateMont", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[1] = Str2Int(param_value);
				if (iValue[1] < 0) iValue[1] = 0;
			} else iValue[1] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "DateYear", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[2] = Str2Int(param_value);
				if (iValue[2] < 0) iValue[2] = 0;
			} else iValue[2] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "TimeHour", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[3] = Str2Int(param_value);
				if (iValue[3] < 0) iValue[3] = 0;
			} else iValue[3] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "TimeMin", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[4] = Str2Int(param_value);
				if (iValue[4] < 0) iValue[4] = 0;
			} else iValue[4] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "TimeSec", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[5] = Str2Int(param_value);
				if (iValue[5] < 0) iValue[5] = 0;
			} else iValue[5] = 0;
		}		
	}
	
	ret = SearchData(MARK_MIC, strlen(MARK_MIC), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MIC;
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) 
		{		
			*uiAction = WEB_ACT_CHANGE;
			if (WEB_get_param_from_url(msg, msg_len, "Vol", param_value, 2048) >= 0) 
			{
				iValue[0] = Str2Int(param_value);
				if ((iValue[0] < 0) || (iValue[0] > 100)) iValue[0] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "AGC", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[2] = 1;
			} else iValue[2] = 0;
			if ((WEB_get_param_from_url(msg, msg_len, "mic_id", param_value, 2048) >= 0) 
				&& (strlen(param_value) < 5))
					memcpy(&iValue[1], param_value, strlen(param_value));
			if (WEB_get_param_from_url(msg, msg_len, "Convert", param_value, 2048) >= 0) 
			{
				iValue[3] = Str2Int(param_value);
				if ((iValue[3] < 0) || (iValue[3] > 2)) iValue[3] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "DigLevel", param_value, 2048) >= 0) 
			{
				iValue[4] = Str2Int(param_value);
				if ((iValue[4] < 1) || (iValue[4] > 14)) iValue[4] = 1;
			}
		}
	}
	
	ret = SearchData(MARK_CAMERA, strlen(MARK_CAMERA), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_CAMERA;
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) 
		{		
			*uiAction = WEB_ACT_CHANGE;
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
			iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "MainCam", param_value, 2048) >= 0) 
			iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PrevCam", param_value, 2048) >= 0) 
			iValue[2] = Str2Int(param_value) & 3;
			if (WEB_get_param_from_url(msg, msg_len, "PrevLevel", param_value, 2048) >= 0) 
			iValue[3] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PrevGane", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[2] |= 4;
			}
			if (WEB_get_param_from_url(msg, msg_len, "BrigCam", param_value, 2048) >= 0) 
			iValue[2] |= (Str2Int(param_value) & 3) << 3;
			if (WEB_get_param_from_url(msg, msg_len, "ColorCam", param_value, 2048) >= 0) 
			iValue[2] |= (Str2Int(param_value) & 3) << 5;
			if (WEB_get_param_from_url(msg, msg_len, "ColorSens", param_value, 2048) >= 0) 
			iValue[4] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "BrigSens", param_value, 2048) >= 0) 
			iValue[5] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PrevSquare", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
					iValue[2] |= 128;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ManBright", param_value, 2048) >= 0) 
			{
				iValue[6] = Str2Int(param_value);	
				if ((iValue[6] < 0) || (iValue[6] > 100)) iValue[6] = 50;
			} else iValue[6] = 50;
			if (WEB_get_param_from_url(msg, msg_len, "AutoBright", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[7] = 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "DestBright", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[8] = Str2Int(param_value);
				if ((iValue[8] < 0) || (iValue[8] > 100)) iValue[8] = 50;
			} else iValue[8] = 50;		
			if (WEB_get_param_from_url(msg, msg_len, "ResetNormMoveSize", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
					iValue[9] = 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ResetSlowMoveSize", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
					iValue[10] = 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "Resources", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[11] = Str2Int(param_value);
				if ((iValue[11] < 0) || (iValue[11] > 2)) iValue[8] = 0;
			} else iValue[11] = 0;		
			if (WEB_get_param_from_url(msg, msg_len, "HighLight", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[12] = 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "HLBright", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[13] = Str2Int(param_value) & 255;
				if ((iValue[13] < 1) || (iValue[13] > 100)) iValue[13] = 20;
			} else iValue[13] = 20;
			if (WEB_get_param_from_url(msg, msg_len, "HLAmplif", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[14] = Str2Int(param_value) & 255;
				if ((iValue[14] < 1) || (iValue[14] > 100)) iValue[14] = 3;
			} else iValue[14] = 3;
			if (WEB_get_param_from_url(msg, msg_len, "Filter", param_value, 2048) >= 0) 
			iValue[15] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Balance", param_value, 2048) >= 0) 
			iValue[16] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "Contrast", param_value, 2048) >= 0) 
			{
				iValue[17] = Str2Int(param_value);	
				if ((iValue[17] < -100) || (iValue[17] > 100)) iValue[17] = 0;
			} else iValue[17] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Sharpness", param_value, 2048) >= 0) 
			{
				iValue[18] = Str2Int(param_value);	
				if ((iValue[18] < -100) || (iValue[18] > 100)) iValue[18] = 0;
			} else iValue[18] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Saturation", param_value, 2048) >= 0) 
			{
				iValue[19] = Str2Int(param_value);	
				if ((iValue[19] < -100) || (iValue[19] > 100)) iValue[19] = 0;
			} else iValue[19] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "LeftCrop", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[20] = Str2Int(param_value);
				if ((iValue[20] < 0) || (iValue[20] > 100)) iValue[20] = 0;						
			} else iValue[20] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "RightCrop", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[21] = Str2Int(param_value);
				if ((iValue[21] < 0) || (iValue[21] > 100)) iValue[21] = 0;						
			} else iValue[21] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "UpCrop", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[22] = Str2Int(param_value);
				if ((iValue[22] < 0) || (iValue[22] > 100)) iValue[22] = 0;						
			} else iValue[22] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "DownCrop", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[23] = Str2Int(param_value);
				if ((iValue[23] < 0) || (iValue[23] > 100)) iValue[23] = 0;						
			} else iValue[23] = 0;
	
			iValue[24] = 0;			
			if (WEB_get_param_from_url(msg, msg_len, "AutoISOhw", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[24] |= 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "AutoISOsw", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[24] |= 2;
			}
			if (WEB_get_param_from_url(msg, msg_len, "AutoEVsw", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[24] |= 4;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ISOvalue", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[25] = Str2Int(param_value);
				if ((iValue[25] < 100) || (iValue[25] > 800)) iValue[25] = 400;
			} else iValue[25] = 400;
			if (WEB_get_param_from_url(msg, msg_len, "DestISOBright", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[26] = Str2Int(param_value);
				if ((iValue[26] < 1) || (iValue[26] > 100)) iValue[26] = 50;
			} else iValue[26] = 50;
			if (WEB_get_param_from_url(msg, msg_len, "EVvalue", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[27] = Str2Int(param_value);
				if ((iValue[27] < -24) || (iValue[27] > 24)) iValue[27] = 0;
			} else iValue[27] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "DestEVBright", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[28] = Str2Int(param_value);
				if ((iValue[28] < 1) || (iValue[28] > 100)) iValue[28] = 50;
			} else iValue[28] = 50;	
		}
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_REFRESH;
	}
	
	ret = SearchData(MARK_IMAGES, strlen(MARK_IMAGES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_IMAGES;
		ret = SearchData(MARK_IMAGE_CAMRECT, strlen(MARK_IMAGE_CAMRECT), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_IMAGE_CAMRECT;
		ret = SearchData(MARK_IMAGE_CAMPREV, strlen(MARK_IMAGE_CAMPREV), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_IMAGE_CAMPREV;
		ret = SearchData(MARK_IMAGE_CAMMAIN, strlen(MARK_IMAGE_CAMMAIN), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_IMAGE_CAMMAIN;
	}
	
	ret = SearchData(MARK_MESSAGES, strlen(MARK_MESSAGES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MESSAGES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_DELETE;
	}
	
	ret = SearchData(MARK_VIEWER, strlen(MARK_VIEWER), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_VIEWER;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_DELETE;
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_REFRESH;
	}
	
	ret = SearchData(MARK_EXPLORER, strlen(MARK_EXPLORER), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_EXPLORER;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
			iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			{
				if (iValue[0] == 2) *pPage2 = Str2Int(param_value); else *pPage = Str2Int(param_value);
			}
		}
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_CHANGE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
		}
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_REFRESH;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_PLAY;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_SHOW, strlen(MARK_SHOW), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_SHOW;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_OPEN, strlen(MARK_OPEN), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_OPEN;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
		}
		ret = SearchData(MARK_PREV, strlen(MARK_PREV), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_PREV;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
		}
		ret = SearchData(MARK_WORK_REQUEST, strlen(MARK_WORK_REQUEST), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_WORK_REQUEST;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Act", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
		}
		ret = SearchData(MARK_WORK_ACCEPT, strlen(MARK_WORK_ACCEPT), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_WORK_ACCEPT;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Act", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
		}
	}		
	
	ret = SearchData(MARK_RADIOS, strlen(MARK_RADIOS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_RADIOS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Freq", param_value, 2048) >= 0) 
				flValue[0] = Str2Float(param_value);
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}		
	
	ret = SearchData(MARK_SETTINGS, strlen(MARK_SETTINGS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_SETTINGS;		
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_SAVE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);		
			if (WEB_get_param_from_url(msg, msg_len, "Val1", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[0], param_value, 255);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Val2", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[256], param_value, 255);
					else memcpy(&strValue[256], param_value, strlen(param_value));
			}		
			if (WEB_get_param_from_url(msg, msg_len, "Val3", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[512], param_value, 255);
					else memcpy(&strValue[512], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "Val4", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[768], param_value, 255);
					else memcpy(&strValue[768], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "Val5", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[1024], param_value, 255);
					else memcpy(&strValue[1024], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "Val6", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[1280], param_value, 255);
					else memcpy(&strValue[1280], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Val7", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[1536], param_value, 255);
					else memcpy(&strValue[1536], param_value, strlen(param_value));
			}		
			if (WEB_get_param_from_url(msg, msg_len, "Val8", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[1792], param_value, 255);
					else memcpy(&strValue[1792], param_value, strlen(param_value));
			}
		}
	}
	
	ret = SearchData(MARK_MODSTATUSES, strlen(MARK_MODSTATUSES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MODSTATUSES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_PLAY;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_STOP, strlen(MARK_STOP), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_STOP;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_LOAD, strlen(MARK_LOAD), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_LOAD;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_ADD;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TempNum", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TempID", param_value, 2048) >= 0)
			{			
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[2], param_value, 4);
					else memcpy(&iValue[2], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "TempInfo", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 31) memcpy(&strValue[0], param_value, 31);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_CHANGE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TempNum", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TempID", param_value, 2048) >= 0)
			{			
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[2], param_value, 4);
					else memcpy(&iValue[2], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "TempInfo", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 31) memcpy(&strValue[0], param_value, 31);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
		}		
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_REFRESH;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TempNum", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
		}
		
		ret = SearchData(MARK_SHOW, strlen(MARK_SHOW), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_SHOW;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "Action", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
		}
		ret = SearchData(MARK_FILTER, strlen(MARK_FILTER), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_FILTER;
			if (WEB_get_param_from_url(msg, msg_len, "Enabled", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "Area", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_SAVE;
			//WEB_decode_url_to_src(param_value, strlen(param_value));	
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);	
			iValue[1] = 0;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh0", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000001;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh1", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000002;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh2", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000004;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh3", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000008;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh4", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000010;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh5", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000020;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh6", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000040;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh7", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000080;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh8", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000100;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh9", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000200;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh10", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000400;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh11", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00000800;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh12", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00001000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh13", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00002000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh14", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00004000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh15", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00008000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh16", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00010000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh17", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00020000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh18", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00040000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh19", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00080000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh20", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00100000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh21", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00200000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh22", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00400000;
			if ((WEB_get_param_from_url(msg, msg_len, "SvCh23", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[1] |= 0x00800000;	
			iValue[2] = 0;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv0", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000001;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv1", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000002;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv2", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000004;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv3", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000008;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv4", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000010;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv5", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000020;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv6", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000040;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv7", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000080;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv8", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000100;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv9", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000200;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv10", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000400;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv11", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00000800;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv12", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00001000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv13", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00002000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv14", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00004000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv15", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00008000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv16", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00010000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv17", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00020000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv18", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00040000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv19", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00080000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv20", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00100000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv21", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00200000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv22", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00400000;
			if ((WEB_get_param_from_url(msg, msg_len, "GnEv23", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[2] |= 0x00800000;
			iValue[3] = 0;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv0", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000001;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv1", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000002;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv2", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000004;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv3", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000008;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv4", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000010;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv5", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000020;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv6", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000040;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv7", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000080;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv8", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000100;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv9", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000200;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv10", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000400;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv11", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00000800;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv12", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00001000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv13", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00002000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv14", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00004000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv15", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00008000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv16", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00010000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv17", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00020000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv18", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00040000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv19", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00080000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv20", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00100000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv21", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00200000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv22", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00400000;
			if ((WEB_get_param_from_url(msg, msg_len, "GlEv23", param_value, 2048) >= 0) 
				&& (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON"))) iValue[3] |= 0x00800000;			
			iValue[4] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "ScanSet", param_value, 2048) >= 0) 
				iValue[4] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_USERS, strlen(MARK_USERS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_USERS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "En", param_value, 2048) >= 0) 
				iValue[1] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;			
			if (WEB_get_param_from_url(msg, msg_len, "Web", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_WEB : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Rtsp", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_RTSP : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Onvif", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_ONVIF : 0;
			if (WEB_get_param_from_url(msg, msg_len, "User", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_USERS : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Menu", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MENU : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Modl", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MODULES : 0;
			if (WEB_get_param_from_url(msg, msg_len, "ModSt", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MODSTATUSES : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Alrm", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_ALARMS : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Rads", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_RADIOS : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Snds", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_SOUNDS : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Mail", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MAILS : 0;		
			if (WEB_get_param_from_url(msg, msg_len, "StrTp", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_STREAMTYPES : 0;		
			if (WEB_get_param_from_url(msg, msg_len, "Strs", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_STREAMS : 0;		
			if (WEB_get_param_from_url(msg, msg_len, "Wdgt", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_WIDGETS : 0;		
			if (WEB_get_param_from_url(msg, msg_len, "Dirs", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_DIRECTORIES : 0;
			if (WEB_get_param_from_url(msg, msg_len, "EvntAct", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_EVNTACTIONS : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "MnlAct", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MNLACTIONS : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Keys", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_KEYS : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Irs", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_IRCODES : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Sqrs", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_CAMRECTS : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Med", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MEDIA : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Cntl", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_CONTROL : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "You", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_YOUTUBE : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Mnal", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MANUAL : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Logs", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_LOG : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Explr", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_EXPLORER : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Setts", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_SETTINGS : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Camr", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_CAMERA : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Mic", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_MIC : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Sys", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_SYSTEM : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Conn", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_CONNECTS : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Hist", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? ACCESS_HISTORY : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Lvl", param_value, 2048) >= 0) 
				iValue[3] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Login", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Pass", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[64], param_value, 63);
					else memcpy(&strValue[64], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_SKIPEVENTS, strlen(MARK_SKIPEVENTS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_SKIPEVENTS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_SAVE;
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "SrcID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[1], param_value, 4);
					else memcpy(&iValue[1], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "SrcSubNumber", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
						
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_ALIENKEYS, strlen(MARK_ALIENKEYS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_ALIENKEYS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_SAVE;
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);						
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_SKIPIRCODES, strlen(MARK_SKIPIRCODES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_SKIPIRCODES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_SAVE;
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_IRCODES, strlen(MARK_IRCODES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_IRCODES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "CodeID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[1], param_value, 4);
					else memcpy(&iValue[1], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "CodeData", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 511) memcpy(&strValue[0], param_value, 511);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_KEYS, strlen(MARK_KEYS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_KEYS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[2], param_value, 4);
					else memcpy(&iValue[2], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Enabled", param_value, 2048) >= 0) 
				iValue[3] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
				else iValue[3] = 0;
			//if (WEB_get_param_from_url(msg, msg_len, "EvntEnbl", param_value, 2048) >= 0) 
			//	iValue[28] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			//	else iValue[28] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[4] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Serial", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}			
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[256], param_value, 63);
					else memcpy(&strValue[256], param_value, strlen(param_value));
			}			
			if (WEB_get_param_from_url(msg, msg_len, "VerKeyA", param_value, 2048) >= 0) 
				iValue[11] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
				else iValue[11] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "KeyA1", param_value, 2048) >= 0) 
				iValue[12] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyA2", param_value, 2048) >= 0) 
				iValue[13] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyA3", param_value, 2048) >= 0) 
				iValue[14] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyA4", param_value, 2048) >= 0) 
				iValue[15] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyA5", param_value, 2048) >= 0) 
				iValue[16] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyA6", param_value, 2048) >= 0) 
				iValue[17] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "VerKeyB", param_value, 2048) >= 0) 
				iValue[18] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
				else iValue[18] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "KeyB1", param_value, 2048) >= 0) 
				iValue[19] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyB2", param_value, 2048) >= 0) 
				iValue[20] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyB3", param_value, 2048) >= 0) 
				iValue[21] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyB4", param_value, 2048) >= 0) 
				iValue[22] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyB5", param_value, 2048) >= 0) 
				iValue[23] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "KeyB6", param_value, 2048) >= 0) 
				iValue[24] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Sector", param_value, 2048) >= 0) 
				iValue[25] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Action", param_value, 2048) >= 0) 
				iValue[26] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "CrId", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[27], param_value, 4);
					else memcpy(&iValue[27], param_value, strlen(param_value));	
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_EVNTACTIONS, strlen(MARK_EVNTACTIONS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_EVNTACTIONS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "NewPos", param_value, 2048) >= 0) 
				iValue[20] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "ActionID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[1], param_value, 4);
					else memcpy(&iValue[1], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "En", param_value, 2048) >= 0) 
				iValue[2] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "SrcID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[3], param_value, 4);
					else memcpy(&iValue[3], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "SrcSubNumber", param_value, 2048) >= 0) 
				iValue[4] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TestType", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[5] = Str2Int(param_value);
			}
			if (WEB_get_param_from_url(msg, msg_len, "SrcStatus", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[6] = (unsigned int)GetModuleSettings(param_value, strlen(param_value), 1);
			}
			if (WEB_get_param_from_url(msg, msg_len, "TestModuleID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[7], param_value, 4);
					else memcpy(&iValue[7], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "TestModuleSubNumber", param_value, 2048) >= 0) 
				iValue[8] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TestModuleStatus", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[9] = (unsigned int)GetModuleSettings(param_value, strlen(param_value), 1);
			}
			if (WEB_get_param_from_url(msg, msg_len, "DestID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[10], param_value, 4);
					else memcpy(&iValue[10], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "DestSubNumber", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[11] = (unsigned int)GetModuleSettings(param_value, strlen(param_value), 1);
			}
			if (WEB_get_param_from_url(msg, msg_len, "DestStatus", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[12] = (unsigned int)GetModuleSettings(param_value, strlen(param_value), 1);
			}
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 127) memcpy(&strValue[0], param_value, 127);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "AfterAccept", param_value, 2048) >= 0) 
				iValue[13] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TimerWaitBefore", param_value, 2048) >= 0) 
				iValue[14] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "TimerOff", param_value, 2048) >= 0) 
				iValue[15] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Mo", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Tu", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 2 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "We", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 4 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Th", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 8 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Fr", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 16 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Sa", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 32 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Su", param_value, 2048) >= 0) 
				iValue[16] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 64 : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Hour11", param_value, 2048) >= 0) 
				iValue[17] = Str2Int(param_value)*10000; else iValue[17] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min11", param_value, 2048) >= 0) 
				iValue[17] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec11", param_value, 2048) >= 0) 
				iValue[17] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Hour12", param_value, 2048) >= 0) 
				iValue[18] = Str2Int(param_value)*10000; else iValue[18] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min12", param_value, 2048) >= 0) 
				iValue[18] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec12", param_value, 2048) >= 0) 
				iValue[18] += Str2Int(param_value);		
			if (WEB_get_param_from_url(msg, msg_len, "RestartWaitTimer", param_value, 2048) >= 0) 
				iValue[19] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "RestartOffTimer", param_value, 2048) >= 0) 
				iValue[19] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 2 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Test2Type", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[21] = Str2Int(param_value);
			}
			if (WEB_get_param_from_url(msg, msg_len, "WaitBeforeStatus", param_value, 2048) >= 0) 
				iValue[22] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "OffStatus", param_value, 2048) >= 0) 
				iValue[23] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Group", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 127) memcpy(&strValue[256], param_value, 127);
					else memcpy(&strValue[256], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Test3ModuleID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[24], param_value, 4);
					else memcpy(&iValue[24], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Test3ModuleSubNumber", param_value, 2048) >= 0) 
				iValue[25] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Test3Type", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[26] = Str2Int(param_value);
			}
			if (WEB_get_param_from_url(msg, msg_len, "Test3ModuleStatus", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[27] = (unsigned int)GetModuleSettings(param_value, strlen(param_value), 1);
			}
		}
		ret = SearchData(MARK_EXECUTE, strlen(MARK_EXECUTE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_EXECUTE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_GET_UP, strlen(MARK_GET_UP), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_UP;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_GET_DOWN, strlen(MARK_GET_DOWN), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DOWN;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RECOVER, strlen(MARK_RECOVER), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RECOVER;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_REFRESH, strlen(MARK_REFRESH), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_REFRESH;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RESTART, strlen(MARK_RESTART), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RESTART;
		}
		ret = SearchData(MARK_FILTER, strlen(MARK_FILTER), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_FILTER;
			if (WEB_get_param_from_url(msg, msg_len, "Index", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[0], param_value, 4);
					else memcpy(&iValue[0], param_value, strlen(param_value));
			}				
			if (WEB_get_param_from_url(msg, msg_len, "Group", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 127) memcpy(&strValue[0], param_value, 127);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "Enabled", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_MNLACTIONS, strlen(MARK_MNLACTIONS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MNLACTIONS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Access", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "ModuleID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[2], param_value, 4);
					else memcpy(&iValue[2], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "SubNumber", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[3] = GetModuleSettings(param_value, strlen(param_value), 1);
			}
			if (WEB_get_param_from_url(msg, msg_len, "Status", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[4] = GetModuleSettings(param_value, strlen(param_value), 0);
			}
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 127) memcpy(&strValue[0], param_value, 127);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}			
		}
		ret = SearchData(MARK_EXECUTE, strlen(MARK_EXECUTE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_EXECUTE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RECOVER, strlen(MARK_RECOVER), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RECOVER;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_CAMRECTS, strlen(MARK_CAMRECTS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_CAMRECTS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "En", param_value, 2048) >= 0) 
				iValue[1] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Group", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PosX1", param_value, 2048) >= 0) 
				iValue[3] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PosY1", param_value, 2048) >= 0) 
				iValue[4] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PosX2", param_value, 2048) >= 0) 
				iValue[5] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "PosY2", param_value, 2048) >= 0) 
				iValue[6] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "RectID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[7], param_value, 4);
					else memcpy(&iValue[7], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_SHOW, strlen(MARK_SHOW), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_SHOW;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RECOVER, strlen(MARK_RECOVER), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RECOVER;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_DIRECTORIES, strlen(MARK_DIRECTORIES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_DIRECTORIES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = 0;
		if (ret <= 0) 
		{
			ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_SAVE;
		}
		if (ret <= 0)  
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret <= 0)  
		{
			ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_PLAY;
		}
		if (ret <= 0)  
		{
			ret = SearchData(MARK_OPEN, strlen(MARK_OPEN), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_OPEN;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Access", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "RemoteAccess", param_value, 2048) >= 0) 
				iValue[2] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[256], param_value, 255);
					else memcpy(&strValue[256], param_value, strlen(param_value));
			}
			
			if (WEB_get_param_from_url(msg, msg_len, "MinSpace", param_value, 2048) >= 0) 
				iValue[3] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "BeginPath", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[512], param_value, 255);
					else memcpy(&strValue[512], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "MaxFileSize", param_value, 2048) >= 0) 
			{
				iValue[4] = Str2Int(param_value);
				if ((iValue[4] < 3) || (iValue[4] > 1000)) iValue[4] = 30;
			}
			if (WEB_get_param_from_url(msg, msg_len, "MaxFileTime", param_value, 2048) >= 0) 
			{
				iValue[5] = Str2Int(param_value);
				if ((iValue[5] < 60) || (iValue[5] > 220000)) iValue[5] = 300;
			}
			if (WEB_get_param_from_url(msg, msg_len, "BackUpPath", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[768], param_value, 255);
					else memcpy(&strValue[768], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveSelectType", param_value, 2048) >= 0) 
			{
				iValue[6] = Str2Int(param_value);
				if ((iValue[6] < 0) || (iValue[6] > 3)) iValue[6] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ArchivePath", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[1024], param_value, 255);
					else memcpy(&strValue[1024], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveTimeFrom", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));
				iValue[7] = Str2Int(param_value);
				if ((iValue[7] < 0) || (iValue[7] > 2359)) iValue[7] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveTimeTo", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));
				iValue[8] = Str2Int(param_value);
				if ((iValue[8] < 0) || (iValue[8] > 2359)) iValue[8] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "DirType", param_value, 2048) >= 0) 
			{				
				iValue[9] = Str2Int(param_value);
				if ((iValue[9] < 0) || (iValue[9] > DIRECTORY_TYPE_OTHER)) iValue[9] = DIRECTORY_TYPE_OTHER;
			}
			if (WEB_get_param_from_url(msg, msg_len, "BackUpMode", param_value, 2048) >= 0) 
			{				
				iValue[10] = Str2Int(param_value);
				if ((iValue[10] < 0) || (iValue[10] > 2)) iValue[10] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "BackUpMaxOrderLen", param_value, 2048) >= 0) 
			{				
				iValue[11] = Str2Int(param_value);
				if ((iValue[11] < 1) || (iValue[11] > 20)) iValue[11] = 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "BackUpUseCopyService", param_value, 2048) >= 0) 
				iValue[12] = SearchStrInDataCaseIgn(param_value, 10, 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "BackUpIPCopyService", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 31) memcpy(&strValue[64], param_value, 31);
					else memcpy(&strValue[64], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "BackUpMaxWaitCancel", param_value, 2048) >= 0) 
				iValue[13] = Str2Int(param_value);		
			if (WEB_get_param_from_url(msg, msg_len, "BackUpMaxWaitMess", param_value, 2048) >= 0) 
				iValue[14] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveMaxOrderLen", param_value, 2048) >= 0) 
			{				
				iValue[15] = Str2Int(param_value);
				if ((iValue[15] < 1) || (iValue[15] > 20)) iValue[15] = 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveUseCopyService", param_value, 2048) >= 0) 
				iValue[16] = SearchStrInDataCaseIgn(param_value, 10, 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveIPCopyService", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 31) memcpy(&strValue[128], param_value, 31);
					else memcpy(&strValue[128], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveMaxWaitCancel", param_value, 2048) >= 0) 
				iValue[17] = Str2Int(param_value);		
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveMaxWaitMess", param_value, 2048) >= 0) 
				iValue[18] = Str2Int(param_value);		
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveMode", param_value, 2048) >= 0) 
			{
				iValue[19] = Str2Int(param_value);
				if ((iValue[19] < 0) || (iValue[19] > 2)) iValue[19] = 0;
			}
			if (WEB_get_param_from_url(msg, msg_len, "BackUpStorage", param_value, 2048) >= 0) 
				iValue[20] = SearchStrInDataCaseIgn(param_value, 10, 0, "ON") ? 1 : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "ArchiveStorage", param_value, 2048) >= 0) 
				iValue[21] = SearchStrInDataCaseIgn(param_value, 10, 0, "ON") ? 1 : 0;
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RECOVER, strlen(MARK_RECOVER), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RECOVER;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}

	ret = SearchData(MARK_WIDGETS, strlen(MARK_WIDGETS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_WIDGETS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}		
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "NewPos", param_value, 2048) >= 0) 
				iValue[28] = Str2Int(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "WidgetID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[1], param_value, 4);
					else memcpy(&iValue[1], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Enabled", param_value, 2048) >= 0) 
				iValue[21] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			
			iValue[2] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "SM01", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? WIDGET_SHOWMODE_DAYTIME : 0;
			if (WEB_get_param_from_url(msg, msg_len, "SM02", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? WIDGET_SHOWMODE_MENU : 0;
			if (WEB_get_param_from_url(msg, msg_len, "SM03", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? WIDGET_SHOWMODE_TIMEOUT : 0;
			if (WEB_get_param_from_url(msg, msg_len, "SM04", param_value, 2048) >= 0) 
				iValue[2] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? WIDGET_SHOWMODE_ALLWAYS : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Repeat", param_value, 2048) >= 0) 
				iValue[20] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "SM03_Time", param_value, 2048) >= 0) 
				iValue[19] = Str2Int(param_value);
			
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[3] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[768], param_value, 255);
					else memcpy(&strValue[768], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Scale", param_value, 2048) >= 0) 
				flValue[0] = Str2Float(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Direct", param_value, 2048) >= 0) 
				iValue[4] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Speed", param_value, 2048) >= 0) 
				flValue[1] = Str2Float(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "SetInt", param_value, 2048) >= 0) 
				iValue[5] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Refresh", param_value, 2048) >= 0) 
				iValue[6] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Mo", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Tu", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 2 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "We", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 4 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Th", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 8 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Fr", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 16 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Sa", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 32 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Su", param_value, 2048) >= 0) 
				iValue[7] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 64 : 0;	
			if (WEB_get_param_from_url(msg, msg_len, "Hour11", param_value, 2048) >= 0) 
				iValue[8] = Str2Int(param_value)*10000; else iValue[8] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min11", param_value, 2048) >= 0) 
				iValue[8] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec11", param_value, 2048) >= 0) 
				iValue[8] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Hour12", param_value, 2048) >= 0) 
				iValue[9] = Str2Int(param_value)*10000; else iValue[9] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min12", param_value, 2048) >= 0) 
				iValue[9] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec12", param_value, 2048) >= 0) 
				iValue[9] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Hour21", param_value, 2048) >= 0) 
				iValue[10] = Str2Int(param_value)*10000; else iValue[10] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min21", param_value, 2048) >= 0) 
				iValue[10] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec21", param_value, 2048) >= 0) 
				iValue[10] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Hour22", param_value, 2048) >= 0) 
				iValue[11] = Str2Int(param_value)*10000; else iValue[11] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min22", param_value, 2048) >= 0) 
				iValue[11] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec22", param_value, 2048) >= 0) 
				iValue[11] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Hour31", param_value, 2048) >= 0) 
				iValue[12] = Str2Int(param_value)*10000; else iValue[12] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min31", param_value, 2048) >= 0) 
				iValue[12] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec31", param_value, 2048) >= 0) 
				iValue[12] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Hour32", param_value, 2048) >= 0) 
				iValue[13] = Str2Int(param_value)*10000; else iValue[13] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min32", param_value, 2048) >= 0) 
				iValue[13] += Str2Int(param_value)*100;
			if (WEB_get_param_from_url(msg, msg_len, "Sec32", param_value, 2048) >= 0) 
				iValue[13] += Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "SelfPt", param_value, 2048) >= 0) 
				iValue[14] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
				else iValue[14] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "SrcID1", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[15], param_value, 4);
					else memcpy(&iValue[15], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "SrcCell1", param_value, 2048) >= 0) 
				iValue[16] = Str2Int(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "Coef", param_value, 2048) >= 0) 
				flValue[2] = Str2Float(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "ShowFrom1", param_value, 2048) >= 0) 
				flValue[3] = Str2Float(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "ShowTo1", param_value, 2048) >= 0) 
				flValue[4] = Str2Float(param_value);			
			
			if (WEB_get_param_from_url(msg, msg_len, "Color", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 6) memcpy(&strValue[64], param_value, 6);
					else memcpy(&strValue[256], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "ValTpNm", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 9) memcpy(&strValue[512], param_value, 9);
					else memcpy(&strValue[512], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Offset", param_value, 2048) >= 0) 
				iValue[18] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "DirectX", param_value, 2048) >= 0) 
				flValue[5] = Str2Float(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "DirectY", param_value, 2048) >= 0) 
				flValue[6] = Str2Float(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Angle", param_value, 2048) >= 0) 
				flValue[7] = Str2Float(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "SrcID2", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[22], param_value, 4);
					else memcpy(&iValue[22], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "SrcCell2", param_value, 2048) >= 0) 
				iValue[23] = Str2Int(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "ShowFrom2", param_value, 2048) >= 0) 
				iValue[24] = Str2Int(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "ShowTo2", param_value, 2048) >= 0) 
				iValue[25] = Str2Int(param_value);
			/*if (WEB_get_param_from_url(msg, msg_len, "ShowIf1", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[26], param_value, 4);
					else memcpy(&iValue[26], param_value, strlen(param_value));
			}*/
			if (WEB_get_param_from_url(msg, msg_len, "ShowIf2", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[27], param_value, 4);
					else memcpy(&iValue[27], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "ShowCamera", param_value, 2048) >= 0) 
				iValue[29] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON");			
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RECOVER, strlen(MARK_RECOVER), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RECOVER;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_GET_UP, strlen(MARK_GET_UP), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_UP;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_GET_DOWN, strlen(MARK_GET_DOWN), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DOWN;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
	}
	
	ret = SearchData(MARK_STREAMS, strlen(MARK_STREAMS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_STREAMS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
			else 
			{
				ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
				if (ret > 0) *uiAction = WEB_ACT_PLAY;
			}
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Access", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "OverTCP", param_value, 2048) >= 0) 
				iValue[3] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON");
			if (WEB_get_param_from_url(msg, msg_len, "ID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[4], param_value, 4);
					else memcpy(&iValue[4], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "VideoEn", param_value, 2048) >= 0) 
				iValue[5] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON");
			if (WEB_get_param_from_url(msg, msg_len, "AudioEn", param_value, 2048) >= 0) 
				iValue[6] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON");
			if (WEB_get_param_from_url(msg, msg_len, "NewPos", param_value, 2048) >= 0) 
				iValue[10] = Str2Int(param_value);			
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Url", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(&strValue[64], param_value, 255);
					else memcpy(&strValue[64], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_GET_UP, strlen(MARK_GET_UP), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_UP;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_GET_DOWN, strlen(MARK_GET_DOWN), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DOWN;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);			
		}
		ret = SearchData(MARK_FILTER, strlen(MARK_FILTER), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_FILTER;
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_STREAMTYPES, strlen(MARK_STREAMTYPES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_STREAMTYPES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_MAILS, strlen(MARK_MAILS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MAILS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Group", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Address1", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[0], param_value, 63);
					else memcpy(&strValue[0], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Address2", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[64], param_value, 63);
					else memcpy(&strValue[64], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "MainText", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[128], param_value, 63);
					else memcpy(&strValue[128], param_value, strlen(param_value));
			}	
			if (WEB_get_param_from_url(msg, msg_len, "BodyText", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[192], param_value, 63);
					else memcpy(&strValue[192], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Path1", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));
				if (strlen(param_value) > 63) memcpy(&strValue[256], param_value, 63);
					else memcpy(&strValue[256], param_value, strlen(param_value));				
			}
			if (WEB_get_param_from_url(msg, msg_len, "Path2", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[320], param_value, 63);
					else memcpy(&strValue[320], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Auth", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 63) memcpy(&strValue[384], param_value, 63);
					else memcpy(&strValue[384], param_value, strlen(param_value));
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_SOUNDS, strlen(MARK_SOUNDS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_SOUNDS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
			else
			{
				ret = SearchData(MARK_PLAY, strlen(MARK_PLAY), msg, msg_len, 0);
				if (ret > 0) *uiAction = WEB_ACT_PLAY;
			}
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "ID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[1], param_value, 4);
					else memcpy(&iValue[1], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(strValue, param_value, 255);
					else memcpy(strValue, param_value, strlen(param_value));
			}					
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_ALARMS, strlen(MARK_ALARMS), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_ALARMS;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		else 
		{
			ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
			if (ret > 0) *uiAction = WEB_ACT_ADD;
		}
		if (ret > 0) 
		{
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "En", param_value, 2048) >= 0) 
				iValue[1] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Hour", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[2] = Str2Int(param_value) * 10000;
			} else iValue[2] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "Min", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[2] += Str2Int(param_value) * 100;
			}
			if (WEB_get_param_from_url(msg, msg_len, "Sec", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[2] += Str2Int(param_value);
			}
			if (WEB_get_param_from_url(msg, msg_len, "Mo", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Tu", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 2 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "We", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 4 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Th", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 8 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Fr", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 16 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Sa", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 32 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Su", param_value, 2048) >= 0) 
				iValue[3] |= SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 64 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Skip", param_value, 2048) >= 0) 
				iValue[4] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "Limit", param_value, 2048) >= 0) 
				iValue[5] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Vol", param_value, 2048) >= 0) 
				iValue[6] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Time", param_value, 2048) >= 0) 
				iValue[7] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Path", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(strValue, param_value, 255);
					else memcpy(strValue, param_value, strlen(param_value));				
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	ret = SearchData(MARK_MODULES, strlen(MARK_MODULES), msg, msg_len, 0);
	if (ret == 0) 
	{
		*uiType = WEB_MSG_MODULES;
		ret = SearchData(MARK_LIST, strlen(MARK_LIST), msg, msg_len, 0);
		if (ret > 0) 
		{			
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
			*pPage = Str2Int(param_value);
		}
		ret = SearchData(MARK_FILTER, strlen(MARK_FILTER), msg, msg_len, 0);
		if (ret > 0) 	
		{
			*uiAction = WEB_ACT_FILTER;
			if (WEB_get_param_from_url(msg, msg_len, "Enabled", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);	
			if (WEB_get_param_from_url(msg, msg_len, "Area", param_value, 2048) >= 0) 
				iValue[1] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
				iValue[2] = Str2Int(param_value);
		}
		ret = SearchData(MARK_ADD, strlen(MARK_ADD), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_ADD;
			if (WEB_get_param_from_url(msg, msg_len, "Page", param_value, 2048) >= 0) 
				*pPage = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "En", param_value, 2048) >= 0) 
				iValue[1] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[2] = Str2Int(param_value);
			}
			if ((iValue[2] <= MODULE_TYPE_UNKNOWN) || (iValue[2] >= MODULE_TYPE_MAX)) 
			{
				WEB_AddMessageInList("Error recv module type");
				return - 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[3], param_value, 4);
					else memcpy(&iValue[3], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(strValue, param_value, 255);
					else memcpy(strValue, param_value, strlen(param_value));				
			}
			if (WEB_get_param_from_url(msg, msg_len, "ViewLevel", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[4] = Str2Int(param_value);
				if ((iValue[4] < 0) || (iValue[4] > 1000)) iValue[4] = 0;
			} else iValue[4] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "AccessLevel", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[5] = Str2Int(param_value);
				if ((iValue[5] < 0) || (iValue[5] > 1000)) iValue[5] = 0;
			} else iValue[5] = 0;
			/*if (WEB_get_param_from_url(msg, msg_len, "Global", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[6] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;			
			} else iValue[6] = 0;*/
			if (WEB_get_param_from_url(msg, msg_len, "ScanSet", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[7] = Str2Int(param_value);
				if ((iValue[7] < 0) || (iValue[7] > 1000000)) iValue[7] = 0;
			} else iValue[7] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "SaveChanges", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[8] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			} else iValue[8] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "GenEvents", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[9] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			} else iValue[9] = 0;
		}
		ret = SearchData(MARK_SAVE, strlen(MARK_SAVE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_SAVE;
		ret = SearchData(MARK_CHANGE, strlen(MARK_CHANGE), msg, msg_len, 0);
		if (ret > 0) *uiAction = WEB_ACT_CHANGE;
		if ((*uiAction == WEB_ACT_SAVE) || (*uiAction == WEB_ACT_CHANGE))
		{
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
			if (WEB_get_param_from_url(msg, msg_len, "En", param_value, 2048) >= 0) 
				iValue[1] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			if (WEB_get_param_from_url(msg, msg_len, "Type", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[2] = Str2Int(param_value);
			}
			if ((iValue[2] <= MODULE_TYPE_UNKNOWN) || (iValue[2] >= MODULE_TYPE_MAX)) 
			{
				WEB_AddMessageInList("Error recv module type");
				return - 1;
			}
			if (WEB_get_param_from_url(msg, msg_len, "ID", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 4) memcpy(&iValue[3], param_value, 4);
					else memcpy(&iValue[3], param_value, strlen(param_value));
			}
			if (WEB_get_param_from_url(msg, msg_len, "Name", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				if (strlen(param_value) > 255) memcpy(strValue, param_value, 255);
					else memcpy(strValue, param_value, strlen(param_value));				
			}
			if (WEB_get_param_from_url(msg, msg_len, "ViewLevel", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[4] = Str2Int(param_value);
				if ((iValue[4] < 0) || (iValue[4] > 1000)) iValue[4] = 0;
			} else iValue[4] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "AccessLevel", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[5] = Str2Int(param_value);
				if ((iValue[5] < 0) || (iValue[5] > 1000)) iValue[5] = 0;
			} else iValue[5] = 0;
			/*if (WEB_get_param_from_url(msg, msg_len, "Global", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[6] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;			
			} else iValue[6] = 0;*/
			if (WEB_get_param_from_url(msg, msg_len, "ScanSet", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[7] = Str2Int(param_value);
				if ((iValue[7] < 0) || (iValue[7] > 1000000)) iValue[7] = 0;
			} else iValue[7] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "SaveChanges", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[8] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			} else iValue[8] = 0;
			if (WEB_get_param_from_url(msg, msg_len, "GenEvents", param_value, 2048) >= 0) 
			{
				WEB_decode_url_to_src(param_value, strlen(param_value));	
				iValue[9] = SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON") ? 1 : 0;
			} else iValue[9] = 0;
			
			switch(iValue[2])
			{
				case MODULE_TYPE_CAMERA:
					if (WEB_get_param_from_url(msg, msg_len, "SubID", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (strlen(param_value) > 4) memcpy(&iValue[10], param_value, 4);
						else memcpy(&iValue[10], param_value, strlen(param_value));
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "ResolCameraW", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
						if ((iValue[11] < 120) || (iValue[11] > 2048)) iValue[11] = 160;
						iValue[11] <<= 16;
					} else iValue[11] = 160 << 16;
					if (WEB_get_param_from_url(msg, msg_len, "ResolCameraH", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						int iH = Str2Int(param_value);						
						if ((iH < 120) || (iH > 2048)) iH = 120;
						iValue[11] |= iH;
					} else iValue[11] |= 120;
					if (WEB_get_param_from_url(msg, msg_len, "MainFrameRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 2) || (iValue[12] > 190)) iValue[12] = 2;
					} else iValue[12] = 2;
					if (WEB_get_param_from_url(msg, msg_len, "MainKeyFrame", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
						if ((iValue[13] < 0) || (iValue[13] > 190)) iValue[13] = 60;
					} else iValue[13] = 60;
					if (WEB_get_param_from_url(msg, msg_len, "MainBitRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
						if ((iValue[14] < 1) || (iValue[14] > 99999)) iValue[14] = 5000;
					} else iValue[14] = 5000;
					if (WEB_get_param_from_url(msg, msg_len, "ResolDetectW", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[15] = Str2Int(param_value);
						if ((iValue[15] < 120) || (iValue[15] > 2048)) iValue[15] = 160;
						iValue[15] <<= 16;
					} else iValue[15] = 160 << 16;
					if (WEB_get_param_from_url(msg, msg_len, "ResolDetectW", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						int iH = Str2Int(param_value);						
						if ((iH < 120) || (iH > 2048)) iH = 120;
						iValue[15] |= iH;
					} else iValue[15] |= 120;
					if (WEB_get_param_from_url(msg, msg_len, "DetectLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[16] = Str2Int(param_value);
						if ((iValue[16] < 0) || (iValue[16] > 255)) iValue[16] = 20;
					} else iValue[16] = 20;
					if (WEB_get_param_from_url(msg, msg_len, "DiffRecLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[17] = Str2Int(param_value);
						if ((iValue[17] < 0) || (iValue[17] > 255)) iValue[17] = 20;
					} else iValue[17] = 20;							
					if (WEB_get_param_from_url(msg, msg_len, "HLAmplif", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[18] |= (Str2Int(param_value) & 255) << 24;
					}
					if (WEB_get_param_from_url(msg, msg_len, "HLBright", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[18] |= (Str2Int(param_value) & 255) << 16;
					}
					if (WEB_get_param_from_url(msg, msg_len, "HighLight", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 16384;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "Resources", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[18] |= (Str2Int(param_value) & 2) << 12;
					}
					if (WEB_get_param_from_url(msg, msg_len, "AutoBright", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 2048;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "SlowBitType", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (Str2Int(param_value) == 1) iValue[18] |= 1024;
					}					
					if (WEB_get_param_from_url(msg, msg_len, "PrevBitType", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (Str2Int(param_value) == 1) iValue[18] |= 512;
					}
					if (WEB_get_param_from_url(msg, msg_len, "MainBitType", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (Str2Int(param_value) == 1) iValue[18] |= 256;
					}
					if (WEB_get_param_from_url(msg, msg_len, "FlipVer", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 128;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "FlipHor", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 64;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "RecNorm", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 32;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "RecDiff", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 16;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "RecSlow", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 8;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "Preview", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 4;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "SlowDiff", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 2;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "SlowCoder", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] |= 1;
					}	
					if (WEB_get_param_from_url(msg, msg_len, "SkipFrames", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[19] = Str2Int(param_value);
						if ((iValue[19] < 1) || (iValue[19] > 1000)) iValue[19] = 60;
					} else iValue[19] = 60;
					if (WEB_get_param_from_url(msg, msg_len, "SaveFramesDiff", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[20] = Str2Int(param_value);
						if ((iValue[20] < 1) || (iValue[20] > 1000)) iValue[20] = 10;
					} else iValue[20] = 10;
					if (WEB_get_param_from_url(msg, msg_len, "SaveFramesSlow", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[21] = Str2Int(param_value);
						if ((iValue[21] < 1) || (iValue[21] > 1000)) iValue[21] = 10;
					} else iValue[21] = 10;		
					if (WEB_get_param_from_url(msg, msg_len, "SlowRecLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[22] = Str2Int(param_value);
						if ((iValue[22] < 0) || (iValue[22] > 255)) iValue[22] = 20;
					} else iValue[22] = 20;	
					if (WEB_get_param_from_url(msg, msg_len, "DiffRecSize", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[23] = Str2Int(param_value);
						if ((iValue[23] < 0) || (iValue[23] > 2000)) iValue[23] = 0;
					} else iValue[23] = 0;		
					if (WEB_get_param_from_url(msg, msg_len, "SlowRecSize", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[24] = Str2Int(param_value);
						if ((iValue[24] < 0) || (iValue[24] > 2000)) iValue[24] = 0;
					} else iValue[24] = 0;	
					if (WEB_get_param_from_url(msg, msg_len, "FastConn", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[25] = 1;
					} else iValue[25] = 0;	
					if (WEB_get_param_from_url(msg, msg_len, "ColorLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[26] = Str2Int(param_value);
						if ((iValue[26] < 0) || (iValue[26] > 255)) iValue[26] = 20;
					} else iValue[26] = 20;		
					if (WEB_get_param_from_url(msg, msg_len, "PrevBitRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[27] = Str2Int(param_value);
						if ((iValue[27] < 1) || (iValue[27] > 99999)) iValue[27] = 5000;
					} else iValue[27] = 5000;
					if (WEB_get_param_from_url(msg, msg_len, "SlowBitRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[28] = Str2Int(param_value);
						if ((iValue[28] < 1) || (iValue[28] > 99999)) iValue[28] = 5000;
					} else iValue[28] = 5000;
					if (WEB_get_param_from_url(msg, msg_len, "MainAvcProfile", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[29] |= Str2Int(param_value) & 255;
					}
					if (WEB_get_param_from_url(msg, msg_len, "PrevAvcProfile", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[29] |= (Str2Int(param_value) & 255) << 8;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SlowAvcProfile", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[29] |= (Str2Int(param_value) & 255) << 16;
					}
					if (WEB_get_param_from_url(msg, msg_len, "MainAvcLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[30] = Str2Int(param_value) & 255;
					}
					if (WEB_get_param_from_url(msg, msg_len, "PrevAvcLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[30] |= (Str2Int(param_value) & 255) << 8;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SlowAvcLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[30] |= (Str2Int(param_value) & 255) << 16;
					}
					if (WEB_get_param_from_url(msg, msg_len, "PrevKeyFrame", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[31] = Str2Int(param_value);
						if ((iValue[31] < 0) || (iValue[31] > 190)) iValue[31] = 60;
					} else iValue[31] = 60;
					if (WEB_get_param_from_url(msg, msg_len, "SlowKeyFrame", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[32] = Str2Int(param_value);
						if ((iValue[32] < 0) || (iValue[32] > 190)) iValue[32] = 60;
					} else iValue[32] = 60;	
					if (WEB_get_param_from_url(msg, msg_len, "SlowFrameRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[33] = Str2Int(param_value);
						if ((iValue[33] < 2) || (iValue[33] > 190)) iValue[33] = 2;
					} else iValue[33] = 2;	
					if (WEB_get_param_from_url(msg, msg_len, "Exposure", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[34] = GetStandartExposure(Str2Int(param_value));						
					} else iValue[34] = GetStandartExposure(0);	
					if (WEB_get_param_from_url(msg, msg_len, "StartBright", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[35] = Str2Int(param_value);
						if ((iValue[35] < 0) || (iValue[35] > 100)) iValue[35] = 50;
					} else iValue[35] = 50;	
					if (WEB_get_param_from_url(msg, msg_len, "DestBright", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[37] = Str2Int(param_value);
						if ((iValue[37] < 0) || (iValue[37] > 100)) iValue[37] = 50;
					} else iValue[37] = 50;	
					if (WEB_get_param_from_url(msg, msg_len, "RotMode", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[36] = Str2Int(param_value) & 3;
					} else iValue[36] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Filter", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[38] = GetStandartImageFilter(Str2Int(param_value));						
					} else iValue[38] = GetStandartImageFilter(0);	
					if (WEB_get_param_from_url(msg, msg_len, "Balance", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[39] = GetStandartWhiteBalance(Str2Int(param_value));						
					} else iValue[39] = GetStandartWhiteBalance(0);
					if (WEB_get_param_from_url(msg, msg_len, "Contrast", param_value, 2048) >= 0) 
					{
						iValue[40] = Str2Int(param_value);	
						if ((iValue[40] < -100) || (iValue[40] > 100)) iValue[40] = 0;
					} else iValue[40] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Sharpness", param_value, 2048) >= 0) 
					{
						iValue[41] = Str2Int(param_value);	
						if ((iValue[41] < -100) || (iValue[41] > 100)) iValue[41] = 0;
					} else iValue[41] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Saturation", param_value, 2048) >= 0) 
					{
						iValue[42] = Str2Int(param_value);	
						if ((iValue[42] < -100) || (iValue[42] > 100)) iValue[42] = 0;
					} else iValue[42] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "LongSensor", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[46] |= Str2Int(param_value) & 7;						
					} else iValue[46] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "LongStep", param_value, 2048) >= 0) 
					{
						iValue[43] = Str2Int(param_value);	
						if ((iValue[43] < 0) || (iValue[43] > 10000)) iValue[43] = 100;
					} else iValue[43] = 100;
					if (WEB_get_param_from_url(msg, msg_len, "LongBrigLevel", param_value, 2048) >= 0) 
					{
						iValue[44] = Str2Int(param_value);	
						if ((iValue[44] < 0) || (iValue[44] > 255)) iValue[44] = 20;
					} else iValue[44] = 20;
					if (WEB_get_param_from_url(msg, msg_len, "LongColLevel", param_value, 2048) >= 0) 
					{
						iValue[45] = Str2Int(param_value);	
						if ((iValue[45] < 0) || (iValue[45] > 255)) iValue[45] = 20;
					} else iValue[45] = 20;
					if (WEB_get_param_from_url(msg, msg_len, "ResolPrevW", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[47] = Str2Int(param_value);
						if ((iValue[47] < 120) || (iValue[47] > 2048)) iValue[47] = 160;	
						iValue[47] <<= 16;
					} else iValue[47] = 160 << 16;
					if (WEB_get_param_from_url(msg, msg_len, "ResolPrevH", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						int iH = Str2Int(param_value);						
						if ((iH < 120) || (iH > 2048)) iH = 120;
						iValue[47] |= iH;					
					} else iValue[47] |= 120;
					if (iValue[47] < iValue[15]) iValue[15] = iValue[47];
					if (WEB_get_param_from_url(msg, msg_len, "LeftCrop", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[48] = Str2Int(param_value);
						if ((iValue[48] < 0) || (iValue[48] > 100)) iValue[48] = 0;						
					} else iValue[48] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "RightCrop", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[49] = Str2Int(param_value);
						if ((iValue[49] < 0) || (iValue[49] > 100)) iValue[49] = 0;						
					} else iValue[49] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "UpCrop", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[50] = Str2Int(param_value);
						if ((iValue[50] < 0) || (iValue[50] > 100)) iValue[50] = 0;						
					} else iValue[50] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DownCrop", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[51] = Str2Int(param_value);
						if ((iValue[51] < 0) || (iValue[51] > 100)) iValue[51] = 0;						
					} else iValue[51] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PtzEn", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[52] |= 1;				
					} else iValue[52] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PtzFocus", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[52] |= 2;				
					}
					if (WEB_get_param_from_url(msg, msg_len, "PtzFocusMap", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[52] |= 8;				
					}
					if (WEB_get_param_from_url(msg, msg_len, "KeepRatio", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[52] |= 4;				
					}	
					if (WEB_get_param_from_url(msg, msg_len, "PtzID", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (strlen(param_value) > 4) memcpy(&iValue[53], param_value, 4);
						else memcpy(&iValue[53], param_value, strlen(param_value));			
					} else iValue[53] = 0;
					
					iValue[54] = 0;			
					if (WEB_get_param_from_url(msg, msg_len, "AutoISOhw", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[54] |= 1;
					}
					if (WEB_get_param_from_url(msg, msg_len, "AutoISOsw", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[54] |= 2;
					}
					if (WEB_get_param_from_url(msg, msg_len, "AutoEVsw", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[54] |= 4;
					}
					if (WEB_get_param_from_url(msg, msg_len, "ISOvalue", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[55] = Str2Int(param_value);
						if ((iValue[55] < 100) || (iValue[55] > 800)) iValue[55] = 400;
					} else iValue[55] = 400;
					if (WEB_get_param_from_url(msg, msg_len, "DestISOBright", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[56] = Str2Int(param_value);
						if ((iValue[56] < 1) || (iValue[56] > 100)) iValue[56] = 50;
					} else iValue[56] = 50;
					if (WEB_get_param_from_url(msg, msg_len, "EVvalue", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[57] = Str2Int(param_value);
						if ((iValue[57] < -24) || (iValue[57] > 24)) iValue[57] = 0;
					} else iValue[57] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DestEVBright", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[58] = Str2Int(param_value);
						if ((iValue[58] < 1) || (iValue[58] > 100)) iValue[58] = 50;
					} else iValue[58] = 50;
					
					iValue[59] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PtzHomeTimeWait", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[59] = Str2Int(param_value) & 0b111111111111;				
					}
					if (WEB_get_param_from_url(msg, msg_len, "PtzHomeStart", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[59] |= 1 << 12;				
					}
					if (WEB_get_param_from_url(msg, msg_len, "PtzHomeReturn", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[59] |= 1 << 13;				
					}							
					break;
				case MODULE_TYPE_TEMP_SENSOR:
					if (WEB_get_param_from_url(msg, msg_len, "Model", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
						if ((iValue[10] != I2C_ADDRESS_AM2320) && (iValue[10] != I2C_ADDRESS_LM75)) iValue[10] = I2C_ADDRESS_LM75;
					} else iValue[10] = I2C_ADDRESS_LM75;
					if (WEB_get_param_from_url(msg, msg_len, "Address", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
						if ((iValue[11] < 0) || (iValue[11] > 7)) iValue[11] = 0;
					} else iValue[11] = 0;
					break;
				case MODULE_TYPE_MCP3421:
					if (WEB_get_param_from_url(msg, msg_len, "Address", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
						if ((iValue[10] < 0) || (iValue[10] > 7)) iValue[10] = 0;
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Resolution", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
						if ((iValue[11] < 0) || (iValue[11] > 3)) iValue[11] = 0;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Gain", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 0) || (iValue[12] > 3)) iValue[12] = 0;
					} else iValue[12] = 0;
					break;
				case MODULE_TYPE_ADS1015:
					if (WEB_get_param_from_url(msg, msg_len, "Address", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
						if ((iValue[10] < 0) || (iValue[10] > 3)) iValue[10] = 0;
					} else iValue[10] = 0;					
					if (WEB_get_param_from_url(msg, msg_len, "Gain", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 0) || (iValue[12] > 7)) iValue[12] = 0;
					} else iValue[12] = 0;
					break;
				case MODULE_TYPE_AS5600:
					if (WEB_get_param_from_url(msg, msg_len, "Address", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
						if ((iValue[10] < 0) || (iValue[10] > 7)) iValue[10] = 0;
					} else iValue[10] = 0;
					break;
				case MODULE_TYPE_HMC5883L:
					break;
				case MODULE_TYPE_GPIO:
					if (WEB_get_param_from_url(msg, msg_len, "Mode", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (Str2Int(param_value) == 1) iValue[10] = MODULE_SECSET_OUTPUT; else iValue[10] = 0;
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Inverse", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= MODULE_SECSET_INVERSE;
					}
					if (WEB_get_param_from_url(msg, msg_len, "PinNum", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
						if ((iValue[11] < 0) || (iValue[11] > 80)) iValue[11] = 0;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Time", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 0) || (iValue[12] > 20000000)) iValue[12] = 0;
					} else iValue[12] = 0;
					break;
				case MODULE_TYPE_COUNTER:
					if (WEB_get_param_from_url(msg, msg_len, "Res1", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] = 0x01000000;
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res2", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[11] = 0x01000000;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res3", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[12] = 0x01000000;							
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res4", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[13] = 0x01000000;		
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res5", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[14] = 0x01000000;		
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res6", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[15] = 0x01000000;		
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res7", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] = 0x01000000;		
					} else iValue[16] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Res8", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[17] = 0x01000000;		
					} else iValue[17] = 0;	
					
					if (WEB_get_param_from_url(msg, msg_len, "Set1", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[10] & 0x00FFFFFF) == 0) iValue[10] = 0;
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set2", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[11] & 0x00FFFFFF) == 0) iValue[11] = 0;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set3", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[12] & 0x00FFFFFF) == 0) iValue[12] = 0;
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set4", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[13] & 0x00FFFFFF) == 0) iValue[13] = 0;
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set5", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[14] & 0x00FFFFFF) == 0) iValue[14] = 0;
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set6", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[15] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[15] & 0x00FFFFFF) == 0) iValue[15] = 0;
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set7", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[16] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[16] & 0x00FFFFFF) == 0) iValue[16] = 0;
					} else iValue[16] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set8", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[17] |= Str2Int(param_value) & 0x00FFFFFF;
						if ((iValue[17] & 0x00FFFFFF) == 0) iValue[17] = 0;
					} else iValue[17] = 0;					
					break;
				case MODULE_TYPE_SYSTEM:
				case MODULE_TYPE_DISPLAY:
				case MODULE_TYPE_MEMORY:					
					if (WEB_get_param_from_url(msg, msg_len, "Set1", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set2", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set3", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set4", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set5", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set6", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[15] = Str2Int(param_value);
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set7", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[16] = Str2Int(param_value);
					} else iValue[16] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Set8", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[17] = Str2Int(param_value);
					} else iValue[17] = 0;					
					break;
				case MODULE_TYPE_TIMER:
					if (WEB_get_param_from_url(msg, msg_len, "Mo", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 1;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Tu", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 2;
					}
					if (WEB_get_param_from_url(msg, msg_len, "We", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 4;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Th", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 8;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Fr", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 16;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Sa", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 32;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Su", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[10] |= 64;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Hour", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value) * 10000;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Min", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] += Str2Int(param_value) * 100;
					}
					if (WEB_get_param_from_url(msg, msg_len, "Sec", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] += Str2Int(param_value);
					}
					break;
				case MODULE_TYPE_IR_RECEIVER:
					if (WEB_get_param_from_url(msg, msg_len, "ScanClk", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PinNum", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 0) || (iValue[12] > 80)) iValue[12] = 0;
					} else iValue[12] = 0;					
					break;
				case MODULE_TYPE_RTC:
					if (WEB_get_param_from_url(msg, msg_len, "Speed", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if (iValue[12] < 0) iValue[12] = 0;
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DateDay", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
						if (iValue[13] < 0) iValue[13] = 0;
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DateMont", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
						if (iValue[14] < 0) iValue[14] = 0;
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DateYear", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[15] = Str2Int(param_value);
						if (iValue[15] < 0) iValue[15] = 0;
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "TimeHour", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[16] = Str2Int(param_value);
						if (iValue[16] < 0) iValue[16] = 0;
					} else iValue[16] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "TimeMin", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[17] = Str2Int(param_value);
						if (iValue[17] < 0) iValue[17] = 0;
					} else iValue[17] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "TimeSec", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[18] = Str2Int(param_value);
						if (iValue[18] < 0) iValue[18] = 0;
					} else iValue[18] = 0;
					break;
				case MODULE_TYPE_RADIO:				
					if (WEB_get_param_from_url(msg, msg_len, "Speed", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if (iValue[12] < 0) iValue[12] = 0;
					} else iValue[12] = 0;
					break;
				case MODULE_TYPE_USB_GPIO:
					//////0>>>>>>>>>>>>>
					iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PortNum", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] |= (Str2Int(param_value) & USBIO_MAIN_SETT_PORT_NUM_MASK) << 16;
					}
					if (WEB_get_param_from_url(msg, msg_len, "AtSc", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
							iValue[10] |= USBIO_MAIN_SETT_AUTOSCAN;
					}
					if (WEB_get_param_from_url(msg, msg_len, "StMd", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
							iValue[10] |= USBIO_MAIN_SETT_SAVE_IN_MODULE;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SensCnt", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						iValue[10] |= (Str2Int(param_value) & 0xFF) << 8;
					}
					//////0<<<<<<<<<<<<
					//////1>>>>>>>>>>>>>
					if (WEB_get_param_from_url(msg, msg_len, "PtSp", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						iValue[11] = Str2Int(param_value);
					} else iValue[11] = 0;
										
					if (WEB_get_param_from_url(msg, msg_len, "SensSel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						iValue[12] = Str2Int(param_value);
					} else iValue[12] = 0;
					//////1<<<<<<<<<<<<
					if (WEB_get_param_from_url(msg, msg_len, "SensPort", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						iValue[13] = Str2Int(param_value) & 0b00111111;
					} else iValue[13] = 0;				
					if (WEB_get_param_from_url(msg, msg_len, "SensEnabl", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
							iValue[14] = 1;
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensInit", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) 
							iValue[15] = 1;
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensPeriod", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						iValue[16] |= Str2Int(param_value) & 0xFFFF;
					} else iValue[16] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensType", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						iValue[17] = Str2Int(param_value) & 0xFF;
					} else iValue[17] = 0;					
					if (WEB_get_param_from_url(msg, msg_len, "SensSett1", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[18] = 1;
							else iValue[18] = Str2Int(param_value);
					} else iValue[18] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensSett2", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[19] = 1;
							else iValue[19] = Str2Int(param_value);
					} else iValue[19] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensSett3", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[20] = 1;
							else iValue[20] = Str2Int(param_value);
					} else iValue[20] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensSett4", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[21] = 1;
							else iValue[21] = Str2Int(param_value);
					} else iValue[21] = 0;	
					if (WEB_get_param_from_url(msg, msg_len, "SensSett5", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[22] = 1;
							else iValue[22] = Str2Int(param_value);
					} else iValue[22] = 0;	
					if (WEB_get_param_from_url(msg, msg_len, "SensSett6", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[23] = 1;
							else iValue[23] = Str2Int(param_value);
					} else iValue[23] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensSett7", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[24] = 1;
							else iValue[24] = Str2Int(param_value);
					} else iValue[24] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SensSett8", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[25] = 1;
							else iValue[25] = Str2Int(param_value);
					} else iValue[25] = 0;
					break;
				case MODULE_TYPE_EXTERNAL:				
					if (WEB_get_param_from_url(msg, msg_len, "TpConn", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Addr", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Speed", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PortNum", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
					} else iValue[13] = 0;
					break;
				case MODULE_TYPE_TFP625A:
					if (WEB_get_param_from_url(msg, msg_len, "SubID", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (strlen(param_value) > 4) memcpy(&iValue[10], param_value, 4);
						else memcpy(&iValue[10], param_value, strlen(param_value));
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PortNum", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PtSl", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
						if (iValue[14] > 1000000) iValue[14] = 0;
					} else iValue[14] = 0;
					break;
				case MODULE_TYPE_RS485:	
				case MODULE_TYPE_PN532:
				case MODULE_TYPE_RC522:				
					if (WEB_get_param_from_url(msg, msg_len, "SubID", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (strlen(param_value) > 4) memcpy(&iValue[10], param_value, 4);
						else memcpy(&iValue[10], param_value, strlen(param_value));
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PortNum", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Speed", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
						if (iValue[13] < 0) iValue[13] = 0;
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "PtSl", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
						if (iValue[14] > 1000000) iValue[14] = 0;
					} else iValue[14] = 0;
					break;
				case MODULE_TYPE_MIC:				
					if (WEB_get_param_from_url(msg, msg_len, "Channels", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[10] = Str2Int(param_value);
						if ((iValue[10] < 0) || (iValue[10] > 7)) iValue[10] = 0;
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Dev1", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
						if ((iValue[11] < 0) || (iValue[11] > 9)) iValue[11] = 0;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Dev2", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 0) || (iValue[12] > 9)) iValue[12] = 0;
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SampleRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
						if ((iValue[13] < 0) || (iValue[13] > 96000)) iValue[13] = 0;
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "BitRate", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
						if ((iValue[14] < 0) || (iValue[14] > 320000)) iValue[14] = 0;
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "ScanClk", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[15] = Str2Int(param_value);
						if ((iValue[15] < 0) || (iValue[15] > 1024)) iValue[15] = 0;
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SenceSet", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[16] = Str2Int(param_value);
						if ((iValue[16] < 0) || (iValue[16] > 1024)) iValue[16] = 0;
					} else iValue[16] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "RecNorm", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[17] = 1;
					} else iValue[17] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "RecVidDiff", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[17] |= 2;
					}
					if (WEB_get_param_from_url(msg, msg_len, "AGC", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[17] |= 4;
					}					
					if (WEB_get_param_from_url(msg, msg_len, "SenceSkipSet", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[18] = Str2Int(param_value);
						if ((iValue[18] < 0) || (iValue[18] > 100)) iValue[18] = 0;
					} else iValue[18] = 0;
					/*if (WEB_get_param_from_url(msg, msg_len, "RecSplit", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[19] |= 1;
					}*/
					if (WEB_get_param_from_url(msg, msg_len, "RecDiff", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[20] = 1;
					} else iValue[20] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "RecLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[21] = Str2Int(param_value);
						if ((iValue[21] < 0) || (iValue[21] > 1024)) iValue[21] = 0;
					} else iValue[21] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DiffSkip", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[22] = Str2Int(param_value);
						if ((iValue[22] < 0) || (iValue[22] > 1000)) iValue[22] = 0;
					} else iValue[22] = 0;		
					if (WEB_get_param_from_url(msg, msg_len, "SaveFrames", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[23] = Str2Int(param_value);
						if ((iValue[23] < 0) || (iValue[23] > 1000)) iValue[23] = 0;
					} else iValue[23] = 0;			
					if (WEB_get_param_from_url(msg, msg_len, "ScanPeriod", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[24] = Str2Int(param_value);
						if ((iValue[24] < 0) || (iValue[24] > 1024)) iValue[24] = 0;
					} else iValue[24] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Amplifier", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[25] = Str2Int(param_value);
						if ((iValue[25] < 0) || (iValue[25] > 100)) iValue[25] = 0;
					} else iValue[25] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Codec", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[26] = Str2Int(param_value);
						//if ((iValue[26] < 0) || (iValue[26] > 5)) iValue[26] = 0;
					} else iValue[26] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Convert", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[27] = Str2Int(param_value);
						if ((iValue[27] < 0) || (iValue[27] > 2)) iValue[27] = 0;
					} else iValue[27] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "DigLevel", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[28] = Str2Int(param_value);
						if ((iValue[28] < 1) || (iValue[28] > 14)) iValue[28] = 1;
					} else iValue[28] = 1;
					break;
				case MODULE_TYPE_SPEAKER:				
					if (WEB_get_param_from_url(msg, msg_len, "SubID", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));
						if (strlen(param_value) > 4) memcpy(&iValue[10], param_value, 4);
						else memcpy(&iValue[10], param_value, strlen(param_value));				
					} else iValue[10] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Dev1", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[11] = Str2Int(param_value);
						if ((iValue[11] < 0) || (iValue[11] > 9)) iValue[11] = 0;
					} else iValue[11] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Dev2", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[12] = Str2Int(param_value);
						if ((iValue[12] < 0) || (iValue[12] > 9)) iValue[12] = 0;
					} else iValue[12] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Channels", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[13] = Str2Int(param_value);
						if ((iValue[13] < 0) || (iValue[13] > 7)) iValue[13] = 0;
					} else iValue[13] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Streams", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[14] = Str2Int(param_value);
						if ((iValue[14] < 0) || (iValue[14] > 20)) iValue[14] = 0;
					} else iValue[14] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "Timer", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						iValue[15] = Str2Int(param_value);
						if ((iValue[15] < 0) || (iValue[15] > 120000)) iValue[15] = 0;
					} else iValue[15] = 0;
					if (WEB_get_param_from_url(msg, msg_len, "SR8", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] |= SAMPLE_RATE_8000;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SR11", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] |= SAMPLE_RATE_11025;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SR22", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] |= SAMPLE_RATE_22050;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SR44", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] |= SAMPLE_RATE_44100;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SR48", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] |= SAMPLE_RATE_48000;
					}
					if (WEB_get_param_from_url(msg, msg_len, "SR96", param_value, 2048) >= 0) 
					{
						WEB_decode_url_to_src(param_value, strlen(param_value));	
						if (SearchStrInDataCaseIgn(param_value, strlen(param_value), 0, "ON")) iValue[16] |= SAMPLE_RATE_96000;
					}
					break;					
				default:
					break;
			}
		}
		ret = SearchData(MARK_DELETE, strlen(MARK_DELETE), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_DELETE;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
		ret = SearchData(MARK_RECOVER, strlen(MARK_RECOVER), msg, msg_len, 0);
		if (ret > 0) 
		{
			*uiAction = WEB_ACT_RECOVER;
			if (WEB_get_param_from_url(msg, msg_len, "Num", param_value, 2048) >= 0) 
				iValue[0] = Str2Int(param_value);
		}
	}
	
	if ((*uiAction) != WEB_ACT_NOTHING)
	{
		ret = SearchData(MARK_REQUEST_ID, strlen(MARK_REQUEST_ID), msg, msg_len, 0);
		if (ret > 0)
		{
			if (WEB_get_param_from_url(msg, msg_len, MARK_REQUEST_ID, param_value, 2048) >= 0) 
			*uiRequestId = Str2Int(param_value);
		}
	}
	DBG_FREE(msg);
	return 1;
}

unsigned int WEB_test_access(unsigned int uiType, unsigned int uiAccess)
{
	if ((uiType == WEB_MSG_MENU) && (!(uiAccess & ACCESS_MENU))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_ALARMS) && (!(uiAccess & ACCESS_ALARMS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_MODULES) && (!(uiAccess & ACCESS_MODULES))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_SOUNDS) && (!(uiAccess & ACCESS_SOUNDS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_MAILS) && (!(uiAccess & ACCESS_MAILS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_STREAMTYPES) && (!(uiAccess & ACCESS_STREAMTYPES))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_STREAMS) && (!(uiAccess & ACCESS_STREAMS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_WIDGETS) && (!(uiAccess & ACCESS_WIDGETS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_DIRECTORIES) && (!(uiAccess & ACCESS_DIRECTORIES))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_MNLACTIONS) && (!(uiAccess & ACCESS_MNLACTIONS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_EVNTACTIONS) && (!(uiAccess & ACCESS_EVNTACTIONS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_KEYS) && (!(uiAccess & ACCESS_KEYS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_IRCODES) && (!(uiAccess & ACCESS_IRCODES))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_CAMRECTS) && (!(uiAccess & ACCESS_CAMRECTS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_CONTROL) && (!(uiAccess & ACCESS_CONTROL))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_YOUTUBE) && (!(uiAccess & ACCESS_YOUTUBE))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_MEDIA) && (!(uiAccess & ACCESS_MEDIA))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_MANUAL) && (!(uiAccess & ACCESS_MANUAL))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_RADIOS) && (!(uiAccess & ACCESS_RADIOS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_LOG) && (!(uiAccess & ACCESS_LOG))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_EXPLORER) && (!(uiAccess & ACCESS_EXPLORER))) uiType = WEB_MSG_LOGOUT;
	if ((uiType == WEB_MSG_VIEWER) && (!(uiAccess & ACCESS_EXPLORER))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_CAMERA) && (!(uiAccess & ACCESS_CAMERA))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_MIC) && (!(uiAccess & ACCESS_MIC))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_SYSTEM) && (!(uiAccess & ACCESS_SYSTEM))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_CONNECTS) && (!(uiAccess & ACCESS_CONNECTS))) uiType = WEB_MSG_LOGOUT;	
	if ((uiType == WEB_MSG_HISTORY) && (!(uiAccess & WEB_MSG_HISTORY))) uiType = WEB_MSG_LOGOUT;	
	return uiType;
}

char* WEB_GetWebMsgTypeName(unsigned int msg_type)
{
	switch(msg_type)
	{
		case WEB_MSG_UNKNOWN: return "UNKNOWN";
		case WEB_MSG_START: return "START";
		case WEB_MSG_GET_ICO: return "GET_ICO";
		case WEB_MSG_CONTROL: return "CONTROL";
		case WEB_MSG_YOUTUBE: return "YOUTUBE";
		case WEB_MSG_MEDIA: return "MEDIA";
		case WEB_MSG_ERROR: return "ERROR";
		case WEB_MSG_TEARDOWN: return "TEARDOWN";
		case WEB_MSG_MENU: return "MENU";
		case WEB_MSG_ALARMS: return "ALARMS";
		case WEB_MSG_MODULES: return "MODULES";
		case WEB_MSG_SOUNDS: return "SOUNDS";
		case WEB_MSG_MAILS: return "MAILS";
		case WEB_MSG_STREAMTYPES: return "STREAMTYPES";
		case WEB_MSG_STREAMS: return "STREAMS";
		case WEB_MSG_WIDGETS: return "WIDGETS";
		case WEB_MSG_DIRECTORIES: return "DIRECTORIES";
		case WEB_MSG_MNLACTIONS: return "MNLACTIONS";
		case WEB_MSG_EVNTACTIONS: return "EVNTACTIONS";
		case WEB_MSG_KEYS: return "KEYS";
		case WEB_MSG_IRCODES: return "IRCODES";
		case WEB_MSG_CAMRECTS: return "CAMRECTS";
		case WEB_MSG_SKIPIRCODES: return "SKIPIRCODES";
		case WEB_MSG_ALIENKEYS: return "ALIENKEYS";
		case WEB_MSG_SKIPEVENTS: return "SKIPEVENTS";
		case WEB_MSG_USERS: return "USERS";
		case WEB_MSG_RADIOS: return "RADIOS";
		case WEB_MSG_LOGOUT: return "LOGOUT";
		case WEB_MSG_MANUAL: return "MANUAL";
		case WEB_MSG_MODSTATUSES: return "MODSTATUSES";
		case WEB_MSG_SETTINGS: return "SETTINGS";
		case WEB_MSG_LOG: return "LOG";
		case WEB_MSG_EXPLORER: return "EXPLORER";
		case WEB_MSG_VIEWER: return "VIEWER";
		case WEB_MSG_IMAGES: return "IMAGES";
		case WEB_MSG_CAMERA: return "CAMERA";
		case WEB_MSG_MIC: return "MIC";
		case WEB_MSG_SYSTEM: return "SYSTEM";
		case WEB_MSG_CONNECTS: return "CONNECTS";
		case WEB_MSG_HISTORY: return "HISTORY";
		default:
			return "UNKNOWN";
	}
	return "UNKNOWN";
}

void PrintListValues(void *pData, char cValueSize, unsigned int uiDataLength, unsigned int uiMaxLenght, char* OutBuffer, unsigned int uiOutBufferSize)
{	
	memset(OutBuffer, 0, uiOutBufferSize);
	if ((cValueSize != 1) && (cValueSize != 2) && (cValueSize != 4)) return;
	if (uiMaxLenght < uiDataLength) return;
	uint8_t *pData8 = (uint8_t*)pData;
	uint16_t *pData16 = (uint16_t*)pData;
	int *pData32 = (int*)pData;
	char buff[32];
	int i;
	int iOutSize = 0;
	
	for (i = 0; i < uiDataLength; i++)
	{
		memset(buff, 0, 32);
		if (cValueSize == 1) snprintf(buff, 32, "%i", pData8[i]);
		if (cValueSize == 2) snprintf(buff, 32, "%i", pData16[i]);
		if (cValueSize == 4) snprintf(buff, 32, "%i", pData32[i]);
		iOutSize += strlen(buff) + 1;
		if (iOutSize > uiOutBufferSize) break;
		strcat(OutBuffer, buff);
		if (i < (uiDataLength - 1)) strcat(OutBuffer, ",");
	}
}


char* WEB_GetWebActTypeName(unsigned int msg_act)
{
	switch(msg_act)
	{
		case WEB_ACT_NOTHING: return "NOTHING";
		case WEB_ACT_PLAY: return "PLAY";
		case WEB_ACT_NEXT: return "NEXT";
		case WEB_ACT_PREV: return "PREV";
		case WEB_ACT_STOP: return "STOP";
		case WEB_ACT_SHOW: return "SHOW";
		case WEB_ACT_PAGE: return "PAGE";
		case WEB_ACT_SAVE: return "SAVE";
		case WEB_ACT_ADD: return "ADD";
		case WEB_ACT_DELETE: return "DELETE";
		case WEB_ACT_RECOVER: return "RECOVER";
		case WEB_ACT_SET_VOLUME: return "SET_VOLUME";
		case WEB_ACT_SET_PLAY_POS: return "SET_PLAY_POS";
		case WEB_ACT_GET_INFO: return "GET_INFO";
		case WEB_ACT_EXECUTE: return "EXECUTE";
		case WEB_ACT_CHANGE: return "CHANGE";
		case WEB_ACT_OPEN: return "OPEN";
		case WEB_ACT_REFRESH: return "REFRESH";
		case WEB_ACT_WORK_ACCEPT: return "WORK_ACCEPT";
		case WEB_ACT_WORK_REQUEST: return "WORK_REQUEST";
		case WEB_ACT_IMAGE_CAMRECT: return "IMAGE_CAMRECT";
		case WEB_ACT_IMAGE_CAMPREV: return "IMAGE_CAMPREV";
		case WEB_ACT_IMAGE_CAMMAIN: return "IMAGE_CAMMAIN";
		case WEB_ACT_OFF: return "OFF";
		case WEB_ACT_REBOOT: return "REBOOT";
		case WEB_ACT_RESTART: return "RESTART";
		case WEB_ACT_EXIT: return "EXIT";
		case WEB_ACT_UPDATE: return "UPDATE";
		case WEB_ACT_CONNECT: return "CONNECT";
		case WEB_ACT_DISCONNECT: return "DISCONNECT";
		default:
			return "UNKNOWN";
	}
	return "UNKNOWN";
}

int WEB_main_form(char *cBuff, int iAuth, unsigned int uiAccess)
{
	int i = 0;
	char cDate[64];	
	GetDateTimeStr(cDate, 64);
	strcat(cBuff, cDate);
	strcat(cBuff, "<br />");
	
	strcat(cBuff, "<input type='button' value='СООБЩЕНИЯ' onclick=\"window.location.href='/messages/'\" style='width: 200px;'>\r\n");
	i++;
	if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	
	if (uiAccess & ACCESS_CONTROL) 
	{
		strcat(cBuff, "<input type='button' value='УПРАВЛЕНИЕ' onclick=\"window.location.href='/control/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_YOUTUBE) 
	{
		strcat(cBuff, "<input type='button' value='YOUTUBE' onclick=\"window.location.href='/youtube/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MEDIA) 
	{
		strcat(cBuff, "<input type='button' value='МЕДИА' onclick=\"window.location.href='/media/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MENU) 
	{
		strcat(cBuff, "<input type='button' value='МЕНЮ' onclick=\"window.location.href='/menu/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_ALARMS) 
	{
		strcat(cBuff, "<input type='button' value='БУДИЛЬНИКИ' onclick=\"window.location.href='/alarms/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_RADIOS) 
	{
		strcat(cBuff, "<input type='button' value='РАДИО' onclick=\"window.location.href='/radios/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MODULES) 
	{
		strcat(cBuff, "<input type='button' value='МОДУЛИ НАСТРОЙКА' onclick=\"window.location.href='/modules/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MODSTATUSES) 
	{
		strcat(cBuff, "<input type='button' value='МОДУЛИ СОСТОЯНИЕ' onclick=\"window.location.href='/modstatuses/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_SOUNDS) 
	{
		strcat(cBuff, "<input type='button' value='ЗВУКИ' onclick=\"window.location.href='/sounds/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MAILS) 
	{
		strcat(cBuff, "<input type='button' value='ПОЧТА' onclick=\"window.location.href='/mails/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_STREAMTYPES) 
	{
		strcat(cBuff, "<input type='button' value='ТИПЫ ПОТОКОВ' onclick=\"window.location.href='/streamtypes/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_STREAMS) 
	{
		strcat(cBuff, "<input type='button' value='ПОТОКИ' onclick=\"window.location.href='/streams/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_WIDGETS) 
	{
		strcat(cBuff, "<input type='button' value='ВИДЖЕТЫ' onclick=\"window.location.href='/widgets/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_DIRECTORIES) 
	{
		strcat(cBuff, "<input type='button' value='ПАПКИ' onclick=\"window.location.href='/directories/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MNLACTIONS) 
	{
		strcat(cBuff, "<input type='button' value='ДЕЙСТВИЯ РУЧНЫЕ' onclick=\"window.location.href='/mnlactions/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_EVNTACTIONS) 
	{
		strcat(cBuff, "<input type='button' value='ДЕЙСТВИЯ НА СОБЫТИЯ' onclick=\"window.location.href='/evntactions/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_KEYS) 
	{
		strcat(cBuff, "<input type='button' value='КЛЮЧИ КАРТ' onclick=\"window.location.href='/keys/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_IRCODES) 
	{
		strcat(cBuff, "<input type='button' value='КОДЫ ПУЛЬТА' onclick=\"window.location.href='/ircodes/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_CAMRECTS) 
	{
		strcat(cBuff, "<input type='button' value='КВАДРАТЫ КАМЕРЫ' onclick=\"window.location.href='/camrects/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_USERS) 
	{
		strcat(cBuff, "<input type='button' value='ПОЛЬЗОВАТЕЛИ' onclick=\"window.location.href='/users/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MANUAL) 
	{
		strcat(cBuff, "<input type='button' value='ИНСТРУКЦИЯ' onclick=\"window.location.href='/manual/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_SETTINGS) 
	{
		strcat(cBuff, "<input type='button' value='НАСТРОЙКИ' onclick=\"window.location.href='/settings/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_LOG) 
	{
		strcat(cBuff, "<input type='button' value='ЛОГ' onclick=\"window.location.href='/log/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_EXPLORER) 
	{
		strcat(cBuff, "<input type='button' value='ПРОВОДНИК' onclick=\"window.location.href='/explorer/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
		strcat(cBuff, "<input type='button' value='ПРОСМОТР' onclick=\"window.location.href='/viewer/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_CAMERA) 
	{
		strcat(cBuff, "<input type='button' value='КАМЕРА' onclick=\"window.location.href='/camera/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_MIC) 
	{
		strcat(cBuff, "<input type='button' value='МИКРОФОН' onclick=\"window.location.href='/mic/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_CONNECTS) 
	{
		strcat(cBuff, "<input type='button' value='ПОДКЛЮЧЕНИЯ' onclick=\"window.location.href='/connects/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_HISTORY) 
	{
		strcat(cBuff, "<input type='button' value='ИСТОРИЯ ПОДКЛЮЧЕНИЙ' onclick=\"window.location.href='/history/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (uiAccess & ACCESS_SYSTEM) 
	{
		strcat(cBuff, "<input type='button' value='СИСТЕМА' onclick=\"window.location.href='/system/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	if (iAuth) 
	{
		strcat(cBuff, "<input type='button' value='ВЫЙТИ' onclick=\"window.location.href='/logout/'\" style='width: 200px;'>\r\n");
		i++;
		if (i > 3){ strcat(cBuff, "<br />\r\n"); i = 0;}
	}
	//strcat(cBuff, "<br />\r\n");
	return 1;			
}


int WEB_main_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
					
	strcat(msg_tx, session->head);
	//printf(msg_tx);
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");

	return 1;
}

int WEB_control_execute(int *pParams)
{
	DBG_MUTEX_LOCK(&system_mutex);
	switch(pParams[0])
	{
		case 0: cCurRandomFile = 0; break;
		case 1: cCurRandomFile = 1; break;
		case 2: cCurRandomDir = 0; break;
		case 3: cCurRandomDir = 1; break;
		default: break;
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);
	return 1;
}

int WEB_control_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"

					"</head>"
					"<body>\r\n");
					
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/control/\"><h1>УПРАВЛЕНИЕ</h1></a><br />\r\n");
	WEB_GetMessageList(msg_tx);
	
	unsigned int uiMiscData[3];
	GetPlayBufferStatus(&uiMiscData[0], &uiMiscData[1], &uiMiscData[2]);
	uiMiscData[0] = audio_get_playback_volume();
	
	char cPlayType[32];
	char cPlayName[256];
	DBG_MUTEX_LOCK(&modulelist_mutex);
	memcpy(cPlayType, cCurrentPlayType, 32);
	memcpy(cPlayName, cCurrentPlayName, 256);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	memset(msg_subbody, 0, 65536);
	DBG_MUTEX_LOCK(&system_mutex);
	sprintf(msg_subbody, 	
					"Состояние:%s%s%s%s%s%s%s%s%s%s%s%s<br />\r\n"
					"Тип:%s<br />\r\n"
					"Папка:%s<br />\r\n"
					"Название:%s<br />\r\n"
					"<form action='/control/set_play_pos'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"Позиция воспроизведения: <input type='range' name='playpos' min='0' max='1000' step='1' value='%i' style='width: 300px;'>\r\n"
					"<input type='submit' value='Сохранить'>\r\n"
					"</form>\r\n"
					"<form action='/control/set_volume'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"Громкость: <input type='range' name='volpos' min='0' max='100' step='1' value='%i' style='width: 300px;'>\r\n"
					"<input type='submit' value='Сохранить'><br />\r\n"
					"<br />\r\n"
					"<input type='button' value='Предыдущий' onclick=\"window.location.href='/control/prev?req_index=%i'\">\r\n"
					"<input type='button' value='Следующий' onclick=\"window.location.href='/control/next?req_index=%i'\">\r\n"					
					"<input type='button' value='Остановить' onclick=\"window.location.href='/control/stop?req_index=%i'\">\r\n"					
					"<input type='button' value='Пауза' onclick=\"window.location.href='/control/pause?req_index=%i'\">\r\n"					
					"<input type='button' value='Продолжить' onclick=\"window.location.href='/control/play?req_index=%i'\"><br />\r\n"					
					"<br />\r\n"
					"<input type='button' value='Предыдущая папка' onclick=\"window.location.href='/control/refresh?req_index=%i&direct=1'\">\r\n"					
					"<input type='button' value='Следующая папка' onclick=\"window.location.href='/control/refresh?req_index=%i&direct=0'\">\r\n"					
					"<br />\r\n"
					"<input type='button' value='Выключить показ' onclick=\"window.location.href='/control/show?req_index=%i&mode=0'\"%s>\r\n"					
					"<input type='button' value='Сон' onclick=\"window.location.href='/control/show?req_index=%i&mode=1'\"%s>\r\n"					
					"<input type='button' value='Включить показ' onclick=\"window.location.href='/control/show?req_index=%i&mode=2'\"%s>\r\n", 
					cCurShowType & SHOW_TYPE_FILE ? "Файл;" : "",
					cCurShowType & SHOW_TYPE_URL ? "Ссылка;" : "",
					cCurShowType & SHOW_TYPE_WEB_STREAM ? "Поток;" : "",
					cCurShowType & SHOW_TYPE_IMAGE ? "Картинка;" : "",
					cCurShowType & SHOW_TYPE_VIDEO ? "Видео;" : "",
					cCurShowType & SHOW_TYPE_AUDIO ? "Аудио;" : "",
					cCurShowType & SHOW_TYPE_CAMERA ? "Камера;" : "",
					cCurShowType & SHOW_TYPE_ALARM1 ? "Будильник;" : "",
					cCurShowType & SHOW_TYPE_MIC_STREAM ? "Микрофон;" : "",
					cCurShowType & SHOW_TYPE_OFF ? "Отключено;" : "",
					cCurShowType & SHOW_TYPE_RADIO_STREAM ? "Радио;" : "",
					cCurShowType & SHOW_TYPE_CAMERA_LIST ? "Камеры;" : "",
					cPlayType,
					cSettShowDirNameText,
					cPlayName,
					session->request_id, uiMiscData[2],
					session->request_id, uiMiscData[0],
					session->request_id, session->request_id, session->request_id, session->request_id, 
					session->request_id, session->request_id, session->request_id,
					session->request_id, (uiShowModeCur == 0) ? " disabled " : "",
					session->request_id, (uiShowModeCur == 1) ? " disabled " : "",
					session->request_id, (uiShowModeCur == 2) ? " disabled " : "");
	strcat(msg_tx,	msg_subbody);
	
	strcat(msg_tx,	"<br /><br /><br />Список воспроизведения<br />\r\n");
	memset(msg_subbody, 0, 65536);
	
	if (cCurRandomDir) 
		sprintf(msg_subbody, "<input type='button' value='Включить последовательный порядок папок' onclick=\"window.location.href='/control/execute?req_index=%i&mode=2'\">\r\n", session->request_id);
		else
		sprintf(msg_subbody, "<input type='button' value='Включить случайный порядок папок' onclick=\"window.location.href='/control/execute?req_index=%i&mode=3'\">\r\n", session->request_id);	
	strcat(msg_tx,	msg_subbody);
	
	if (cCurRandomFile) 
		sprintf(msg_subbody, "<input type='button' value='Включить последовательный порядок файлов' onclick=\"window.location.href='/control/execute?req_index=%i&mode=0'\"><br /><br />\r\n", session->request_id);
		else
		sprintf(msg_subbody, "<input type='button' value='Включить случайный порядок файлов' onclick=\"window.location.href='/control/execute?req_index=%i&mode=1'\"><br /><br />\r\n", session->request_id);	
	strcat(msg_tx,	msg_subbody);
					
	int i;
	unsigned int blen;
	char cbuff[128];
	char *cCursor;
	char *cStatus;
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='2'><tr><th>Папки</th><th>Файлы</th></tr><tr><td valign='top'>");
	for (i = 0; i < iListDirsCount; i++)
	{
		if (i == iListDirsCurPos) cCursor = "***"; else cCursor = "---";
		cStatus = "_";
		if (cListDirs[i].Flag == -1) cStatus = "X";
		if (cListDirs[i].Flag > 0) cStatus = "V";
		memset(cbuff, 0, 128);
		blen = 61;
		utf8_to_cp866(cListDirs[i].Name, (strlen(cListDirs[i].Name) < blen) ? strlen(cListDirs[i].Name) : 60, cbuff, &blen);
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, "%s [%s] <a href=\"/control/change?req_index=%i&type=0&position=%i\">%s</a><br />\r\n", 
							cCursor, cStatus, session->request_id, i, cbuff);
		strcat(msg_tx,	msg_subbody);
		if ((strlen(msg_tx) + 512) > WEB_TX_BUF_SIZE_MAX) 
		{
			strcat(msg_tx,	" . . . <br />");
			break;
		}
	}
	strcat(msg_tx,	"</td><td valign='top'>");
	for (i = 0; i < iListFilesCount; i++)
	{
		if (i == iListFilesCurPos) cCursor = "***"; else cCursor = "---";
		cStatus = "_";
		if (cListFiles[i].Flag == -1) cStatus = "X";
		if (cListFiles[i].Flag > 0) cStatus = "V";
		memset(cbuff, 0, 128);
		blen = 61;
		utf8_to_cp866(cListFiles[i].Name, (strlen(cListFiles[i].Name) < blen) ? strlen(cListFiles[i].Name) : 60, cbuff, &blen);
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, "%s [%s] <a href=\"/control/change?req_index=%i&type=1&position=%i\">%s</a><br />\r\n", 
							cCursor, cStatus, session->request_id, i, cbuff);
		strcat(msg_tx,	msg_subbody);
		if ((strlen(msg_tx) + 512) > WEB_TX_BUF_SIZE_MAX) 
		{
			strcat(msg_tx,	" . . . <br />");
			break;
		}
	}
	strcat(msg_tx,	"</td></tr>");
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");	

	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_youtube_show(char *msg_tx, WEB_SESSION *session, int *pParams, char* strParam)
{
	if (pParams[1] == 0)
	{
		if (strlen(strParam) == 0)
		{		
			WEB_AddMessageInList("URL : NULL");
			return 0;
		}
		if (strlen(strParam) < MAX_FILE_LEN) 
		{
			memset(cYouTubeLastUrl, 0, MAX_FILE_LEN);
			strcpy(cYouTubeLastUrl, strParam);
		}
		if (PlayYouTubeFromURL2(strParam, pParams[0]) == 1)
			WEB_AddMessageInList("Play started");
			else
			WEB_AddMessageInList("Play error");
	}
	else
	{
		if (PlayYouTubeFromURL2(cYouTubeLastUrl, pParams[0]) == 1)
			WEB_AddMessageInList("Play started");
			else
			WEB_AddMessageInList("Play error");
	}
	return 1;
}

int WEB_youtube_update(int *pParams)
{
	char cBuff[256];
	if (pParams[0] == 0) Exec_In_Buff("youtube-dl -U", cBuff, 255);
	if (pParams[0] == 1) Exec_In_Buff("pip install --upgrade youtube-dl", cBuff, 255);
	cBuff[255] = 0;
	WEB_AddMessageInList(cBuff);
	return 1;
}

int WEB_youtube_open(char *msg_tx, WEB_SESSION *session, int *pParams, char* strParam)
{
	if (pParams[1] == 0)
	{
		if (strlen(strParam) == 0)
		{
			WEB_AddMessageInList("URL : NULL");
			return 0;
		}
		if (strlen(strParam) < MAX_FILE_LEN) 
		{
			memset(cYouTubeLastUrl, 0, MAX_FILE_LEN);
			strcpy(cYouTubeLastUrl, strParam);
		}		
		if (PlayYouTubeFromURL(strParam) == 1) 
			WEB_AddMessageInList("Play started");
			else
			WEB_AddMessageInList("Play error");
	}
	else
	{
		if (PlayYouTubeFromURL(cYouTubeLastUrl) == 1) 
			WEB_AddMessageInList("Play started");
			else
			WEB_AddMessageInList("Play error");
	}
	return 1;
}

int WEB_youtube_save(char *msg_rx, int iLen)
{
	char *msg = (char*)DBG_MALLOC(iLen);
	
	if (WEB_get_url(msg_rx, msg, iLen) == 0) 
	{
		WEB_AddMessageInList("WEB_youtube_save: WEB_get_url error");
		DBG_FREE(msg);
		return 0;
	}
	
	char *body = (char*)DBG_MALLOC(iLen);
	int iDL = WEB_get_param_from_url(msg, iLen, "script_body", body, iLen);
	if (iDL >= 0) 
	{
		WEB_decode_url_to_src(body, iDL);
	
		FILE *f;
		if ((f = fopen("youtube.pl","w")) == NULL)
		{
			dbgprintf(1, "Error save:youtube.pl\n");			
		}
		else
		{
			iDL = strlen(body);
			
			int iPos = SearchStrInData(body, iDL, 0, "print STDERR \"$progname: reading");
			if (iPos > 0)
			{
				iPos -= 1;
				if (fwrite(body, 1, iPos, f) != iPos)
				{
					WEB_AddMessageInList("Write JWZ script Error 1");
					dbgprintf(2, "Error 1 write:youtube.pl\n");
				} 
				else 
				{
					fputs("print STDOUT \"$url2\"; exit 0;  \n\t", f);
					iDL -= iPos;
					if (fwrite(&body[iPos], 1, iDL, f) != iDL)
					{
						WEB_AddMessageInList("Write JWZ script Error 2");
						dbgprintf(2, "Error 2 write:youtube.pl\n");
					} else WEB_AddMessageInList("Write JWZ script OK");
				}
			}
			else
			{
				WEB_AddMessageInList("Not found key JWZ script");
				dbgprintf(2, "Not found key JWZ script\n");
			}
			fclose(f);
		}
	}
	
	DBG_FREE(msg);
	DBG_FREE(body);
	return 1;
}

int WEB_youtube_load(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	char *cBuff = (char*)DBG_MALLOC(2048);
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset='utf-8'"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");					
	strcat(msg_tx, session->head);
	memset(cBuff, 0,  2048);
	sprintf(cBuff, 	"<h1>Окно загрузки JWZ скрипта</h1><br />"
					"<form action='/youtube/save'>"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<p><textarea  rows='10' cols='245' name='script_body'></textarea></p>"
					"<p><input type='submit'></p>"
					"</form>\r\n", session->request_id);
	strcat(msg_tx, cBuff);	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	DBG_FREE(cBuff);
	return 1;
}

int WEB_youtube_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");					
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/youtube/\"><h1>YOUTUBE</h1></a><br />\r\n");
	WEB_GetMessageList(msg_tx);
	
	char * cTypesHtml = (char*)DBG_MALLOC(16384);
	char cBuff[256];
	memset(cTypesHtml, 0, 16384);
	int n;
	strcpy(cTypesHtml, "<select name='type' style='width: 300;'>\r\n");
	for (n = 0; n < 350; n++)
	{
		if (WEB_get_youtube_content_name(n) != NULL)
		{
			memset(cBuff, 0, 256);
			sprintf(cBuff, "		<option %s value='%i'>%s</option>\r\n", (n == cYouTubeLastType) ? " selected " : "", n, WEB_get_youtube_content_name(n));
			strcat(cTypesHtml, 	cBuff);										
		}
	}
	strcat(cTypesHtml, "	</select>\r\n");
	
	char *cBody = (char*)DBG_MALLOC(20048);
	memset(cBody, 0, 20048);	
	sprintf(cBody, 	"<a href='https://www.jwz.org/hacks/youtubedown' target='_blank'>Открыть JWZ скрипт в новом окне</a><br />\r\n"
					"<a href='/youtube/load?req_index=%i' target='_blank'>Окно загрузки JWZ скрипта</a><br />\r\n"
					"<a href='/youtube/refresh?req_index=%i&Type=0'>Обновить YouTube-dl (способ 1)</a><br />\r\n"
					"<a href='/youtube/refresh?req_index=%i&Type=1'>Обновить YouTube-dl (способ 2)</a><br />\r\n",
					session->request_id, session->request_id, session->request_id);	
	strcat(msg_tx, cBody);
	
	memset(cBody, 0, 20048);
	sprintf(cBody, 	"<form action='/youtube/open'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<p>URL: <input type='text' name='url' value='' style='width: 400;'></p>\r\n"
					"<p>Воспроизвести предыдущий URL <input type='checkbox' name='replay'>: %s</p>\r\n"
					"<input type='submit' value='Открыть через JWZ'>\r\n"
					"<button type='submit' formaction='/youtube/show'>Открыть через YouTube-dl</button>%s\r\n"
					"</form>\r\n", 
					session->request_id, 
					cYouTubeLastUrl, cTypesHtml);
	strcat(msg_tx, cBody);
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	DBG_FREE(cTypesHtml);
	DBG_FREE(cBody);
	return 1;
}

int WEB_media_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");					
	strcat(msg_tx, session->head);
	char *cBody = (char*)DBG_MALLOC(2048);
	memset(cBody, 0, 2048);	
	sprintf(cBody, 	"<a href=\"/media/\"><h1>МЕДИА ПРОИГРЫВАТЕЛЬ</h1></a><br />\r\n"
					"<form action='/media/play'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<p>Ссылка(Путь): <input type='text' name='url' value='%s'></p>\r\n"
					"<input type='submit' value='Воспроизвести'>\r\n"
					"</form>\r\n", 
					session->request_id, 
					session->media_url);
	strcat(msg_tx, cBody);
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	DBG_FREE(cBody);
	return 1;
}

int WEB_menu_prev()
{
	if ((iCurrentWebMenuPage != 0) && (pWebMenu[iCurrentWebMenuPage].Options[pWebMenu[iCurrentWebMenuPage].SelectedOption].PrevPage != -1))
	{
		iCurrentWebMenuPage = pWebMenu[iCurrentWebMenuPage].Options[pWebMenu[iCurrentWebMenuPage].SelectedOption].PrevPage;
		if (pWebMenu[iCurrentWebMenuPage].OpenFunc != NULL) pWebMenu[iCurrentWebMenuPage].OpenFunc(pWebMenu, iCurrentWebMenuPage);
	}
	return 1;
}

int WEB_menu_next(int iNum, int iPos, unsigned int uiAccess)
{
	int n;
	int i = 0;
	if ((iNum >= 0) && (iNum < MAX_MENU_PAGES))
	{
		iCurrentWebMenuPage = iNum;
		if ((iPos >= 0) && (iPos < pWebMenu[iNum].CountOptions))
		{
			i = pWebMenu[iCurrentWebMenuPage].Options[iPos].NextPage;
			pWebMenu[iCurrentWebMenuPage].SelectedOption = iPos;
			if (pWebMenu[iCurrentWebMenuPage].Options[iPos].ActionFunc != NULL)
						pWebMenu[iCurrentWebMenuPage].Options[iPos].ActionFunc(pWebMenu, iCurrentWebMenuPage);
			if (i != 0)
			{
				if (pWebMenu[i].OpenFunc != NULL) pWebMenu[i].OpenFunc(pWebMenu, i);		
				for (n = 0; n < pWebMenu[i].CountOptions; n++)
					if ((pWebMenu[i].Options[n].Show == 1) && (pWebMenu[i].Options[n].ViewLevel <= uiAccess)) break;
				if (n != pWebMenu[i].CountOptions) 
				{
					for (n = 0; n < pWebMenu[i].CountOptions; n++) 
						if (pWebMenu[i].Options[n].PrevPageNoHistory == 0) pWebMenu[i].Options[n].PrevPage = iCurrentWebMenuPage;
					iCurrentWebMenuPage = i;									
				}
			}	
		}
	} else iCurrentWebMenuPage = 0;
	return 1;
}

int WEB_menu_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	char *msg_head = (char*)DBG_MALLOC(2048);
	char *msg_body = (char*)DBG_MALLOC(2048);
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	char *msg_back_btn = (char*)DBG_MALLOC(128);
	char *msg_list = (char*)DBG_MALLOC(16384);
	char *msg_info = (char*)DBG_MALLOC(256);
	
	if (pWebMenu[iCurrentWebMenuPage].RefreshFunc != NULL) pWebMenu[iCurrentWebMenuPage].RefreshFunc(pWebMenu, iCurrentWebMenuPage);
	
	int n;	
	memset(msg_list, 0, 16384);
	for (n = 0; n < pWebMenu[iCurrentWebMenuPage].CountOptions; n++)
	{
		if (strlen(msg_list) < 15000)
		{
			if ((pWebMenu[iCurrentWebMenuPage].Options[n].Show == 1) && (pWebMenu[iCurrentWebMenuPage].Options[n].ViewLevel <= session->access_level))
			{
				memset(msg_back_btn, 0, 128);
				sprintf(msg_back_btn, "<a href=\"/menu/page?req_index=%i&num=%i&pos=%i\">%s</a><br />\r\n", 
					session->request_id,
					iCurrentWebMenuPage, n, pWebMenu[iCurrentWebMenuPage].Options[n].Name);
				strcat(msg_list, msg_back_btn);
			}
		} else break;
	}
	
	memset(msg_back_btn, 0, 128);
	if (iCurrentWebMenuPage > 0) sprintf(msg_back_btn, "<input type=\"button\" value=\"<< BACK\" onclick=\"window.location.href='/menu/prev?req_index=%i'\"><br />\r\n", session->request_id);
	
	WEB_get_module_info(msg_info, 256);
	
	memset(msg_head, 0, 2048);
	strcpy(msg_head,"HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: %i\r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n"
					"%s"
					"\r\n");
	memset(msg_body, 0, 2048);
	strcpy(msg_body,"<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n"
					"%s"
					"<br />\r\n"
					"<h1>МЕНЮ</h1>"
					"<a href=\"/menu/page?req_index=%i&num=0\">На первую страницу</a><br /><br />\r\n"
					"%s"
					"%s<br />\r\n"					
					"%s"
					"</body>\r\n"
					"</html>\r\n");
								
	memset(msg_subbody, 0, 65536);								
	sprintf(msg_subbody, msg_body, session->head, session->request_id, msg_back_btn, msg_info, msg_list);
	sprintf(msg_tx, msg_head, strlen(msg_subbody), msg_subbody);
	
	DBG_FREE(msg_head);
	DBG_FREE(msg_body);
	DBG_FREE(msg_subbody);
	DBG_FREE(msg_back_btn);
	DBG_FREE(msg_list);
	DBG_FREE(msg_info);
	return 1;
}

int WEB_skipevent_del(int *pParams)
{
	ClearSkipEventList();
	return 1;
}

int WEB_skipevent_save(int *pParams)
{
	if ((pParams[0] > 0) && (pParams[0] < 256))
	{
		DBG_MUTEX_LOCK(&skipevent_mutex);
		iSkipEventMaxCnt = pParams[0];
		uiSkipEventListFilter = pParams[1];
		uiSkipEventNumberFilter = pParams[2];
		DBG_MUTEX_UNLOCK(&skipevent_mutex);
	}	
	ClearSkipEventList();
	return 1;
}

int WEB_skipevents_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/skipevents/\"><h1>НОВЫЕ СОБЫТИЯ</h1></a>\r\n");
	
	DBG_MUTEX_LOCK(&skipevent_mutex);
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody,
					"<form action='/skipevents/save'>Максимальное количество записей: \r\n"
					"<input type='number' name='Num' min=1 max=255 value=%i style='width: 60px;'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					" Источник(ID) <input type='text' name='SrcID' maxlength=4 value='%.4s' style='width: 60px;'>\r\n"
					" Номер <input type='number' name='SrcSubNumber' min=0 value=%i style='width: 60px;'>\r\n"
					"<button type='submit'>Сохранить</button>"
					"<button type='submit' formaction='/skipevents/delete?req_index=%i'>Очистить</button></form>\r\n", 
					iSkipEventMaxCnt, session->request_id, (char*)&uiSkipEventListFilter, uiSkipEventNumberFilter, session->request_id);		
	strcat(msg_tx, msg_subbody);
	
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Дата</th><th>ИД</th><th>Номер</th><th>Статус</th><th>Действие</th></tr>");
	
	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iSkipEventListCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurSkipEventPage = iPage;
	iCurPage = iCurSkipEventPage;
	
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurSkipEventPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurSkipEventPage = iCurPage;
	}
	
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iSkipEventListCnt) iLastOption = iSkipEventListCnt;
	
	WEB_pages_list(msg_tx, "skipevents", iSubPages, iCurPage, "");	
	
	char SrcStatusBuff[64];
	char SrcStatusBuff2[64];
	
	for (n = iFirstOption; n < iLastOption; n++)
	{		
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, 
					"<tr><form action='/evntactions/add'><td>%i</td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td>%s</td>\r\n"
					"<td><input type='text' name='SrcID' maxlength=4 value='%.4s' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='SrcSubNumber' min=0 value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='text' name='SrcStatus2' value='%s' style='width: 60px;' disabled>\r\n"
					"<input type='text' name='SrcStatus' maxlength=64 value='%s' style='width: 200px;'></td>\r\n"
					"<input type='hidden' name='Mo' value='on'>\r\n"
					"<input type='hidden' name='Tu' value='on'>\r\n"
					"<input type='hidden' name='We' value='on'>\r\n"
					"<input type='hidden' name='Th' value='on'>\r\n"
					"<input type='hidden' name='Fr' value='on'>\r\n"
					"<input type='hidden' name='Sa' value='on'>\r\n"
					"<input type='hidden' name='Su' value='on'>\r\n"
					"<input type='hidden' name='Hour11' value='0'>\r\n"
					"<input type='hidden' name='Min11' value='0'>\r\n"
					"<input type='hidden' name='Sec11' value='0'>\r\n"
					"<input type='hidden' name='Hour12' value='23'>\r\n"
					"<input type='hidden' name='Min12' value='59'>\r\n"
					"<input type='hidden' name='Sec12' value='59'></td>\r\n"
					"<input type='hidden' name='Page' value=999>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n",
					n,
					session->request_id,
					cSkipEventList[n].Date,
					(char*)&cSkipEventList[n].ID, cSkipEventList[n].SubNumber,
					GetActionCodeName(cSkipEventList[n].Status, SrcStatusBuff, 64, 4),
					GetActionCodeName(cSkipEventList[n].Status, SrcStatusBuff2, 64, 2));		
		strcat(msg_tx, msg_subbody);
	
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	
	DBG_MUTEX_UNLOCK(&skipevent_mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "skipevents", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_alienkey_del(int *pParams)
{
	ClearAlienKeyList();
	return 1;
}

int WEB_alienkey_save(int *pParams)
{
	if ((pParams[0] > 0) && (pParams[0] < 256))
	{
		DBG_MUTEX_LOCK(&alienkey_mutex);
		iAlienKeyMaxCnt = pParams[0];
		DBG_MUTEX_UNLOCK(&alienkey_mutex);
	}	
	ClearAlienKeyList();
	return 1;
}

int WEB_alienkeys_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/alienkeys/\"><h1>НОВЫЕ КЛЮЧИ СМАРТКАРТ</h1></a>\r\n");
	
	DBG_MUTEX_LOCK(&alienkey_mutex);
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody,
					"<form action='/alienkeys/save'>Максимальное количество записей: \r\n"
					"<input type='number' name='Num' min=1 max=255 value=%i style='width: 60px;'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<button type='submit'>Сохранить</button>"
					"<button type='submit' formaction='/alienkeys/delete?req_index=%i'>Очистить</button></form>\r\n", iAlienKeyMaxCnt, session->request_id, session->request_id);		
	strcat(msg_tx, msg_subbody);
	
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>ИД</th><th>Активный</th><th>Создавать события</th><th>Тип</th><th>Серийный номер</th><th>Имя</th><th>Сектор</th>"
					"<th>Проверять код</th><th>Код доступа к сектору</th><th>Проверять содержимое</th><th>Содержимое сектора</th>"
					"<th>Операция</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	char cTempBuff[96];
	iSubPages = (int)ceil((double)iAlienKeyListCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurAlienKeyPage = iPage;
	iCurPage = iCurAlienKeyPage;
	
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurAlienKeyPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurAlienKeyPage = iCurPage;
	}
	
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iAlienKeyListCnt) iLastOption = iAlienKeyListCnt;
	
	WEB_pages_list(msg_tx, "alienkeys", iSubPages, iCurPage, "");	
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		PrintListValues(cAlienKeyList[n].Serial, 1, cAlienKeyList[n].SerialLength, MAX_SECURITY_SERIAL_LEN, cTempBuff, 96);
		memset(msg_subbody, 0, 65536);		
		sprintf(msg_subbody, 
					"<tr><form action='/keys/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='text' name='KeyID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='Enabled'></td>\r\n"
					//"<td><input type='checkbox' name='EvntEnbl'></td>\r\n"
					"<td><select name='Type' style='width: 140px;'>\r\n"
					"		<option %s value='%i'>Смарт карта</option>\r\n"
					"		<option %s disabled value='%i'>Неизвестное</option>\r\n"									
					"	</select></td>\r\n"
					"<td><input type='text' name='Serial' maxlength=64 value='%s' style='width: 300px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=64 value='' style='width: 300px;'></td>\r\n"
					"<td><input type='number' name='Sector' min=0 max=255 value=0 style='width: 45px;'></td>\r\n"
					"<td><input type='checkbox' name='VerKeyA'></td>\r\n"
					"<td><input type='number' name='KeyA1' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyA2' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyA3' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyA4' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyA5' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyA6' min=0 max=255 value=255 style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='VerKeyB'></td>\r\n"
					"<td><input type='number' name='KeyB1' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyB2' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyB3' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyB4' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyB5' min=0 max=255 value=255 style='width: 60px;'>\r\n"
					"<input type='number' name='KeyB6' min=0 max=255 value=255 style='width: 60px;'></td>\r\n"
					"<td></td>\r\n"
						"<input type='hidden' name='Page' value=%i>"
						"<td><button type='submit'>Добавить</button></td></form></tr>\r\n",
						session->request_id,
						(cAlienKeyList[n].Type == 0) ? "selected" : "", cAlienKeyList[n].Type,
						(cAlienKeyList[n].Type > 0) ? "selected" : "", cAlienKeyList[n].Type,
						cTempBuff,			
						iSubPages - 1);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	
	DBG_MUTEX_UNLOCK(&alienkey_mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "alienkeys", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_skipircode_del(int *pParams)
{
	ClearSkipIrCodeList();
	return 1;
}

int WEB_skipircode_save(int *pParams)
{
	if ((pParams[0] > 0) && (pParams[0] < 256))
	{
		DBG_MUTEX_LOCK(&skipircode_mutex);
		iSkipIrCodeMaxCnt = pParams[0];
		DBG_MUTEX_UNLOCK(&skipircode_mutex);
	}	
	ClearSkipIrCodeList();
	return 1;
}

int WEB_skipircodes_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/skipircodes/\"><h1>НОВЫЕ КОДЫ ПУЛЬТОВ</h1></a>\r\n");
	
	DBG_MUTEX_LOCK(&skipircode_mutex);
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody,
					"<form action='/skipircodes/save'>Максимальное количество записей: \r\n"
					"<input type='number' name='Num' min=1 max=255 value=%i style='width: 60px;'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<button type='submit'>Сохранить</button>"
					"<button type='submit' formaction='/skipircodes/delete?req_index=%i'>Очистить</button></form>\r\n", iSkipIrCodeMaxCnt, session->request_id, session->request_id);		
	strcat(msg_tx, msg_subbody);
	
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>ИД</th><th>Длина</th><th>Код</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iSkipIrCodeListCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurSkipIrCodePage = iPage;
	iCurPage = iCurSkipIrCodePage;
	
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurSkipIrCodePage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurSkipIrCodePage = iCurPage;
	}
	
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iSkipIrCodeListCnt) iLastOption = iSkipIrCodeListCnt;
	
	WEB_pages_list(msg_tx, "skipircodes", iSubPages, iCurPage, "");	
	
	char codebuff[1024];	
	unsigned int i;
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(codebuff, 0, 1024);		
		if (cSkipIrCodeList[n].Len < MAX_IRCOMMAND_LEN)
			for (i = 0; i < cSkipIrCodeList[n].Len; i++)
			{
				memset(msg_subbody, 0, 32);
				if ((i+1) < cSkipIrCodeList[n].Len)
					sprintf(msg_subbody, "%i,", cSkipIrCodeList[n].Code[i]);
					else
					sprintf(msg_subbody, "%i", cSkipIrCodeList[n].Code[i]);
				strcat(codebuff, msg_subbody);
			}
				
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, 
					"<tr><form action='/ircodes/add'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='CodeID' maxlength=4 value='%.4s' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='Len' min=0 max=255 value=%i style='width: 60px;' disabled>\r\n"
					"<td><input type='text' name='CodeData' maxlength=512 value='%s' style='width: 400px;'></td>\r\n"
					"<input type='hidden' name='Page' value=999>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n",
					session->request_id,
					n,n, 
					(char*)&cSkipIrCodeList[n].ID,
					cSkipIrCodeList[n].Len, codebuff);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	
	DBG_MUTEX_UNLOCK(&skipircode_mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "skipircodes", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_ircode_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&ircode_mutex);
	iIRCommandCnt++;
	mIRCommandList = (IR_COMMAND_TYPE*)DBG_REALLOC(mIRCommandList, sizeof(IR_COMMAND_TYPE)*iIRCommandCnt);
	memset(&mIRCommandList[iIRCommandCnt-1], 0, sizeof(IR_COMMAND_TYPE));
	mIRCommandList[iIRCommandCnt-1].ID = pParams[1];
	mIRCommandList[iIRCommandCnt-1].Len = 0;
	int n, m;
	char buffout[10];
	m = strlen(strParam);
	for (n = 0; n < MAX_IRCOMMAND_LEN; n++)
		if (GetParamSetting(n, 44, strParam, m, buffout, 10) == 1)
		{
			mIRCommandList[iIRCommandCnt-1].Code[n] = 0;
			if ((strlen(buffout) == 1) && (buffout[0] == 42)) mIRCommandList[iIRCommandCnt-1].Code[n] = 0x0200;
			if ((strlen(buffout) > 1) && (buffout[0] == 38)) mIRCommandList[iIRCommandCnt-1].Code[n] = 0x0100 | ((uint16_t)Str2Int(&buffout[1]) & 0xFF);
			
			if (mIRCommandList[iIRCommandCnt-1].Code[n] == 0) mIRCommandList[iIRCommandCnt-1].Code[n] = (uint16_t)Str2Int(buffout) & 0xFF;				
			mIRCommandList[iIRCommandCnt-1].Len++;
		} else break;
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	SaveIrCodes();
	return 1;
}

int WEB_ircode_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&ircode_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iIRCommandCnt) && iIRCommandCnt)
	{
		IR_COMMAND_TYPE *ict = (IR_COMMAND_TYPE*)DBG_MALLOC(sizeof(IR_COMMAND_TYPE)*(iIRCommandCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iIRCommandCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ict[clk], &mIRCommandList[i], sizeof(IR_COMMAND_TYPE));
				clk++;
			}
		DBG_FREE(mIRCommandList);
		mIRCommandList = ict;
		iIRCommandCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	if (ret) SaveIrCodes();
	return 1;
}

int WEB_ircode_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&ircode_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iIRCommandCnt) && iIRCommandCnt)
	{
		memset(&mIRCommandList[pParams[0]], 0, sizeof(IR_COMMAND_TYPE));
		mIRCommandList[pParams[0]].ID = pParams[1];
		mIRCommandList[pParams[0]].Len = 0;
		int n, m;
		char buffout[10];
		m = strlen(strParam);
		for (n = 0; n < MAX_IRCOMMAND_LEN; n++)
			if (GetParamSetting(n, 44, strParam, m, buffout, 10) == 1)
			{
				mIRCommandList[pParams[0]].Code[n] = 0;
				if ((strlen(buffout) == 1) && (buffout[0] == 42)) mIRCommandList[pParams[0]].Code[n] = 0x0200;
				if ((strlen(buffout) > 1) && (buffout[0] == 38)) mIRCommandList[pParams[0]].Code[n] = 0x0100 | ((uint16_t)Str2Int(&buffout[1]) & 0xFF);
				
				if (mIRCommandList[pParams[0]].Code[n] == 0) mIRCommandList[pParams[0]].Code[n] = (uint16_t)Str2Int(buffout) & 0xFF;				
				mIRCommandList[pParams[0]].Len++;
			} else break;
		ret = 1;				
	}
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	if (ret) SaveIrCodes();
	return 1;
}

int WEB_ircodes_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&ircode_mutex);
	TestIrCodes(1);
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/ircodes/\"><h1>КОДЫ ПУЛЬТОВ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	strcat(msg_tx, "<br /><input type='button' value='Добавить новые' onclick=\"window.location.href='/skipircodes/'\" style='width: 170px;'>\r\n");
			
	DBG_MUTEX_LOCK(&ircode_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>ИД</th><th>Длина</th><th>Код</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iIRCommandCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurIrCodePage = iPage;
	iCurPage = iCurIrCodePage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurIrCodePage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurIrCodePage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iIRCommandCnt) iLastOption = iIRCommandCnt;
	
	WEB_pages_list(msg_tx, "ircodes", iSubPages, iCurPage, "");	
		
	char codebuff[1024];	
	unsigned int i;
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(codebuff, 0, 1024);		
		if (mIRCommandList[n].Len < MAX_IRCOMMAND_LEN)
			for (i = 0; i < mIRCommandList[n].Len; i++)
			{
				memset(msg_subbody, 0, 32);
				if ((i+1) < mIRCommandList[n].Len)
				{
					if ((mIRCommandList[n].Code[i] & 0x0300) == 0) sprintf(msg_subbody, "%i,", mIRCommandList[n].Code[i]);
					if (mIRCommandList[n].Code[i] & 0x0100)	sprintf(msg_subbody, "&%i,", mIRCommandList[n].Code[i] & 0xFF);
					if (mIRCommandList[n].Code[i] & 0x0200)	sprintf(msg_subbody, "*,");
				}
				else
				{
					if ((mIRCommandList[n].Code[i] & 0x0300) == 0) sprintf(msg_subbody, "%i", mIRCommandList[n].Code[i]);
					if (mIRCommandList[n].Code[i] & 0x0100)	sprintf(msg_subbody, "&%i", mIRCommandList[n].Code[i] & 0xFF);
					if (mIRCommandList[n].Code[i] & 0x0200)	sprintf(msg_subbody, "*");
				}
				strcat(codebuff, msg_subbody);
			}
				
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, 
					"<tr><form action='/ircodes/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='CodeID' maxlength=4 value='%.4s' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='Len' min=0 max=255 value=%i style='width: 60px;' disabled>\r\n"
					"<td><input type='text' name='CodeData' maxlength=512 value='%s' style='width: 400px;'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/ircodes/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n, 
					(char*)&mIRCommandList[n].ID,
					mIRCommandList[n].Len, codebuff);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody,
					"<tr><form action='/ircodes/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='text' name='CodeID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='Len' min=0 max=255 value=0 style='width: 60px;' disabled>\r\n"
					"<td><input type='text' name='CodeData' maxlength=512 value='' style='width: 400px;'></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "ircodes", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_key_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int uiSystemID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);
	
	DBG_MUTEX_LOCK(&securitylist_mutex);
	iSecurityKeyCnt++;
	skiSecurityKeys = (SECURITY_KEY_INFO*)DBG_REALLOC(skiSecurityKeys, sizeof(SECURITY_KEY_INFO)*iSecurityKeyCnt);
	memset(&skiSecurityKeys[iSecurityKeyCnt-1], 0, sizeof(SECURITY_KEY_INFO));
	skiSecurityKeys[iSecurityKeyCnt-1].ParentID = uiSystemID;
	skiSecurityKeys[iSecurityKeyCnt-1].Local = 1;
	skiSecurityKeys[iSecurityKeyCnt-1].ID = pParams[2];
	skiSecurityKeys[iSecurityKeyCnt-1].Enabled = pParams[3];
	//skiSecurityKeys[iSecurityKeyCnt-1].EventEnabled = pParams[28];
	skiSecurityKeys[iSecurityKeyCnt-1].Type = pParams[4];
	skiSecurityKeys[iSecurityKeyCnt-1].SerialLength = 0;
	int m = strlen(&strParam[0]);
	int n;
	char buffout[16];
	for (n = 0; n < MAX_SECURITY_SERIAL_LEN; n++)
		if (GetParamSetting(n, 44, &strParam[0], m, buffout, 10) == 1)
		{
			skiSecurityKeys[iSecurityKeyCnt-1].Serial[skiSecurityKeys[iSecurityKeyCnt-1].SerialLength] = (uint8_t)Str2Int(buffout) & 0xFF;				
			skiSecurityKeys[iSecurityKeyCnt-1].SerialLength++;
		} else break;
	if (strlen(&strParam[256]) > 63) memcpy(skiSecurityKeys[iSecurityKeyCnt-1].Name, &strParam[256], 63);
		else memcpy(skiSecurityKeys[iSecurityKeyCnt-1].Name, &strParam[256], strlen(&strParam[256]));
	skiSecurityKeys[iSecurityKeyCnt-1].VerifyKeyA = pParams[11];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyA[0] = pParams[12];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyA[1] = pParams[13];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyA[2] = pParams[14];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyA[3] = pParams[15];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyA[4] = pParams[16];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyA[5] = pParams[17];
	skiSecurityKeys[iSecurityKeyCnt-1].VerifyKeyB = pParams[18];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyB[0] = pParams[19];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyB[1] = pParams[20];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyB[2] = pParams[21];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyB[3] = pParams[22];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyB[4] = pParams[23];
	skiSecurityKeys[iSecurityKeyCnt-1].KeyB[5] = pParams[24];
	skiSecurityKeys[iSecurityKeyCnt-1].Sector = pParams[25];
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	SaveKeys();
	DBG_MUTEX_LOCK(&system_mutex);
	unsigned char cBcTCP = ucBroadCastTCP;
	DBG_MUTEX_UNLOCK(&system_mutex);
	SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_CHANGED_SECURITYLIST, (char*)&uiSystemID, sizeof(uiSystemID), NULL, 0);
	return 1;
}

int WEB_key_del(int *pParams)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int uiSystemID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);
	
	int ret = 0;
	DBG_MUTEX_LOCK(&securitylist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iSecurityKeyCnt) && iSecurityKeyCnt)
	{
		SECURITY_KEY_INFO *ski = (SECURITY_KEY_INFO*)DBG_MALLOC(sizeof(SECURITY_KEY_INFO)*(iSecurityKeyCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iSecurityKeyCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ski[clk], &skiSecurityKeys[i], sizeof(SECURITY_KEY_INFO));
				clk++;
			}
		DBG_FREE(skiSecurityKeys);
		skiSecurityKeys = ski;
		iSecurityKeyCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	if (ret) 
	{
		SaveKeys();
		DBG_MUTEX_LOCK(&system_mutex);
		unsigned char cBcTCP = ucBroadCastTCP;
		DBG_MUTEX_UNLOCK(&system_mutex);
		SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_CHANGED_SECURITYLIST, (char*)&uiSystemID, sizeof(uiSystemID), NULL, 0);
	}
	return 1;
}

int WEB_key_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int uiSystemID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);
			
	DBG_MUTEX_LOCK(&securitylist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iSecurityKeyCnt) && iSecurityKeyCnt)
	{
		if (skiSecurityKeys[pParams[0]].Local == 1)
		{
			SECURITY_KEY_INFO ski;
			int n, m;
			char buffout[10];
			memset(&ski, 0, sizeof(SECURITY_KEY_INFO));
			ski.Local = 1;
			ski.ID = pParams[2];
			ski.ParentID = uiSystemID;
			ski.Enabled = pParams[3];
			//ski.EventEnabled = pParams[28];
			ski.Type = pParams[4];
			
			ski.SerialLength = 0;
			m = strlen(&strParam[0]);
			for (n = 0; n < MAX_SECURITY_SERIAL_LEN; n++)
				if (GetParamSetting(n, 44, &strParam[0], m, buffout, 10) == 1)
				{
					ski.Serial[ski.SerialLength] = (uint8_t)Str2Int(buffout) & 0xFF;				
					ski.SerialLength++;
				} else break;
			if (strlen(&strParam[256]) > 63) memcpy(ski.Name, &strParam[256], 63);
				else memcpy(ski.Name, &strParam[256], strlen(&strParam[256]));
			ski.VerifyKeyA = pParams[11];
			ski.KeyA[0] = pParams[12];
			ski.KeyA[1] = pParams[13];
			ski.KeyA[2] = pParams[14];
			ski.KeyA[3] = pParams[15];
			ski.KeyA[4] = pParams[16];
			ski.KeyA[5] = pParams[17];
			ski.VerifyKeyB = pParams[18];
			ski.KeyB[0] = pParams[19];
			ski.KeyB[1] = pParams[20];
			ski.KeyB[2] = pParams[21];
			ski.KeyB[3] = pParams[22];
			ski.KeyB[4] = pParams[23];
			ski.KeyB[5] = pParams[24];
			ski.Sector = pParams[25];
			
			if (pParams[26] == 0)
			{
				memcpy(&skiSecurityKeys[pParams[0]], &ski, sizeof(SECURITY_KEY_INFO));
				ret = 1;
			}
			else
			{
				ret = 0;				
				DBG_MUTEX_UNLOCK(&securitylist_mutex);
				
				struct sockaddr_in parAddress;
				int iLocal = -1;
				
				DBG_MUTEX_LOCK(&modulelist_mutex);
				n = ModuleIdToNum(pParams[27], 2);
				if (n >= 0) iLocal = miModuleList[n].Local;	
				memcpy(&parAddress, &miModuleList[n].Address, sizeof(struct sockaddr_in));				
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				
				if (n >= 0)
				{
					DBG_MUTEX_LOCK(&securitylist_mutex);
					memcpy(&skiUpdateKeyInfo, &ski, sizeof(SECURITY_KEY_INFO));
					iUpdateKeyInfoAction = pParams[26];
					uiUpdateKeyInfoReader = pParams[27];
					iUpdateKeyInfoResult = 0;
					iUpdateKeyInfoTimer = 0;						
					get_ms(&iUpdateKeyInfoTimer);						
					DBG_MUTEX_UNLOCK(&securitylist_mutex);
					
					if (iLocal == 0)
						SendTCPMessage(TYPE_MESSAGE_GET_CHANGE_SMARTCARD, (char*)&pParams[26], sizeof(unsigned int)*2, (char*)&ski, sizeof(SECURITY_KEY_INFO), &parAddress);
					
					n = 0;
					DBG_MUTEX_LOCK(&securitylist_mutex);
					while(1)
					{
						n = iUpdateKeyInfoResult;
						if (n != 0)	break;
						if ((unsigned int)get_ms(&iUpdateKeyInfoTimer) > 10000) break;
						DBG_MUTEX_UNLOCK(&securitylist_mutex);
						usleep(100000);
						DBG_MUTEX_LOCK(&securitylist_mutex);
					}
					DBG_MUTEX_UNLOCK(&securitylist_mutex);
					if (n == 1)
					{
						ret = 1;
						memcpy(&skiSecurityKeys[pParams[0]], &ski, sizeof(SECURITY_KEY_INFO));
						dbgprintf(3, "Changed smartcard %.4s\n", (char*)&skiSecurityKeys[pParams[0]].ID);
					}
					if (n == 2) dbgprintf(2, "Auth Error change smartcard %.4s\n", (char*)&skiSecurityKeys[pParams[0]].ID);
					if (n < -1) dbgprintf(2, "System Error %i change smartcard %.4s\n", n, (char*)&skiSecurityKeys[pParams[0]].ID);
					if (n == -1) dbgprintf(2, "Access Error change smartcard %.4s\n", (char*)&skiSecurityKeys[pParams[0]].ID);
					if (n == 0) dbgprintf(2, "Timeout change smartcard %.4s\n", (char*)&skiSecurityKeys[pParams[0]].ID);
				}
				DBG_MUTEX_LOCK(&securitylist_mutex);
			}
		}		
	}
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	if (ret) 
	{
		SaveKeys();
		/*DBG_MUTEX_LOCK(&systemlist_mutex);
		cThreadMainStatus = 1;
		DBG_MUTEX_UNLOCK(&systemlist_mutex);	*/
		DBG_MUTEX_LOCK(&system_mutex);
		unsigned char cBcTCP = ucBroadCastTCP;
		DBG_MUTEX_UNLOCK(&system_mutex);
		SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_CHANGED_SECURITYLIST, (char*)&uiSystemID, sizeof(uiSystemID), NULL, 0);
	}
	return 1;
}
/*
int WEB_key_change(int *pParams, char* strParam)
{
	int ret = 0;
	SECURITY_KEY_INFO ski;
	memset(&ski, 0, sizeof(SECURITY_KEY_INFO));
	DBG_MUTEX_LOCK(&securitylist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iSecurityKeyCnt) && iSecurityKeyCnt)
	{
		if ((skiSecurityKeys[pParams[0]].Local == 1)
			&& (skiSecurityKeys[pParams[0]].ID = pParams[2]))
		{
			memcpy(&ski, &skiSecurityKeys[pParams[0]], sizeof(SECURITY_KEY_INFO));
			DBG_MUTEX_UNLOCK(&securitylist_mutex);
			if (UpdateSecurityKey(&ski, pParams[3], &pParams[4]))
				ret = 1;
			DBG_MUTEX_LOCK(&securitylist_mutex);
		}		
	}
	if (ret)
	{
		if (skiSecurityKeys[pParams[0]].ID == ski.ID) memcpy(&skiSecurityKeys[pParams[0]], &ski, sizeof(SECURITY_KEY_INFO));
	}
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	if (ret) SaveKeys();
	return 1;
}*/

int WEB_keys_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&securitylist_mutex);
	TestKeys(1);
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/keys/\"><h1>КОДЫ СМАРТКАРТ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	strcat(msg_tx, "<datalist id='crlist'>\r\n");
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 1) && 
			((miModuleList[n].Type == MODULE_TYPE_USB_GPIO) || (miModuleList[n].Type == MODULE_TYPE_PN532) || (miModuleList[n].Type == MODULE_TYPE_RC522)))
			{
				memset(msg_subbody, 0, 65536);
				sprintf(msg_subbody, "<option value='%.4s'>\r\n", (char*)&miModuleList[n].ID);
				strcat(msg_tx, msg_subbody);
			}
	strcat(msg_tx, "</datalist>\r\n");
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	strcat(msg_tx, "<br /><input type='button' value='Добавить новые' onclick=\"window.location.href='/alienkeys/'\" style='width: 170px;'>\r\n");
		
	DBG_MUTEX_LOCK(&securitylist_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>ИД</th><th>Активный</th><th>Тип</th><th>Серийный номер</th><th>Имя</th><th>Сектор</th>"
					"<th>Проверять код A</th><th>Код доступа A к сектору</th><th>Проверять код B</th><th>Код доступа B к сектору</th>"
					"<th>Операция</th><th>Действие/Считыватель</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iSecurityKeyCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurKeyPage = iPage;
	iCurPage = iCurKeyPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurKeyPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurKeyPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iSecurityKeyCnt) iLastOption = iSecurityKeyCnt;
	
	WEB_pages_list(msg_tx, "keys", iSubPages, iCurPage, "");	
	
	char* cDisableFlag;
	unsigned int uiID;	
	char cTempBuff[96];
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		if (skiSecurityKeys[n].Local == 0) cDisableFlag = " disabled "; else cDisableFlag = "";
		PrintListValues(skiSecurityKeys[n].Serial, 1, skiSecurityKeys[n].SerialLength, MAX_SECURITY_SERIAL_LEN, cTempBuff, 96);
			
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, 
					"<tr><form action='/keys/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='KeyID' maxlength=4 value='%.4s' style='width: 60px;'%s></td>\r\n"
					"<td><input type='checkbox' name='Enabled'%s%s></td>\r\n"
					//"<td><input type='checkbox' name='EvntEnbl'%s%s></td>\r\n"
					"<td><select name='Type' style='width: 140px;'%s>\r\n"
					"		<option %s value='%i'>Смарт карта</option>\r\n"
					"		<option %s disabled value='%i'>Неизвестное</option>\r\n"									
					"	</select></td>\r\n"
					"<td><input type='text' name='Serial' maxlength=64 value='%s' style='width: 300px;'%s></td>\r\n"
					"<td><input type='text' name='Name' maxlength=64 value='%s' style='width: 300px;'%s></td>\r\n"
					"<td><input type='number' name='Sector' min=0 max=255 value=%i style='width: 45px;'%s></td>\r\n"
					"<td><input type='checkbox' name='VerKeyA'%s%s></td>\r\n"
					"<td><input type='number' name='KeyA1' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyA2' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyA3' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyA4' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyA5' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyA6' min=0 max=255 value=%i style='width: 45px;'%s></td>\r\n"
					"<td><input type='checkbox' name='VerKeyB'%s%s></td>\r\n"
					"<td><input type='number' name='KeyB1' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyB2' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyB3' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyB4' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyB5' min=0 max=255 value=%i style='width: 45px;'%s>\r\n"
					"<input type='number' name='KeyB6' min=0 max=255 value=%i style='width: 45px;'%s></td>\r\n"
					"<td><select name='Action' style='width: 140px;'%s>\r\n"
						"<option selected value='0'>Ничего</option>\r\n"
						"<option value='1'>Изменить в карте через A</option>\r\n"
						"<option value='2'>Изменить в карте через B</option>\r\n"
						"</select>\r\n"
					"<input type='text' list='crlist' name='CrId' maxlength=4 style='width: 100px;'%s></td>\r\n",					
					session->request_id,
					n,n, 
					(char*)&skiSecurityKeys[n].ID, cDisableFlag,
					skiSecurityKeys[n].Enabled ? " checked " : "", cDisableFlag,
					//skiSecurityKeys[n].EventEnabled ? " checked " : "", cDisableFlag,
					cDisableFlag,
					(skiSecurityKeys[n].Type == 0) ? "selected" : "", skiSecurityKeys[n].Type,
					(skiSecurityKeys[n].Type > 0) ? "selected" : "", skiSecurityKeys[n].Type,
					cTempBuff, cDisableFlag,
					skiSecurityKeys[n].Name, cDisableFlag,
					skiSecurityKeys[n].Sector, cDisableFlag,
					skiSecurityKeys[n].VerifyKeyA ? " checked " : "", cDisableFlag,
					skiSecurityKeys[n].KeyA[0], cDisableFlag, skiSecurityKeys[n].KeyA[1], cDisableFlag,
					skiSecurityKeys[n].KeyA[2], cDisableFlag, skiSecurityKeys[n].KeyA[3], cDisableFlag,
					skiSecurityKeys[n].KeyA[4], cDisableFlag, skiSecurityKeys[n].KeyA[5], cDisableFlag,
					skiSecurityKeys[n].VerifyKeyB ? " checked " : "", cDisableFlag,
					skiSecurityKeys[n].KeyB[0], cDisableFlag, skiSecurityKeys[n].KeyB[1], cDisableFlag,
					skiSecurityKeys[n].KeyB[2], cDisableFlag, skiSecurityKeys[n].KeyB[3], cDisableFlag,
					skiSecurityKeys[n].KeyB[4], cDisableFlag, skiSecurityKeys[n].KeyB[5], cDisableFlag,					
					cDisableFlag, cDisableFlag);		
		strcat(msg_tx, msg_subbody);
		if (skiSecurityKeys[n].Local)
		{			
			strcat(msg_tx, "<td><button type='submit'>Сохранить</button>\r\n"
							"<button type='submit' formaction='/keys/add'>Копировать</button>"
							"<button type='submit' formaction='/keys/delete'>Удалить</button></td></form></tr>\r\n");
		}
		else 
		{
			uiID = skiSecurityKeys[n].ParentID;
			DBG_MUTEX_UNLOCK(&securitylist_mutex);
			DBG_MUTEX_LOCK(&systemlist_mutex);
			SYSTEM_INFO* si = GetSysInfo(uiID);
			if (si) 
			{
				memset(msg_subbody, 0, 3000);
				sprintf(msg_subbody, "<td><button type='submit' formaction='/keys/add'>Копировать</button>"
										"<input type='button' value='Редактировать' onclick=\"window.location.href='http://%s/keys/'\"></td></form></tr>\r\n",inet_ntoa(si->Address.sin_addr));				
				strcat(msg_tx, msg_subbody);				
			}
			DBG_MUTEX_UNLOCK(&systemlist_mutex);			
			DBG_MUTEX_LOCK(&securitylist_mutex);
		}
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody, 
					"<tr><form action='/keys/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='text' name='KeyID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='Enabled'></td>\r\n"
					//"<td><input type='checkbox' name='EvntEnbl'></td>\r\n"
					"<td><select name='Type' style='width: 140px;'>\r\n"
					"		<option selected value='0'>Смарт карта</option>\r\n"									
					"	</select></td>\r\n"
					"<td><input type='text' name='Serial' maxlength=64 value='' style='width: 300px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=64 value='' style='width: 300px;'></td>\r\n"
					"<td><input type='number' name='Sector' min=0 max=255 value=0 style='width: 45px;'>\r\n"
					"<td><input type='checkbox' name='VerKeyA'></td>\r\n"
					"<td><input type='number' name='KeyA1' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyA2' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyA3' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyA4' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyA5' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyA6' min=0 max=255 value=255 style='width: 45px;'></td>\r\n"
					"<td><input type='checkbox' name='VerKeyB'></td>\r\n"
					"<td><input type='number' name='KeyB1' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyB2' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyB3' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyB4' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyB5' min=0 max=255 value=255 style='width: 45px;'>\r\n"
					"<input type='number' name='KeyB6' min=0 max=255 value=255 style='width: 45px;'></td>\r\n"
					"<td></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n",
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "keys", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_evntaction_recover(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
	{
		if (maActionInfo[pParams[0]].Enabled == -1) maActionInfo[pParams[0]].Enabled = 0;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	if (ret) SaveEvntActions();
	return 1;
}

int WEB_change_pos(int iFrom, int iTo, char* pData, unsigned int uiTotalCnt, unsigned int uiSizePos)
{
	int n, m;
	dbgprintf(4, "remove evntaction from %i to %i total:%i\n",iFrom, iTo, uiTotalCnt);
	if ((iFrom >= 0) && (iFrom < uiTotalCnt) && uiTotalCnt && (iTo >= 0) && (iTo < uiTotalCnt) && (iFrom != iTo))
	{
		char *maIF = (char*)DBG_MALLOC(uiTotalCnt*uiSizePos);
		memcpy(maIF, pData, uiTotalCnt * uiSizePos);
		m = 0;
		memcpy(&pData[iTo * uiSizePos], &maIF[iFrom * uiSizePos], uiSizePos);
		for(n = 0; n < uiTotalCnt; n++)
		{
			if (n != iFrom) 
			{
				if (m == iTo) m++;			
				memcpy(&pData[m * uiSizePos], &maIF[n * uiSizePos], uiSizePos);
				m++;
			}
		}
		DBG_FREE(maIF);
	}	
	return 1;
}

int WEB_evntaction_up(int *pParams)
{
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
	{
		if (pParams[0] > 0) WEB_change_pos(pParams[0], pParams[0] - 1, (char*)maActionInfo, iActionCnt, sizeof(ACTION_INFO));
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	return 1;
}

int WEB_evntaction_down(int *pParams)
{
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
	{
		if (pParams[0] < (iActionCnt - 1)) WEB_change_pos(pParams[0], pParams[0] + 1, (char*)maActionInfo, iActionCnt, sizeof(ACTION_INFO));
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	return 1;
}

int WEB_evntaction_exec(int *pParams)
{
	int ret = 0;
	unsigned int uiData[3];
	char cData[128];
	
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
	{
		//maActionInfo[pParams[0]].Access = pParams[1];
		uiData[0] = maActionInfo[pParams[0]].DestID;
		uiData[1] = maActionInfo[pParams[0]].DestSubNumber;
		uiData[2] = maActionInfo[pParams[0]].DestStatus;		
		memcpy(cData, maActionInfo[pParams[0]].Name, 128);
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	if (ret) AddModuleEventInList(uiData[0], uiData[1], uiData[2], cData, strlen(cData), 1);
	return 1;
}

int WEB_evntaction_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&evntaction_mutex);
	iActionCnt++;
	maActionInfo = (ACTION_INFO*)DBG_REALLOC(maActionInfo, sizeof(ACTION_INFO)*iActionCnt);
	memset(&maActionInfo[iActionCnt-1], 0, sizeof(ACTION_INFO));
	maActionInfo[iActionCnt-1].ActionID = pParams[1];
	maActionInfo[iActionCnt-1].Enabled = pParams[2];
	maActionInfo[iActionCnt-1].SrcID = pParams[3];
	maActionInfo[iActionCnt-1].SrcSubNumber = pParams[4];
	maActionInfo[iActionCnt-1].TestType = pParams[5];
	maActionInfo[iActionCnt-1].SrcStatus = pParams[6];
	maActionInfo[iActionCnt-1].TestModuleID = pParams[7];
	maActionInfo[iActionCnt-1].TestModuleSubNumber = pParams[8];
	maActionInfo[iActionCnt-1].TestModuleStatus = pParams[9];
	maActionInfo[iActionCnt-1].DestID = pParams[10];
	maActionInfo[iActionCnt-1].DestSubNumber = pParams[11];
	maActionInfo[iActionCnt-1].DestStatus = pParams[12];
	if (strlen(&strParam[0]) > 128) memcpy(maActionInfo[iActionCnt-1].Name, &strParam[0], 128);
	else memcpy(maActionInfo[iActionCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	maActionInfo[iActionCnt-1].AfterAccept = pParams[13];
	maActionInfo[iActionCnt-1].TimerWaitBefore = pParams[14];
	maActionInfo[iActionCnt-1].TimerOff = pParams[15];
	maActionInfo[iActionCnt-1].WeekDays = pParams[16];
	maActionInfo[iActionCnt-1].NotBeforeTime = pParams[17];
	maActionInfo[iActionCnt-1].NotAfterTime = pParams[18];
	maActionInfo[iActionCnt-1].RestartTimer = pParams[19];
	maActionInfo[iActionCnt-1].Test2Type = pParams[21];
	maActionInfo[iActionCnt-1].TimerWaitBeforeStatus = pParams[22];
	maActionInfo[iActionCnt-1].TimerOffStatus = pParams[23];
	maActionInfo[iActionCnt-1].Test3ModuleID = pParams[24];
	maActionInfo[iActionCnt-1].Test3ModuleSubNumber = pParams[25];
	maActionInfo[iActionCnt-1].Test3Type = pParams[26];
	maActionInfo[iActionCnt-1].Test3ModuleStatus = pParams[27];
	if (strlen(&strParam[256]) > 128) memcpy(maActionInfo[iActionCnt-1].GroupName, &strParam[256], 128);
	else memcpy(maActionInfo[iActionCnt-1].GroupName, &strParam[256], strlen(&strParam[256]));
	
	if (pParams[20] < (iActionCnt-1)) WEB_change_pos(iActionCnt-1, pParams[20], (char*)maActionInfo, iActionCnt, sizeof(ACTION_INFO));
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	SaveEvntActions();
	return 1;
}

int WEB_evntaction_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
	{
		if (maActionInfo[pParams[0]].Enabled != -1) 
		{
			maActionInfo[pParams[0]].Enabled = -1;
			ret = 1;
		}
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	if (ret) SaveEvntActions();
	return 1;
}

int WEB_evntaction_restart()
{
	int n;
	DBG_MUTEX_LOCK(&evntaction_mutex);
	for (n = 0; n < iActionCnt; n++) 
		maActionInfo[n].CounterExec = 0;
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	return 1;
}

int WEB_evntaction_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
	{
		memset(&maActionInfo[pParams[0]], 0, sizeof(ACTION_INFO));
		maActionInfo[pParams[0]].ActionID = pParams[1];
		maActionInfo[pParams[0]].Enabled = pParams[2];
		maActionInfo[pParams[0]].SrcID = pParams[3];
		maActionInfo[pParams[0]].SrcSubNumber = pParams[4];
		maActionInfo[pParams[0]].TestType = pParams[5];
		maActionInfo[pParams[0]].SrcStatus = pParams[6];
		maActionInfo[pParams[0]].TestModuleID = pParams[7];
		maActionInfo[pParams[0]].TestModuleSubNumber = pParams[8];
		maActionInfo[pParams[0]].TestModuleStatus = pParams[9];
		maActionInfo[pParams[0]].DestID = pParams[10];
		maActionInfo[pParams[0]].DestSubNumber = pParams[11];
		maActionInfo[pParams[0]].DestStatus = pParams[12];
		if (strlen(&strParam[0]) > 128) memcpy(maActionInfo[pParams[0]].Name, &strParam[0], 128);
		else memcpy(maActionInfo[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));
		maActionInfo[pParams[0]].AfterAccept = pParams[13];
		maActionInfo[pParams[0]].TimerWaitBefore = pParams[14];
		maActionInfo[pParams[0]].TimerOff = pParams[15];
		maActionInfo[pParams[0]].WeekDays = pParams[16];
		maActionInfo[pParams[0]].NotBeforeTime = pParams[17];
		maActionInfo[pParams[0]].NotAfterTime = pParams[18];
		maActionInfo[pParams[0]].RestartTimer = pParams[19];
		maActionInfo[pParams[0]].Test2Type = pParams[21];
		maActionInfo[pParams[0]].TimerWaitBeforeStatus = pParams[22];
		maActionInfo[pParams[0]].TimerOffStatus = pParams[23];
		maActionInfo[pParams[0]].Test3ModuleID = pParams[24];
		maActionInfo[pParams[0]].Test3ModuleSubNumber = pParams[25];
		maActionInfo[pParams[0]].Test3Type = pParams[26];
		maActionInfo[pParams[0]].Test3ModuleStatus = pParams[27];
		
		if (strlen(&strParam[256]) > 128) memcpy(maActionInfo[pParams[0]].GroupName, &strParam[256], 128);
		else memcpy(maActionInfo[pParams[0]].GroupName, &strParam[256], strlen(&strParam[256]));
		
		if (pParams[0] != pParams[20]) WEB_change_pos(pParams[0], pParams[20], (char*)maActionInfo, iActionCnt, sizeof(ACTION_INFO));
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	if (ret) SaveEvntActions();
	return 1;
}

int WEB_evntaction_filters(int *pParams, char* strParam)
{
	uiCurEvntActIdFilter = pParams[0];
	memset(cCurEvntActGroupFilter, 0, 130);
	if (strlen(&strParam[0]) > 128) memcpy(cCurEvntActGroupFilter, &strParam[0], 128);
		else memcpy(cCurEvntActGroupFilter, &strParam[0], strlen(&strParam[0]));
	cCurEvntActEnFilter = pParams[1] & 3;
	if (cCurEvntActEnFilter > 2) cCurEvntActEnFilter = 2;	
	return 1;
}

int WEB_evntaction_refresh(int *pParams)
{
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionCnt) && iActionCnt)
			maActionInfo[pParams[0]].CounterExec = 0;
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	return 1;
}

int WEB_evntactions_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&evntaction_mutex);
	TestEvntActions(1);
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	char SrcStatusBuff[64];
	char TestStatusBuff[64];
	char Test3StatusBuff[64];
	char DestStatusBuff[64];
	char DestSubNumberBuff[64];	
	char SrcStatusBuff2[64];
	char TestStatusBuff2[64];
	char Test3StatusBuff2[64];
	char DestStatusBuff2[64];
	char DestSubNumberBuff2[64];	
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/evntactions/\"><h1>ДЕЙСТВИЯ НА СОБЫТИЯ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	strcat(msg_tx, "<br /><input type='button' value='Добавить новые' onclick=\"window.location.href='/skipevents/'\" style='width: 170px;'>\r\n");
	
	
	strcat(msg_tx, "<br />Справочник действий:<select style='width: 240px;'%s>\r\n");
	for (n = MENU_PAGE_MAIN; n < SYSTEM_CMD_MAX; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", n, GetActionCodeName(n, NULL, 0, 5));
		strcat(msg_tx, msg_subbody);	
	}
	strcat(msg_tx, "	</select>\r\n");
	
	DBG_MUTEX_LOCK(&ircode_mutex);
	strcat(msg_tx, "   Справочник IR:<select style='width: 240px;'%s>\r\n");
	for (n = 0; n < iIRCommandCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%i'>%.4s</option>\r\n", n, (char*)&mIRCommandList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	strcat(msg_tx, "	</select>\r\n");
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	strcat(msg_tx, "   Справочник модулей:<select style='width: 240px;'%s>\r\n");
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%i'>[%.4s] %s</option>\r\n", n, (char*)&miModuleList[n].ID, miModuleList[n].Name);
		strcat(msg_tx, msg_subbody);
	}		
	strcat(msg_tx, "	</select><br />\r\n");
	
	strcat(msg_tx, "<datalist id='idlist'>\r\n");
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&miModuleList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	strcat(msg_tx, "</datalist>\r\n");	
	strcat(msg_tx, "<datalist id='numberlist'>\r\n");
	for (n = SYSTEM_CMD_NULL + 1; n < MENU_PAGE_MAX; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%s'></option>\r\n", GetActionCodeName(n, SrcStatusBuff, 64, 0));
		strcat(msg_tx, msg_subbody);
	}
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&miModuleList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	strcat(msg_tx, "</datalist>\r\n");	
		
	strcat(msg_tx, "<datalist id='statuslist'>\r\n");
	for (n = MENU_PAGE_MAX + 1; n < SYSTEM_CMD_MAX; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%s'></option>\r\n", GetActionCodeName(n, SrcStatusBuff, 64, 0));
		strcat(msg_tx, msg_subbody);
	}
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&miModuleList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	DBG_MUTEX_LOCK(&ircode_mutex);
	for (n = 0; n < iIRCommandCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&mIRCommandList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	strcat(msg_tx, "</datalist>\r\n");	
	
	DBG_MUTEX_LOCK(&evntaction_mutex);
	
	unsigned int *pIdList = (unsigned int*)DBG_MALLOC(sizeof(unsigned int));
	int iIdCnt = 1;
	pIdList[0] = 0;
	int i, m;
	for (n = 0; n < iActionCnt; n++)
	{
		m = 0;
		for (i = 0; i < iIdCnt; i++)
			if (pIdList[i] == maActionInfo[n].SrcID)
			{
				m = 1;
				break;
			}
		if (m == 0)
		{
			iIdCnt++;
			pIdList = (unsigned int*)DBG_REALLOC(pIdList, sizeof(unsigned int)*iIdCnt);
			pIdList[iIdCnt-1] = maActionInfo[n].SrcID;
		}
	}
	
	GROUP_TYPE *pGroupList = (GROUP_TYPE*)DBG_MALLOC(sizeof(GROUP_TYPE));
	int iGroupCnt = 1;
	memset(&pGroupList[0], 0, sizeof(GROUP_TYPE));
	for (n = 0; n < iActionCnt; n++)
	{
		m = 0;
		for (i = 0; i < iGroupCnt; i++)
			if (((pGroupList[i].Name[0] == 0) && (maActionInfo[n].GroupName[0] == 0)) ||
				(CompareStrCaseIgn(pGroupList[i].Name, maActionInfo[n].GroupName) == 1))
			{
				m = 1;
				break;
			}
		if (m == 0)
		{
			iGroupCnt++;
			pGroupList = (GROUP_TYPE*)DBG_REALLOC(pGroupList, sizeof(GROUP_TYPE)*iGroupCnt);
			memset(&pGroupList[iGroupCnt-1], 0, 128);
			if (strlen(maActionInfo[n].GroupName) > 128) memcpy(pGroupList[iGroupCnt-1].Name, maActionInfo[n].GroupName, 128);
				else memcpy(pGroupList[iGroupCnt-1].Name, maActionInfo[n].GroupName, strlen(maActionInfo[n].GroupName));
		}
	}
	
	memset(msg_subbody, 0, 128);
	sprintf(msg_subbody, "<form action='/evntactions/filters'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"Активность: <select name='Enabled' style='width: 140px;'>\r\n"
								"		<option %s value='0'>Отключенные</option>\r\n"
								"		<option %s value='1'>Включенные</option>\r\n"
								"		<option %s value='2'>Все</option>\r\n"											
								"	</select>\r\n"
						"Идентификатор: <select name='Index' style='width: 140px;'>\r\n",
						session->request_id,
						(cCurEvntActEnFilter == 0) ? " selected " : "",
						(cCurEvntActEnFilter == 1) ? " selected " : "",
						(cCurEvntActEnFilter == 2) ? " selected " : "");
	strcat(msg_tx, 	msg_subbody);
	for (n = 0; n < iIdCnt; n++)
	{
		memset(msg_subbody, 0, 128);
		sprintf(msg_subbody, "		<option %s value='%.4s'>%.4s</option>\r\n", 
								(uiCurEvntActIdFilter == pIdList[n]) ? " selected " : "",
								(char*)&pIdList[n],
								(char*)&pIdList[n]);
		strcat(msg_tx, 	msg_subbody);
	}
	strcat(msg_tx, 	"	</select>\r\n");
	
	memset(msg_subbody, 0, 128);
	sprintf(msg_subbody, "Группа: <select name='Group' style='width: 300px;'>\r\n");
	strcat(msg_tx, 	msg_subbody);	
	for (n = 0; n < iGroupCnt; n++)
	{
		memset(msg_subbody, 0, 128);
		sprintf(msg_subbody, "		<option %s value='%s'>%s</option>\r\n", 
						((CompareStrCaseIgn(pGroupList[n].Name, cCurEvntActGroupFilter) == 1) ||
						((pGroupList[n].Name[0] == 0) && (cCurEvntActGroupFilter[0] == 0))) ? " selected " : "",
						pGroupList[n].Name,
						pGroupList[n].Name);
		strcat(msg_tx, 	msg_subbody);
	}
	strcat(msg_tx, 	"	</select></td>\r\n"
						"<button type='submit'>Применить</button>\r\n"
						"<button type='submit' formaction='/evntactions/restart'>Сбросить счетчики</button></form>\r\n");
	
	if (iGroupCnt) DBG_FREE(pGroupList);
	if (iIdCnt) DBG_FREE(pIdList);	
		
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>ИД действия</th><th>Включен</th><th>Группа</th><th>ИД ист.</th><th>Номер ист.</th><th>Тип проверки</th><th>Статус ист.</th>"
					"<th>ИД тест.</th><th>Номер тест.</th><th>Тип проверки</th><th>Статус тест.</th><th>ИД цель.</th><th>Номер цель.</th><th>Статус цель.</th>"
					"<th>Наименование</th><th>После выполнения</th><th>Задержка перед выполнением (мс); Перезапускать; Текущее</th><th>Пропускать повторные события (мс); Перезапускать; Текущее</th>"
					"<th>Дни недели</th><th>Интервал времени</th><th>Срабатываний</th><th>Действие</th></tr>");
	
	
	int iSubPages, iCurPage, iFirstOption, iLastOption;

	int iActionFiltCnt = 0;
	for (n = 0; n < iActionCnt; n++)
	{
		if (((cCurEvntActEnFilter == 2) || (cCurEvntActEnFilter == maActionInfo[n].Enabled)) &&
			((uiCurEvntActIdFilter == 0) || (uiCurEvntActIdFilter == maActionInfo[n].SrcID)) &&
			((cCurEvntActGroupFilter[0] == 0) || 
			(CompareStrCaseIgn(cCurEvntActGroupFilter, maActionInfo[n].GroupName) == 1) ||
			((cCurEvntActGroupFilter[0] == 0) && (maActionInfo[n].GroupName[0] == 0))))
		{
			iActionFiltCnt++;
		}
	}
	
	iSubPages = (int)ceil((double)iActionFiltCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurEvntActionsPage = iPage;
	iCurPage = iCurEvntActionsPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurEvntActionsPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurEvntActionsPage = iCurPage;
	}
	
	iFirstOption = 0;
	iLastOption = 0;
	i = 0;
	int k = 0;
	for (n = 0; n < iActionCnt; n++)
	{
		if (((cCurEvntActEnFilter == 2) || (cCurEvntActEnFilter == maActionInfo[n].Enabled)) &&
			((uiCurEvntActIdFilter == 0) || (uiCurEvntActIdFilter == maActionInfo[n].SrcID)) &&
			((cCurEvntActGroupFilter[0] == 0) || 
			(CompareStrCaseIgn(cCurEvntActGroupFilter, maActionInfo[n].GroupName) == 1) ||
			((cCurEvntActGroupFilter[0] == 0) && (maActionInfo[n].GroupName[0] == 0))))
		{			
			if (i < (iCurPage * WEB_PAGE_MAX_LEN)) {i++; iFirstOption = n;}
			if (k < (iFirstOption + WEB_PAGE_MAX_LEN)) {k++; iLastOption = n;} else break;
		}
	}	
	
	WEB_pages_list(msg_tx, "evntactions", iSubPages, iCurPage, "");	
	
	int fl1[2];
	int fl2[2];
	int fl3[2];
	char* cDisableFlag;	
	
	if (iLastOption < iActionCnt)
	for (n = iFirstOption; n <= iLastOption; n++)
	{		
		if (((cCurEvntActEnFilter == 2) || (cCurEvntActEnFilter == maActionInfo[n].Enabled)) &&
			((uiCurEvntActIdFilter == 0) || (uiCurEvntActIdFilter == maActionInfo[n].SrcID)) &&
			((cCurEvntActGroupFilter[0] == 0) || 
			(CompareStrCaseIgn(cCurEvntActGroupFilter, maActionInfo[n].GroupName) == 1) ||
			((cCurEvntActGroupFilter[0] == 0) && (maActionInfo[n].GroupName[0] == 0))))
		{
			if (maActionInfo[n].Enabled == -1) cDisableFlag = " disabled "; else cDisableFlag = "";
			fl1[0] = (int)floor((float)maActionInfo[n].NotBeforeTime/10000);
			fl2[0] = (int)floor((float)maActionInfo[n].NotBeforeTime/100);
			fl2[0] = fl2[0] - (fl1[0]*100);
			fl3[0] = maActionInfo[n].NotBeforeTime - ((fl1[0]*10000) + (fl2[0]*100));
			fl1[1] = (int)floor((float)maActionInfo[n].NotAfterTime/10000);
			fl2[1] = (int)floor((float)maActionInfo[n].NotAfterTime/100);
			fl2[1] = fl2[1] - (fl1[1]*100);
			fl3[1] = maActionInfo[n].NotAfterTime - ((fl1[1]*10000) + (fl2[1]*100));
			
			memset(msg_subbody, 0, 65536);
			sprintf(msg_subbody,
						"<tr><form action='/evntactions/save'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"<input type='hidden' name='Num' value=%i>"
						"<td><input type='number' name='NewPos' value=%i style='width: 50px;'></td>\r\n"
						"<td><input type='text' name='ActionID' maxlength=4 value='%.4s' style='width: 60px;'%s></td>\r\n"
						"<td><input type='checkbox' name='En'%s%s></td>\r\n"
						"<td><input type='text' name='Group' maxlength=122 value='%s' style='width: 180px;'%s></td>\r\n"
						"<td><input type='text' list='idlist' name='SrcID' maxlength=4 value='%.4s' style='width: 60px;'%s></td>\r\n"
						"<td><input type='number' name='SrcSubNumber' min=0 value=%i style='width: 60px;'%s></td>\r\n"
						"<td><select name='TestType' style='width: 140px;'%s>\r\n"
							"<option %s value='%i' disabled>Неизвестно</option>\r\n"
							"<option %s value='%i'>Равно</option>\r\n"
							"<option %s value='%i'>Меньше</option>\r\n"
							"<option %s value='%i'>Больше</option>\r\n"
							"<option %s value='%i'>Лог. И</option>\r\n"
							"<option %s value='%i'>Не равно</option>\r\n"
							"</select></td>\r\n"
						"<td><input type='text' name='SrcStatus2' value='%s' style='width: 60px;' disabled>\r\n"
						"<input type='text' list='statuslist' name='SrcStatus' maxlength=64 value='%s' style='width: 200px;'%s></td>\r\n"
						"<td><input type='text' list='idlist' name='TestModuleID' maxlength=4 value='%.4s' style='width: 60px;'%s><br />\r\n"
							"<input type='text' list='idlist' name='Test3ModuleID' maxlength=4 value='%.4s' style='width: 60px;'%s></td>\r\n"
						"<td><input type='number' name='TestModuleSubNumber' min=0 value=%i style='width: 60px;'%s><br />\r\n"
							"<input type='number' name='Test3ModuleSubNumber' min=0 value=%i style='width: 60px;'%s></td>\r\n"
						"<td><select name='Test2Type' style='width: 140px;'%s>\r\n"
							"<option %s value='%i' disabled>Неизвестно</option>\r\n"
							"<option %s value='%i'>Равно</option>\r\n"
							"<option %s value='%i'>Меньше</option>\r\n"
							"<option %s value='%i'>Больше</option>\r\n"
							"<option %s value='%i'>Лог. И</option>\r\n"
							"<option %s value='%i'>Не равно</option>\r\n"
							"</select><br />\r\n"
							"<select name='Test3Type' style='width: 140px;'%s>\r\n"
							"<option %s value='%i' disabled>Неизвестно</option>\r\n"
							"<option %s value='%i'>Равно</option>\r\n"
							"<option %s value='%i'>Меньше</option>\r\n"
							"<option %s value='%i'>Больше</option>\r\n"
							"<option %s value='%i'>Лог. И</option>\r\n"
							"<option %s value='%i'>Не равно</option>\r\n"
							"</select></td>\r\n"
						"<td><input type='text' name='TestModuleStatus2' value='%s' style='width: 60px;' disabled><br />\r\n"
							"<input type='text' list='statuslist' name='TestModuleStatus' maxlength=64 value='%s' style='width: 100px;'%s><br />\r\n"
							"<input type='text' name='Test3ModuleStatus2' value='%s' style='width: 60px;' disabled><br />\r\n"
							"<input type='text' list='statuslist' name='Test3ModuleStatus' maxlength=64 value='%s' style='width: 100px;'%s></td>\r\n"
						"<td><input type='text' list='idlist' name='DestID' maxlength=4 value='%.4s' style='width: 60px;'%s></td>\r\n"
						"<td><input type='text' name='DestSubNumber2' value='%s' style='width: 60px;' disabled>\r\n"
						"<input type='text' list='numberlist' name='DestSubNumber' maxlength=64 value='%s' style='width: 100px;'%s></td>\r\n"
						"<td><input type='text' name='DestStatus2' value='%s' style='width: 60px;' disabled>\r\n"
						"<input type='text' list='statuslist' name='DestStatus' maxlength=64 value='%s' style='width: 200px;'%s></td>\r\n"
						"<td><input type='text' name='Name' maxlength=63 value='%s' style='width: 300px;'%s></td>\r\n"
						"<td><select name='AfterAccept' style='width: 140px;'%s>\r\n"
							"<option %s value='%i'>Ничего</option>\r\n"
							"<option %s value='%i'>Остановить</option>\r\n"
							"<option %s value='%i'>Отключить</option>\r\n"
							"<option %s value='%i'>Остановить и отключить</option>\r\n"
							"<option %s value='%i'>Остановить если повторное</option>\r\n"
							"</select></td>\r\n"
						"<td><input type='number' name='TimerWaitBefore' min=0 value=%i style='width: 120px;'%s>\r\n"
						"<input type='checkbox' name='RestartWaitTimer'%s%s>\r\n"
						"<input type='number' name='WaitBeforeStatus' min=0 value=%i style='width: 120px;'%s></td>\r\n"
						"<td><input type='number' name='TimerOff' min=0 value=%i style='width: 120px;'%s>\r\n"
						"<input type='checkbox' name='RestartOffTimer'%s%s>\r\n"
						"<input type='number' name='OffStatus' min=0 value=%i style='width: 120px;'%s></td>\r\n"
						"<td> Пн:<input type='checkbox' name='Mo'%s%s>\r\n"
							" Вт:<input type='checkbox' name='Tu'%s%s>\r\n"
							" Ср:<input type='checkbox' name='We'%s%s>\r\n"
							" Чт:<input type='checkbox' name='Th'%s%s>\r\n"
							" Пт:<input type='checkbox' name='Fr'%s%s>\r\n"
							" Сб:<input type='checkbox' name='Sa'%s%s>\r\n"
							" Вс:<input type='checkbox' name='Su'%s%s></td>\r\n"
						"<td><input type='number' name='Hour11' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
							":<input type='number' name='Min11' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
							":<input type='number' name='Sec11' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						"-<input type='number' name='Hour12' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
							":<input type='number' name='Min12' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
							":<input type='number' name='Sec12' min=0 max=59 value='%i' style='width: 37px;'%s></td>\r\n"
						"<td>%i</td>",
						session->request_id,
						n,n, (char*)&maActionInfo[n].ActionID, cDisableFlag, 
							(maActionInfo[n].Enabled == 1) ? " checked " : "", cDisableFlag,
							maActionInfo[n].GroupName, cDisableFlag,
							(char*)&maActionInfo[n].SrcID, cDisableFlag, 
							maActionInfo[n].SrcSubNumber, cDisableFlag,
							cDisableFlag,
							((maActionInfo[n].TestType != ACTION_TEST_TYPE_EQUALLY) 
								&& (maActionInfo[n].TestType != ACTION_TEST_TYPE_LESS) 
								&& (maActionInfo[n].TestType != ACTION_TEST_TYPE_MORE)
								&& (maActionInfo[n].TestType != ACTION_TEST_TYPE_AND)
								&& (maActionInfo[n].TestType != ACTION_TEST_TYPE_NOT)) ? "selected" : "", 0,
							(maActionInfo[n].TestType == ACTION_TEST_TYPE_EQUALLY) ? "selected" : "", ACTION_TEST_TYPE_EQUALLY,
							(maActionInfo[n].TestType == ACTION_TEST_TYPE_LESS) ? "selected" : "", ACTION_TEST_TYPE_LESS,
							(maActionInfo[n].TestType == ACTION_TEST_TYPE_MORE) ? "selected" : "", ACTION_TEST_TYPE_MORE,
							(maActionInfo[n].TestType == ACTION_TEST_TYPE_AND) ? "selected" : "", ACTION_TEST_TYPE_AND,
							(maActionInfo[n].TestType == ACTION_TEST_TYPE_NOT) ? "selected" : "", ACTION_TEST_TYPE_NOT,
							GetActionCodeName(maActionInfo[n].SrcStatus, SrcStatusBuff, 64, 4),
							GetActionCodeName(maActionInfo[n].SrcStatus, SrcStatusBuff2, 64, 2), cDisableFlag, 
							(char*)&maActionInfo[n].TestModuleID, cDisableFlag, 
							(char*)&maActionInfo[n].Test3ModuleID, cDisableFlag, 
							maActionInfo[n].TestModuleSubNumber, cDisableFlag, 
							maActionInfo[n].Test3ModuleSubNumber, cDisableFlag, 
							cDisableFlag,
							((maActionInfo[n].Test2Type != ACTION_TEST_TYPE_EQUALLY) 
								&& (maActionInfo[n].Test2Type != ACTION_TEST_TYPE_LESS) 
								&& (maActionInfo[n].Test2Type != ACTION_TEST_TYPE_MORE)
								&& (maActionInfo[n].Test2Type != ACTION_TEST_TYPE_AND)
								&& (maActionInfo[n].Test2Type != ACTION_TEST_TYPE_NOT)) ? "selected" : "", 0,
							(maActionInfo[n].Test2Type == ACTION_TEST_TYPE_EQUALLY) ? "selected" : "", ACTION_TEST_TYPE_EQUALLY,
							(maActionInfo[n].Test2Type == ACTION_TEST_TYPE_LESS) ? "selected" : "", ACTION_TEST_TYPE_LESS,
							(maActionInfo[n].Test2Type == ACTION_TEST_TYPE_MORE) ? "selected" : "", ACTION_TEST_TYPE_MORE,						
							(maActionInfo[n].Test2Type == ACTION_TEST_TYPE_AND) ? "selected" : "", ACTION_TEST_TYPE_AND,						
							(maActionInfo[n].Test2Type == ACTION_TEST_TYPE_NOT) ? "selected" : "", ACTION_TEST_TYPE_NOT,						
							cDisableFlag,
							((maActionInfo[n].Test3Type != ACTION_TEST_TYPE_EQUALLY) 
								&& (maActionInfo[n].Test3Type != ACTION_TEST_TYPE_LESS) 
								&& (maActionInfo[n].Test3Type != ACTION_TEST_TYPE_MORE)
								&& (maActionInfo[n].Test3Type != ACTION_TEST_TYPE_AND)
								&& (maActionInfo[n].Test3Type != ACTION_TEST_TYPE_NOT)) ? "selected" : "", 0,
							(maActionInfo[n].Test3Type == ACTION_TEST_TYPE_EQUALLY) ? "selected" : "", ACTION_TEST_TYPE_EQUALLY,
							(maActionInfo[n].Test3Type == ACTION_TEST_TYPE_LESS) ? "selected" : "", ACTION_TEST_TYPE_LESS,
							(maActionInfo[n].Test3Type == ACTION_TEST_TYPE_MORE) ? "selected" : "", ACTION_TEST_TYPE_MORE,
							(maActionInfo[n].Test3Type == ACTION_TEST_TYPE_AND) ? "selected" : "", ACTION_TEST_TYPE_AND,
							(maActionInfo[n].Test3Type == ACTION_TEST_TYPE_NOT) ? "selected" : "", ACTION_TEST_TYPE_NOT,
							GetActionCodeName(maActionInfo[n].TestModuleStatus, TestStatusBuff, 64, 4),
							GetActionCodeName(maActionInfo[n].TestModuleStatus, TestStatusBuff2, 64, 2), cDisableFlag, 
							GetActionCodeName(maActionInfo[n].Test3ModuleStatus, Test3StatusBuff, 64, 4),
							GetActionCodeName(maActionInfo[n].Test3ModuleStatus, Test3StatusBuff2, 64, 2), cDisableFlag, 
							(char*)&maActionInfo[n].DestID, cDisableFlag, 
							GetActionCodeName(maActionInfo[n].DestSubNumber, DestSubNumberBuff, 64, 4),
							GetActionCodeName(maActionInfo[n].DestSubNumber, DestSubNumberBuff2, 64, 2), cDisableFlag, 
							GetActionCodeName(maActionInfo[n].DestStatus, DestStatusBuff, 64, 4),
							GetActionCodeName(maActionInfo[n].DestStatus, DestStatusBuff2, 64, 2), cDisableFlag,
							maActionInfo[n].Name, cDisableFlag, 
							 cDisableFlag, (maActionInfo[n].AfterAccept == 0) ? "selected" : "", 0,
							(maActionInfo[n].AfterAccept == POST_ACTION_STOP) ? "selected" : "", POST_ACTION_STOP,
							(maActionInfo[n].AfterAccept == POST_ACTION_OFF) ? "selected" : "", POST_ACTION_OFF,
							(maActionInfo[n].AfterAccept == POST_ACTION_OFF_STOP) ? "selected" : "", POST_ACTION_OFF_STOP,
							(maActionInfo[n].AfterAccept == POST_ACTION_STOP_IF_WAIT) ? "selected" : "", POST_ACTION_STOP_IF_WAIT,							
							maActionInfo[n].TimerWaitBefore, cDisableFlag, 
							maActionInfo[n].RestartTimer & 1 ? " checked" : "", cDisableFlag,
							maActionInfo[n].TimerWaitBeforeStatus, cDisableFlag, 
							maActionInfo[n].TimerOff, cDisableFlag,
							maActionInfo[n].RestartTimer & 2 ? " checked" : "", cDisableFlag,
							maActionInfo[n].TimerOffStatus, cDisableFlag,
							maActionInfo[n].WeekDays & 1 ? " checked" : "", cDisableFlag,
							maActionInfo[n].WeekDays & 2 ? " checked" : "", cDisableFlag,
							maActionInfo[n].WeekDays & 4 ? " checked" : "", cDisableFlag,
							maActionInfo[n].WeekDays & 8 ? " checked" : "", cDisableFlag,
							maActionInfo[n].WeekDays & 16 ? " checked" : "", cDisableFlag,
							maActionInfo[n].WeekDays & 32 ? " checked" : "", cDisableFlag,
							maActionInfo[n].WeekDays & 64 ? " checked" : "", cDisableFlag,
							fl1[0], cDisableFlag, fl2[0], cDisableFlag, fl3[0], cDisableFlag, 
							fl1[1], cDisableFlag, fl2[1], cDisableFlag, fl3[1], cDisableFlag,
							maActionInfo[n].CounterExec);		
			strcat(msg_tx, msg_subbody);
			if (maActionInfo[n].Enabled == -1) 
				strcat(msg_tx, "<td><button type='submit' formaction='/evntactions/recover'>Вернуть</button></td></form></tr>\r\n");
				else
				strcat(msg_tx, "<td><button type='submit'>Сохранить</button>\r\n"
								"<button type='submit' formaction='/evntactions/refresh'>Обнулить счет</button>\r\n"
								"<button type='submit' formaction='/evntactions/delete'>Удалить</button>\r\n"
								"<button type='submit' formaction='/evntactions/execute'>Выполнить</button>\r\n"
								"<button type='submit' formaction='/evntactions/get_up'>Выше</button>\r\n"
								"<button type='submit' formaction='/evntactions/get_down'>Ниже</button></td></form></tr>\r\n");
			if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
		}
	}	
	
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody, 
					"<tr><form action='/evntactions/add'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='number' name='NewPos' value=%i style='width: 50px;'></td>\r\n"					
					"<td><input type='text' name='ActionID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='En'></td>\r\n"
					"<td><input type='text' name='Group' maxlength=122 value='' style='width: 180px;'></td>\r\n"
					"<td><input type='text' list='idlist' name='SrcID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='SrcSubNumber' min=0 value=0 style='width: 60px;'></td>\r\n"
					"<td><select name='TestType' style='width: 140px;'>\r\n"
						"<option selected value='%i'>Равно</option>\r\n"
						"<option value='%i'>Меньше</option>\r\n"
						"<option value='%i'>Больше</option>\r\n"
						"<option value='%i'>Лог. И</option>\r\n"
						"<option value='%i'>Не равно</option>\r\n"
						"</select></td>\r\n"
					"<td><input type='text' list='statuslist' name='SrcStatus' maxlength=64 value='' style='width: 200px;'></td>\r\n"
					"<td><input type='text' list='idlist' name='TestModuleID' maxlength=4 value='' style='width: 60px;'><br />\r\n"
						"<input type='text' list='idlist' name='Test3ModuleID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='TestModuleSubNumber' min=0 value=0 style='width: 60px;'><br />\r\n"
						"<input type='number' name='Test3ModuleSubNumber' min=0 value=0 style='width: 60px;'></td>\r\n"
					"<td><select name='Test2Type' style='width: 140px;'>\r\n"
						"<option selected value='%i'>Равно</option>\r\n"
						"<option value='%i'>Меньше</option>\r\n"
						"<option value='%i'>Больше</option>\r\n"
						"<option value='%i'>Лог. И</option>\r\n"
						"<option value='%i'>Не равно</option>\r\n"
						"</select><br />\r\n"
						"<select name='Test3Type' style='width: 140px;'>\r\n"
						"<option selected value='%i'>Равно</option>\r\n"
						"<option value='%i'>Меньше</option>\r\n"
						"<option value='%i'>Больше</option>\r\n"
						"<option value='%i'>Лог. И</option>\r\n"
						"<option value='%i'>Не равно</option>\r\n"
						"</select></td>\r\n"
					"<td><input type='text' list='statuslist' name='TestModuleStatus' maxlength=64 value='0' style='width: 100px;'><br />\r\n"
						"<input type='text' list='statuslist' name='Test3ModuleStatus' maxlength=64 value='0' style='width: 100px;'></td>\r\n"					
					"<td><input type='text' list='idlist' name='DestID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='text' list='numberlist' name='DestSubNumber' maxlength=64 value='' style='width: 100px;'></td>\r\n"
					"<td><input type='text' list='statuslist' name='DestStatus' maxlength=64 value='' style='width: 200px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=63 value='' style='width: 300px;'></td>\r\n"
					"<td><select name='AfterAccept' style='width: 140px;'>\r\n"
						"<option selected value='%i'>Ничего</option>\r\n"
						"<option value='%i'>Остановить</option>\r\n"
						"<option value='%i'>Отключить</option>\r\n"
						"<option value='%i'>Остановить и отключить</option>\r\n"
						"<option value='%i'>Остановить если повторное</option>\r\n"
						"</select></td>\r\n"
					"<td><input type='number' name='TimerWaitBefore' min=0 value=0 style='width: 120px;'>\r\n"
					"<input type='checkbox' name='RestartWaitTimer'></td>\r\n"
					"<td><input type='number' name='TimerOff' min=0 value=0 style='width: 120px;'>\r\n"
					"<input type='checkbox' name='RestartOffTimer'></td>\r\n"
					"<td> Пн:<input type='checkbox' name='Mo' checked>\r\n"
						" Вт:<input type='checkbox' name='Tu' checked>\r\n"
						" Ср:<input type='checkbox' name='We' checked>\r\n"
						" Чт:<input type='checkbox' name='Th' checked>\r\n"
						" Пт:<input type='checkbox' name='Fr' checked>\r\n"
						" Сб:<input type='checkbox' name='Sa' checked>\r\n"
						" Вс:<input type='checkbox' name='Su' checked></td>\r\n"
					"<td><input type='number' name='Hour11' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min11' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec11' min=0 max=59 value='0' style='width: 37px;'>\r\n"
					"-<input type='number' name='Hour12' min=0 max=24 value='23' style='width: 37px;'>\r\n"
						":<input type='number' name='Min12' min=0 max=59 value='59' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec12' min=0 max=59 value='59' style='width: 37px;'></td>\r\n"
					"<td></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n",
					session->request_id,
					iActionCnt,
					ACTION_TEST_TYPE_EQUALLY, ACTION_TEST_TYPE_LESS, ACTION_TEST_TYPE_MORE, ACTION_TEST_TYPE_AND, ACTION_TEST_TYPE_NOT, 
					ACTION_TEST_TYPE_EQUALLY, ACTION_TEST_TYPE_LESS, ACTION_TEST_TYPE_MORE, ACTION_TEST_TYPE_AND, ACTION_TEST_TYPE_NOT, 
					ACTION_TEST_TYPE_EQUALLY, ACTION_TEST_TYPE_LESS, ACTION_TEST_TYPE_MORE, ACTION_TEST_TYPE_AND, ACTION_TEST_TYPE_NOT,
					0, POST_ACTION_STOP, POST_ACTION_OFF, POST_ACTION_OFF_STOP, POST_ACTION_STOP_IF_WAIT, iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "evntactions", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_mnlaction_exec(int *pParams)
{
	int ret = 0;
	unsigned int uiData[3];
	char cData[128];
	
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionManualCnt) && iActionManualCnt)
	{
		//amiActionManual[pParams[0]].Access = pParams[1];
		uiData[0] = amiActionManual[pParams[0]].ID;
		uiData[1] = amiActionManual[pParams[0]].SubNumber;
		uiData[2] = amiActionManual[pParams[0]].Status;		
		memcpy(cData, amiActionManual[pParams[0]].Name, 128);
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	dbgprintf(4, "AddModuleEventInList ID:%.4s, Sub:%i, Stat:%i, Mode:%i\n", &uiData[0], uiData[1], uiData[2], 1);
	if (ret) AddModuleEventInList(uiData[0], uiData[1], uiData[2], cData, strlen(cData), 1);
	return 1;
}

int WEB_mnlaction_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	iActionManualCnt++;
	amiActionManual = (ACTION_MANUAL_INFO*)DBG_REALLOC(amiActionManual, sizeof(ACTION_MANUAL_INFO)*iActionManualCnt);
	memset(&amiActionManual[iActionManualCnt-1], 0, sizeof(ACTION_MANUAL_INFO));
	amiActionManual[iActionManualCnt-1].Access = pParams[1];
	amiActionManual[iActionManualCnt-1].ID = pParams[2];
	amiActionManual[iActionManualCnt-1].SubNumber = pParams[3];
	amiActionManual[iActionManualCnt-1].Status = pParams[4];
	if (strlen(&strParam[0]) > 128) memcpy(amiActionManual[iActionManualCnt-1].Name, &strParam[0], 128);
		else memcpy(amiActionManual[iActionManualCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	SaveMnlActions();
	return 1;
}

int WEB_mnlaction_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionManualCnt) && iActionManualCnt)
	{
		ACTION_MANUAL_INFO *ami = (ACTION_MANUAL_INFO*)DBG_MALLOC(sizeof(ACTION_MANUAL_INFO)*(iActionManualCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iActionManualCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ami[clk], &amiActionManual[i], sizeof(ACTION_MANUAL_INFO));
				clk++;
			}
		DBG_FREE(amiActionManual);
		amiActionManual = ami;
		iActionManualCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	if (ret) SaveMnlActions();
	return 1;
}

int WEB_mnlaction_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iActionManualCnt) && iActionManualCnt)
	{
		memset(&amiActionManual[pParams[0]], 0, sizeof(ACTION_MANUAL_INFO));
		amiActionManual[pParams[0]].Access = pParams[1];
		amiActionManual[pParams[0]].ID = pParams[2];
		amiActionManual[pParams[0]].SubNumber = pParams[3];
		amiActionManual[pParams[0]].Status = pParams[4];
		if (strlen(&strParam[0]) > 128) memcpy(amiActionManual[pParams[0]].Name, &strParam[0], 128);
			else memcpy(amiActionManual[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	if (ret) SaveMnlActions();
	return 1;
}

int WEB_mnlactions_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	TestMnlActions(1);
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	char StatusBuff[64];
	char StatusBuff2[64];
	char SubNumberBuff[64];	
	char SubNumberBuff2[64];	
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/mnlactions/\"><h1>РУЧНЫЕ ДЕЙСТВИЯ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	strcat(msg_tx, "<br />Справочник действий:<select style='width: 240px;'%s>\r\n");
	for (n = MENU_PAGE_MAIN; n < SYSTEM_CMD_MAX; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", n, GetActionCodeName(n, NULL, 0, 5));
		strcat(msg_tx, msg_subbody);
	}		
	strcat(msg_tx, "	</select>\r\n");
	
	DBG_MUTEX_LOCK(&ircode_mutex);
	strcat(msg_tx, "   Справочник IR:<select style='width: 240px;'%s>\r\n");
	for (n = 0; n < iIRCommandCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", n, (char*)&mIRCommandList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	strcat(msg_tx, "	</select>\r\n");
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	strcat(msg_tx, "   Справочник модулей:<select style='width: 240px;'%s>\r\n");
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%i'>[%.4s] %s</option>\r\n", n, (char*)&miModuleList[n].ID, miModuleList[n].Name);
		strcat(msg_tx, msg_subbody);
	}		
	strcat(msg_tx, "	</select><br />\r\n");
	
	strcat(msg_tx, "<datalist id='idlist'>\r\n");
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&miModuleList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	strcat(msg_tx, "</datalist>\r\n");	
	strcat(msg_tx, "<datalist id='numberlist'>\r\n");
	for (n = SYSTEM_CMD_NULL + 1; n < MENU_PAGE_MAX; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%s'></option>\r\n", GetActionCodeName(n, SubNumberBuff, 64, 0));
		strcat(msg_tx, msg_subbody);
	}
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&miModuleList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	strcat(msg_tx, "</datalist>\r\n");	
	
	strcat(msg_tx, "<datalist id='statuslist'>\r\n");
	for (n = MENU_PAGE_MAX + 1; n < SYSTEM_CMD_MAX; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%s'></option>\r\n", GetActionCodeName(n, SubNumberBuff, 64, 0));
		strcat(msg_tx, msg_subbody);
	}
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&miModuleList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	DBG_MUTEX_LOCK(&ircode_mutex);
	for (n = 0; n < iIRCommandCnt; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option value='%.4s'></option>\r\n", (char*)&mIRCommandList[n].ID);
		strcat(msg_tx, msg_subbody);		
	}
	DBG_MUTEX_UNLOCK(&ircode_mutex);
	strcat(msg_tx, "</datalist>\r\n");	
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Доступ</th><th>Идентификатор</th><th>Подномер</th><th>Статус</th><th>Название</th><th>Действие</th></tr>");
	
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	
	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iActionManualCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurMnlActionsPage = iPage;
	iCurPage = iCurMnlActionsPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurMnlActionsPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurMnlActionsPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iActionManualCnt) iLastOption = iActionManualCnt;
	
	WEB_pages_list(msg_tx, "mnlactions", iSubPages, iCurPage, "");	
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody,
					"<tr><form action='/mnlactions/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='number' name='Access' min=0 max=1000 value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='text' list='idlist' name='ModuleID' maxlength=4 value='%.4s' style='width: 60px;'></td>\r\n"
					"<td><input type='text' name='SubNumber2' value='%s' style='width: 60px;' disabled>\r\n"
					"<input type='text' list='numberlist' name='SubNumber' maxlength=64 value='%s' style='width: 300px;'></td>\r\n"
					"<td><input type='text' name='Status2' value='%s' style='width: 60px;' disabled>\r\n"
					"<input type='text' list='statuslist' name='Status' maxlength=64 value='%s' style='width: 300px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=128 value='%s' style='width: 300px;'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/mnlactions/delete'>Удалить</button>\r\n"
					"<button type='submit' formaction='/mnlactions/execute'>Выполнить</button></td></form></tr>\r\n",
					session->request_id,
					n,n, amiActionManual[n].Access,
					(char*)&amiActionManual[n].ID, 
					GetActionCodeName(amiActionManual[n].SubNumber, SubNumberBuff, 64, 4),
					GetActionCodeName(amiActionManual[n].SubNumber, SubNumberBuff2, 64, 2),
					GetActionCodeName(amiActionManual[n].Status, StatusBuff, 64, 4),
					GetActionCodeName(amiActionManual[n].Status, StatusBuff2, 64, 2),
					amiActionManual[n].Name);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/mnlactions/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='number' name='Access' min=0 max=1000 value=0 style='width: 60px;'></td>\r\n"
					"<td><input type='text' list='idlist' name='ModuleID' maxlength=4 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='text' list='numberlist' name='SubNumber' maxlength=64 value='' style='width: 300px;'></td>\r\n"
					"<td><input type='text' list='statuslist' name='Status' maxlength=64 value='' style='width: 300px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=128 value='' style='width: 300px;'></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "mnlactions", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_camrect_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&rectangle_mutex);
	iRectangleCnt++;
	riRectangleList = (RECT_INFO*)DBG_REALLOC(riRectangleList, sizeof(RECT_INFO)*iRectangleCnt);
	memset(&riRectangleList[iRectangleCnt-1], 0, sizeof(RECT_INFO));
	riRectangleList[iRectangleCnt-1].Enabled = pParams[1];
	riRectangleList[iRectangleCnt-1].Group = pParams[2];
	riRectangleList[iRectangleCnt-1].X1 = pParams[3];
	riRectangleList[iRectangleCnt-1].Y1 = pParams[4];
	riRectangleList[iRectangleCnt-1].X2 = pParams[5];
	riRectangleList[iRectangleCnt-1].Y2 = pParams[6];
	if (strlen(&strParam[0]) > 63) memcpy(riRectangleList[iRectangleCnt-1].Name, &strParam[0], 63);
		else memcpy(riRectangleList[iRectangleCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	riRectangleList[iRectangleCnt-1].ID = pParams[7];		
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
	SaveCamRectangles();
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	int ret;
	for (ret = 0; ret < iModuleCnt; ret++)
		if ((miModuleList[ret].Local == 1) && 
				(miModuleList[ret].Type == MODULE_TYPE_CAMERA) &&
				(miModuleList[ret].Enabled & 1)) 
				{
					miModuleList[ret].Status[5] |= 8;					 
					break;
				}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	return 1;
}

int WEB_camrect_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&rectangle_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iRectangleCnt) && iRectangleCnt)
	{
		RECT_INFO *ri = (RECT_INFO*)DBG_MALLOC(sizeof(RECT_INFO)*(iRectangleCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iRectangleCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ri[clk], &riRectangleList[i], sizeof(RECT_INFO));
				clk++;
			}
		DBG_FREE(riRectangleList);
		riRectangleList = ri;
		iRectangleCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
	if (ret) SaveCamRectangles();
	return 1;
}

int WEB_camrect_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&rectangle_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iRectangleCnt) && iRectangleCnt)
	{
		memset(&riRectangleList[pParams[0]], 0, sizeof(RECT_INFO));
		riRectangleList[pParams[0]].Enabled = pParams[1];
		riRectangleList[pParams[0]].Group = pParams[2];
		riRectangleList[pParams[0]].X1 = pParams[3];
		riRectangleList[pParams[0]].Y1 = pParams[4];
		riRectangleList[pParams[0]].X2 = pParams[5];
		riRectangleList[pParams[0]].Y2 = pParams[6];
		if (strlen(&strParam[0]) > 63) memcpy(riRectangleList[pParams[0]].Name, &strParam[0], 63);
			else memcpy(riRectangleList[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));		
		riRectangleList[pParams[0]].ID = pParams[7];		
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
	
	if (ret) 
	{
		SaveCamRectangles();
		
		DBG_MUTEX_LOCK(&modulelist_mutex);
		for (ret = 0; ret < iModuleCnt; ret++)
			if ((miModuleList[ret].Local == 1) && 
				(miModuleList[ret].Type == MODULE_TYPE_CAMERA) &&
				(miModuleList[ret].Enabled & 1)) 
				{
					miModuleList[ret].Status[5] |= 8;					 
					break;
				}	
		DBG_MUTEX_UNLOCK(&modulelist_mutex);		
	}
	return 1;
}

int WEB_camrect_view(int *pParams)
{
	int n;
	int ret = 0;
	int mNum;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Local == 1) && 
			(miModuleList[n].Type == MODULE_TYPE_CAMERA) &&
			(miModuleList[n].Enabled & 1)) 
			{
				miModuleList[n].Status[5] |= 4;
				mNum = n;
				ret = 1; 
				break;
			}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret)
	{
		DBG_MUTEX_LOCK(&system_mutex);
		iCurrentShowRectangle = pParams[0];
		DBG_MUTEX_UNLOCK(&system_mutex);
		for (n = 0; n < 15; n++)
		{
			DBG_MUTEX_LOCK(&modulelist_mutex);
			ret = miModuleList[mNum].Status[5] & 4;
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			if (ret == 0) break;
			usleep(200000);
		}
	}	
	return 1;
}

int WEB_camrects_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&rectangle_mutex);
	TestCamRectangles(1);
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/camrects/\"><h1>КВАДРАТЫ КАМЕРЫ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&rectangle_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Идентификатор</th><th>Включено</th><th>Группа</th><th>Верхний левый угол (\%)</th><th>Правый нижний угол (\%)</th><th>Наименование</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iRectangleCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurCamRectPage = iPage;
	iCurPage = iCurCamRectPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurCamRectPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurCamRectPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iRectangleCnt) iLastOption = iRectangleCnt;
	
	WEB_pages_list(msg_tx, "camrects", iSubPages, iCurPage, "");	
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, 
					"<tr><form action='/camrects/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='RectID' value='%.4s' maxlength=4 style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='En'%s></td>\r\n"
					"<td><input type='number' name='Group' min=0 max=50 value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='PosX1' min=0 max=100 value=%i style='width: 60px;'>\r\n"
					"X<input type='number' name='PosY1' min=0 max=100 value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='PosX2' min=0 max=100 value=%i style='width: 60px;'>\r\n"
					"X<input type='number' name='PosY2' min=0 max=100 value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=64 value='%s' style='width: 300px;'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/camrects/show'>Показать</button>"
					"<button type='submit' formaction='/camrects/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id, 
					n,n, 
					(char*)&riRectangleList[n].ID,
					riRectangleList[n].Enabled ? " checked " : "", 
					riRectangleList[n].Group, 
					riRectangleList[n].X1, riRectangleList[n].Y1,
					riRectangleList[n].X2, riRectangleList[n].Y2,
					riRectangleList[n].Name);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/camrects/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='text' name='RectID' value='' maxlength=4 style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='En'></td>\r\n"
					"<td><input type='number' name='Group' min=0 max=50 value=0 style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='PosX1' min=0 max=100 value=0 style='width: 60px;'>\r\n"
					"X<input type='number' name='PosY1' min=0 max=100 value=0 style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='PosX2' min=0 max=100 value=0 style='width: 60px;'>\r\n"
					"X<input type='number' name='PosY2' min=0 max=100 value=0 style='width: 60px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=64 value='' style='width: 300px;'></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id, iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "camrects", iSubPages, iCurPage, "");
	
	strcat(msg_tx,	"<br /><img src='/images/image_camrect' alt='Квадраты'>\r\n");
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_camera_refresh(int *pParams)
{
	int n;
	int ret = 0;
	int mNum;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Local == 1) && 
			(miModuleList[n].Type == MODULE_TYPE_CAMERA) &&
			(miModuleList[n].Enabled & 1)) 
			{
				miModuleList[n].Status[5] |= 2;
				mNum = n;
				ret = 1; 
				break;
			}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret)
	{
		for (n = 0; n < 15; n++)
		{
			DBG_MUTEX_LOCK(&modulelist_mutex);
			ret = miModuleList[mNum].Status[5] & 2;
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			if (ret == 0) break;
			usleep(200000);
		}
	}	
	return 1;
}

int WEB_camera_change(int *pParams)
{
	int ret;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	ispCameraImageSettings.Updated = 1;
	ispCameraImageSettings.Brightness = pParams[6];
	ispCameraImageSettings.ColorSaturation = pParams[19];
	ispCameraImageSettings.Contrast = pParams[17];
	ispCameraImageSettings.Sharpness = pParams[18];
	ispCameraImageSettings.Exposure.Mode = pParams[0];
	//ispCameraImageSettings.Focus;
	ispCameraImageSettings.ImageFilter = pParams[15];
	
	ispCameraImageSettings.WhiteBalance.Mode = pParams[16];
	ispCameraImageSettings.FrameLeftCrop = pParams[20];
	ispCameraImageSettings.FrameRightCrop = pParams[21];
	ispCameraImageSettings.FrameUpCrop = pParams[22];
	ispCameraImageSettings.FrameDownCrop = pParams[23];
	
	ispCameraImageSettings.AutoBrightControl = pParams[7];	
	ispCameraImageSettings.DestBrightControl = pParams[8];
	
	ispCameraImageSettings.MainSettings = pParams[1];	
	ispCameraImageSettings.PrevSettings = pParams[2];
	ispCameraImageSettings.PrevShowLevel = pParams[3];
	ispCameraImageSettings.PrevColorLevel = pParams[4] & 255;	
	ispCameraImageSettings.PrevBrigLevel = pParams[5] & 255;
	ispCameraImageSettings.HighLightMode = pParams[12];
	ispCameraImageSettings.HighLightBright = pParams[13];
	ispCameraImageSettings.HighLightAmplif = pParams[14];
	
	ispCameraImageSettings.ISOControl = pParams[25]; 
	ispCameraImageSettings.HardAutoISOControl = (pParams[24] & 1) ? 1 : 0;
	ispCameraImageSettings.SoftAutoISOControl = (pParams[24] & 2) ? 1 : 0;
	ispCameraImageSettings.DestAutoISOControl = pParams[26]; 
	ispCameraImageSettings.EVControl = pParams[27]; 
	//ispCameraImageSettings.HardAutoEVControl;
	ispCameraImageSettings.SoftAutoEVControl = (pParams[24] & 4) ? 1 : 0;
	ispCameraImageSettings.DestAutoEVControl = pParams[28]; 
	
	
	for (ret = 0; ret < iModuleCnt; ret++)
		if ((miModuleList[ret].Local == 1) && 
				(miModuleList[ret].Type == MODULE_TYPE_CAMERA) &&
				(miModuleList[ret].Enabled & 1)) 
				{
					miModuleList[ret].Status[2] = pParams[0];		//iCurExp
					miModuleList[ret].Status[15] = pParams[6];		//iCurBright
					if (pParams[9]) miModuleList[ret].Status[9] = 0;
					if (pParams[10]) miModuleList[ret].Status[10] = 0;
					miModuleList[ret].Status[16] = pParams[7];		//uiAutoBrightControl
					miModuleList[ret].Status[17] = pParams[8];		//uiDestBrightControl
					miModuleList[ret].Status[19] = pParams[15];		//iCurFilter
					miModuleList[ret].Status[20] = pParams[16];		//iCurBalance
					miModuleList[ret].Status[21] = pParams[17];		//iCurContrast
					miModuleList[ret].Status[22] = pParams[18];		//iCurSharpness
					miModuleList[ret].Status[23] = pParams[19];		//iCurSaturation	
					omx_set_image_sensor_module_params(&ispCameraImageSettings, &miModuleList[ret]);						
					break;
				}
	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	DBG_MUTEX_LOCK(&OMX_mutex);
	omx_resource_priority = pParams[11] & 3;
	DBG_MUTEX_UNLOCK(&OMX_mutex);		
	return 1;
}

int WEB_camera_respond(char *msg_tx, WEB_SESSION *session)
{
	int ret = 0;
	int uiCurrEV = 0;
	int uiCurrISO = 400;
	int i;
	
	image_sensor_params ispCSI;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (i = 0; i < iModuleCnt; i++)
		if ((miModuleList[i].Local == 1) && 
				(miModuleList[i].Type == MODULE_TYPE_CAMERA) &&
				(miModuleList[i].Enabled & 1)) 
				{
					uiCurrISO = miModuleList[i].Status[24];
					uiCurrEV = miModuleList[i].Status[25];
					ret = 1;
					break;
				}
	memcpy(&ispCSI, &ispCameraImageSettings, sizeof(image_sensor_params));
	int iKeepRatio = ispCameraImageSettings.KeepRatio;
	int iCurExp = ispCameraImageSettings.Exposure.Mode;
	int iCurBright = ispCameraImageSettings.Brightness;
	int iCurContrast = ispCameraImageSettings.Contrast;
	int iCurSharpness = ispCameraImageSettings.Sharpness;
	int iCurSaturation = ispCameraImageSettings.ColorSaturation;
	int iCurFilter = ispCameraImageSettings.ImageFilter;
	int iCurBalance = ispCameraImageSettings.WhiteBalance.Mode;
	unsigned int uiAutoBrightControl = ispCameraImageSettings.AutoBrightControl;
	unsigned int uiDestBrightControl = ispCameraImageSettings.DestBrightControl;
	unsigned int uiMainSettings = ispCameraImageSettings.MainSettings;	
	unsigned int uiPrevSettings = ispCameraImageSettings.PrevSettings;
	unsigned int uiPrevShowLevel = ispCameraImageSettings.PrevShowLevel;
	unsigned int uiPrevColorLevel = ispCameraImageSettings.PrevColorLevel;
	unsigned int uiPrevBrigLevel = ispCameraImageSettings.PrevBrigLevel;
	int iHighLightMode = ispCameraImageSettings.HighLightMode;
	int iHighLightBright = ispCameraImageSettings.HighLightBright;
	int iHighLightAmplif = ispCameraImageSettings.HighLightAmplif;
	unsigned int uiLeftCrop = ispCameraImageSettings.FrameLeftCrop;
	unsigned int uiRightCrop = ispCameraImageSettings.FrameRightCrop;
	unsigned int uiUpCrop = ispCameraImageSettings.FrameUpCrop;
	unsigned int uiDownCrop = ispCameraImageSettings.FrameDownCrop;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	
	DBG_MUTEX_LOCK(&OMX_mutex);
	char cGPUResources = omx_resource_priority;
	DBG_MUTEX_UNLOCK(&OMX_mutex);	
	
	char *msg_subbody = (char*)DBG_MALLOC(8192);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/camera/\"><h1>КАМЕРА</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	memset(msg_subbody, 0, 8192);
	sprintf(msg_subbody,"<form action='/camera/change'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"Ресурсы:<select name='Resources' style='width: 140px;'%s>\r\n"
										"		<option %s value='%i'>Разделять</option>\r\n"
										"		<option %s value='%i'>Забирать</option>\r\n"
										"		<option %s value='%i'>Отдавать</option>\r\n"
										"	</select><br />\r\n"
						"Экспозиция:<select name='Type' style='width: 140px;'%s>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"	</select><br />\r\n"
						"Фильтр:<select name='Filter' style='width: 140px;'%s>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"	</select><br />\r\n"
						"Баланс белого:<select name='Balance' style='width: 140px;'%s>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"		<option %s value='%i'>%s</option>\r\n"
						"	</select><br />\r\n"
						"Обрезка:<br />\r\n"
							"&ensp;&ensp;Сохранять пропорции:<input type='checkbox' name='KeepRatio'%s disabled><br />\r\n"
							"&ensp;&ensp;Слева:<input type='number' name='LeftCrop' min='0' max='100' step='1' value='%i' style='width: 100px;'><br />\r\n"
							"&ensp;&ensp;Справа:<input type='number' name='RightCrop' min='0' max='100' step='1' value='%i' style='width: 100px;'><br />\r\n"
							"&ensp;&ensp;Сверху:<input type='number' name='UpCrop' min='0' max='100' step='1' value='%i' style='width: 100px;'><br />\r\n"
							"&ensp;&ensp;Снизу:<input type='number' name='DownCrop' min='0' max='100' step='1' value='%i' style='width: 100px;'><br />\r\n"
						"Яркость: <input type='range' name='ManBright' min='1' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
						"Контраст: <input type='range' name='Contrast' min='-100' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
						"Резкость: <input type='range' name='Sharpness' min='-100' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
						"Насыщенность: <input type='range' name='Saturation' min='-100' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
						"Автояркость:<input type='checkbox' name='AutoBright'%s%s><br />\r\n"
							"&ensp;&ensp;Поддерживаемая яркость: <input type='range' name='DestBright' min='1' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
						"ISO: <input type='range' name='ISOvalue' min='100' max='800' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
							"&ensp;&ensp;Автоматически (HW):<input type='checkbox' name='AutoISOhw'%s%s><br />\r\n"
							"&ensp;&ensp;Автоматически (SW):<input type='checkbox' name='AutoISOsw'%s%s><br />\r\n"
							"&ensp;&ensp;&ensp;&ensp;Текущий:<input type='range' name='ISOcurrent' min='100' max='800' step='1' value='%i' style='width: 400px;' disabled>(%i)<br />\r\n"
							"&ensp;&ensp;&ensp;&ensp;Поддерживаемая яркость: <input type='range' name='DestISOBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"										
						"EV компенсация: <input type='range' name='EVvalue' min='-24' max='24' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"										
							"&ensp;&ensp;Автоматически (SW):<input type='checkbox' name='AutoEVsw'%s%s><br />\r\n"
							"&ensp;&ensp;&ensp;&ensp;Текущий:<input type='range' name='EVcurrent' min='-24' max='24' step='1' value='%i' style='width: 400px;' disabled>(%i)<br />\r\n"
							"&ensp;&ensp;Поддерживаемая яркость: <input type='range' name='DestEVBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"										
						"Режим подсветки:<input type='checkbox' name='HighLight'%s%s><br />\r\n"
							"&ensp;&ensp;Уровень яркости для активации: <input type='range' name='HLBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
							"&ensp;&ensp;Уровень усиления: <input type='range' name='HLAmplif' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
						"Настройки основного потока:<select name='MainCam' style='width: 300px;'%s>\r\n"
							"		<option %s value='0'>Нормальный режим</option>\r\n"
							"		<option %s value='1'>Показывать зоны обнаружения</option>\r\n"
							"	</select>\r\n"
						"<br />\r\n"
						"Настройки дополнительного потока:<select name='PrevCam' style='width: 300px;'%s>\r\n"
							"		<option %s value='0'>Нормальный режим</option>\r\n"
							"		<option %s value='1'>Показывать яркостный детектор</option>\r\n"
							"		<option %s value='2'>Показывать цветовой детектор</option>\r\n"							
							"		<option %s value='3'>Показывать яркостный + цветовой детектор</option>\r\n"
							"	</select>\r\n"
						"<br />\r\n"
							"  Уровень усиления: <input type='range' name='PrevLevel' min='1' max='30' step='1' value='%i' style='width: 300px;' %s>\r\n"
							"  Учитывать порог детектора: <input type='checkbox' name='PrevGane'%s%s>\r\n"
						"<br />\r\n"
						"Уровень яркостного порога:<input type='number' name='BrigSens' min=0 max=255 value='%i' style='width: 50px;'>\r\n"
						"<br />\r\n"
						"Отображение яркостного детектора:<select name='BrigCam' style='width: 300px;'%s>\r\n"
							"		<option %s value='0'>Только отличия</option>\r\n"
							"		<option %s value='1'>Инвертировать</option>\r\n"
							"		<option %s value='2'>Яркое пятно</option>\r\n"						
							"		<option %s value='3'>Темное пятно</option>\r\n"		
							"	</select>\r\n"						
						"<br />\r\n"
						"Уровень цветового порога:<input type='number' name='ColorSens' min=0 max=255 value='%i' style='width: 50px;'>\r\n"
						"<br />\r\n"
						"Отображение цветового детектора:<select name='ColorCam' style='width: 300px;'%s>\r\n"
							"		<option %s value='0'>Только отличия</option>\r\n"
							"		<option %s value='1'>Инвертировать</option>\r\n"
							"		<option %s value='2'>Красное/Синее пятно</option>\r\n"						
							"		<option %s value='3'>Зеленое.Желтое пятно</option>\r\n"		
							"	</select>\r\n"						
						"<br />\r\n"
						"Показывать зоны обнаружения: <input type='checkbox' name='PrevSquare'%s%s><br />\r\n"
						"Сбросить размер изменений (норм.): <input type='checkbox' name='ResetNormMoveSize'%s><br />\r\n"
						"Сбросить размер изменений (медл.): <input type='checkbox' name='ResetSlowMoveSize'%s><br />\r\n"
						"<input type='submit' value='Сохранить'>\r\n"
						"</form>\r\n"
					
					"<br /><br /><input type='button' value='Обновить снимок' onclick=\"window.location.href='/camera/refresh?req_index=%i'\"><br />\r\n"						
					"<img src='/images/image_cammain' alt='Камера'>\r\n",
					session->request_id,
					ret == 0 ? " disabled " : "",
					(cGPUResources == 0) ? " selected" : "", 0,
					(cGPUResources == 1) ? " selected" : "", 1,
					(cGPUResources == 2) ? " selected" : "", 2,
					ret == 0 ? " disabled " : "",
					(iCurExp == OMX_ExposureControlOff) ? " selected " : "", OMX_ExposureControlOff, GetNameExposure(OMX_ExposureControlOff),
					(iCurExp == OMX_ExposureControlAuto) ? " selected " : "", OMX_ExposureControlAuto, GetNameExposure(OMX_ExposureControlAuto),
					(iCurExp == OMX_ExposureControlNight) ? " selected " : "", OMX_ExposureControlNight, GetNameExposure(OMX_ExposureControlNight),
					(iCurExp == OMX_ExposureControlBackLight) ? " selected " : "", OMX_ExposureControlBackLight, GetNameExposure(OMX_ExposureControlBackLight),
					(iCurExp == OMX_ExposureControlSpotLight) ? " selected " : "", OMX_ExposureControlSpotLight, GetNameExposure(OMX_ExposureControlSpotLight),
					(iCurExp == OMX_ExposureControlSports) ? " selected " : "", OMX_ExposureControlSports, GetNameExposure(OMX_ExposureControlSports),
					(iCurExp == OMX_ExposureControlSnow) ? " selected " : "", OMX_ExposureControlSnow, GetNameExposure(OMX_ExposureControlSnow),
					(iCurExp == OMX_ExposureControlBeach) ? " selected " : "", OMX_ExposureControlBeach, GetNameExposure(OMX_ExposureControlBeach),
					(iCurExp == OMX_ExposureControlLargeAperture) ? " selected " : "", OMX_ExposureControlLargeAperture, GetNameExposure(OMX_ExposureControlLargeAperture),
					(iCurExp == OMX_ExposureControlSmallAperture) ? " selected " : "", OMX_ExposureControlSmallAperture, GetNameExposure(OMX_ExposureControlSmallAperture),
					(iCurExp == OMX_ExposureControlVeryLong) ? " selected " : "", OMX_ExposureControlVeryLong, GetNameExposure(OMX_ExposureControlVeryLong),
					(iCurExp == OMX_ExposureControlFixedFps) ? " selected " : "", OMX_ExposureControlFixedFps, GetNameExposure(OMX_ExposureControlFixedFps),
					(iCurExp == OMX_ExposureControlNightWithPreview) ? " selected " : "", OMX_ExposureControlNightWithPreview, GetNameExposure(OMX_ExposureControlNightWithPreview),
					(iCurExp == OMX_ExposureControlAntishake) ? " selected " : "", OMX_ExposureControlAntishake, GetNameExposure(OMX_ExposureControlAntishake),
					(iCurExp == OMX_ExposureControlFireworks) ? " selected " : "", OMX_ExposureControlFireworks, GetNameExposure(OMX_ExposureControlFireworks),
					ret == 0 ? " disabled " : "",
					(iCurFilter == OMX_ImageFilterNone) ? " selected " : "", OMX_ImageFilterNone, GetNameImageFilter(OMX_ImageFilterNone),
					(iCurFilter == OMX_ImageFilterNoise) ? " selected " : "", OMX_ImageFilterNoise, GetNameImageFilter(OMX_ImageFilterNoise),
					(iCurFilter == OMX_ImageFilterEmboss) ? " selected " : "", OMX_ImageFilterEmboss, GetNameImageFilter(OMX_ImageFilterEmboss),
					(iCurFilter == OMX_ImageFilterNegative) ? " selected " : "", OMX_ImageFilterNegative, GetNameImageFilter(OMX_ImageFilterNegative),
					(iCurFilter == OMX_ImageFilterSketch) ? " selected " : "", OMX_ImageFilterSketch, GetNameImageFilter(OMX_ImageFilterSketch),
					(iCurFilter == OMX_ImageFilterOilPaint) ? " selected " : "", OMX_ImageFilterOilPaint, GetNameImageFilter(OMX_ImageFilterOilPaint),
					(iCurFilter == OMX_ImageFilterHatch) ? " selected " : "", OMX_ImageFilterHatch, GetNameImageFilter(OMX_ImageFilterHatch),
					(iCurFilter == OMX_ImageFilterGpen) ? " selected " : "", OMX_ImageFilterGpen, GetNameImageFilter(OMX_ImageFilterGpen),
					(iCurFilter == OMX_ImageFilterAntialias) ? " selected " : "", OMX_ImageFilterAntialias, GetNameImageFilter(OMX_ImageFilterAntialias),
					(iCurFilter == OMX_ImageFilterDeRing) ? " selected " : "", OMX_ImageFilterDeRing, GetNameImageFilter(OMX_ImageFilterDeRing),
					(iCurFilter == OMX_ImageFilterSolarize) ? " selected " : "", OMX_ImageFilterSolarize, GetNameImageFilter(OMX_ImageFilterSolarize),
					(iCurFilter == OMX_ImageFilterWatercolor) ? " selected " : "", OMX_ImageFilterWatercolor, GetNameImageFilter(OMX_ImageFilterWatercolor),
					(iCurFilter == OMX_ImageFilterPastel) ? " selected " : "", OMX_ImageFilterPastel, GetNameImageFilter(OMX_ImageFilterPastel),
					(iCurFilter == OMX_ImageFilterSharpen) ? " selected " : "", OMX_ImageFilterSharpen, GetNameImageFilter(OMX_ImageFilterSharpen),
					(iCurFilter == OMX_ImageFilterFilm) ? " selected " : "", OMX_ImageFilterFilm, GetNameImageFilter(OMX_ImageFilterFilm),
					(iCurFilter == OMX_ImageFilterBlur) ? " selected " : "", OMX_ImageFilterBlur, GetNameImageFilter(OMX_ImageFilterBlur),
					(iCurFilter == OMX_ImageFilterSaturation) ? " selected " : "", OMX_ImageFilterSaturation, GetNameImageFilter(OMX_ImageFilterSaturation),
					(iCurFilter == OMX_ImageFilterDeInterlaceLineDouble) ? " selected " : "", OMX_ImageFilterDeInterlaceLineDouble, GetNameImageFilter(OMX_ImageFilterDeInterlaceLineDouble),
					(iCurFilter == OMX_ImageFilterDeInterlaceAdvanced) ? " selected " : "", OMX_ImageFilterDeInterlaceAdvanced, GetNameImageFilter(OMX_ImageFilterDeInterlaceAdvanced),
					(iCurFilter == OMX_ImageFilterColourSwap) ? " selected " : "", OMX_ImageFilterColourSwap, GetNameImageFilter(OMX_ImageFilterColourSwap),
					(iCurFilter == OMX_ImageFilterWashedOut) ? " selected " : "", OMX_ImageFilterWashedOut, GetNameImageFilter(OMX_ImageFilterWashedOut),
				    (iCurFilter == OMX_ImageFilterColourPoint) ? " selected " : "", OMX_ImageFilterColourPoint, GetNameImageFilter(OMX_ImageFilterColourPoint),
				    (iCurFilter == OMX_ImageFilterPosterise) ? " selected " : "", OMX_ImageFilterPosterise, GetNameImageFilter(OMX_ImageFilterPosterise),
				    (iCurFilter == OMX_ImageFilterColourBalance) ? " selected " : "", OMX_ImageFilterColourBalance, GetNameImageFilter(OMX_ImageFilterColourBalance),
				    (iCurFilter == OMX_ImageFilterCartoon) ? " selected " : "", OMX_ImageFilterCartoon, GetNameImageFilter(OMX_ImageFilterCartoon),
				    (iCurFilter == OMX_ImageFilterAnaglyph) ? " selected " : "", OMX_ImageFilterAnaglyph, GetNameImageFilter(OMX_ImageFilterAnaglyph),
				    (iCurFilter == OMX_ImageFilterDeInterlaceFast) ? " selected " : "", OMX_ImageFilterDeInterlaceFast, GetNameImageFilter(OMX_ImageFilterDeInterlaceFast),
					ret == 0 ? " disabled " : "",
					(iCurBalance == OMX_WhiteBalControlOff) ? " selected " : "", OMX_WhiteBalControlOff, GetNameWhiteBalance(OMX_WhiteBalControlOff),
					(iCurBalance == OMX_WhiteBalControlAuto) ? " selected " : "", OMX_WhiteBalControlAuto, GetNameWhiteBalance(OMX_WhiteBalControlAuto),
					(iCurBalance == OMX_WhiteBalControlSunLight) ? " selected " : "", OMX_WhiteBalControlSunLight, GetNameWhiteBalance(OMX_WhiteBalControlSunLight),
					(iCurBalance == OMX_WhiteBalControlCloudy) ? " selected " : "", OMX_WhiteBalControlCloudy, GetNameWhiteBalance(OMX_WhiteBalControlCloudy),
					(iCurBalance == OMX_WhiteBalControlShade) ? " selected " : "", OMX_WhiteBalControlShade, GetNameWhiteBalance(OMX_WhiteBalControlShade),
					(iCurBalance == OMX_WhiteBalControlTungsten) ? " selected " : "", OMX_WhiteBalControlTungsten, GetNameWhiteBalance(OMX_WhiteBalControlTungsten),
					(iCurBalance == OMX_WhiteBalControlFluorescent) ? " selected " : "", OMX_WhiteBalControlFluorescent, GetNameWhiteBalance(OMX_WhiteBalControlFluorescent),
					(iCurBalance == OMX_WhiteBalControlIncandescent) ? " selected " : "", OMX_WhiteBalControlIncandescent, GetNameWhiteBalance(OMX_WhiteBalControlIncandescent),
					(iCurBalance == OMX_WhiteBalControlFlash) ? " selected " : "", OMX_WhiteBalControlFlash, GetNameWhiteBalance(OMX_WhiteBalControlFlash),
					(iCurBalance == OMX_WhiteBalControlHorizon) ? " selected " : "", OMX_WhiteBalControlHorizon, GetNameWhiteBalance(OMX_WhiteBalControlHorizon),
					iKeepRatio ? " checked " : "",					//Сохранять пропорции после обрезки
					uiLeftCrop,																		//LeftCrop
					uiRightCrop,																	//RightCrop
					uiUpCrop,																		//UpCrop
					uiDownCrop,																		//DownCrop
					iCurBright, ret == 0 ? " disabled " : "", iCurBright,							//Яркость
					iCurContrast, ret == 0 ? " disabled " : "", iCurContrast,	
					iCurSharpness, ret == 0 ? " disabled " : "", iCurSharpness,	
					iCurSaturation, ret == 0 ? " disabled " : "", iCurSaturation,	
					uiAutoBrightControl ? " checked " : "", ret == 0 ? " disabled " : "",
					uiDestBrightControl, ret == 0 ? " disabled " : "", uiDestBrightControl,
					
					ispCSI.ISOControl, ret == 0 ? " disabled " : "", ispCSI.ISOControl,
					ispCSI.HardAutoISOControl ? " checked " : "", ret == 0 ? " disabled " : "",						//Авто ISO hw
					ispCSI.SoftAutoISOControl ? " checked " : "", ret == 0 ? " disabled " : "",						//Авто ISO sw
					uiCurrISO, uiCurrISO,
					ispCSI.DestAutoISOControl, ret == 0 ? " disabled " : "", ispCSI.DestAutoISOControl,				//уровень автояркости для ISO
					ispCSI.EVControl, ret == 0 ? " disabled " : "", ispCSI.EVControl,
					ispCSI.SoftAutoEVControl ? " checked " : "", ret == 0 ? " disabled " : "",						//Авто EV sw
					uiCurrEV, uiCurrEV,
					ispCSI.DestAutoEVControl, ret == 0 ? " disabled " : "", ispCSI.DestAutoEVControl,				//уровень автояркости для EV
										
					iHighLightMode ? " checked " : "", ret == 0 ? " disabled " : "",
					iHighLightBright, ret == 0 ? " disabled " : "", iHighLightBright, 
					iHighLightAmplif, ret == 0 ? " disabled " : "", iHighLightAmplif, 
					ret == 0 ? " disabled " : "",
					(uiMainSettings == 0) ? " selected " : "",
					(uiMainSettings == 1) ? " selected " : "",
					ret == 0 ? " disabled " : "",
					((uiPrevSettings & 3) == 0) ? " selected " : "",
					((uiPrevSettings & 3) == 1) ? " selected " : "",
					((uiPrevSettings & 3) == 2) ? " selected " : "",
					((uiPrevSettings & 3) == 3) ? " selected " : "",
					uiPrevShowLevel, ret == 0 ? " disabled " : "", 
					uiPrevSettings & 4 ? " checked " : "", ret == 0 ? " disabled " : "",
					uiPrevBrigLevel,
					ret == 0 ? " disabled " : "",
					(((uiPrevSettings & 24) >> 3) == 0) ? " selected " : "",
					(((uiPrevSettings & 24) >> 3) == 1) ? " selected " : "",
					(((uiPrevSettings & 24) >> 3) == 2) ? " selected " : "",
					(((uiPrevSettings & 24) >> 3) == 3) ? " selected " : "",
					uiPrevColorLevel,
					ret == 0 ? " disabled " : "",
					(((uiPrevSettings & 96) >> 5) == 0) ? " selected " : "",
					(((uiPrevSettings & 96) >> 5) == 1) ? " selected " : "",
					(((uiPrevSettings & 96) >> 5) == 2) ? " selected " : "",
					(((uiPrevSettings & 96) >> 5) == 3) ? " selected " : "",
					uiPrevSettings & 128 ? " checked " : "", ret == 0 ? " disabled " : "",
					ret == 0 ? " disabled " : "",ret == 0 ? " disabled " : "",
					session->request_id);
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	DBG_FREE(msg_subbody);
	return 1;
}

int WEB_mic_change(int *pParams)
{
	int ret = 0;
	int n;
	int dev;
		
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Local == 1) && 
				(miModuleList[n].Type == MODULE_TYPE_MIC) &&
				(miModuleList[n].Enabled & 1) &&
				(miModuleList[n].ID == pParams[1])) 
				{
					ret = 1;
					dev = miModuleList[n].Settings[1];
					break;
				}	
	if (pParams[2] & 1) miModuleList[n].Settings[7] |= 4;
	if ((pParams[2] == 0) && (miModuleList[n].Settings[7] &= 4)) miModuleList[n].Settings[7] ^= 4;
	miModuleList[n].Settings[15] = pParams[0];
	miModuleList[n].Settings[17] = pParams[3];
	miModuleList[n].Settings[18] = pParams[4];
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret && (pParams[0] >= 0) && (pParams[0] <= 100)) 
	{
		audio_set_capture_volume(dev, pParams[0]);
		audio_set_capture_agc(dev, pParams[2] ? 1 : 0);
	}
	return 1;
}

int WEB_mic_respond(char *msg_tx, WEB_SESSION *session)
{
	int iCurVol = 0;	
					
	char *msg_subbody = (char*)DBG_MALLOC(8192);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/mic/\"><h1>МИКРОФОН</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	int n;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) && 
			(miModuleList[n].Type == MODULE_TYPE_MIC))
			{
				iCurVol = audio_get_capture_volume(miModuleList[n].Settings[1]);
				if ((iCurVol < 0) || (iCurVol > 100)) iCurVol = 0;
	
				memset(msg_subbody, 0, 8192);
				sprintf(msg_subbody,"<form action='/mic/change'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"<input type='hidden' name='mic_id' value=%.4s>%s :   \r\n"
						"Чувствительность: <input type='range' name='Vol' min='0' max='100' step='1' value='%i' style='width: 300px;' >\r\n"
						"Aвтоусиление: <input type='checkbox' name='AGC'%s><br />\r\n"
						"Обработка:<select name='Convert' style='width: 200px;'>\r\n"
												"		<option %s value='0'>Отключено</option>\r\n"
												"		<option %s value='1'>Цифровое усиление</option>\r\n"
												"		<option %s value='2'>Цифровое автоусиление</option>\r\n"
												"	</select><br />\r\n"
						"&emsp;&emsp;Уровень цифрового усиления: <input type='range' name='DigLevel' min='1' max='14' step='1' value='%i' style='width: 200px;'><br />\r\n"
						"<button type='submit'>Сохранить</button></form>\r\n", 
						session->request_id, (char*)&miModuleList[n].ID, (char*)&miModuleList[n].ID, iCurVol, 
							audio_get_capture_agc(miModuleList[n].Settings[1]) ? " checked" : "",
							(miModuleList[n].Settings[17] == 0) ? "selected" : "",
							(miModuleList[n].Settings[17] == 1) ? "selected" : "",
							(miModuleList[n].Settings[17] == 2) ? "selected" : "",
							miModuleList[n].Settings[18]);
				strcat(msg_tx, msg_subbody);
			}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	DBG_FREE(msg_subbody);
	return 1;
}

void WEB_system_stop(int *pParams)
{
	if (pParams[1] == 0) 
		Media_NextRecNum(pParams[0], CAPTURE_TYPE_AUDIO, 0);
	if (pParams[1] == 1)
		Media_NextRecNum(pParams[0], CAPTURE_TYPE_AUDIO, CMD_SAVE_COPY_OFFSET);
	if (pParams[1] == 2)
	{
		DBG_MUTEX_LOCK(&system_mutex);
		if (cThreadScanerStatus == 1) cThreadScanerStatus = 3;
		DBG_MUTEX_UNLOCK(&system_mutex);
	}	
	if (pParams[1] == 3) tx_eventer_send_event(&modevent_evnt, EVENT_SPLIT_EVENTS);
	if (pParams[1] == 4) tx_eventer_send_event(&modevent_evnt, EVENT_SPLIT_ACTIONS);
}

void WEB_system_save(int *pParams)
{
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
	timeinfo.tm_mday = pParams[0];
	timeinfo.tm_mon = pParams[1] - 1;
	timeinfo.tm_year = pParams[2] - 1900;
	timeinfo.tm_hour = pParams[3];
	timeinfo.tm_min = pParams[4];
	timeinfo.tm_sec = pParams[5];
	time_t rawtime = mktime(&timeinfo);
	a_stime(&rawtime);
}

void WEB_system_change()
{
	time_t rawtime;
	time(&rawtime);	
	struct tm timeinfo;
	localtime_r(&rawtime, &timeinfo);
	char timebuff[64];
	strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
	dbgprintf(3,"Send system time '%s' to ALL\n", timebuff);
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int SysID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	unsigned char cBcTCP = ucBroadCastTCP;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_FORCE_DATETIME,  (char*)&rawtime, sizeof(rawtime), (char*)&SysID, sizeof(SysID));
}

void WEB_system_close()
{
	System_Close(NULL, 0);
}

void WEB_system_exit()
{
	System_Exit(NULL, 0);
}

void WEB_system_reboot()
{
	System_Reboot(NULL, 0);
}

void WEB_system_reset()
{
	System_ResetDefault(NULL, 0);
}

void WEB_system_restart()
{
	System_Restart(NULL, 0);
}

void WEB_system_off()
{
	System_Shutdown(NULL, 0);
}

void WEB_system_update()
{
	System_Update();
}

int WEB_system_respond(char *msg_tx, WEB_SESSION *session)
{
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/system/\"><h1>СИСТЕМА</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	struct tm timeinfo;
	time_t rawtime;
	time(&rawtime);	
	localtime_r(&rawtime, &timeinfo);
					
	char *cBody = (char*)DBG_MALLOC(4096);
	
	char cSysVersion[64];	
	DBG_MUTEX_LOCK(&system_mutex);
	SYSTEM_INFO *si = GetLocalSysInfo();
	memcpy(cSysVersion, si->Version, 64);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_MUTEX_LOCK(&system_mutex);
	memset(cBody, 0, 4096);
	sprintf(cBody, "<form action='/system/save'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"Версия: %s<br />\r\n"
						"Запущено: %s<br />\r\n"
						"Режим запуска:%s%s%s%s%s<br />\r\n"
						"Системное время:  <input type='number' name='DateDay' min=1 max=31 value='%i' style='width: 50px;'>\r\n"
						".<input type='number' name='DateMont' min=1 max=12 value='%i' style='width: 50px;'>\r\n"
						".<input type='number' name='DateYear' min=2000 max=2159 value='%i' style='width: 100px;'>\r\n"
						"   <input type='number' name='TimeHour' min=0 max=24 value='%i' style='width: 50px;'>\r\n"
						":<input type='number' name='TimeMin' min=0 max=59 value='%i' style='width: 50px;'>\r\n"
						":<input type='number' name='TimeSec' min=0 max=59 value='%i' style='width: 50px;'>\r\n"
						"<button type='submit'>Сохранить</button></form>\r\n"						
					"<input type='button' value='Установить всем текущее время' onclick=\"window.location.href='/system/change?req_index=%i'\"><br /><br />\r\n"
					"<input type='button' value='Перезагрузить' onclick=\"window.location.href='/system/reboot?req_index=%i'\">\r\n"
					"<input type='button' value='Выйти' onclick=\"window.location.href='/system/exit?req_index=%i'\">\r\n"					
					"<input type='button' value='Выключить' onclick=\"window.location.href='/system/shutdown?req_index=%i'\">\r\n"
					"<input type='button' value='Перезапуск сервиса' onclick=\"window.location.href='/system/restart?req_index=%i'\">\r\n"
					"<input type='button' value='Аварийный выход' onclick=\"window.location.href='/system/close?req_index=%i'\">\r\n"
					"<input type='button' value='Сброс настроек' onclick=\"window.location.href='/system/refresh?req_index=%i'\"><br /><br />\r\n"
					"<input type='button' value='Обновить' onclick=\"window.location.href='/system/update?req_index=%i'\"><br /><br />\r\n"
					, session->request_id, cSysVersion, cStartTime,
					(uiStartMode & 0x0001) ? " Reset" : "",
					(uiStartMode & 0x0002) ? " Debug" : "",
					(uiStartMode & 0x0004) ? " Safe" : "",
					(uiStartMode & 0x0008) ? " NoDie" : "",
					(uiStartMode & 0x0010) ? " Pause" : "",
					timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, 
					timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
					session->request_id, session->request_id,
					session->request_id, session->request_id, 
					session->request_id, session->request_id, session->request_id,
					session->request_id);
	strcat(msg_tx,	cBody);
	
	int n;
	char *cName;
	for (n = 0; n < iStreamCnt; n++)
		if ((SyncStream[n]->VidID) || (SyncStream[n]->AudID))
		{
			switch(SyncStream[n]->Type)
			{
				case CAPTURE_TYPE_FULL: cName = "Реальная"; break;
				case CAPTURE_TYPE_SLOW: cName = "Замедленная"; break;
				case CAPTURE_TYPE_DIFF: cName = "Только изменения"; break;
				case CAPTURE_TYPE_AUDIO: cName = "Только звук"; break;
				default: cName = "Неизвестно"; break;
			}
			
			memset(cBody, 0, 4096);
			sprintf(cBody,"Тип записи: '%s' <input type='button' value='Разделить поток %.4s %.4s' onclick=\"window.location.href='/system/stop?req_index=%i&mode=0&type=%i'\">\r\n"
							"<input type='button' value='Разделить и скопировать поток %.4s %.4s' onclick=\"window.location.href='/system/stop?req_index=%i&mode=1&type=%i'\"><br />\r\n",
							cName,
							(char*)&SyncStream[n]->VidID, (char*)&SyncStream[n]->AudID, session->request_id, n,
							(char*)&SyncStream[n]->VidID, (char*)&SyncStream[n]->AudID, session->request_id, n);
			strcat(msg_tx, cBody);
		}
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	memset(cBody, 0, 4096);
	sprintf(cBody,"<input type='button' value='Разделить сохранение состояния модулей' onclick=\"window.location.href='/system/stop?req_index=%i&mode=2'\"><br />\r\n"
				  "<input type='button' value='Разделить сохранение событий' onclick=\"window.location.href='/system/stop?req_index=%i&mode=3'\"><br />\r\n"
				  "<input type='button' value='Разделить сохранение действий' onclick=\"window.location.href='/system/stop?req_index=%i&mode=4'\"><br />\r\n",
			session->request_id, session->request_id, session->request_id);
	strcat(msg_tx, cBody);
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");		
	DBG_FREE(cBody);
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	return 1;
}

int WEB_directory_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&system_mutex);
	iDirsCnt++;
	diDirList = (DIRECTORY_INFO*)DBG_REALLOC(diDirList, sizeof(DIRECTORY_INFO)*iDirsCnt);
	memset(&diDirList[iDirsCnt-1], 0, sizeof(DIRECTORY_INFO));
	/*diDirList[iDirsCnt-1].Access = pParams[1];
	diDirList[iDirsCnt-1].Type = pParams[2];
	if (strlen(&strParam[0]) > 63) memcpy(diDirList[iDirsCnt-1].Name, &strParam[0], 63);
		else memcpy(diDirList[iDirsCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	if (strlen(&strParam[64]) > 255) memcpy(diDirList[iDirsCnt-1].Path, &strParam[64], 255);
		else memcpy(diDirList[iDirsCnt-1].Path, &strParam[64], strlen(&strParam[64]));
	while (diDirList[iDirsCnt-1].Path[strlen(diDirList[iDirsCnt-1].Path)-1] == 47) 
							diDirList[iDirsCnt-1].Path[strlen(diDirList[iDirsCnt-1].Path)-1] = 0;*/
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveDirectories();
	return 1;
}

int WEB_directory_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iDirsCnt) && iDirsCnt)
	{
		DIRECTORY_INFO *di = (DIRECTORY_INFO*)DBG_MALLOC(sizeof(DIRECTORY_INFO)*(iDirsCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iDirsCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&di[clk], &diDirList[i], sizeof(DIRECTORY_INFO));
				clk++;
			}
		DBG_FREE(diDirList);
		diDirList = di;
		iDirsCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveDirectories();
	return 1;
}

int WEB_directory_play(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);	
	if ((pParams[0] >= 0) && (pParams[0] < iDirsCnt) && iDirsCnt)
	{
		ret = UpdateListFiles(diDirList[pParams[0]].Path, UPDATE_LIST_SCANTYPE_CUR);
		if (ret == 0) 
		{		
			if (uiShowModeCur >= 2) uiShowModeCur = 1;
			dbgprintf(3, "No files in WEB_directory_play '%s'\n", diDirList[pParams[0]].Path);
			ShowNewMessage("Нет файлов для воспроизведения");
		} 
		else 
		{
			if (uiShowModeCur < 2) uiShowModeCur = 2;
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SetChangeShowNow(1);	
	return 1;
}

int WEB_directory_open(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);	
	if ((pParams[0] >= 0) && (pParams[0] < iDirsCnt) && iDirsCnt)
	{
		memset(cCurrentFileLocation, 0, 256);
		memcpy(cCurrentFileLocation, diDirList[pParams[0]].Path, strlen(diDirList[pParams[0]].Path));
		dbgprintf(4, "Set show path(w): '%s'\n", diDirList[pParams[0]].Path);
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) 
	{
		add_sys_cmd_in_list(SYSTEM_CMD_SET_SHOW_PATH, 0);
		//SetChangeShowNow(1);
	}
	return 1;
}

int WEB_directory_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iDirsCnt) && iDirsCnt)
	{
		memset(&diDirList[pParams[0]], 0, sizeof(DIRECTORY_INFO));
		diDirList[pParams[0]].Access = pParams[1];		
		diDirList[pParams[0]].RemoteAccess = pParams[2];
						
		if (strlen(&strParam[0]) > 63) memcpy(diDirList[pParams[0]].Name, &strParam[0], 63);
			else memcpy(diDirList[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));
				
		if (strlen(&strParam[64]) > 31) memcpy(diDirList[pParams[0]].BackUpIPCopyService, &strParam[64], 31);
			else memcpy(diDirList[pParams[0]].BackUpIPCopyService, &strParam[64], strlen(&strParam[64]));
				
		if (strlen(&strParam[128]) > 31) memcpy(diDirList[pParams[0]].ArchiveIPCopyService, &strParam[128], 31);
			else memcpy(diDirList[pParams[0]].ArchiveIPCopyService, &strParam[128], strlen(&strParam[128]));		
		
		if (strlen(&strParam[256]) > 255) memcpy(diDirList[pParams[0]].Path, &strParam[256], 255);
			else memcpy(diDirList[pParams[0]].Path, &strParam[256], strlen(&strParam[256]));
		if (strlen(diDirList[pParams[0]].Path))
			while (diDirList[pParams[0]].Path[strlen(diDirList[pParams[0]].Path)-1] == 47) 
							diDirList[pParams[0]].Path[strlen(diDirList[pParams[0]].Path)-1] = 0;
		
		diDirList[pParams[0]].MinSpace = pParams[3];	
		diDirList[pParams[0]].MaxFileSize = pParams[4];	
		diDirList[pParams[0]].MaxFileTime = pParams[5];	
		diDirList[pParams[0]].Type = pParams[9];
		diDirList[pParams[0]].BackUpMode = pParams[10];
		diDirList[pParams[0]].BackUpMaxOrderLen = pParams[11];
		diDirList[pParams[0]].BackUpUseCopyService = pParams[12];
		diDirList[pParams[0]].BackUpMaxWaitCancel = pParams[13];
		diDirList[pParams[0]].BackUpMaxWaitMess = pParams[14];
		diDirList[pParams[0]].ArchiveSelectType = pParams[6];	
		diDirList[pParams[0]].ArchiveTimeFrom = pParams[7];	
		diDirList[pParams[0]].ArchiveTimeTo = pParams[8];		
		diDirList[pParams[0]].ArchiveMaxOrderLen = pParams[15];
		diDirList[pParams[0]].ArchiveUseCopyService = pParams[16];
		diDirList[pParams[0]].ArchiveMaxWaitCancel = pParams[17];
		diDirList[pParams[0]].ArchiveMaxWaitMess = pParams[18];
		diDirList[pParams[0]].ArchiveMode = pParams[19];
		diDirList[pParams[0]].BackUpStorage = pParams[20];
		diDirList[pParams[0]].ArchiveStorage = pParams[21];
		
		if (strlen(&strParam[512]) > 255) memcpy(diDirList[pParams[0]].BeginPath, &strParam[512], 255);
			else memcpy(diDirList[pParams[0]].BeginPath, &strParam[512], strlen(&strParam[512]));
		if (strlen(diDirList[pParams[0]].BeginPath))
			while (diDirList[pParams[0]].BeginPath[strlen(diDirList[pParams[0]].BeginPath)-1] == 47) 
							diDirList[pParams[0]].BeginPath[strlen(diDirList[pParams[0]].BeginPath)-1] = 0;
		
		if (strlen(&strParam[768]) > 255) memcpy(diDirList[pParams[0]].BackUpPath, &strParam[768], 255);
			else memcpy(diDirList[pParams[0]].BackUpPath, &strParam[768], strlen(&strParam[768]));
		if (strlen(diDirList[pParams[0]].BackUpPath))
			while (diDirList[pParams[0]].BackUpPath[strlen(diDirList[pParams[0]].BackUpPath)-1] == 47) 
							diDirList[pParams[0]].BackUpPath[strlen(diDirList[pParams[0]].BackUpPath)-1] = 0;
		
		if (strlen(&strParam[1024]) > 255) memcpy(diDirList[pParams[0]].ArchivePath, &strParam[1024], 255);
			else memcpy(diDirList[pParams[0]].ArchivePath, &strParam[1024], strlen(&strParam[1024]));
		if (strlen(diDirList[pParams[0]].ArchivePath))
			while (diDirList[pParams[0]].ArchivePath[strlen(diDirList[pParams[0]].ArchivePath)-1] == 47) 
							diDirList[pParams[0]].ArchivePath[strlen(diDirList[pParams[0]].ArchivePath)-1] = 0;																							
		
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveDirectories();
	return 1;
}

int WEB_directories_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&system_mutex);
	TestDirectories(1);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(8192);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/directories/\"><h1>ПАПКИ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&system_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Тип</th><th>Доступ</th><th>Удаленный доступ</th><th>Наименование</th><th>Путь</th><th>Общие настройки</th><th>Настройки копирования</th><th>Настройки архивирования</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iDirsCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurDirectoryPage = iPage;
	iCurPage = iCurDirectoryPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurDirectoryPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurDirectoryPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iDirsCnt) iLastOption = iDirsCnt;
	
	WEB_pages_list(msg_tx, "directories", iSubPages, iCurPage, "");	
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 8192);
		sprintf(msg_subbody, 
					"<tr><form action='/directories/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><select name='DirType' style='width: 200px;'>\r\n"
							"		<option %s value='%i'>Медиа</option>\r\n"
							"		<option %s value='%i'>Нормальная запись</option>\r\n"
							"		<option %s value='%i'>Замедленная запись</option>\r\n"
							"		<option %s value='%i'>Запись изменений</option>\r\n"
							"		<option %s value='%i'>Запись аудио</option>\r\n"
							"		<option %s value='%i'>Запись состояний</option>\r\n"
							"		<option %s value='%i'>Запись событий</option>\r\n"
							"		<option %s value='%i'>Запись действий</option>\r\n"
							"		<option %s value='%i'>Другое</option>\r\n"
							"	</select><br></td>\r\n"
					"<td><input type='number' name='Access' min=0 max=1000 value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='RemoteAccess' %s></td>\r\n"
					"<td><input type='text' name='Name' maxlength=64 value='%s' style='width: 300px;'></td>\r\n"
					"<td><input type='text' name='Path' maxlength=255 value='%s' style='width: 360px;'></td>\r\n"
					"<td>\r\n"
						"Освобождать место:<input type='number' name='MinSpace' min=0 value=%i style='width: 60px;'><br>\r\n"
						"Начало добавочного пути к файлу:<br>"
							"&ensp;&ensp;<input type='text' name='BeginPath' maxlength=255 value='%s' style='width: 300px;'><br>\r\n"
						"Размер файла (MB):<input type='number' name='MaxFileSize' min=3 max=1000 value='%i' style='width: 300px;'><br>\r\n"
						"Время файла (сек.):<input type='number' name='MaxFileTime' min=60 max=220000 value='%i' style='width: 300px;'><br>\r\n"	
						"Использовать как хранения для копий: <input type='checkbox' name='BackUpStorage' %s><br>\r\n"
						"Использовать как хранилище для архива: <input type='checkbox' name='ArchiveStorage' %s><br>\r\n"				
					"</td>\r\n"
					"<td>\r\n"
						"Режим копирования:"
							"<select name='BackUpMode' style='width: 300;'>\r\n"
								"<option %s value='0'>Отключено</option>\r\n"
								"<option %s value='1'>Копировать</option>\r\n"
								"<option %s value='2'>Переместить</option>\r\n"
								"</select><br>\r\n"
						"Путь к файлам резервных копий:<br>"
							"&ensp;&ensp;<input type='text' name='BackUpPath' maxlength=255 value='%s' style='width: 300px;'><br>\r\n"
						"Макс. длина очереди:<input type='number' name='BackUpMaxOrderLen' min=0 max=30 value='%i' style='width: 300px;'><br>\r\n"
						"Использовать сервис:<input type='checkbox' name='BackUpUseCopyService'%s><br>\r\n"
						"IP адрес сервера:<input type='text' name='BackUpIPCopyService' maxlength=15 value='%s' style='width: 300px;'><br>\r\n"
						"Макс. время ожидания до отмены(сек.):<input type='number' name='BackUpMaxWaitCancel' min=10 max=10000 value='%i' style='width: 300px;'><br>\r\n"
						"Макс. время ожидания до оповещения(сек.):<input type='number' name='BackUpMaxWaitMess' min=10 max=10000 value='%i' style='width: 300px;'><br>\r\n"
					"</td>\r\n"
					"<td>\r\n"
						"Режим помещения в архив:"
							"<select name='ArchiveMode' style='width: 300;'>\r\n"
								"<option %s value='0'>Отключено</option>\r\n"
								"<option %s value='1'>Копировать</option>\r\n"
								"<option %s value='2'>Переместить</option>\r\n"
								"</select><br>\r\n"
						"Тип выборки:"
							"<select name='ArchiveSelectType' style='width: 300;'>\r\n"
								"<option %s value='0'>Отключено</option>\r\n"
								"<option %s value='1'>Случайный</option>\r\n"
								"<option %s value='2'>Макс. размер</option>\r\n"
								"<option %s value='3'>Мин. размер</option>\r\n"
								"</select><br>\r\n"
						"Путь к архиву (полный для SMB или добавочный для сервиса копирования):<br>"
							"&ensp;&ensp;<input type='text' name='ArchivePath' maxlength=255 value='%s' style='width: 300px;'><br>\r\n"
						"Макс. длина очереди:<input type='number' name='ArchiveMaxOrderLen' min=0 max=30 value='%i' style='width: 300px;'><br>\r\n"
						"Использовать сервис:<input type='checkbox' name='ArchiveUseCopyService'%s><br>\r\n"
						"IP адрес сервера:<input type='text' name='ArchiveIPCopyService' maxlength=15 value='%s' style='width: 300px;'><br>\r\n"
						"Макс. время ожидания до отмены(сек.):<input type='number' name='ArchiveMaxWaitCancel' min=10 max=10000 value='%i' style='width: 300px;'><br>\r\n"
						"Макс. время ожидания до оповещения(сек.):<input type='number' name='ArchiveMaxWaitMess' min=10 max=10000 value='%i' style='width: 300px;'><br>\r\n"
						"Интервалы копирования:<br>"
							"&ensp;&ensp;<input type='time' name='ArchiveTimeFrom' min='00:00' max='23:59' value='%.2i:%.2i' style='width: 100px;'>"
								" по <input type='time' name='ArchiveTimeTo' min='00:00' max='23:59' value='%.2i:%.2i' style='width: 100px;'><br>\r\n"				
					"</td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/directories/delete'>Удалить</button>\r\n"
					"<button type='submit' formaction='/directories/play'>Воспроизвести</button>\r\n"
					"<button type='submit' formaction='/directories/open'>Воспроизвести подпапки</button></td></form></tr>\r\n",
					session->request_id,
					n,n, 
					(diDirList[n].Type == DIRECTORY_TYPE_MEDIA) ? "selected" : "", DIRECTORY_TYPE_MEDIA,
					(diDirList[n].Type == DIRECTORY_TYPE_NORM) ? "selected" : "", DIRECTORY_TYPE_NORM,
					(diDirList[n].Type == DIRECTORY_TYPE_SLOW) ? "selected" : "", DIRECTORY_TYPE_SLOW,
					(diDirList[n].Type == DIRECTORY_TYPE_DIFF) ? "selected" : "", DIRECTORY_TYPE_DIFF,
					(diDirList[n].Type == DIRECTORY_TYPE_AUDIO) ? "selected" : "", DIRECTORY_TYPE_AUDIO,
					(diDirList[n].Type == DIRECTORY_TYPE_STATUSES) ? "selected" : "", DIRECTORY_TYPE_STATUSES,
					(diDirList[n].Type == DIRECTORY_TYPE_EVENTS) ? "selected" : "", DIRECTORY_TYPE_EVENTS,
					(diDirList[n].Type == DIRECTORY_TYPE_ACTIONS) ? "selected" : "", DIRECTORY_TYPE_ACTIONS,
					(diDirList[n].Type == DIRECTORY_TYPE_OTHER) ? "selected" : "", DIRECTORY_TYPE_OTHER,
					diDirList[n].Access, 
					diDirList[n].RemoteAccess ? " checked " : "",
					diDirList[n].Name, diDirList[n].Path,
					diDirList[n].MinSpace,
					diDirList[n].BeginPath,
					diDirList[n].MaxFileSize,
					diDirList[n].MaxFileTime,
					diDirList[n].BackUpStorage ? " checked " : "",
					diDirList[n].ArchiveStorage ? " checked " : "",
					(diDirList[n].BackUpMode == 0) ? " selected " : "", 
					(diDirList[n].BackUpMode == 1) ? " selected " : "", 
					(diDirList[n].BackUpMode == 2) ? " selected " : "",	
					diDirList[n].BackUpPath,
					diDirList[n].BackUpMaxOrderLen,
					diDirList[n].BackUpUseCopyService ? " checked " : "",
					diDirList[n].BackUpIPCopyService,
					diDirList[n].BackUpMaxWaitCancel,
					diDirList[n].BackUpMaxWaitMess,
					(diDirList[n].ArchiveMode == 0) ? " selected " : "", 
					(diDirList[n].ArchiveMode == 1) ? " selected " : "", 
					(diDirList[n].ArchiveMode == 2) ? " selected " : "",	
					(diDirList[n].ArchiveSelectType == 0) ? "selected" : "", 
					(diDirList[n].ArchiveSelectType == 1) ? "selected" : "", 
					(diDirList[n].ArchiveSelectType == 2) ? "selected" : "", 
					(diDirList[n].ArchiveSelectType == 3) ? "selected" : "",
					diDirList[n].ArchivePath,
					diDirList[n].ArchiveMaxOrderLen,
					diDirList[n].ArchiveUseCopyService ? " checked " : "",
					diDirList[n].ArchiveIPCopyService,
					diDirList[n].ArchiveMaxWaitCancel,
					diDirList[n].ArchiveMaxWaitMess,					
					(int)(diDirList[n].ArchiveTimeFrom / 100), diDirList[n].ArchiveTimeFrom - (int)(diDirList[n].ArchiveTimeFrom / 100)*100, 
					(int)(diDirList[n].ArchiveTimeTo / 100), diDirList[n].ArchiveTimeTo - (int)(diDirList[n].ArchiveTimeTo / 100)*100);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	memset(msg_subbody, 0, 8192);
	sprintf(msg_subbody, 
					"<tr><form action='/directories/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,										
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "directories", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_widget_add(int *pParams, char* strParam, float* flParams)
{
	DBG_MUTEX_LOCK(&widget_mutex);
	iWidgetsCnt++;
	wiWidgetList = (WIDGET_INFO*)DBG_REALLOC(wiWidgetList, sizeof(WIDGET_INFO)*iWidgetsCnt);
	memset(&wiWidgetList[iWidgetsCnt-1], 0, sizeof(WIDGET_INFO));
	if ((pParams[3] < WIDGET_TYPE_UNKNOWN) || (pParams[3] >= WIDGET_TYPE_MAX)) pParams[3] = 0;		
	wiWidgetList[iWidgetsCnt-1].Status = -1;
	wiWidgetList[iWidgetsCnt-1].WidgetID = pParams[1];
	wiWidgetList[iWidgetsCnt-1].ShowMode = pParams[2];
	wiWidgetList[iWidgetsCnt-1].Type = pParams[3];
	wiWidgetList[iWidgetsCnt-1].Scale = flParams[0];
	wiWidgetList[iWidgetsCnt-1].Direct = pParams[4];
	wiWidgetList[iWidgetsCnt-1].Speed = flParams[1];
	wiWidgetList[iWidgetsCnt-1].Settings[0] = pParams[5];
	wiWidgetList[iWidgetsCnt-1].Refresh = pParams[6];
	wiWidgetList[iWidgetsCnt-1].WeekDays = pParams[7];
	wiWidgetList[iWidgetsCnt-1].NotBeforeTime = pParams[8];
	wiWidgetList[iWidgetsCnt-1].NotAfterTime = pParams[9];
	wiWidgetList[iWidgetsCnt-1].NotBeforeTime2 = pParams[10];
	wiWidgetList[iWidgetsCnt-1].NotAfterTime2 = pParams[11];
	wiWidgetList[iWidgetsCnt-1].NotBeforeTime3 = pParams[12];
	wiWidgetList[iWidgetsCnt-1].NotAfterTime3 = pParams[13];	
	wiWidgetList[iWidgetsCnt-1].SelfPath = pParams[14];	
	wiWidgetList[iWidgetsCnt-1].ModuleID = pParams[15];	
	wiWidgetList[iWidgetsCnt-1].SourceCell = pParams[16];		
	wiWidgetList[iWidgetsCnt-1].Coefficient = flParams[2];
	wiWidgetList[iWidgetsCnt-1].ShowValueFrom = flParams[3];
	wiWidgetList[iWidgetsCnt-1].ShowValueTo = flParams[4];
	wiWidgetList[iWidgetsCnt-1].OffsetValue = pParams[18];				
	wiWidgetList[iWidgetsCnt-1].DirectX = flParams[5];
	wiWidgetList[iWidgetsCnt-1].DirectY = flParams[6];
	wiWidgetList[iWidgetsCnt-1].ShowTime = pParams[19];
	wiWidgetList[iWidgetsCnt-1].ShowRepeat = pParams[20];
	wiWidgetList[iWidgetsCnt-1].Enabled = pParams[21];
	wiWidgetList[iWidgetsCnt-1].Angle = flParams[7];
	wiWidgetList[iWidgetsCnt-1].ModuleID2 = pParams[22];	
	wiWidgetList[iWidgetsCnt-1].SourceCell2 = pParams[23];		
	wiWidgetList[iWidgetsCnt-1].ShowValue2From = pParams[24];
	wiWidgetList[iWidgetsCnt-1].ShowValue2To = pParams[25];
	//wiWidgetList[iWidgetsCnt-1].ShowValueStr = flParams[8];
	wiWidgetList[iWidgetsCnt-1].ShowValue2Str = pParams[27];
	wiWidgetList[iWidgetsCnt-1].ShowWithCamera = pParams[29];
	
	memset(wiWidgetList[iWidgetsCnt-1].Name, 0, 64);
	if (strlen(&strParam[0]) > 63) memcpy(wiWidgetList[iWidgetsCnt-1].Name, &strParam[0], 63);
		else memcpy(wiWidgetList[iWidgetsCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	memset(wiWidgetList[iWidgetsCnt-1].Color, 0, 10);
	if (strlen(&strParam[256]) > 6) memcpy(wiWidgetList[iWidgetsCnt-1].Color, &strParam[256], 6);
		else memcpy(wiWidgetList[iWidgetsCnt-1].Color, &strParam[256], strlen(&strParam[256]));
	unsigned int uiColor = Hex2Int(wiWidgetList[iWidgetsCnt-1].Color);
	wiWidgetList[iWidgetsCnt-1].RGBColor.Red = uiColor & 255;
	wiWidgetList[iWidgetsCnt-1].RGBColor.Green = (uiColor >> 8) & 255;
	wiWidgetList[iWidgetsCnt-1].RGBColor.Blue = (uiColor >> 16) & 255;
	
	memset(wiWidgetList[iWidgetsCnt-1].ValueTypeName, 0, 10);
	if (strlen(&strParam[512]) > 9) memcpy(wiWidgetList[iWidgetsCnt-1].ValueTypeName, &strParam[512], 9);
		else memcpy(wiWidgetList[iWidgetsCnt-1].ValueTypeName, &strParam[512], strlen(&strParam[512]));
	memset(wiWidgetList[iWidgetsCnt-1].Path, 0, 256);
	if (strlen(&strParam[768]) > 255) memcpy(wiWidgetList[iWidgetsCnt-1].Path, &strParam[768], 255);
		else memcpy(wiWidgetList[iWidgetsCnt-1].Path, &strParam[768], strlen(&strParam[768]));
			
	if (pParams[28] < (iWidgetsCnt-1)) WEB_change_pos(iWidgetsCnt-1, pParams[28], (char*)wiWidgetList, iWidgetsCnt, sizeof(WIDGET_INFO));
	DBG_MUTEX_UNLOCK(&widget_mutex);
	SaveWidgets();
	return 1;
}

int WEB_widget_recover(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&widget_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iWidgetsCnt) && iWidgetsCnt)
	{
		if (wiWidgetList[pParams[0]].Status == -2) wiWidgetList[pParams[0]].Status = -1;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&widget_mutex);
	if (ret) SaveWidgets();
	return 1;
}

int WEB_widget_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&widget_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iWidgetsCnt) && iWidgetsCnt)
	{
		wiWidgetList[pParams[0]].Status = -2;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&widget_mutex);
	if (ret) SaveWidgets();
	return 1;
}

int WEB_widget_up(int *pParams)
{
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iWidgetsCnt) && iWidgetsCnt)
	{
		if (pParams[0] > 0) WEB_change_pos(pParams[0], pParams[0] - 1, (char*)wiWidgetList, iWidgetsCnt, sizeof(WIDGET_INFO));
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	return 1;
}

int WEB_widget_down(int *pParams)
{
	DBG_MUTEX_LOCK(&evntaction_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iWidgetsCnt) && iWidgetsCnt)
	{
		if (pParams[0] < (iWidgetsCnt - 1)) WEB_change_pos(pParams[0], pParams[0] + 1, (char*)wiWidgetList, iWidgetsCnt, sizeof(WIDGET_INFO));
	}
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	return 1;
}

int WEB_widget_save(int *pParams, char* strParam, float* flParams)
{
	int ret = 0;
	int iShowMode = -3;
	DBG_MUTEX_LOCK(&widget_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iWidgetsCnt) && iWidgetsCnt)
	{
		if ((pParams[3] < WIDGET_TYPE_UNKNOWN) || (pParams[3] >= WIDGET_TYPE_MAX)) pParams[3] = WIDGET_TYPE_UNKNOWN;		
		//memset(&wiWidgetList[pParams[0]], 0, sizeof(WIDGET_INFO)); 
		wiWidgetList[pParams[0]].WidgetID = pParams[1];
		iShowMode = wiWidgetList[pParams[0]].ShowMode;
		wiWidgetList[pParams[0]].ShowMode = pParams[2];
		wiWidgetList[pParams[0]].Type = pParams[3];
		wiWidgetList[pParams[0]].Scale = flParams[0];
		wiWidgetList[pParams[0]].Direct = pParams[4];
		wiWidgetList[pParams[0]].Speed = flParams[1];
		wiWidgetList[pParams[0]].Settings[0] = pParams[5];
		wiWidgetList[pParams[0]].Refresh = pParams[6];
		wiWidgetList[pParams[0]].WeekDays = pParams[7];
		wiWidgetList[pParams[0]].NotBeforeTime = pParams[8];
		wiWidgetList[pParams[0]].NotAfterTime = pParams[9];
		wiWidgetList[pParams[0]].NotBeforeTime2 = pParams[10];
		wiWidgetList[pParams[0]].NotAfterTime2 = pParams[11];
		wiWidgetList[pParams[0]].NotBeforeTime3 = pParams[12];
		wiWidgetList[pParams[0]].NotAfterTime3 = pParams[13];	
		wiWidgetList[pParams[0]].SelfPath = pParams[14];	
		wiWidgetList[pParams[0]].ModuleID = pParams[15];	
		wiWidgetList[pParams[0]].SourceCell = pParams[16];	
		wiWidgetList[pParams[0]].Coefficient = flParams[2];
		wiWidgetList[pParams[0]].ShowValueFrom = flParams[3];
		wiWidgetList[pParams[0]].ShowValueTo = flParams[4];
		wiWidgetList[pParams[0]].OffsetValue = pParams[18];			
		wiWidgetList[pParams[0]].DirectX = flParams[5];			
		wiWidgetList[pParams[0]].DirectY = flParams[6];			
		wiWidgetList[pParams[0]].ShowTime = pParams[19];
		wiWidgetList[pParams[0]].ShowRepeat = pParams[20];		
		wiWidgetList[pParams[0]].Enabled = pParams[21];
		wiWidgetList[pParams[0]].Angle = flParams[7];			
		wiWidgetList[pParams[0]].ModuleID2 = pParams[22];	
		wiWidgetList[pParams[0]].SourceCell2 = pParams[23];		
		wiWidgetList[pParams[0]].ShowValue2From = pParams[24];
		wiWidgetList[pParams[0]].ShowValue2To = pParams[25];
		//wiWidgetList[pParams[0]].ShowValueStr = flParams[8];	
		wiWidgetList[pParams[0]].ShowValue2Str = pParams[27];
		wiWidgetList[pParams[0]].ShowWithCamera = pParams[29];
		
		memset(wiWidgetList[pParams[0]].Name, 0, 64);
		if (strlen(&strParam[0]) > 63) memcpy(wiWidgetList[pParams[0]].Name, &strParam[0], 63);
			else memcpy(wiWidgetList[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));
		memset(wiWidgetList[pParams[0]].Color, 0, 10);
		if (strlen(&strParam[256]) > 6) memcpy(wiWidgetList[pParams[0]].Color, &strParam[256], 6);
			else memcpy(wiWidgetList[pParams[0]].Color, &strParam[256], strlen(&strParam[256]));
		unsigned int uiColor = Hex2Int(wiWidgetList[pParams[0]].Color);
		wiWidgetList[pParams[0]].RGBColor.Red = uiColor & 255;
		wiWidgetList[pParams[0]].RGBColor.Green = (uiColor >> 8) & 255;
		wiWidgetList[pParams[0]].RGBColor.Blue = (uiColor >> 16) & 255;
		
		memset(wiWidgetList[pParams[0]].ValueTypeName, 0, 10);
		if (strlen(&strParam[512]) > 9) memcpy(wiWidgetList[pParams[0]].ValueTypeName, &strParam[512], 9);
			else memcpy(wiWidgetList[pParams[0]].ValueTypeName, &strParam[512], strlen(&strParam[512]));
		memset(wiWidgetList[pParams[0]].Path, 0, 256);
		if (strlen(&strParam[768]) > 255) memcpy(wiWidgetList[pParams[0]].Path, &strParam[768], 255);
			else memcpy(wiWidgetList[pParams[0]].Path, &strParam[768], strlen(&strParam[768]));
		
		if (pParams[0] != pParams[28]) WEB_change_pos(pParams[0], pParams[28], (char*)wiWidgetList, iWidgetsCnt, sizeof(WIDGET_INFO));		
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&widget_mutex);
	if (ret) SaveWidgets();
	if (iShowMode >= 0) add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_UPDATE, pParams[1]);
	return 1;
}

int WEB_widgets_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&widget_mutex);
	TestWidgets(1);
	DBG_MUTEX_UNLOCK(&widget_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/widgets/\"><h1>ВИДЖЕТЫ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&widget_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th>"
						"<th>Включен</th>"
						"<th>Статус</th>"
						"<th>Идентификатор</th>"
						"<th>Режим работы</th>"
						"<th>Тип</th><th>Наименование</th>"
						"<th>Масштаб</th>"
						"<th>Направление движения</th>"
						"<th>Скорость</th>"
						"<th>День (Weather)</th>"
						"<th>Период перезагрузки (сек.)</th>"
						"<th>Дни недели</th>"
						"<th>Интервал времени 1</th>"
						"<th>Интервал времени 2</th>"
						"<th>Интервал времени 3</th>"
						"<th>ИД1[Номер сенсора]=С-По|Равно</th>"
						"<th>ИД2[Номер сенсора]=С-По|Равно</th>"
						"<th>Цвет</th>"
						"<th>Смещение значения</th>"
						"<th>Коэффициент</th>"
						"<th>Единицы измерения</th>"
						"<th>Собственная траектория движения</th>"
						"<th>Путь</th>"
						"<th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iWidgetsCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurWidgetPage = iPage;
	iCurPage = iCurWidgetPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurWidgetPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurWidgetPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iWidgetsCnt) iLastOption = iWidgetsCnt;
	
	WEB_pages_list(msg_tx, "widgets", iSubPages, iCurPage, "");	
		
	int fl1[6];
	int fl2[6];
	int fl3[6];
	char* cDisableFlag;
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		if ((wiWidgetList[n].Type < WIDGET_TYPE_UNKNOWN) || (wiWidgetList[n].Type >= WIDGET_TYPE_MAX)) wiWidgetList[n].Type = WIDGET_TYPE_UNKNOWN;
		if (wiWidgetList[n].Status == -2) cDisableFlag = " disabled "; else cDisableFlag = "";
		fl1[0] = (int)floor((float)wiWidgetList[n].NotBeforeTime/10000);
		fl2[0] = (int)floor((float)wiWidgetList[n].NotBeforeTime/100);
		fl2[0] = fl2[0] - (fl1[0]*100);
		fl3[0] = wiWidgetList[n].NotBeforeTime - ((fl1[0]*10000) + (fl2[0]*100));
		fl1[1] = (int)floor((float)wiWidgetList[n].NotAfterTime/10000);
		fl2[1] = (int)floor((float)wiWidgetList[n].NotAfterTime/100);
		fl2[1] = fl2[1] - (fl1[1]*100);
		fl3[1] = wiWidgetList[n].NotAfterTime - ((fl1[1]*10000) + (fl2[1]*100));
		
		fl1[2] = (int)floor((float)wiWidgetList[n].NotBeforeTime2/10000);
		fl2[2] = (int)floor((float)wiWidgetList[n].NotBeforeTime2/100);
		fl2[2] = fl2[2] - (fl1[2]*100);
		fl3[2] = wiWidgetList[n].NotBeforeTime2 - ((fl1[2]*10000) + (fl2[2]*100));
		fl1[3] = (int)floor((float)wiWidgetList[n].NotAfterTime2/10000);
		fl2[3] = (int)floor((float)wiWidgetList[n].NotAfterTime2/100);
		fl2[3] = fl2[3] - (fl1[3]*100);
		fl3[3] = wiWidgetList[n].NotAfterTime2 - ((fl1[3]*10000) + (fl2[3]*100));
		
		fl1[4] = (int)floor((float)wiWidgetList[n].NotBeforeTime3/10000);
		fl2[4] = (int)floor((float)wiWidgetList[n].NotBeforeTime3/100);
		fl2[4] = fl2[4] - (fl1[4]*100);
		fl3[4] = wiWidgetList[n].NotBeforeTime3 - ((fl1[4]*10000) + (fl2[4]*100));
		fl1[5] = (int)floor((float)wiWidgetList[n].NotAfterTime3/10000);
		fl2[5] = (int)floor((float)wiWidgetList[n].NotAfterTime3/100);
		fl2[5] = fl2[5] - (fl1[5]*100);
		fl3[5] = wiWidgetList[n].NotAfterTime3 - ((fl1[5]*10000) + (fl2[5]*100));
	
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, 
					"<tr><form action='/widgets/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='NewPos' value=%i style='width: 50px;'></td>\r\n"
					"<td><input type='checkbox' name='Enabled'%s%s></td>\r\n"
					"<td><input type='text' name='Status' value='%s' style='width: 70px;' disabled></td>\r\n"
					"<td><input type='text' name='WidgetID' value='%.4s' maxlength=4 style='width: 60px;'%s></td>\r\n"
					"<td>По расписанию:<input type='checkbox' style='width: 150px;' name='SM01'%s%s><br>\r\n"
						"&emsp;&emsp;Камера:<input type='checkbox' name='ShowCamera'%s%s><br>\r\n"
						"Меню:<input type='checkbox' name='SM02'%s%s><br>\r\n"
						"Таймер:<input type='checkbox' name='SM03'%s%s>\r\n"
							"<input type='number' name='SM03_Time' value=%i style='width: 50px;' %s>(%i)сек.<br>\r\n"
							"&emsp;&emsp;Повтор:<input type='checkbox' name='Repeat'%s%s><br>\r\n"
						"Всегда:<input type='checkbox' name='SM04'%s%s>\r\n"
						"</td>\r\n"
					"<td><select name='Type' style='width: 240px;'%s>\r\n"
						"<option %s value='%i' disabled>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"
						"</select></td>\r\n"
					"<td><input type='text' name='Name' value='%s'%s></td>\r\n"
					"<td><input type='number' name='Scale' min=0.1 max=10 value=%g step=0.01%s></td>\r\n"
					"<td><select name='Direct' style='width: 240px;'%s>\r\n"
						"<option %s value='%i' disabled>%s</option>\r\n"
						"<option %s value='%i'>%s</option>\r\n"					
						"<option %s value='%i'>%s</option>\r\n"						
						"<option %s value='%i'>%s</option>\r\n"						
						"<option %s value='%i'>%s</option>\r\n"						
						"</select>\r\n"
						"<br />Позиция X:<input type='number' name='DirectX' min=0 max=100 value=%g step=0.1 %s>\r\n"
						"<br />Позиция Y:<input type='number' name='DirectY' min=0 max=100 value=%g step=0.1 %s>\r\n"
						"<br />Угол:<input type='number' name='Angle' min=0 max=360 value=%g step=1 %s>\r\n"
						"</td>\r\n"
					"<td><input type='number' name='Speed' min=0.1 max=10 value=%g step=0.01%s></td>\r\n"
					"<td><input type='number' name='SetInt' min=0 max=7 value=%i%s></td>\r\n"
					"<td><input type='number' name='Refresh' value=%i%s></td>\r\n"
					"<td> Пн:<input type='checkbox' name='Mo'%s%s>\r\n"
						" Вт:<input type='checkbox' name='Tu'%s%s>\r\n"
						" Ср:<input type='checkbox' name='We'%s%s>\r\n"
						" Чт:<input type='checkbox' name='Th'%s%s>\r\n"
						" Пт:<input type='checkbox' name='Fr'%s%s>\r\n"
						" Сб:<input type='checkbox' name='Sa'%s%s>\r\n"
						" Вс:<input type='checkbox' name='Su'%s%s></td>\r\n"
					"<td><input type='number' name='Hour11' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Min11' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Sec11' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
					"-<input type='number' name='Hour12' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Min12' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Sec12' min=0 max=59 value='%i' style='width: 37px;'%s></td>\r\n"
					"<td><input type='number' name='Hour21' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Min21' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Sec21' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
					"-<input type='number' name='Hour22' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Min22' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Sec22' min=0 max=59 value='%i' style='width: 37px;'%s></td>\r\n"
					"<td><input type='number' name='Hour31' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Min31' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Sec31' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
					"-<input type='number' name='Hour32' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Min32' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
						":<input type='number' name='Sec32' min=0 max=59 value='%i' style='width: 37px;'%s></td>\r\n"										
					"<td><input type='text' name='SrcID1' value='%.4s' maxlength=4 style='width: 60px;'%s>\r\n"
						":<input type='number' name='SrcCell1' min=1 max=%i value=%i%s>\r\n"
						"=<input type='number' name='ShowFrom1' step=0.0001 value=%g  style='width: 60px;' %s>\r\n"
						"-<input type='number' name='ShowTo1' step=0.0001 value=%g  style='width: 60px;' %s></td>\r\n"	
					"<td><input type='text' name='SrcID2' value='%.4s' maxlength=4 style='width: 60px;'%s>\r\n"
						":<input type='number' name='SrcCell2' min=1 max=%i value=%i%s>\r\n"
						"=<input type='number' name='ShowFrom2' value=%i  style='width: 60px;' %s>\r\n"
						"-<input type='number' name='ShowTo2' value=%i  style='width: 60px;' %s>\r\n"	
						"|<input type='text' name='ShowIf2' value='%.4s' maxlength=4 style='width: 60px;'%s></td>\r\n"				
					"<td><input type='text' name='Color' maxlength=6 value='%s' style='width: 60px;'%s></td>\r\n"
					"<td><input type='number' name='Offset' value=%i style='width: 60px;' %s></td>\r\n"
					"<td><input type='number' name='Coef' min=0.0001 max=100 step=0.0001 value=%g%s></td>\r\n"
					"<td><input type='text' name='ValTpNm' maxlength=9 value='%s' style='width: 60px;'%s></td>\r\n"
					"<td><input type='checkbox' name='SelfPt'%s%s></td>\r\n"
					"<td><input type='text' name='Path' maxlength=255 value='%s' style='width: 380px;'%s></td>\r\n",
					session->request_id,
					n,n, 
					wiWidgetList[n].Enabled ? " checked" : "", cDisableFlag,
					wiWidgetList[n].Status < 0 ? "Не загружен" : "Загружен",
					(char*)&wiWidgetList[n].WidgetID, cDisableFlag, 
					wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_DAYTIME ? " checked" : "", cDisableFlag,
					wiWidgetList[n].ShowWithCamera ? " checked" : "", cDisableFlag,						
					wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_MENU ? " checked" : "", cDisableFlag,
					wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_TIMEOUT ? " checked" : "", cDisableFlag,	
					wiWidgetList[n].ShowTime, cDisableFlag, wiWidgetList[n].ShowTimer,
					wiWidgetList[n].ShowRepeat ? " checked" : "", cDisableFlag,	
					wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_ALLWAYS ? " checked" : "", cDisableFlag,										
					cDisableFlag,
					wiWidgetList[n].Type == WIDGET_TYPE_UNKNOWN ? "selected" : "", WIDGET_TYPE_UNKNOWN, GetWidgetTypeName(WIDGET_TYPE_UNKNOWN),
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_IMAGE ? "selected" : "", WIDGET_TYPE_SENSOR_IMAGE, GetWidgetTypeName(WIDGET_TYPE_SENSOR_IMAGE),
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_TACHOMETER ? "selected" : "", WIDGET_TYPE_SENSOR_TACHOMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_TACHOMETER),
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_BLACKTACHO ? "selected" : "", WIDGET_TYPE_SENSOR_BLACKTACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_BLACKTACHO),
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_TACHO_SCALE ? "selected" : "", WIDGET_TYPE_SENSOR_TACHO_SCALE, GetWidgetTypeName(WIDGET_TYPE_SENSOR_TACHO_SCALE),
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_WHITETACHO ? "selected" : "", WIDGET_TYPE_SENSOR_WHITETACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_WHITETACHO),
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_TEMPMETER ? "selected" : "", WIDGET_TYPE_SENSOR_TEMPMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_TEMPMETER),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_BLACKREGULATOR ? "selected" : "", WIDGET_TYPE_SENSOR_BLACKREGULATOR, GetWidgetTypeName(WIDGET_TYPE_SENSOR_BLACKREGULATOR),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_CIRCLETACHO ? "selected" : "", WIDGET_TYPE_SENSOR_CIRCLETACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_CIRCLETACHO),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_DARKMETER ? "selected" : "", WIDGET_TYPE_SENSOR_DARKMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_DARKMETER),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_DARKTERMOMETER ? "selected" : "", WIDGET_TYPE_SENSOR_DARKTERMOMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_DARKTERMOMETER),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_GREENTACHO ? "selected" : "", WIDGET_TYPE_SENSOR_GREENTACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_GREENTACHO),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_OFFTACHO ? "selected" : "", WIDGET_TYPE_SENSOR_OFFTACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_OFFTACHO),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_SILVERREGULATOR ? "selected" : "", WIDGET_TYPE_SENSOR_SILVERREGULATOR, GetWidgetTypeName(WIDGET_TYPE_SENSOR_SILVERREGULATOR),	
					wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_WHITETERMOMETER ? "selected" : "", WIDGET_TYPE_SENSOR_WHITETERMOMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_WHITETERMOMETER),						
					wiWidgetList[n].Type == WIDGET_TYPE_IMAGE ? "selected" : "", WIDGET_TYPE_IMAGE, GetWidgetTypeName(WIDGET_TYPE_IMAGE),
					wiWidgetList[n].Type == WIDGET_TYPE_WEATHER ? "selected" : "", WIDGET_TYPE_WEATHER, GetWidgetTypeName(WIDGET_TYPE_WEATHER),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_EL ? "selected" : "", WIDGET_TYPE_CLOCK_EL, GetWidgetTypeName(WIDGET_TYPE_CLOCK_EL),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_MECH ? "selected" : "", WIDGET_TYPE_CLOCK_MECH, GetWidgetTypeName(WIDGET_TYPE_CLOCK_MECH),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_WHITE ? "selected" : "", WIDGET_TYPE_CLOCK_WHITE, GetWidgetTypeName(WIDGET_TYPE_CLOCK_WHITE),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_BROWN ? "selected" : "", WIDGET_TYPE_CLOCK_BROWN, GetWidgetTypeName(WIDGET_TYPE_CLOCK_BROWN),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_QUARTZ ? "selected" : "", WIDGET_TYPE_CLOCK_QUARTZ, GetWidgetTypeName(WIDGET_TYPE_CLOCK_QUARTZ),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_SKYBLUE ? "selected" : "", WIDGET_TYPE_CLOCK_SKYBLUE, GetWidgetTypeName(WIDGET_TYPE_CLOCK_SKYBLUE),
					wiWidgetList[n].Type == WIDGET_TYPE_CLOCK_ARROW ? "selected" : "", WIDGET_TYPE_CLOCK_ARROW, GetWidgetTypeName(WIDGET_TYPE_CLOCK_ARROW),
					wiWidgetList[n].Name,  cDisableFlag, wiWidgetList[n].Scale, cDisableFlag, 
					cDisableFlag,
					wiWidgetList[n].Direct == WIDGET_DIRECT_UNKNOWN ? "selected" : "", WIDGET_DIRECT_UNKNOWN, GetWidgetDirectName(WIDGET_DIRECT_UNKNOWN),
					wiWidgetList[n].Direct == WIDGET_DIRECT_BORDER_CW ? "selected" : "", WIDGET_DIRECT_BORDER_CW, GetWidgetDirectName(WIDGET_DIRECT_BORDER_CW),
					wiWidgetList[n].Direct == WIDGET_DIRECT_BORDER_CCW ? "selected" : "", WIDGET_DIRECT_BORDER_CCW, GetWidgetDirectName(WIDGET_DIRECT_BORDER_CCW),
					wiWidgetList[n].Direct == WIDGET_DIRECT_BORDER_PINGPONG ? "selected" : "", WIDGET_DIRECT_BORDER_PINGPONG, GetWidgetDirectName(WIDGET_DIRECT_BORDER_PINGPONG),
					wiWidgetList[n].Direct == WIDGET_DIRECT_STATIC ? "selected" : "", WIDGET_DIRECT_STATIC, GetWidgetDirectName(WIDGET_DIRECT_STATIC),
					wiWidgetList[n].DirectX, cDisableFlag, wiWidgetList[n].DirectY, cDisableFlag, wiWidgetList[n].Angle, cDisableFlag,
					wiWidgetList[n].Speed, cDisableFlag, wiWidgetList[n].Settings[0], cDisableFlag, wiWidgetList[n].Refresh, cDisableFlag, 
					wiWidgetList[n].WeekDays & 1 ? " checked" : "", cDisableFlag,
					wiWidgetList[n].WeekDays & 2 ? " checked" : "", cDisableFlag,
					wiWidgetList[n].WeekDays & 4 ? " checked" : "", cDisableFlag,
					wiWidgetList[n].WeekDays & 8 ? " checked" : "", cDisableFlag,
					wiWidgetList[n].WeekDays & 16 ? " checked" : "", cDisableFlag,
					wiWidgetList[n].WeekDays & 32 ? " checked" : "", cDisableFlag,
					wiWidgetList[n].WeekDays & 64 ? " checked" : "", cDisableFlag,
					fl1[0], cDisableFlag, fl2[0], cDisableFlag, fl3[0], cDisableFlag, 
					fl1[1], cDisableFlag, fl2[1], cDisableFlag, fl3[1], cDisableFlag,
					fl1[2], cDisableFlag, fl2[2], cDisableFlag, fl3[2], cDisableFlag,
					fl1[3], cDisableFlag, fl2[3], cDisableFlag, fl3[3], cDisableFlag,
					fl1[4], cDisableFlag, fl2[4], cDisableFlag, fl3[4], cDisableFlag,
					fl1[5], cDisableFlag, fl2[5], cDisableFlag, fl3[5], cDisableFlag,
					(char*)&wiWidgetList[n].ModuleID, cDisableFlag,
					MAX_MODULE_STATUSES, wiWidgetList[n].SourceCell, cDisableFlag,
					wiWidgetList[n].ShowValueFrom, cDisableFlag,
					wiWidgetList[n].ShowValueTo, cDisableFlag,
					(char*)&wiWidgetList[n].ModuleID2, cDisableFlag,
					MAX_MODULE_STATUSES, wiWidgetList[n].SourceCell2, cDisableFlag,
					wiWidgetList[n].ShowValue2From, cDisableFlag,
					wiWidgetList[n].ShowValue2To, cDisableFlag,
					(char*)&wiWidgetList[n].ShowValue2Str, cDisableFlag,
					wiWidgetList[n].Color, cDisableFlag,
					wiWidgetList[n].OffsetValue, cDisableFlag,
					wiWidgetList[n].Coefficient, cDisableFlag,
					wiWidgetList[n].ValueTypeName, cDisableFlag,
					wiWidgetList[n].SelfPath ? " checked" : "", cDisableFlag,
					wiWidgetList[n].Path, cDisableFlag
					);		
		strcat(msg_tx, msg_subbody);
		if (wiWidgetList[n].Status != -2) strcat(msg_tx, "<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/widgets/delete'>Удалить</button>\r\n"
					"<button type='submit' formaction='/widgets/get_up'>Выше</button>\r\n"
					"<button type='submit' formaction='/widgets/get_down'>Ниже</button></td></form></tr>\r\n");
				else
					strcat(msg_tx, "<td><button type='submit' formaction='/widgets/recover'>Вернуть</button></td></form></tr>\r\n");
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody, 
					"<tr><form action='/widgets/add'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='number' name='NewPos' value=%i style='width: 50px;'></td>\r\n"
					"<td><input type='checkbox' name='Enabled'></td>\r\n"
					"<td></td>\r\n"
					"<td><input type='text' name='WidgetID' value='' maxlength=4 style='width: 60px;'></td>\r\n"
					"<td>По расписанию:<input type='checkbox' style='width: 150px;' name='SM01'><br>\r\n"
						"&emsp;&emsp;Камера:<input type='checkbox' name='ShowCamera'><br>\r\n"
						"Меню:<input type='checkbox' name='SM02'><br>\r\n"
						"Таймер:<input type='checkbox' name='SM03'>\r\n"
							"<input type='number' name='SM03_Time' value=1 style='width: 50px;'><br>\r\n"
							"&emsp;&emsp;Повтор:<input type='checkbox' name='Repeat'><br>\r\n"
						"Всегда:<input type='checkbox' name='SM04'>\r\n"
						"</td>\r\n"
					"<td><select name='Type' style='width: 240px;'>\r\n"
						"<option selected value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"</select></td>\r\n"					
					"<td><input type='text' name='Name' value=''></td>\r\n"
					"<td><input type='number' name='Scale' min=0.01 max=5 value=1 step=0.01></td>\r\n"
					"<td><select name='Direct' style='width: 240px;'>\r\n"
						"<option selected value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"<option value='%i'>%s</option>\r\n"
						"</select>\r\n"
						"<br />Позиция X:<input type='number' name='DirectX' min=0 max=100 value=50 step=0.1>\r\n"
						"<br />Позиция Y:<input type='number' name='DirectY' min=0 max=100 value=50 step=0.1>\r\n"
						"<br />Угол:<input type='number' name='Angle' min=0 max=360 value=45 step=1>\r\n"
						"</td>\r\n"
					"<td><input type='number' name='Speed' min=0.01 max=5 value=1 step=0.01></td>\r\n"
					"<td><input type='number' name='SetInt' min=0 max=7 value=0></td>\r\n"
					"<td><input type='number' name='Refresh' value=0></td>\r\n"
					"<td> Пн:<input type='checkbox' name='Mo'>\r\n"
						" Вт:<input type='checkbox' name='Tu'>\r\n"
						" Ср:<input type='checkbox' name='We'>\r\n"
						" Чт:<input type='checkbox' name='Th'>\r\n"
						" Пт:<input type='checkbox' name='Fr'>\r\n"
						" Сб:<input type='checkbox' name='Sa'>\r\n"
						" Вс:<input type='checkbox' name='Su'></td>\r\n"
					"<td><input type='number' name='Hour11' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min11' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec11' min=0 max=59 value='0' style='width: 37px;'>\r\n"
					"-<input type='number' name='Hour12' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min12' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec12' min=0 max=59 value='0' style='width: 37px;'></td>\r\n"
					"<td><input type='number' name='Hour21' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min21' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec21' min=0 max=59 value='0' style='width: 37px;'>\r\n"
					"-<input type='number' name='Hour22' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min22' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec22' min=0 max=59 value='0' style='width: 37px;'></td>\r\n"
					"<td><input type='number' name='Hour31' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min31' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec31' min=0 max=59 value='0' style='width: 37px;'>\r\n"
					"-<input type='number' name='Hour32' min=0 max=24 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Min32' min=0 max=59 value='0' style='width: 37px;'>\r\n"
						":<input type='number' name='Sec32' min=0 max=59 value='0' style='width: 37px;'></td>\r\n"										
					"<td><input type='text' name='SrcID1' value='' maxlength=4 style='width: 60px;'>\r\n"
						":<input type='number' name='SrcCell1' min=1 max=%i value=1>\r\n"
						"=<input type='number' name='ShowFrom1' step=0.0001 value=0  style='width: 60px;'>\r\n"
						"-<input type='number' name='ShowTo1' step=0.0001 value=0  style='width: 60px;'></td>\r\n"		
					"<td><input type='text' name='SrcID2' value='' maxlength=4 style='width: 60px;'>\r\n"
						":<input type='number' name='SrcCell2' min=1 max=%i value=1>\r\n"
						"=<input type='number' name='ShowFrom2' value=0  style='width: 60px;'>\r\n"
						"-<input type='number' name='ShowTo2' value=0  style='width: 60px;'>\r\n"	
						"|<input type='text' name='ShowIf2' value='' maxlength=4 style='width: 60px;'></td>\r\n"				
					"<td><input type='text' name='Color' maxlength=6 value='00FF00' style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='Offset' value=0 style='width: 60px;'></td>\r\n"
					"<td><input type='number' name='Coef' min=0.0001 max=100 step=0.0001 value=1></td>\r\n"
					"<td><input type='text' name='ValTpNm' maxlength=9 value='' style='width: 60px;'></td>\r\n"
					"<td><input type='checkbox' name='SelfPt'></td>\r\n"
					"<td><input type='text' name='Path' value='' style='width: 380px;'></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iWidgetsCnt,
					WIDGET_TYPE_SENSOR_IMAGE, GetWidgetTypeName(WIDGET_TYPE_SENSOR_IMAGE),
					WIDGET_TYPE_SENSOR_TACHOMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_TACHOMETER),
					WIDGET_TYPE_SENSOR_BLACKTACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_BLACKTACHO),
					WIDGET_TYPE_SENSOR_TACHO_SCALE, GetWidgetTypeName(WIDGET_TYPE_SENSOR_TACHO_SCALE),
					WIDGET_TYPE_SENSOR_WHITETACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_WHITETACHO),
					WIDGET_TYPE_SENSOR_TEMPMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_TEMPMETER),
					WIDGET_TYPE_SENSOR_BLACKREGULATOR, GetWidgetTypeName(WIDGET_TYPE_SENSOR_BLACKREGULATOR),
					WIDGET_TYPE_SENSOR_CIRCLETACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_CIRCLETACHO),
					WIDGET_TYPE_SENSOR_DARKMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_DARKMETER),
					WIDGET_TYPE_SENSOR_DARKTERMOMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_DARKTERMOMETER),
					WIDGET_TYPE_SENSOR_GREENTACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_GREENTACHO),
					WIDGET_TYPE_SENSOR_OFFTACHO, GetWidgetTypeName(WIDGET_TYPE_SENSOR_OFFTACHO),
					WIDGET_TYPE_SENSOR_SILVERREGULATOR, GetWidgetTypeName(WIDGET_TYPE_SENSOR_SILVERREGULATOR),
					WIDGET_TYPE_SENSOR_WHITETERMOMETER, GetWidgetTypeName(WIDGET_TYPE_SENSOR_WHITETERMOMETER),
					WIDGET_TYPE_IMAGE, GetWidgetTypeName(WIDGET_TYPE_IMAGE),
					WIDGET_TYPE_WEATHER, GetWidgetTypeName(WIDGET_TYPE_WEATHER),
					WIDGET_TYPE_CLOCK_EL, GetWidgetTypeName(WIDGET_TYPE_CLOCK_EL),
					WIDGET_TYPE_CLOCK_MECH, GetWidgetTypeName(WIDGET_TYPE_CLOCK_MECH), 
					WIDGET_TYPE_CLOCK_WHITE, GetWidgetTypeName(WIDGET_TYPE_CLOCK_WHITE), 
					WIDGET_TYPE_CLOCK_BROWN, GetWidgetTypeName(WIDGET_TYPE_CLOCK_BROWN), 
					WIDGET_TYPE_CLOCK_QUARTZ, GetWidgetTypeName(WIDGET_TYPE_CLOCK_QUARTZ), 
					WIDGET_TYPE_CLOCK_SKYBLUE, GetWidgetTypeName(WIDGET_TYPE_CLOCK_SKYBLUE), 
					WIDGET_TYPE_CLOCK_ARROW, GetWidgetTypeName(WIDGET_TYPE_CLOCK_ARROW),
					WIDGET_DIRECT_BORDER_CW, GetWidgetDirectName(WIDGET_DIRECT_BORDER_CW),
					WIDGET_DIRECT_BORDER_CCW, GetWidgetDirectName(WIDGET_DIRECT_BORDER_CCW),					
					WIDGET_DIRECT_BORDER_PINGPONG, GetWidgetDirectName(WIDGET_DIRECT_BORDER_PINGPONG),					
					WIDGET_DIRECT_STATIC, GetWidgetDirectName(WIDGET_DIRECT_STATIC),					
					MAX_MODULE_STATUSES, MAX_MODULE_STATUSES,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "widgets", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	DBG_MUTEX_UNLOCK(&widget_mutex);
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_stream_up(int *pParams)
{
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iInternetRadioCnt) && iInternetRadioCnt)
	{
		if (pParams[0] > 0) WEB_change_pos(pParams[0], pParams[0] - 1, (char*)irInternetRadio, iInternetRadioCnt, sizeof(STREAM_SETT));
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	return 1;
}

int WEB_stream_down(int *pParams)
{
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iInternetRadioCnt) && iInternetRadioCnt)
	{
		if (pParams[0] < (iInternetRadioCnt - 1)) WEB_change_pos(pParams[0], pParams[0] + 1, (char*)irInternetRadio, iInternetRadioCnt, sizeof(STREAM_SETT));
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	return 1;
}

int WEB_stream_play(int *pParams)
{
	StreamOn(pParams[0]);
	return 1;
}

int WEB_stream_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iInternetRadioCnt) && iInternetRadioCnt)
	{
		memset(&irInternetRadio[pParams[0]], 0, sizeof(STREAM_SETT));
		irInternetRadio[pParams[0]].Access = pParams[1];	
		if ((pParams[2] < 0) || (pParams[2] >= iInternetRadioCnt)) pParams[2] = 0;
		irInternetRadio[pParams[0]].Type = pParams[2];
		if (strlen(&strParam[0]) > 63) memcpy(irInternetRadio[pParams[0]].Name, &strParam[0], 63);
			else memcpy(irInternetRadio[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));
		if (strlen(&strParam[64]) > 255) memcpy(irInternetRadio[pParams[0]].URL, &strParam[64], 255);
			else memcpy(irInternetRadio[pParams[0]].URL, &strParam[64], strlen(&strParam[64]));		
		irInternetRadio[pParams[0]].OverTCP = pParams[3];
		irInternetRadio[pParams[0]].ID = pParams[4];
		irInternetRadio[pParams[0]].VideoEn = pParams[5];
		irInternetRadio[pParams[0]].AudioEn = pParams[6];
	
		if (pParams[0] != pParams[10]) WEB_change_pos(pParams[0], pParams[10], (char*)irInternetRadio, iInternetRadioCnt, sizeof(STREAM_SETT));
		
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveStreams();
	return 1;
}

int WEB_stream_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iInternetRadioCnt) && iInternetRadioCnt)
	{
		STREAM_SETT *ir = (STREAM_SETT*)DBG_MALLOC(sizeof(STREAM_SETT)*(iInternetRadioCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iInternetRadioCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ir[clk], &irInternetRadio[i], sizeof(STREAM_SETT));
				clk++;
			}
		DBG_FREE(irInternetRadio);
		irInternetRadio = ir;
		iInternetRadioCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveStreams();
	return 1;
}

int WEB_stream_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&system_mutex);
	iInternetRadioCnt++;
	irInternetRadio = (STREAM_SETT*)DBG_REALLOC(irInternetRadio, sizeof(STREAM_SETT)*iInternetRadioCnt);
	memset(&irInternetRadio[iInternetRadioCnt-1], 0, sizeof(STREAM_SETT));
	irInternetRadio[iInternetRadioCnt-1].Access = pParams[1];	
	if ((pParams[2] < 0) || (pParams[2] >= iInternetRadioCnt)) pParams[2] = 0;
	irInternetRadio[iInternetRadioCnt-1].Type = pParams[2];
	if (strlen(&strParam[0]) > 63) memcpy(irInternetRadio[iInternetRadioCnt-1].Name, &strParam[0], 63);
		else memcpy(irInternetRadio[iInternetRadioCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	if (strlen(&strParam[64]) > 255) memcpy(irInternetRadio[iInternetRadioCnt-1].URL, &strParam[64], 255);
		else memcpy(irInternetRadio[iInternetRadioCnt-1].URL, &strParam[64], strlen(&strParam[64]));
	irInternetRadio[iInternetRadioCnt-1].OverTCP = pParams[3];
	irInternetRadio[iInternetRadioCnt-1].ID = pParams[4];
	irInternetRadio[iInternetRadioCnt-1].VideoEn = pParams[5];
	irInternetRadio[iInternetRadioCnt-1].AudioEn = pParams[6];
	
	if (pParams[10] < (iInternetRadioCnt-1)) WEB_change_pos(iInternetRadioCnt-1, pParams[10], (char*)irInternetRadio, iInternetRadioCnt, sizeof(STREAM_SETT));
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveStreams();
	return 1;
}

int WEB_streams_filters(int *pParams)
{
	iCurStreamTypeFilter = pParams[0];	
	return 1;
}

int WEB_streams_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n, i, k;	
	
	DBG_MUTEX_LOCK(&system_mutex);
	TestStreamTypes(1);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	char *msg_listtype = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/streams/\"><h1>ПОТОКИ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&system_mutex);
	
	k = 0;
	memset(msg_listtype, 0, 3072);
	if ((iCurStreamTypeFilter < -1) || (iCurStreamTypeFilter >= iStreamTypeCnt)) iCurStreamTypeFilter = -1;
	
	for (i = -1; i < iStreamTypeCnt; i++)
	{
		memset(msg_subbody, 0, 256);				
		if (iCurStreamTypeFilter != i)
		{
			if (strlen(msg_listtype) < 2800) 
			{
				if (i == -1)
					sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", i, "ВСЕ");
					else
					sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", i, stStreamType[i].Name);
			}
			else
			{
				if (k) break;
			}
		}
		else
		{
			if (iCurStreamTypeFilter == -1)
				sprintf(msg_subbody, "<option selected value='%i'>%s</option>\r\n", iCurStreamTypeFilter, "ВСЕ");
				else
				sprintf(msg_subbody, "<option selected value='%i'>%s</option>\r\n", iCurStreamTypeFilter, stStreamType[i].Name);
			k = 1;
		}
		strcat(msg_listtype, msg_subbody);
	}
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, "<form action='/streams/filters'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"Тип: <select name='Type' style='width: 130px;'>\r\n"
						"	%s\r\n"
						"	</select></td>\r\n"
						"<button type='submit'>Применить</button></form>\r\n",
						session->request_id, msg_listtype);
	strcat(msg_tx, msg_subbody);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>ИД</th><th>Уровень доступа</th><th>Тип</th><th>Наименование</th><th>Путь(Адрес)</th><th>TCP/Video/Audio</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	
	int iStreamFiltCnt = 0;
	for (n = 0; n < iInternetRadioCnt; n++)
	{
		if ((irInternetRadio[n].Type < 0) || (irInternetRadio[n].Type >= iStreamTypeCnt)) irInternetRadio[n].Type = 0;
		if ((iCurStreamTypeFilter == -1) || (iCurStreamTypeFilter == irInternetRadio[n].Type))
		{
			iStreamFiltCnt++;
		}
	}
	
	iSubPages = (int)ceil((double)iStreamFiltCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurStreamPage = iPage;
	iCurPage = iCurStreamPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurStreamPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurStreamPage = iCurPage;
	}
	
	/*iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iInternetRadioCnt) iLastOption = iInternetRadioCnt;*/
	
	iFirstOption = 0;
	iLastOption = 0;
	i = 0;
	k = 0;
	for (n = 0; n < iInternetRadioCnt; n++)
	{
		if ((iCurStreamTypeFilter == -1) || (iCurStreamTypeFilter == irInternetRadio[n].Type))
		{			
			if (i < (iCurPage * WEB_PAGE_MAX_LEN)) {i++; iFirstOption = n;}
			if (k < (iFirstOption + WEB_PAGE_MAX_LEN)) {k++; iLastOption = n;} else break;
		}
	}
	
	WEB_pages_list(msg_tx, "streams", iSubPages, iCurPage, "");	
									
	if (iLastOption < iInternetRadioCnt)
	for (n = iFirstOption; n <= iLastOption; n++)
	{
		if ((iCurStreamTypeFilter == -1) || (iCurStreamTypeFilter == irInternetRadio[n].Type))
		{		
			k = 0;
			memset(msg_listtype, 0, 3072);
			for (i = 0; i < iStreamTypeCnt; i++)
			{
				memset(msg_subbody, 0, 256);				
				if (strlen(msg_listtype) >= 2800)
				{
					if (k == 0)
					{
						sprintf(msg_subbody, "<option selected value='%i'>%s</option>\r\n", i, stStreamType[irInternetRadio[n].Type].Name);
						strcat(msg_listtype, msg_subbody);
					}
					break;
				}
				if (irInternetRadio[n].Type != i)
					sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", i, stStreamType[i].Name);
					else
					{
						sprintf(msg_subbody, "<option selected value='%i'>%s</option>\r\n", i, stStreamType[i].Name);
						k = 1;
					}
				strcat(msg_listtype, msg_subbody);
			}
			
			memset(msg_subbody, 0, 3072);
			sprintf(msg_subbody, 
						"<tr><form action='/streams/save'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"<input type='hidden' name='Num' value=%i>"
						"<td><input type='number' name='NewPos' value=%i style='width: 50px;'></td>\r\n"
						"<td><input type='text' name='ID' value='%.4s' maxlength=4 style='width: 60px;'></td>\r\n"
						"<td><input type='number' name='Access' min=0 max=1000 value=%i style='width: 50px;'></td>\r\n"
						"<td><select name='Type' style='width: 140px;'>\r\n"
							"%s"
							"</select></td>\r\n"
						"<td><input type='text' name='Name' value='%s'></td>\r\n"
						"<td><input type='text' name='Url' value='%s' style='width: 380px;'></td>\r\n"
						"<td>"
							"<input type='checkbox' name='OverTCP'%s>"
							"<input type='checkbox' name='VideoEn'%s>"
							"<input type='checkbox' name='AudioEn'%s>"
						"</td>\r\n"
						"<td>%s\r\n"
						"<button type='submit' formaction='/streams/play'>Воспроизвести</button></td></form></tr>\r\n",
						session->request_id,
						n,n, 
						(char*)&irInternetRadio[n].ID,
						irInternetRadio[n].Access, msg_listtype, irInternetRadio[n].Name, irInternetRadio[n].URL, 
						irInternetRadio[n].OverTCP ? " checked " : "", 
						irInternetRadio[n].VideoEn ? " checked " : "",
						irInternetRadio[n].AudioEn ? " checked " : "",
						(session->access_level > 0) ? "<button type='submit'>Сохранить</button>\r\n"
													"<button type='submit' formaction='/streams/delete'>Удалить</button>\r\n"
													"<button type='submit' formaction='/streams/get_up'>Выше</button>\r\n"
													"<button type='submit' formaction='/streams/get_down'>Ниже</button>\r\n" : "");		
			strcat(msg_tx, msg_subbody);
		
			if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
		}
	}	
	
	memset(msg_listtype, 0, 3072);
	for (i = 0; i < iStreamTypeCnt; i++)
	{
		if (strlen(msg_listtype) >= 2800) break;
		memset(msg_subbody, 0, 256);				
		if (i != 0) sprintf(msg_subbody, "<option value='%i'>%s</option>\r\n", i, stStreamType[i].Name);
		else sprintf(msg_subbody, "<option selected value='%i'>%s</option>\r\n", i, stStreamType[i].Name);
		strcat(msg_listtype, msg_subbody);
	}
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/streams/add'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='number' name='NewPos' value=%i style='width: 50px;'></td>\r\n"	
					"<td><input type='text' name='ID' value='' maxlength=4 style='width: 60px;'></td>\r\n"				
					"<td><input type='number' name='Access' min=0 max=1000 value=0 style='width: 50px;'></td>\r\n"
					"<td><select name='Type' value='%s' style='width: 140px;'>\r\n"
						"%s"
						"</select></td>\r\n"
					"<td><input type='text' name='Name' value=''></td>\r\n"
					"<td><input type='text' name='Url' value='' style='width: 380px;'></td>\r\n"
					"<td>"
						"<input type='checkbox' name='OverTCP'>"
						"<input type='checkbox' name='VideoEn' checked>"
						"<input type='checkbox' name='AudioEn' checked>"
					"</td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iInternetRadioCnt, stStreamType[0].Name, msg_listtype, iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "streams", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	DBG_FREE(msg_listtype);	
	
	return 1;
}

int WEB_streamtype_save(int *pParams, char* strParam)
{
	int ret = 0;
	int n;		
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iStreamTypeCnt) && iStreamTypeCnt)
	{
		memset(&stStreamType[pParams[0]], 0, sizeof(STREAM_TYPE));
		if (strlen(&strParam[0]) > 63) memcpy(stStreamType[pParams[0]].Name, &strParam[0], 63);
			else memcpy(stStreamType[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));	
		for (n = 0; n < iInternetRadioCnt; n++)
			if (irInternetRadio[n].Type == pParams[0]) irInternetRadio[n].Type = 0;		
		ret = 1;
		
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveStreamTypes();
	return 1;
}

int WEB_streamtype_del(int *pParams)
{
	int ret = 0;
	int n;		
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iStreamTypeCnt) && iStreamTypeCnt)
	{
		if (iStreamTypeCnt > 1)
		{
			STREAM_TYPE *st = (STREAM_TYPE*)DBG_MALLOC(sizeof(STREAM_TYPE)*(iStreamTypeCnt-1));
			int i;
			int clk = 0;
			for (i = 0; i < iStreamTypeCnt; i++)
				if (i != pParams[0])
				{
					memcpy(&st[clk], &stStreamType[i], sizeof(STREAM_TYPE));
					clk++;
				}
			DBG_FREE(stStreamType);
			stStreamType = st;
			iStreamTypeCnt--;	
		}
		else
		{
			memset(&stStreamType[pParams[0]], 0, sizeof(STREAM_TYPE));
			strcpy(stStreamType[pParams[0]].Name, "ALL");
		}		
		for (n = 0; n < iInternetRadioCnt; n++)
			if (irInternetRadio[n].Type == pParams[0]) irInternetRadio[n].Type = 0;		
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveStreamTypes();
	return 1;
}

int WEB_streamtype_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&system_mutex);
	iStreamTypeCnt++;
	stStreamType = (STREAM_TYPE*)DBG_REALLOC(stStreamType, sizeof(STREAM_TYPE)*iStreamTypeCnt);
	memset(&stStreamType[iStreamTypeCnt-1], 0, sizeof(STREAM_TYPE));
	if (strlen(&strParam[0]) > 63) memcpy(stStreamType[iStreamTypeCnt-1].Name, &strParam[0], 63);
		else memcpy(stStreamType[iStreamTypeCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveStreamTypes();
	return 1;
}

int WEB_streamtypes_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&system_mutex);
	TestStreamTypes(1);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/streamtypes/\"><h1>ТИПЫ ПОТОКОВ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&system_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Наименование</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iStreamTypeCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurStrmTypePage = iPage;
	iCurPage = iCurStrmTypePage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurStrmTypePage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurStrmTypePage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iStreamTypeCnt) iLastOption = iStreamTypeCnt;
	
	WEB_pages_list(msg_tx, "streamtypes", iSubPages, iCurPage, "");	
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, 
					"<tr><form action='/streamtypes/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='Name' value='%s'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/streamtypes/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n, stStreamType[n].Name);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/streamtypes/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='text' name='Name' value=''></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "streamtypes", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_mail_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iMailCnt) && iMailCnt)
	{
		memset(&miMailList[pParams[0]], 0, sizeof(MAIL_INFO));
		miMailList[pParams[0]].Group = pParams[1];
		if (strlen(&strParam[0]) > 63) memcpy(miMailList[pParams[0]].Address, &strParam[0], 63);
			else memcpy(miMailList[pParams[0]].Address, &strParam[0], strlen(&strParam[0]));
		if (strlen(&strParam[64]) > 63) memcpy(miMailList[pParams[0]].Address2, &strParam[64], 63);
			else memcpy(miMailList[pParams[0]].Address2, &strParam[64], strlen(&strParam[64]));
		if (strlen(&strParam[128]) > 63) memcpy(miMailList[pParams[0]].MainText, &strParam[128], 63);
			else memcpy(miMailList[pParams[0]].MainText, &strParam[128], strlen(&strParam[128]));
		if (strlen(&strParam[192]) > 63) memcpy(miMailList[pParams[0]].BodyText, &strParam[192], 63);
			else memcpy(miMailList[pParams[0]].BodyText, &strParam[192], strlen(&strParam[192]));
		if (strlen(&strParam[256]) > 63) memcpy(miMailList[pParams[0]].FilePath, &strParam[256], 63);
			else memcpy(miMailList[pParams[0]].FilePath, &strParam[256], strlen(&strParam[256]));
		if (strlen(&strParam[320]) > 63) memcpy(miMailList[pParams[0]].FilePath2, &strParam[320], 63);
			else memcpy(miMailList[pParams[0]].FilePath2, &strParam[320], strlen(&strParam[320]));
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveMails();
	return 1;
}

int WEB_mail_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iMailCnt) && iMailCnt)
	{
		MAIL_INFO *mi = (MAIL_INFO*)DBG_MALLOC(sizeof(MAIL_INFO)*(iMailCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iMailCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&mi[clk], &miMailList[i], sizeof(MAIL_INFO));
				clk++;
			}
		DBG_FREE(miMailList);
		miMailList = mi;
		iMailCnt--;		
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveMails();
	return 1;
}

int WEB_mail_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&system_mutex);
	iMailCnt++;
	miMailList = (MAIL_INFO*)DBG_REALLOC(miMailList, sizeof(MAIL_INFO)*iMailCnt);
	memset(&miMailList[iMailCnt-1], 0, sizeof(MAIL_INFO));
	miMailList[iMailCnt-1].Group = pParams[1];
	if (strlen(&strParam[0]) > 63) memcpy(miMailList[iMailCnt-1].Address, &strParam[0], 63);
		else memcpy(miMailList[iMailCnt-1].Address, &strParam[0], strlen(&strParam[0]));
	if (strlen(&strParam[64]) > 63) memcpy(miMailList[iMailCnt-1].Address2, &strParam[64], 63);
		else memcpy(miMailList[iMailCnt-1].Address2, &strParam[64], strlen(&strParam[64]));
	if (strlen(&strParam[128]) > 63) memcpy(miMailList[iMailCnt-1].MainText, &strParam[128], 63);
		else memcpy(miMailList[iMailCnt-1].MainText, &strParam[128], strlen(&strParam[128]));
	if (strlen(&strParam[192]) > 63) memcpy(miMailList[iMailCnt-1].BodyText, &strParam[192], 63);
		else memcpy(miMailList[iMailCnt-1].BodyText, &strParam[192], strlen(&strParam[192]));
	if (strlen(&strParam[256]) > 63) memcpy(miMailList[iMailCnt-1].FilePath, &strParam[256], 63);
		else memcpy(miMailList[iMailCnt-1].FilePath, &strParam[256], strlen(&strParam[256]));
	if (strlen(&strParam[320]) > 63) memcpy(miMailList[iMailCnt-1].FilePath2, &strParam[320], 63);
		else memcpy(miMailList[iMailCnt-1].FilePath2, &strParam[320], strlen(&strParam[320]));	
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveMails();
	return 1;
}

int WEB_mails_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&system_mutex);
	TestMails(1);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/mails/\"><h1>ПОЧТОВЫЕ АДРЕСА</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&system_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Код группы</th><th>Адрес 1</th><th>Адрес 2</th><th>Тема</th>"
						"<th>Тело</th><th>Файл 1</th><th>Файл 2</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iMailCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurMailPage = iPage;
	iCurPage = iCurMailPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurMailPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurMailPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iMailCnt) iLastOption = iMailCnt;
	
	WEB_pages_list(msg_tx, "mails", iSubPages, iCurPage, "");	
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, 
					"<tr><form action='/mails/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='number' name='Group' min=0 max=50 value='%i'></td>\r\n"
					"<td><input type='text' name='Address1' value='%s'></td>\r\n"
					"<td><input type='text' name='Address2' value='%s'></td>\r\n"
					"<td><input type='text' name='MainText' value='%s'></td>\r\n"
					"<td><input type='text' name='BodyText' value='%s'></td>\r\n"
					"<td><input type='text' name='Path1' value='%s'></td>\r\n"
					"<td><input type='text' name='Path2' value='%s'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/mails/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n, miMailList[n].Group, miMailList[n].Address, miMailList[n].Address2,
					miMailList[n].MainText,miMailList[n].BodyText,miMailList[n].FilePath,miMailList[n].FilePath2);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/mails/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='number' name='Group' min=0 max=50 value=''></td>\r\n"
					"<td><input type='text' name='Address1' value=''></td>\r\n"
					"<td><input type='text' name='Address2' value=''></td>\r\n"
					"<td><input type='text' name='MainText' value=''></td>\r\n"
					"<td><input type='text' name='BodyText' value=''></td>\r\n"
					"<td><input type='text' name='Path1' value=''></td>\r\n"
					"<td><input type='text' name='Path2' value=''></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "mails", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_sound_play(int *pParams)
{
	Action_PlaySound(pParams[1], 0, 100);
	return 1;
}

int WEB_sound_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iSoundListCnt) && iSoundListCnt)
	{
		mSoundList[pParams[0]].ID = pParams[1];
		memset(mSoundList[pParams[0]].Path, 0, 256);
		if (strlen(strParam) > 255) memcpy(mSoundList[pParams[0]].Path, strParam, 255);
			else memcpy(mSoundList[pParams[0]].Path, strParam, strlen(strParam));
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret) SaveSounds();
	return 1;
}

int WEB_sound_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iSoundListCnt) && iSoundListCnt)
	{
		SYS_SOUND_TYPE *sst = (SYS_SOUND_TYPE*)DBG_MALLOC(sizeof(SYS_SOUND_TYPE)*(iSoundListCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iSoundListCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&sst[clk], &mSoundList[i], sizeof(SYS_SOUND_TYPE));
				clk++;
			}
		DBG_FREE(mSoundList);
		mSoundList = sst;
		iSoundListCnt--;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret) SaveSounds();
	return 1;
}

int WEB_sound_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	iSoundListCnt++;
	mSoundList = (SYS_SOUND_TYPE*)DBG_REALLOC(mSoundList, sizeof(SYS_SOUND_TYPE)*iSoundListCnt);
	memset(&mSoundList[iSoundListCnt-1], 0, sizeof(SYS_SOUND_TYPE));
	mSoundList[iSoundListCnt-1].ID = pParams[1];
	memset(mSoundList[iSoundListCnt-1].Path, 0, 256);
	if (strlen(strParam) > 255) memcpy(mSoundList[iSoundListCnt-1].Path, strParam, 255);
		else memcpy(mSoundList[iSoundListCnt-1].Path, strParam, strlen(strParam));
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	SaveSounds();
	return 1;
}

int WEB_sounds_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	TestSounds(1);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/sounds/\"><h1>ЗВУКИ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Идентификатор</th><th>Путь</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iSoundListCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurSoundPage = iPage;
	iCurPage = iCurSoundPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurSoundPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurSoundPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iSoundListCnt) iLastOption = iSoundListCnt;
	
	WEB_pages_list(msg_tx, "sounds", iSubPages, iCurPage, "");	
								
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, 
					"<tr><form action='/sounds/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='ID' maxlength=4 value='%.4s'></td>\r\n"
					"<td><input type='text' name='Path' value='%s'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/sounds/play'>Воспроизвести</button>\r\n"
					"<button type='submit' formaction='/sounds/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n, (char*)&mSoundList[n].ID, mSoundList[n].Path);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/sounds/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='text' name='ID' value=''></td>\r\n"
					"<td><input type='text' name='Path' value=''></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "sounds", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_explorer_play(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);	
	int iCurPath;
	char *cLastPath;
	if (pParams[0] == 1)
	{
		iCurPath = iCurExplLeftPath;
		cLastPath = cLastLeftPath;
	}
	if (pParams[0] == 2)
	{
		iCurPath = iCurExplRightPath;
		cLastPath = cLastRightPath;
	}
	
	if ((pParams[0] & 3) && (iCurPath < iDirsCnt) && iDirsCnt)
	{
		if ((strlen(diDirList[iCurPath].Path) + strlen(cLastPath) + 2) < MAX_FILE_LEN) 
		{
			char cDir[MAX_FILE_LEN];
			memset(cDir, 0, MAX_FILE_LEN);
			strcpy(cDir, diDirList[iCurPath].Path);
			strcat(cDir, cLastPath);
			//printf("exp play '%s'\n", cDir);
			ret = UpdateListFiles(cDir, UPDATE_LIST_SCANTYPE_CUR);
			if (ret == 0) 
			{		
				if (uiShowModeCur >= 2) uiShowModeCur = 1;
				dbgprintf(3, "No files in WEB_explorer_play '%s'\n", diDirList[pParams[0]].Path);
				ShowNewMessage("Нет файлов для воспроизведения");
				UpdateListFiles(NULL, UPDATE_LIST_SCANTYPE_FULL);
			}
			else
			{
				if (uiShowModeCur < 2) uiShowModeCur = 2;
			}
		} else dbgprintf(2, "Long path for play\n");
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SetChangeShowNow(1);	
	return 1;
}

int WEB_explorer_show(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);	
	int iCurPath;
	char *cLastPath;
	if (pParams[0] == 1)
	{
		iCurPath = iCurExplLeftPath;
		cLastPath = cLastLeftPath;
	}
	if (pParams[0] == 2)
	{
		iCurPath = iCurExplRightPath;
		cLastPath = cLastRightPath;
	}
	
	if ((pParams[0] & 3) && (iCurPath < iDirsCnt) && iDirsCnt)
	{		
		if ((strlen(diDirList[iCurPath].Path) + strlen(cLastPath) + 2) < MAX_FILE_LEN) 
		{
			char cDir[MAX_FILE_LEN];
			memset(cDir, 0, MAX_FILE_LEN);
			strcpy(cDir, diDirList[iCurPath].Path);
			strcat(cDir, cLastPath);
			//printf("exp show '%s'\n", cDir);
			memset(cCurrentFileLocation, 0, 256);
			memcpy(cCurrentFileLocation, cDir, strlen(cDir));
			dbgprintf(4, "Set show path(w): '%s'\n", cDir);
			ret = 1;
		} else dbgprintf(2, "Long path for show\n");
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) 
	{
		add_sys_cmd_in_list(SYSTEM_CMD_SET_SHOW_PATH, 0);
		SetChangeShowNow(1);
	}
	return 1;
}

int WEB_explorer_refresh(int *pParams)
{
	int res = 0;
	int ret = 0;
	if (pParams[0] == 1) 
	{
		res = WEB_refresh_dir(iCurExplLeftPath, cLastLeftPath, &diExplorerLeft, &uiExplorerLeftCnt);
		ret = 1;
	}
	if (pParams[0] == 2) 
	{
		res = WEB_refresh_dir(iCurExplRightPath, cLastRightPath, &diExplorerRight, &uiExplorerRightCnt);
		ret = 1;
	}
	if (ret && (res == 0)) WEB_AddMessageInList("Error open Dir: %i", ret);
	return 1;
}

int WEB_explorer_prev(int *pParams)
{
	int res = 0;
	int ret = 0;
	int i, n;
	if (pParams[0] == 1)
	{
		i = strlen(cLastLeftPath);
		for(n = (i - 1); n >= 0; n--) if (cLastLeftPath[n] == 47) break;
		if (n >= 0) memset(&cLastLeftPath[n], 0, MAX_FILE_LEN - n);
		res = WEB_refresh_dir(iCurExplLeftPath, cLastLeftPath, &diExplorerLeft, &uiExplorerLeftCnt);
		ret = 1;
	}
	if (pParams[0] == 2)
	{
		i = strlen(cLastRightPath);
		for(n = (i - 1); n >= 0; n--) if (cLastRightPath[n] == 47) break;
		if (n >= 0) memset(&cLastRightPath[n], 0, MAX_FILE_LEN - n);
		res = WEB_refresh_dir(iCurExplRightPath, cLastRightPath, &diExplorerRight, &uiExplorerRightCnt);
		ret = 1;
	}
	if (ret && (res == 0)) WEB_AddMessageInList("Error open Dir: %i", ret);
	return 1;
}
			
int WEB_explorer_open(int *pParams, unsigned int uiAccess)
{
	int res = 0;
	int ret = 0;
	int imode = 0;
	char buff[256];
	if ((pParams[0] == 1) && (pParams[1] >= 0) && (pParams[1] < uiExplorerLeftCnt) && uiExplorerLeftCnt)
	{
		memset(buff, 0, 256);
		res = 0;
		DBG_MUTEX_LOCK(&system_mutex);
		if (iDirsCnt && (diDirList[iCurExplLeftPath].Access <= uiAccess))
		{
			strcpy(buff, diDirList[iCurExplLeftPath].Path);
			res = 1;
		}
		DBG_MUTEX_UNLOCK(&system_mutex);		
		if ((res == 1) && ((strlen(buff) + strlen(cLastLeftPath) + strlen(diExplorerLeft[pParams[1]].Path)) < 255))
		{
			if (diExplorerLeft[pParams[1]].Type == 0)
			{
				if (strlen(buff) && (buff[strlen(buff)-1] == 47)) buff[strlen(buff)-1] = 0;
				if (cLastLeftPath[0] != 47) strcat(buff, "/");
				strcat(buff, cLastLeftPath);
				if (strlen(buff) && (buff[strlen(buff)-1] != 47)) strcat(buff, "/");
				strcat(buff, diExplorerLeft[pParams[1]].Path);
				res = GetFileType(diExplorerLeft[pParams[1]].Path);
				if (res)
				{
					PlayURL(buff, 1);
					ret = 1;
				}
				else
				{
					memset(cLastViewerFile, 0, MAX_FILE_LEN);
					if (mbFileBuffer.data_size != 0) 
					{
						DBG_FREE(mbFileBuffer.void_data);
						memset(&mbFileBuffer, 0, sizeof(mbFileBuffer));
					}
					res = WEB_load_limit_file_in_buff(buff, &mbFileBuffer, iCurViewSize);
					if (res == 0) WEB_AddMessageInList("Error load file: '%s'", diExplorerLeft[pParams[1]].Path);
					if (res == 1) 
					{
						strcpy(cLastViewerFile, buff);
						imode = 1;
					}
					if (res == -1) WEB_AddMessageInList("Error load file (file size > %i): '%s'", iCurViewSize, diExplorerLeft[pParams[1]].Path);	
				}
			}
			if (diExplorerLeft[pParams[1]].Type == 1)
			{
				if (cLastLeftPath[strlen(cLastLeftPath)-1] != 47) strcat(cLastLeftPath, "/");
				strcat(cLastLeftPath, diExplorerLeft[pParams[1]].Path);
				res = WEB_refresh_dir(iCurExplLeftPath, cLastLeftPath, &diExplorerLeft, &uiExplorerLeftCnt);
				ret = 1;
			}
		} else WEB_AddMessageInList("Long Dir: %i > 255", strlen(cLastLeftPath) + strlen(diExplorerLeft[pParams[1]].Path));
	}
	if ((pParams[0] == 2) && (pParams[1] >= 0) && (pParams[1] < uiExplorerRightCnt) && uiExplorerRightCnt)
	{
		memset(buff, 0, 256);
		res = 0;
		DBG_MUTEX_LOCK(&system_mutex);
		if (iDirsCnt && (diDirList[iCurExplRightPath].Access <= uiAccess))
		{
			strcpy(buff, diDirList[iCurExplRightPath].Path);
			res = 1;
		}
		DBG_MUTEX_UNLOCK(&system_mutex);		
		if ((res == 1) && ((strlen(buff) + strlen(cLastRightPath) + strlen(diExplorerRight[pParams[1]].Path)) < 255))
		{
			if (diExplorerRight[pParams[1]].Type == 0)
			{
				if (strlen(buff) && (buff[strlen(buff)-1] == 47)) buff[strlen(buff)-1] = 0;
				if (cLastLeftPath[0] != 47) strcat(buff, "/");
				strcat(buff, cLastRightPath);
				if (strlen(buff) && (buff[strlen(buff)-1] != 47)) strcat(buff, "/");
				strcat(buff, diExplorerRight[pParams[1]].Path);
				res = GetFileType(diExplorerRight[pParams[1]].Path);
				if (res)
				{
					PlayURL(buff, 1);
					ret = 1;
				}
				else
				{
					memset(cLastViewerFile, 0, MAX_FILE_LEN);
					if (mbFileBuffer.data_size != 0) 
					{
						DBG_FREE(mbFileBuffer.void_data);
						memset(&mbFileBuffer, 0, sizeof(mbFileBuffer));
					}
					res = WEB_load_limit_file_in_buff(buff, &mbFileBuffer, iCurViewSize);
					if (res == 0) WEB_AddMessageInList("Error load file: '%s'", diExplorerRight[pParams[1]].Path);
					if (res == 1) 
					{
						strcpy(cLastViewerFile, buff);
						imode = 1;
					}
					if (res == -1) WEB_AddMessageInList("Error load file (file size > %i): '%s'", iCurViewSize, diExplorerRight[pParams[1]].Path);	
				}
			}
			if (diExplorerRight[pParams[1]].Type == 1)
			{
				if (cLastRightPath[strlen(cLastRightPath)-1] != 47) strcat(cLastRightPath, "/");
				strcat(cLastRightPath, diExplorerRight[pParams[1]].Path);
				res = WEB_refresh_dir(iCurExplRightPath, cLastRightPath, &diExplorerRight, &uiExplorerRightCnt);
				ret = 1;
			}
		}
		else WEB_AddMessageInList("Long Dir: %i > 255", strlen(cLastRightPath) + strlen(diExplorerRight[pParams[1]].Path));
	}
	if (ret && (res == 0)) WEB_AddMessageInList("Error open directory or file: %i", ret);
	return imode;
}

int WEB_explorer_change(int *pParams, unsigned int uiAccess)
{
	int res = 0;
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 1) && (pParams[0] <= 2) && (pParams[1] >= 0) && (pParams[1] < iDirsCnt) && iDirsCnt)
	{
		if (diDirList[pParams[1]].Access <= uiAccess)
		{
			if (pParams[0] == 1) 
			{
				iCurExplLeftPath = pParams[1];
				iExplorerLeftPage = 0;
				memset(cLastLeftPath, 0, MAX_FILE_LEN);
			}
			else 
			{
				iCurExplRightPath = pParams[1];
				iExplorerRightPage = 0;
				memset(cLastRightPath, 0, MAX_FILE_LEN);
			}
			ret = pParams[0];
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret == 1) res = WEB_refresh_dir(iCurExplLeftPath, cLastLeftPath, &diExplorerLeft, &uiExplorerLeftCnt);
	if (ret == 2) res = WEB_refresh_dir(iCurExplRightPath, cLastRightPath, &diExplorerRight, &uiExplorerRightCnt);	
	if (ret && (res == 0)) WEB_AddMessageInList("Error open Dir: %i", ret);
	return 1;
}

int WEB_explorer_work_request(char *msg_tx, WEB_SESSION *session, int *pParams, unsigned int uiAccess)
{
	char *buff0 = (char*)DBG_MALLOC(256);
	char *buff1 = (char*)DBG_MALLOC(1024);
	char *buff2 = (char*)DBG_MALLOC(1024);
	char *msg_subbody = (char*)DBG_MALLOC(1024);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	if (pParams[2] == 1) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>КОПИРОВАНИЕ</h1></a><br />\r\n");	
	if (pParams[2] == 2) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ПЕРЕМЕЩЕНИЕ</h1></a><br />\r\n");	
	if (pParams[2] == 3) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>УДАЛЕНИЕ</h1></a><br />\r\n");	
	if (pParams[2] == 4) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ПЕРЕИМЕНОВАНИЕ</h1></a><br />\r\n");	
	if (pParams[2] == 5) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>СОЗДАНИЕ</h1></a><br />\r\n");	
	
	memset(buff0, 0, 256);
	memset(buff1, 0, 1024);
	memset(buff2, 0, 1024);
	int res = 0;
	
	DBG_MUTEX_LOCK(&system_mutex);		
	if ((pParams[0] == 1) && (pParams[1] >= 0) && ((pParams[2] == 5) || ((pParams[1] < uiExplorerLeftCnt) && uiExplorerLeftCnt)))
	{
		if (iDirsCnt && (diDirList[iCurExplLeftPath].Access <= uiAccess))
		{
			strcpy(buff1, diDirList[iCurExplLeftPath].Path);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] == 47)) buff1[strlen(buff1)-1] = 0;
			if (cLastLeftPath[0] != 47) strcat(buff1, "/");
			strcat(buff1, cLastLeftPath);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] != 47)) strcat(buff1, "/");
			if (pParams[2] < 4) strcat(buff1, diExplorerLeft[pParams[1]].Path);
			if (pParams[2] < 5) strcpy(buff0, diExplorerLeft[pParams[1]].Path);
			
			strcpy(buff2, diDirList[iCurExplRightPath].Path);
			if (strlen(buff2) && (buff2[strlen(buff2)-1] == 47)) buff2[strlen(buff2)-1] = 0;
			if (cLastRightPath[0] != 47) strcat(buff2, "/");
			strcat(buff2, cLastRightPath);			
			res = 1;
		}
	}
	if ((pParams[0] == 2) && (pParams[1] >= 0) && ((pParams[2] == 5) || ((pParams[1] < uiExplorerRightCnt) && uiExplorerRightCnt)))
	{
		if (iDirsCnt && (diDirList[iCurExplRightPath].Access <= uiAccess))
		{
			strcpy(buff1, diDirList[iCurExplRightPath].Path);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] == 47)) buff1[strlen(buff1)-1] = 0;
			if (cLastRightPath[0] != 47) strcat(buff1, "/");
			strcat(buff1, cLastRightPath);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] != 47)) strcat(buff1, "/");
			if (pParams[2] < 4) strcat(buff1, diExplorerRight[pParams[1]].Path);
			if (pParams[2] < 5) strcpy(buff0, diExplorerRight[pParams[1]].Path);
			
			strcpy(buff2, diDirList[iCurExplLeftPath].Path);
			if (strlen(buff2) && (buff2[strlen(buff2)-1] == 47)) buff2[strlen(buff2)-1] = 0;
			if (cLastLeftPath[0] != 47) strcat(buff2, "/");
			strcat(buff2, cLastLeftPath);			
			res = 1;
		}
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);		
	
	if (res)
	{
		memset(msg_subbody, 0, 1024);
		if (pParams[2] == 1)
			sprintf(msg_subbody, 							
							"<form action='/explorer/work_accept'>\r\n"
							"<input type='hidden' name='Num' value=%i>"
							"<input type='hidden' name='Path' value=%i>"
							"<input type='hidden' name='Act' value=%i>"
							"Копировать '%s' в '%s'\r\n"
							"<input type='text' name='Name' value='%s' minlength=1 maxlength=64 style='width: 150px;'>\r\n"							
							"<button type='submit'>Выполнить</button></form>\r\n", pParams[0], pParams[1], pParams[2], buff1, buff2, buff0);
		if (pParams[2] == 2)
			sprintf(msg_subbody, 
							"<form action='/explorer/work_accept'>\r\n"
							"<input type='hidden' name='Num' value=%i>"
							"<input type='hidden' name='Path' value=%i>"
							"<input type='hidden' name='Act' value=%i>"
							"Переместить '%s' в '%s'\r\n"
							"<input type='text' name='Name' value='%s' minlength=1 maxlength=64 style='width: 150px;'>\r\n"							
							"<button type='submit'>Выполнить</button></form>\r\n", pParams[0], pParams[1], pParams[2], buff1, buff2, buff0);
		if (pParams[2] == 3)
			sprintf(msg_subbody, 
							"<form action='/explorer/work_accept'>\r\n"
							"<input type='hidden' name='Num' value=%i>"
							"<input type='hidden' name='Path' value=%i>"
							"<input type='hidden' name='Act' value=%i>"
							"Удалить '%s'\r\n"
							"<button type='submit'>Выполнить</button></form>\r\n", pParams[0], pParams[1], pParams[2], buff1);
		if (pParams[2] == 4)
			sprintf(msg_subbody, 
							"<form action='/explorer/work_accept'>\r\n"
							"<input type='hidden' name='Num' value=%i>"
							"<input type='hidden' name='Path' value=%i>"
							"<input type='hidden' name='Act' value=%i>"
							"Переименовать '%s%s' в '%s'\r\n"
							"<input type='text' name='Name' value='%s' minlength=1 maxlength=64 style='width: 150px;'>\r\n"							
							"<button type='submit'>Выполнить</button></form>\r\n", pParams[0], pParams[1], pParams[2], buff1, buff0, buff1, buff0);
		if (pParams[2] == 5)
			sprintf(msg_subbody, 
							"<form action='/explorer/work_accept'>\r\n"
							"<input type='hidden' name='Num' value=%i>"
							"<input type='hidden' name='Path' value=%i>"
							"<input type='hidden' name='Act' value=%i>"
							"Создать в '%s'\r\n"
							"<input type='text' name='Name' value='%s' minlength=1 maxlength=64 style='width: 150px;'>\r\n"							
							"<button type='submit'>Выполнить</button></form>\r\n", pParams[0], pParams[1], pParams[2], buff1, buff0);
		strcat(msg_tx, msg_subbody);
	}
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	DBG_FREE(buff0);
	DBG_FREE(buff1);
	DBG_FREE(buff2);
	
	return 1;
}

int WEB_explorer_work_accept(int *pParams, char *strParams, unsigned int uiAccess)
{
	char *buff1 = (char*)DBG_MALLOC(1024);
	char *buff2 = (char*)DBG_MALLOC(1024);
	
	memset(buff1, 0, 1024);
	memset(buff2, 0, 1024);
	int res = 0;
	
	DBG_MUTEX_LOCK(&system_mutex);		
	if ((pParams[0] == 1) && (pParams[1] >= 0) && ((pParams[2] == 5) || ((pParams[1] < uiExplorerLeftCnt) && uiExplorerLeftCnt)))
	{
		if (iDirsCnt && (diDirList[iCurExplLeftPath].Access <= uiAccess))
		{
			strcpy(buff1, diDirList[iCurExplLeftPath].Path);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] == 47)) buff1[strlen(buff1)-1] = 0;
			if (cLastLeftPath[0] != 47) strcat(buff1, "/");
			strcat(buff1, cLastLeftPath);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] != 47)) strcat(buff1, "/");
			if (pParams[2] == 4) strcpy(buff2, buff1);
			if (pParams[2] != 5) strcat(buff1, diExplorerLeft[pParams[1]].Path);
				else strcat(buff1, strParams);
				
			if (pParams[2] != 4)
			{
				strcpy(buff2, diDirList[iCurExplRightPath].Path);
				if (strlen(buff2) && (buff2[strlen(buff2)-1] == 47)) buff2[strlen(buff2)-1] = 0;
				if (cLastRightPath[0] != 47) strcat(buff2, "/");
				strcat(buff2, cLastRightPath);
				if (strlen(buff2) && (buff1[strlen(buff2)-1] != 47)) strcat(buff2, "/");			
			}
			strcat(buff2, strParams);			
			res = 1;
		}
	}
	if ((pParams[0] == 2) && (pParams[1] >= 0) && ((pParams[2] == 5) || ((pParams[1] < uiExplorerRightCnt) && uiExplorerRightCnt)))
	{
		if (iDirsCnt && (diDirList[iCurExplRightPath].Access <= uiAccess))
		{
			strcpy(buff1, diDirList[iCurExplRightPath].Path);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] == 47)) buff1[strlen(buff1)-1] = 0;
			if (cLastRightPath[0] != 47) strcat(buff1, "/");
			strcat(buff1, cLastRightPath);
			if (strlen(buff1) && (buff1[strlen(buff1)-1] != 47)) strcat(buff1, "/");
			if (pParams[2] == 4) strcpy(buff2, buff1);
			if (pParams[2] != 5) strcat(buff1, diExplorerRight[pParams[1]].Path);
				else strcat(buff1, strParams);
			
			if (pParams[2] != 4)
			{
				strcpy(buff2, diDirList[iCurExplLeftPath].Path);
				if (strlen(buff2) && (buff2[strlen(buff2)-1] == 47)) buff2[strlen(buff2)-1] = 0;
				if (cLastLeftPath[0] != 47) strcat(buff2, "/");
				strcat(buff2, cLastLeftPath);			
				if (strlen(buff2) && (buff1[strlen(buff2)-1] != 47)) strcat(buff2, "/");
			}
			strcat(buff2, strParams);			
			res = 1;
		}
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);		
	
	if (res)
	{
		DBG_MUTEX_LOCK(&WEB_explorer_mutex);
		if (cExplorerBusy == 0)
		{
			cExplorerBusy = pParams[2];
			misc_buffer *mBuff = (misc_buffer*)DBG_MALLOC(sizeof(misc_buffer));
			mBuff->void_data = buff1;
			mBuff->void_data2 = buff2;
			mBuff->flag = cExplorerBusy;
			res = 2;
			pthread_create(&threadWEB_file, &tattrWEB_file, thread_WEB_file, (void*)mBuff); 
		}
		DBG_MUTEX_UNLOCK(&WEB_explorer_mutex);	
	}
	
	if (res != 2) 
	{
		DBG_FREE(buff1);
		DBG_FREE(buff2);
	}
	
	return 1;
}

int WEB_explorer_busy_test(char *msg_tx, WEB_SESSION *session)
{
	int iMode = 0;
	DBG_MUTEX_LOCK(&WEB_explorer_mutex);
	iMode = cExplorerBusy;
	DBG_MUTEX_UNLOCK(&WEB_explorer_mutex);
	if (iMode == 0) return 0;
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ПРОВОДНИК ЗАНЯТ</h1></a><br />\r\n");	
	if (iMode == 1) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ВЫПОЛНЯЕТСЯ КОПИРОВАНИЕ</h1></a><br />\r\n");	
	if (iMode == 2) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ВЫПОЛНЯЕТСЯ ПЕРЕМЕЩЕНИЕ</h1></a><br />\r\n");	
	if (iMode == 3) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ВЫПОЛНЯЕТСЯ УДАЛЕНИЕ</h1></a><br />\r\n");	
	if (iMode == 4) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ВЫПОЛНЯЕТСЯ ПЕРЕИМЕНОВАНИЕ</h1></a><br />\r\n");	
	if (iMode == 5) strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ВЫПОЛНЯЕТСЯ СОЗДАНИЕ</h1></a><br />\r\n");	
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	return 2;
}

int WEB_explorer_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage1, int iPage2, int errcode)
{
	int n, k;		
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	char *msg_pagebody1 = (char*)DBG_MALLOC(16384);
	char *msg_pagebody2 = (char*)DBG_MALLOC(16384);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/explorer/\"><h1>ПРОВОДНИК</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Путь</th><th>Действие</th><th>Действие</th><th>Путь</th><th>Номер</th></tr>");
	
	DBG_MUTEX_LOCK(&system_mutex);
	if ((iCurExplLeftPath < 0) || (iCurExplLeftPath >= iDirsCnt)) iCurExplLeftPath = 0;
	if ((iCurExplRightPath < 0) || (iCurExplRightPath >= iDirsCnt)) iCurExplRightPath = 0;
	if (iCurExplLeftPath < iDirsCnt)
	{
		memset(msg_pagebody1, 0, 16384);
		strcpy(msg_pagebody1, "<select name='Path' style='width: 140px;'%s>\r\n");
		for (n = 0; n < iDirsCnt; n++)
			if (strlen(msg_pagebody1) < 16000)
			{
				memset(msg_subbody, 0, 256);
				sprintf(msg_subbody, "<option %s value='%i'>%s</option>\r\n", (iCurExplLeftPath == n) ? " selected " : "", n, diDirList[n].Name);
				strcat(msg_pagebody1, msg_subbody);
			}
			else break;
		strcat(msg_pagebody1, "</select>\r\n");
		memset(msg_subbody, 0, 256);
		sprintf(msg_subbody, "<tr><form action='/explorer/change'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=1>"
					"<input type='hidden' name='Act' value=5>"
					"<td><button type='submit'>Сменить</button>%s</td>"
					"<td>%s</td><td>\r\n"
					"<button type='submit' formaction='/explorer/prev' %s>Назад</button>\r\n"
					"<button type='submit' formaction='/explorer/refresh'>Обновить</button>\r\n"
					"<button type='submit' formaction='/explorer/work_request'>Создать</button>\r\n"
					"<button type='submit' formaction='/explorer/play'>Воспроизвести</button>\r\n"
					"<button type='submit' formaction='/explorer/show'>Воспроизвести подпапки</button>\r\n"
					"</td></form>\r\n", 
					session->request_id,
					msg_pagebody1, cLastLeftPath,
					(strlen(cLastLeftPath) == 0) ? " disabled " : "");
		strcat(msg_tx, msg_subbody);
	}
	else
	{
		memset(msg_subbody, 0, 256);
		sprintf(msg_subbody, "<tr><td></td><td>Нет настроек</td><td></td>");
		strcat(msg_tx, msg_subbody);
	}
	if (iCurExplRightPath < iDirsCnt)
	{
		memset(msg_pagebody2, 0, 16384);
		strcpy(msg_pagebody2, "<select name='Path' style='width: 140px;'%s>\r\n");
		for (n = 0; n < iDirsCnt; n++)
			if (strlen(msg_pagebody2) < 16000)
			{
				memset(msg_subbody, 0, 256);
				sprintf(msg_subbody, "<option %s value='%i' %s>%s</option>\r\n", (iCurExplRightPath == n) ? " selected " : "", n, 
								(diDirList[n].Access > session->access_level) ? " disabled " : "", diDirList[n].Name);
				strcat(msg_pagebody2, msg_subbody);
			} else break;
		strcat(msg_pagebody2, "</select>\r\n");
		memset(msg_subbody, 0, 256);
		sprintf(msg_subbody, "<form action='/explorer/change'><td>"
							"<input type='hidden' name='req_index' value=%i>\r\n"
							"<button type='submit' formaction='/explorer/prev' %s>Назад</button>\r\n"					
							"<button type='submit' formaction='/explorer/refresh'>Обновить</button>\r\n"
							"<button type='submit' formaction='/explorer/work_request'>Создать</button>\r\n"
							"<button type='submit' formaction='/explorer/play'>Воспроизвести</button>\r\n"
							"<button type='submit' formaction='/explorer/show'>Воспроизвести подпапки</button>\r\n"
							"</td>\r\n"
							"<input type='hidden' name='Num' value=2>"
							"<input type='hidden' name='Act' value=5>"
							"<td>%s</td>"
							"<td><button type='submit'>Сменить</button>%s</td>"
							"</form></tr>\r\n", 
							session->request_id,
							(strlen(cLastRightPath) == 0) ? " disabled " : "",
							cLastRightPath, msg_pagebody2);
		strcat(msg_tx, msg_subbody);
	}
	else
	{
		memset(msg_subbody, 0, 256);
		sprintf(msg_subbody, "<td></td><td></td><td>Нет настроек</td></tr>");
		strcat(msg_tx, msg_subbody);
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
					
					
	int iSubPages1, iCurPage1, iFirstOption1, iLastOption1;
	iSubPages1 = (int)ceil((double)uiExplorerLeftCnt / WEB_PAGE_MAX_LEN);
	if (iPage1 != -1) iExplorerLeftPage = iPage1;
	iCurPage1 = iExplorerLeftPage;
	if (iCurPage1 >= iSubPages1) 
	{
		iCurPage1 = iSubPages1 - 1;
		iExplorerLeftPage = iCurPage1;
	}
	if (iCurPage1 <= 0) 
	{
		iCurPage1 = 0;
		iExplorerLeftPage = iCurPage1;
	}
	iFirstOption1 = iCurPage1 * WEB_PAGE_MAX_LEN;
	iLastOption1 = iFirstOption1 + WEB_PAGE_MAX_LEN;
	if (iLastOption1 >= uiExplorerLeftCnt) iLastOption1 = uiExplorerLeftCnt;
	
	int iSubPages2, iCurPage2, iFirstOption2, iLastOption2;
	iSubPages2 = (int)ceil((double)uiExplorerRightCnt / WEB_PAGE_MAX_LEN);
	if (iPage2 != -1) iExplorerRightPage = iPage2;
	iCurPage2 = iExplorerRightPage;
	if (iCurPage2 >= iSubPages2) 
	{
		iCurPage2 = iSubPages2 - 1;
		iExplorerRightPage = iCurPage2;
	}
	if (iCurPage2 <= 0) 
	{
		iCurPage2 = 0;
		iExplorerRightPage = iCurPage2;
	}
	iFirstOption2 = iCurPage2 * WEB_PAGE_MAX_LEN;
	iLastOption2 = iFirstOption2 + WEB_PAGE_MAX_LEN;
	if (iLastOption2 >= uiExplorerRightCnt) iLastOption2 = uiExplorerRightCnt;
		
	memset(msg_pagebody1, 0, 16384);
	WEB_pages_list(msg_pagebody1, "explorer", iSubPages1, iCurPage1, "&Num=1");	
		
	memset(msg_pagebody2, 0, 16384);
	WEB_pages_list(msg_pagebody2, "explorer", iSubPages2, iCurPage2, "&Num=2");	
		
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody, "<tr><td></td><td>%s</td><td></td><td></td><td>%s</td><td></td></tr>\r\n", msg_pagebody1, msg_pagebody2);	
	strcat(msg_tx, msg_subbody);
	
	char cType[4];
	cType[1] = 0;
	cType[3] = 0;
								
	for (n = 0; n < WEB_PAGE_MAX_LEN; n++)
	{
		if (iFirstOption1 < iLastOption1) 
		{
			k = diExplorerLeft[iFirstOption1].Sort;
			if (diExplorerLeft[k].Type == 0) 
			{
				cType[0] = 0;
				cType[2] = 0;
			}
			if (diExplorerLeft[k].Type == 1) 
			{
				cType[0] = 91;
				cType[2] = 93;
			}
			if (diExplorerLeft[k].Type > 1) 
			{
				cType[0] = 123;
				cType[2] = 125;
			}
			memset(msg_pagebody1, 0, 3072);
			if (diExplorerLeft[k].Type == 0) 
			{
				sprintf(msg_pagebody1, 
					"<tr>"
					"<td>%i</td>\r\n"
					"<td><a href=\"/explorer/open?req_index=%i&Num=1&Path=%i\">%s%s%s</a></td>\r\n"
					"<td><a href=\"/explorer/work_request?req_index=%i&Num=1&Act=3&Path=%i\">X</a>\r\n"
					"<a href=\"/explorer/work_request?req_index=%i&Num=1&Act=4&Path=%i\">[]</a>\r\n"
					"<a href=\"/explorer/work_request?req_index=%i&Num=1&Act=1&Path=%i\">&gt;</a>\r\n"
					"<a href=\"/explorer/work_request?req_index=%i&Num=1&Act=2&Path=%i\">&gt;&gt;</a></td>\r\n", 
					iFirstOption1,
					session->request_id, k, 
					&cType[0], diExplorerLeft[k].Path, &cType[2],
					session->request_id, k, 
					session->request_id, k, 
					session->request_id, k, 
					session->request_id, k);		
			}
			else
			{
				sprintf(msg_pagebody1, 
					"<tr>"
					"<td>%i</td>\r\n"
					"<td><a href=\"/explorer/open?req_index=%i&Num=1&Path=%i\">%s%s%s</a></td>\r\n"
					"<td><a href=\"/explorer/work_request?req_index=%i&Num=1&Act=3&Path=%i\">X</a></td>\r\n", 
					iFirstOption1,
					session->request_id, k, 
					&cType[0], diExplorerLeft[k].Path, &cType[2],
					session->request_id, k);		
			}
			strcat(msg_tx, msg_pagebody1);
			iFirstOption1++;
		}
		else strcat(msg_tx, "<tr><td></td><td></td><td></td>\r\n");
		if (iFirstOption2 < iLastOption2) 
		{
			k = diExplorerRight[iFirstOption2].Sort;
			if (diExplorerRight[k].Type == 0) 
			{
				cType[0] = 0;
				cType[2] = 0;
			}
			if (diExplorerRight[k].Type == 1) 
			{
				cType[0] = 91;
				cType[2] = 93;
			}
			if (diExplorerRight[k].Type > 1) 
			{
				cType[0] = 123;
				cType[2] = 125;
			}
			memset(msg_pagebody1, 0, 3072);
			if (diExplorerRight[k].Type == 0) 
			{
				sprintf(msg_pagebody1, 
						"<td><a href=\"/explorer/work_request?req_index=%i&Num=2&Act=2&Path=%i\">&lt;&lt;</a>\r\n"
						"<a href=\"/explorer/work_request?req_index=%i&Num=2&Act=1&Path=%i\">&lt;</a>\r\n"
						"<a href=\"/explorer/work_request?req_index=%i&Num=2&Act=4&Path=%i\">[]</a>\r\n"
						"<a href=\"/explorer/work_request?req_index=%i&Num=2&Act=3&Path=%i\">X</a></td>\r\n"
						"<td><a href=\"/explorer/open?req_index=%i&Num=2&Act=4&Path=%i\">%s%s%s</a></td>\r\n"
						"<td>%i</td>"
						"</tr>\r\n", 
						session->request_id, k, 
						session->request_id, k, 
						session->request_id, k, 
						session->request_id, k, 
						session->request_id, k, 
						&cType[0], diExplorerRight[k].Path, &cType[2], 
						iFirstOption2);		
			}
			else
			{
				sprintf(msg_pagebody1, 
						"<td><a href=\"/explorer/work_request?req_index=%i&Num=2&Act=3&Path=%i\">X</a></td>\r\n"
						"<td><a href=\"/explorer/open?req_index=%i&Num=2&Act=4&Path=%i\">%s%s%s</a></td>\r\n"
						"<td>%i</td>"
						"</tr>\r\n", 
						session->request_id, k, 
						session->request_id, k, 
						&cType[0], diExplorerRight[k].Path, &cType[2], 
						iFirstOption2);		
			}
			strcat(msg_tx, msg_pagebody1);
			iFirstOption2++;
		} else strcat(msg_tx, "<td></td><td></td><td></td></tr>\r\n");
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	
	strcat(msg_tx, msg_subbody);	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	DBG_FREE(msg_pagebody1);
	DBG_FREE(msg_pagebody2);
	
	return 1;
}

int WEB_messages_del()
{
	Menu_ClearMessageList(NULL, 0);	
	return 1;
}

int WEB_messages_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	char *cBody = (char*)DBG_MALLOC(2048);
	memset(cBody, 0, 2048);
	sprintf(cBody, "<br /><a href=\"/messages/\"><h1>СООБЩЕНИЯ</h1></a>\r\n"
					"<input type=\"button\" value='Очистить' onclick=\"window.location.href='/messages/delete?req_index=%i'\"><br />\r\n",
					session->request_id);	
	strcat(msg_tx, cBody);
	DBG_FREE(cBody);
	
	DBG_MUTEX_LOCK(&message_mutex);
		
	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iMessagesListCnt / (WEB_PAGE_MAX_LEN * 128));
	if (iPage != -1) iCurMessagesPage = iPage;
	iCurPage = iCurMessagesPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurMessagesPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurMessagesPage = iCurPage;
	}
	iFirstOption = iCurPage * (WEB_PAGE_MAX_LEN * 128);
	iLastOption = iFirstOption + (WEB_PAGE_MAX_LEN * 128);
	if (iFirstOption > 256) iFirstOption -= 256;
	if (iLastOption >= iMessagesListCnt) iLastOption = iMessagesListCnt;
						
	WEB_pages_list(msg_tx, "messages", iSubPages, iCurPage, "");
							
	strcat(msg_tx, "<p>");
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		strcat(msg_tx, &cMessagesList[n*MESSAGES_MAX_LEN]);
		strcat(msg_tx, "<br />");
	}
	strcat(msg_tx, "</p>");
	
	DBG_MUTEX_UNLOCK(&message_mutex);
	
	WEB_pages_list(msg_tx, "messages", iSubPages, iCurPage, "");
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_viewer_del()
{
	memset(cLastViewerFile, 0, MAX_FILE_LEN);
	if (mbFileBuffer.data_size != 0) 
	{
		DBG_FREE(mbFileBuffer.void_data);
		memset(&mbFileBuffer, 0, sizeof(mbFileBuffer));
	}
	strcpy(cLastViewerFile, "Файл не загружен");	
	return 1;
}

int WEB_viewer_refresh()
{
	if (mbFileBuffer.data_size != 0) 
	{
		DBG_FREE(mbFileBuffer.void_data);
		memset(&mbFileBuffer, 0, sizeof(mbFileBuffer));
		int res = WEB_load_limit_file_in_buff(cLastViewerFile, &mbFileBuffer, iCurViewSize);
		if (res == 0) WEB_AddMessageInList("Error refresh file");
		if (res == -1) WEB_AddMessageInList("Error refresh file (file size > %i)", iCurViewSize);	
	}	
	return 1;
}

int WEB_viewer_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	
	char *cBody = (char*)DBG_MALLOC(2048);
	memset(cBody, 0, 2048);
	sprintf(cBody, "<br /><a href=\"/viewer/\"><h1>ПРОСМОТР ФАЙЛА</h1></a>\r\n"
					"<input type=\"button\" value='Очистить' onclick=\"window.location.href='/viewer/delete?req_index=%i'\">\r\n"
					"<input type=\"button\" value='Обновить' onclick=\"window.location.href='/viewer/refresh?req_index=%i'\"><br />\r\n",
					session->request_id, session->request_id);	
	strcat(msg_tx, cBody);
	DBG_FREE(cBody);
		
	DBG_MUTEX_LOCK(&system_mutex);
						
	memset(msg_subbody, 0, 512);
	if (mbFileBuffer.data_size == 0)
		sprintf(msg_subbody, "<p>%s</p>\r\n", cLastViewerFile);
		else
		sprintf(msg_subbody, "<p>%s   : %i байт</p>\r\n", cLastViewerFile, mbFileBuffer.data_size);
	strcat(msg_tx, msg_subbody);
	
	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)mbFileBuffer.data_size / (WEB_PAGE_MAX_LEN * 128));
	if (iPage != -1) iCurViewerPage = iPage;
	iCurPage = iCurViewerPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurViewerPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurViewerPage = iCurPage;
	}
	iFirstOption = iCurPage * (WEB_PAGE_MAX_LEN * 128);
	iLastOption = iFirstOption + (WEB_PAGE_MAX_LEN * 128);
	if (iFirstOption > 256) iFirstOption -= 256;
	if (iLastOption >= mbFileBuffer.data_size) iLastOption = mbFileBuffer.data_size;
						
	WEB_pages_list(msg_tx, "viewer", iSubPages, iCurPage, "");
							
	strcat(msg_tx, "<p>");
	int i =	strlen(msg_tx);
	char* data = mbFileBuffer.void_data;
	char cPrev = 0;
	for (n = iFirstOption; n < iLastOption; n++)
	{
		if ((data[n] > 31) && (data[n] != 38) && (data[n] != 60) && (data[n] != 62)) {msg_tx[i] = data[n];i++;}
		if (data[n] == 38) {memcpy(&msg_tx[i], "&amp;", strlen("&amp;")); i += strlen("&amp;");}
		if (data[n] == 60) {memcpy(&msg_tx[i], "&lt;", strlen("&lt;")); i += strlen("&lt;");}
		if (data[n] == 62) {memcpy(&msg_tx[i], "&gt;", strlen("&gt;")); i += strlen("&gt;");}
		if ((cPrev != 10) && (cPrev != 13) && (data[n] == 13)) {memcpy(&msg_tx[i], "<br />", strlen("<br />")); i += strlen("<br />");}
		if ((cPrev != 10) && (cPrev != 13) && (data[n] == 10)) {memcpy(&msg_tx[i], "<br />", strlen("<br />")); i += strlen("<br />");}
		if (data[n] == 9) {memcpy(&msg_tx[i], "     ", strlen("     ")); i += strlen("     ");}
		cPrev = data[n];
	}
	strcat(msg_tx, "</p>");
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	WEB_pages_list(msg_tx, "viewer", iSubPages, iCurPage, "");
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_manual_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/manual/\"><h1>ИНСТРУКЦИЯ</h1></a>\r\n");
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)mbManual.data_size / (WEB_PAGE_MAX_LEN * 128));
	if (iPage != -1) iCurManualPage = iPage;
	iCurPage = iCurManualPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurManualPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurManualPage = iCurPage;
	}
	iFirstOption = iCurPage * (WEB_PAGE_MAX_LEN * 128);
	iLastOption = iFirstOption + (WEB_PAGE_MAX_LEN * 128);
	if (iFirstOption > 256) iFirstOption -= 256;
	if (iLastOption >= mbManual.data_size) iLastOption = mbManual.data_size;
	
	WEB_pages_list(msg_tx, "manual", iSubPages, iCurPage, "");	
	
	strcat(msg_tx, "<p>");
	int i =	strlen(msg_tx);
	char* data = mbManual.void_data;
	char cPrev = 0;
	for (n = iFirstOption; n < iLastOption; n++)
	{
		if ((data[n] > 31) && (data[n] != 38) && (data[n] != 60) && (data[n] != 62)) {msg_tx[i] = data[n];i++;}
		if (data[n] == 38) {memcpy(&msg_tx[i], "&amp;", strlen("&amp;")); i += strlen("&amp;");}
		if (data[n] == 60) {memcpy(&msg_tx[i], "&lt;", strlen("&lt;")); i += strlen("&lt;");}
		if (data[n] == 62) {memcpy(&msg_tx[i], "&gt;", strlen("&gt;")); i += strlen("&gt;");}
		if ((cPrev != 10) && (cPrev != 13) && (data[n] == 13)) {memcpy(&msg_tx[i], "<br />", strlen("<br />")); i += strlen("<br />");}
		if ((cPrev != 10) && (cPrev != 13) && (data[n] == 10)) {memcpy(&msg_tx[i], "<br />", strlen("<br />")); i += strlen("<br />");}
		if (data[n] == 9) {memcpy(&msg_tx[i], "     ", strlen("     ")); i += strlen("     ");}
		cPrev = data[n];
	}
	strcat(msg_tx, "</p>");
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	WEB_pages_list(msg_tx, "manual", iSubPages, iCurPage, "");
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	return 1;
}

int WEB_log_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	char *msg_body = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/log/\"><h1>ЛОГ</h1></a>\r\n");
	
	memset(msg_body, 0, 65536);
	unsigned int dlen = load_last_dbg_data(msg_body, 65535);
	
	strcat(msg_tx, "<p>");
	int i =	strlen(msg_tx);
	char cPrev = 0;
	
	for (n = 0; n <= dlen; n++)
	{
		if ((msg_body[n] > 31) && (msg_body[n] != 38) && (msg_body[n] != 60) && (msg_body[n] != 62)) {msg_tx[i] = msg_body[n];i++;}
		if (msg_body[n] == 38) {memcpy(&msg_tx[i], "&amp;", strlen("&amp;")); i += strlen("&amp;");}
		if (msg_body[n] == 60) {memcpy(&msg_tx[i], "&lt;", strlen("&lt;")); i += strlen("&lt;");}
		if (msg_body[n] == 62) {memcpy(&msg_tx[i], "&gt;", strlen("&gt;")); i += strlen("&gt;");}
		if ((cPrev != 10) && (cPrev != 13) && (msg_body[n] == 13)) {memcpy(&msg_tx[i], "<br />", strlen("<br />")); i += strlen("<br />");}
		if ((cPrev != 10) && (cPrev != 13) && (msg_body[n] == 10)) {memcpy(&msg_tx[i], "<br />", strlen("<br />")); i += strlen("<br />");}
		if (msg_body[n] == 9) {memcpy(&msg_tx[i], "     ", strlen("     ")); i += strlen("     ");}
		cPrev = msg_body[n];
	}
	strcat(msg_tx, "</p>");
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_body);	
	return 1;
}

int WEB_user_save(int *pParams, char* strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&user_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iUserCnt) && iUserCnt)
	{
		uiUserList[pParams[0]].Enabled = pParams[1];
		uiUserList[pParams[0]].Access = pParams[2];
		uiUserList[pParams[0]].Level = pParams[3];
		memset(uiUserList[pParams[0]].Login, 0, 64);
		memset(uiUserList[pParams[0]].Password, 0, 64);
		if (strlen(&strParam[0]) > 63) memcpy(uiUserList[pParams[0]].Login, &strParam[0], 63);
			else memcpy(uiUserList[pParams[0]].Login, &strParam[0], strlen(&strParam[0]));
		if (strlen(&strParam[64]) > 63) memcpy(uiUserList[pParams[0]].Password, &strParam[64], 63);
			else memcpy(uiUserList[pParams[0]].Password, &strParam[64], strlen(&strParam[64]));
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&user_mutex);
	if (ret) SaveUsers();
	return 1;
}

int WEB_user_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&user_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iUserCnt) && iUserCnt)
	{
		USER_INFO *ui = (USER_INFO*)DBG_MALLOC(sizeof(USER_INFO)*(iUserCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iUserCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ui[clk], &uiUserList[i], sizeof(USER_INFO));
				clk++;
			}
		DBG_FREE(uiUserList);
		uiUserList = ui;
		iUserCnt--;	
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&user_mutex);
	if (ret) SaveUsers();
	return 1;
}

int WEB_user_add(int *pParams, char* strParam)
{
	DBG_MUTEX_LOCK(&user_mutex);
	iUserCnt++;
	uiUserList = (USER_INFO*)DBG_REALLOC(uiUserList, sizeof(USER_INFO)*iUserCnt);
	memset(&uiUserList[iUserCnt-1], 0, sizeof(USER_INFO));
	uiUserList[iUserCnt-1].Enabled = pParams[1];
	uiUserList[iUserCnt-1].Access = pParams[2];
	uiUserList[iUserCnt-1].Level = pParams[3];
	if (strlen(&strParam[0]) > 63) memcpy(uiUserList[iUserCnt-1].Login, &strParam[0], 63);
		else memcpy(uiUserList[iUserCnt-1].Login, &strParam[0], strlen(&strParam[0]));
	if (strlen(&strParam[64]) > 63) memcpy(uiUserList[iUserCnt-1].Password, &strParam[64], 63);
		else memcpy(uiUserList[iUserCnt-1].Password, &strParam[64], strlen(&strParam[64]));
	DBG_MUTEX_UNLOCK(&user_mutex);
	SaveUsers();
	return 1;
}

int WEB_users_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&user_mutex);
	TestUsers(1);
	DBG_MUTEX_UNLOCK(&user_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/users/\"><h1>ПРАВА ПОЛЬЗОВАТЕЛЕЙ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&user_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Включен</th><th>Уровень</th><th>Логин</th><th>Пароль</th><th>WEB</th><th>RTSP</th><th>ONVIF</th><th>Управление</th><th>YouTube</th><th>Медиа</th>"
					"<th>Пользователи</th><th>Меню</th><th>Модули (настр.)</th><th>Модули (сост.)</th><th>Будильники</th><th>Радио</th><th>Звуки</th><th>Почта</th>"
					"<th>Типы потоков</th><th>Потоки</th><th>Виджеты</th><th>Папки</th><th>События и действия</th><th>Ручные действия</th><th>Смарткарты</th>"
					"<th>Команды пультов</th><th>Квадраты камеры</th><th>Инструкция</th><th>Лог</th><th>Проводник</th><th>Настройки</th><th>Камера</th>"
					"<th>Микрофон</th><th>Система</th><th>Подключения</th><th>История подключений</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iUserCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurUserPage = iPage;
	iCurPage = iCurUserPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurUserPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurUserPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iUserCnt) iLastOption = iUserCnt;
	
	WEB_pages_list(msg_tx, "users", iSubPages, iCurPage, "");	
							
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, 
					"<tr><form action='/users/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='checkbox' name='En'%s></td>\r\n"
					"<td><input type='number' name='Lvl' value=%i style='width: 60px;'></td>\r\n"
					"<td><input type='text' name='Login' value='%s' maxlength=60 style='width: 150px;'></td>\r\n"
					"<td><input type='text' name='Pass' value='%s' maxlength=60 style='width: 150px;'></td>\r\n"
					"<td><input type='checkbox' name='Web'%s></td>\r\n"
					"<td><input type='checkbox' name='Rtsp'%s></td>\r\n"
					"<td><input type='checkbox' name='Onvif'%s></td>\r\n"
					"<td><input type='checkbox' name='Cntl'%s></td>\r\n"
					"<td><input type='checkbox' name='You'%s></td>\r\n"
					"<td><input type='checkbox' name='Med'%s></td>\r\n"
					"<td><input type='checkbox' name='User'%s></td>\r\n"
					"<td><input type='checkbox' name='Menu'%s></td>\r\n"
					"<td><input type='checkbox' name='Modl'%s></td>\r\n"
					"<td><input type='checkbox' name='ModSt'%s></td>\r\n"
					"<td><input type='checkbox' name='Alrm'%s></td>\r\n"
					"<td><input type='checkbox' name='Rads'%s></td>\r\n"
					"<td><input type='checkbox' name='Snds'%s></td>\r\n"
					"<td><input type='checkbox' name='Mail'%s></td>\r\n"
					"<td><input type='checkbox' name='StrTp'%s></td>\r\n"
					"<td><input type='checkbox' name='Strs'%s></td>\r\n"
					"<td><input type='checkbox' name='Wdgt'%s></td>\r\n"
					"<td><input type='checkbox' name='Dirs'%s></td>\r\n"
					"<td><input type='checkbox' name='EvntAct'%s></td>\r\n"
					"<td><input type='checkbox' name='MnlAct'%s></td>\r\n"
					"<td><input type='checkbox' name='Keys'%s></td>\r\n"
					"<td><input type='checkbox' name='Irs'%s></td>\r\n"
					"<td><input type='checkbox' name='Sqrs'%s></td>\r\n"
					"<td><input type='checkbox' name='Mnal'%s></td>\r\n"
					"<td><input type='checkbox' name='Logs'%s></td>\r\n"
					"<td><input type='checkbox' name='Explr'%s></td>\r\n"
					"<td><input type='checkbox' name='Setts'%s></td>\r\n"
					"<td><input type='checkbox' name='Camr'%s></td>\r\n"
					"<td><input type='checkbox' name='Mic'%s></td>\r\n"
					"<td><input type='checkbox' name='Sys'%s></td>\r\n"
					"<td><input type='checkbox' name='Conn'%s></td>\r\n"
					"<td><input type='checkbox' name='Hist'%s></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/users/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n,
					uiUserList[n].Enabled ? " checked" : "", 
					uiUserList[n].Level, uiUserList[n].Login, uiUserList[n].Password,
					uiUserList[n].Access & ACCESS_WEB ? " checked" : "",
					uiUserList[n].Access & ACCESS_RTSP ? " checked" : "",
					uiUserList[n].Access & ACCESS_ONVIF ? " checked" : "",
					uiUserList[n].Access & ACCESS_CONTROL ? " checked" : "",
					uiUserList[n].Access & ACCESS_YOUTUBE ? " checked" : "",
					uiUserList[n].Access & ACCESS_MEDIA ? " checked" : "",
					uiUserList[n].Access & ACCESS_USERS ? " checked" : "",
					uiUserList[n].Access & ACCESS_MENU ? " checked" : "",
					uiUserList[n].Access & ACCESS_MODULES ? " checked" : "",
					uiUserList[n].Access & ACCESS_MODSTATUSES ? " checked" : "",
					uiUserList[n].Access & ACCESS_ALARMS ? " checked" : "",
					uiUserList[n].Access & ACCESS_RADIOS ? " checked" : "",
					uiUserList[n].Access & ACCESS_SOUNDS ? " checked" : "",
					uiUserList[n].Access & ACCESS_MAILS ? " checked" : "",
					uiUserList[n].Access & ACCESS_STREAMTYPES ? " checked" : "",
					uiUserList[n].Access & ACCESS_STREAMS ? " checked" : "",
					uiUserList[n].Access & ACCESS_WIDGETS ? " checked" : "",
					uiUserList[n].Access & ACCESS_DIRECTORIES ? " checked" : "",
					uiUserList[n].Access & ACCESS_EVNTACTIONS ? " checked" : "",
					uiUserList[n].Access & ACCESS_MNLACTIONS ? " checked" : "",
					uiUserList[n].Access & ACCESS_KEYS ? " checked" : "",
					uiUserList[n].Access & ACCESS_IRCODES ? " checked" : "",
					uiUserList[n].Access & ACCESS_CAMRECTS ? " checked" : "",
					uiUserList[n].Access & ACCESS_MANUAL ? " checked" : "",
					uiUserList[n].Access & ACCESS_LOG ? " checked" : "",
					uiUserList[n].Access & ACCESS_EXPLORER ? " checked" : "",
					uiUserList[n].Access & ACCESS_SETTINGS ? " checked" : "",
					uiUserList[n].Access & ACCESS_CAMERA ? " checked" : "",
					uiUserList[n].Access & ACCESS_MIC ? " checked" : "",
					uiUserList[n].Access & ACCESS_SYSTEM ? " checked" : "",
					uiUserList[n].Access & ACCESS_CONNECTS ? " checked" : "",
					uiUserList[n].Access & ACCESS_HISTORY ? " checked" : "");		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&user_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/users/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='checkbox' name='En'></td>\r\n"
					"<td><input type='number' name='Lvl' value=0 style='width: 60px;'></td>\r\n"
					"<td><input type='text' name='Log' value='' maxlength=60 style='width: 150px;'></td>\r\n"
					"<td><input type='text' name='Pass' value='' maxlength=60 style='width: 150px;'></td>\r\n"
					"<td><input type='checkbox' name='Web'></td>\r\n"
					"<td><input type='checkbox' name='Rtsp'></td>\r\n"
					"<td><input type='checkbox' name='Onvif'></td>\r\n"
					"<td><input type='checkbox' name='Cntl'></td>\r\n"
					"<td><input type='checkbox' name='You'></td>\r\n"
					"<td><input type='checkbox' name='Med'></td>\r\n"
					"<td><input type='checkbox' name='User'></td>\r\n"
					"<td><input type='checkbox' name='Menu'></td>\r\n"
					"<td><input type='checkbox' name='Modl'></td>\r\n"
					"<td><input type='checkbox' name='ModSt'></td>\r\n"
					"<td><input type='checkbox' name='Alrm'></td>\r\n"
					"<td><input type='checkbox' name='Rads'></td>\r\n"
					"<td><input type='checkbox' name='Snds'></td>\r\n"
					"<td><input type='checkbox' name='Mail'></td>\r\n"
					"<td><input type='checkbox' name='StrTp'></td>\r\n"
					"<td><input type='checkbox' name='Strs'></td>\r\n"
					"<td><input type='checkbox' name='Wdgt'></td>\r\n"
					"<td><input type='checkbox' name='Dirs'></td>\r\n"
					"<td><input type='checkbox' name='EvntAct'></td>\r\n"
					"<td><input type='checkbox' name='MnlAct'></td>\r\n"
					"<td><input type='checkbox' name='Keys'></td>\r\n"
					"<td><input type='checkbox' name='Irs'></td>\r\n"
					"<td><input type='checkbox' name='Sqrs'></td>\r\n"
					"<td><input type='checkbox' name='Mnal'></td>\r\n"
					"<td><input type='checkbox' name='Logs'></td>\r\n"
					"<td><input type='checkbox' name='Explr'></td>\r\n"
					"<td><input type='checkbox' name='Setts'></td>\r\n"
					"<td><input type='checkbox' name='Camr'></td>\r\n"
					"<td><input type='checkbox' name='Mic'></td>\r\n"
					"<td><input type='checkbox' name='Sys'></td>\r\n"
					"<td><input type='checkbox' name='Conn'></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "users", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_radio_save(int *pParams, char *strParam, float *fParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iRadioStationsCnt) && iRadioStationsCnt)
	{
		riRadioStation[pParams[0]].Frequency = fParam[0];
		memset(riRadioStation[pParams[0]].Name, 0, 64);
		if (strlen(&strParam[0]) > 63) memcpy(riRadioStation[pParams[0]].Name, &strParam[0], 63);
		else memcpy(riRadioStation[pParams[0]].Name, &strParam[0], strlen(&strParam[0]));
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveRadios();
	return 1;
}

int WEB_radio_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iRadioStationsCnt) && iRadioStationsCnt)
	{
		RADIO_INFO *ri = (RADIO_INFO*)DBG_MALLOC(sizeof(RADIO_INFO)*(iRadioStationsCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iRadioStationsCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&ri[clk], &riRadioStation[i], sizeof(RADIO_INFO));
				clk++;
			}
		DBG_FREE(riRadioStation);
		riRadioStation = ri;
		iRadioStationsCnt--;	
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveRadios();
	return 1;
}

int WEB_radio_add(int *pParams, char *strParam, float *fParam)
{
	DBG_MUTEX_LOCK(&system_mutex);
	iRadioStationsCnt++;
	riRadioStation = (RADIO_INFO*)DBG_REALLOC(riRadioStation, sizeof(RADIO_INFO)*iRadioStationsCnt);
	memset(&riRadioStation[iRadioStationsCnt-1], 0, sizeof(RADIO_INFO));
	riRadioStation[iRadioStationsCnt-1].Frequency = fParam[0];
	if (strlen(&strParam[0]) > 63) memcpy(riRadioStation[iRadioStationsCnt-1].Name, &strParam[0], 63);
		else memcpy(riRadioStation[iRadioStationsCnt-1].Name, &strParam[0], strlen(&strParam[0]));
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveRadios();
	return 1;
}

int WEB_radios_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	
	DBG_MUTEX_LOCK(&system_mutex);
	TestRadios(1);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/radios/\"><h1>РАДИО</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&system_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Частота</th><th>Название</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iRadioStationsCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurRadioPage = iPage;
	iCurPage = iCurRadioPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurRadioPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurRadioPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iRadioStationsCnt) iLastOption = iRadioStationsCnt;
	
	WEB_pages_list(msg_tx, "radios", iSubPages, iCurPage, "");	
							
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, 
					"<tr><form action='/radios/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='number' name='Freq' min=65 max=108 step=0.1 value=%.1f style='width: 70px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=63 value='%s'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/radios/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n,
					riRadioStation[n].Frequency, 
					riRadioStation[n].Name);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/radios/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='number' name='Freq' min=65 max=108 step=0.1 value='108.00' style='width: 70px;'></td>\r\n"
					"<td><input type='text' name='Name' maxlength=63 value=''></td>\r\n"	
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "radios", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_alarm_save(int *pParams, char *strParam)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iAlarmClocksCnt) && iAlarmClocksCnt)
	{
		actAlarmClockInfo[pParams[0]].Enabled = pParams[1];
		actAlarmClockInfo[pParams[0]].Time = pParams[2];
		actAlarmClockInfo[pParams[0]].Days = (char)pParams[3];
		actAlarmClockInfo[pParams[0]].Skip = pParams[4];
		actAlarmClockInfo[pParams[0]].Limit = pParams[5];
		actAlarmClockInfo[pParams[0]].BeginVolume = pParams[6];
		actAlarmClockInfo[pParams[0]].TimeSetVolume = pParams[7];
		memset(actAlarmClockInfo[pParams[0]].Path, 0, MAX_FILE_LEN);
		strcpy(actAlarmClockInfo[pParams[0]].Path, strParam);
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveAlarms();
	return 1;
}

int WEB_alarm_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iAlarmClocksCnt) && iAlarmClocksCnt)
	{
		ALARM_CLOCK_TYPE *aci = (ALARM_CLOCK_TYPE*)DBG_MALLOC(sizeof(ALARM_CLOCK_TYPE)*(iAlarmClocksCnt-1));
		int i;
		int clk = 0;
		for (i = 0; i < iAlarmClocksCnt; i++)
			if (i != pParams[0])
			{
				memcpy(&aci[clk], &actAlarmClockInfo[i], sizeof(ALARM_CLOCK_TYPE));
				clk++;
			}
		DBG_FREE(actAlarmClockInfo);
		actAlarmClockInfo = aci;
		iAlarmClocksCnt--;	
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SaveAlarms();
	return 1;
}

int WEB_alarm_add(int *pParams, char *strParam)
{
	DBG_MUTEX_LOCK(&system_mutex);
	iAlarmClocksCnt++;
	actAlarmClockInfo = (ALARM_CLOCK_TYPE*)DBG_REALLOC(actAlarmClockInfo, sizeof(ALARM_CLOCK_TYPE)*iAlarmClocksCnt);
	memset(&actAlarmClockInfo[iAlarmClocksCnt-1], 0, sizeof(ALARM_CLOCK_TYPE));
	actAlarmClockInfo[iAlarmClocksCnt-1].Enabled = pParams[1];
	actAlarmClockInfo[iAlarmClocksCnt-1].Time = pParams[2];
	actAlarmClockInfo[iAlarmClocksCnt-1].Days = (char)pParams[3];	
	actAlarmClockInfo[iAlarmClocksCnt-1].Skip = pParams[4];
	actAlarmClockInfo[iAlarmClocksCnt-1].Limit = pParams[5];
	actAlarmClockInfo[iAlarmClocksCnt-1].BeginVolume = pParams[6];
	actAlarmClockInfo[iAlarmClocksCnt-1].TimeSetVolume = pParams[7];
	strcpy(actAlarmClockInfo[iAlarmClocksCnt-1].Path, strParam);
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveAlarms();
	return 1;
}

int WEB_alarms_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n;	
	int fl1, fl2, fl3;
	
	DBG_MUTEX_LOCK(&system_mutex);
	TestAlarms(1);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/alarms/\"><h1>БУДИЛЬНИКИ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&system_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Включен</th><th>Отключить через</th><th>Пропустить</th><th>Начальная громкость</th><th>Время установки громкости</th><th>Понедельник</th><th>Вторник</th><th>Среда</th><th>Четверг</th><th>Пятница</th><th>Суббота</th>"
					"<th>Воскресенье</th><th>Время</th><th>Путь</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iAlarmClocksCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurAlarmPage = iPage;
	iCurPage = iCurAlarmPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurAlarmPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurAlarmPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iAlarmClocksCnt) iLastOption = iAlarmClocksCnt;
	
	WEB_pages_list(msg_tx, "alarms", iSubPages, iCurPage, "");	
					
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		fl1 = (int)floor((float)actAlarmClockInfo[n].Time/10000);
		fl2 = (int)floor((float)actAlarmClockInfo[n].Time/100);
		fl2 = fl2 - (fl1*100);
		fl3 = actAlarmClockInfo[n].Time - ((fl1*10000) + (fl2*100));
		sprintf(msg_subbody, 
					"<tr><form action='/alarms/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='checkbox' name='En'%s></td>\r\n"
					"<td><input type='number' name='Limit' value=%i style='width: 70px;'></td>\r\n"
					"<td><input type='number' name='Skip' value=%i style='width: 70px;'></td>\r\n"
					"<td><input type='number' name='Vol' value=%i style='width: 70px;'></td>\r\n"
					"<td><input type='number' name='Time' value=%i style='width: 70px;'></td>\r\n"
					"<td><input type='checkbox' name='Mo'%s></td>\r\n"
					"<td><input type='checkbox' name='Tu'%s></td>\r\n"
					"<td><input type='checkbox' name='We'%s></td>\r\n"
					"<td><input type='checkbox' name='Th'%s></td>\r\n"
					"<td><input type='checkbox' name='Fr'%s></td>\r\n"
					"<td><input type='checkbox' name='Sa'%s></td>\r\n"
					"<td><input type='checkbox' name='Su'%s></td>\r\n"
					"<td><input type='number' name='Hour' min = 0 max=24 value='%i'>\r\n"
					"<input type='number' name='Min' min = 0 max=59 value='%i'>\r\n"
					"<input type='number' name='Sec' min = 0 max=59 value='%i'></td>\r\n"	
					"<td><input type='text' name='Path' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
					"<td><button type='submit'>Сохранить</button>\r\n"
					"<button type='submit' formaction='/alarms/delete'>Удалить</button></td></form></tr>\r\n",
					session->request_id,
					n,n,
					actAlarmClockInfo[n].Enabled ? " checked" : "", 
					actAlarmClockInfo[n].Limit,
					actAlarmClockInfo[n].Skip,
					actAlarmClockInfo[n].BeginVolume,
					actAlarmClockInfo[n].TimeSetVolume,
					actAlarmClockInfo[n].Days & 1 ? " checked" : "",
					actAlarmClockInfo[n].Days & 2 ? " checked" : "",
					actAlarmClockInfo[n].Days & 4 ? " checked" : "",
					actAlarmClockInfo[n].Days & 8 ? " checked" : "",
					actAlarmClockInfo[n].Days & 16 ? " checked" : "",
					actAlarmClockInfo[n].Days & 32 ? " checked" : "",
					actAlarmClockInfo[n].Days & 64 ? " checked" : "",
					fl1, fl2, fl3, actAlarmClockInfo[n].Path);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
					"<tr><form action='/alarms/add'><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><input type='checkbox' name='En'></td>\r\n"
					"<td><input type='number' name='Limit' value=0 style='width: 70px;'></td>\r\n"
					"<td><input type='number' name='Skip' value=0 style='width: 70px;'></td>\r\n"
					"<td><input type='number' name='Vol' value=0 style='width: 70px;'></td>\r\n"
					"<td><input type='number' name='Time' value=0 style='width: 70px;'></td>\r\n"
					"<td><input type='checkbox' name='Mo'></td>\r\n"
					"<td><input type='checkbox' name='Tu'></td>\r\n"
					"<td><input type='checkbox' name='We'></td>\r\n"
					"<td><input type='checkbox' name='Th'></td>\r\n"
					"<td><input type='checkbox' name='Fr'></td>\r\n"
					"<td><input type='checkbox' name='Sa'></td>\r\n"
					"<td><input type='checkbox' name='Su'></td>\r\n"
					"<td><input type='number' name='Hour' min = 0 max=24 value='0'>\r\n"
					"<input type='number' name='Min' min = 0 max=59 value='0'>\r\n"
					"<input type='number' name='Sec' min = 0 max=59 value='0'></td>\r\n"
					"<input type='hidden' name='Page' value=%i>\r\n"
					"<td><input type='text' name='Path' maxlength=255 value='' style='width: 300px;'></td>\r\n"
					"<td><button type='submit'>Добавить</button></td></form></tr>\r\n", 
					session->request_id,
					iSubPages - 1);		
	strcat(msg_tx, msg_subbody);
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "alarms", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_history_del(int *pParams)
{
	WEB_ClearWebHistory();
	return 1;
}

int WEB_history_respond(char *msg_tx, WEB_SESSION *session, int iPage)
{
	int n;	
	
	char *msg_subbody = (char*)DBG_MALLOC(65536);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/history/\"><h1>История подключений</h1></a>\r\n");
	
	memset(msg_subbody, 0, 65536);
	sprintf(msg_subbody,
					"<input type='button' value='Очистить' onclick=\"window.location.href='/history/delete?req_index=%i'\" style='width: 200px;'><br />\r\n", session->request_id);		
	strcat(msg_tx, msg_subbody);	
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Протокол</th><th>Дата</th><th>Адрес</th><th>Авторизация</th><th>Логин</th><th>Тип</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)iWebHistoryCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurHistoryPage = iPage;
	iCurPage = iCurHistoryPage;
	
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurHistoryPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurHistoryPage = iCurPage;
	}
	
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= iWebHistoryCnt) iLastOption = iWebHistoryCnt;
	
	WEB_pages_list(msg_tx, "history", iSubPages, iCurPage, "");	
		
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 65536);
		sprintf(msg_subbody, 
					"<tr>\r\n"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='Prot' value='%s' style='width: 140px;'></td>\r\n"
					"<td><input type='text' name='Date' value='%s' style='width: 140px;'></td>\r\n"
					"<td><input type='text' name='Addr' value='%s' style='width: 100px;'></td>\r\n"
					"<td><input type='text' name='Auth' value='%i' style='width: 100px;'></td>\r\n"
					"<td><input type='text' name='Log' value='%s' style='width: 100px;'></td>\r\n"
					"<td><input type='text' name='Type' value='%s' style='width: 100px;'></td>\r\n"
					"<td><input type='text' name='Act' value='%s' style='width: 100px;'></td>\r\n"
					"</tr>\r\n",
					n, whWebHistory[n].Prot ? "WEB" : "RTSP",
					whWebHistory[n].Date, whWebHistory[n].Addr, whWebHistory[n].Auth, whWebHistory[n].Login, 
					WEB_GetWebMsgTypeName(whWebHistory[n].Type), WEB_GetWebActTypeName(whWebHistory[n].Action));		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "history", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_connect_del(int *pParams)
{
	if (pParams[1] == 0)
	{
		DBG_MUTEX_LOCK(&Network_Mutex);
		if ((pParams[0] > 0) && (pParams[0] < Connects_Count))
		{		
			DBG_MUTEX_LOCK(&Connects_Info[pParams[0]].Socket_Mutex);
			if (Connects_Info[pParams[0]].Status == CONNECT_STATUS_ONLINE) SendSignalType(pParams[0], SIGNAL_CLOSE);
			DBG_MUTEX_UNLOCK(&Connects_Info[pParams[0]].Socket_Mutex);		
		}
		DBG_MUTEX_UNLOCK(&Network_Mutex);
	}
	if (pParams[1] == 1)
	{
		DBG_MUTEX_LOCK(&RTP_session_mutex);
		if ((pParams[0] >= 0) && (pParams[0] < RTP_session_cnt))
		{		
			if ((RTP_session[pParams[0]].status != 0) && (RTP_session[pParams[0]].rtsp_session->socketpair[0] != 0))
			{
				char cType = SIGNAL_CLOSE;
				int ret;
				ret = write(RTP_session[pParams[0]].rtsp_session->socketpair[0], &cType, 1);
				if (ret < 1) dbgprintf(4, "WEB_connect_del: write socketpair signal %i (errno:%i, %s)\n", pParams[0], errno, strerror(errno));
			}
		}
		DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	}
	return 1;
}

int WEB_connects_respond(char *msg_tx, WEB_SESSION *session, int iPage, int iPage2)
{
	int n;	
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/connects/\"><h1>ПОДКЛЮЧЕНИЯ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&Network_Mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Время подключения</th><th>Протокол</th><th>Адрес</th><th>Тип</th><th>Содержимое</th><th>Передано</th><th>Принято</th><th>Время до закрытия</th><th>Состояние</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	iSubPages = (int)ceil((double)Connects_Max_Active / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurConnectPage = iPage;
	iCurPage = iCurConnectPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurConnectPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurConnectPage = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= Connects_Max_Active) iLastOption = Connects_Max_Active;
	
	WEB_pages_list(msg_tx, "connects", iSubPages, iCurPage, "");	
	char *cTraffType;
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
		switch(Connects_Info[n].TraffType)
		{
			case TRAFFIC_FULL_VIDEO:
				cTraffType = "Полное видео";
				break;
			case TRAFFIC_PREV_VIDEO:
				cTraffType = "Малое видео";
				break;
			case TRAFFIC_AUDIO:
				cTraffType = "Аудио";
				break;
			case TRAFFIC_SERVICE:
				cTraffType = "Служебный";
				break;
			case TRAFFIC_REMOTE_FILE:
				cTraffType = "Просмотр файлов";
				break;
			case TRAFFIC_REMOTE_VIDEO:
				cTraffType = "Видео данные";
				break;
			case TRAFFIC_REMOTE_AUDIO:
				cTraffType = "Аудио данные";
				break;
			case TRAFFIC_TRANSFER_FILE:
				cTraffType = "Передача файла";
				break;
			default:
				cTraffType = "Неизвестно";
				break;	
		}
		sprintf(msg_subbody,
					"<tr><form action='/connects/delete'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>\r\n"
					"<input type='hidden' name='Code' value=0>\r\n"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='Date' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Prot' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Addr' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Type' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Traff' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='number' name='Send' value=%i style='width: 150px;' disabled></td>\r\n"
					"<td><input type='number' name='Recv' value=%i style='width: 150px;' disabled></td>\r\n"
					"<td><input type='number' name='Time' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='Status' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td>%s</td></form></tr>\r\n",
					session->request_id,
					n, n, Connects_Info[n].DateConnect,
					n == 0 ? "UDP" : "TCP", 
					inet_ntoa(Connects_Info[n].Addr.sin_addr),
					(Connects_Info[n].Type == CONNECT_CLIENT) ? "Исходящее" : "Входящее",
					cTraffType,
					Connects_Info[n].SendedBytes, Connects_Info[n].RecvedBytes, 
					CONNECT_TIMEOUT - Connects_Info[n].Timer,
					(Connects_Info[n].Status == CONNECT_STATUS_ONLINE) ? "Подключено" : "Отключено",
					(Connects_Info[n].Status == CONNECT_STATUS_ONLINE) ? "<button type='submit'>Закрыть</button>" : "");		
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
	}	
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "connects", iSubPages, iCurPage, "");	
	
	///////////////////RTSP///////////////////
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Время подключения</th><th>Протокол</th><th>Адрес</th><th>Тип</th><th>Содержимое</th><th>Передано</th><th>Принято</th><th>Состояние</th><th>Действие</th></tr>");

	iSubPages = (int)ceil((double)RTP_session_cnt / WEB_PAGE_MAX_LEN);
	if (iPage2 != -1) iCurConnectPage2 = iPage2;
	iCurPage = iCurConnectPage2;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurConnectPage2 = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurConnectPage2 = iCurPage;
	}
	iFirstOption = iCurPage * WEB_PAGE_MAX_LEN;
	iLastOption = iFirstOption + WEB_PAGE_MAX_LEN;
	if (iLastOption >= RTP_session_cnt) iLastOption = RTP_session_cnt;
	
	WEB_pages_list(msg_tx, "connects", iSubPages, iCurPage, "");	
	
	for (n = iFirstOption; n < iLastOption; n++)
	{
		memset(msg_subbody, 0, 3072);
		switch(RTP_session[n].type)
		{
			case 0:
				cTraffType = "Видео";
				break;
			case 1:
				cTraffType = "Аудио";
				break;
			default:
				cTraffType = "Неизвестно";
				break;	
		}
		sprintf(msg_subbody,
					"<tr><form action='/connects/delete'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>\r\n"
					"<input type='hidden' name='Code' value=1>\r\n"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='text' name='Date' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Prot' value='RTP' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Addr' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Type' value='Входящее' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='Traff' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='number' name='Send' value=%i style='width: 150px;' disabled></td>\r\n"
					"<td><input type='number' name='Recv' value=%i style='width: 150px;' disabled></td>\r\n"
					"<td><input type='text' name='Status' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td>%s</td></form></tr>\r\n",
					session->request_id,
					n, n, RTP_session[n].DateConnect,
					RTP_session[n].rtsp_session ? RTP_session[n].rtsp_session->ip_str : "",
					cTraffType,
					RTP_session[n].sended_bytes, RTP_session[n].recved_bytes,
					(RTP_session[n].status != 0) ? "Подключено" : "Отключено",
					(RTP_session[n].status != 0) ? "<button type='submit'>Закрыть</button>" : "");		
		strcat(msg_tx, msg_subbody);
		if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;	
	}	
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "connects", iSubPages, iCurPage, "");	
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_images_respond(char *msg_tx, WEB_SESSION *session, int msg_act)
{
	int ret = 0;
	char cPath[512];
	memset(cPath, 0, 512);
	DBG_MUTEX_LOCK(&system_mutex);	
	if (msg_act == WEB_ACT_IMAGE_CAMRECT) strcpy(cPath, cCameraRectFile);
	if (msg_act == WEB_ACT_IMAGE_CAMPREV) strcpy(cPath, cCameraSensorFile);
	if (msg_act == WEB_ACT_IMAGE_CAMMAIN) strcpy(cPath, cCameraShotFile);
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	if (strlen(cPath))
	{
		misc_buffer mbuffer;
		memset(&mbuffer, 0, sizeof(misc_buffer));
		if (WEB_load_limit_file_in_buff(cPath, &mbuffer, WEB_TX_BUF_SIZE_MAX - 1024) == 1)
		{			
			sprintf(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: image/jpeg\r\n"
					"Content-Length: %i\r\n"
					"\r\n", mbuffer.data_size);
			ret = strlen(msg_tx) + mbuffer.data_size;
			memcpy(&msg_tx[strlen(msg_tx)], mbuffer.void_data, mbuffer.data_size);
			DBG_FREE(mbuffer.void_data);
		} else dbgprintf(3, "No file or file big %s\n", cPath);
	} else dbgprintf(2, "Unknown file type request (action:%i)\n", msg_act);
	
	return ret;
}

int WEB_module_change(int *pParams, char *strValue)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt
		&& (miModuleList[pParams[0]].Local == 1) && (miModuleList[pParams[0]].Type == pParams[2]))
	{
		switch(miModuleList[pParams[0]].Type)
		{
			case MODULE_TYPE_RTC:
				{
					struct tm timeinfo;
					memset(&timeinfo, 0, sizeof(timeinfo));
					timeinfo.tm_mday = pParams[13];
					timeinfo.tm_mon = pParams[14] - 1;
					timeinfo.tm_year = pParams[15] - 1900;
					timeinfo.tm_hour = pParams[16];
					timeinfo.tm_min = pParams[17];
					timeinfo.tm_sec = pParams[18];
					i2c_write_spec_timedate3231(I2C_ADDRESS_CLOCK, &timeinfo);
				}
				break;
			default:
				break;
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	return 1;
}

int WEB_module_save(int *pParams, char *strValue)
{
	int iEnabled = 0;
	int ret = 0;
	unsigned int uiTemp;
	unsigned int uiParentID = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt
		&& (miModuleList[pParams[0]].Local == 1) && (miModuleList[pParams[0]].Type == pParams[2]))
	{
		iEnabled = miModuleList[pParams[0]].Enabled & 1;
		uiParentID = miModuleList[pParams[0]].ParentID;
		if (pParams[1] && ((miModuleList[pParams[0]].Enabled & 2) == 0)) miModuleList[pParams[0]].Enabled |= 2;
		if ((pParams[1] == 0) && (miModuleList[pParams[0]].Enabled & 2)) miModuleList[pParams[0]].Enabled ^= 2;
		//miModuleList[pParams[0]].Type = pParams[2];
		miModuleList[pParams[0]].NewID = pParams[3];
		memset(miModuleList[pParams[0]].Name, 0, 64);
		if (strlen(strValue) > 63) 
			memcpy(miModuleList[pParams[0]].Name, strValue, 63); 
			else
			strcpy(miModuleList[pParams[0]].Name, strValue);
		miModuleList[pParams[0]].ViewLevel = pParams[4];
		miModuleList[pParams[0]].AccessLevel = pParams[5];
		//miModuleList[pParams[0]].Global = pParams[6];
		miModuleList[pParams[0]].ScanSet = pParams[7] / 50;
		//miModuleList[pParams[0]].SaveChanges = pParams[8];
		if ((miModuleList[pParams[0]].Type != MODULE_TYPE_SYSTEM)
			&&
			(miModuleList[pParams[0]].Type != MODULE_TYPE_USB_GPIO))
		{
			miModuleList[pParams[0]].Settings[0] = pParams[10];
			miModuleList[pParams[0]].Settings[1] = pParams[11];
			miModuleList[pParams[0]].Settings[2] = pParams[12];
			miModuleList[pParams[0]].Settings[3] = pParams[13];
			miModuleList[pParams[0]].Settings[4] = pParams[14];
			miModuleList[pParams[0]].Settings[5] = pParams[15];
			miModuleList[pParams[0]].Settings[6] = pParams[16];
			miModuleList[pParams[0]].Settings[7] = pParams[17];
			miModuleList[pParams[0]].Settings[8] = pParams[18];
			miModuleList[pParams[0]].Settings[9] = pParams[19];
			miModuleList[pParams[0]].Settings[10] = pParams[20];
			miModuleList[pParams[0]].Settings[11] = pParams[21];
			miModuleList[pParams[0]].Settings[12] = pParams[22];
			miModuleList[pParams[0]].Settings[13] = pParams[23];
			miModuleList[pParams[0]].Settings[14] = pParams[24];
			miModuleList[pParams[0]].Settings[15] = pParams[25];
			miModuleList[pParams[0]].Settings[16] = pParams[26];
			miModuleList[pParams[0]].Settings[17] = pParams[27];
			miModuleList[pParams[0]].Settings[18] = pParams[28];
			miModuleList[pParams[0]].Settings[19] = pParams[29];
			miModuleList[pParams[0]].Settings[20] = pParams[30];
			miModuleList[pParams[0]].Settings[21] = pParams[31];
			miModuleList[pParams[0]].Settings[22] = pParams[32];
			miModuleList[pParams[0]].Settings[23] = pParams[33];
			miModuleList[pParams[0]].Settings[24] = pParams[34];
			miModuleList[pParams[0]].Settings[25] = pParams[35];
			miModuleList[pParams[0]].Settings[26] = pParams[36];
			miModuleList[pParams[0]].Settings[27] = pParams[37];
			miModuleList[pParams[0]].Settings[28] = pParams[38];
			miModuleList[pParams[0]].Settings[29] = pParams[39];
			miModuleList[pParams[0]].Settings[30] = pParams[40];
			miModuleList[pParams[0]].Settings[31] = pParams[41];
			miModuleList[pParams[0]].Settings[32] = pParams[42];
			miModuleList[pParams[0]].Settings[33] = pParams[43];
			miModuleList[pParams[0]].Settings[34] = pParams[44];
			miModuleList[pParams[0]].Settings[35] = pParams[45];
			miModuleList[pParams[0]].Settings[36] = pParams[46];
			miModuleList[pParams[0]].Settings[37] = pParams[47];
		}
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_CAMERA)
		{
			int iW, iH;
			iW = (miModuleList[pParams[0]].Settings[1] >> 16) & 0xFFFF;
			iH = miModuleList[pParams[0]].Settings[1] & 0xFFFF;
			iW = (iW+31)&~31;
			iH = (iH+15)&~15;
			miModuleList[pParams[0]].Settings[1] = (iW & 0xFFFF) << 16;
			miModuleList[pParams[0]].Settings[1] |= iH & 0xFFFF;
			
			iW = (miModuleList[pParams[0]].Settings[5] >> 16) & 0xFFFF;
			iH = miModuleList[pParams[0]].Settings[5] & 0xFFFF;
			iW = (iW+31)&~31;
			iH = (iH+15)&~15;
			miModuleList[pParams[0]].Settings[5] = (iW & 0xFFFF) << 16;
			miModuleList[pParams[0]].Settings[5] |= iH & 0xFFFF;
			
			iW = (miModuleList[pParams[0]].Settings[37] >> 16) & 0xFFFF;
			iH = miModuleList[pParams[0]].Settings[37] & 0xFFFF;
			iW = (iW+31)&~31;
			iH = (iH+15)&~15;
			miModuleList[pParams[0]].Settings[37] = (iW & 0xFFFF) << 16;
			miModuleList[pParams[0]].Settings[37] |= iH & 0xFFFF;
						
			miModuleList[pParams[0]].Settings[10] |= (pParams[48] & 0xFF) << 24;
			miModuleList[pParams[0]].Settings[10] |= (pParams[49] & 0xFF) << 16;
			miModuleList[pParams[0]].Settings[11] |= (pParams[50] & 0xFF) << 24;
			miModuleList[pParams[0]].Settings[11] |= (pParams[51] & 0xFF) << 16;
			miModuleList[pParams[0]].Settings[38] = pParams[52];
			miModuleList[pParams[0]].Settings[39] = pParams[53];
			miModuleList[pParams[0]].Settings[42] = pParams[59];
			omx_get_image_sensor_module_params(&ispCameraImageSettings, &miModuleList[pParams[0]]);
			
			ispCameraImageSettings.ISOControl = pParams[55]; 
			ispCameraImageSettings.HardAutoISOControl = (pParams[54] & 1) ? 1 : 0;
			ispCameraImageSettings.SoftAutoISOControl = (pParams[54] & 2) ? 1 : 0;
			ispCameraImageSettings.DestAutoISOControl = pParams[56]; 
			ispCameraImageSettings.EVControl = pParams[57]; 
			//ispCameraImageSettings.HardAutoEVControl;
			ispCameraImageSettings.SoftAutoEVControl = (pParams[54] & 4) ? 1 : 0;
			ispCameraImageSettings.DestAutoEVControl = pParams[58]; 
			omx_set_image_sensor_module_params(&ispCameraImageSettings, &miModuleList[pParams[0]]);
			
			ispCameraImageSettings.Updated = 1;
		}
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_USB_GPIO)
		{
			miModuleList[pParams[0]].Settings[0] = pParams[10];
			miModuleList[pParams[0]].Settings[1] = pParams[11];

			int iPortsCnt = (pParams[10] >> 8) & 0xFF;
			if (miModuleList[pParams[0]].SettingsCount != iPortsCnt)
			{
				if ((iPortsCnt > 0) && (iPortsCnt <= miModuleList[pParams[0]].ParamsCount))
				{
					void* tt = DBG_MALLOC(iPortsCnt * sizeof(Sensor_Params));
					memset(tt, 0, iPortsCnt * sizeof(Sensor_Params));
					if (iPortsCnt > miModuleList[pParams[0]].SettingsCount)
						memcpy(tt, miModuleList[pParams[0]].SettingList, miModuleList[pParams[0]].SettingsCount * sizeof(Sensor_Params));
						else
						memcpy(tt, miModuleList[pParams[0]].SettingList, iPortsCnt * sizeof(Sensor_Params));
					if (miModuleList[pParams[0]].SettingsCount) DBG_FREE(miModuleList[pParams[0]].SettingList);
					miModuleList[pParams[0]].SettingList = tt;
					miModuleList[pParams[0]].SettingsCount = iPortsCnt;					
				}				
			}
			
			if (miModuleList[pParams[0]].SettingSelected >= miModuleList[pParams[0]].SettingsCount)
					miModuleList[pParams[0]].SettingSelected = 0;
			if (pParams[12] != (miModuleList[pParams[0]].SettingSelected + 1)) 
			{
				if ((pParams[12] > 0) && (pParams[12] <= iPortsCnt))
					miModuleList[pParams[0]].SettingSelected = pParams[12] - 1;
					else
					miModuleList[pParams[0]].SettingSelected = 0;	
			}
			else
			{
				if ((miModuleList[pParams[0]].SettingSelected < miModuleList[pParams[0]].SettingsCount) 
					&& (pParams[13] >= 0) && (pParams[13] < miModuleList[pParams[0]].ParamsCount))
				{
					int iSelected = miModuleList[pParams[0]].SettingSelected;
					int iID = miModuleList[pParams[0]].SettingList[iSelected].ID;
					
					if (pParams[13] != iID)
					{						
						memset(&miModuleList[pParams[0]].SettingList[iSelected], 0, sizeof(Sensor_Params));					
						miModuleList[pParams[0]].SettingList[iSelected].ID = pParams[13] & 0b00111111;						
					}
					else
					{
						uiTemp = miModuleList[pParams[0]].SettingList[iSelected].CurrentType;
						memset(&miModuleList[pParams[0]].SettingList[iSelected], 0, sizeof(Sensor_Params));
					
						miModuleList[pParams[0]].SettingList[iSelected].CurrentType = pParams[17] & 0xFF;
						miModuleList[pParams[0]].SettingList[iSelected].ID = pParams[13] & 0b00111111;
						miModuleList[pParams[0]].SettingList[iSelected].Enabled = pParams[14] & 1;
						miModuleList[pParams[0]].SettingList[iSelected].PwrInit = pParams[15] & 1;
						miModuleList[pParams[0]].SettingList[iSelected].Interval = (pParams[16] / 50) & 0xFFFF;
							
						if (uiTemp == miModuleList[pParams[0]].SettingList[iSelected].CurrentType)
						{
							switch(miModuleList[pParams[0]].SettingList[iSelected].CurrentType)
							{
									case USBIO_PIN_SETT_TYPE_INPUT:								
										miModuleList[pParams[0]].SettingList[iSelected].Pull = pParams[18] & 0b11;
										miModuleList[pParams[0]].SettingList[iSelected].Inverted = pParams[19] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].Analog = pParams[20] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].Accuracy = pParams[21] & 0b1111;
										if (miModuleList[pParams[0]].SettingList[iSelected].Analog)
										{
											miModuleList[pParams[0]].SettingList[iSelected].Pull = 0;
											if (miModuleList[pParams[0]].SettingList[iSelected].Accuracy > 12)
												miModuleList[pParams[0]].SettingList[iSelected].Accuracy = 0;
										}
										else
										{
											if (miModuleList[pParams[0]].SettingList[iSelected].Pull == 3)
												miModuleList[pParams[0]].SettingList[iSelected].Pull = 0;										
										}
										break;
									case USBIO_PIN_SETT_TYPE_OUTPUT:
										miModuleList[pParams[0]].SettingList[iSelected].Pull = pParams[18] & 0b11;
										miModuleList[pParams[0]].SettingList[iSelected].Inverted = pParams[19] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].ImpulseLen = (pParams[20] / 50) & 0xFF;
										miModuleList[pParams[0]].SettingList[iSelected].DefaultValue = pParams[21] & 0xFF;
										miModuleList[pParams[0]].SettingList[iSelected].OpenDrain = pParams[22] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].PWM = pParams[23] & 1;
										if (miModuleList[pParams[0]].SettingList[iSelected].Pull == 3)
											miModuleList[pParams[0]].SettingList[iSelected].Pull = 0;
										miModuleList[pParams[0]].SettingList[iSelected].Mode = pParams[24] & 0b11;
										miModuleList[pParams[0]].SettingList[iSelected].PortSpeedCode = (pParams[25] / 200) & 0xFF;											
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MISC:
										miModuleList[pParams[0]].SettingList[iSelected].Address = pParams[18] & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].ResultType = pParams[19] & 0b11111;
										miModuleList[pParams[0]].SettingList[iSelected].PortSpeedCode = pParams[20] & 0b111111;
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AM2320:
										miModuleList[pParams[0]].SettingList[iSelected].ResultType = pParams[18] & 1;
										break;
									case USBIO_PIN_SETT_TYPE_I2C_LM75:
										miModuleList[pParams[0]].SettingList[iSelected].Address = pParams[18] & 0b111;
										break;
									case USBIO_PIN_SETT_TYPE_I2C_ADS1015:
										miModuleList[pParams[0]].SettingList[iSelected].Address = pParams[18] & 0b11;
										miModuleList[pParams[0]].SettingList[iSelected].Gain = pParams[19] & 0b111;
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MCP3421:
										miModuleList[pParams[0]].SettingList[iSelected].Address = pParams[18] & 0b111;
										miModuleList[pParams[0]].SettingList[iSelected].Accuracy = pParams[19] & 0b11;
										miModuleList[pParams[0]].SettingList[iSelected].Gain = pParams[20] & 0b11;
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AS5600:
										miModuleList[pParams[0]].SettingList[iSelected].Address = pParams[18] & 0b111;
										miModuleList[pParams[0]].SettingList[iSelected].ResultType = pParams[19] & 0b1;
										break;
									case USBIO_PIN_SETT_TYPE_I2C_HMC5883L:
										miModuleList[pParams[0]].SettingList[iSelected].ResultType = pParams[18] & 0b111;
										break;
									case USBIO_PIN_SETT_TYPE_IR_RECIEVER:
										miModuleList[pParams[0]].SettingList[iSelected].TimeSkip = (pParams[18] / 50) & 0xFF;
										break;
									case USBIO_PIN_SETT_TYPE_RS485_MISC:
										miModuleList[pParams[0]].SettingList[iSelected].Address = pParams[18] & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].ResultType = pParams[19] & 0b11111;										
										miModuleList[pParams[0]].SettingList[iSelected].PortSpeedCode = pParams[20] & 0b111111;
										miModuleList[pParams[0]].SettingList[iSelected].IOWControlUse = pParams[21] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].IOWControlPort = (pParams[22] - 1) & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].ImpulseLen = (pParams[23] / 100) & 0b011111;
										miModuleList[pParams[0]].SettingList[iSelected].IOModeControl = pParams[24] & 1;
										break;	
									case USBIO_PIN_SETT_TYPE_RS485:
										miModuleList[pParams[0]].SettingList[iSelected].PortSpeedCode = pParams[18] & 0b111111;
										miModuleList[pParams[0]].SettingList[iSelected].IOWControlUse = pParams[19] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].IORControlUse = pParams[20] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].IOWControlPort = (pParams[21] - 1) & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].IORControlPort = (pParams[22] - 1) & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].ImpulseLen = (pParams[23] / 100) & 0b01111111;
										miModuleList[pParams[0]].SettingList[iSelected].IOModeControl = pParams[25] & 1;
										break;								
									case USBIO_PIN_SETT_TYPE_RS485_PN532:
									case USBIO_PIN_SETT_TYPE_RS485_RC522:
										miModuleList[pParams[0]].SettingList[iSelected].IOWControlUse = pParams[19] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].IORControlUse = pParams[20] & 1;
										miModuleList[pParams[0]].SettingList[iSelected].IOWControlPort = (pParams[21] - 1) & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].IORControlPort = (pParams[22] - 1) & 0b1111111;
										miModuleList[pParams[0]].SettingList[iSelected].ImpulseLen = (pParams[23]/100) & 0b01111111;
										miModuleList[pParams[0]].SettingList[iSelected].TimeSkip = pParams[24] & 0b11;
										miModuleList[pParams[0]].SettingList[iSelected].IOModeControl = pParams[25] & 1;
										break;								
									default:
										break;
							}
						}
					}
				} else dbgprintf(2, "USB IO modules no settings area %i<%i %i>%i\n", 
									miModuleList[pParams[0]].SettingSelected,
									miModuleList[pParams[0]].SettingsCount,
									miModuleList[pParams[0]].ParamsCount,
									pParams[13]);
			}
			usb_gpio_convert_settings(miModuleList[pParams[0]].SettingList, miModuleList[pParams[0]].SettingsCount, &miModuleList[pParams[0]].Settings[2], 1);
			if (iEnabled) 
			{
				int iStat = 0;
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				DBG_MUTEX_LOCK(&system_mutex);	
				if (cThreadScanerStatus)
				{
					if (cThreadScanerStatusUsbio == 0) 
					{
						cThreadScanerStatusUsbio = 1;
						iStat = 1;
					}
				} else iStat = 1;
				DBG_MUTEX_UNLOCK(&system_mutex);
				if (iStat)
				{
					int cnt = 100;
					do
					{
						DBG_MUTEX_LOCK(&system_mutex);				
						iStat = cThreadScanerStatusUsbio;
						if (cThreadScanerStatus == 0) iStat = 2;
						DBG_MUTEX_UNLOCK(&system_mutex);
						if (iStat == 2) break;
						usleep(10000);
						cnt--;
					} while(cnt);
					//printf("ss %i %i\n", iStat, cnt);
					if (cnt)
					{	
						DBG_MUTEX_LOCK(&modulelist_mutex);
						usb_gpio_save_settings(&miModuleList[pParams[0]], miModuleList[pParams[0]].Settings, MAX_MODULE_SETTINGS);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
					}
					else dbgprintf(2, "Error stoping scan usbio, timeout\n");
					DBG_MUTEX_LOCK(&system_mutex);	
					cThreadScanerStatusUsbio = 0;
					DBG_MUTEX_UNLOCK(&system_mutex);
				}
				DBG_MUTEX_LOCK(&modulelist_mutex);
			}
		}
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret) SaveModules(); else WEB_AddMessageInList("Wrong module params");
	
	DBG_MUTEX_LOCK(&system_mutex);
	int iSafe = uiStartMode & 0x0004;
	unsigned char cBcTCP = ucBroadCastTCP;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (ret && iEnabled && (iSafe == 0)) SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_CHANGED,  &uiParentID, sizeof(unsigned int), NULL, 0);
	
	return 1;
}

void WEB_get_uart_select_block(char *mBuff, unsigned int uiMaxLen, unsigned int uiPortNum, char* pDisableFlag)
{
	unsigned int uiLen = 0;
	char chain[32];
	memset(mBuff, 0, uiMaxLen);
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, "<select name='PortNum' style='width: 200px;'%s>\r\n", pDisableFlag);
	strcat(mBuff, msg_subbody);
	uart_info *tty_list;
	uint32_t tty_length;				
	get_uart_ports(&tty_list, &tty_length);
	int cnt = 0;
	int i;
	for (i = 0; i < tty_length; i++)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option %s value='%i'>[%s] %s</option>\r\n", 
							(tty_list[i].Port == uiPortNum) ? "selected" : "",
							tty_list[i].Port, tty_list[i].Chain, tty_list[i].Name);						
		uiLen += strlen(msg_subbody);
		if (uiLen > (uiMaxLen - 50)) break;
		
		strcat(mBuff, msg_subbody);
		if (tty_list[i].Port == uiPortNum) cnt = 1;
	}
	if (tty_length) DBG_FREE(tty_list);	
	if (cnt == 0)
	{
		memset(msg_subbody, 0, 3072);
		sprintf(msg_subbody, "<option selected value='%i'>[%s] N/A</option>\r\n", uiPortNum, get_uart_chain_name(uiPortNum, chain, 32));						
		strcat(mBuff, msg_subbody);
	}
	strcat(mBuff, "	</select>\r\n");
	DBG_FREE(msg_subbody);
}

int WEB_module_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) 
		&& (miModuleList[pParams[0]].Local == 1) && iModuleCnt)
	{
		miModuleList[pParams[0]].Enabled |= 4;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	if (ret) SaveModules();
	return 1;
}

int WEB_module_recover(int *pParams)
{
	int ret = 0;	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt 
		&& (miModuleList[pParams[0]].Local == 1) && (miModuleList[pParams[0]].Enabled & 4))
	{
		miModuleList[pParams[0]].Enabled ^= 4;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	if (ret) SaveModules();
	return 1;
}

int WEB_module_add(int *pParams, char *strValue)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	iModuleCnt++;
	miModuleList = (MODULE_INFO*)DBG_REALLOC(miModuleList, sizeof(MODULE_INFO)*iModuleCnt);
	memset(&miModuleList[iModuleCnt-1], 0, sizeof(MODULE_INFO));
	miModuleList[iModuleCnt-1].Enabled = 0;
	miModuleList[iModuleCnt-1].Local = 1;
	miModuleList[iModuleCnt-1].Type = pParams[2];	
	
	miModuleList[iModuleCnt-1].ID = 0;
	miModuleList[iModuleCnt-1].NewID = pParams[3];
	memset(miModuleList[iModuleCnt-1].Name, 0, 64);
	if (strlen(strValue) > 63) 
		memcpy(miModuleList[iModuleCnt-1].Name, strValue, 63); 
		else
		strcpy(miModuleList[iModuleCnt-1].Name, strValue);
	miModuleList[iModuleCnt-1].ViewLevel = pParams[4];
	miModuleList[iModuleCnt-1].AccessLevel = pParams[5];
	//miModuleList[iModuleCnt-1].Global = pParams[6];
	miModuleList[iModuleCnt-1].ScanSet = pParams[7] / 50;
	//miModuleList[iModuleCnt-1].SaveChanges = pParams[8];
	
	SetModuleDefaultSettings(&miModuleList[iModuleCnt-1]); 
	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	SaveModules();
	return 1;
}

int WEB_module_filters(int *pParams)
{
	if ((pParams[0] >= 0) && (pParams[0] <= 2)) iCurModuleEnabledFilter = pParams[0];
	if ((pParams[1] >= 0) && (pParams[1] <= 2)) iCurModuleAreaFilter = pParams[1];
	if ((pParams[2] >= 0) && (pParams[2] <= MODULE_TYPE_MAX)) iCurModuleTypeFilter = pParams[2];
	
	return 1;
}

int WEB_modules_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n, cnt;	
	int fl1, fl2;
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	TestModules(1);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	unsigned int uiBuffSize = 16384;
	
	char *msg_subbody = (char*)DBG_MALLOC(uiBuffSize);
	char *mBuff = (char*)DBG_MALLOC(4096);
	char *usbio_port_options = (char*)DBG_MALLOC(4096);
	
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/modules/\"><h1>МОДУЛИ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
		
	DBG_MUTEX_LOCK(&modulelist_mutex);
	strcat(msg_tx, "<datalist id='gpiolist'>\r\n");
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 3) && (miModuleList[n].Local == 1) && 
			(miModuleList[n].Type == MODULE_TYPE_GPIO) && (miModuleList[n].Settings[0] & MODULE_SECSET_OUTPUT))
			{
				memset(msg_subbody, 0, 16384);
				sprintf(msg_subbody, "<option value='%.4s'>\r\n", (char*)&miModuleList[n].ID);
				strcat(msg_tx, msg_subbody);
			}
	strcat(msg_tx, "</datalist>\r\n");
	strcat(msg_tx, "<datalist id='templist'>\r\n");
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 3) && (miModuleList[n].Type == MODULE_TYPE_TEMP_SENSOR))
			{
				memset(msg_subbody, 0, 16384);
				sprintf(msg_subbody, "<option value='%.4s'>\r\n", (char*)&miModuleList[n].ID);
				strcat(msg_tx, msg_subbody);
			}
	strcat(msg_tx, "</datalist>\r\n");

	
	memset(msg_subbody, 0, 16384);
	sprintf(msg_subbody, "<form action='/modules/filters'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"Активность: <select name='Enabled' style='width: 140px;'>\r\n"
											"		<option %s value='0'>Отключенные</option>\r\n"
											"		<option %s value='1'>Включенные</option>\r\n"
											"		<option %s value='2'>Все</option>\r\n"											
											"	</select>\r\n"
						"  Расположение: <select name='Area' style='width: 140px;'>\r\n"
											"		<option %s value='0'>Удаленные</option>\r\n"
											"		<option %s value='1'>Локальные</option>\r\n"
											"		<option %s value='2'>Все</option>\r\n"											
											"	</select>\r\n"
						"  Тип: <select name='Type' style='width: 130px;'>\r\n"
											"		<option %s value='%i'>ВСЕ</option>\r\n"
											"		<option %s value='%i'>CAMERA</option>\r\n"
											"		<option %s value='%i'>COUNTER</option>\r\n"
											"		<option %s value='%i'>DISPLAY</option>\r\n"
											"		<option %s value='%i'>GPIO</option>\r\n"
											"		<option %s value='%i'>IR_RECEIVER</option>\r\n"
											"		<option %s value='%i'>MEMORY</option>\r\n"
											"		<option %s value='%i'>MICROPHONE</option>\r\n"
											"		<option %s value='%i'>PN532</option>\r\n"
											"		<option %s value='%i'>RADIO</option>\r\n"
											"		<option %s value='%i'>RC522</option>\r\n"
											"		<option %s value='%i'>REALTIMECLOCK</option>\r\n"
											"		<option %s value='%i'>RS485</option>\r\n"
											"		<option %s value='%i'>EXTERNAL</option>\r\n"
											"		<option %s value='%i'>USB GPIO</option>\r\n"
											"		<option %s value='%i'>SPEAKER</option>\r\n"
											"		<option %s value='%i'>SYSTEM</option>\r\n"
											"		<option %s value='%i'>TEMPSENSOR</option>\r\n"
											"		<option %s value='%i'>TIMER</option>\r\n"
											"		<option %s value='%i'>KEYBOARD</option>\r\n"
											"		<option %s value='%i'>ADS1015</option>\r\n"
											"		<option %s value='%i'>MCP3421</option>\r\n"
											"		<option %s value='%i'>AS5600</option>\r\n"
											"		<option %s value='%i'>HMC5883L</option>\r\n"
											"		<option %s value='%i'>TFP625A</option>\r\n"
											"	</select></td>\r\n"
						"<button type='submit'>Применить</button></form>\r\n",
						session->request_id,
						(iCurModuleEnabledFilter == 0) ? "selected" : "",
						(iCurModuleEnabledFilter == 1) ? "selected" : "",
						(iCurModuleEnabledFilter == 2) ? "selected" : "",
						(iCurModuleAreaFilter == 0) ? "selected" : "",
						(iCurModuleAreaFilter == 1) ? "selected" : "",
						(iCurModuleAreaFilter == 2) ? "selected" : "",
						(iCurModuleTypeFilter == MODULE_TYPE_MAX) ? "selected" : "", 		MODULE_TYPE_MAX, 
						(iCurModuleTypeFilter == MODULE_TYPE_CAMERA) ? "selected" : "", 	MODULE_TYPE_CAMERA, 
						(iCurModuleTypeFilter == MODULE_TYPE_COUNTER) ? "selected" : "", 	MODULE_TYPE_COUNTER, 
						(iCurModuleTypeFilter == MODULE_TYPE_DISPLAY) ? "selected" : "", 	MODULE_TYPE_DISPLAY, 
						(iCurModuleTypeFilter == MODULE_TYPE_GPIO) ? "selected" : "", 		MODULE_TYPE_GPIO,	
						(iCurModuleTypeFilter == MODULE_TYPE_IR_RECEIVER) ? "selected" : "", MODULE_TYPE_IR_RECEIVER,
						(iCurModuleTypeFilter == MODULE_TYPE_MEMORY) ? "selected" : "", 	MODULE_TYPE_MEMORY,	
						(iCurModuleTypeFilter == MODULE_TYPE_MIC) ? "selected" : "", 		MODULE_TYPE_MIC, 
						(iCurModuleTypeFilter == MODULE_TYPE_PN532) ? "selected" : "", 		MODULE_TYPE_PN532,	
						(iCurModuleTypeFilter == MODULE_TYPE_RADIO) ? "selected" : "", 		MODULE_TYPE_RADIO, 
						(iCurModuleTypeFilter == MODULE_TYPE_RC522) ? "selected" : "", 		MODULE_TYPE_RC522,
						(iCurModuleTypeFilter == MODULE_TYPE_RTC) ? "selected" : "", 		MODULE_TYPE_RTC, 
						(iCurModuleTypeFilter == MODULE_TYPE_RS485) ? "selected" : "", 		MODULE_TYPE_RS485,	
						(iCurModuleTypeFilter == MODULE_TYPE_EXTERNAL) ? "selected" : "", 	MODULE_TYPE_EXTERNAL,	
						(iCurModuleTypeFilter == MODULE_TYPE_USB_GPIO) ? "selected" : "", 	MODULE_TYPE_USB_GPIO,	
						(iCurModuleTypeFilter == MODULE_TYPE_SPEAKER) ? "selected" : "", 	MODULE_TYPE_SPEAKER, 
						(iCurModuleTypeFilter == MODULE_TYPE_SYSTEM) ? "selected" : "", 	MODULE_TYPE_SYSTEM, 
						(iCurModuleTypeFilter == MODULE_TYPE_TEMP_SENSOR) ? "selected" : "", MODULE_TYPE_TEMP_SENSOR,
						(iCurModuleTypeFilter == MODULE_TYPE_TIMER) ? "selected" : "", 		MODULE_TYPE_TIMER,
						(iCurModuleTypeFilter == MODULE_TYPE_KEYBOARD) ? "selected" : "", 	MODULE_TYPE_KEYBOARD,
						(iCurModuleTypeFilter == MODULE_TYPE_ADS1015) ? "selected" : "", 	MODULE_TYPE_ADS1015,
						(iCurModuleTypeFilter == MODULE_TYPE_MCP3421) ? "selected" : "", 	MODULE_TYPE_MCP3421,
						(iCurModuleTypeFilter == MODULE_TYPE_AS5600) ? "selected" : "", 	MODULE_TYPE_AS5600,
						(iCurModuleTypeFilter == MODULE_TYPE_HMC5883L) ? "selected" : "", 	MODULE_TYPE_HMC5883L,
						(iCurModuleTypeFilter == MODULE_TYPE_TFP625A) ? "selected" : "", 	MODULE_TYPE_TFP625A);
	strcat(msg_tx, msg_subbody);			
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Включен (тек./буд.)</th><th>Тип</th><th>ИД</th><th>Версия</th><th>Имя</th><th>Уровень видимости</th>"
					"<th>Уровень доступа</th><th>Интервал проверки (кратно 50мс)</th><th>Сохранять изменения в файл</th><th>Настройки</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	
	int iModuleFiltCnt = 0;
	for (n = 0; n < iModuleCnt; n++)
	{
		if (((iCurModuleEnabledFilter == (miModuleList[n].Enabled & 1)) || (iCurModuleEnabledFilter == 2)) &&
			((iCurModuleAreaFilter == miModuleList[n].Local) || (iCurModuleAreaFilter == 2)) &&
			((iCurModuleTypeFilter == miModuleList[n].Type) || (iCurModuleTypeFilter == MODULE_TYPE_MAX)))
		{
			iModuleFiltCnt++;
		}
	}
	
	iSubPages = (int)ceil((double)iModuleFiltCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurModulePage = iPage;
	iCurPage = iCurModulePage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurModulePage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurModulePage = iCurPage;
	}
	
	iFirstOption = 0;
	iLastOption = 0;
	int i = 0;
	int k = 0;
	for (n = 0; n < iModuleCnt; n++)
	{
		if (((iCurModuleEnabledFilter == (miModuleList[n].Enabled & 1)) || (iCurModuleEnabledFilter == 2)) &&
			((iCurModuleAreaFilter == miModuleList[n].Local) || (iCurModuleAreaFilter == 2)) &&
			((iCurModuleTypeFilter == miModuleList[n].Type) || (iCurModuleTypeFilter == MODULE_TYPE_MAX)))
		{			
			if (i < (iCurPage * WEB_PAGE_MAX_LEN)) {i++; iFirstOption = n;}
			if (k < (iFirstOption + WEB_PAGE_MAX_LEN)) {k++; iLastOption = n;} else break;
		}
	}	
	
	WEB_pages_list(msg_tx, "modules", iSubPages, iCurPage, "");	
	
	char *pDisableFlag;
	unsigned int uiSettCnt = 2;
	if (iLastOption < iModuleCnt)
	for (n = iFirstOption; n <= iLastOption; n++)
	{
		if (((iCurModuleEnabledFilter == (miModuleList[n].Enabled & 1)) || (iCurModuleEnabledFilter == 2)) &&
			((iCurModuleAreaFilter == miModuleList[n].Local) || (iCurModuleAreaFilter == 2)) &&
			((iCurModuleTypeFilter == miModuleList[n].Type) || (iCurModuleTypeFilter == MODULE_TYPE_MAX)))
		{			
			if ((miModuleList[n].Local != 1) || (miModuleList[n].Enabled & 4)) pDisableFlag = " disabled"; else pDisableFlag = "";
			//printf("msg_list len %i %i\n", strlen(msg_tx), strlen(msg_subbody));	
			
			memset(msg_subbody, 0, 16384);
			sprintf(msg_subbody, 
					"<tr><form action='/modules/save'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>\r\n"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='checkbox' name='Enn' %s disabled>\r\n"
					"<input type='checkbox' name='En'%s%s></td>\r\n"
					"<input type='hidden' name='Type' value='%i'>"
					"<td><input type='text' name='Tp' value='%s' style='width: 140px;' disabled><br />%s</td>\r\n"
					"<td>%.4s<input type='text' name='ID' value='%.4s' maxlength=4 style='width: 60px;'%s></td>\r\n"
					"<td>%i.%i.%i.%i</td>"
					"<td><input type='text' name='Name' value='%s' maxlength=60 style='width: 150px;'%s></td>\r\n"
					"<td><input type='number' name='ViewLevel' min=0 max=1000 value='%i' maxlength=2 style='width: 50px;'%s></td>\r\n"
					"<td><input type='number' name='AccessLevel' min=0 max=1000 value='%i' maxlength=2 style='width: 50px;'%s></td>\r\n"
					"<td><input type='number' name='ScanSet' min=0 value='%i' maxlength=6 style='width: 50px;'%s>(%is)</td>\r\n"
					"<td><input type='checkbox' name='SaveChanges'%s disabled></td>\r\n",
					session->request_id,
					n,n,
					(miModuleList[n].Enabled & 1) ? " checked" : "", 
					(miModuleList[n].Enabled & 2) ? " checked" : "", 
					pDisableFlag,
					miModuleList[n].Type, GetModuleTypeName(miModuleList[n].Type), GetModuleSubTypeName(miModuleList[n].SubType),
					(char*)&miModuleList[n].ID, (char*)&miModuleList[n].NewID, pDisableFlag,
					miModuleList[n].Version[0], miModuleList[n].Version[1], miModuleList[n].Version[2], miModuleList[n].Version[3],
					miModuleList[n].Name, pDisableFlag,
					miModuleList[n].ViewLevel, pDisableFlag,
					miModuleList[n].AccessLevel, pDisableFlag,
					miModuleList[n].ScanSet * 50, pDisableFlag, miModuleList[n].ScanSet * 5 / 100, 
					(miModuleList[n].SaveChanges != 0) ? " checked" : "");		
			strcat(msg_tx, msg_subbody);
			
			memset(msg_subbody, 0, 16384);		
			switch(miModuleList[n].Type)
			{
				case MODULE_TYPE_SPEAKER:
					sprintf(msg_subbody, "<td>ИД(I/O):<input type='text' list='gpiolist' name='SubID' value='%.4s' maxlength=4 style='width: 60px;'%s><br />\r\n"
											"Номер (hw:?,0):<input type='number' name='Dev1' min=0 max=9 value='%i' style='width: 60px;'%s>\r\n"
											"Субномер (hw:0,?):<input type='number' name='Dev2' min=0 max=9 value='%i' style='width: 60px;'%s><br />\r\n"
											"Использовать каналов:<input type='number' name='Channels' min=0 max=7 value='%i' style='width: 60px;'%s><br />\r\n"
											"Максимум потоков:<input type='number' name='Streams' min=0 max=20 value='%i' style='width: 60px;'%s><br />\r\n"
											"Время задержки отключения (PWR):<input type='number' name='Timer' min=0 max=120000 value='%i' style='width: 60px;'%s><br />\r\n"
											"Использовать частоты: 8K:<input type='checkbox' name='SR8'%s%s>\r\n"
																"  11K:<input type='checkbox' name='SR11'%s%s>\r\n"
																"  16K:<input type='checkbox' name='SR16'%s%s>\r\n"
																"  22K:<input type='checkbox' name='SR22'%s%s>\r\n"
																"  44K:<input type='checkbox' name='SR44'%s%s>\r\n"
																"  48K:<input type='checkbox' name='SR48'%s%s>\r\n"
																"  96K:<input type='checkbox' name='SR96'%s%s></td>\r\n",
										(char*)&miModuleList[n].Settings[0], pDisableFlag,
										miModuleList[n].Settings[1], pDisableFlag,
										miModuleList[n].Settings[2], pDisableFlag,
										miModuleList[n].Settings[3], pDisableFlag,
										miModuleList[n].Settings[4], pDisableFlag,
										miModuleList[n].Settings[5], pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_8000) ? " checked" : "", pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_11025) ? " checked" : "", pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_16000) ? " checked" : "", pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_22050) ? " checked" : "", pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_44100) ? " checked" : "", pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_48000) ? " checked" : "", pDisableFlag,
										(miModuleList[n].Settings[6] & SAMPLE_RATE_96000) ? " checked" : "", pDisableFlag);
					break;
				case MODULE_TYPE_MIC:
					sprintf(msg_subbody, "<td>Каналов:<input type='number' name='Channels' min=0 max=7 value='%i' style='width: 60px;'%s><br />\r\n"
											"Номер (hw:?,0):<input type='number' name='Dev1' min=0 max=9 value='%i' style='width: 60px;'%s><br />\r\n"
											"Субномер (hw:0,?):<input type='number' name='Dev2' min=0 max=9 value='%i' style='width: 60px;'%s><br />\r\n"
											"Чувствительность: <input type='range' name='Amplifier' min='0' max='100' step='1' value='%i' style='width: 200px;' %s><br />\r\n"
											"Aвтоусиление: <input type='checkbox' name='AGC'%s %s><br />\r\n"
											"Обработка:<select name='Convert' style='width: 200px;'%s>\r\n"
												"		<option %s value='0'>Отключено</option>\r\n"
												"		<option %s value='1'>Цифровое усиление</option>\r\n"
												"		<option %s value='2'>Цифровое автоусиление</option>\r\n"
												"	</select><br />\r\n"
											"&emsp;&emsp;Уровень цифрового усиления: <input type='range' name='DigLevel' min='1' max='14' step='1' value='%i' style='width: 200px;' %s><br />\r\n"							
											"Частота:<input type='number' name='SampleRate' min=0 max=96000 value='%i' style='width: 100px;'%s><br />\r\n"
											"Кодек:<select name='Codec' style='width: 200px;'%s>\r\n"
												"		<option %s value='0'>AAC</option>\r\n"
												"		<option %s value='1'>MP3</option>\r\n"
												"		<option %s value='2'>Speex</option>\r\n"
												"		<option %s value='3'>PCM</option>\r\n"
												"		<option %s value='4'>ADPCM</option>\r\n"
												"		<option %s value='5'>NELLYMOSER</option>\r\n"
												"		<option %s value='6'>MP2 (без видео)</option>\r\n"
												"		<option %s value='7' disabled>WMA v1 (экспериментальный)</option>\r\n"
												"		<option %s value='8' disabled>WMA v2 (экспериментальный)</option>\r\n"
												"		<option %s value='9' disabled>WMA voice (экспериментальный)</option>\r\n"
												"		<option %s value='10' disabled>WMA pro (экспериментальный)</option>\r\n"
												"		<option %s value='11' disabled>WMA lossless (экспериментальный)</option>\r\n"
												"		<option %s value='12' disabled>MP1 (экспериментальный)</option>\r\n"
												"		<option %s value='13' disabled>MPEG2TS (экспериментальный)</option>\r\n"
												"		<option %s value='14'>AC3 (без видео)</option>\r\n"
												"		<option %s value='15' disabled>DTS)</option>\r\n"
												"		<option %s value='16' disabled>VORBIS</option>\r\n"
												"		<option %s value='17' disabled>WAVPACK</option>\r\n"
												"		<option %s value='18'>EAC3 (без видео)</option>\r\n"
												"	</select><br />\r\n"
											"Битрэйт:<select name='BitRate' style='width: 140px;'%s>\r\n"
												"		<option %s value='0'>0(VBR)(High)</option>\r\n"
												"		<option %s value='1'>1(VBR)</option>\r\n"
												"		<option %s value='2'>2(VBR)</option>\r\n"
												"		<option %s value='3'>3(VBR)</option>\r\n"
												"		<option %s value='4'>4(VBR)</option>\r\n"
												"		<option %s value='5'>5(VBR)(Low)</option>\r\n"
												"		<option %s value='16000'>16K(CBR)</option>\r\n"
												"		<option %s value='32000'>32K(CBR)</option>\r\n"
												"		<option %s value='64000'>64K(CBR)</option>\r\n"
												"		<option %s value='96000'>96K(CBR)</option>\r\n"
												"		<option %s value='128000'>128K(CBR)</option>\r\n"
												"		<option %s value='160000'>160K(CBR)</option>\r\n"
												"		<option %s value='192000'>192K(CBR)</option>\r\n"
												"		<option %s value='256000'>256K(CBR)</option>\r\n"
												"		<option %s value='320000'>320K(CBR)</option>\r\n"
												"	</select><br />\r\n"
											"Шаг сканирования:<input type='number' name='ScanClk' min=1 max=1024 value='%i' style='width: 100px;'%s><br />\r\n"
											"&emsp;&emsp;Уровень обнаружения:<input type='number' name='SenceSet' min=0 max=1024 value='%i' style='width: 100px;'%s><br />\r\n"
											"&emsp;&emsp;Пропуск фреймов при сканировании:<input type='number' name='SenceSkipSet' min=0 max=100 value='%i' style='width: 100px;'%s><br />\r\n"
											"Записывать нормальный режим:<input type='checkbox' name='RecNorm'%s%s><br />\r\n"
											"Записывать дифференциальный режим:<input type='checkbox' name='RecVidDiff'%s%s><br />\r\n"
											//"&emsp;&emsp;Отдельно от видео:<input type='checkbox' name='RecSplit'%s%s><br />\r\n"
											"Записывать выше уровня:<input type='checkbox' name='RecDiff'%s%s><br />\r\n"
											"&emsp;&emsp;Уровень:<input type='number' name='RecLevel' min=0 max=1024 value='%i' maxlength=2 style='width: 100px;'%s><br />\r\n"									
											"&emsp;&emsp;Пропуск фреймов при сканировании:<input type='number' name='DiffSkip' min=0 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"
											"&emsp;&emsp;Сохранять фреймов после обнаружения:<input type='number' name='SaveFrames' min=1 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"
											"&emsp;&emsp;Шаг сканирования:<input type='number' name='ScanPeriod' min=1 max=1024 value='%i' style='width: 100px;'%s></td>\r\n",
										miModuleList[n].Settings[0], pDisableFlag,  
										miModuleList[n].Settings[1], pDisableFlag,
										miModuleList[n].Settings[2], pDisableFlag,
										miModuleList[n].Settings[15], pDisableFlag,
										(miModuleList[n].Settings[7] & 4) ? " checked " : "", pDisableFlag,
										pDisableFlag,
										(miModuleList[n].Settings[17] == 0) ? "selected" : "",
										(miModuleList[n].Settings[17] == 1) ? "selected" : "",
										(miModuleList[n].Settings[17] == 2) ? "selected" : "",	
										miModuleList[n].Settings[18], pDisableFlag,										
										miModuleList[n].Settings[3], pDisableFlag,
										pDisableFlag,
										(miModuleList[n].Settings[16] == 0) ? "selected" : "",
										(miModuleList[n].Settings[16] == 1) ? "selected" : "",
										(miModuleList[n].Settings[16] == 2) ? "selected" : "",
										(miModuleList[n].Settings[16] == 3) ? "selected" : "",
										(miModuleList[n].Settings[16] == 4) ? "selected" : "",
										(miModuleList[n].Settings[16] == 5) ? "selected" : "",
										(miModuleList[n].Settings[16] == 6) ? "selected" : "",
										(miModuleList[n].Settings[16] == 7) ? "selected" : "",
										(miModuleList[n].Settings[16] == 8) ? "selected" : "",
										(miModuleList[n].Settings[16] == 9) ? "selected" : "",
										(miModuleList[n].Settings[16] == 10) ? "selected" : "",
										(miModuleList[n].Settings[16] == 11) ? "selected" : "",
										(miModuleList[n].Settings[16] == 12) ? "selected" : "",
										(miModuleList[n].Settings[16] == 13) ? "selected" : "",
										(miModuleList[n].Settings[16] == 14) ? "selected" : "",
										(miModuleList[n].Settings[16] == 15) ? "selected" : "",
										(miModuleList[n].Settings[16] == 16) ? "selected" : "",
										(miModuleList[n].Settings[16] == 17) ? "selected" : "",
										(miModuleList[n].Settings[16] == 18) ? "selected" : "",
										pDisableFlag,
										(miModuleList[n].Settings[4] == 0) ? "selected" : "",
										(miModuleList[n].Settings[4] == 1) ? "selected" : "",
										(miModuleList[n].Settings[4] == 2) ? "selected" : "",
										(miModuleList[n].Settings[4] == 3) ? "selected" : "",
										(miModuleList[n].Settings[4] == 4) ? "selected" : "",
										(miModuleList[n].Settings[4] == 5) ? "selected" : "",
										(miModuleList[n].Settings[4] == 16000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 32000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 64000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 96000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 128000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 160000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 192000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 256000) ? "selected" : "",
										(miModuleList[n].Settings[4] == 320000) ? "selected" : "",
										miModuleList[n].Settings[5], pDisableFlag,
										miModuleList[n].Settings[6], pDisableFlag,
										miModuleList[n].Settings[8], pDisableFlag,
										(miModuleList[n].Settings[7] & 1) ? " checked " : "", pDisableFlag,
										(miModuleList[n].Settings[7] & 2) ? " checked " : "", pDisableFlag,
										//(miModuleList[n].Settings[9] & 1) ? " checked " : "", pDisableFlag,
										miModuleList[n].Settings[10] ? " checked " : "", pDisableFlag,
										miModuleList[n].Settings[11], pDisableFlag,
										miModuleList[n].Settings[12], pDisableFlag,
										miModuleList[n].Settings[13], pDisableFlag,
										miModuleList[n].Settings[14], pDisableFlag);
					break;
				case MODULE_TYPE_USB_GPIO:
					WEB_get_uart_select_block(mBuff, 4000, miModuleList[n].Settings[0] >> 16, pDisableFlag);					
					sprintf(msg_subbody, "<td>Порт:%s<br />"
										"Скорость UART: <input type='number' name='PtSp' min=0 value='%i' style='width: 100px;'%s><br />\r\n"
										"Автосканирование: <input type='checkbox' name='AtSc'%s%s><br />\r\n"
										"Настройки в модуле: <input type='checkbox' name='StMd'%s%s><br />\r\n"
										"Количество портов: <input type='number' name='SensCnt' min=0 value='%i' style='width: 50px;'%s><br />\r\n"
										"Выбор для редактирования: <input type='number' name='SensSel' min=1 max=%i value='%i' style='width: 50px;'%s><br />\r\n",
										mBuff,
										miModuleList[n].Settings[1], pDisableFlag,
										(miModuleList[n].Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) ? " checked " : "", pDisableFlag,
										(miModuleList[n].Settings[0] & USBIO_MAIN_SETT_SAVE_IN_MODULE) ? " checked " : "", pDisableFlag,
										miModuleList[n].SettingsCount, pDisableFlag,
										miModuleList[n].SettingsCount ? miModuleList[n].SettingsCount : 1, miModuleList[n].SettingSelected + 1, pDisableFlag);
					
					char *cColor;
					for (i = 0; i < miModuleList[n].SettingsCount; i++)
					{
						memset(mBuff, 0, 4096);
						if (i & 1) cColor = "Blue"; else cColor = "Green";
						if (miModuleList[n].Local && ((miModuleList[n].Enabled & 4) == 0) && miModuleList[n].ParamsCount)
						{	
							int iPortSelected = miModuleList[n].SettingList[i].ID;
							if (iPortSelected >= miModuleList[n].ParamsCount) iPortSelected = 0;
							if (miModuleList[n].ParamList[iPortSelected].CurrentType == USBIO_PIN_SETT_TYPE_DISABLED) cColor = "Black";
							if ((miModuleList[n].ParamList[iPortSelected].CurrentType > USBIO_PIN_SETT_TYPE_DISABLED)
								&&
								(miModuleList[n].ParamList[iPortSelected].CurrentType < USBIO_PIN_SETT_TYPE_INPUT))
									cColor = "Red";
							if (miModuleList[n].SettingSelected == i)
							{
								memset(usbio_port_options, 0, 4096);
								for (k = 0; k < miModuleList[n].ParamsCount; k++)
								{
									char supname[64];
									memset(supname, 0, 64);
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_DIGITAL_IN) strcat(supname, "IN ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_ANALOG_IN) strcat(supname, "ADC ");
									if (miModuleList[n].ParamList[k].SupportTypes & (USBIO_PIN_PARAM_TYPE_DIGITAL_OUT_OD | USBIO_PIN_PARAM_TYPE_DIGITAL_OUT_PP)) 
																												strcat(supname, "OUT ");
									if (miModuleList[n].ParamList[k].SupportTypes & (USBIO_PIN_PARAM_TYPE_PWM_OUT_OD | USBIO_PIN_PARAM_TYPE_PWM_OUT_PP)) 
																												strcat(supname, "PWM ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_IR_RECIEVER) strcat(supname, "IR ");
									//if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_20MA) strcat(supname, "20mA ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_3MA) strcat(supname, "3mA ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_TOLERANT_5V) strcat(supname, "5V ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_UART_TX) strcat(supname, "UART_TX ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_UART_RX) strcat(supname, "UART_RX ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_SCL) strcat(supname, "I2C_SCL ");
									if (miModuleList[n].ParamList[k].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_SDA) strcat(supname, "I2C_SDA ");
									
									memset(mBuff, 0, 4096);
									sprintf(mBuff, "<option %s value='%i'>%c%i (%s)</option>\r\n", 
											(k == iPortSelected) ? " selected " : "",
											k, miModuleList[n].ParamList[k].Port + 65, miModuleList[n].ParamList[k].Number,
											supname);
									strcat(usbio_port_options, mBuff);
								}
								memset(mBuff, 0, 4096);
								sprintf(mBuff, "<font color='%s'>[%i] Порт: <select name='SensPort' style='width: 270px;'>%s</select>:<br />\r\n"
										"&ensp;&ensp;Сохраненные настройки:<br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Вкл.: <input type='checkbox' name='SensEnabl'%s><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Вкл. по питанию: <input type='checkbox' name='SensInit'%s%s><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Тип: <select name='SensType' style='width: 200px;'>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option disabled %s value='%i'>%s</option>\r\n"
										"		<option disabled %s value='%i'>%s</option>\r\n"
										"		<option disabled %s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"		<option %s%s value='%i'>%s</option>\r\n"
										"	</select><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Интерв. скан.: <input type='number' name='SensPeriod' min=0 max=3276750 value='%i' style='width: 100px;'>(%is)<br />\r\n",
										cColor, i + 1, usbio_port_options,										
										miModuleList[n].SettingList[i].Enabled ? " checked " : "",
										miModuleList[n].SettingList[i].PwrInit ? " checked " : "", 
											(miModuleList[n].Settings[0] & USBIO_MAIN_SETT_SAVE_IN_MODULE) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_DISABLED) ? "selected" : "", 0, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_DISABLED),
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_BUSY) ? "selected" : "", 0, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_BUSY),
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_UNSUPPORTED) ? "selected" : "", 0, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_UNSUPPORTED),
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_WRONG_PARAMS) ? "selected" : "", 0, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_WRONG_PARAMS),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & (USBIO_PIN_PARAM_TYPE_DIGITAL_IN | USBIO_PIN_PARAM_TYPE_ANALOG_IN)) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_INPUT) ? "selected" : "", USBIO_PIN_SETT_TYPE_INPUT, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_INPUT),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & (USBIO_PIN_PARAM_TYPE_DIGITAL_OUT_OD | 
																									USBIO_PIN_PARAM_TYPE_DIGITAL_OUT_PP |
																									USBIO_PIN_PARAM_TYPE_PWM_OUT_OD |
																									USBIO_PIN_PARAM_TYPE_PWM_OUT_PP)) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_OUTPUT) ? "selected" : "", USBIO_PIN_SETT_TYPE_OUTPUT, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_OUTPUT),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_IR_RECIEVER) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_IR_RECIEVER) ? "selected" : "", USBIO_PIN_SETT_TYPE_IR_RECIEVER, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_IR_RECIEVER),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_LM75) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_LM75, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_LM75),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_AM2320) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_AM2320, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_AM2320),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_ADS1015) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_ADS1015, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_ADS1015),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_MCP3421) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_MCP3421, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_MCP3421),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_AS5600) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_AS5600, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_AS5600),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_HMC5883L) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_HMC5883L, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_HMC5883L),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_RS485) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_RS485) ? "selected" : "", USBIO_PIN_SETT_TYPE_RS485, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_RS485),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_RS485) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_RS485_RC522) ? "selected" : "", USBIO_PIN_SETT_TYPE_RS485_RC522, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_RS485_RC522),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_RS485) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_RS485_PN532) ? "selected" : "", USBIO_PIN_SETT_TYPE_RS485_PN532, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_RS485_PN532),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_I2C_MISC) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_I2C_MISC) ? "selected" : "", USBIO_PIN_SETT_TYPE_I2C_MISC, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_I2C_MISC),
										(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_RS485) ? "" : " disabled ",
										(miModuleList[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_RS485_MISC) ? "selected" : "", USBIO_PIN_SETT_TYPE_RS485_MISC, usb_gpio_get_type_name(USBIO_PIN_SETT_TYPE_RS485_MISC),
										miModuleList[n].SettingList[i].Interval * 50, (miModuleList[n].SettingList[i].Interval * 5) / 100);
								strcat(msg_subbody, mBuff);
								switch(miModuleList[n].SettingList[i].CurrentType)
								{
									case USBIO_PIN_SETT_TYPE_INPUT:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Подтяжка: <select name='SensSett1' style='width: 100px;'%s>\r\n"
														"		<option %s value='0'>%s</option>\r\n"
														"		<option %s value='1'>%s</option>\r\n"
														"		<option %s value='2'>%s</option>\r\n"
														"	</select><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Инверт.: <input type='checkbox' name='SensSett2'%s><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Аналоговый: <input type='checkbox' name='SensSett3'%s%s><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Разрядность: <input type='number' name='SensSett4' min=0 max=12 value='%i' style='width: 50px;'%s><br />\r\n",
														(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_DIGITAL_IN) ? "" : " disabled ",														
														(miModuleList[n].SettingList[i].Pull == 0) ? "selected" : "", usb_gpio_get_pull_name(0),
														(miModuleList[n].SettingList[i].Pull == 1) ? "selected" : "", usb_gpio_get_pull_name(1),
														(miModuleList[n].SettingList[i].Pull == 2) ? "selected" : "", usb_gpio_get_pull_name(2),
														miModuleList[n].SettingList[i].Inverted ? " checked " : "",
														miModuleList[n].SettingList[i].Analog ? " checked " : "",
														(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_ANALOG_IN) ? "" : " disabled ",
														miModuleList[n].SettingList[i].Accuracy,
														(miModuleList[n].ParamList[iPortSelected].SupportTypes & USBIO_PIN_PARAM_TYPE_ANALOG_IN) ? "" : " disabled ");	
											strcat(msg_subbody, mBuff);
										}
										break;									
									case USBIO_PIN_SETT_TYPE_OUTPUT:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Подтяжка: <select name='SensSett1' style='width: 100px;'>\r\n"
														"		<option %s value='0'>%s</option>\r\n"
														"		<option %s value='1'>%s</option>\r\n"
														"		<option %s value='2'>%s</option>\r\n"
														"	</select><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Значение по умолч.: <input type='number' name='SensSett4' min=0 max=255 value='%i' style='width: 100px;'><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Инверт.: <input type='checkbox' name='SensSett2'%s><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Режим: <select name='SensSett7' style='width: 100px;'>\r\n"
														"		<option %s value='0'>%s</option>\r\n"
														"		<option %s value='1'>%s</option>\r\n"
														"		<option %s value='2'>%s</option>\r\n"
														"	</select><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Импульс.: <input type='number' name='SensSett3' min=0 max=12750 value='%i' style='width: 100px;'>(%is)<br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Open drain: <input type='checkbox' name='SensSett5'%s><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;ШИМ: <input type='checkbox' name='SensSett6'%s><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;Частота ШИМ: <input type='number' name='SensSett8' min=0 max=51000 value='%i' style='width: 100px;'>Hz<br />\r\n",
														(miModuleList[n].SettingList[i].Pull == 0) ? "selected" : "", usb_gpio_get_pull_name(0),
														(miModuleList[n].SettingList[i].Pull == 1) ? "selected" : "", usb_gpio_get_pull_name(1),
														(miModuleList[n].SettingList[i].Pull == 2) ? "selected" : "", usb_gpio_get_pull_name(2),
														miModuleList[n].SettingList[i].DefaultValue,
														miModuleList[n].SettingList[i].Inverted ? " checked " : "", 
														(miModuleList[n].SettingList[i].Mode == USBIO_CMD_MODE_STATIC) ? "selected" : "", usb_gpio_get_mode_name(USBIO_CMD_MODE_STATIC),
														(miModuleList[n].SettingList[i].Mode == USBIO_CMD_MODE_IMPULSE) ? "selected" : "", usb_gpio_get_mode_name(USBIO_CMD_MODE_IMPULSE),
														(miModuleList[n].SettingList[i].Mode == USBIO_CMD_MODE_PERIOD) ? "selected" : "", usb_gpio_get_mode_name(USBIO_CMD_MODE_PERIOD),
														miModuleList[n].SettingList[i].ImpulseLen * 50, miModuleList[n].SettingList[i].ImpulseLen * 5 / 100,
														miModuleList[n].SettingList[i].OpenDrain ? " checked " : "", 
														miModuleList[n].SettingList[i].PWM ? " checked " : "",
														miModuleList[n].SettingList[i].PortSpeedCode * 200);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AM2320:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Датчик: <select name='SensSett1' style='width: 100px;'>\r\n"
														"		<option %s value='0'>Температуры</option>\r\n"
														"		<option %s value='1'>Влажности</option>\r\n"
														"	</select><br />\r\n",
														(miModuleList[n].SettingList[i].ResultType == 0) ? "selected" : "",
														(miModuleList[n].SettingList[i].ResultType != 0) ? "selected" : "");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MISC:
										{
											int iSpeed;
											memset(usbio_port_options, 0, 4096);
											for (k = 0; k < 63; k++)
											{
												iSpeed = get_i2c_code_to_speed(k);
												if ((iSpeed == 0) && k) break;
												memset(mBuff, 0, 4096);
												sprintf(mBuff, "<option %s%s value='%i'>%i</option>\r\n", 
														k ? "" : " disabled ",
														(k == miModuleList[n].SettingList[i].PortSpeedCode) ? " selected " : "",
														k, iSpeed);
												strcat(usbio_port_options, mBuff);
											}
											
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: <input type='number' name='SensSett1' min=0 max=127 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Датчик: <input type='number' name='SensSett2' min=1 max=32 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Скорость: <select name='SensSett3' style='width: 100px;'>%s</select><br />\r\n",
														miModuleList[n].SettingList[i].Address,
														miModuleList[n].SettingList[i].ResultType,
														usbio_port_options);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_LM75:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: <input type='number' name='SensSett1' min=0 max=7 value='%i' style='width: 100px;'><br />\r\n",
														miModuleList[n].SettingList[i].Address);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_ADS1015:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: <input type='number' name='SensSett1' min=0 max=3 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Диапазон: <select name='SensSett2' style='width: 100px;'>\r\n"
															"		<option %s value=0>%s</option>\r\n"
															"		<option %s value=1>%s</option>\r\n"
															"		<option %s value=2>%s</option>\r\n"
															"		<option %s value=3>%s</option>\r\n"
															"		<option %s value=4>%s</option>\r\n"
															"		<option %s value=5>%s</option>\r\n"
															"		<option %s value=6>%s</option>\r\n"
															"		<option %s value=7>%s</option>\r\n"
															"	</select><br />\r\n",															
														miModuleList[n].SettingList[i].Address,
														(miModuleList[n].SettingList[i].Gain == 0) ? " selected" : "", Get_ADS1015_RangeName(0),
														(miModuleList[n].SettingList[i].Gain == 1) ? " selected" : "", Get_ADS1015_RangeName(1),
														(miModuleList[n].SettingList[i].Gain == 2) ? " selected" : "", Get_ADS1015_RangeName(2),
														(miModuleList[n].SettingList[i].Gain == 3) ? " selected" : "", Get_ADS1015_RangeName(3),
														(miModuleList[n].SettingList[i].Gain == 4) ? " selected" : "", Get_ADS1015_RangeName(4),
														(miModuleList[n].SettingList[i].Gain == 5) ? " selected" : "", Get_ADS1015_RangeName(5),
														(miModuleList[n].SettingList[i].Gain == 6) ? " selected" : "", Get_ADS1015_RangeName(6),
														(miModuleList[n].SettingList[i].Gain == 7) ? " selected" : "", Get_ADS1015_RangeName(7));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MCP3421:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: <input type='number' name='SensSett1' min=0 max=7 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Точность: <select name='SensSett2' style='width: 100px;'>\r\n"
															"		<option %s value='0'>%s</option>\r\n"
															"		<option %s value='1'>%s</option>\r\n"
															"		<option %s value='2'>%s</option>\r\n"
															"		<option %s value='3'>%s</option>\r\n"
															"	</select><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Усиление: <select name='SensSett3' style='width: 100px;'>\r\n"
															"		<option %s value='0'>%s</option>\r\n"
															"		<option %s value='1'>%s</option>\r\n"
															"		<option %s value='2'>%s</option>\r\n"
															"		<option %s value='3'>%s</option>\r\n"
															"	</select><br />\r\n",
														miModuleList[n].SettingList[i].Address,
														(miModuleList[n].SettingList[i].Accuracy == 0) ? " selected" : "", Get_MCP3421_AccuracyName(0),
														(miModuleList[n].SettingList[i].Accuracy == 1) ? " selected" : "", Get_MCP3421_AccuracyName(1),
														(miModuleList[n].SettingList[i].Accuracy == 2) ? " selected" : "", Get_MCP3421_AccuracyName(2),
														(miModuleList[n].SettingList[i].Accuracy == 3) ? " selected" : "", Get_MCP3421_AccuracyName(3),
														(miModuleList[n].SettingList[i].Gain == 0) ? " selected" : "", Get_MCP3421_GainName(0),
														(miModuleList[n].SettingList[i].Gain == 1) ? " selected" : "", Get_MCP3421_GainName(1),
														(miModuleList[n].SettingList[i].Gain == 2) ? " selected" : "", Get_MCP3421_GainName(2),
														(miModuleList[n].SettingList[i].Gain == 3) ? " selected" : "", Get_MCP3421_GainName(3));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AS5600:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: <input type='number' name='SensSett1' min=0 max=7 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Тип: <select name='SensSett2' style='width: 100px;'>\r\n"
															"		<option %s value='0'>RAW</option>\r\n"
															"		<option %s value='1'>Норм.</option>\r\n"
															"	</select><br />\r\n",
														miModuleList[n].SettingList[i].Address,
														(miModuleList[n].SettingList[i].ResultType == 0) ? " selected" : "",
														(miModuleList[n].SettingList[i].ResultType == 1) ? " selected" : "");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_HMC5883L:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: <select name='SensSett1' style='width: 100px;'>\r\n"
															"		<option %s value='0'>X raw</option>\r\n"
															"		<option %s value='1'>Y raw</option>\r\n"
															"		<option %s value='2'>Z raw</option>\r\n"
															"		<option %s value='3'>X в градусах</option>\r\n"
															"		<option %s value='4'>Y в градусах</option>\r\n"
															"		<option %s value='5'>Z в градусах</option>\r\n"
															"	</select><br />\r\n",
														(miModuleList[n].SettingList[i].ResultType == 0) ? " selected" : "",
														(miModuleList[n].SettingList[i].ResultType == 1) ? " selected" : "",
														(miModuleList[n].SettingList[i].ResultType == 2) ? " selected" : "",
														(miModuleList[n].SettingList[i].ResultType == 3) ? " selected" : "",
														(miModuleList[n].SettingList[i].ResultType == 4) ? " selected" : "",
														(miModuleList[n].SettingList[i].ResultType == 5) ? " selected" : "");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_IR_RECIEVER:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Время пропуска повторных команд: <input type='number' name='SensSett1' min=0 max=12750 value='%i' style='width: 100px;'>(%is)<br />\r\n",
														miModuleList[n].SettingList[i].TimeSkip * 50, miModuleList[n].SettingList[i].TimeSkip * 5 / 100);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485:
										{
											int iWPortSubSelected = miModuleList[n].SettingList[i].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;	
											int iRPortSubSelected = miModuleList[n].SettingList[i].IORControlPort;
											if (iRPortSubSelected < miModuleList[n].SettingsCount)
												iRPortSubSelected = miModuleList[n].SettingList[iRPortSubSelected].ID;
												else
												iRPortSubSelected = 0;	
											
											int iSpeed;
											memset(usbio_port_options, 0, 4096);
											for (k = 0; k < 63; k++)
											{
												iSpeed = get_uart_code_to_speed(k);
												if ((iSpeed == 0) && k) break;
												memset(mBuff, 0, 4096);
												sprintf(mBuff, "<option %s%s value='%i'>%i</option>\r\n", 
														k ? "" : " disabled ",
														(k == miModuleList[n].SettingList[i].PortSpeedCode) ? " selected " : "",
														k, iSpeed);
												strcat(usbio_port_options, mBuff);
											}
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Скорость: <select name='SensSett1' style='width: 100px;'>%s</select><br />\r\n"															
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' name='SensSett2'%s><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправка: <input type='number' name='SensSett4' min=1 max=%i value='%i' style='width: 100px;'>(%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления прием: <input type='checkbox' name='SensSett3'%s><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт приема: <input type='number' name='SensSett5' min=1 max=%i value='%i' style='width: 100px;'>(%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): <input type='number' name='SensSett6' min=0 max=127 value='%i' style='width: 100px;'>us<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим управления: <select name='SensSett8' style='width: 100px;'>\r\n"
															"		<option %s value='0'>Операциями</option>\r\n"
															"		<option %s value='1'>Передача/Прием</option>\r\n"
															"	</select><br />\r\n",
															usbio_port_options,
															miModuleList[n].SettingList[i].IOWControlUse ? " checked " : "",
															miModuleList[n].SettingsCount, miModuleList[n].SettingList[i].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].SettingList[i].IORControlUse ? " checked " : "",
															miModuleList[n].SettingsCount, miModuleList[n].SettingList[i].IORControlPort + 1, 
															miModuleList[n].ParamList[iRPortSubSelected].Port + 65, miModuleList[n].ParamList[iRPortSubSelected].Number,
															miModuleList[n].SettingList[i].ImpulseLen * 100,
															(miModuleList[n].SettingList[i].IOModeControl == 0) ? "selected" : "",
															(miModuleList[n].SettingList[i].IOModeControl != 0) ? "selected" : "");
																																							
											strcat(msg_subbody, mBuff);											
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485_MISC:
										{
											int iWPortSubSelected = miModuleList[n].SettingList[i].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;												
											
											int iSpeed;
											memset(usbio_port_options, 0, 4096);
											for (k = 0; k < 63; k++)
											{
												iSpeed = get_uart_code_to_speed(k);
												if ((iSpeed == 0) && k) break;
												memset(mBuff, 0, 4096);
												sprintf(mBuff, "<option %s%s value='%i'>%i</option>\r\n", 
														k ? "" : " disabled ",
														(k == miModuleList[n].SettingList[i].PortSpeedCode) ? " selected " : "",
														k, iSpeed);
												strcat(usbio_port_options, mBuff);
											}
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: <input type='number' name='SensSett1' min=0 max=127 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Датчик: <input type='number' name='SensSett2' min=1 max=32 value='%i' style='width: 100px;'><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Скорость: <select name='SensSett3' style='width: 100px;'>%s</select><br />\r\n"															
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' name='SensSett4'%s><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправка: <input type='number' name='SensSett5' min=1 max=%i value='%i' style='width: 100px;'>(%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): <input type='number' name='SensSett6' min=0 max=127 value='%i' style='width: 100px;'>us<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим управления: <select name='SensSett7' style='width: 100px;'>\r\n"
															"		<option %s value='0'>Операциями</option>\r\n"
															"		<option %s value='1'>Передача/Прием</option>\r\n"
															"	</select><br />\r\n",
															miModuleList[n].SettingList[i].Address,
															miModuleList[n].SettingList[i].ResultType,
															usbio_port_options,
															miModuleList[n].SettingList[i].IOWControlUse ? " checked " : "",
															miModuleList[n].SettingsCount, miModuleList[n].SettingList[i].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].SettingList[i].ImpulseLen * 100,
															(miModuleList[n].SettingList[i].IOModeControl == 0) ? "selected" : "",
															(miModuleList[n].SettingList[i].IOModeControl != 0) ? "selected" : "");
																																							
											strcat(msg_subbody, mBuff);											
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485_PN532:
									case USBIO_PIN_SETT_TYPE_RS485_RC522:
										{
											int iWPortSubSelected = miModuleList[n].SettingList[i].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;	
											int iRPortSubSelected = miModuleList[n].SettingList[i].IORControlPort;
											if (iRPortSubSelected < miModuleList[n].SettingsCount)
												iRPortSubSelected = miModuleList[n].SettingList[iRPortSubSelected].ID;
												else
												iRPortSubSelected = 0;												
											
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' name='SensSett2'%s><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправка: <input type='number' name='SensSett4' min=1 max=%i value='%i' style='width: 100px;'>(%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления прием: <input type='checkbox' name='SensSett3'%s><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт приема: <input type='number' name='SensSett5' min=1 max=%i value='%i' style='width: 100px;'>(%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): <input type='number' name='SensSett6' min=0 max=127 value='%i' style='width: 100px;'>us<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим управления: <select name='SensSett8' style='width: 100px;'>\r\n"
															"		<option %s value='0'>Операциями</option>\r\n"
															"		<option %s value='1'>Передача/Прием</option>\r\n"
															"	</select><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка до повторного считывания: <input type='number' name='SensSett7' min=0 max=3 value='%i' style='width: 100px;'>s<br />\r\n",
															miModuleList[n].SettingList[i].IOWControlUse ? " checked " : "",
															miModuleList[n].SettingsCount, miModuleList[n].SettingList[i].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].SettingList[i].IORControlUse ? " checked " : "",
															miModuleList[n].SettingsCount, miModuleList[n].SettingList[i].IORControlPort + 1, 
															miModuleList[n].ParamList[iRPortSubSelected].Port + 65, miModuleList[n].ParamList[iRPortSubSelected].Number,
															miModuleList[n].SettingList[i].ImpulseLen*100,
															(miModuleList[n].SettingList[i].IOModeControl == 0) ? "selected" : "",
															(miModuleList[n].SettingList[i].IOModeControl != 0) ? "selected" : "",
															miModuleList[n].SettingList[i].TimeSkip);																								
											strcat(msg_subbody, mBuff);											
										}
										break;											
									default:
										break;
								}	
							}
							else
							{
								memset(mBuff, 0, 4096);
								sprintf(mBuff, "<font color='%s'>[%i] Порт: %c%i:<br />\r\n"
										"&ensp;&ensp;Сохраненные настройки:<br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Вкл.: <input type='checkbox' %s disabled><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Вкл. по питанию: <input type='checkbox' %s disabled><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Тип: %s<br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Интерв. скан.: %i (%is)<br />\r\n",
										cColor, i + 1, miModuleList[n].ParamList[iPortSelected].Port + 65, miModuleList[n].ParamList[iPortSelected].Number,
										miModuleList[n].SettingList[i].Enabled ? " checked " : "",
										miModuleList[n].SettingList[i].PwrInit ? " checked " : "",
										usb_gpio_get_type_name(miModuleList[n].SettingList[i].CurrentType),
										miModuleList[n].SettingList[i].Interval * 50, miModuleList[n].SettingList[i].Interval * 5 / 100);
								strcat(msg_subbody, mBuff);
								
								switch(miModuleList[n].SettingList[i].CurrentType)
								{
									case USBIO_PIN_SETT_TYPE_INPUT:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Подтяжка: %s<br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Инверт.: <input type='checkbox' %s disabled><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Аналоговый: <input type='checkbox' %s disabled><br />\r\n"															
														"&ensp;&ensp;&ensp;&ensp;Разрядность: %i<br />\r\n",
														usb_gpio_get_pull_name(miModuleList[n].SettingList[i].Pull),
														miModuleList[n].SettingList[i].Inverted ? " checked " : "",
														miModuleList[n].SettingList[i].Analog ? " checked " : "",
														miModuleList[n].SettingList[i].Accuracy);	
											strcat(msg_subbody, mBuff);
										}
										break;									
									case USBIO_PIN_SETT_TYPE_OUTPUT:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Подтяжка: %s<br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Значение по умолч.: %i<br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Инверт.: <input type='checkbox' %s disabled><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Режим: %s<br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Импульс.: %i (%is)<br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;Open drain: <input type='checkbox' %s disabled><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;ШИМ: <input type='checkbox' %s disabled><br />\r\n"
														"&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;Частота ШИМ: %i Hz<br />\r\n",
														usb_gpio_get_pull_name(miModuleList[n].SettingList[i].PortSpeedCode),
														miModuleList[n].SettingList[i].DefaultValue,
														miModuleList[n].SettingList[i].Inverted ? " checked " : "", 
														usb_gpio_get_mode_name(miModuleList[n].SettingList[i].Mode),
														miModuleList[n].SettingList[i].ImpulseLen * 50, miModuleList[n].SettingList[i].ImpulseLen * 5 / 100,
														miModuleList[n].SettingList[i].OpenDrain ? " checked " : "", 
														miModuleList[n].SettingList[i].PWM ? " checked " : "",
														miModuleList[n].SettingList[i].PortSpeedCode * 200);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AM2320:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Датчик: %s<br />\r\n", 
													miModuleList[n].SettingList[i].ResultType ? "Влажности" : "Температуры");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MISC:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Датчик: %i из %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Скорость: %i<br />\r\n",
															miModuleList[n].SettingList[i].Address,
															miModuleList[n].SettingList[i].ResultType, miModuleList[n].SettingList[i].DefaultValue,
															get_i2c_code_to_speed(miModuleList[n].SettingList[i].PortSpeedCode));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_LM75:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n", miModuleList[n].SettingList[i].Address);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_ADS1015:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Диапазон: %s<br />\r\n"
															, miModuleList[n].SettingList[i].Address,
															Get_ADS1015_RangeName(miModuleList[n].SettingList[i].Gain));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MCP3421:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Точность: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Усиление: %s<br />\r\n"
															, miModuleList[n].SettingList[i].Address,
															Get_MCP3421_AccuracyName(miModuleList[n].SettingList[i].Accuracy),
															Get_MCP3421_GainName(miModuleList[n].SettingList[i].Gain));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AS5600:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Тип: %s<br />\r\n"
															, miModuleList[n].SettingList[i].Address,
															miModuleList[n].SettingList[i].ResultType ? "RAW" : "Норм.");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_HMC5883L:
										{
											memset(mBuff, 0, 4096);
											switch(miModuleList[n].SettingList[i].ResultType)
											{
												case 0: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: X raw<br />\r\n"); break;
												case 1: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Y raw<br />\r\n"); break;
												case 2: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Z raw<br />\r\n"); break;
												case 3: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: X в градусах<br />\r\n"); break;
												case 4: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Y в градусах<br />\r\n"); break;
												case 5: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Z в градусах<br />\r\n"); break;
												default: break;
											}											
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_IR_RECIEVER:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Время пропуска повторных команд: %i (%is)<br />\r\n",
														miModuleList[n].SettingList[i].TimeSkip * 50, miModuleList[n].SettingList[i].TimeSkip * 5 / 100);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485:
										{
											int iWPortSubSelected = miModuleList[n].SettingList[i].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;											
											int iRPortSubSelected = miModuleList[n].SettingList[i].IORControlPort;
											if (iRPortSubSelected < miModuleList[n].SettingsCount)
												iRPortSubSelected = miModuleList[n].SettingList[iRPortSubSelected].ID;
												else
												iRPortSubSelected = 0;											
												
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Скорость: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправки: %i (%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления прием: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт приема: %i (%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): %ius<br />\r\n"															
															"&ensp;&ensp;&ensp;&ensp;Режим управления: %s<br />\r\n",
															get_uart_code_to_speed(miModuleList[n].SettingList[i].PortSpeedCode),
															miModuleList[n].SettingList[i].IOWControlUse ? " checked " : "",
															miModuleList[n].SettingList[i].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].SettingList[i].IORControlUse ? " checked " : "",
															miModuleList[n].SettingList[i].IORControlPort + 1, 
															miModuleList[n].ParamList[iRPortSubSelected].Port + 65, miModuleList[n].ParamList[iRPortSubSelected].Number,
															miModuleList[n].SettingList[i].ImpulseLen * 100,
															miModuleList[n].SettingList[i].IOModeControl ? "Прием/Передача" : "Операциями");
											strcat(msg_subbody, mBuff);											
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485_MISC:
										{
											int iWPortSubSelected = miModuleList[n].SettingList[i].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;			
												
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Датчик: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Скорость: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправки: %i (%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): %ius<br />\r\n"															
															"&ensp;&ensp;&ensp;&ensp;Режим управления: %s<br />\r\n",
															miModuleList[n].SettingList[i].Address,
															miModuleList[n].SettingList[i].ResultType,
															get_uart_code_to_speed(miModuleList[n].SettingList[i].PortSpeedCode),
															miModuleList[n].SettingList[i].IOWControlUse ? " checked " : "",
															miModuleList[n].SettingList[i].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].SettingList[i].ImpulseLen * 100,
															miModuleList[n].SettingList[i].IOModeControl ? "Прием/Передача" : "Операциями");
											strcat(msg_subbody, mBuff);											
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485_PN532:
									case USBIO_PIN_SETT_TYPE_RS485_RC522:	
										{
											int iWPortSubSelected = miModuleList[n].SettingList[i].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;											
											int iRPortSubSelected = miModuleList[n].SettingList[i].IORControlPort;
											if (iRPortSubSelected < miModuleList[n].SettingsCount)
												iRPortSubSelected = miModuleList[n].SettingList[iRPortSubSelected].ID;
												else
												iRPortSubSelected = 0;											
												
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправки: %i (%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления прием: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт приема: %i (%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): %ius<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим управления: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка до повторного считывания: %is<br />\r\n",														
															miModuleList[n].SettingList[i].IOWControlUse ? " checked " : "",
															miModuleList[n].SettingList[i].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].SettingList[i].IORControlUse ? " checked " : "",
															miModuleList[n].SettingList[i].IORControlPort + 1, 
															miModuleList[n].ParamList[iRPortSubSelected].Port + 65, miModuleList[n].ParamList[iRPortSubSelected].Number,
															miModuleList[n].SettingList[i].ImpulseLen * 100,
															miModuleList[n].SettingList[i].IOModeControl ? "Прием/Передача" : "Операциями",
															miModuleList[n].SettingList[i].TimeSkip & 0b11);
											strcat(msg_subbody, mBuff);											
										}
										break;									
									default:
										break;
								}	
							}
							//if (miModuleList[n].Local && ((miModuleList[n].Enabled & 4) == 0))
							{
								memset(mBuff, 0, 4096);
								sprintf(mBuff, "&ensp;&ensp;Текущие настройки:<br />\r\n"
												"&ensp;&ensp;&ensp;&ensp;Вкл.: <input type='checkbox' %s disabled><br />\r\n"
												"&ensp;&ensp;&ensp;&ensp;Вкл. по питанию: <input type='checkbox' %s disabled><br />\r\n"
												"&ensp;&ensp;&ensp;&ensp;Тип: %s<br />\r\n"
												"&ensp;&ensp;&ensp;&ensp;Интерв. скан.: %i (%is)<br />\r\n",
												miModuleList[n].ParamList[iPortSelected].Enabled ? " checked " : "", 
												miModuleList[n].ParamList[iPortSelected].PwrInit ? " checked " : "", 
												usb_gpio_get_type_name(miModuleList[n].ParamList[iPortSelected].CurrentType),
												miModuleList[n].ParamList[iPortSelected].Interval * 50, miModuleList[n].ParamList[iPortSelected].Interval * 5 / 100);	
								strcat(msg_subbody, mBuff);
								
								switch(miModuleList[n].ParamList[iPortSelected].CurrentType)
								{
									case USBIO_PIN_SETT_TYPE_INPUT:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Подтяжка: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Инверт.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Аналоговый: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Разрядность: %i<br />\r\n",
															usb_gpio_get_pull_name(miModuleList[n].ParamList[iPortSelected].Pull),
															miModuleList[n].ParamList[iPortSelected].Inverted ? " checked " : "",
															miModuleList[n].ParamList[iPortSelected].Analog ? " checked " : "",
															miModuleList[n].ParamList[iPortSelected].Accuracy);	
											strcat(msg_subbody, mBuff);
										}
										break;									
									case USBIO_PIN_SETT_TYPE_OUTPUT:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Подтяжка: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Значение по умолч.: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Инверт.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Импульс.: %i (%is)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Open drain: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;ШИМ: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;Частота ШИМ: %i Hz<br />\r\n",
															usb_gpio_get_pull_name(miModuleList[n].ParamList[iPortSelected].Pull),
															miModuleList[n].ParamList[iPortSelected].DefaultValue,
															miModuleList[n].ParamList[iPortSelected].Inverted ? " checked " : "", 
															usb_gpio_get_mode_name(miModuleList[n].ParamList[iPortSelected].Mode),
															miModuleList[n].ParamList[iPortSelected].ImpulseLen * 50, miModuleList[n].ParamList[iPortSelected].ImpulseLen * 5 / 100,
															miModuleList[n].ParamList[iPortSelected].OpenDrain ? " checked " : "", 
															miModuleList[n].ParamList[iPortSelected].PWM ? " checked " : "",
															miModuleList[n].ParamList[iPortSelected].PortSpeedCode * 200);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AM2320:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Датчик: %s<br />\r\n", 
													miModuleList[n].ParamList[iPortSelected].ResultType ? "Влажности" : "Температуры");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MISC:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Датчик: %i из %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Скорость: %i<br />\r\n",
															miModuleList[n].ParamList[iPortSelected].Address,
															miModuleList[n].ParamList[iPortSelected].ResultType, miModuleList[n].ParamList[iPortSelected].DefaultValue,
															get_i2c_code_to_speed(miModuleList[n].ParamList[iPortSelected].PortSpeedCode));
											strcat(msg_subbody, mBuff);
										}
										break;																			
									case USBIO_PIN_SETT_TYPE_I2C_LM75:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n", miModuleList[n].ParamList[iPortSelected].Address);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_ADS1015:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Диапазон: %s<br />\r\n"
															, miModuleList[n].ParamList[iPortSelected].Address,
															Get_ADS1015_RangeName(miModuleList[n].ParamList[iPortSelected].Gain));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_MCP3421:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Точность: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Усиление: %s<br />\r\n"
															, miModuleList[n].ParamList[iPortSelected].Address,
															Get_MCP3421_AccuracyName(miModuleList[n].ParamList[iPortSelected].Accuracy),
															Get_MCP3421_GainName(miModuleList[n].ParamList[iPortSelected].Gain));
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_AS5600:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Тип: %s<br />\r\n"
															, miModuleList[n].ParamList[iPortSelected].Address,
															miModuleList[n].ParamList[iPortSelected].ResultType ? "RAW" : "Норм.");
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_I2C_HMC5883L:
										{
											memset(mBuff, 0, 4096);
											switch(miModuleList[n].ParamList[iPortSelected].ResultType)
											{
												case 0: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: X raw<br />\r\n"); break;
												case 1: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Y raw<br />\r\n"); break;
												case 2: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Z raw<br />\r\n"); break;
												case 3: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: X в градусах<br />\r\n"); break;
												case 4: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Y в градусах<br />\r\n"); break;
												case 5: sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Параметр: Z в градусах<br />\r\n"); break;
												default: break;
											}											
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_IR_RECIEVER:
										{
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Время пропуска повторных команд: %i (%is)<br />\r\n",
														miModuleList[n].ParamList[iPortSelected].TimeSkip * 50, miModuleList[n].ParamList[iPortSelected].TimeSkip * 5 / 100);
											strcat(msg_subbody, mBuff);
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485:
										{
											int iWPortSubSelected = miModuleList[n].ParamList[iPortSelected].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;											
											if (miModuleList[n].ParamList[iPortSelected].IOWControlPort > miModuleList[n].ParamsCount)
												miModuleList[n].ParamList[iPortSelected].IOWControlPort = 0;
											
											int iRPortSubSelected = miModuleList[n].ParamList[iPortSelected].IORControlPort;
											if (iRPortSubSelected < miModuleList[n].SettingsCount)
												iRPortSubSelected = miModuleList[n].SettingList[iRPortSubSelected].ID;
												else
												iRPortSubSelected = 0;											
											if (miModuleList[n].ParamList[iPortSelected].IORControlPort > miModuleList[n].ParamsCount)
												miModuleList[n].ParamList[iPortSelected].IORControlPort = 0;
											
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Скорость: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправки: %c%i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления прием: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт приема: %c%i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): %ius<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим управления: %s<br />\r\n",
															get_uart_code_to_speed(miModuleList[n].ParamList[iPortSelected].PortSpeedCode),
															miModuleList[n].ParamList[iPortSelected].IOWControlUse ? " checked " : "",
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, 
															miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].ParamList[iPortSelected].IORControlUse ? " checked " : "",
															miModuleList[n].ParamList[iRPortSubSelected].Port + 65, 
															miModuleList[n].ParamList[iRPortSubSelected].Number,
															miModuleList[n].ParamList[iPortSelected].ImpulseLen * 100,
															miModuleList[n].ParamList[iPortSelected].IOModeControl ? "Прием/Передача" : "Операциями");
											strcat(msg_subbody, mBuff);											
										}
										break;
									case USBIO_PIN_SETT_TYPE_RS485_MISC:
										{
											int iWPortSubSelected = miModuleList[n].ParamList[iPortSelected].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;											
											if (miModuleList[n].ParamList[iPortSelected].IOWControlPort > miModuleList[n].ParamsCount)
												miModuleList[n].ParamList[iPortSelected].IOWControlPort = 0;
											
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Адрес: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Датчик: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Скорость: %i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправки: %i (%c%i)<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): %ius<br />\r\n"															
															"&ensp;&ensp;&ensp;&ensp;Режим управления: %s<br />\r\n",
															miModuleList[n].ParamList[iPortSelected].Address,
															miModuleList[n].ParamList[iPortSelected].ResultType,
															get_uart_code_to_speed(miModuleList[n].ParamList[iPortSelected].PortSpeedCode),
															miModuleList[n].ParamList[iPortSelected].IOWControlUse ? " checked " : "",
															miModuleList[n].ParamList[iPortSelected].IOWControlPort + 1, 
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].ParamList[iPortSelected].ImpulseLen * 100,
															miModuleList[n].ParamList[iPortSelected].IOModeControl ? "Прием/Передача" : "Операциями");
											strcat(msg_subbody, mBuff);											
										}
										break;									
									case USBIO_PIN_SETT_TYPE_RS485_PN532:
									case USBIO_PIN_SETT_TYPE_RS485_RC522:
										{
											int iWPortSubSelected = miModuleList[n].ParamList[iPortSelected].IOWControlPort;
											if (iWPortSubSelected < miModuleList[n].SettingsCount)
												iWPortSubSelected = miModuleList[n].SettingList[iWPortSubSelected].ID;
												else
												iWPortSubSelected = 0;											
											if (miModuleList[n].ParamList[iPortSelected].IOWControlPort > miModuleList[n].ParamsCount)
												miModuleList[n].ParamList[iPortSelected].IOWControlPort = 0;
											
											int iRPortSubSelected = miModuleList[n].ParamList[iPortSelected].IORControlPort;
											if (iRPortSubSelected < miModuleList[n].SettingsCount)
												iRPortSubSelected = miModuleList[n].SettingList[iRPortSubSelected].ID;
												else
												iRPortSubSelected = 0;											
											if (miModuleList[n].ParamList[iPortSelected].IORControlPort > miModuleList[n].ParamsCount)
												miModuleList[n].ParamList[iPortSelected].IORControlPort = 0;
											
											memset(mBuff, 0, 4096);
											sprintf(mBuff, "&ensp;&ensp;&ensp;&ensp;Использовать порт для управления отпр.: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт отправки: %c%i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Использовать порт для управления прием: <input type='checkbox' %s disabled><br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Управляющий порт приема: %c%i<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка управления (до и после): %ius<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Режим управления: %s<br />\r\n"
															"&ensp;&ensp;&ensp;&ensp;Задержка до повторного считывания: %is<br />\r\n",	
															miModuleList[n].ParamList[iPortSelected].IOWControlUse ? " checked " : "",
															miModuleList[n].ParamList[iWPortSubSelected].Port + 65, 
															miModuleList[n].ParamList[iWPortSubSelected].Number,
															miModuleList[n].ParamList[iPortSelected].IORControlUse ? " checked " : "",
															miModuleList[n].ParamList[iRPortSubSelected].Port + 65, 
															miModuleList[n].ParamList[iRPortSubSelected].Number,
															miModuleList[n].ParamList[iPortSelected].ImpulseLen*100,
															miModuleList[n].ParamList[iPortSelected].IOModeControl ? "Прием/Передача" : "Операциями",
															miModuleList[n].ParamList[iPortSelected].TimeSkip);
											strcat(msg_subbody, mBuff);											
										}
										break;
									default:
										break;
								}	
								strcat(msg_subbody, "</font>\r\n");
							}
						
							cnt = strlen(msg_subbody);
							if ((cnt + 4096) > uiBuffSize)
							{							
								uiBuffSize += 16384;
								msg_subbody = (char*)DBG_REALLOC(msg_subbody, uiBuffSize);
								memset(&msg_subbody[cnt], 0, uiBuffSize - cnt);
								if (uiBuffSize > 4000000)
								{
									strcat(msg_subbody, "<font color='Red'>...</font>");
									break;
								}
							}
							uiSettCnt += 2;
							if (uiSettCnt >= MAX_MODULE_SETTINGS)
							{
								strcat(msg_subbody, "<font color='Red'>...</font>");
								dbgprintf(2, "Very big count USBIO submodules, low size settings\n");
								break;
							}
						}
					}
					strcat(msg_subbody, "</td>\r\n");
					break;
				case MODULE_TYPE_RTC:
					{
						struct tm timeinfo;
						i2c_read_spec_timedate3231(I2C_ADDRESS_CLOCK, &timeinfo);
						sprintf(msg_subbody, "<td>Скорость I2C: <input type='number' name='Speed' min=0 value='%i' style='width: 100px;'%s><br />\r\n"
										"Дата: <input type='number' name='DateDay' min=1 max=31 value='%i' style='width: 50px;'%s>\r\n"
										".<input type='number' name='DateMont' min=1 max=12 value='%i' style='width: 50px;'%s>\r\n"
										".<input type='number' name='DateYear' min=2000 max=2159 value='%i' style='width: 100px;'%s>\r\n"
										"   <input type='number' name='TimeHour' min=0 max=24 value='%i' style='width: 50px;'%s>\r\n"
										":<input type='number' name='TimeMin' min=0 max=59 value='%i' style='width: 50px;'%s>\r\n"
										":<input type='number' name='TimeSec' min=0 max=59 value='%i' style='width: 50px;'%s></td>\r\n",
										miModuleList[n].Settings[2], pDisableFlag,
										timeinfo.tm_mday, pDisableFlag,
										timeinfo.tm_mon+1, pDisableFlag,
										timeinfo.tm_year+1900, pDisableFlag,
										timeinfo.tm_hour, pDisableFlag,
										timeinfo.tm_min, pDisableFlag,
										timeinfo.tm_sec, pDisableFlag);
					}
					break;
				case MODULE_TYPE_RADIO:
					sprintf(msg_subbody, "<td>Скорость I2C:<input type='number' name='Speed' min=0 value='%i' style='width: 100px;'%s></td>\r\n",
										miModuleList[n].Settings[2], pDisableFlag);
					break;
				case MODULE_TYPE_TFP625A:
				case MODULE_TYPE_PN532:
				case MODULE_TYPE_RC522:
					WEB_get_uart_select_block(mBuff, 4000, miModuleList[n].Settings[1], pDisableFlag);				
					sprintf(msg_subbody, "<td>Порт:%s<br />\r\n"
										"Пауза при управлении: <input type='number' name='PtSl' min=0 max=1000000 value='%i' style='width: 100px;'%s>us<br />\r\n"
										"ИД модуля управления(I/O):<input type='text' list='gpiolist' name='SubID' value='%.4s' maxlength=4 style='width: 60px;'%s></td>\r\n",
										mBuff,
										miModuleList[n].Settings[4], pDisableFlag,
										(char*)&miModuleList[n].Settings[0], pDisableFlag);
					break;					
				case MODULE_TYPE_RS485:
					WEB_get_uart_select_block(mBuff, 4000, miModuleList[n].Settings[1], pDisableFlag);
					sprintf(msg_subbody, "<td>%s<br />\r\n"
										"Скорость UART:<input type='number' name='Speed' min=0 value='%i' style='width: 100px;'%s><br />\r\n"
										"Пауза при управлении: <input type='number' name='PtSl' min=0 max=1000000 value='%i' style='width: 100px;'%s>us<br />\r\n"
										"ИД(I/O):<input type='text' list='gpiolist' name='SubID' value='%.4s' maxlength=4 style='width: 60px;'%s><br />\r\n",
										mBuff,
										miModuleList[n].Settings[3], pDisableFlag,
										miModuleList[n].Settings[4], pDisableFlag,
										(char*)&miModuleList[n].Settings[0], pDisableFlag);
					break;
				case MODULE_TYPE_IR_RECEIVER:
					sprintf(msg_subbody, "<td>Время пропуска следующих команд (x50ms):<input type='number' name='ScanClk' min=1 value='%i' style='width: 100px;'%s><br />\r\n"
										"Номер контакта:<input type='number' name='PinNum' min=0 max=80 value='%i' maxlength=2 style='width: 60px;'%s></td>\r\n",
										miModuleList[n].Settings[1], pDisableFlag,
										miModuleList[n].Settings[2], pDisableFlag);
					break;
				case MODULE_TYPE_TIMER:
					fl1 = (int)floor((float)miModuleList[n].Settings[1]/10000);
					fl2 = (int)floor((float)miModuleList[n].Settings[1]/100);
					sprintf(msg_subbody, "<td>Пн:<input type='checkbox' name='Mo'%s%s>\r\n"
											"  Вт:<input type='checkbox' name='Tu'%s%s>\r\n"
											"  Ср:<input type='checkbox' name='We'%s%s>\r\n"
											"  Чт:<input type='checkbox' name='Th'%s%s>\r\n"
											"  Пт:<input type='checkbox' name='Fr'%s%s>\r\n"
											"  Сб:<input type='checkbox' name='Sa'%s%s>\r\n"
											"  Вс:<input type='checkbox' name='Su'%s%s><br />\r\n"
											"Время:<input type='number' name='Hour' min=0 max=24 value='%i' style='width: 37px;'%s>\r\n"
											":<input type='number' name='Min' min=0 max=59 value='%i' style='width: 37px;'%s>\r\n"
											":<input type='number' name='Sec' min=0 max=59 value='%i' style='width: 37px;'%s></td>\r\n",
											miModuleList[n].Settings[0] & 1 ? " checked" : "", pDisableFlag,
											miModuleList[n].Settings[0] & 2 ? " checked" : "", pDisableFlag,
											miModuleList[n].Settings[0] & 4 ? " checked" : "", pDisableFlag,
											miModuleList[n].Settings[0] & 8 ? " checked" : "", pDisableFlag,
											miModuleList[n].Settings[0] & 16 ? " checked" : "", pDisableFlag,
											miModuleList[n].Settings[0] & 32 ? " checked" : "", pDisableFlag,
											miModuleList[n].Settings[0] & 64 ? " checked" : "", pDisableFlag,
											fl1, pDisableFlag,
											fl2 - (fl1*100), pDisableFlag,
											miModuleList[n].Settings[1] - (fl2*100), pDisableFlag);
					break;	
				case MODULE_TYPE_SYSTEM:
					sprintf(msg_subbody, "<td>Версия: %i.%i.%i.%i<br />\r\n"
											"Доступ к файлам: %s<br />"
											"Требуемый уровень для доступа к последним файлам: %i<br />"
											"Требуемый уровень для доступа к архивным файлам: %i<br />"
											"Образцовая дата и время: %s<br />"
											"</td>\r\n",
										miModuleList[n].Version[0],
										miModuleList[n].Version[1],
										miModuleList[n].Version[2],
										miModuleList[n].Version[3],
										miModuleList[n].Settings[4] ? "Включен" : "Отключен",
										miModuleList[n].Settings[5],
										miModuleList[n].Settings[6],
										miModuleList[n].Settings[0] ? "Да" : "Нет");
					break;
				case MODULE_TYPE_DISPLAY:
				case MODULE_TYPE_COUNTER:
				case MODULE_TYPE_MEMORY:
					sprintf(msg_subbody, "<td>Сбрасывать через время (ms)<br />\r\n"
											"<input type='checkbox' name='Res1'%s%s>\r\n"
												"<input type='number' name='Set1' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res2'%s%s>\r\n"
												"<input type='number' name='Set2' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res3'%s%s>\r\n"
												"<input type='number' name='Set3' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res4'%s%s>\r\n"
												"<input type='number' name='Set4' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res5'%s%s>\r\n"
												"<input type='number' name='Set5' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res6'%s%s>\r\n"
												"<input type='number' name='Set6' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res7'%s%s>\r\n"
												"<input type='number' name='Set7' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='checkbox' name='Res8'%s%s>\r\n"
												"<input type='number' name='Set8' value='%i' style='width: 100px;'%s></td>\r\n",
										((miModuleList[n].Settings[0] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[0] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[1] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[1] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[2] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[2] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[3] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[3] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[4] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[4] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[5] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[5] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[6] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[6] & 0x00FFFFFF, pDisableFlag,
										((miModuleList[n].Settings[7] >> 24) == 1) ? " checked" : "", pDisableFlag,
										miModuleList[n].Settings[7] & 0x00FFFFFF, pDisableFlag);
					break;
				case MODULE_TYPE_GPIO:
					sprintf(msg_subbody, "<td>Режим:<select name='Mode' style='width: 140px;'%s>\r\n"
												"		<option %s value='0'>Input</option>\r\n"
												"		<option %s value='1'>Output</option>\r\n"
												"	</select>\r\n"
										"<br />Инверсный:<input type='checkbox' name='Inverse'%s%s>\r\n"
										"Номер контакта:<input type='number' name='PinNum' min=0 max=80 value='%i' maxlength=2 style='width: 60px;'%s><br />\r\n"
										"Время вкл.(мс) (0=пост.):<input type='number' name='Time' min=0 max=20000000 value='%i' maxlength=5 style='width: 100px;'%s></td>\r\n",
										pDisableFlag,
										((miModuleList[n].Settings[0] & MODULE_SECSET_OUTPUT) == 0) ? " selected" : "", 
										(miModuleList[n].Settings[0] & MODULE_SECSET_OUTPUT) ? " selected" : "", 
										(miModuleList[n].Settings[0] & MODULE_SECSET_INVERSE) ? " checked" : "", 
										pDisableFlag,
										miModuleList[n].Settings[1], pDisableFlag,
										miModuleList[n].Settings[2], pDisableFlag);
					break;
				case MODULE_TYPE_EXTERNAL:
					WEB_get_uart_select_block(mBuff, 4000, miModuleList[n].Settings[3], pDisableFlag);				
					sprintf(msg_subbody, "<td>Тип подключения:<select name='TpConn' style='width: 140px;'%s>\r\n"
												"		<option %s value='%i'>Disabled</option>\r\n"
												"		<option %s value='%i'>I2C</option>\r\n"
												"		<option %s value='%i'>UART</option>\r\n"
												"	</select><br />\r\n"
										"Адрес:<input type='number' name='Addr' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"
										"Скорость порта:<input type='number' name='Speed' min=0 value='%i' style='width: 100px;'%s><br />"
										"UART порт:%s<br />\r\n"
										"Количество сенсоров:%i<br />\r\n"
										"</td>\r\n",
										pDisableFlag,
										((miModuleList[n].Settings[0] != 1) && (miModuleList[n].Settings[0] != 2)) ? " selected" : "", 1,
										(miModuleList[n].Settings[0] == 1) ? " selected" : "", 1,
										(miModuleList[n].Settings[0] == 2) ? " selected" : "", 2,
										miModuleList[n].Settings[1], pDisableFlag,	//Адрес
										miModuleList[n].Settings[2], pDisableFlag,	//Скорость
										mBuff,										//Номер порта
										miModuleList[n].Settings[5]);				//Количество субмодулей
					break;
				case MODULE_TYPE_TEMP_SENSOR:
					sprintf(msg_subbody, "<td>Тип:<select name='Model' style='width: 140px;'%s>\r\n"
												"		<option %s value='%i'>AM2320</option>\r\n"
												"		<option %s value='%i'>LM75</option>\r\n"
												"	</select>\r\n"
										"<br />Адрес:<input type='number' name='Address' min=0 max=7 value='%i' maxlength=1 style='width: 50px;'%s><br />\r\n"
										"</td>\r\n",
										pDisableFlag,
										(miModuleList[n].Settings[0] == I2C_ADDRESS_AM2320) ? " selected" : "", I2C_ADDRESS_AM2320,
										(miModuleList[n].Settings[0] == I2C_ADDRESS_LM75) ? " selected" : "", I2C_ADDRESS_LM75,
										miModuleList[n].Settings[1], pDisableFlag);
					break;
				case MODULE_TYPE_MCP3421:
					sprintf(msg_subbody, "<td>Адрес:<input type='number' name='Address' min=0 max=7 value='%i' maxlength=1 style='width: 50px;'%s>\r\n"
										"<br />Точность:<select name='Resolution' style='width: 140px;'%s>\r\n"
										"		<option %s value=0>%s</option>\r\n"
										"		<option %s value=1>%s</option>\r\n"
										"		<option %s value=2>%s</option>\r\n"
										"		<option %s value=3>%s</option>\r\n"
										"	</select><br />\r\n"
										"<br />Усиление:<select name='Gain' style='width: 140px;'%s>\r\n"
										"		<option %s value=0>%s</option>\r\n"
										"		<option %s value=1>%s</option>\r\n"
										"		<option %s value=2>%s</option>\r\n"
										"		<option %s value=3>%s</option>\r\n"
										"	</select><br />\r\n"
										"</td>\r\n",
										miModuleList[n].Settings[0], pDisableFlag,
										pDisableFlag,
										(miModuleList[n].Settings[1] == 0) ? " selected" : "", Get_MCP3421_AccuracyName(0),
										(miModuleList[n].Settings[1] == 1) ? " selected" : "", Get_MCP3421_AccuracyName(1),
										(miModuleList[n].Settings[1] == 2) ? " selected" : "", Get_MCP3421_AccuracyName(2),
										(miModuleList[n].Settings[1] == 3) ? " selected" : "", Get_MCP3421_AccuracyName(3),
										pDisableFlag,
										(miModuleList[n].Settings[2] == 0) ? " selected" : "", Get_MCP3421_GainName(0),
										(miModuleList[n].Settings[2] == 1) ? " selected" : "", Get_MCP3421_GainName(1),
										(miModuleList[n].Settings[2] == 2) ? " selected" : "", Get_MCP3421_GainName(2),
										(miModuleList[n].Settings[2] == 3) ? " selected" : "", Get_MCP3421_GainName(3));
					break;
				case MODULE_TYPE_ADS1015:
					sprintf(msg_subbody, "<td>Адрес:<input type='number' name='Address' min=0 max=3 value='%i' maxlength=1 style='width: 50px;'%s>\r\n"
										"<br />Усиление:<select name='Gain' style='width: 140px;'%s>\r\n"
										"		<option %s value=0>%s</option>\r\n"
										"		<option %s value=1>%s</option>\r\n"
										"		<option %s value=2>%s</option>\r\n"
										"		<option %s value=3>%s</option>\r\n"
										"		<option %s value=4>%s</option>\r\n"
										"		<option %s value=5>%s</option>\r\n"
										"		<option %s value=6>%s</option>\r\n"
										"		<option %s value=7>%s</option>\r\n"
										"	</select><br />\r\n"
										"</td>\r\n",
										miModuleList[n].Settings[0], pDisableFlag,
										pDisableFlag,
										(miModuleList[n].Settings[2] == 0) ? " selected" : "", Get_ADS1015_RangeName(0),
										(miModuleList[n].Settings[2] == 1) ? " selected" : "", Get_ADS1015_RangeName(1),
										(miModuleList[n].Settings[2] == 2) ? " selected" : "", Get_ADS1015_RangeName(2),
										(miModuleList[n].Settings[2] == 3) ? " selected" : "", Get_ADS1015_RangeName(3),
										(miModuleList[n].Settings[2] == 4) ? " selected" : "", Get_ADS1015_RangeName(4),
										(miModuleList[n].Settings[2] == 5) ? " selected" : "", Get_ADS1015_RangeName(5),
										(miModuleList[n].Settings[2] == 6) ? " selected" : "", Get_ADS1015_RangeName(6),
										(miModuleList[n].Settings[2] == 7) ? " selected" : "", Get_ADS1015_RangeName(7));
					break;
				case MODULE_TYPE_AS5600:
					sprintf(msg_subbody, "<td>Адрес:<input type='number' name='Address' min=0 max=7 value='%i' maxlength=1 style='width: 50px;'%s>\r\n"
										"</td>\r\n",
										miModuleList[n].Settings[0], pDisableFlag);
					break;
				case MODULE_TYPE_HMC5883L:
					sprintf(msg_subbody, "<td></td>\r\n");
					break;
				case MODULE_TYPE_CAMERA:
					{
						image_sensor_params ispCSI;
						omx_get_image_sensor_module_params(&ispCSI, &miModuleList[n]);
						sprintf(msg_subbody, "<td>ИД(темп. мод.):<input type='text' list='templist' name='SubID' value='%.4s' maxlength=4 style='width: 60px;'%s><br />\r\n"
										"Ресурсы:<select name='Resources' style='width: 140px;'%s>\r\n"
										"		<option %s value='%i'>Разделять</option>\r\n"
										"		<option %s value='%i'>Забирать</option>\r\n"
										"		<option %s value='%i'>Отдавать</option>\r\n"
										"	</select><br />\r\n"
										"Экспозиция:<select name='Exposure' style='width: 140px;'%s>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"	</select><br />\r\n"
										"Фильтр:<select name='Filter' style='width: 140px;'%s>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"	</select><br />\r\n"
										"Баланс белого:<select name='Balance' style='width: 140px;'%s>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"		<option %s value='%i'>%s</option>\r\n"
										"	</select><br />\r\n"
										"Обрезка:<br />\r\n" 
											"&ensp;&ensp;Сохранять пропорции:<input type='checkbox' name='KeepRatio'%s%s><br />\r\n"
											"&ensp;&ensp;Слева:<input type='number' name='LeftCrop' min='0' max='100' step='1' value='%i' style='width: 100px;' %s><br />\r\n"
											"&ensp;&ensp;Справа:<input type='number' name='RightCrop' min='0' max='100' step='1' value='%i' style='width: 100px;' %s><br />\r\n"
											"&ensp;&ensp;Сверху:<input type='number' name='UpCrop' min='0' max='100' step='1' value='%i' style='width: 100px;' %s><br />\r\n"
											"&ensp;&ensp;Снизу:<input type='number' name='DownCrop' min='0' max='100' step='1' value='%i' style='width: 100px;' %s><br />\r\n"
										"Яркость: <input type='range' name='StartBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
										"Контраст: <input type='range' name='Contrast' min='-100' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
										"Резкость: <input type='range' name='Sharpness' min='-100' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
										"Насыщенность: <input type='range' name='Saturation' min='-100' max='100' step='1' value='%i' style='width: 300px;' %s>(%i)<br />\r\n"
										"Автояркость:<input type='checkbox' name='AutoBright'%s%s><br />\r\n"
											"&ensp;&ensp;Поддерживаемая яркость: <input type='range' name='DestBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
										"ISO: <input type='range' name='ISOvalue' min='100' max='800' step='1' value='%i' style='width: 400px;' %s><br />\r\n"
											"&ensp;&ensp;Автоматически (HW):<input type='checkbox' name='AutoISOhw'%s%s><br />\r\n"
											"&ensp;&ensp;Автоматически (SW):<input type='checkbox' name='AutoISOsw'%s%s><br />\r\n"
											"&ensp;&ensp;&ensp;&ensp;Поддерживаемая яркость: <input type='range' name='DestISOBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"										
										"EV компенсация: <input type='range' name='EVvalue' min='-24' max='24' step='1' value='%i' style='width: 400px;' %s><br />\r\n"										
											"&ensp;&ensp;Автоматически (SW):<input type='checkbox' name='AutoEVsw'%s%s><br />\r\n"
											"&ensp;&ensp;Поддерживаемая яркость: <input type='range' name='DestEVBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"										
										"Поворот:<select name='RotMode' style='width: 140px;'%s>\r\n"
										"		<option %s value='%i'>0</option>\r\n"
										"		<option %s value='%i'>90</option>\r\n"
										"		<option %s value='%i'>180</option>\r\n"
										"		<option %s value='%i'>270</option>\r\n"
										"	</select> градусов<br />\r\n"
										"Разворот по горизонтали<input type='checkbox' name='FlipHor'%s%s><br />\r\n"
										"Разворот по вертикали<input type='checkbox' name='FlipVer'%s%s><br />\r\n"
										"Режим подсветки:<input type='checkbox' name='HighLight'%s%s><br />\r\n"
											"&ensp;&ensp;Уровень яркости для активации: <input type='range' name='HLBright' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
											"&ensp;&ensp;Уровень усиления: <input type='range' name='HLAmplif' min='1' max='100' step='1' value='%i' style='width: 400px;' %s>(%i)<br />\r\n"
										
										"Разрешение:<input type='number' name='ResolCameraW' min=120 max=2048 value='%i' style='width: 100px;'%s>x\r\n"
										"		<input type='number' name='ResolCameraH' min=120 max=2048 value='%i' style='width: 100px;'%s><br />\r\n"
										
										"Частота кадров:<input type='number' name='MainFrameRate' min=2 max=190 value='%i' style='width: 100px;'%s><br />\r\n"
										"Интервал ключа:<input type='number' name='MainKeyFrame' min=0 value='%i' maxlength=3 style='width: 100px;'%s><br />\r\n"
										"Битрэйт (kbt/s):<input type='number' name='MainBitRate' min=1 value='%i' maxlength=5 style='width: 100px;'%s><br />\r\n"
										"Тип битрэйта:<select name='MainBitType' style='width: 140px;'%s>\r\n"
										"		<option %s value='0'>Переменный</option>\r\n"
										"		<option %s value='1'>Постоянный</option>\r\n"
										"	</select><br />\r\n"
										"Профиль кодека:<select name='MainAvcProfile' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Не указан</option>\r\n"
															"		<option %s value='1'>Baseline</option>\r\n"
															"		<option %s value='2'>Main</option>\r\n"
															"		<option %s value='3'>Extended</option>\r\n"
															"		<option %s value='4'>High</option>\r\n"
															"		<option %s value='5'>High10</option>\r\n"
															"		<option %s value='6'>High422</option>\r\n"
															"		<option %s value='7'>High444</option>\r\n"
															"	</select><br />\r\n"
										"Уровень кодека:<select name='MainAvcLevel' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Не указан</option>\r\n"
															"		<option %s value='1'>1</option>\r\n"
															"		<option %s value='2'>1b</option>\r\n"
															"		<option %s value='3'>11</option>\r\n"
															"		<option %s value='4'>12</option>\r\n"
															"		<option %s value='5'>13</option>\r\n"
															"		<option %s value='6'>2</option>\r\n"
															"		<option %s value='7'>21</option>\r\n"
															"		<option %s value='8'>22</option>\r\n"
															"		<option %s value='9'>3</option>\r\n"
															"		<option %s value='10'>31</option>\r\n"
															"		<option %s value='11'>32</option>\r\n"
															"		<option %s value='12'>4</option>\r\n"
															"		<option %s value='13'>41</option>\r\n"
															"		<option %s value='14'>42</option>\r\n"
															"		<option %s value='15'>5</option>\r\n"
															"		<option %s value='16'>51</option>\r\n"
															"	</select><br />\r\n"
										"Разрешение детектора:<input type='number' name='ResolDetectW' min=120 max=2048 value='%i' style='width: 100px;'%s>x\r\n"
										"		<input type='number' name='ResolDetectH' min=120 max=2048 value='%i' style='width: 100px;'%s><br />\r\n"
																				
										"Основной детектор движения:<input type='checkbox' name='MainSensor'%s disabled><br />\r\n"		
										"&ensp;&ensp;Шаг проверяемых кадров:<input type='number' name='MainStep' min=0 max=10000 value='%i' style='width: 100px;' disabled><br />\r\n"
										"&ensp;&ensp;Чувств. яркостного датчика:<input type='number' name='DetectLevel' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;Чувств. цветового датчика:<input type='number' name='ColorLevel' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"
										
										"Доп. детектор движения:<select name='LongSensor' style='width: 140px;'%s>\r\n"
											"		<option %s value='0'>Отключено</option>\r\n"
											"		<option %s value='1'>1=2 2!=3 3=4</option>\r\n"
											"		<option %s value='2'>1=2 2!=3 2!=4</option>\r\n"											
											"		<option %s value='5'>1=2 2!=3 3=4 (отладка)</option>\r\n"
											"		<option %s value='6'>1=2 2!=3 2!=4 (отладка)</option>\r\n"											
											"	</select><br />\r\n"
																
										"&ensp;&ensp;Шаг проверяемых кадров:<input type='number' name='LongStep' min=0 max=10000 value='%i' style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;Чувствительность по яркости:<input type='number' name='LongBrigLevel' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;Чувствительность по цвету:<input type='number' name='LongColLevel' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"
										
										"Быстрое подключение:<input type='checkbox' name='FastConn'%s%s><br />\r\n"
										"Предпросмотр:<input type='checkbox' name='Preview'%s%s><br />\r\n"
										"&ensp;&ensp;Разрешение:<input type='number' name='ResolPrevW' min=120 max=2048 value='%i' style='width: 100px;'%s>x\r\n"
										"		<input type='number' name='ResolPrevH' min=120 max=2048 value='%i' style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;Интервал ключа:<input type='number' name='PrevKeyFrame' min=0 value='%i' maxlength=3 style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;Битрэйт (kbt/s):<input type='number' name='PrevBitRate' min=1 value='%i' maxlength=5 style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;Тип битрэйта:<select name='PrevBitType' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Переменный</option>\r\n"
															"		<option %s value='1'>Постоянный</option>\r\n"
															"	</select><br />\r\n"
										"&ensp;&ensp;Профиль кодека:<select name='PrevAvcProfile' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Не указан</option>\r\n"
															"		<option %s value='1'>Baseline</option>\r\n"
															"		<option %s value='2'>Main</option>\r\n"
															"		<option %s value='3'>Extended</option>\r\n"
															"		<option %s value='4'>High</option>\r\n"
															"		<option %s value='5'>High10</option>\r\n"
															"		<option %s value='6'>High422</option>\r\n"
															"		<option %s value='7'>High444</option>\r\n"
															"	</select><br />\r\n"
										"&ensp;&ensp;Уровень кодека:<select name='PrevAvcLevel' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Не указан</option>\r\n"
															"		<option %s value='1'>1</option>\r\n"
															"		<option %s value='2'>1b</option>\r\n"
															"		<option %s value='3'>11</option>\r\n"
															"		<option %s value='4'>12</option>\r\n"
															"		<option %s value='5'>13</option>\r\n"
															"		<option %s value='6'>2</option>\r\n"
															"		<option %s value='7'>21</option>\r\n"
															"		<option %s value='8'>22</option>\r\n"
															"		<option %s value='9'>3</option>\r\n"
															"		<option %s value='10'>31</option>\r\n"
															"		<option %s value='11'>32</option>\r\n"
															"		<option %s value='12'>4</option>\r\n"
															"		<option %s value='13'>41</option>\r\n"
															"		<option %s value='14'>42</option>\r\n"
															"		<option %s value='15'>5</option>\r\n"
															"		<option %s value='16'>51</option>\r\n"
															"	</select><br />\r\n"
										"Записывать нормальный режим:<input type='checkbox' name='RecNorm'%s%s><br />\r\n"
										"Записывать дифференциальный режим:<input type='checkbox' name='RecDiff'%s%s><br />\r\n"
										"&ensp;Чувствительность:<input type='number' name='DiffRecLevel' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"
										"&ensp;Размерность:<input type='number' name='DiffRecSize' min=0 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"
										"&emsp;&emsp;Сохранять кадров после движения:<input type='number' name='SaveFramesDiff' min=1 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"
										"Записывать медленный режим:<input type='checkbox' name='RecSlow'%s%s><br />\r\n"
										"&ensp;&ensp;Кодировать отдельно:<input type='checkbox' name='SlowCoder'%s%s><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Частота кадров:<input type='number' name='SlowFrameRate' min=2 max=190 value='%i' style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Интервал ключа:<input type='number' name='SlowKeyFrame' min=0 value='%i' maxlength=3 style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Битрэйт (kbt/s):<input type='number' name='SlowBitRate' min=1 value='%i' maxlength=5 style='width: 100px;'%s><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Тип битрэйта:<select name='SlowBitType' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Переменный</option>\r\n"
															"		<option %s value='1'>Постоянный</option>\r\n"
															"	</select><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Профиль кодека:<select name='SlowAvcProfile' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Не указан</option>\r\n"
															"		<option %s value='1'>Baseline</option>\r\n"
															"		<option %s value='2'>Main</option>\r\n"
															"		<option %s value='3'>Extended</option>\r\n"
															"		<option %s value='4'>High</option>\r\n"
															"		<option %s value='5'>High10</option>\r\n"
															"		<option %s value='6'>High422</option>\r\n"
															"		<option %s value='7'>High444</option>\r\n"
															"	</select><br />\r\n"
										"&ensp;&ensp;&ensp;&ensp;Уровень кодека:<select name='SlowAvcLevel' style='width: 140px;'%s>\r\n"
															"		<option %s value='0'>Не указан</option>\r\n"
															"		<option %s value='1'>1</option>\r\n"
															"		<option %s value='2'>1b</option>\r\n"
															"		<option %s value='3'>11</option>\r\n"
															"		<option %s value='4'>12</option>\r\n"
															"		<option %s value='5'>13</option>\r\n"
															"		<option %s value='6'>2</option>\r\n"
															"		<option %s value='7'>21</option>\r\n"
															"		<option %s value='8'>22</option>\r\n"
															"		<option %s value='9'>3</option>\r\n"
															"		<option %s value='10'>31</option>\r\n"
															"		<option %s value='11'>32</option>\r\n"
															"		<option %s value='12'>4</option>\r\n"
															"		<option %s value='13'>41</option>\r\n"
															"		<option %s value='14'>42</option>\r\n"
															"		<option %s value='15'>5</option>\r\n"
															"		<option %s value='16'>51</option>\r\n"
															"	</select><br />\r\n"
										"&emsp;&emsp;Пропускать кадров:<input type='number' name='SkipFrames' min=1 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"
										"&emsp;&emsp;Только изменения:<input type='checkbox' name='SlowDiff'%s%s><br />\r\n"
										"&emsp;&emsp;&ensp;&ensp;Чувствительность:<input type='number' name='SlowRecLevel' min=0 max=255 value='%i' style='width: 100px;'%s><br />\r\n"									
										"&emsp;&emsp;&ensp;&ensp;Размерность:<input type='number' name='SlowRecSize' min=0 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"									
										"&emsp;&emsp;&emsp;&emsp;Сохранять кадров после движения:<input type='number' name='SaveFramesSlow' min=1 max=1000 value='%i' style='width: 100px;'%s><br />\r\n"
										"PTZ:<input type='checkbox' name='PtzEn'%s%s><br />\r\n"										
										"&emsp;&emsp;PTZ модуль:<input type='text' name='PtzID' value='%.4s' maxlength=4 style='width: 60px;'%s><br />\r\n"
										"&emsp;&emsp;&emsp;&emsp;Автофокус:<input type='checkbox' name='PtzFocus'%s%s><br />\r\n"										
										"&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;Карта фокуса:<input type='checkbox' name='PtzFocusMap'%s%s><br />\r\n"										
										"&emsp;&emsp;&emsp;&emsp;Домой при запуске:<input type='checkbox' name='PtzHomeStart'%s%s><br />\r\n"										
										"&emsp;&emsp;&emsp;&emsp;Домой после смены:<input type='checkbox' name='PtzHomeReturn'%s%s><br />\r\n"										
										"&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;Время до возврата:<input type='number' name='PtzHomeTimeWait' min=1 max=2000 value='%i' style='width: 100px;'%s>сек.<br />\r\n"
										"</td>\r\n",
										(char*)&miModuleList[n].Settings[0], pDisableFlag,
										pDisableFlag,
										((miModuleList[n].Settings[8] & (4096 | 8192)) == 0) ? " selected" : "", 0,
										((miModuleList[n].Settings[8] & (4096 | 8192)) == 4096) ? " selected" : "", 1,
										((miModuleList[n].Settings[8] & (4096 | 8192)) == 8192) ? " selected" : "", 2,
										pDisableFlag,
										(miModuleList[n].Settings[24] == OMX_ExposureControlOff) ? " selected " : "", OMX_ExposureControlOff, GetNameExposure(OMX_ExposureControlOff),
										(miModuleList[n].Settings[24] == OMX_ExposureControlAuto) ? " selected " : "", OMX_ExposureControlAuto, GetNameExposure(OMX_ExposureControlAuto),
										(miModuleList[n].Settings[24] == OMX_ExposureControlNight) ? " selected " : "", OMX_ExposureControlNight, GetNameExposure(OMX_ExposureControlNight),
										(miModuleList[n].Settings[24] == OMX_ExposureControlBackLight) ? " selected " : "", OMX_ExposureControlBackLight, GetNameExposure(OMX_ExposureControlBackLight),
										(miModuleList[n].Settings[24] == OMX_ExposureControlSpotLight) ? " selected " : "", OMX_ExposureControlSpotLight, GetNameExposure(OMX_ExposureControlSpotLight),
										(miModuleList[n].Settings[24] == OMX_ExposureControlSports) ? " selected " : "", OMX_ExposureControlSports, GetNameExposure(OMX_ExposureControlSports),
										(miModuleList[n].Settings[24] == OMX_ExposureControlSnow) ? " selected " : "", OMX_ExposureControlSnow, GetNameExposure(OMX_ExposureControlSnow),
										(miModuleList[n].Settings[24] == OMX_ExposureControlBeach) ? " selected " : "", OMX_ExposureControlBeach, GetNameExposure(OMX_ExposureControlBeach),
										(miModuleList[n].Settings[24] == OMX_ExposureControlLargeAperture) ? " selected " : "", OMX_ExposureControlLargeAperture, GetNameExposure(OMX_ExposureControlLargeAperture),
										(miModuleList[n].Settings[24] == OMX_ExposureControlSmallAperture) ? " selected " : "", OMX_ExposureControlSmallAperture, GetNameExposure(OMX_ExposureControlSmallAperture),
										(miModuleList[n].Settings[24] == OMX_ExposureControlVeryLong) ? " selected " : "", OMX_ExposureControlVeryLong, GetNameExposure(OMX_ExposureControlVeryLong),
										(miModuleList[n].Settings[24] == OMX_ExposureControlFixedFps) ? " selected " : "", OMX_ExposureControlFixedFps, GetNameExposure(OMX_ExposureControlFixedFps),
										(miModuleList[n].Settings[24] == OMX_ExposureControlNightWithPreview) ? " selected " : "", OMX_ExposureControlNightWithPreview, GetNameExposure(OMX_ExposureControlNightWithPreview),
										(miModuleList[n].Settings[24] == OMX_ExposureControlAntishake) ? " selected " : "", OMX_ExposureControlAntishake, GetNameExposure(OMX_ExposureControlAntishake),
										(miModuleList[n].Settings[24] == OMX_ExposureControlFireworks) ? " selected " : "", OMX_ExposureControlFireworks, GetNameExposure(OMX_ExposureControlFireworks),										
										pDisableFlag,
										(miModuleList[n].Settings[28] == OMX_ImageFilterNone) ? " selected " : "", OMX_ImageFilterNone, GetNameImageFilter(OMX_ImageFilterNone),
										(miModuleList[n].Settings[28] == OMX_ImageFilterNoise) ? " selected " : "", OMX_ImageFilterNoise, GetNameImageFilter(OMX_ImageFilterNoise),
										(miModuleList[n].Settings[28] == OMX_ImageFilterEmboss) ? " selected " : "", OMX_ImageFilterEmboss, GetNameImageFilter(OMX_ImageFilterEmboss),
										(miModuleList[n].Settings[28] == OMX_ImageFilterNegative) ? " selected " : "", OMX_ImageFilterNegative, GetNameImageFilter(OMX_ImageFilterNegative),
										(miModuleList[n].Settings[28] == OMX_ImageFilterSketch) ? " selected " : "", OMX_ImageFilterSketch, GetNameImageFilter(OMX_ImageFilterSketch),
										(miModuleList[n].Settings[28] == OMX_ImageFilterOilPaint) ? " selected " : "", OMX_ImageFilterOilPaint, GetNameImageFilter(OMX_ImageFilterOilPaint),
										(miModuleList[n].Settings[28] == OMX_ImageFilterHatch) ? " selected " : "", OMX_ImageFilterHatch, GetNameImageFilter(OMX_ImageFilterHatch),
										(miModuleList[n].Settings[28] == OMX_ImageFilterGpen) ? " selected " : "", OMX_ImageFilterGpen, GetNameImageFilter(OMX_ImageFilterGpen),
										(miModuleList[n].Settings[28] == OMX_ImageFilterAntialias) ? " selected " : "", OMX_ImageFilterAntialias, GetNameImageFilter(OMX_ImageFilterAntialias),
										(miModuleList[n].Settings[28] == OMX_ImageFilterDeRing) ? " selected " : "", OMX_ImageFilterDeRing, GetNameImageFilter(OMX_ImageFilterDeRing),
										(miModuleList[n].Settings[28] == OMX_ImageFilterSolarize) ? " selected " : "", OMX_ImageFilterSolarize, GetNameImageFilter(OMX_ImageFilterSolarize),
										(miModuleList[n].Settings[28] == OMX_ImageFilterWatercolor) ? " selected " : "", OMX_ImageFilterWatercolor, GetNameImageFilter(OMX_ImageFilterWatercolor),
										(miModuleList[n].Settings[28] == OMX_ImageFilterPastel) ? " selected " : "", OMX_ImageFilterPastel, GetNameImageFilter(OMX_ImageFilterPastel),
										(miModuleList[n].Settings[28] == OMX_ImageFilterSharpen) ? " selected " : "", OMX_ImageFilterSharpen, GetNameImageFilter(OMX_ImageFilterSharpen),
										(miModuleList[n].Settings[28] == OMX_ImageFilterFilm) ? " selected " : "", OMX_ImageFilterFilm, GetNameImageFilter(OMX_ImageFilterFilm),
										(miModuleList[n].Settings[28] == OMX_ImageFilterBlur) ? " selected " : "", OMX_ImageFilterBlur, GetNameImageFilter(OMX_ImageFilterBlur),
										(miModuleList[n].Settings[28] == OMX_ImageFilterSaturation) ? " selected " : "", OMX_ImageFilterSaturation, GetNameImageFilter(OMX_ImageFilterSaturation),
										(miModuleList[n].Settings[28] == OMX_ImageFilterDeInterlaceLineDouble) ? " selected " : "", OMX_ImageFilterDeInterlaceLineDouble, GetNameImageFilter(OMX_ImageFilterDeInterlaceLineDouble),
										(miModuleList[n].Settings[28] == OMX_ImageFilterDeInterlaceAdvanced) ? " selected " : "", OMX_ImageFilterDeInterlaceAdvanced, GetNameImageFilter(OMX_ImageFilterDeInterlaceAdvanced),
										(miModuleList[n].Settings[28] == OMX_ImageFilterColourSwap) ? " selected " : "", OMX_ImageFilterColourSwap, GetNameImageFilter(OMX_ImageFilterColourSwap),
										(miModuleList[n].Settings[28] == OMX_ImageFilterWashedOut) ? " selected " : "", OMX_ImageFilterWashedOut, GetNameImageFilter(OMX_ImageFilterWashedOut),
										(miModuleList[n].Settings[28] == OMX_ImageFilterColourPoint) ? " selected " : "", OMX_ImageFilterColourPoint, GetNameImageFilter(OMX_ImageFilterColourPoint),
										(miModuleList[n].Settings[28] == OMX_ImageFilterPosterise) ? " selected " : "", OMX_ImageFilterPosterise, GetNameImageFilter(OMX_ImageFilterPosterise),
										(miModuleList[n].Settings[28] == OMX_ImageFilterColourBalance) ? " selected " : "", OMX_ImageFilterColourBalance, GetNameImageFilter(OMX_ImageFilterColourBalance),
										(miModuleList[n].Settings[28] == OMX_ImageFilterCartoon) ? " selected " : "", OMX_ImageFilterCartoon, GetNameImageFilter(OMX_ImageFilterCartoon),
										(miModuleList[n].Settings[28] == OMX_ImageFilterAnaglyph) ? " selected " : "", OMX_ImageFilterAnaglyph, GetNameImageFilter(OMX_ImageFilterAnaglyph),
										(miModuleList[n].Settings[28] == OMX_ImageFilterDeInterlaceFast) ? " selected " : "", OMX_ImageFilterDeInterlaceFast, GetNameImageFilter(OMX_ImageFilterDeInterlaceFast),
										pDisableFlag,
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlOff) ? " selected " : "", OMX_WhiteBalControlOff, GetNameWhiteBalance(OMX_WhiteBalControlOff),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlAuto) ? " selected " : "", OMX_WhiteBalControlAuto, GetNameWhiteBalance(OMX_WhiteBalControlAuto),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlSunLight) ? " selected " : "", OMX_WhiteBalControlSunLight, GetNameWhiteBalance(OMX_WhiteBalControlSunLight),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlCloudy) ? " selected " : "", OMX_WhiteBalControlCloudy, GetNameWhiteBalance(OMX_WhiteBalControlCloudy),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlShade) ? " selected " : "", OMX_WhiteBalControlShade, GetNameWhiteBalance(OMX_WhiteBalControlShade),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlTungsten) ? " selected " : "", OMX_WhiteBalControlTungsten, GetNameWhiteBalance(OMX_WhiteBalControlTungsten),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlFluorescent) ? " selected " : "", OMX_WhiteBalControlFluorescent, GetNameWhiteBalance(OMX_WhiteBalControlFluorescent),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlIncandescent) ? " selected " : "", OMX_WhiteBalControlIncandescent, GetNameWhiteBalance(OMX_WhiteBalControlIncandescent),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlFlash) ? " selected " : "", OMX_WhiteBalControlFlash, GetNameWhiteBalance(OMX_WhiteBalControlFlash),
										(miModuleList[n].Settings[29] == OMX_WhiteBalControlHorizon) ? " selected " : "", OMX_WhiteBalControlHorizon, GetNameWhiteBalance(OMX_WhiteBalControlHorizon),
										(miModuleList[n].Settings[38] & 4) ? " checked " : "", pDisableFlag,					//Сохранять пропорции после обрезки
										((miModuleList[n].Settings[10] >> 24) & 0xFF), pDisableFlag,					//LeftCrop
										((miModuleList[n].Settings[10] >> 16) & 0xFF), pDisableFlag,					//RightCrop
										((miModuleList[n].Settings[11] >> 24) & 0xFF), pDisableFlag,					//UpCrop
										((miModuleList[n].Settings[11] >> 16) & 0xFF), pDisableFlag,					//DownCrop
										miModuleList[n].Settings[25], pDisableFlag,	miModuleList[n].Settings[25],		//Яркость
										miModuleList[n].Settings[30], pDisableFlag,	miModuleList[n].Settings[30],		//Контраст
										miModuleList[n].Settings[31], pDisableFlag,	miModuleList[n].Settings[31],		//Резкость
										miModuleList[n].Settings[32], pDisableFlag,	miModuleList[n].Settings[32],		//Насыщенность
										(miModuleList[n].Settings[8] & 2048) ? " checked " : "", pDisableFlag,			//Автояркость
										miModuleList[n].Settings[27], pDisableFlag, miModuleList[n].Settings[27],  		//уровень автояркости
										ispCSI.ISOControl, pDisableFlag,
										ispCSI.HardAutoISOControl ? " checked " : "", pDisableFlag,						//Авто ISO hw
										ispCSI.SoftAutoISOControl ? " checked " : "", pDisableFlag,						//Авто ISO sw
										ispCSI.DestAutoISOControl, pDisableFlag, ispCSI.DestAutoISOControl,				//уровень автояркости для ISO
										ispCSI.EVControl, pDisableFlag,
										ispCSI.SoftAutoEVControl ? " checked " : "", pDisableFlag,						//Авто EV sw
										ispCSI.DestAutoEVControl, pDisableFlag, ispCSI.DestAutoEVControl,				//уровень автояркости для EV
										pDisableFlag,
										((miModuleList[n].Settings[26] & 3) == 0) ? " selected" : "", 0,
										((miModuleList[n].Settings[26] & 3) == 1) ? " selected" : "", 1,
										((miModuleList[n].Settings[26] & 3) == 2) ? " selected" : "", 2,
										((miModuleList[n].Settings[26] & 3) == 3) ? " selected" : "", 3,									
										(miModuleList[n].Settings[8] & 64) ? " checked " : "", pDisableFlag,			//FlipHorisontal
										(miModuleList[n].Settings[8] & 128) ? " checked " : "", pDisableFlag,			//FlipVertical
										(miModuleList[n].Settings[8] & 16384) ? " checked " : "", pDisableFlag,			//HighLight
										(miModuleList[n].Settings[8] >> 16) & 255, pDisableFlag, (miModuleList[n].Settings[8] >> 16) & 255,  	//уровень яркости для HighLight
										(miModuleList[n].Settings[8] >> 24) & 255, pDisableFlag, (miModuleList[n].Settings[8] >> 24) & 255,  	//уровень усиления для HighLight
										
										(miModuleList[n].Settings[1] >> 16) & 0xFFFF, pDisableFlag,				//Разрешение W
										miModuleList[n].Settings[1] & 0xFFFF, pDisableFlag,						//Разрешение H
										
										miModuleList[n].Settings[2], pDisableFlag,
										miModuleList[n].Settings[3], pDisableFlag,
										miModuleList[n].Settings[4], pDisableFlag,
										pDisableFlag,															//Тип битрэйта
										((miModuleList[n].Settings[8] & 256) == 0) ? " selected " : "",			//Тип битрэйта
										(miModuleList[n].Settings[8] & 256) ? " selected " : "",				//Тип битрэйта
										pDisableFlag,															//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 0) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 1) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 2) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 3) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 4) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 5) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 6) ? " selected " : "",	//Профиль кодека
										((miModuleList[n].Settings[19] & 255) == 7) ? " selected " : "",	//Профиль кодека
										pDisableFlag,															//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 0) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 1) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 2) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 3) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 4) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 5) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 6) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 7) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 8) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 9) ? " selected " : "",	//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 10) ? " selected " : "",//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 11) ? " selected " : "",//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 12) ? " selected " : "",//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 13) ? " selected " : "",//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 14) ? " selected " : "",//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 15) ? " selected " : "",//Уровень кодека
										((miModuleList[n].Settings[20] & 255) == 16) ? " selected " : "",//Уровень кодека
										
										(miModuleList[n].Settings[5] >> 16) & 0xFFFF, pDisableFlag,			//Разрешение W
										miModuleList[n].Settings[5] & 0xFFFF, pDisableFlag,					//Разрешение H
										
										miModuleList[n].ScanSet ? " checked " : "",								//Основной детектор движения
										miModuleList[n].ScanSet,
										miModuleList[n].Settings[6], pDisableFlag,
										miModuleList[n].Settings[16], pDisableFlag,
										
										pDisableFlag,
										(miModuleList[n].Settings[36] == 0) ? " selected " : "", 		//Доп. детектор движения
										(miModuleList[n].Settings[36] == 1) ? " selected " : "", 		//Доп. детектор движения
										(miModuleList[n].Settings[36] == 2) ? " selected " : "", 		//Доп. детектор движения
										(miModuleList[n].Settings[36] == 5) ? " selected " : "", 		//Доп. детектор движения
										(miModuleList[n].Settings[36] == 6) ? " selected " : "", 		//Доп. детектор движения
										
										miModuleList[n].Settings[33], pDisableFlag,
										miModuleList[n].Settings[34], pDisableFlag,
										miModuleList[n].Settings[35], pDisableFlag,
										
										miModuleList[n].Settings[15] ? " checked " : "", pDisableFlag,			//Быстрое подключение
										(miModuleList[n].Settings[8] & 4) ? " checked " : "", pDisableFlag,		//Предпросмотр
										
										(miModuleList[n].Settings[37] >> 16) & 0xFFFF, pDisableFlag,			//Разрешение W
										miModuleList[n].Settings[37] & 0xFFFF, pDisableFlag,					//Разрешение H
										
										miModuleList[n].Settings[21], pDisableFlag,  							//Интервал ключа
										miModuleList[n].Settings[17], pDisableFlag,  							//Битрэйт
										pDisableFlag,															//Тип битрэйта
										((miModuleList[n].Settings[8] & 512) == 0) ? " selected " : "",			//Тип битрэйта
										(miModuleList[n].Settings[8] & 512) ? " selected " : "",				//Тип битрэйта
										pDisableFlag,															//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 0) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 1) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 2) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 3) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 4) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 5) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 6) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 8) & 255) == 7) ? " selected " : "",	//Профиль кодека
										pDisableFlag,															//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 0) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 1) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 2) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 3) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 4) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 5) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 6) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 7) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 8) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 9) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 10) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 11) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 12) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 13) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 14) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 15) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 8) & 255) == 16) ? " selected " : "",//Уровень кодека
										
										(miModuleList[n].Settings[8] & 32) ? " checked " : "", pDisableFlag,	//Записывать нормальный режим
										(miModuleList[n].Settings[8] & 16) ? " checked " : "", pDisableFlag,	//Записывать дифференциальный режим
										miModuleList[n].Settings[7], pDisableFlag,								//Чувствительность
										miModuleList[n].Settings[13], pDisableFlag,								//Размерность
										miModuleList[n].Settings[10] & 0xFFFF, pDisableFlag,								//Сохранять кадров после движения
										(miModuleList[n].Settings[8] & 8) ? " checked " : "", pDisableFlag,		//Записывать медленный режим
										(miModuleList[n].Settings[8] & 1) ? " checked " : "", pDisableFlag, 	//Кодировать отдельно
										miModuleList[n].Settings[23], pDisableFlag,  							//Частота кадров
										miModuleList[n].Settings[22], pDisableFlag,  							//Интервал ключа
										miModuleList[n].Settings[18], pDisableFlag,  							//Битрэйт
										pDisableFlag,															//Тип битрэйта
										((miModuleList[n].Settings[8] & 1024) == 0) ? " selected " : "",		//Тип битрэйта
										(miModuleList[n].Settings[8] & 1024) ? " selected " : "",				//Тип битрэйта
										pDisableFlag,															//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 0) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 1) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 2) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 3) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 4) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 5) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 6) ? " selected " : "",	//Профиль кодека
										(((miModuleList[n].Settings[19] >> 16) & 255) == 7) ? " selected " : "",	//Профиль кодека
										pDisableFlag,															//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 0) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 1) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 2) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 3) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 4) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 5) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 6) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 7) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 8) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 9) ? " selected " : "",	//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 10) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 11) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 12) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 13) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 14) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 15) ? " selected " : "",//Уровень кодека
										(((miModuleList[n].Settings[20] >> 16) & 255) == 16) ? " selected " : "",//Уровень кодека
										miModuleList[n].Settings[9], pDisableFlag,								//Пропускать кадров
										(miModuleList[n].Settings[8] & 2) ? " checked " : "", pDisableFlag,		//Только изменения
										miModuleList[n].Settings[12], pDisableFlag,								//Чувствительность
										miModuleList[n].Settings[14], pDisableFlag,								//Размерность
										miModuleList[n].Settings[11] & 0xFFFF, pDisableFlag,					//Сохранять кадров после движения
										(miModuleList[n].Settings[38] & 1) ? " checked " : "", pDisableFlag,	//PTZ
										(char*)&miModuleList[n].Settings[39], pDisableFlag,						//PTZ модуль
										(miModuleList[n].Settings[38] & 2) ? " checked " : "", pDisableFlag,		//PTZ автофокус
										(miModuleList[n].Settings[38] & 8) ? " checked " : "", pDisableFlag,		//PTZ автофокус кекширование
										((miModuleList[n].Settings[42] >> 12) & 1) ? " checked " : "", pDisableFlag,	//PTZ домой при запуске
										((miModuleList[n].Settings[42] >> 13) & 1) ? " checked " : "", pDisableFlag,	//PTZ домой после движения
										miModuleList[n].Settings[42] & 0b111111111111, pDisableFlag						//PTZ время до возврата домой
										);
					}
					break;
				default:
					sprintf(msg_subbody, "<td><input type='number' name='Set1' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set2' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set3' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set4' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set5' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set6' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set7' value='%i' style='width: 100px;'%s><br />\r\n"
											"<input type='number' name='Set8' value='%i' style='width: 100px;'%s></td>\r\n",
										miModuleList[n].Settings[0], pDisableFlag,
										miModuleList[n].Settings[1], pDisableFlag,
										miModuleList[n].Settings[2], pDisableFlag,
										miModuleList[n].Settings[3], pDisableFlag,
										miModuleList[n].Settings[4], pDisableFlag,
										miModuleList[n].Settings[5], pDisableFlag,
										miModuleList[n].Settings[6], pDisableFlag,
										miModuleList[n].Settings[7], pDisableFlag);
					break;
			}
			
			strcat(msg_tx, msg_subbody);
			
			if (miModuleList[n].Local == 1)
			{
				//if (miModuleList[n].Type != MODULE_TYPE_SYSTEM)
				{
					if ((miModuleList[n].Enabled & 4) == 0)
					{
						strcat(msg_tx, "<td><button type='submit'>Сохранить</button>\r\n");
						if (miModuleList[n].Type == MODULE_TYPE_RTC)
							strcat(msg_tx, "<button type='submit' formaction='/modules/change'>Установить дату</button>\r\n");
						strcat(msg_tx, "<button type='submit' formaction='/modules/delete'>Удалить</button></td>\r\n"
										"</form></tr>\r\n");
					}
					else strcat(msg_tx, "<td><button type='submit' formaction='/modules/recover'>Вернуть</button></td>\r\n"
									"</form></tr>\r\n");
				}
				//else strcat(msg_tx, "<td></td></form></tr>\r\n");
			}
			else 
			{
				memset(msg_subbody, 0, 16384);
				sprintf(msg_subbody, "<td><input type='button' value='Редактировать' onclick=\"window.location.href='http://%s/modules/'\"></form></tr>\r\n",inet_ntoa(miModuleList[n].Address.sin_addr));				
				strcat(msg_tx, msg_subbody);
			}		
			if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;		
		}
	}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	memset(msg_subbody, 0, 16384);
	sprintf(msg_subbody,  "<tr><form action='/modules/add'>\r\n"
					"<td></td><td></td>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<td><select name='Type' style='width: 130px;'>\r\n"
					"		<option value='%i'>CAMERA</option>\r\n"
					"		<option value='%i'>COUNTER</option>\r\n"
					"		<option value='%i'>DISPLAY</option>\r\n"
					"		<option selected value='%i'>GPIO</option>\r\n"
					"		<option value='%i'>IR_RECEIVER</option>\r\n"
					"		<option value='%i'>MEMORY</option>\r\n"
					"		<option value='%i'>MICROPHONE</option>\r\n"
					"		<option value='%i'>PN532</option>\r\n"
					"		<option value='%i'>RADIO</option>\r\n"
					"		<option value='%i'>RC522</option>\r\n"
					"		<option value='%i'>REALTIMECLOCK</option>\r\n"
					"		<option value='%i'>RS485</option>\r\n"
					"		<option value='%i'>USB GPIO</option>\r\n"
					"		<option value='%i'>SPEAKER</option>\r\n"
					"		<option value='%i'>SYSTEM</option>\r\n"
					"		<option value='%i'>TEMPSENSOR</option>\r\n"
					"		<option value='%i'>TIMER</option>\r\n"
					"		<option value='%i'>KEYBOARD</option>\r\n"
					"		<option value='%i'>EXTERNAL</option>\r\n"					
					"		<option value='%i'>ADS1015</option>\r\n"
					"		<option value='%i'>MCP3421</option>\r\n"
					"		<option value='%i'>AS5600</option>\r\n"
					"		<option value='%i'>HMC5883L</option>\r\n"
					"		<option value='%i'>TFP625A</option>\r\n"
					"	</select></td>\r\n"
					"<td><input type='text' name='ID' value='' maxlength=4 style='width: 50px;'></td>\r\n"
					"<td></td>\r\n"
					"<td><input type='text' name='Name' value='' maxlength=60 style='width: 150px;'></td>\r\n"
					"<td><input type='number' name='ViewLevel' min=0 max=1000 value='0' maxlength=2 style='width: 100px;'></td>\r\n"
					"<td><input type='number' name='AccessLevel' min=0 max=1000 value='0' maxlength=2 style='width: 100px;'></td>\r\n"
					"<td><input type='number' name='ScanSet' min=0 value='0' maxlength=6 style='width: 100px;'></td>\r\n"
					"<td></td>\r\n"
					"<td></td>\r\n"
					"<input type='hidden' name='Page' value=%i>"
					"<td><button type='submit'>Добавить</button></td>\r\n"
					"</form></tr>\r\n", session->request_id,
										MODULE_TYPE_CAMERA, MODULE_TYPE_COUNTER, MODULE_TYPE_DISPLAY, MODULE_TYPE_GPIO,	MODULE_TYPE_IR_RECEIVER,
										MODULE_TYPE_MEMORY,	MODULE_TYPE_MIC, MODULE_TYPE_PN532,	MODULE_TYPE_RADIO, MODULE_TYPE_RC522,
										MODULE_TYPE_RTC, MODULE_TYPE_RS485,	MODULE_TYPE_USB_GPIO, MODULE_TYPE_SPEAKER, MODULE_TYPE_SYSTEM, 
										MODULE_TYPE_TEMP_SENSOR, MODULE_TYPE_TIMER, MODULE_TYPE_KEYBOARD, MODULE_TYPE_EXTERNAL,
										MODULE_TYPE_ADS1015, MODULE_TYPE_MCP3421, MODULE_TYPE_AS5600, MODULE_TYPE_HMC5883L, MODULE_TYPE_TFP625A, iSubPages - 1);
	strcat(msg_tx, msg_subbody);	
					
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	
	WEB_pages_list(msg_tx, "modules", iSubPages, iCurPage, "");	
	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	DBG_FREE(mBuff);
	DBG_FREE(usbio_port_options);
	
	return 1;
}

int WEB_modstatus_save(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt && (miModuleList[pParams[0]].Local == 1))
	{
		miModuleList[pParams[0]].SaveChanges = pParams[1];		
		miModuleList[pParams[0]].GenEvents = pParams[2];	
		miModuleList[pParams[0]].Global = pParams[3];
		miModuleList[pParams[0]].ScanSet = pParams[4] / 50;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret) SaveModules(); else WEB_AddMessageInList("Wrong module params");
	
	return 1;
}

int WEB_modstatus_add(int *pParams, char *strParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt && (miModuleList[pParams[0]].Local == 1))
	{
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_TFP625A)
		{
			ret = TFP625A_AutoEnroll(&miModuleList[pParams[0]], pParams[1], pParams[2], &strParams[0]);
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret == 1)
		WEB_AddMessageInList("Операция выполнена успешно\n");
		else WEB_AddMessageInList("Операция не выполнена\n");
	return ret;
}

int WEB_modstatus_del(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt && (miModuleList[pParams[0]].Local == 1))
	{
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_TFP625A)
		{
			ret = TFP625A_DeleteTemplate(&miModuleList[pParams[0]], pParams[1]);
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret == 1)
		WEB_AddMessageInList("Операция выполнена успешно\n");
		else WEB_AddMessageInList("Операция не выполнена\n");	
	return ret;
}

int WEB_modstatus_refresh(int *pParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt && (miModuleList[pParams[0]].Local == 1))
	{
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_TFP625A)
		{
			ret = TFP625A_CleanDatabase(&miModuleList[pParams[0]]);
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret == 1)
		WEB_AddMessageInList("Операция выполнена успешно\n");
		else WEB_AddMessageInList("Операция не выполнена\n");	
	return ret;
}

int WEB_modstatus_change(int *pParams, char *strParams)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt && (miModuleList[pParams[0]].Local == 1))
	{
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_TFP625A)
		{
			ret = TFP625A_ChangeInfo(&miModuleList[pParams[0]], pParams[1], pParams[2], &strParams[0]);
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	if (ret == 1)
		WEB_AddMessageInList("Операция выполнена успешно\n");
		else WEB_AddMessageInList("Операция не выполнена\n");	
	return ret;
}

int WEB_modstatus_play(int *pParams)
{
	int ret = 0;
	int submod = 0;
	int status = 0;
	unsigned int uiID = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt)
	{
		uiID = miModuleList[pParams[0]].ID;
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_GPIO) status = 1;
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_MIC) submod = 1;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret) AddModuleEventInList(uiID, submod, status, NULL, 0, 1);
	return 1;
}

int WEB_modstatus_stop(int *pParams)
{
	int ret = 0;
	int status = 0;
	unsigned int uiID;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt)
	{
		uiID = miModuleList[pParams[0]].ID;
		//if (miModuleList[pParams[0]].Type == MODULE_TYPE_GPIO) status = 0;
		//if (miModuleList[pParams[0]].Type == MODULE_TYPE_MIC) submod = 0;
		ret = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret) AddModuleEventInList(uiID, 0, status, NULL, 0, 1);
	return 1;
}

int WEB_modstatus_show(int *pParams)
{
	int ret = 0;
	unsigned int uiID;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt)
	{
		uiID = miModuleList[pParams[0]].ID;
		ret = 1;
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_RTC) ret = 2;
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_TFP625A) TFP625A_UpdateInfo(&miModuleList[pParams[0]]);
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret == 1) ReqModuleStatus(uiID);
	if (ret == 2) AddModuleEventInList(uiID, 0, pParams[1], NULL, 0, 1);
	return 1;
}

int WEB_modstatus_load(int *pParams)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if ((pParams[0] >= 0) && (pParams[0] < iModuleCnt) && iModuleCnt)
	{
		if (miModuleList[pParams[0]].Type == MODULE_TYPE_COUNTER)
		{
			int i;
			for (i = 0; i < MAX_MODULE_SETTINGS; i++)
				if (i < MAX_MODULE_STATUSES) 
				{
					if ((miModuleList[pParams[0]].Settings[i] >> 24) == 0) 
						miModuleList[pParams[0]].Status[i] = miModuleList[pParams[0]].Settings[i];
				}
				else break;
		}
		
		if	(miModuleList[pParams[0]].Type == MODULE_TYPE_MEMORY)
		{
			int i;
			for (i = 0; i < MAX_MODULE_SETTINGS; i++)
				if (i < MAX_MODULE_STATUSES) miModuleList[pParams[0]].Status[i] = miModuleList[pParams[0]].Settings[i];
					else break;
		}				
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	return 1;
}

int WEB_modstatus_filters(int *pParams)
{
	if ((pParams[0] >= 0) && (pParams[0] <= 2)) iCurModStatusEnabledFilter = pParams[0];
	if ((pParams[1] >= 0) && (pParams[1] <= 2)) iCurModStatusAreaFilter = pParams[1];
	if ((pParams[2] >= 0) && (pParams[2] <= MODULE_TYPE_MAX)) iCurModStatusTypeFilter = pParams[2];
	
	return 1;
}

int WEB_modstatuses_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int n , ret;	
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/modstatuses/\"><h1>СОСТОЯНИЕ МОДУЛЕЙ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	char cDisplayEnabled = 0;
	char cSpeakerEnabled = 0;
	if (ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1) >= 0) cDisplayEnabled = 1;
	if (ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1) >= 0) cSpeakerEnabled = 1;
	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, "<form action='/modstatuses/filters'>\r\n"
						"<input type='hidden' name='req_index' value=%i>\r\n"
						"Активность: <select name='Enabled' style='width: 140px;'>\r\n"
											"		<option %s value='0'>Отключенные</option>\r\n"
											"		<option %s value='1'>Включенные</option>\r\n"
											"		<option %s value='2'>Все</option>\r\n"											
											"	</select>\r\n"
						"  Расположение: <select name='Area' style='width: 140px;'>\r\n"
											"		<option %s value='0'>Удаленные</option>\r\n"
											"		<option %s value='1'>Локальные</option>\r\n"
											"		<option %s value='2'>Все</option>\r\n"											
											"	</select>\r\n"
						"  Тип: <select name='Type' style='width: 130px;'>\r\n"
											"		<option %s value='%i'>ВСЕ</option>\r\n"
											"		<option %s value='%i'>CAMERA</option>\r\n"
											"		<option %s value='%i'>COUNTER</option>\r\n"
											"		<option %s value='%i'>DISPLAY</option>\r\n"
											"		<option %s value='%i'>GPIO</option>\r\n"
											"		<option %s value='%i'>IR_RECEIVER</option>\r\n"
											"		<option %s value='%i'>MEMORY</option>\r\n"
											"		<option %s value='%i'>MICROPHONE</option>\r\n"
											"		<option %s value='%i'>PN532</option>\r\n"
											"		<option %s value='%i'>RADIO</option>\r\n"
											"		<option %s value='%i'>RC522</option>\r\n"
											"		<option %s value='%i'>REALTIMECLOCK</option>\r\n"
											"		<option %s value='%i'>RS485</option>\r\n"
											"		<option %s value='%i'>EXTERNAL</option>\r\n"
											"		<option %s value='%i'>USB GPIO</option>\r\n"
											"		<option %s value='%i'>SPEAKER</option>\r\n"
											"		<option %s value='%i'>SYSTEM</option>\r\n"
											"		<option %s value='%i'>TEMPSENSOR</option>\r\n"
											"		<option %s value='%i'>TIMER</option>\r\n"
											"		<option %s value='%i'>KEYBOARD</option>\r\n"
											"		<option %s value='%i'>ADS1015</option>\r\n"
											"		<option %s value='%i'>MCP3421</option>\r\n"
											"		<option %s value='%i'>AS5600</option>\r\n"
											"		<option %s value='%i'>HMC5883L</option>\r\n"
											"		<option %s value='%i'>TFP625A</option>\r\n"
											"	</select></td>\r\n"
						"<button type='submit'>Применить</button></form>\r\n",
						session->request_id,
						(iCurModStatusEnabledFilter == 0) ? "selected" : "",
						(iCurModStatusEnabledFilter == 1) ? "selected" : "",
						(iCurModStatusEnabledFilter == 2) ? "selected" : "",
						(iCurModStatusAreaFilter == 0) ? "selected" : "",
						(iCurModStatusAreaFilter == 1) ? "selected" : "",
						(iCurModStatusAreaFilter == 2) ? "selected" : "",
						(iCurModStatusTypeFilter == MODULE_TYPE_MAX) ? "selected" : "", 		MODULE_TYPE_MAX, 
						(iCurModStatusTypeFilter == MODULE_TYPE_CAMERA) ? "selected" : "", 		MODULE_TYPE_CAMERA, 
						(iCurModStatusTypeFilter == MODULE_TYPE_COUNTER) ? "selected" : "", 	MODULE_TYPE_COUNTER, 
						(iCurModStatusTypeFilter == MODULE_TYPE_DISPLAY) ? "selected" : "", 	MODULE_TYPE_DISPLAY, 
						(iCurModStatusTypeFilter == MODULE_TYPE_GPIO) ? "selected" : "", 		MODULE_TYPE_GPIO,	
						(iCurModStatusTypeFilter == MODULE_TYPE_IR_RECEIVER) ? "selected" : "", MODULE_TYPE_IR_RECEIVER,
						(iCurModStatusTypeFilter == MODULE_TYPE_MEMORY) ? "selected" : "", 		MODULE_TYPE_MEMORY,	
						(iCurModStatusTypeFilter == MODULE_TYPE_MIC) ? "selected" : "", 		MODULE_TYPE_MIC, 
						(iCurModStatusTypeFilter == MODULE_TYPE_PN532) ? "selected" : "", 		MODULE_TYPE_PN532,	
						(iCurModStatusTypeFilter == MODULE_TYPE_RADIO) ? "selected" : "", 		MODULE_TYPE_RADIO, 
						(iCurModStatusTypeFilter == MODULE_TYPE_RC522) ? "selected" : "", 		MODULE_TYPE_RC522,
						(iCurModStatusTypeFilter == MODULE_TYPE_RTC) ? "selected" : "", 		MODULE_TYPE_RTC, 
						(iCurModStatusTypeFilter == MODULE_TYPE_RS485) ? "selected" : "", 		MODULE_TYPE_RS485,	
						(iCurModStatusTypeFilter == MODULE_TYPE_EXTERNAL) ? "selected" : "", 	MODULE_TYPE_EXTERNAL,	
						(iCurModStatusTypeFilter == MODULE_TYPE_USB_GPIO) ? "selected" : "", 	MODULE_TYPE_USB_GPIO,	
						(iCurModStatusTypeFilter == MODULE_TYPE_SPEAKER) ? "selected" : "", 	MODULE_TYPE_SPEAKER, 
						(iCurModStatusTypeFilter == MODULE_TYPE_SYSTEM) ? "selected" : "", 		MODULE_TYPE_SYSTEM, 
						(iCurModStatusTypeFilter == MODULE_TYPE_TEMP_SENSOR) ? "selected" : "", MODULE_TYPE_TEMP_SENSOR,
						(iCurModStatusTypeFilter == MODULE_TYPE_TIMER) ? "selected" : "", 		MODULE_TYPE_TIMER,
						(iCurModStatusTypeFilter == MODULE_TYPE_KEYBOARD) ? "selected" : "", 	MODULE_TYPE_KEYBOARD,
						(iCurModStatusTypeFilter == MODULE_TYPE_ADS1015) ? "selected" : "", 	MODULE_TYPE_ADS1015,
						(iCurModStatusTypeFilter == MODULE_TYPE_MCP3421) ? "selected" : "", 	MODULE_TYPE_MCP3421,
						(iCurModStatusTypeFilter == MODULE_TYPE_AS5600) ? "selected" : "", 		MODULE_TYPE_AS5600,
						(iCurModStatusTypeFilter == MODULE_TYPE_HMC5883L) ? "selected" : "", 	MODULE_TYPE_HMC5883L,
						(iCurModStatusTypeFilter == MODULE_TYPE_TFP625A) ? "selected" : "", 	MODULE_TYPE_TFP625A);
	strcat(msg_tx, msg_subbody);				
						
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Номер</th><th>Включен</th><th>IP адрес</th><th>Тип</th><th>ИД</th><th>Версия</th><th>Имя</th><th>Интервал проверки (кратно 50мс)</th>"
					"<th>Состояние<br />(Номер,Лог,События[локально,глобально],Имя,Значение)</th><th>Управление</th><th>Действие</th></tr>");

	int iSubPages, iCurPage, iFirstOption, iLastOption;
	
	int iModuleFiltCnt = 0;
	for (n = 0; n < iModuleCnt; n++)
	{
		if (((iCurModStatusEnabledFilter == (miModuleList[n].Enabled & 1)) || (iCurModStatusEnabledFilter == 2)) &&
			((iCurModStatusAreaFilter == miModuleList[n].Local) || (iCurModStatusAreaFilter == 2)) &&
			((iCurModStatusTypeFilter == miModuleList[n].Type) || (iCurModStatusTypeFilter == MODULE_TYPE_MAX)))
		{
			iModuleFiltCnt++;
		}
	}
	
	iSubPages = (int)ceil((double)iModuleFiltCnt / WEB_PAGE_MAX_LEN);
	if (iPage != -1) iCurModStatusPage = iPage;
	iCurPage = iCurModStatusPage;
	if (iCurPage >= iSubPages) 
	{
		iCurPage = iSubPages - 1;
		iCurModStatusPage = iCurPage;
	}
	if (iCurPage <= 0) 
	{
		iCurPage = 0;
		iCurModStatusPage = iCurPage;
	}
	
	iFirstOption = 0;
	iLastOption = 0;
	int i = 0;
	int k = 0;
	for (n = 0; n < iModuleCnt; n++)
	{
		if (((iCurModStatusEnabledFilter == (miModuleList[n].Enabled & 1)) || (iCurModStatusEnabledFilter == 2)) &&
			((iCurModStatusAreaFilter == miModuleList[n].Local) || (iCurModStatusAreaFilter == 2)) &&
			((iCurModStatusTypeFilter == miModuleList[n].Type) || (iCurModStatusTypeFilter == MODULE_TYPE_MAX)))
		{			
			if (i < (iCurPage * WEB_PAGE_MAX_LEN)) {i++; iFirstOption = n;}
			if (k < (iFirstOption + WEB_PAGE_MAX_LEN)) {k++; iLastOption = n;} else break;
		}
	}
	
	WEB_pages_list(msg_tx, "modstatuses", iSubPages, iCurPage, "");	
	
	if (iLastOption < iModuleCnt)
	for (n = iFirstOption; n <= iLastOption; n++)
	{
		if (((iCurModStatusEnabledFilter == (miModuleList[n].Enabled & 1)) || (iCurModStatusEnabledFilter == 2)) &&
			((iCurModStatusAreaFilter == miModuleList[n].Local) || (iCurModStatusAreaFilter == 2)) &&
			((iCurModStatusTypeFilter == miModuleList[n].Type) || (iCurModStatusTypeFilter == MODULE_TYPE_MAX)))
		{
			memset(msg_subbody, 0, 3072);
			sprintf(msg_subbody, 
					"<tr><form action='/modstatuses/show'>\r\n"
					"<input type='hidden' name='req_index' value=%i>\r\n"
					"<input type='hidden' name='Num' value=%i>\r\n"
					"<td><input type='number' name='Pp' value=%i style='width: 50px;' disabled></td>\r\n"
					"<td><input type='checkbox' name='En'%s disabled></td>\r\n"
					"<td>%s</td>\r\n"
					"<td><input type='text' name='Tp' value='%s' style='width: 140px;' disabled></td>\r\n"
					"<td><input type='text' name='ID' value='%.4s' maxlength=4 style='width: 60px;' disabled></td>\r\n"
					"<td>%i.%i.%i.%i</td>"
					"<td><input type='text' name='Name' value='%s' maxlength=60 style='width: 150px;' disabled></td>\r\n"
					"<td><input type='number' name='ScanSet' min=0 value='%i' maxlength=6 style='width: 100px;'>(%is)</td>\r\n",
					session->request_id,
					n,n,
					(miModuleList[n].Enabled & 1) ? " checked" : "", 
					inet_ntoa(miModuleList[n].Address.sin_addr),
					GetModuleTypeName(miModuleList[n].Type),
					(char*)&miModuleList[n].ID,
					miModuleList[n].Version[0], miModuleList[n].Version[1], miModuleList[n].Version[2], miModuleList[n].Version[3],
					miModuleList[n].Name,
					miModuleList[n].ScanSet * 50, miModuleList[n].ScanSet * 5 / 100);		
			strcat(msg_tx, msg_subbody);
			
			char cBufferName[64];
			char cBufferValue[64];
			char cBufferValueType[32];
			char cStatusPrn[256];
			int flag = 1;
			memset(msg_subbody, 0, 3072);
			
			strcat(msg_tx, "<td>");			
			ret = 0;
			for (i = 0; i < MAX_MODULE_STATUSES; i++)
			{
				if (!GetModuleStatusEn(miModuleList[n].Type, i)) break;
				
				memset(cStatusPrn, 0, 256);	
				sprintf(cStatusPrn, "[%i] <input type='checkbox' name='SvCh%i'%s>[<input type='checkbox' name='GnEv%i'%s><input type='checkbox' name='GlEv%i'%s>]\r\n %s:%s%s<br />\r\n",									
									i+1, i,
									(miModuleList[n].SaveChanges & flag) ? " checked" : "",
									i,
									(miModuleList[n].GenEvents & flag) ? " checked" : "",
									i,
									(miModuleList[n].Global & flag) ? " checked" : "",
									GetModuleStatusName(miModuleList[n].Type, i, cBufferName, 64, 1),
									GetModuleStatusValue(miModuleList[n].Type, i, miModuleList[n].Status[i], cBufferValue, 64),
									GetModuleStatusValueType(miModuleList[n].Type, i, cBufferValueType, 32));
				strcat(msg_tx, cStatusPrn);
				flag <<= 1;
				ret++;
			}			
			
			if (miModuleList[n].Type == MODULE_TYPE_RTC)
			{
				strcat(msg_tx, "<select name='Action' style='width: 300;'>\r\n"
										"<option selected value='0'>Записать время в часы</option>\r\n"
										"<option value='1'>Считать время в систему</option>\r\n"
										"<option value='2'>Считать время во все системы</option>\r\n"
										"<option value='3'>Системное время во все системы</option>\r\n"
										"</select>");
			}
			
			if (miModuleList[n].Type == MODULE_TYPE_TFP625A)
			{
				DBG_MUTEX_LOCK(miModuleList[n].IO_mutex);				
				
				strcat(msg_tx, "<select name='TempNum' style='width: 300;'>\r\n");
				FINGER_INFO *fing = (FINGER_INFO*)miModuleList[n].IO_data;
				
				memset(cStatusPrn, 0, 256);
				snprintf(cStatusPrn, 256, "<option value='-1'>Добавить</option>\r\n");
				strcat(msg_tx, cStatusPrn);
				
				for (i = 0; i < 1024; i++)
				{
					if (fing[i].Filled)
					{
						memset(cStatusPrn, 0, 256);
						snprintf(cStatusPrn, 256, "<option value='%i'>%i) [%.4s] %s</option>\r\n", i, i, (char*)&fing[i].ID, fing[i].Info);
						strcat(msg_tx, cStatusPrn);
					}
				}
				strcat(msg_tx, "</select><br />\r\n");
				strcat(msg_tx, "Новый ID: <input type='text' name='TempID' value='' maxlength=4 style='width: 60px;'>\r\n"
									"Описание: <input type='text' name='TempInfo' value='' maxlength=31 style='width: 140px;'>\r\n");
											
				DBG_MUTEX_UNLOCK(miModuleList[n].IO_mutex);
			}
			
			strcat(msg_tx, "</td><td>\r\n");
			
			ret = 0;
			for (i = 0; i < MAX_MODULE_STATUSES; i++)
			{
				if (!GetModuleActionEn(miModuleList[n].Type, i)) break;
				
				memset(cStatusPrn, 0, 256);	
				sprintf(cStatusPrn, "%s<br />\r\n",	GetModuleActionName(miModuleList[n].Type, i, cBufferName, 64, 1));
				strcat(msg_tx, cStatusPrn);
				flag <<= 1;
				ret++;
			}
			
			strcat(msg_tx, "</td>\r\n");
			
			//strcat(msg_tx, msg_subbody);
						//if ((ret) && 
			if (miModuleList[n].Enabled & 1)
			{
				strcat(msg_tx, "<td>");
				switch(miModuleList[n].Type)
				{
					case MODULE_TYPE_MIC:
						strcat(msg_tx, "<button type='submit'>Обновить</button><br>\r\n");
						if (cSpeakerEnabled)
							strcat(msg_tx, "<button type='submit' formaction='/modstatuses/play'>Подключиться</button><br>\r\n"
											"<button type='submit' formaction='/modstatuses/stop'>Отключиться</button><br>");
							//else strcat(msg_tx, "</td></form></tr>\r\n");
						break;
					case MODULE_TYPE_CAMERA:
						strcat(msg_tx, "<button type='submit'>Обновить</button>\r\n");
						if (cDisplayEnabled) 
								strcat(msg_tx, "<button type='submit' formaction='/modstatuses/play'>Подключиться</button><br>");
							//else strcat(msg_tx, "</td></form></tr>\r\n");
						break;
					case MODULE_TYPE_GPIO:
						strcat(msg_tx, "<button type='submit'>Обновить</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/play'>Включить</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/stop'>Выключить</button><br>");
						break;
					case MODULE_TYPE_TFP625A:
						strcat(msg_tx, "<button type='submit'>Обновить список ID</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/add'>Добавить отпечаток</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/change'>Обновить описание</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/delete'>Удалить отпечаток</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/refresh'>Удалить все</button><br>\r\n");
						break;
					case MODULE_TYPE_SPEAKER:
						strcat(msg_tx, "<button type='submit' formaction='/modstatuses/play'>Проверить</button><br>"
										"<button type='submit' formaction='/modstatuses/stop'>Отключиться</button><br>");
						break;
					case MODULE_TYPE_RTC:
						strcat(msg_tx, 	"<button type='submit'>Выполнить действие</button><br>");
						break;
					case MODULE_TYPE_COUNTER:
					case MODULE_TYPE_MEMORY:
						if (miModuleList[n].Local)
							strcat(msg_tx, "<button type='submit'>Обновить</button><br>\r\n"
									"<button type='submit' formaction='/modstatuses/load'>Сбросить</button><br>");
							else strcat(msg_tx, "<button type='submit'>Обновить</button><br>");
						break;
					default:
						strcat(msg_tx, "<button type='submit'>Обновить</button>\r\n");	
						break;
				}
				if (ret && miModuleList[n].Local) strcat(msg_tx, "<button type='submit' formaction='/modstatuses/save'>Сохранить</button>");				
				strcat(msg_tx, "</td></form></tr>\r\n");
			}
			else strcat(msg_tx, "<td></td></form></tr>\r\n");
			if (strlen(msg_tx) > (WEB_TX_BUF_SIZE_MAX - 2000)) break;	
		}
	}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
					
	strcat(msg_tx,	"</table>"
					"<br />\r\n");
	WEB_pages_list(msg_tx, "modstatuses", iSubPages, iCurPage, "");	
	strcat(msg_tx,	"<br />\r\n"
					"</body>\r\n"
					"</html>\r\n");	
	
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	//printf("msg_tx totlen %i\n", strlen(msg_tx));
	DBG_FREE(msg_subbody);
	
	return 1;
}

int WEB_settings_save(int *pParams, char *strParams)
{
	SYSTEM_INFO* si;
	
	switch(pParams[0])
	{
		case 0:
			DBG_MUTEX_LOCK(&system_mutex);
			si = GetLocalSysInfo();
			memcpy(&si->ID, &strParams[0], 4);
			memset(si->Name, 0, 64);
			if (strlen(&strParams[256]) < 64) memcpy(si->Name, &strParams[256], strlen(&strParams[256]));
											else memcpy(si->Name, &strParams[256], 63);
			memset(cManualFile, 0, 256);
			if (strlen(&strParams[512]) < 256) memcpy(cManualFile, &strParams[512], strlen(&strParams[512]));
											else memcpy(cManualFile, &strParams[512], 255);
			memset(cDirectoryFile, 0, 256);
			if (strlen(&strParams[1280]) < 256) memcpy(cDirectoryFile, &strParams[1280], strlen(&strParams[1280]));
											else memcpy(cDirectoryFile, &strParams[1280], 255);
			DBG_MUTEX_UNLOCK(&system_mutex);
			
			DBG_MUTEX_LOCK(&user_mutex);
			memset(cUserFile, 0, 256);
			if (strlen(&strParams[768]) < 256) memcpy(cUserFile, &strParams[768], strlen(&strParams[768]));
											else memcpy(cUserFile, &strParams[768], 255);
			DBG_MUTEX_UNLOCK(&user_mutex);			

			DBG_MUTEX_LOCK(&rectangle_mutex);
			memset(cCamRectangleFile, 0, 256);
			if (strlen(&strParams[1024]) < 256) memcpy(cCamRectangleFile, &strParams[1024], strlen(&strParams[1024]));
											else memcpy(cCamRectangleFile, &strParams[1024], 255);
			DBG_MUTEX_UNLOCK(&rectangle_mutex);
			
			DBG_MUTEX_LOCK(&mnlaction_mutex);			
			memset(cMnlActionFile, 0, 256);
			if (strlen(&strParams[1536]) < 256) memcpy(cMnlActionFile, &strParams[1536], strlen(&strParams[1536]));
											else memcpy(cMnlActionFile, &strParams[1536], 255);
			DBG_MUTEX_UNLOCK(&mnlaction_mutex);	
			
			DBG_MUTEX_LOCK(&evntaction_mutex);			
			memset(cEvntActionFile, 0, 256);
			if (strlen(&strParams[1792]) < 256) memcpy(cEvntActionFile, &strParams[1792], strlen(&strParams[1792]));
											else memcpy(cEvntActionFile, &strParams[1792], 255);
			DBG_MUTEX_UNLOCK(&evntaction_mutex);			
			break;
		case 1:		
			DBG_MUTEX_LOCK(&ircode_mutex);
			memset(cIrCodeFile, 0, 256);
			if (strlen(&strParams[0]) < 256) memcpy(cIrCodeFile, &strParams[0], strlen(&strParams[0]));
											else memcpy(cIrCodeFile, &strParams[0], 255);
			DBG_MUTEX_UNLOCK(&ircode_mutex);
			
			DBG_MUTEX_LOCK(&securitylist_mutex);			
			memset(cKeyFile, 0, 256);
			if (strlen(&strParams[256]) < 256) memcpy(cKeyFile, &strParams[256], strlen(&strParams[256]));
											else memcpy(cKeyFile, &strParams[256], 255);
			DBG_MUTEX_UNLOCK(&securitylist_mutex);
			
			DBG_MUTEX_LOCK(&widget_mutex);			
			memset(cWidgetFile, 0, 256);
			if (strlen(&strParams[512]) < 256) memcpy(cWidgetFile, &strParams[512], strlen(&strParams[512]));
											else memcpy(cWidgetFile, &strParams[512], 255);
			DBG_MUTEX_UNLOCK(&widget_mutex);
			
			DBG_MUTEX_LOCK(&system_mutex);
			memset(cStreamFile, 0, 256);
			if (strlen(&strParams[768]) < 256) memcpy(cStreamFile, &strParams[768], strlen(&strParams[768]));
											else memcpy(cStreamFile, &strParams[768], 255);
			memset(cStreamTypeFile, 0, 256);
			if (strlen(&strParams[1024]) < 256) memcpy(cStreamTypeFile, &strParams[1024], strlen(&strParams[1024]));
											else memcpy(cStreamTypeFile, &strParams[1024], 255);
			memset(cMailFile, 0, 256);
			if (strlen(&strParams[1280]) < 256) memcpy(cMailFile, &strParams[1280], strlen(&strParams[1280]));
											else memcpy(cMailFile, &strParams[1280], 255);
			memset(cRadioFile, 0, 256);
			if (strlen(&strParams[1792]) < 256) memcpy(cRadioFile, &strParams[1792], strlen(&strParams[1792]));
											else memcpy(cRadioFile, &strParams[1792], 255);
			DBG_MUTEX_UNLOCK(&system_mutex);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);			
			memset(cSoundFile, 0, 256);
			if (strlen(&strParams[1536]) < 256) memcpy(cSoundFile, &strParams[1536], strlen(&strParams[1536]));
											else memcpy(cSoundFile, &strParams[1536], 255);
			DBG_MUTEX_UNLOCK(&modulelist_mutex);			
			break; 
		case 2:
			DBG_MUTEX_LOCK(&system_mutex);
			memset(cAlarmFile, 0, 256);
			if (strlen(&strParams[0]) < 256) memcpy(cAlarmFile, &strParams[0], strlen(&strParams[0]));
											else memcpy(cAlarmFile, &strParams[0], 255);
			memset(cFileLocation, 0, 256);
			if (strlen(&strParams[512]) < 256) memcpy(cFileLocation, &strParams[512], strlen(&strParams[512]));
											else memcpy(cFileLocation, &strParams[512], 255);
			memset(cFileAlarmLocation, 0, 256);
			if (strlen(&strParams[768]) < 256) memcpy(cFileAlarmLocation, &strParams[768], strlen(&strParams[768]));
											else memcpy(cFileAlarmLocation, &strParams[768], 255);
			uiTextColor = Hex2Int(&strParams[1280]);
			fMenuSize = (float)Str2Float(&strParams[1536]);
			if ((fMenuSize < 0.1f) || (fMenuSize > 10.0f)) fMenuSize = 1.0f;
			DBG_MUTEX_UNLOCK(&system_mutex);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);
			memset(cModuleFile, 0, 256);
			if (strlen(&strParams[256]) < 256) memcpy(cModuleFile, &strParams[256], strlen(&strParams[256]));
											else memcpy(cModuleFile, &strParams[256], 255);
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			
			DBG_MUTEX_LOCK(&widget_mutex);
			iTimeCor = Str2Int(&strParams[1024]);
			uiPaddingSize = Str2Int(&strParams[1792]);
			DBG_MUTEX_UNLOCK(&widget_mutex);			
			break;
		case 3:
			pthread_mutex_lock(&dbg_mutex);
			iScreenLog = Str2Int(&strParams[0]);
			iMessageLog = Str2Int(&strParams[256]);
			iFileLog = Str2Int(&strParams[512]);
			memset(cFileLogName, 0, 256);
			if (strlen(&strParams[768]) < 256) memcpy(cFileLogName, &strParams[768], strlen(&strParams[768]));
											else memcpy(cFileLogName, &strParams[768], 255);
			pthread_mutex_unlock(&dbg_mutex);			
			
			DBG_MUTEX_LOCK(&system_mutex);	
			VSync = SearchStrInDataCaseIgn(&strParams[1024], 10, 0, "ON") ? 1 : 0;			
			memset(cCameraShotFile, 0, 256);
			if (strlen(&strParams[1280]) < 256) memcpy(cCameraShotFile, &strParams[1280], strlen(&strParams[1280]));
											else memcpy(cCameraShotFile, &strParams[1280], 255);
			memset(cCameraSensorFile, 0, 256);
			if (strlen(&strParams[1536]) < 256) memcpy(cCameraSensorFile, &strParams[1536], strlen(&strParams[1536]));
											else memcpy(cCameraSensorFile, &strParams[1536], 255);
			DBG_MUTEX_UNLOCK(&system_mutex);	
			
			DBG_MUTEX_LOCK(&systemlist_mutex);	
			uiShowerLiveCtrlTime = Str2Int(&strParams[1792]);
			DBG_MUTEX_UNLOCK(&systemlist_mutex);	
			break;
		case 4:
			DBG_MUTEX_LOCK(&system_mutex);				
			uiShowMode = Str2Int(&strParams[0]);
			if ((uiShowMode < 0) || (uiShowMode > 2)) uiShowMode = 0;
			iSlideShowTimerSet = Str2Int(&strParams[256]);
			iDefAccessLevel = Str2Int(&strParams[512]);
			iCameraTimeMaxSet = Str2Int(&strParams[768]);
			iCurViewSize = Str2Int(&strParams[1792]) * 1000000;
			DBG_MUTEX_UNLOCK(&system_mutex);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);
			memcpy(&uiDefAlarmSound, &strParams[1024], 4);
			uiDefAlarmRepeats = Str2Int(&strParams[1280]);
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			
			pthread_mutex_lock(&dbg_mutex);
			iLocalMessageLog = Str2Int(&strParams[1536]);
			pthread_mutex_unlock(&dbg_mutex);			
	
			//ucFastCaptureVideo = SearchStrInDataCaseIgn(&strParams[768], 10, 0, "ON") ? 1 : 0;			
			//ucSlowCaptureVideo = SearchStrInDataCaseIgn(&strParams[1024], 10, 0, "ON") ? 1 : 0;			
			//ucDiffCaptureVideo = SearchStrInDataCaseIgn(&strParams[1280], 10, 0, "ON") ? 1 : 0;			
			//ucCaptureAudio = SearchStrInDataCaseIgn(&strParams[1536], 10, 0, "ON") ? 1 : 0;	
			
			break;
		case 5:
			DBG_MUTEX_LOCK(&system_mutex);				
			iSlideShowOnTime = Str2Int(&strParams[0]);
			iSlideShowOffTime = Str2Int(&strParams[256]);
			ucCaptEnabledStatuses = SearchStrInDataCaseIgn(&strParams[512], 10, 0, "ON") ? 1 : 0;
			ucCaptEnabledEvents	= 0;
			ucCaptEnabledEvents |= SearchStrInDataCaseIgn(&strParams[768], 10, 0, "ON") ? 1 : 0;	
			ucCaptEnabledEvents |= SearchStrInDataCaseIgn(&strParams[1280], 10, 0, "ON") ? 2 : 0;	
			ucCaptEnabledActions = SearchStrInDataCaseIgn(&strParams[1024], 10, 0, "ON") ? 1 : 0;	
			ucCaptEnabledVideo = SearchStrInDataCaseIgn(&strParams[1536], 10, 0, "ON") ? 1 : 0;	
			ucCaptEnabledAudio = SearchStrInDataCaseIgn(&strParams[1792], 10, 0, "ON") ? 1 : 0;	
			DBG_MUTEX_UNLOCK(&system_mutex);
			break;
		case 6:		
			DBG_MUTEX_LOCK(&system_mutex);				
			cFileWriterService = SearchStrInDataCaseIgn(&strParams[768], 10, 0, "ON") ? 1 : 0;
			memset(cWriterServicePath, 0, MAX_PATH);
			if (strlen(&strParams[1024]) < MAX_PATH) memcpy(cWriterServicePath, &strParams[1024], strlen(&strParams[1024]));
											else memcpy(cWriterServicePath, &strParams[1024], MAX_PATH - 1);
			memset(cNewSourcePath, 0, 256);
			if (strlen(&strParams[0]) < 256) memcpy(cNewSourcePath, &strParams[0], strlen(&strParams[0]));
											else memcpy(cNewSourcePath, &strParams[0], 255);
			memset(cNewSourceFile, 0, 256);
			if (strlen(&strParams[256]) < 256) memcpy(cNewSourceFile, &strParams[256], strlen(&strParams[256]));
											else memcpy(cNewSourceFile, &strParams[256], 255);											
			memset(cNewSourceLogin, 0, 256);
			if (strlen(&strParams[512]) < 256) memcpy(cNewSourceLogin, &strParams[512], strlen(&strParams[512]));
											else memcpy(cNewSourceLogin, &strParams[512], 255);
			memset(cNewSourcePass, 0, 256);
			if (strlen(&strParams[1280]) < 256) memcpy(cNewSourcePass, &strParams[1280], strlen(&strParams[1280]));
											else memcpy(cNewSourcePass, &strParams[1280], 255);
			cRebootAfterUpdate = SearchStrInDataCaseIgn(&strParams[1536], 10, 0, "ON") ? 1 : 0;
			DBG_MUTEX_UNLOCK(&system_mutex);
			break;
		case 7:
			DBG_MUTEX_LOCK(&system_mutex);				
			iBasicVolume = Str2Int(&strParams[256]);
			if ((iBasicVolume < 0) || (iBasicVolume > 100)) iBasicVolume = 0;
			uiMediaIoTimeout = Str2Int(&strParams[512]);
			if ((uiMediaIoTimeout < 0) || (uiMediaIoTimeout > 100)) uiMediaIoTimeout = 60;
			iAlarmVolume = Str2Int(&strParams[768]);
			if ((iAlarmVolume < 0) || (iAlarmVolume > 100)) iAlarmVolume = 0;
			iIntervalRescanCard = Str2Int(&strParams[1024]);
			ucBroadCastTCP = SearchStrInDataCaseIgn(&strParams[1280], 10, 0, "ON") ? 1 : 0;			
			DBG_MUTEX_UNLOCK(&system_mutex);			
			
			DBG_MUTEX_LOCK(&modulelist_mutex);
			iRadioVolume = Str2Int(&strParams[0]);
			if ((iRadioVolume < 0) || (iRadioVolume > 15)) iRadioVolume = 0;
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			
			DBG_MUTEX_LOCK(&Connects_Info[0].Socket_Mutex);				
			memcpy(cUdpTargetAddress, &strParams[1536], 15);
			DBG_MUTEX_UNLOCK(&Connects_Info[0].Socket_Mutex);		
			break;
		case 8:
			DBG_MUTEX_LOCK(&system_mutex);			
			cSettRandomFile = SearchStrInDataCaseIgn(&strParams[0], 10, 0, "ON") ? 1 : 0;			
			iStreamTimeMaxSet = Str2Int(&strParams[512]);
			uiWEBServer = SearchStrInDataCaseIgn(&strParams[768], 10, 0, "ON") ? 1 : 0;			
			WebAuth = SearchStrInDataCaseIgn(&strParams[1024], 10, 0, "ON") ? 1 : 0;			
			uiWebPort = Str2Int(&strParams[1280]);			
			WebMaxTimeIdle = Str2Int(&strParams[1536]);	
			memset(cCameraRectFile, 0, 256);
			if (strlen(&strParams[1792]) < 256)	memcpy(cCameraRectFile, &strParams[1792], strlen(&strParams[1792]));
											else memcpy(cCameraRectFile, &strParams[1792], 255);
			DBG_MUTEX_UNLOCK(&system_mutex);			
			
			pthread_mutex_lock(&dbg_mutex);			
			memset(cLogIP, 0, 256);
			memcpy(cLogIP, &strParams[256], 15);			
			pthread_mutex_unlock(&dbg_mutex);
			break;
		case 9:
			DBG_MUTEX_LOCK(&system_mutex);			
			uiRTSPStream = SearchStrInDataCaseIgn(&strParams[0], 10, 0, "ON") ? 1 : 0;			
			RtspAuth = SearchStrInDataCaseIgn(&strParams[256], 10, 0, "ON") ? 1 : 0;			
			uiRTSPPort = Str2Int(&strParams[512]);			
			uiRTPClntVidPort = Str2Int(&strParams[768]);	
			uiRTPClntAudPort = Str2Int(&strParams[1024]);	
			uiRTPServVidPort = Str2Int(&strParams[1280]);	
			uiRTPServAudPort = Str2Int(&strParams[1536]);	
			uiRTSPForceAudio = SearchStrInDataCaseIgn(&strParams[1792], 10, 0, "ON") ? 1 : 0;
			DBG_MUTEX_UNLOCK(&system_mutex);			
			break;
		case 10:
			DBG_MUTEX_LOCK(&system_mutex);
			uiErrorCameraRestart = Str2Int(&strParams[0]);
			uiErrorCameraRestartWait = Str2Int(&strParams[256]);
			uiErrorAudioRestart = Str2Int(&strParams[512]);
			uiErrorAudioRestartWait = Str2Int(&strParams[768]);
			uiErrorVideoRestart = Str2Int(&strParams[1024]);
			uiErrorVideoRestartWait = Str2Int(&strParams[1024]);
			cZoom = SearchStrInDataCaseIgn(&strParams[1792], 10, 0, "ON") ? 1 : 0;
			DBG_MUTEX_UNLOCK(&system_mutex);
			break;
		case 11:
			DBG_MUTEX_LOCK(&system_mutex);			
			cCaptureFilesView = SearchStrInDataCaseIgn(&strParams[0], 10, 0, "ON") ? 1 : 0;
			cCaptureFilesLevel = Str2Int(&strParams[256]);
			cBackUpFilesView = SearchStrInDataCaseIgn(&strParams[512], 10, 0, "ON") ? 1 : 0;
			cBackUpFilesLevel = Str2Int(&strParams[768]);
			cCaptureFilesViewDef = Str2Int(&strParams[1024]);
			uiFlvBufferSize = Str2Int(&strParams[1280]);
			if (uiFlvBufferSize > 15) uiFlvBufferSize = 3;
			DBG_MUTEX_UNLOCK(&system_mutex);
			break;
		case 12:
			DBG_MUTEX_LOCK(&system_mutex);			
			memset(cMailAddress, 0, 64);
			if (strlen(&strParams[0]) < 64) memcpy(cMailAddress, &strParams[0], strlen(&strParams[0]));
											else memcpy(cMailAddress, &strParams[0], 63);
			memset(cMailServer, 0, 64);
			if (strlen(&strParams[256]) < 64) memcpy(cMailServer, &strParams[256], strlen(&strParams[256]));
											else memcpy(cMailServer, &strParams[256], 63);
			memset(cMailLogin, 0, 64);
			if (strlen(&strParams[512]) < 64) memcpy(cMailLogin, &strParams[512], strlen(&strParams[512]));
											else memcpy(cMailLogin, &strParams[512], 63);
			memset(cMailPassword, 0, 64);
			if (strlen(&strParams[768]) < 64) memcpy(cMailPassword, &strParams[768], strlen(&strParams[768]));
											else memcpy(cMailPassword, &strParams[768], 63);
			pthread_mutex_lock(&dbg_mutex);
			memcpy(cLogMailAddress, cMailAddress, 64);
			memcpy(cLogMailServer, cMailServer, 64);
			memcpy(cLogMailLogin, cMailLogin, 64);
			memcpy(cLogMailPassword, cMailPassword, 64);
			pthread_mutex_unlock(&dbg_mutex);
			DBG_MUTEX_UNLOCK(&system_mutex);
			break;
		case 13:
			DBG_MUTEX_LOCK(&system_mutex);			
			memset(cMailAuth, 0, 64);
			if (strlen(&strParams[0]) < 64) memcpy(cMailAuth, &strParams[0], strlen(&strParams[0]));
											else memcpy(cMailAuth, &strParams[0], 63);
			pthread_mutex_lock(&dbg_mutex);
			memcpy(cLogMailAuth, cMailAuth, 64);
			iEmailLog = Str2Int(&strParams[256]);
			memset(cLogMlList.Address, 0, 64);
			if (strlen(&strParams[512]) < 64) memcpy(cLogMlList.Address, &strParams[512], strlen(&strParams[512]));
											else memcpy(cLogMlList.Address, &strParams[512], 63);
			uiLogEmailPauseSize = Str2Int(&strParams[768]) * 1000;
			if (uiLogEmailPauseSize < 60000) uiLogEmailPauseSize = 60000;
			pthread_mutex_unlock(&dbg_mutex);
			DBG_MUTEX_UNLOCK(&system_mutex);
			
			DBG_MUTEX_LOCK(&message_mutex);
			memcpy(&uiSoundMessageID, &strParams[1024], 4);
			uiSoundMessagePauseSize = Str2Int(&strParams[1280]) * 1000;
			uiSoundMessageVolume = Str2Int(&strParams[1536]);
			DBG_MUTEX_UNLOCK(&message_mutex);
			break;
		case 14:
			DBG_MUTEX_LOCK(&system_mutex);
			memcpy(&uiTerminalMenuID, &strParams[0], 4);
			uiMenuWidth = Str2Int(&strParams[256]);
			if ((uiMenuWidth < 0) || (uiMenuWidth > 5000)) uiMenuWidth = 0;			
			uiMenuHeight = Str2Int(&strParams[512]);
			if ((uiMenuHeight < 0) || (uiMenuHeight > 5000)) uiMenuHeight = 0;	
			cAccelerateTextRender = SearchStrInDataCaseIgn(&strParams[768], 10, 0, "ON") ? 1 : 0;
			cDateTimeReference = SearchStrInDataCaseIgn(&strParams[1024], 10, 0, "ON") ? 1 : 0;
			memset(cNTPServer, 0, 64);
			if (strlen(&strParams[1280]) < 64) 
				strcpy(cNTPServer, &strParams[1280]);
				else memcpy(cNTPServer, &strParams[1280], 63);
			if (cNTPServer[0] == 0) ucTimeUpdated = 2; else ucTimeUpdated = 1;
			DBG_MUTEX_UNLOCK(&system_mutex);
			break;
		case 15:
			DBG_MUTEX_LOCK(&system_mutex);			
			uiOnvifStream = SearchStrInDataCaseIgn(&strParams[0], 10, 0, "ON") ? 1 : 0;			
			OnvifAuth = SearchStrInDataCaseIgn(&strParams[256], 10, 0, "ON") ? 1 : 0;			
			uiOnvifPort = Str2Int(&strParams[512]);
			DBG_MUTEX_UNLOCK(&system_mutex);			
			break;
		
		default:
			dbgprintf(2, "Wrong num save settings (WEB)\n");
			break;
	}
	SaveSettings();
	add_sys_cmd_in_list(SYSTEM_CMD_TIMERS_UPDATE, 0);
	return 1;
}

int WEB_settings_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session, int iPage, int errcode)
{
	int cnt = 0;	
	
	char *msg_subbody = (char*)DBG_MALLOC(3072);
	
	TestSettings(1);
	
	DBG_MUTEX_LOCK(&ircode_mutex);
	char Copy_cIrCodeFile[256];
	memcpy(Copy_cIrCodeFile, cIrCodeFile, 256);
	DBG_MUTEX_UNLOCK(&ircode_mutex);
					
	pthread_mutex_lock(&dbg_mutex);
	int Copy_iFileLog = iFileLog;
	int Copy_iScreenLog = iScreenLog;
	int Copy_iMessageLog = iMessageLog;
	int Copy_iLocalMessageLog = iLocalMessageLog;
	int Copy_iEmailLog = iEmailLog;	
	unsigned int Copy_uiLogEmailPauseSize = uiLogEmailPauseSize;
	char Copy_cFileLogName[256];
	char Copy_cLogIP[256];
	char LogMailAddress[64];
	memcpy(Copy_cFileLogName, cFileLogName, 256);
	memcpy(Copy_cLogIP, cLogIP, 256);
	memcpy(LogMailAddress, cLogMlList.Address, 64);	
	pthread_mutex_unlock(&dbg_mutex);
	
	DBG_MUTEX_LOCK(&evntaction_mutex);
	char Copy_cEvntActionFile[256];
	memcpy(Copy_cEvntActionFile, cEvntActionFile, 256);		
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	char Copy_cMnlActionFile[256];
	memcpy(Copy_cMnlActionFile, cMnlActionFile, 256);		
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	char Copy_cSoundFile[256];
	char Copy_cModuleFile[256];
	int Copy_iRadioVolume;
	unsigned int Copy_uiDefAlarmSound;
	unsigned int Copy_uiDefAlarmRepeats;
	memcpy(Copy_cSoundFile, cSoundFile, 256);
	memcpy(Copy_cModuleFile, cModuleFile, 256);
	Copy_iRadioVolume = iRadioVolume;
	Copy_uiDefAlarmSound = uiDefAlarmSound;
	Copy_uiDefAlarmRepeats = uiDefAlarmRepeats;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	DBG_MUTEX_LOCK(&rectangle_mutex);
	char Copy_cCamRectangleFile[256];
	memcpy(Copy_cCamRectangleFile, cCamRectangleFile, 256);		
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
	
	DBG_MUTEX_LOCK(&securitylist_mutex);
	char Copy_cKeyFile[256];
	memcpy(Copy_cKeyFile, cKeyFile, 256);		
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int Copy_uiShowerLiveCtrlTime = uiShowerLiveCtrlTime;
	DBG_MUTEX_UNLOCK(&systemlist_mutex);
	
	DBG_MUTEX_LOCK(&user_mutex);
	char Copy_cUserFile[256];
	memcpy(Copy_cUserFile, cUserFile, 256);	
	DBG_MUTEX_UNLOCK(&user_mutex);
	
	DBG_MUTEX_LOCK(&message_mutex);
	unsigned int Copy_uiSoundMessageID = uiSoundMessageID;
	unsigned int Copy_uiSoundMessagePauseSize = uiSoundMessagePauseSize;
	unsigned int Copy_uiSoundMessageVolume = uiSoundMessageVolume;
	DBG_MUTEX_UNLOCK(&message_mutex);
	
	DBG_MUTEX_LOCK(&widget_mutex);
	char Copy_cWidgetFile[256];
	memcpy(Copy_cWidgetFile, cWidgetFile, 256);	
	unsigned int Copy_uiPaddingSize	= uiPaddingSize;
	int Copy_iTimeCor = iTimeCor;
	DBG_MUTEX_UNLOCK(&widget_mutex);
	
	DBG_MUTEX_LOCK(&Connects_Info[0].Socket_Mutex);
	char Copy_cUdpTargetAddress[64];
	memcpy(Copy_cUdpTargetAddress, cUdpTargetAddress, 64);
	DBG_MUTEX_UNLOCK(&Connects_Info[0].Socket_Mutex);
	
	strcpy(msg_tx, "HTTP/1.1 200 OK\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: ");
	int iPosLen = strlen(msg_tx);
	strcat(msg_tx, "      \r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n");
	int iHeadLen = strlen(msg_tx);				
	strcat(msg_tx, "<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body>\r\n");
	strcat(msg_tx, session->head);
	strcat(msg_tx, "<br /><a href=\"/settings/\"><h1>НАСТРОЙКИ</h1></a>\r\n");
	
	WEB_GetMessageList(msg_tx);
	
	strcat(msg_tx, "<table border='1' width='100%' cellpadding='5'>"
					"<tr><th>Параметр</th><th>Значение</th><th>Действие</th></tr>");
	DBG_MUTEX_LOCK(&system_mutex);
	SYSTEM_INFO* si = GetLocalSysInfo();
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Системный ИД:</td><td><input type='text' name='Val1' maxlength=4 value='%.4s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Системное имя:</td><td><input type='text' name='Val2' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл инструкции:</td><td><input type='text' name='Val3' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл пользователей:</td><td><input type='text' name='Val4' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл прямоугольников:</td><td><input type='text' name='Val5' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл директорий:</td><td><input type='text' name='Val6' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл ручных действий:</td><td><input type='text' name='Val7' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл действий на события:</td><td><input type='text' name='Val8' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, (char*)&si->ID, si->Name, cManualFile, Copy_cUserFile, Copy_cCamRectangleFile, cDirectoryFile, Copy_cMnlActionFile, Copy_cEvntActionFile);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Файл команд пульта:</td><td><input type='text' name='Val1' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл смарткарт:</td><td><input type='text' name='Val2' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл виджетов:</td><td><input type='text' name='Val3' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл потоков:</td><td><input type='text' name='Val4' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл типов потоков:</td><td><input type='text' name='Val5' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл почтовых адресов:</td><td><input type='text' name='Val6' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл звуков:</td><td><input type='text' name='Val7' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл радиоволн:</td><td><input type='text' name='Val8' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, Copy_cIrCodeFile, Copy_cKeyFile, Copy_cWidgetFile, cStreamFile, cStreamTypeFile, cMailFile, Copy_cSoundFile, cRadioFile);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Файл будильников:</td><td><input type='text' name='Val1' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Файл модулей:</td><td><input type='text' name='Val2' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Путь к файлам слайдшоу:</td><td><input type='text' name='Val3' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Путь к файлам будильника:</td><td><input type='text' name='Val4' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Часовой пояс (HHMMSS):</td><td><input type='number' name='Val5' min=-120000 max=120000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>* Цвет текста (HEX RGB):</td><td><input type='text' name='Val6' maxlength=6 value='%06X' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Размер меню:</td><td><input type='number' name='Val7' min=0.1 max=5 step=0.1 value='%g' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Размер бортов (pix):</td><td><input type='number' name='Val8' min=0 max=1000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, cAlarmFile, Copy_cModuleFile, cFileLocation, cFileAlarmLocation, Copy_iTimeCor, uiTextColor, fMenuSize, Copy_uiPaddingSize);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Использовать вертикальную синхронизацию:</td><td><input type='checkbox' name='Val5'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Plum'><td>Путь к файлу снимка камеры:</td><td><input type='text' name='Val6' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Plum'><td>Путь к файлу снимка сенсора камеры:</td><td><input type='text' name='Val7' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Время ожидания бездействия слайдшоу перед аварийным завершением (1=15сек.):</td><td><input type='number' name='Val8' min=0 max=1000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Red'><td>Уровень логирования на экран:</td><td><input type='number' name='Val1' min=0 max=50 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Red'><td>Уровень логирования на центральный терминал:</td><td><input type='number' name='Val2' min=0 max=50 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Red'><td>Уровень логирования в файл:</td><td><input type='number' name='Val3' min=0 max=50 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Red'><td>Путь к файлу логирования:</td><td><input type='text' name='Val4' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"</form>\r\n",
				session->request_id,
				cnt, VSync ? " checked " : "", cCameraShotFile, cCameraSensorFile, Copy_uiShowerLiveCtrlTime, Copy_iScreenLog, Copy_iMessageLog, Copy_iFileLog, Copy_cFileLogName);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody,
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr bgcolor='Red'><td>Уровень логирования на локальный терминал:</td><td><input type='number' name='Val7' min=0 max=50 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Режим отображения:</td>"
				"<td><select name='Val1' style='width: 300;'>\r\n"
						"<option %s value='0'>Отключено</option>\r\n"
						"<option %s value='1'>Сон</option>\r\n"
						"<option %s value='2'>Включено</option>\r\n"
						"</select></td>\r\n"						
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Интервал смены слайда:</td><td><input type='number' name='Val2' min=15 max=600 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Уровень доступа по умолчанию:</td><td><input type='number' name='Val3' min=0 max=1000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Время воспроизведения камеры или микрофона (сек.):</td><td><input type='number' name='Val4' min=60 max=3600 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Идентификатор звука будильника по умолчанию:</td><td><input type='text' name='Val5' maxlength=4 value='%.4s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Повторений звука будильника:</td><td><input type='number' name='Val6' min=0 max=100 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Максимальный размер файла для просмотра (МБ):</td><td><input type='number' name='Val8' min=1 max=50 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, 
				Copy_iLocalMessageLog,
				(uiShowMode == 0) ? " selected " : "", 
				(uiShowMode == 1) ? " selected " : "", 
				(uiShowMode == 2) ? " selected " : "",				
				iSlideShowTimerSet, iDefAccessLevel, iCameraTimeMaxSet, (char*)&Copy_uiDefAlarmSound, Copy_uiDefAlarmRepeats, 
				iCurViewSize / 1000000);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Время начала работы слайдшоу (HHMMSS):</td><td><input type='number' name='Val1' min=0 max=240000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Время окончания работы слайдшоу (HHMMSS):</td><td><input type='number' name='Val2' min=0 max=240000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Peru'><td>Настройки записи:</td><td></td><td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='PapayaWhip'><td>* Записывать видео:</td><td><input type='checkbox' name='Val7'%s></td></tr>\r\n"
				"<tr bgcolor='PapayaWhip'><td>* Записывать аудио:</td><td><input type='checkbox' name='Val8'%s></td></tr>\r\n"
				"<tr bgcolor='PapayaWhip'><td>* Записывать статусы</td><td><input type='checkbox' name='Val3'%s></td></tr>\r\n"
				"<tr bgcolor='PapayaWhip'><td>* Записывать события (только внутренние)</td><td><input type='checkbox' name='Val4'%s><input type='checkbox' name='Val6'%s></td></tr>\r\n"
				"<tr bgcolor='PapayaWhip'><td>* Записывать действия</td><td><input type='checkbox' name='Val5'%s></td></tr>\r\n"
				"</form>\r\n",
				session->request_id,
				cnt,iSlideShowOnTime, iSlideShowOffTime,
				ucCaptEnabledVideo ? " checked " : "",
				ucCaptEnabledAudio ? " checked " : "",
				ucCaptEnabledStatuses ? " checked " : "",
				(ucCaptEnabledEvents & 1) ? " checked " : "",
				(ucCaptEnabledEvents & 2) ? " checked " : "",
				ucCaptEnabledActions ? " checked " : "");
	strcat(msg_tx, msg_subbody);
	
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody,
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"				
				"<tr bgcolor='DarkOrange'><td>* Сервис копирования:</td><td><input type='checkbox' name='Val4'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkOrange'><td>* Путь для сервиса копирования (по умолчанию):</td><td><input type='text' name='Val5' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightOrange'><td>Путь к новым исходникам для обновления:</td><td><input type='text' name='Val1' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightOrange'><td>Логин:</td><td><input type='text' name='Val3' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightOrange'><td>Пароль:</td><td><input type='text' name='Val6' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightOrange'><td>Файл (Makefile) компилирования:</td><td><input type='text' name='Val2' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightOrange'><td>Перезагрузить устройство:</td><td><input type='checkbox' name='Val7'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"</form>\r\n",
				session->request_id,
				cnt,
				cFileWriterService ? " checked " : "",
				cWriterServicePath,
				cNewSourcePath, cNewSourceLogin, cNewSourcePass, cNewSourceFile,
				cRebootAfterUpdate ? " checked " : ""
				);
	strcat(msg_tx, msg_subbody);
	cnt++;	
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Громкость радио:</td><td><input type='range' name='Val1' min='0' max='15' step='1' value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Громкость воспроизведения:</td><td><input type='range' name='Val2' min='0' max='100' step='1' value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Таймаут воспроизведения(I/O):</td><td><input type='range' name='Val3' min='0' max='100' step='1' value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Громкость будильника:</td><td><input type='range' name='Val4' min='0' max='100' step='1' value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>* Интервал сканирования смарткарт (милисек.):</td><td><input type='number' name='Val5' min=0 max=100000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>\r\n"
				"<tr><td>* Гарантированная рассылка служебных сообщений (протокол TCP):</td><td><input type='checkbox' name='Val6'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>\r\n"
				"<tr><td>* IP адрес связи точка-точка:</td><td><input type='text' name='Val7' maxlength=15 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"</form>\r\n",
				session->request_id,
				cnt,
				Copy_iRadioVolume, iBasicVolume, uiMediaIoTimeout,
				iAlarmVolume, iIntervalRescanCard,
				ucBroadCastTCP ? " checked " : "",
				Copy_cUdpTargetAddress);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Случайный порядок:</td><td><input type='checkbox' name='Val1'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>IP адрес терминала:</td><td><input type='text' name='Val2' maxlength=15 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr><td>Время воспроизведения потока (сек.):</td><td><input type='number' name='Val3' min=60 max=3600 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkTurquoise'><td>* WEB интерфейс:</td><td><input type='checkbox' name='Val4'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkTurquoise'><td>WEB авторизация:</td><td><input type='checkbox' name='Val5'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkTurquoise'><td>* WEB порт:</td><td><input type='number' name='Val6' min=1 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkTurquoise'><td>Максимальное время WEB сессии (сек.):</td><td><input type='number' name='Val7' min=1 max=3600 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Plum'><td>Путь к файлу снимка квадратов камеры:</td><td><input type='text' name='Val8' maxlength=255 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, 
				cSettRandomFile ? " checked " : "", 
				Copy_cLogIP, iStreamTimeMaxSet,
				uiWEBServer ? " checked " : "", 
				WebAuth ? " checked " : "",
				uiWebPort, WebMaxTimeIdle, cCameraRectFile);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr bgcolor='LightGreen'><td>* RTSP интерфейс:</td><td><input type='checkbox' name='Val1'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>* RTSP авторизация:</td><td><input type='checkbox' name='Val2'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>* RTSP порт:</td><td><input type='number' name='Val3' min=1 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>RTP видео порт клиента:</td><td><input type='number' name='Val4' min=0 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>RTP аудио порт клиента:</td><td><input type='number' name='Val5' min=0 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>RTP видео порт сервера:</td><td><input type='number' name='Val6' min=0 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>RTP аудио порт сервера:</td><td><input type='number' name='Val7' min=0 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>RTSP принудительное аудио:</td><td><input type='checkbox' name='Val8'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"</form>\r\n",
				session->request_id,
				cnt, 
				uiRTSPStream ? " checked " : "",
				RtspAuth ? " checked " : "",
				uiRTSPPort,	uiRTPClntVidPort, uiRTPClntAudPort, uiRTPServVidPort, uiRTPServAudPort,
				uiRTSPForceAudio ? " checked " : "");
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr><td>Увеличение:</td><td><input type='checkbox' name='Val8'%s></td><td><button type='submit'>Сохранить</button></td></tr>\r\n"
				"<tr bgcolor='#d0cca7'><td>Действие при ошибке записи видео:</td>"
					"<td><select name='Val1' style='width: 300;'>\r\n"
						"<option %s value='0'>Игнорировать</option>\r\n"
						"<option %s value='1'>Перезапуск</option>\r\n"
						"<option %s value='2'>Перезагрузка</option>\r\n"
						"</select></td></tr>\r\n"
				"<tr bgcolor='#d0cca7'><td>Время (сек.) перед действием:</td><td><input type='number' name='Val2' min=0 max=255 value='%i' style='width: 300px;'></td></tr>\r\n"
				"<tr bgcolor='#d0cca7'><td>Действие при ошибке записи аудио:</td>"
					"<td><select name='Val3' style='width: 300;'>\r\n"
						"<option %s value='0'>Игнорировать</option>\r\n"
						"<option %s value='1'>Перезапуск</option>\r\n"
						"<option %s value='2'>Перезагрузка</option>\r\n"
						"</select></td></tr>\r\n"
				"<tr bgcolor='#d0cca7'><td>Время (сек.) перед действием:</td><td><input type='number' name='Val4' min=0 max=255 value='%i' style='width: 300px;'></td></tr>\r\n"
				"<tr bgcolor='#d0cca7'><td>Действие при ошибке воспроизведения видео:</td>"
					"<td><select name='Val5' style='width: 300;'>\r\n"
						"<option %s value='0'>Игнорировать</option>\r\n"
						"<option %s value='1'>Перезапуск</option>\r\n"
						"<option %s value='2'>Перезагрузка</option>\r\n"
						"</select></td></tr>\r\n"
				"<tr bgcolor='#d0cca7'><td>Время (сек.) перед действием:</td><td><input type='number' name='Val6' min=0 max=255 value='%i' style='width: 300px;'></td></tr>\r\n"
				"</form>\r\n",
				session->request_id,
				cnt, 
				cZoom ? " checked " : "",
				(uiErrorCameraRestart == 0) ? " selected " : "", 
				(uiErrorCameraRestart == 1) ? " selected " : "", 
				(uiErrorCameraRestart == 2) ? " selected " : "",
				uiErrorCameraRestartWait, 
				(uiErrorAudioRestart == 0) ? " selected " : "", 
				(uiErrorAudioRestart == 1) ? " selected " : "", 
				(uiErrorAudioRestart == 2) ? " selected " : "",
				uiErrorAudioRestartWait, 
				(uiErrorVideoRestart == 0) ? " selected " : "", 
				(uiErrorVideoRestart == 1) ? " selected " : "", 
				(uiErrorVideoRestart == 2) ? " selected " : "",
				uiErrorVideoRestartWait);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				
				"<tr bgcolor='LemonChiffon'><td>* Доступ к файлам захвата:</td><td><input type='checkbox' name='Val1'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LemonChiffon'><td>* Требуемый уровень доступа к файлам захвата:</td><td><input type='number' name='Val2' min=0 max=100 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LemonChiffon'><td>* Доступ к файлам копий:</td><td><input type='checkbox' name='Val3'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LemonChiffon'><td>* Требуемый уровень доступа к файлам копий:</td><td><input type='number' name='Val4' min=0 max=100 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				
				"<tr bgcolor='LemonChiffon'><td>* Выбор папки по умолчанию для просмотра файлов:</td>"
					"<td><select name='Val5' style='width: 300;'>\r\n"
						"<option %s value='0'>Отключено</option>\r\n"
						"<option %s value='1'>Реальная</option>\r\n"
						"<option %s value='2'>Медленная</option>\r\n"
						"<option %s value='3'>Меняющаяся</option>\r\n"
						"</select></td>\r\n"				
				"<td><button type='submit'>Сохранить</button></td></tr>"
				
				"<tr bgcolor='LemonChiffon'><td>* Размер буффера записи на диск для одного файла (Аудио/Видео):</td>"
					"<td><select name='Val6' style='width: 300;'>\r\n"
						"<option %s value='0'>5Mb</option>\r\n"
						"<option %s value='1'>10Mb</option>\r\n"
						"<option %s value='2'>15Mb</option>\r\n"
						"<option %s value='3'>20Mb</option>\r\n"
						"<option %s value='4'>25Mb</option>\r\n"
						"<option %s value='5'>30Mb</option>\r\n"
						"<option %s value='6'>35Mb</option>\r\n"
						"<option %s value='7'>40Mb</option>\r\n"
						"<option %s value='8'>45Mb</option>\r\n"
						"<option %s value='9'>50Mb</option>\r\n"
						"<option %s value='10'>55Mb</option>\r\n"
						"<option %s value='11'>60Mb</option>\r\n"
						"<option %s value='12'>65Mb</option>\r\n"
						"<option %s value='13'>70Mb</option>\r\n"
						"<option %s value='14'>75Mb</option>\r\n"
						"<option %s value='15'>80Mb</option>\r\n"
						"</select></td>\r\n"				
				"<td><button type='submit'>Сохранить</button></td></tr>"
				
				"</form>\r\n",
				session->request_id,
				cnt, 
				cCaptureFilesView ? " checked " : "", cCaptureFilesLevel,
				cBackUpFilesView ? " checked " : "", cBackUpFilesLevel,
				(cCaptureFilesViewDef == 0) ? " selected " : "", 
				(cCaptureFilesViewDef == 1) ? " selected " : "", 
				(cCaptureFilesViewDef == 2) ? " selected " : "",				
				(cCaptureFilesViewDef == 3) ? " selected " : "",
				(uiFlvBufferSize == 0) ? " selected " : "", 
				(uiFlvBufferSize == 1) ? " selected " : "", 
				(uiFlvBufferSize == 2) ? " selected " : "",				
				(uiFlvBufferSize == 3) ? " selected " : "",
				(uiFlvBufferSize == 4) ? " selected " : "", 
				(uiFlvBufferSize == 5) ? " selected " : "", 
				(uiFlvBufferSize == 6) ? " selected " : "",				
				(uiFlvBufferSize == 7) ? " selected " : "",
				(uiFlvBufferSize == 8) ? " selected " : "", 
				(uiFlvBufferSize == 9) ? " selected " : "", 
				(uiFlvBufferSize == 10) ? " selected " : "",				
				(uiFlvBufferSize == 11) ? " selected " : "",
				(uiFlvBufferSize == 12) ? " selected " : "", 
				(uiFlvBufferSize == 13) ? " selected " : "", 
				(uiFlvBufferSize == 14) ? " selected " : "",				
				(uiFlvBufferSize == 15) ? " selected " : "");
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr bgcolor='Gold'><td>Адрес почты:</td><td><input type='text' name='Val1' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Gold'><td>Адрес сервера почты:</td><td><input type='text' name='Val2' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Gold'><td>Логин почты:</td><td><input type='text' name='Val3' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='Gold'><td>Пароль почты:</td><td><input type='text' name='Val4' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, 
				cMailAddress, cMailServer, cMailLogin, cMailPassword);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody,
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr bgcolor='Gold'><td>Тип авторизации почты:</td><td><input type='text' name='Val1' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td></td></tr>"
				"<tr bgcolor='Gold'><td>Уровень логирования на EMail:</td><td><input type='number' name='Val2' min=0 max=50 value='%i' style='width: 300px;'></td>\r\n"
				"<td></td></tr>"
				"<tr bgcolor='Gold'><td>Адрес получателя почты логирования:</td><td><input type='text' name='Val3' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td></td></tr>\r\n"
				"<tr bgcolor='Gold'><td>Пауза до повторной отсылки письма логирования (сек.):</td><td><input type='text' name='Val4' maxlength=63 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkSalmon'><td>Идентификатор звука о новых сообщениях:</td><td><input type='text' name='Val5' maxlength=4 value='%.4s' style='width: 300px;'></td>\r\n"
				"<td></td></tr>"
				"<tr bgcolor='DarkSalmon'><td>Пауза до повторного оповещения о новых сообщениях (сек.):</td><td><input type='text' name='Val6' maxlength=63 value='%i' style='width: 300px;'></td>\r\n"
				"<td></td></tr>"
				"<tr bgcolor='DarkSalmon'><td>Громкость звука:</td><td><input type='range' name='Val7' min='0' max='100' step='1' value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr></form>\r\n",
				session->request_id,
				cnt, 
				cMailAuth, Copy_iEmailLog, LogMailAddress, Copy_uiLogEmailPauseSize / 1000, (char*)&Copy_uiSoundMessageID, Copy_uiSoundMessagePauseSize / 1000, Copy_uiSoundMessageVolume);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody,
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr bgcolor='DarkKhaki'><td>Идентификатор терминала для отображения меню:</td><td><input type='text' name='Val1' maxlength=4 value='%.4s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkKhaki'><td>Ширина экрана для меню (px)</td><td><input type='number' name='Val2' min=0 max=5000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkKhaki'><td>Высота экрана для меню (px)</td><td><input type='number' name='Val3' min=0 max=5000 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkKhaki'><td>Ускорение вывода текста</td><td><input type='checkbox' name='Val4'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkKhaki'><td>Эталон даты и времени</td><td><input type='checkbox' name='Val5'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='DarkKhaki'><td>NTP сервер</td><td><input type='text' name='Val6' maxlength=63 value='%s' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"</form>\r\n",
				session->request_id,
				cnt, 
				(char*)&uiTerminalMenuID, uiMenuWidth, uiMenuHeight, 
				cAccelerateTextRender ? " checked " : "",
				cDateTimeReference ? " checked " : "",
				cNTPServer);
	strcat(msg_tx, msg_subbody);
	cnt++;
	memset(msg_subbody, 0, 3072);
	sprintf(msg_subbody, 
				"<form action='/settings/save'>\r\n"
				"<input type='hidden' name='req_index' value=%i>\r\n"
				"<input type='hidden' name='Num' value=%i>\r\n"
				"<tr bgcolor='LightGreen'><td>* ONVIF интерфейс:</td><td><input type='checkbox' name='Val1'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>* ONVIF авторизация:</td><td><input type='checkbox' name='Val2'%s></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"<tr bgcolor='LightGreen'><td>* ONVIF порт:</td><td><input type='number' name='Val3' min=1 max=65535 value='%i' style='width: 300px;'></td>\r\n"
				"<td><button type='submit'>Сохранить</button></td></tr>"
				"</form>\r\n",
				session->request_id,
				cnt, 
				uiOnvifStream ? " checked " : "",
				OnvifAuth ? " checked " : "",
				uiOnvifPort);
	strcat(msg_tx, msg_subbody);
	cnt++;
	DBG_MUTEX_UNLOCK(&system_mutex);
		
	strcat(msg_tx,	"</table><br />\r\n"
					"</body>\r\n"
					"</html>\r\n"
					"<p> * - Применяются после перезапуска</p>\r\n");		
	
	char buff[10];
	memset(buff, 0, 10);
	sprintf(buff, "%i", (int)strlen(msg_tx) - iHeadLen);
	if (strlen(buff) < 7) memcpy(&msg_tx[iPosLen], buff, strlen(buff));
		else dbgprintf(2, "Web page big size %s\n", buff);
	strcat(msg_tx,	"\r\n");
	
	DBG_FREE(msg_subbody);	
	return 1;
}

int WEB_404_respond(char *msg_rx, char *msg_tx)
{
	strcpy(msg_tx, "HTTP/1.1 404 Not Found\r\n"
					"Server: nginx/1.2.1\r\n"
					"Date: Sat, 08 Mar 2014 22:53:46 GMT\r\n"
					"Content-Type: text/html; charset=cp866\r\n"
					"Content-Length: 0\r\n"
					"Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT\r\n"
					"Accept-Ranges: bytes\r\n"
					"\r\n"
					"\r\n");
	return 1;
}

int WEB_media_play_respond(char *msg_rx, char *msg_tx, WEB_SESSION *session)
{
	char cUrl[2048];
	if (WEB_get_url(msg_rx, cUrl, 2048) == 0) return 0;
	if (WEB_get_param_from_url(cUrl, strlen(cUrl), "url", session->media_url, 256) <= 0) 
	{
		memset(session->media_url, 0, 256);
		WEB_main_respond(msg_rx, msg_tx, session);
		return 1;
	}
	WEB_decode_url_to_src(session->media_url, strlen(session->media_url));
	PlayURL(session->media_url, 0);
	WEB_media_respond(msg_rx, msg_tx, session);
	
	return 1;
}

static char WEB_describe_respond_401(WEB_SESSION *session, char *msg_rx, char *msg_tx)
{
	char msg_body[2048];
	memset(msg_body, 0, 2048);
	strcpy(msg_body,"<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\">"
					"<style>"
					"input[type=button], input[type=submit], input[type=reset], button[type=submit] {"
					"  background-color: #f4AA6D;"
					"  border: none;"
					"  color: white;"
					"  padding: 16px 32px;"
					"  text-decoration: none;"
					"  margin: 4px 2px;"
					"  cursor: pointer;"
					"}"
					"</style>"
					"</head>"
					"<body><a href=\"/\"><h1>401 Unauthorized.</h1></a></body>"
					"</html>\r\n");
    snprintf(msg_tx, 
             WEB_TX_BUF_SIZE_MAX, 
				"HTTP/1.0 401 Unauthorized\r\n"
				"Server: AZ daemon\r\n"
				"WWW-Authenticate: Digest realm=\"Streaming Server\", qop=\"auth\", nonce=\"%s\"\r\n"
				"Content-Type: text/html; charset=cp866\r\n"
				"Content-Length: %i\r\n"
				"\r\n"
				"%s"
				"\r\n", session->nonce, (int)strlen(msg_body), msg_body);
	//printf(msg_tx);
	return 1; 
}

char WEB_Auth(WEB_SESSION *session, char *msg_rx)
{	
	char *auth = strstr(msg_rx, AUTH_MARK);
	if (auth == NULL) {dbgprintf(4, "WEB: Not find AUTH_MARK\n"); return 0;}
	auth += strlen(AUTH_MARK);
    char *uri_start = strstr(auth, URI_MARK);
	if (uri_start == NULL) {dbgprintf(4, "WEB: Not find URI_MARK\n"); return 0;}
	uri_start += strlen(URI_MARK);
    char *uri_end = strstr(uri_start, "\"");
	if (uri_end == NULL) {dbgprintf(4, "WEB: Not find end URI_MARK\n"); return 0;}
	int urilen = uri_end - uri_start;
	if (urilen >= 4096) {dbgprintf(4, "WEB: Big len URI\n"); return 0;}
	char uri[4096];
	memset(uri, 0, 4096);
    memcpy(uri, uri_start, urilen);
	
	char *cnonce_start = strstr(auth, CNONCE_MARK);
	if (cnonce_start == NULL) {dbgprintf(4, "WEB: Not find CNONCE_MARK\n"); return 0;}
	cnonce_start += strlen(CNONCE_MARK);
    char *cnonce_end = strstr(cnonce_start, "\"");
	if (cnonce_end == NULL) {dbgprintf(4, "WEB: Not find end CNONCE_MARK\n"); return 0;}
	int cnoncelen = cnonce_end - cnonce_start;
	if (cnoncelen >= 128) {dbgprintf(4, "WEB: Big len CNONCE\n"); return 0;}
	char cnonce[128];
	memset(cnonce, 0, 128);
    memcpy(cnonce, cnonce_start, cnoncelen);
	
	char *nc_start = strstr(auth, NC_MARK);
	if (nc_start == NULL) {dbgprintf(4, "WEB: Not find NC_MARK\n"); return 0;}
	nc_start += strlen(NC_MARK);
    char *nc_end = strstr(nc_start, ",");
	if (nc_end == NULL) {dbgprintf(4, "WEB: Not find end NC_MARK\n"); return 0;}
	int nclen = nc_end - nc_start;
	if (nclen >= 128) {dbgprintf(4, "WEB: Big len CNONCE\n"); return 0;}
	char nc[128];
	memset(nc, 0, 128);
    memcpy(nc, nc_start, nclen);
	
	char *name_start = strstr(auth, USERNAME_MARK);
	if (name_start == NULL) {dbgprintf(4, "WEB: Not find USERNAME_MARK\n"); return 0;}
	name_start += strlen(USERNAME_MARK);
    char *name_end = strstr(name_start, "\"");
	if (name_end == NULL) {dbgprintf(4, "WEB: Not find end USERNAME_MARK\n"); return 0;}
	int namelen = name_end - name_start;
	if (namelen >= 128) {dbgprintf(4, "WEB: Big len USERNAME\n"); return 0;}
	char username[128];
	char password[128];
	memset(username, 0, 128);
    memcpy(username, name_start, namelen);
	memset(password, 0, 128);    
	
	char *resp_start = strstr(auth, RESPONSE_MARK);
	if (resp_start == NULL) {dbgprintf(4, "WEB: Not find RESPONSE_MARK\n"); return 0;}
	resp_start += strlen(RESPONSE_MARK);
    char *resp_end = strstr(resp_start, "\"");
	if (resp_end == NULL) {dbgprintf(4, "WEB: Not find end RESPONSE_MARK\n"); return 0;}
	int resplen = resp_end - resp_start;
	if (resplen >= 128) {dbgprintf(4, "WEB: Big len RESPONSE\n"); return 0;}
	char resp[128];
	memset(resp, 0, 128);
    memcpy(resp, resp_start, resplen);
	
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
			
	DBG_MUTEX_LOCK(&user_mutex);
	unsigned int n, ret = 0;
	for (n = 0; n < iUserCnt; n++)
	{
		if ((uiUserList[n].Enabled) && (uiUserList[n].Access & ACCESS_WEB) && (SearchStrInDataCaseIgn(uiUserList[n].Login, strlen(uiUserList[n].Login), 0, username)))
		{
			memset(session->login, 0, 64);
			memset(session->password, 0, 64);
			strcpy(session->login, uiUserList[n].Login);
			strcpy(session->password, uiUserList[n].Password);
			session->access = uiUserList[n].Access;
			session->access_level = uiUserList[n].Level;
			ret = 1;
			break;
		}
	}	
	DBG_MUTEX_UNLOCK(&user_mutex);
	if (ret != 1) 
	{
		WEB_Auth_lock_time = nanotime.tv_sec;
		dbgprintf(2, "WEB_Auth: wrong username '%s'\n", username);
		return -1;
	}
	
	HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;
	
    DigestCalcHA1("md5", session->login, "Streaming Server", session->password, session->nonce, cnonce, HA1);
    DigestCalcResponse(HA1, session->nonce, nc, cnonce, "auth", "GET", uri, HA2, Response);
    //printf("nonce SERV:'%s' %s %s\n", session->nonce,session->login, session->password);
	//printf("RESPONSE SERV:'%s' '%s'\n", Response, resp);
	if (strcmp(Response, resp) == 0) 
		return 1; 
		else
		{
			session->access = 0;
				
			WEB_Auth_lock_time = nanotime.tv_sec;			
			dbgprintf(2, "WEB_Auth: wrong password\n");
			return -1;
		}
}

void * thread_WEB_file(void* pData)
{
	pthread_setname_np(pthread_self(), "WEB_file");
	
	misc_buffer *mBuff = (misc_buffer*)pData;
	int ret = 0;
	switch(mBuff->flag)
	{
		case 1:
			if (CopyFile((char*)mBuff->void_data, (char*)mBuff->void_data2, NULL, 1, 0, 0, 0, NULL, 20, 20, 0) != 0) ret = 1;			
			//printf("Copy\n%s\nto\n%s\n", (char*)mBuff->void_data, (char*)mBuff->void_data2);			
			break;
		case 2:
			if (rename((char*)mBuff->void_data, (char*)mBuff->void_data2) != 0)
			{
				if (errno != EACCES)
				{
					if (CopyFile((char*)mBuff->void_data, (char*)mBuff->void_data2, NULL, 1, 1, 0, 0, NULL, 20, 20, 0) != 0) ret = 1;					
				}
			} else ret = 1;
			//printf("Remove\n%s\nto\n%s\n", (char*)mBuff->void_data, (char*)mBuff->void_data2);									
			break;
		case 3:
			if (remove((char*)mBuff->void_data) == 0) ret = 1;			
			//printf("Delete\n%s\n", (char*)mBuff->void_data);									
			break;
		case 4:
			if (rename((char*)mBuff->void_data, (char*)mBuff->void_data2) == 0) ret = 1;
			//printf("Rename\n%s\nto\n%s\n", (char*)mBuff->void_data, (char*)mBuff->void_data2);									
			break;
		case 5:
			if (mkdir((char*)mBuff->void_data, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) ret = 1;	
			//printf("Create\n%s\n\n", (char*)mBuff->void_data);									
			break;
		default:
			dbgprintf(2, "WEB unknown work for explorer: %i\n", mBuff->flag);
			break;
	}
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	if (mBuff->flag == 1) WEB_AddMessageInList("Копирование: '%s'\n", (char*)mBuff->void_data);
	if (mBuff->flag == 2) WEB_AddMessageInList("Перемещение: '%s'\n", (char*)mBuff->void_data);
	if (mBuff->flag == 3) WEB_AddMessageInList("Удаление: '%s'\n", (char*)mBuff->void_data);
	if (mBuff->flag == 4) WEB_AddMessageInList("Переименование: '%s'\n", (char*)mBuff->void_data);
	if (mBuff->flag == 5) WEB_AddMessageInList("Создание: '%s'\n", (char*)mBuff->void_data);
	if (ret == 1)
		WEB_AddMessageInList("Операция выполнена успешно\n");
		else WEB_AddMessageInList("Операция не выполнена\n");
	DBG_MUTEX_UNLOCK(&WEB_mutex);			
			
	DBG_FREE(mBuff->void_data);
	DBG_FREE(mBuff->void_data2);
	DBG_FREE(mBuff);
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	DBG_MUTEX_LOCK(&WEB_explorer_mutex);
	cExplorerBusy = 0;
	DBG_MUTEX_UNLOCK(&WEB_explorer_mutex);
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	
	return NULL;
}

void * thread_WEB_income(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "WEB_income");
	
	unsigned int *uiSettings = (unsigned int*)pData;
	unsigned int uiPort			= uiSettings[0];
	unsigned int request_id		= GenRandomInt(0x00FFFFFF);
	DBG_FREE(uiSettings);
	
	struct sockaddr_in      client_addr;
    struct sockaddr_in      client_port;  
	
	WEB_CLIENT_LIST			web_client_list[WEB_MAX_CLIENTS];
	memset(web_client_list, 0, WEB_MAX_CLIENTS*sizeof(WEB_CLIENT_LIST));
	
	unsigned int            addr_len, n, uiTime;
	
	int tcp_sock;
	WEB_SESSION *session;
	// Create socket. 
    int web_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(web_sock != -1);
	
	n = 1;
	if (setsockopt(web_sock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int)) < 0)
		dbgprintf(2, "WEB setsockopt(SO_REUSEADDR) failed\n");

	// Setup address. 
    client_addr.sin_family      = AF_INET;
    client_addr.sin_port        = htons(uiPort);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&client_addr.sin_zero, 0, 8);

    // Bind to address. 
    int rc = bind(web_sock,(struct sockaddr *) &client_addr, sizeof(client_addr)); 
    if (rc != 0)
	{
		dbgprintf(1, "thread_WEB_income: Error bind socket\n");
		close(web_sock);
		DBG_LOG_OUT();
		pthread_exit(0);
	}

    // Listening for traffic. 
    if (listen(web_sock, 5) == -1) 
	{
        dbgprintf(1,"Error listen web socket(%i) %s\n", errno, strerror(errno));
        close(web_sock);
		DBG_LOG_OUT();
		pthread_exit(0);
    }
	
	int flags = fcntl(web_sock, F_GETFL, 0);
	if (fcntl(web_sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(1,"error set web nonblock(%i) %s\n", errno, strerror(errno));
		close(web_sock);
		DBG_LOG_OUT();
        pthread_exit(0);
	}
	
	int iLoop = 1;
	char SignalBuff;
	
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, Web_Pair) < 0)
	{	
		dbgprintf(1,"error create socketpair\n");
		Web_Pair[0] = 0;
		Web_Pair[1] = 0;
		iLoop = 0;
	}
	else
	{
		flags = fcntl(Web_Pair[0], F_GETFL, 0);
		if (fcntl(Web_Pair[0], F_SETFL, flags | O_NONBLOCK) < 0)
		{
			dbgprintf(1,"error set socketpair nonblock\n");
			iLoop = 0;
		}
		else
		{
			flags = fcntl(Web_Pair[1], F_GETFL, 0);
			if (fcntl(Web_Pair[1], F_SETFL, flags | O_NONBLOCK) < 0)
			{
				dbgprintf(1,"error set socketpair nonblock\n");
				iLoop = 0;
			}
		}
	}
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	cThreadWebStatus++;
	WEB_Auth_lock_time = 0;
	DBG_MUTEX_UNLOCK(&WEB_mutex);	
	
	char cDisplayEnabled = 0;
	char cSpeakerEnabled = 0;
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if (ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1) >= 0) cDisplayEnabled = 1;
	if (ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1) >= 0) cSpeakerEnabled = 1;	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	int max_fd = -1;
	fd_set rfds;
	char ReadSignalReady = 1;
	struct timeval tv;
	int ret;

	while(iLoop)
    {
		FD_ZERO(&rfds);		
		FD_SET(web_sock, &rfds);
		max_fd = web_sock;
		if (ReadSignalReady == 0)
		{
			FD_SET(Web_Pair[1], &rfds);
			if (Web_Pair[1] > max_fd) max_fd = Web_Pair[1];
			tv.tv_sec = 1;
		} else tv.tv_sec = 0;
		
		tv.tv_usec = 0;		
		
		ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		
		if ((ReadSignalReady == 0) && (FD_ISSET(Web_Pair[1], &rfds))) ReadSignalReady = 1;
		if (ReadSignalReady == 1)
		{	
			ret = read(Web_Pair[1], &SignalBuff, 1);
			if ((ret > 0) && (SignalBuff == SIGNAL_CLOSE))
			{
				iLoop = 0;
				//printf("SIGNAL_CLOSE %i\n", conn_num);
				dbgprintf(4, "Close WEB signal\n");	
				break;
			}
			else
			{
				if (errno == EAGAIN) ReadSignalReady = 0;
				else 
				{
					dbgprintf(2, "Close WEB socketpair (errno:%i, %s)\n",errno, strerror(errno));
					break;
				}
			}
		}
		if (ret != 0)
		{
			if (FD_ISSET(web_sock, &rfds))
			{
				// Accept new TCP connection from specified address. 
				addr_len = sizeof(client_addr); 
				tcp_sock = accept(web_sock, (struct sockaddr *) &client_addr, &addr_len); 
				if (tcp_sock != -1)
				{
					session = (WEB_SESSION*)DBG_MALLOC(sizeof(WEB_SESSION));
					memset(session, 0, sizeof(WEB_SESSION));
					session->display_enabled = cDisplayEnabled;
					session->speaker_enabled = cSpeakerEnabled;
									
					addr_len = sizeof(client_port);
					getsockname(tcp_sock, (struct sockaddr*)&client_port, (socklen_t*)&addr_len);
					flags = fcntl(tcp_sock, F_GETFL, 0);
					if (fcntl(tcp_sock, F_SETFL, flags | O_NONBLOCK) == -1)
					{
						dbgprintf(1,"error set web nonblock accepted socket(%i) %s\n", errno, strerror(errno));
						close(tcp_sock);
						tcp_sock = -1;
					}
					session->ip   			= client_addr.sin_addr.s_addr;
					strcpy(session->ip_str, inet_ntoa(client_addr.sin_addr));
					session->port			= client_port.sin_port;
					session->web_port		= uiPort;
					DBG_MUTEX_LOCK(&system_mutex);
					session->auth			= WebAuth;
					session->maxtimeidle	= WebMaxTimeIdle;
					DBG_MUTEX_UNLOCK(&system_mutex);
					if (session->auth == 0) 
					{
						session->access = 0xFFFFFFFF; 
						session->access_level = 1000;
					}
					else 
					{
						session->access = 0;
						session->access_level = 0;
					}
					memset(session->login, 0, 64);
					memset(session->password, 0, 64);			
					session->socket 		= tcp_sock; 
					memset(session->media_url, 0, 256);
					if (session->auth)
					{
						for (n = 0; n < WEB_MAX_CLIENTS; n++)
						{
							if (web_client_list[n].status == 1)
							{
								uiTime = (unsigned int)(get_ms(&web_client_list[n].prev_time) / 1000);
								if (uiTime > session->maxtimeidle) 
								{
									dbgprintf(4, "Timeout session from:%i time:%is\n", session->ip_str, uiTime);
									memset(&web_client_list[n], 0, sizeof(WEB_CLIENT_LIST));							
								}
							}
						}
						for (n = 0; n < WEB_MAX_CLIENTS; n++)
						{
							if ((web_client_list[n].ip == session->ip) && (web_client_list[n].status == 1))
							{
								session->code = web_client_list[n].code;
								memcpy(session->nonce, web_client_list[n].nonce, 30);
								uiTime = (unsigned int)(get_ms(&web_client_list[n].prev_time) / 1000);
								DBG_MUTEX_LOCK(&WEB_req_mutex);
								session->request_id = web_client_list[n].request_id;
								session->p_request_id = &web_client_list[n].request_id;
								DBG_MUTEX_UNLOCK(&WEB_req_mutex);
								dbgprintf(4, "session from:%i lifetime:%is\n", session->ip_str, uiTime);
								break;
							}
						}
						if (n == WEB_MAX_CLIENTS)
						{
							for (n = 0; n < WEB_MAX_CLIENTS; n++)
							{
								if (web_client_list[n].status == 0)
								{
									web_client_list[n].status = 1;
									web_client_list[n].ip = session->ip;
									web_client_list[n].port = session->port;
									web_client_list[n].prev_time = 0;
									get_ms(&web_client_list[n].prev_time);
									web_client_list[n].code = GenRandomInt(0xFFFFFFFF);
									session->code = web_client_list[n].code;
									memcpy(&web_client_list[n].nonce[0], &web_client_list[n].ip, 4);
									memcpy(&web_client_list[n].nonce[4], &web_client_list[n].code, 4);
									memcpy(&web_client_list[n].nonce[8], &web_client_list[n].port, 2);
									memset(session->nonce, 0, 30);
									Char2Hex(session->nonce, (unsigned char*)web_client_list[n].nonce, 10, 0);
									LowerText(session->nonce);
									memcpy(web_client_list[n].nonce, session->nonce, 30);
									DBG_MUTEX_LOCK(&WEB_req_mutex);
									web_client_list[n].request_id = GenRandomInt(0x00FFFFFF);
									session->request_id = web_client_list[n].request_id;
									session->p_request_id = &web_client_list[n].request_id;
									DBG_MUTEX_UNLOCK(&WEB_req_mutex);
									dbgprintf(4, "NEW session from:%i nonce:%s\n", session->ip_str, session->nonce);
									break;
								}
							}
							if (n == WEB_MAX_CLIENTS) 
							{
								close(tcp_sock);
								tcp_sock = -1;
								dbgprintf(2, "overfull web clients %s\n", session->ip_str);
							}
						}
					}
					else 
					{
						DBG_MUTEX_LOCK(&WEB_req_mutex);
						session->request_id = request_id;
						session->p_request_id = &request_id;
						DBG_MUTEX_UNLOCK(&WEB_req_mutex);
					}
					if (tcp_sock != -1)
						pthread_create(&threadWEB_io, &tattrWEB_io, thread_WEB_io, (void*)session); 					
					else DBG_FREE(session);
				}
			}
		}
    }
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	if (Web_Pair[0] != 0) close(Web_Pair[0]);
	if (Web_Pair[1] != 0) close(Web_Pair[1]);
	Web_Pair[0] = 0;
	Web_Pair[1] = 0;
	cThreadWebStatus--;
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	    
	close(web_sock);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)0;
}

void * thread_WEB_io(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "WEB_io");
	
	WEB_SESSION				*session = (WEB_SESSION*)pData;
	
	unsigned int            id, msg_type, msg_act;
	unsigned int			request_id = 0;
	int						msg_val[64];
	float					msg_fl_val[20];
	int						errcode;
	int						iPage = -1;
	int						iPage2 = -1;
	char					*strValue = (char*)DBG_MALLOC(2048);
    char                    *msg_rx = (char*)DBG_MALLOC(WEB_RX_BUF_SIZE_DEF);
    char                    *msg_tx = (char*)DBG_MALLOC(WEB_TX_BUF_SIZE_MAX);
    int                     ret; 
	int 					iReadCnt;
	int						iReaded;
	int						iReaderSize = WEB_RX_BUF_SIZE_DEF;
	int 					iPageSize = 0;
	
	memset(msg_tx, 0, WEB_TX_BUF_SIZE_MAX);
	memset(msg_rx, 0, WEB_RX_BUF_SIZE_DEF);
	
	id = (unsigned int)(intptr_t)pData; 
	//memset(session->ip_str, 0, 16);
	//sprintf(session->ip_str, "%d.%d.%d.%d",
      //  ((unsigned char *) &session->ip)[0], ((unsigned char *) &session->ip)[1], ((unsigned char *) &session->ip)[2], ((unsigned char *) &session->ip)[3]); 	
	
    dbgprintf(5, "thread_WEB_io: New WEB server thread. [id = %d]\n", id); 
    dbgprintf(5, "thread_WEB_io: Client IP: %s\n", session->ip_str);

	session->head = (char*)DBG_MALLOC(4096);
	memset(session->head, 0, 4096);
	
	int iAuthLocked = 0;
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	iAuthLocked = (WEB_Auth_lock_time != 0) ? 1 : 0;
	
	if (WEB_Auth_lock_time && ((nanotime.tv_sec - WEB_Auth_lock_time) > WEB_AUTH_LOCK_TIME)) 
	{
		iAuthLocked = 0;
		WEB_Auth_lock_time = 0;
	}
	
	cThreadWebStatus++;
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	
	do
    {
		memset(msg_rx, 0, WEB_RX_BUF_SIZE_DEF);
		iReaded = 0;
		iReadCnt = 100;
		do
		{
			ret = read(session->socket, &msg_rx[iReaded], iReaderSize - iReaded); 
			if (ret < 1) 
			{
				if (ret == 0)
				{
					dbgprintf(5, "WEB_io: read error(closed)\n");	
					break; 
				}
				if (errno != EAGAIN)			
				{
					session->socket = 0;
					dbgprintf(5, "WEB_io: read error(errno:%i, %s)\n", errno, strerror(errno));
					break; 
				}
				//printf("wait data %i\n", iReadCnt);
				iReadCnt--;
				usleep(10000);	
			}
			else
			{
				iReadCnt = 10;
				iReaded += ret;
				if (iReaded == iReaderSize)
				{
					if (iReaded >= WEB_RX_BUF_SIZE_MAX) break;
					iReaderSize += WEB_RX_BUF_SIZE_DEF;
					msg_rx = (char*)DBG_REALLOC(msg_rx, iReaderSize);
					//printf("up in buffer to %i  %i\n", iReaderSize, iReaded);
				}
			}
		} while(iReadCnt);
		if (iReaded > 0)
		{
			DBG_MUTEX_LOCK(&WEB_mutex);
			//printf("WEB_io: WEB Request [session ID = %d]:\n", id); 
			memset(msg_tx, 0, WEB_TX_BUF_SIZE_MAX);
			memset(msg_val, 0, sizeof(int)*64);
			memset(msg_fl_val, 0, sizeof(float)*20);
			memset(strValue, 0, 2048);
			iPage = -1;
			errcode = WEB_get_msg_type(msg_rx, iReaded, &msg_type, &msg_act, &request_id, &iPage, &iPage2, msg_val, strValue, msg_fl_val);
			dbgprintf(5, "thread_WEB_io: WEB_get_msg_type type:%i, act:%i\n", msg_type, msg_act);
			
			if ((msg_act != WEB_ACT_NOTHING) && (msg_type != WEB_MSG_IMAGES) && (session->request_id != request_id))
			{
				//WEB_AddMessageInList("-------------------------");
				dbgprintf(3, "thread_WEB_io: wrong request_id, action %i, %i != %i\n", msg_act, session->request_id, request_id);	
				msg_type = WEB_MSG_START;
				msg_act = WEB_ACT_NOTHING;
			}
			if ((msg_act != WEB_ACT_NOTHING) && (msg_type != WEB_MSG_IMAGES)) 
			{
				DBG_MUTEX_UNLOCK(&WEB_mutex);
				DBG_MUTEX_LOCK(&WEB_req_mutex);
				(*session->p_request_id)++;
				if ((*session->p_request_id) > 80000000) *session->p_request_id = GenRandomInt(0x00FFFFFF);					
				session->request_id = *session->p_request_id;
				DBG_MUTEX_UNLOCK(&WEB_req_mutex);
				DBG_MUTEX_LOCK(&WEB_mutex);
			}			
				
			if ((msg_type == WEB_MSG_LOGOUT) && (session->auth)) session->auth = 2;
			
			if ((msg_type != WEB_MSG_UNKNOWN) 
				&& ((session->auth == 0) || ((session->auth) && (iAuthLocked == 0) && (WEB_Auth(session, msg_rx) == 1))))
			{
				//printf("#############\n%s#################\n\n", msg_rx);
						
				WEB_main_form(session->head, session->auth, session->access);
				if (session->auth) msg_type = WEB_test_access(msg_type, session->access);
						
				dbgprintf(5, "WEB_io: type=%s\n", WEB_GetWebMsgTypeName(msg_type));
				dbgprintf(5, "WEB_io: type=%s\n", WEB_GetWebActTypeName(msg_act));
				
				switch(msg_type)
				{
					case WEB_MSG_LOGOUT: 
						WEB_describe_respond_401(session, msg_rx, msg_tx);		
						break;				
					case WEB_MSG_GET_ICO: 
						//msg_type = WEB_MSG_ERROR;					
						break;				
					case WEB_MSG_START: 
						if (WEB_main_respond(msg_rx, msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_main_respond ERROR\n");
						}
						break;					
					case WEB_MSG_LOG: 
						if (WEB_log_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_log_respond ERROR\n");
						}
						break;					
					case WEB_MSG_MANUAL: 
						if (WEB_manual_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_manual_respond ERROR\n");
						}
						break;					
					case WEB_MSG_CONTROL: 
						if (msg_act == WEB_ACT_EXECUTE) WEB_control_execute(msg_val);
						if ((msg_act == WEB_ACT_CHANGE) && (msg_val[1] == 0)) add_sys_cmd_in_list(SYSTEM_CMD_SET_DIRLIST_POS, msg_val[0]);
						if ((msg_act == WEB_ACT_CHANGE) && (msg_val[1] == 1)) add_sys_cmd_in_list(SYSTEM_CMD_SET_PLAYLIST_POS, msg_val[0]);
						if (msg_act == WEB_ACT_NEXT) add_sys_cmd_in_list(SYSTEM_CMD_SHOW_NEXT, msg_val[0]);
						if (msg_act == WEB_ACT_PREV) add_sys_cmd_in_list(SYSTEM_CMD_SHOW_PREV, msg_val[0]);
						if (msg_act == WEB_ACT_STOP) add_sys_cmd_in_list(SYSTEM_CMD_SHOW_FULL_NEXT, msg_val[0]);
						if (msg_act == WEB_ACT_SHOW) add_sys_cmd_in_list(SYSTEM_CMD_SET_SHOW_MODE, msg_val[0]);
						if (msg_act == WEB_ACT_PAUSE) add_sys_cmd_in_list(SYSTEM_CMD_SHOW_PAUSE, msg_val[0]);
						if (msg_act == WEB_ACT_PLAY) add_sys_cmd_in_list(SYSTEM_CMD_SHOW_PLAY, msg_val[0]);
						if ((msg_act == WEB_ACT_SET_VOLUME) && session->speaker_enabled)
						{
							if (session->display_enabled)
								add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_SET, msg_val[0]);
								else 
								MusicVolumeSet(msg_val[0]);
						}
						if (msg_act == WEB_ACT_SET_PLAY_POS) add_sys_cmd_in_list(SYSTEM_CMD_PLAY_NEW_POS, msg_val[0]);
						if ((msg_act == WEB_ACT_REFRESH) && (msg_val[0] == 0)) add_sys_cmd_in_list(SYSTEM_CMD_PLAY_NEXT_DIR, msg_val[0]);
						if ((msg_act == WEB_ACT_REFRESH) && (msg_val[0] == 1)) add_sys_cmd_in_list(SYSTEM_CMD_PLAY_PREV_DIR, msg_val[0]);
						if ((msg_act == WEB_ACT_SET_VOLUME) || (msg_act == WEB_ACT_SET_PLAY_POS)) usleep(300000);
						if (WEB_control_respond(msg_rx, msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_control_respond ERROR\n");
						}
						break;
					case WEB_MSG_YOUTUBE:
						if (msg_act == WEB_ACT_REFRESH) WEB_youtube_update(msg_val);
						if (msg_act == WEB_ACT_OPEN) WEB_youtube_open(msg_tx, session, msg_val, strValue);
						if (msg_act == WEB_ACT_SHOW) WEB_youtube_show(msg_tx, session, msg_val, strValue);
						if (msg_act == WEB_ACT_LOAD) {WEB_youtube_load(msg_rx, msg_tx, session); break;}
						if (msg_act == WEB_ACT_SAVE) WEB_youtube_save(msg_rx, iReaded);
						if (WEB_youtube_respond(msg_rx, msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_youtube_respond ERROR\n");
						}
						break; 
					case WEB_MSG_MEDIA:
						if (msg_act == WEB_ACT_NOTHING)
						{
							if (WEB_media_respond(msg_rx, msg_tx, session) == 0)
							{
								msg_type = WEB_MSG_ERROR; 
								dbgprintf(2, "WEB_media_respond ERROR\n");
							}
						}			
						if (msg_act == WEB_ACT_PLAY)
						{
							if (WEB_media_play_respond(msg_rx, msg_tx, session) == 0)
							{
								msg_type = WEB_MSG_ERROR; 
								dbgprintf(2, "WEB_media_play_respond ERROR\n");
							} 		
						}
						break; 
					case WEB_MSG_MENU: 
						if (msg_act == WEB_ACT_PREV) WEB_menu_prev(msg_val[0]);							
						if (msg_act == WEB_ACT_PAGE) WEB_menu_next(msg_val[0], msg_val[1], session->access_level);	
						if (WEB_menu_respond(msg_rx, msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_menu_respond ERROR\n");
						}
						break;
					case WEB_MSG_HISTORY: 
						if (msg_act == WEB_ACT_DELETE) WEB_history_del(msg_val);
						if (WEB_history_respond(msg_tx, session, iPage) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_history_respond ERROR\n");
						}
						break;
					case WEB_MSG_CONNECTS: 
						if (msg_act == WEB_ACT_DELETE) WEB_connect_del(msg_val);
						if (WEB_connects_respond(msg_tx, session, iPage, iPage2) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_mic_respond ERROR\n");
						}
						break;
					case WEB_MSG_SYSTEM: 
						if (msg_act == WEB_ACT_SAVE) WEB_system_save(msg_val);
						if (msg_act == WEB_ACT_CHANGE) WEB_system_change();
						if (msg_act == WEB_ACT_STOP) WEB_system_stop(msg_val);
						if (msg_act == WEB_ACT_REBOOT) WEB_system_reboot();	
						if (msg_act == WEB_ACT_OFF) WEB_system_off();	
						if (msg_act == WEB_ACT_EXIT) WEB_system_exit();	
						if (msg_act == WEB_ACT_CLOSE) WEB_system_close();	
						if (msg_act == WEB_ACT_RESTART) WEB_system_restart();
						if (msg_act == WEB_ACT_REFRESH) WEB_system_reset();
						if (msg_act == WEB_ACT_UPDATE) WEB_system_update();
						if (WEB_system_respond(msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_system_respond ERROR\n");
						}
						break;
					case WEB_MSG_MIC: 
						if (msg_act == WEB_ACT_CHANGE) WEB_mic_change(msg_val);
						if (WEB_mic_respond(msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_mic_respond ERROR\n");
						}
						break;
					case WEB_MSG_CAMERA: 
						if (msg_act == WEB_ACT_REFRESH) WEB_camera_refresh(msg_val);							
						if (msg_act == WEB_ACT_CHANGE) WEB_camera_change(msg_val);
						if (WEB_camera_respond(msg_tx, session) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_camera_respond ERROR\n");
						}
						break;
					case WEB_MSG_IMAGES:
						iPageSize = WEB_images_respond(msg_tx, session, msg_act);
						if (iPageSize == 0)
						{
							//msg_type = WEB_MSG_ERROR; 
							dbgprintf(4, "WEB_images_respond ERROR\n");
						}
						break;
					case WEB_MSG_MESSAGES: 
						if (msg_act == WEB_ACT_DELETE) WEB_messages_del();
						if (WEB_messages_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_viewer_respond ERROR\n");
						}
						break;
					case WEB_MSG_VIEWER: 
						if (msg_act == WEB_ACT_REFRESH) WEB_viewer_refresh();							
						if (msg_act == WEB_ACT_DELETE) WEB_viewer_del();
						if (WEB_viewer_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_viewer_respond ERROR\n");
						}
						break;
					case WEB_MSG_EXPLORER: 
						ret = WEB_explorer_busy_test(msg_tx, session);
						if (ret == 0)
						{
							if (msg_act == WEB_ACT_PLAY) WEB_explorer_play(msg_val);							
							if (msg_act == WEB_ACT_SHOW) WEB_explorer_show(msg_val);							
							if (msg_act == WEB_ACT_REFRESH) WEB_explorer_refresh(msg_val);							
							if (msg_act == WEB_ACT_PREV) WEB_explorer_prev(msg_val);							
							if (msg_act == WEB_ACT_OPEN) ret = WEB_explorer_open(msg_val, session->access_level);
							if (msg_act == WEB_ACT_CHANGE) WEB_explorer_change(msg_val, session->access_level);	
							if (msg_act == WEB_ACT_WORK_REQUEST) 
							{
								if (WEB_explorer_work_request(msg_tx, session, msg_val, session->access_level) == 0)
								{
									msg_type = WEB_MSG_ERROR; 
									dbgprintf(2, "WEB_explorer_work_request ERROR\n");
								}
								ret = 10;
							}
							if (msg_act == WEB_ACT_WORK_ACCEPT) 
							{
								if (WEB_explorer_work_accept(msg_val, strValue, session->access_level) == 0)
								{
									msg_type = WEB_MSG_ERROR; 
									dbgprintf(2, "WEB_explorer_work_accept ERROR\n");
								}
							}
							if (ret == 0)
							{
								if (WEB_explorer_respond(msg_rx, msg_tx, session, iPage, iPage2, errcode) == 0)
								{
									msg_type = WEB_MSG_ERROR; 
									dbgprintf(2, "WEB_explorer_respond ERROR\n");
								}
							}
							if (ret == 1)
							{
								if (WEB_viewer_respond(msg_rx, msg_tx, session, 0, errcode) == 0)
								{
									msg_type = WEB_MSG_ERROR; 
									dbgprintf(2, "WEB_viewer_respond ERROR\n");
								}
							}
						}
						break;
					case WEB_MSG_SETTINGS: 
						if (msg_act == WEB_ACT_SAVE) WEB_settings_save(msg_val, strValue);
						if (WEB_settings_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_settings_respond ERROR\n");
						}	
						break;
					case WEB_MSG_MODSTATUSES: 
						if (msg_act == WEB_ACT_FILTER) WEB_modstatus_filters(msg_val);
						if (msg_act == WEB_ACT_LOAD) WEB_modstatus_load(msg_val);
						if (msg_act == WEB_ACT_PLAY) WEB_modstatus_play(msg_val);
						if (msg_act == WEB_ACT_SAVE) WEB_modstatus_save(msg_val);							
						if (msg_act == WEB_ACT_ADD) WEB_modstatus_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_modstatus_del(msg_val);
						if (msg_act == WEB_ACT_REFRESH) WEB_modstatus_refresh(msg_val);		
						if (msg_act == WEB_ACT_CHANGE) WEB_modstatus_change(msg_val, strValue);		
						if (msg_act == WEB_ACT_STOP) WEB_modstatus_stop(msg_val);
						if (msg_act == WEB_ACT_SHOW) WEB_modstatus_show(msg_val);
						if (WEB_modstatuses_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_modstatuses_respond ERROR\n");
						}	
						break;
					case WEB_MSG_SKIPEVENTS: 
						if (msg_act == WEB_ACT_SAVE) WEB_skipevent_save(msg_val);							
						if (msg_act == WEB_ACT_DELETE) WEB_skipevent_del(msg_val);
						if (WEB_skipevents_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_skipevents_respond ERROR\n");
						}	
						break;
					case WEB_MSG_ALIENKEYS: 
						if (msg_act == WEB_ACT_SAVE) WEB_alienkey_save(msg_val);							
						if (msg_act == WEB_ACT_DELETE) WEB_alienkey_del(msg_val);
						if (WEB_alienkeys_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_aleinkeys_respond ERROR\n");
						}	
						break;
					case WEB_MSG_SKIPIRCODES: 
						if (msg_act == WEB_ACT_SAVE) WEB_skipircode_save(msg_val);							
						if (msg_act == WEB_ACT_DELETE) WEB_skipircode_del(msg_val);
						if (WEB_skipircodes_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_skipircodes_respond ERROR\n");
						}	
						break;
					case WEB_MSG_RADIOS: 
						if (msg_act == WEB_ACT_SAVE) WEB_radio_save(msg_val, strValue, msg_fl_val);							
						if (msg_act == WEB_ACT_ADD) WEB_radio_add(msg_val, strValue, msg_fl_val);
						if (msg_act == WEB_ACT_DELETE) WEB_radio_del(msg_val);
						if (WEB_radios_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_radios_respond ERROR\n");
						}	
						break;
					case WEB_MSG_IRCODES: 
						if (msg_act == WEB_ACT_SAVE) WEB_ircode_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_ircode_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_ircode_del(msg_val);
						if (WEB_ircodes_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_ircodes_respond ERROR\n");
						}	
						break;
					case WEB_MSG_KEYS: 
						if (msg_act == WEB_ACT_SAVE) WEB_key_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_key_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_key_del(msg_val);
						if (WEB_keys_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_evntactions_respond ERROR\n");
						}	
						break;
					case WEB_MSG_EVNTACTIONS: 
						if (msg_act == WEB_ACT_RESTART) WEB_evntaction_restart();							
						if (msg_act == WEB_ACT_FILTER) WEB_evntaction_filters(msg_val, strValue);							
						if (msg_act == WEB_ACT_REFRESH) WEB_evntaction_refresh(msg_val);							
						if (msg_act == WEB_ACT_UP) WEB_evntaction_up(msg_val);							
						if (msg_act == WEB_ACT_DOWN) WEB_evntaction_down(msg_val);							
						if (msg_act == WEB_ACT_SAVE) WEB_evntaction_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_evntaction_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_evntaction_del(msg_val);
						if (msg_act == WEB_ACT_EXECUTE) WEB_evntaction_exec(msg_val);						
						if (msg_act == WEB_ACT_RECOVER) WEB_evntaction_recover(msg_val);
						if (WEB_evntactions_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_evntactions_respond ERROR\n");
						}	
						break;
					case WEB_MSG_MNLACTIONS: 
						if (msg_act == WEB_ACT_SAVE) WEB_mnlaction_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_mnlaction_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_mnlaction_del(msg_val);
						if (msg_act == WEB_ACT_EXECUTE) WEB_mnlaction_exec(msg_val);						
						if (WEB_mnlactions_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_mnlactions_respond ERROR\n");
						}	
						break;
					case WEB_MSG_CAMRECTS: 
						if (msg_act == WEB_ACT_SHOW) WEB_camrect_view(msg_val);
						if (msg_act == WEB_ACT_SAVE) WEB_camrect_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_camrect_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_camrect_del(msg_val);
						if (WEB_camrects_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_camrects_respond ERROR\n");
						}	
						break;
					case WEB_MSG_DIRECTORIES: 
						if (msg_act == WEB_ACT_OPEN) WEB_directory_open(msg_val, strValue);							
						if (msg_act == WEB_ACT_PLAY) WEB_directory_play(msg_val, strValue);							
						if (msg_act == WEB_ACT_SAVE) WEB_directory_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_directory_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_directory_del(msg_val);
						if (WEB_directories_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_directories_respond ERROR\n");
						}	
						break;
					case WEB_MSG_WIDGETS: 
						if (msg_act == WEB_ACT_UP) WEB_widget_up(msg_val);							
						if (msg_act == WEB_ACT_DOWN) WEB_widget_down(msg_val);							
						if (msg_act == WEB_ACT_SAVE) WEB_widget_save(msg_val, strValue, msg_fl_val);							
						if (msg_act == WEB_ACT_ADD) WEB_widget_add(msg_val, strValue, msg_fl_val);
						if (msg_act == WEB_ACT_DELETE) WEB_widget_del(msg_val);
						if (msg_act == WEB_ACT_RECOVER) WEB_widget_recover(msg_val);
						if (WEB_widgets_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_widgets_respond ERROR\n");
						}	
						break;
					case WEB_MSG_STREAMS: 
						if (msg_act == WEB_ACT_FILTER) WEB_streams_filters(msg_val);
						if (msg_act == WEB_ACT_UP) WEB_stream_up(msg_val);							
						if (msg_act == WEB_ACT_DOWN) WEB_stream_down(msg_val);							
						if (msg_act == WEB_ACT_PLAY) WEB_stream_play(msg_val);							
						if (msg_act == WEB_ACT_SAVE) WEB_stream_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_stream_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_stream_del(msg_val);
						if (WEB_streams_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_streams_respond ERROR\n");
						}	
						break;
					case WEB_MSG_STREAMTYPES: 
						if (msg_act == WEB_ACT_SAVE) WEB_streamtype_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_streamtype_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_streamtype_del(msg_val);
						if (WEB_streamtypes_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_streamtypes_respond ERROR\n");
						}	
						break;
					case WEB_MSG_MAILS: 
						if (msg_act == WEB_ACT_SAVE) WEB_mail_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_mail_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_mail_del(msg_val);
						if (WEB_mails_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_mails_respond ERROR\n");
						}	
						break;
					case WEB_MSG_SOUNDS: 					
						if (msg_act == WEB_ACT_PLAY) WEB_sound_play(msg_val);							
						if (msg_act == WEB_ACT_SAVE) WEB_sound_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_sound_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_sound_del(msg_val);
						if (WEB_sounds_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_sound_respond ERROR\n");
						}	
						break;
					case WEB_MSG_USERS: 
						if (msg_act == WEB_ACT_SAVE) WEB_user_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_user_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_user_del(msg_val);
						if (WEB_users_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_users_respond ERROR\n");
						}	
						break;
					case WEB_MSG_ALARMS: 
						if (msg_act == WEB_ACT_SAVE) WEB_alarm_save(msg_val, strValue);							
						if (msg_act == WEB_ACT_ADD) WEB_alarm_add(msg_val, strValue);
						if (msg_act == WEB_ACT_DELETE) WEB_alarm_del(msg_val);
						if (WEB_alarms_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_alarms_respond ERROR\n");
						}	
						break;
					case WEB_MSG_MODULES: 
						if (errcode == 1)
						{
							if (msg_act == WEB_ACT_CHANGE) WEB_module_change(msg_val, strValue);							
							if (msg_act == WEB_ACT_SAVE) WEB_module_save(msg_val, strValue);							
							if (msg_act == WEB_ACT_ADD) WEB_module_add(msg_val, strValue);
							if (msg_act == WEB_ACT_DELETE) WEB_module_del(msg_val);
							if (msg_act == WEB_ACT_RECOVER) WEB_module_recover(msg_val);							
						}		
						if (msg_act == WEB_ACT_FILTER) WEB_module_filters(msg_val);
						if (WEB_modules_respond(msg_rx, msg_tx, session, iPage, errcode) == 0)
						{
							msg_type = WEB_MSG_ERROR; 
							dbgprintf(2, "WEB_modules_respond ERROR\n");
						}	
						break;
					default:
						msg_type = WEB_MSG_ERROR; 
						break; 
				}
			}
			else
			{
				if (session->auth) 
				{
					WEB_describe_respond_401(session, msg_rx, msg_tx);
					dbgprintf(4, "WEB_io: Error authentifycation\n"); 
					//msg_type = WEB_MSG_TEARDOWN;
					//printf("%s\n", msg_rx);
				}
			}
			
			WEB_AddWebHistory(1, session->ip_str, msg_type, msg_act, session->auth, session->login);
	
			DBG_MUTEX_UNLOCK(&WEB_mutex);	

			/*if (msg_act != WEB_ACT_NOTHING) 
			{
				DBG_MUTEX_LOCK(&WEB_req_mutex);
				*session->p_request_id = session->request_id;
				DBG_MUTEX_UNLOCK(&WEB_req_mutex);
			}	*/		
			
			if (msg_type == WEB_MSG_ERROR) 
			{
				dbgprintf(2, "WEB_MSG_ERROR\n");
				//printf("%s\n", msg_rx);
				WEB_404_respond(msg_rx, msg_tx);
				msg_type = WEB_MSG_TEARDOWN;
				break;
			}
			if (msg_type == WEB_MSG_TEARDOWN)
			{
				dbgprintf(5, "WEB_MSG_TEARDOWN \n"); 			
				break; 
			}
			//printf("R########################\n");
			//printf("%s\n", msg_rx); 
			//printf("########################\n");
			if (msg_type != WEB_MSG_IMAGES) iPageSize = strlen(msg_tx);
		}
		int iSended = 0;
			
		while(iPageSize)
		{
			ret = write(session->socket, &msg_tx[iSended], iPageSize);			
			if (ret < 1) 
			{
				if (errno == EAGAIN) 
				{
					dbgprintf(5, "WEB_io: write error (errno:%i, %s)\n", errno, strerror(errno));	
					//printf("No data on %i\n", n);
				}
				else
				{
					//session->socket = 0;
					dbgprintf(5, "WEB_io: Write TCP error. [client IP: %s]\n", session->ip_str); 
					break; //EXIT
				}								
			}
			else
			{
				iSended += ret;
				iPageSize -= ret;
			}
		}			
    } while(0);	
	
	dbgprintf(5, "WEB_io: Done. [client IP: %s]\n", session->ip_str); 
	
	if (session->socket > 0) 
	{
		close(session->socket);
		session->socket = 0;		
	}
	DBG_FREE(session->head);
	DBG_FREE(session);
	DBG_FREE(msg_rx);
	DBG_FREE(msg_tx);
	DBG_FREE(strValue);
	DBG_MUTEX_LOCK(&WEB_mutex);
	cThreadWebStatus--;
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)0;
}

void WEB_ClearMessageList()
{
	if (iWebMessListCnt != 0) 
	{
		DBG_FREE(cWebMessList);
		cWebMessList = NULL;
		iWebMessListCnt = 0;	
	}
}

void WEB_AddMessageInList(char *cMessage, ...)
{
	if (iWebMessListCnt < WEB_MESSAGES_MAX_CNT)
	{
		int iMessageLen = strlen(cMessage) * 2 + 64;
		char * textbuffer = (char*)DBG_MALLOC(iMessageLen);
		
		/*time_t rawtime;
		struct tm timeinfo;
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);*/
		
		memset(textbuffer, 0 , iMessageLen);		
		va_list valist;
		va_start(valist, cMessage);	
		vsprintf(textbuffer, cMessage, valist);	
		va_end(valist);
			
		int ret = strlen(textbuffer);							
		
		iWebMessListCnt++;		
		cWebMessList = (char*)DBG_REALLOC(cWebMessList, iWebMessListCnt * WEB_MESSAGES_MAX_LEN);								
		//else memmove(cWebMessList, &cWebMessList[WEB_MESSAGES_MAX_LEN], WEB_MESSAGES_MAX_LEN * (WEB_MESSAGES_MAX_CNT-1));
		memset(&cWebMessList[(iWebMessListCnt-1) * WEB_MESSAGES_MAX_LEN],0, WEB_MESSAGES_MAX_LEN);
		if (ret >= WEB_MESSAGES_MAX_LEN) memcpy(&cWebMessList[(iWebMessListCnt-1) * WEB_MESSAGES_MAX_LEN], textbuffer, WEB_MESSAGES_MAX_LEN - 1);
			else memcpy(&cWebMessList[(iWebMessListCnt-1) * WEB_MESSAGES_MAX_LEN], textbuffer, ret);
		
		DBG_FREE(textbuffer);
	}
}

void WEB_GetMessageList(char *msg_tx)
{
	int n;
	for (n = 0; n < iWebMessListCnt; n++)
	{
		strcat(msg_tx, "<p style='color:#ff0000'>");
		strcat(msg_tx, &cWebMessList[n * WEB_MESSAGES_MAX_LEN]);
		strcat(msg_tx, "</p>\r\n");
	}	
	WEB_ClearMessageList();			
}


void WEB_ClearWebHistory()
{
	if (iWebHistoryCnt != 0) 
	{
		DBG_FREE(whWebHistory);
		whWebHistory = NULL;
		iWebHistoryCnt = 0;	
	}	
}

void WEB_AddWebHistory(int iProt, char *cAddr, unsigned int uiType, unsigned int uiAct, int Auth, char *cLogin)
{
	if (iWebHistoryCnt < WEB_HISTORY_MAX_CNT)
	{
		iWebHistoryCnt++;
		whWebHistory = (WEB_HISTORY_INFO*)DBG_REALLOC(whWebHistory, iWebHistoryCnt * sizeof(WEB_HISTORY_INFO));
	} 
	else memmove(whWebHistory, &whWebHistory[1], sizeof(WEB_HISTORY_INFO) * (WEB_HISTORY_MAX_CNT-1));
	memset(&whWebHistory[iWebHistoryCnt-1], 0, sizeof(WEB_HISTORY_INFO));	
	char bb[32];
	strcpy(whWebHistory[iWebHistoryCnt-1].Date, GetCurrDateTimeStr(bb, 32));
	whWebHistory[iWebHistoryCnt-1].Prot = iProt;
	strcpy(whWebHistory[iWebHistoryCnt-1].Addr, cAddr);
	whWebHistory[iWebHistoryCnt-1].Type = uiType;
	whWebHistory[iWebHistoryCnt-1].Action = uiAct;		
	whWebHistory[iWebHistoryCnt-1].Auth = Auth;		
	strcpy(whWebHistory[iWebHistoryCnt-1].Login, cLogin);
}

