
//#include <curses.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "bcm_host.h"
#include "iconv.h"
#include <sys/vfs.h>
#include <linux/mempolicy.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "linux/input.h"

#include "interface/vmcs_host/vc_dispmanx.h"
#include "interface/vmcs_host/vc_vchi_gencmd.h"
#include "interface/vmcs_host/vc_vchi_bufman.h"
#include "interface/vmcs_host/vc_tvservice.h"
#include "interface/vmcs_host/vc_cecservice.h"
#include "interface/vchiq_arm/vchiq_if.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include <pthread.h>
#include <GLES2/gl2.h>

#include "main.h"
#include "omx_client.h" 
#include "audio.h"
#include "widgets.h"
#include "text_func.h"
#include "weather_func.h"
#include "gpio.h"
#include "nfc.h"
#include "rc522.h"
#include "network.h"
#include "flv-muxer.h"
#include "debug.h"
#include "rtsp.h"
#include "onvif.h"
#include "web.h"
#include "streamer.h"
#include "writer.h"
#include "system.h"
#include "ir_control.h"
#include "tfp625a.h"

#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#define PATH "./"

#define MENU_BACK_SPEED 					3
#define MENU_FORW_SPEED 					2
#define MENU_IR_SPEED 						400
#define SKIPEVENT_MAX_CNT					20
#define ALIENKEY_MAX_CNT					20
#define SKIPIRCODE_MAX_CNT					20
#define MAX_CAMERA_LIST_CNT					35
#define CAMERA_RECONNECT_TIME				1500
#define TIMEWAIT_FREE_SPACE_REPEAT 			4

#define FILE_VIEW_TIME						350
#define FILE_SHOW_TIME						3500
#define DIR_SETTINGS						"Settings"

#ifndef M_PI
   #define M_PI 3.141592654
#endif

#define SHOW_DISABLED			0
#define SHOW_ENABLED			1
#define SHOW_DEFAULT			2

#define MAX_FILE_DELETE			100

#define NTP_TIMESTAMP_DELTA 2208988800ull
#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) // (li   & 11 000 000) >> 6
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) // (vn   & 00 111 000) >> 3
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) // (mode & 00 000 111) >> 0


#define DBG						1

typedef int (*tfSaveFunc)(int, void*, void*);
 
static void Init_Ortho(GL_STATE_T *state);
static void Init_GL(GL_STATE_T *state);
static void Rendering(GL_STATE_T *state, int iSlideShowRenderStatus, int cvShowType, int iSync, CAMERA_LIST_INFO * clCamList, int iCameraListCnt);
void RenderText(GL_STATE_T *pstate, int iTextSize, GLfloat gX, GLfloat gY, char cStatus, char *cText, ...);
void RenderGraph(GL_STATE_T *pstate, GLfloat gX, GLfloat gY, GLfloat gScX, GLfloat gScY);
static void Init_Textures(GL_STATE_T *state, char cZoomSet);
//static void Load_Tex_Images(GL_STATE_T *state);
static void Exit_Func(void);
int Menu_CameraAction(void* pData, int iNum);
int Menu_ModuleAction(void* pData, int iNum);
int Menu_ArchivAction(void *pData, int iNum);
int Menu_GetModuleStatus(void* pData, int iNum);
int Menu_GpioAction(void* pData, int iNum);
int Menu_ActionEventSet(void* pData, int iNum);
int Menu_ActionMenuExec(void *pData, int iNum);
int Menu_GetNext(void* pData, int iMenuNum);
int PlayForward(int iMenuNum);
int PlayBackward(int iMenuNum);
int Menu_GetAgain(void* pData, int iMenuNum);
int Menu_GetPrev(void* pData, int iMenuNum);
int Menu_SwitchPlayOrder(void* pData, int iNum);
int Menu_SwitchDirOrder(void* pData, int iNum);
char GetChangeShowNow();
int GetShowTimeMax();
void SetShowTimeMax(int val);
unsigned int GetNewShowType();
unsigned int GetCurShowType();
void SetNewShowType(unsigned int val);
void SetCurShowType(unsigned int val);
void AddNewShowType(unsigned int val);
void AddCurShowType(unsigned int val);
void DelCurShowType(unsigned int val);
void SetShowMenu(char val);
char GetShowMenu();
void RenderGLGraph(GL_STATE_T *pstate, GLfloat gX, GLfloat gY, GLfloat gScX, GLfloat gScY);
float get_sys_temp();
float get_sys_volt();
int get_sys_core();
int Menu_SwitchPlayDirMode(void* pData, int iNum);
int Menu_SwitchPlayMode(void* pData, int iNum);
int Menu_RefreshAlarmsMenu(void *pData, int iMenuNum);
int Menu_RefreshRadiosMenu(void *pData, int iMenuNum);
int Menu_AlarmClockSwitch(void *pData, int iNum);
int Menu_PlaySelectedFile(void* pData, int iNum);
int Menu_PlaySelectedDir(void* pData, int iNum);
int Menu_RefreshMessageListMenu(void *pData, int iMenuNum);
int utf8_to_cp866(char *cSourceStr, unsigned int uiSourceLen, char *cDestStr, unsigned int *uiDestLen);
char* GetModuleTypeShowName(int iCode);
int SaveModuleStatuses(int iMode, void *pSaveData, void *pCaptSet);
int SaveCapturedData(char cSplit,
						capture_settings *capt_set, 
						char *cCapturePath, 
						char *cBackUpPath, 
						unsigned int uiCaptureSizeLimit, 
						unsigned int uiCaptureTimeSize, 
						unsigned char ucCaptOrderLmt,
						unsigned int ucBackUpCaptured, 
						char* cAddrOrderer,
						char* cPrePath,
						char* cType,
						unsigned int uiID,
						void *SFunc,
						void *pSaveData,
						unsigned int uiMaxWaitCopyTime, 
						unsigned int uiMessWaitCopyTime,
						unsigned int uiDestType);

static struct termios initial_settings, new_settings;
static volatile int terminate;
static GL_STATE_T _state, *state=&_state;

static pthread_t threadVideoCapture;
static pthread_t threadShower;
static pthread_t threadStreamer;
static pthread_t threadWriter;
static pthread_t threadEventer;
static pthread_t threadScaner;
static pthread_t threadCardReader;
static pthread_t threadFileIO;
static pthread_t threadFinger;
pthread_attr_t tattrVideoCapture;
pthread_attr_t tattrShower;
pthread_attr_t tattrStreamer;
pthread_attr_t tattrWriter;
pthread_attr_t tattrEventer;
pthread_attr_t tattrScaner;
pthread_attr_t tattrCardReader;
//pthread_attr_t tattrPlayMedia;
pthread_attr_t tattrFileIO;
pthread_attr_t tattrFinger;

FILE *keyboard_file = NULL;
//char keyboard_map[KEY_MAX/8 + 1];
//struct input_event keyboard_event[64];

static void* eglImage = NULL;
// static pthread_t thread1;

int iListAlarmFilesCount=0;
LIST_FILES_TYPE *cListAlarmFiles=NULL;

//char cCurrentDir[256];
char cCurrentAlarmDir[256];

int iShowerLiveControl = MAX_SYSTEM_INFO_LIVE;

int iRefreshClk = 0;
int iChangeDirTime = ((100 / 3) + (5 * 5)) * 60 * 30;
char cKeyDownClock = 0;

int	iFade = 255;

int iShowTimeMax = 20;
int iRadioTimeMax = 0;
int iCurrentVolume = 50;
unsigned int uiTimerShowVolume = 0;
unsigned int uiTimerShowTimer = 0;

char cCurrentFile[256];
char cNextPlayPath[256];

int iSysMessageCnt = 0;
SYS_MESSAGE_INFO smiSysMessage[MAX_SYS_MESSAGE_LEN];

int iCurrentMenuPage = 0;

int CountVideos = 0;
int CountImages = 0;
char cShowMenu = 0;
unsigned int cNewShowType = 0;

char cChangeShowNow = 0;
int iTimerToNextShow = 0;
unsigned int iMenuKeyCode = 0;

char cKeyMem = 0;
char cKeyUpClock = 0;
unsigned int cMenuBackClk = 0;
char cMenuForwClk = 0;
	
char cSeek = 0;

float incr = 0.0f;

char cThreadShowerStatus;
char cThreadEventerStatus;
char cThreadReaderStatus;

int64_t iAccessTimer = 0;
unsigned int iAccessLevelCopy;	
unsigned int uiNextCameraID;
unsigned int uiNextMicID;
unsigned int uiCurCameraID;
unsigned int uiCurMicID;
unsigned int iRadioCode = 0;
unsigned int iVolumeCode = 0;
int iListFilesOrderCnt = 0;
int iListDirsOrderCnt = 0;
char cPlayDirect = 0;
unsigned int version[4];
   
MENU_PAGE *pScreenMenu;

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/times.h"
#include "sys/vtimes.h"

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

void to_freeze()
{
	while(1) usleep(60000000);
}

int FillConfigPath(char *cPath, unsigned int uiLen, char *cFile, char cCreate)
{	
	memset(cPath, 0, uiLen);
	getcwd(cPath, uiLen);
	strcat(cPath, "/");
	if (strlen(DIR_SETTINGS)) 
	{
		strcpy(cPath, DIR_SETTINGS);
		strcat(cPath, "/");
	}
	strcat(cPath, cFile);
	if (cCreate) CreateDirectoryFromPath(cPath, 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	return 1;
}

char *GetDayName(int iDayNum)
{
	switch(iDayNum)
	{
		case 0: return "Воскресенье";
		case 1: return "Понедельник";
		case 2: return "Вторник";
		case 3: return "Среда";
		case 4: return "Четверг";
		case 5: return "Пятница";
		case 6: return "Суббота";
		default: return "Воскресенье";
	}
	return "";
}

void GetDateStr(char *buff, int iLen)
{
	memset(buff, 0, iLen);
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	sprintf(buff, "%.2i.%.2i.%.4i %s", 
						timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, GetDayName(timeinfo.tm_wday));
}

void GetShowModeStr(char mode, char *buff, int iLen)
{
	memset(buff, 0, iLen);
	switch (mode)
	{
		case 0: 
			strcpy(buff, "Откл.");
			break;
		case 1: 
			strcpy(buff, "Сон");
			break;
		case 2: 
			strcpy(buff, "Вкл.");
			break;
		default:
			strcpy(buff, "Неизв.");
			break;
	}
}

int Menu_ChangeShowMode(void *pData, int iMenuNum)
{
	DBG_MUTEX_LOCK(&system_mutex);
	uiShowModeCur++;
	if (uiShowModeCur > 2) uiShowModeCur = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
	SetChangeShowNow(1);
	SetNewShowType(SHOW_TYPE_NEW);
	pScreenMenu[0].Options[20].Show = 0;
	//'printf("stopped streams SYSTEM_CMD_SET_SHOW_MODE\n");
	/*Media_StopPlay(0);
	omx_stop_video(0);
	Audio_Stop(0);
	//printf("stopped streams\n");
	CloseAllConnects(CONNECT_CLIENT, 0);	
	//CloseAllConnects(CONNECT_SERVER, 0xFFFF);		*/
	return 1;
}

void SetShowMode(char val)
{
	DBG_MUTEX_LOCK(&system_mutex);
	if (val == 3) 
	{
		uiShowModeCur++;
		if (uiShowModeCur == 3) uiShowModeCur = 0;
	}
	else uiShowModeCur = val;
	DBG_MUTEX_UNLOCK(&system_mutex);
	SetChangeShowNow(1);
	SetNewShowType(SHOW_TYPE_NEW);
	pScreenMenu[0].Options[20].Show = 0;
	//'printf("stopped streams SYSTEM_CMD_SET_SHOW_MODE\n");
	/*Media_StopPlay(0);
	omx_stop_video(0);
	Audio_Stop(0);	
	//printf("stopped streams\n");
	CloseAllConnects(CONNECT_CLIENT, 0);	
	//CloseAllConnects(CONNECT_SERVER, 0xFFFF);		*/
}
					
void GetDateTimeStr(char *buff, int iLen)
{
	memset(buff, 0, iLen);
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	sprintf(buff, "%s %.2i.%.2i.%.4i %.2i:%.2i:%.2i", GetDayName(timeinfo.tm_wday),
					timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900,
					timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

int GetNtpServerTime(char *host_name, time_t *rawtime)
{
	int sockfd, n; // Socket file descriptor and the n return result from writing/reading from the socket.
	int portno = 123; // NTP UDP port number.
	//char* host_name = "us.pool.ntp.org"; // NTP server host-name.

	// Structure that defines the 48 byte NTP packet protocol.
	typedef struct
	{
			uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
									// li.   Two bits.   Leap indicator.
									// vn.   Three bits. Version number of the protocol.
									// mode. Three bits. Client will pick mode 3 for client.

			uint8_t stratum;         // Eight bits. Stratum level of the local clock.
			uint8_t poll;            // Eight bits. Maximum interval between successive messages.
			uint8_t precision;       // Eight bits. Precision of the local clock.

			uint32_t rootDelay;      // 32 bits. Total round trip delay time.
			uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
			uint32_t refId;          // 32 bits. Reference clock identifier.

			uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
			uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

			uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
			uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

			uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
			uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

			uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
			uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.
	} ntp_packet;              // Total: 384 bits or 48 bytes.

	// Create and zero out the packet. All 48 bytes worth.
	ntp_packet packet;
	memset(&packet, 0, sizeof(ntp_packet));

	// Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.

	*( ( char * ) &packet + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.

	// Create a UDP socket, convert the host-name to an IP address, set the port number,
	// connect to the server, send the packet, and then read in the return packet.

	struct sockaddr_in serv_addr; // Server address data structure.
	struct hostent *server;      // Server data structure.
	fd_set rfds, wfds;
	struct timeval tv;
	int ret;
	
	sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.
	if ( sockfd < 0 ) 
	{
		dbgprintf(2, "NTP: ERROR opening socket\n");
		return 0;
	}
	
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(2,"NTP: error set nonblock NTP\n");
		close(sockfd);
		return 0;
	}
	
	server = gethostbyname(host_name); // Convert URL to IP.
	if ( server == NULL ) 
	{
		dbgprintf(2, "NTP: ERROR, no such host:'%s'\n", host_name);
		close(sockfd);
		return 0;
	}

	// Zero out the server address structure.
	bzero( ( char* ) &serv_addr, sizeof( serv_addr ) );
	serv_addr.sin_family = AF_INET;

	// Copy the server's IP address to the server address structure.
	bcopy( ( char* )server->h_addr, ( char* ) &serv_addr.sin_addr.s_addr, server->h_length );

	// Convert the port number integer to network big-endian style and save it to the server address structure.
	serv_addr.sin_port = htons( portno );
	
	// Call up the server using its IP address and port number.
	ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof( serv_addr));
	if (ret != 0) 
	{
		if (errno == EINPROGRESS) 
		{
			//printf("connecting\n");
			FD_ZERO(&wfds);
			FD_ZERO(&rfds);
			FD_SET(sockfd, &wfds);
			FD_SET(sockfd, &rfds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;	
			ret = select(sockfd + 1, &rfds, &wfds, NULL, &tv);
			if (ret == 0)
			{
				dbgprintf(2,"NTP: timeout connect connect\n");
				shutdown(sockfd, SHUT_RDWR);
				close(sockfd);
				return 0;			
			}			
		}
		else		
		{
			dbgprintf(2,"NTP: error connect, get out\n");
			close(sockfd);
			return 0;
		}				
	}
	
	// Send it the NTP packet it wants. If n == -1, it failed.
	n = write(sockfd, (char*) &packet, sizeof(ntp_packet));
	if (n < 0) 
	{
		dbgprintf(2, "NTP: ERROR writing to socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
		return 0;
	}
	
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;	
	ret = select(sockfd + 1, &rfds, NULL, NULL, &tv);
	if (ret == 0)
	{
		dbgprintf(2,"NTP: timeout read\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
		return 0;			
	}	
	if (!FD_ISSET(sockfd, &rfds))
	{
		dbgprintf(3,"NTP: no read signal\n");
	}
	
	// Wait and receive the packet back from the server. If n == -1, it failed.
	n = read(sockfd, ( char* ) &packet, sizeof(ntp_packet));
	if (n < 0) 
	{
		dbgprintf(2, "NTP: ERROR reading from socket\n");
		close(sockfd);
		return 0;
	}
	
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	// These two fields contain the time-stamp seconds as the packet left the NTP server.
	// The number of seconds correspond to the seconds passed since 1900.
	// ntohl() converts the bit/byte order from the network's to host's "endianness".

	packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
	packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
    // Subtract 70 years worth of seconds from the seconds since 1900.
    // This leaves the seconds since the UNIX epoch of 1970.
    // (1900)------------------(1970)**************************************(Time Packet Left the Server)

    time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );

    // Print the time we got from the server, accounting for local timezone and conversion from UTC time.
	memcpy(rawtime, &txTm, sizeof(time_t));
    //printf( "Time: %s", ctime((const time_t* ) &txTm));
	return 1;
}

int mem_parseLine(char* line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int mem_getValue()
{ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = mem_parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

unsigned int get_memory_ram(unsigned int *puitotalPhysMem, unsigned int *puiphysMemUsed, unsigned int *puitotalMyPhysMem)
{
	struct sysinfo memInfo;

	sysinfo (&memInfo);
	long long totalPhysMem = memInfo.totalram;
	totalPhysMem *= memInfo.mem_unit;
	*puitotalPhysMem = totalPhysMem / 1048576;
	long long physMemUsed = memInfo.totalram - memInfo.freeram;
	physMemUsed *= memInfo.mem_unit;
	*puiphysMemUsed = physMemUsed / 1048576;
	*puitotalMyPhysMem = mem_getValue() / 1024;
	return 1;
}

unsigned int get_mem_cpu_mb()
{
   char response[80] = "";
   int arm_mem = 0;
   if (vc_gencmd(response, sizeof response, "get_mem arm") == 0)
      vc_gencmd_number_property(response, "arm", &arm_mem);
   return arm_mem;
}

unsigned int get_mem_gpu_mb()
{
   char response[80] = "";
   int gpu_mem = 0;
   if (vc_gencmd(response, sizeof response, "get_mem gpu") == 0)
      vc_gencmd_number_property(response, "gpu", &gpu_mem);
   return gpu_mem;
}

unsigned int get_used_mem_gpu_mb2()
{
	FILE * fmem;
	char *num = (char*)DBG_MALLOC(65536);
	memset(num, 0, 65536);
	int iLen;
	unsigned int res = 0;
	int iPos = 0;
	int i, n;
	
	fmem = popen("vcdbg reloc", "r");
	iLen = fread(num, 1, 65536, fmem);
	pclose(fmem);
	
	do
	{
		iPos = SearchStrInDataCaseIgn(num, iLen, iPos, "[");
		if (iPos == 0)  break;
		i = SearchStrInDataCaseIgn(num, iLen, iPos, "size");
		if (i != 0)
		{
			i += 3;
			n = SearchStrInData(num, iLen, i, ",");
			if (n != 0)
			{
				n--;
				res += Str2IntLimit(&num[i], n - i);
				iPos = n;
			}
		}			
	} while ((i != 0) && (n != 0));
  
	DBG_FREE(num);
	return res/1048576;
}

int Exec_In_Buff(char *cPath, char *cBuff, unsigned int uiLen)
{
	FILE * fmem;
	memset(cBuff, 0, uiLen);
	int iLen;
	
	fmem = popen(cPath, "r");
	iLen = fread(cBuff, 1, uiLen, fmem);
	pclose(fmem);
	
	return iLen;
}

unsigned int get_used_mem_gpu_mb()
{
	FILE * fmem;
	char *num = (char*)DBG_MALLOC(65536);
	memset(num, 0, 65536);
	int iLen;
	unsigned int res = 0;
	
	fmem = popen("vcdbg reloc | awk -F '[ ,]*' '/^\\[/ {sum += $12} END { print sum }'", "r");
	iLen = fread(num, 1, 65536, fmem);
	pclose(fmem);
	
	res = Str2IntLimit(num, iLen);
  
	DBG_FREE(num);
	return res/1048576;
}

void total_cpu_load_init()
{
    FILE* file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
        &lastTotalSys, &lastTotalIdle);
    fclose(file);
}

double total_cpu_load_value()
{
    double percent;
    FILE* file;
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
        &totalSys, &totalIdle);
    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
        totalSys < lastTotalSys || totalIdle < lastTotalIdle){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else
	{
        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
            (totalSys - lastTotalSys);
        percent = total;
        total += (totalIdle - lastTotalIdle);
        percent /= total;
        percent *= 100;
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}

void my_cpu_load_init()
{
    FILE* file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while(fgets(line, 128, file) != NULL)
	{
        if (strncmp(line, "processor", 9) == 0) numProcessors++;
    }
    fclose(file);
}

double my_cpu_load_value()
{
    struct tms timeSample;
    clock_t now;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        percent = (timeSample.tms_stime - lastSysCPU) +
            (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        percent /= numProcessors;
        percent *= 100;
    }
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return percent;
}

int load_file_in_buff(char *filename, misc_buffer * buffer) 
{
  int32_t pos;
  FILE *fp;
  
  buffer->data_size = 0;
  buffer->void_data = NULL;
  fp = fopen (filename, "rb");
  if (!fp) 
  {
	  dbgprintf(1,"Error open file %s \n",filename);
	  return 0;
  }
  if (fseek (fp, 0L, SEEK_END) < 0) 
  {
    dbgprintf(1,"Error seek file %s \n",filename);
	fclose (fp);
    return 0;
  };
  pos = ftell (fp);
  if (pos == LONG_MAX) 
  {
    dbgprintf(1,"Error tell file %s \n",filename);
	fclose (fp);
    return 0;
  };
  buffer->data_size = pos;
  fseek (fp, 0L, SEEK_SET);
  buffer->void_data = DBG_MALLOC(buffer->data_size);
  fread (buffer->void_data, 1, buffer->data_size, fp);
  fclose (fp);
  return 1;
}

int load_limit_file_in_buff(char *filename, misc_buffer * buffer, unsigned int uiLen) 
{
	int32_t pos;
	FILE *fp;
	  
	buffer->data_size = 0;
	buffer->void_data = NULL;
	fp = fopen (filename, "rb");
	if (!fp) 
	{
		dbgprintf(1,"Error open file %s \n",filename);
		return 0;
	}
	if (fseek (fp, 0L, SEEK_END) < 0) 
	{
		dbgprintf(1,"Error seek file %s \n",filename);
		fclose (fp);
		return 0;
	}
	pos = ftell (fp);
	if (pos == LONG_MAX) 
	{
		dbgprintf(1,"Error tell file %s \n",filename);
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

void init_keyboard_scan() 
{
	keyboard_file = fopen("/dev/input/event0", "r");
	if (!keyboard_file) dbgprintf(2,"Error init_keyboard (%i) %s\n", errno, strerror(errno));
}

int get_keyboard_scan(int *results)
{
	if (!keyboard_file) return 0;
	uint8_t key_map[MAX_MODULE_STATUSES];
	memset(key_map, 0, MAX_MODULE_STATUSES);
	ioctl(fileno(keyboard_file), EVIOCGKEY(MAX_MODULE_STATUSES), key_map);
	
	int n;
	for (n = 0; n < MAX_MODULE_STATUSES; n++) results[n] = key_map[n];
	
	return 0;
}

void close_keyboard_scan() 
{
	if (keyboard_file) fclose(keyboard_file);
}

void init_keyboard() 
{
	tcgetattr(0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
}

int get_keyboard() 
{
	char ch;
	int nread;
	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
	if (nread == 1) 
	{
		return ch;
	}
	return -1;
}

void close_keyboard() 
{
	tcsetattr(0, TCSANOW, &initial_settings);
}

void emergency_stop()
{
	printf("\n\033[1m\033[32mPress 'Esc' to cancel start 'azad' VER:%s :\033[0m\n", VERSION); 
	init_keyboard();
	int i = 40;
	int ret;
	while(i)
	{
		ret = get_keyboard();
		if (ret == 27) 
		{
			printf("\nPressed 'Esc' emegnecy exit.\n");
			close_keyboard();			
			exit(0);
		}
		if (ret == 10) break;
		printf(".");
		i--;
		usleep(50000);
	}	
	printf(" Skipped.\n");	
	close_keyboard();
}

int Action_PlaySound(unsigned int iNum, int iRepeats)
{
	DBG_LOG_IN();
	
	int n, res = 0, iSnd;
	func_link *f_link;
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (iSnd = 0; iSnd < iSoundListCnt; iSnd++)
		if (mSoundList[iSnd].ID == iNum) {res = 1;break;}								
	if (res)
	{	
		res = 0;
		n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
		if (n >= 0)
		{
			f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
			memset(f_link, 0, sizeof(func_link));
			if ((iRepeats >= 0) && (iRepeats < 100)) 
				f_link->ConnectNum = iRepeats; else f_link->ConnectNum = 0;
			f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
			memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));								
			
			if (miModuleList[n].Settings[0] == 0) 
			{
				f_link->pSubModule = NULL;
			}
			else 
			{
				f_link->pSubModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
				memcpy(f_link->pSubModule, (MODULE_INFO*)&miModuleList[miModuleList[n].SubModule], sizeof(MODULE_INFO));																
			}
			f_link->mbuffer.void_data = mSoundList[iSnd].Data;		
			f_link->mbuffer.data_size = mSoundList[iSnd].Len;
			//audio_set_playback_volume(iAlarmVolume);	
			res = PlayAudioSound(f_link);			
		}
	} else dbgprintf(2, "Action_PlaySound: not found SYS Sound ID: %.4s\n", (char*)&iNum);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);

	DBG_LOG_OUT();	
	return res;
}

int Action_PlayTestSound()
{
	DBG_LOG_IN();
	
	int n, res = 0;
	func_link *f_link;

	DBG_MUTEX_LOCK(&modulelist_mutex);
	n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
	if (n >= 0)
	{
		f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
		memset(f_link, 0, sizeof(func_link));
		f_link->ConnectNum = -1;
		f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
		memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));								
		
		if (miModuleList[n].Settings[0] == 0) 
		{
			f_link->pSubModule = NULL;
		}
		else 
		{
			f_link->pSubModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
			memcpy(f_link->pSubModule, (MODULE_INFO*)&miModuleList[miModuleList[n].SubModule], sizeof(MODULE_INFO));																
		}
		f_link->mbuffer.data_size = 65536 * 2 * 3;
		f_link->mbuffer.void_data = DBG_MALLOC(f_link->mbuffer.data_size);
		int16_t *iSoundArr = f_link->mbuffer.void_data;
		res = 65536 * 2;
		int val = 0;
		int ch = 0;
		int len = 65536 * 3;
		char dir = 0;
		for (n = 0; n < len; n++)
		{
			if (((ch == 0) && (n < res)) ||
				((ch == 1) && ((n < 65536) || (n > res))))	iSoundArr[n] = val;
			ch++;
			if (ch == 2) 
			{
				ch = 0;
				if (dir == 0)
				{
					val += 1000;
					if (val > 32000) dir = 1;	
				}
				else
				{
					val -= 1000;
					if (val < -32000) dir = 0;	
				}
			}
		}			
		
		//audio_set_playback_volume(iAlarmVolume);	
		res = PlayAudioSound(f_link);			
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);

	DBG_LOG_OUT();	
	return res;
}

void UpdateModuleStatus(MODULE_INFO *pModule, char cAll)
{
	DBG_LOG_IN();
	MODULE_INFO *pLockModules;
	int ret;
	time_t rawtime;
	struct tm timeinfo;
	
	switch(pModule->Type)
	{
		case MODULE_TYPE_SYSTEM:
			time(&rawtime);
			localtime_r(&rawtime, &timeinfo);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);
			pLockModules = ModuleIdToPoint(pModule->ID, 1);
			if (pLockModules != NULL)
			{						
				memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
			} else dbgprintf(1, "UpdateModuleStatus, not finded local SYSTEM module\n");
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			DBG_MUTEX_LOCK(&system_mutex);			
			if (cAll || (pModule->SaveChanges & 0x00000001)
					 || (pModule->GenEvents & 0x00000001)
					 || (pModule->Global & 0x00000001)) pModule->Status[0] = (int)(get_sys_temp() * 10);
			if (cAll || (pModule->SaveChanges & 0x00000002)
					 || (pModule->GenEvents & 0x00000002)
					 || (pModule->Global & 0x00000002)) pModule->Status[1] = iAccessLevel;
			if (cAll || (pModule->SaveChanges & 0x00000004)
					 || (pModule->GenEvents & 0x00000004)
					 || (pModule->Global & 0x00000004)) pModule->Status[2] = total_cpu_load_value() * 10;
			if (cAll || (pModule->SaveChanges & 0x00000008)
					 || (pModule->GenEvents & 0x00000008)
					 || (pModule->Global & 0x00000008)) pModule->Status[3] = my_cpu_load_value() * 10;
			if (cAll || (pModule->SaveChanges & 0x00000010)
					 || (pModule->GenEvents & 0x00000010)
					 || (pModule->Global & 0x00000010)) get_memory_ram((unsigned int*)&pModule->Status[4], (unsigned int*)&pModule->Status[5], (unsigned int*)&pModule->Status[6]);
			//if (pModule->GenEvents & 0x00000020)
			//if (pModule->GenEvents & 0x00000040)
				//if (pModule->GenEvents & 0x00000080) pModule->Status[7] = get_mem_gpu_mb();
			if (cAll || (pModule->SaveChanges & 0x00000100)
					 || (pModule->GenEvents & 0x00000100)
					 || (pModule->Global & 0x00000100)) pModule->Status[8] = get_used_mem_gpu_mb();
			//if (pModule->GenEvents & 0x00000200) pModule->Status[9] = get_mem_cpu_mb();
			if (cAll || (pModule->SaveChanges & 0x00000400)
					 || (pModule->GenEvents & 0x00000400)
					 || (pModule->Global & 0x00000400)) pModule->Status[10] = (int)(get_sys_volt(0) * 1000);
			if (cAll || (pModule->SaveChanges & 0x00000800)
					 || (pModule->GenEvents & 0x00000800)
					 || (pModule->Global & 0x00000800)) pModule->Status[11] = (int)(get_sys_volt(1) * 1000);
			if (cAll || (pModule->SaveChanges & 0x00001000)
					 || (pModule->GenEvents & 0x00001000)
					 || (pModule->Global & 0x00001000)) pModule->Status[12] = (int)(get_sys_volt(2) * 1000);
			if (cAll || (pModule->SaveChanges & 0x00002000)
					 || (pModule->GenEvents & 0x00002000)
					 || (pModule->Global & 0x00002000)) pModule->Status[13] = (int)(get_sys_volt(3) * 1000);
			//if (pModule->GenEvents & 0x00004000)
			if (cAll || (pModule->SaveChanges & 0x00008000)
					 || (pModule->GenEvents & 0x00008000)
					 || (pModule->Global & 0x00008000)) pModule->Status[15] = timeinfo.tm_hour;
			if (cAll || (pModule->SaveChanges & 0x00010000)
					 || (pModule->GenEvents & 0x00010000)
					 || (pModule->Global & 0x00010000)) pModule->Status[16] = timeinfo.tm_min;
			if (cAll || (pModule->SaveChanges & 0x00020000)
					 || (pModule->GenEvents & 0x00020000)
					 || (pModule->Global & 0x00020000)) pModule->Status[17] = timeinfo.tm_sec;
			if (cAll || (pModule->SaveChanges & 0x00040000)
					 || (pModule->GenEvents & 0x00040000)
					 || (pModule->Global & 0x00040000))	pModule->Status[18] = get_sys_core() & 1;
			//pModule->Status[10] = EVENTS
			DBG_MUTEX_UNLOCK(&system_mutex);			
			break;
		case MODULE_TYPE_GPIO:
			pModule->Status[0] = gpio_get_status_module(pModule);
			break;
		case MODULE_TYPE_EXTERNAL:
			external_get_statuses_module(pModule);
			break;
		case MODULE_TYPE_TFP625A:
			//TFP625A_get_statuses_module(pModule);
			break;
		case MODULE_TYPE_USB_GPIO:
			usb_gpio_get_status_module(pModule, 1);
			break;
		case MODULE_TYPE_TEMP_SENSOR:
			pModule->Status[0] = 0;
			pModule->Status[1] = 0;
			if (pModule->Settings[0] == I2C_ADDRESS_AM2320)
			{
				ret = AM2320_read(pModule->InitParams[0], (int*)&pModule->Status[0], (int*)&pModule->Status[1]);
				if (ret == 0) 
				{
					dbgprintf(1,"temp sensor AM2320 NOT readed\n");
					pModule->Status[0] = 0;
					pModule->Status[1] = 0;
				}
			}
			if (pModule->Settings[0] == I2C_ADDRESS_LM75)
			{
				ret = LM75_read(pModule->InitParams[0], pModule->Settings[0] | pModule->Settings[1], (int*)&pModule->Status[0]);
				if (ret == 0) 
				{
					dbgprintf(1,"temp sensor LM75 NOT readed\n");
					pModule->Status[0] = 0;
					pModule->Status[1] = 0;
				}
			}
			break;
		case MODULE_TYPE_AS5600:
			pModule->Status[0] = 0;
			pModule->Status[1] = 0;
			ret = AS5600_read(pModule->InitParams[0], I2C_ADDRESS_AS5600 | pModule->Settings[0], (int*)&pModule->Status[0], (int*)&pModule->Status[1]);
			if (ret == 0) 
			{
				dbgprintf(1,"Angle sensor AS5600 NOT readed\n");
				pModule->Status[0] = 0;
				pModule->Status[1] = 0;
			}
			break;
		case MODULE_TYPE_HMC5883L:
			memset(pModule->Status, 0, sizeof(int)*8);
			if ((!pModule->Settings[8]) && (i2c_echo(pModule->InitParams[0], I2C_ADDRESS_HMC5883L))) 
				pModule->Settings[8] = HMC5883L_init(pModule->InitParams[0]);
			
			if (pModule->Settings[8])
			{
				ret = HMC5883L_read(pModule->InitParams[0],  
																(int*)&pModule->Status[0], 
																(int*)&pModule->Status[1], 
																(int*)&pModule->Status[2]);
				if (ret == 0)
				{
					dbgprintf(1,"Position sensor HMC5883L NOT readed\n");
					memset(pModule->Status, 0, sizeof(int)*8);			
					pModule->Settings[8] = 0;
				}			
				else
				{
					int a;
					a = atan2( pModule->Status[1], pModule->Status[0] ) * 180.0 / M_PI;
					pModule->Status[3] = a < 0 ? 360 + a : a;	 
					a = atan2( pModule->Status[2], pModule->Status[0] ) * 180.0 / M_PI;
					pModule->Status[4] = a < 0 ? 360 + a : a;
					a = atan2( pModule->Status[2], pModule->Status[1] ) * 180.0 / M_PI;
					pModule->Status[5] = a < 0 ? 360 + a : a;	
					
					printf("%i\t%i\t%i\t\t\t%i\t%i\t%i\n", pModule->Status[0], pModule->Status[1], pModule->Status[2], pModule->Status[3],
														pModule->Status[4], pModule->Status[5]);
				}
			}
			break;
		case MODULE_TYPE_ADS1015:
			pModule->Status[0] = 0;
			if ((!pModule->Settings[8]) && (i2c_echo(pModule->InitParams[0], I2C_ADDRESS_ADS1015 | pModule->Settings[0]))) 
			{
				pModule->Settings[8] = ADS1015_init(pModule->InitParams[0], I2C_ADDRESS_ADS1015 | pModule->Settings[0],
													pModule->Settings[2]);
			}
			if (pModule->Settings[8])
			{
				ret = ADS1015_read(pModule->InitParams[0], I2C_ADDRESS_ADS1015 | pModule->Settings[0], (int*)&pModule->Status[0]);
				if (ret == 0) 
				{
					dbgprintf(1,"ADC sensor ADS1015 NOT readed\n");
					pModule->Status[0] = 0;
					pModule->Settings[8] = 0;
				}
			}
			break;
		case MODULE_TYPE_MCP3421:
			pModule->Status[0] = 0;
			if ((!pModule->Settings[8]) && (i2c_echo(pModule->InitParams[0], I2C_ADDRESS_MCP3421 | pModule->Settings[0]))) 
			{
				pModule->Settings[8] = MCP3421_init(pModule->InitParams[0], I2C_ADDRESS_MCP3421 | pModule->Settings[0],
													pModule->Settings[1], pModule->Settings[2]);
			}
			if (pModule->Settings[8])
			{
				ret = MCP3421_read(pModule->InitParams[0], I2C_ADDRESS_MCP3421 | pModule->Settings[0], pModule->Settings[1], (int*)&pModule->Status[0]);
				if (ret == 0) 
				{
					dbgprintf(1,"ADC sensor MCP3421 NOT readed\n");
					pModule->Status[0] = 0;
					pModule->Settings[8] = 0;
				}
			}
			break;
		case MODULE_TYPE_RS485:
			ret = get_char();
			if (ret >= 0) pModule->Status[0] = ret;
				else pModule->Status[0] = -2;			 
			break;
		case MODULE_TYPE_COUNTER:
			DBG_MUTEX_LOCK(&modulelist_mutex);
			pLockModules = ModuleIdToPoint(pModule->ID, 1);
			if (pLockModules != NULL)
			{
				memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
			} else dbgprintf(1, "UpdateModuleStatus, not finded local COUNTER module\n");			
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			break;
		case MODULE_TYPE_MEMORY:
			DBG_MUTEX_LOCK(&modulelist_mutex);
			pLockModules = ModuleIdToPoint(pModule->ID, 1);
			if (pLockModules != NULL)
			{						
				memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
			} else dbgprintf(1, "UpdateModuleStatus, not finded local MEMORY module\n");
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			break;
		case MODULE_TYPE_DISPLAY:
			DBG_MUTEX_LOCK(&modulelist_mutex);
			pLockModules = ModuleIdToPoint(pModule->ID, 1);
			if (pLockModules != NULL)
			{						
				memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
			} else dbgprintf(1, "UpdateModuleStatus, not finded local DISPLAY module\n");
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			break;
		case MODULE_TYPE_CAMERA:
			{
				char incstat = (TestIncomeConnects() & 2) ? 1 : 0;
				DBG_MUTEX_LOCK(&modulelist_mutex);
				pLockModules = ModuleIdToPoint(pModule->ID, 1);
				if (pLockModules != NULL)
				{			
					pLockModules->Status[21] = incstat;
					memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
				} else dbgprintf(1, "UpdateModuleStatus, not finded local CAMERA module\n");
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
			}
			break;
		case MODULE_TYPE_MIC:
			{
				char incstat = TestIncomeConnects() & 1;
				DBG_MUTEX_LOCK(&modulelist_mutex);
				pLockModules = ModuleIdToPoint(pModule->ID, 1);
				if (pLockModules != NULL)
				{	
					pLockModules->Status[1] = incstat;			
					memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
				} else dbgprintf(1, "UpdateModuleStatus, not finded local MIC module\n");
				DBG_MUTEX_UNLOCK(&modulelist_mutex);			
			}
			break;
		case MODULE_TYPE_SPEAKER:
			{
				char incstat = TestOutcomeConnects() & 1;
				DBG_MUTEX_LOCK(&modulelist_mutex);
				pLockModules = ModuleIdToPoint(pModule->ID, 1);
				if (pLockModules != NULL)
				{	
					pLockModules->Status[1] = incstat;			
					memcpy(pModule->Status, pLockModules->Status, MAX_MODULE_STATUSES*sizeof(pModule->Status[0]));
				} else dbgprintf(1, "UpdateModuleStatus, not finded local SPEAKER module\n");
				DBG_MUTEX_UNLOCK(&modulelist_mutex);			
			}
			break;			
		case MODULE_TYPE_KEYBOARD:
			get_keyboard_scan(pModule->Status);	 
			break;		
		default:
			break;
	}
	DBG_LOG_OUT();	
	return;
}

void UpdateDeviceType()
{
	DBG_LOG_IN();
	int n;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	for (n = 0; n != iModuleCnt; n++)
	{
		if (miModuleList[n].Enabled & 1)
		{
			switch (miModuleList[n].Type)
			{			
				case MODULE_TYPE_USB_GPIO:
					uiDeviceType |= DEVICE_TYPE_USB_GPIO;
					break;
				case MODULE_TYPE_GPIO:
					uiDeviceType |= DEVICE_TYPE_GPIO;
					break;
				case MODULE_TYPE_TEMP_SENSOR:
					uiDeviceType |= DEVICE_TYPE_TEMP | DEVICE_TYPE_I2C;
					break;
				case MODULE_TYPE_ADS1015:
				case MODULE_TYPE_MCP3421:
				case MODULE_TYPE_AS5600:
				case MODULE_TYPE_HMC5883L:
					uiDeviceType |= DEVICE_TYPE_I2C;
					break;
				case MODULE_TYPE_TFP625A:
					uiDeviceType |= DEVICE_TYPE_UART;
					break;
				case MODULE_TYPE_PN532:
					uiDeviceType |= DEVICE_TYPE_PN532 | DEVICE_TYPE_UART;
					break;
				case MODULE_TYPE_RTC:
					uiDeviceType |= DEVICE_TYPE_CLOCK | DEVICE_TYPE_I2C;
					break;
				case MODULE_TYPE_RADIO:
					uiDeviceType |= DEVICE_TYPE_RADIO | DEVICE_TYPE_I2C;
					break;
				case MODULE_TYPE_SPEAKER:
					uiDeviceType |= DEVICE_TYPE_AUDIO_OUT | DEVICE_TYPE_GPIO;
					break;
				case MODULE_TYPE_MIC:
					uiDeviceType |= DEVICE_TYPE_AUDIO_IN;
					break;
				case MODULE_TYPE_DISPLAY:
					uiDeviceType |= DEVICE_TYPE_VIDEO_OUT;
					break;
				case MODULE_TYPE_CAMERA:
					uiDeviceType |= DEVICE_TYPE_VIDEO_IN;
					break;
				case MODULE_TYPE_RS485:
					uiDeviceType |= DEVICE_TYPE_RS485 | DEVICE_TYPE_UART;
					break;
				case MODULE_TYPE_RC522:
					uiDeviceType |= DEVICE_TYPE_RC522 | DEVICE_TYPE_UART;
					break;
				case MODULE_TYPE_IR_RECEIVER:
					uiDeviceType |= DEVICE_TYPE_IR_RECEIVER;
					break;
				case MODULE_TYPE_EXTERNAL:
					if (miModuleList[n].Settings[0] == 1) uiDeviceType |= DEVICE_TYPE_I2C;
					if (miModuleList[n].Settings[0] == 2) uiDeviceType |= DEVICE_TYPE_UART;
					break;
				default:
					break;
			}
		}
   }
   DBG_MUTEX_UNLOCK(&modulelist_mutex);	
   DBG_LOG_OUT();
}

int CloseMenu(int iMenuNum)
{
	DBG_LOG_IN();
	SetShowMenu(0);
	DBG_LOG_OUT();
	return 0;
}

int SearchDataCombine(uint16_t *FindData, int FindDataLen, uint16_t *InData, int InDataLen, int Pos)
{
	int Flag1 = 0;
	int n;
	unsigned char ucSett;
	if (FindDataLen > InDataLen) return -1;
	for (n = Pos; n < InDataLen; n++)
	{
		ucSett = (InData[n] >> 8) & 255;
		if (((ucSett == 0) && ((FindData[Flag1] & 255) == (InData[n] & 255))) || 
			((ucSett == 1) && ((FindData[Flag1] & 255) & (InData[n] & 255))) ||
			(ucSett == 2))
		{
			Flag1++; 
		}
		else 
		{
			Flag1 = 0;
			if (FindData[Flag1] == InData[n]) Flag1++;
		}
		if (Flag1 == FindDataLen)
		{
			return (n + 1) - FindDataLen;
		}
	}
	return -1;
}

int Menu_MusicVolumeInc(void *pData, int iPos)
{
	DBG_LOG_IN();
	iCurrentVolume += iPos;
	if (iCurrentVolume > 100) iCurrentVolume = 100;
		else audio_set_playback_volume(iCurrentVolume);
	DBG_MUTEX_LOCK(&system_mutex);	
	iBasicVolume = iCurrentVolume;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_MusicVolumeUp(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	iCurrentVolume += 5;
	if (iCurrentVolume > 100) iCurrentVolume = 100;
		else audio_set_playback_volume(iCurrentVolume);
	DBG_MUTEX_LOCK(&system_mutex);	
	iBasicVolume = iCurrentVolume;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_MusicVolumeDec(void *pData, int iPos)
{
	DBG_LOG_IN();
	iCurrentVolume -= iPos;
	if (iCurrentVolume < 0) iCurrentVolume = 0;		
	audio_set_playback_volume(iCurrentVolume);
	DBG_MUTEX_LOCK(&system_mutex);	
	iBasicVolume = iCurrentVolume;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_MusicVolumeDown(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	iCurrentVolume -= 5;
	if (iCurrentVolume < 0) iCurrentVolume = 0;		
	audio_set_playback_volume(iCurrentVolume);
	DBG_MUTEX_LOCK(&system_mutex);	
	iBasicVolume = iCurrentVolume;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int MusicVolumeSet(int iNum)
{
	DBG_LOG_IN();
	if ((iNum >= 0) && (iNum <= 100))
	{
		iCurrentVolume = iNum;
		audio_set_playback_volume(iCurrentVolume);
		DBG_MUTEX_LOCK(&system_mutex);	
		iBasicVolume = iCurrentVolume;
		DBG_MUTEX_UNLOCK(&system_mutex);	
		uiTimerShowVolume = 0;
	} else dbgprintf(2, "Wrong level volume: %i (Need: 0 - 100)\n", iNum);
	DBG_LOG_OUT();
	return 0;
}

int Menu_MusicVolumeMute(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	iCurrentVolume = 0;
	audio_set_playback_volume(iCurrentVolume);
	DBG_MUTEX_LOCK(&system_mutex);	
	iBasicVolume = iCurrentVolume;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_MusicVolumeMiddle(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	iCurrentVolume = 50;
	audio_set_playback_volume(iCurrentVolume);
	DBG_MUTEX_LOCK(&system_mutex);	
	iBasicVolume = iCurrentVolume;	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_AlarmVolumeSet(void *pData, int iPos)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);	
	iAlarmVolume = iPos;
	if (iAlarmVolume < 0) iAlarmVolume = 100;
	if (iAlarmVolume > 100) iAlarmVolume = 100;
	DBG_MUTEX_UNLOCK(&system_mutex);		
		//else audio_set_playback_volume(iAlarmVolume);
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_AlarmVolumeUp(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);	
	iAlarmVolume += 5;
	if (iAlarmVolume > 100) iAlarmVolume = 100;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	//else audio_set_playback_volume(iAlarmVolume);	
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_AlarmVolumeInc(void *pData, int iPos)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);		
	iAlarmVolume += iPos;
	if (iAlarmVolume > 100) iAlarmVolume = 100;
	DBG_MUTEX_UNLOCK(&system_mutex);		
		//else audio_set_playback_volume(iAlarmVolume);
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_AlarmVolumeDown(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);		
	iAlarmVolume -= 5;
	if (iAlarmVolume < 0) iAlarmVolume = 0;	
	DBG_MUTEX_UNLOCK(&system_mutex);		
	//audio_set_playback_volume(iMusicVolume);
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_AlarmVolumeDec(void *pData, int iPos)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);	
	iAlarmVolume -= iPos;
	if (iAlarmVolume < 0) iAlarmVolume = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);		
	//audio_set_playback_volume(iMusicVolume);
	uiTimerShowVolume = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_RadioVolumeUp(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	int ret2 = -1;		
	int iRadioVol;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	iRadioVolume++;
	if (iRadioVolume > 15) iRadioVolume = 15;
		else ret2 = ModuleTypeToNum(MODULE_TYPE_RADIO, 1);
	iRadioVol = iRadioVolume;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret2 != -1) RDA5807M_setVolume(iRadioVol);
	DBG_LOG_OUT();
	return 0;
}

int Menu_RadioVolumeDown(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	int ret2 = -1;
	int iRadioVol;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	if (iRadioVolume > 0) 
	{
		iRadioVolume--;
		ret2 = ModuleTypeToNum(MODULE_TYPE_RADIO, 1);
		iRadioVol = iRadioVolume;
	}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (ret2 != -1) RDA5807M_setVolume(iRadioVol);
	DBG_LOG_OUT();
	return 0;
}

int Menu_GetCurrentRadioStation(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	int iRadioVol = iRadioVolume;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if ((uiDeviceType & DEVICE_TYPE_RADIO) == 0) return 0;
	iRefreshClk++;
	if (iRefreshClk > 6)
	{
		iRefreshClk = 0;
		memset(pMenu[iMenuNum].Name, 0, 64);
		sprintf(pMenu[iMenuNum].Name, "<<Радио (%.1f) Громкость:%i>>", (float)RDA5807M_getFrequency(1) / 100, iRadioVol);		
	}
	DBG_LOG_OUT();
	return 0;
}

int Menu_GetMusicVolume(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	if ((uiDeviceType & DEVICE_TYPE_AUDIO_OUT) == 0) return 0;
	iRefreshClk++;
	if (iRefreshClk > 6)
	{
		iRefreshClk = 0;
		memset(pMenu[iMenuNum].Name, 0, 64);
		DBG_MUTEX_LOCK(&system_mutex);		
		sprintf(pMenu[iMenuNum].Name, "<<Основная громкость:%i>>", iBasicVolume);	
		DBG_MUTEX_UNLOCK(&system_mutex);		
	}
	DBG_LOG_OUT();
	return 0;
}

int Menu_GetAlarmVolume(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	if ((uiDeviceType & DEVICE_TYPE_AUDIO_OUT) == 0) return 0;
	iRefreshClk++;
	if (iRefreshClk > 6)
	{
		iRefreshClk = 0;
		memset(pMenu[iMenuNum].Name, 0, 64);
		DBG_MUTEX_LOCK(&system_mutex);		
		sprintf(pMenu[iMenuNum].Name, "<<Громкость будильника:%i>>", iAlarmVolume);	
		DBG_MUTEX_UNLOCK(&system_mutex);
	}
	DBG_LOG_OUT();
	return 0;
}

int UpdateSecurityKey(int iAction, MODULE_INFO *pModule, SECURITY_KEY_INFO *skiUpdateInfo, uint8_t uiSensnum)
{
	SECURITY_KEY_INFO sKey;
	int i, k;
	int res = 0;
	int finded = 0;
	DBG_MUTEX_LOCK(&securitylist_mutex);
	for (i = 0; i < iSecurityKeyCnt; i++)
	{		
		k = 0;
		if ((skiUpdateInfo->SerialLength == skiSecurityKeys[i].SerialLength) 
			&& (skiSecurityKeys[i].ParentID == skiUpdateInfo->ParentID)
			&& (skiSecurityKeys[i].ID == skiUpdateInfo->ID)
			&& (skiSecurityKeys[i].Type == skiUpdateInfo->Type)
			&& (skiSecurityKeys[i].Sector == skiUpdateInfo->Sector))	
				for (k = 0; k < skiUpdateInfo->SerialLength; k++) 
					if (skiUpdateInfo->Serial[k] != skiSecurityKeys[i].Serial[k]) break;
		
		if (k && (k == skiUpdateInfo->SerialLength))
		{
			memcpy(&sKey, &skiSecurityKeys[i], sizeof(SECURITY_KEY_INFO));
			finded = 1;				
			break;
		}
	}
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	
	if (finded)
	{
		res = 2;
		uint8_t keydata[16] = {255,255,255,255,255,255,255,7,128,105,255,255,255,255,255,255};
		if (skiUpdateInfo->VerifyKeyB)
		{
			/*Switch change auth protect from KEY A to KEY B*/
			keydata[6] = 127;
			keydata[7] = 7;
			keydata[8] = 136;
		}
			
		memcpy(&keydata[0], skiUpdateInfo->KeyA, 6);
		memcpy(&keydata[10], skiUpdateInfo->KeyB, 6);
		int iFirstBlock = skiUpdateInfo->Sector * 4;
				
		if (pModule->Type == MODULE_TYPE_RC522)
		{
			if (MFRC522_selectTag(pModule->InitParams[0], sKey.Serial))
			{
				if (MFRC522_authenticate(pModule->InitParams[0], (iAction == 2) ? MF1_AUTHENT1B : MF1_AUTHENT1A, iFirstBlock, (iAction == 2) ? (uint8_t*)sKey.KeyB : (uint8_t*)sKey.KeyA, sKey.Serial) == MI_OK)
				{
					if (MFRC522_writeToTag(pModule->InitParams[0], iFirstBlock + 3, keydata) == MI_OK)
							res = 1; else res = -1;
				}
				MFRC522_haltTag(pModule->InitParams[0]);
			}
		}
		if (pModule->Type == MODULE_TYPE_PN532)
		{
			if (PN532_mifareclassic_AuthenticateBlock(pModule->InitParams[0], sKey.Serial, 4, iFirstBlock, (iAction == 2) ? 1 : 0, (iAction == 2) ? (uint8_t*)sKey.KeyB : (uint8_t*)sKey.KeyA))
			{
				if (PN532_mifareclassic_WriteDataBlock(pModule->InitParams[0], iFirstBlock + 3, keydata))
					res = 1; else res = -1;
			}
		}
		if (pModule->Type == MODULE_TYPE_USB_GPIO)
		{
			//printf("Get auth block %i %i\n", iFirstBlock, iAction);			
			res = usb_gpio_write_data_block(pModule, uiSensnum, iFirstBlock + 3, iAction, (void*)&sKey, sizeof(SECURITY_KEY_INFO), keydata, 16);
			//printf("res auth block %i\n", res);			
		}		
		
		if (sKey.Local == 0) 
		{
			struct sockaddr_in parAddress;
			int iLocal = -1;
				
			DBG_MUTEX_LOCK(&modulelist_mutex);
			int n = ModuleIdToNum(sKey.ParentID, 2);
			if (n >= 0) iLocal = miModuleList[n].Local;	
			memcpy(&parAddress, &miModuleList[n].Address, sizeof(struct sockaddr_in));				
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			if (iLocal == 0)
				SendTCPMessage(TYPE_MESSAGE_SET_CHANGE_SMARTCARD, (char*)&sKey.ID, sizeof(unsigned int), (char*)&res, sizeof(int), &parAddress);
		}
		else
		{
			DBG_MUTEX_LOCK(&securitylist_mutex);
			if (iUpdateKeyInfoResult == 0)
			{
				iUpdateKeyInfoResult = res;
			}
			DBG_MUTEX_UNLOCK(&securitylist_mutex);
		}
	}
	return res;
}

int ClearSecurityKey(unsigned int uiID)
{	
	int cnt;
	int skicnt = 0;
	int ret = 0;
	SECURITY_KEY_INFO* SKI = NULL;
	DBG_MUTEX_LOCK(&securitylist_mutex);
	for (cnt = 0; cnt < iSecurityKeyCnt; cnt++) if (skiSecurityKeys[cnt].ParentID == uiID) {ret = 1; break;}
	if (ret == 1)
	{
		for (cnt = 0; cnt < iSecurityKeyCnt; cnt++)
		{
			if (skiSecurityKeys[cnt].ParentID != uiID)
			{
				skicnt++;
				SKI = (SECURITY_KEY_INFO*)DBG_REALLOC(SKI, skicnt*sizeof(SECURITY_KEY_INFO));
				memcpy(&SKI[skicnt - 1], &skiSecurityKeys[cnt], sizeof(SECURITY_KEY_INFO));
			}
		}
		DBG_FREE(skiSecurityKeys);
		skiSecurityKeys = SKI;
		iSecurityKeyCnt = skicnt;
	}
	DBG_MUTEX_UNLOCK(&securitylist_mutex);	
	return ret;
}

int ClearModuleList(unsigned int uiID)
{
	int cnt;
	int nmicnt = 0;
	int ret = 0;
	MODULE_INFO* NMI = NULL;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (cnt = 0; cnt < iModuleCnt; cnt++) if (miModuleList[cnt].ParentID == uiID) {ret = 1; break;}
	if (ret == 1)
	{
		for (cnt = 0; cnt < iModuleCnt; cnt++)
		{
			if (miModuleList[cnt].ParentID != uiID)
			{
				nmicnt++;
				NMI = (MODULE_INFO*)DBG_REALLOC(NMI, nmicnt*sizeof(MODULE_INFO));
				memcpy(&NMI[nmicnt - 1], &miModuleList[cnt], sizeof(MODULE_INFO));
			}
		}
		DBG_FREE(miModuleList);
		miModuleList = NMI;
		iModuleCnt = nmicnt;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	return ret;
}	

int ClearSystemList(unsigned int uiID)
{
	int cnt;
	int nsicnt = 0;
	int ret = 0;
	SYSTEM_INFO* NSI = NULL;
	DBG_MUTEX_LOCK(&systemlist_mutex);
	for (cnt = 0; cnt < iSystemListCnt; cnt++) if (miSystemList[cnt].ID == uiID) {ret = 1; break;}
	if (ret == 1)
	{
		for (cnt = 0; cnt < iSystemListCnt; cnt++)
		{
			if (miSystemList[cnt].ID != uiID)
			{
				nsicnt++;
				NSI = (SYSTEM_INFO*)DBG_REALLOC(NSI, nsicnt*sizeof(SYSTEM_INFO));
				memcpy(&NSI[nsicnt - 1], &miSystemList[cnt], sizeof(SYSTEM_INFO));
			}
		}
		DBG_FREE(miSystemList);
		miSystemList = NSI;
		iSystemListCnt = nsicnt;
	}
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	return ret;
}	

unsigned int GetLocalSysID()
{
	int n;
	unsigned int ret = 0;
	for (n = 0; n < iSystemListCnt; n++)
	{		
		if (miSystemList[n].Local == 1) 
		{
			ret = miSystemList[n].ID;
			break;
		}
	}
	return ret;
}

SYSTEM_INFO* GetLocalSysInfo()
{
	int n;
	SYSTEM_INFO* ret = NULL;
	
	for (n = 0; n < iSystemListCnt; n++)
	{
		if (miSystemList[n].Local == 1) 
		{
			ret = &miSystemList[n];
			break;
		}
	}
	return ret;
}

SYSTEM_INFO* GetSysInfo(unsigned int uiID)
{
	int n;
	SYSTEM_INFO* ret = NULL;
	for (n = 0; n < iSystemListCnt; n++)
	{
		if (miSystemList[n].ID == uiID) 
		{
			ret = &miSystemList[n];
			break;
		}
	}
	return ret;
}

struct sockaddr_in* ModuleTypeToAddress(int iType, char cLocal)
{
	int n;
	struct sockaddr_in* ret = NULL;
	for (n = 0; n != iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == iType) && ((miModuleList[n].Local == cLocal) || (cLocal == 2)))
		{
			ret = &miModuleList[n].Address;
			break;
		}
	}
	return ret;
}

MODULE_INFO* ModuleTypeToPoint(int iType, char cLocal)
{
	int n;
	MODULE_INFO* ret = NULL;
	for (n = 0; n != iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == iType) && ((miModuleList[n].Local == cLocal) || (cLocal == 2)))
		{
			ret = &miModuleList[n];
			break;
		}
	}
	return ret;
}

MODULE_INFO* ModuleIdToPoint(int iID, char cLocal)
{
	int n;
	MODULE_INFO* ret = NULL;
	for (n = 0; n < iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].ID == iID) && ((miModuleList[n].Local == cLocal) || (cLocal == 2)))
		{
			ret = &miModuleList[n];
			break;
		}
	}
	return ret;
}

struct sockaddr_in* ModuleIdToAddress(int iID, char cLocal)
{
	int n;
	struct sockaddr_in* ret = NULL;
	for (n = 0; n != iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].ID == iID) && ((miModuleList[n].Local == cLocal) || (cLocal == 2)))
		{
			ret = &miModuleList[n].Address;
			break;
		}
	}
	return ret;
}

int ModuleTypeToNum(int iType, char cLocal)
{
	int n;
	int ret = -1;
	for (n = 0; n < iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == iType) && ((miModuleList[n].Local == cLocal) || (cLocal == 2)))
		{
			ret = n;
			break;
		}
	}
	return ret;
}

int ModuleIdToNum(unsigned int iID, char cLocal)
{
	int n;
	int ret = -1;
	for (n = 0; n < iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].ID == iID) && ((miModuleList[n].Local == cLocal) || (cLocal == 2)))
		{
			ret = n;
			break;
		}
	}
	return ret;
}

int ModuleSortToNum(unsigned int iSort)
{
	int ret = miModuleList[iSort].Sort;
	return ret;
}

int StreamOn(int iNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);	
	if (iInternetRadioCnt <= iNum) iNum = iRadioCode;
	if (iInternetRadioCnt > iNum)
	{
		iRadioTimeMax = iStreamTimeMaxSet;
		memcpy(cCurrentFile, irInternetRadio[iNum].URL, 256);
		iRadioCode = iNum;
		DBG_MUTEX_UNLOCK(&system_mutex);		
		SetNewShowType(SHOW_TYPE_URL);
		SetChangeShowNow(1);
	} 
	else 
	{
		dbgprintf(2, "Wrong num Stream: %i(Max:%i)\n", iNum, iInternetRadioCnt);
		DBG_MUTEX_UNLOCK(&system_mutex);		
	}
	
	DBG_LOG_OUT();
	return 0;
}

int StreamOnLast()
{
	DBG_LOG_IN();
	int iNum;
	DBG_MUTEX_LOCK(&system_mutex);
	if ((iRadioCode < 0) && (iRadioCode >= iInternetRadioCnt)) iRadioCode = 0;
	iNum = iRadioCode;
	DBG_MUTEX_UNLOCK(&system_mutex);
	StreamOn(iNum);
	DBG_LOG_OUT();
	return 0;
}

int StreamOnNext()
{
	DBG_LOG_IN();
	int iNum;
	DBG_MUTEX_LOCK(&system_mutex);	
	iRadioCode++;	
	if (iRadioCode >= iInternetRadioCnt) iRadioCode = 0;
	iNum = iRadioCode;
	DBG_MUTEX_UNLOCK(&system_mutex);
	StreamOn(iNum);
	DBG_LOG_OUT();
	return 0;
}

int StreamOnPrev()
{
	DBG_LOG_IN();
	int iNum;
	DBG_MUTEX_LOCK(&system_mutex);	
	if (iRadioCode <= 0) iRadioCode = iInternetRadioCnt;
	iRadioCode--;	
	iNum = iRadioCode;
	DBG_MUTEX_UNLOCK(&system_mutex);
	StreamOn(iNum);
	DBG_LOG_OUT();
	return 0;
}

int StreamOnRandom()
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);	
	int ret = iInternetRadioCnt;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	StreamOn(GenRandomInt(ret - 1));
	DBG_LOG_OUT();
	return 0;
}

int StreamTypeOn(int iNum)
{
	DBG_LOG_IN();
	int i, n = 9000;
	DBG_MUTEX_LOCK(&system_mutex);	
	
	for (i = 0; i < iInternetRadioCnt; i++) 
	{
		if (irInternetRadio[i].Type == iNum) 
		{
			n = i;
			break;
		}
	}
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (n != 9000) StreamOn(n);
	DBG_LOG_OUT();
	return 0;
}

int StreamTypeOnRandom(int iNum)
{
	DBG_LOG_IN();
	int iCnt = 0;
	int i, n, m;
	m = 9000;
	DBG_MUTEX_LOCK(&system_mutex);	
	
	for(i = 0; i < iInternetRadioCnt; i++) if (irInternetRadio[i].Type == iNum) iCnt++;
	if (iCnt) 
	{
		iCnt = GenRandomInt(iCnt - 1);
		n = 0;
		for (i = 0; i < iInternetRadioCnt; i++) 
		{
			if (irInternetRadio[i].Type == iNum) 
			{
				if (iCnt == n) 
				{
					m = i;
					break;
				} else n++;
			}
		}
	}
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (m != 9000) StreamOn(m);	
	DBG_LOG_OUT();
	return 0;
}

int Menu_InternetRadioOn(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	StreamOn(pMenu[iMenuNum].Options[pMenu[iMenuNum].SelectedOption].MiscData);
	DBG_LOG_OUT();
	return 0;
}

int Menu_PlaySubDirectories(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int ret;
	
	char buff[256];
	memset(buff, 0, 256);
	
	DBG_MUTEX_LOCK(&system_mutex);	
	if (pData) 
	{
		memcpy(buff, pMenu[16].Path, strlen(pMenu[16].Path));
		dbgprintf(4, "Set show path(w): '%s'\n", pMenu[16].Path);
		ret = UpdateListFiles(buff, UPDATE_LIST_SCANTYPE_SUB);
	
	} 
	else ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_SUB);
	
	if (ret == 0) 
	{		
		if (uiShowModeCur >= 2) uiShowModeCur = 1;
		dbgprintf(3, "No files in Menu_PlaySubDirectories '%s'\n", buff);		
	}
	else
	{
		if (uiShowModeCur < 2) uiShowModeCur = 2;		
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SetChangeShowNow(1); else ShowNewMessage("Нет файлов для воспроизведения");
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_PlayDirectory(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&system_mutex);	
	
	int ret = UpdateListFiles(pMenu[16].Path, UPDATE_LIST_SCANTYPE_CUR);
	if (ret == 0) 
	{		
		if (uiShowModeCur >= 2) uiShowModeCur = 1;
		dbgprintf(3, "No files in Menu_PlayDirectory '%s'\n", pMenu[16].Path);		
		UpdateListFiles(NULL, UPDATE_LIST_SCANTYPE_FULL);
	}
	else 
	{
		if (uiShowModeCur < 2) uiShowModeCur = 2;
		//printf("UpdateListFiles %i\n", ret);
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) SetChangeShowNow(1); else ShowNewMessage("Нет файлов для воспроизведения");
	DBG_LOG_OUT();
	return 0;
}

int Menu_OnRadioStation(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int ret1, ret2;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	ret1 = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
	ret2 = ModuleTypeToNum(MODULE_TYPE_RADIO, 1);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if ((ret1 != -1) && (ret2 != -1))
	{
		DBG_MUTEX_LOCK(&modulelist_mutex);	
		if (iRadioVolume > 15) iRadioVolume = 15;
		int iRadioVol = iRadioVolume;
		MODULE_INFO* SpkMod = ModuleTypeToPoint(MODULE_TYPE_SPEAKER, 1);
		if ((SpkMod) && (SpkMod->SubModule >= 0)) gpio_switch_on_module(&miModuleList[SpkMod->SubModule]);
		DBG_MUTEX_UNLOCK(&modulelist_mutex);	
		RDA5807M_setMute(0);
		if (cSeek == 1) {RDA5807M_seekStop(); usleep(500000); cSeek = 0;}
		if (iRadioTimeMax == 0) iRadioTimeMax = 300;
		RDA5807M_setVolume(iRadioVol);
		DBG_MUTEX_LOCK(&system_mutex);	
		float flFreq = riRadioStation[pMenu[iMenuNum].SelectedOption - 5].Frequency;
		DBG_MUTEX_UNLOCK(&system_mutex);	
		RDA5807M_setFrequency((int)(flFreq*100));
		AddCurShowType(SHOW_TYPE_RADIO_STREAM);
	}
	cKeyDownClock = 0;	
	DBG_LOG_OUT();
	return 0;
}

int Menu_OffRadioStation(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	iRadioTimeMax = 0;
	add_sys_cmd_in_list(SYSTEM_CMD_RADIO_OFF, 0);
	add_sys_cmd_in_list(SYSTEM_CMD_STREAM_OFF, 0);
	SetShowMenu(0);
	DBG_LOG_OUT();
	return 0;
}

int Menu_NextRadioStation(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	if (iRadioTimeMax == 0) iRadioTimeMax = 300;
	
	int ret1, ret2;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	ret1 = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
	ret2 = ModuleTypeToNum(MODULE_TYPE_RADIO, 1);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if ((ret1 != -1) && (ret2 != -1))
	{
		DBG_MUTEX_LOCK(&modulelist_mutex);	
		MODULE_INFO* SpkMod = ModuleTypeToPoint(MODULE_TYPE_SPEAKER, 1);
		if ((SpkMod) && (SpkMod->SubModule >= 0)) gpio_switch_on_module(&miModuleList[SpkMod->SubModule]);
		DBG_MUTEX_UNLOCK(&modulelist_mutex);		
		RDA5807M_setMute(0);
		RDA5807M_seekUp(1);
	}
	cSeek = 1;
	cKeyDownClock = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_PrevRadioStation(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	if (iRadioTimeMax == 0) iRadioTimeMax = 300;
	
	int ret1, ret2;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	ret1 = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
	ret2 = ModuleTypeToNum(MODULE_TYPE_RADIO, 1);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if ((ret1 != -1) && (ret2 != -1))
	{
		DBG_MUTEX_LOCK(&modulelist_mutex);	
		MODULE_INFO* SpkMod = ModuleTypeToPoint(MODULE_TYPE_SPEAKER, 1);
		if ((SpkMod) && (SpkMod->SubModule >= 0)) gpio_switch_off_module(&miModuleList[SpkMod->SubModule]);
		DBG_MUTEX_UNLOCK(&modulelist_mutex);	
		RDA5807M_setMute(0);
		RDA5807M_seekDown(1);
	}
	cSeek = 1;
	cKeyDownClock = 0;
	DBG_LOG_OUT();
	return 0;
}

int Menu_PlayNextDir(void *pData, int iMenuNum)
{
	DBG_MUTEX_LOCK(&system_mutex);
	int ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_NEXT);
	if (ret == 0)
	{
		if (uiShowModeCur >= 2) uiShowModeCur = 1;
		dbgprintf(3, "No files in Menu_PlayNextDir '%s'\n", cCurrentFileLocation);		
	}
	else
	{
		if (uiShowModeCur < 2) uiShowModeCur = 2;		
	}
		
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (ret) SetChangeShowNow(1); else ShowNewMessage("Нет файлов для воспроизведения");
	SetShowMenu(0);
	return 0;
}

int Menu_PlayPrevDir(void *pData, int iMenuNum)
{
	DBG_MUTEX_LOCK(&system_mutex);
	int ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_PREV);
	if (ret == 0)
	{
		if (uiShowModeCur >= 2) uiShowModeCur = 1;
		dbgprintf(3, "No files in Menu_PlayPrevDir '%s'\n", cCurrentFileLocation);		
	}
	else
	{
		if (uiShowModeCur < 2) uiShowModeCur = 2;		
	}
		
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (ret) SetChangeShowNow(1); else ShowNewMessage("Нет файлов для воспроизведения");
	SetShowMenu(0);
	return 0;
}

int Menu_ChangeWidgetShowMode(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	//int wID = pMenu[iMenuNum].Options[pMenu[iMenuNum].SelectedOption].MiscData;
	int n = pMenu[iMenuNum].SelectedOption;
	//for (n = 0; n < iWidgetsCnt; n++) if (wiWidgetList[n].WidgetID == wID) break;
	DBG_MUTEX_LOCK(&widget_mutex);
	
	if (n < iWidgetsCnt)
	{
		if (wiWidgetList[n].Enabled && (wiWidgetList[n].Status >= 0))
		{
			wiWidgetList[n].ShowMode++;
			if (wiWidgetList[n].ShowMode > WIDGET_SHOWMODE_ALLWAYS) wiWidgetList[n].ShowMode = 0;
			sprintf(pMenu[iMenuNum].Options[pMenu[iMenuNum].SelectedOption].Name, "(%s%s%s%s%s) %s",
					(wiWidgetList[n].ShowMode == 0) ? "Скрыто" : "",
					(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_DAYTIME) ? "Время " : "",
					(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_MENU) ? " Меню " : "",
					(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_TIMEOUT) ? " Интервал " : "",
					(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_ALLWAYS) ? " Всегда" : "",
					wiWidgetList[n].Name);
			if (wiWidgetList[n].Type == WIDGET_TYPE_IMAGE)
			{
				if (wiWidgetList[n].ShowMode == 0) 
				{	
					ReleaseWidget(&wiWidgetList[n]);
				}
				if (wiWidgetList[n].ShowMode) 
				{
					wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
					CreateWidget(&wiWidgetList[n]);
				}
			}		
		}
	}
	UpdateWidgets(state, 0);
	DBG_MUTEX_UNLOCK(&widget_mutex);
	DBG_LOG_OUT();	
	return 0;
}

int Menu_GetNext(void *pData, int iMenuNum)
{
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	SetChangeShowNow(1);
	cPlayDirect = 5;
	pMenu[0].Options[20].Show = 0;
	return 0;
}

int Menu_GetPrev(void *pData, int iMenuNum)
{
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	SetChangeShowNow(1);
	cPlayDirect = 1;
	pMenu[0].Options[20].Show = 0;
	return 0;
}

int Menu_GetAgain(void *pData, int iMenuNum)
{
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	SetChangeShowNow(1);
	cPlayDirect = 2;
	pMenu[0].Options[20].Show = 0;
	return 0;
}

int Menu_SetTimeOut(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	iTimerToNextShow = 0;
	if (iMenuNum == 3)
	{
		switch (pMenu[iMenuNum].SelectedOption)
		{
			case 1:
				SetShowTimeMax(60);
				break;
			case 2:
				SetShowTimeMax(180);
				break;
			case 3:
				SetShowTimeMax(300);
				break;
			case 4:
				SetShowTimeMax(600);
				break;
			case 5:
				SetShowTimeMax(900);
				break;
			case 6:
				SetShowTimeMax(1200);
				break;
			default:
				break;
		}
	}
	if (iMenuNum == 4)
	{
		switch (pMenu[iMenuNum].SelectedOption)
		{
			case 0:
				SetChangeShowNow(1);
				break;
			case 1:
				SetShowTimeMax(60);
				break;
			case 2:
				SetShowTimeMax(180);
				break;
			case 3:
				SetShowTimeMax(300);
				break;
			default:
				break;
		}
	}
	if (iMenuNum == 5)
	{
		switch (pMenu[iMenuNum].SelectedOption)
		{
			case 0:
				SetChangeShowNow(1);
				break;
			case 1:
				Media_Replay();
				break;
			default:
				break;
		}
	}
	if (iMenuNum == 7)
	{
		switch (pMenu[iMenuNum].SelectedOption)
		{
			case 1:
				iRadioTimeMax = 600; 	//10
				break;
			case 2:
				iRadioTimeMax = 900;	//15
				break;
			case 3:
				iRadioTimeMax = 1200;	//20
				break;
			case 4:
				iRadioTimeMax = 1500;	//25
				break;
			case 5:
				iRadioTimeMax = 1800;	//30
				break;
			case 6:
				iRadioTimeMax = 2400;	//40
				break;
			case 7:
				iRadioTimeMax = 3600;	//60
				break;
			case 8:
				iRadioTimeMax = 4800;	//80
				break;
			default:
				break;
		}
	}
	SetShowMenu(0);
	DBG_LOG_OUT();
	return 0;
}

int Menu_SetExposure(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected = iNum;
	unsigned int uiID;
	iSelected = pMenu[iSelected].Options[pMenu[iSelected].SelectedOption].PrevPage;	
	if (iSelected == 3)
	{
		uiID = (unsigned int)pMenu[0].Options[1].MiscData;
	}
	else
	{
		iSelected = pMenu[iSelected].Options[pMenu[iSelected].SelectedOption].PrevPage;
		uiID = pMenu[iSelected].Options[pMenu[iSelected].SelectedOption].MiscData;
	}
	
	switch (pMenu[iNum].SelectedOption)
	{
		case 0:
			SetModuleStatus(uiID, 3, OMX_ExposureControlOff);
			break;
		case 1:
			SetModuleStatus(uiID, 3, OMX_ExposureControlAuto);
			break;
		case 2:
			SetModuleStatus(uiID, 3, OMX_ExposureControlNight);
			break;
		case 3:
			SetModuleStatus(uiID, 3, OMX_ExposureControlBackLight);
			break;
		case 4:
			SetModuleStatus(uiID, 3, OMX_ExposureControlSpotLight);
			break;
		case 5:
			SetModuleStatus(uiID, 3, OMX_ExposureControlSports);
			break;
		case 6:
			SetModuleStatus(uiID, 3, OMX_ExposureControlSnow);
			break;
		case 7:
			SetModuleStatus(uiID, 3, OMX_ExposureControlBeach);
			break;
		case 8:
			SetModuleStatus(uiID, 3, OMX_ExposureControlLargeAperture);
			break;
		case 9:
			SetModuleStatus(uiID, 3, OMX_ExposureControlSmallAperture);
			break;
		case 10:
			SetModuleStatus(uiID, 3, OMX_ExposureControlVeryLong);
			break;
		case 11:
			SetModuleStatus(uiID, 3, OMX_ExposureControlFixedFps);
			break;
		case 12:
			SetModuleStatus(uiID, 3, OMX_ExposureControlNightWithPreview);
			break;
		case 13:
			SetModuleStatus(uiID, 3, OMX_ExposureControlAntishake);
			break;
		case 14:
			SetModuleStatus(uiID, 3, OMX_ExposureControlFireworks);
			break;
		default:
			break;		
	}	
	DBG_LOG_OUT();
	return 1;
}

int DisconnectFromAllModules(int iMenuNum)
{
	DBG_LOG_IN();	
	dbgprintf(5, "DisconnectFromAllModules\n");
	Media_StopPlay(0);
	Media_StopPlay(1);
	omx_stop_video(0);
	Audio_Stop(0);	
	omx_stop_video(1);
	Audio_Stop(1);	
	CloseAllConnects(CONNECT_CLIENT, 0xFFFF, 0);			
	CloseAllConnects(CONNECT_SERVER, 0xFFFF, 0);		
	DBG_LOG_OUT();
	return 1;
}
/*
int StopAllVideoStreams()
{
	DBG_LOG_IN();	
	unsigned int uiType = GetCurShowType();
	if (uiType & (SHOW_TYPE_VIDEO | SHOW_TYPE_CAMERA))
	{
		Media_StopPlay(0);
		Media_StopPlay(1);
		omx_stop_video(0);
		if (uiType & SHOW_TYPE_AUDIO) Audio_Stop(0);	
		omx_stop_video(1);
		if (uiType & SHOW_TYPE_AUDIO) Audio_Stop(1);
	}
	//CloseAllConnects(CONNECT_CLIENT, 0);
	DBG_LOG_OUT();
	return 1;
}*/

int Action_ShowCamera(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
/*	if (cChangeShowNow != 0) return -1;
	int i = 0;
	int DevCnt = 0;	
	iShowTimeMax = 20;
	DBG_MUTEX_LOCK(&host_mutex);
	for (i = 0; i != iHostListCnt; i++)
	{
		if (hHostList[i].Type & DEVICE_TYPE_VIDEO_IN) DevCnt++;
		
		if ((pMenu[iMenuNum].SelectedOption + 1) == DevCnt)
		{			
			break;
		}
	}
	DBG_MUTEX_UNLOCK(&host_mutex);
	if (i != iHostListCnt)
	{
		memcpy(&saCameraAddr, &hHostList[i].Addr, sizeof(struct sockaddr_in));
		cNewShowType = SHOW_TYPE_CAMERA;
		if (hHostList[i].Type & DEVICE_TYPE_AUDIO_IN) 
		{
			memcpy(&saMicAddr, &hHostList[i].Addr, sizeof(struct sockaddr_in));		
			cNewShowType |= SHOW_TYPE_AUDIO_STREAM;	
		}
		cChangeShowNow = 1;
		//if (omx_IsFree_Video() == 0) omx_stop_video(1);
		//if (audio_IsFree() == 0) audio_stop(1);
		pMenu[0].Options[1].Show = 1;
		pMenu[0].Options[0].Show = 0;
		pMenu[0].SelectedOption = 1;	
	}*/
	DBG_LOG_OUT();
	return 0;
}

int Action_ShowCameraModule(unsigned int uiID)
{
	DBG_LOG_IN();
	/*if (GetChangeShowNow() > 1) 
	{
		DBG_LOG_OUT();	
		return -1;
	}*/
	int n = 0;
	int result = 0;
	int ret;
	DBG_MUTEX_LOCK(&system_mutex);
	ret = iCameraTimeMaxSet;
	DBG_MUTEX_UNLOCK(&system_mutex);
	SetShowTimeMax(ret);
	
	DBG_MUTEX_LOCK(&system_mutex);
	if (uiCurCameraID != uiID) uiNextCameraID = uiID; else n = 1;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (n == 0)
	{
		result = 1;
		SetNewShowType(SHOW_TYPE_CAMERA);
		SetChangeShowNow(1);
	}
	//printf("SHOW_TYPE_CAMERA\n");
	//if (omx_IsFree_Video() == 0) omx_stop_video(1);
	//if (audio_IsFree() == 0) audio_stop(1);
	
	DBG_LOG_OUT();
	return result;
}

int Action_PlayAudioModule(unsigned int uiID)
{
	DBG_LOG_IN();
	
	struct sockaddr_in *pMicAddr;
	MODULE_INFO* pModule;	
	int n = 0;
	int result = 0;
	int ret;
	func_link *f_link;
	
	if (GetCurShowType() & SHOW_TYPE_OFF)
	{
		DBG_MUTEX_LOCK(&modulelist_mutex);
		n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		if (n >= 0)
		{
			Audio_Stop(1);
			CloseAllConnects(CONNECT_CLIENT, FLAG_AUDIO_STREAM, 0);				
		
			f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
			memset(f_link, 0, sizeof(func_link));
			f_link->FuncRecv = RecvAudioFrame;
			f_link->ConnectNum = 1;
			f_link->DeviceNum = 0;
			pMicAddr = NULL;
						
			DBG_MUTEX_LOCK(&modulelist_mutex);
			pMicAddr = ModuleIdToAddress(uiID, 2);
			if (pMicAddr) memcpy(&f_link->RecvAddress, pMicAddr, sizeof(f_link->RecvAddress));
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			if (pMicAddr)
			{				
				f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
				DBG_MUTEX_LOCK(&modulelist_mutex);
				memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));								
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				pModule = f_link->pModule;
				if (pModule->Settings[0] == 0) 
				{
					f_link->pSubModule = NULL;
				}
				else 
				{
					f_link->pSubModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
					DBG_MUTEX_LOCK(&modulelist_mutex);
					memcpy(f_link->pSubModule, &miModuleList[miModuleList[n].SubModule], sizeof(MODULE_INFO));	
					DBG_MUTEX_UNLOCK(&modulelist_mutex);					
				}
				audio_set_playback_volume(iCurrentVolume);
				PlayAudioStream(f_link);
				result = 1;
			} else DBG_FREE(f_link);
		}						
	} 
	else 
	{
		n = 0;
		DBG_MUTEX_LOCK(&system_mutex);
		if (uiCurMicID != uiID) uiNextMicID = uiID; else n = 1;
		ret = iCameraTimeMaxSet;
		DBG_MUTEX_UNLOCK(&system_mutex);		
		if (n == 0)
		{
			result = 1;
			SetNewShowType(SHOW_TYPE_MIC_STREAM);
			//printf("SHOW_TYPE_MIC_STREAM\n");
			SetShowTimeMax(ret);
			SetChangeShowNow(1);
		}
	}
	
	DBG_LOG_OUT();
	return result;
}

int Menu_RefreshDirectory(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int n, i;
	unsigned int ilen;
	
	if (iMenuNum == 16) 
	{
		memset(pMenu[16].Path, 0, MAX_FILE_LEN);
		memcpy(pMenu[16].Path, diDirList[pMenu[16].SelectedOption].Path, strlen(diDirList[pMenu[16].SelectedOption].Path));
	}
	else
	{
		if (pMenu[17].SelectedOption == 0)
		{
			i = strlen(pMenu[16].Path);
			for(n = (i - 1); n >= 0; n--) if (pMenu[16].Path[n] == 47) break;
			if (n > 0) memset(&pMenu[16].Path[n], 0, MAX_FILE_LEN - n);
			else
			{
				pMenu[17].Options[pMenu[17].SelectedOption].NextPage = 16;
				pMenu[17].SelectedOption = 0;
				DBG_LOG_OUT();
				return 0;
			}
		}
		else
		{
			if ((strlen(pMenu[16].Path) + strlen(pMenu[17].Options[pMenu[17].SelectedOption].Name2) + 1) < MAX_FILE_LEN)
			{
				strcat(pMenu[16].Path, "/");
				strcat(pMenu[16].Path, pMenu[17].Options[pMenu[17].SelectedOption].Name2);
				memset(pMenu[iMenuNum].Name, 0, 64);
				strcpy(pMenu[iMenuNum].Name, pMenu[17].Options[pMenu[17].SelectedOption].Name);
			}				
		}			
	}
	
	iMenuNum = 17;	
	
	if (pMenu[iMenuNum].CountOptions != 0)
	{
		if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
		pMenu[iMenuNum].Options = NULL;
		pMenu[iMenuNum].CountOptions = 0;
	}

	memset(pMenu[17].Name, 0, 64);
	ilen = 64;
	utf8_to_cp866(pMenu[16].Path, (strlen(pMenu[16].Path) < ilen) ? strlen(pMenu[16].Path) : 60, pMenu[17].Name, &ilen);
					
	//if (strlen(pMenu[16].Path) < 64) memcpy(pMenu[17].Name, pMenu[16].Path, strlen(pMenu[16].Path));
		//else memcpy(pMenu[17].Name, &pMenu[16].Path[strlen(pMenu[16].Path)-63], 63);
	
	pMenu[iMenuNum].SelectedOption = 0;
	pMenu[iMenuNum].CountOptions = 1;
	pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(sizeof(MENU_OPTION));
	memset(&pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1], 0, sizeof(MENU_OPTION));
	strcpy(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name, "..");								
	pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Show = 1;
	pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPage = 16;
	pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPageNoHistory = 1;
	pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].ActionFunc = Menu_RefreshDirectory;
	pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 17;
		
	DIR *dir;
	struct dirent *dp;
	dir = opendir(pMenu[16].Path);
	if (dir != NULL)
	{
		/*pMenu[iMenuNum].SelectedOption = 0;
		pMenu[iMenuNum].CountOptions = 1;
		pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(sizeof(MENU_OPTION));
		memset(&pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1], 0, sizeof(MENU_OPTION));
		strcpy(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name, "..");								
		pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Show = 1;
		pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPage = 16;
		pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].ActionFunc = Menu_RefreshDirectory;*/
		if (strcmp(pMenu[16].Path, diDirList[pMenu[16].SelectedOption].Path) == 0)
		{
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 16;
		}
		else
		{
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 17;
		}
		{		
			pMenu[iMenuNum].CountOptions++;
			pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_REALLOC(pMenu[iMenuNum].Options, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
			memset(&pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1], 0, sizeof(MENU_OPTION));
			strcpy(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name, "[Воспроизвести папку]");
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Show = 1;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPage = 16;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPageNoHistory = 1;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 17;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].ActionFunc = Menu_PlayDirectory;	
			
			pMenu[iMenuNum].CountOptions++;
			pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_REALLOC(pMenu[iMenuNum].Options, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
			memset(&pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1], 0, sizeof(MENU_OPTION));
			strcpy(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name, "[Воспроизвести подпапки]");
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Show = 1;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPage = 16;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPageNoHistory = 1;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 17;
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].ActionFunc = Menu_PlaySubDirectories;	
		}
	
		while((dp=readdir(dir)) != NULL)
		{
			n = 1;
			if (((strlen(dp->d_name) == 1) && (dp->d_name[0] == 46))
				||
				((strlen(dp->d_name) == 2) && (dp->d_name[0] == 46) && (dp->d_name[1] == 46))) n = 0;
				
			if ((strlen(dp->d_name) < MAX_FILE_LEN) && (n == 1))
			{			
				pMenu[iMenuNum].CountOptions++;
				pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_REALLOC(pMenu[iMenuNum].Options, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
				memset(&pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1], 0, sizeof(MENU_OPTION));
							
				memset(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name , 0, 64);
				ilen = 63;
				utf8_to_cp866(dp->d_name, (strlen(dp->d_name) < MAX_FILE_LEN) ? strlen(dp->d_name) : MAX_FILE_LEN - 1, pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name, &ilen);
				
				ilen = strlen(dp->d_name);
				memset(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name2, 0, MAX_FILE_LEN);
				if (ilen < MAX_FILE_LEN) memcpy(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name2, dp->d_name, ilen);
					else memcpy(pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Name2, dp->d_name, MAX_FILE_LEN - 1);
				pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].Show = 1;
				pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPage = 16;
				pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].PrevPageNoHistory = 1;
				pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 17;
				pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].ActionFunc = Menu_RefreshDirectory;
			}
		}
		closedir(dir);
    }
	else 
	{
		if (errno == ENOTDIR)
		{
			memcpy(cCurrentFile, pMenu[16].Path, 256);
			//printf("file: %s\n", cBuff);
			int ret = GetFileType(pMenu[16].Path);
			if (ret == FILE_TYPE_MEDIA) SetNewShowType(SHOW_TYPE_FILE); else SetNewShowType(SHOW_TYPE_IMAGE);
			SetChangeShowNow(1);
			//cShowMenu = 0;
		} 
		else 
		{
			dbgprintf(3,"opendir path:'%s'\n", pMenu[16].Path);
			dbgprintf(3,"opendir error:(%i)%s\n", errno, strerror(errno));
			pMenu[iMenuNum].Options[pMenu[iMenuNum].CountOptions-1].NextPage = 16;
		}
	}
	
	DBG_LOG_OUT();
	return 0;
}

char Its_Directory(char* cPath)
{
	char ret = -1;
	DIR *dir = opendir(cPath);
	if (dir != NULL)
	{
		ret = 1;
		closedir(dir);
    }
	else 
	{
		if (errno == ENOTDIR) ret = 0;
	}
	return ret;
}

int Menu_RefreshCameraMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	int i, n, Cnt, k;
	char buff2[256];
	char buff3[128];
	iMenuNum = 1;
	Cnt = 0;
	for (i = 0; i < iModuleCnt; i++) 
		if ((miModuleList[i].Enabled & 1) && (miModuleList[i].Type == MODULE_TYPE_CAMERA)) Cnt++;
		
	if (Cnt == 0)
	{
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Нет доступных камер");
		//printf("cnt: %i, %s\n",iModuleCnt, "Нет доступных камер");
		/*
		pMenu[iMenuNum].CountOptions = 1;
		pMenu[iMenuNum].SelectedOption = 0;
		pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		pMenu[iMenuNum].Options[0].Show = 1;
		pMenu[iMenuNum].Options[0].PrevPage = 0; //iPrevMenu;
		pMenu[iMenuNum].Options[0].NextPage = 0;
		pMenu[iMenuNum].Options[0].ActionFunc = NULL;
		pMenu[iMenuNum].Options[0].Service = 1;
		strcat(pMenu[iMenuNum].Options[0].Name, "Нет доступных камер");*/
	}
	else
	{	
		//printf("RefreshModuleListMenu %i\n",pMenu[iMenuNum].CountOptions);
		if (pMenu[iMenuNum].CountOptions != 0)
		{
			if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
			pMenu[iMenuNum].Options = NULL;
			pMenu[iMenuNum].CountOptions = 0;
		}	
		pMenu[iMenuNum].CountOptions = Cnt;
		if (pMenu[iMenuNum].SelectedOption >= pMenu[iMenuNum].CountOptions) pMenu[iMenuNum].SelectedOption = 0;
		pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		Cnt = 0;
		for (i = 0; i < iModuleCnt; i++)
		{
			k = ModuleSortToNum(i);
			if ((miModuleList[k].Enabled & 1) && (miModuleList[k].Type == MODULE_TYPE_CAMERA))
			{				
				pMenu[iMenuNum].Options[Cnt].Show = 1;
				pMenu[iMenuNum].Options[Cnt].MiscData = miModuleList[k].ID;
				pMenu[iMenuNum].Options[Cnt].PrevPage = 0; //iPrevMenu;
				memset(buff3,0,128);
				memset(pMenu[iMenuNum].Options[Cnt].Name,0,64);
				for (n = 0; n < iModuleCnt; n++) 
				{
					if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_MIC)						
						&& (miModuleList[n].Address.sin_addr.s_addr == miModuleList[k].Address.sin_addr.s_addr))
							{
								sprintf(buff3, "+%s(%.4s)", miModuleList[n].Name, (char*)&miModuleList[n].ID);
								break;
							}
				}
				memset(buff2, 0, 256);
				sprintf(buff2, "%s(%.4s)%s", miModuleList[k].Name, (char*)&miModuleList[k].ID, buff3);
				pMenu[iMenuNum].Options[Cnt].NextPage = 0;	
				pMenu[iMenuNum].Options[Cnt].ActionFunc = Menu_CameraAction;
				pMenu[iMenuNum].Options[Cnt].Access = miModuleList[k].AccessLevel;
				pMenu[iMenuNum].Options[Cnt].ViewLevel = miModuleList[k].ViewLevel;
				if (strlen(buff2) < 64) memcpy(pMenu[iMenuNum].Options[Cnt].Name, buff2, strlen(buff2));
					else memcpy(pMenu[iMenuNum].Options[Cnt].Name, buff2, 63);
				Cnt++;
			}
		}			
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshRadiosMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	
	DBG_MUTEX_LOCK(&system_mutex);	
	
	if (pMenu[6].CountOptions > 5)
	{
		if (pMenu[6].Options != NULL) DBG_FREE(pMenu[6].Options);
		pMenu[6].Options = NULL;
		pMenu[6].CountOptions = 0;
	}	
	pMenu[6].CountOptions = iRadioStationsCnt + 5;;
	if (pMenu[6].SelectedOption >= pMenu[6].CountOptions) pMenu[6].SelectedOption = 0;
    pMenu[6].RefreshFunc = Menu_GetCurrentRadioStation;   
	pMenu[6].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[6].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[6].Options, 0, pMenu[6].CountOptions*sizeof(MENU_OPTION));
	
	pMenu[6].Options[0].Show = 1;
	pMenu[6].Options[0].PrevPage = 0;
	pMenu[6].Options[0].NextPage = 7;
	pMenu[6].Options[0].ActionFunc = NULL;
	strcpy(pMenu[6].Options[0].Name,"Отключить");
	pMenu[6].Options[1].Show = 1;
	pMenu[6].Options[1].PrevPage = 0;
	pMenu[6].Options[1].NextPage = 0;
	pMenu[6].Options[1].ActionFunc = Menu_NextRadioStation;
	strcpy(pMenu[6].Options[1].Name,"Сканировать вверх");
	pMenu[6].Options[2].Show = 1;
	pMenu[6].Options[2].PrevPage = 0;
	pMenu[6].Options[2].NextPage = 0;
	pMenu[6].Options[2].ActionFunc = Menu_PrevRadioStation;
	strcpy(pMenu[6].Options[2].Name,"Сканировать вниз");
	pMenu[6].Options[3].Show = 1;
	pMenu[6].Options[3].PrevPage = 0;
	pMenu[6].Options[3].NextPage = 0;
	pMenu[6].Options[3].ActionFunc = Menu_RadioVolumeUp;
	strcpy(pMenu[6].Options[3].Name,"Увеличить громкость");
	pMenu[6].Options[4].Show = 1;
	pMenu[6].Options[4].PrevPage = 0;
	pMenu[6].Options[4].NextPage = 0;
	pMenu[6].Options[4].ActionFunc = Menu_RadioVolumeDown;
	strcpy(pMenu[6].Options[4].Name,"Уменьшить громкость");
	
	int i;
	for (i = 5; i != (iRadioStationsCnt + 5); i++)
	{
		pMenu[6].Options[i].Show = 1;
		pMenu[6].Options[i].PrevPage = 0;
		pMenu[6].Options[i].NextPage = 0;
		pMenu[6].Options[i].ActionFunc = Menu_OnRadioStation;
		sprintf(pMenu[6].Options[i].Name, "%s(%.1f)", riRadioStation[i-5].Name, riRadioStation[i-5].Frequency);
	}
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshAlarmsMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	
	iMenuNum = 2;
	DBG_MUTEX_LOCK(&system_mutex);	
	int i;
	if (iAlarmClocksCnt == 0)
	{
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Нет будильников");
	}
	else
	{
		//printf("RefreshModuleListMenu %i\n",pMenu[iMenuNum].CountOptions);
		if (pMenu[iMenuNum].CountOptions != 0)
		{
			if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
			pMenu[iMenuNum].Options = NULL;
			pMenu[iMenuNum].CountOptions = 0;
		}	
	
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Список будильников");		
		pMenu[iMenuNum].CountOptions = iAlarmClocksCnt;
		if (pMenu[iMenuNum].SelectedOption >= pMenu[iMenuNum].CountOptions) pMenu[iMenuNum].SelectedOption = 0;
		pMenu[iMenuNum].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		for (i = 0; i < iAlarmClocksCnt; i++)
		{
			pMenu[iMenuNum].Options[i].Show = 1;
			pMenu[iMenuNum].Options[i].PrevPage = 0;
			pMenu[iMenuNum].Options[i].NextPage = 0;
			pMenu[iMenuNum].Options[i].ActionFunc = Menu_AlarmClockSwitch;
			sprintf(pMenu[iMenuNum].Options[i].Name, "%s(%i) %i %s%s%s%s%s%s%s",
				actAlarmClockInfo[i].Enabled ? "Вкл." : "Выкл.",
				actAlarmClockInfo[i].Skip,				
				actAlarmClockInfo[i].Time,
				actAlarmClockInfo[i].Days & 1 ? "[ПН]" : "",
				actAlarmClockInfo[i].Days & 2 ? "[ВТ]" : "",
				actAlarmClockInfo[i].Days & 4 ? "[СР]" : "",
				actAlarmClockInfo[i].Days & 8 ? "[ЧТ]" : "",
				actAlarmClockInfo[i].Days & 16 ? "[ПТ]" : "",
				actAlarmClockInfo[i].Days & 32 ? "[СБ]" : "",
				actAlarmClockInfo[i].Days & 64 ? "[ВС]" : "");
		}
	}		
	DBG_MUTEX_UNLOCK(&system_mutex);
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshModuleTypeList(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i;
	int iCnt = 0;
	char cTypes[MODULE_TYPE_MAX];
	memset(cTypes, 0, MODULE_TYPE_MAX);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (i = 0; i < iModuleCnt; i++) 
		if (miModuleList[i].Enabled & 1) 
		{
			if (miModuleList[i].Type <= MODULE_TYPE_MAX) 
				cTypes[miModuleList[i].Type] = 1;
				else
				cTypes[0] = 1;
		}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	for (i = 0; i < MODULE_TYPE_MAX; i++) if (cTypes[i]) iCnt++;
		
	if (pMenu[26].CountOptions != 0)
	{
		if (pMenu[26].Options != NULL) DBG_FREE(pMenu[26].Options);
		pMenu[26].Options = NULL;
		pMenu[26].CountOptions = 0;
	}	
	pMenu[26].CountOptions = iCnt;
	if (pMenu[26].SelectedOption >= pMenu[26].CountOptions) pMenu[26].SelectedOption = 0;
    pMenu[26].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[26].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[26].Options, 0, pMenu[26].CountOptions*sizeof(MENU_OPTION));
	
	iCnt = 0;
	for (i = 0; i < MODULE_TYPE_MAX; i++)
	{
		if (cTypes[i])
		{
			pMenu[26].Options[iCnt].Show = 1;
			pMenu[26].Options[iCnt].PrevPage = 0;
			pMenu[26].Options[iCnt].NextPage = 8;
			pMenu[26].Options[iCnt].MiscData = i;
			pMenu[26].Options[iCnt].ActionFunc = NULL;
			pMenu[26].Options[iCnt].Access = 0;
			strcpy(pMenu[26].Options[iCnt].Name, GetModuleTypeShowName(i));
			iCnt++;
		}
	}	
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshStreamTypeList(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i;
	DBG_MUTEX_LOCK(&system_mutex);	
	
	if (pMenu[18].CountOptions != 0)
	{
		if (pMenu[18].Options != NULL) DBG_FREE(pMenu[18].Options);
		pMenu[18].Options = NULL;
		pMenu[18].CountOptions = 0;
	}	
	pMenu[18].CountOptions = iStreamTypeCnt;
	if (pMenu[18].SelectedOption >= pMenu[18].CountOptions) pMenu[18].SelectedOption = 0;
    pMenu[18].RefreshFunc = NULL;   
	pMenu[18].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[18].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[18].Options, 0, pMenu[18].CountOptions*sizeof(MENU_OPTION));
	
	for (i = 0; i < iStreamTypeCnt; i++)
	{
		pMenu[18].Options[i].Show = 1;
		pMenu[18].Options[i].PrevPage = 0;
		pMenu[18].Options[i].NextPage = 20;
		pMenu[18].Options[i].MiscData = i;
		pMenu[18].Options[i].ActionFunc = NULL;
		pMenu[18].Options[i].Access = 0;
		if (strlen(stStreamType[i].Name) < 64) memcpy(pMenu[18].Options[i].Name, stStreamType[i].Name, strlen(stStreamType[i].Name));
			else memcpy(pMenu[18].Options[i].Name, &stStreamType[i].Name[strlen(stStreamType[i].Name)-63], 63);		
	}
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshStreamList(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iCurPos = pMenu[18].SelectedOption;
	int iCnt = 0;
	int i;
	DBG_MUTEX_LOCK(&system_mutex);	
	
	for(i = 0; i < iInternetRadioCnt; i++) if (irInternetRadio[i].Type == iCurPos) iCnt++;
	if (pMenu[20].CountOptions != 0)
	{
		if (pMenu[20].Options != NULL) DBG_FREE(pMenu[20].Options);
		pMenu[20].Options = NULL;
		pMenu[20].CountOptions = 0;
	}	
	pMenu[20].CountOptions = iCnt;
	if (pMenu[20].SelectedOption >= pMenu[20].CountOptions) pMenu[20].SelectedOption = 0;
    pMenu[20].RefreshFunc = NULL;   
	pMenu[20].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[20].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[20].Options, 0, pMenu[20].CountOptions*sizeof(MENU_OPTION));
	iCnt = 0;
	
	for (i = 0; i < iInternetRadioCnt; i++)
	{
		if (irInternetRadio[i].Type == iCurPos)
		{
			pMenu[20].Options[iCnt].Show = 1;
			pMenu[20].Options[iCnt].PrevPage = 0;
			pMenu[20].Options[iCnt].NextPage = 7;
			pMenu[20].Options[iCnt].MiscData = i;
			pMenu[20].Options[iCnt].ActionFunc = Menu_InternetRadioOn;
			pMenu[20].Options[iCnt].Access = irInternetRadio[i].Access;
			if (strlen(irInternetRadio[i].Name) < 64) memcpy(pMenu[20].Options[iCnt].Name, irInternetRadio[i].Name, strlen(irInternetRadio[i].Name));
				else memcpy(pMenu[20].Options[iCnt].Name, &irInternetRadio[i].Name[strlen(irInternetRadio[i].Name)-63], 63);
			iCnt++;
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshMnlActionList(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i;
	
	if (pMenu[21].CountOptions != 0)
	{
		if (pMenu[21].Options != NULL) DBG_FREE(pMenu[20].Options);
		pMenu[21].Options = NULL;
		pMenu[21].CountOptions = 0;
	}	
	DBG_MUTEX_LOCK(&mnlaction_mutex);	
	pMenu[21].CountOptions = iActionManualCnt;
	if (pMenu[21].SelectedOption >= pMenu[21].CountOptions) pMenu[21].SelectedOption = 0;
    pMenu[21].RefreshFunc = NULL;   
	pMenu[21].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[21].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[21].Options, 0, pMenu[21].CountOptions*sizeof(MENU_OPTION));
	
	for (i = 0; i < iActionManualCnt; i++)
	{
		pMenu[21].Options[i].Show = 1;
		pMenu[21].Options[i].PrevPage = 0;
		pMenu[21].Options[i].NextPage = 0;
		pMenu[21].Options[i].ActionFunc = Menu_ActionMenuExec;
		pMenu[21].Options[i].Access = amiActionManual[i].Access;
		if (strlen(amiActionManual[i].Name) < 64) memcpy(pMenu[21].Options[i].Name, amiActionManual[i].Name, strlen(amiActionManual[i].Name));
			else memcpy(pMenu[21].Options[i].Name, &amiActionManual[i].Name[strlen(amiActionManual[i].Name)-63], 63);
	}
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);	
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshWidgetList(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i;
		
	if (pMenu[15].CountOptions != 0)
	{
		if (pMenu[15].Options != NULL) DBG_FREE(pMenu[15].Options);
		pMenu[15].Options = NULL;
		pMenu[15].CountOptions = 0;
	}	
	DBG_MUTEX_LOCK(&widget_mutex);	
	pMenu[15].CountOptions = iWidgetsCnt;
	if (pMenu[15].SelectedOption >= pMenu[15].CountOptions) pMenu[15].SelectedOption = 0;
	pMenu[15].RefreshFunc = NULL;   
	pMenu[15].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[15].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[15].Options, 0, pMenu[15].CountOptions*sizeof(MENU_OPTION));
	
	for (i = 0; i < iWidgetsCnt; i++)
	{
		pMenu[15].Options[i].Show = (wiWidgetList[i].Status >= 0) ? 1 : 0;
		pMenu[15].Options[i].PrevPage = 0;
		pMenu[15].Options[i].NextPage = 0;
		pMenu[15].Options[i].ActionFunc = Menu_ChangeWidgetShowMode;
		pMenu[15].Options[i].MiscData = wiWidgetList[i].WidgetID;
		sprintf(pMenu[15].Options[i].Name, "(%s%s%s%s%s) %s",
			(wiWidgetList[i].ShowMode == 0) ? "Скрыто" : "",
			(wiWidgetList[i].ShowMode & WIDGET_SHOWMODE_DAYTIME) ? "Время " : "",
			(wiWidgetList[i].ShowMode & WIDGET_SHOWMODE_MENU) ? " Меню " : "",
			(wiWidgetList[i].ShowMode & WIDGET_SHOWMODE_TIMEOUT) ? " Интервал " : "",
			(wiWidgetList[i].ShowMode & WIDGET_SHOWMODE_ALLWAYS) ? " Всегда" : "",
			wiWidgetList[i].Name);
	}
	
	DBG_MUTEX_UNLOCK(&widget_mutex);	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshDirList(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i;
		
	if (pMenu[16].CountOptions != 0)
	{
		if (pMenu[16].Options != NULL) DBG_FREE(pMenu[16].Options);
		pMenu[16].Options = NULL;
		pMenu[16].CountOptions = 0;
	}	
	DBG_MUTEX_LOCK(&system_mutex);	
	pMenu[16].CountOptions = iDirsCnt;
	if (pMenu[16].SelectedOption >= pMenu[16].CountOptions) pMenu[16].SelectedOption = 0;
	pMenu[16].RefreshFunc = NULL;   
	pMenu[16].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[16].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[16].Options, 0, pMenu[16].CountOptions*sizeof(MENU_OPTION));
	
	for (i = 0; i < iDirsCnt; i++)
	{
		pMenu[16].Options[i].Show = 1;
		pMenu[16].Options[i].PrevPage = 0;
		pMenu[16].Options[i].PrevPageNoHistory = 1;
		pMenu[16].Options[i].NextPage = 17;
		pMenu[16].Options[i].ActionFunc = Menu_RefreshDirectory;
		pMenu[16].Options[i].Access = diDirList[i].Access;
		if (strlen(diDirList[i].Name) < 64) memcpy(pMenu[16].Options[i].Name, diDirList[i].Name, strlen(diDirList[i].Name));
			else memcpy(pMenu[16].Options[i].Name, &diDirList[i].Name[strlen(diDirList[i].Name)-63], 63);
	}
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshArchivListMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	int i, k, iModuleEnblCnt, iCnt;
	char buff2[128];
	iMenuNum = 25;
	
	iModuleEnblCnt = 0;
	for (i = 0; i < iModuleCnt; i++) if ((miModuleList[i].Enabled & 1) && 
										(miModuleList[i].Type == MODULE_TYPE_SYSTEM) &&
										miModuleList[i].Settings[4]) iModuleEnblCnt++;
	if (iModuleEnblCnt == 0)
	{
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Нет доступных архивов");
	}
	else
	{
		//printf("RefreshModuleListMenu %i\n",pMenu[iMenuNum].CountOptions);
		int iCurPos = 0;
		if (pMenu[iMenuNum].CountOptions != 0)
		{
			iCurPos = pMenu[iMenuNum].SelectedOption;
			if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
			pMenu[iMenuNum].Options = NULL;
			pMenu[iMenuNum].CountOptions = 0;
		}
		
		if (iModuleEnblCnt <= iCurPos) iCurPos = 0;
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Список доступных архивов");		
		pMenu[iMenuNum].CountOptions = iModuleEnblCnt;
		pMenu[iMenuNum].SelectedOption = iCurPos;
		pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		iCnt = 0;
		for (i = 0; i < iModuleCnt; i++)
		{
			k = ModuleSortToNum(i);				
			if ((miModuleList[k].Enabled & 1) && (miModuleList[k].Type == MODULE_TYPE_SYSTEM) && miModuleList[k].Settings[4])
			{
				pMenu[iMenuNum].Options[iCnt].Show = 1;
				pMenu[iMenuNum].Options[iCnt].MiscData = miModuleList[k].ID;
				pMenu[iMenuNum].Options[iCnt].PrevPage = 0; //iPrevMenu;
				memset(buff2,0,128);
				memset(pMenu[iMenuNum].Options[iCnt].Name,0,64);
				sprintf(buff2, "%s(%.4s)", miModuleList[k].Name, (char*)&miModuleList[k].ID);
				pMenu[iMenuNum].Options[iCnt].NextPage = 0;
				pMenu[iMenuNum].Options[iCnt].ActionFunc = Menu_ArchivAction;
				pMenu[iMenuNum].Options[iCnt].Access = (miModuleList[k].Settings[4] & 1) ? miModuleList[k].Settings[5] : miModuleList[k].Settings[6];
				pMenu[iMenuNum].Options[iCnt].ViewLevel = pMenu[iMenuNum].Options[iCnt].Access;
				if (strlen(buff2) < 64) memcpy(pMenu[iMenuNum].Options[iCnt].Name, buff2, strlen(buff2));
					else memcpy(pMenu[iMenuNum].Options[iCnt].Name, buff2, 63);
				iCnt++;
			}
		}
	}		
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshModuleListMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	
	int iTypeNum = pMenu[26].Options[pMenu[26].SelectedOption].MiscData;
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	int i, k, iModuleTypeCnt, iCnt;
	char buff2[128];
	iMenuNum = 8;
	if (iModuleCnt == 0)
	{
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Нет доступных устройств");
	}
	else
	{
		//printf("RefreshModuleListMenu %i\n",pMenu[iMenuNum].CountOptions);
		int iCurPos = 0;
		if (pMenu[iMenuNum].CountOptions != 0)
		{
			iCurPos = pMenu[iMenuNum].SelectedOption;
			if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
			pMenu[iMenuNum].Options = NULL;
			pMenu[iMenuNum].CountOptions = 0;
		}
		
		iModuleTypeCnt = 0;
		for (i = 0; i < iModuleCnt; i++) 
			if ((miModuleList[i].Enabled & 1) && (miModuleList[i].Type == iTypeNum)) iModuleTypeCnt++;
	
		if (iModuleTypeCnt <= iCurPos) iCurPos = 0;
		memset(pMenu[iMenuNum].Name, 0, 64);		
		strcpy(pMenu[iMenuNum].Name, "Модули: ");	
		strcat(pMenu[iMenuNum].Name, GetModuleTypeShowName(iTypeNum));	
		
		pMenu[iMenuNum].CountOptions = iModuleTypeCnt;
		pMenu[iMenuNum].SelectedOption = iCurPos;
		pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		iCnt = 0;
		for (i = 0; i < iModuleCnt; i++)
		{
			k = ModuleSortToNum(i);				
			if ((miModuleList[k].Enabled & 1) && (miModuleList[k].Type == iTypeNum))
			{
				pMenu[iMenuNum].Options[iCnt].Show = 1;
				pMenu[iMenuNum].Options[iCnt].MiscData = miModuleList[k].ID;
				pMenu[iMenuNum].Options[iCnt].PrevPage = 26; //iPrevMenu;
				memset(buff2,0,128);
				memset(pMenu[iMenuNum].Options[iCnt].Name,0,64);
				sprintf(buff2, "%s(%.4s)", miModuleList[k].Name, (char*)&miModuleList[k].ID);
				pMenu[iMenuNum].Options[iCnt].ActionFunc = NULL;				
				switch(miModuleList[k].Type)
				{
						case MODULE_TYPE_EXTERNAL:
						case MODULE_TYPE_USB_GPIO:
						case MODULE_TYPE_TFP625A:						
							break;
						case MODULE_TYPE_GPIO:
						case MODULE_TYPE_TEMP_SENSOR:
						case MODULE_TYPE_ADS1015:
						case MODULE_TYPE_MCP3421:
						case MODULE_TYPE_AS5600:
						case MODULE_TYPE_HMC5883L:
							pMenu[iMenuNum].Options[iCnt].NextPage = 9;
							pMenu[iMenuNum].Options[iCnt].ActionFunc = Menu_GpioAction;
							break;
						case MODULE_TYPE_PN532:
							break;
						case MODULE_TYPE_RTC:
							break;
						case MODULE_TYPE_RADIO:
							pMenu[iMenuNum].Options[iCnt].NextPage = 8;
							break;
						case MODULE_TYPE_SPEAKER:
							pMenu[iMenuNum].Options[iCnt].ActionFunc = Menu_ModuleAction;				
							break;
						case MODULE_TYPE_MIC:
							pMenu[iMenuNum].Options[iCnt].ActionFunc = Menu_ModuleAction;				
							break;
						case MODULE_TYPE_DISPLAY:
							break;
						case MODULE_TYPE_CAMERA:
							pMenu[iMenuNum].Options[iCnt].NextPage = 13;
							break;
						case MODULE_TYPE_RS485:
							break;
						case MODULE_TYPE_SYSTEM:
							pMenu[iMenuNum].Options[iCnt].NextPage = 19;
							pMenu[iMenuNum].Options[iCnt].ActionFunc = Menu_GpioAction;
							break;
						case MODULE_TYPE_KEYBOARD:
							break;
						default:
							pMenu[iMenuNum].Options[iCnt].NextPage = 0;
							break;
				}
				pMenu[iMenuNum].Options[iCnt].Access = miModuleList[k].AccessLevel;
				pMenu[iMenuNum].Options[iCnt].ViewLevel = miModuleList[k].ViewLevel;
				if (strlen(buff2) < 64) memcpy(pMenu[iMenuNum].Options[iCnt].Name, buff2, strlen(buff2));
					else memcpy(pMenu[iMenuNum].Options[iCnt].Name, buff2, 63);
				iCnt++;
			}
		}
	}		
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	DBG_LOG_OUT();
	return 0;
}


int Menu_ClearMessageList(void *pData, int iNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&message_mutex);	
	if (iMessagesListCnt != 0) 
	{
		DBG_FREE(cMessagesList);
		cMessagesList = NULL;
		iMessageListChanged = 1;
		iMessagesListCnt = 0;	
	}
	DBG_MUTEX_UNLOCK(&message_mutex);	
	if (pData != NULL) Menu_RefreshMessageListMenu(pData, iNum);
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshActionListMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&evntaction_mutex);	
	int i;
	
	if (iActionCnt)
	{
		if (pMenu[22].Options != NULL) DBG_FREE(pMenu[22].Options);
		pMenu[22].Options = NULL;
		pMenu[22].CountOptions = iActionCnt;		
		if (pMenu[22].SelectedOption >= iActionCnt) pMenu[22].SelectedOption = 0;		
		pMenu[22].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[22].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[22].Options, 0, pMenu[22].CountOptions*sizeof(MENU_OPTION));
		
		for (i = 0; i != iActionCnt; i++)
		{
			pMenu[22].Options[i].Show = 1;
			pMenu[22].Options[i].PrevPage = 0; //iPrevMenu;
			memset(pMenu[22].Options[i].Name,0,64);			
			pMenu[22].Options[i].NextPage = 0;
			pMenu[22].Options[i].ActionFunc = Menu_ActionEventSet;
			pMenu[22].Options[i].Access = 0;
			if (maActionInfo[i].Enabled > 0) strcpy(pMenu[22].Options[i].Name, "[*]"); else strcpy(pMenu[22].Options[i].Name, "[ ]");
			memcpy(&pMenu[22].Options[i].Name[3], maActionInfo[i].Name, 60);
		}
	}		
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	DBG_LOG_OUT();
	return 0;
}

int Menu_RefreshMessageListMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&message_mutex);	
	int i, len;
	
	if (iMessageListChanged == 1)
	{
		iMessageListChanged = 0;
		//printf("RefreshModuleListMenu %i\n",pMenu[iMenuNum].CountOptions);
		if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
		pMenu[iMenuNum].Options = NULL;
		pMenu[iMenuNum].CountOptions = iMessagesListCnt + 1;
		pMenu[iMenuNum].SelectedOption = 0;
		pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
		pMenu[iMenuNum].Options[0].Show = 1;
		pMenu[iMenuNum].Options[0].PrevPage = 0; //iPrevMenu;
		pMenu[iMenuNum].Options[0].NextPage = 0;
		pMenu[iMenuNum].Options[0].ActionFunc = Menu_ClearMessageList;
		strcat(pMenu[iMenuNum].Options[0].Name, "Очистить");
		for (i = 1; i != (iMessagesListCnt + 1); i++)
		{
			pMenu[iMenuNum].Options[i].Show = 1;
			pMenu[iMenuNum].Options[i].PrevPage = 0; //iPrevMenu;
			memset(pMenu[iMenuNum].Options[i].Name,0,64);
			pMenu[iMenuNum].Options[i].NextPage = 0;
			pMenu[iMenuNum].Options[i].ActionFunc = NULL;
			pMenu[iMenuNum].Options[i].Access = 0;
			len = (i - 1) * MESSAGES_MAX_LEN;
			if (strlen(&cMessagesList[len]) < 64) memcpy(pMenu[iMenuNum].Options[i].Name, &cMessagesList[len], strlen(&cMessagesList[len]));
				else memcpy(pMenu[iMenuNum].Options[i].Name, &cMessagesList[len], 63);
		}
	}		
	DBG_MUTEX_UNLOCK(&message_mutex);
	DBG_LOG_OUT();
	return 0;
}

int RefreshDirListMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);
	
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i, len;
	unsigned int blen;
	//char cbuff[2048];
	iMenuNum = 24;
	if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
	pMenu[iMenuNum].Options = NULL;
	pMenu[iMenuNum].CountOptions = iListDirsCount + 2;
	if (pMenu[iMenuNum].SelectedOption >= iListDirsCount) pMenu[iMenuNum].SelectedOption = 0;
	pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
	pMenu[iMenuNum].Options[0].Show = 1;
	pMenu[iMenuNum].Options[0].PrevPage = 0; //iPrevMenu;
	pMenu[iMenuNum].Options[0].NextPage = 0;
	pMenu[iMenuNum].Options[0].ActionFunc = Menu_SwitchPlayDirMode;
	strcat(pMenu[iMenuNum].Options[0].Name, "Сменить порядок воспроизведения");
	pMenu[iMenuNum].Options[1].Show = 1;
	pMenu[iMenuNum].Options[1].PrevPage = 0; //iPrevMenu;
	pMenu[iMenuNum].Options[1].NextPage = 0;
	pMenu[iMenuNum].Options[1].ActionFunc = Menu_SwitchDirOrder;
	strcat(pMenu[iMenuNum].Options[1].Name, "Сбросить порядок воспроизведения");
	for (i = 2; i < (iListDirsCount + 2); i++)
	{
		pMenu[iMenuNum].Options[i].Show = 1;
		pMenu[iMenuNum].Options[i].PrevPage = 0; //iPrevMenu;
		memset(pMenu[iMenuNum].Options[i].Name,0,64);
		pMenu[iMenuNum].Options[i].NextPage = 0;
		pMenu[iMenuNum].Options[i].ActionFunc = Menu_PlaySelectedDir;
		pMenu[iMenuNum].Options[i].Access = 0;
		len = i - 2;
		memset(pMenu[iMenuNum].Options[i].Name,32,3);
		if (len == iListDirsCurPos) pMenu[iMenuNum].Options[i].Name[0] = '>';
		if (cListDirs[len].Flag) pMenu[iMenuNum].Options[i].Name[1] = 'v';
		
		//memset(cbuff, 0, 2048);
		blen = 61;
		utf8_to_cp866(cListDirs[len].Name, (strlen(cListDirs[len].Name) < blen) ? strlen(cListDirs[len].Name) : 60, &pMenu[iMenuNum].Options[i].Name[3], &blen);
	}
	
	DBG_MUTEX_UNLOCK(&system_mutex);		
	DBG_LOG_OUT();
	return 0;
}

int RefreshPlayListMenu(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);
	
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int i, len;
	unsigned int blen;
	//char cbuff[2048];
	iMenuNum = 23;
	if (pMenu[iMenuNum].Options != NULL) DBG_FREE(pMenu[iMenuNum].Options);
	pMenu[iMenuNum].Options = NULL;
	pMenu[iMenuNum].CountOptions = iListFilesCount + 2;
	if (pMenu[iMenuNum].SelectedOption >= iListFilesCount) pMenu[iMenuNum].SelectedOption = 0;
	pMenu[iMenuNum].Options = (MENU_OPTION*) DBG_MALLOC(pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[iMenuNum].Options, 0, pMenu[iMenuNum].CountOptions*sizeof(MENU_OPTION));
	pMenu[iMenuNum].Options[0].Show = 1;
	pMenu[iMenuNum].Options[0].PrevPage = 0; //iPrevMenu;
	pMenu[iMenuNum].Options[0].NextPage = 0;
	pMenu[iMenuNum].Options[0].ActionFunc = Menu_SwitchPlayMode;
	strcat(pMenu[iMenuNum].Options[0].Name, "Сменить порядок воспроизведения");
	pMenu[iMenuNum].Options[1].Show = 1;
	pMenu[iMenuNum].Options[1].PrevPage = 0; //iPrevMenu;
	pMenu[iMenuNum].Options[1].NextPage = 0;
	pMenu[iMenuNum].Options[1].ActionFunc = Menu_SwitchPlayOrder;
	strcat(pMenu[iMenuNum].Options[1].Name, "Сбросить порядок воспроизведения");
	for (i = 2; i < (iListFilesCount + 2); i++)
	{
		pMenu[iMenuNum].Options[i].Show = 1;
		pMenu[iMenuNum].Options[i].PrevPage = 0; //iPrevMenu;
		memset(pMenu[iMenuNum].Options[i].Name,0,64);
		pMenu[iMenuNum].Options[i].NextPage = 0;
		pMenu[iMenuNum].Options[i].ActionFunc = Menu_PlaySelectedFile;
		pMenu[iMenuNum].Options[i].Access = 0;
		len = i - 2;
		memset(pMenu[iMenuNum].Options[i].Name,32,3);
		if (len == iListFilesCurPos) pMenu[iMenuNum].Options[i].Name[0] = '>';
		if (cListFiles[len].Flag) pMenu[iMenuNum].Options[i].Name[1] = 'v';
		//memset(cbuff, 0, 2048);
		blen = 61;
		utf8_to_cp866(cListFiles[len].Name, (strlen(cListFiles[len].Name) < blen) ? strlen(cListFiles[len].Name) : 60, &pMenu[iMenuNum].Options[i].Name[3], &blen);
	}
	
	DBG_MUTEX_UNLOCK(&system_mutex);		
	DBG_LOG_OUT();
	return 0;
}

int Menu_SwitchPlayDirMode(void* pData, int iNum)
{
	if (cCurRandomDir == 0) cCurRandomDir = 1; else cCurRandomDir = 0;
	return 0;
}

int Menu_SwitchPlayMode(void* pData, int iNum)
{
	if (cCurRandomFile == 0) cCurRandomFile = 1; else cCurRandomFile = 0;
	return 0;
}

int Menu_SwitchPlayOrder(void* pData, int iNum)
{
	DBG_MUTEX_LOCK(&system_mutex);
	int i;
	for (i = 0; i < iListFilesCount; i++) cListFiles[i].Flag = 0;	
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (pScreenMenu) RefreshPlayListMenu(pScreenMenu, 0);
	if (pWebMenu) RefreshPlayListMenu(pWebMenu, 0);
	
	return 1;
}

int Menu_SwitchDirOrder(void* pData, int iNum)
{
	DBG_MUTEX_LOCK(&system_mutex);
	int i;
	for (i = 0; i < iListDirsCount; i++) cListDirs[i].Flag = 0;	
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (pScreenMenu) RefreshDirListMenu(pScreenMenu, 0);
	if (pWebMenu) RefreshDirListMenu(pWebMenu, 0);
	
	return 1;
}

int Menu_PlaySelectedDir(void *pData, int iNum)
{
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&system_mutex);
	iListDirsCurPos = pMenu[24].SelectedOption - 2;
	int ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_CHANGE);
	if (ret == 0) 
	{		
		if (uiShowModeCur >= 2) uiShowModeCur = 1;
		dbgprintf(3, "No files in Menu_PlaySelectedDir '%s'\n", cCurrentFileLocation);		
	}
	else
	{
		if (uiShowModeCur < 2) uiShowModeCur = 2;		
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret)
	{
		SetChangeShowNow(1);
		SetNewShowType(SHOW_TYPE_NEW);
		cPlayDirect = 3;
		pMenu[0].Options[20].Show = 0;
	} else ShowNewMessage("Нет файлов для воспроизведения");
	return 0;
}

int Menu_PlaySelectedFile(void *pData, int iNum)
{
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	DBG_MUTEX_LOCK(&system_mutex);
	iListFilesCurPos = pMenu[23].SelectedOption - 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	SetChangeShowNow(1);
	SetNewShowType(SHOW_TYPE_NEW);
	cPlayDirect = 3;
	pMenu[0].Options[20].Show = 0;
	return 0;
}

int Menu_ResetShower(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	//pMenu[0].Options[1].Show = 0;
	//pMenu[0].Options[0].Show = 1;
	//pMenu[0].SelectedOption = 0;
	
	if (iNum >= 0) CloseMenu(iNum);
	if (!(GetCurShowType() & SHOW_TYPE_ALARM1)) SetNewShowType(SHOW_TYPE_NEW);
	pMenu[0].Options[20].Show = 0;
	if (GetChangeShowNow() == 0) SetChangeShowNow(1);
	DBG_LOG_OUT();
	return 0;
}

int Menu_CloseVideo(void *pData, int iNum)
{
	DBG_LOG_IN();
	if (iNum >= 0) CloseMenu(iNum);
	if (GetChangeShowNow() == 0) 
	{
		SetNewShowType(SHOW_TYPE_CLOSE_VIDEO);	
		SetChangeShowNow(1);
	}
	DBG_LOG_OUT();
	return 0;
}

int Menu_CloseAudio(void *pData, int iNum)
{
	DBG_LOG_IN();
	if (iNum >= 0) CloseMenu(iNum);
	if (GetChangeShowNow() == 0) 
	{
		SetNewShowType(SHOW_TYPE_CLOSE_AUDIO);	
		SetChangeShowNow(1);
	}
	DBG_LOG_OUT();
	return 0;
}

int System_Shutdown(void *pData, int iNum)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);
	cThreadMainStatus = 1;
	unsigned int iID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
	
	dbgprintf(3, "ShutdownSystem\n");
	//system("shutdown -h now");
	dbgprintf(4, "Closing Main thread\n");
	tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);
	return 0;
}

int System_Reboot(void *pData, int iNum)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);	
	cThreadMainStatus = 2;
	unsigned int iID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
	
	dbgprintf(3, "RebootSystem\n");
	//system("reboot");
	dbgprintf(4, "Closing Main thread\n");
	tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);
	return 0;
}

int System_ResetDefault(void *pData, int iNum)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);	
	cThreadMainStatus = 6;
	unsigned int iID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
	
	dbgprintf(3, "ResetSystem\n");
	//system("reboot");
	dbgprintf(4, "Closing Main thread\n");
	tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);
	return 0;
}

int System_Update()
{
	DBG_MUTEX_LOCK(&systemlist_mutex);	
	cThreadMainStatus = 7;
	unsigned int iID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
	
	dbgprintf(3, "UpdateSystem\n");
	//system("reboot");
	dbgprintf(4, "Closing Main thread\n");
	tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);
	return 0;
}

int System_Close(void *pData, int iNum)
{
	dbg_death_signal(SIGKILL);
	return 0;
}

int System_Exit(void *pData, int iNum)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);	
	unsigned int iID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
	
	dbgprintf(3, "ExitToSystem\n");
	dbgprintf(4, "Closing Main thread\n");
	tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);
	return 0;
}

int System_Restart(void *pData, int iNum)
{
	DBG_MUTEX_LOCK(&systemlist_mutex);	
	cThreadMainStatus = 3;
	unsigned int iID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int iSafe = uiStartMode & 0x0004;
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (iSafe == 0) SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
	
	dbgprintf(3, "RestartSystem\n");
	dbgprintf(4, "Closing Main thread\n");
	tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);//exit(1);
	return 0;
}

int System_Exec_App(int iNum, char *pPath)
{
	if (strlen(pPath) < 256)
	{
		if (iNum)
		{
			DBG_MUTEX_LOCK(&systemlist_mutex);
			memset(cExternalApp, 0, 256);
			strcpy(cExternalApp, pPath);
			cThreadMainStatus = 5;
			unsigned int iID = GetLocalSysID();
			DBG_MUTEX_UNLOCK(&systemlist_mutex);	
			SendUDPMessage(TYPE_MESSAGE_DEVICE_STOPED, (char*)&iID, sizeof(iID), NULL, 0, NULL);
		
			dbgprintf(3, "ExitToExternalApp\n");
			dbgprintf(4, "Closing Main thread\n");
			tx_eventer_send_event(&main_thread_evnt, EVENT_STOP);
		}
		else
		{
			char cBuff[256];
			memset(cBuff, 0, 256);
			strcpy(cBuff, pPath);
			if (cBuff[strlen(cBuff) - 1] != 38) strcat(cBuff, " &"); //'&'
			dbgprintf(4, "Execute external app without wait: '%s'\n", cBuff);
			system(cBuff);
		}
	}
	return 0;
}

int SendSetAlarmStatus(int iNum)
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	iAlarmEvents |= iNum;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	//DBG_MUTEX_LOCK(&conn_info_mutex);
	//SendMessage(&Connects_Info[MAX_CONNECTIONS], UDP_TRAFFIC_BUFFER_SIZE, TYPE_MESSAGE_ALARM_SET, (char*)&iNum, sizeof(iNum), NULL, 0, NULL);
	//DBG_MUTEX_UNLOCK(&conn_info_mutex);	
				
	DBG_LOG_OUT();
	return 1;
}

int SendResetAlarmStatus(int iNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&system_mutex);
	iAlarmEvents &= 0xFFFFFFFF ^ iNum;
	DBG_MUTEX_UNLOCK(&system_mutex);
	//DBG_MUTEX_LOCK(&conn_info_mutex);
	//SendMessage(&Connects_Info[MAX_CONNECTIONS], UDP_TRAFFIC_BUFFER_SIZE, TYPE_MESSAGE_ALARM_RESET, (char*)&iNum, sizeof(iNum), NULL, 0, NULL);
	//DBG_MUTEX_UNLOCK(&conn_info_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int ClockAlarmStop(int iVal)
{
	DBG_LOG_IN();
	
	//pMenu[0].Options[4].Show = 1;
	//pMenu[0].Options[5].Show = 0;
	SendResetAlarmStatus(ALARM_TYPE_CLOCK);
	
	//DisconnectFromAllModules(0);
	if (iVal) SetChangeShowNow(1);
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_ClockAlarmStop(void *pData, int iMenuNum)
{
	DBG_LOG_IN();
	ClockAlarmStop(1);
	CloseMenu(iMenuNum);
	DBG_LOG_OUT();
	return 0;
}

/*int Menu_RequestModules(int iNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&modulelist_mutex);
	if (iModuleCnt != 0)
	{
		if (miModuleList != NULL) DBG_FREE(miModuleList);
		miModuleList = NULL;
	}
	iModuleCnt = 0;	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (iNum >= 0)
	{
		int i = pMenu[iNum].Options[pMenu[iNum].SelectedOption].NextPage;
		pMenu[i].SelectedOption = 0;
		if (pMenu[i].Options != NULL) DBG_FREE(pMenu[i].Options);
		pMenu[i].Options = NULL;
		pMenu[i].CountOptions = 0;
	}
	DBG_MUTEX_LOCK(&conn_info_mutex);
	SendUDPMessage(TYPE_MESSAGE_REQUEST_MODULE_LIST, NULL, 0, NULL, 0, NULL);
	DBG_MUTEX_UNLOCK(&conn_info_mutex);
	DBG_LOG_OUT();
	return 0;
}*/

int Menu_CameraAction(void *pData, int iNum)
{
	DBG_LOG_IN();	
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	unsigned int iID = 0;
	if (iNum == 1) iID = pMenu[1].Options[pMenu[1].SelectedOption].MiscData;
	
	unsigned int puiData[4];
	int n, iSelected;
	SetShowMenu(0);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	iSelected = ModuleIdToNum(iID, 2);
	puiData[0] = iID;
	puiData[1] = 0;
	
	if (iSelected >= 0)
	{
		for (n = 0; n != iModuleCnt; n++) 
		{
			if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_MIC)						
				&& (miModuleList[n].Address.sin_addr.s_addr == miModuleList[iSelected].Address.sin_addr.s_addr)
				&& (miModuleList[n].AccessLevel <= iAccessLevelCopy))
				{
					puiData[1] = miModuleList[n].ID;
					break;
				}		
		}
	} else dbgprintf(1, "Error ModuleIdToNum in Menu_CameraAction\n");
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	DBG_MUTEX_LOCK(&system_mutex);
	n = iCameraTimeMaxSet;
	DBG_MUTEX_UNLOCK(&system_mutex);
	SetShowTimeMax(n);	
	
	n = 0;
	char cST = 0;
	
	if (GetChangeShowNow() == 0)
	{
		DBG_MUTEX_LOCK(&system_mutex);
	
		if (uiCurCameraID != puiData[0]) 
		{
			uiNextCameraID = puiData[0];
			cST = SHOW_TYPE_CAMERA;	
			n++;
		}
		if ((puiData[1] != 0) && (uiCurMicID != puiData[1])) 
		{
			uiNextMicID = puiData[1];
			cST |= SHOW_TYPE_MIC_STREAM;
			n++;
		}
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (n) 
		{
			SetNewShowType(cST);
			SetChangeShowNow(1);
		}		
	}
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_AlarmClockSwitch(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected = pMenu[iNum].SelectedOption;
	
	DBG_MUTEX_LOCK(&system_mutex);
	if (iSelected < iAlarmClocksCnt)
	{
		if (actAlarmClockInfo[iSelected].Enabled) 
				actAlarmClockInfo[iSelected].Enabled = 0;
			else 
				actAlarmClockInfo[iSelected].Enabled = 1;
		actAlarmClockInfo[iSelected].Skip = 0;
		memset(pMenu[iNum].Options[iSelected].Name, 0, 64);
		
		sprintf(pMenu[iNum].Options[iSelected].Name, "%s %i %s%s%s%s%s%s%s",
			actAlarmClockInfo[iSelected].Enabled ? "Вкл." : "Выкл.", actAlarmClockInfo[iSelected].Time,
			actAlarmClockInfo[iSelected].Days & 1 ? "[ПН]" : "",
			actAlarmClockInfo[iSelected].Days & 2 ? "[ВТ]" : "",
			actAlarmClockInfo[iSelected].Days & 4 ? "[СР]" : "",
			actAlarmClockInfo[iSelected].Days & 8 ? "[ЧТ]" : "",
			actAlarmClockInfo[iSelected].Days & 16 ? "[ПТ]" : "",
			actAlarmClockInfo[iSelected].Days & 32 ? "[СБ]" : "",
			actAlarmClockInfo[iSelected].Days & 64 ? "[ВС]" : "");
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	DBG_LOG_OUT();
	return 0;
}

int Menu_SystemAction(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected = pMenu[iNum].SelectedOption;
	unsigned int uiID = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	MODULE_INFO *ptMI = ModuleIdToPoint(pMenu[8].Options[pMenu[8].SelectedOption].MiscData, 2);	
	if (ptMI) uiID = ptMI->ID;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	if (uiID != 0)
	{
		switch(iSelected)
		{
			case 0:
				ReqModuleStatus(uiID);
				break;
			case 1:
				SetModuleStatus(uiID, 0, SYSTEM_CMD_EXIT);
				break;
			case 2:
				SetModuleStatus(uiID, 0, SYSTEM_CMD_REBOOT);
				break;
			case 3:
				SetModuleStatus(uiID, 0, SYSTEM_CMD_SHUTDOWN);
				break;
			case 4:
				SetModuleStatus(uiID, 0, SYSTEM_CMD_RESTART);
				break;
			case 5:
				SetModuleStatus(uiID, 0, SYSTEM_CMD_CLOSE);
				break;
			default:
				break;
		}
	} else dbgprintf(2, "Menu_SystemAction: Not Find ID system\n");
	DBG_LOG_OUT();
	return 0;
}

int Menu_GpioAction(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected = pMenu[iNum].SelectedOption;
	int iNextPage = pMenu[iNum].Options[iSelected].NextPage;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	MODULE_INFO *ptMI = ModuleIdToPoint(pMenu[iNum].Options[iSelected].MiscData, 2);	
	if (ptMI)
	{
		if (ptMI->Type == MODULE_TYPE_GPIO)
		{
				if (ptMI->Settings[0] & MODULE_SECSET_OUTPUT)
				{
					pMenu[iNextPage].Options[1].Show = 1;
					//if (ptMI->Settings[2] == 0) 
						pMenu[iNextPage].Options[2].Show = 1;
						//else 
						//pMenu[iNextPage].Options[2].Show = 0;					
				}
				if ((ptMI->Settings[0] & MODULE_SECSET_OUTPUT) == 0)
				{
					pMenu[iNextPage].Options[1].Show = 0;
					pMenu[iNextPage].Options[2].Show = 0;				
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				Menu_GetModuleStatus(pData, iNum);
				DBG_MUTEX_LOCK(&modulelist_mutex);			
		}
		if ((ptMI->Type == MODULE_TYPE_TEMP_SENSOR) ||
			(ptMI->Type == MODULE_TYPE_ADS1015) ||
			(ptMI->Type == MODULE_TYPE_MCP3421) ||
			(ptMI->Type == MODULE_TYPE_AS5600) ||
			(ptMI->Type == MODULE_TYPE_HMC5883L))
		{
			pMenu[iNextPage].Options[1].Show = 0;
			pMenu[iNextPage].Options[2].Show = 0;				
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			Menu_GetModuleStatus(pData, iNum);
			DBG_MUTEX_LOCK(&modulelist_mutex);			
		}
		if (ptMI->Type == MODULE_TYPE_SYSTEM)
		{
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			Menu_GetModuleStatus(pData, iNum);
			DBG_MUTEX_LOCK(&modulelist_mutex);			
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
	}
	DBG_LOG_OUT();
	return 0;
}

int Menu_ActionMenuExec(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected = pMenu[iNum].SelectedOption;
	AddModuleEventInList(amiActionManual[iSelected].ID, amiActionManual[iSelected].SubNumber, amiActionManual[iSelected].Status, NULL, 0, 1);
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_ActionEventSet(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected = pMenu[iNum].SelectedOption;
	if (maActionInfo[iSelected].Enabled) maActionInfo[iSelected].Enabled = 0;
		else maActionInfo[iSelected].Enabled = 1;
	Menu_RefreshActionListMenu(pData, 0);
	
	DBG_LOG_OUT();
	return 0;
}

int OpenRemoteList(unsigned int uiID, struct sockaddr_in *sAddr, unsigned int uiMode)
{
	if (rsiRemoteStatus.FileCnt) DBG_FREE(rsiRemoteStatus.FileList);
	if (rsiRemoteStatus.NewFileCnt) DBG_FREE(rsiRemoteStatus.NewFileList);
	memset(&rsiRemoteStatus, 0, sizeof(rsiRemoteStatus));
	rsiRemoteStatus.ID = uiID;
	memcpy(&rsiRemoteStatus.Address, sAddr, sizeof(rsiRemoteStatus.Address));
	rsiRemoteStatus.FileCnt = 0;
	rsiRemoteStatus.FileList = NULL;
	rsiRemoteStatus.NewFileCnt = 0;
	rsiRemoteStatus.NewFileList = NULL;
	rsiRemoteStatus.Selected = 0;
	rsiRemoteStatus.CurrentShow = 0;
	rsiRemoteStatus.Offset = 0;
	rsiRemoteStatus.Status = 1;
	memset(rsiRemoteStatus.Path, 0, MAX_PATH);
	rsiRemoteStatus.Timer = FILE_VIEW_TIME;
	DBG_MUTEX_UNLOCK(&system_mutex);
	RequestFileList(sAddr, uiMode);
	DBG_MUTEX_LOCK(&system_mutex);
	return 1;
}
						
int Menu_ArchivAction(void *pData, int iNum)
{
	DBG_LOG_IN();
	
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected;
	//iNum = pMenu[25].Options[pMenu[25].SelectedOption].PrevPage;
	//printf("Prev %i\n", iNum);
	iSelected = pMenu[iNum].SelectedOption;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	MODULE_INFO *ptMI = ModuleIdToPoint(pMenu[iNum].Options[iSelected].MiscData, 2);	
	if (ptMI)
	{
		struct sockaddr_in sAddr;
		unsigned int uiID = ptMI->ID;
		memcpy(&sAddr, &ptMI->Address, sizeof(sAddr));	
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		DBG_MUTEX_LOCK(&system_mutex);	
		OpenRemoteList(uiID, &sAddr, 0);
		DBG_MUTEX_UNLOCK(&system_mutex);
		SetShowMenu(0);
	} else DBG_MUTEX_UNLOCK(&modulelist_mutex);
	DBG_LOG_OUT();
	return 1;
}

int Menu_ModuleAction(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	int iSelected;
	unsigned int uiSubModule = 0;
	if (iNum == 13) iNum = pMenu[iNum].Options[pMenu[iNum].SelectedOption].PrevPage;
	iSelected = pMenu[iNum].SelectedOption;
	int iNextPage = pMenu[iNum].Options[iSelected].NextPage;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	MODULE_INFO *ptMI = ModuleIdToPoint(pMenu[iNum].Options[iSelected].MiscData, 2);	
	if (ptMI)
	{
		unsigned int uiID = ptMI->ID;
		switch(ptMI->Type)
		{
			case MODULE_TYPE_GPIO:			
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				Menu_GetModuleStatus(pData, iNum);
				DBG_MUTEX_LOCK(&modulelist_mutex);			
				break;
			case MODULE_TYPE_TEMP_SENSOR:
			case MODULE_TYPE_ADS1015:
			case MODULE_TYPE_MCP3421:
			case MODULE_TYPE_AS5600:
			case MODULE_TYPE_HMC5883L:
			case MODULE_TYPE_TFP625A:
				pMenu[iNextPage].Options[1].Show = 0;
				pMenu[iNextPage].Options[2].Show = 0;	
				break;
			case MODULE_TYPE_MIC:
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				SetShowMenu(0);
				DBG_MUTEX_LOCK(&modulelist_mutex);
				break;
			case MODULE_TYPE_CAMERA:
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				SetShowMenu(0);
				DBG_MUTEX_LOCK(&modulelist_mutex);
				break;
			default:
				break;		
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		
		AddModuleEventInList(uiID, uiSubModule, 0, NULL, 0, 1);
	}
	DBG_LOG_OUT();
	return 0;
}

int ModuleAction(unsigned int iModuleID, int iSubModuleNum, unsigned int iActionCode, char* cActionName)
{
	DBG_LOG_IN();
	MODULE_INFO *pModuleList;
	MODULE_INFO *pModuleList2;
	unsigned int iID;
	unsigned int iAccess;
	unsigned int iSysAccessLevel;
	int i, res, testnum, teststat1, teststat2;
	pModuleList = ModuleIdToPoint(iModuleID, 2);	
	if (pModuleList)
	{
		iID = pModuleList->ID;		
		//if ((iSubModuleNum <= MAX_MODULE_STATUSES) && (iSubModuleNum != 0)) pModuleList->Status[iSubModuleNum-1] = iActionCode;
		//dbgprintf(3, "ModuleAction %.4s, %i, %i, %i, %i\n", (char*)&pModuleList->ID, iSubModuleNum, iActionCode, pModuleList->Type, (unsigned int)cActionName);
		switch (pModuleList->Type)
		{
			case MODULE_TYPE_EXTERNAL:
				if (pModuleList->Local == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);		
					SetModuleStatus(iID, iSubModuleNum, iActionCode);
					ReqModuleStatus(iID);
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				else
				{
					external_set_status_module(pModuleList, iSubModuleNum, iActionCode);
					int uiStats[MAX_MODULE_STATUSES];
					memcpy(uiStats, pModuleList->Status, MAX_MODULE_STATUSES * sizeof(int));
					external_get_statuses_module(pModuleList);
					if (pModuleList->ScanSet == 0)
					{
						int i;
						for (i = 0; i < MAX_MODULE_STATUSES; i++)
						{
							if (pModuleList->Status[i] != uiStats[i])
							{
								teststat1 = pModuleList->Status[i];
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								AddModuleEventInList(iID, 1, teststat1, NULL, 0, 0);
								DBG_MUTEX_LOCK(&modulelist_mutex);
							}
						}
					}
				}
				break;
			case MODULE_TYPE_USB_GPIO:
				if (pModuleList->Local == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);		
					SetModuleStatus(iID, iSubModuleNum, iActionCode);
					ReqModuleStatus(iID);
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				else
				{
					usb_gpio_set_status_module(pModuleList, iSubModuleNum, iActionCode);
					int uiStats[MAX_MODULE_STATUSES];
					memcpy(uiStats, pModuleList->Status, MAX_MODULE_STATUSES * sizeof(int));
					if (usb_gpio_get_status_module(pModuleList, 0))
					{
						if (pModuleList->ScanSet == 0)
						{
							int i;
							for (i = 0; i < MAX_MODULE_STATUSES; i++)
							{
								if (pModuleList->Status[i] != uiStats[i])
								{
									teststat1 = pModuleList->Status[i];
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									AddModuleEventInList(iID, 1, teststat1, NULL, 0, 0);
									DBG_MUTEX_LOCK(&modulelist_mutex);
								}
							}
						}
					}
				}
				break;
			case MODULE_TYPE_GPIO:
				if (pModuleList->Local == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);		
					SetModuleStatus(iID, iSubModuleNum, iActionCode);
					ReqModuleStatus(iID);
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				else
				{
					if (iActionCode == 0) gpio_switch_off_module(pModuleList);
					if (iActionCode == 1) gpio_switch_on_module(pModuleList);
					if (iActionCode == 2) 
					{
						if (gpio_get_status_module(pModuleList) == 0) gpio_switch_on_module(pModuleList);
						else if (gpio_get_status_module(pModuleList) == 1) gpio_switch_off_module(pModuleList);
					}
					teststat1 = pModuleList->Status[0];
					pModuleList->Status[0] = gpio_get_status_module(pModuleList);
					if ((pModuleList->ScanSet == 0) && (pModuleList->Status[0] != teststat1))
					{
						teststat1 = pModuleList->Status[0];
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						AddModuleEventInList(iID, 1, teststat1, NULL, 0, 0);
						DBG_MUTEX_LOCK(&modulelist_mutex);
					}
				}
				break;
			case MODULE_TYPE_SPEAKER:
				if (pModuleList->Local == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					SetModuleStatus(iID, iSubModuleNum, iActionCode);
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				else
				{
					if (iActionCode == 0)
					{
						DBG_MUTEX_UNLOCK(&modulelist_mutex);		
						if (iSubModuleNum != 0)
						{							
							Action_PlayTestSound();
						}
						else
						{
							Audio_Stop(1);
							CloseAllConnects(CONNECT_CLIENT, FLAG_AUDIO_STREAM, 0);
						}
						DBG_MUTEX_LOCK(&modulelist_mutex);
					}
					else
					{
						pModuleList2 = NULL;
						iAccess = 0;
						res = 0;
						for (i = 0; i != iSoundListCnt; i++)
							if (mSoundList[i].ID == iActionCode) {res = 1;break;}	
						if (res == 0)
						{
							pModuleList2 = ModuleIdToPoint(iActionCode, 2);
							if (pModuleList2) iAccess = pModuleList2->AccessLevel;
							 else dbgprintf(2, "Module not found for connect to %.4s\n", (char*)&iActionCode);
						}
						DBG_MUTEX_UNLOCK(&modulelist_mutex);		
						if (res) Action_PlaySound(iActionCode, iSubModuleNum); 
						else 
						{
							if (pModuleList2)
							{
								DBG_MUTEX_LOCK(&system_mutex);	
								iSysAccessLevel = iAccessLevel;
								DBG_MUTEX_UNLOCK(&system_mutex);
								if (iSysAccessLevel >= iAccess)	Action_PlayAudioModule(iActionCode); else dbgprintf(2, "Access denied for connect to %.4s\n", (char*)&iActionCode);
							}
						}
						DBG_MUTEX_LOCK(&modulelist_mutex);		
					}
				}
				break;
			case MODULE_TYPE_DISPLAY:
				if (pModuleList->Local == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);		
					SetModuleStatus(iID, iSubModuleNum, iActionCode);
					//ReqModuleStatus(iID);	
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				else
				{
					int iCamAccept = 0;
					pModuleList2 = ModuleIdToPoint(iActionCode, 2);
					if (pModuleList2)
					{
						iAccess = pModuleList2->AccessLevel;
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						DBG_MUTEX_LOCK(&system_mutex);	
						iSysAccessLevel = iAccessLevel;
						DBG_MUTEX_UNLOCK(&system_mutex);
						if (iSysAccessLevel >= iAccess)	iCamAccept = Action_ShowCameraModule(iActionCode); else dbgprintf(2, "Access denied for connect to %.4s\n", (char*)&iActionCode);				
						DBG_MUTEX_LOCK(&modulelist_mutex);		
					} else dbgprintf(2, "Module not found for connect to %.4s\n", (char*)&iActionCode);
					
					if (iSubModuleNum)
					{
						pModuleList2 = ModuleIdToPoint(iSubModuleNum, 2);
						if (pModuleList2) 
						{
							iAccess = pModuleList2->AccessLevel;
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							DBG_MUTEX_LOCK(&system_mutex);	
							iSysAccessLevel = iAccessLevel;
							DBG_MUTEX_UNLOCK(&system_mutex);
							if (iSysAccessLevel >= iAccess)
							{
								if (Action_PlayAudioModule(iSubModuleNum) && iCamAccept) AddNewShowType(SHOW_TYPE_CAMERA);
							} else dbgprintf(2, "Access denied for connect to %.4s\n", (char*)&iSubModuleNum);
							DBG_MUTEX_LOCK(&modulelist_mutex);	
						} else dbgprintf(2, "Module not found for connect to %.4s\n", (char*)&iSubModuleNum);
					}
				}
				break;
			case MODULE_TYPE_PN532:
				break;
			case MODULE_TYPE_RC522:
				break;				
			case MODULE_TYPE_RTC:
			 	if (iActionCode == 0) i2c_write_timedate3231(I2C_ADDRESS_CLOCK);
				if (iActionCode == 1) i2c_read_timedate3231(I2C_ADDRESS_CLOCK);
				if (iActionCode == 2)
				{
					struct tm timeinfo;
					i2c_read_spec_timedate3231(I2C_ADDRESS_CLOCK, &timeinfo);
					time_t rawtime = mktime(&timeinfo);
					char timebuff[64];
					strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
					dbgprintf(3,"Send module time '%s' to ALL\n", timebuff);
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					unsigned int SysID = 0;
					DBG_MUTEX_LOCK(&system_mutex);
					unsigned char cBcTCP = ucBroadCastTCP;
					DBG_MUTEX_UNLOCK(&system_mutex);
					SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_FORCE_DATETIME,  (char*)&rawtime, sizeof(rawtime), (char*)&SysID, sizeof(SysID));	
					DBG_MUTEX_LOCK(&modulelist_mutex);		
				}
				if (iActionCode == 3)
				{
					time_t rawtime;
					time(&rawtime);	
					struct tm timeinfo;
					localtime_r(&rawtime, &timeinfo);
					char timebuff[64];
					strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
					dbgprintf(3,"Send system time '%s' to ALL\n", timebuff);
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					DBG_MUTEX_LOCK(&systemlist_mutex);
					unsigned int SysID = GetLocalSysID();
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					DBG_MUTEX_LOCK(&system_mutex);
					unsigned char cBcTCP = ucBroadCastTCP;
					DBG_MUTEX_UNLOCK(&system_mutex);
					SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_FORCE_DATETIME,  (char*)&rawtime, sizeof(rawtime), (char*)&SysID, sizeof(SysID));	
					DBG_MUTEX_LOCK(&modulelist_mutex);		
				}
				break;
			case MODULE_TYPE_RADIO:
				break;
			case MODULE_TYPE_TFP625A:
				break;
			case MODULE_TYPE_COUNTER:
				//printf("MODULE_TYPE_COUNTER %i, %i\n", iActionCode, pModuleList->Settings[1]);
				if (pModuleList->Local == 1) 	
				{	
					testnum = 0;
					teststat1 = 0;
					teststat2 = 0;
					if ((iSubModuleNum > 0) && (iSubModuleNum <= MAX_MODULE_STATUSES))
					{	
						testnum = iSubModuleNum;
						teststat1 = pModuleList->Status[iSubModuleNum-1];					
						if (iActionCode == MODULE_COUNTER_RESET) pModuleList->Status[iSubModuleNum-1] = 0;
						if (iActionCode == MODULE_COUNTER_INCREMENT) pModuleList->Status[iSubModuleNum-1]++;	
						if (iActionCode == MODULE_COUNTER_SET) pModuleList->Status[iSubModuleNum-1] = 1;	
						if (iActionCode == MODULE_COUNTER_DECREMENT) pModuleList->Status[iSubModuleNum-1]--;
						if (cActionName)
						{
							if (iActionCode == MODULE_COUNTER_INC_N_VALUE) pModuleList->Status[iSubModuleNum-1] += Str2Int(cActionName);
							if (iActionCode == MODULE_COUNTER_DEC_N_VALUE) pModuleList->Status[iSubModuleNum-1] -= Str2Int(cActionName);
							if (iActionCode == MODULE_COUNTER_SET_N_VALUE) pModuleList->Status[iSubModuleNum-1] = Str2Int(cActionName);						
						}
						teststat2 = pModuleList->Status[iSubModuleNum-1];
					}
					if ((iActionCode == MODULE_COUNTER_SET_VALUE) ||
						(iActionCode == MODULE_COUNTER_SET_INC) ||
						(iActionCode == MODULE_COUNTER_SET_DEC))
						{
							testnum = 1;
							teststat1 = pModuleList->Status[0];
							if (iActionCode == MODULE_COUNTER_SET_VALUE) pModuleList->Status[0] = iSubModuleNum;
							if (iActionCode == MODULE_COUNTER_SET_INC) pModuleList->Status[0] += iSubModuleNum;
							if (iActionCode == MODULE_COUNTER_SET_DEC) pModuleList->Status[0] -= iSubModuleNum;
							teststat2 = pModuleList->Status[0];
						}					
					if ((pModuleList->ScanSet == 0) && (teststat1 != teststat2))
					{
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						AddModuleEventInList(iID, testnum, teststat2, NULL, 0, 0);
						DBG_MUTEX_LOCK(&modulelist_mutex);
					}
				}
				break;
			case MODULE_TYPE_MIC:
				iAccess = pModuleList->AccessLevel;
				DBG_MUTEX_UNLOCK(&modulelist_mutex);		
				DBG_MUTEX_LOCK(&system_mutex);	
				iSysAccessLevel = iAccessLevel;
				DBG_MUTEX_UNLOCK(&system_mutex);
				if (iSubModuleNum == 0)
				{
					if (pModuleList->Local) CloseAllConnects(CONNECT_SERVER, FLAG_AUDIO_STREAM, 0);
				}
				else
				{
					if (iSysAccessLevel >= iAccess)	Action_PlayAudioModule(iID); else dbgprintf(2, "Access denied for connect to %.4s\n", (char*)&iID);
				}
				DBG_MUTEX_LOCK(&modulelist_mutex);
				break;
			case MODULE_TYPE_MEMORY:
				if ((iSubModuleNum > 0) && (iSubModuleNum <= MAX_MODULE_STATUSES))
				{					
					if (pModuleList->Local == 0)
					{
						DBG_MUTEX_UNLOCK(&modulelist_mutex);		
						SetModuleStatus(iID, iSubModuleNum, iActionCode);
						//ReqModuleStatus(iID);	
						DBG_MUTEX_LOCK(&modulelist_mutex);
					}
					else
					{
						teststat1 = pModuleList->Status[iSubModuleNum-1];
						pModuleList->Status[iSubModuleNum-1] = iActionCode;
						if ((pModuleList->ScanSet == 0) && (teststat1 != iActionCode))
						{
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							AddModuleEventInList(iID, iSubModuleNum, iActionCode, NULL, 0, 0);
							DBG_MUTEX_LOCK(&modulelist_mutex);
						}
					}					
					dbgprintf(5, "MODULE_TYPE_MEMORY %i\n", pModuleList->Status[iSubModuleNum-1]);				
				}
				break;
			case MODULE_TYPE_CAMERA:
				if (iSubModuleNum != 0)
				{
					if (pModuleList->Local == 0)
					{
						DBG_MUTEX_UNLOCK(&modulelist_mutex);		
						SetModuleStatus(iID, iSubModuleNum, iActionCode);
						//ReqModuleStatus(iID);	
						DBG_MUTEX_LOCK(&modulelist_mutex);
					}
					else
					{
						if ((iSubModuleNum > 0) && (iSubModuleNum <= MAX_MODULE_STATUSES))
						{
							//pModuleList->Status[iSubModuleNum-1] = iActionCode;
							switch(iSubModuleNum)
							{
								case 3:
									ispCameraImageSettings.Exposure.Mode = GetStandartExposure(iActionCode);
									break;
								case 6:
								case 7:
								case 12:
									pModuleList->Status[iSubModuleNum-1] = iActionCode;
									break;
								case 16:
									if ((iActionCode >= 0) && (iActionCode <= 100))
										ispCameraImageSettings.Brightness = iActionCode;
									break;
								case 20:
									ispCameraImageSettings.ImageFilter = GetStandartImageFilter(iActionCode);
									break;
								case 21:
									ispCameraImageSettings.WhiteBalance.Mode = GetStandartWhiteBalance(iActionCode);
									break;
								case 22:
									if ((iActionCode >= -100) && (iActionCode <= 100))
										ispCameraImageSettings.Contrast = iActionCode;
									break;
								case 23:
									if ((iActionCode >= -100) && (iActionCode <= 100))
										ispCameraImageSettings.Sharpness = iActionCode;
									break;
								case 24:
									if ((iActionCode >= -100) && (iActionCode <= 100))
										ispCameraImageSettings.ColorSaturation = iActionCode;
									break;
								default:
									break;
							}
							ispCameraImageSettings.Updated = 1;									
						}
					}
				}
				if (iSubModuleNum == 0)
				{
					iAccess = pModuleList->AccessLevel;
					int iMicAccess = 0;
					int iCamAccept = 0;
					pModuleList2 = NULL;
					if (iActionCode)
					{
						pModuleList2 = ModuleIdToPoint(iActionCode, 2);
						if (pModuleList2) iMicAccess = pModuleList2->AccessLevel;
						 else dbgprintf(2, "Module not found for connect to %.4s\n", (char*)&iActionCode);
					}
					DBG_MUTEX_UNLOCK(&modulelist_mutex);		
					DBG_MUTEX_LOCK(&system_mutex);	
					iSysAccessLevel = iAccessLevel;
					DBG_MUTEX_UNLOCK(&system_mutex);
					if (iSysAccessLevel >= iAccess)	iCamAccept = Action_ShowCameraModule(iID); else dbgprintf(2, "Access denied for connect to %.4s\n", (char*)&iID);
					if (pModuleList2)
					{
						if (iSysAccessLevel >= iMicAccess)
						{
							if (Action_PlayAudioModule(iActionCode) && iCamAccept) AddNewShowType(SHOW_TYPE_CAMERA);
						} else dbgprintf(2, "Access denied for connect to %.4s\n", (char*)&iActionCode);
					}		
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				break;
			case MODULE_TYPE_RS485:
				if (pModuleList->Local == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					SetModuleStatus(iID, iSubModuleNum, iActionCode);
					DBG_MUTEX_LOCK(&modulelist_mutex);
				}
				else 
				{
					if (pModuleList->SubModule >= 0) put_char(iActionCode, &miModuleList[pModuleList->SubModule]);
				}
				break;	
			case MODULE_TYPE_KEYBOARD:
				break;			
			case MODULE_TYPE_SYSTEM:
				//printf("%i\n", iActionCode);
				//if (iSubModuleNum == 0)
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					switch (iActionCode)
					{
						case SYSTEM_CMD_ACTION_OFF:
							DBG_MUTEX_LOCK(&evntaction_mutex);
							for (i = 0; i != iActionCnt; i++)
								if (maActionInfo[i].ActionID == iSubModuleNum) maActionInfo[i].Enabled = 0;
							DBG_MUTEX_UNLOCK(&evntaction_mutex);	
							break;
						case SYSTEM_CMD_ACTION_ON:
							DBG_MUTEX_LOCK(&evntaction_mutex);
							for (i = 0; i != iActionCnt; i++)
								if (maActionInfo[i].ActionID == iSubModuleNum) maActionInfo[i].Enabled = 1;
							DBG_MUTEX_UNLOCK(&evntaction_mutex);	
							break;
						case SYSTEM_CMD_ACTION_TEMP_OFF:
							DBG_MUTEX_LOCK(&evntaction_mutex);
							for (i = 0; i != iActionCnt; i++)
								if (maActionInfo[i].ActionID == iSubModuleNum) maActionInfo[i].TimerOffStatus = Str2Int(cActionName);
							DBG_MUTEX_UNLOCK(&evntaction_mutex);	
							break;
						case SYSTEM_CMD_DOOR_ALARM_ON:
							DBG_MUTEX_LOCK(&system_mutex);			
							iAlarmEvents |= ALARM_TYPE_DOOR;
							DBG_MUTEX_UNLOCK(&system_mutex);	
							break;
						case SYSTEM_CMD_CLOCK_ALARM_ON:
							DBG_MUTEX_LOCK(&system_mutex);
							iAlarmEvents |= ALARM_TYPE_CLOCK;
							DBG_MUTEX_UNLOCK(&system_mutex);
							break;
						case SYSTEM_CMD_CLOCK_ALARM_OFF:
							DBG_MUTEX_LOCK(&system_mutex);
							iAlarmEvents &= 0xFFFFFFFF ^ ALARM_TYPE_CLOCK;
							DBG_MUTEX_UNLOCK(&system_mutex);
							break;
						case SYSTEM_CMD_MENU_KEY_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_ON, iSubModuleNum);		
							break;
						case SYSTEM_CMD_MENU_KEY_OFF:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_OFF, iSubModuleNum);	
							break;
						case SYSTEM_CMD_MENU_KEY_MENU:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_MENU, iSubModuleNum);	
							break;
						case SYSTEM_CMD_MENU_KEY_LEFT:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_LEFT, iSubModuleNum);		
							break;
						case SYSTEM_CMD_MENU_KEY_RIGHT:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_RIGHT, iSubModuleNum);	
							break;
						case SYSTEM_CMD_MENU_KEY_OK:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_OK, iSubModuleNum);
							break;
						case SYSTEM_CMD_MENU_KEY_BACK:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_BACK, iSubModuleNum);
							break;						
						case SYSTEM_CMD_MENU_KEY_UP:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_UP, iSubModuleNum);		
							break;
						case SYSTEM_CMD_MENU_KEY_DOWN:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_KEY_DOWN, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RADIO_VOLUME_DOWN:
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_VOLUME_DOWN, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RADIO_VOLUME_UP:
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_VOLUME_UP, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RADIO_STATION_NEXT:
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_STATION_NEXT, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RADIO_STATION_PREV:							
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_STATION_PREV, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RADIO_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_ON, iSubModuleNum);
							break;
						case SYSTEM_CMD_RADIO_OFF:
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_OFF, iSubModuleNum);
							break;
						case SYSTEM_CMD_ALARM_VOLUME_DEC:
							add_sys_cmd_in_list(SYSTEM_CMD_ALARM_VOLUME_DEC, iSubModuleNum);
							break;
						case SYSTEM_CMD_ALARM_VOLUME_INC:
							add_sys_cmd_in_list(SYSTEM_CMD_ALARM_VOLUME_INC, iSubModuleNum);
							break;
						case SYSTEM_CMD_ALARM_VOLUME_DOWN:
							add_sys_cmd_in_list(SYSTEM_CMD_ALARM_VOLUME_DOWN, iSubModuleNum);
							break;
						case SYSTEM_CMD_ALARM_VOLUME_UP:
							add_sys_cmd_in_list(SYSTEM_CMD_ALARM_VOLUME_UP, iSubModuleNum);
							break;
						case SYSTEM_CMD_ALARM_VOLUME_SET:
							add_sys_cmd_in_list(SYSTEM_CMD_ALARM_VOLUME_SET, iSubModuleNum);
							break;
						case SYSTEM_CMD_SYS_SOUND_PLAY:
							if (iSubModuleNum == 0)
							{
								Action_PlayTestSound();
							}
							else
							{
								res = 0;
								DBG_MUTEX_LOCK(&modulelist_mutex);
								for (i = 0; i < iSoundListCnt; i++)
									if (mSoundList[i].ID == iSubModuleNum) {res = 1;break;}								
								DBG_MUTEX_UNLOCK(&modulelist_mutex);		
								if (res) Action_PlaySound(iSubModuleNum, 0); 
									else dbgprintf(2, "ModuleAction: not found SYS Sound ID: %.4s\n", (char*)&iSubModuleNum);
							}
							break;						
						case SYSTEM_CMD_RESET_TIMER:
							add_sys_cmd_in_list(SYSTEM_CMD_RESET_TIMER, iSubModuleNum);
							break;
						case SYSTEM_CMD_SOUND_VOLUME_DEC:
							add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_DEC, iSubModuleNum);
							break;
						case SYSTEM_CMD_SOUND_VOLUME_INC:
							add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_INC, iSubModuleNum);
							break;
						case SYSTEM_CMD_SOUND_VOLUME_DOWN:
							add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_DOWN, iSubModuleNum);
							break;
						case SYSTEM_CMD_SOUND_VOLUME_UP:
							add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_UP, iSubModuleNum);
							break;
						case SYSTEM_CMD_SOUND_VOLUME_MUTE:
							add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_MUTE, iSubModuleNum);
							break;
						case SYSTEM_CMD_SOUND_VOLUME_SET:
							if (GetCurShowType() & SHOW_TYPE_OFF)
							{
								if ((iSubModuleNum >= 0) && (iSubModuleNum <= 100))
									audio_set_playback_volume(iSubModuleNum);
							}
							else add_sys_cmd_in_list(SYSTEM_CMD_SOUND_VOLUME_SET, iSubModuleNum);
							break;
						case SYSTEM_CMD_MIC_VOLUME_SET:
							res = GetFirstMicDevNum();
							if ((res != -1) && (iSubModuleNum >= 0) && (iSubModuleNum <= 100))
									audio_set_capture_volume(res, iSubModuleNum);
							break;
						case SYSTEM_CMD_MIC_VOLUME_UP:
							res = GetFirstMicDevNum();
							if (res != -1)
							{
								i = audio_get_capture_volume(res) + 5;
								if (i > 100) i = 100;
								audio_set_capture_volume(res, i);
							}
							break;
						case SYSTEM_CMD_MIC_VOLUME_DOWN:
							res = GetFirstMicDevNum();
							if (res != -1)
							{
								i = audio_get_capture_volume(res) - 5;
								if (i < 0) i = 0;
								audio_set_capture_volume(res, i);
							}
							break;
						case SYSTEM_CMD_MIC_VOLUME_INC:
							res = GetFirstMicDevNum();
							if (res != -1)
							{
								i = audio_get_capture_volume(res) + iSubModuleNum;
								if (i > 100) i = 100;
								audio_set_capture_volume(res, i);
							}
							break;
						case SYSTEM_CMD_MIC_VOLUME_DEC:
							res = GetFirstMicDevNum();
							if (res != -1)
							{
								i = audio_get_capture_volume(res) - iSubModuleNum;
								if (i < 0) i = 0;
								audio_set_capture_volume(res, i);
							}
							break;
						case SYSTEM_CMD_SOFT_VOLUME_SET:
							add_sys_cmd_in_list(SYSTEM_CMD_SOFT_VOLUME_SET, iSubModuleNum);	
							break;
						case SYSTEM_CMD_SOFT_VOLUME_STEP:
							add_sys_cmd_in_list(SYSTEM_CMD_SOFT_VOLUME_STEP, iSubModuleNum);
							break;
						case SYSTEM_CMD_SET_MESSAGE:
							if (cActionName) AddMessageInList(cActionName, strlen(cActionName), 0);	
								else dbgprintf(2, "ModuleAction: error AddMessageInList, Message name NULL\n");
							break;	
						case SYSTEM_CMD_CLEAR_MESSAGE:
							add_sys_cmd_in_list(SYSTEM_CMD_CLEAR_MESSAGE, iSubModuleNum);	
							break;								
						case SYSTEM_CMD_DISPLAY_SET_TEXT:
							if (cActionName) 
							{
								DBG_MUTEX_LOCK(&system_mutex);
								memset(cDisplayMessageText, 0, 128);
								cDisplayMessageChanged = 1;
								if (strlen(cActionName) < 128)
									strcpy(cDisplayMessageText, cActionName);
									else
									memcpy(cDisplayMessageText, cActionName, 127);
								DBG_MUTEX_UNLOCK(&system_mutex);
							}
							else dbgprintf(2, "ModuleAction: error Set display text, Text name NULL\n");
							break;	
						case SYSTEM_CMD_DISPLAY_CLEAR_TEXT:
							DBG_MUTEX_LOCK(&system_mutex);
							memset(cDisplayMessageText, 0, 128);
							cDisplayMessageChanged = 1;
							DBG_MUTEX_UNLOCK(&system_mutex);
							break;								
						case SYSTEM_CMD_SHOW_FULL_NEXT:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_FULL_NEXT, iSubModuleNum);								
							break;
						case SYSTEM_CMD_SHOW_STOP_AV:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_STOP_AV, iSubModuleNum);								
							break;
						case SYSTEM_CMD_SHOW_STOP_VIDEO:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_STOP_VIDEO, iSubModuleNum);								
							break;
						case SYSTEM_CMD_SHOW_STOP_AUDIO:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_STOP_AUDIO, iSubModuleNum);								
							break;							
						case SYSTEM_CMD_CLIENT_STOP:
							dbgprintf(5, "stopped streams SYSTEM_CMD_CLIENT_STOP\n");
							Media_StopPlay(0);
							omx_stop_video(0);
							Audio_Stop(0);	
							CloseAllConnects(CONNECT_CLIENT, 0, 0);													
							break;
						case SYSTEM_CMD_FULL_STOP:
							dbgprintf(5, "stopped streams SYSTEM_CMD_FULL_STOP\n");
							Media_StopPlay(0);
							omx_stop_video(0);
							Audio_Stop(0);	
							CloseAllConnects(CONNECT_CLIENT, 0, 0);	
							CloseAllConnects(CONNECT_SERVER, 0xFFFF, 0);
							DBG_MUTEX_LOCK(&system_mutex);	
							i = 0;
							if (strcmp(cCurrentFileLocation, cFileLocation) != 0)
							{
								memset(cCurrentFileLocation, 0, 256);
								strcpy(cCurrentFileLocation, cFileLocation);
								i = 1;
							}
							DBG_MUTEX_UNLOCK(&system_mutex);
							Menu_PlaySubDirectories(NULL, 0);
							break;
						case SYSTEM_CMD_CAMLIST_CLEAR:
							add_sys_cmd_in_list(SYSTEM_CMD_CAMLIST_CLEAR, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_CAMLIST_ADD:
							add_sys_cmd_in_list(SYSTEM_CMD_CAMLIST_ADD, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_CAMLIST_SHOW:
							add_sys_cmd_in_list(SYSTEM_CMD_CAMLIST_SHOW, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_CAMERA_ERROR:
							add_sys_cmd_in_list(SYSTEM_CMD_CAMERA_ERROR, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_VIDEO_ERROR:
							add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_AUDIO_ERROR:
							add_sys_cmd_in_list(SYSTEM_CMD_AUDIO_ERROR, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_SET_SHOW_MODE:
							add_sys_cmd_in_list(SYSTEM_CMD_SET_SHOW_MODE, iSubModuleNum);						
							break;	
						case SYSTEM_CMD_SET_SHOW_STATUS:
							add_sys_cmd_in_list(SYSTEM_CMD_SET_SHOW_STATUS, iSubModuleNum);						
							break;						
						case SYSTEM_CMD_SET_ZOOM_MODE:
							add_sys_cmd_in_list(SYSTEM_CMD_SET_ZOOM_MODE, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_SHOW_NEXT:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_NEXT, iSubModuleNum);						
							break;	
						case SYSTEM_CMD_SHOW_PREV:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_PREV, iSubModuleNum);
							break;						
						case SYSTEM_CMD_SHOW_PAUSE:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_PAUSE, iSubModuleNum);							
							break;							
						case SYSTEM_CMD_SHOW_PLAY:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_PLAY, iSubModuleNum);
							break;							
						case SYSTEM_CMD_SHOW_INFO:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_INFO, iSubModuleNum);							
							break;
						case SYSTEM_CMD_SHOW_EVENT_TEXT:
							if (cActionName) ShowNewMessage(cActionName);
							break;							
						case SYSTEM_CMD_SHOW_FORWARD:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_FORWARD, iSubModuleNum);							
							break;							
						case SYSTEM_CMD_SHOW_BACKWARD:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_BACKWARD, iSubModuleNum);					
							break;
						case SYSTEM_CMD_PLAY_NEW_POS:
							add_sys_cmd_in_list(SYSTEM_CMD_PLAY_NEW_POS, iSubModuleNum);
							break;	
						case SYSTEM_CMD_MENU_FORWARD:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_FORWARD, iSubModuleNum);									
							break;	
						case SYSTEM_CMD_MENU_BACKWARD:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_BACKWARD, iSubModuleNum);								
							break;							
						case SYSTEM_CMD_SHOW_AGAIN:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_AGAIN, iSubModuleNum);								
							break;
						case SYSTEM_CMD_MENU_NEXT:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_NEXT, iSubModuleNum);								
							break;
						case SYSTEM_CMD_MENU_PREV:
							add_sys_cmd_in_list(SYSTEM_CMD_MENU_PREV, iSubModuleNum);								
							break;						
						case SYSTEM_CMD_LOST:
							break;
						case SYSTEM_CMD_FIND:
							break;
						case SYSTEM_CMD_NEW:
							break;
						case SYSTEM_CMD_SAVE_REC_NORM:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_FULL, 0);
							break;
						case SYSTEM_CMD_SAVE_REC_SLOW:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_SLOW, 0);
							break;
						case SYSTEM_CMD_SAVE_REC_DIFF:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_DIFF, 0);
							break;
						case SYSTEM_CMD_SAVE_REC_AUD:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_AUDIO, 0);
							break;
						case SYSTEM_CMD_SAVE_COPY_REC_NORM:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_FULL, CMD_SAVE_COPY_OFFSET);
							break;
						case SYSTEM_CMD_SAVE_COPY_REC_SLOW:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_SLOW, CMD_SAVE_COPY_OFFSET);							
							break;
						case SYSTEM_CMD_SAVE_COPY_REC_DIFF:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_DIFF, CMD_SAVE_COPY_OFFSET);							
							break;
						case SYSTEM_CMD_SAVE_COPY_REC_AUD:
							Media_NextRecID(iSubModuleNum, CAPTURE_TYPE_AUDIO, CMD_SAVE_COPY_OFFSET);
							break;
						case SYSTEM_CMD_EXEC_EXT_APP_NOW:
							if (cActionName) System_Exec_App(0, cActionName); else dbgprintf(2, "NULL External App Path\n");
							break;
						case SYSTEM_CMD_EXEC_EXT_APP_EXIT:
							if (cActionName) System_Exec_App(1, cActionName); else dbgprintf(2, "NULL External App Path\n");
							break;						
						case SYSTEM_CMD_CLOSE:
							System_Close(NULL, 0);
							break;
						case SYSTEM_CMD_EXIT:
							System_Exit(NULL, 0);
							break;
						case SYSTEM_CMD_REBOOT:
							System_Reboot(NULL, 0);
							break;
						case SYSTEM_CMD_SHUTDOWN:
							System_Shutdown(NULL, 0);
							break;
						case SYSTEM_CMD_RESTART:
							System_Restart(NULL, 0);
							break;	
						case SYSTEM_CMD_APP_UPDATE:
							System_Update();
							break;	
						case SYSTEM_CMD_SET_ACCESS_LEVEL:
							DBG_MUTEX_LOCK(&system_mutex);
							iAccessLevel = iSubModuleNum;
							iAccessTimer = 0;
							get_ms(&iAccessTimer);
							DBG_MUTEX_UNLOCK(&system_mutex);
							DBG_MUTEX_LOCK(&modulelist_mutex);
							i = ModuleTypeToNum(MODULE_TYPE_SYSTEM, 1);
							if (i >= 0) miModuleList[i].Status[1] = iSubModuleNum;	
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							break;	
						case SYSTEM_CMD_SKIP:
							break;						
						case SYSTEM_CMD_TIMERS_INCREASE:
							add_sys_cmd_in_list(SYSTEM_CMD_TIMERS_INCREASE, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_TIMERS_DECREASE:
							add_sys_cmd_in_list(SYSTEM_CMD_TIMERS_DECREASE, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_TIMERS_UPDATE:
							add_sys_cmd_in_list(SYSTEM_CMD_TIMERS_UPDATE, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_PLAY_NEXT_DIR:
							add_sys_cmd_in_list(SYSTEM_CMD_PLAY_NEXT_DIR, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_PLAY_PREV_DIR:
							add_sys_cmd_in_list(SYSTEM_CMD_PLAY_PREV_DIR, iSubModuleNum);						
							break;							
						case SYSTEM_CMD_RANDOM_FILE_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_RANDOM_FILE_ON, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RANDOM_FILE_OFF:
							add_sys_cmd_in_list(SYSTEM_CMD_RANDOM_FILE_OFF, iSubModuleNum);	
							break;						
						case SYSTEM_CMD_RANDOM_DIR_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_RANDOM_DIR_ON, iSubModuleNum);	
							break;
						case SYSTEM_CMD_RANDOM_DIR_OFF:
							add_sys_cmd_in_list(SYSTEM_CMD_RANDOM_DIR_OFF, iSubModuleNum);	
							break;						
						case SYSTEM_CMD_STREAM_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_ON, iSubModuleNum);	
							break;
						case SYSTEM_CMD_STREAM_ON_LAST:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_ON_LAST, iSubModuleNum);	
							break;
						case SYSTEM_CMD_STREAM_ON_NEXT:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_ON_NEXT, iSubModuleNum);	
							break;
						case SYSTEM_CMD_STREAM_ON_PREV:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_ON_PREV, iSubModuleNum);	
							break;							
						case SYSTEM_CMD_STREAM_OFF:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_OFF, iSubModuleNum);	
							break;
						case SYSTEM_CMD_STREAM_TYPE_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_TYPE_ON, iSubModuleNum);	
							break;
						case SYSTEM_CMD_STREAM_TYPE_RND_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_TYPE_RND_ON, iSubModuleNum);	
							break;
						case SYSTEM_CMD_STREAM_RND_ON:
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_RND_ON, iSubModuleNum);	
							break;
						case SYSTEM_CMD_WIDGET_STATUS_OFF:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_STATUS_OFF, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_STATUS_ALLWAYS:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_STATUS_ALLWAYS, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_STATUS_DAYTIME:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_STATUS_DAYTIME, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_STATUS_TIMEOUT:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_STATUS_TIMEOUT, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_STATUS_MENU:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_STATUS_MENU, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_TIMEDOWN:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_TIMEDOWN, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_TIMEUP:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_TIMEUP, iSubModuleNum);
							break;
						case SYSTEM_CMD_WIDGET_UPDATE:
							add_sys_cmd_in_list(SYSTEM_CMD_WIDGET_UPDATE, iSubModuleNum);
							break;
						case SYSTEM_CMD_SHOW_MENU:
							add_sys_cmd_in_list(SYSTEM_CMD_SHOW_MENU, (iSubModuleNum & SYSTEM_CMD_NULL) ? iSubModuleNum ^ SYSTEM_CMD_NULL : iSubModuleNum);
							break;							
						case SYSTEM_CMD_HIDE_MENU:
							add_sys_cmd_in_list(SYSTEM_CMD_HIDE_MENU, iSubModuleNum);
							break;							
						case SYSTEM_CMD_SET_PLAYLIST_POS:
							add_sys_cmd_in_list(SYSTEM_CMD_SET_PLAYLIST_POS, iSubModuleNum);
							break;
						case SYSTEM_CMD_SET_DIRLIST_POS:
							add_sys_cmd_in_list(SYSTEM_CMD_SET_DIRLIST_POS, iSubModuleNum);
							break;							
						case SYSTEM_CMD_PLAY_DIR:
							if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
							{
								DBG_MUTEX_LOCK(&system_mutex);	
								memset(cNextPlayPath, 0, 256);
								memcpy(cNextPlayPath, cActionName, strlen(cActionName));
								smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_PLAY_DIR;
								smiSysMessage[iSysMessageCnt].Data = iSubModuleNum;
								iSysMessageCnt++;
								DBG_MUTEX_UNLOCK(&system_mutex);
							}
							else dbgprintf(2, "ModuleAction: error set cCurrentFile, DIR name NULL\n");
							break;
						case SYSTEM_CMD_PLAY_DIR_RND_FILE:
							if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
							{
								DBG_MUTEX_LOCK(&system_mutex);	
								memset(cNextPlayPath, 0, 256);
								memcpy(cNextPlayPath, cActionName, strlen(cActionName));
								smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_PLAY_DIR_RND_FILE;
								smiSysMessage[iSysMessageCnt].Data = iSubModuleNum;
								iSysMessageCnt++;
								DBG_MUTEX_UNLOCK(&system_mutex);
							}								
							else dbgprintf(2, "ModuleAction: error set cCurrentFile, DIR name NULL\n");
							break;
						case SYSTEM_CMD_SET_SHOW_PATH:
							if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
							{
								DBG_MUTEX_LOCK(&system_mutex);	
								memset(cCurrentFileLocation, 0, 256);
								memcpy(cCurrentFileLocation, cActionName, strlen(cActionName));
								dbgprintf(4, "Set show path: '%s'\n", cActionName);
								smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_SET_SHOW_PATH;
								smiSysMessage[iSysMessageCnt].Data = iSubModuleNum;
								iSysMessageCnt++;
								DBG_MUTEX_UNLOCK(&system_mutex);
							}								
							else dbgprintf(2, "ModuleAction: error set cCurrentFile, DIR name NULL\n");
							break;
						case SYSTEM_CMD_PLAY_FILE:
							if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
								PlayURL(cActionName, iSubModuleNum);
							else dbgprintf(2, "ModuleAction: error set cCurrentFile, DIR name NULL\n");
							break;
						case SYSTEM_CMD_PLAY_YOUTUBE_JWZ:
							if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
								PlayYouTubeFromURL(cActionName);
							else dbgprintf(2, "ModuleAction: error set cCurrentFile, DIR name NULL\n");
							break;							
						case SYSTEM_CMD_PLAY_YOUTUBE_DL:
							if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
								PlayYouTubeFromURL2(cActionName, iSubModuleNum);
							else dbgprintf(2, "ModuleAction: error set cCurrentFile, DIR name NULL\n");
							break;							
						case SYSTEM_CMD_EMAIL:							
							DBG_MUTEX_LOCK(&system_mutex);
							if ((cActionName) && (strlen(cMailAddress) > 0))
							{
								i = 0;
								for (i = 0; i < iMailCnt; i++)
									if (miMailList[i].Group == iSubModuleNum)
									{
										dbgprintf(4, "SendMailFile: Login:'%s'; Pass:'%s'; URL:'%s'; Addr:'%s'; NumList:%i\n", cMailLogin, cMailPassword, cMailServer, cMailAddress,i);	
										SendMailFile(cMailLogin, cMailPassword, cMailServer, cMailAddress, cMailAuth, &miMailList[i]);										
										i++;
									}
								if (i == 0) dbgprintf(2, "ModuleAction: Not found group '%i' for send email\n", iSubModuleNum);
							}
							else dbgprintf(2, "ModuleAction: error SendMail, Message name NULL\n");		
							DBG_MUTEX_UNLOCK(&system_mutex);								
							break;
						case SYSTEM_CMD_SET_STATUS_NUM:
						case SYSTEM_CMD_RESET_STATUS_NUM:
						case SYSTEM_CMD_VALUE_STATUS_NUM:
							if ((pModuleList->Local == 1) && (iSubModuleNum > 0) && (iSubModuleNum <= MAX_MODULE_STATUSES))
							{
								teststat1 = pModuleList->Status[iSubModuleNum-1];
								if (iActionCode == SYSTEM_CMD_SET_STATUS_NUM) pModuleList->Status[iSubModuleNum-1] = 1;
								if (iActionCode == SYSTEM_CMD_VALUE_STATUS_NUM) pModuleList->Status[iSubModuleNum-1] = Str2Int(cActionName);
								if (iActionCode == SYSTEM_CMD_RESET_STATUS_NUM) pModuleList->Status[iSubModuleNum-1] = 0;
								teststat2 = pModuleList->Status[iSubModuleNum-1];
								
								if ((pModuleList->ScanSet == 0) && (teststat1 != teststat2))
								{
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									AddModuleEventInList(iID, iSubModuleNum, teststat2, NULL, 0, 0);
									DBG_MUTEX_LOCK(&modulelist_mutex);
								}
							}
							break;
						case SYSTEM_CMD_RESET_STATUS_MESSAGE:
							if (pModuleList->Local == 1) 	
							{	
								if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
								{
									DBG_MUTEX_UNLOCK(&system_mutex);
									memset(cSysStatus, 0, 64);
									smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_SET_STATUS_MESSAGE;
									smiSysMessage[iSysMessageCnt].Data = iSubModuleNum;
									iSysMessageCnt++;
									DBG_MUTEX_UNLOCK(&system_mutex);
								}	
							}
							break;
						case SYSTEM_CMD_SET_STATUS_MESSAGE:
							if (pModuleList->Local == 1) 	
							{	
								if ((cActionName) && (iSysMessageCnt < MAX_SYS_MESSAGE_LEN))
								{
									DBG_MUTEX_UNLOCK(&system_mutex);
									memset(cSysStatus, 0, 64);
									memcpy(cSysStatus, cActionName, strlen(cActionName));
									smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_SET_STATUS_MESSAGE;
									smiSysMessage[iSysMessageCnt].Data = iSubModuleNum;
									iSysMessageCnt++;
									DBG_MUTEX_UNLOCK(&system_mutex);
								}	
							}
							break;
						default:
							dbgprintf(2, "Unknown Cmd for system:%s, ID:%.4s, ActionCode:%i(%.4s)\n", cActionName, (char*)&iModuleID, iActionCode, (char*)&iActionCode);
							break;							
					}
					DBG_MUTEX_LOCK(&modulelist_mutex);				
				}
				break;	
			default:
				dbgprintf(3, "Unknown Module type for ModuleAction:%s, ID:%.4s, ActionCode:%i\n", cActionName, (char*)&iModuleID, iActionCode);							
				break;
		}
	} else dbgprintf(3, "ID not found for ModuleAction:%s, ID:%.4s\n", cActionName, (char*)&iModuleID);
	DBG_LOG_OUT();
	return 0;
}

void SortModules(MODULE_INFO *pModuleList, int iModCnt)
{
	DBG_LOG_IN();
	
	int i, n;
	int iSortNum = 0;
	int iModNum = -1;
	char cFirstName[65];
	char *cSortTmp = (char*)DBG_MALLOC(iModCnt);
	
	memset(cSortTmp, 0, iModCnt);
	for (i = 0; i < iModCnt; i++) pModuleList[i].Sort = -1;
	for (iSortNum = 0; iSortNum < iModCnt; iSortNum++)
	{
		iModNum = -1;
		memset(cFirstName, 255, 64);
		cFirstName[64] = 0;
		for (i = 0; i < iModCnt; i++)
		{
			if (cSortTmp[i] == 0)
			{	
				for (n = 0; n < 64; n++)
				{
					if (cFirstName[n] > pModuleList[i].Name[n])
					{
						iModNum = i;
						memcpy(cFirstName, pModuleList[i].Name, 64);
						cFirstName[64] = 0;
						break;
					}
					if (cFirstName[n] < pModuleList[i].Name[n]) break;
					if ((cFirstName[n] == 0) && (pModuleList[i].Name[n] == 0))
					{
						iModNum = i;
						break;
					}
				} 
				if (n == 64) iModNum = i;					
			}
			if (iModNum < 0) iModNum = 0;
		}
		pModuleList[iSortNum].Sort = iModNum;
		cSortTmp[iModNum] = 1;
	}
	
	if (cSortTmp) DBG_FREE(cSortTmp);
	
	DBG_LOG_OUT();
}

void SortFiles(LIST_FILES_TYPE *pFileList, int iFileCnt)
{
	DBG_LOG_IN();
	
	int i, n;
	int iSortNum = 0;
	int iFileNum = -1;
	char cFirstName[MAX_FILE_LEN+1];
	char *cSortTmp = (char*)DBG_MALLOC(iFileCnt);
	
	memset(cSortTmp, 0, iFileCnt);
	for (i = 0; i < iFileCnt; i++) pFileList[i].Sort = -1;
	for (iSortNum = 0; iSortNum < iFileCnt; iSortNum++)
	{
		iFileNum = -1;
		memset(cFirstName, 255, MAX_FILE_LEN);
		cFirstName[MAX_FILE_LEN] = 0;
		for (i = 0; i < iFileCnt; i++)
		{
			if (cSortTmp[i] == 0)
			{	
				for (n = 0; n < MAX_FILE_LEN; n++)
				{
					if (cFirstName[n] > pFileList[i].Name[n])
					{
						iFileNum = i;
						memcpy(cFirstName, pFileList[i].Name, MAX_FILE_LEN);
						cFirstName[MAX_FILE_LEN] = 0;
						break;
					}
					if (cFirstName[n] < pFileList[i].Name[n]) break;
					if ((cFirstName[n] == 0) && (pFileList[i].Name[n] == 0))
					{
						iFileNum = i;
						break;
					}
				} 
				if (n == MAX_FILE_LEN) iFileNum = i;					
			}
		}
		pFileList[iSortNum].Sort = iFileNum;
		cSortTmp[iFileNum] = 1;
	}
	if (cSortTmp) DBG_FREE(cSortTmp);
	DBG_LOG_OUT();
}


int GetModuleStatus(unsigned int uiID, unsigned int uiSubNumber)
{
	DBG_LOG_IN();
	
	if (uiID == 0) 
	{
		DBG_LOG_OUT();
		return 0;
	}
	int ret = 0;
	int num;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	num = ModuleIdToNum(uiID, 2);
	if (num >= 0) 
	{
		if ((uiSubNumber > 0) && (uiSubNumber <= MAX_MODULE_STATUSES)) 
				ret = miModuleList[num].Status[uiSubNumber - 1];
	} else ret = 0;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	
	DBG_LOG_OUT();
	return ret;
}

int Menu_GetModuleStatus(void *pData, int iNum)
{
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	unsigned int iID = pMenu[iNum].Options[pMenu[iNum].SelectedOption].PrevPage;
	if (((iID == 0) || (iID == 3)) && (iNum != 8))
		iID = pMenu[0].Options[1].MiscData;	
		else
		iID = pMenu[8].Options[pMenu[8].SelectedOption].MiscData;	
	ReqModuleStatus(iID);
	return 1;
}

int ReqModuleStatus(unsigned int iID)
{
	DBG_LOG_IN();
	struct sockaddr_in addr;	
	struct sockaddr_in *paddr;
	MODULE_INFO	*tMI;
	MODULE_INFO	*tMI_temp;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	tMI = ModuleIdToPoint(iID, 1);
	if (tMI)
	{
		tMI_temp = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
		memcpy(tMI_temp, tMI, sizeof(MODULE_INFO));
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		UpdateModuleStatus(tMI_temp, 1);
		DBG_MUTEX_LOCK(&modulelist_mutex);
		tMI = ModuleIdToPoint(iID, 1);
		if (tMI) memcpy(tMI->Status, tMI_temp->Status, MAX_MODULE_STATUSES*sizeof(tMI_temp->Status[0]));
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		DBG_FREE(tMI_temp);
	}
	else
	{
		paddr = ModuleIdToAddress(iID, 0);
		if (paddr)
		{
			memcpy(&addr, paddr, sizeof(struct sockaddr_in));
			DBG_MUTEX_UNLOCK(&modulelist_mutex);	
			SendTCPMessage(TYPE_MESSAGE_REQUEST_MODULE_STATUS, (char*)&iID, sizeof(unsigned int), NULL, 0, &addr);
		} 
		else 
		{
			DBG_MUTEX_UNLOCK(&modulelist_mutex);				
			dbgprintf(3, "Not find ID module ReqModuleStatus %.4s\n", (char*)&iID);
		}
	}
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_SendGpioModuleOn(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	unsigned int uiID = pMenu[8].Options[pMenu[8].SelectedOption].MiscData;
	AddModuleEventInList(uiID, 1, 1, NULL, 0, 1);
	ReqModuleStatus(uiID);
	
	DBG_LOG_OUT();
	return 0;
}

int Menu_SendGpioModuleOff(void *pData, int iNum)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu = (MENU_PAGE*)pData;
	unsigned int uiID = pMenu[8].Options[pMenu[8].SelectedOption].MiscData;
	AddModuleEventInList(uiID, 1, 0, NULL, 0, 1);
	ReqModuleStatus(uiID);
	
	DBG_LOG_OUT();
	return 0;
}

int SetModuleStatus(unsigned int iID, unsigned int iSubData, unsigned int iStatus)
{
	DBG_LOG_IN();
	
	struct sockaddr_in addr;	
	unsigned int iBuff[3];
	int ret;
	DBG_MUTEX_LOCK(&modulelist_mutex);		
	iBuff[0] = iID;
	iBuff[1] = iSubData;
	iBuff[2] = iStatus;
	MODULE_INFO *ptMI = ModuleIdToPoint(iID, 2);
	if (ptMI == NULL)
	{
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		dbgprintf(1, "Error find module ID:%.4s\n", (char*)iBuff);
		DBG_LOG_OUT();
		return 0;
	}
	ret = ptMI->Local;
	if (ret == 0) memcpy(&addr, &ptMI->Address, sizeof(struct sockaddr_in));	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	if (ret)
		AddModuleEventInList(iBuff[0],iBuff[1], iBuff[2], NULL, 0, 1);
		else
		SendTCPMessage(TYPE_MESSAGE_MODULE_SET, (char*)iBuff, 3*sizeof(unsigned int), NULL, 0, &addr);
	DBG_LOG_OUT();
	return 0;
}

void reset_sys_cmd_list()
{
	DBG_MUTEX_LOCK(&system_mutex);
	memset(smiSysMessage, 0, sizeof(SYS_MESSAGE_INFO) * MAX_SYS_MESSAGE_LEN);
	iSysMessageCnt = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void add_sys_cmd_in_list(unsigned int uiID, unsigned int uiVal)
{
	DBG_MUTEX_LOCK(&system_mutex);
	if (iSysMessageCnt < MAX_SYS_MESSAGE_LEN)
	{
		smiSysMessage[iSysMessageCnt].ID = uiID;
		smiSysMessage[iSysMessageCnt].Data = uiVal;
		iSysMessageCnt++;
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
}

int PlayYouTubeFromURL(char *cURL)
{
	FILE * fmem;
	char *num = (char*)DBG_MALLOC(65536);
	char cmdstr[256];
	
	memset(num, 0, 65536);
	memset(cmdstr, 0, 256);
	sprintf(cmdstr, "perl youtube.pl --no-mux %s", cURL);
	
	int n;
	fmem = popen(cmdstr, "r");
	n = fread(num, 1, 65536, fmem);
	pclose(fmem);
	int i;
	for (i = 0; i < n; i++) if (num[i] < 32) num[i] = 0;
	int ret = 1;
	if (strlen(num) > 0) 
		PlayYouTube(cURL, num); 
	else 
	{
		dbgprintf(2, "PlayYouTubeFromURL: no result for '%s'", cmdstr);
		ret = 0;
	}
	DBG_FREE(num);
	return ret;
}

int PlayYouTubeFromURL2(char *cURL, int iType)
{
	FILE * fmem;
	if ((iType <= 0) || (iType > 400))
	{
		dbgprintf(2, "Unknown type youtube video %i\n", iType);
		return 0;
	}
	char *num = (char*)DBG_MALLOC(65536);
	char cmdstr[256];
	
	memset(num, 0, 65536);
	memset(cmdstr, 0, 256);
	sprintf(cmdstr, "youtube-dl -g -f %i %s", iType, cURL);
	//printf("YT:'%s'\n", cmdstr);
	
	int n;
	fmem = popen(cmdstr, "r");
	n = fread(num, 1, 65536, fmem);
	pclose(fmem);
	int i;
	for (i = 0; i < n; i++) if (num[i] < 32) num[i] = 0;
	//printf("URL:'%s'\n",num);
	int ret = 1;
	if (strlen(num) > 0) 
		PlayYouTube(cURL, num); 
	else 
	{
		dbgprintf(2, "PlayYouTubeFromURL2: no result for '%s'", cmdstr);
		ret = 0;
	}
	DBG_FREE(num);
	return ret;
}

int PlayYouTube(char * cUrl, char * cFullUrl)
{
	DBG_MUTEX_LOCK(&system_mutex);			
	memset(cCurrentFile, 0, 256);
	memcpy(cCurrentFile, cUrl, strlen(cUrl));
	memset(cNextYouTubeURL, 0, 4096);
	memcpy(cNextYouTubeURL, cFullUrl, strlen(cFullUrl));		
	smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_PLAY_YOUTUBE_JWZ;
	smiSysMessage[iSysMessageCnt].Data = 0;
	iSysMessageCnt++;			
	DBG_MUTEX_UNLOCK(&system_mutex);			
	return 1;
}

int PlayURL(char* cUrl, int iNum)
{
	DBG_MUTEX_LOCK(&system_mutex);	
	memset(cNextPlayPath, 0, 256);
	memcpy(cNextPlayPath, cUrl, strlen(cUrl));
	smiSysMessage[iSysMessageCnt].ID = SYSTEM_CMD_PLAY_FILE;
	smiSysMessage[iSysMessageCnt].Data = iNum;
	iSysMessageCnt++;
	DBG_MUTEX_UNLOCK(&system_mutex);
	return 1;
}

void PlayAlarmSound()
{
	dbgprintf(3,"PlayAlarmSound\n");
	unsigned int n;
	unsigned int uiID = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iSoundListCnt; n++)
		if (mSoundList[n].ID == uiDefAlarmSound) {uiID = uiDefAlarmSound;break;}
	if ((uiID == 0) && iSoundListCnt) uiID = mSoundList[0].ID;
	n = uiDefAlarmRepeats;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (uiID) Action_PlaySound(uiID, n); 
	else 
	{
		dbgprintf(1, "Error play alarm, no files and no sounds\n");
		Action_PlayTestSound();
	}
}	

unsigned int OpenServerFile(struct sockaddr_in *Address, unsigned int uiSelected, unsigned int uiSupportStreams, unsigned int cWaitMs)
{
	unsigned int ret = 0;
	unsigned int res = 0;
	unsigned int crashed = 0;
	unsigned int uiData[2];
	uiData[0] = uiSelected;
	uiData[1] = uiSupportStreams;
	SendTCPMessage(TYPE_MESSAGE_GET_PLAY_FILE, (char*)&uiData, sizeof(unsigned int) * 2, NULL, 0, Address);
	{
		do
		{
			DBG_MUTEX_LOCK(&system_mutex);
			ret = rsiRemoteStatus.FileOpened;
			crashed = rsiRemoteStatus.flv_info.Crashed;
			res = (rsiRemoteStatus.flv_info.AudioEnabled << 1) | rsiRemoteStatus.flv_info.VideoEnabled;
			DBG_MUTEX_UNLOCK(&system_mutex);
			if (ret || crashed) break;
			res = 0;
			if (cWaitMs < 100) break;
			cWaitMs -= 100;
			usleep(100000);
		}
		while (cWaitMs);
		DBG_MUTEX_LOCK(&system_mutex);
		if (rsiRemoteStatus.FileOpened == 0) rsiRemoteStatus.flv_info.Crashed = 0;
		DBG_MUTEX_UNLOCK(&system_mutex);
		/*printf("OpenServerFile %i %i %i %i %i\n", rsiRemoteStatus.flv_info.AudioEnabled, rsiRemoteStatus.flv_info.VideoEnabled,
														rsiRemoteStatus.flv_info.AudioStream,
														rsiRemoteStatus.flv_info.VideoStream, rsiRemoteStatus.flv_info.Crashed);
			
		printf("#### %i %i %i %i\n", cWaitMs, res, uiSupportStreams, ret);*/
	}
	return res;
}
						

void CloseServerFile(unsigned int cWaitMs)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&system_mutex);	
	if (rsiRemoteStatus.Status > 1)
	{
		rsiRemoteStatus.Status = 1;
		rsiRemoteStatus.Direct = 21;
	}
	
	if (rsiRemoteStatus.FileOpened)	
	{
		struct sockaddr_in sFileAddr;					
		memcpy(&sFileAddr, &rsiRemoteStatus.Address, sizeof(struct sockaddr_in));
		DBG_MUTEX_UNLOCK(&system_mutex);
		CloseRemoteFile(&sFileAddr);
		while (cWaitMs)
		{
			DBG_MUTEX_LOCK(&system_mutex);
			ret = rsiRemoteStatus.flv_info.Status;
			if (ret == 0) rsiRemoteStatus.FileOpened = 0;
			DBG_MUTEX_UNLOCK(&system_mutex);
			if (ret == 0) break;
			if (cWaitMs < 100) break;
			cWaitMs -= 100;
			usleep(100000);
		}
	} else DBG_MUTEX_UNLOCK(&system_mutex);
}
										

int ReleaseMenu(MENU_PAGE *pMenu)
{
	DBG_LOG_IN();
	int n;
	for (n = 0; n < MAX_MENU_PAGES; n++) if (pMenu[n].Options != NULL) DBG_FREE(pMenu[n].Options);
	DBG_FREE(pMenu);
	DBG_LOG_OUT();
	return 1;
}

int WEB_get_module_info(char* cStr, int iLen)
{
	memset(cStr, 0, iLen);
	
	if (pWebMenu[iCurrentWebMenuPage].ShowInfo == 0)
	{
		snprintf(cStr, iLen, "%s", pWebMenu[iCurrentWebMenuPage].Name);
	}
	else
	{			
		DBG_MUTEX_LOCK(&modulelist_mutex);	
		int iSelected = pWebMenu[iCurrentWebMenuPage].Options[pWebMenu[iCurrentWebMenuPage].SelectedOption].PrevPage;
		if (((iSelected == 0) || (iSelected == 3)) && (iCurrentWebMenuPage != 8))
			iSelected = ModuleIdToNum(pWebMenu[0].Options[1].MiscData, 2);
			else 
			iSelected = ModuleIdToNum(pWebMenu[8].Options[pWebMenu[8].SelectedOption].MiscData, 2);
		if (iSelected != -1)
		{
			switch(miModuleList[iSelected].Type)
			{
				case MODULE_TYPE_CAMERA:
					snprintf(cStr, iLen, "%s(Свет:%i  Движение(размер):%i(%i) Экспозиция:%s)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0], 
															miModuleList[iSelected].Status[1],
															miModuleList[iSelected].Status[3],
															GetNameExposure(miModuleList[iSelected].Status[2]));
					break;
				case MODULE_TYPE_EXTERNAL:
				case MODULE_TYPE_USB_GPIO:
					snprintf(cStr, iLen, "%s(%i, %i, %i)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0],
														miModuleList[iSelected].Status[1],
														miModuleList[iSelected].Status[2]);
					break;
				case MODULE_TYPE_GPIO:
					snprintf(cStr, iLen, "%s(%i)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0]);
					break;
				case MODULE_TYPE_TEMP_SENSOR:
					if (miModuleList[iSelected].Status[0] == 10000)
						snprintf(cStr, iLen, "%s(Температура:N/A)", miModuleList[iSelected].Name);
					else
					{
						if (miModuleList[iSelected].Settings[0] == I2C_ADDRESS_AM2320)
							snprintf(cStr, iLen, "%s(Температура:%.1f`C  Влажность:%i)", 
								miModuleList[iSelected].Name, (float)miModuleList[iSelected].Status[0]/10, miModuleList[iSelected].Status[1]/10);
						if (miModuleList[iSelected].Settings[0] == I2C_ADDRESS_LM75)
							snprintf(cStr, iLen, "%s(Температура:%.1f`C)", 
								miModuleList[iSelected].Name, (float)miModuleList[iSelected].Status[0]/10);
					}
					break;
				case MODULE_TYPE_SYSTEM:
					if (miModuleList[iSelected].Status[0] == 10000)
						snprintf(cStr, iLen, "%s(Температура:N/A)", miModuleList[iSelected].Name);
					else
					{
						snprintf(cStr, iLen, "%s(Температура:%.1f`C)", 
								miModuleList[iSelected].Name, (float)miModuleList[iSelected].Status[0]/10);
					}
					break;
				case MODULE_TYPE_MIC:
					snprintf(cStr, iLen, "%s(Звук:%i)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0]);
					break;
				case MODULE_TYPE_ADS1015:
				case MODULE_TYPE_MCP3421:
					snprintf(cStr, iLen, "%s(Значение:%i)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0]);
					break;
				case MODULE_TYPE_AS5600:
					snprintf(cStr, iLen, "%s(Поворот RAW:%i, Поворот:%i)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0], miModuleList[iSelected].Status[1]);
					break;
				case MODULE_TYPE_HMC5883L:
					snprintf(cStr, iLen, "%s(X:%i, Y:%i, Z:%i)", 
						miModuleList[iSelected].Name, miModuleList[iSelected].Status[0], 
						miModuleList[iSelected].Status[1], miModuleList[iSelected].Status[2]);
					break;
				default:
					snprintf(cStr, iLen, "%s", pWebMenu[8].Name);
					break;					
			}				
		} else snprintf(cStr, iLen, "%s", pWebMenu[8].Name);
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
	}
	return 1;
}

int RenderRemoteFileMenu(GL_STATE_T *state, unsigned int iKeyCode, float fMSize)
{
	if (rsiRemoteStatus.FileCnt == 0) return 0;
	DBG_LOG_IN();
	int ret = 0;
	int i;
	
	if (rsiRemoteStatus.Selected == -1) rsiRemoteStatus.Selected = rsiRemoteStatus.FileCnt - 1;
	if (rsiRemoteStatus.CurrentShow == -1) rsiRemoteStatus.CurrentShow = rsiRemoteStatus.Selected;
	
	if (rsiRemoteStatus.Direct == 0)
	{
		if (iKeyCode == SYSTEM_CMD_MENU_KEY_BACK)
		{
			if (rsiRemoteStatus.Status >= 2) 
			{
				rsiRemoteStatus.Status = 1;
				rsiRemoteStatus.Direct = 21;
			}
			else
			{
				if (rsiRemoteStatus.PlayStatus) 
					rsiRemoteStatus.Status = 3;
					else 
					rsiRemoteStatus.Status = 2;
				rsiRemoteStatus.Direct = 1;
			}
		}
		if (rsiRemoteStatus.Status == 1)
		{
			if ((iKeyCode == SYSTEM_CMD_MENU_KEY_UP) && (rsiRemoteStatus.Selected > 0)) rsiRemoteStatus.Selected--;
			if ((iKeyCode == SYSTEM_CMD_MENU_KEY_DOWN) && (rsiRemoteStatus.Selected < (rsiRemoteStatus.FileCnt - 1))) rsiRemoteStatus.Selected++;
			if ((rsiRemoteStatus.Offset == 0) && (rsiRemoteStatus.Selected == rsiRemoteStatus.CurrentShow))
			{
				if (iKeyCode == SYSTEM_CMD_MENU_KEY_LEFT)
				{
					struct sockaddr_in sAddr;
					memcpy(&sAddr, &rsiRemoteStatus.Address, sizeof(rsiRemoteStatus.Address));
					DBG_MUTEX_UNLOCK(&system_mutex);	
					PrevRemoteFile(&sAddr);
					DBG_MUTEX_LOCK(&system_mutex);
				}
				
				if ((iKeyCode == SYSTEM_CMD_MENU_KEY_RIGHT) && (rsiRemoteStatus.FileList[rsiRemoteStatus.Selected].Type == 1))
				{
					struct sockaddr_in sAddr;
					memcpy(&sAddr, &rsiRemoteStatus.Address, sizeof(rsiRemoteStatus.Address));
					i = rsiRemoteStatus.Selected;
					DBG_MUTEX_UNLOCK(&system_mutex);	
					//printf(">>>> %s \n", inet_ntoa(sAddr.sin_addr));
					
					NextRemoteFile(&sAddr, i);
					DBG_MUTEX_LOCK(&system_mutex);				
				}
				
				if ((iKeyCode == SYSTEM_CMD_MENU_KEY_OK) && (rsiRemoteStatus.FileList[rsiRemoteStatus.Selected].Type == 0))
				{
					ret = 1;
					//PlayRemoteFile(&rsiRemoteStatus.Address, rsiRemoteStatus.Selected, 1);
					rsiRemoteStatus.Direct = 1;
				}
			}
		}
		if (rsiRemoteStatus.Status >= 2)
		{
			if ((iKeyCode == SYSTEM_CMD_MENU_KEY_LEFT) && (rsiRemoteStatus.NewPos > 0)) 
				rsiRemoteStatus.NewPos = ((rsiRemoteStatus.NewPos - 50) < 0) ? 0 : rsiRemoteStatus.NewPos - 50;
			if ((iKeyCode == SYSTEM_CMD_MENU_KEY_RIGHT) && (rsiRemoteStatus.NewPos < 995)) 
				rsiRemoteStatus.NewPos = ((rsiRemoteStatus.NewPos + 50) > 995) ? 995 : rsiRemoteStatus.NewPos + 50;		
			if (iKeyCode == SYSTEM_CMD_MENU_KEY_UP)
			{
				if (rsiRemoteStatus.PlayStatus == 1) omx_player_speedup(); else iKeyCode = SYSTEM_CMD_SHOW_PLAY;
			}
			if (iKeyCode == SYSTEM_CMD_MENU_KEY_DOWN) 
			{
				i = omx_player_get_speed();
				if (i > 1) omx_player_speeddown(); else iKeyCode = SYSTEM_CMD_SHOW_PAUSE;
			}
			if (iKeyCode == SYSTEM_CMD_SHOW_STOP_AV) 
			{
				DBG_MUTEX_UNLOCK(&system_mutex);
				Audio_Stop(0);				
				omx_stop_video(0);
				CloseServerFile(5000);
				Audio_Stop(1);				
				omx_stop_video(1);
				DBG_MUTEX_LOCK(&system_mutex);
			}
			if (iKeyCode == SYSTEM_CMD_SHOW_PAUSE) 
			{
				Audio_Pause();
				omx_player_pause();
				rsiRemoteStatus.PlayStatus = 0;
				rsiRemoteStatus.Status = 2;
				rsiRemoteStatus.Timer = FILE_SHOW_TIME;
			}
			if (iKeyCode == SYSTEM_CMD_SHOW_PLAY) 
			{
				omx_player_play();
				Audio_Play();
				rsiRemoteStatus.PlayStatus = 1;
				rsiRemoteStatus.Status = 3;
				rsiRemoteStatus.Timer = FILE_SHOW_TIME;
			}
			if (iKeyCode == SYSTEM_CMD_MENU_KEY_OK) 
			{
				struct sockaddr_in sAddr;
				memcpy(&sAddr, &rsiRemoteStatus.Address, sizeof(rsiRemoteStatus.Address));
				i = rsiRemoteStatus.NewPos;
				DBG_MUTEX_UNLOCK(&system_mutex);
				ChangeRemoteFilePos(&sAddr, i);	
				DBG_MUTEX_LOCK(&system_mutex);							
			}
		}
	}
	
	if (rsiRemoteStatus.Selected < rsiRemoteStatus.CurrentShow) 
	{
		rsiRemoteStatus.Offset -= (rsiRemoteStatus.CurrentShow - rsiRemoteStatus.Selected) * 2;
		if (rsiRemoteStatus.Offset <= -30)
		{			
			rsiRemoteStatus.CurrentShow--;
			if (rsiRemoteStatus.Selected == rsiRemoteStatus.CurrentShow) rsiRemoteStatus.Offset = 0; else rsiRemoteStatus.Offset += 30;
		}
	}
	if (rsiRemoteStatus.Selected > rsiRemoteStatus.CurrentShow) 
	{
		rsiRemoteStatus.Offset += (rsiRemoteStatus.Selected - rsiRemoteStatus.CurrentShow) * 2;
		if (rsiRemoteStatus.Offset >= 30)
		{			
			rsiRemoteStatus.CurrentShow++;	
			if (rsiRemoteStatus.Selected == rsiRemoteStatus.CurrentShow) rsiRemoteStatus.Offset = 0; else rsiRemoteStatus.Offset -= 30;			
		}
	}
	
	int n, k;
	int iX = 0;
	int iY = 0;
	
	if ((rsiRemoteStatus.Status >= 2) && (rsiRemoteStatus.Direct == 0)) iX = -rsiRemoteStatus.AnimXPos;
	if (rsiRemoteStatus.Direct == 1)
	{
		rsiRemoteStatus.Direct = 10;
		rsiRemoteStatus.AnimXPos = 0;
		rsiRemoteStatus.AnimYPos = 0;
		rsiRemoteStatus.FileLen = 0;
		for (n = 0; n < rsiRemoteStatus.FileCnt; n++) 
		{
			k = strlen(rsiRemoteStatus.FileList[n].ShowName);
			if (k > rsiRemoteStatus.FileLen) rsiRemoteStatus.FileLen = k;
		}
		rsiRemoteStatus.FileLen *= 20;		
	}
	if (rsiRemoteStatus.Direct == 10)
	{
		rsiRemoteStatus.AnimXPos += 15;
		if (rsiRemoteStatus.AnimXPos > rsiRemoteStatus.FileLen)
		{
			if (rsiRemoteStatus.Status >= 2) 
			{
				rsiRemoteStatus.Direct = 0;
			}
			else
			{
				if (rsiRemoteStatus.NewFileCnt)
				{
					rsiRemoteStatus.Direct = 11;
					rsiRemoteStatus.AnimXPos = 0;
					rsiRemoteStatus.AnimYPos = state->menu_height / 2;
					if (rsiRemoteStatus.FileCnt) DBG_FREE(rsiRemoteStatus.FileList);
					rsiRemoteStatus.FileCnt = rsiRemoteStatus.NewFileCnt;
					rsiRemoteStatus.FileList = rsiRemoteStatus.NewFileList;
					rsiRemoteStatus.NewFileCnt = 0;
					rsiRemoteStatus.NewFileList = NULL;
					rsiRemoteStatus.Offset = 0;
					rsiRemoteStatus.Current = 0;
					rsiRemoteStatus.Selected = (int)floor((double)rsiRemoteStatus.FileCnt / 2);
					rsiRemoteStatus.CurrentShow = rsiRemoteStatus.Selected;
				}
				else rsiRemoteStatus.Direct = 21;
			}			
		}
		iX = -rsiRemoteStatus.AnimXPos;
	}
	if (rsiRemoteStatus.Direct == 11)
	{
		rsiRemoteStatus.AnimYPos -= 45;
		if (rsiRemoteStatus.AnimYPos < 3)
		{
			rsiRemoteStatus.AnimXPos = 0;
			rsiRemoteStatus.AnimYPos = 0;
			rsiRemoteStatus.Direct = 0;
		}
	}
	
	if (rsiRemoteStatus.Direct == 2)
	{
		rsiRemoteStatus.Direct = 20;
		rsiRemoteStatus.AnimXPos = 0;
		rsiRemoteStatus.AnimYPos = 0;
	}
	if (rsiRemoteStatus.Direct == 20)
	{
		rsiRemoteStatus.AnimYPos += 45;
		if (rsiRemoteStatus.AnimYPos > ((state->menu_height / 2) - 3))
		{
			if (rsiRemoteStatus.FileCnt) DBG_FREE(rsiRemoteStatus.FileList);
			rsiRemoteStatus.FileCnt = rsiRemoteStatus.NewFileCnt;
			rsiRemoteStatus.FileList = rsiRemoteStatus.NewFileList;
			rsiRemoteStatus.NewFileCnt = 0;
			rsiRemoteStatus.NewFileList = NULL;	
			rsiRemoteStatus.FileLen = 0;
			for (n = 0; n < rsiRemoteStatus.FileCnt; n++) 
			{
				k = strlen(rsiRemoteStatus.FileList[n].ShowName);
				if (k > rsiRemoteStatus.FileLen) rsiRemoteStatus.FileLen = k;
			}
			rsiRemoteStatus.FileLen *= 20;
			rsiRemoteStatus.Direct = 21;
			rsiRemoteStatus.AnimYPos = 0;
			rsiRemoteStatus.AnimXPos = rsiRemoteStatus.FileLen;
			rsiRemoteStatus.Offset = 0;
			rsiRemoteStatus.Current = 0;	
			rsiRemoteStatus.Selected = (int)floor((double)rsiRemoteStatus.FileCnt / 2);
			rsiRemoteStatus.CurrentShow = rsiRemoteStatus.Selected;	
		}
	}
	if (rsiRemoteStatus.Direct == 21)
	{
		rsiRemoteStatus.AnimXPos -= 15;
		if (rsiRemoteStatus.AnimXPos < 3)
		{
			rsiRemoteStatus.Direct = 0;
			rsiRemoteStatus.AnimXPos = 0;
			rsiRemoteStatus.AnimYPos = 0;
		}
		iX = -rsiRemoteStatus.AnimXPos;
	}
		
		
	iY = (state->menu_height / 2) + (20*fMSize);	
	if (rsiRemoteStatus.Direct == 0) iY += rsiRemoteStatus.Offset;
	
	if ((rsiRemoteStatus.Direct == 11) || (rsiRemoteStatus.Direct == 20)) iY += rsiRemoteStatus.AnimYPos;
	
	for (i = rsiRemoteStatus.CurrentShow; i >= 0; i--)
	{
		if (i == rsiRemoteStatus.Selected) 
			RenderText(state, 40*fMSize, iX, iY, 0, rsiRemoteStatus.FileList[i].ShowName);
			else
			RenderText(state, 30*fMSize, iX, iY, 0, rsiRemoteStatus.FileList[i].ShowName);
		iY += 30;
		if (iY > (state->menu_height-35)) break;
	}
//	printf("x=%i\n", iX);
	iY = (state->menu_height / 2) + (20*fMSize) - 30;
	if ((rsiRemoteStatus.Direct == 11) || (rsiRemoteStatus.Direct == 20)) iY -= rsiRemoteStatus.AnimYPos;
	if (rsiRemoteStatus.Direct == 0) iY += rsiRemoteStatus.Offset;
	
	for (i = rsiRemoteStatus.CurrentShow + 1; i < rsiRemoteStatus.FileCnt; i++)
	{
		if (i == rsiRemoteStatus.Selected)
			RenderText(state, 40*fMSize, iX, iY, 0, rsiRemoteStatus.FileList[i].ShowName);			
			else
			RenderText(state, 30*fMSize, iX, iY, 0, rsiRemoteStatus.FileList[i].ShowName);
		
		iY -= 30;		
		if (iY < 100) break;			
	}
			
	DBG_LOG_OUT();
	return ret;
}

int RenderRemoteFileStatus(GL_STATE_T *state, float fMSize)
{
	if ((rsiRemoteStatus.Status >= 2) || rsiRemoteStatus.FileOpened)
	{
		RenderText(state, 30*fMSize, 30*fMSize, 60, 0, rsiRemoteStatus.flv_info.ShowPath);
		RenderText(state, 30*fMSize, 30*fMSize, 30, 0, rsiRemoteStatus.flv_info.Name);
		if (rsiRemoteStatus.PlayStatus == 1) 
			RenderText(state, 40*fMSize, 0, 0, 0, ">");
		if (rsiRemoteStatus.PlayStatus == 0) 
			RenderText(state, 30*fMSize, 0, 0, 0, "||");
		if (rsiRemoteStatus.PlayStatus == 3) 
			RenderText(state, 40*fMSize, 0, 0, 0, "=");
		RenderGraph(state, 50*fMSize, 							30.0f*fMSize, 	515.0f*fMSize, 					3.0f*fMSize);
		RenderGraph(state, 50*fMSize, 							0.0f, 			515.0f*fMSize, 					3.0f*fMSize);
		RenderGraph(state, 50*fMSize, 							0.0f, 			5.0f*fMSize, 					30.0f*fMSize);
		RenderGraph(state, 560*fMSize, 							0.0f, 			5.0f*fMSize, 					30.0f*fMSize);
		RenderGraph(state, (50 + (float)rsiRemoteStatus.NewPos/2)*fMSize, 		
																5.0f*fMSize, 	10.0f*fMSize, 					23.0f*fMSize);
		RenderGraph(state, 55*fMSize, 							11.0f*fMSize, 	
													(float)rsiRemoteStatus.flv_info.CurrentScrollFrame/rsiRemoteStatus.flv_info.FrameWeight/2*fMSize, 	
																												10.0f*fMSize);
		//DBG_MUTEX_LOCK(&system_mutex);		
		//DBG_MUTEX_UNLOCK(&system_mutex);
		RenderText(state, 30*fMSize, 600*fMSize, 0, 0, "%s %if/s f%i", rsiRemoteStatus.flv_info.Crashed ? "~" : "", omx_player_get_speed(), rsiRemoteStatus.Timer);
	}
	return 1;
}

int RenderMenu(MENU_PAGE *pMenu, GL_STATE_T *state, char cKeyStatus, unsigned int iKeyCode, float fMSize)
{
	DBG_LOG_IN();
	//if ((cShowMenu != 0) && (pMenu[iCurrentMenuPage].CountOptions == 0)) RenderText(state, 42, 220, state->menu_height - 40,"%s (пустое)", pMenu[iCurrentMenuPage].Name);
	int n, i, k;
	if (GetShowMenu() != 0)
	{		
		k = pMenu[iCurrentMenuPage].SelectedOption;
		while ((pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].Show != 1) 
			|| (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ViewLevel > iAccessLevelCopy))
		{
			pMenu[iCurrentMenuPage].SelectedOption--;		
			if (pMenu[iCurrentMenuPage].SelectedOption == pMenu[iCurrentMenuPage].CountOptions) pMenu[iCurrentMenuPage].SelectedOption = 0;		
			if (k == pMenu[iCurrentMenuPage].SelectedOption) 
			{
				SetShowMenu(0);
				break;
			}
		}
	}
	if (GetShowMenu() != 0)
	{		
		if (pMenu[iCurrentMenuPage].CountOptions == 0) iCurrentMenuPage = 0;
		int iX = 0; //state->menu_width;
		int iY = state->menu_height;
		int iLines = (state->menu_height / (40*fMSize)) - 2;
		if (!iLines) iLines = 1;
		int iColumns = (state->menu_width / (40*fMSize)) / 32;
		if (!iColumns) iColumns = 1;
		int iTotalLines = iLines * iColumns;
		
		iY -= 40*fMSize;
		iX += 120*fMSize;
		
		if ((iKeyCode == SYSTEM_CMD_MENU_KEY_OFF) || (iKeyCode == SYSTEM_CMD_MENU_KEY_ON))
		{
			if ((cKeyStatus == 1) || (iKeyCode == SYSTEM_CMD_NULL))
			{
				cMenuForwClk++;
				if (cMenuForwClk == MENU_FORW_SPEED)				
				{
					cKeyDownClock++;
					cMenuForwClk = 0;
				}
			}
			else
			{
				cMenuBackClk++;
				if (cMenuBackClk == MENU_BACK_SPEED)
				{
					cKeyUpClock++;
					cMenuBackClk = 0;
				}
			}
			if (cKeyUpClock == 22) cKeyUpClock = 21;
			if (cKeyDownClock == 17) cKeyDownClock = 16;
		}
		else
		{
			if (iKeyCode == SYSTEM_CMD_NULL)
			{
				cMenuBackClk++;
				if (cMenuBackClk >= MENU_IR_SPEED)
				{
					cMenuBackClk = 0;
					SetShowMenu(0);
				}
			} else cMenuBackClk = 0;
		}
		if (pMenu[iCurrentMenuPage].CountOptions != 0)
		{
			k = pMenu[iCurrentMenuPage].SelectedOption;
			while ((pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].Show != 1) 
				|| (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ViewLevel > iAccessLevelCopy))
			{
				pMenu[iCurrentMenuPage].SelectedOption++;		
				if (pMenu[iCurrentMenuPage].SelectedOption == pMenu[iCurrentMenuPage].CountOptions) pMenu[iCurrentMenuPage].SelectedOption = 0;		
				if (k == pMenu[iCurrentMenuPage].SelectedOption) 
				{
					SetShowMenu(0);
					break;
				}
			} 			
		}
		if ((iKeyCode == SYSTEM_CMD_MENU_KEY_DOWN) || ((cKeyStatus == 0) && (cKeyDownClock != 0) && (cKeyDownClock < 16)))
		{
			//if ((cKeyMem == 0) && 
			if ((pMenu[iCurrentMenuPage].CountOptions != 0))
			{
				k = pMenu[iCurrentMenuPage].SelectedOption;
				do
				{			
					pMenu[iCurrentMenuPage].SelectedOption++;		
					if (pMenu[iCurrentMenuPage].SelectedOption == pMenu[iCurrentMenuPage].CountOptions)	pMenu[iCurrentMenuPage].SelectedOption = 0;
					if (k == pMenu[iCurrentMenuPage].SelectedOption) 
					{
						//SetShowMenu(0);
						break;
					}
				} while ((pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].Show != 1)
						|| (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ViewLevel > iAccessLevelCopy));
			}
			cKeyMem = 0;		
			cKeyUpClock = 0;
			cKeyDownClock = 0;
		}
		if (iKeyCode == SYSTEM_CMD_MENU_KEY_UP)
		{
			//if ((cKeyMem == 0) && 
			if ((pMenu[iCurrentMenuPage].CountOptions != 0))
			{
				k = pMenu[iCurrentMenuPage].SelectedOption;
				do
				{			
					if (pMenu[iCurrentMenuPage].SelectedOption == 0) pMenu[iCurrentMenuPage].SelectedOption = pMenu[iCurrentMenuPage].CountOptions;	
					pMenu[iCurrentMenuPage].SelectedOption--;	
					if (k == pMenu[iCurrentMenuPage].SelectedOption) 
					{
						SetShowMenu(0);
						break;
					}
				} while ((pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].Show != 1)
						|| (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ViewLevel > iAccessLevelCopy));
			}
			//*cKeyMem = 0;		
			//*cKeyUpClock = 0;
			//*cKeyDownClock = 0;
		}
		if ((iKeyCode == SYSTEM_CMD_MENU_KEY_LEFT) || ((cKeyStatus == 0) && (cKeyUpClock == 21)))
		{
			if (pMenu[iCurrentMenuPage].CountOptions != 0)
			{
				for (n = 0; n < pMenu[iCurrentMenuPage].CountOptions; n++) 
					if ((pMenu[iCurrentMenuPage].Options[n].Show == 1) && (pMenu[iCurrentMenuPage].Options[n].ViewLevel <= iAccessLevelCopy)) break;
				if ((pMenu[iCurrentMenuPage].SelectedOption != n) && (n != pMenu[iCurrentMenuPage].CountOptions))
				{
					if (pMenu[iCurrentMenuPage].SelectedOption >= iLines)
							pMenu[iCurrentMenuPage].SelectedOption -= iLines;
						else 
							pMenu[iCurrentMenuPage].SelectedOption = 0;							
				}
				else
				{
					if ((iCurrentMenuPage != 0) && (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].PrevPage != -1))
					{
						i = pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].PrevPage;
						if (pMenu[i].OpenFunc != NULL) pMenu[i].OpenFunc(pMenu, i);
						if (pMenu[i].CountOptions != 0) iCurrentMenuPage = i;
					}
					else
					SetShowMenu(0);
				}
			} else SetShowMenu(0);
			cKeyUpClock = 0;
			cKeyDownClock = 0;
		}
		if (((iKeyCode == SYSTEM_CMD_MENU_KEY_OK) || ((cKeyStatus == 1) && (cKeyDownClock == 16)))
			&& (pMenu[iCurrentMenuPage].CountOptions != 0))			
		{
			if (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].Access <= iAccessLevelCopy)			
				i = pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].NextPage;
				else i = 0;
			//if (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ActionFunc != NULL)
				//printf("action menu page:%i opt:%i\n", iCurrentMenuPage, pMenu[iCurrentMenuPage].SelectedOption);
			if ((pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ActionFunc != NULL)
				&& (pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].Access <= iAccessLevelCopy))
				pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].ActionFunc(pMenu, iCurrentMenuPage);
			cKeyMem = 1;		
			cKeyUpClock = 0;
			cKeyDownClock = 0;
			//DBG_ON(DBG_MODE_MEMORY_ERROR | DBG_MODE_TRACKING);
			
			if (i != 0)
			{
				if (pMenu[i].OpenFunc != NULL) pMenu[i].OpenFunc(pMenu, i);					
				for (n = 0; n < pMenu[i].CountOptions; n++) if ((pMenu[i].Options[n].Show == 1) && (pMenu[i].Options[n].ViewLevel <= iAccessLevelCopy)) break;
				if (n != pMenu[i].CountOptions) 
				{
					for (n = 0; n < pMenu[i].CountOptions; n++) 
						if (pMenu[i].Options[n].PrevPageNoHistory == 0) pMenu[i].Options[n].PrevPage = iCurrentMenuPage;					
					iCurrentMenuPage = i;
				}
			}		
		}
		if ((iKeyCode == SYSTEM_CMD_MENU_KEY_RIGHT) || 
			((cKeyStatus == 1) && (cKeyDownClock == 16) && (pMenu[iCurrentMenuPage].CountOptions != 0))
			)
		{
			if (pMenu[iCurrentMenuPage].CountOptions != 0)
			{
				pMenu[iCurrentMenuPage].SelectedOption += iLines;
				if (pMenu[iCurrentMenuPage].SelectedOption >= pMenu[iCurrentMenuPage].CountOptions) 
					pMenu[iCurrentMenuPage].SelectedOption = pMenu[iCurrentMenuPage].CountOptions - 1;				
			} else SetShowMenu(0);			
			cKeyUpClock = 0;
			cKeyDownClock = 0;
		}
		if (pMenu[iCurrentMenuPage].RefreshFunc != NULL) pMenu[iCurrentMenuPage].RefreshFunc(pMenu, iCurrentMenuPage);
		if (pMenu[iCurrentMenuPage].ShowInfo == 0)
		{
			RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s", pMenu[iCurrentMenuPage].Name);
		}
		else
		{			
			DBG_MUTEX_LOCK(&modulelist_mutex);	
			int iSelected = pMenu[iCurrentMenuPage].Options[pMenu[iCurrentMenuPage].SelectedOption].PrevPage;
			if (((iSelected == 0) || (iSelected == 3)) && (iCurrentMenuPage != 8))
				iSelected = ModuleIdToNum(pMenu[0].Options[1].MiscData, 2);
				else 
				iSelected = ModuleIdToNum(pMenu[8].Options[pMenu[8].SelectedOption].MiscData, 2);
			if (iSelected != -1)
			{
				switch(miModuleList[iSelected].Type)
				{
					case MODULE_TYPE_CAMERA:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Свет:%i  Движение(размер):%i(%i) Экспозиция:%s)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0], 
																miModuleList[iSelected].Status[1],
																miModuleList[iSelected].Status[3],
																GetNameExposure(miModuleList[iSelected].Status[2]));
						break;
					case MODULE_TYPE_EXTERNAL:
					case MODULE_TYPE_USB_GPIO:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(%i, %i, %i)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0],
															miModuleList[iSelected].Status[1],
															miModuleList[iSelected].Status[2]);
						break;
					case MODULE_TYPE_GPIO:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(%i)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0]);
						break;
					case MODULE_TYPE_TEMP_SENSOR:
						if (miModuleList[iSelected].Status[0] == 10000)
							RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Температура:N/A)", miModuleList[iSelected].Name);
						else
						{
							if (miModuleList[iSelected].Settings[0] == I2C_ADDRESS_AM2320)
								RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Температура:%.1f`C  Влажность:%i%)", 
									miModuleList[iSelected].Name, (float)miModuleList[iSelected].Status[0]/10, miModuleList[iSelected].Status[1]/10);
							if (miModuleList[iSelected].Settings[0] == I2C_ADDRESS_LM75)
								RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Температура:%.1f`C)", 
									miModuleList[iSelected].Name, (float)miModuleList[iSelected].Status[0]/10);
						}
						break;
					case MODULE_TYPE_SYSTEM:
						if (miModuleList[iSelected].Status[0] == 10000)
							RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Температура:N/A)", miModuleList[iSelected].Name);
						else
						{
							RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Температура:%.1f`C Напряжение:%gv)", 
									miModuleList[iSelected].Name, 
									(float)miModuleList[iSelected].Status[0]/10, 
									(float)miModuleList[iSelected].Status[10]/1000);
						}
						break;
					case MODULE_TYPE_MIC:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Звук:%i)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0]);
						break;
					case MODULE_TYPE_ADS1015:
					case MODULE_TYPE_MCP3421:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Значение:%i)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0]);
						break;
					case MODULE_TYPE_AS5600:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(Поворот RAW:%i, Поворот:%i)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0], miModuleList[iSelected].Status[1]);
						break;
					case MODULE_TYPE_HMC5883L:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s(X:%i, Y:%i, Z:%i)", 
							miModuleList[iSelected].Name, miModuleList[iSelected].Status[0], 
							miModuleList[iSelected].Status[1], miModuleList[iSelected].Status[2]);
						break;
					default:
						RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s", pMenu[8].Name);
						break;					
				}				
			} 
			else 
			{
				RenderText(state, 42*fMSize, iX + (100*fMSize), iY, 0,"%s", pMenu[8].Name);
			}
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
		}
		iY -= (50*fMSize);
		int iSubPages, iCurPage, iFirstOption, iLastOption, iLineCnt, iYopt;
		iYopt = iY;
		iX = 0;
		iSubPages = (int)ceil((double)pMenu[iCurrentMenuPage].CountOptions / iTotalLines);
		if (!iSubPages) iSubPages = 1;
		iCurPage = (int)floor((double)pMenu[iCurrentMenuPage].SelectedOption / iTotalLines);
		iFirstOption = iCurPage * iTotalLines;
		iLastOption = iFirstOption + iTotalLines;
		if (iLastOption > pMenu[iCurrentMenuPage].CountOptions) iLastOption = pMenu[iCurrentMenuPage].CountOptions;
		if (iSubPages > 1)
		{
			RenderText(state, 20*fMSize, 140*fMSize, iY+(30*fMSize), 0,"[");
			RenderText(state, 20*fMSize, 150*fMSize + (50*fMSize / iSubPages * iCurPage), iY+(30*fMSize), 0,"<>");
			RenderText(state, 20*fMSize, 210*fMSize, iY+(30*fMSize), 0,"]");
		}
		iLineCnt = 0;
		for (n = iFirstOption; n < iLastOption; n++)
		{
			if ((pMenu[iCurrentMenuPage].Options[n].Show == 1) && (pMenu[iCurrentMenuPage].Options[n].ViewLevel <= iAccessLevelCopy))
			{
				if (pMenu[iCurrentMenuPage].SelectedOption == n) 
				{
					RenderText(state, 36*fMSize, iX, iY, 0,"|");
					RenderText(state, 36*fMSize, iX + 90*fMSize, iY, 0,"|");
					if (cKeyStatus == 0) 
					{
						RenderText(state, 36*fMSize, iX + 10*fMSize, iY, 0,"<<");
						//for (i = 0; i < cKeyUpClock; i++) {RenderText(state, 36*fMSize, iX +(50-(i*2))*fMSize, iY, 0,"<");}
					}
					if ((iKeyCode == SYSTEM_CMD_MENU_KEY_OFF) || (iKeyCode == SYSTEM_CMD_MENU_KEY_ON))
					{
						if ((cKeyStatus == 1) && (cKeyDownClock > 0) && (cKeyDownClock < 32))
						{
							/*char cKeyBuff[32];
							memset(cKeyBuff, 62, cKeyDownClock + 1);
							cKeyBuff[(unsigned char)cKeyDownClock] = 0;*/
							RenderText(state, 36*fMSize, iX + 50*fMSize, iY, 0, ">>");
						}
					}
					else 
					{
						/*char cKeyBuff[11];
						memset(cKeyBuff, 62, 10);
						cKeyBuff[10] = 0;*/
						RenderText(state, 36*fMSize, iX + 50*fMSize, iY, 0, ">>");
					}
				}
				if (pMenu[iCurrentMenuPage].Options[n].Access <= iAccessLevelCopy) 
					RenderText(state, 36*fMSize, iX+(120*fMSize), iY, 0,"%s", pMenu[iCurrentMenuPage].Options[n].Name);	
					else
					RenderText(state, 36*fMSize, iX+(120*fMSize), iY, 0,"[X] %s", pMenu[iCurrentMenuPage].Options[n].Name);	
					
				iY -= 36*fMSize;	
				iLineCnt++;
				if (iLineCnt == iLines)
				{
					iLineCnt = 0;
					iY = iYopt;
					iX += 1280*fMSize;
				}
			}
		}
	}
	DBG_LOG_OUT();
	return 1;
}

int CreateMenu(MENU_PAGE **pCurMenu)
{
	DBG_LOG_IN();
	MENU_PAGE *pMenu; 
	
	iCurrentMenuPage = 0;
	int iPages = MAX_MENU_PAGES;
	pMenu = (MENU_PAGE*)DBG_MALLOC(iPages*sizeof(MENU_PAGE));
	memset(pMenu, 0, iPages*sizeof(MENU_PAGE));
	*pCurMenu = pMenu;
	
	strcpy(pMenu[0].Name,"<<Главное меню>>");
	pMenu[0].SelectedOption = 0;
    pMenu[0].CountOptions = 27;
	pMenu[0].RefreshFunc = NULL;   
	pMenu[0].OpenFunc = NULL;
	pMenu[0].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[0].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[0].Options, 0, pMenu[0].CountOptions*sizeof(MENU_OPTION));
	
	pMenu[0].Options[0].Show = 1;
	pMenu[0].Options[0].PrevPage = -1;
	pMenu[0].Options[0].NextPage = 1;
	pMenu[0].Options[0].ActionFunc = NULL;
	strcpy(pMenu[0].Options[0].Name,"Камеры");
	pMenu[0].Options[1].Show = 0;
	pMenu[0].Options[1].PrevPage = -1;
	pMenu[0].Options[1].NextPage = 3;
	pMenu[0].Options[1].ActionFunc = NULL;
	strcpy(pMenu[0].Options[1].Name,"Камера");
	pMenu[0].Options[2].Show = 0;
	pMenu[0].Options[2].PrevPage = -1;
	pMenu[0].Options[2].NextPage = 4;
	pMenu[0].Options[2].ActionFunc = NULL;
	strcpy(pMenu[0].Options[2].Name,"Фото");
    pMenu[0].Options[3].Show = 0;
	pMenu[0].Options[3].PrevPage = -1;
	pMenu[0].Options[3].NextPage = 5;
	pMenu[0].Options[3].ActionFunc = NULL;
	strcpy(pMenu[0].Options[3].Name,"Медиа");
	pMenu[0].Options[4].Show = 1;
	pMenu[0].Options[4].PrevPage = -1;
	pMenu[0].Options[4].NextPage = 2;
	pMenu[0].Options[4].ActionFunc = NULL;
	strcpy(pMenu[0].Options[4].Name,"Будильники");
	pMenu[0].Options[5].Show = 0;
	pMenu[0].Options[5].PrevPage = -1;
	pMenu[0].Options[5].NextPage = 0;
	pMenu[0].Options[5].ActionFunc = Menu_ClockAlarmStop;
	strcpy(pMenu[0].Options[5].Name,"Отключить будильник");
	pMenu[0].Options[6].Show = 0;
	pMenu[0].Options[6].PrevPage = -1;
	pMenu[0].Options[6].NextPage = 0;
	pMenu[0].Options[6].ActionFunc = Menu_CloseAudio;
	strcpy(pMenu[0].Options[6].Name,"Аудио");
	if (uiDeviceType & DEVICE_TYPE_RADIO) pMenu[0].Options[7].Show = 1; else pMenu[0].Options[7].Show = 0;
	pMenu[0].Options[7].PrevPage = -1;
	pMenu[0].Options[7].NextPage = 6;
	pMenu[0].Options[7].ActionFunc = NULL;
	strcpy(pMenu[0].Options[7].Name,"Радио");
	pMenu[0].Options[8].Show = 1;
	pMenu[0].Options[8].PrevPage = -1;
	pMenu[0].Options[8].NextPage = 26;
	pMenu[0].Options[8].ActionFunc = NULL;
	strcpy(pMenu[0].Options[8].Name,"Модули");	
    pMenu[0].Options[9].Show = 1;
	pMenu[0].Options[9].PrevPage = -1;
	pMenu[0].Options[9].NextPage = 0;
	pMenu[0].Options[9].ActionFunc = Menu_ResetShower;
	strcpy(pMenu[0].Options[9].Name,"Сменить");	
    pMenu[0].Options[10].Show = 1;
	pMenu[0].Options[10].PrevPage = -1;
	pMenu[0].Options[10].NextPage = 10;
	pMenu[0].Options[10].ActionFunc = NULL;
	strcpy(pMenu[0].Options[10].Name,"Основная громкость");	
    pMenu[0].Options[11].Show = 1;
	pMenu[0].Options[11].PrevPage = -1;
	pMenu[0].Options[11].NextPage = 11;
	pMenu[0].Options[11].ActionFunc = NULL;
	strcpy(pMenu[0].Options[11].Name,"Громкость будильника");	
	pMenu[0].Options[12].Show = 1;
	pMenu[0].Options[12].PrevPage = -1;
	pMenu[0].Options[12].NextPage = 12;
	pMenu[0].Options[12].ActionFunc = NULL;
	pMenu[0].Options[12].Access = 0;
	strcpy(pMenu[0].Options[12].Name,"Сообщения");	
    pMenu[0].Options[13].Show = 1;
	pMenu[0].Options[13].PrevPage = -1;
	pMenu[0].Options[13].NextPage = 0;
	pMenu[0].Options[13].ActionFunc = System_Reboot;
	pMenu[0].Options[13].Access = 1;
	strcpy(pMenu[0].Options[13].Name,"Перезагрузить");	
    pMenu[0].Options[14].Show = 1;
	pMenu[0].Options[14].PrevPage = -1;
	pMenu[0].Options[14].NextPage = 0;
	pMenu[0].Options[14].ActionFunc = System_Exit;
	pMenu[0].Options[14].Access = 1;
	strcpy(pMenu[0].Options[14].Name,"Выход в систему");	
    pMenu[0].Options[15].Show = 1;
	pMenu[0].Options[15].PrevPage = -1;
	pMenu[0].Options[15].NextPage = 0;
	pMenu[0].Options[15].ActionFunc = System_Shutdown;
	pMenu[0].Options[15].Access = 1;
	strcpy(pMenu[0].Options[15].Name,"Выключить систему");	
    pMenu[0].Options[16].Show = 1;
	pMenu[0].Options[16].PrevPage = -1;
	pMenu[0].Options[16].NextPage = 15;
	pMenu[0].Options[16].ActionFunc = NULL;
	pMenu[0].Options[16].Access = 0;
	strcpy(pMenu[0].Options[16].Name,"Визуализаторы");	
    pMenu[0].Options[17].Show = 1;
	pMenu[0].Options[17].PrevPage = -1;
	pMenu[0].Options[17].NextPage = 16;
	pMenu[0].Options[17].ActionFunc = NULL;
	pMenu[0].Options[17].Access = 0;
	strcpy(pMenu[0].Options[17].Name,"Файлы");	
	pMenu[0].Options[18].Show = 1;
	pMenu[0].Options[18].PrevPage = -1;
	pMenu[0].Options[18].NextPage = 0;
	pMenu[0].Options[18].ActionFunc = Menu_PlayNextDir;
	pMenu[0].Options[18].Access = 0;
	strcpy(pMenu[0].Options[18].Name,"Сменить тему");	
	pMenu[0].Options[19].Show = 1;
	pMenu[0].Options[19].PrevPage = -1;
	pMenu[0].Options[19].NextPage = 18;
	pMenu[0].Options[19].ActionFunc = NULL;
	pMenu[0].Options[19].Access = 0;
	strcpy(pMenu[0].Options[19].Name,"Интернет поток");	
	pMenu[0].Options[20].Show = 0;
	pMenu[0].Options[20].PrevPage = -1;
	pMenu[0].Options[20].NextPage = 7;
	pMenu[0].Options[20].ActionFunc = NULL;
	strcpy(pMenu[0].Options[20].Name,"Поток");
    pMenu[0].Options[21].Show = 1;
	pMenu[0].Options[21].PrevPage = -1;
	pMenu[0].Options[21].NextPage = 21;
	pMenu[0].Options[21].ActionFunc = NULL;
	pMenu[0].Options[21].Access = 0;
	strcpy(pMenu[0].Options[21].Name,"Действия");
	pMenu[0].Options[22].Show = 1;
	pMenu[0].Options[22].PrevPage = -1;
	pMenu[0].Options[22].NextPage = 22;
	pMenu[0].Options[22].ActionFunc = NULL;
	pMenu[0].Options[22].Access = 1;
	strcpy(pMenu[0].Options[22].Name,"Действия на события");
    pMenu[0].Options[23].Show = 1;
	pMenu[0].Options[23].PrevPage = -1;
	pMenu[0].Options[23].NextPage = 23;
	pMenu[0].Options[23].ActionFunc = NULL;
	pMenu[0].Options[23].Access = 0;
	strcpy(pMenu[0].Options[23].Name,"Плейлист");
	pMenu[0].Options[24].Show = 1;
	pMenu[0].Options[24].PrevPage = -1;
	pMenu[0].Options[24].NextPage = 24;
	pMenu[0].Options[24].ActionFunc = NULL;
	pMenu[0].Options[24].Access = 0;
	strcpy(pMenu[0].Options[24].Name,"Папки");
	pMenu[0].Options[25].Show = 1;
	pMenu[0].Options[25].PrevPage = -1;
	pMenu[0].Options[25].NextPage = 0;
	pMenu[0].Options[25].ActionFunc = Menu_ChangeShowMode;
	pMenu[0].Options[25].Access = 0;
	strcpy(pMenu[0].Options[25].Name,"Сменить режим");
	pMenu[0].Options[26].Show = 1;
	pMenu[0].Options[26].PrevPage = -1;
	pMenu[0].Options[26].NextPage = 25;
	pMenu[0].Options[26].ActionFunc = NULL;
	strcpy(pMenu[0].Options[26].Name,"Архив");	
    
   
	strcpy(pMenu[1].Name,"<<Список камер>>");
	pMenu[1].SelectedOption = 0;
    pMenu[1].CountOptions = 0;
	pMenu[1].RefreshFunc = NULL;   
	pMenu[1].Options = NULL;
	pMenu[1].OpenFunc = Menu_RefreshCameraMenu;
	
	strcpy(pMenu[2].Name,"<<Список будильников>>");
	pMenu[2].SelectedOption = 0;
    pMenu[2].CountOptions = 0;
	pMenu[2].RefreshFunc = NULL;   
	pMenu[2].Options = NULL;
	pMenu[2].OpenFunc = Menu_RefreshAlarmsMenu;
			
	strcpy(pMenu[3].Name,"<<Отключиться от камеры:>>");
	pMenu[3].SelectedOption = 0;
	pMenu[3].ShowInfo = 1;
    pMenu[3].CountOptions = 9;
	pMenu[3].RefreshFunc = NULL;   
	pMenu[3].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[3].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[3].Options, 0, pMenu[3].CountOptions*sizeof(MENU_OPTION));	
	pMenu[3].Options[0].Show = 1;
	pMenu[3].Options[0].PrevPage = -1;
	pMenu[3].Options[0].NextPage = 0;
	pMenu[3].Options[0].ActionFunc = Menu_CloseVideo;
	strcpy(pMenu[3].Options[0].Name,"сейчас");
	pMenu[3].Options[1].Show = 1;
	pMenu[3].Options[1].PrevPage = -1;
	pMenu[3].Options[1].NextPage = 0;
	pMenu[3].Options[1].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[3].Options[1].Name,"через 1 минуту");
	pMenu[3].Options[2].Show = 1;
	pMenu[3].Options[2].PrevPage = -1;
	pMenu[3].Options[2].NextPage = 0;
	pMenu[3].Options[2].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[3].Options[2].Name,"через 3 минуты");
	pMenu[3].Options[3].Show = 1;
	pMenu[3].Options[3].PrevPage = -1;
	pMenu[3].Options[3].NextPage = 0;
	pMenu[3].Options[3].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[3].Options[3].Name,"через 5 минут");
	pMenu[3].Options[4].Show = 1;
	pMenu[3].Options[4].PrevPage = -1;
	pMenu[3].Options[4].NextPage = 0;
	pMenu[3].Options[4].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[3].Options[4].Name,"через 10 минут");
	pMenu[3].Options[5].Show = 1;
	pMenu[3].Options[5].PrevPage = -1;
	pMenu[3].Options[5].NextPage = 0;
	pMenu[3].Options[5].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[3].Options[5].Name,"через 15 минут");
	pMenu[3].Options[6].Show = 1;
	pMenu[3].Options[6].PrevPage = -1;
	pMenu[3].Options[6].NextPage = 0;
	pMenu[3].Options[6].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[3].Options[6].Name,"через 20 минут");	
	pMenu[3].Options[7].Show = 1;
	pMenu[3].Options[7].PrevPage = 3;
	pMenu[3].Options[7].NextPage = 14;
	pMenu[3].Options[7].ActionFunc = NULL;
	strcpy(pMenu[3].Options[7].Name,"Изменить экспозицию");
	pMenu[3].Options[8].Show = 1;
	pMenu[3].Options[8].PrevPage = 3;
	pMenu[3].Options[8].NextPage = 0;
	pMenu[3].Options[8].ActionFunc = Menu_GetModuleStatus;
	strcpy(pMenu[3].Options[8].Name,"Обновить состояние");
	
	
	strcpy(pMenu[4].Name,"<<Сменить фото:>>");
	pMenu[4].SelectedOption = 0;
    pMenu[4].CountOptions = 5;
	pMenu[4].RefreshFunc = NULL;   
	pMenu[4].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[4].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[4].Options, 0, pMenu[4].CountOptions*sizeof(MENU_OPTION));
	pMenu[4].Options[0].Show = 1;
	pMenu[4].Options[0].PrevPage = 0;
	pMenu[4].Options[0].NextPage = 0;
	pMenu[4].Options[0].ActionFunc = Menu_GetNext;
	strcpy(pMenu[4].Options[0].Name,"следующий");
	pMenu[4].Options[1].Show = 1;
	pMenu[4].Options[1].PrevPage = 0;
	pMenu[4].Options[1].NextPage = 0;
	pMenu[4].Options[1].ActionFunc = Menu_GetPrev;
	strcpy(pMenu[4].Options[1].Name,"предыдущий");
	pMenu[4].Options[2].Show = 1;
	pMenu[4].Options[2].PrevPage = 0;
	pMenu[4].Options[2].NextPage = 0;
	pMenu[4].Options[2].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[4].Options[2].Name,"через 1 минуту");
	pMenu[4].Options[3].Show = 1;
	pMenu[4].Options[3].PrevPage = 0;
	pMenu[4].Options[3].NextPage = 0;
	pMenu[4].Options[3].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[4].Options[3].Name,"через 3 минуты");
	pMenu[4].Options[4].Show = 1;
	pMenu[4].Options[4].PrevPage = 0;
	pMenu[4].Options[4].NextPage = 0;
	pMenu[4].Options[4].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[4].Options[4].Name,"через 5 минут");
	
	strcpy(pMenu[5].Name,"<<Медиа:>>");
	pMenu[5].SelectedOption = 0;
    pMenu[5].CountOptions = 3;
	pMenu[5].RefreshFunc = NULL;   
	pMenu[5].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[5].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[5].Options, 0, pMenu[5].CountOptions*sizeof(MENU_OPTION));
	pMenu[5].Options[0].Show = 1;
	pMenu[5].Options[0].PrevPage = 0;
	pMenu[5].Options[0].NextPage = 0;
	pMenu[5].Options[0].ActionFunc = Menu_GetNext;
	strcpy(pMenu[5].Options[0].Name,"Следующий");
	pMenu[5].Options[1].Show = 1;
	pMenu[5].Options[1].PrevPage = 0;
	pMenu[5].Options[1].NextPage = 0;
	pMenu[5].Options[1].ActionFunc = Menu_GetAgain;
	strcpy(pMenu[5].Options[1].Name,"Показать сначала");
	pMenu[5].Options[2].Show = 1;
	pMenu[5].Options[2].PrevPage = 0;
	pMenu[5].Options[2].NextPage = 0;
	pMenu[5].Options[2].ActionFunc = Menu_GetPrev;
	strcpy(pMenu[5].Options[2].Name,"Предыдущий");
	
	strcpy(pMenu[6].Name,"<<Радио>>");
	pMenu[6].SelectedOption = 0;
    pMenu[6].CountOptions = 0;
	pMenu[6].RefreshFunc = NULL;   
	pMenu[6].Options = NULL;
	pMenu[6].OpenFunc = Menu_RefreshRadiosMenu;
	
	strcpy(pMenu[7].Name,"<<Выключить>>");
	pMenu[7].SelectedOption = 0;
    pMenu[7].CountOptions = 9;
	pMenu[7].RefreshFunc = NULL;   
	pMenu[7].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[7].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[7].Options, 0, pMenu[7].CountOptions*sizeof(MENU_OPTION));	
	
	pMenu[7].Options[0].Show = 1;
	pMenu[7].Options[0].PrevPage = -1;
	pMenu[7].Options[0].NextPage = 0;
	pMenu[7].Options[0].ActionFunc = Menu_OffRadioStation;
	strcpy(pMenu[7].Options[0].Name,"сейчас");
	pMenu[7].Options[1].Show = 1;
	pMenu[7].Options[1].PrevPage = -1;
	pMenu[7].Options[1].NextPage = 0;
	pMenu[7].Options[1].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[1].Name,"через 10 минут");
	pMenu[7].Options[2].Show = 1;
	pMenu[7].Options[2].PrevPage = -1;
	pMenu[7].Options[2].NextPage = 0;
	pMenu[7].Options[2].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[2].Name,"через 15 минут");
	pMenu[7].Options[3].Show = 1;
	pMenu[7].Options[3].PrevPage = -1;
	pMenu[7].Options[3].NextPage = 0;
	pMenu[7].Options[3].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[3].Name,"через 20 минут");
	pMenu[7].Options[4].Show = 1;
	pMenu[7].Options[4].PrevPage = -1;
	pMenu[7].Options[4].NextPage = 0;
	pMenu[7].Options[4].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[4].Name,"через 25 минут");
	pMenu[7].Options[5].Show = 1;
	pMenu[7].Options[5].PrevPage = -1;
	pMenu[7].Options[5].NextPage = 0;
	pMenu[7].Options[5].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[5].Name,"через 30 минут");
	pMenu[7].Options[6].Show = 1;
	pMenu[7].Options[6].PrevPage = -1;
	pMenu[7].Options[6].NextPage = 0;
	pMenu[7].Options[6].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[6].Name,"через 40 минут");	
	pMenu[7].Options[7].Show = 1;
	pMenu[7].Options[7].PrevPage = -1;
	pMenu[7].Options[7].NextPage = 0;
	pMenu[7].Options[7].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[7].Name,"через 60 минут");	
	pMenu[7].Options[8].Show = 1;
	pMenu[7].Options[8].PrevPage = -1;
	pMenu[7].Options[8].NextPage = 0;
	pMenu[7].Options[8].ActionFunc = Menu_SetTimeOut;
	strcpy(pMenu[7].Options[8].Name,"через 80 минут");	
	
	strcpy(pMenu[8].Name,"<<Список модулей>>");
	pMenu[8].ShowInfo = 1;
	pMenu[8].SelectedOption = 0;
    pMenu[8].CountOptions = 0;
	pMenu[8].RefreshFunc = NULL;   
	pMenu[8].Options = NULL;
	pMenu[8].OpenFunc = Menu_RefreshModuleListMenu;
	
	strcpy(pMenu[9].Name,"<<Меню модуля>>");
	pMenu[9].ShowInfo = 1;
	pMenu[9].SelectedOption = 0;
    pMenu[9].CountOptions = 3;
	pMenu[9].RefreshFunc = NULL;   
	pMenu[9].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[9].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[9].Options, 0, pMenu[9].CountOptions*sizeof(MENU_OPTION));	
	
	pMenu[9].Options[0].Show = 1;
	pMenu[9].Options[0].PrevPage = 8;
	pMenu[9].Options[0].NextPage = 0;
	pMenu[9].Options[0].ActionFunc = Menu_GetModuleStatus;
	strcpy(pMenu[9].Options[0].Name,"Обновить состояние");
	pMenu[9].Options[1].Show = 0;
	pMenu[9].Options[1].PrevPage = 8;
	pMenu[9].Options[1].NextPage = 0;
	pMenu[9].Options[1].ActionFunc = Menu_SendGpioModuleOn;
	strcpy(pMenu[9].Options[1].Name,"Включить");
	pMenu[9].Options[2].Show = 0;
	pMenu[9].Options[2].PrevPage = 8;
	pMenu[9].Options[2].NextPage = 0;
	pMenu[9].Options[2].ActionFunc = Menu_SendGpioModuleOff;
	strcpy(pMenu[9].Options[2].Name,"Отключить");
	
	strcpy(pMenu[10].Name,"<<Основная громкость>>");
	pMenu[10].SelectedOption = 0;
    pMenu[10].CountOptions = 4;
	pMenu[10].RefreshFunc = Menu_GetMusicVolume;   
	pMenu[10].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[10].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[10].Options, 0, pMenu[10].CountOptions*sizeof(MENU_OPTION));
	pMenu[10].Options[0].Show = 1;
	pMenu[10].Options[0].PrevPage = 0;
	pMenu[10].Options[0].NextPage = 0;
	pMenu[10].Options[0].ActionFunc = Menu_MusicVolumeUp;
	strcpy(pMenu[10].Options[0].Name,"Увеличить громкость");
	pMenu[10].Options[1].Show = 1;
	pMenu[10].Options[1].PrevPage = 0;
	pMenu[10].Options[1].NextPage = 0;
	pMenu[10].Options[1].ActionFunc = Menu_MusicVolumeDown;
	strcpy(pMenu[10].Options[1].Name,"Уменьшить громкость");
	pMenu[10].Options[2].Show = 1;
	pMenu[10].Options[2].PrevPage = 0;
	pMenu[10].Options[2].NextPage = 0;
	pMenu[10].Options[2].ActionFunc = Menu_MusicVolumeMute;
	strcpy(pMenu[10].Options[2].Name,"Минимальная громкость");
	pMenu[10].Options[3].Show = 1;
	pMenu[10].Options[3].PrevPage = 0;
	pMenu[10].Options[3].NextPage = 0;
	pMenu[10].Options[3].ActionFunc = Menu_MusicVolumeMiddle;
	strcpy(pMenu[10].Options[3].Name,"Средняя громкость");
	
	strcpy(pMenu[11].Name,"<<Громкость будильника>>");
	pMenu[11].SelectedOption = 0;
    pMenu[11].CountOptions = 2;
	pMenu[11].RefreshFunc = Menu_GetAlarmVolume;   
	pMenu[11].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[11].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[11].Options, 0, pMenu[11].CountOptions*sizeof(MENU_OPTION));
	pMenu[11].Options[0].Show = 1;
	pMenu[11].Options[0].PrevPage = 0;
	pMenu[11].Options[0].NextPage = 0;
	pMenu[11].Options[0].ActionFunc = Menu_AlarmVolumeUp;
	strcpy(pMenu[11].Options[0].Name,"Увеличить громкость");
	pMenu[11].Options[1].Show = 1;
	pMenu[11].Options[1].PrevPage = 0;
	pMenu[11].Options[1].NextPage = 0;
	pMenu[11].Options[1].ActionFunc = Menu_AlarmVolumeDown;
	strcpy(pMenu[11].Options[1].Name,"Уменьшить громкость");

	strcpy(pMenu[12].Name,"<<Список сообщений>>");
	pMenu[12].SelectedOption = 0;
    pMenu[12].CountOptions = 1;
	pMenu[12].RefreshFunc = NULL;   
	pMenu[12].OpenFunc = Menu_RefreshMessageListMenu;
	pMenu[12].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[12].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[12].Options, 0, pMenu[12].CountOptions*sizeof(MENU_OPTION));
	pMenu[12].Options[0].Show = 1;
	pMenu[12].Options[0].PrevPage = 0;
	pMenu[12].Options[0].NextPage = 0;
	pMenu[12].Options[0].ActionFunc = Menu_ClearMessageList;
	strcpy(pMenu[12].Options[0].Name,"Очистить");
	
	strcpy(pMenu[13].Name,"<<Действия с камерой>>");
	pMenu[13].SelectedOption = 0;
	pMenu[13].ShowInfo = 1;	
    pMenu[13].CountOptions = 3;
	pMenu[13].RefreshFunc = NULL;   
	pMenu[13].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[13].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[13].Options, 0, pMenu[13].CountOptions*sizeof(MENU_OPTION));
	pMenu[13].Options[0].Show = 1;
	pMenu[13].Options[0].PrevPage = 8;
	pMenu[13].Options[0].NextPage = 0;
	pMenu[13].Options[0].ActionFunc = Menu_GetModuleStatus;
	strcpy(pMenu[13].Options[0].Name,"Обновить состояние");
	pMenu[13].Options[1].Show = 1;
	pMenu[13].Options[1].PrevPage = 8;
	pMenu[13].Options[1].NextPage = 0;
	pMenu[13].Options[1].ActionFunc = Menu_ModuleAction;
	strcpy(pMenu[13].Options[1].Name,"Просмотреть");
	pMenu[13].Options[2].Show = 1;
	pMenu[13].Options[2].PrevPage = 8;
	pMenu[13].Options[2].NextPage = 14;
	pMenu[13].Options[2].ActionFunc = NULL;
	strcpy(pMenu[13].Options[2].Name,"Изменить экспозицию");
		
	strcpy(pMenu[14].Name,"<<Изменить экспозицию>>");
	pMenu[14].SelectedOption = 0;
	pMenu[14].ShowInfo = 1;	
    pMenu[14].CountOptions = 15;
	pMenu[14].RefreshFunc = NULL;   
	pMenu[14].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[14].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[14].Options, 0, pMenu[14].CountOptions*sizeof(MENU_OPTION));
	pMenu[14].Options[0].Show = 1;
	pMenu[14].Options[0].PrevPage = 13;
	pMenu[14].Options[0].NextPage = 0;
	pMenu[14].Options[0].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[0].Name,"Off");
	pMenu[14].Options[1].Show = 1;
	pMenu[14].Options[1].PrevPage = 13;
	pMenu[14].Options[1].NextPage = 0;
	pMenu[14].Options[1].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[1].Name,"Auto");
	pMenu[14].Options[2].Show = 1;
	pMenu[14].Options[2].PrevPage = 13;
	pMenu[14].Options[2].NextPage = 0;
	pMenu[14].Options[2].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[2].Name,"Night");
	pMenu[14].Options[3].Show = 1;
	pMenu[14].Options[3].PrevPage = 13;
	pMenu[14].Options[3].NextPage = 0;
	pMenu[14].Options[3].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[3].Name,"BackLight");
	pMenu[14].Options[4].Show = 1;
	pMenu[14].Options[4].PrevPage = 13;
	pMenu[14].Options[4].NextPage = 0;
	pMenu[14].Options[4].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[4].Name,"SpotLight");
	pMenu[14].Options[5].Show = 1;
	pMenu[14].Options[5].PrevPage = 13;
	pMenu[14].Options[5].NextPage = 0;
	pMenu[14].Options[5].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[5].Name,"Sports");
	pMenu[14].Options[6].Show = 1;
	pMenu[14].Options[6].PrevPage = 13;
	pMenu[14].Options[6].NextPage = 0;
	pMenu[14].Options[6].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[6].Name,"Snow");
	pMenu[14].Options[7].Show = 1;
	pMenu[14].Options[7].PrevPage = 13;
	pMenu[14].Options[7].NextPage = 0;
	pMenu[14].Options[7].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[7].Name,"Beach");
	pMenu[14].Options[8].Show = 1;
	pMenu[14].Options[8].PrevPage = 13;
	pMenu[14].Options[8].NextPage = 0;
	pMenu[14].Options[8].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[8].Name,"LargeAperture");
	pMenu[14].Options[9].Show = 1;
	pMenu[14].Options[9].PrevPage = 13;
	pMenu[14].Options[9].NextPage = 0;
	pMenu[14].Options[9].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[9].Name,"SmallApperture");
	pMenu[14].Options[10].Show = 1;
	pMenu[14].Options[10].PrevPage = 13;
	pMenu[14].Options[10].NextPage = 0;
	pMenu[14].Options[10].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[10].Name,"VeryLong");	
	pMenu[14].Options[11].Show = 1;
	pMenu[14].Options[11].PrevPage = 13;
	pMenu[14].Options[11].NextPage = 0;
	pMenu[14].Options[11].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[11].Name,"FixedFps");	
	pMenu[14].Options[12].Show = 1;
	pMenu[14].Options[12].PrevPage = 13;
	pMenu[14].Options[12].NextPage = 0;
	pMenu[14].Options[12].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[12].Name,"NightWithPreview");	
	pMenu[14].Options[13].Show = 1;
	pMenu[14].Options[13].PrevPage = 13;
	pMenu[14].Options[13].NextPage = 0;
	pMenu[14].Options[13].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[13].Name,"Antishake");	
	pMenu[14].Options[14].Show = 1;
	pMenu[14].Options[14].PrevPage = 13;
	pMenu[14].Options[14].NextPage = 0;
	pMenu[14].Options[14].ActionFunc = Menu_SetExposure;
	strcpy(pMenu[14].Options[14].Name,"Fireworks");	
	
	strcpy(pMenu[15].Name,"<<Список визуализаторов>>");
	pMenu[15].SelectedOption = 0;
    pMenu[15].CountOptions = 0;
	pMenu[15].RefreshFunc = NULL;
	pMenu[15].OpenFunc = Menu_RefreshWidgetList;  
	pMenu[15].Options = NULL;
	
	strcpy(pMenu[16].Name,"<<Список директорий>>");
	pMenu[16].SelectedOption = 0;
    pMenu[16].CountOptions = 0;
	pMenu[16].RefreshFunc = NULL;   
	pMenu[16].OpenFunc = Menu_RefreshDirList;
	pMenu[16].Options = NULL;
	
	strcpy(pMenu[17].Name,"<<Список файлов>>");
	pMenu[17].SelectedOption = 0;
    pMenu[17].CountOptions = 0;
	pMenu[17].RefreshFunc = NULL;   
	pMenu[17].Options = NULL;	
	
	strcpy(pMenu[18].Name,"<<Темы потоков>>");
	pMenu[18].SelectedOption = 0;
    pMenu[18].CountOptions = 0;
	pMenu[18].RefreshFunc = NULL;   
	pMenu[18].OpenFunc = Menu_RefreshStreamTypeList;   
	pMenu[18].Options = NULL;
	
	strcpy(pMenu[19].Name,"<<Управление сисемой>>");
	pMenu[19].SelectedOption = 0;
	pMenu[19].ShowInfo = 1;    
    pMenu[19].CountOptions = 6;
	pMenu[19].RefreshFunc = NULL;   
	pMenu[19].Options = (MENU_OPTION*)DBG_MALLOC(pMenu[19].CountOptions*sizeof(MENU_OPTION));
	memset(pMenu[19].Options, 0, pMenu[19].CountOptions*sizeof(MENU_OPTION));
	pMenu[19].Options[0].Show = 1;
	pMenu[19].Options[0].PrevPage = 0;
	pMenu[19].Options[0].NextPage = 0;
	pMenu[19].Options[0].Access = 0;
	pMenu[19].Options[0].ActionFunc = Menu_SystemAction;
	strcpy(pMenu[19].Options[0].Name,"Обновить статус");
	pMenu[19].Options[1].Show = 1;
	pMenu[19].Options[1].PrevPage = 0;
	pMenu[19].Options[1].NextPage = 0;
	pMenu[19].Options[1].Access = 1;
	pMenu[19].Options[1].ActionFunc = Menu_SystemAction;
	strcpy(pMenu[19].Options[1].Name,"Закрыть");
	pMenu[19].Options[2].Show = 1;
	pMenu[19].Options[2].PrevPage = 0;
	pMenu[19].Options[2].NextPage = 0;
	pMenu[19].Options[2].Access = 1;
	pMenu[19].Options[2].ActionFunc = Menu_SystemAction;
	strcpy(pMenu[19].Options[2].Name,"Перезагрузить");
	pMenu[19].Options[3].Show = 1;
	pMenu[19].Options[3].PrevPage = 0;
	pMenu[19].Options[3].NextPage = 0;
	pMenu[19].Options[3].Access = 1;
	pMenu[19].Options[3].ActionFunc = Menu_SystemAction;
	strcpy(pMenu[19].Options[3].Name,"Выключить");
	pMenu[19].Options[4].Show = 1;
	pMenu[19].Options[4].PrevPage = 0;
	pMenu[19].Options[4].NextPage = 0;
	pMenu[19].Options[4].Access = 1;
	pMenu[19].Options[4].ActionFunc = Menu_SystemAction;
	strcpy(pMenu[19].Options[4].Name,"Перезапуск");
	pMenu[19].Options[5].Show = 1;
	pMenu[19].Options[5].PrevPage = 0;
	pMenu[19].Options[5].NextPage = 0;
	pMenu[19].Options[5].Access = 1;
	pMenu[19].Options[5].ActionFunc = Menu_SystemAction;
	strcpy(pMenu[19].Options[5].Name,"Закрыть аварийно");
	
	strcpy(pMenu[20].Name,"<<Список потоков>>");
	pMenu[20].SelectedOption = 0;
    pMenu[20].CountOptions = 0;
	pMenu[20].RefreshFunc = NULL;   
	pMenu[20].Options = NULL;
	pMenu[20].OpenFunc = Menu_RefreshStreamList;
	
	strcpy(pMenu[21].Name,"<<Действия>>");
	pMenu[21].SelectedOption = 0;
    pMenu[21].CountOptions = 0;
	pMenu[21].RefreshFunc = NULL;   
	pMenu[21].Options = NULL;
	pMenu[21].OpenFunc = Menu_RefreshMnlActionList;
	
	strcpy(pMenu[22].Name,"<<Действия на события>>");
	pMenu[22].SelectedOption = 0;
    pMenu[22].CountOptions = 0;
	pMenu[22].RefreshFunc = NULL; 
	pMenu[22].OpenFunc = Menu_RefreshActionListMenu;
	pMenu[22].Options = NULL;
	
	strcpy(pMenu[23].Name,"<<Плейлист>>");
	pMenu[23].SelectedOption = 0;
    pMenu[23].CountOptions = 0;
	pMenu[23].RefreshFunc = NULL;   
	pMenu[23].Options = NULL;
	
	strcpy(pMenu[24].Name,"<<Папки>>");
	pMenu[24].SelectedOption = 0;
    pMenu[24].CountOptions = 0;
	pMenu[24].RefreshFunc = NULL;   
	pMenu[24].Options = NULL;
	
	strcpy(pMenu[25].Name,"<<Список архивов>>");
	pMenu[25].ShowInfo = 1;
	pMenu[25].SelectedOption = 0;
    pMenu[25].CountOptions = 0;
	pMenu[25].RefreshFunc = NULL;   
	pMenu[25].Options = NULL;
	pMenu[25].OpenFunc = Menu_RefreshArchivListMenu;
	
	strcpy(pMenu[26].Name,"<<Типы модулей>>");
	pMenu[26].SelectedOption = 0;
    pMenu[26].CountOptions = 0;
	pMenu[26].RefreshFunc = NULL;   
	pMenu[26].OpenFunc = Menu_RefreshModuleTypeList;   
	pMenu[26].Options = NULL;
	
	DBG_LOG_OUT();
	return 1;
}

int SearchDataInBuffer(char *cBuffer, unsigned int iBufferLen, unsigned int iPos, char *cData, unsigned int uiDataLen)
{
	//DBG_LOG_IN();
	int iFlag = 0;
	int n;
	int ret = 0;
	for (n = iPos; n != iBufferLen; n++)
	{
		if (cData[iFlag] == cBuffer[n]) iFlag++; else iFlag = 0;
		if (iFlag == uiDataLen) 
		{
			ret = (n -uiDataLen + 2);
			break;
		}
	}
	//DBG_LOG_OUT();
	return ret;
}

int SearchStrInData(char *cData, int iDataLen, int iPos, char *cStr)
{
	//DBG_LOG_IN();
	int iStrLen = strlen(cStr);
	int iFlag = 0;
	int n;
	int ret = 0;
	for (n = iPos; n != iDataLen; n++)
	{
		if (cStr[iFlag] == cData[n]) iFlag++; else iFlag = 0;
		if (iFlag == iStrLen) 
		{
			ret = (n -iStrLen + 2);
			break;
		}
	}
	//DBG_LOG_OUT();
	return ret;
}

int SearchStrInDataCaseIgn(char *Data, int DataLen, int Pos, char *Str)
{
	int StrLen = strlen(Str);
	//int ss = strcmp(Str, "Host: ");
	//if (ss == 0) printf("len: %i %s [%i %i]\n", StrLen, Str, Pos, DataLen);
	int Flag1 = 0;
	char cLeft;
	char cRight;
	int n;
	for (n = Pos; n < DataLen; n++)
	{
		cLeft = Data[n];
		cRight = Str[Flag1];
		if ((cLeft > 96) && (cLeft < 123)) cLeft = cLeft - 32;
		if (cLeft > 223) cLeft = cLeft - 32;
		if ((cRight > 96) && (cRight < 123)) cRight = cRight - 32;
		if (cRight > 223) cRight = cRight - 32;
	
		//if (ss == 0) printf(": %i %i '%c' '%c'\n", cLeft, cRight, cLeft, cRight);
		
		if (cLeft == cRight) Flag1++; else 
		{
			Flag1 = 0;
			cRight = Str[Flag1];
			if ((cRight > 96) && (cRight < 123)) cRight = cRight - 32;
			if (cRight > 223) cRight = cRight - 32;
			
			if (cLeft == cRight) Flag1++;
		}
		if (Flag1 == StrLen) return (n - StrLen + 2);
	}
	return 0;
}

int CompareStrCaseIgn(char *Data, char *Str)
{
	int StrLen = strlen(Str);
	int Flag1 = 0;
	char cLeft;
	char cRight;
	int Pos = 0;
	int DataLen = strlen(Data);
	if (StrLen != DataLen) return 0;
	int n;
	for (n = Pos; n < DataLen; n++)
	{
		cLeft = Data[n];
		cRight = Str[Flag1];
		if ((cLeft > 96) && (cLeft < 123)) cLeft = cLeft - 32;
		if (cLeft > 223) cLeft = cLeft - 32;
		if ((cRight > 96) && (cRight < 123)) cRight = cRight - 32;
		if (cRight > 223) cRight = cRight - 32;
	
		if (cLeft == cRight) Flag1++; else 
		{
			Flag1 = 0;
			cRight = Str[Flag1];
			if ((cRight > 96) && (cRight < 123)) cRight = cRight - 32;
			if (cRight > 223) cRight = cRight - 32;
			
			if (cLeft == cRight) Flag1++;
		}
		if (Flag1 == StrLen) return (n - StrLen + 2);
	}
	return 0;
}

unsigned int GenRandomInt(unsigned int cmask)
{
	DBG_LOG_IN();
	unsigned int bit_level = 0;
	unsigned int cmask_cpy = cmask;
	while (cmask_cpy != 0) {cmask_cpy >>= 1; bit_level++;}	
	while (bit_level != 0) {cmask_cpy <<= 1; cmask_cpy |= 1; bit_level--;}
	unsigned int ret = 0;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	ret |= timeinfo.tm_sec;
	ret <<= 3;
	ret |= timeinfo.tm_wday;
	ret <<= 3;
	ret |= timeinfo.tm_min;
	ret <<= 3;
	ret |= timeinfo.tm_hour;
	ret <<= 3;
	ret |= timeinfo.tm_mday;
	ret <<= 3;
	ret |= timeinfo.tm_mon;
	ret <<= 3;
	ret |= timeinfo.tm_yday;
	struct timespec nanotime;
	long int now_nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	now_nanotime = nanotime.tv_nsec;
	//printf("nano:%u\n", (unsigned int)now_nanotime);
	ret ^= (unsigned int)now_nanotime;
	//now_nanotime = now_nanotime >> 8;
	//ret ^= (cmask_cpy & now_nanotime);
	//now_nanotime = now_nanotime >> 8;
	//ret ^= (cmask_cpy & now_nanotime);
	ret &= cmask_cpy;
	while (ret > cmask) ret = ret >> 1;
	//printf("nano:%u < %i < %i\n", (unsigned int)ret, cmask, cmask_cpy);
	DBG_LOG_OUT();
	return ret;
}

unsigned int GetModuleType(char *Buff, int iLen)
{
	//DBG_LOG_IN();
	
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "EXTERNAL") == 1) return MODULE_TYPE_EXTERNAL;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "USBIO") == 1) return MODULE_TYPE_USB_GPIO;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "GPIO") == 1) return MODULE_TYPE_GPIO;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "TEMPSENSOR") == 1) return MODULE_TYPE_TEMP_SENSOR;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "ADS1015") == 1) return MODULE_TYPE_ADS1015;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "MCP3421") == 1) return MODULE_TYPE_MCP3421;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "AS5600") == 1) return MODULE_TYPE_AS5600;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "HMC5883L") == 1) return MODULE_TYPE_HMC5883L;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "PN532") == 1) return MODULE_TYPE_PN532;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "REALTIMECLOCK") == 1) return MODULE_TYPE_RTC;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "RADIO") == 1) return MODULE_TYPE_RADIO;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "SPEAKER") == 1) return MODULE_TYPE_SPEAKER;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "MICROPHONE") == 1) return MODULE_TYPE_MIC;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "DISPLAY") == 1) return MODULE_TYPE_DISPLAY;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "CAMERA") == 1) return MODULE_TYPE_CAMERA;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "RS485") == 1) return MODULE_TYPE_RS485;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "RC522") == 1) return MODULE_TYPE_RC522;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "TFP625A") == 1) return MODULE_TYPE_TFP625A;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "TIMER") == 1) return MODULE_TYPE_TIMER;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "SYSTEM") == 1) return MODULE_TYPE_SYSTEM;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "IR_RECEIVER") == 1) return MODULE_TYPE_IR_RECEIVER;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "COUNTER") == 1) return MODULE_TYPE_COUNTER;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "MEMORY") == 1) return MODULE_TYPE_MEMORY;
	if (SearchStrInDataCaseIgn(Buff, iLen, 0, "KEYBOARD") == 1) return MODULE_TYPE_KEYBOARD;
			
	//DBG_LOG_OUT();
	return MODULE_TYPE_UNKNOWN;
}

char* GetActionCodeName(int iCode, char* cBuff, int iBufflen, int iMode)
{
	unsigned char *pCode = (unsigned char*)&iCode;
	if ((iMode == 4) && (cBuff) && iBufflen)
	{
		if (((pCode[0] > 32) && (pCode[0] < 128) &&
			(pCode[1] > 32) && (pCode[1] < 128) &&
			(pCode[2] > 32) && (pCode[2] < 128) &&
			(pCode[3] > 32) && (pCode[3] < 128))
			|| ((iCode > SYSTEM_CMD_NULL) && (iCode < SYSTEM_CMD_MAX)))
				return "";
		memset(cBuff, 0, iBufflen);
		sprintf(cBuff, "%i", iCode);
		return cBuff;		
	}
	switch(iCode)
	{
		case MENU_PAGE_MAIN:				return "M_MAIN";
		case MENU_PAGE_CAMERA_LIST:			return "M_CAMERA_LIST";
		case MENU_PAGE_RING_LIST:			return "M_RING_LIST";
		case MENU_PAGE_CAMERA_OFF_ACT:		return "M_CAMERA_OFF_ACT";
		case MENU_PAGE_FOTO_ACT:			return "M_FOTO_ACT";
		case MENU_PAGE_MEDIA_ACT:			return "M_MEDIA_ACT";
		case MENU_PAGE_RADIO_ACT:			return "M_RADIO_ACT";
		case MENU_PAGE_MEDIA_OFF:			return "M_MEDIA_OFF";
		case MENU_PAGE_MODULE_LIST:			return "M_MODULE_LIST";
		case MENU_PAGE_MODULE_ACT:			return "M_MODULE_ACT";
		case MENU_PAGE_MAIN_VOLUME:			return "M_MAIN_VOLUME";
		case MENU_PAGE_RING_VOLUME:			return "M_RING_VOLUME";
		case MENU_PAGE_MESSAGE_LIST:		return "M_MESSAGE_LIST";
		case MENU_PAGE_CAMERA_ACT:			return "M_CAMERA_ACT";
		case MENU_PAGE_CAMERA_EXP:			return "M_CAMERA_EXP";
		case MENU_PAGE_WIDGET_LIST:			return "M_WIDGET_LIST";
		case MENU_PAGE_DIRECTORIES:			return "M_DIRECTORIES";
		case MENU_PAGE_FILES:				return "M_FILES";
		case MENU_PAGE_STREAMTYPE_LIST:		return "M_STREAMTYPE_LIST";
		case MENU_PAGE_SYSTEM_ACT:			return "M_SYSTEM_ACT";
		case MENU_PAGE_STREAM_LIST:			return "M_STREAM_LIST";
		case MENU_PAGE_ACTION_LIST:			return "M_ACTION_LIST";
		case MENU_PAGE_SYS_ACTION_LIST:		return "M_SYS_ACTION_LIST";
		case MENU_PAGE_PLAY_LIST:			return "M_PLAY_LIST";
		case MENU_PAGE_DIR_LIST:			return "M_DIR_LIST";
		case MODULE_COUNTER_RESET:			return "COUNTER_RESET";
		case MODULE_COUNTER_SET:			return "COUNTER_SET";
		case MODULE_COUNTER_INCREMENT:		return "COUNTER_INCREMENT";
		case MODULE_COUNTER_DECREMENT:		return "COUNTER_DECREMENT";
		case MODULE_COUNTER_SET_VALUE:		return "COUNTER_SET_VALUE";
		case MODULE_COUNTER_SET_INC:		return "COUNTER_SET_INC";
		case MODULE_COUNTER_SET_DEC:		return "COUNTER_SET_DEC";
		case MODULE_COUNTER_SET_N_VALUE:	return "COUNTER_SET_N_VALUE";
		case MODULE_COUNTER_INC_N_VALUE:	return "COUNTER_INC_N_VALUE";
		case MODULE_COUNTER_DEC_N_VALUE:	return "COUNTER_DEC_N_VALUE";
		case SYSTEM_CMD_CLOCK_ALARM_ON:		return "SYS_CLOCK_ALARM_ON";
		case SYSTEM_CMD_CLOCK_ALARM_OFF:	return "SYS_CLOCK_ALARM_OFF";
		case SYSTEM_CMD_MENU_KEY_ON:		return "SYS_MENU_KEY_ON";
		case SYSTEM_CMD_MENU_KEY_OFF:		return "SYS_MENU_KEY_OFF";
		case SYSTEM_CMD_MENU_KEY_UP:		return "SYS_MENU_KEY_UP";
		case SYSTEM_CMD_MENU_KEY_DOWN:		return "SYS_MENU_KEY_DOWN";
		case SYSTEM_CMD_MENU_KEY_LEFT:		return "SYS_MENU_KEY_LEFT";
		case SYSTEM_CMD_MENU_KEY_RIGHT:		return "SYS_MENU_KEY_RIGHT";
		case SYSTEM_CMD_MENU_KEY_OK:		return "SYS_MENU_KEY_OK";
		case SYSTEM_CMD_MENU_KEY_BACK:		return "SYS_MENU_KEY_BACK";		
		case SYSTEM_CMD_MENU_KEY_MENU:		return "SYS_MENU_KEY_MENU";
		case SYSTEM_CMD_SET_MESSAGE:		return "SYS_SET_MESSAGE";
		case SYSTEM_CMD_CLEAR_MESSAGE:		return "SYS_CLEAR_MESSAGE";
		case SYSTEM_CMD_DISPLAY_SET_TEXT:	return "SYS_DISPLAY_SET_TEXT";
		case SYSTEM_CMD_DISPLAY_CLEAR_TEXT:	return "SYS_DISPLAY_CLEAR_TEXT";
		
		case SYSTEM_CMD_EMAIL:				return "SYS_EMAIL";
		case SYSTEM_CMD_DOOR_ALARM_ON:		return "SYS_DOOR_ALARM_ON";
		case SYSTEM_CMD_SHOW_FULL_NEXT:		return "SYS_SHOW_FULL_NEXT";
		case SYSTEM_CMD_SHOW_NEXT:			return "SYS_SHOW_NEXT";
		case SYSTEM_CMD_SHOW_PREV:			return "SYS_SHOW_PREV";
		case SYSTEM_CMD_SHOW_PLAY:			return "SYS_SHOW_PLAY";
		case SYSTEM_CMD_SHOW_INFO:			return "SYS_SHOW_INFO";
		case SYSTEM_CMD_SHOW_EVENT_TEXT:	return "SYS_SHOW_EVENT_TEXT";
		
		case SYSTEM_CMD_SHOW_PAUSE:			return "SYS_SHOW_PAUSE";
		case SYSTEM_CMD_SHOW_FORWARD:		return "SYS_SHOW_FORWARD";
		case SYSTEM_CMD_SHOW_BACKWARD:		return "SYS_SHOW_BACKWARD";
		case SYSTEM_CMD_SHOW_AGAIN:			return "SYS_SHOW_AGAIN";
		case SYSTEM_CMD_SHOW_STOP_AV:		return "SYS_SHOW_STOP_AV";
		case SYSTEM_CMD_SHOW_STOP_VIDEO:	return "SYS_SHOW_STOP_VIDEO";
		case SYSTEM_CMD_SHOW_STOP_AUDIO:	return "SYS_SHOW_STOP_AUDIO";
		case SYSTEM_CMD_MENU_NEXT:			return "SYS_CMD_MENU_NEXT";
		case SYSTEM_CMD_MENU_PREV:			return "SYS_CMD_MENU_PREV";
		
		case SYSTEM_CMD_EXIT:				return "SYS_EXIT";
		case SYSTEM_CMD_CLOSE:				return "SYS_CLOSE";		
		case SYSTEM_CMD_REBOOT:				return "SYS_REBOOT";		
		case SYSTEM_CMD_SHUTDOWN:			return "SYS_SHUTDOWN";
		case SYSTEM_CMD_RESTART:			return "SYS_RESTART";
		case SYSTEM_CMD_LOST:				return "SYS_LOST";
		case SYSTEM_CMD_FIND:				return "SYS_FIND";
		case SYSTEM_CMD_NEW:				return "SYS_NEW";
		case SYSTEM_CMD_SAVE_REC_NORM:		return "SYS_SAVE_REC_NORM";
		case SYSTEM_CMD_SAVE_REC_SLOW:		return "SYS_SAVE_REC_SLOW";
		case SYSTEM_CMD_SAVE_REC_DIFF:		return "SYS_SAVE_REC_DIFF";
		case SYSTEM_CMD_SAVE_REC_AUD:		return "SYS_SAVE_REC_AUD";		
		case SYSTEM_CMD_SAVE_COPY_REC_NORM:	return "SYS_SAVE_COPY_REC_NORM";
		case SYSTEM_CMD_SAVE_COPY_REC_SLOW:	return "SYS_SAVE_COPY_REC_SLOW";
		case SYSTEM_CMD_SAVE_COPY_REC_DIFF:	return "SYS_SAVE_COPY_REC_DIFF";
		case SYSTEM_CMD_SAVE_COPY_REC_AUD:	return "SYS_SAVE_COPY_REC_AUD";		
		case SYSTEM_CMD_RADIO_VOLUME_DOWN:	return "SYS_RADIO_VOLUME_DOWN";
		case SYSTEM_CMD_RADIO_VOLUME_UP:	return "SYS_RADIO_VOLUME_UP";
		case SYSTEM_CMD_RADIO_STATION_NEXT:	return "SYS_RADIO_STATION_NEXT";
		case SYSTEM_CMD_RADIO_STATION_PREV:	return "SYS_RADIO_STATION_PREV";
		case SYSTEM_CMD_ALARM_VOLUME_DEC:	return "SYS_ALARM_VOLUME_DEC";
		case SYSTEM_CMD_ALARM_VOLUME_INC:	return "SYS_ALARM_VOLUME_INC";
		case SYSTEM_CMD_ALARM_VOLUME_DOWN:	return "SYS_ALARM_VOLUME_DOWN";
		case SYSTEM_CMD_ALARM_VOLUME_UP:	return "SYS_ALARM_VOLUME_UP";
		case SYSTEM_CMD_ALARM_VOLUME_SET:	return "SYS_ALARM_VOLUME_SET";
		case SYSTEM_CMD_SYS_SOUND_PLAY:		return "SYS_SOUND_PLAY";
		case SYSTEM_CMD_RESET_TIMER:		return "SYS_RESET_TIMER";		
		case SYSTEM_CMD_SOUND_VOLUME_DEC:	return "SYS_SOUND_VOLUME_DEC";
		case SYSTEM_CMD_SOUND_VOLUME_INC:	return "SYS_SOUND_VOLUME_INC";
		case SYSTEM_CMD_SOUND_VOLUME_DOWN:	return "SYS_SOUND_VOLUME_DOWN";
		case SYSTEM_CMD_SOUND_VOLUME_UP:	return "SYS_SOUND_VOLUME_UP";
		case SYSTEM_CMD_SOUND_VOLUME_MUTE:	return "SYS_SOUND_VOLUME_MUTE";
		case SYSTEM_CMD_SOUND_VOLUME_SET:	return "SYS_SOUND_VOLUME_SET";
		case SYSTEM_CMD_MIC_VOLUME_SET:		return "SYS_MIC_VOLUME_SET";
		case SYSTEM_CMD_MIC_VOLUME_DEC:		return "SYS_MIC_VOLUME_DEC";
		case SYSTEM_CMD_MIC_VOLUME_INC:		return "SYS_MIC_VOLUME_INC";
		case SYSTEM_CMD_MIC_VOLUME_DOWN:	return "SYS_MIC_VOLUME_DOWN";
		case SYSTEM_CMD_MIC_VOLUME_UP:		return "SYS_MIC_VOLUME_UP";
		case SYSTEM_CMD_SOFT_VOLUME_SET:	return "SYS_SOFT_VOLUME_SET";
		case SYSTEM_CMD_SOFT_VOLUME_STEP:	return "SYS_SOFT_VOLUME_STEP";
		case SYSTEM_CMD_RADIO_ON:			return "SYS_RADIO_ON";
		case SYSTEM_CMD_RADIO_OFF:			return "SYS_RADIO_OFF";
		case SYSTEM_CMD_ACTION_ON:			return "SYS_ACTION_ON";
		case SYSTEM_CMD_ACTION_OFF:			return "SYS_ACTION_OFF";
		case SYSTEM_CMD_ACTION_TEMP_OFF:	return "SYS_ACTION_TEMP_OFF";
		case SYSTEM_CMD_SET_ACCESS_LEVEL:	return "SYS_SET_ACCESS_LEVEL";
		case SYSTEM_CMD_SKIP:				return "SYS_SKIP";
		case SYSTEM_CMD_STREAM_ON:			return "SYS_STREAM_ON";
		case SYSTEM_CMD_STREAM_OFF:			return "SYS_STREAM_OFF";
		case SYSTEM_CMD_STREAM_ON_LAST:		return "SYS_STREAM_ON_LAST";
		case SYSTEM_CMD_STREAM_ON_NEXT:		return "SYS_STREAM_ON_NEXT";
		case SYSTEM_CMD_STREAM_ON_PREV:		return "SYS_STREAM_ON_PREV";
		case SYSTEM_CMD_STREAM_TYPE_ON:		return "SYS_STREAM_TYPE_ON";
		case SYSTEM_CMD_STREAM_TYPE_RND_ON:	return "SYS_STREAM_TYPE_RND_ON";
		case SYSTEM_CMD_STREAM_RND_ON:		return "SYS_STREAM_RND_ON";
		case SYSTEM_CMD_PLAY_DIR:			return "SYS_PLAY_DIR";
		case SYSTEM_CMD_PLAY_FILE:			return "SYS_PLAY_FILE";
		case SYSTEM_CMD_PLAY_DIR_RND_FILE:	return "SYS_PLAY_DIR_RND_FILE";
		case SYSTEM_CMD_SET_SHOW_PATH:		return "SYS_SET_SHOW_PATH";		
		case SYSTEM_CMD_SET_PLAYLIST_POS:	return "SYS_SET_PLAYLIST_POS";
		case SYSTEM_CMD_SET_DIRLIST_POS:	return "SYS_SET_DIRLIST_POS";		
		case SYSTEM_CMD_PLAY_NEXT_DIR:		return "SYS_PLAY_NEXT_DIR";
		case SYSTEM_CMD_PLAY_PREV_DIR:		return "SYS_PLAY_PREV_DIR";
		case SYSTEM_CMD_REDIRECT:			return "SYS_REDIRECT";
		case SYSTEM_CMD_RANDOM_FILE_ON:		return "SYS_RANDOM_FILE_ON";
		case SYSTEM_CMD_RANDOM_FILE_OFF:	return "SYS_RANDOM_FILE_OFF";
		case SYSTEM_CMD_RANDOM_DIR_ON:		return "SYS_RANDOM_DIR_ON";
		case SYSTEM_CMD_RANDOM_DIR_OFF:		return "SYS_RANDOM_DIR_OFF";
		case SYSTEM_CMD_CLIENT_STOP:		return "SYS_CLIENT_STOP";
		case SYSTEM_CMD_FULL_STOP:			return "SYS_FULL_STOP";
		case SYSTEM_CMD_EVENT_TIMER:		return "SYS_EVENT_TIMER";
		case SYSTEM_CMD_EVENT_START:		return "SYS_EVENT_START";
		case SYSTEM_CMD_EVENT_MODULE:		return "SYS_EVENT_MODULE";
		case SYSTEM_CMD_APP_UPDATE:			return "SYS_APP_UPDATE";
		
		
		case SYSTEM_CMD_WIDGET_STATUS_OFF:	return "SYS_WIDGET_STATUS_OFF";
		case SYSTEM_CMD_WIDGET_STATUS_ALLWAYS:	return "SYS_WIDGET_STATUS_ALLWAYS";
		case SYSTEM_CMD_WIDGET_STATUS_MENU:	return "SYS_WIDGET_STATUS_MENU";
		case SYSTEM_CMD_WIDGET_STATUS_TIMEOUT:	return "SYS_WIDGET_STATUS_TIMEOUT";
		case SYSTEM_CMD_WIDGET_TIMEDOWN:	return "SYS_WIDGET_TIMEDOWN";
		case SYSTEM_CMD_WIDGET_TIMEUP:		return "SYS_WIDGET_TIMEUP";
		case SYSTEM_CMD_WIDGET_UPDATE:		return "SYS_WIDGET_UPDATE";
		case SYSTEM_CMD_SHOW_MENU:			return "SYS_SHOW_MENU";
		case SYSTEM_CMD_HIDE_MENU:			return "SYS_HIDE_MENU";
		case SYSTEM_CMD_PLAY_YOUTUBE_JWZ:	return "SYS_PLAY_YOUTUBE_JWZ";
		case SYSTEM_CMD_PLAY_YOUTUBE_DL:	return "SYS_PLAY_YOUTUBE_DL";
		case SYSTEM_CMD_PLAY_NEW_POS:		return "SYS_PLAY_NEW_POS";
		case SYSTEM_CMD_MENU_FORWARD:		return "SYS_MENU_FORWARD";
		case SYSTEM_CMD_MENU_BACKWARD:		return "SYS_MENU_BACKWARD";
		case SYSTEM_CMD_SET_SHOW_MODE:		return "SYS_SET_SHOW_MODE";
		case SYSTEM_CMD_SET_SHOW_STATUS:	return "SYS_SET_SHOW_STATUS";		
		case SYSTEM_CMD_SET_ZOOM_MODE:		return "SYS_SET_ZOOM_MODE";
		case SYSTEM_CMD_CAMLIST_CLEAR:		return "SYS_CAMLIST_CLEAR";
		case SYSTEM_CMD_CAMLIST_ADD:		return "SYS_CAMLIST_ADD";
		case SYSTEM_CMD_CAMLIST_SHOW:		return "SYS_CAMLIST_SHOW";
		case SYSTEM_CMD_CAMERA_ERROR:		return "SYS_CAMERA_ERROR";
		case SYSTEM_CMD_VIDEO_ERROR:		return "SYS_VIDEO_ERROR";
		case SYSTEM_CMD_AUDIO_ERROR:		return "SYS_AUDIO_ERROR";
		case SYSTEM_CMD_OPENED_RTSP:		return "SYS_OPENED_RTSP";
		case SYSTEM_CMD_OPENED_CAMERA:		return "SYS_OPENED_CAMERA";
		case SYSTEM_CMD_OPENED_FILE:		return "SYS_OPENED_FILE";
		case SYSTEM_CMD_CLOSED_FILE:		return "SYS_CLOSED_FILE";
		case SYSTEM_CMD_DONE_FILE:			return "SYS_DONE_FILE";
		case SYSTEM_CMD_NEW_FILE_POS:		return "SYS_NEW_FILE_POS";
		case SYSTEM_CMD_ERROR_FILE:			return "SYS_ERROR_FILE";		
		case SYSTEM_CMD_PLAY_VIDEO:			return "SYS_PLAY_VIDEO";
		case SYSTEM_CMD_PLAY_AUDIO:			return "SYS_PLAY_AUDIO";
		case SYSTEM_CMD_STOPED_VIDEO:		return "SYS_STOPED_VIDEO";
		case SYSTEM_CMD_STOPED_AUDIO:		return "SYS_STOPED_AUDIO";
		case SYSTEM_CMD_OPENED_MIC:			return "SYS_OPENED_MIC";
		case SYSTEM_CMD_CLOSED_RTSP:		return "SYS_CLOSED_RTSP";
		case SYSTEM_CMD_CLOSED_CAMERA:		return "SYS_CLOSED_CAMERA";
		case SYSTEM_CMD_CLOSED_MIC:			return "SYS_CLOSED_MIC";
		case SYSTEM_CMD_TIMERS_INCREASE:	return "SYS_TIMERS_INCREASE";
		case SYSTEM_CMD_TIMERS_DECREASE:	return "SYS_TIMERS_DECREASE";
		case SYSTEM_CMD_TIMERS_UPDATE:		return "SYS_TIMERS_UPDATE";
		case SYSTEM_CMD_RESET_STATUS_NUM: 	return "SYS_RESET_STATUS_NUM";
		case SYSTEM_CMD_SET_STATUS_NUM:		return "SYS_SET_STATUS_NUM";
		case SYSTEM_CMD_VALUE_STATUS_NUM:	return "SYS_VALUE_STATUS_NUM";
		case SYSTEM_CMD_BUSY_FILE:			return "SYS_BUSY_FILE";
		case SYSTEM_CMD_FREE_FILE:			return "SYS_FREE_FILE";
		case SYSTEM_CMD_EVENT_SECONDS:		return "SYS_EVENT_SECONDS";
		case SYSTEM_CMD_EVENT_MINUTES:		return "SYS_EVENT_MINUTES";
		case SYSTEM_CMD_EVENT_HOURS:		return "SYS_EVENT_HOURS";		
		
		case SYSTEM_CMD_RESET_STATUS_MESSAGE: return "SYS_RESET_STATUS_MESSAGE";
		case SYSTEM_CMD_SET_STATUS_MESSAGE: return "SYS_SET_STATUS_MESSAGE";
		case SYSTEM_CMD_EXEC_EXT_APP_NOW: 	return "SYS_EXEC_EXT_APP_NOW";
		case SYSTEM_CMD_EXEC_EXT_APP_EXIT: 	return "SYS_EXEC_EXT_APP_EXIT";
		
		case EXPOSURE_OFF: 					return "EXPOSURE_OFF";
		case EXPOSURE_AUTO: 				return "EXPOSURE_AUTO";
		case EXPOSURE_NIGHT: 				return "EXPOSURE_NIGHT";
		case EXPOSURE_BACKLIGHT: 			return "EXPOSURE_BACKLIGHT";
		case EXPOSURE_SPOTLIGHT: 			return "EXPOSURE_SPOTLIGHT";
		case EXPOSURE_SPORTS: 				return "EXPOSURE_SPORTS";
		case EXPOSURE_SNOW: 				return "EXPOSURE_SNOW";
		case EXPOSURE_BEACH: 				return "EXPOSURE_BEACH";
		case EXPOSURE_LARGEAPERTURE: 		return "EXPOSURE_LARGEAPERTURE";
		case EXPOSURE_SMALLAPERTURE: 		return "EXPOSURE_SMALLAPERTURE";
		case EXPOSURE_VERYLONG: 			return "EXPOSURE_VERYLONG";
		case EXPOSURE_FIXEDFPS: 			return "EXPOSURE_FIXEDFPS";
		case EXPOSURE_NIGHTWITHPREVIEW: 	return "EXPOSURE_NIGHTWITHPREVIEW";
		case EXPOSURE_ANTISHAKE: 			return "EXPOSURE_ANTISHAKE";
		case EXPOSURE_FIREWORKS: 			return "EXPOSURE_FIREWORKS";
		
		case FILTER_NONE: 					return "FILTER_NONE";
		case FILTER_NOISE: 					return "FILTER_NOISE";
		case FILTER_EMBOSS: 				return "FILTER_EMBOSS";
		case FILTER_NEGATIVE: 				return "FILTER_NEGATIVE";
		case FILTER_SKETCH: 				return "FILTER_SKETCH";
		case FILTER_OILPAINT: 				return "FILTER_OILPAINT";
		case FILTER_HATCH: 					return "FILTER_HATCH";
		case FILTER_GPEN: 					return "FILTER_GPEN";
		case FILTER_ANTIALIAS: 				return "FILTER_ANTIALIAS";
		case FILTER_DERING: 				return "FILTER_DERING";
		case FILTER_SOLARISE: 				return "FILTER_SOLARISE";
		case FILTER_WATERCOLOR: 			return "FILTER_WATERCOLOR";
		case FILTER_PASTEL: 				return "FILTER_PASTEL";
		case FILTER_SHARPEN: 				return "FILTER_SHARPEN";
		case FILTER_FILM: 					return "FILTER_FILM";
		case FILTER_BLUR: 					return "FILTER_BLUR";
		case FILTER_SATURATION: 			return "FILTER_SATURATION";
		case FILTER_DEINTERLACELINEDOUBLE: 	return "FILTER_DEINTERLACELINEDOUBLE";
		case FILTER_DEINTERLACEADVANCED: 	return "FILTER_DEINTERLACEADVANCED";
		case FILTER_COLOURSWAP: 			return "FILTER_COLOURSWAP";
		case FILTER_WASHEDOUT: 				return "FILTER_WASHEDOUT";
		case FILTER_COLOURPOINT: 			return "FILTER_COLOURPOINT";
		case FILTER_POSTERIZE: 				return "FILTER_POSTERIZE";
		case FILTER_COLOURBALANCE: 			return "FILTER_COLOURBALANCE";
		case FILTER_CARTOON: 				return "FILTER_CARTOON";
		case FILTER_ANAGLYPH: 				return "FILTER_ANAGLYPH";
		case FILTER_DEINTERLACEFAST: 		return "FILTER_DEINTERLACEFAST";
		
		case WBALANCE_OFF: 					return "WBALANCE_OFF";
		case WBALANCE_AUTO: 				return "WBALANCE_AUTO";
		case WBALANCE_SUNLIGHT: 			return "WBALANCE_SUNLIGHT";
		case WBALANCE_CLOUDY: 				return "WBALANCE_CLOUDY";
		case WBALANCE_SHADE: 				return "WBALANCE_SHADE";
		case WBALANCE_TUNGSTEN: 			return "WBALANCE_TUNGSTEN";
		case WBALANCE_FLUORESCENT: 			return "WBALANCE_FLUORESCENT";
		case WBALANCE_INCANDESCENT: 		return "WBALANCE_INCANDESCENT";
		case WBALANCE_FLASH: 				return "WBALANCE_FLASH";
		case WBALANCE_HORIZON: 				return "WBALANCE_HORIZON";
	
		default:
			if (iMode == 5) return "0";
			if ((!cBuff) || (!iBufflen)) return "0";
			if (iMode)
			{
				
				if (((pCode[0] > 32) && (pCode[0] < 128))
					&& (((pCode[1] > 32) && (pCode[1] < 128)) || ((pCode[1] == 0) && (pCode[2] == 0) && (pCode[3] == 0)))
					&& (((pCode[2] > 32) && (pCode[2] < 128)) || ((pCode[2] == 0) && (pCode[3] == 0)))
					&& (((pCode[3] > 32) && (pCode[3] < 128)) || (pCode[3] == 0)))
				{
					memset(cBuff, 0, iBufflen);
					memcpy(cBuff, pCode, 4);
					return cBuff;
				} 
				else 
				{
					if (iMode == 1) return "";
				}
			}			
			memset(cBuff, 0, iBufflen);
			sprintf(cBuff, "%i", iCode);
			return cBuff;			
			break;
	}
	if ((!cBuff) || (!iBufflen)) return "0";
	memset(cBuff, 0, iBufflen);
	memcpy(cBuff, pCode, 4);
	return cBuff;
}

char* GetAfterAcceptName(int iCode)
{
	switch(iCode)
	{
		case PA_STOP: return "PA_STOP";
		case PA_OFF: return "PA_OFF";
		case (PA_OFF | PA_STOP): return "PA_OFF,PA_STOP";
		default:
			return "";
	}
	return "";
}

char* GetModuleSubTypeName(int iCode)
{
	switch(iCode)
	{
		case MODULE_SUBTYPE_IO:
			return "IO";
		case MODULE_SUBTYPE_GATE:
			return "GATE";
		case MODULE_SUBTYPE_PTZ:
			return "PTZ";
		default:
			return "";
	}
	return "";
}

char* GetModuleTypeName(int iCode)
{
	switch(iCode)
	{
		case MODULE_TYPE_TFP625A:
			return "TFP625A";
		case MODULE_TYPE_EXTERNAL:
			return "EXTERNAL";
		case MODULE_TYPE_USB_GPIO:
			return "USBIO";
		case MODULE_TYPE_GPIO:
			return "GPIO";
		case MODULE_TYPE_TEMP_SENSOR:
			return "TEMPSENSOR";
		case MODULE_TYPE_AS5600:
			return "AS5600";
		case MODULE_TYPE_HMC5883L:
			return "HMC5883L";
		case MODULE_TYPE_MCP3421:
			return "MCP3421";
		case MODULE_TYPE_ADS1015:
			return "ADS1015";
		case MODULE_TYPE_PN532:
			return "PN532";
		case MODULE_TYPE_RTC:
			return "REALTIMECLOCK";
		case MODULE_TYPE_RADIO:
			return "RADIO";
		case MODULE_TYPE_SPEAKER:
			return "SPEAKER";
		case MODULE_TYPE_MIC:
			return "MICROPHONE";
		case MODULE_TYPE_DISPLAY:
			return "DISPLAY";			
		case MODULE_TYPE_CAMERA:
			return "CAMERA";
		case MODULE_TYPE_RS485:
			return "RS485";
		case MODULE_TYPE_RC522:
			return "RC522";
		case MODULE_TYPE_TIMER:
			return "TIMER";
		case MODULE_TYPE_SYSTEM:
			return "SYSTEM";
		case MODULE_TYPE_IR_RECEIVER:
			return "IR_RECEIVER";
		case MODULE_TYPE_COUNTER:
			return "COUNTER";
		case MODULE_TYPE_MEMORY:
			return "MEMORY";
		case MODULE_TYPE_KEYBOARD:
			return "KEYBOARD";		
		default:
			return "UNKNOWN";
	}
	return "UNKNOWN";
}

char* GetModuleTypeShowName(int iCode)
{
	switch(iCode)
	{
		case MODULE_TYPE_TFP625A:
			return "Считыватель пальца";
		case MODULE_TYPE_EXTERNAL:
			return "Внешние";
		case MODULE_TYPE_USB_GPIO:
			return "Расширенные управляющие";
		case MODULE_TYPE_GPIO:
			return "Управляющие";
		case MODULE_TYPE_TEMP_SENSOR:
			return "Датчики тепмературы";
		case MODULE_TYPE_ADS1015:
			return "Датчики ADC ADS1015";
		case MODULE_TYPE_MCP3421:
			return "Датчики ADC MCP3421";
		case MODULE_TYPE_AS5600:
			return "Датчики угла AS5600";
		case MODULE_TYPE_HMC5883L:
			return "Датчики положения HMC5883L";
		case MODULE_TYPE_PN532:
			return "Считыватели карт PN532";
		case MODULE_TYPE_RTC:
			return "Чысы";
		case MODULE_TYPE_RADIO:
			return "Радио";
		case MODULE_TYPE_SPEAKER:
			return "Колонки";
		case MODULE_TYPE_MIC:
			return "Микрофоны";
		case MODULE_TYPE_DISPLAY:
			return "Дисплеи";			
		case MODULE_TYPE_CAMERA:
			return "Камеры";
		case MODULE_TYPE_RS485:
			return "Интерфейсы RS485";
		case MODULE_TYPE_RC522:
			return "Считыватели карт RC522";
		case MODULE_TYPE_TIMER:
			return "Таймеры";
		case MODULE_TYPE_SYSTEM:
			return "Системы";
		case MODULE_TYPE_IR_RECEIVER:
			return "ИК приемники";
		case MODULE_TYPE_COUNTER:
			return "Счетчики";
		case MODULE_TYPE_MEMORY:
			return "Память";
		case MODULE_TYPE_KEYBOARD:
			return "Клавиатуры";
		default:
			return "Неизвестные";
	}
	return "Неизвестные";
}

int GetFirstMicDevNum()
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	int dev = -1;
	int i;
	for (i = 0; i < iModuleCnt; i++)
		if ((miModuleList[i].Local == 1) && 
			(miModuleList[i].Type == MODULE_TYPE_MIC) &&
			(miModuleList[i].Enabled & 1))
			{
				dev = miModuleList[i].Settings[1];
				break;
			}	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	return dev;		
}							

unsigned int GetModuleSettings(char *Buff, int iLen, char iInt)
{
	DBG_LOG_IN();
	if (iLen == 0) 
	{
		DBG_LOG_OUT();
		return 0;	
	}
	
	unsigned int ret = 0;
	unsigned int iID = 0;	
	if (iLen > 4) memcpy(&iID, Buff, 4); else memcpy(&iID, Buff, iLen);
	if (!iInt) UpperTextLimit(Buff, iLen);
	
	if (SearchStrInData(Buff, iLen, 0, "[WEB]") != 0) 			ret |= ACCESS_WEB;
	if (SearchStrInData(Buff, iLen, 0, "[RTSP]") != 0) 			ret |= ACCESS_RTSP;
	if (SearchStrInData(Buff, iLen, 0, "[CONT]") != 0) 			ret |= ACCESS_CONTROL;
	if (SearchStrInData(Buff, iLen, 0, "[YOUT]") != 0) 			ret |= ACCESS_YOUTUBE;
	if (SearchStrInData(Buff, iLen, 0, "[MED]") != 0) 			ret |= ACCESS_MEDIA;
	if (SearchStrInData(Buff, iLen, 0, "[USER]") != 0) 			ret |= ACCESS_USERS;
	if (SearchStrInData(Buff, iLen, 0, "[MENU]") != 0) 			ret |= ACCESS_MENU;
	if (SearchStrInData(Buff, iLen, 0, "[MODL]") != 0) 			ret |= ACCESS_MODULES;
	if (SearchStrInData(Buff, iLen, 0, "[ALRM]") != 0) 			ret |= ACCESS_ALARMS;
	if (SearchStrInData(Buff, iLen, 0, "[SNDS]") != 0) 			ret |= ACCESS_SOUNDS;
	if (SearchStrInData(Buff, iLen, 0, "[MAIL]") != 0) 			ret |= ACCESS_MAILS;
	if (SearchStrInData(Buff, iLen, 0, "[STRT]") != 0) 			ret |= ACCESS_STREAMTYPES;
	if (SearchStrInData(Buff, iLen, 0, "[STRM]") != 0) 			ret |= ACCESS_STREAMS;
	if (SearchStrInData(Buff, iLen, 0, "[WIDS]") != 0) 			ret |= ACCESS_WIDGETS;
	if (SearchStrInData(Buff, iLen, 0, "[DIRS]") != 0) 			ret |= ACCESS_DIRECTORIES;
	if (SearchStrInData(Buff, iLen, 0, "[EACT]") != 0) 			ret |= ACCESS_EVNTACTIONS;
	if (SearchStrInData(Buff, iLen, 0, "[MACT]") != 0) 			ret |= ACCESS_MNLACTIONS;
	if (SearchStrInData(Buff, iLen, 0, "[KEYS]") != 0) 			ret |= ACCESS_KEYS;
	if (SearchStrInData(Buff, iLen, 0, "[IRC]") != 0) 			ret |= ACCESS_IRCODES;
	if (SearchStrInData(Buff, iLen, 0, "[RECT]") != 0) 			ret |= ACCESS_CAMRECTS;
	if (SearchStrInData(Buff, iLen, 0, "[MAN]") != 0) 			ret |= ACCESS_MANUAL;
	if (SearchStrInData(Buff, iLen, 0, "[MODS]") != 0) 			ret |= ACCESS_MODSTATUSES;
	if (SearchStrInData(Buff, iLen, 0, "[RADS]") != 0) 			ret |= ACCESS_RADIOS;
	if (SearchStrInData(Buff, iLen, 0, "[LOG]") != 0) 			ret |= ACCESS_LOG;
	if (SearchStrInData(Buff, iLen, 0, "[EXPL]") != 0) 			ret |= ACCESS_EXPLORER;
	if (SearchStrInData(Buff, iLen, 0, "[SETT]") != 0) 			ret |= ACCESS_SETTINGS;
	if (SearchStrInData(Buff, iLen, 0, "[CAM]") != 0) 			ret |= ACCESS_CAMERA;
	if (SearchStrInData(Buff, iLen, 0, "[MIC]") != 0) 			ret |= ACCESS_MIC;
	if (SearchStrInData(Buff, iLen, 0, "[SYS]") != 0) 			ret |= ACCESS_SYSTEM;
	if (SearchStrInData(Buff, iLen, 0, "[CONN]") != 0) 			ret |= ACCESS_CONNECTS;
	if (SearchStrInData(Buff, iLen, 0, "[HIST]") != 0) 			ret |= ACCESS_HISTORY;
	
	if (SearchStrInData(Buff, iLen, 0, "SR_ANY") != 0) 			ret |= SAMPLE_RATE_ANY;
	if (SearchStrInData(Buff, iLen, 0, "SR_8000") != 0) 		ret |= SAMPLE_RATE_8000;
	if (SearchStrInData(Buff, iLen, 0, "SR_11025") != 0) 		ret |= SAMPLE_RATE_11025;
	if (SearchStrInData(Buff, iLen, 0, "SR_16000") != 0) 		ret |= SAMPLE_RATE_16000;
	if (SearchStrInData(Buff, iLen, 0, "SR_22050") != 0) 		ret |= SAMPLE_RATE_22050;
	if (SearchStrInData(Buff, iLen, 0, "SR_44100") != 0) 		ret |= SAMPLE_RATE_44100;
	if (SearchStrInData(Buff, iLen, 0, "SR_48000") != 0) 		ret |= SAMPLE_RATE_48000;
	if (SearchStrInData(Buff, iLen, 0, "SR_96000") != 0) 		ret |= SAMPLE_RATE_96000;	
	
	if (SearchStrInData(Buff, iLen, 0, "M_MAIN") != 0) 			ret = MENU_PAGE_MAIN;
	if (SearchStrInData(Buff, iLen, 0, "M_CAMERA_LIST") != 0) 	ret = MENU_PAGE_CAMERA_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_RING_LIST") != 0) 	ret = MENU_PAGE_RING_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_CAMERA_OFF_ACT") != 0) ret = MENU_PAGE_CAMERA_OFF_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_FOTO_ACT") != 0) 		ret = MENU_PAGE_FOTO_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_MEDIA_ACT") != 0) 	ret = MENU_PAGE_MEDIA_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_RADIO_ACT") != 0) 	ret = MENU_PAGE_RADIO_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_MEDIA_OFF") != 0) 	ret = MENU_PAGE_MEDIA_OFF;
	if (SearchStrInData(Buff, iLen, 0, "M_MODULE_LIST") != 0) 	ret = MENU_PAGE_MODULE_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_MODULE_ACT") != 0) 	ret = MENU_PAGE_MODULE_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_MAIN_VOLUME") != 0) 	ret = MENU_PAGE_MAIN_VOLUME;
	if (SearchStrInData(Buff, iLen, 0, "M_RING_VOLUME") != 0) 	ret = MENU_PAGE_RING_VOLUME;
	if (SearchStrInData(Buff, iLen, 0, "M_MESSAGE_LIST") != 0) 	ret = MENU_PAGE_MESSAGE_LIST; 
	if (SearchStrInData(Buff, iLen, 0, "M_CAMERA_ACT") != 0) 	ret = MENU_PAGE_CAMERA_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_CAMERA_EXP") != 0) 	ret = MENU_PAGE_CAMERA_EXP;
	if (SearchStrInData(Buff, iLen, 0, "M_WIDGET_LIST") != 0) 	ret = MENU_PAGE_WIDGET_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_DIRECTORIES") != 0) 	ret = MENU_PAGE_DIRECTORIES;
	if (SearchStrInData(Buff, iLen, 0, "M_FILES") != 0) 		ret = MENU_PAGE_FILES;
	if (SearchStrInData(Buff, iLen, 0, "M_STREAMTYPE_LIST") != 0) ret = MENU_PAGE_STREAMTYPE_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_SYSTEM_ACT") != 0) 	ret = MENU_PAGE_SYSTEM_ACT;
	if (SearchStrInData(Buff, iLen, 0, "M_STREAM_LIST") != 0) 	ret = MENU_PAGE_STREAM_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_ACTION_LIST") != 0) 	ret = MENU_PAGE_ACTION_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_SYS_ACTION_LIST") != 0) ret = MENU_PAGE_SYS_ACTION_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_PLAY_LIST") != 0) 	ret = MENU_PAGE_PLAY_LIST;
	if (SearchStrInData(Buff, iLen, 0, "M_DIR_LIST") != 0) 		ret = MENU_PAGE_DIR_LIST;
	
	if (SearchStrInData(Buff, iLen, 0, "PA_STOP") != 0) 		ret |= PA_STOP;	
	if (SearchStrInData(Buff, iLen, 0, "PA_OFF") != 0) 			ret |= PA_OFF;	
		
	if (SearchStrInData(Buff, iLen, 0, "STEREO") != 0) 			ret |= MODULE_SECSET_STEREO;
	if (SearchStrInData(Buff, iLen, 0, "MONO") != 0) 			ret |= MODULE_SECSET_MONO;
	
	if (SearchStrInData(Buff, iLen, 0, "AM2320") != 0) 			ret = I2C_ADDRESS_AM2320;
	if (SearchStrInData(Buff, iLen, 0, "LM75") != 0) 			ret = I2C_ADDRESS_LM75;
	
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_OFF") == 1) 				ret = EXPOSURE_OFF;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_AUTO") == 1) 				ret = EXPOSURE_AUTO;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_NIGHT") == 1) 				ret = EXPOSURE_NIGHT;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_BACKLIGHT") == 1) 			ret = EXPOSURE_BACKLIGHT;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_SPOTLIGHT") == 1) 			ret = EXPOSURE_SPOTLIGHT;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_SPORTS") == 1) 			ret = EXPOSURE_SPORTS;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_SNOW") == 1) 				ret = EXPOSURE_SNOW;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_BEACH") == 1) 				ret = EXPOSURE_BEACH;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_LARGEAPERTURE") == 1) 		ret = EXPOSURE_LARGEAPERTURE;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_SMALLAPERTURE") == 1) 		ret = EXPOSURE_SMALLAPERTURE;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_VERYLONG") == 1) 			ret = EXPOSURE_VERYLONG;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_FIXEDFPS") == 1) 			ret = EXPOSURE_FIXEDFPS;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_NIGHTWITHPREVIEW") == 1) 	ret = EXPOSURE_NIGHTWITHPREVIEW;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_ANTISHAKE") == 1) 			ret = EXPOSURE_ANTISHAKE;
	if (SearchStrInData(Buff, iLen, 0, "EXPOSURE_FIREWORKS") == 1) 			ret = EXPOSURE_FIREWORKS;
	
	if (SearchStrInData(Buff, iLen, 0, "FILTER_NONE") == 1) 				ret = FILTER_NONE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_NOISE") == 1) 				ret = FILTER_NOISE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_EMBOSS") == 1) 				ret = FILTER_EMBOSS;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_NEGATIVE") == 1) 			ret = FILTER_NEGATIVE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_SKETCH") == 1) 				ret = FILTER_SKETCH;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_OILPAINT") == 1) 			ret = FILTER_OILPAINT;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_HATCH") == 1) 				ret = FILTER_HATCH;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_GPEN") == 1) 				ret = FILTER_GPEN;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_ANTIALIAS") == 1) 			ret = FILTER_ANTIALIAS;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_DERING") == 1) 				ret = FILTER_DERING;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_SOLARISE") == 1) 			ret = FILTER_SOLARISE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_WATERCOLOR") == 1) 			ret = FILTER_WATERCOLOR;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_PASTEL") == 1) 				ret = FILTER_PASTEL;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_SHARPEN") == 1) 				ret = FILTER_SHARPEN;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_FILM") == 1) 				ret = FILTER_FILM;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_BLUR") == 1) 				ret = FILTER_BLUR;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_SATURATION") == 1) 			ret = FILTER_SATURATION;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_DEINTERLACELINEDOUBLE") == 1) ret = FILTER_DEINTERLACELINEDOUBLE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_DEINTERLACEADVANCED") == 1) 	ret = FILTER_DEINTERLACEADVANCED;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_COLOURSWAP") == 1) 			ret = FILTER_COLOURSWAP;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_WASHEDOUT") == 1) 			ret = FILTER_WASHEDOUT;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_COLOURPOINT") == 1) 			ret = FILTER_COLOURPOINT;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_POSTERIZE") == 1) 			ret = FILTER_POSTERIZE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_COLOURBALANCE") == 1) 		ret = FILTER_COLOURBALANCE;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_CARTOON") == 1) 				ret = FILTER_CARTOON;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_ANAGLYPH") == 1) 			ret = FILTER_ANAGLYPH;
	if (SearchStrInData(Buff, iLen, 0, "FILTER_DEINTERLACEFAST") == 1) 		ret = FILTER_DEINTERLACEFAST;
		
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_OFF") == 1) 			ret = WBALANCE_OFF;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_AUTO") == 1) 			ret = WBALANCE_AUTO;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_SUNLIGHT") == 1) 		ret = WBALANCE_SUNLIGHT;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_CLOUDY") == 1) 		ret = WBALANCE_CLOUDY;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_SHADE") == 1) 			ret = WBALANCE_SHADE;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_TUNGSTEN") == 1) 		ret = WBALANCE_TUNGSTEN;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_FLUORESCENT") == 1) 	ret = WBALANCE_FLUORESCENT;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_INCANDESCENT") == 1) 	ret = WBALANCE_INCANDESCENT;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_FLASH") == 1) 			ret = WBALANCE_FLASH;
	if (SearchStrInData(Buff, iLen, 0, "WBALANCE_HORIZON") == 1) 		ret = WBALANCE_HORIZON;
	
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_RESET") == 1) 		ret = MODULE_COUNTER_RESET;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_SET") == 1) 		ret = MODULE_COUNTER_SET;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_INCREMENT") == 1) 	ret = MODULE_COUNTER_INCREMENT;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_DECREMENT") == 1) 	ret = MODULE_COUNTER_DECREMENT;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_SET_VALUE") == 1) 	ret = MODULE_COUNTER_SET_VALUE;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_SET_INC") == 1) 	ret = MODULE_COUNTER_SET_INC;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_SET_DEC") == 1) 	ret = MODULE_COUNTER_SET_DEC;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_INC_N_VALUE") == 1) ret = MODULE_COUNTER_INC_N_VALUE;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_DEC_N_VALUE") == 1) ret = MODULE_COUNTER_DEC_N_VALUE;
	if (SearchStrInData(Buff, iLen, 0, "COUNTER_SET_N_VALUE") == 1) ret = MODULE_COUNTER_SET_N_VALUE;
	
				
	if (SearchStrInData(Buff, iLen, 0, "SYS_APP_UPDATE") != 0) 			ret = SYSTEM_CMD_APP_UPDATE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_EVENT_MODULE") != 0) 		ret = SYSTEM_CMD_EVENT_MODULE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_EVENT_START") != 0) 		ret = SYSTEM_CMD_EVENT_START;
	if (SearchStrInData(Buff, iLen, 0, "SYS_EVENT_TIMER") != 0) 		ret = SYSTEM_CMD_EVENT_TIMER;
	if (SearchStrInData(Buff, iLen, 0, "SYS_DOOR_ALARM_ON") != 0) 		ret = SYSTEM_CMD_DOOR_ALARM_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOCK_ALARM_ON") != 0) 		ret = SYSTEM_CMD_CLOCK_ALARM_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOCK_ALARM_OFF") != 0) 	ret = SYSTEM_CMD_CLOCK_ALARM_OFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_ON") != 0) 		ret = SYSTEM_CMD_MENU_KEY_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_OFF") != 0) 		ret = SYSTEM_CMD_MENU_KEY_OFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_UP") != 0) 		ret = SYSTEM_CMD_MENU_KEY_UP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_DOWN") != 0) 		ret = SYSTEM_CMD_MENU_KEY_DOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_LEFT") != 0) 		ret = SYSTEM_CMD_MENU_KEY_LEFT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_RIGHT") != 0) 		ret = SYSTEM_CMD_MENU_KEY_RIGHT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_OK") != 0) 		ret = SYSTEM_CMD_MENU_KEY_OK;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_BACK") != 0) 		ret = SYSTEM_CMD_MENU_KEY_BACK;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_KEY_MENU") != 0) 		ret = SYSTEM_CMD_MENU_KEY_MENU;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_MESSAGE") != 0) 		ret = SYSTEM_CMD_SET_MESSAGE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLEAR_MESSAGE") != 0) 		ret = SYSTEM_CMD_CLEAR_MESSAGE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_DISPLAY_SET_TEXT") != 0) 	ret = SYSTEM_CMD_DISPLAY_SET_TEXT;	
	if (SearchStrInData(Buff, iLen, 0, "SYSTEM_CMD_DISPLAY_CLEAR_TEXT") != 0) 	ret = SYSTEM_CMD_DISPLAY_CLEAR_TEXT;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_EMAIL") != 0) 				ret = SYSTEM_CMD_EMAIL;
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_YOUTUBE_JWZ") != 0) 	ret = SYSTEM_CMD_PLAY_YOUTUBE_JWZ;		
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_YOUTUBE_DL") != 0) 	ret = SYSTEM_CMD_PLAY_YOUTUBE_DL;		
	if (SearchStrInData(Buff, iLen, 0, "SYS_EXIT") != 0) 				ret = SYSTEM_CMD_EXIT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHUTDOWN") != 0) 			ret = SYSTEM_CMD_SHUTDOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOSE") != 0) 				ret = SYSTEM_CMD_CLOSE;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_REBOOT") != 0) 				ret = SYSTEM_CMD_REBOOT;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_RESTART") != 0) 			ret = SYSTEM_CMD_RESTART;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_ACCESS_LEVEL") != 0) 	ret = SYSTEM_CMD_SET_ACCESS_LEVEL;
	if (SearchStrInData(Buff, iLen, 0, "SYS_LOST") != 0) 				ret = SYSTEM_CMD_LOST;
	if (SearchStrInData(Buff, iLen, 0, "SYS_FIND") != 0) 				ret = SYSTEM_CMD_FIND;
	if (SearchStrInData(Buff, iLen, 0, "SYS_NEW") != 0) 				ret = SYSTEM_CMD_NEW;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_REC_DIFF") != 0) 		ret = SYSTEM_CMD_SAVE_REC_DIFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_REC_NORM") != 0) 		ret = SYSTEM_CMD_SAVE_REC_NORM;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_REC_SLOW") != 0) 		ret = SYSTEM_CMD_SAVE_REC_SLOW;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_REC_AUD") != 0) 		ret = SYSTEM_CMD_SAVE_REC_AUD;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_COPY_REC_DIFF") != 0) 	ret = SYSTEM_CMD_SAVE_COPY_REC_DIFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_COPY_REC_NORM") != 0) 	ret = SYSTEM_CMD_SAVE_COPY_REC_NORM;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_COPY_REC_SLOW") != 0) 	ret = SYSTEM_CMD_SAVE_COPY_REC_SLOW;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SAVE_COPY_REC_AUD") != 0) 	ret = SYSTEM_CMD_SAVE_COPY_REC_AUD;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_PLAY") != 0) 			ret = SYSTEM_CMD_SYS_SOUND_PLAY;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RESET_TIMER") != 0) 		ret = SYSTEM_CMD_RESET_TIMER;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_VOLUME_SET") != 0) 	ret = SYSTEM_CMD_SOUND_VOLUME_SET;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_VOLUME_MUTE") != 0) 	ret = SYSTEM_CMD_SOUND_VOLUME_MUTE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_VOLUME_INC") != 0) 	ret = SYSTEM_CMD_SOUND_VOLUME_INC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_VOLUME_DEC") != 0) 	ret = SYSTEM_CMD_SOUND_VOLUME_DEC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_VOLUME_UP") != 0) 	ret = SYSTEM_CMD_SOUND_VOLUME_UP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOUND_VOLUME_DOWN") != 0) 	ret = SYSTEM_CMD_SOUND_VOLUME_DOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MIC_VOLUME_SET") != 0) 		ret = SYSTEM_CMD_MIC_VOLUME_SET;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MIC_VOLUME_INC") != 0) 		ret = SYSTEM_CMD_MIC_VOLUME_INC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MIC_VOLUME_DEC") != 0) 		ret = SYSTEM_CMD_MIC_VOLUME_DEC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MIC_VOLUME_UP") != 0) 		ret = SYSTEM_CMD_MIC_VOLUME_UP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MIC_VOLUME_DOWN") != 0) 	ret = SYSTEM_CMD_MIC_VOLUME_DOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOFT_VOLUME_SET") != 0) 	ret = SYSTEM_CMD_SOFT_VOLUME_SET;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SOFT_VOLUME_STEP") != 0) 	ret = SYSTEM_CMD_SOFT_VOLUME_STEP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ALARM_VOLUME_SET") != 0) 	ret = SYSTEM_CMD_ALARM_VOLUME_SET;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ALARM_VOLUME_INC") != 0) 	ret = SYSTEM_CMD_ALARM_VOLUME_INC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ALARM_VOLUME_DEC") != 0) 	ret = SYSTEM_CMD_ALARM_VOLUME_DEC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ALARM_VOLUME_UP") != 0) 	ret = SYSTEM_CMD_ALARM_VOLUME_UP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ALARM_VOLUME_DOWN") != 0) 	ret = SYSTEM_CMD_ALARM_VOLUME_DOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RADIO_VOLUME_UP") != 0) 	ret = SYSTEM_CMD_RADIO_VOLUME_UP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RADIO_VOLUME_DOWN") != 0) 	ret = SYSTEM_CMD_RADIO_VOLUME_DOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RADIO_STATION_PREV") != 0) 	ret = SYSTEM_CMD_RADIO_STATION_PREV;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RADIO_STATION_NEXT") != 0) 	ret = SYSTEM_CMD_RADIO_STATION_NEXT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RADIO_ON") != 0) 			ret = SYSTEM_CMD_RADIO_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RADIO_OFF") != 0) 			ret = SYSTEM_CMD_RADIO_OFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ACTION_ON") != 0) 			ret = SYSTEM_CMD_ACTION_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ACTION_OFF") != 0) 			ret = SYSTEM_CMD_ACTION_OFF;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_ACTION_TEMP_OFF") != 0) 	ret = SYSTEM_CMD_ACTION_TEMP_OFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SKIP") != 0) 				ret = SYSTEM_CMD_SKIP;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_ON") != 0) 			ret = SYSTEM_CMD_STREAM_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_ON_LAST") != 0) 		ret = SYSTEM_CMD_STREAM_ON_LAST;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_OFF") != 0) 			ret = SYSTEM_CMD_STREAM_OFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_ON_NEXT") != 0) 		ret = SYSTEM_CMD_STREAM_ON_NEXT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_ON_PREV") != 0) 		ret = SYSTEM_CMD_STREAM_ON_PREV;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_TYPE_ON") != 0) 		ret = SYSTEM_CMD_STREAM_TYPE_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_TYPE_RND_ON") != 0) 	ret = SYSTEM_CMD_STREAM_TYPE_RND_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STREAM_RND_ON") != 0) 		ret = SYSTEM_CMD_STREAM_RND_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_DIR") != 0) 			ret = SYSTEM_CMD_PLAY_DIR;
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_FILE") != 0) 			ret = SYSTEM_CMD_PLAY_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_DIR_RND_FILE") != 0) 	ret = SYSTEM_CMD_PLAY_DIR_RND_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_SHOW_PATH") != 0) 		ret = SYSTEM_CMD_SET_SHOW_PATH;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_PLAYLIST_POS") != 0) 	ret = SYSTEM_CMD_SET_PLAYLIST_POS;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_DIRLIST_POS") != 0) 	ret = SYSTEM_CMD_SET_DIRLIST_POS;	

	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_NEXT_DIR") != 0) 		ret = SYSTEM_CMD_PLAY_NEXT_DIR;
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_PREV_DIR") != 0) 		ret = SYSTEM_CMD_PLAY_PREV_DIR;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_RANDOM_FILE_ON") != 0) 		ret = SYSTEM_CMD_RANDOM_FILE_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RANDOM_FILE_OFF") != 0) 	ret = SYSTEM_CMD_RANDOM_FILE_OFF;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_RANDOM_DIR_ON") != 0) 		ret = SYSTEM_CMD_RANDOM_DIR_ON;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RANDOM_DIR_OFF") != 0) 		ret = SYSTEM_CMD_RANDOM_DIR_OFF;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_FULL_NEXT") != 0) 		ret = SYSTEM_CMD_SHOW_FULL_NEXT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_STOP_AV") != 0) 		ret = SYSTEM_CMD_SHOW_STOP_AV;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_STOP_VIDEO") != 0) 	ret = SYSTEM_CMD_SHOW_STOP_VIDEO;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_STOP_AUDIO") != 0) 	ret = SYSTEM_CMD_SHOW_STOP_AUDIO;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_NEXT") != 0) 			ret = SYSTEM_CMD_SHOW_NEXT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_PREV") != 0) 			ret = SYSTEM_CMD_SHOW_PREV;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_BACKWARD") != 0) 		ret = SYSTEM_CMD_SHOW_BACKWARD;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_FORWARD") != 0) 		ret = SYSTEM_CMD_SHOW_FORWARD;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_PAUSE") != 0) 			ret = SYSTEM_CMD_SHOW_PAUSE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_PLAY") != 0) 			ret = SYSTEM_CMD_SHOW_PLAY;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_INFO") != 0) 			ret = SYSTEM_CMD_SHOW_INFO;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_EVENT_TEXT") != 0) 	ret = SYSTEM_CMD_SHOW_EVENT_TEXT;
		
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_NEW_POS") != 0) 		ret = SYSTEM_CMD_PLAY_NEW_POS;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_BACKWARD") != 0) 		ret = SYSTEM_CMD_MENU_BACKWARD;
	if (SearchStrInData(Buff, iLen, 0, "SYS_MENU_FORWARD") != 0) 		ret = SYSTEM_CMD_MENU_FORWARD;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_SHOW_MODE") != 0) 		ret = SYSTEM_CMD_SET_SHOW_MODE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_SHOW_STATUS") != 0) 	ret = SYSTEM_CMD_SET_SHOW_STATUS;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_ZOOM_MODE") != 0) 		ret = SYSTEM_CMD_SET_ZOOM_MODE;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_CAMLIST_CLEAR") != 0) 		ret = SYSTEM_CMD_CAMLIST_CLEAR;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CAMLIST_ADD") != 0) 		ret = SYSTEM_CMD_CAMLIST_ADD;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CAMLIST_SHOW") != 0) 		ret = SYSTEM_CMD_CAMLIST_SHOW;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CAMERA_ERROR") != 0) 		ret = SYSTEM_CMD_CAMERA_ERROR;
	if (SearchStrInData(Buff, iLen, 0, "SYS_VIDEO_ERROR") != 0) 		ret = SYSTEM_CMD_VIDEO_ERROR;
	if (SearchStrInData(Buff, iLen, 0, "SYS_AUDIO_ERROR") != 0) 		ret = SYSTEM_CMD_AUDIO_ERROR;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_VIDEO") != 0) 			ret = SYSTEM_CMD_PLAY_VIDEO;
	if (SearchStrInData(Buff, iLen, 0, "SYS_PLAY_AUDIO") != 0) 			ret = SYSTEM_CMD_PLAY_AUDIO;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STOPED_VIDEO") != 0) 		ret = SYSTEM_CMD_STOPED_VIDEO;
	if (SearchStrInData(Buff, iLen, 0, "SYS_STOPED_AUDIO") != 0) 		ret = SYSTEM_CMD_STOPED_AUDIO;
		
	if (SearchStrInData(Buff, iLen, 0, "SYS_OPENED_RTSP") != 0) 		ret = SYSTEM_CMD_OPENED_RTSP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_OPENED_CAMERA") != 0) 		ret = SYSTEM_CMD_OPENED_CAMERA;
	if (SearchStrInData(Buff, iLen, 0, "SYS_OPENED_FILE") != 0) 		ret = SYSTEM_CMD_OPENED_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOSED_FILE") != 0) 		ret = SYSTEM_CMD_CLOSED_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_DONE_FILE") != 0) 			ret = SYSTEM_CMD_DONE_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_NEW_FILE_POS") != 0) 		ret = SYSTEM_CMD_NEW_FILE_POS;
	if (SearchStrInData(Buff, iLen, 0, "SYS_ERROR_FILE") != 0) 			ret = SYSTEM_CMD_ERROR_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_BUSY_FILE") != 0) 			ret = SYSTEM_CMD_BUSY_FILE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_FREE_FILE") != 0) 			ret = SYSTEM_CMD_FREE_FILE;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_OPENED_MIC") != 0) 			ret = SYSTEM_CMD_OPENED_MIC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOSED_RTSP") != 0) 		ret = SYSTEM_CMD_CLOSED_RTSP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOSED_CAMERA") != 0) 		ret = SYSTEM_CMD_CLOSED_CAMERA;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLOSED_MIC") != 0) 			ret = SYSTEM_CMD_CLOSED_MIC;
	if (SearchStrInData(Buff, iLen, 0, "SYS_TIMERS_INCREASE") != 0) 	ret = SYSTEM_CMD_TIMERS_INCREASE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_TIMERS_DECREASE") != 0) 	ret = SYSTEM_CMD_TIMERS_DECREASE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_TIMERS_UPDATE") != 0) 		ret = SYSTEM_CMD_TIMERS_UPDATE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_RESET_STATUS_NUM") != 0) 	ret = SYSTEM_CMD_RESET_STATUS_NUM;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_STATUS_NUM") != 0) 		ret = SYSTEM_CMD_SET_STATUS_NUM;
	if (SearchStrInData(Buff, iLen, 0, "SYS_VALUE_STATUS_NUM") != 0) 	ret = SYSTEM_CMD_VALUE_STATUS_NUM;	
	if (SearchStrInData(Buff, iLen, 0, "SYS_RESET_STATUS_MESSAGE") != 0)ret = SYSTEM_CMD_RESET_STATUS_MESSAGE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SET_STATUS_MESSAGE") != 0) 	ret = SYSTEM_CMD_SET_STATUS_MESSAGE;
	if (SearchStrInData(Buff, iLen, 0, "EXEC_EXT_APP_NOW") != 0) 		ret = SYSTEM_CMD_EXEC_EXT_APP_NOW;
	if (SearchStrInData(Buff, iLen, 0, "EXEC_EXT_APP_EXIT") != 0) 		ret = SYSTEM_CMD_EXEC_EXT_APP_EXIT;
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_CMD_MENU_NEXT") != 0) 		ret = SYSTEM_CMD_MENU_NEXT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CMD_MENU_PREV") != 0) 		ret = SYSTEM_CMD_MENU_PREV;	
	
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_AGAIN") != 0) 			ret = SYSTEM_CMD_SHOW_AGAIN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_FULL_STOP") != 0) 			ret = SYSTEM_CMD_FULL_STOP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_CLIENT_STOP") != 0) 		ret = SYSTEM_CMD_CLIENT_STOP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_STATUS_OFF") != 0) 	ret = SYSTEM_CMD_WIDGET_STATUS_OFF;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_STATUS_DAYTIME") != 0) 	ret = SYSTEM_CMD_WIDGET_STATUS_DAYTIME;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_STATUS_ALLWAYS") != 0) 	ret = SYSTEM_CMD_WIDGET_STATUS_ALLWAYS;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_STATUS_TIMEOUT") != 0) 	ret = SYSTEM_CMD_WIDGET_STATUS_TIMEOUT;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_STATUS_MENU") != 0) 	ret = SYSTEM_CMD_WIDGET_STATUS_MENU;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_TIMEDOWN") != 0) 	ret = SYSTEM_CMD_WIDGET_TIMEDOWN;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_TIMEUP") != 0) 		ret = SYSTEM_CMD_WIDGET_TIMEUP;
	if (SearchStrInData(Buff, iLen, 0, "SYS_WIDGET_UPDATE") != 0) 		ret = SYSTEM_CMD_WIDGET_UPDATE;
	if (SearchStrInData(Buff, iLen, 0, "SYS_SHOW_MENU") != 0) 			ret = SYSTEM_CMD_SHOW_MENU;
	if (SearchStrInData(Buff, iLen, 0, "SYS_HIDE_MENU") != 0) 			ret = SYSTEM_CMD_HIDE_MENU;
	if (SearchStrInData(Buff, iLen, 0, "SYS_EVENT_SECONDS") != 0) 		ret = SYSTEM_CMD_EVENT_SECONDS;
	if (SearchStrInData(Buff, iLen, 0, "SYS_EVENT_MINUTES") != 0) 		ret = SYSTEM_CMD_EVENT_MINUTES;
	if (SearchStrInData(Buff, iLen, 0, "SYS_EVENT_HOURS") != 0) 		ret = SYSTEM_CMD_EVENT_HOURS;
						
	if (SearchStrInData(Buff, iLen, 0, "[MO]") != 0) ret |= 1;
	if (SearchStrInData(Buff, iLen, 0, "[TU]") != 0) ret |= 2;
	if (SearchStrInData(Buff, iLen, 0, "[WE]") != 0) ret |= 4;
	if (SearchStrInData(Buff, iLen, 0, "[TH]") != 0) ret |= 8;
	if (SearchStrInData(Buff, iLen, 0, "[FR]") != 0) ret |= 16;
	if (SearchStrInData(Buff, iLen, 0, "[SA]") != 0) ret |= 32;
	if (SearchStrInData(Buff, iLen, 0, "[SU]") != 0) ret |= 64;		
	
	if (ret == 0)
	{		
		int n;
		for (n = 0; n != iLen; n++) 
			if (((n == 0) && ((Buff[n] < 48) || (Buff[n] > 57)) && (Buff[n] != 45))
				|| 
				((n != 0) && ((Buff[n] < 48) || (Buff[n] > 57))))
						{ret = 1; break;}
		if (ret == 0)
		{
			ret = (unsigned int)Str2IntLimit(Buff, iLen);
		}
		else
		{
			if (iLen > 4) dbgprintf(2, "Unknown parameter: '%s'\n", Buff);
			ret = iID;
		}
	}
	
	DBG_LOG_OUT();	
	return ret;
}

int GetParamSetting(unsigned int uiNum, char cParamKey, char *cBuffIn, unsigned int uiBuffInSize, char *cBuffOut, unsigned int uiBuffOutSize)
{
	if (uiBuffInSize == 0) return 0;
	if (uiBuffOutSize == 0) return 0;
	memset(cBuffOut, 0, uiBuffOutSize);
	uiBuffOutSize--;
	int n, m;
	int Clk = 0;
	int PrevPos = 0;
	uiBuffInSize--;
	for (n = 0; n <= uiBuffInSize; n++)
	{
		if ((cBuffIn[n] == cParamKey) || (n == uiBuffInSize))
		{
			if (Clk == uiNum)
			{
				if (cBuffIn[n] != cParamKey) n++;
				m = n - PrevPos;
				if (m > uiBuffOutSize)
				{
					memcpy(cBuffOut, &cBuffIn[PrevPos], uiBuffOutSize);
					return 2;
				}
				else 
				{
					memcpy(cBuffOut, &cBuffIn[PrevPos], m);
					return 1;
				}
			}
			PrevPos = n + 1;
			Clk++;
		}
	}
	return 0;	
}

char *GetModuleStatusName(unsigned int uiType, unsigned int uiStatusNum, char*OutBuff, unsigned int OutSize, char cLang)
{
	memset(OutBuff, 0, OutSize);
	unsigned int BuffLen = 64;
	char Buffer[64];
	memset(Buffer, 0, BuffLen);
	char *ret = OutBuff;
	
	switch(uiType)
	{
		case MODULE_TYPE_SPEAKER:
			if (cLang)
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Микрофон ID");
				if (uiStatusNum == 1) strcpy(Buffer, "Подключено");
			}
			else
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Mic ID");
				if (uiStatusNum == 1) strcpy(Buffer, "Connected");
			}
			if (uiStatusNum > 2) ret = NULL;
			break;	
		case MODULE_TYPE_KEYBOARD:
			if (cLang) strcpy(Buffer, "Код"); else strcpy(Buffer, "Code");
			break;	
		case MODULE_TYPE_COUNTER:
		case MODULE_TYPE_MEMORY:
			if (cLang) sprintf(Buffer, "Ячейка %i", uiStatusNum + 1); else sprintf(Buffer, "Cell %i", uiStatusNum + 1);	
			break;		
		case MODULE_TYPE_RS485:
			if (uiStatusNum == 0) 
			{
				if (cLang) strcpy(Buffer, "Код"); else strcpy(Buffer, "Code");
			}
			if (uiStatusNum > 1) ret = NULL;
			break;
		case MODULE_TYPE_EXTERNAL:
		case MODULE_TYPE_USB_GPIO:
			if (cLang) sprintf(Buffer, "Субмодуль %i", uiStatusNum + 1); else sprintf(Buffer, "Submodule %i", uiStatusNum + 1);	
			break;
		case MODULE_TYPE_IR_RECEIVER:
		case MODULE_TYPE_TFP625A:
			if (cLang)
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Статус");
				if (uiStatusNum == 1) strcpy(Buffer, "Номер");
				if (uiStatusNum == 2) strcpy(Buffer, "ID");
			}
			else
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Status");
				if (uiStatusNum == 1) strcpy(Buffer, "Number");
				if (uiStatusNum == 2) strcpy(Buffer, "ID");
			}
			if (uiStatusNum > 2) ret = NULL;
			break;
		case MODULE_TYPE_GPIO:
			if (uiStatusNum == 0) 
			{
				if (cLang) strcpy(Buffer, "Уровень"); else strcpy(Buffer, "Level");
			}
			if (uiStatusNum > 1) ret = NULL;
			break;
		case MODULE_TYPE_DISPLAY:
			if (cLang)
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Камера ID");
				if (uiStatusNum == 1) strcpy(Buffer, "PTZ ID");
				if (uiStatusNum == 2) strcpy(Buffer, "Меню");
			}
			else
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Camera ID");
				if (uiStatusNum == 1) strcpy(Buffer, "PTZ ID");
				if (uiStatusNum == 2) strcpy(Buffer, "Menu");
			}
			if (uiStatusNum > 2) ret = NULL;
			break;
		case MODULE_TYPE_TEMP_SENSOR:
			if (cLang)
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Температура");
				if (uiStatusNum == 1) strcpy(Buffer, "Влажность");
			}
			else
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Temperature");
				if (uiStatusNum == 1) strcpy(Buffer, "Humidity");
			}
			if (uiStatusNum > 1) ret = NULL;
			break;
		case MODULE_TYPE_AS5600:
			if (cLang)
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Значение RAW");
				if (uiStatusNum == 1) strcpy(Buffer, "Значение");
			}
			else
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Value RAW");
				if (uiStatusNum == 1) strcpy(Buffer, "Value");
			}
			if (uiStatusNum > 1) ret = NULL;
			break;
		case MODULE_TYPE_HMC5883L:
			if (uiStatusNum == 0) strcpy(Buffer, "X raw");
			if (uiStatusNum == 1) strcpy(Buffer, "Y raw");
			if (uiStatusNum == 2) strcpy(Buffer, "Z raw");
			if (uiStatusNum == 3) strcpy(Buffer, "X");
			if (uiStatusNum == 4) strcpy(Buffer, "Y");
			if (uiStatusNum == 5) strcpy(Buffer, "Z");				
			if (uiStatusNum > 5) ret = NULL;
			break;
		case MODULE_TYPE_ADS1015:
		case MODULE_TYPE_MCP3421:
			if (uiStatusNum == 0) 
			{
				if (cLang) strcpy(Buffer, "Значение"); else strcpy(Buffer, "Value");
			}
			if (uiStatusNum > 0) ret = NULL;
			break;
		case MODULE_TYPE_MIC:
			if (cLang)
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Уровень");
				if (uiStatusNum == 1) strcpy(Buffer, "Подключен");
			}
			else
			{
				if (uiStatusNum == 0) strcpy(Buffer, "Level");
				if (uiStatusNum == 1) strcpy(Buffer, "Connected");
			}
			if (uiStatusNum > 1) ret = NULL;
			break;
		case MODULE_TYPE_SYSTEM:
			if (cLang)
			{
				switch(uiStatusNum)
				{
					case 0:	strcpy(Buffer, "Температура"); break;
					case 1:	strcpy(Buffer, "Уровень доступа"); break;
					case 2:	strcpy(Buffer, "Загрузка процессора(всего)"); break;
					case 3:	strcpy(Buffer, "Загрузка процессора(сервисом)"); break;
					case 4: strcpy(Buffer, "Всего памяти"); break;
					case 5:	strcpy(Buffer, "Всего памяти используется"); break;
					case 6:	strcpy(Buffer, "Всего памяти используется сервисом"); break;
					case 7:	strcpy(Buffer, "Всего видео памяти"); break;
					case 8:	strcpy(Buffer, "Всего видео памяти используется"); break;
					case 9:	strcpy(Buffer, "Всего памяти"); break;
					case 10: strcpy(Buffer, "Напряжение core"); break;
					case 11: strcpy(Buffer, "Напряжение sdram_c"); break;
					case 12: strcpy(Buffer, "Напряжение sdram_i"); break;
					case 13: strcpy(Buffer, "Напряжение sdram_p"); break;
					case 14: strcpy(Buffer, "Системные события"); break;
					case 15: strcpy(Buffer, "Текущий час"); break;
					case 16: strcpy(Buffer, "Текущия минута"); break;
					case 17: strcpy(Buffer, "Текущая секунда"); break;
					case 18: strcpy(Buffer, "Пониженное питание"); break;
					default: ret = NULL; break;
				}
			}
			else
			{
				switch(uiStatusNum)
				{
					case 0:	strcpy(Buffer, "Temperature"); break;
					case 1:	strcpy(Buffer, "Access level"); break;
					case 2:	strcpy(Buffer, "CPU load(total)"); break;
					case 3:	strcpy(Buffer, "CPU load(service)"); break;
					case 4: strcpy(Buffer, "CPU memory"); break;
					case 5:	strcpy(Buffer, "CPU memory use"); break;
					case 6:	strcpy(Buffer, "CPU memory use service"); break;
					case 7:	strcpy(Buffer, "GPU memory"); break;
					case 8:	strcpy(Buffer, "GPU memory use"); break;
					case 9:	strcpy(Buffer, "CPU memory"); break;
					case 10: strcpy(Buffer, "Voltage core"); break;
					case 11: strcpy(Buffer, "Voltage sdram_c"); break;
					case 12: strcpy(Buffer, "Voltage sdram_i"); break;
					case 13: strcpy(Buffer, "Voltage sdram_p"); break;
					case 14: strcpy(Buffer, "System events"); break;
					case 15: strcpy(Buffer, "Current hour"); break;
					case 16: strcpy(Buffer, "Current minute"); break;
					case 17: strcpy(Buffer, "Current second"); break;
					case 18: strcpy(Buffer, "Low power"); break;
					default: ret = NULL; break;
				}
			}
			break;	
		case MODULE_TYPE_CAMERA:
			if (cLang)
			{
				switch(uiStatusNum)
				{
					case 0:	strcpy(Buffer, "Освещенность");	break;
					case 1:	strcpy(Buffer, "Изменения"); break;
					case 2:	strcpy(Buffer, "Экспозиция"); break;
					case 3:	strcpy(Buffer, "Размер яркостных изменений"); break;
					case 4:	strcpy(Buffer, "Движение в квадрате"); break;
					case 5:	strcpy(Buffer, "*Выполнение действий"); break;
					case 6:	strcpy(Buffer, "Выполнение действий"); break;
					case 7:	strcpy(Buffer, "Таймер записи движения"); break;
					case 8:	strcpy(Buffer, "Таймер записи медл. движения");	break;
					case 9:	strcpy(Buffer, "*Размер движения"); break;
					case 10: strcpy(Buffer, "*Размер медл. движения"); break;
					case 11: strcpy(Buffer, "Выполнение действий");	break;
					case 12: strcpy(Buffer, "Яркостное изменение в квадрате номер"); break;
					case 13: strcpy(Buffer, "Размер цветовых изменений"); break;
					case 14: strcpy(Buffer, "Цветовое изменение в квадрате номер"); break;
					case 15: strcpy(Buffer, "Яркость"); break;
					case 16: strcpy(Buffer, "*Автояркость"); break;
					case 17: strcpy(Buffer, "*Настройка автояркости"); break;
					case 18: strcpy(Buffer, "Текущая освещенность"); break;
					case 19: strcpy(Buffer, "Фильтр"); break;
					case 20: strcpy(Buffer, "Баланс белого"); break;
					case 21: strcpy(Buffer, "Подключен"); break;
					default: ret = NULL; break;
				}
			}
			else
			{
				switch(uiStatusNum)
				{
					case 0:	strcpy(Buffer, "Illumination");	break;
					case 1:	strcpy(Buffer, "Changes"); break;
					case 2:	strcpy(Buffer, "Exposure"); break;
					case 3:	strcpy(Buffer, "Size bright changes"); break;
					case 4:	strcpy(Buffer, "Move in square"); break;
					case 5:	strcpy(Buffer, "*Execute actions"); break;
					case 6:	strcpy(Buffer, "Execute actions"); break;
					case 7:	strcpy(Buffer, "Timer move capture"); break;
					case 8:	strcpy(Buffer, "Timer slow move capture");	break;
					case 9:	strcpy(Buffer, "*Move size"); break;
					case 10: strcpy(Buffer, "*Slow move size"); break;
					case 11: strcpy(Buffer, "Execute actions");	break;
					case 12: strcpy(Buffer, "Bright move in square"); break;
					case 13: strcpy(Buffer, "Size color changes"); break;
					case 14: strcpy(Buffer, "Color changes in square"); break;
					case 15: strcpy(Buffer, "Bright"); break;
					case 16: strcpy(Buffer, "*Autobright"); break;
					case 17: strcpy(Buffer, "*Autobright sett"); break;
					case 18: strcpy(Buffer, "Current illumination"); break;
					case 19: strcpy(Buffer, "Filter"); break;
					case 20: strcpy(Buffer, "White balance"); break;
					case 21: strcpy(Buffer, "Connected"); break;
					default: ret = NULL; break;
				}
			}
			break;
		case MODULE_TYPE_RTC:
			if (uiStatusNum == 0) 
			{
				if (cLang) strcpy(Buffer, "Время"); else strcpy(Buffer, "Time");
			}
			if (uiStatusNum > 0) ret = NULL;
			break;
		case MODULE_TYPE_PN532:
		case MODULE_TYPE_RC522:
			if (uiStatusNum == 0) 
			{
				if (cLang) strcpy(Buffer, "Код события"); else strcpy(Buffer, "Event code");
			}
			if (uiStatusNum == 1) 
			{
				if (cLang) strcpy(Buffer, "ID карты"); else strcpy(Buffer, "ID card");
			}
			if (uiStatusNum > 1) ret = NULL;
			break;			
		default:
			ret = NULL;
			break;
	}
	if (ret)
	{
		if (OutSize < BuffLen) 
			memcpy(OutBuff, Buffer, OutSize - 1);
			else
			memcpy(OutBuff, Buffer, BuffLen);
	}
	return ret;
}

char *GetModuleStatusValue(unsigned int uiType, unsigned int uiStatusNum, int iStatus, char*OutBuff, unsigned int OutSize)
{
	memset(OutBuff, 0, OutSize);
	char Buffer[64];
	unsigned int BuffLen = 64;
	memset(Buffer, 0, BuffLen);

	switch(uiType)
	{
		case MODULE_TYPE_SPEAKER:
			if (uiStatusNum == 0) sprintf(Buffer, "%.4s", (char*)&iStatus);
			if (uiStatusNum == 1) sprintf(Buffer, "%i", iStatus);
			break;		
		case MODULE_TYPE_IR_RECEIVER:
			if (uiStatusNum == 0) GetActionCodeName(iStatus, Buffer, 64, 2);
			break;
		case MODULE_TYPE_KEYBOARD:
			break;
		case MODULE_TYPE_TFP625A:
			if (uiStatusNum == 0) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 1) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 2) sprintf(Buffer, "%.4s", (char*)&iStatus);
			break;
		case MODULE_TYPE_RC522:
		case MODULE_TYPE_PN532:
			GetActionCodeName(iStatus, Buffer, 64, 2);
			break;
		case MODULE_TYPE_EXTERNAL:
		case MODULE_TYPE_USB_GPIO:
		case MODULE_TYPE_COUNTER:
		case MODULE_TYPE_MEMORY:
			//GetActionCodeName(iStatus, Buffer, 64, 2);
			sprintf(Buffer, "%i", iStatus);
			break;
		case MODULE_TYPE_GPIO:
			if (uiStatusNum == 0) sprintf(Buffer, "%s", iStatus ? "On" : "Off");
			break;
		case MODULE_TYPE_DISPLAY:
			if (uiStatusNum == 0) sprintf(Buffer, "%.4s", (char*)&iStatus);
			if (uiStatusNum == 1) sprintf(Buffer, "%.4s", (char*)&iStatus);
			if (uiStatusNum == 2) sprintf(Buffer, "%i", iStatus);
			break;
		case MODULE_TYPE_TEMP_SENSOR:
			if (uiStatusNum == 0) sprintf(Buffer, "%.1f", (float)iStatus / 10);
			if (uiStatusNum == 1) sprintf(Buffer, "%.1f", (float)iStatus / 10);
			break;
		case MODULE_TYPE_ADS1015:
		case MODULE_TYPE_MCP3421:
			if (uiStatusNum == 0) sprintf(Buffer, "%i", iStatus);
			break;
		case MODULE_TYPE_AS5600:
			if (uiStatusNum == 0) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 1) sprintf(Buffer, "%i", iStatus);
			break;
		case MODULE_TYPE_HMC5883L:
			if (uiStatusNum == 0) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 1) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 2) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 3) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 4) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 5) sprintf(Buffer, "%i", iStatus);
			break;
		case MODULE_TYPE_MIC:
			if (uiStatusNum == 0) sprintf(Buffer, "%i", iStatus);
			if (uiStatusNum == 1) sprintf(Buffer, "%i", iStatus);
			break;
		case MODULE_TYPE_SYSTEM:
			switch(uiStatusNum)
			{
				case 0: sprintf(Buffer, "%.1f", (float)iStatus / 10); break;
				case 1:	sprintf(Buffer, "%i", iStatus); break;
				case 2: sprintf(Buffer, "%.1f", (float)iStatus / 10); break;
				case 3: sprintf(Buffer, "%.1f", (float)iStatus / 10); break;
				case 4: sprintf(Buffer, "%i", iStatus); break;
				case 5: sprintf(Buffer, "%i", iStatus); break;
				case 6: sprintf(Buffer, "%i", iStatus); break;
				case 7: sprintf(Buffer, "%i", iStatus); break;
				case 8: sprintf(Buffer, "%i", iStatus); break;
				case 9: sprintf(Buffer, "%i", iStatus); break;
				case 10: sprintf(Buffer, "%g", (float)iStatus / 1000); break;
				case 11: sprintf(Buffer, "%g", (float)iStatus / 1000); break;
				case 12: sprintf(Buffer, "%g", (float)iStatus / 1000); break;
				case 13: sprintf(Buffer, "%g", (float)iStatus / 1000); break;
				//case 14: sprintf(Buffer, "%g", (float)iStatus / 1000); break;
				case 15: sprintf(Buffer, "%i", iStatus); break;
				case 16: sprintf(Buffer, "%i", iStatus); break;
				case 17: sprintf(Buffer, "%i", iStatus); break;
				case 18: sprintf(Buffer, "%i", iStatus); break;
				default: break;
			}
			break;	
		case MODULE_TYPE_CAMERA:
			switch(uiStatusNum)
			{
				case 0:	sprintf(Buffer, "%i", iStatus); break;
				case 1:	sprintf(Buffer, "%i", iStatus); break;
				case 2: sprintf(Buffer, "%s", GetNameExposure(iStatus)); break;
				case 3:	sprintf(Buffer, "%i", iStatus); break;
				case 4: sprintf(Buffer, "%i", iStatus); break;
				case 5: 
					 sprintf(Buffer, "%s%s%s%s", 
								(iStatus & 1) ? " [Shot sensor]" : "",
								(iStatus & 2) ? " [Shot camera]" : "",
								(iStatus & 4) ? " [Shot square]" : "",
								(iStatus & 8) ? " [Chenge square]" : "");
					break;
				case 6: sprintf(Buffer, "%s", iStatus ? " [Show square]" : ""); break;
				case 7:	sprintf(Buffer, "%i", iStatus); break;
				case 8:	sprintf(Buffer, "%i", iStatus); break;
				case 9:	sprintf(Buffer, "%i", iStatus); break;
				case 10: sprintf(Buffer, "%i", iStatus); break;
				case 11: sprintf(Buffer, "%s%s", 
								(iStatus & 1) ? " [Move signal(norm)]" : "",										
								(iStatus & 2) ? " [Move signal(slow)]" : "");										
					break;				
				case 12: sprintf(Buffer, "%i", iStatus); break;
				case 13: sprintf(Buffer, "%i", iStatus); break;
				case 14: sprintf(Buffer, "%i", iStatus); break;
				case 15: sprintf(Buffer, "%i", iStatus); break;
				case 16: sprintf(Buffer, "%s", iStatus ? "On" : "Off"); break;
				case 17: sprintf(Buffer, "%i", iStatus); break;
				case 18: sprintf(Buffer, "%i", iStatus); break;
				case 19: sprintf(Buffer, "%s", GetNameImageFilter(iStatus)); break;
				case 20: sprintf(Buffer, "%s", GetNameWhiteBalance(iStatus)); break;
				case 21: sprintf(Buffer, "%i", iStatus); break;
				default: break;
			}
			break;
		case MODULE_TYPE_RTC:
			if (uiStatusNum == 0) i2c_read_to_buff_timedate3231(I2C_ADDRESS_CLOCK, Buffer, 64);
			break;
		default:
			sprintf(Buffer, "%i/'%.4s'", iStatus, (char*)&iStatus);
			break;
	}
	
	int i;
	if (OutSize < BuffLen) 
	{
		memcpy(OutBuff, Buffer, OutSize - 1);
		for(i = 0; i < (OutSize - 1); i++) if (OutBuff[i] == 46) OutBuff[i] = 44;
	}
	else
	{
		memcpy(OutBuff, Buffer, BuffLen);
		for(i = 0; i < BuffLen; i++) if (OutBuff[i] == 46) OutBuff[i] = 44;
	}
	
	return OutBuff;
}

char *GetModuleStatusValueType(unsigned int uiType, unsigned int uiStatusNum, char*OutBuff, unsigned int OutSize)
{
	memset(OutBuff, 0, OutSize);
	char Buffer[32];
	unsigned int BuffLen = 32;
	memset(Buffer, 0, BuffLen);

	switch(uiType)
	{
		case MODULE_TYPE_EXTERNAL:
		case MODULE_TYPE_COUNTER:
		case MODULE_TYPE_MEMORY:
		case MODULE_TYPE_GPIO:
		case MODULE_TYPE_USB_GPIO:
		case MODULE_TYPE_MIC:
		case MODULE_TYPE_RTC:
		case MODULE_TYPE_ADS1015:
		case MODULE_TYPE_MCP3421:
		case MODULE_TYPE_AS5600:
		case MODULE_TYPE_KEYBOARD:
		case MODULE_TYPE_DISPLAY:
		case MODULE_TYPE_TFP625A:
		case MODULE_TYPE_SPEAKER:
			break;
		case MODULE_TYPE_HMC5883L:
			switch(uiStatusNum)
			{
				case 3:
				case 4:
				case 5: strcpy(Buffer, "`"); break;				
				default: break;
			}
			break;
		case MODULE_TYPE_IR_RECEIVER:
			if (uiStatusNum == 0) strcpy(Buffer, " ID");
			break;
		case MODULE_TYPE_TEMP_SENSOR:
			if (uiStatusNum == 0) strcpy(Buffer, "`C");
			if (uiStatusNum == 1) strcpy(Buffer, "%");
			break;
		case MODULE_TYPE_SYSTEM:
			switch(uiStatusNum)
			{
				case 0: strcpy(Buffer, "`C"); break;
				case 1:	break;
				case 2: strcpy(Buffer, "%"); break;
				case 3: strcpy(Buffer, "%"); break;
				case 4: strcpy(Buffer, "Mb"); break;
				case 5: strcpy(Buffer, "Mb"); break;
				case 6: strcpy(Buffer, "Mb"); break;
				case 7: strcpy(Buffer, "Mb"); break;
				case 8: strcpy(Buffer, "Mb"); break;
				case 9: strcpy(Buffer, "Mb"); break;
				case 10: strcpy(Buffer, "v"); break;
				case 11: strcpy(Buffer, "v"); break;
				case 12: strcpy(Buffer, "v"); break;
				case 13: strcpy(Buffer, "v"); break;
				case 14: break;
				case 15: strcpy(Buffer, "hour"); break;
				case 16: strcpy(Buffer, "min"); break;
				case 17: strcpy(Buffer, "sec"); break;
				case 18: break;
				default: break;
			}
			break;	
		case MODULE_TYPE_CAMERA:
			switch(uiStatusNum)
			{
				case 0:	break;
				case 1:	break;
				case 2: break;
				case 3:	break;
				case 4: break;
				case 5: break;
				case 6: break;
				case 7:	break;
				case 8:	break;
				case 9:	break;
				case 10: break;
				case 11: break;				
				case 12: break;
				case 13: break;
				case 14: break;
				case 15: break;
				case 16: break;
				case 17: break;
				case 18: break;
				case 19: break;
				case 20: break;
				case 21: break;
				default: break;
			}
			break;
		default:
			break;
	}
	if (OutSize < BuffLen) 
		memcpy(OutBuff, Buffer, OutSize - 1);
		else
		memcpy(OutBuff, Buffer, BuffLen);
	
	return OutBuff;
}

char GetModuleStatusEn(unsigned int uiType, unsigned int uiStatusNum)
{
	char ret = 0;

	switch(uiType)
	{
		case MODULE_TYPE_EXTERNAL:
		case MODULE_TYPE_COUNTER:
		case MODULE_TYPE_MEMORY:
		case MODULE_TYPE_USB_GPIO:
		case MODULE_TYPE_KEYBOARD:
			ret = 1;
			break;
		case MODULE_TYPE_GPIO:
		case MODULE_TYPE_IR_RECEIVER:
		case MODULE_TYPE_RS485:
		case MODULE_TYPE_RTC:
		case MODULE_TYPE_ADS1015:
		case MODULE_TYPE_MCP3421:
			if (uiStatusNum == 0) ret = 1;
			break;
		case MODULE_TYPE_MIC:
		case MODULE_TYPE_PN532:
		case MODULE_TYPE_RC522:
		case MODULE_TYPE_TEMP_SENSOR:
		case MODULE_TYPE_AS5600:
		case MODULE_TYPE_SPEAKER:
			if (uiStatusNum < 2) ret = 1;
			break;
		case MODULE_TYPE_DISPLAY:
		case MODULE_TYPE_TFP625A:
			if (uiStatusNum < 3) ret = 1;
			break;
		case MODULE_TYPE_HMC5883L:
			if (uiStatusNum < 6) ret = 1;
			break;
		case MODULE_TYPE_SYSTEM:
			if (uiStatusNum < 19) ret = 1;
			break;	
		case MODULE_TYPE_CAMERA:
			if (uiStatusNum < 22) ret = 1;
			break;
		default:
			break;
	}
	
	return ret;
}

int SetModuleDefaultSettings(MODULE_INFO* miModule)
{
	memset(miModule->Settings, 0, sizeof(int) * MAX_MODULE_SETTINGS);
	
	switch(miModule->Type)
	{
		case MODULE_TYPE_SPEAKER:
			miModule->Settings[4] = 5;
			break;
		case MODULE_TYPE_MIC:
			miModule->Settings[0] = 1;
			miModule->Settings[3] = 44100;
			miModule->Settings[5] = 1;
			miModule->Settings[13] = 1;
			miModule->Settings[14] = 1;
			break;
		case MODULE_TYPE_CAMERA:
			miModule->Settings[1] = 5;
			miModule->Settings[2] = 5;
			miModule->Settings[3] = 60;
			miModule->Settings[4] = 2000;
			miModule->Settings[9] = 60;
			miModule->Settings[10] = 1;
			miModule->Settings[11] = 1;
			
			miModule->Settings[6] = 20;
			miModule->Settings[16] = 20;
			
			miModule->Settings[17] = 500;
			miModule->Settings[18] = 500;
			miModule->Settings[21] = 60;
			miModule->Settings[22] = 60;
			miModule->Settings[23] = 5;
			miModule->Settings[24] = OMX_ExposureControlAuto;
			miModule->Settings[25] = 50;
			miModule->Settings[28] = OMX_ImageFilterNone;
			miModule->Settings[29] = OMX_WhiteBalControlAuto;
			
			miModule->Settings[33] = 100;
			miModule->Settings[34] = 20;			
			miModule->Settings[35] = 20;
			
			miModule->Settings[40] = 1 << 10;
			miModule->Settings[42] = 100;
			break;
		default:
			break;
	}
	return 1;
}

int InitSettings()
{
	DBG_LOG_IN();	
	
	memset(&mbManual, 0, sizeof(misc_buffer));
	
	memset(cConfigFile, 0, 256);
	strcpy(cConfigFile, "Config.ini");
	memset(cManualFile, 0, 256);
	strcpy(cManualFile, "Manual.txt");
	memset(cUserFile, 0, 256);
	strcpy(cUserFile, "Users.ini");
	memset(cCamRectangleFile, 0, 256);
	strcpy(cCamRectangleFile, "CamRectangles.ini");
	memset(cDirectoryFile, 0, 256);
	strcpy(cDirectoryFile, "Directories.ini");
	memset(cMnlActionFile, 0, 256);
	strcpy(cMnlActionFile, "ManualActions.ini");
	memset(cEvntActionFile, 0, 256);
	strcpy(cEvntActionFile, "EventActions.ini");
	memset(cIrCodeFile, 0, 256);
	strcpy(cIrCodeFile, "IrCodes.ini");
	memset(cKeyFile, 0, 256);
	strcpy(cKeyFile, "Keys.ini");
	memset(cWidgetFile, 0, 256);
	strcpy(cWidgetFile, "Widgets.ini");
	memset(cStreamFile, 0, 256);
	strcpy(cStreamFile, "Streams.ini");
	memset(cPtzFile, 0, 256);
	strcpy(cPtzFile, "PTZs.ini");
	memset(cStreamTypeFile, 0, 256);
	strcpy(cStreamTypeFile, "StreamTypes.ini");
	memset(cMailFile, 0, 256);
	strcpy(cMailFile, "Mails.ini");
	memset(cSoundFile, 0, 256);
	strcpy(cSoundFile, "Sounds.ini");
	memset(cAlarmFile, 0, 256);
	strcpy(cAlarmFile, "Alarms.ini");
	memset(cModuleFile, 0, 256);
	strcpy(cModuleFile, "Modules.ini");
	memset(cRadioFile, 0, 256);
	strcpy(cRadioFile, "Radios.ini");
	
	memset(cNewSourcePath, 0, 256);
	memset(cNewSourceFile, 0, 256);
	memset(cNewSourceLogin, 0, 256);
	memset(cNewSourcePass, 0, 256);
	cRebootAfterUpdate = 1;
	
	memset(cNTPServer, 0, 64);
	
	memset(&ispCameraImageSettings, 0, sizeof(image_sensor_params));
	
	iUpdateKeyInfoAction = 0;
	iUpdateKeyInfoResult = 0;
	cCurShowType = 0;
	OnvifAuth = 1;
	RtspAuth = 1;
	WebAuth = 0;
	WebMaxTimeIdle = 3600;
	iStreamTimeMaxSet = 1800;
	iCameraTimeMaxSet = 60;
	uiWebPort = 80;
	uiWEBServer = 1;
	uiRTSPForceAudio = 1;
	pWebMenu = NULL;
	iCurrentWebMenuPage = 0;
	iCurrentShowRectangle = 0;
	cThreadShowerStatus = 0;
	cThreadFingerStatus = 0;
	cThreadFingerRun = 1;
	cThreadScanerStatus = 0;
	cThreadScanerStatusUsbio = 0;
	cThreadEventerStatus = 0;
	cThreadReaderStatus = 0;
	cThreadMainStatus = 0;
	cThreadAudCaptStatus = 0;
	uiThreadCopyStatus = 0;
	cThreadWriterStatus = 0;
	memset(&tfTransferFileStatus, 0, sizeof(tfTransferFileStatus));
	
	uiMaxFileCopyTimeBreakValue	= 180;
	uiMaxFileCopyTimeMessageValue = 120;
	
	iStreamCnt = 0;
	SyncStream = NULL;
	
	memset(&rsiRemoteStatus, 0, sizeof(rsiRemoteStatus));
	rsiRemoteStatus.Current = -1;
	rsiRemoteStatus.Selected = -1;

	iSysMessageCnt = 0;
	memset(smiSysMessage, 0, sizeof(SYS_MESSAGE_INFO) * MAX_SYS_MESSAGE_LEN);
	
	ispCameraImageSettings.PrevColorLevel = 1;
	ispCameraImageSettings.PrevBrigLevel = 1;
	ispCameraImageSettings.PrevShowLevel = 1;
	ispCameraImageSettings.PrevSettings = 0;
	ispCameraImageSettings.MainSettings = 0;
	
	uiDefAlarmSound = 0;	
	uiDefAlarmRepeats = 5;
	uiMediaSlowSettings = 0;
	uiMediaSlowFramesSkip = 60;
	uiMediaIoTimeout = 60;
	
	uiCaptureMinFreeSpaceFull = 0;
	uiCaptureMinFreeSpaceSlow = 0;
	uiCaptureMinFreeSpaceDiff = 0;
	uiCaptureMinFreeSpaceAudio = 0;
	uiCaptureMinFreeSpaceStatuses = 0;
	uiCaptureMinFreeSpaceEvents = 0;
	uiCaptureMinFreeSpaceActions = 0;
	
	uiBackUpMinFreeSpaceFull = 0;
	uiBackUpMinFreeSpaceSlow = 0;
	uiBackUpMinFreeSpaceDiff = 0;
	uiBackUpMinFreeSpaceAudio = 0;
	uiBackUpMinFreeSpaceStatuses = 0;
	uiBackUpMinFreeSpaceEvents = 0;
	uiBackUpMinFreeSpaceActions = 0;
	
	uiArchiveMinFreeSpaceFull = 0;
	uiArchiveMinFreeSpaceSlow = 0;
	uiArchiveMinFreeSpaceDiff = 0;
	uiArchiveMinFreeSpaceAudio = 0;
	uiArchiveMinFreeSpaceStatuses = 0;
	uiArchiveMinFreeSpaceEvents = 0;
	uiArchiveMinFreeSpaceActions = 0;
	
	ucMediaArchiveTimeFromFull = 0;
	ucMediaArchiveTimeToFull = 2359;
	ucMediaArchiveTimeFromSlow = 0;
	ucMediaArchiveTimeToSlow = 2359;
	ucMediaArchiveTimeFromDiff = 0;
	ucMediaArchiveTimeToDiff = 2359;
	ucMediaArchiveTimeFromAudio = 0;
	ucMediaArchiveTimeToAudio = 2359;
	ucMediaArchiveTimeFromStatuses = 0;
	ucMediaArchiveTimeToStatuses = 2359;
	ucMediaArchiveTimeFromEvents = 0;
	ucMediaArchiveTimeToEvents = 2359;
	ucMediaArchiveTimeFromActions = 0;
	ucMediaArchiveTimeToActions = 2359;
	
	cCaptureFilesView = 0;
	cBackUpFilesView = 0;
	cCaptureFilesLevel = 0;
	cBackUpFilesLevel = 0;
	cCaptureFilesViewDef = 0;
	
	ucBackUpOrderLmtFull = 1;
	ucBackUpOrderLmtSlow = 1;
	ucBackUpOrderLmtDiff = 1;
	ucBackUpOrderLmtAudio = 1;
	ucBackUpOrderLmtStatuses = 1;
	ucBackUpOrderLmtEvents = 1;
	ucBackUpOrderLmtActions = 1;
	ucBackUpOrderLmtArchive = 1;
	
	uiBackUpOrderWaitCnclFull = 120;
	uiBackUpOrderWaitCnclSlow = 120;
	uiBackUpOrderWaitCnclDiff = 120;
	uiBackUpOrderWaitCnclAudio = 120;
	uiBackUpOrderWaitCnclStatuses = 120;
	uiBackUpOrderWaitCnclEvents = 120;
	uiBackUpOrderWaitCnclActions = 120;
	uiBackUpOrderWaitCnclArchive = 120;

	uiBackUpOrderWaitMessFull = 60;
	uiBackUpOrderWaitMessSlow = 60;
	uiBackUpOrderWaitMessDiff = 60;
	uiBackUpOrderWaitMessAudio = 60;
	uiBackUpOrderWaitMessStatuses = 60;
	uiBackUpOrderWaitMessEvents = 60;
	uiBackUpOrderWaitMessActions = 60;
	uiBackUpOrderWaitMessArchive = 60;	
	
	ucCaptEnabledEvents = 0;
	ucCaptEnabledStatuses = 1;
	ucCaptEnabledVideo = 1;
	ucCaptEnabledAudio = 1;
	ucCaptEnabledActions = 0;
	uiErrorCameraRestart = 1;
	uiErrorCameraRestartWait = 60;
	uiErrorAudioRestart = 1;
	uiErrorAudioRestartWait = 60;
	uiErrorVideoRestart = 1;
	uiErrorVideoRestartWait = 60;
	
	memset(ucBackUpOrderAddrFull, 0, 32);
	memset(ucBackUpOrderAddrSlow, 0, 32);
	memset(ucBackUpOrderAddrDiff, 0, 32);
	memset(ucBackUpOrderAddrAudio, 0, 32);
	memset(ucBackUpOrderAddrStatuses, 0, 32);
	memset(ucBackUpOrderAddrEvents, 0, 32);
	memset(ucBackUpOrderAddrActions, 0, 32);
	memset(ucBackUpOrderAddrArchive, 0, 32);
	
	memset(ucCaptureStartPathFull, 0, MAX_PATH);
	memset(ucCaptureStartPathSlow, 0, MAX_PATH);
	memset(ucCaptureStartPathDiff, 0, MAX_PATH);
	memset(ucCaptureStartPathAudio, 0, MAX_PATH);
	memset(ucCaptureStartPathStatuses, 0, MAX_PATH);
	memset(ucCaptureStartPathEvents, 0, MAX_PATH);
	memset(ucCaptureStartPathActions, 0, MAX_PATH);
	memset(ucCaptureStartPathArchive, 0, MAX_PATH);
		
	cStreamerExData[0] = 0;
	cStreamerExData[1] = 0;

	memset(cCameraRectFile, 0, 256);
	strcpy(cCameraRectFile, "CameraRectFile.jpg");
	memset(cCameraShotFile, 0, 256);
	strcpy(cCameraShotFile, "CameraShotFile.jpg");
	memset(cCameraSensorFile, 0, 256);
	strcpy(cCameraSensorFile, "CameraSensorFile.jpg");
	
	cFileWriterService = 0;
	
	ucBackUpOrderEnableFull = 0;
	ucBackUpOrderEnableSlow = 0;
	ucBackUpOrderEnableDiff = 0;
	ucBackUpOrderEnableAudio = 0;
	ucBackUpOrderEnableStatuses = 0;
	ucBackUpOrderEnableEvents = 0;
	ucBackUpOrderEnableActions = 0;
	ucBackUpOrderEnableArchive = 0;
	
	memset(cWriterServicePath, 0, MAX_PATH);
	
	memset(cMediaCapturePathFull, 0, MAX_PATH);	
	memset(cMediaCapturePathSlow, 0, MAX_PATH);	
	memset(cMediaCapturePathDiff, 0, MAX_PATH);	
	memset(cMediaCapturePathAudio, 0, MAX_PATH);	
	memset(cMediaCapturePathStatuses, 0, MAX_PATH);	
	memset(cMediaCapturePathEvents, 0, MAX_PATH);	
	memset(cMediaCapturePathActions, 0, MAX_PATH);	
	
	memset(cMediaBackUpPathFull, 0, MAX_PATH);
	memset(cMediaBackUpPathSlow, 0, MAX_PATH);
	memset(cMediaBackUpPathDiff, 0, MAX_PATH);
	memset(cMediaBackUpPathAudio, 0, MAX_PATH);
	memset(cMediaBackUpPathStatuses, 0, MAX_PATH);
	memset(cMediaBackUpPathEvents, 0, MAX_PATH);
	memset(cMediaBackUpPathActions, 0, MAX_PATH);
	
	memset(cMediaArchivePathFull, 0, MAX_PATH);
	memset(cMediaArchivePathSlow, 0, MAX_PATH);
	memset(cMediaArchivePathDiff, 0, MAX_PATH);
	memset(cMediaArchivePathAudio, 0, MAX_PATH);
	memset(cMediaArchivePathStatuses, 0, MAX_PATH);
	memset(cMediaArchivePathEvents, 0, MAX_PATH);
	memset(cMediaArchivePathActions, 0, MAX_PATH);
	
	memset(cLocalBackUpPathFull, 0, MAX_PATH);
	memset(cLocalBackUpPathSlow, 0, MAX_PATH);
	memset(cLocalBackUpPathDiff, 0, MAX_PATH);
	memset(cLocalBackUpPathAudio, 0, MAX_PATH);
	memset(cLocalBackUpPathStatuses, 0, MAX_PATH);
	memset(cLocalBackUpPathEvents, 0, MAX_PATH);
	memset(cLocalBackUpPathActions, 0, MAX_PATH);
	
	memset(cLocalArchivePathFull, 0, MAX_PATH);
	memset(cLocalArchivePathSlow, 0, MAX_PATH);
	memset(cLocalArchivePathDiff, 0, MAX_PATH);
	memset(cLocalArchivePathAudio, 0, MAX_PATH);
	memset(cLocalArchivePathStatuses, 0, MAX_PATH);
	memset(cLocalArchivePathEvents, 0, MAX_PATH);
	memset(cLocalArchivePathActions, 0, MAX_PATH);
	
	ucMediaArchiveModeFull = 0;
	ucMediaArchiveModeSlow = 0;
	ucMediaArchiveModeDiff = 0;
	ucMediaArchiveModeAudio = 0;
	ucMediaArchiveModeStatuses = 0;
	ucMediaArchiveModeEvents = 0;
	ucMediaArchiveModeActions = 0;
	
	memset(cFileLocation, 0, 256);
	memset(cCurrentFileLocation, 0, 256);	
	memset(cFileAlarmLocation, 0, 256);
	memset(cFileLogName, 0, 256);
	strcpy(cFileLogName, "Log.txt");
	memset(cMailAddress, 0, 64);
	memset(cMailServer, 0, 64);
	memset(cMailLogin, 0, 64);
	memset(cMailPassword, 0, 64);
	memset(cMailAuth, 0, 64);
	memset(cSysStatus, 0, 64);
	memset(cCurrentPlayType, 0, 32);	
	memset(cCurrentPlayName, 0, 256);	
	memset(cCurrentPlayDir, 0, 256);	
	memset(cDisplayMessageText, 0, 128);
	cDisplayMessageChanged = 0;
	
	iTimeCor = 40000;
	uiDeviceType = 0;
	iSlideShowTimerSet = 20;
	iSlideShowOnTime = 80000;
	iSlideShowOffTime = 230000;
	iAlarmClocksCnt = 0;
	actAlarmClockInfo = NULL;
	cSettRandomFile = 0;
	cSettRandomDir = 0;
	cCurRandomFile = 0;
	cCurRandomDir = 0;
	iListFilesOrderCnt = 0;	
	iListDirsOrderCnt = 0;
	cPlayDirect = 0;
	uiShowMode = 0;
	uiShowModeCur = 0;
	uiShowerLiveCtrlTime = MAX_SYSTEM_INFO_LIVE;
	iShowerLiveControl = uiShowerLiveCtrlTime;
	
	iRadioCode = 0;
	iVolumeCode = 0;
	
	iSkipIrCodeMaxCnt = SKIPIRCODE_MAX_CNT;
	iSkipIrCodeListCnt = 0;
	cSkipIrCodeList = NULL;
	
	iAlienKeyMaxCnt = ALIENKEY_MAX_CNT;
	iAlienKeyListCnt = 0;
	cAlienKeyList = NULL;
	
	iPtzSettingsListCnt = 0;
	psiPtzSettingsList = NULL;
	memset(&psiPtzHomeSettings, 0, sizeof(psiPtzHomeSettings));
	
	iSkipEventMaxCnt = SKIPEVENT_MAX_CNT;
	iSkipEventListCnt = 0;
	cSkipEventList = NULL;
	uiSkipEventListFilter = 0;
	uiSkipEventListFilter = 0;
	iRectangleCnt = 0;
	riRectangleList = NULL;
	iMailCnt = 0;
	miMailList = NULL;
	iActionManualCnt = 0;
	amiActionManual = NULL;
	iStreamTypeCnt = 0;
	stStreamType = NULL;
	iInternetRadioCnt = 0;
	irInternetRadio = NULL;
	iSecurityKeyCnt = 0;
	skiSecurityKeys = NULL;
	iRadioStationsCnt = 0;
	riRadioStation = NULL;
	iWidgetsCnt = 0;
	wiWidgetList = NULL;
	iDirsCnt = 0;
	diDirList = NULL;
	iModuleCnt = 0;
	miModuleList = NULL;
	iRadioVolume = 1;
	ucBroadCastTCP = 0;
	uiPaddingSize = 0;
	uiTextColor = 255;
	fMenuSize = 1.0f;
	
	iBasicVolume = 50;
	iAlarmVolume = 50;
	iCurrentVolume = 50;
	
	uiCaptureFileSizeFull = 100;
	uiCaptureTimeLimitFull = 300;
	uiCaptureFileSizeSlow = 100;
	uiCaptureTimeLimitSlow = 300;
	uiCaptureFileSizeDiff = 100;
	uiCaptureTimeLimitDiff = 300;
	uiCaptureFileSizeAudio = 10;
	uiCaptureTimeLimitAudio = 300;
	uiCaptureFileSizeStatuses = 10;
	uiCaptureTimeLimitStatuses = 3000;
	uiCaptureFileSizeEvents = 10;
	uiCaptureTimeLimitEvents = 3000;
	uiCaptureFileSizeActions = 10;
	uiCaptureTimeLimitActions = 3000;
	
	ucBackUpModeFull = 0;
	ucBackUpModeSlow = 0;
	ucBackUpModeDiff = 0;
	ucBackUpModeAudio = 0;
	ucBackUpModeStatuses = 0;
	ucBackUpModeEvents = 0;
	ucBackUpModeActions = 0;
	ucBackUpModeArchive = 0;
	
	iIntervalRescanCard = 5000;
	uiOnvifStream = 0;
	uiOnvifPort = 80;
	uiRTSPStream = 0;
	uiRTSPPort = 554;
	uiRTPClntVidPort = 0;
	uiRTPClntAudPort = 0;
	uiRTPServVidPort = 11110;
	uiRTPServAudPort = 11120;
	
	iAlarmEvents = 0;
	iAccessTimer = 0;
	VSync = 1;
	cZoom = 0;
	cDateTimeReference = 0;
	uiFlvBufferSize = 3;
	cAccelerateTextRender = 1;
	uiTerminalMenuID = 0;
	uiMenuWidth = 0;
	uiMenuHeight = 0;
	
	iActionCnt = 0;
	maActionInfo = NULL;
	iModuleEventCnt = 0;
	meModuleEventList = NULL;
	iSoundListCnt = 0;
	mSoundList = NULL;
	
	iIRCommandCnt = 0;
	mIRCommandList = NULL;
	
	iUserCnt = 0;
	uiUserList = NULL;
	
	iMenuKeyCode = 0;
	
	uiMediaIoTimeout = 60;
	
	Connects_Info = NULL;
		
	DBG_LOG_OUT();
		
	return 1;
}

int LoadSettings(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{		
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{				
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				if (strlen(Buff2)) memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);						
				
				if ((SearchStrInData(Buff2, len2, 0, "SYSTEMID=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1))
						memcpy(&miSystemList[0].ID, Buff4, 4);							
				
				if ((SearchStrInData(Buff2, len2, 0, "SYSTEMNAME=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64)))
						strcpy(miSystemList[0].Name, Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "PADDINGSIZE=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiPaddingSize = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "MANUALFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cManualFile, 0, 256);
					strcpy(cManualFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "USERFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cUserFile, 0, 256);
					strcpy(cUserFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "CAMRECTANGLEFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cCamRectangleFile, 0, 256);
					strcpy(cCamRectangleFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "DIRECTORYFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cDirectoryFile, 0, 256);
					strcpy(cDirectoryFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MNLACTIONFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMnlActionFile, 0, 256);
					strcpy(cMnlActionFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "EVNTACTIONFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cEvntActionFile, 0, 256);
					strcpy(cEvntActionFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "IRCODEFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cIrCodeFile, 0, 256);
					strcpy(cIrCodeFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "KEYFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cKeyFile, 0, 256);
					strcpy(cKeyFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "WIDGETFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cWidgetFile, 0, 256);
					strcpy(cWidgetFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "STREAMFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cStreamFile, 0, 256);
					strcpy(cStreamFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "PTZFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cPtzFile, 0, 256);
					strcpy(cPtzFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "STREAMTYPEFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cStreamTypeFile, 0, 256);
					strcpy(cStreamTypeFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MAILFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMailFile, 0, 256);
					strcpy(cMailFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "SOUNDFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cSoundFile, 0, 256);
					strcpy(cSoundFile, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "RADIOFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cRadioFile, 0, 256);
					strcpy(cRadioFile, Buff4);
				}	
				if ((SearchStrInData(Buff2, len2, 0, "ALARMFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cAlarmFile, 0, 256);
					strcpy(cAlarmFile, Buff4);
				}					
				if ((SearchStrInData(Buff2, len2, 0, "MODULEFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cModuleFile, 0, 256);
					strcpy(cModuleFile, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "ALARMPATH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cFileAlarmLocation, 0, 256);
					strcpy(cFileAlarmLocation, Buff4);
				}												
				
				if ((SearchStrInData(Buff2, len2, 0, "TIMEZONE=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iTimeCor = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "TEXTCOLOR=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiTextColor = Hex2Int(Buff4);				
				
				if ((SearchStrInData(Buff2, len2, 0, "MENUSIZE=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) 
					{
						fMenuSize = (float)Str2Float(Buff4);		
						if ((fMenuSize < 0.1f) || (fMenuSize > 10.0f)) fMenuSize = 1.0f;
					}
				
				if ((SearchStrInData(Buff2, len2, 0, "FILELOG=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iFileLog = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SCREENLOG=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iScreenLog = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "MESSAGELOG=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iMessageLog = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "LOCALMESSLOG=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iLocalMessageLog = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "EMAILLOG=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iEmailLog = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "EMAILLOGPAUSE=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) 
					{
						uiLogEmailPauseSize = Str2Int(Buff4);
						if (uiLogEmailPauseSize < 60000) uiLogEmailPauseSize = 60000;
					}
				
				if ((SearchStrInData(Buff2, len2, 0, "SOUNDMESSAGEID=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1))				
				{
					uiSoundMessageID = 0;
					if ((strlen(Buff4) > 0) && (strlen(Buff4) <= 4)) memcpy(&uiSoundMessageID, Buff4, strlen(Buff4));
							else memcpy(&uiSoundMessageID, Buff4, 4);
				}	
				
				if ((SearchStrInData(Buff2, len2, 0, "SOUNDMESSAGEPAUSE=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) 
					{
						uiSoundMessagePauseSize = Str2Int(Buff4);
						if (uiSoundMessagePauseSize < 5000) uiSoundMessagePauseSize = 5000;
					}
				
				if ((SearchStrInData(Buff2, len2, 0, "LOGPATH=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))				
				{
					memset(cFileLogName, 0, 256); 
					strcpy(cFileLogName, Buff4);
				}	
				
				if ((SearchStrInData(Buff2, len2, 0, "LOGIP=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLogIP, 0, 256);
					strcpy(cLogIP, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "LOGEMAILADDR=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))				
				{
					memset(cLogMlList.Address, 0, 64); 
					strcpy(cLogMlList.Address, Buff4);
				}	
				
				if ((SearchStrInData(Buff2, len2, 0, "DEFALARMSOUND=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1))				
				{
					uiDefAlarmSound = 0;
					if ((strlen(Buff4) > 0) && (strlen(Buff4) <= 4)) memcpy(&uiDefAlarmSound, Buff4, strlen(Buff4));
							else memcpy(&uiDefAlarmSound, Buff4, 4);
				}	
				
				if ((SearchStrInData(Buff2, len2, 0, "DEFALARMREPEATS=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiDefAlarmRepeats = Str2Int(Buff4);	
				
				if ((SearchStrInData(Buff2, len2, 0, "VSYNC=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) VSync = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SHOWSPATH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cFileLocation, 0, 256);
					strcpy(cFileLocation, Buff4);
					memset(cCurrentFileLocation, 0, 256);
					strcpy(cCurrentFileLocation, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "CAMERASHOTFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cCameraShotFile, 0, 256);
					strcpy(cCameraShotFile, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "CAMERASENSORFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cCameraSensorFile, 0, 256);
					strcpy(cCameraSensorFile, Buff4);
				}	
				if ((SearchStrInData(Buff2, len2, 0, "CAMERARECTFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cCameraRectFile, 0, 256);
					strcpy(cCameraRectFile, Buff4);
				}				
				
				if ((SearchStrInData(Buff2, len2, 0, "SHOWERLIVECTRLTIME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiShowerLiveCtrlTime = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SHOWMODE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiShowMode = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SLIDESHOWTIMER=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iSlideShowTimerSet = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SLIDESHOWONTIME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iSlideShowOnTime = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SLIDESHOWOFFTIME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iSlideShowOffTime = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "ACCESSLEVEL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iDefAccessLevel = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "INTERVALRESCANCARD=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iIntervalRescanCard = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RADIOVOLUME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iRadioVolume = Str2Int(Buff4);
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BroadCastTCP=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBroadCastTCP = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "FULLCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeFull = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "FULLCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitFull = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "SLOWCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeSlow = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "SLOWCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitSlow = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "DIFFCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeDiff = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "DIFFCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitDiff = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "AUDIOCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeAudio = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "AUDIOCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitAudio = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "STATUSESCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeStatuses = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "STATUSESCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitStatuses = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "EVENTSCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeEvents = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "EVENTSCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitEvents = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "ACTIONSCAPTUREFILESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureFileSizeActions = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "ACTIONSCAPTURETIMESIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiCaptureTimeLimitActions = Str2Int(Buff4);
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeFull = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeSlow = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeDiff = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeAudio = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeStatuses = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeEvents = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeActions = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpModeArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpModeArchive = Str2Int(Buff4);

				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtFull = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtSlow = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtDiff = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtAudio = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtStatuses = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtEvents = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtActions = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderLmtArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderLmtArchive = Str2Int(Buff4);

				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclFull = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclSlow = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclDiff = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclAudio = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclStatuses = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclEvents = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclActions = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitCnclArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitCnclArchive = Str2Int(Buff4);

				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessFull = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessSlow = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessDiff = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessAudio = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessStatuses = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessEvents = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessActions = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderWaitMessArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiBackUpOrderWaitMessArchive = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "EVENTSCAPTENABLED=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucCaptEnabledEvents = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "STATUSESCAPTENABLED=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucCaptEnabledStatuses = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "VIDEOCAPTENABLED=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucCaptEnabledVideo = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "AUDIOCAPTENABLED=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucCaptEnabledAudio = Str2Int(Buff4);				
				if ((SearchStrInData(Buff2, len2, 0, "ACTIONSCAPTENABLED=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucCaptEnabledActions = Str2Int(Buff4);
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "ErrorCameraRestart=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiErrorCameraRestart = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "ErrorCameraRestartWait=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiErrorCameraRestartWait = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "ErrorAudioRestart=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiErrorAudioRestart = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "ErrorAudioRestartWait=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiErrorAudioRestartWait = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "ErrorVideoRestart=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiErrorVideoRestart = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "ErrorVideoRestartWait=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiErrorVideoRestartWait = Str2Int(Buff4);
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrFull, 0, 32);
					strcpy(ucBackUpOrderAddrFull, Buff4);
				}			
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrSlow, 0, 32);
					strcpy(ucBackUpOrderAddrSlow, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrDiff, 0, 32);
					strcpy(ucBackUpOrderAddrDiff, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrAudio, 0, 32);
					strcpy(ucBackUpOrderAddrAudio, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrStatuses, 0, 32);
					strcpy(ucBackUpOrderAddrStatuses, Buff4);
				}			
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrEvents, 0, 32);
					strcpy(ucBackUpOrderAddrEvents, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrActions, 0, 32);
					strcpy(ucBackUpOrderAddrActions, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderAddrArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1))
				{
					memset(ucBackUpOrderAddrArchive, 0, 32);
					strcpy(ucBackUpOrderAddrArchive, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathFull, 0, MAX_PATH);
					strcpy(ucCaptureStartPathFull, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathSlow, 0, MAX_PATH);
					strcpy(ucCaptureStartPathSlow, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathDiff, 0, MAX_PATH);
					strcpy(ucCaptureStartPathDiff, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathAudio, 0, MAX_PATH);
					strcpy(ucCaptureStartPathAudio, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathStatuses, 0, MAX_PATH);
					strcpy(ucCaptureStartPathStatuses, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathEvents, 0, MAX_PATH);
					strcpy(ucCaptureStartPathEvents, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathActions, 0, MAX_PATH);
					strcpy(ucCaptureStartPathActions, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "CaptureStartPathArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(ucCaptureStartPathArchive, 0, MAX_PATH);
					strcpy(ucCaptureStartPathArchive, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "BASICVOLUME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iBasicVolume = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "ALARMVOLUME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iAlarmVolume = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RANDOMFILE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cSettRandomFile = Str2Int(Buff4) & 1;
				
				if ((SearchStrInData(Buff2, len2, 0, "RANDOMDIR=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cSettRandomDir = Str2Int(Buff4) & 1;
				
				if ((SearchStrInData(Buff2, len2, 0, "STREAMTIME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iStreamTimeMaxSet = Str2Int(Buff4);				
				
				if ((SearchStrInData(Buff2, len2, 0, "CAMERATIME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) iCameraTimeMaxSet = Str2Int(Buff4);				
				
				if ((SearchStrInData(Buff2, len2, 0, "WEBSERVER=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiWEBServer = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTSP=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTSPStream = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "ONVIF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiOnvifStream = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTSPFORCEAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTSPForceAudio = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTSPPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTSPPort = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "ONVIFPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiOnvifPort = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTPCLNTVIDPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTPClntVidPort = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTPCLNTAUDPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTPClntAudPort = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTPSERVVIDPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTPServVidPort = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTPSERVAUDPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiRTPServAudPort = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "RTSPAUTH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) RtspAuth = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "ONVIFAUTH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) OnvifAuth = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "WEBAUTH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) WebAuth = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "WEBPORT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiWebPort = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "WEBMAXTIMEIDLE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) WebMaxTimeIdle = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "ZOOM=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cZoom = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "DATETIMEREFERENCE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cDateTimeReference = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "NTPSERVER=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))
				{
					memset(cNTPServer, 0, 64);
					strcpy(cNTPServer, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "NewSourcePath=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cNewSourcePath, 0, 256);
					strcpy(cNewSourcePath, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "NewSourceFile=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cNewSourceFile, 0, 256);
					strcpy(cNewSourceFile, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "NewSourceLogin=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cNewSourceLogin, 0, 256);
					strcpy(cNewSourceLogin, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "NewSourcePass=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cNewSourcePass, 0, 256);
					strcpy(cNewSourcePass, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "RebootAfterUpdate=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					cRebootAfterUpdate = Str2Int(Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "FLVBUFFERSIZE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) 
				{
					uiFlvBufferSize = Str2Int(Buff4);
					if (uiFlvBufferSize > 15) uiFlvBufferSize = 3;
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "ACCELERATETEXTRENDER=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cAccelerateTextRender = Str2Int(Buff4);
							
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableFull = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableSlow = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableDiff = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableAudio = Str2Int(Buff4);				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableStatuses = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableEvents = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableActions = Str2Int(Buff4);
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "BackUpOrderEnableArchive=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) ucBackUpOrderEnableArchive = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "FILEWRITERSERVICE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cFileWriterService = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "WRITERSERVICEPATH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, MAX_PATH) == 1))
				{
					memset(cWriterServicePath, 0, MAX_PATH);
					strcpy(cWriterServicePath, Buff4);
				}				
				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathFull, 0, 256);
					strcpy(cMediaCapturePathFull, Buff4);
				}				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHSLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathSlow, 0, 256);
					strcpy(cMediaCapturePathSlow, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathDiff, 0, 256);
					strcpy(cMediaCapturePathDiff, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathAudio, 0, 256);
					strcpy(cMediaCapturePathAudio, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHSTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathStatuses, 0, 256);
					strcpy(cMediaCapturePathStatuses, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathEvents, 0, 256);
					strcpy(cMediaCapturePathEvents, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIACAPTUREPATHACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaCapturePathActions, 0, 256);
					strcpy(cMediaCapturePathActions, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathFull, 0, 256);
					strcpy(cMediaBackUpPathFull, Buff4);
				}				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHSLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathSlow, 0, 256);
					strcpy(cMediaBackUpPathSlow, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathDiff, 0, 256);
					strcpy(cMediaBackUpPathDiff, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathAudio, 0, 256);
					strcpy(cMediaBackUpPathAudio, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHSTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathStatuses, 0, 256);
					strcpy(cMediaBackUpPathStatuses, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathEvents, 0, 256);
					strcpy(cMediaBackUpPathEvents, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIABACKUPPATHACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaBackUpPathActions, 0, 256);
					strcpy(cMediaBackUpPathActions, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathFull, 0, 256);
					strcpy(cMediaArchivePathFull, Buff4);
				}				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHSLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathSlow, 0, 256);
					strcpy(cMediaArchivePathSlow, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathDiff, 0, 256);
					strcpy(cMediaArchivePathDiff, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathAudio, 0, 256);
					strcpy(cMediaArchivePathAudio, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHSTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathStatuses, 0, 256);
					strcpy(cMediaArchivePathStatuses, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathEvents, 0, 256);
					strcpy(cMediaArchivePathEvents, Buff4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEPATHACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cMediaArchivePathActions, 0, 256);
					strcpy(cMediaArchivePathActions, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathFull, 0, 256);
					strcpy(cLocalBackUpPathFull, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathSlow, 0, 256);
					strcpy(cLocalBackUpPathSlow, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathDiff, 0, 256);
					strcpy(cLocalBackUpPathDiff, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathAudio, 0, 256);
					strcpy(cLocalBackUpPathAudio, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathStatuses, 0, 256);
					strcpy(cLocalBackUpPathStatuses, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathEvents, 0, 256);
					strcpy(cLocalBackUpPathEvents, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalBackUpPathActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalBackUpPathActions, 0, 256);
					strcpy(cLocalBackUpPathActions, Buff4);
				}
				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathFull, 0, 256);
					strcpy(cLocalArchivePathFull, Buff4);
				}				
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathSlow, 0, 256);
					strcpy(cLocalArchivePathSlow, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathDiff, 0, 256);
					strcpy(cLocalArchivePathDiff, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathAudio, 0, 256);
					strcpy(cLocalArchivePathAudio, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathStatuses, 0, 256);
					strcpy(cLocalArchivePathStatuses, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathEvents, 0, 256);
					strcpy(cLocalArchivePathEvents, Buff4);
				}
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "LocalArchivePathActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 256) == 1))
				{
					memset(cLocalArchivePathActions, 0, 256);
					strcpy(cLocalArchivePathActions, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODEFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeFull = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODESLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeSlow = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODEDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeDiff = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODEAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeAudio = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODESTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeStatuses = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODEEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeEvents = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MEDIAARCHIVEMODEACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveModeActions = Str2Int(Buff4);
								
				if ((SearchStrInData(Buff2, len2, 0, "MAILADDRESS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))
				{
					memset(cMailAddress, 0, 64);
					strcpy(cMailAddress, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MAILSERVER=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))
				{
					memset(cMailServer, 0, 64);
					strcpy(cMailServer, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MAILLOGIN=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))
				{
					memset(cMailLogin, 0, 64);
					strcpy(cMailLogin, Buff4);
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MAILPASSWORD=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))
				{
					memset(cMailPassword, 0, 64);
					strcpy(cMailPassword, Buff4);	
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "MAILAUTH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1))
				{
					memset(cMailAuth, 0, 64);
					strcpy(cMailAuth, Buff4);	
				}
				
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACEFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceFull = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACESLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceSlow = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACEDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceDiff = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACEAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceAudio = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACESTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceStatuses = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACEEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceEvents = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREMINFREESPACEACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiCaptureMinFreeSpaceActions = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACEFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceFull = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACESLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceSlow = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACEDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceDiff = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACEAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceAudio = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACESTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceStatuses = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACEEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceEvents = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPMINFREESPACEACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiBackUpMinFreeSpaceActions = Str2Int(Buff4);	
				
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACEFULL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceFull = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACESLOW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceSlow = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACEDIFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceDiff = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACEAUDIO=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceAudio = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACESTATUSES=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceStatuses = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACEEVENTS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceEvents = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "ARCHIVEMINFREESPACEACTIONS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiArchiveMinFreeSpaceActions = Str2Int(Buff4);	
		
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromFull = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToFull=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToFull = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromSlow = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToSlow=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToSlow = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromDiff = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToDiff=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToDiff = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromAudio = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToAudio=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToAudio = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromStatuses = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToStatuses=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToStatuses = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromEvents = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToEvents=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToEvents = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeFromActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeFromActions = Str2Int(Buff4);	
				if ((SearchStrInDataCaseIgn(Buff2, len2, 0, "MediaArchiveTimeToActions=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) ucMediaArchiveTimeToActions = Str2Int(Buff4);	
		
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREFILESVIEW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) cCaptureFilesView = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPFILESVIEW=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) cBackUpFilesView = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREFILESLEVEL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) cCaptureFilesLevel = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "BACKUPFILESLEVEL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) cBackUpFilesLevel = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "CAPTUREFILESVIEWDEF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) cCaptureFilesViewDef = Str2Int(Buff4);	

				if ((SearchStrInData(Buff2, len2, 0, "MEDIAIOTIMEOUT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiMediaIoTimeout = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "TERMINALMENUID=") == 1) 
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1))				
				{
					uiTerminalMenuID = 0;
					if ((strlen(Buff4) > 0) && (strlen(Buff4) <= 4)) memcpy(&uiTerminalMenuID, Buff4, strlen(Buff4));
							else memcpy(&uiTerminalMenuID, Buff4, 4);
				}
				if ((SearchStrInData(Buff2, len2, 0, "MENUWIDTH=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiMenuWidth = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "MENUHEIGHT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiMenuHeight = Str2Int(Buff4);	
				
				if ((SearchStrInData(Buff2, len2, 0, "MAXFILECOPYTIMEBREAKVALUE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiMaxFileCopyTimeBreakValue = Str2Int(Buff4);	
				if ((SearchStrInData(Buff2, len2, 0, "MAXFILECOPYTIMEMESSAGEVALUE=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 64) == 1)) uiMaxFileCopyTimeMessageValue = Str2Int(Buff4);
			}
		}
	}
		
	fclose(f);	
	UpdateDeviceType();	
	
	DBG_LOG_OUT();
	
	return 1;
}

int SaveSettings()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cConfigFile, 1);
	if ((strlen(cConfigFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cConfigFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SystemID=%.4s\n", (char*)&miSystemList[0].ID); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SystemName=%s\n", miSystemList[0].Name); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "PaddingSize=%i\n", uiPaddingSize); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ManualFile=%s\n", cManualFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "UserFile=%s\n", cUserFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CamRectangleFile=%s\n", cCamRectangleFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "DirectoryFile=%s\n", cDirectoryFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MnlActionFile=%s\n", cMnlActionFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "EvntActionFile=%s\n", cEvntActionFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "IrCodeFile=%s\n", cIrCodeFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "KeyFile=%s\n", cKeyFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "WidgetFile=%s\n", cWidgetFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "StreamFile=%s\n", cStreamFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "PtzFile=%s\n", cPtzFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "StreamTypeFile=%s\n", cStreamTypeFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MailFile=%s\n", cMailFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SoundFile=%s\n", cSoundFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RadioFile=%s\n", cRadioFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AlarmFile=%s\n", cAlarmFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ModuleFile=%s\n", cModuleFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AlarmPath=%s\n", cFileAlarmLocation); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "TimeZone=%i\n", iTimeCor); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "TextColor=%06X\n", uiTextColor); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MenuSize=%g\n", fMenuSize); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "FileLog=%i\n", iFileLog); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ScreenLog=%i\n", iScreenLog); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MessageLog=%i\n", iMessageLog);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalMessLog=%i\n", iLocalMessageLog); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "EmailLog=%i\n", iEmailLog); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "EmailLogPause=%i\n", uiLogEmailPauseSize); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SoundMessagePause=%i\n", uiSoundMessagePauseSize); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LogPath=%s\n", cFileLogName); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "DefAlarmSound=%.4s\n", (char*)&uiDefAlarmSound); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SoundMessageID=%.4s\n", (char*)&uiSoundMessageID); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "DefAlarmRepeats=%i\n", uiDefAlarmRepeats); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "VSync=%i\n", VSync); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ShowsPath=%s\n", cFileLocation); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CameraShotFile=%s\n", cCameraShotFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CameraSensorFile=%s\n", cCameraSensorFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CameraRectFile=%s\n", cCameraRectFile);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ShowerLiveCtrlTime=%i\n", uiShowerLiveCtrlTime); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ShowMode=%i\n", uiShowMode); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SlideShowTimer=%i\n", iSlideShowTimerSet);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SlideShowOnTime=%i\n", iSlideShowOnTime);	fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SlideShowOffTime=%i\n", iSlideShowOffTime); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AccessLevel=%i\n", iDefAccessLevel); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "IntervalRescanCard=%i\n", iIntervalRescanCard); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RadioVolume=%i\n", iRadioVolume); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BroadCastTCP=%i\n", ucBroadCastTCP); fputs(Buff1, f);	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "FullCaptureFileSize=%i\n", uiCaptureFileSizeFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "FullCaptureTimeSize=%i\n", uiCaptureTimeLimitFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SlowCaptureFileSize=%i\n", uiCaptureFileSizeSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "SlowCaptureTimeSize=%i\n", uiCaptureTimeLimitSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "DiffCaptureFileSize=%i\n", uiCaptureFileSizeDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "DiffCaptureTimeSize=%i\n", uiCaptureTimeLimitDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AudioCaptureFileSize=%i\n", uiCaptureFileSizeAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AudioCaptureTimeSize=%i\n", uiCaptureTimeLimitAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "StatusesCaptureFileSize=%i\n", uiCaptureFileSizeStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "StatusesCaptureTimeSize=%i\n", uiCaptureTimeLimitStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "EventsCaptureFileSize=%i\n", uiCaptureFileSizeEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "EventsCaptureTimeSize=%i\n", uiCaptureTimeLimitEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ActionsCaptureFileSize=%i\n", uiCaptureFileSizeActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ActionsCaptureTimeSize=%i\n", uiCaptureTimeLimitActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeFull=%i\n", ucBackUpModeFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeSlow=%i\n", ucBackUpModeSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeDiff=%i\n", ucBackUpModeDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeAudio=%i\n", ucBackUpModeAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeStatuses=%i\n", ucBackUpModeStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeEvents=%i\n", ucBackUpModeEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeActions=%i\n", ucBackUpModeActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpModeArchive=%i\n", ucBackUpModeArchive); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtFull=%i\n", ucBackUpOrderLmtFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtSlow=%i\n", ucBackUpOrderLmtSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtDiff=%i\n", ucBackUpOrderLmtDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtAudio=%i\n", ucBackUpOrderLmtAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtStatuses=%i\n", ucBackUpOrderLmtStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtEvents=%i\n", ucBackUpOrderLmtEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtActions=%i\n", ucBackUpOrderLmtActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderLmtArchive=%i\n", ucBackUpOrderLmtArchive); fputs(Buff1, f);

	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclFull=%i\n", uiBackUpOrderWaitCnclFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclSlow=%i\n", uiBackUpOrderWaitCnclSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclDiff=%i\n", uiBackUpOrderWaitCnclDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclAudio=%i\n", uiBackUpOrderWaitCnclAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclStatuses=%i\n", uiBackUpOrderWaitCnclStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclEvents=%i\n", uiBackUpOrderWaitCnclEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclActions=%i\n", uiBackUpOrderWaitCnclActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitCnclArchive=%i\n", uiBackUpOrderWaitCnclArchive); fputs(Buff1, f);

	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessFull=%i\n", uiBackUpOrderWaitMessFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessSlow=%i\n", uiBackUpOrderWaitMessSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessDiff=%i\n", uiBackUpOrderWaitMessDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessAudio=%i\n", uiBackUpOrderWaitMessAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessStatuses=%i\n", uiBackUpOrderWaitMessStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessEvents=%i\n", uiBackUpOrderWaitMessEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessActions=%i\n", uiBackUpOrderWaitMessActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderWaitMessArchive=%i\n", uiBackUpOrderWaitMessArchive); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "EventsCaptEnabled=%i\n", ucCaptEnabledEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "StatusesCaptEnabled=%i\n", ucCaptEnabledStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "VideoCaptEnabled=%i\n", ucCaptEnabledVideo); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AudioCaptEnabled=%i\n", ucCaptEnabledAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ActionsCaptEnabled=%i\n", ucCaptEnabledActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ErrorCameraRestart=%i\n", uiErrorCameraRestart); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ErrorCameraRestartWait=%i\n", uiErrorCameraRestartWait); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ErrorAudioRestart=%i\n", uiErrorAudioRestart); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ErrorAudioRestartWait=%i\n", uiErrorAudioRestartWait); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ErrorVideoRestart=%i\n", uiErrorVideoRestart); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ErrorVideoRestartWait=%i\n", uiErrorVideoRestartWait); fputs(Buff1, f);
		
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrFull=%s\n", ucBackUpOrderAddrFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrSlow=%s\n", ucBackUpOrderAddrSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrDiff=%s\n", ucBackUpOrderAddrDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrAudio=%s\n", ucBackUpOrderAddrAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrStatuses=%s\n", ucBackUpOrderAddrStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrEvents=%s\n", ucBackUpOrderAddrEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrActions=%s\n", ucBackUpOrderAddrActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderAddrArchive=%s\n", ucBackUpOrderAddrArchive); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathFull=%s\n", ucCaptureStartPathFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathSlow=%s\n", ucCaptureStartPathSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathDiff=%s\n", ucCaptureStartPathDiff); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathAudio=%s\n", ucCaptureStartPathAudio); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathStatuses=%s\n", ucCaptureStartPathStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathEvents=%s\n", ucCaptureStartPathEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathActions=%s\n", ucCaptureStartPathActions); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureStartPathArchive=%s\n", ucCaptureStartPathArchive); fputs(Buff1, f);	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BasicVolume=%i\n", iBasicVolume); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AlarmVolume=%i\n", iAlarmVolume); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RandomFile=%i\n", cSettRandomFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RandomDir=%i\n", cSettRandomDir); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LogIP=%s\n", cLogIP); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LOGEMAILADDR=%s\n", cLogMlList.Address); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "StreamTime=%i\n", iStreamTimeMaxSet); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CameraTime=%i\n", iCameraTimeMaxSet); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "WebServer=%i\n", uiWEBServer); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ONVIF=%i\n", uiOnvifStream); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ONVIFPort=%i\n", uiOnvifPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ONVIFAuth=%i\n", OnvifAuth); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTSP=%i\n", uiRTSPStream); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTSPForceAudio=%i\n", uiRTSPForceAudio); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTSPPort=%i\n", uiRTSPPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTPClntVidPort=%i\n", uiRTPClntVidPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTPClntAudPort=%i\n", uiRTPClntAudPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTPServVidPort=%i\n", uiRTPServVidPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTPServAudPort=%i\n", uiRTPServAudPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RTSPAuth=%i\n", RtspAuth); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "WebAuth=%i\n", WebAuth); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "WebPort=%i\n", uiWebPort); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "WebMaxTimeIdle=%i\n", WebMaxTimeIdle); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "Zoom=%i\n", cZoom); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "DateTimeReference=%i\n", cDateTimeReference); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "NTPServer=%s\n", cNTPServer); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "NewSourcePath=%s\n", cNewSourcePath); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "NewSourceFile=%s\n", cNewSourceFile); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "NewSourceLogin=%s\n", cNewSourceLogin); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "NewSourcePass=%s\n", cNewSourcePass); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "RebootAfterUpdate=%i\n", cRebootAfterUpdate); fputs(Buff1, f);
	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "FlvBufferSize=%i\n", uiFlvBufferSize); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "AccelerateTextRender=%i\n", cAccelerateTextRender); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableFull=%i\n", ucBackUpOrderEnableFull); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableSlow=%i\n", ucBackUpOrderEnableSlow); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableDiff=%i\n", ucBackUpOrderEnableDiff); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableAudio=%i\n", ucBackUpOrderEnableAudio); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableStatuses=%i\n", ucBackUpOrderEnableStatuses); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableEvents=%i\n", ucBackUpOrderEnableEvents); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableActions=%i\n", ucBackUpOrderEnableActions); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpOrderEnableArchive=%i\n", ucBackUpOrderEnableArchive); fputs(Buff1, f);	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "FileWriterService=%i\n", cFileWriterService); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "WriterServicePath=%s\n", cWriterServicePath); fputs(Buff1, f);	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathFull=%s\n", cMediaCapturePathFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathSlow=%s\n", cMediaCapturePathSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathDiff=%s\n", cMediaCapturePathDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathAudio=%s\n", cMediaCapturePathAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathStatuses=%s\n", cMediaCapturePathStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathEvents=%s\n", cMediaCapturePathEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaCapturePathActions=%s\n", cMediaCapturePathActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathFull=%s\n", cMediaBackUpPathFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathSlow=%s\n", cMediaBackUpPathSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathDiff=%s\n", cMediaBackUpPathDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathAudio=%s\n", cMediaBackUpPathAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathStatuses=%s\n", cMediaBackUpPathStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathEvents=%s\n", cMediaBackUpPathEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaBackUpPathActions=%s\n", cMediaBackUpPathActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathFull=%s\n", cMediaArchivePathFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathSlow=%s\n", cMediaArchivePathSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathDiff=%s\n", cMediaArchivePathDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathAudio=%s\n", cMediaArchivePathAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathStatuses=%s\n", cMediaArchivePathStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathEvents=%s\n", cMediaArchivePathEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchivePathActions=%s\n", cMediaArchivePathActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathFull=%s\n", cLocalBackUpPathFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathSlow=%s\n", cLocalBackUpPathSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathDiff=%s\n", cLocalBackUpPathDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathAudio=%s\n", cLocalBackUpPathAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathStatuses=%s\n", cLocalBackUpPathStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathEvents=%s\n", cLocalBackUpPathEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalBackUpPathActions=%s\n", cLocalBackUpPathActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathFull=%s\n", cLocalArchivePathFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathSlow=%s\n", cLocalArchivePathSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathDiff=%s\n", cLocalArchivePathDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathAudio=%s\n", cLocalArchivePathAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathStatuses=%s\n", cLocalArchivePathStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathEvents=%s\n", cLocalArchivePathEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "LocalArchivePathActions=%s\n", cLocalArchivePathActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeFull=%i\n", ucMediaArchiveModeFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeSlow=%i\n", ucMediaArchiveModeSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeDiff=%i\n", ucMediaArchiveModeDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeAudio=%i\n", ucMediaArchiveModeAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeStatuses=%i\n", ucMediaArchiveModeStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeEvents=%i\n", ucMediaArchiveModeEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveModeActions=%i\n", ucMediaArchiveModeActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MailAddress=%s\n", cMailAddress); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MailServer=%s\n", cMailServer); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MailLogin=%s\n", cMailLogin); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MailPassword=%s\n", cMailPassword); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MailAuth=%s\n", cMailAuth); fputs(Buff1, f);	
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceFull=%i\n", uiCaptureMinFreeSpaceFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceSlow=%i\n", uiCaptureMinFreeSpaceSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceDiff=%i\n", uiCaptureMinFreeSpaceDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceAudio=%i\n", uiCaptureMinFreeSpaceAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceStatuses=%i\n", uiCaptureMinFreeSpaceStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceEvents=%i\n", uiCaptureMinFreeSpaceEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureMinFreeSpaceActions=%i\n", uiCaptureMinFreeSpaceActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceFull=%i\n", uiBackUpMinFreeSpaceFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceSlow=%i\n", uiBackUpMinFreeSpaceSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceDiff=%i\n", uiBackUpMinFreeSpaceDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceAudio=%i\n", uiBackUpMinFreeSpaceAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceStatuses=%i\n", uiBackUpMinFreeSpaceStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceEvents=%i\n", uiBackUpMinFreeSpaceEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpMinFreeSpaceActions=%i\n", uiBackUpMinFreeSpaceActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceFull=%i\n", uiArchiveMinFreeSpaceFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceSlow=%i\n", uiArchiveMinFreeSpaceSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceDiff=%i\n", uiArchiveMinFreeSpaceDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceAudio=%i\n", uiArchiveMinFreeSpaceAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceStatuses=%i\n", uiArchiveMinFreeSpaceStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceEvents=%i\n", uiArchiveMinFreeSpaceEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "ArchiveMinFreeSpaceActions=%i\n", uiArchiveMinFreeSpaceActions); fputs(Buff1, f);

	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromFull=%i\n", ucMediaArchiveTimeFromFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToFull=%i\n", ucMediaArchiveTimeToFull); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromSlow=%i\n", ucMediaArchiveTimeFromSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToSlow=%i\n", ucMediaArchiveTimeToSlow); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromDiff=%i\n", ucMediaArchiveTimeFromDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToDiff=%i\n", ucMediaArchiveTimeToDiff); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromAudio=%i\n", ucMediaArchiveTimeFromAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToAudio=%i\n", ucMediaArchiveTimeToAudio); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromStatuses=%i\n", ucMediaArchiveTimeFromStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToStatuses=%i\n", ucMediaArchiveTimeToStatuses); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromEvents=%i\n", ucMediaArchiveTimeFromEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToEvents=%i\n", ucMediaArchiveTimeToEvents); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeFromActions=%i\n", ucMediaArchiveTimeFromActions); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaArchiveTimeToActions=%i\n", ucMediaArchiveTimeToActions); fputs(Buff1, f);
	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureFilesView=%i\n", cCaptureFilesView); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpFilesView=%i\n", cBackUpFilesView); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureFilesLevel=%i\n", cCaptureFilesLevel); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "BackUpFilesLevel=%i\n", cBackUpFilesLevel); fputs(Buff1, f);	
	memset(Buff1, 0, 1024);	sprintf(Buff1, "CaptureFilesViewDef=%i\n", cCaptureFilesViewDef); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MediaIoTimeout=%i\n", uiMediaIoTimeout); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "TerminalMenuID=%.4s\n", (char*)&uiTerminalMenuID); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MenuWidth=%i\n", uiMenuWidth); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MenuHeight=%i\n", uiMenuHeight); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MaxFileCopyTimeBreakValue=%i\n", uiMaxFileCopyTimeBreakValue); fputs(Buff1, f);
	memset(Buff1, 0, 1024);	sprintf(Buff1, "MaxFileCopyTimeMessageValue=%i\n", uiMaxFileCopyTimeMessageValue); fputs(Buff1, f);
		
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestSettings(int iMode)
{	
	DBG_LOG_IN();
	
	if (iMode) DBG_MUTEX_LOCK(&system_mutex);	
	if (miSystemList[0].ID == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SYSTEMID value is null\n");		
		else WEB_AddMessageInList("TestSettings: SYSTEMID value is null");	
	}			
	if (strlen(miSystemList[0].Name) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SYSTEMNAME(%i) value is null\n");
		else WEB_AddMessageInList("TestSettings: SYSTEMNAME(%i) value is null");
	}
	if (strlen(cFileAlarmLocation) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: ALARMPATH value is null\n");
		else WEB_AddMessageInList("TestSettings: ALARMPATH value is null");
	}
	if (strlen(cStreamFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: STREAMFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: STREAMFILE value is null");
	}
	if (strlen(cPtzFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: PTZFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: PTZFILE value is null");
	}	
	if (strlen(cStreamTypeFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: STREAMTYPEFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: STREAMTYPEFILE value is null");
	}	
	if (strlen(cRadioFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: RADIOFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: RADIOFILE value is null");	
	}
	if (strlen(cMailFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MAILFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: MAILFILE value is null");	
	}
	if (strlen(cAlarmFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: ALARMFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: ALARMFILE value is null");	
	}		
	if (uiTextColor == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: TEXTCOLOR value is null (BLACK)\n");
		else WEB_AddMessageInList("TestSettings: TEXTCOLOR value is null (BLACK)");
	}
	if ((fMenuSize < 0.1f) || (fMenuSize > 10.0f)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MENUSIZE value not between 0.1 and 10\n");
		else WEB_AddMessageInList("TestSettings: MENUSIZE value not between 0.1 and 10");
	}
	if ((VSync < 0) || (VSync > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: VSYNC value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: VSYNC value not between 0 and 1");
	}
	if (strlen(cFileLocation) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: SHOWSPATH value is null\n");
		else WEB_AddMessageInList("TestSettings: SHOWSPATH value is null");
	}
	if (strlen(cCameraShotFile) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CAMERASHOTFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: CAMERASHOTFILE value is null");
	}
	if (strlen(cCameraSensorFile) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CAMERASENSORFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: CAMERASENSORFILE value is null");
	}
	if (strlen(cCameraRectFile) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CAMERARECTFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: CAMERARECTFILE value is null");
	}
	if ((uiShowMode < 0) || (uiShowMode > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SHOWMODE value not between 0 and 2\n");	
		else WEB_AddMessageInList("TestSettings: SHOWMODE value not between 0 and 2");	
	}
	if ((iSlideShowTimerSet < 0) || (iSlideShowTimerSet > 1000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SLIDESHOWTIMER value not between 0 and 1000\n");
		else WEB_AddMessageInList("TestSettings: SLIDESHOWTIMER value not between 0 and 1000");
	}
	if ((iSlideShowOnTime < 0) || (iSlideShowOnTime > 235959)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SLIDESHOWONTIME value not between 0 and 235959\n");
		else WEB_AddMessageInList("TestSettings: SLIDESHOWONTIME value not between 0 and 235959");
	}
	if ((iSlideShowOffTime < 0) || (iSlideShowOffTime > 235959)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SLIDESHOWOFFTIME value not between 0 and 235959\n");
		else WEB_AddMessageInList("TestSettings: SLIDESHOWOFFTIME value not between 0 and 235959");
	}
	if ((iDefAccessLevel < 0) || (iDefAccessLevel > MAX_ACCESS_LEVELS)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ACCESSLEVEL value not between 0 and %i\n", MAX_ACCESS_LEVELS);
		else WEB_AddMessageInList("TestSettings: ACCESSLEVEL value not between 0 and %i", MAX_ACCESS_LEVELS);
	}
	if ((iIntervalRescanCard < 0) || (iIntervalRescanCard > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: INTERVALRESCANCARD value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: INTERVALRESCANCARD value not between 0 and 20000");
	}
	
	if (uiBackUpOrderWaitCnclFull < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclFull value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclFull value < 10");
	}
	if (uiBackUpOrderWaitCnclSlow < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclSlow value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclSlow value < 10");
	}
	if (uiBackUpOrderWaitCnclDiff < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclDiff value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclDiff value < 10");
	}
	if (uiBackUpOrderWaitCnclAudio < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclAudio value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclAudio value < 10");
	}
	if (uiBackUpOrderWaitCnclStatuses < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclStatuses value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclStatuses value < 10");
	}
	if (uiBackUpOrderWaitCnclEvents < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclEvents value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclEvents value < 10");
	}
	if (uiBackUpOrderWaitCnclActions < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitCnclActions value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitCnclActions value < 10");
	}

	if (uiBackUpOrderWaitMessFull < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessFull value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessFull value < 10");
	}
	if (uiBackUpOrderWaitMessSlow < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessSlow value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessSlow value < 10");
	}
	if (uiBackUpOrderWaitMessDiff < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessDiff value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessDiff value < 10");
	}
	if (uiBackUpOrderWaitMessAudio < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessAudio value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessAudio value < 10");
	}
	if (uiBackUpOrderWaitMessStatuses < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessStatuses value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessStatuses value < 10");
	}
	if (uiBackUpOrderWaitMessEvents < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessEvents value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessEvents value < 10");
	}
	if (uiBackUpOrderWaitMessActions < 10) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpOrderWaitMessActions value < 10\n");
		else WEB_AddMessageInList("TestSettings: BackUpOrderWaitMessActions value < 10");
	}	
	
	if ((uiCaptureFileSizeFull < 10) || (uiCaptureFileSizeFull > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: FASTCAPTUREFILESIZE value not between 10 and 20000\n");
		else WEB_AddMessageInList("TestSettings: FASTCAPTUREFILESIZE value not between 10 and 20000");
	}
	if ((uiCaptureTimeLimitFull < 10) || (uiCaptureTimeLimitFull > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: FASTCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: FASTCAPTURETIMESIZE value not between 10 and 90000");
	}
	if ((uiCaptureFileSizeSlow < 10) || (uiCaptureFileSizeSlow > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SLOWCAPTUREFILESIZE value not between 10 and 20000\n");
		else WEB_AddMessageInList("TestSettings: SLOWCAPTUREFILESIZE value not between 10 and 20000");
	}
	if ((uiCaptureTimeLimitSlow < 10) || (uiCaptureTimeLimitSlow > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SLOWCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: SLOWCAPTURETIMESIZE value not between 10 and 90000");
	}
	if ((uiCaptureFileSizeDiff < 10) || (uiCaptureFileSizeDiff > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: DIFFCAPTUREFILESIZE value not between 10 and 20000\n");
		else WEB_AddMessageInList("TestSettings: DIFFCAPTUREFILESIZE value not between 10 and 20000");
	}
	if ((uiCaptureTimeLimitDiff < 10) || (uiCaptureTimeLimitDiff > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: DIFFCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: DIFFCAPTURETIMESIZE value not between 10 and 90000");
	}
	if ((uiCaptureFileSizeAudio < 1) || (uiCaptureFileSizeAudio > 2000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: AUDIOCAPTUREFILESIZE value not between 1 and 2000\n");
		else WEB_AddMessageInList("TestSettings: AUDIOCAPTUREFILESIZE value not between 1 and 2000");
	}
	if ((uiCaptureTimeLimitAudio < 10) || (uiCaptureTimeLimitAudio > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: AUDIOCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: AUDIOCAPTURETIMESIZE value not between 10 and 90000");
	}
	if ((uiCaptureFileSizeStatuses < 1) || (uiCaptureFileSizeStatuses > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: STATUSESCAPTUREFILESIZE value not between 1 and 20000\n");
		else WEB_AddMessageInList("TestSettings: STATUSESCAPTUREFILESIZE value not between 1 and 20000");
	}
	if ((uiCaptureTimeLimitStatuses < 10) || (uiCaptureTimeLimitStatuses > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: STATUSESCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: STATUSESCAPTURETIMESIZE value not between 10 and 90000");
	}
	if ((uiCaptureFileSizeEvents < 1) || (uiCaptureFileSizeEvents > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: EVENTSCAPTUREFILESIZE value not between 1 and 20000\n");
		else WEB_AddMessageInList("TestSettings: EVENTSCAPTUREFILESIZE value not between 1 and 20000");
	}
	if ((uiCaptureTimeLimitEvents < 10) || (uiCaptureTimeLimitEvents > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: EVENTSCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: EVENTSCAPTURETIMESIZE value not between 10 and 90000");
	}
	if ((uiCaptureFileSizeActions < 1) || (uiCaptureFileSizeActions > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ACTIONSCAPTUREFILESIZE value not between 1 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ACTIONSCAPTUREFILESIZE value not between 1 and 20000");
	}
	if ((uiCaptureTimeLimitActions < 10) || (uiCaptureTimeLimitActions > 90000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ACTIONSCAPTURETIMESIZE value not between 10 and 90000\n");
		else WEB_AddMessageInList("TestSettings: ACTIONSCAPTURETIMESIZE value not between 10 and 90000");
	}
	
	if ((ucBackUpModeFull < 0) || (ucBackUpModeFull > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeFull value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeFull value not between 0 and 2");
	}
	if ((ucBackUpModeSlow < 0) || (ucBackUpModeSlow > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeSlow value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeSlow value not between 0 and 2");
	}
	if ((ucBackUpModeDiff < 0) || (ucBackUpModeDiff > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeDiff value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeDiff value not between 0 and 2");
	}
	if ((ucBackUpModeAudio < 0) || (ucBackUpModeAudio > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeAudio value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeAudio value not between 0 and 2");
	}
	if ((ucBackUpModeStatuses < 0) || (ucBackUpModeStatuses > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeStatuses value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeStatuses value not between 0 and 2");
	}
	if ((ucBackUpModeEvents < 0) || (ucBackUpModeEvents > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeEvents value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeEvents value not between 0 and 2");
	}	
	if ((ucBackUpModeActions < 0) || (ucBackUpModeActions > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeActions value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeActions value not between 0 and 2");
	}
	if ((ucBackUpModeArchive < 0) || (ucBackUpModeArchive > 2)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpModeArchive value not between 0 and 2\n");
		else WEB_AddMessageInList("TestSettings: BackUpModeArchive value not between 0 and 2");
	}
	
	if ((iBasicVolume < 0) || (iBasicVolume > 100)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BASICVOLUME value not between 0 and 100\n");
		else WEB_AddMessageInList("TestSettings: BASICVOLUME value not between 0 and 100");
	}	
	if ((iAlarmVolume < 0) || (iAlarmVolume > 100)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ALARMVOLUME value not between 0 and 100\n");
		else WEB_AddMessageInList("TestSettings: ALARMVOLUME value not between 0 and 100");
	}
	if ((cSettRandomFile < 0) || (cSettRandomFile > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RANDOMFILE value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: RANDOMFILE value not between 0 and 1");
	}
	if ((cSettRandomDir < 0) || (cSettRandomDir > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RANDOMDIR value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: RANDOMDIR value not between 0 and 1");
	}
	if ((uiWEBServer < 0) || (uiWEBServer > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: WEBSERVER value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: WEBSERVER value not between 0 and 1");
	}
	if ((uiRTSPForceAudio < 0) || (uiRTSPForceAudio > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTSPFORCEAUDIO value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: RTSPFORCEAUDIO value not between 0 and 1");
	}
	if ((uiRTSPStream < 0) || (uiRTSPStream > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTSP value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: RTSP value not between 0 and 1");
	}
	if ((uiRTSPPort < 1) || (uiRTSPPort > 65535)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTSPPORT value not between 1 and 65535\n");
		else WEB_AddMessageInList("TestSettings: RTSPPORT value not between 1 and 65535");
	}
	if ((uiRTPClntVidPort < 0) || (uiRTPClntVidPort > 65535)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTPCLNTVIDPORT value not between 0 and 65535\n");
		else WEB_AddMessageInList("TestSettings: RTPCLNTVIDPORT value not between 0 and 65535");
	}
	if ((uiRTPClntAudPort < 0) || (uiRTPClntAudPort > 65535)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTPCLNTAUDPORT value not between 0 and 65535\n");
		else WEB_AddMessageInList("TestSettings: RTPCLNTAUDPORT value not between 0 and 65535");
	}
	if ((uiRTPServVidPort < 0) || (uiRTPServVidPort > 65535)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTPSERVVIDPORT value not between 0 and 65535\n");
		else WEB_AddMessageInList("TestSettings: RTPSERVVIDPORT value not between 0 and 65535");
	}
	if ((uiRTPServAudPort < 0) || (uiRTPServAudPort > 65535)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTPSERVAUDPORT value not between 0 and 65535\n");
		else WEB_AddMessageInList("TestSettings: RTPSERVAUDPORT value not between 0 and 65535");
	}
	if ((RtspAuth < 0) || (RtspAuth > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RTSPAUTH value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: RTSPAUTH value not between 0 and 1");
	}
	if ((WebAuth < 0) || (WebAuth > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: WEBAUTH value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: WEBAUTH value not between 0 and 1");
	}
	if ((uiWebPort < 1) || (uiWebPort > 65535)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: WEBPORT value not between 1 and 65535\n");
		else WEB_AddMessageInList("TestSettings: WEBPORT value not between 1 and 65535");
	}
	if ((WebMaxTimeIdle < 30) || (WebMaxTimeIdle > 9000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: WEBMAXTIMEIDLE value not between 30 and 9000\n");
		else WEB_AddMessageInList("TestSettings: WEBMAXTIMEIDLE value not between 30 and 9000");
	}
	if ((cZoom < 0) || (cZoom > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ZOOM value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: ZOOM value not between 0 and 1");
	}
	if ((cDateTimeReference < 0) || (cDateTimeReference > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: DATETIMETEFERENCE value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: DATETIMETEFERENCE value not between 0 and 1");
	}	
	if (uiFlvBufferSize > 15) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: FLVBUFFERSIZE value not between 0 and 15\n");
		else WEB_AddMessageInList("TestSettings: FLVBUFFERSIZE value not between 0 and 15");
	}	
	if ((cAccelerateTextRender < 0) || (cAccelerateTextRender > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ACCELERATETEXTRENDER value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: ACCELERATETEXTRENDER value not between 0 and 1");
	}
	
	if (strlen(cMediaBackUpPathFull) == 0)
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHFULL value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHFULL value is null\n");
	}
	if (strlen(cMediaBackUpPathSlow) == 0)
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHSLOW value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHSLOW value is null\n");
	}
	if (strlen(cMediaBackUpPathDiff) == 0)
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHDIFF value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHDIFF value is null\n");
	}
	if (strlen(cMediaBackUpPathAudio) == 0)
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHAUDIO value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHAUDIO value is null\n");
	}
	if (strlen(cMediaBackUpPathStatuses) == 0)
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHSTATUSES value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHSTATUSES value is null\n");
	}
	if (strlen(cMediaBackUpPathEvents) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHEVENTS value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHEVENTS value is null\n");
	}
	if (strlen(cMediaBackUpPathActions) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIABACKUPPATHACTIONS value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIABACKUPPATHACTIONS value is null\n");
	}
	
	if (strlen(cMediaCapturePathFull) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHFULL value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHFULL value is null");
	}
	if (strlen(cMediaCapturePathSlow) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHSLOW value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHSLOW value is null");
	}
	if (strlen(cMediaCapturePathDiff) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHDIFF value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHDIFF value is null");
	}
	if (strlen(cMediaCapturePathAudio) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHAUDIO value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHAUDIO value is null");
	}
	if (strlen(cMediaCapturePathStatuses) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHSTATUSES value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHSTATUSES value is null");
	}
	if (strlen(cMediaCapturePathEvents) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHEVENTS value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHEVENTS value is null");
	}
	if (strlen(cMediaCapturePathActions) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIACAPTUREPATHACTIONS value is null\n");
		else WEB_AddMessageInList("TestSettings: MEDIACAPTUREPATHACTIONS value is null");
	}
	
	if ((strlen(cMailAddress) < 1) || (strlen(cMailAddress) > 30)) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MAILADDRESS value length not between 1 and 30\n");
		else WEB_AddMessageInList("TestSettings: MAILADDRESS value length not between 1 and 30");
	}
	if ((strlen(cMailServer) < 1) || (strlen(cMailServer) > 30)) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MAILSERVER value length not between 1 and 30\n");
		else WEB_AddMessageInList("TestSettings: MAILSERVER value length not between 1 and 30");
	}
	if ((strlen(cMailLogin) < 1) || (strlen(cMailLogin) > 30)) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MAILLOGIN value length not between 1 and 30\n");
		else WEB_AddMessageInList("TestSettings: MAILLOGIN value length not between 1 and 30");
	}
	if ((strlen(cMailPassword) < 1) || (strlen(cMailPassword) > 30)) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MAILPASSWORD value length not between 1 and 30\n");
		else WEB_AddMessageInList("TestSettings: MAILPASSWORD value length not between 1 and 30");
	}
	if (iMode) DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (iMode) pthread_mutex_lock(&dbg_mutex);
	if ((iFileLog < 0) || (iFileLog > 10)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: FILELOG value not between 0 and 10\n");
		else 
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: FILELOG value not between 0 and 10");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if ((iScreenLog < 0) || (iScreenLog > 10)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SCREENLOG value not between 0 and 10\n");
		else 
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: SCREENLOG value not between 0 and 10");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if ((iMessageLog < 0) || (iMessageLog > 10)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MESSAGELOG value not between 0 and 10\n");
		else 
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: MESSAGELOG value not between 0 and 10");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if ((iLocalMessageLog < 0) || (iLocalMessageLog > 10)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: LOCALMESSAGELOG value not between 0 and 10\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: LOCALMESSAGELOG value not between 0 and 10");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if ((iEmailLog < 0) || (iEmailLog > 10)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: EMAILLOG value not between 0 and 10\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: EMAILLOG value not between 0 and 10");
			pthread_mutex_lock(&dbg_mutex);
		}
	}	
	if (uiLogEmailPauseSize < 60000) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: EMAILLOGPAUSE value < 60\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: EMAILLOGPAUSE value < 60");
			pthread_mutex_lock(&dbg_mutex);
		}
	}		
	if (uiSoundMessagePauseSize < 5000) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SOUNDMESSAGEPAUSE value < 5\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: SOUNDMESSAGEPAUSE value < 5");
			pthread_mutex_lock(&dbg_mutex);
		}
	}		
	if (strlen(cFileLogName) == 0) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: LOGPATH value is null\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: LOGPATH value is null");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if ((strlen(cLogIP) < 1) || (strlen(cLogIP) > 15)) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: LOGIP value length not between 1 and 15\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: LOGIP value length not between 1 and 15");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if ((strlen(cLogMlList.Address) < 5) || (strlen(cLogMlList.Address) > 64))
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: LOGEMAILADDR value length not between 5 and 64\n");
		else  
		{
			pthread_mutex_unlock(&dbg_mutex);
			WEB_AddMessageInList("TestSettings: LOGEMAILADDR value length not between 5 and 64");
			pthread_mutex_lock(&dbg_mutex);
		}
	}
	if (iMode) pthread_mutex_unlock(&dbg_mutex);
	
	if (iMode) DBG_MUTEX_LOCK(&evntaction_mutex);
	if (strlen(cEvntActionFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: EVNTACTIONFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: EVNTACTIONFILE value is null");	
	}	
	if (iMode) DBG_MUTEX_UNLOCK(&evntaction_mutex);
		
	if (iMode) DBG_MUTEX_LOCK(&mnlaction_mutex);
	if (strlen(cMnlActionFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MNLACTIONFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: MNLACTIONFILE value is null");
	}	
	if (iMode) DBG_MUTEX_UNLOCK(&mnlaction_mutex);
		
	if (iMode) DBG_MUTEX_LOCK(&modulelist_mutex);
	if (strlen(cSoundFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: SOUNDFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: SOUNDFILE value is null");	
	}
	if ((iRadioVolume < 0) || (iRadioVolume > 15)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: RADIOVOLUME value not between 0 and 15\n");
		else WEB_AddMessageInList("TestSettings: RADIOVOLUME value not between 0 and 15");
	}
	if (strlen(cModuleFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: MODULEFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: MODULEFILE value is null");	
	}
	if (iMode) DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	if (iMode) DBG_MUTEX_LOCK(&rectangle_mutex);
	if (strlen(cCamRectangleFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: CAMRECTANGLEFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: CAMRECTANGLEFILE value is null");
	}
	if (iMode) DBG_MUTEX_UNLOCK(&rectangle_mutex);
	
	if (iMode) DBG_MUTEX_LOCK(&securitylist_mutex);
	if (strlen(cKeyFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: KEYFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: KEYFILE value is null");	
	}
	if (iMode) DBG_MUTEX_UNLOCK(&securitylist_mutex);
	
	if (iMode) DBG_MUTEX_LOCK(&user_mutex);
	if (strlen(cUserFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: USERFILE value is null\n");
		else WEB_AddMessageInList("TestSettings: USERFILE value is null");
	}
	if (iMode) DBG_MUTEX_UNLOCK(&user_mutex);
	
	if (iMode) DBG_MUTEX_LOCK(&widget_mutex);
	if (strlen(cWidgetFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: WIDGETFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: WIDGETFILE value is null");	
	}	
	if ((iTimeCor < -120000) || (iTimeCor > 120000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: TIMEZONE value not between -1200 and 1200\n");	
		else WEB_AddMessageInList("TestSettings: TIMEZONE value not between -1200 and 1200");	
	}
	if ((uiPaddingSize < 0) || (uiPaddingSize > 1000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: PADDINGSIZE value not between 0 and 1000\n");	
		else WEB_AddMessageInList("TestSettings: PADDINGSIZE value not between 0 and 1000");	
	}
	if (iMode) DBG_MUTEX_UNLOCK(&widget_mutex);
	
	if (iMode) DBG_MUTEX_LOCK(&ircode_mutex);
	if (strlen(cIrCodeFile) == 0) 
	{
		if (iMode == 0) dbgprintf(3, "TestSettings: IRCODEFILE value is null\n");	
		else WEB_AddMessageInList("TestSettings: IRCODEFILE value is null");	
	}
	if (iMode) DBG_MUTEX_UNLOCK(&ircode_mutex);	
	
	if (iMode) DBG_MUTEX_LOCK(&systemlist_mutex);
	if ((uiShowerLiveCtrlTime < 0) || (uiShowerLiveCtrlTime > 1000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: SHOWERLIVECTRLTIME value not between 0 and 1000\n");
		else WEB_AddMessageInList("TestSettings: SHOWERLIVECTRLTIME value not between 0 and 1000");
	}	
	if ((uiCaptureMinFreeSpaceFull < 0) || (uiCaptureMinFreeSpaceFull > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceFull value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceFull value not between 0 and 20000");
	}
	if ((uiCaptureMinFreeSpaceSlow < 0) || (uiCaptureMinFreeSpaceSlow > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceSlow value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceSlow value not between 0 and 20000");
	}
	if ((uiCaptureMinFreeSpaceDiff < 0) || (uiCaptureMinFreeSpaceDiff > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceDiff value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceDiff value not between 0 and 20000");
	}
	if ((uiCaptureMinFreeSpaceAudio < 0) || (uiCaptureMinFreeSpaceAudio > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceAudio value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceAudio value not between 0 and 20000");
	}
	if ((uiCaptureMinFreeSpaceStatuses < 0) || (uiCaptureMinFreeSpaceStatuses > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceStatuses value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceStatuses value not between 0 and 20000");
	}
	if ((uiCaptureMinFreeSpaceEvents < 0) || (uiCaptureMinFreeSpaceEvents > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceEvents value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceEvents value not between 0 and 20000");
	}
	if ((uiCaptureMinFreeSpaceActions < 0) || (uiCaptureMinFreeSpaceActions > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CaptureMinFreeSpaceActions value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: CaptureMinFreeSpaceActions value not between 0 and 20000");
	}
	
	if ((uiBackUpMinFreeSpaceFull < 0) || (uiBackUpMinFreeSpaceFull > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceFull value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceFull value not between 0 and 20000");
	}
	if ((uiBackUpMinFreeSpaceSlow < 0) || (uiBackUpMinFreeSpaceSlow > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceSlow value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceSlow value not between 0 and 20000");
	}
	if ((uiBackUpMinFreeSpaceDiff < 0) || (uiBackUpMinFreeSpaceDiff > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceDiff value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceDiff value not between 0 and 20000");
	}
	if ((uiBackUpMinFreeSpaceAudio < 0) || (uiBackUpMinFreeSpaceAudio > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceAudio value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceAudio value not between 0 and 20000");
	}
	if ((uiBackUpMinFreeSpaceStatuses < 0) || (uiBackUpMinFreeSpaceStatuses > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceStatuses value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceStatuses value not between 0 and 20000");
	}
	if ((uiBackUpMinFreeSpaceEvents < 0) || (uiBackUpMinFreeSpaceEvents > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceEvents value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceEvents value not between 0 and 20000");
	}
	if ((uiBackUpMinFreeSpaceActions < 0) || (uiBackUpMinFreeSpaceActions > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BackUpMinFreeSpaceActions value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: BackUpMinFreeSpaceActions value not between 0 and 20000");
	}
	
	if ((uiArchiveMinFreeSpaceFull < 0) || (uiArchiveMinFreeSpaceFull > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceFull value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceFull value not between 0 and 20000");
	}
	if ((uiArchiveMinFreeSpaceSlow < 0) || (uiArchiveMinFreeSpaceSlow > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceSlow value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceSlow value not between 0 and 20000");
	}
	if ((uiArchiveMinFreeSpaceDiff < 0) || (uiArchiveMinFreeSpaceDiff > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceDiff value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceDiff value not between 0 and 20000");
	}
	if ((uiArchiveMinFreeSpaceAudio < 0) || (uiArchiveMinFreeSpaceAudio > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceAudio value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceAudio value not between 0 and 20000");
	}
	if ((uiArchiveMinFreeSpaceStatuses < 0) || (uiArchiveMinFreeSpaceStatuses > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceStatuses value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceStatuses value not between 0 and 20000");
	}
	if ((uiArchiveMinFreeSpaceEvents < 0) || (uiArchiveMinFreeSpaceEvents > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceEvents value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceEvents value not between 0 and 20000");
	}
	if ((uiArchiveMinFreeSpaceActions < 0) || (uiArchiveMinFreeSpaceActions > 20000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: ArchiveMinFreeSpaceActions value not between 0 and 20000\n");
		else WEB_AddMessageInList("TestSettings: ArchiveMinFreeSpaceActions value not between 0 and 20000");
	}
	
	if ((cCaptureFilesView < 0) || (cCaptureFilesView > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CAPTUREFILESVIEW value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: CAPTUREFILESVIEW value not between 0 and 1");
	}
	
	if ((cBackUpFilesView < 0) || (cBackUpFilesView > 1)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BACKUPFILESVIEW value not between 0 and 1\n");
		else WEB_AddMessageInList("TestSettings: BACKUPFILESVIEW value not between 0 and 1");
	}
	
	if ((cCaptureFilesLevel < 0) || (cCaptureFilesLevel > 100)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CAPTUREFILESLEVEL value not between 0 and 100\n");
		else WEB_AddMessageInList("TestSettings: CAPTUREFILESLEVEL value not between 0 and 100");
	}
	
	if ((cBackUpFilesLevel < 0) || (cBackUpFilesLevel > 100)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: BACKUPFILESLEVEL value not between 0 and 100\n");
		else WEB_AddMessageInList("TestSettings: BACKUPFILESLEVEL value not between 0 and 100");
	}
	
	if ((cCaptureFilesViewDef < 0) || (cCaptureFilesViewDef > 3)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: CAPTUREFILESVIEWDEF value not between 0 and 3\n");
		else WEB_AddMessageInList("TestSettings: CAPTUREFILESVIEWDEF value not between 0 and 3");
	}
	
	if ((uiMediaIoTimeout < 1) || (uiMediaIoTimeout > 1000)) 
	{
		if (iMode == 0) dbgprintf(2, "TestSettings: MEDIAIOTIMEOUT value not between 1 and 1000\n");
		else WEB_AddMessageInList("TestSettings: MEDIAIOTIMEOUT value not between 1 and 1000");
	}	
	
	if (iMode) DBG_MUTEX_UNLOCK(&systemlist_mutex);	
	
	if (iMode == 0)
	{
		TestModules(0);
		TestAlarms(0);
		TestSounds(0);
		TestMails(0);
		TestStreamTypes(0);
		TestStreams(0);
		TestWidgets(0);
		TestCamRectangles(0);
		TestIrCodes(0);
		TestKeys(0);
		TestEvntActions(0);
		TestMnlActions(0);
		TestDirectories(0);
		TestUsers(0);
		TestRadios(0);
	}
	
	DBG_LOG_OUT();
	return 1;
}

int LoadCamRectangles(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "CAMRECT=") == 1)
				{
					iRectangleCnt++;
					riRectangleList = (RECT_INFO*)DBG_REALLOC(riRectangleList, sizeof(RECT_INFO)*iRectangleCnt);
					memset(&riRectangleList[iRectangleCnt-1], 0, sizeof(RECT_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						riRectangleList[iRectangleCnt-1].Enabled = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						riRectangleList[iRectangleCnt-1].Group = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 10) == 1)
						riRectangleList[iRectangleCnt-1].X1 = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 10) == 1)
						riRectangleList[iRectangleCnt-1].Y1 = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 10) == 1)
						riRectangleList[iRectangleCnt-1].X2 = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 10) == 1)
						riRectangleList[iRectangleCnt-1].Y2 = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 64))
						strcpy(riRectangleList[iRectangleCnt-1].Name, Buff4);
					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&riRectangleList[iRectangleCnt-1].ID, Buff4, 4);					
				}							
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveCamRectangles()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&rectangle_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cCamRectangleFile, 1);
	if ((strlen(cCamRectangleFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cCamRectangleFile);		
		DBG_MUTEX_UNLOCK(&rectangle_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iRectangleCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "CamRect=%i;%i;%i;%i;%i;%i;%s;%s;\n", 
				riRectangleList[n].Enabled, 
				riRectangleList[n].Group,
				riRectangleList[n].X1,
				riRectangleList[n].Y1,
				riRectangleList[n].X2,
				riRectangleList[n].Y2,
				riRectangleList[n].Name,
				(char*)&riRectangleList[n].ID);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&rectangle_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestCamRectangles(int iMode)
{
	int i;
	for (i = 0; i < iRectangleCnt; i++)
	{
		if ((riRectangleList[i].Enabled < 0) || (riRectangleList[i].Enabled > 1))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) Enabled not between 0 and 1\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) Enabled not between 0 and 1", i);
		}
		if ((riRectangleList[i].Group < 0) || (riRectangleList[i].Group > 20))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) Group not between 0 and 20\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) Group not between 0 and 20", i);
		}
		if ((riRectangleList[i].X1 < 0) || (riRectangleList[i].X1 > 4000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) X1 not between 0 and 4000\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) X1 not between 0 and 4000", i);
		}
		if ((riRectangleList[i].Y1 < 0) || (riRectangleList[i].Y1 > 4000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) Y1 not between 0 and 4000\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) Y1 not between 0 and 4000", i);
		}
		if ((riRectangleList[i].X2 < 0) || (riRectangleList[i].X2 > 4000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) X2 not between 0 and 4000\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) X2 not between 0 and 4000", i);
		}
		if ((riRectangleList[i].Y2 < 0) || (riRectangleList[i].Y2 > 4000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) Y2 not between 0 and 4000\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) Y2 not between 0 and 4000", i);
		}	
		if (riRectangleList[i].X1 > riRectangleList[i].X2)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) X1 > X2\n", i);
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) X1 > X2", i);
		}
		if (riRectangleList[i].Y1 > riRectangleList[i].Y2)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) Y1 > Y2\n", i);	
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) Y1 > Y2", i);	
		}
		if (strlen(riRectangleList[i].Name) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: CAMRECT(%i) Name is null\n", i);	
			else WEB_AddMessageInList("TestSettings: CAMRECT(%i) Name is null", i);	
		}
	}
	return 1;
}

int LoadUsers(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "USER=") == 1)
				{
					iUserCnt++;
					uiUserList = (USER_INFO*)DBG_REALLOC(uiUserList, sizeof(USER_INFO)*iUserCnt);
					memset(&uiUserList[iUserCnt-1], 0, sizeof(USER_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						uiUserList[iUserCnt-1].Enabled = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						uiUserList[iUserCnt-1].Level = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64))
						strcpy(uiUserList[iUserCnt-1].Login, Buff4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 64))
						strcpy(uiUserList[iUserCnt-1].Password, Buff4);
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 256))
						uiUserList[iUserCnt-1].Access = (unsigned int)Str2Int(Buff4);
				}				
			}
		}
	}
	fclose(f);
	
	DBG_LOG_OUT();
	return 1;
}

int SaveUsers()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&user_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cUserFile, 1);
	if ((strlen(cUserFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cUserFile);		
		DBG_MUTEX_UNLOCK(&user_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iUserCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "User=%i;%i;%s;%s;%i;\n", 
				uiUserList[n].Enabled, 
				uiUserList[n].Level, 
				uiUserList[n].Login, 
				uiUserList[n].Password, 
				uiUserList[n].Access);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&user_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestUsers(int iMode)
{
	int i;
	for (i = 0; i < iUserCnt; i++)
	{
		if ((uiUserList[i].Enabled < 0) || (uiUserList[i].Enabled > 1))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: USER(%i) Enabled not between 0 and 1 (%i)\n", i,uiUserList[i].Enabled);
			else WEB_AddMessageInList("TestSettings: USER(%i) Enabled not between 0 and 1 (%i)", i,uiUserList[i].Enabled);
		}
		if ((uiUserList[i].Level < 0) || (uiUserList[i].Level > 1000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: USER(%i) Level not between 0 and 1000 (%i)\n", i,uiUserList[i].Level);
			else WEB_AddMessageInList("TestSettings: USER(%i) Level not between 0 and 1000 (%i)", i,uiUserList[i].Level);
		}
		if (strlen(uiUserList[i].Login) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: USER(%i) Login is null\n", i);
			else WEB_AddMessageInList("TestSettings: USER(%i) Login is null", i);
		}
		if (strlen(uiUserList[i].Password) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: USER(%i) Password is null\n", i);
			else WEB_AddMessageInList("TestSettings: USER(%i) Password is null", i);
		}
	}
	return 1;
}

int LoadRadios(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "RADIOSTATION=") == 1)
				{
					iRadioStationsCnt++;
					riRadioStation = (RADIO_INFO*)DBG_REALLOC(riRadioStation, sizeof(RADIO_INFO)*iRadioStationsCnt);
					memset(&riRadioStation[iRadioStationsCnt-1], 0, sizeof(RADIO_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 64))
						strcpy(riRadioStation[iRadioStationsCnt-1].Name, Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						riRadioStation[iRadioStationsCnt-1].Frequency = (double)Str2Float(Buff4);					
				}			
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveRadios()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cRadioFile, 1);
	if ((strlen(cRadioFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cRadioFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iRadioStationsCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "RadioStation=%s;%.1f;\n", riRadioStation[n].Name,	(float)riRadioStation[n].Frequency);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestRadios(int iMode)
{	
	int i;	
	for (i = 0; i < iRadioStationsCnt; i++)
	{
		if (strlen(riRadioStation[i].Name) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: RADIOSTATION(%i) null name\n", i);
			else WEB_AddMessageInList("TestSettings: RADIOSTATION(%i) null name", i);
		}
		if ((riRadioStation[i].Frequency < 76) || (riRadioStation[i].Frequency > 108))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: RADIOSTATION(%i) Frequency not between 76.0 and 108.0\n", i);	
			else WEB_AddMessageInList("TestSettings: RADIOSTATION(%i) Frequency not between 76.0 and 108.0", i);	
		}
	}	
	return 1;
}

int LoadAlarms(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "ALARMCLOCK=") == 1)
				{
					iAlarmClocksCnt++;
					actAlarmClockInfo = (ALARM_CLOCK_TYPE*)DBG_REALLOC(actAlarmClockInfo, sizeof(ALARM_CLOCK_TYPE)*iAlarmClocksCnt);
					memset(&actAlarmClockInfo[iAlarmClocksCnt-1], 0, sizeof(ALARM_CLOCK_TYPE));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].Enabled = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].Time = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].Days = (unsigned char)GetModuleSettings(Buff4, strlen(Buff4), 0);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 10) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].Skip = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 10) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].Limit = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, MAX_FILE_LEN) == 1)
						strcpy(actAlarmClockInfo[iAlarmClocksCnt-1].Path, Buff4);		
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 10) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].BeginVolume = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 10) == 1)
						actAlarmClockInfo[iAlarmClocksCnt-1].TimeSetVolume = (unsigned int)Str2Int(Buff4);											
				}				
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveAlarms()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cAlarmFile, 1);
	if ((strlen(cAlarmFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save alarms:%s\n", cAlarmFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iAlarmClocksCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "AlarmClock=%i;%i;%s%s%s%s%s%s%s;%i;%i;%s;%i;%i;\n", 
				actAlarmClockInfo[n].Enabled, 
				actAlarmClockInfo[n].Time,
				actAlarmClockInfo[n].Days & 1 ? "[Mo]" : "",
				actAlarmClockInfo[n].Days & 2 ? "[Tu]" : "",
				actAlarmClockInfo[n].Days & 4 ? "[We]" : "",
				actAlarmClockInfo[n].Days & 8 ? "[Th]" : "",
				actAlarmClockInfo[n].Days & 16 ? "[Fr]" : "",
				actAlarmClockInfo[n].Days & 32 ? "[Sa]" : "",
				actAlarmClockInfo[n].Days & 64 ? "[Su]" : "",
				actAlarmClockInfo[n].Skip,
				actAlarmClockInfo[n].Limit,
				actAlarmClockInfo[n].Path,
				actAlarmClockInfo[n].BeginVolume,
				actAlarmClockInfo[n].TimeSetVolume
				);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestAlarms(int iMode)
{
	int i;
	for (i = 0; i < iAlarmClocksCnt; i++)
	{
		if ((actAlarmClockInfo[i].Enabled < 0) || (actAlarmClockInfo[i].Enabled > 1))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ALARMCLOCK(%i) Enabled not between 0 and 1 (%i)\n", i,actAlarmClockInfo[i].Enabled);
			else WEB_AddMessageInList("TestSettings: ALARMCLOCK(%i) Enabled not between 0 and 1 (%i)", i,actAlarmClockInfo[i].Enabled);
		}
		if ((actAlarmClockInfo[i].Time < 0) || (actAlarmClockInfo[i].Time > 235959)) 
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ALARMCLOCK(%i) Time not between 0 and 235959 (%i)\n", i,actAlarmClockInfo[i].Time);
			else WEB_AddMessageInList("TestSettings: ALARMCLOCK(%i) Time not between 0 and 235959 (%i)", i,actAlarmClockInfo[i].Time);
		}
		if ((actAlarmClockInfo[i].Days < 0) || (actAlarmClockInfo[i].Days > 127)) 
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ALARMCLOCK(%i) Days not between 0 and 127 (%i)", i,actAlarmClockInfo[i].Days);
			else WEB_AddMessageInList("TestSettings: ALARMCLOCK(%i) Days not between 0 and 127 (%i)", i,actAlarmClockInfo[i].Days);
		}
		if ((actAlarmClockInfo[i].Skip < 0) || (actAlarmClockInfo[i].Skip > 1000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ALARMCLOCK(%i) Skip not between 0 and 1000 (%i)\n", i,actAlarmClockInfo[i].Skip);
			else WEB_AddMessageInList("TestSettings: ALARMCLOCK(%i) Skip not between 0 and 1000 (%i)", i,actAlarmClockInfo[i].Skip);
		}
		if ((actAlarmClockInfo[i].Limit < 0) || (actAlarmClockInfo[i].Limit > 1000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ALARMCLOCK(%i) Limit not between 0 and 1000 (%i)\n", i,actAlarmClockInfo[i].Skip);
			else WEB_AddMessageInList("TestSettings: ALARMCLOCK(%i) Limit not between 0 and 1000 (%i)", i,actAlarmClockInfo[i].Skip);
		}
	}
	return 1;
}

int LoadIrCodes(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "IRCOMMAND=") == 1)
				{
					iIRCommandCnt++;
					mIRCommandList = (IR_COMMAND_TYPE*)DBG_REALLOC(mIRCommandList, sizeof(IR_COMMAND_TYPE)*iIRCommandCnt);
					memset(&mIRCommandList[iIRCommandCnt-1], 0, sizeof(IR_COMMAND_TYPE));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&mIRCommandList[iIRCommandCnt-1].ID, Buff4, 4);
					for (n = 0; n < MAX_IRCOMMAND_LEN; n++)
						if (GetParamSetting(n + 1, 59, Buff3, len3, Buff4, 10) == 1)
						{
							mIRCommandList[iIRCommandCnt-1].Code[n] = 0;
							if ((strlen(Buff4) == 1) && (Buff4[0] == 42)) mIRCommandList[iIRCommandCnt-1].Code[n] = 0x0200;
							if ((strlen(Buff4) > 1) && (Buff4[0] == 38)) mIRCommandList[iIRCommandCnt-1].Code[n] = 0x0100 | ((uint16_t)Str2Int(&Buff4[1]) & 0xFF);
							
							if (mIRCommandList[iIRCommandCnt-1].Code[n] == 0) mIRCommandList[iIRCommandCnt-1].Code[n] = (uint16_t)Str2Int(Buff4) & 0xFF;
							mIRCommandList[iIRCommandCnt-1].Len = n + 1;
						}
						else break;						
				}				
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveIrCodes()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&ircode_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cIrCodeFile, 1);
	if ((strlen(cIrCodeFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cIrCodeFile);		
		DBG_MUTEX_UNLOCK(&ircode_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	char Buff2[32];	
	unsigned int n, i;
		
	for (n = 0; n < iIRCommandCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "IrCommand=%.4s;", (char*)&mIRCommandList[n].ID);
		for (i = 0; i < mIRCommandList[n].Len; i++)
		{
			memset(Buff2, 0, 32);
			if (mIRCommandList[n].Code[i] & 0x0100) sprintf(Buff2, "&%i;", mIRCommandList[n].Code[i] & 0xFF);
			if (mIRCommandList[n].Code[i] & 0x0200) strcpy(Buff2, "*;");
			if ((mIRCommandList[n].Code[i] & 0x0300) == 0) sprintf(Buff2, "%i;", mIRCommandList[n].Code[i]);
							
			strcat(Buff1, Buff2);
		}
		strcat(Buff1, "\n");
		fputs(Buff1, f);		
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&ircode_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestIrCodes(int iMode)
{
	int i, n;
	for (i = 0; i < iIRCommandCnt; i++)
	{
		if (mIRCommandList[i].ID == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: IRCOMMAND(%i) ID is null\n", i);
			else WEB_AddMessageInList("TestSettings: IRCOMMAND(%i) ID is null", i);
		}
		if ((mIRCommandList[i].Len < 1) || (mIRCommandList[i].Len > 60))
		{
			if (iMode == 0) dbgprintf(3, "TestSettings: IRCOMMAND(%i) Length not between 1 and 60\n", i);
			else WEB_AddMessageInList("TestSettings: IRCOMMAND(%i) Length not between 1 and 60", i);
		}
		if (mIRCommandList[i].Len > MAX_IRCOMMAND_LEN)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: IRCOMMAND(%i) Length very big(>%i)\n", i, MAX_IRCOMMAND_LEN);
			else WEB_AddMessageInList("TestSettings: IRCOMMAND(%i) Length very big(>%i)", i, MAX_IRCOMMAND_LEN);
		}
		else
		{
			for (n = 0; n < mIRCommandList[i].Len; n++)
			{
				if (mIRCommandList[i].Code[n] > 0x0200)
				{
					if (iMode == 0) dbgprintf(2, "TestSettings: IRCOMMAND(%i) Code(%i) not between 0 and 0x0200,  :%i\n", i, n, mIRCommandList[i].Code[n]);
						else WEB_AddMessageInList("TestSettings: IRCOMMAND(%i) Code(%i) not between 0 and 0x0200,  :%i", i, n, mIRCommandList[i].Code[n]);
					break;
				}
			}
		}
	}
	return 1;
}

int LoadKeys(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	char Buff5[10];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "SECURITYKEY=") == 1)
				{
					iSecurityKeyCnt++;
					skiSecurityKeys = (SECURITY_KEY_INFO*)DBG_REALLOC(skiSecurityKeys, sizeof(SECURITY_KEY_INFO)*iSecurityKeyCnt);
					memset(&skiSecurityKeys[iSecurityKeyCnt-1], 0, sizeof(SECURITY_KEY_INFO));
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&skiSecurityKeys[iSecurityKeyCnt-1].ID, Buff4, 4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 64) == 1)
						skiSecurityKeys[iSecurityKeyCnt-1].Enabled = (unsigned char)Str2Int(Buff4);
					//if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64) == 1)
					//	skiSecurityKeys[iSecurityKeyCnt-1].EventEnabled = (unsigned char)Str2Int(Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64) == 1)
						skiSecurityKeys[iSecurityKeyCnt-1].Type = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 64) == 1)
						skiSecurityKeys[iSecurityKeyCnt-1].Sector = (unsigned char)Str2Int(Buff4);
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 64) == 1)
						skiSecurityKeys[iSecurityKeyCnt-1].VerifyKeyA = (unsigned char)Str2Int(Buff4);
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 64) == 1)
						skiSecurityKeys[iSecurityKeyCnt-1].VerifyKeyB = (unsigned char)Str2Int(Buff4);
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 64) == 1)
					{
						m = strlen(Buff4);
						skiSecurityKeys[iSecurityKeyCnt-1].SerialLength = 0;
						for (n = 0; n < MAX_SECURITY_SERIAL_LEN; n++)
							if (GetParamSetting(n, 44, Buff4, m, Buff5, 10) == 1)
							{
								skiSecurityKeys[iSecurityKeyCnt-1].Serial[skiSecurityKeys[iSecurityKeyCnt-1].SerialLength] = (unsigned char)Str2Int(Buff5);
								skiSecurityKeys[iSecurityKeyCnt-1].SerialLength++;
							}
							else break;
					}
					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 64) == 1)
					{
						m = strlen(Buff4);
						for (n = 0; n < 6; n++)
							if (GetParamSetting(n, 44, Buff4, m, Buff5, 10) == 1)
								skiSecurityKeys[iSecurityKeyCnt-1].KeyA[n] = (unsigned char)Str2Int(Buff5);
								else break;
					}
					if (GetParamSetting(8, 59, Buff3, len3, Buff4, 64) == 1)
					{
						m = strlen(Buff4);
						for (n = 0; n < 6; n++)
							if (GetParamSetting(n, 44, Buff4, m, Buff5, 10) == 1)
								skiSecurityKeys[iSecurityKeyCnt-1].KeyB[n] = (unsigned char)Str2Int(Buff5);
								else break;
					}
					if (GetParamSetting(9, 59, Buff3, len3, Buff4, 64))
						strcpy(skiSecurityKeys[iSecurityKeyCnt-1].Name, Buff4);					
				}				
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveKeys()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&securitylist_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cKeyFile, 1);
	if ((strlen(cKeyFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cKeyFile);		
		DBG_MUTEX_UNLOCK(&securitylist_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	char Buff2[64];	
	unsigned int n, i;
		
	for (n = 0; n < iSecurityKeyCnt; n++)
	{
		if (skiSecurityKeys[n].Local == 1)
		{
			memset(Buff1, 0, 1024);
			sprintf(Buff1, "SecurityKey=%.4s;%i;%i;%i;%i;%i;", (char*)&skiSecurityKeys[n].ID,
												skiSecurityKeys[n].Enabled,
												//skiSecurityKeys[n].EventEnabled,
												skiSecurityKeys[n].Type,
												skiSecurityKeys[n].Sector,
												skiSecurityKeys[n].VerifyKeyA,
												skiSecurityKeys[n].VerifyKeyB);
			if (skiSecurityKeys[n].SerialLength)
			{
				for (i = 0; i < skiSecurityKeys[n].SerialLength; i++)
				{
					memset(Buff2, 0, 32);
					sprintf(Buff2, "%i", skiSecurityKeys[n].Serial[i]);
					strcat(Buff1, Buff2);
					if (i < (skiSecurityKeys[n].SerialLength - 1)) strcat(Buff1, ",");
				}
			} 
			strcat(Buff1, ";");
			for (i = 0; i < 6; i++)
			{
				memset(Buff2, 0, 32);
				sprintf(Buff2, "%i", skiSecurityKeys[n].KeyA[i]);
				strcat(Buff1, Buff2);
				if (i < 5) strcat(Buff1, ","); else strcat(Buff1, ";");
			}
			for (i = 0; i < 6; i++)
			{
				memset(Buff2, 0, 32);
				sprintf(Buff2, "%i", skiSecurityKeys[n].KeyB[i]);
				strcat(Buff1, Buff2);
				if (i < 5) strcat(Buff1, ","); else strcat(Buff1, ";");
			}			
			strcat(Buff1, skiSecurityKeys[n].Name);
			strcat(Buff1, ";\n");
			
			fputs(Buff1, f);
		}
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&securitylist_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestKeys(int iMode)
{
	int i;
	for (i = 0; i < iSecurityKeyCnt; i++)
	{
		if (skiSecurityKeys[i].ID == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SECURITYKEY(%i) ID is null\n", i);
			else WEB_AddMessageInList("TestSettings: SECURITYKEY(%i) ID is null", i);
		}			
		/*if ((skiSecurityKeys[i].Serial[0] == 0) &&
			(skiSecurityKeys[i].Serial[1] == 0) &&
			(skiSecurityKeys[i].Serial[2] == 0) &&
			(skiSecurityKeys[i].Serial[3] == 0))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SECURITYKEY(%i) Serial is null\n", i);
			else WEB_AddMessageInList("TestSettings: SECURITYKEY(%i) Serial is null", i);
		}			
		if ((skiSecurityKeys[i].KeyA1[0] == 0) &&
			(skiSecurityKeys[i].KeyA1[1] == 0) &&
			(skiSecurityKeys[i].KeyA1[2] == 0) &&
			(skiSecurityKeys[i].KeyA1[3] == 0) &&
			(skiSecurityKeys[i].KeyA1[4] == 0) &&
			(skiSecurityKeys[i].KeyA1[5] == 0))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SECURITYKEY(%i) Code is null\n", i);
			else WEB_AddMessageInList("TestSettings: SECURITYKEY(%i) Code is null", i);
		}			
		if ((skiSecurityKeys[i].Pass1[0] == 0) &&
			(skiSecurityKeys[i].Pass1[1] == 0) &&
			(skiSecurityKeys[i].Pass1[2] == 0) &&
			(skiSecurityKeys[i].Pass1[3] == 0) &&
			(skiSecurityKeys[i].Pass1[4] == 0) &&
			(skiSecurityKeys[i].Pass1[5] == 0))
		{
			if (iMode == 0) dbgprintf(3, "TestSettings: SECURITYKEY(%i) Value is null\n", i);
			else WEB_AddMessageInList("TestSettings: SECURITYKEY(%i) Value is null", i);
		}*/
	}
	return 1;
}

int LoadWidgets(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "WIDGET=") == 1)
				{
					iWidgetsCnt++;
					wiWidgetList = (WIDGET_INFO*)DBG_REALLOC(wiWidgetList, sizeof(WIDGET_INFO)*iWidgetsCnt);
					memset(&wiWidgetList[iWidgetsCnt-1], 0, sizeof(WIDGET_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&wiWidgetList[iWidgetsCnt-1].WidgetID, Buff4, 4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].ShowMode = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 32) == 1)
						wiWidgetList[iWidgetsCnt-1].Type = (unsigned char)Str2Int(Buff4);	
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 64))
						strcpy(wiWidgetList[iWidgetsCnt-1].Name, Buff4);	
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Scale = (double)Str2Float(Buff4);	
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Direct = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Speed = (double)Str2Float(Buff4);	
					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Settings[0] = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(8, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Refresh = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(9, 59, Buff3, len3, Buff4, 64) == 1)
						wiWidgetList[iWidgetsCnt-1].WeekDays = (unsigned char)GetModuleSettings(Buff4, strlen(Buff4), 0);	
					if (GetParamSetting(10, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].NotBeforeTime = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(11, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].NotAfterTime = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(12, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].NotBeforeTime2 = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(13, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].NotAfterTime2 = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(14, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].NotBeforeTime3 = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(15, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].NotAfterTime3 = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(16, 59, Buff3, len3, Buff4, 255) == 1)
						strcpy(wiWidgetList[iWidgetsCnt-1].Path, Buff4);	
					if (GetParamSetting(17, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].SelfPath = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(18, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&wiWidgetList[iWidgetsCnt-1].ModuleID, Buff4, 4);
					if (GetParamSetting(19, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].SourceCell = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(20, 59, Buff3, len3, Buff4, 7) == 1)
					{
						strcpy(wiWidgetList[iWidgetsCnt-1].Color, Buff4);
						unsigned int uiColor = Hex2Int(wiWidgetList[iWidgetsCnt-1].Color);
						wiWidgetList[iWidgetsCnt-1].RGBColor.Red = uiColor & 255;
						wiWidgetList[iWidgetsCnt-1].RGBColor.Green = (uiColor >> 8) & 255;
						wiWidgetList[iWidgetsCnt-1].RGBColor.Blue = (uiColor >> 16) & 255;						
					}
					if (GetParamSetting(21, 59, Buff3, len3, Buff4, 10) == 1)
						strcpy(wiWidgetList[iWidgetsCnt-1].ValueTypeName, Buff4);	
					if (GetParamSetting(22, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Coefficient = (double)Str2Float(Buff4);
					if (GetParamSetting(23, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].ShowValueFrom = (double)Str2Float(Buff4);
					if (GetParamSetting(24, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].ShowValueTo = (double)Str2Float(Buff4);	
					//if (GetParamSetting(25, 59, Buff3, len3, Buff4, 10) == 1)
						//wiWidgetList[iWidgetsCnt-1].SubType = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(26, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].OffsetValue = Str2Int(Buff4);
					if (GetParamSetting(27, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].DirectX = (double)Str2Float(Buff4);	
					if (GetParamSetting(28, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].DirectY = (double)Str2Float(Buff4);	
					if (GetParamSetting(29, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].ShowTime = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(30, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Enabled = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(31, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].Angle = (double)Str2Float(Buff4);
					if (GetParamSetting(32, 59, Buff3, len3, Buff4, 10) == 1)
						wiWidgetList[iWidgetsCnt-1].ShowRepeat= (unsigned int)Str2Int(Buff4);
				}				
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveWidgets()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&widget_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cWidgetFile, 1);
	if ((strlen(cWidgetFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cWidgetFile);		
		DBG_MUTEX_UNLOCK(&widget_mutex);
		DBG_LOG_OUT();
		return 0;
	}	
	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iWidgetsCnt; n++)
	{
		if (wiWidgetList[n].Status != -2)
		{
			memset(Buff1, 0, 1024);
			sprintf(Buff1, "Widget=%.4s;%i;%i;%s;%g;%i;%g;%i;%i;%s%s%s%s%s%s%s;%i;%i;%i;%i;%i;%i;%s;%i;%.4s;%i;%s;%s;%g;%g;%g;%i;%i;%g;%g;%i;%i;%g;%i;\n", 
							(char*)&wiWidgetList[n].WidgetID, wiWidgetList[n].ShowMode,
							wiWidgetList[n].Type, wiWidgetList[n].Name, (float)wiWidgetList[n].Scale,
							wiWidgetList[n].Direct, (float)wiWidgetList[n].Speed, wiWidgetList[n].Settings[0], wiWidgetList[n].Refresh,
							wiWidgetList[n].WeekDays & 1 ? "[MO]" : "",
							wiWidgetList[n].WeekDays & 2 ? "[TU]" : "",
							wiWidgetList[n].WeekDays & 4 ? "[WE]" : "",
							wiWidgetList[n].WeekDays & 8 ? "[TH]" : "",
							wiWidgetList[n].WeekDays & 16 ? "[FR]" : "",
							wiWidgetList[n].WeekDays & 32 ? "[SA]" : "",
							wiWidgetList[n].WeekDays & 64 ? "[SU]" : "",
							wiWidgetList[n].NotBeforeTime, wiWidgetList[n].NotAfterTime,
							wiWidgetList[n].NotBeforeTime2, wiWidgetList[n].NotAfterTime2,
							wiWidgetList[n].NotBeforeTime3, wiWidgetList[n].NotAfterTime3,
							wiWidgetList[n].Path, wiWidgetList[n].SelfPath, 
							(char*)&wiWidgetList[n].ModuleID,
							wiWidgetList[n].SourceCell,
							wiWidgetList[n].Color,
							wiWidgetList[n].ValueTypeName,
							wiWidgetList[n].Coefficient,
							wiWidgetList[n].ShowValueFrom,
							wiWidgetList[n].ShowValueTo,
							0, //wiWidgetList[n].SubType,
							wiWidgetList[n].OffsetValue,
							wiWidgetList[n].DirectX,
							wiWidgetList[n].DirectY,
							wiWidgetList[n].ShowTime,
							wiWidgetList[n].Enabled,
							wiWidgetList[n].Angle,
							wiWidgetList[n].ShowRepeat);
			fputs(Buff1, f);
		}
	}
	fclose(f);	
	DBG_MUTEX_UNLOCK(&widget_mutex);
	
	DBG_LOG_OUT();
	return 1;
}

int TestWidgets(int iMode)
{
	int i;
	for (i = 0; i < iWidgetsCnt; i++)
	{
		if ((wiWidgetList[i].Enabled < 0) || (wiWidgetList[i].Enabled > 1))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Enabled not between 0 and 1\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Enabled not between 0 and 1", i);
		}			
		if ((wiWidgetList[i].Angle < 0.0f) || (wiWidgetList[i].Angle > 360.0f))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Angle not between 0 and 360\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Angle not between 0 and 360", i);
		}			
		if (wiWidgetList[i].WidgetID == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) WidgetID is null\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) WidgetID is null", i);
		}			
		if ((wiWidgetList[i].ShowMode < 0) || (wiWidgetList[i].ShowMode > 15))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) ShowMode not between 0 and 15\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) ShowMode not between 0 and 15", i);
		}			
		if ((wiWidgetList[i].Type < 0) || (wiWidgetList[i].Type >= WIDGET_TYPE_MAX))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Type not between 0 and %i\n", i, WIDGET_TYPE_MAX-1);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Type not between 0 and %i", i, WIDGET_TYPE_MAX-1);
		}			
		/*if (strlen(wiWidgetList[i].Name) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Name is null\n", i);	
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Name is null", i);	
		}	*/		
		if ((wiWidgetList[i].Scale < 0.1f) || (wiWidgetList[i].Scale > 10.0f))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Scale not between 0.1 and 10.0\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Scale not between 0.1 and 10.0", i);
		}			
		if ((wiWidgetList[i].Direct <= WIDGET_DIRECT_UNKNOWN) || (wiWidgetList[i].Direct >= WIDGET_DIRECT_MAX))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Direct unknown\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Direct unknown", i);
		}			
		if ((wiWidgetList[i].Speed < 0.1f) || (wiWidgetList[i].Scale > 10.0f))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Speed not between 0.1 and 10.0\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Speed not between 0.1 and 10.0", i);
		}			
		if ((wiWidgetList[i].Settings[0] < 0) || (wiWidgetList[i].Settings[0] > 7))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Settings not between 0 and 7\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Settings not between 0 and 7", i);
		}			
		if ((wiWidgetList[i].Refresh < 0) || (wiWidgetList[i].Refresh > 10000))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Refresh not between 0 and 10000\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Refresh not between 0 and 10000", i);
		}			
		if ((wiWidgetList[i].WeekDays < 0) || (wiWidgetList[i].WeekDays > 127))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) WeekDays not between 0 and 127\n", i);	
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) WeekDays not between 0 and 127", i);	
		}				
		if ((wiWidgetList[i].NotBeforeTime < 0) || (wiWidgetList[i].NotBeforeTime > 235959))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) NotBeforeTime not between 0 and 235959\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) NotBeforeTime not between 0 and 235959", i);
		}			
		if ((wiWidgetList[i].NotAfterTime < 0) || (wiWidgetList[i].NotAfterTime > 235959))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) NotAfterTime not between 0 and 235959\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) NotAfterTime not between 0 and 235959", i);
		}			
		if ((wiWidgetList[i].NotBeforeTime2 < 0) || (wiWidgetList[i].NotBeforeTime2 > 235959))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) NotBeforeTime2 not between 0 and 235959\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) NotBeforeTime2 not between 0 and 235959", i);
		}			
		if ((wiWidgetList[i].NotAfterTime2 < 0) || (wiWidgetList[i].NotAfterTime2 > 235959))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) NotAfterTime2 not between 0 and 235959\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) NotAfterTime2 not between 0 and 235959", i);
		}			
		if ((wiWidgetList[i].NotBeforeTime3 < 0) || (wiWidgetList[i].NotBeforeTime3 > 235959))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) NotBeforeTime3 not between 0 and 235959\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) NotBeforeTime3 not between 0 and 235959", i);
		}			
		if ((wiWidgetList[i].NotAfterTime3 < 0) || (wiWidgetList[i].NotAfterTime3 > 235959))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) NotAfterTime3 not between 0 and 235959\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) NotAfterTime3 not between 0 and 235959", i);
		}			
		/*if (strlen(wiWidgetList[i].Path) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) Path is null\n", i);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) Path is null", i);
		}*/
		if ((wiWidgetList[i].SourceCell < 1) || (wiWidgetList[i].SourceCell > MAX_MODULE_STATUSES))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: WIDGET(%i) SourceCell not between 1 and %i\n", i, MAX_MODULE_STATUSES);
			else WEB_AddMessageInList("TestSettings: WIDGET(%i) SourceCell not between 1 and %i\n", i, MAX_MODULE_STATUSES);
		}		
	}
	return 0;
}

int LoadModules(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "MODULE=") == 1)
				{
					iModuleCnt++;
					miModuleList = (MODULE_INFO*)DBG_REALLOC(miModuleList, sizeof(MODULE_INFO)*iModuleCnt);
					memset(&miModuleList[iModuleCnt-1], 0, sizeof(MODULE_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 32) == 1)
						miModuleList[iModuleCnt-1].Enabled = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 32) == 1)
						miModuleList[iModuleCnt-1].Type = (unsigned char)GetModuleType(Buff4, strlen(Buff4));
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 5) == 1)
					{
						memcpy(&miModuleList[iModuleCnt-1].ID, Buff4, 4);
						miModuleList[iModuleCnt-1].NewID = miModuleList[iModuleCnt-1].ID;
					}
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 64) == 1)
						strcpy(miModuleList[iModuleCnt-1].Name, Buff4);
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 10) == 1)
						miModuleList[iModuleCnt-1].ViewLevel = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 10) == 1)
						miModuleList[iModuleCnt-1].AccessLevel = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 64) == 1)
						miModuleList[iModuleCnt-1].Global = (unsigned int)Str2Int(Buff4);			
					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 10) == 1)
						miModuleList[iModuleCnt-1].ScanSet = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(8, 59, Buff3, len3, Buff4, 10) == 1)
						miModuleList[iModuleCnt-1].SaveChanges = (unsigned int)Str2Int(Buff4);					
					if (GetParamSetting(9, 59, Buff3, len3, Buff4, 10) == 1)
						miModuleList[iModuleCnt-1].GenEvents = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(10, 59, Buff3, len3, Buff4, 64) == 1)
					{
						miModuleList[iModuleCnt-1].Settings[0] = GetModuleSettings(Buff4, strlen(Buff4), 0);
						if (miModuleList[iModuleCnt-1].Type == MODULE_TYPE_USB_GPIO)
								miModuleList[iModuleCnt-1].Settings[0] = (unsigned int)Str2Int(Buff4);
						if ((miModuleList[iModuleCnt-1].Type == MODULE_TYPE_RS485) ||
							(miModuleList[iModuleCnt-1].Type == MODULE_TYPE_PN532) ||
							(miModuleList[iModuleCnt-1].Type == MODULE_TYPE_RC522) ||
							(miModuleList[iModuleCnt-1].Type == MODULE_TYPE_SPEAKER) ||
							(miModuleList[iModuleCnt-1].Type == MODULE_TYPE_TFP625A))
						{
							miModuleList[iModuleCnt-1].Settings[0] = 0;
							if (strlen(Buff4) >= 4) memcpy(&miModuleList[iModuleCnt-1].Settings[0], Buff4, 4);
								else memcpy(&miModuleList[iModuleCnt-1].Settings[0], Buff4, strlen(Buff4));
						}							
					}
					for (n = 1; n < MAX_MODULE_SETTINGS; n++)
					{
						if (((miModuleList[iModuleCnt-1].Type != MODULE_TYPE_USB_GPIO) && (n == 6)) || 
							((miModuleList[iModuleCnt-1].Type == MODULE_TYPE_CAMERA) && ((n == 24) || (n == 28) || (n == 29))))
							{
								if (GetParamSetting(n + 10, 59, Buff3, len3, Buff4, 64) == 1)
									miModuleList[iModuleCnt-1].Settings[n] = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
									else break;							
							}
							else
							{
								if (GetParamSetting(n + 10, 59, Buff3, len3, Buff4, 12) == 1)
									miModuleList[iModuleCnt-1].Settings[n] = (unsigned int)Str2Int(Buff4);
									else break;	
							}
						if ((miModuleList[iModuleCnt-1].Type == MODULE_TYPE_CAMERA) && ((n == 1) || (n == 37) || (n == 5)))
						{
							if (miModuleList[iModuleCnt-1].Settings[n] < RESOLUTION_TYPE_MAX)
							{
								int iW = 160;
								int iH = 120;
								GetResolutionFromMode(miModuleList[iModuleCnt-1].Settings[n], &iW, &iH);
								miModuleList[iModuleCnt-1].Settings[n] = ((iW & 0xFFFF) << 16) | (iH & 0xFFFF);
							}
						}
					}
				}				
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveModules()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cModuleFile, 1);
	if ((strlen(cModuleFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save modules:%s\n", cModuleFile);		
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		DBG_LOG_OUT();
		return 0;
	}	
	char Buff1[1024];	
	char Buff2[32];	
	unsigned int n, i;
	
	for (n = 0; n < iModuleCnt; n++)
	{
		if ((miModuleList[n].Local) && ((miModuleList[n].Enabled & 4) == 0))
		{
			memset(Buff1, 0, 1024);
			sprintf(Buff1, "Module=%i;%s;%.4s;%s;%i;%i;%i;%i;%i;%i;", 
					miModuleList[n].Enabled & 2 ? 1 : 0, 
					GetModuleTypeName(miModuleList[n].Type),
					(char*)&miModuleList[n].NewID,
					miModuleList[n].Name,
					miModuleList[n].ViewLevel,
					miModuleList[n].AccessLevel,
					miModuleList[n].Global,
					miModuleList[n].ScanSet,
					miModuleList[n].SaveChanges,
					miModuleList[n].GenEvents);
			fputs(Buff1, f);
			memset(Buff1, 0, 1024);
			switch(miModuleList[n].Type)
			{	
				case MODULE_TYPE_EXTERNAL:
					sprintf(Buff1, "%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],	//Type connect
						miModuleList[n].Settings[1],	//Address
						miModuleList[n].Settings[2],	//Speed
						miModuleList[n].Settings[3],	//SerialPortType
						miModuleList[n].Settings[4]		//SerialPortNum
						);	
					break;
				case MODULE_TYPE_USB_GPIO:
					for(i = 0; i < MAX_MODULE_SETTINGS; i++)
					{
						memset(Buff2, 0, 32);
						sprintf(Buff2, "%i;", miModuleList[n].Settings[i]);
						strcat(Buff1, Buff2);
					}
					strcat(Buff1, "\n");
					break;
				case MODULE_TYPE_GPIO:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_TEMP_SENSOR:
					sprintf(Buff1, "%s%s;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0] == I2C_ADDRESS_AM2320 ? "AM2320" : "",
						miModuleList[n].Settings[0] == I2C_ADDRESS_LM75 ? "LM75" : "",
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_ADS1015:
				case MODULE_TYPE_MCP3421:
				case MODULE_TYPE_AS5600:
				case MODULE_TYPE_HMC5883L:
					sprintf(Buff1, "%i;%i;%i;0;0;0;0;0;\n", miModuleList[n].Settings[0], miModuleList[n].Settings[1], miModuleList[n].Settings[2]);
					break;
				case MODULE_TYPE_RTC:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_RADIO:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_SPEAKER:
					sprintf(Buff1, "%.4s;%i;%i;%i;%i;%i;%s%s%s%s%s%s%s;%i\n",	
					(char*)&miModuleList[n].Settings[0],
					miModuleList[n].Settings[1],
					miModuleList[n].Settings[2],
					miModuleList[n].Settings[3],
					miModuleList[n].Settings[4],
					miModuleList[n].Settings[5],
					miModuleList[n].Settings[6] & SAMPLE_RATE_8000 ? "SR_8000," : "",
					miModuleList[n].Settings[6] & SAMPLE_RATE_11025 ? "SR_11025," : "",
					miModuleList[n].Settings[6] & SAMPLE_RATE_16000 ? "SR_16000," : "",
					miModuleList[n].Settings[6] & SAMPLE_RATE_22050 ? "SR_22050," : "",
					miModuleList[n].Settings[6] & SAMPLE_RATE_44100 ? "SR_44100," : "",
					miModuleList[n].Settings[6] & SAMPLE_RATE_48000 ? "SR_48000," : "",
					miModuleList[n].Settings[6] & SAMPLE_RATE_96000 ? "SR_96000," : "",
					miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_MIC:
					sprintf(Buff1, "%s%s;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;\n",
						miModuleList[n].Settings[0] == MODULE_SECSET_MONO ? "MONO" : "",
						miModuleList[n].Settings[0] == MODULE_SECSET_STEREO ? "STEREO" : "",
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7],
						miModuleList[n].Settings[8],
						miModuleList[n].Settings[9],
						miModuleList[n].Settings[10],
						miModuleList[n].Settings[11],
						miModuleList[n].Settings[12],
						miModuleList[n].Settings[13],
						miModuleList[n].Settings[14],
						miModuleList[n].Settings[15],
						miModuleList[n].Settings[16],
						miModuleList[n].Settings[17],
						miModuleList[n].Settings[18]);
					break;
				case MODULE_TYPE_DISPLAY:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_CAMERA:
					sprintf(Buff1, "%.4s;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;\n",	
						(char*)&miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7],
						miModuleList[n].Settings[8],
						miModuleList[n].Settings[9],
						miModuleList[n].Settings[10],
						miModuleList[n].Settings[11],
						miModuleList[n].Settings[12],
						miModuleList[n].Settings[13],
						miModuleList[n].Settings[14],
						miModuleList[n].Settings[15],
						miModuleList[n].Settings[16],
						miModuleList[n].Settings[17],
						miModuleList[n].Settings[18],
						miModuleList[n].Settings[19],
						miModuleList[n].Settings[20],
						miModuleList[n].Settings[21],
						miModuleList[n].Settings[22],
						miModuleList[n].Settings[23],
						miModuleList[n].Settings[24], //GetCodeNameExposure(miModuleList[n].Settings[24]),
						miModuleList[n].Settings[25],
						miModuleList[n].Settings[26],
						miModuleList[n].Settings[27],
						miModuleList[n].Settings[28], //GetCodeNameImageFilter(miModuleList[n].Settings[28]),
						miModuleList[n].Settings[29], //GetCodeNameWhiteBalance(miModuleList[n].Settings[29]),
						miModuleList[n].Settings[30],
						miModuleList[n].Settings[31],
						miModuleList[n].Settings[32],
						miModuleList[n].Settings[33],
						miModuleList[n].Settings[34],
						miModuleList[n].Settings[35],
						miModuleList[n].Settings[36],
						miModuleList[n].Settings[37],
						miModuleList[n].Settings[38],
						miModuleList[n].Settings[39],
						miModuleList[n].Settings[40],
						miModuleList[n].Settings[41],
						miModuleList[n].Settings[42]);
					break;
				case MODULE_TYPE_TIMER:
					sprintf(Buff1, "%s%s%s%s%s%s%s;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0] & 1 ? "[MO]" : "",
						miModuleList[n].Settings[0] & 2 ? "[TU]" : "",
						miModuleList[n].Settings[0] & 4 ? "[WE]" : "",
						miModuleList[n].Settings[0] & 8 ? "[TH]" : "",
						miModuleList[n].Settings[0] & 16 ? "[FR]" : "",
						miModuleList[n].Settings[0] & 32 ? "[SA]" : "",
						miModuleList[n].Settings[0] & 64 ? "[SU]" : "",
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_RS485:
				case MODULE_TYPE_PN532:
				case MODULE_TYPE_RC522:
				case MODULE_TYPE_TFP625A:
					sprintf(Buff1, "%.4s;%i;%i;%i;%i;%i;%i;%i;\n",	
						(char*)&miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_SYSTEM:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n",	
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_IR_RECEIVER:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_COUNTER:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_MEMORY:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				case MODULE_TYPE_KEYBOARD:
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
				default: 
					sprintf(Buff1, "%i;%i;%i;%i;%i;%i;%i;%i;\n", 
						miModuleList[n].Settings[0],
						miModuleList[n].Settings[1],
						miModuleList[n].Settings[2],
						miModuleList[n].Settings[3],
						miModuleList[n].Settings[4],
						miModuleList[n].Settings[5],
						miModuleList[n].Settings[6],
						miModuleList[n].Settings[7]);
					break;
			}
			fputs(Buff1, f);	
		}
	}	
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestModules(int iMode)
{
	int i, n;
	
	for (i = 0; i < iModuleCnt; i++)
	{
		for (n = 0; n < iModuleCnt; n++)
			if ((n != i) && (miModuleList[i].NewID == miModuleList[n].NewID))
			{
				if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) ID not unique: %.4s\n", i, (char*)&miModuleList[i].ID);
					else WEB_AddMessageInList("TestModules: MODULE(%i) ID not unique: %.4s", i, (char*)&miModuleList[i].ID);
				break;
			}
		if ((miModuleList[i].Type < 0) || (miModuleList[i].Type >= MODULE_TYPE_MAX))
		{
			if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) Type not between 0 and %i\n", i, MODULE_TYPE_MAX - 1);
				else WEB_AddMessageInList("TestModules: MODULE(%i) Type not between 0 and %i", i, MODULE_TYPE_MAX - 1);
		}
		if (miModuleList[i].ID == 0) 
		{
			if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) ID is null\n", i);
				else WEB_AddMessageInList("TestModules: MODULE(%i) ID is null", i);
		}
		if (strlen(miModuleList[i].Name) == 0) 
		{
			if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) Name is null\n", i);
			else WEB_AddMessageInList("TestModules: MODULE(%i) Name is null", i);
		}
		if ((miModuleList[i].ViewLevel < 0) || (miModuleList[i].ViewLevel > MAX_ACCESS_LEVELS))
		{
			if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) ViewLevel not between 0 and %i\n", i, MAX_ACCESS_LEVELS);
			else WEB_AddMessageInList("TestModules: MODULE(%i) AccessLevel not between 0 and %i", i, MAX_ACCESS_LEVELS);
		}
		if ((miModuleList[i].AccessLevel < 0) || (miModuleList[i].AccessLevel > MAX_ACCESS_LEVELS)) 
		{
			if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) AccessLevel not between 0 and %i\n", i, MAX_ACCESS_LEVELS);
			else WEB_AddMessageInList("TestModules: MODULE(%i) AccessLevel not between 0 and %i", i, MAX_ACCESS_LEVELS);
		}		
		switch (miModuleList[i].Type)
		{
			case MODULE_TYPE_GPIO:
				if (miModuleList[i].Settings[0] > 3)
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) GPIO Settings[0] wrong\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) GPIO Settings[0] wrong", i);
				}
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 40))
				{
					if (iMode == 0)	dbgprintf(2, "TestModules: MODULE(%i) GPIO Settings[1] not between 0 and 40\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) GPIO Settings[1] not between 0 and 40", i);
				}
				if ((!(miModuleList[i].Settings[0] & MODULE_SECSET_OUTPUT)) && (miModuleList[i].Settings[2] != 0))
				{
					if (iMode == 0)	dbgprintf(2, "TestModules: MODULE(%i) GPIO Settings[2] for INPUT not 0\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) GPIO Settings[2] for INPUT not 0", i);
				}
				break;
			case MODULE_TYPE_TEMP_SENSOR:
				if ((miModuleList[i].Settings[0] != I2C_ADDRESS_AM2320) && (miModuleList[i].Settings[0] != I2C_ADDRESS_LM75))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) TEMP_SENSOR Settings[0] not in (AM2320, LM75)\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) TEMP_SENSOR Settings[0] not in (AM2320, LM75)", i);
				}
				if ((miModuleList[i].Settings[0] == I2C_ADDRESS_LM75) && ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 7)))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) TEMP_SENSOR LM75 Settings[1] not between 0 and 40\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) TEMP_SENSOR LM75 Settings[1] not between 0 and 40", i);
				}
				if ((miModuleList[i].Settings[0] != I2C_ADDRESS_LM75) && (miModuleList[i].Settings[1] != 0))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) TEMP_SENSOR NOT LM75 Settings[1] not 0\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) TEMP_SENSOR NOT LM75 Settings[1] not 0", i);
				}
				if ((miModuleList[i].Settings[2] < 0) || (miModuleList[i].Settings[2] > 4000000))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) TEMP_SENSOR Settings[2] Speed not between 0 and 4000000\n", i);
						else WEB_AddMessageInList("TestModules: MODULE(%i) TEMP_SENSOR Settings[2] Speed not between 0 and 4000000", i);
				}
				break;
			case MODULE_TYPE_PN532:
				break;
			case MODULE_TYPE_RTC:
				if ((miModuleList[i].Settings[2] < 0) || (miModuleList[i].Settings[2] > 4000000))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) RTC Settings[2] Speed not between 0 and 4000000\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) RTC Settings[2] Speed not between 0 and 4000000", i);
				}
				break;
			case MODULE_TYPE_RADIO:
				if ((miModuleList[i].Settings[2] < 0) || (miModuleList[i].Settings[2] > 4000000))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) RADIO Settings[2] Speed not between 0 and 4000000\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) RADIO Settings[2] Speed not between 0 and 4000000", i);
				}
				break;
			case MODULE_TYPE_SPEAKER:
				if (miModuleList[i].Settings[0] == 0) 
				{
					if (iMode == 0) dbgprintf(3, "TestModules: MODULE(%i) SPEAKER Settings[0] SubID is null\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[0] SubID is null", i);
				}
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) SPEAKER Settings[1] Device not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[1] Device not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[2] < 0) || (miModuleList[i].Settings[2] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) SPEAKER Settings[2] SubDevice not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[2] SubDevice not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[3] < 0) || (miModuleList[i].Settings[3] > 2))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) SPEAKER Settings[3] Channels not between 0 and 2\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[3] Channels not between 0 and 2", i);
				}
				if ((miModuleList[i].Settings[4] < 0) || (miModuleList[i].Settings[4] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) SPEAKER Settings[4] MaxSoundsPlay not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[4] MaxSoundsPlay not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[5] < 0) || (miModuleList[i].Settings[5] > 300))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) SPEAKER Settings[5] TimeOutPwrSndOff not between 0 and 300\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[5] TimeOutPwrSndOff not between 0 and 300", i);
				}
				if ((miModuleList[i].Settings[6] < 0) || (miModuleList[i].Settings[6] >= 128))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) SPEAKER Settings[5] SampleRate not in SR_ANY(0), SR_8000, SR_11000, SR_16000, SR_22000, SR_44000, SR_48000 and SR_96000\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) SPEAKER Settings[5] SampleRate not in SR_ANY(0), SR_8000, SR_11000, SR_16000, SR_22000, SR_44000, SR_48000 and SR_96000", i);
				}
				break;
			case MODULE_TYPE_MIC:
				if ((miModuleList[i].Settings[0] < 1) || (miModuleList[i].Settings[0] > 2))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[0] Channels not between 1 and 2\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[0] Channels not between 1 and 2", i);
				}
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[1] Device not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[1] Device not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[2] < 0) || (miModuleList[i].Settings[2] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[2] SubDevice not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[2] SubDevice not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[3] != 8000) && (miModuleList[i].Settings[3] != 11025) &&
					(miModuleList[i].Settings[3] != 16000) && (miModuleList[i].Settings[3] != 22050) && (miModuleList[i].Settings[3] != 44100) &&
					(miModuleList[i].Settings[3] != 48000) && (miModuleList[i].Settings[3] != 96000))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[3] SampleRate not in (8000, 11025, 16000, 22050, 44100, 48000, 96000)\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[3] SampleRate not in (8000, 11025, 16000, 22050, 44100, 48000, 96000)", i);
				}
				/*if (((miModuleList[i].Settings[4] < 0) || (miModuleList[i].Settings[4] > 5)) && 
					((miModuleList[i].Settings[4] != 32000) && (miModuleList[i].Settings[4] != 96000) &&
					(miModuleList[i].Settings[4] != 128000) && (miModuleList[i].Settings[4] != 160000) && (miModuleList[i].Settings[4] != 192000) &&
					(miModuleList[i].Settings[4] != 256000) && (miModuleList[i].Settings[4] != 320000)))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[4] BitRate not in (VAR(1-5), CONST(32000, 96000, 128000, 160000, 192000, 256000, 320000))\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[4] BitRate not in (VAR(1-5), CONST(32000, 96000, 128000, 160000, 192000, 256000, 320000))", i);
				}*/
				if ((miModuleList[i].Settings[5] < 0) || (miModuleList[i].Settings[5] > 1024))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[5] ScanInterval not between 0 and 1024\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[5] ScanInterval not between 0 and 1024", i);
				}
				if ((miModuleList[i].Settings[6] < 0) || (miModuleList[i].Settings[6] > 100))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MIC Settings[6] SenceLevel not between 0 and 100\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MIC Settings[6] SenceLevel not between 0 and 100", i);
				}
				break;
			case MODULE_TYPE_DISPLAY:
				break;
			case MODULE_TYPE_CAMERA:
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[1] Resolution not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[1] Resolution not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[2] < 2) || (miModuleList[i].Settings[2] > 90))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[2] FrameRate not between 2 and 90\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[2] FrameRate not between 2 and 90", i);
				}
				if ((miModuleList[i].Settings[4] < 16) || (miModuleList[i].Settings[4] > 100000))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[4] BitRate not between 16 and 100000\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[4] BitRate not between 16 and 100000", i);
				}
				if ((miModuleList[i].Settings[5] < 0) || (miModuleList[i].Settings[5] > 10))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[5] MoveSensorResolution not between 0 and 10\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[5] MoveSensorResolution not between 0 and 10", i);
				}
				if ((miModuleList[i].Settings[6] < 0) || (miModuleList[i].Settings[6] > 100))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[6] MoveDetectLevel not between 0 and 100\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[6] MoveDetectLevel not between 0 and 100", i);
				}
				if ((miModuleList[i].Settings[7] < 0) || (miModuleList[i].Settings[7] > 100))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[7] MoveRecLevel not between 0 and 100\n", i);	
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[7] MoveRecLevel not between 0 and 100", i);	
				}
				if ((miModuleList[i].Settings[16] < 0) || (miModuleList[i].Settings[16] > 100))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) CAMERA Settings[6] MoveColorDetectLevel not between 0 and 100\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) CAMERA Settings[6] MoveColorDetectLevel not between 0 and 100", i);
				}
				break;
			case MODULE_TYPE_RS485:
				if ((miModuleList[i].Settings[1] != 0) && (miModuleList[i].Settings[1] != 4800) &&
					(miModuleList[i].Settings[1] != 9600) && (miModuleList[i].Settings[1] != 19200) &&
					(miModuleList[i].Settings[1] != 38400) && (miModuleList[i].Settings[1] != 57600) && 
					(miModuleList[i].Settings[1] != 115200))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) RS485 Settings[1] BaudRate not in (0, 4800, 9600, 19200, 38400, 57600, 115200)\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) RS485 Settings[1] BaudRate not in (0, 4800, 9600, 19200, 38400, 57600, 115200)", i);
				}
				break;
			case MODULE_TYPE_RC522:
				break;
			case MODULE_TYPE_TIMER:
				if ((miModuleList[i].Settings[0] < 0) || (miModuleList[i].Settings[0] > 127))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) TIMER Settings[0] Days not between 0 and 127\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) TIMER Settings[0] Days not between 0 and 127", i);
				}
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 235959))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) TIMER Settings[1] Time not between 0 and 235959\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) TIMER Settings[1] Time not between 0 and 235959", i);
				}
				break;
			case MODULE_TYPE_SYSTEM:
				break;
			case MODULE_TYPE_IR_RECEIVER:
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 100))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) IR_RECEIVER Settings[1] Time not between 0 and 100\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) IR_RECEIVER Settings[1] Time not between 0 and 100", i);
				}
				break;
			case MODULE_TYPE_COUNTER:
				break;
			case MODULE_TYPE_MEMORY:
				break;	
			case MODULE_TYPE_KEYBOARD:
				break;
			case MODULE_TYPE_USB_GPIO:
				break;
			case MODULE_TYPE_EXTERNAL:
				break;
			case MODULE_TYPE_ADS1015:
				break;
			case MODULE_TYPE_TFP625A:
				break;
			case MODULE_TYPE_MCP3421:
				if ((miModuleList[i].Settings[1] < 0) || (miModuleList[i].Settings[1] > 3))
				{
					if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) MCP3421 Settings[1] Resolution not between 0 and 3\n", i);
					else WEB_AddMessageInList("TestModules: MODULE(%i) MCP3421 Settings[1] Resolution not between 0 and 3", i);
				}
				break;
			case MODULE_TYPE_AS5600:
				break;
			case MODULE_TYPE_HMC5883L:
				break;
			default:
				if (iMode == 0) dbgprintf(2, "TestModules: MODULE(%i) Type wrong\n", i);
				else WEB_AddMessageInList("TestModules: MODULE(%i) Type wrong", i);
				break;
		}		
	}
	return 1;
}

int LoadSounds(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "SOUND=") == 1) 
				{
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 256) == 1)
					{
						misc_buffer m_buff;
						
						//if (load_file_in_buff(Buff4, &m_buff))
						AudioFileInBuffer(Buff4, &m_buff.void_data, &m_buff.data_size, SAMPLE_RATE_44100, 2);
						{
							iSoundListCnt++;
							mSoundList = (SYS_SOUND_TYPE*)DBG_REALLOC(mSoundList, sizeof(SYS_SOUND_TYPE)*iSoundListCnt);
							memset(&mSoundList[iSoundListCnt-1], 0, sizeof(SYS_SOUND_TYPE));
							
							if (GetParamSetting(0, 59, Buff3, len3, Buff4, 5))
								memcpy(&mSoundList[iSoundListCnt-1].ID, Buff4, 4);
							GetParamSetting(1, 59, Buff3, len3, Buff4, 256);
							strcpy(mSoundList[iSoundListCnt-1].Path, Buff4);
							mSoundList[iSoundListCnt-1].Data = m_buff.void_data;
							mSoundList[iSoundListCnt-1].Len = m_buff.data_size;
						}
					}
					else dbgprintf(2, "LoadSettings: error load SOUND setting\n");					
				}					
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveSounds()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cSoundFile, 1);
	if ((strlen(cSoundFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cSoundFile);		
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		DBG_LOG_OUT();
		return 0;
	}	
	char Buff1[1024];	
	unsigned int n;	
	
	for (n = 0; n < iSoundListCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "Sound=%.4s;%s;\n", (char*)&mSoundList[n].ID, mSoundList[n].Path);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestSounds(int iMode)
{
	int i;
	for (i = 0; i < iSoundListCnt; i++)
	{
		if (mSoundList[i].ID == 0) 
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SOUND(%i) ID is null\n", i);
			else WEB_AddMessageInList("TestSettings: SOUND(%i) ID is null", i);
		}
		if (strlen(mSoundList[i].Path) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SOUND(%i) Path is null\n", i);
			else WEB_AddMessageInList("TestSettings: SOUND(%i) Path is null", i);
		}
		if (mSoundList[i].Data == NULL)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SOUND(%i) DATA point null\n", i);
			else WEB_AddMessageInList("TestSettings: SOUND(%i) DATA point null", i);
		}
		if (mSoundList[i].Len == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: SOUND(%i) DATA length = 0\n", i);
			else WEB_AddMessageInList("TestSettings: SOUND(%i) DATA length = 0", i);
		}			
	}
	return 1;
}

int LoadEvntActions(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);
				len3 = strlen(Buff3);
				
				if (SearchStrInData(Buff2, len2, 0, "ACTION=") == 1)
				{
					iActionCnt++;
					maActionInfo = (ACTION_INFO*)DBG_REALLOC(maActionInfo, sizeof(ACTION_INFO)*iActionCnt);
					memset(&maActionInfo[iActionCnt-1], 0, sizeof(ACTION_INFO));
					
					maActionInfo[iActionCnt-1].TestType = ACTION_TEST_TYPE_EQUALLY;
					maActionInfo[iActionCnt-1].Test2Type = ACTION_TEST_TYPE_EQUALLY;
					maActionInfo[iActionCnt-1].Test3Type = ACTION_TEST_TYPE_EQUALLY;
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&maActionInfo[iActionCnt-1].ActionID, Buff4, 4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].Enabled = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&maActionInfo[iActionCnt-1].SrcID, Buff4, 4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].SrcSubNumber = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 2) == 1)
					{
						//if (Buff4[0] == ACTION_TEST_TYPE_EQUALLY) maActionInfo[iActionCnt-1].TestType = ACTION_TEST_TYPE_EQUALLY;
						if (Buff4[0] == ACTION_TEST_TYPE_LESS) maActionInfo[iActionCnt-1].TestType = ACTION_TEST_TYPE_LESS;
						if (Buff4[0] == ACTION_TEST_TYPE_MORE) maActionInfo[iActionCnt-1].TestType = ACTION_TEST_TYPE_MORE;	
						if (Buff4[0] == ACTION_TEST_TYPE_AND) maActionInfo[iActionCnt-1].TestType = ACTION_TEST_TYPE_AND;	
						if (Buff4[0] == ACTION_TEST_TYPE_NOT) maActionInfo[iActionCnt-1].TestType = ACTION_TEST_TYPE_NOT;	
					}
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].SrcStatus = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 1);
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&maActionInfo[iActionCnt-1].TestModuleID, Buff4, 4);
					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].TestModuleSubNumber = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(8, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].TestModuleStatus = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
					if (GetParamSetting(9, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&maActionInfo[iActionCnt-1].DestID, Buff4, 4);
					if (GetParamSetting(10, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].DestSubNumber = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 1);
					if (GetParamSetting(11, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].DestStatus = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
					if (GetParamSetting(12, 59, Buff3, len3, Buff4, 128))
						strcpy(maActionInfo[iActionCnt-1].Name, Buff4);
					if (GetParamSetting(13, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].AfterAccept = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
					if (GetParamSetting(14, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].TimerWaitBefore = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(15, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].TimerOff = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(16, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].WeekDays = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
					if (GetParamSetting(17, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].NotBeforeTime = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(18, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].NotAfterTime = (unsigned int)Str2Int(Buff4);		
					if (GetParamSetting(19, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].RestartTimer = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(20, 59, Buff3, len3, Buff4, 2) == 1)
					{
						//if (Buff4[0] == ACTION_TEST_TYPE_EQUALLY) maActionInfo[iActionCnt-1].Test2Type = ACTION_TEST_TYPE_EQUALLY;
						if (Buff4[0] == ACTION_TEST_TYPE_LESS) maActionInfo[iActionCnt-1].Test2Type = ACTION_TEST_TYPE_LESS;
						if (Buff4[0] == ACTION_TEST_TYPE_MORE) maActionInfo[iActionCnt-1].Test2Type = ACTION_TEST_TYPE_MORE;
						if (Buff4[0] == ACTION_TEST_TYPE_AND) maActionInfo[iActionCnt-1].Test2Type = ACTION_TEST_TYPE_AND;	
						if (Buff4[0] == ACTION_TEST_TYPE_NOT) maActionInfo[iActionCnt-1].Test2Type = ACTION_TEST_TYPE_NOT;									
					}
					if (GetParamSetting(21, 59, Buff3, len3, Buff4, 128))
						strcpy(maActionInfo[iActionCnt-1].GroupName, Buff4);
					if (GetParamSetting(22, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&maActionInfo[iActionCnt-1].Test3ModuleID, Buff4, 4);
					if (GetParamSetting(23, 59, Buff3, len3, Buff4, 10) == 1)
						maActionInfo[iActionCnt-1].Test3ModuleSubNumber = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(24, 59, Buff3, len3, Buff4, 2) == 1)
					{
						//if (Buff4[0] == ACTION_TEST_TYPE_EQUALLY) maActionInfo[iActionCnt-1].Test3Type = ACTION_TEST_TYPE_EQUALLY;
						if (Buff4[0] == ACTION_TEST_TYPE_LESS) maActionInfo[iActionCnt-1].Test3Type = ACTION_TEST_TYPE_LESS;
						if (Buff4[0] == ACTION_TEST_TYPE_MORE) maActionInfo[iActionCnt-1].Test3Type = ACTION_TEST_TYPE_MORE;
						if (Buff4[0] == ACTION_TEST_TYPE_AND) maActionInfo[iActionCnt-1].Test3Type = ACTION_TEST_TYPE_AND;	
						if (Buff4[0] == ACTION_TEST_TYPE_NOT) maActionInfo[iActionCnt-1].Test3Type = ACTION_TEST_TYPE_NOT;
					}
					if (GetParamSetting(25, 59, Buff3, len3, Buff4, 64) == 1)
						maActionInfo[iActionCnt-1].Test3ModuleStatus = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
					
				}		
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveEvntActions()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&evntaction_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cEvntActionFile, 1);
	if ((strlen(cEvntActionFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cEvntActionFile);		
		DBG_MUTEX_UNLOCK(&evntaction_mutex);
		DBG_LOG_OUT();
		return 0;
	}	
	char Buff1[1024];
	char SrcStatusBuff[64];
	char DestStatusBuff[64];
	char TestStatusBuff[64];	
	char Test3StatusBuff[64];	
	char SubNumberBuff[64];	
	unsigned int n;	
	
	for (n = 0; n < iActionCnt; n++)
	{
		if (maActionInfo[n].Enabled >= 0)
		{
			memset(Buff1, 0, 1024);
			sprintf(Buff1, "Action=%.4s;%i;%.4s;%i;%c;%s;%.4s;%i;%s;%.4s;%s;%s;%s;%s;%i;%i;%s%s%s%s%s%s%s;%i;%i;%i;%c;%s;%.4s;%i;%c;%s;\n", (char*)&maActionInfo[n].ActionID, maActionInfo[n].Enabled,
							(char*)&maActionInfo[n].SrcID, maActionInfo[n].SrcSubNumber, 
							((maActionInfo[n].TestType == ACTION_TEST_TYPE_AND) 
								|| (maActionInfo[n].TestType == ACTION_TEST_TYPE_LESS) 
								|| (maActionInfo[n].TestType == ACTION_TEST_TYPE_EQUALLY) 
								|| (maActionInfo[n].TestType == ACTION_TEST_TYPE_MORE)
								|| (maActionInfo[n].TestType == ACTION_TEST_TYPE_NOT)) ? (char)maActionInfo[n].TestType : ACTION_TEST_TYPE_EQUALLY, 
							GetActionCodeName(maActionInfo[n].SrcStatus, SrcStatusBuff, 64, 2), 
							(char*)&maActionInfo[n].TestModuleID, maActionInfo[n].TestModuleSubNumber, 
							GetActionCodeName(maActionInfo[n].TestModuleStatus, TestStatusBuff, 64, 2), 
							(char*)&maActionInfo[n].DestID, 
							GetActionCodeName(maActionInfo[n].DestSubNumber, SubNumberBuff, 64, 2), 
							GetActionCodeName(maActionInfo[n].DestStatus, DestStatusBuff, 64, 2),
							maActionInfo[n].Name, 
							GetAfterAcceptName(maActionInfo[n].AfterAccept), maActionInfo[n].TimerWaitBefore, maActionInfo[n].TimerOff,
							maActionInfo[n].WeekDays & 1 ? "[Mo]" : "",
							maActionInfo[n].WeekDays & 2 ? "[Tu]" : "",
							maActionInfo[n].WeekDays & 4 ? "[We]" : "",
							maActionInfo[n].WeekDays & 8 ? "[Th]" : "",
							maActionInfo[n].WeekDays & 16 ? "[Fr]" : "",
							maActionInfo[n].WeekDays & 32 ? "[Sa]" : "",
							maActionInfo[n].WeekDays & 64 ? "[Su]" : "",
							maActionInfo[n].NotBeforeTime, maActionInfo[n].NotAfterTime,
							maActionInfo[n].RestartTimer, 
							((maActionInfo[n].Test2Type == ACTION_TEST_TYPE_AND) 
								|| (maActionInfo[n].Test2Type == ACTION_TEST_TYPE_LESS)
								|| (maActionInfo[n].Test2Type == ACTION_TEST_TYPE_EQUALLY)
								|| (maActionInfo[n].Test2Type == ACTION_TEST_TYPE_MORE)
								|| (maActionInfo[n].Test2Type == ACTION_TEST_TYPE_NOT)) ? (char)maActionInfo[n].Test2Type : ACTION_TEST_TYPE_EQUALLY,
							maActionInfo[n].GroupName,
							(char*)&maActionInfo[n].Test3ModuleID, maActionInfo[n].Test3ModuleSubNumber,
							((maActionInfo[n].Test3Type == ACTION_TEST_TYPE_AND) 
								|| (maActionInfo[n].Test3Type == ACTION_TEST_TYPE_LESS)
								|| (maActionInfo[n].Test3Type == ACTION_TEST_TYPE_EQUALLY)
								|| (maActionInfo[n].Test3Type == ACTION_TEST_TYPE_MORE)
								|| (maActionInfo[n].Test3Type == ACTION_TEST_TYPE_NOT)) ? (char)maActionInfo[n].Test3Type : ACTION_TEST_TYPE_EQUALLY,								 
								GetActionCodeName(maActionInfo[n].Test3ModuleStatus, Test3StatusBuff, 64, 2)							
							);
			fputs(Buff1, f);
		}
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&evntaction_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestEvntActions(int iMode)
{
	int i;
	for (i = 0; i < iActionCnt; i++)
	{
		if (maActionInfo[i].Enabled >= 0)
		{
			if (maActionInfo[i].ActionID == 0)		
			{
				if (iMode == 0) dbgprintf(3, "TestSettings: ACTION(%i) ActionID is null\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) ActionID is null", i);
			}			
			if ((maActionInfo[i].Enabled < 0) || (maActionInfo[i].Enabled > 1))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) Enabled not between 0 and 1\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) Enabled not between 0 and 1", i);
			}			
			//if (maActionInfo[i].SrcID == 0) dbgprintf(2, "TestSettings: ACTION(%i) SrcID is null\n", i);
			if ((maActionInfo[i].TestType != ACTION_TEST_TYPE_AND) &&
				(maActionInfo[i].TestType != ACTION_TEST_TYPE_LESS) &&
				(maActionInfo[i].TestType != ACTION_TEST_TYPE_EQUALLY) &&
				(maActionInfo[i].TestType != ACTION_TEST_TYPE_MORE) &&
				(maActionInfo[i].TestType != ACTION_TEST_TYPE_NOT))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) TestType not in (< = > &)\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) TestType not in (< = > &)", i);
			}	
			if ((maActionInfo[i].Test2Type != ACTION_TEST_TYPE_AND) &&
				(maActionInfo[i].Test2Type != ACTION_TEST_TYPE_LESS) &&
				(maActionInfo[i].Test2Type != ACTION_TEST_TYPE_EQUALLY) &&
				(maActionInfo[i].Test2Type != ACTION_TEST_TYPE_MORE) &&
				(maActionInfo[i].Test2Type != ACTION_TEST_TYPE_NOT))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) Test2Type not in (< = > &)\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) Test2Type not in (< = > &)", i);
			}	
			if ((maActionInfo[i].Test3Type != ACTION_TEST_TYPE_AND) &&
				(maActionInfo[i].Test3Type != ACTION_TEST_TYPE_LESS) &&
				(maActionInfo[i].Test3Type != ACTION_TEST_TYPE_EQUALLY) &&
				(maActionInfo[i].Test3Type != ACTION_TEST_TYPE_MORE) &&
				(maActionInfo[i].Test3Type != ACTION_TEST_TYPE_NOT))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) Test3Type not in (< = > &)\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) Test3Type not in (< = > &)", i);
			}			
			if (maActionInfo[i].DestID == 0)		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) DestID is null\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) DestID is null", i);
			}			
			if (strlen(maActionInfo[i].Name) == 0)		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) Name is null\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) Name is null", i);
			}			
			//if ((maActionInfo[i].AfterAccept < 0) || (maActionInfo[i].AfterAccept > 2)) dbgprintf(2, "TestSettings: ACTION(%i) AfterAccept not between 0 and 2\n", i);
			if ((maActionInfo[i].WeekDays < 0) || (maActionInfo[i].WeekDays > 127))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) WeekDays not between 0 and 127\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) WeekDays not between 0 and 127", i);
			}					
			if ((maActionInfo[i].NotBeforeTime < 0) || (maActionInfo[i].NotBeforeTime > 235959))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) NotBeforeTime not between 0 and 235959\n", i);
				else WEB_AddMessageInList("TestSettings: ACTION(%i) NotBeforeTime not between 0 and 235959", i);
			}					
			if ((maActionInfo[i].NotAfterTime < 0) || (maActionInfo[i].NotAfterTime > 235959))		
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: ACTION(%i) NotAfterTime not between 0 and 235959\n", i);	
				else WEB_AddMessageInList("TestSettings: ACTION(%i) NotAfterTime not between 0 and 235959", i);
			}	
		}
	}
	return 1;
}

int LoadMnlActions(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "ACTIONMANUAL=") == 1)
				{
					iActionManualCnt++;
					amiActionManual = (ACTION_MANUAL_INFO*)DBG_REALLOC(amiActionManual, sizeof(ACTION_MANUAL_INFO)*iActionManualCnt);
					memset(&amiActionManual[iActionManualCnt-1], 0, sizeof(ACTION_MANUAL_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						amiActionManual[iActionManualCnt-1].Access = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 5) == 1)
						memcpy(&amiActionManual[iActionManualCnt-1].ID, Buff4, 4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64) == 1)
						amiActionManual[iActionManualCnt-1].SubNumber = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 1);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 64) == 1)
						amiActionManual[iActionManualCnt-1].Status = (unsigned int)GetModuleSettings(Buff4, strlen(Buff4), 0);
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 128))
						strcpy(amiActionManual[iActionManualCnt-1].Name, Buff4);
				}			
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveMnlActions()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&mnlaction_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cMnlActionFile, 1);
	if ((strlen(cMnlActionFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cMnlActionFile);		
		DBG_MUTEX_UNLOCK(&mnlaction_mutex);
		DBG_LOG_OUT();
		return 0;
	}	
	char Buff1[1024];
	char StatusBuff[64];
	char SubNumberBuff[64];	
	unsigned int n;	
	
	for (n = 0; n < iActionManualCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "ActionManual=%i;%.4s;%s;%s;%s;\n", 
						amiActionManual[n].Access, (char*)&amiActionManual[n].ID,
						GetActionCodeName(amiActionManual[n].SubNumber, SubNumberBuff, 64, 0),
						GetActionCodeName(amiActionManual[n].Status, StatusBuff, 64, 0),
						amiActionManual[n].Name);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&mnlaction_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestMnlActions(int iMode)
{
	int i;
	for (i = 0; i < iActionManualCnt; i++)
	{
		if ((amiActionManual[i].Access < 0) || (amiActionManual[i].Access > MAX_ACCESS_LEVELS))		
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ACTIONMENU(%i) Access not between 0 and %i\n", i, MAX_ACCESS_LEVELS);
			else WEB_AddMessageInList("TestSettings: ACTIONMENU(%i) Access not between 0 and %i\n", i, MAX_ACCESS_LEVELS);
		}			
		if (amiActionManual[i].ID == 0)
		{
			if (iMode == 0) dbgprintf(3, "TestSettings: ACTIONMENU(%i) ID is null\n", i);
			else WEB_AddMessageInList("TestSettings: ACTIONMENU(%i) ID is null", i);
		}			
		if (strlen(amiActionManual[i].Name) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: ACTIONMENU(%i) Name is null\n", i);
			else WEB_AddMessageInList("TestSettings: ACTIONMENU(%i) Name is null", i);
		}			
	}
	return 1;
}

int LoadMails(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "MAIL=") == 1)
				{
					iMailCnt++;
					miMailList = (MAIL_INFO*)DBG_REALLOC(miMailList, sizeof(MAIL_INFO)*iMailCnt);
					memset(&miMailList[iMailCnt-1], 0, sizeof(MAIL_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						miMailList[iMailCnt-1].Group = (unsigned int)Str2Int(Buff4);	
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 64) == 1)
						strcpy(miMailList[iMailCnt-1].Address, Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64) == 1)
						strcpy(miMailList[iMailCnt-1].Address2, Buff4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 64))
						strcpy(miMailList[iMailCnt-1].MainText, Buff4);
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 64))
						strcpy(miMailList[iMailCnt-1].BodyText, Buff4);
					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 64) == 1)
						strcpy(miMailList[iMailCnt-1].FilePath, Buff4);
					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 64) == 1)
						strcpy(miMailList[iMailCnt-1].FilePath2, Buff4);					
				}					
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveMails()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cMailFile, 1);
	if ((strlen(cMailFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cMailFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}

	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iMailCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "Mail=%i;%s;%s;%s;%s;%s;%s;\n", miMailList[n].Group, miMailList[n].Address,
				miMailList[n].Address2, miMailList[n].MainText, miMailList[n].BodyText,
				miMailList[n].FilePath, miMailList[n].FilePath2);
		fputs(Buff1, f);
	}

	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_LOG_OUT();
	return 1;
}

int TestMails(int iMode)
{
	int i;
	for (i = 0; i < iMailCnt; i++)
	{
		if ((miMailList[i].Group < 0) || (miMailList[i].Group > 20))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) Group not between 0 and 20\n", i);
			else WEB_AddMessageInList("TestSettings: MAIL(%i) Group not between 0 and 20", i);
		}			
		if ((strlen(miMailList[i].Address) < 1) || (strlen(miMailList[i].Address) > 30))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) Address length not between 1 and 30\n", i);
			else WEB_AddMessageInList("TestSettings: MAIL(%i) Address length not between 1 and 30", i);
		}			
		if ((strlen(miMailList[i].Address2) != 0) && (strlen(miMailList[i].Address2) > 30))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) Address2 length not between 1 and 30\n", i);
			else WEB_AddMessageInList("TestSettings: MAIL(%i) Address2 length not between 1 and 30", i);
		}			
		if (strlen(miMailList[i].MainText) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) MainText is null\n", i);	
			else WEB_AddMessageInList("TestSettings: MAIL(%i) MainText is null", i);	
		}			
		if (strlen(miMailList[i].BodyText) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) BodyText is null\n", i);
			else WEB_AddMessageInList("TestSettings: MAIL(%i) BodyText is null", i);
		}			
		/*if (strlen(miMailList[i].FilePath) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) FilePath is null\n", i);
			else WEB_AddMessageInList("TestSettings: MAIL(%i) FilePath is null", i);
		}			
		if (strlen(miMailList[i].FilePath2) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: MAIL(%i) FilePath2 is null\n", i);
			else WEB_AddMessageInList("TestSettings: MAIL(%i) FilePath2 is null", i);
		}*/ 
	}
	return 1;
}

int LoadStreamTypes(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
								
				if (SearchStrInData(Buff2, len2, 0, "STREAMTYPE=") == 1)
				{
					iStreamTypeCnt++;
					stStreamType = (STREAM_TYPE*)DBG_REALLOC(stStreamType, sizeof(STREAM_TYPE)*iStreamTypeCnt);
					memset(&stStreamType[iStreamTypeCnt-1], 0, sizeof(STREAM_TYPE));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 64))
						strcpy(stStreamType[iStreamTypeCnt-1].Name, Buff4);					
				}				
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveStreamTypes()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cStreamTypeFile, 1);
	if ((strlen(cStreamTypeFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cStreamTypeFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}
	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iStreamTypeCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "StreamType=%s;\n", stStreamType[n].Name);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestStreamTypes(int iMode)
{
	int i;	
	for (i = 0; i < iStreamTypeCnt; i++)
		if (strlen(stStreamType[i].Name) == 0) dbgprintf(2, "TestSettings: STREAMTYPE(%i) Name is null\n", i);
	return 1;
}

int LoadPTZs(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "PTZ=") == 1)
				{
					iPtzSettingsListCnt++;
					psiPtzSettingsList = (PTZ_SET_INFO*)DBG_REALLOC(psiPtzSettingsList, sizeof(PTZ_SET_INFO)*iPtzSettingsListCnt);
					memset(&psiPtzSettingsList[iPtzSettingsListCnt-1], 0, sizeof(PTZ_SET_INFO));
					
					psiPtzSettingsList[iPtzSettingsListCnt-1].Used = 1;
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						psiPtzSettingsList[iPtzSettingsListCnt-1].PanTiltX = Str2Float(Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						psiPtzSettingsList[iPtzSettingsListCnt-1].PanTiltY = Str2Float(Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 10) == 1)
						psiPtzSettingsList[iPtzSettingsListCnt-1].Zoom = Str2Float(Buff4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 10) == 1)
						psiPtzSettingsList[iPtzSettingsListCnt-1].Focus = Str2Float(Buff4);
					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 64))
						strcpy(psiPtzSettingsList[iPtzSettingsListCnt-1].Name, Buff4);
				}							
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SavePTZs()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&ptz_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cPtzFile, 1);
	if ((strlen(cPtzFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cPtzFile);		
		DBG_MUTEX_UNLOCK(&ptz_mutex);
		DBG_LOG_OUT();
		return 0;
	}
	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iPtzSettingsListCnt; n++)
	{
		if (psiPtzSettingsList[n].Used)
		{
			memset(Buff1, 0, 1024);
			sprintf(Buff1, "PTZ=%g;%g;%g;%g;%s;\n", psiPtzSettingsList[n].PanTiltX, psiPtzSettingsList[n].PanTiltY,
										psiPtzSettingsList[n].Zoom, psiPtzSettingsList[n].Focus, psiPtzSettingsList[n].Name);
			fputs(Buff1, f);
		}
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&ptz_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestPTZs(int iMode)
{
	int i;	
	for (i = 0; i < iPtzSettingsListCnt; i++)
	{
		if (psiPtzSettingsList[i].Used)
		{
			if ((psiPtzSettingsList[i].Zoom < 0) || (psiPtzSettingsList[i].Zoom > 1))
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: PTZ(%i) Zoom level not between 0 and 1\n", i);
				else WEB_AddMessageInList("TestSettings: PTZ(%i) Zoom level not between 0 and 1\n", i);
			}
			if ((psiPtzSettingsList[i].PanTiltX < -1.0f) || (psiPtzSettingsList[i].PanTiltX > 1.0f))
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: PTZ(%i) PanTiltX level not between -1.0 and 1.0\n", i);
				else WEB_AddMessageInList("TestSettings: PTZ(%i) PanTiltX level not between -1.0 and 1.0\n", i);
			}
			if ((psiPtzSettingsList[i].PanTiltY < -1.0f) || (psiPtzSettingsList[i].PanTiltY > 1.0f))
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: PTZ(%i) PanTiltY level not between -1.0 and 1.0\n", i);
				else WEB_AddMessageInList("TestSettings: PTZ(%i) PanTiltY level not between -1.0 and 1.0\n", i);
			}
			if ((psiPtzSettingsList[i].Focus < 0) || (psiPtzSettingsList[i].Focus > 1))
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: PTZ(%i) Focus level not between 0 and 1\n", i);
				else WEB_AddMessageInList("TestSettings: PTZ(%i) Focus level not between 0 and 1\n", i);
			}
			if (strlen(psiPtzSettingsList[i].Name) == 0)
			{
				if (iMode == 0) dbgprintf(2, "TestSettings: PTZ(%i) Name is null\n", i);
				else WEB_AddMessageInList("TestSettings: PTZ(%i) Name is null", i);
			}
		}		
	}
	return 1;
}

int LoadStreams(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "STREAM=") == 1)
				{
					iInternetRadioCnt++;
					irInternetRadio = (STREAM_SETT*)DBG_REALLOC(irInternetRadio, sizeof(STREAM_SETT)*iInternetRadioCnt);
					memset(&irInternetRadio[iInternetRadioCnt-1], 0, sizeof(STREAM_SETT));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						irInternetRadio[iInternetRadioCnt-1].Access = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						irInternetRadio[iInternetRadioCnt-1].Type = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64))
						strcpy(irInternetRadio[iInternetRadioCnt-1].Name, Buff4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 256) == 1)
						strcpy(irInternetRadio[iInternetRadioCnt-1].URL, Buff4);									
				}							
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveStreams()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cStreamFile, 1);
	if ((strlen(cStreamFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cStreamFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}
	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iInternetRadioCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "Stream=%i;%i;%s;%s;\n", irInternetRadio[n].Access, irInternetRadio[n].Type,
										irInternetRadio[n].Name, irInternetRadio[n].URL);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 1;
}

int TestStreams(int iMode)
{
	int i;	
	for (i = 0; i < iInternetRadioCnt; i++)
	{
		if ((irInternetRadio[i].Access < 0) || (irInternetRadio[i].Access > MAX_ACCESS_LEVELS))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: STREAM(%i) Access level not between 0 and %i\n", i, MAX_ACCESS_LEVELS);
			else WEB_AddMessageInList("TestSettings: STREAM(%i) Access level not between 0 and %i", i, MAX_ACCESS_LEVELS);
		}
		if ((irInternetRadio[i].Type < 0) || (irInternetRadio[i].Type >= iInternetRadioCnt))
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: STREAM(%i) Type not between 0 and %i\n", i, iInternetRadioCnt - 1);
			else WEB_AddMessageInList("TestSettings: STREAM(%i) Type not between 0 and %i", i, iInternetRadioCnt - 1);
		}
		if (strlen(irInternetRadio[i].Name) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: STREAM(%i) Name is null\n", i);
			else WEB_AddMessageInList("TestSettings: STREAM(%i) Name is null", i);
		}
		if (strlen(irInternetRadio[i].URL) == 0)
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: STREAM(%i) URL is null\n", i);
			else WEB_AddMessageInList("TestSettings: STREAM(%i) URL is null", i);
		}			
	}
	return 1;
}

int LoadDirectories(char *Buff)
{	
	DBG_LOG_IN();	
	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		dbgprintf(1,"Error load settings:%s\n", Buff);
		DBG_LOG_OUT();
		return 0;
	}
	
	char Buff1[1024];
	char Buff2[1024];
	char Buff3[1024];
	char Buff4[256];
	int n, m, len2, len3;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1024, f) != NULL)
	{
		if ((Buff1[0] != 35) && (Buff1[0] > 32))
		{
			memset(Buff2, 0, 1024);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((unsigned char)Buff1[n] > 31) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 1024);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
				if (SearchStrInData(Buff2, len2, 0, "DIRECTORY=") == 1) 
				{
					iDirsCnt++;
					diDirList = (DIRECTORY_INFO*)DBG_REALLOC(diDirList, sizeof(DIRECTORY_INFO)*iDirsCnt);
					memset(&diDirList[iDirsCnt-1], 0, sizeof(DIRECTORY_INFO));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)
						diDirList[iDirsCnt-1].Access = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 10) == 1)
						diDirList[iDirsCnt-1].RemoteAccess = (unsigned int)Str2Int(Buff4);
					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 64))
						strcpy(diDirList[iDirsCnt-1].Name, Buff4);
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, MAX_FILE_LEN) == 1)
					{
						strcpy(diDirList[iDirsCnt-1].Path, Buff4);
						while (diDirList[iDirsCnt-1].Path[strlen(diDirList[iDirsCnt-1].Path)-1] == 47) 
							diDirList[iDirsCnt-1].Path[strlen(diDirList[iDirsCnt-1].Path)-1] = 0;
					} else dbgprintf(2, "LoadSettings: error load DIRECTORY setting\n");
				}						
			}
		}
	}
	fclose(f);
	UpdateDeviceType();
	
	DBG_LOG_OUT();
	return 1;
}

int SaveDirectories()
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&system_mutex);
	
	FILE *f;
	char cPath[MAX_PATH];
	FillConfigPath(cPath, MAX_PATH, cDirectoryFile, 1);
	if ((strlen(cDirectoryFile) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		dbgprintf(1, "Error save:%s\n", cDirectoryFile);		
		DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}
	char Buff1[1024];	
	unsigned int n;
	
	for (n = 0; n < iDirsCnt; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "Directory=%i;%i;%s;%s;\n", diDirList[n].Access, diDirList[n].RemoteAccess, diDirList[n].Name, diDirList[n].Path);
		fputs(Buff1, f);
	}
	fclose(f);
	
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return 1;
}


int TestDirectories(int iMode)
{
	int i;	
	for (i = 0; i < iDirsCnt; i++)
	{
		if ((diDirList[i].Access < 0) || (diDirList[i].Access > MAX_ACCESS_LEVELS))		
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: DIRECTORY(%i) Access level not between 0 and %i\n", i, MAX_ACCESS_LEVELS);
			else WEB_AddMessageInList("TestSettings: DIRECTORY(%i) Access level not between 0 and %i", i, MAX_ACCESS_LEVELS);
		}			
		if (strlen(diDirList[i].Name) == 0)		
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: DIRECTORY(%i) Name is null\n", i);
			else WEB_AddMessageInList("TestSettings: DIRECTORY(%i) Name is null", i);
		}	
		if (strlen(diDirList[i].Path) == 0)		
		{
			if (iMode == 0) dbgprintf(2, "TestSettings: DIRECTORY(%i) Path is null\n", i);
			else WEB_AddMessageInList("TestSettings: DIRECTORY(%i) Path is null", i);
		}			
	}
	return 1;
}

void PrintSettings(void)
{
	unsigned int n, i;
	
	dbgprintf(4,"#####DEVICE#########\n");
	dbgprintf(4,"AudioIN=%s\n", (uiDeviceType & DEVICE_TYPE_AUDIO_IN) ? "Yes" : "No");
	dbgprintf(4,"AudioOUT=%s\n", (uiDeviceType & DEVICE_TYPE_AUDIO_OUT) ? "Yes" : "No");
	dbgprintf(4,"VideoIN=%s\n", (uiDeviceType & DEVICE_TYPE_VIDEO_IN) ? "Yes" : "No");
	dbgprintf(4,"VideoOUT=%s\n", (uiDeviceType & DEVICE_TYPE_VIDEO_OUT) ? "Yes" : "No");
	dbgprintf(4,"UART=%s\n", (uiDeviceType & DEVICE_TYPE_UART) ? "Yes" : "No");
	dbgprintf(4,"RS485=%s\n", (uiDeviceType & DEVICE_TYPE_RS485) ? "Yes" : "No");
	dbgprintf(4,"I2C=%s\n", (uiDeviceType & DEVICE_TYPE_I2C) ? "Yes" : "No");
	dbgprintf(4,"SPI=%s\n", (uiDeviceType & DEVICE_TYPE_SPI) ? "Yes" : "No");
	dbgprintf(4,"GPIO=%s\n", (uiDeviceType & DEVICE_TYPE_GPIO) ? "Yes" : "No");
	dbgprintf(4,"USBIO=%s\n", (uiDeviceType & DEVICE_TYPE_USB_GPIO) ? "Yes" : "No");
	dbgprintf(4,"Radio=%s\n", (uiDeviceType & DEVICE_TYPE_RADIO) ? "Yes" : "No");
	dbgprintf(4,"Temp=%s\n", (uiDeviceType & DEVICE_TYPE_TEMP) ? "Yes" : "No");
	dbgprintf(4,"Clock=%s\n", (uiDeviceType & DEVICE_TYPE_CLOCK) ? "Yes" : "No");
	dbgprintf(4,"PN532=%s\n", (uiDeviceType & DEVICE_TYPE_PN532) ? "Yes" : "No");
	dbgprintf(4,"Zoom=%s\n", (cZoom == 1) ? "Yes" : "No");
	dbgprintf(4,"DateTimeReference=%s\n", (cDateTimeReference == 1) ? "Yes" : "No");	
	dbgprintf(4,"NTPServer=%s\n", cNTPServer);	
	dbgprintf(4,"NewSourcePath=%s\n", cNewSourcePath);
	dbgprintf(4,"NewSourceFile=%s\n", cNewSourceFile);
	dbgprintf(4,"NewSourceLogin=%s\n", cNewSourceLogin);
	dbgprintf(4,"NewSourcePass=%s\n", cNewSourcePass);
	dbgprintf(4,"RebootAfterUpdate=%i\n", cRebootAfterUpdate);
	
	
	dbgprintf(4,"FlvBufferSize=%i\n", uiFlvBufferSize);	
	dbgprintf(4,"AccelerateTextRender=%s\n", (cAccelerateTextRender == 1) ? "Yes" : "No");
	dbgprintf(4,"Random=%s\n", (cSettRandomFile == 1) ? "Yes" : "No");
	dbgprintf(4,"TextColor=%i\n", uiTextColor);
	dbgprintf(4,"ShowMode=%i\n", uiShowMode);	
	dbgprintf(4,"MenuSize=%.1f\n", fMenuSize);
	dbgprintf(4,"BroadCastTCP=%.1f\n", ucBroadCastTCP);	
	
	dbgprintf(4,"BackUpModeFull=%s\n", ucBackUpModeFull ? "Yes" : "No");
	dbgprintf(4,"BackUpModeSlow=%s\n", ucBackUpModeSlow ? "Yes" : "No");
	dbgprintf(4,"BackUpModeDiff=%s\n", ucBackUpModeDiff ? "Yes" : "No");
	dbgprintf(4,"BackUpModeAudio=%s\n", ucBackUpModeAudio ? "Yes" : "No");
	dbgprintf(4,"BackUpModeStatuses=%s\n", ucBackUpModeStatuses ? "Yes" : "No");
	dbgprintf(4,"BackUpModeEvents=%s\n", ucBackUpModeEvents ? "Yes" : "No");
	dbgprintf(4,"BackUpModeActions=%s\n", ucBackUpModeActions ? "Yes" : "No");
	dbgprintf(4,"BackUpModeArchive=%s\n", ucBackUpModeArchive ? "Yes" : "No");
	
	dbgprintf(4,"FullCaptureFileSize=%i\n", uiCaptureFileSizeFull);
	dbgprintf(4,"FullCaptureTimeSize=%i\n", uiCaptureTimeLimitFull);
	dbgprintf(4,"SlowCaptureFileSize=%i\n", uiCaptureFileSizeSlow);
	dbgprintf(4,"SlowCaptureTimeSize=%i\n", uiCaptureTimeLimitSlow);
	dbgprintf(4,"DiffCaptureFileSize=%i\n", uiCaptureFileSizeDiff);
	dbgprintf(4,"DiffCaptureTimeSize=%i\n", uiCaptureTimeLimitDiff);
	dbgprintf(4,"AudioCaptureFileSize=%i\n", uiCaptureFileSizeAudio);
	dbgprintf(4,"AudioCaptureTimeSize=%i\n", uiCaptureTimeLimitAudio);
	dbgprintf(4,"StatusesCaptureFileSize=%i\n", uiCaptureFileSizeStatuses);
	dbgprintf(4,"StatusesCaptureTimeSize=%i\n", uiCaptureTimeLimitStatuses);
	dbgprintf(4,"EventsCaptureFileSize=%i\n", uiCaptureFileSizeEvents);
	dbgprintf(4,"EventsCaptureTimeSize=%i\n", uiCaptureTimeLimitEvents);
	dbgprintf(4,"ActionsCaptureFileSize=%i\n", uiCaptureFileSizeActions);
	dbgprintf(4,"ActionsCaptureTimeSize=%i\n", uiCaptureTimeLimitActions);
	
	dbgprintf(4,"BackUpOrderLmtFull=%i\n", ucBackUpOrderLmtFull);
	dbgprintf(4,"BackUpOrderLmtSlow=%i\n", ucBackUpOrderLmtSlow);
	dbgprintf(4,"BackUpOrderLmtDiff=%i\n", ucBackUpOrderLmtDiff);
	dbgprintf(4,"BackUpOrderLmtAudio=%i\n", ucBackUpOrderLmtAudio);
	dbgprintf(4,"BackUpOrderLmtStatuses=%i\n", ucBackUpOrderLmtStatuses);
	dbgprintf(4,"BackUpOrderLmtEvents=%i\n", ucBackUpOrderLmtEvents);
	dbgprintf(4,"BackUpOrderLmtActions=%i\n", ucBackUpOrderLmtActions);
	dbgprintf(4,"BackUpOrderLmtArchive=%i\n", ucBackUpOrderLmtArchive);

	dbgprintf(4,"BackUpOrderWaitCnclFull=%i\n", uiBackUpOrderWaitCnclFull);
	dbgprintf(4,"BackUpOrderWaitCnclSlow=%i\n", uiBackUpOrderWaitCnclSlow);
	dbgprintf(4,"BackUpOrderWaitCnclDiff=%i\n", uiBackUpOrderWaitCnclDiff);
	dbgprintf(4,"BackUpOrderWaitCnclAudio=%i\n", uiBackUpOrderWaitCnclAudio);
	dbgprintf(4,"BackUpOrderWaitCnclStatuses=%i\n", uiBackUpOrderWaitCnclStatuses);
	dbgprintf(4,"BackUpOrderWaitCnclEvents=%i\n", uiBackUpOrderWaitCnclEvents);
	dbgprintf(4,"BackUpOrderWaitCnclActions=%i\n", uiBackUpOrderWaitCnclActions);
	dbgprintf(4,"BackUpOrderWaitCnclArchive=%i\n", uiBackUpOrderWaitCnclArchive);

	dbgprintf(4,"BackUpOrderWaitMessFull=%i\n", uiBackUpOrderWaitMessFull);
	dbgprintf(4,"BackUpOrderWaitMessSlow=%i\n", uiBackUpOrderWaitMessSlow);
	dbgprintf(4,"BackUpOrderWaitMessDiff=%i\n", uiBackUpOrderWaitMessDiff);
	dbgprintf(4,"BackUpOrderWaitMessAudio=%i\n", uiBackUpOrderWaitMessAudio);
	dbgprintf(4,"BackUpOrderWaitMessStatuses=%i\n", uiBackUpOrderWaitMessStatuses);
	dbgprintf(4,"BackUpOrderWaitMessEvents=%i\n", uiBackUpOrderWaitMessEvents);
	dbgprintf(4,"BackUpOrderWaitMessActions=%i\n", uiBackUpOrderWaitMessActions);
	dbgprintf(4,"BackUpOrderWaitMessArchive=%i\n", uiBackUpOrderWaitMessArchive);
	
	dbgprintf(4,"EventsCaptEnabled=%i\n", ucCaptEnabledEvents);
	dbgprintf(4,"StatusesCaptEnabled=%i\n", ucCaptEnabledStatuses);
	dbgprintf(4,"VideoCaptEnabled=%i\n", ucCaptEnabledVideo);
	dbgprintf(4,"AudioCaptEnabled=%i\n", ucCaptEnabledAudio);
	dbgprintf(4,"ActionsCaptEnabled=%i\n", ucCaptEnabledActions);
	
	dbgprintf(4, "ErrorCameraRestart=%i\n", uiErrorCameraRestart);
	dbgprintf(4, "ErrorCameraRestartWait=%i\n", uiErrorCameraRestartWait);
	dbgprintf(4, "ErrorAudioRestart=%i\n", uiErrorAudioRestart);
	dbgprintf(4, "ErrorAudioRestartWait=%i\n", uiErrorAudioRestartWait);
	dbgprintf(4, "ErrorVideoRestart=%i\n", uiErrorVideoRestart);
	dbgprintf(4, "ErrorVideoRestartWait=%i\n", uiErrorVideoRestartWait);
		
	dbgprintf(4,"BackUpOrderAddrFull=%s\n", ucBackUpOrderAddrFull);
	dbgprintf(4,"BackUpOrderAddrSlow=%s\n", ucBackUpOrderAddrSlow);
	dbgprintf(4,"BackUpOrderAddrDiff=%s\n", ucBackUpOrderAddrDiff);	
	dbgprintf(4,"BackUpOrderAddrAudio=%s\n", ucBackUpOrderAddrAudio);	
	dbgprintf(4,"BackUpOrderAddrStatuses=%s\n", ucBackUpOrderAddrStatuses);
	dbgprintf(4,"BackUpOrderAddrEvents=%s\n", ucBackUpOrderAddrEvents);
	dbgprintf(4,"BackUpOrderAddrActions=%s\n", ucBackUpOrderAddrActions);
	dbgprintf(4,"BackUpOrderAddrArchive=%s\n", ucBackUpOrderAddrArchive);
	
	dbgprintf(4,"CaptureStartPathFull=%s\n", ucCaptureStartPathFull);
	dbgprintf(4,"CaptureStartPathSlow=%s\n", ucCaptureStartPathSlow);
	dbgprintf(4,"CaptureStartPathDiff=%s\n", ucCaptureStartPathDiff);	
	dbgprintf(4,"CaptureStartPathAudio=%s\n", ucCaptureStartPathAudio);	
	dbgprintf(4,"CaptureStartPathStatuses=%s\n", ucCaptureStartPathStatuses);
	dbgprintf(4,"CaptureStartPathEvents=%s\n", ucCaptureStartPathEvents);
	dbgprintf(4,"CaptureStartPathActions=%s\n", ucCaptureStartPathActions);	
	dbgprintf(4,"CaptureStartPathArchive=%s\n", ucCaptureStartPathArchive);	
	
	dbgprintf(4,"CaptureMinFreeSpaceFull=%i\n", uiCaptureMinFreeSpaceFull);
	dbgprintf(4,"CaptureMinFreeSpaceSlow=%i\n", uiCaptureMinFreeSpaceSlow);
	dbgprintf(4,"CaptureMinFreeSpaceDiff=%i\n", uiCaptureMinFreeSpaceDiff);
	dbgprintf(4,"CaptureMinFreeSpaceAudio=%i\n", uiCaptureMinFreeSpaceAudio);
	dbgprintf(4,"CaptureMinFreeSpaceStatuses=%i\n", uiCaptureMinFreeSpaceStatuses);
	dbgprintf(4,"CaptureMinFreeSpaceEvents=%i\n", uiCaptureMinFreeSpaceEvents);
	dbgprintf(4,"CaptureMinFreeSpaceActions=%i\n", uiCaptureMinFreeSpaceActions);
	
	dbgprintf(4,"BackUpMinFreeSpaceFull=%i\n", uiBackUpMinFreeSpaceFull);
	dbgprintf(4,"BackUpMinFreeSpaceSlow=%i\n", uiBackUpMinFreeSpaceSlow);
	dbgprintf(4,"BackUpMinFreeSpaceDiff=%i\n", uiBackUpMinFreeSpaceDiff);
	dbgprintf(4,"BackUpMinFreeSpaceAudio=%i\n", uiBackUpMinFreeSpaceAudio);
	dbgprintf(4,"BackUpMinFreeSpaceStatuses=%i\n", uiBackUpMinFreeSpaceStatuses);
	dbgprintf(4,"BackUpMinFreeSpaceEvents=%i\n", uiBackUpMinFreeSpaceEvents);
	dbgprintf(4,"BackUpMinFreeSpaceActions=%i\n", uiBackUpMinFreeSpaceActions);
	
	dbgprintf(4,"ArchiveMinFreeSpaceFull=%i\n", uiArchiveMinFreeSpaceFull);
	dbgprintf(4,"ArchiveMinFreeSpaceSlow=%i\n", uiArchiveMinFreeSpaceSlow);
	dbgprintf(4,"ArchiveMinFreeSpaceDiff=%i\n", uiArchiveMinFreeSpaceDiff);
	dbgprintf(4,"ArchiveMinFreeSpaceAudio=%i\n", uiArchiveMinFreeSpaceAudio);
	dbgprintf(4,"ArchiveMinFreeSpaceStatuses=%i\n", uiArchiveMinFreeSpaceStatuses);
	dbgprintf(4,"ArchiveMinFreeSpaceEvents=%i\n", uiArchiveMinFreeSpaceEvents);
	dbgprintf(4,"ArchiveMinFreeSpaceActions=%i\n", uiArchiveMinFreeSpaceActions);

	dbgprintf(4,"MediaArchiveTimeFromFull=%i\n", ucMediaArchiveTimeFromFull);
	dbgprintf(4,"MediaArchiveTimeToFull=%i\n", ucMediaArchiveTimeToFull);
	dbgprintf(4,"MediaArchiveTimeFromSlow=%i\n", ucMediaArchiveTimeFromSlow);
	dbgprintf(4,"MediaArchiveTimeToSlow=%i\n", ucMediaArchiveTimeToSlow);
	dbgprintf(4,"MediaArchiveTimeFromDiff=%i\n", ucMediaArchiveTimeFromDiff);
	dbgprintf(4,"MediaArchiveTimeToDiff=%i\n", ucMediaArchiveTimeToDiff);
	dbgprintf(4,"MediaArchiveTimeFromAudio=%i\n", ucMediaArchiveTimeFromAudio);
	dbgprintf(4,"MediaArchiveTimeToAudio=%i\n", ucMediaArchiveTimeToAudio);
	dbgprintf(4,"MediaArchiveTimeFromStatuses=%i\n", ucMediaArchiveTimeFromStatuses);
	dbgprintf(4,"MediaArchiveTimeToStatuses=%i\n", ucMediaArchiveTimeToStatuses);
	dbgprintf(4,"MediaArchiveTimeFromEvents=%i\n", ucMediaArchiveTimeFromEvents);
	dbgprintf(4,"MediaArchiveTimeToEvents=%i\n", ucMediaArchiveTimeToEvents);
	dbgprintf(4,"MediaArchiveTimeFromActions=%i\n", ucMediaArchiveTimeFromActions);
	dbgprintf(4,"MediaArchiveTimeToActions=%i\n", ucMediaArchiveTimeToActions);
	
	dbgprintf(4,"CaptureFilesView=%i\n", cCaptureFilesView);
	dbgprintf(4,"BackUpFilesView=%i\n", cBackUpFilesView);
	dbgprintf(4,"CaptureFilesLevel=%i\n", cCaptureFilesLevel);
	dbgprintf(4,"BackUpFilesLevel=%i\n", cBackUpFilesLevel);
	dbgprintf(4,"CaptureFilesViewDef=%i\n", cCaptureFilesViewDef);
	dbgprintf(4,"LogIP=%s\n", cLogIP);
	dbgprintf(4,"LogEmailAddr=%s\n", cLogMlList.Address);	
	dbgprintf(4,"FileWriterService=%i\n",cFileWriterService);	
	dbgprintf(4,"WriterServicePath=%s\n",cWriterServicePath);	
	
	dbgprintf(4,"BackUpOrderEnableFull=%s\n", ucBackUpOrderEnableFull ? "Yes" : "No");	
	dbgprintf(4,"BackUpOrderEnableSlow=%s\n", ucBackUpOrderEnableSlow ? "Yes" : "No");	
	dbgprintf(4,"BackUpOrderEnableDiff=%s\n", ucBackUpOrderEnableDiff ? "Yes" : "No");	
	dbgprintf(4,"BackUpOrderEnableAudio=%s\n", ucBackUpOrderEnableAudio ? "Yes" : "No");	
	dbgprintf(4,"BackUpOrderEnableStatuses=%s\n", ucBackUpOrderEnableStatuses ? "Yes" : "No");	
	dbgprintf(4,"BackUpOrderEnableEvents=%s\n", ucBackUpOrderEnableEvents ? "Yes" : "No");	
	dbgprintf(4,"BackUpOrderEnableActions=%s\n", ucBackUpOrderEnableActions ? "Yes" : "No");
	dbgprintf(4,"BackUpOrderEnableArchive=%s\n", ucBackUpOrderEnableArchive ? "Yes" : "No");
	
	dbgprintf(4,"MediaCapturePathFull=%s\n",cMediaCapturePathFull);	
	dbgprintf(4,"MediaCapturePathSlow=%s\n",cMediaCapturePathSlow);	
	dbgprintf(4,"MediaCapturePathDiff=%s\n",cMediaCapturePathDiff);	
	dbgprintf(4,"MediaCapturePathAudio=%s\n",cMediaCapturePathAudio);	
	dbgprintf(4,"MediaCapturePathStatuses=%s\n",cMediaCapturePathStatuses);	
	dbgprintf(4,"MediaCapturePathEvents=%s\n",cMediaCapturePathEvents);	
	dbgprintf(4,"MediaCapturePathActions=%s\n",cMediaCapturePathActions);	
	
	dbgprintf(4,"MediaBackUpPathFull=%s\n",cMediaBackUpPathFull);	
	dbgprintf(4,"MediaBackUpPathSlow=%s\n",cMediaBackUpPathSlow);	
	dbgprintf(4,"MediaBackUpPathDiff=%s\n",cMediaBackUpPathDiff);	
	dbgprintf(4,"MediaBackUpPathAudio=%s\n",cMediaBackUpPathAudio);	
	dbgprintf(4,"MediaBackUpPathStatuses=%s\n",cMediaBackUpPathStatuses);	
	dbgprintf(4,"MediaBackUpPathEvents=%s\n",cMediaBackUpPathEvents);	
	dbgprintf(4,"MediaBackUpPathActions=%s\n",cMediaBackUpPathActions);	
	
	dbgprintf(4,"MediaArchivePathFull=%s\n",cMediaArchivePathFull);	
	dbgprintf(4,"MediaArchivePathSlow=%s\n",cMediaArchivePathSlow);	
	dbgprintf(4,"MediaArchivePathDiff=%s\n",cMediaArchivePathDiff);	
	dbgprintf(4,"MediaArchivePathAudio=%s\n",cMediaArchivePathAudio);	
	dbgprintf(4,"MediaArchivePathStatuses=%s\n",cMediaArchivePathStatuses);	
	dbgprintf(4,"MediaArchivePathEvents=%s\n",cMediaArchivePathEvents);	
	dbgprintf(4,"MediaArchivePathActions=%s\n",cMediaArchivePathActions);	
	
	dbgprintf(4,"LocalBackUpNormPath=%s\n",cLocalBackUpPathFull);	
	dbgprintf(4,"LocalBackUpSlowPath=%s\n",cLocalBackUpPathSlow);	
	dbgprintf(4,"LocalBackUpDiffPath=%s\n",cLocalBackUpPathDiff);	
	dbgprintf(4,"LocalBackUpAudioPath=%s\n",cLocalBackUpPathAudio);	
	dbgprintf(4,"LocalBackUpStatusesPath=%s\n",cLocalBackUpPathStatuses);	
	dbgprintf(4,"LocalBackUpEventsPath=%s\n",cLocalBackUpPathEvents);	
	dbgprintf(4,"LocalBackUpActionsPath=%s\n",cLocalBackUpPathActions);	
	
	dbgprintf(4,"LocalArchiveNormPath=%s\n",cLocalArchivePathFull);	
	dbgprintf(4,"LocalArchiveSlowPath=%s\n",cLocalArchivePathSlow);	
	dbgprintf(4,"LocalArchiveDiffPath=%s\n",cLocalArchivePathDiff);	
	dbgprintf(4,"LocalArchiveAudioPath=%s\n",cLocalArchivePathAudio);	
	dbgprintf(4,"LocalArchiveStatusesPath=%s\n",cLocalArchivePathStatuses);	
	dbgprintf(4,"LocalArchiveEventsPath=%s\n",cLocalArchivePathEvents);	
	dbgprintf(4,"LocalArchiveActionsPath=%s\n",cLocalArchivePathActions);	
	
	dbgprintf(4,"MediaArchiveModeFull=%i\n",ucMediaArchiveModeFull);	
	dbgprintf(4,"MediaArchiveModeSlow=%i\n",ucMediaArchiveModeSlow);	
	dbgprintf(4,"MediaArchiveModeDiff=%i\n",ucMediaArchiveModeDiff);	
	dbgprintf(4,"MediaArchiveModeAudio=%i\n",ucMediaArchiveModeAudio);	
	dbgprintf(4,"MediaArchiveModeStatuses=%i\n",ucMediaArchiveModeStatuses);	
	dbgprintf(4,"MediaArchiveModeEvents=%i\n",ucMediaArchiveModeEvents);	
	dbgprintf(4,"MediaArchiveModeActions=%i\n",ucMediaArchiveModeActions);	
	
	dbgprintf(4,"CameraShotFile=%s\n",cCameraShotFile);	
	dbgprintf(4,"CameraSensorFile=%s\n",cCameraSensorFile);	
	dbgprintf(4,"CameraRectFile=%s\n",cCameraRectFile);	
	dbgprintf(4,"MediaIoTimeout=%i\n",uiMediaIoTimeout);	
	dbgprintf(4,"#####WEB SERVER#########\n");
	dbgprintf(4,"WEBServer=%s\n", (uiWEBServer == 1) ? "Yes" : "No");
	dbgprintf(4,"RTSPForceAudio=%s\n", (uiRTSPForceAudio == 1) ? "Yes" : "No");
	dbgprintf(4,"WEBAuth=%i\n", WebAuth);	
	dbgprintf(4,"WEBPort=%i\n", uiWebPort);
	dbgprintf(4,"WebMaxTimeIdle=%i\n", WebMaxTimeIdle);	
	dbgprintf(4,"#####TIME#########\n");
	dbgprintf(4,"PaddingSize=%i\n",uiPaddingSize);
	dbgprintf(4,"AlarmFile=%s\n",cAlarmFile);	
	dbgprintf(4,"AlarmPath=%s\n",cFileAlarmLocation);		
	dbgprintf(4,"#####WEATHER#########\n");
	dbgprintf(4,"TimeZone=%i\n",iTimeCor);
	dbgprintf(4,"#####DEBUG#########\n");
	dbgprintf(4,"FileLog=%s\n",iFileLog ? "Yes" : "No");
	dbgprintf(4,"ScreenLog=%s\n",iScreenLog ? "Yes" : "No");
	dbgprintf(4,"MessageLog=%s\n",iMessageLog ? "Yes" : "No");
	dbgprintf(4,"LocalMessLog=%s\n",iLocalMessageLog ? "Yes" : "No");	
	dbgprintf(4,"EmailLog=%s\n",iEmailLog ? "Yes" : "No");	
	dbgprintf(4,"EmailLogPause=%i\n",uiLogEmailPauseSize);		
	dbgprintf(4,"SoundMessagePause=%i\n",uiSoundMessagePauseSize);		
	dbgprintf(4,"LogPath=%s\n",cFileLogName);
	dbgprintf(4,"DefAlarmSound=%.4s\n",(char*)&uiDefAlarmSound);	
	dbgprintf(4,"SoundMessageID=%.4s\n",(char*)&uiSoundMessageID);	
	dbgprintf(4,"DefAlarmRepeats=%i\n",uiDefAlarmRepeats);	
	dbgprintf(4,"#####SHOW#########\n");
	dbgprintf(4,"VSync=%s\n", VSync ? "Yes" : "No");
	dbgprintf(4,"ShowsPath=%s\n",cFileLocation);
	dbgprintf(4,"SlideShowTimer=%i\n",iSlideShowTimerSet);
	dbgprintf(4,"SlideShowOnTime=%i\n",iSlideShowOnTime);
	dbgprintf(4,"SlideShowOffTime=%i\n",iSlideShowOffTime);
	dbgprintf(4,"ShowerLiveCtrlTime=%i\n",uiShowerLiveCtrlTime);
	dbgprintf(4,"#####RADIO/MUSIC#########\n");
	dbgprintf(4,"RadioVolume=%i\n",iRadioVolume);	
	dbgprintf(4,"BasicVolume=%i\n",iBasicVolume);	
	dbgprintf(4,"AlarmVolume=%i\n",iAlarmVolume);
	for (n = 0; n != iRadioStationsCnt; n++) dbgprintf(4,"RadioStation%i=%.1f;%s\n",n,riRadioStation[n].Frequency,riRadioStation[n].Name);
	dbgprintf(4,"#####RTSP#########\n");
	dbgprintf(4,"RTSP=%s\n", (uiRTSPStream == 1) ? "Yes" : "No");	
	dbgprintf(4,"RTSPAuth=%i\n", RtspAuth);	
	dbgprintf(4,"RTSPPort=%i\n", uiRTSPPort);	
	dbgprintf(4,"RTPClntVidPort=%i\n", uiRTPClntVidPort);	
	dbgprintf(4,"RTPClntAudioPort=%i\n", uiRTPClntAudPort);
	dbgprintf(4,"RTPServVidPort=%i\n", uiRTPServVidPort);	
	dbgprintf(4,"RTPServAudPort=%i\n", uiRTPServAudPort);	
	dbgprintf(4,"#####ONVIF#########\n");
	dbgprintf(4,"ONVIF=%s\n", (uiOnvifStream == 1) ? "Yes" : "No");	
	dbgprintf(4,"ONVIFAuth=%i\n", OnvifAuth);	
	dbgprintf(4,"ONVIFPort=%i\n", uiOnvifPort);	
	dbgprintf(4,"#####MAIL#########\n");
	dbgprintf(4,"MailAddress=%s\n", cMailAddress);	
	dbgprintf(4,"MailServer=%s\n", cMailServer);	
	dbgprintf(4,"MailLogin=%s\n", cMailLogin);	
	dbgprintf(4,"MailPassword=%s\n", cMailPassword);
	dbgprintf(4,"MailAuth=%s\n", cMailAuth);
		
	dbgprintf(4,"\n");
	
	dbgprintf(4,"SystemName=%s\n",miSystemList[0].Name);
	dbgprintf(4,"SystemID=%.4s\n",(char*)&miSystemList[0].ID);
	
	dbgprintf(4,"#####IR COMMANDS#########\n");
	for (n = 0; n < iIRCommandCnt; n++) 
	{
	 dbgprintf(4,"IRCommand%i=%.4s; len:%i\n",n,(char*)&mIRCommandList[n].ID, mIRCommandList[n].Len);
	}
	dbgprintf(4,"#####ALARMCLOCK#########\n");
	for (n = 0; n < iAlarmClocksCnt; n++) 
	{
		dbgprintf(4,"AlarmClock%i=%i;%i;%s%s%s%s%s%s%s;%i;\n",n,actAlarmClockInfo[n].Enabled, actAlarmClockInfo[n].Time,
		(actAlarmClockInfo[n].Days & 1) ? "[Mo]" : "",
		(actAlarmClockInfo[n].Days & 2) ? "[Tu]" : "",
		(actAlarmClockInfo[n].Days & 4) ? "[We]" : "",
		(actAlarmClockInfo[n].Days & 8) ? "[Th]" : "",
		(actAlarmClockInfo[n].Days & 16) ? "[Fr]" : "",
		(actAlarmClockInfo[n].Days & 32) ? "[Sa]" : "",
		(actAlarmClockInfo[n].Days & 64) ? "[Su]" : "",
		actAlarmClockInfo[n].Skip);
	}
	dbgprintf(4,"#####SECURITY#########\n");
	dbgprintf(4,"AccessLevel=%i\n",iDefAccessLevel);
	dbgprintf(4,"IntervalRescanCard=%i\n",iIntervalRescanCard);
	for (n = 0; n < iSecurityKeyCnt; n++) 
	{
	 dbgprintf(4,"SecurityKey%i=ID:%.4s, Serial:%i.%i.%i.%i; Sector:%i; VerifyCode:%i; VerifyValue:%i;\n",n,
											(char*)&skiSecurityKeys[n].ID,skiSecurityKeys[n].Serial[0],skiSecurityKeys[n].Serial[1]
												,skiSecurityKeys[n].Serial[2],skiSecurityKeys[n].Serial[3],
												skiSecurityKeys[n].Sector, skiSecurityKeys[n].VerifyKeyA, skiSecurityKeys[n].VerifyKeyB);
	 dbgprintf(4,"	KeyA:%i.%i.%i.%i.%i.%i;\n",skiSecurityKeys[n].KeyA[0],skiSecurityKeys[n].KeyA[1],skiSecurityKeys[n].KeyA[2]
														,skiSecurityKeys[n].KeyA[3],skiSecurityKeys[n].KeyA[4],skiSecurityKeys[n].KeyA[5]);
	 dbgprintf(4,"	KeyB:%i.%i.%i.%i.%i.%i;\n",skiSecurityKeys[n].KeyB[0],skiSecurityKeys[n].KeyB[1],skiSecurityKeys[n].KeyB[2]
														,skiSecurityKeys[n].KeyB[3],skiSecurityKeys[n].KeyB[4],skiSecurityKeys[n].KeyB[5]);	 
	}
	dbgprintf(4,"#####ACTIONS#########\n");
	for (n = 0; n < iActionCnt; n++) 
	{
		dbgprintf(4,"Action%i=ActionID:%.4s, Enabled:%s\n", n, (char*)&maActionInfo[n].ActionID, maActionInfo[n].Enabled ? "Yes" : "No");
		dbgprintf(4,"\tSource:ID:%.4s, SubNum:%i, SrcStatus:%i(%.4s)(%c)\n", (char*)&maActionInfo[n].SrcID, maActionInfo[n].SrcSubNumber, maActionInfo[n].SrcStatus, (char*)&maActionInfo[n].SrcStatus, maActionInfo[n].TestType);
		dbgprintf(4,"\tDest:ID:%.4s, SubNum:%i, DestStatus:%i(%.4s)\n", (char*)&maActionInfo[n].DestID, maActionInfo[n].DestSubNumber, maActionInfo[n].DestStatus, (char*)&maActionInfo[n].DestStatus);	
		dbgprintf(4,"\tAfterAccept:%i TimerWaitBefore:%i TimerAfterOff:%i\n", maActionInfo[n].AfterAccept, maActionInfo[n].TimerWaitBefore, maActionInfo[n].TimerOff);			
		dbgprintf(4,"\tName:%s\n", maActionInfo[n].Name);			
		dbgprintf(4,"\tWeekDays:%i, TimeFrom:%i, TimeTo:%i\n", maActionInfo[n].WeekDays, maActionInfo[n].NotBeforeTime, maActionInfo[n].NotAfterTime);			
	}	
	dbgprintf(4,"#####WIDGETS#########\n");
	for (n = 0; n < iWidgetsCnt; n++) 
	{
		dbgprintf(4,"Widget%i=WidgetID:%.4s, Enabled:%i ShowMode:%i\n", n, (char*)&wiWidgetList[n].WidgetID, wiWidgetList[n].Enabled, wiWidgetList[n].ShowMode);
		dbgprintf(4,"\tType:%i, Name:%s\n", wiWidgetList[n].Type, wiWidgetList[n].Name);	
		dbgprintf(4,"\tScale:%g, Direct:%i, Speed:%g, Setting:%i\n", 
					wiWidgetList[n].Scale, wiWidgetList[n].Direct, wiWidgetList[n].Speed, wiWidgetList[n].Settings[0]);
		dbgprintf(4,"\tRefresh:%i, WeekDays:%i\n", 
					wiWidgetList[n].Refresh, wiWidgetList[n].WeekDays); 
		dbgprintf(4,"\tNotBeforeTime:%i, NotAfterTime:%i, NotBeforeTime2:%i, NotAfterTime2:%i, NotBeforeTime3:%i, NotAfterTime3:%i\n", 
					wiWidgetList[n].NotBeforeTime, wiWidgetList[n].NotAfterTime,
					wiWidgetList[n].NotBeforeTime2, wiWidgetList[n].NotAfterTime2,
					wiWidgetList[n].NotBeforeTime3, wiWidgetList[n].NotAfterTime3);
		dbgprintf(4,"\tPath:%s\n", wiWidgetList[n].Path);			
	}
	dbgprintf(4,"#####MODULES#########\n");
	dbgprintf(4,"ModuleFile=%s\n",cModuleFile);		
	for (n = 0; n != iModuleCnt; n++) 
	{
		dbgprintf(4,"Module%i=Status:%i;ID:%.4s;Type:%i;Name:%s;\n", n, miModuleList[n].Enabled,(char*)&miModuleList[n].ID, miModuleList[n].Type, miModuleList[n].Name);
		dbgprintf(4,"\tView:%i;Access:%i;Global:%i;ScanSet:%i;\n",miModuleList[n].ViewLevel,miModuleList[n].AccessLevel,
																	miModuleList[n].Global,miModuleList[n].ScanSet);
		char cBuff[1024];
		char cBuff2[256];
		memset(cBuff, 0, 1024);
		strcpy(cBuff ,"\t");
			
		for (i = 0; i < MAX_MODULE_SETTINGS; i++) 
		{
			memset(cBuff2, 0, 256);
			sprintf(cBuff2 ,"(%i)",miModuleList[n].Settings[i]);
			strcat(cBuff, cBuff2);
		}
		dbgprintf(4,"\tSetts:%s;\n",cBuff);
	}
	dbgprintf(4,"#####SOUNDS#########\n");
	
	for (n = 0; n < iSoundListCnt; n++) 
	{
		dbgprintf(4,"Sound%i=ID:%.4s;Path:%s\n",
				n, (char*)&mSoundList[n].ID, mSoundList[n].Path);
	}
	dbgprintf(4,"#####DIRECTORYES#########\n");
	for (n = 0; n != iDirsCnt; n++) 
	{
		dbgprintf(4,"Directory%i=Access:%i Path:%s\n", n, diDirList[n].Access, diDirList[n].Path);
	}
	
	dbgprintf(4,"#####STREAM TYPES#########\n");
	for (n = 0; n != iStreamTypeCnt; n++) 
	{
		dbgprintf(4,"StreamType%i=Name:'%s'\n", n, stStreamType[n].Name);
	}
	dbgprintf(4,"#####INTERNET STREAMS#########\n");
	for (n = 0; n < iInternetRadioCnt; n++) 
	{
		dbgprintf(4,"Stream%i=Access:%i Type: %i Name:'%s' URL:'%s'\n", n, irInternetRadio[n].Access, irInternetRadio[n].Type, irInternetRadio[n].Name, irInternetRadio[n].URL);
	}
	
	dbgprintf(4,"#####MENU ACTIONS#########\n");
	for (n = 0; n != iActionManualCnt; n++) 
	{
		dbgprintf(4,"ActionMenu%i=Access:%i ID:%.4s Sub:%i Status:%i Name:'%s'\n", n, 
			amiActionManual[n].Access, (char*)&amiActionManual[n].ID, amiActionManual[n].SubNumber, amiActionManual[n].Status, amiActionManual[n].Name);
	}
	
	dbgprintf(4,"#####CAMERA RECTANGLES#########\n");
	for (n = 0; n != iRectangleCnt; n++) 
	{
		dbgprintf(4,"CamRect%i=Enabled:%i Group:%i X1:%i Y1:%i X2:%i Y2:%i\n", n, 
			riRectangleList[n].Enabled, riRectangleList[n].Group, 
			riRectangleList[n].X1, riRectangleList[n].Y1, 
			riRectangleList[n].X2, riRectangleList[n].Y2);
	}
	
	dbgprintf(4,"#####MAILES#########\n");
	for (n = 0; n < iMailCnt; n++) 
	{
		dbgprintf(4,"Mail%i=Group:%i Address:'%s' Address2:'%s'\n\tMainText:%s\tBodyText:%s\n\tFilePath:%s\tFilePath2:%s\n", n, 
			miMailList[n].Group, miMailList[n].Address, miMailList[n].Address2, miMailList[n].MainText, miMailList[n].BodyText, miMailList[n].FilePath, miMailList[n].FilePath2);
	}
	
	dbgprintf(4,"\n");
}

void Init_Ortho(GL_STATE_T *state)
{
	DBG_LOG_IN();
	glDisable(GL_BLEND);
	glClearColor(0.0f,0.0f,0.0f,0.5f); //initialising clear colour 

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrthof(0,(GLsizei)state->screen_width-1,0, (GLsizei)state->screen_height-1,-1,1);    
   
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DBG_LOG_OUT();
}

static void Init_GL(GL_STATE_T *state)
{
   DBG_LOG_IN();
   
   int32_t success = 0;
   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };

   EGLConfig config;
   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);
   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);
   // get an appropriate EGL frame buffer configuration
   //result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   result = eglSaneChooseConfigBRCM(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);
   // create an EGL rendering context 
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
   assert(state->context!=EGL_NO_CONTEXT);
   
   // create an EGL window surface
   success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
   assert( success >= 0 );

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = state->screen_width;
   dst_rect.height = state->screen_height;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = state->screen_width << 16;
   src_rect.height = state->screen_height << 16;

   state->dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   state->dispman_update = vc_dispmanx_update_start( 0 );
   state->dispman_element = vc_dispmanx_element_add (state->dispman_update, state->dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

   nativewindow.element = state->dispman_element;
   nativewindow.width = state->screen_width;
   nativewindow.height = state->screen_height;
   vc_dispmanx_update_submit_sync(state->dispman_update );

   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);

   //result = eglSwapInterval(state->display, 1);
   //assert(EGL_FALSE != result);
   
   // Set background color and clear buffers
   //glClearColor(0.15f, 0.25f, 0.35f, 1.0f);

   // Enable back face culling.
   glEnable(GL_CULL_FACE);
   glColorMask(1.0f,1.0f,1.0f,0.0f);

  // glMatrixMode(GL_MODELVIEW);
  DBG_LOG_OUT();
}

static void DeInit_GL(GL_STATE_T *state)
{
	DBG_LOG_IN();
	int ret;
	state->dispman_update = vc_dispmanx_update_start( 10 );
    assert(state->dispman_update);
    ret = vc_dispmanx_element_remove(state->dispman_update, state->dispman_element);
    assert( ret == 0 );
    ret = vc_dispmanx_update_submit_sync(state->dispman_update);
    assert( ret == 0 );
    ret = vc_dispmanx_display_close(state->dispman_update);
    assert( ret == 0 );
	
	DBG_LOG_OUT();
}

void RenderWidgets(GL_STATE_T *pstate, int iSlideShowRenderStatus)
{
	int n;
	int iMenuSh = GetShowMenu();
	DBG_MUTEX_LOCK(&widget_mutex);
	for (n = 0; n < iWidgetsCnt; n++)
	{
		if (wiWidgetList[n].Enabled && (wiWidgetList[n].Status == 1))
		{
			if (wiWidgetList[n].ShowMode != 0) 
			{
				AnimateWidget(pstate, &wiWidgetList[n]);	
				/*if (n == 1) printf("Widget mode %i\t %g\t%g\t%g\t%g\t%g\n",wiWidgetList[n].AnimateMode, wiWidgetList[n].PosX, wiWidgetList[n].PosY,
					((double)(pstate->screen_width) - (double)(uiPaddingSize) - (wiWidgetList[n].SizeX * wiWidgetList[n].Scale)), 
					wiWidgetList[n].SizeX, 
					wiWidgetList[n].SizeX * wiWidgetList[n].Scale);*/
			}
			if (((wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_DAYTIME) && wiWidgetList[n].InTimeLimit)
				|| 
				((wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_MENU) && iMenuSh)
				||
				(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_ALLWAYS)
				||
				((wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_TIMEOUT) && wiWidgetList[n].ShowTimer && !wiWidgetList[n].ShowDirect))
			{
				switch(wiWidgetList[n].Type)
				{
					case WIDGET_TYPE_SENSOR_IMAGE:
					case WIDGET_TYPE_SENSOR_TACHOMETER:
					case WIDGET_TYPE_SENSOR_TACHO_SCALE:
					case WIDGET_TYPE_SENSOR_TEMPMETER:
					case WIDGET_TYPE_SENSOR_WHITETACHO:
					case WIDGET_TYPE_SENSOR_BLACKTACHO:
					case WIDGET_TYPE_SENSOR_BLACKREGULATOR:
					case WIDGET_TYPE_SENSOR_CIRCLETACHO:
					case WIDGET_TYPE_SENSOR_DARKMETER:
					case WIDGET_TYPE_SENSOR_DARKTERMOMETER:
					case WIDGET_TYPE_SENSOR_GREENTACHO:
					case WIDGET_TYPE_SENSOR_OFFTACHO:
					case WIDGET_TYPE_SENSOR_SILVERREGULATOR:
					case WIDGET_TYPE_SENSOR_WHITETERMOMETER:
							if ((wiWidgetList[n].SensorValue >= wiWidgetList[n].ShowValueFrom) && (wiWidgetList[n].SensorValue <= wiWidgetList[n].ShowValueTo))
							RenderWidget(&wiWidgetList[n]);
						break;
					case WIDGET_TYPE_IMAGE:
					case WIDGET_TYPE_CLOCK_EL:
					case WIDGET_TYPE_CLOCK_MECH:
					case WIDGET_TYPE_CLOCK_WHITE:
					case WIDGET_TYPE_CLOCK_BROWN:
					case WIDGET_TYPE_CLOCK_QUARTZ:
					case WIDGET_TYPE_CLOCK_SKYBLUE:
					case WIDGET_TYPE_CLOCK_ARROW:
						RenderWidget(&wiWidgetList[n]);
						break;
					case WIDGET_TYPE_WEATHER:
						//RenderGLStaticWeather(1);
						RenderGLWeather(wiWidgetList[n].Scale, wiWidgetList[n].PosX, wiWidgetList[n].PosY, wiWidgetList[n].Settings[0]);
						//printf("RenderGLWeather %i, %g, %g, %g, %i\n",n, wiWidgetList[n].Scale, wiWidgetList[n].PosX, wiWidgetList[n].PosY, wiWidgetList[n].Settings[0]);
						break;
					default:
						break;
				}
			}
		}
	}
	DBG_MUTEX_UNLOCK(&widget_mutex);	
}

void RenderStart()
{
	glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);      
}

static void Rendering(GL_STATE_T *pstate, int iSlideShowRenderStatus, int cvShowType, int iSync, CAMERA_LIST_INFO * clCamList, int iCameraListCnt)
{	
	DBG_LOG_IN();
	
    glLoadIdentity();   
    
	int ret = 100;
	int n;
	if (iFade != 255)
	{
		if (iSlideShowRenderStatus == 1)
		{
			if (!(cvShowType & SHOW_TYPE_CAMERA_LIST))
			{
				if ((pstate->screen_width_center != 0) || (pstate->screen_height_center != 0)) 
					glTranslatef(pstate->screen_width_center, pstate->screen_height_center, 0.0f);
				
				ret = 0;
				if ((iSync == 1) && (cvShowType & SHOW_TYPE_VIDEO) && (omx_IsFree_Video() == 0))
				{
					ret = 1;
					tx_semaphore_add_in_list(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY);
					n = tx_semaphore_wait_event_timeout(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY, 100);
					if (n)
					{
						//printf("wait render %i ms\n", 1000 - ret);
					} 
					else 
					{
						tx_semaphore_go(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY);
						dbgprintf(4,"timeout video framerate VSync\n");
					}
				}
				glVertexPointer(3, GL_FLOAT, 0, pstate->fScreenVertCoords); //point to vert array 
				glEnableClientState(GL_VERTEX_ARRAY); //enable vert array 		
				glTexCoordPointer(2, GL_FLOAT, 0, pstate->fScreenTextCoords); 
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);        
				glBindTexture(GL_TEXTURE_2D, pstate->tex[0]);		
				glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
				if (ret == 1) tx_semaphore_go(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);	
			}
			else
			{				
				glVertexPointer(3, GL_FLOAT, 0, pstate->fScreenVertCoords); //point to vert array 
				glEnableClientState(GL_VERTEX_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, pstate->fScreenTextCoords); 
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);        
				for(n = 0; n < iCameraListCnt; n++)
				{
					glTranslatef(clCamList[n].PosW, clCamList[n].PosH, 0.0f);				
					if ((clCamList[n].Status) && (clCamList[n].ID != 0))
					{
						glBindTexture(GL_TEXTURE_2D, clCamList[n].Texture);		
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	
					}
					glTranslatef(-clCamList[n].PosW, -clCamList[n].PosH, 0.0f);
				}
			}
		}
	}
	
	if (iFade != 0)
	{		
		glLoadIdentity(); 
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);	
		glVertexPointer(3, GL_FLOAT, 0, pstate->fAlphaVertCoords);
		glEnableClientState(GL_VERTEX_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, pstate->fAlphaTextCoords); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, pstate->tex[1]);
		glDrawArrays(GL_TRIANGLE_STRIP, iFade*4, 4);
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);		
	}
		
	DBG_LOG_OUT();
}

void RenderGLGraph(GL_STATE_T *pstate, GLfloat gX, GLfloat gY, GLfloat gScX, GLfloat gScY)
{
	glLoadIdentity(); 
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glVertexPointer(3, GL_FLOAT, 0, pstate->fGraphVertCoords);
	glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, pstate->fGraphTextCoords); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, pstate->tex[2]);
	
	glScalef(gScX,gScY,0.0f);
	glTranslatef(gX / gScX, gY / gScY, 0.0f);
				
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}

void RenderDisplayContent(GL_STATE_T *pstate, char cStatus)
{
	int ret = 0;
	DBG_MUTEX_LOCK(&dyspcont_mutex);
	int iCnt;
	DISPLAY_CONTENT_INFO *dci;
	
	if (iDisplayContentCurrent)
	{
		iCnt = iDisplayContentCnt1;
		dci = dciDisplayContent1;
	}
	else
	{
		iCnt = iDisplayContentCnt2;
		dci = dciDisplayContent2;
	}
	if (iCnt != 0) 
	{
		int n;
		for (n = 0; n < iCnt; n++)
		{
			if (dci[n].Type == 2) 
				RenderGLText(dci[n].Size, dci[n].PosX, dci[n].PosY, dci[n].Text);
			if ((dci[n].Type == 3) && cStatus)
				RenderGLText(dci[n].Size, dci[n].PosX, dci[n].PosY, dci[n].Text);
			if (dci[n].Type == 4) 
				RenderGLGraph(pstate, dci[n].PosX, dci[n].PosY, dci[n].ScaleX, dci[n].ScaleY);
		}
		iDisplayContentLiveTime++;
		ret = iDisplayContentLiveTime;
	}
	DBG_MUTEX_UNLOCK(&dyspcont_mutex);	
	if (ret > 25) 
	{
		DisplayContentListClear();
		DisplayContentListSwitch();		
	}
}

void RenderText(GL_STATE_T *pstate, int iTextSize, GLfloat gX, GLfloat gY, char cStatus, char *cText, ...)
{
	DBG_LOG_IN();
	char textbuffer[256];
	memset(textbuffer, 0 , 256);
	va_list valist;
	va_start(valist, cText);		
	vsprintf(textbuffer, cText, valist);		
	va_end(valist);
	
	
	RenderGLText(iTextSize, gX, gY, textbuffer);
	if ((pstate->menu_skip_timer == 0) && (pstate->menu_terminal_id))
	{				
		if (pstate->menu_terminal_enabled > 0)
		{
			DISPLAY_CONTENT_INFO dci;
			if (pstate->menu_render_count == 0)
			{
				dci.Type = 0;
				if (!SendTCPMessage(TYPE_MESSAGE_DISPLAY_CONTENT, (char*)&dci, sizeof(DISPLAY_CONTENT_INFO), NULL, 0, &pstate->menu_terminal_addr))
					pstate->menu_terminal_enabled = -100;
			}
			if (pstate->menu_terminal_enabled > 0)
			{
				if (!cStatus) dci.Type = 2;	//TEXT
					else dci.Type = 3; // FLASH TEXT
				dci.Size = iTextSize;
				dci.PosX = gX;
				dci.PosY = gY;
				dci.ScaleX = 0.0;
				dci.ScaleY = 0.0;
				memcpy(dci.Text, textbuffer, 63);
				dci.Text[63] = 0;
				if (!SendTCPMessage(TYPE_MESSAGE_DISPLAY_CONTENT, (char*)&dci, sizeof(DISPLAY_CONTENT_INFO), NULL, 0, &pstate->menu_terminal_addr))
					pstate->menu_terminal_enabled = -100;
			}
		}
	}
	pstate->menu_render_count++;
	DBG_LOG_OUT();
}

void RenderGraph(GL_STATE_T *pstate, GLfloat gX, GLfloat gY, GLfloat gScX, GLfloat gScY)
{
	DBG_LOG_IN();
	RenderGLGraph(pstate, gX, gY, gScX, gScY);
	if ((pstate->menu_skip_timer == 0) && (pstate->menu_terminal_id))
	{
		if (pstate->menu_terminal_enabled > 0)
		{
			DISPLAY_CONTENT_INFO dci;
			if (pstate->menu_render_count == 0)
			{
				dci.Type = 0;
				if (!SendTCPMessage(TYPE_MESSAGE_DISPLAY_CONTENT, (char*)&dci, sizeof(DISPLAY_CONTENT_INFO), NULL, 0, &pstate->menu_terminal_addr))
					pstate->menu_terminal_enabled = -100;
			}
			if (pstate->menu_terminal_enabled > 0)
			{
				dci.Type = 4; //GRAPH
				dci.Size = 0;
				dci.PosX = gX;
				dci.PosY = gY;
				dci.ScaleX = gScX;
				dci.ScaleY = gScY;
				if (!SendTCPMessage(TYPE_MESSAGE_DISPLAY_CONTENT, (char*)&dci, sizeof(DISPLAY_CONTENT_INFO), NULL, 0, &pstate->menu_terminal_addr))
					pstate->menu_terminal_enabled = -100;
			}
		}
	}
	pstate->menu_render_count++;
	DBG_LOG_OUT();
}

void RenderMenuDone(GL_STATE_T *pstate)
{
	if ((pstate->menu_skip_timer == 0) && (pstate->menu_terminal_id) && (pstate->menu_terminal_enabled > 0))
	{
		DISPLAY_CONTENT_INFO dci;
		dci.Type = 1;  //DONE
		if (!SendTCPMessage(TYPE_MESSAGE_DISPLAY_CONTENT, (char*)&dci, sizeof(DISPLAY_CONTENT_INFO), NULL, 0, &pstate->menu_terminal_addr))
			pstate->menu_terminal_enabled = -100;
	}
}

void RenderDone(GL_STATE_T *pstate)
{
	DBG_LOG_IN();
	eglSwapBuffers(pstate->display, pstate->surface);
	DBG_LOG_OUT();
}

void CreateScreenModel(GL_STATE_T *pstate, GLfloat sW, GLfloat sH, int iMirror, int iZoom, int iStretch)
{
	DBG_LOG_IN();
	if ((sH != pstate->screen_height) || (sW != pstate->screen_width))
	{
		if (iStretch)
		{
			double dDiffH = (double) pstate->screen_height / (double) sH; 
			double dDiffW = (double) pstate->screen_width / (double) sW;		
			if (iZoom == 0)
			{
				if (dDiffH > dDiffW)
				{
					sW = (unsigned int)(dDiffW * sW);
					sH = (unsigned int)(dDiffW * sH);
				}
				else
				{
					sW = (unsigned int)(dDiffH * sW);
					sH = (unsigned int)(dDiffH * sH);
				}
			}
			if (iZoom == 1)
			{
				if (dDiffH > dDiffW)
				{
					sW = (unsigned int)(dDiffH * sW);
					sH = (unsigned int)(dDiffH * sH);
				}
				else
				{
					sW = (unsigned int)(dDiffW * sW);
					sH = (unsigned int)(dDiffW * sH);
				}	
			}		
		}
	}
	
	memset(pstate->fScreenVertCoords, 0, 12*sizeof(GLfloat));
	pstate->fScreenVertCoords[3] = sW - 1;
	pstate->fScreenVertCoords[9] = sW - 1;
	pstate->fScreenVertCoords[7] = sH - 1;
	pstate->fScreenVertCoords[10] = sH - 1;
	memset(pstate->fScreenTextCoords, 0, 8*sizeof(GLfloat));
	if (iMirror == 0)
	{
		pstate->fScreenTextCoords[2] = 1.0f;
		pstate->fScreenTextCoords[5] = 1.0f;
		pstate->fScreenTextCoords[6] = 1.0f;
		pstate->fScreenTextCoords[7] = 1.0f;
	}
	else
	{
		pstate->fScreenTextCoords[1] = 1.0f;
		pstate->fScreenTextCoords[2] = 1.0f;
		pstate->fScreenTextCoords[3] = 1.0f;
		pstate->fScreenTextCoords[6] = 1.0f;
	}
	
	pstate->screen_width_center = ((int)pstate->screen_width - (int)sW)/2;
	pstate->screen_height_center = ((int)pstate->screen_height - (int)sH)/2;
	
	DBG_LOG_OUT();
}

void CreateMainModel(GL_STATE_T *pstate, int iMirror)
{		
	DBG_LOG_IN();
	memset(pstate->fScreenVertCoords, 0, 12*sizeof(GLfloat));
	pstate->fScreenVertCoords[3] = pstate->screen_width - 1;
	pstate->fScreenVertCoords[9] = pstate->screen_width - 1;
	pstate->fScreenVertCoords[7] = pstate->screen_height - 1;
	pstate->fScreenVertCoords[10] = pstate->screen_height - 1;
	memset(pstate->fScreenTextCoords, 0, 8*sizeof(GLfloat));
	if (iMirror == 0)
	{
		pstate->fScreenTextCoords[2] = 1.0f;
		pstate->fScreenTextCoords[5] = 1.0f;
		pstate->fScreenTextCoords[6] = 1.0f;
		pstate->fScreenTextCoords[7] = 1.0f;
	}
	else
	{
		pstate->fScreenTextCoords[1] = 1.0f;
		pstate->fScreenTextCoords[2] = 1.0f;
		pstate->fScreenTextCoords[3] = 1.0f;
		pstate->fScreenTextCoords[6] = 1.0f;
	}
	
	pstate->screen_width_center = 0;
	pstate->screen_height_center = 0;
	
	pstate->tex_main_size = ((pstate->screen_width+31)&~31) * ((pstate->screen_height+15)&~15) * sizeof(RGBA_T);
	pstate->tex_main = DBG_MALLOC(pstate->tex_main_size);
	memset(pstate->tex_main, 0, pstate->tex_main_size);	
	DBG_LOG_OUT();
}

void CreateAlphaModel(GL_STATE_T *pstate)
{
	DBG_LOG_IN();
	int n;
	memset(pstate->fAlphaVertCoords, 0, 256*12*sizeof(GLfloat));
	for (n = 0; n < 256; n++)
	{		
		pstate->fAlphaVertCoords[3+(n*12)] = pstate->screen_width - 1;
		pstate->fAlphaVertCoords[9+(n*12)] = pstate->screen_width - 1;
		pstate->fAlphaVertCoords[7+(n*12)] = pstate->screen_height - 1;
		pstate->fAlphaVertCoords[10+(n*12)] = pstate->screen_height - 1;
	}
	GLfloat fOffset = 1.0f/256;
	for (n = 0; n < 256; n++)
	{   
		pstate->fAlphaTextCoords[(n*8)] = n*fOffset;
		pstate->fAlphaTextCoords[1+(n*8)] = 0.0f;
		pstate->fAlphaTextCoords[2+(n*8)] = (n+1)*fOffset;
		pstate->fAlphaTextCoords[3+(n*8)] = 0.0f;//(n+1)*fOffset;
		pstate->fAlphaTextCoords[4+(n*8)] = n*fOffset;
		pstate->fAlphaTextCoords[5+(n*8)] = 1.0f;
		pstate->fAlphaTextCoords[6+(n*8)] = (n+1)*fOffset;
		pstate->fAlphaTextCoords[7+(n*8)] = 1.0f;
	}
	
	pstate->tex_alpha = DBG_MALLOC(256*sizeof(RGBA_T));
	memset(pstate->tex_alpha, 0 , 256*sizeof(RGBA_T));
	for (n = 0; n < 256; n++) 
			pstate->tex_alpha[n].Alpha = n;
	
	
	DBG_LOG_OUT();
}

void CreateGraphModel(GL_STATE_T *pstate)
{
	DBG_LOG_IN();
	
	memset(pstate->fGraphVertCoords, 0, 12*sizeof(GLfloat));
	pstate->fGraphVertCoords[3] = 1;
	pstate->fGraphVertCoords[9] = 1;
	pstate->fGraphVertCoords[7] = 1;
	pstate->fGraphVertCoords[10] = 1;
	memset(pstate->fGraphTextCoords, 0, 8*sizeof(GLfloat));
	pstate->fGraphTextCoords[2] = 1.0f;
	pstate->fGraphTextCoords[5] = 1.0f;
	pstate->fGraphTextCoords[6] = 1.0f;
	pstate->fGraphTextCoords[7] = 1.0f;
		
	pstate->tex_graph = DBG_MALLOC(sizeof(RGBA_T));
	memset(pstate->tex_graph, 0 , sizeof(RGBA_T));
	pstate->tex_graph[0].Alpha = 255;
	pstate->tex_graph[0].Green = 255;
	
	DBG_LOG_OUT();
}

static void Init_Textures(GL_STATE_T *pstate, char cZoomSet)
{
	DBG_LOG_IN();
	//CreateMainModel(pstate, 1);
	CreateScreenModel(pstate, pstate->screen_width, pstate->screen_height, 0, cZoomSet, 1);
	CreateAlphaModel(pstate);
	CreateGraphModel(pstate);
   
	glGenTextures(3, &pstate->tex[0]);
  // omx_play_video_on_egl("/opt/vc/src/hello_pi/test.h264", pstate->tex[0], &pstate->display, &pstate->context, &eglImage);
   
	glBindTexture (GL_TEXTURE_2D, pstate->tex[0]);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pstate->screen_width, pstate->screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pstate->tex_main);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//GL_LINEAR
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST
  
  	glBindTexture(GL_TEXTURE_2D, pstate->tex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pstate->tex_alpha);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
	DBG_FREE(pstate->tex_alpha);
	
	glBindTexture(GL_TEXTURE_2D, pstate->tex[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pstate->tex_graph);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
	DBG_FREE(pstate->tex_graph);
	
	glEnable(GL_TEXTURE_2D);
	DBG_LOG_OUT();
}

//static void Load_Tex_Images(GL_STATE_T *state)
//{
 /*  FILE *tex_file1 = NULL, *tex_file2=NULL; //, *tex_file3 = NULL;
   //int bytes_read;
   int image_sz = IMAGE_SIZE*IMAGE_SIZE*4;
//   int im = IMAGE_SIZE*IMAGE_SIZE*3;

   state->tex_buf1 = DBG_MALLOC(image_sz);
   state->tex_buf2 = DBG_MALLOC(image_sz);
   char *tex_buf3 = DBG_MALLOC(image_sz);

   tex_file1 = fopen(PATH "Lucca_128_128.raw", "rb");
   if (tex_file1 && state->tex_buf1)
   {
      //bytes_read=
      fread(state->tex_buf1, 1, image_sz, tex_file1);
     // assert(bytes_read == image_sz);  // some problem with file?
      fclose(tex_file1);
   }
   tex_file2 = fopen(PATH "Djenne_128_128.raw", "rb");
   if (tex_file2 && tex_buf3)
   {
      //bytes_read=
      fread(state->tex_buf2, 1, image_sz, tex_file2);
      //assert(bytes_read == image_sz);  // some problem with file?
      fclose(tex_file2);
   }*/
/*
   tex_file3 = fopen(PATH "Gaudi_128_128.raw", "rb");
   if (tex_file3 && state->tex_buf3)
   {
      bytes_read=fread(state->tex_buf3, 1, image_sz, tex_file3);
      assert(bytes_read == image_sz);  // some problem with file?
      fclose(tex_file3);
   }*/
//}

static void Exit_Func(void) // Function to be passed to atexit().
{
	DBG_LOG_IN();
	glDeleteTextures(3, &state->tex[0]);
	if (eglImage != 0)
	{
      if (!eglDestroyImageKHR(state->display, (EGLImageKHR) eglImage))
         dbgprintf(1,"eglDestroyImageKHR failed.\n");
	}
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );
    eglSwapBuffers(state->display, state->surface);

    // Release OpenGL resources
    eglMakeCurrent( state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( state->display, state->surface );
    eglDestroyContext( state->display, state->context );
    eglTerminate( state->display );

    // release texture buffers
    //	DBG_FREE(state->tex_alpha);
    //  DBG_FREE(state->tex_buf2);
    //  DBG_FREE(state->tex_buf3);

    //dbgprintf(3,"cube closed\n");
    DBG_LOG_OUT();
} // exit_func()

int utf8_to_cp866(char *cSourceStr, unsigned int uiSourceLen, char *cDestStr, unsigned int *uiDestLen)
{
    unsigned int uiMaxOutSize = *uiDestLen;
    if (uiMaxOutSize == 0) return 0;
	iconv_t cd;
 
    cd = iconv_open("CP866", "UTF-8");
    if (cd == (iconv_t)(-1)) 
	{
		dbgprintf(1, "Error open iconv\n");
		return 0;
	}
 
    size_t in_size= uiSourceLen;
    char* in= cSourceStr;
	memset(cDestStr, 0, uiMaxOutSize);
	uiMaxOutSize--;
	char *out_buf = (char*)DBG_MALLOC(uiMaxOutSize);	
    char* out;
	char* pout = cDestStr;
	*uiDestLen = 0;
    size_t out_size;
	int ret = 1;
    size_t  k;
 
    //while(in_size > 0)
    {
            out = out_buf;
            out_size = uiMaxOutSize;
 
            errno = 0;
            k = iconv(cd, &in,  &in_size, &out, &out_size);
 
            if (k == (size_t)-1 && errno) 
			{
				if (errno == EBADF) 
				{
					dbgprintf(1, "Error iconv %i\n", errno);
					ret = 0;
				} 
				else if ((errno != E2BIG) && (errno != EINVAL)) dbgprintf(2, "Warning iconv '%s'\n", strerror(errno));
			}			
            if (ret)
			{
				memcpy(&pout[*uiDestLen], out_buf, uiMaxOutSize-out_size);
				*uiDestLen = (*uiDestLen) + uiMaxOutSize-out_size;
			}
    }
	DBG_FREE(out_buf);
    if(iconv_close(cd)!=0 ) 
	{
		dbgprintf(1, "Error close iconv\n");
		return 0;
	}
	return ret;
}

int GetFileCreationTime(char *path, struct tm *file_time) 
{	
	struct stat attr;
    if (stat(path, &attr) == 0)
	{
		localtime_r(&attr.st_mtime, file_time);
	} else return 0;
	return 1;
}

unsigned int GetFileSize(char* filename)
{
	FILE *fp = fopen (filename, "rb");
	if (!fp) 
	{
		dbgprintf(1,"Error open file %s \n",filename);
		return 0;
	}
	if (fseek (fp, 0L, SEEK_END) < 0) 
	{
		dbgprintf(1,"Error seek file %s \n",filename);
		fclose (fp);
		return 0;
	}
	unsigned int pos = ftell (fp);
	if (pos == LONG_MAX) 
	{
		dbgprintf(1,"Error tell file %s \n",filename);
		fclose (fp);
		return 0;
	}
	fclose (fp);
	return pos;
}

int GetListFiles(char* cDir, LIST_FILES_TYPE** cList, char cType)
{
	DBG_LOG_IN();
	DIR *dir;
	struct dirent *dp;
	*cList = NULL;
	LIST_FILES_TYPE *cList1;
	LIST_FILES_TYPE *cList2 = NULL;
	int iFilesCount = 0;
	char cFlag;
	char cBuff[1024];
	dir = opendir(cDir);
	if (dir != NULL)
	{
		while((dp=readdir(dir)) != NULL)
		{		
			cFlag = 1;
			memset(cBuff, 0, 1024);
			strcpy(cBuff, cDir);
			strcat(cBuff, "/");
			strcat(cBuff, dp->d_name);
			if ((cType == 0) && (!Its_Directory(cBuff))) cFlag = 0;
			if ((cType == 1) && (Its_Directory(cBuff))) cFlag = 0;
			if (cFlag && (strlen(cBuff) < MAX_FILE_LEN) && ((cType == 1) || (dp->d_name[0] != 46)))
			{			
				iFilesCount++;
				cList2 = (LIST_FILES_TYPE*)DBG_REALLOC(cList2, iFilesCount*sizeof(LIST_FILES_TYPE));
				memset(cList2[iFilesCount-1].Name, 0, MAX_FILE_LEN);
				cList2[iFilesCount-1].Flag = 0;
				strcpy(cList2[iFilesCount-1].Name, dp->d_name);
				dbgprintf(5,"file %i:%s %i\n", iFilesCount, cList2[iFilesCount-1].Name, cList2[iFilesCount-1].Flag);
			}
		}
		closedir(dir);
    }
	
	if (iFilesCount)
	{
		cList1 = (LIST_FILES_TYPE*)DBG_MALLOC(iFilesCount*sizeof(LIST_FILES_TYPE));
		memset(cList1, 0, iFilesCount*sizeof(LIST_FILES_TYPE));
		int n;
		unsigned int ilen;
		for (n = 0; n < iFilesCount; n++)
		{
			ilen = MAX_FILE_LEN;
			utf8_to_cp866(cList2[n].Name, MAX_FILE_LEN, cList1[n].Name, &ilen);
			UpperTextLimit(cList1[n].Name, ilen);
		}
			
		SortFiles(cList1, iFilesCount);
		
		for (n = 0; n < iFilesCount; n++)
			memcpy(cList1[n].Name, cList2[cList1[n].Sort].Name, MAX_FILE_LEN);
		
		DBG_FREE(cList2);
		*cList = cList1;
	}
	
	dbgprintf(5,"cnt:%i, Dir:%s\n", iFilesCount, cDir);
	DBG_LOG_OUT();
	return iFilesCount;
}

int UpdateListFiles(char *cPath, int iMode)
{
	DBG_LOG_IN();	
	
	int iNewPath = 0;
	
	if (cPath && (cCurrentFileLocation != cPath))
	{
		memset(cCurrentFileLocation, 0, 256);
		strcpy(cCurrentFileLocation, cPath);
		iNewPath = 1;
	}
	
	if ((iMode == UPDATE_LIST_SCANTYPE_FULL) && (!cPath))
	{
		memset(cCurrentFileLocation, 0, 256);
		strcpy(cCurrentFileLocation, cFileLocation);
		cPath = cCurrentFileLocation;
		iNewPath = 1;
	}
	
	int iFilesCount = 0;
	int iFilesCurPos = -1;
	LIST_FILES_TYPE *lFiles = NULL;
/*	int iDirsCount = iListDirsCount;
	int iDirsOrderCnt = iListDirsOrderCnt;
	int iDirsCurPos = iListDirsCurPos;
	LIST_FILES_TYPE *lDirs = (LIST_FILES_TYPE*)DBG_MALLOC(sizeof(LIST_FILES_TYPE) * iListDirsCount);*/
	
	iListFilesCount = 0;	
	iListFilesCurPos = -1;
	iListFilesOrderCnt = 0;	
	if (cListFiles != NULL) {DBG_FREE(cListFiles);cListFiles = NULL;}
	
	if ((iMode < UPDATE_LIST_SCANTYPE_NEXT) || iNewPath)
	{
		iListDirsOrderCnt = 0;
		iListDirsCount = 0;
		if (cListDirs != NULL) {DBG_FREE(cListDirs);cListDirs = NULL;}
		iListDirsCurPos = 0;
	}
	
	if (strlen(cPath) == 0){DBG_LOG_OUT(); return 0;}
	
	char cPath2[256];
	memset(cPath2,0,256);
	
	char *cFile;
	int value;
	
	if ((iMode == UPDATE_LIST_SCANTYPE_FULL) || 
		(((iMode == UPDATE_LIST_SCANTYPE_NEXT) || (iMode == UPDATE_LIST_SCANTYPE_PREV)) && (iListDirsCount == 0)))
	{
		time_t rawtime;
		struct tm timeinfo;
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		char mon[10];
		char day[10];
		
		memset(mon,0,10);
		memset(day,0,10);
		memset(cPath2,0,256);
		sprintf(mon, "%i",timeinfo.tm_mon+1);
		sprintf(day, "%i",timeinfo.tm_mday);
		strcpy(cPath2, cPath);
		if (cPath[strlen(cPath)-1] != 47) strcat(cPath2, "/");
		strcat(cPath2, mon);
		strcat(cPath2, "/");
		strcat(cPath2, day);	
	
		//////SCAN DATE PATH//////////
		iListDirsCount = GetListFiles(cPath2, &cListDirs, 0);
		//printf("//////SCAN DATE PATH///////// %i\n", iListDirsCount);
		if (iListDirsCount) 
		{
			if (cPath[strlen(cPath)-1] != 47) strcat(cPath, "/");
			strcat(cPath, mon);
			strcat(cPath, "/");
			strcat(cPath, day);	
		}
		else
		{
			memset(cPath2, 0, 256);
			strcpy(cPath2, cPath);
			if (cPath[strlen(cPath)-1] != 47) strcat(cPath2, "/");
			strcat(cPath2, "misc");	
			//////SCAN MISC PATH//////////
			iListDirsCount = GetListFiles(cPath2, &cListDirs, 0);
			if (iListDirsCount) 
			{
				if (cPath[strlen(cPath)-1] != 47) strcat(cPath, "/");
				strcat(cPath, "misc");
			}
			//printf("//////SCAN MISC PATH///////// %i\n", iListDirsCount);
		}
	}
	
	if ((iListDirsCount == 0) && (iMode != UPDATE_LIST_SCANTYPE_CUR))
	{
		memset(cPath2, 0, 256);
		strcpy(cPath2, cPath);			
		iListDirsCount = GetListFiles(cPath2, &cListDirs, 0); //////SCAN MAIN PATH FOR DIRS//////////
		//printf("//////SCAN MAIN PATH FOR DIRS///////// %i\n", iListDirsCount);
	}
	
	if (iMode == UPDATE_LIST_SCANTYPE_CUR)
	{
		memset(cPath2, 0, 256);
		strcpy(cPath2, cPath);	
		if (cPath2[strlen(cPath2)-1] == 47) cPath2[strlen(cPath2)-1] = 0;		
		iFilesCount = GetListFiles(cPath2, &lFiles, 1); 							
		cChangeShowNow = 1;
		cNewShowType = SHOW_TYPE_NEW;
		//printf("//////SCAN MAIN PATH FOR FILES////////// %i\n", iFilesCount);
	}
	
	if (iMode == UPDATE_LIST_SCANTYPE_NEXT)
	{
		if (cCurRandomDir == 0)
		{
			iListDirsCurPos++;
			if (iListDirsCurPos >= iListDirsCount) iListDirsCurPos = 0;
		} else iListDirsCurPos = GenRandomInt(iListDirsCount-1);
		iFilesCount = 0;	
		iFilesCurPos = -1;
		if (lFiles != NULL) {DBG_FREE(lFiles);lFiles = NULL;}	
		//printf("//////CLEAR FILES 2 mode////////// %i\n", iFilesCount);
	}
	
	if (iMode == UPDATE_LIST_SCANTYPE_PREV)
	{
		if ((iListDirsCurPos >= 0) && (iListDirsCurPos < iListDirsCount)) cListDirs[iListDirsCurPos].Flag = 0;
		if (cCurRandomDir == 0)
		{
			iListDirsCurPos--;
			if (iListDirsCurPos < 0) iListDirsCurPos = iListDirsCount - 1;
		} 
		else 
		{
			int iCheckCnt;
			for (iCheckCnt = 0; iCheckCnt < iListDirsCount; iCheckCnt++)
				if (cListDirs[iCheckCnt].Flag == iListDirsOrderCnt) 
				{
					cListDirs[iCheckCnt].Flag = 0;
					break;
				}
			if (iListDirsOrderCnt) value = iListDirsOrderCnt - 1;
			for (iCheckCnt = 0; iCheckCnt < iFilesCount; iCheckCnt++)
				if (cListDirs[iCheckCnt].Flag == value) 
				{
					iListDirsCurPos = iCheckCnt;
					break;
				}
			if (iCheckCnt == iFilesCount) value = 0;
		}
		iFilesCount = 0;	
		iFilesCurPos = -1;
		if (lFiles != NULL) {DBG_FREE(lFiles);lFiles = NULL;}	
	}
	
	if ((iListDirsCount != 0) && (iMode != UPDATE_LIST_SCANTYPE_CUR)) 
	{
		iFilesCount = 0;	
		iFilesCurPos = -1;
		if (lFiles != NULL) {DBG_FREE(lFiles);lFiles = NULL;}
		
		memset(cPath2, 0, 256);
		strcpy(cPath2, cPath);	
		//printf(">>> %s\n", cPath2);
		//printf("!!! %i %i\n", iListDirsCurPos, iListDirsCount);
		value = iListDirsCurPos;
		iListDirsOrderCnt++;
		cListDirs[value].Flag = iListDirsOrderCnt;
		cFile = cListDirs[value].Name;
		strcat(cPath2, "/");
		strcat(cPath2, cFile);
		
		iFilesCount = GetListFiles(cPath2, &lFiles, 1);
		if (cChangeShowNow == 0) 
		{
			cChangeShowNow = 1;
			cNewShowType = SHOW_TYPE_NEW;
		}
		//printf("//////GetListFiles////////// %i %s\n", iFilesCount, cPath2);
		
		if (iFilesCount != 0) iListDirsCurPos = value;
	}	
	
	unsigned int ilen = 252;
	char buff[256];
	memset(buff, 0, 256);
	memset(cPath2,0,256);
	strcpy(cPath2, cPath);
	if (iListDirsCount && (iListDirsCurPos >= 0))
	{
		strcat(cPath2, "/");
		strcat(cPath2, cListDirs[iListDirsCurPos].Name);
	}
	utf8_to_cp866(cPath2, (strlen(cPath2) < ilen) ? strlen(cPath2) : 252, buff, &ilen);
	if (buff[strlen(buff)-1] == 47) buff[strlen(buff)-1] = 0;	
	int i;
	int t = 0;
	int tprev = 0;
	ilen = strlen(buff);
	for (i = 0; i < ilen; i++) if (buff[i] == 47) { tprev = t; t = i;}
	if (iListDirsCount && (iListDirsCurPos >= 0)) t = tprev;
	if ((ilen - t) > 127) t = ilen - 127;
	t++;
	memset(cSettShowDirNameText, 0, 128);
	memcpy(cSettShowDirNameText, &buff[t], ilen - t);
	//printf("'%s' '%s' '%s'\n", cPath2, buff, cSettShowDirNameText);
		
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (pScreenMenu)
	{
		RefreshDirListMenu(pScreenMenu, 0);
		RefreshPlayListMenu(pScreenMenu, 0);
	}
	if (pWebMenu)
	{
		RefreshDirListMenu(pWebMenu, 0);
		RefreshPlayListMenu(pWebMenu, 0);
	}
	DBG_MUTEX_LOCK(&system_mutex);	
	
	if (iFilesCount)
	{
		iListFilesOrderCnt = 0;	
		iListFilesCount = iFilesCount;	
		iListFilesCurPos = iFilesCurPos;
		if (cListFiles != NULL) {DBG_FREE(cListFiles);cListFiles = NULL;}
		cListFiles = lFiles;
	}
	
/*	if ((!iFilesCount) && (iDirsCount)) 
	{
		iListDirsCount = iDirsCount;
		iListDirsOrderCnt = iDirsOrderCnt;
		iListDirsCurPos = iDirsCurPos;
		if (cListDirs != NULL) {DBG_FREE(cListDirs);cListDirs = NULL;}
		cListDirs = lDirs;	
	}
	else DBG_FREE(lDirs);*/
	
	DBG_LOG_OUT();
	return iFilesCount;
}

int UpdateAlarmFiles(char *cPath)
{
	DBG_LOG_IN();
	if (cListAlarmFiles != NULL) {DBG_FREE(cListAlarmFiles);cListAlarmFiles = NULL;}
	iListAlarmFilesCount = 0;
	if (strlen(cPath) == 0){DBG_LOG_OUT(); return 0;}
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	char mon[10];
	char day[10];
	char cPath2[256];
	char *cFile;
	memset(mon,0,10);
	memset(day,0,10);
	memset(cPath2,0,256);
	sprintf(mon, "%i",timeinfo.tm_mon+1);
	sprintf(day, "%i",timeinfo.tm_mday);
	strcpy(cPath2, cPath);
	if (cPath[strlen(cPath)-1] != 47) strcat(cPath2, "/");
	strcat(cPath2, mon);
	strcat(cPath2, "/");
	strcat(cPath2, day);
	iListAlarmFilesCount = GetListFiles(cPath2, &cListAlarmFiles, 0);
	if (iListAlarmFilesCount == 0) 
	{
		memset(cPath2, 0, 256);
		strcpy(cPath2, cPath);
		if (cPath2[strlen(cPath2)-1] == 47) cPath2[strlen(cPath2)-1] = 0;
		strcat(cPath2, "misc");	
		iListAlarmFilesCount = GetListFiles(cPath2, &cListAlarmFiles, 0);
	}
	if (iListAlarmFilesCount == 0) 
	{
		memset(cPath2, 0, 256);
		strcpy(cPath2, cPath);
		if (cPath2[strlen(cPath2)-1] == 47) cPath2[strlen(cPath2)-1] = 0;
		iListAlarmFilesCount = GetListFiles(cPath2, &cListAlarmFiles, 1);
	}
	if (iListAlarmFilesCount != 0) 
	{
		//int random_value = (rand() % iListAlarmFilesCount);
		int random_value = GenRandomInt(iListAlarmFilesCount-1);
		//printf("random value2: %i\n", random_value);
		cFile = cListAlarmFiles[random_value].Name;
		strcat(cPath2, "/");
		strcat(cPath2, cFile);
		if (cListAlarmFiles != NULL) {DBG_FREE(cListAlarmFiles);cListAlarmFiles = NULL;}
		iListAlarmFilesCount = GetListFiles(cPath2, &cListAlarmFiles, 1);
		if (iListAlarmFilesCount != 0) 
		{
			memset(cCurrentAlarmDir,0, 256);
			strcpy(cCurrentAlarmDir, cPath2);	
		}
	}
	if (iListAlarmFilesCount == 0)  
	{
		memset(cPath2, 0, 256);
		strcpy(cPath2, cPath);
		if (cPath2[strlen(cPath2)-1] == 47) cPath2[strlen(cPath2)-1] = 0;
		iListAlarmFilesCount = GetListFiles(cPath2, &cListAlarmFiles, 1);	
		if (iListAlarmFilesCount != 0) 
		{
			memset(cCurrentAlarmDir,0, 256);
			strcpy(cCurrentAlarmDir, cPath2);	
		}
	}
	if (iListAlarmFilesCount == 0)  
	{
		iListAlarmFilesCount = GetListFiles(".", &cListAlarmFiles, 1);	
		if (iListAlarmFilesCount != 0) 
		{
			memset(cCurrentAlarmDir,0, 256);
			strcpy(cCurrentAlarmDir, ".");	
		}
	}
	DBG_LOG_OUT();
	return iListAlarmFilesCount;
}

int UpdateAlarmFilesNoSub(char *cPath)
{	
	DBG_LOG_IN();
	
	if (cListAlarmFiles != NULL) {DBG_FREE(cListAlarmFiles);cListAlarmFiles = NULL;}
	iListAlarmFilesCount = 0;
	if (strlen(cPath) == 0){DBG_LOG_OUT(); return 0;}
	
	char cPath2[256];
	
	memset(cPath2,0,256);
	strcpy(cPath2, cPath);
	if (cPath2[strlen(cPath2)-1] == 47) cPath2[strlen(cPath2)-1] = 0;
	
	iListAlarmFilesCount = GetListFiles(cPath2, &cListAlarmFiles, 1);
	if (iListAlarmFilesCount != 0) 
	{
		memset(cCurrentAlarmDir,0, 256);
		strcpy(cCurrentAlarmDir, cPath2);	
	}
	if (iListAlarmFilesCount == 0)  
	{
		//printf("3) %s\n",cPath2);		
		iListAlarmFilesCount = GetListFiles(".", &cListAlarmFiles, 1);	
		if (iListAlarmFilesCount != 0) 
		{
			memset(cCurrentAlarmDir, 0, 256);
			strcpy(cCurrentAlarmDir, ".");	
		}
	}
	
	DBG_LOG_OUT();
	return iListAlarmFilesCount;
}

void AnimateWidget(GL_STATE_T *state, WIDGET_INFO *wiWidget)
{
	double dSizeX = wiWidget->SizeX * (wiWidget->Scale * wiWidget->DefaultScale);
	double dSizeY = wiWidget->SizeY * (wiWidget->Scale * wiWidget->DefaultScale);
	double dDiffX;
	double dDiffY;
	
	if (wiWidget->SelfPath)
	{
		dDiffX = 0;
		dDiffY = 0;
	}
	else
	{
		dDiffX = wiWidget->MaxSizeX - dSizeX;
		dDiffY = wiWidget->MaxSizeY - dSizeY;
	}
	//printf("%i %i  % %f\n", wiWidget->Direct, wiWidget->AnimateMode, wiWidget->SizeX, wiWidget->Scale);
		
	switch (wiWidget->Direct)
	{
		case WIDGET_DIRECT_BORDER_CW:
			if ((wiWidget->AnimateMode < 0) || (wiWidget->AnimateMode > 4)) wiWidget->AnimateMode = 0;
			if (wiWidget->AnimateMode == 0)
			{ 
				if (wiWidget->PosY > 0) wiWidget->PosY -= wiWidget->Speed;
				if (wiWidget->PosX < ((double)(state->screen_width) - (double)(uiPaddingSize) - dSizeX - dDiffX)) wiWidget->PosX += wiWidget->Speed;
					else wiWidget->AnimateMode = 1;
			}
			if (wiWidget->AnimateMode == 1)
			{ 
				if (wiWidget->PosX < ((double)(state->screen_width) - (double)(uiPaddingSize) - dSizeX)) wiWidget->PosX += wiWidget->Speed;
				if (wiWidget->PosY < (state->screen_height - dSizeY - dDiffY)) wiWidget->PosY += wiWidget->Speed;
					else wiWidget->AnimateMode = 2;		
			}
			if (wiWidget->AnimateMode == 2)
			{
				if (wiWidget->PosY < ((double)(state->screen_height) - dSizeY)) wiWidget->PosY += wiWidget->Speed;				
				if (wiWidget->PosX > ((double)(uiPaddingSize) + dDiffX)) wiWidget->PosX -= wiWidget->Speed;
					else wiWidget->AnimateMode = 3;	
			}
			if (wiWidget->AnimateMode == 3)
			{ 
				if (wiWidget->PosX > ((double)uiPaddingSize)) wiWidget->PosX -= wiWidget->Speed;
				if (wiWidget->PosY > dDiffY) wiWidget->PosY -= wiWidget->Speed;
					else wiWidget->AnimateMode = 0;	
			}
			break;
		case WIDGET_DIRECT_BORDER_CCW:
			if ((wiWidget->AnimateMode < 0) || (wiWidget->AnimateMode > 4)) wiWidget->AnimateMode = 0;
			if (wiWidget->AnimateMode == 0)
			{ 
				if ((wiWidget->PosX + (wiWidget->SizeX * (wiWidget->Scale * wiWidget->DefaultScale)) + (double)(uiPaddingSize)) <= (double)(state->screen_width)) wiWidget->PosX += wiWidget->Speed;
					else wiWidget->AnimateMode = 3;
			}
			if (wiWidget->AnimateMode == 1) 
			{
				if (wiWidget->PosY + (wiWidget->SizeY * (wiWidget->Scale * wiWidget->DefaultScale)) <= (double)(state->screen_height)) wiWidget->PosY += wiWidget->Speed;
					else wiWidget->AnimateMode = 0; 
			}
			if (wiWidget->AnimateMode == 2) 
			{
				if (wiWidget->PosX > (double)(uiPaddingSize)) wiWidget->PosX -= wiWidget->Speed;
					else wiWidget->AnimateMode = 1;
			}
			if (wiWidget->AnimateMode == 3) 
			{
				if (wiWidget->PosY > 0) wiWidget->PosY -= wiWidget->Speed;
					else wiWidget->AnimateMode = 2;
			}
			break;
		case WIDGET_DIRECT_BORDER_PINGPONG:
			if ((wiWidget->AnimateMode < 0) || (wiWidget->AnimateMode > 3)) wiWidget->AnimateMode = 0;
			if (wiWidget->AnimateMode & 1)
			{ 
				if ((wiWidget->PosX + (wiWidget->SizeX * (wiWidget->Scale * wiWidget->DefaultScale)) + (double)(uiPaddingSize)) <= (double)(state->screen_width)) wiWidget->PosX += wiWidget->Speed * wiWidget->OffsetX;
					else wiWidget->AnimateMode ^= 1;
			}
			else
			{
				if (wiWidget->PosX > (double)(uiPaddingSize)) wiWidget->PosX -= wiWidget->Speed * wiWidget->OffsetX;
					else wiWidget->AnimateMode |= 1;
			}
			
			if (wiWidget->AnimateMode & 2) 
			{
				if (wiWidget->PosY + (wiWidget->SizeY * (wiWidget->Scale * wiWidget->DefaultScale)) <= (double)(state->screen_height)) wiWidget->PosY += wiWidget->Speed * wiWidget->OffsetY;
					else wiWidget->AnimateMode ^= 2;
			}
			else
			{
				if (wiWidget->PosY > 0) wiWidget->PosY -= wiWidget->Speed * wiWidget->OffsetY;
					else wiWidget->AnimateMode |= 2;
			}
			break;
		case WIDGET_DIRECT_STATIC:
			wiWidget->PosX = (double)(state->screen_width) / 100 * wiWidget->DirectX;
			wiWidget->PosY = (double)(state->screen_height) / 100 * wiWidget->DirectY;
			break;
		default:
			break;
	}	
}

int RecalcWidgetsCoords(GL_STATE_T *pstate)
{
	int iDirect, n, i, iFirst;
	double dPosX = 0;
	double dSizeX;
	double dSizeY;
	double dSizeW;
	double dWidgetMaxSizeX = 0;
	double dWidgetMaxSizeY = 0;
	
	for (n = 0; n < iWidgetsCnt; n++)
	{
		if (wiWidgetList[n].Enabled && (wiWidgetList[n].Status == 1))
		{		
			wiWidgetList[n].MaxSizeX = wiWidgetList[n].SizeX*(wiWidgetList[n].Scale*wiWidgetList[n].DefaultScale);
			wiWidgetList[n].MaxSizeY = wiWidgetList[n].SizeY*(wiWidgetList[n].Scale*wiWidgetList[n].DefaultScale);				
		}
	}
	
	for (i = WIDGET_DIRECT_BORDER_CW; i <= WIDGET_DIRECT_BORDER_CCW; i++)
	{		
		for (n = 0; n < iWidgetsCnt; n++)
		{
			if (wiWidgetList[n].Enabled && (wiWidgetList[n].Status == 1) && (wiWidgetList[n].Direct == i))
			{
				dSizeX = wiWidgetList[n].SizeX*(wiWidgetList[n].Scale*wiWidgetList[n].DefaultScale);
				dSizeY = wiWidgetList[n].SizeY*(wiWidgetList[n].Scale*wiWidgetList[n].DefaultScale);
				if (dSizeX > dWidgetMaxSizeX) dWidgetMaxSizeX = dSizeX;
				if (dSizeY > dWidgetMaxSizeY) dWidgetMaxSizeY = dSizeY;
			}
		}
		
		for (n = 0; n < iWidgetsCnt; n++)
		{
			if (wiWidgetList[n].Enabled && (wiWidgetList[n].Status == 1) && (wiWidgetList[n].Direct == i))
			{
				wiWidgetList[n].MaxSizeX = dWidgetMaxSizeX;
				wiWidgetList[n].MaxSizeY = dWidgetMaxSizeY;
			}
		}
	}
		
	for (iDirect = WIDGET_DIRECT_BORDER_CW; iDirect <= WIDGET_DIRECT_BORDER_CCW; iDirect++)
	{		
		iFirst = 1;
		for (n = 0; n < iWidgetsCnt; n++)
		{
			if (wiWidgetList[n].Enabled && (wiWidgetList[n].Direct == iDirect) && (wiWidgetList[n].Status == 1))
			{
				dSizeX = wiWidgetList[n].SizeX*(wiWidgetList[n].Scale*wiWidgetList[n].DefaultScale);
				dSizeY = wiWidgetList[n].SizeY*(wiWidgetList[n].Scale*wiWidgetList[n].DefaultScale);
				dSizeW = sqrt((dSizeX*dSizeX) + (dSizeY*dSizeY));
				if (iFirst == 1)
				{
					iFirst = 0;
					wiWidgetList[n].PosX = (double)(pstate->screen_width)/2 - dSizeX/2;
				}
				else wiWidgetList[n].PosX = dPosX - dSizeW;
				
				dPosX = wiWidgetList[n].PosX;
				if (iDirect == WIDGET_DIRECT_BORDER_CW) wiWidgetList[n].PosY = 0;
				if (iDirect == WIDGET_DIRECT_BORDER_CCW) wiWidgetList[n].PosY = (double)(pstate->screen_height) - dSizeY;
				wiWidgetList[n].AnimateMode = 0;
				//printf("%g,\t%g,\t%g,\t%g,\t%g\n", wiWidgetList[n].PosX, wiWidgetList[n].PosY, wiWidgetList[n].SizeX, wiWidgetList[n].SizeY, dSizeW);
			}			
		}
	}
	return 1;
}

int UpdateWidgets(GL_STATE_T *pstate, int iRecalcForce)
{
	DBG_LOG_IN();
	
	int n, i, sW, sH;
	int res = iRecalcForce;
	int iWeatherUpdate = 0;
	time_t rawtime;
	struct tm timeinfo;
	int NowTime, iCurStatus;
	unsigned char NowDay;	
	
	for (n = 0; n < iWidgetsCnt; n++)
	{
		if (wiWidgetList[n].Enabled && iRecalcForce && (wiWidgetList[n].Status >= 0))
		{
			wiWidgetList[n].PosX = (double)(state->screen_width) / 100 * wiWidgetList[n].DirectX;
			wiWidgetList[n].PosY = (double)(state->screen_height) / 100 * wiWidgetList[n].DirectY;
			if (wiWidgetList[n].Direct == WIDGET_DIRECT_BORDER_PINGPONG)
			{
				wiWidgetList[n].OffsetX	= sin(M_PI/180*wiWidgetList[n].Angle);
				wiWidgetList[n].OffsetY	= cos(M_PI/180*wiWidgetList[n].Angle);
				
				//printf("######## %i %f %f %f\n", n, wiWidgetList[n].Angle, wiWidgetList[n].OffsetX, wiWidgetList[n].OffsetY);
				wiWidgetList[n].AnimateMode = 0;
				if (wiWidgetList[n].OffsetX >= 0.0f) wiWidgetList[n].AnimateMode |= 1; else	wiWidgetList[n].OffsetX *= -1;
				if (wiWidgetList[n].OffsetY >= 0.0f) wiWidgetList[n].AnimateMode |= 2; else	wiWidgetList[n].OffsetY *= -1;
				//printf("######## %i %f %f\n", n, wiWidgetList[n].OffsetX, wiWidgetList[n].OffsetY);
			}
		}
		
		if (wiWidgetList[n].Enabled && (wiWidgetList[n].ShowMode != 0) && (wiWidgetList[n].Status >= 0))
		{				
			time(&rawtime);
			localtime_r(&rawtime, &timeinfo);
			NowTime = timeinfo.tm_hour * 10000;
			NowTime += (timeinfo.tm_min) * 100;
			NowTime += timeinfo.tm_sec;					
			NowDay = 1 << timeinfo.tm_wday;
			if (NowDay == 1) NowDay |= 128;
			NowDay = NowDay >> 1;
			
			iCurStatus = wiWidgetList[n].Status;
			
			if ((NowDay & wiWidgetList[n].WeekDays)
					&& 
					(	
						(
							((wiWidgetList[n].NotBeforeTime <= wiWidgetList[n].NotAfterTime) 
							&& (wiWidgetList[n].NotBeforeTime <= NowTime) 
							&& (wiWidgetList[n].NotAfterTime >= NowTime)
							)
							||
							((wiWidgetList[n].NotBeforeTime > wiWidgetList[n].NotAfterTime) 
							&& ((wiWidgetList[n].NotBeforeTime <= NowTime) 
							|| (wiWidgetList[n].NotAfterTime >= NowTime))
							)
						)
						||
						(
							((wiWidgetList[n].NotBeforeTime2 <= wiWidgetList[n].NotAfterTime2) 
							&& (wiWidgetList[n].NotBeforeTime2 <= NowTime) 
							&& (wiWidgetList[n].NotAfterTime2 >= NowTime)
							)
							||
							((wiWidgetList[n].NotBeforeTime2 > wiWidgetList[n].NotAfterTime2) 
							&& ((wiWidgetList[n].NotBeforeTime2 <= NowTime) 
							|| (wiWidgetList[n].NotAfterTime2 >= NowTime))
							)
						)
						||
						(
							((wiWidgetList[n].NotBeforeTime3 <= wiWidgetList[n].NotAfterTime3) 
							&& (wiWidgetList[n].NotBeforeTime3 <= NowTime) 
							&& (wiWidgetList[n].NotAfterTime3 >= NowTime)
							)
							||
							((wiWidgetList[n].NotBeforeTime3 > wiWidgetList[n].NotAfterTime3) 
							&& ((wiWidgetList[n].NotBeforeTime3 <= NowTime) 
							|| (wiWidgetList[n].NotAfterTime3 >= NowTime))
							)
						)						
					)					
				)
				wiWidgetList[n].InTimeLimit = 1; else wiWidgetList[n].InTimeLimit = 0;
		
			if (((wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_DAYTIME) && wiWidgetList[n].InTimeLimit)
				|| 
				(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_MENU)
				||
				(wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_ALLWAYS)
				||
				((wiWidgetList[n].ShowMode & WIDGET_SHOWMODE_TIMEOUT) && (wiWidgetList[n].ShowTimer || wiWidgetList[n].ShowRepeat)))
			{
				if (wiWidgetList[n].Status == 2) 
				{
					wiWidgetList[n].Status = 0;	
					wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
				}
				wiWidgetList[n].Timer++;	
				
				if (wiWidgetList[n].ShowRepeat)
				{
					if (wiWidgetList[n].ShowDirect)
					{
						if (wiWidgetList[n].ShowTimer < wiWidgetList[n].ShowTime) 
								wiWidgetList[n].ShowTimer++;
							else
								wiWidgetList[n].ShowDirect ^= 1;
					}
					else
					{
						if (wiWidgetList[n].ShowTimer) 
								wiWidgetList[n].ShowTimer--;
							else
								wiWidgetList[n].ShowDirect ^= 1;
					}
				} 
				else
				{
					if (wiWidgetList[n].ShowTimer) wiWidgetList[n].ShowTimer--;
				}
				
				if (wiWidgetList[n].Timer >= wiWidgetList[n].Refresh)
				{
					switch(wiWidgetList[n].Type)
					{
						case WIDGET_TYPE_SENSOR_IMAGE:
						case WIDGET_TYPE_SENSOR_TACHOMETER:
						case WIDGET_TYPE_SENSOR_TACHO_SCALE:
						case WIDGET_TYPE_SENSOR_TEMPMETER:
						case WIDGET_TYPE_SENSOR_WHITETACHO:
						case WIDGET_TYPE_SENSOR_BLACKTACHO:
						case WIDGET_TYPE_SENSOR_BLACKREGULATOR:
						case WIDGET_TYPE_SENSOR_CIRCLETACHO:
						case WIDGET_TYPE_SENSOR_DARKMETER:
						case WIDGET_TYPE_SENSOR_DARKTERMOMETER:
						case WIDGET_TYPE_SENSOR_GREENTACHO:
						case WIDGET_TYPE_SENSOR_OFFTACHO:
						case WIDGET_TYPE_SENSOR_SILVERREGULATOR:
						case WIDGET_TYPE_SENSOR_WHITETERMOMETER:
							wiWidgetList[n].Status = 1;
							wiWidgetList[n].SensorValue = 0;
							memset(wiWidgetList[n].SensorValueStr, 0, 64);									
							wiWidgetList[n].Timer = 0;
							DBG_MUTEX_LOCK(&modulelist_mutex);							
							i = ModuleIdToNum(wiWidgetList[n].ModuleID, 2);
							if (i != -1)
							{
								if (wiWidgetList[n].Type == WIDGET_TYPE_SENSOR_IMAGE)
								{
									if ((wiWidgetList[n].SourceCell > 0) && (wiWidgetList[n].SourceCell <= MAX_MODULE_STATUSES))
									{
										wiWidgetList[n].SensorValue = (double)(miModuleList[i].Status[wiWidgetList[n].SourceCell-1]+wiWidgetList[n].OffsetValue)*wiWidgetList[n].Coefficient;
										if ((wiWidgetList[n].SensorValue >= wiWidgetList[n].ShowValueFrom) && (wiWidgetList[n].SensorValue <= wiWidgetList[n].ShowValueTo))
												snprintf(wiWidgetList[n].SensorValueStr, 64, "%g%s", wiWidgetList[n].SensorValue, wiWidgetList[n].ValueTypeName);
									}
									
									sW = wiWidgetList[n].Width + wiWidgetList[n].Height*0.89*strlen(wiWidgetList[n].SensorValueStr);
									sH = wiWidgetList[n].Height;
									if (wiWidgetList[n].Name[0] != 0) sH += 20;
									if ((wiWidgetList[n].SizeX < sW) || (wiWidgetList[n].SizeY < sH)) res = 1;					
									wiWidgetList[n].SizeX = sW;
									wiWidgetList[n].SizeY = sH;									
								}
								else
								{
									if ((wiWidgetList[n].Type > WIDGET_TYPE_SENSOR_MIN) && (wiWidgetList[n].Type < WIDGET_TYPE_SENSOR_MAX))
									{
										if ((wiWidgetList[n].SourceCell > 0) && (wiWidgetList[n].SourceCell <= MAX_MODULE_STATUSES))
										{
											wiWidgetList[n].SensorValue = (double)(miModuleList[i].Status[wiWidgetList[n].SourceCell-1]+wiWidgetList[n].OffsetValue)*wiWidgetList[n].Coefficient;
										}
										
										sW = wiWidgetList[n].Width;
										sH = wiWidgetList[n].Height;
										if (wiWidgetList[n].Name[0] != 0) sH += 42;
										if ((wiWidgetList[n].SizeX < sW) || (wiWidgetList[n].SizeY < sH)) res = 1;					
										wiWidgetList[n].SizeX = sW;
										wiWidgetList[n].SizeY = sH;
									}
								}
								
								if (wiWidgetList[n].Status == 0)
								{
									wiWidgetList[n].Status = 1;										
								}
							}
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							break;
						case WIDGET_TYPE_IMAGE:
							wiWidgetList[n].Timer = 0;
							if (wiWidgetList[n].Status != 0) ReleaseWidget(&wiWidgetList[n]);								
							if (CreateWidget(&wiWidgetList[n]) == 2) res = 1;
							break;
						case WIDGET_TYPE_WEATHER:
							if (iWeatherUpdate != 1) iWeatherUpdate = PreRenderGLWeather(iTimeCor, 7);
							if (iWeatherUpdate == 1)
							{
								wiWidgetList[n].Status = 1;
								GetWeatherSize(&wiWidgetList[n].SizeX, &wiWidgetList[n].SizeY, wiWidgetList[n].Settings[0]);
							}
							if (iWeatherUpdate == 0)
							{
								//if (wiWidgetList[n].Refresh > 5) wiWidgetList[n].Timer -= 5; else 
								wiWidgetList[n].Timer = 0;
							}
							if (iWeatherUpdate < 0) wiWidgetList[n].Status = 0;
							break;
						case WIDGET_TYPE_CLOCK_EL:
						case WIDGET_TYPE_CLOCK_MECH:
						case WIDGET_TYPE_CLOCK_WHITE:
						case WIDGET_TYPE_CLOCK_BROWN:
						case WIDGET_TYPE_CLOCK_QUARTZ:
						case WIDGET_TYPE_CLOCK_SKYBLUE:
						case WIDGET_TYPE_CLOCK_ARROW:						
							if (wiWidgetList[n].Status == 0)
							{
								wiWidgetList[n].Status = 1;
							}
							wiWidgetList[n].Timer = 0;
							break;
						default:
							wiWidgetList[n].Timer = 0;					
							break;			
					}
				}
			}
			else
			{
				
				if (wiWidgetList[n].Status == 1) 
				{
					wiWidgetList[n].Status = 2;
					wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
				}
			}
			if (iCurStatus != wiWidgetList[n].Status) res = 1;
		}
	}
	
	if (res == 1) RecalcWidgetsCoords(pstate);
	DBG_LOG_OUT();
	return 1;
}

char GetChangeShowNow()
{
	char ret;
	DBG_MUTEX_LOCK(&system_mutex);								
	ret = cChangeShowNow;	
	DBG_MUTEX_UNLOCK(&system_mutex);								
	return ret;						
}

void SetChangeShowNow(char val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	cChangeShowNow = val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
}

int GetShowTimeMax()
{
	int ret;
	DBG_MUTEX_LOCK(&system_mutex);								
	ret = iShowTimeMax;	
	DBG_MUTEX_UNLOCK(&system_mutex);								
	return ret;						
}

void SetShowTimeMax(int val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	iShowTimeMax = val;	
	DBG_MUTEX_UNLOCK(&system_mutex);	
}

unsigned int GetNewShowType()
{
	unsigned int ret;
	DBG_MUTEX_LOCK(&system_mutex);								
	ret = cNewShowType;	
	DBG_MUTEX_UNLOCK(&system_mutex);								
	return ret;						
}

unsigned int GetCurShowType()
{
	unsigned int ret;
	DBG_MUTEX_LOCK(&system_mutex);								
	ret = cCurShowType;	
	DBG_MUTEX_UNLOCK(&system_mutex);								
	return ret;						
}

void SetNewShowType(unsigned int val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	cNewShowType = val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void SetCurShowType(unsigned int val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	cCurShowType = val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void AddNewShowType(unsigned int val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	cNewShowType |= val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void AddCurShowType(unsigned int val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	cCurShowType |= val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void DelCurShowType(unsigned int val)
{
	DBG_MUTEX_LOCK(&system_mutex);								
	cCurShowType ^= cCurShowType & val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void SetShowMenu(char val)
{
	char PrevStat;
	DBG_MUTEX_LOCK(&system_mutex);								
	PrevStat = cShowMenu;
	cShowMenu = val;	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (PrevStat != val)
	{
		DBG_MUTEX_LOCK(&systemlist_mutex);
		unsigned int uiLocalSysID = GetLocalSysID();
		DBG_MUTEX_UNLOCK(&systemlist_mutex);
		AddModuleEventInList(uiLocalSysID, 15, val ? SYSTEM_CMD_SHOW_MENU : SYSTEM_CMD_HIDE_MENU, NULL, 0, 0);	
		DBG_MUTEX_LOCK(&modulelist_mutex);
		int n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
		if (n >= 0) miModuleList[n].Status[2] = val;
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
	}
}

char* GetCurrDateTimeStr(char* cBuff, int iLen)
{
	if (iLen < 1) return NULL;
	char bb[64];
	memset(bb, 0, 64);
	memset(cBuff, 0, iLen);
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	sprintf(bb, "%.2i.%.2i.%.4i %.2i:%.2i:%.2i", timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900,
												timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	if (strlen(bb) < iLen) strcat(cBuff, bb); else memcpy(cBuff, bb, iLen - 1);
	return cBuff;						
}

char GetShowMenu()
{
	unsigned int ret;
	DBG_MUTEX_LOCK(&system_mutex);								
	ret = cShowMenu;	
	DBG_MUTEX_UNLOCK(&system_mutex);								
	return ret;						
}

char GetNextFile(char* CurrentFile, int SizeBuff, char cRandom, int *iFileNum, int *iOrderCnt)
{		
	DBG_LOG_IN();
	char ret = 0;
	int n = 0;
	int iCheckCnt = 0;
	int value;
	int iUpdated = 0;
	*iFileNum = -1;
	*iOrderCnt = 0;
	char cMess = 0;
	
	DBG_MUTEX_LOCK(&system_mutex);	
	if (iListFilesCount == 0)
	{
		if (iListDirsCount) 
			ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_NEXT);
			else ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_FULL);
		if (ret == 0)
		{
			if (uiShowModeCur >= 2) uiShowModeCur = 1;
			dbgprintf(3, "No files in GetNextFile1 '%s'\n", cCurrentFileLocation);
			cMess = 1;
		}
		else
		{
			if (uiShowModeCur < 2) uiShowModeCur = 2;		
		}	
	}
	ret = 0;
	
	while(iListFilesCount)
	{
		if (cRandom)
		{
			//value = (rand() % iListFilesCount);
			value = GenRandomInt(iListFilesCount-1);
			//printf("random value: %i\n", value);
		}
		else
		{
			/*for (iCheckCnt = 0; iCheckCnt < iListFilesCount; iCheckCnt++)
			if (cListFiles[iCheckCnt].Flag == iListFilesOrderCnt) 
			{
				value = iCheckCnt;
				break;
			}	*/	
			value = iListFilesCurPos;
			if (iListFilesOrderCnt) value++;
			if ((value < 0) || (value >= iListFilesCount)) value = 0;
		}
		if (value < iListFilesCount)
		{
			while ((cListFiles[value].Flag != 0) && (iCheckCnt < iListFilesCount))
			{
				value++;
				if (value >= iListFilesCount) value = 0;
				iCheckCnt++;
				//printf("Correct choose files '%s' %i\n", cListFiles[value].Name, cListFiles[value].Flag);			
			}
			if (cListFiles[value].Flag != 0)
			{
				//printf("Need new files '%s'\n", cCurrentFileLocation);
				if ((iUpdated) || (UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_NEXT) == 0))
				{
					if (uiShowModeCur >= 2) uiShowModeCur = 1;
					dbgprintf(3, "No files in GetNextFile2 '%s'\n", cCurrentFileLocation);
					cMess = 1;
					break;
				}
				n = 0;
				iUpdated = 1;
			}
			else
			{
				//cFile = &cListFiles[value];
				//printf("file: %s\n", cBuff);
				ret = GetFileType(cListFiles[value].Name);
				//printf("file '%s' %i %i %i %i\n", cListFiles[value].Name, value, n, iListFilesCount, ret);				
				if (ret != 0)
				{
					*iOrderCnt = iListFilesOrderCnt + 1;
					*iFileNum = value;
				
					memset(CurrentFile, 0, SizeBuff);
					strcpy(CurrentFile, cCurrentFileLocation);
					if ((iListDirsCurPos >= 0) && (iListDirsCount))
					{
						strcat(CurrentFile, "/");					
						strcat(CurrentFile, cListDirs[iListDirsCurPos].Name);
					}
					strcat(CurrentFile, "/");
					strcat(CurrentFile, cListFiles[value].Name);
					iListFilesCurPos=value;
					DBG_MUTEX_UNLOCK(&system_mutex);
					if (pScreenMenu) RefreshPlayListMenu(pScreenMenu, 0);
					if (pWebMenu) RefreshPlayListMenu(pWebMenu, 0);
					DBG_MUTEX_LOCK(&system_mutex);
					break;
				}
				else 
				{
					cListFiles[value].Flag = -1;
					n++;
				}
				if (n >= iListFilesCount) 
				{
					//printf("new files '%s'\n", cCurrentFileLocation);				
					if ((iUpdated) || (UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_NEXT) == 0))
					{
						if (uiShowModeCur >= 2) uiShowModeCur = 1;
						dbgprintf(3, "No files in GetNextFile3 '%s'\n", cCurrentFileLocation);
						cMess = 1;
						break;
					}
					n = 0;
					iUpdated = 1;					
				}
			}		    
		}
		else break;		
	}
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	if (cMess == 1) ShowNewMessage("Нет файлов для воспроизведения");
	
	DBG_LOG_OUT();	
	return ret;
}

char GetPrevFile(char* CurrentFile, int SizeBuff, char cRandom, int *iFileNum,  int *iOrderCnt)
{	
	DBG_LOG_IN();
	char ret = 0;
	int iCheckCnt = 0;
	int value;
	
	*iFileNum = -1;
	*iOrderCnt = 0;
	
	DBG_MUTEX_LOCK(&system_mutex);	
	
	if (iListFilesCount != 0)
	{	
		if ((iListFilesCurPos >= 0) && (iListFilesCurPos < iListFilesCount)) cListFiles[iListFilesCurPos].Flag = 0;
		if (cRandom)
		{
			for (iCheckCnt = 0; iCheckCnt < iListFilesCount; iCheckCnt++)
				if (cListFiles[iCheckCnt].Flag == iListFilesOrderCnt) 
				{
					cListFiles[iCheckCnt].Flag = 0;
					break;
				}
			if (iListFilesOrderCnt) value = iListFilesOrderCnt - 1;
			for (iCheckCnt = 0; iCheckCnt < iListFilesCount; iCheckCnt++)
				if (cListFiles[iCheckCnt].Flag == value) 
				{
					value = iCheckCnt;
					break;
				}
			if (iCheckCnt == iListFilesCount) value = 0;
		}
		else
		{			
			if (iListFilesCurPos) value = iListFilesCurPos - 1;
				else value = iListFilesCount - 1;
		}
		
		//cFile = &cListFiles[value*MAX_FILE_LEN];
		//printf("file: %s\n", cBuff);
		ret = GetFileType(cListFiles[value].Name);
		if (ret != 0)
		{
			if (iListFilesOrderCnt) *iOrderCnt = iListFilesOrderCnt - 1; else *iOrderCnt = 1;
			*iFileNum = value;
		
			memset(CurrentFile, 0, SizeBuff);
			strcpy(CurrentFile, cCurrentFileLocation);
			if ((iListDirsCurPos >= 0) && (iListDirsCount))
			{
				strcat(CurrentFile, "/");					
				strcat(CurrentFile, cListDirs[iListDirsCurPos].Name);
			}
			strcat(CurrentFile, "/");
			strcat(CurrentFile, cListFiles[value].Name);
			iListFilesCurPos=value;
			DBG_MUTEX_UNLOCK(&system_mutex);
			if (pScreenMenu) RefreshPlayListMenu(pScreenMenu, 0);
			if (pWebMenu) RefreshPlayListMenu(pWebMenu, 0);
			DBG_MUTEX_LOCK(&system_mutex);
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);	
	DBG_LOG_OUT();
	return ret;
}

int GetFileType(char *FileName)
{
	int ret = 0;
	int len = strlen(FileName);
	
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".AAC") != 0)) ret = FILE_TYPE_MEDIA;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".AC3") != 0)) ret = FILE_TYPE_MEDIA;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".OGG") != 0)) ret = FILE_TYPE_MEDIA;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".MP4") != 0)) ret = FILE_TYPE_MEDIA;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".MP3") != 0)) ret = FILE_TYPE_MEDIA;				
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".MKV") != 0)) ret = FILE_TYPE_MEDIA;				
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".FLV") != 0)) ret = FILE_TYPE_MEDIA;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".JPG") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-5, ".JPEG") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-5, ".EXIF") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-5, ".TIFF") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".PNG") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".GIF") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".LZW") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".BMP") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".TGA") != 0)) ret = FILE_TYPE_IMAGE;
	if ((ret == 0) && (SearchStrInDataCaseIgn(FileName, len, len-4, ".PPM") != 0)) ret = FILE_TYPE_IMAGE;		
	return ret;
}

STREAM_INFO * AddCaptureStream(unsigned int uiType, unsigned int uiVidID, unsigned int uiAudID)
{
	STREAM_INFO *si = NULL;	
	DBG_MUTEX_LOCK(&system_mutex);
	//int ret = iStreamCnt;
	iStreamCnt++;
	SyncStream = (STREAM_INFO**)DBG_REALLOC(SyncStream, iStreamCnt * sizeof(STREAM_INFO*));
	SyncStream[iStreamCnt - 1] = (STREAM_INFO*)DBG_MALLOC(sizeof(STREAM_INFO));
	si = SyncStream[iStreamCnt - 1];
	memset(SyncStream[iStreamCnt - 1], 0, sizeof(STREAM_INFO));
	SyncStream[iStreamCnt - 1]->Num = iStreamCnt;
	pthread_mutex_init(&SyncStream[iStreamCnt - 1]->mutex, NULL);
	
	if ((uiType != CAPTURE_TYPE_MODULE) && (uiType != CAPTURE_TYPE_EVENT) && (uiType != CAPTURE_TYPE_ACTION))
	{
		tx_eventer_create(&SyncStream[iStreamCnt - 1]->run, 0);
		tx_eventer_create(&SyncStream[iStreamCnt - 1]->pevntV, 0);
		tx_eventer_create(&SyncStream[iStreamCnt - 1]->pevntA, 0);	
		tx_eventer_create(&SyncStream[iStreamCnt - 1]->pevntS, 1);	
		tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->pevntS, EVENT_VIDEO);
		tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->pevntS, EVENT_AUDIO);
		tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->pevntS, EVENT_START);
	}
	pthread_mutex_init(&SyncStream[iStreamCnt - 1]->CopyMutex, NULL);
	tx_eventer_create(&SyncStream[iStreamCnt - 1]->CopyEvent, 1);	
	tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->CopyEvent, WRT_EVENT_COPY_FILE_REJECT);
	tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->CopyEvent, WRT_EVENT_COPY_FILE_ACCEPT);
	tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->CopyEvent, WRT_EVENT_COPY_FILE_BUSY);
	tx_eventer_add_event(&SyncStream[iStreamCnt - 1]->CopyEvent, WRT_EVENT_COPY_FILE_DONE);
	
	SyncStream[iStreamCnt - 1]->Type = uiType;
	SyncStream[iStreamCnt - 1]->VidID = uiVidID;
	SyncStream[iStreamCnt - 1]->AudID = uiAudID;
	
	SyncStream[iStreamCnt - 1]->CurrentPath = (char*)DBG_MALLOC(MAX_PATH);
	memset(SyncStream[iStreamCnt - 1]->CurrentPath, 0, MAX_PATH);
	DBG_MUTEX_UNLOCK(&system_mutex);
	return si;
}

void ReleaseCaptureStreams()
{
	DBG_MUTEX_LOCK(&system_mutex);
	int n;
	for (n = 0; n < iStreamCnt; n++)
	{
		pthread_mutex_destroy(&SyncStream[n]->mutex);
		if ((SyncStream[n]->Type != CAPTURE_TYPE_MODULE) && (SyncStream[n]->Type != CAPTURE_TYPE_EVENT) && (SyncStream[n]->Type != CAPTURE_TYPE_ACTION))
		{
			tx_eventer_delete(&SyncStream[n]->run);
			tx_eventer_delete(&SyncStream[n]->pevntV);
			tx_eventer_delete(&SyncStream[n]->pevntA);
			tx_eventer_delete(&SyncStream[n]->pevntS);
		}		
		pthread_mutex_destroy(&SyncStream[n]->CopyMutex);
		tx_eventer_delete(&SyncStream[n]->CopyEvent);
		DBG_FREE(SyncStream[n]->CurrentPath);
		DBG_FREE(SyncStream[n]);
	}
	if (iStreamCnt) DBG_FREE(SyncStream);
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void AcceptFileInList(int iFileNum,  int iOrderCnt)
{
	if (iFileNum >= 0)
	{
		DBG_MUTEX_LOCK(&system_mutex);	
		cListFiles[iFileNum].Flag = iOrderCnt;
		DBG_MUTEX_UNLOCK(&system_mutex);	
	}
}

char GetSpecFile(char* CurrentFile, int SizeBuff, int *iFileNum,  int *iOrderCnt)
{	
	DBG_LOG_IN();
	char ret = 0;
	int value;
	
	*iFileNum = -1;
	*iOrderCnt = 0;
	
	DBG_MUTEX_LOCK(&system_mutex);	
	
	if (iListFilesCount != 0)
	{		
		value = iListFilesCurPos;
		if ((value < 0) || (value >= iListFilesCount)) value = 0;		
		
		//cFile = &cListFiles[value*MAX_FILE_LEN];
		//printf("file: %s\n", cBuff);
		ret = GetFileType(cListFiles[value].Name);
		if (ret != 0)
		{	
			if (cListFiles[value].Flag == 0) 
			{
				*iOrderCnt = iListFilesOrderCnt + 1;
				*iFileNum = value;
			}	
			memset(CurrentFile, 0, SizeBuff);
			strcpy(CurrentFile, cCurrentFileLocation);
			if ((iListDirsCurPos >= 0) && (iListDirsCount))
			{
				strcat(CurrentFile, "/");					
				strcat(CurrentFile, cListDirs[iListDirsCurPos].Name);
			}
			strcat(CurrentFile, "/");
			strcat(CurrentFile, cListFiles[value].Name);
			iListFilesCurPos=value;
			DBG_MUTEX_UNLOCK(&system_mutex);
			if (pScreenMenu) RefreshPlayListMenu(pScreenMenu, 0);
			if (pWebMenu) RefreshPlayListMenu(pWebMenu, 0);
			DBG_MUTEX_LOCK(&system_mutex);
		}
	}
	DBG_MUTEX_UNLOCK(&system_mutex);
	DBG_LOG_OUT();
	return ret;
}

char GetNextAlarmFile(char* CurrentFile, int SizeBuff)
{	
	DBG_LOG_IN();
	char ret = 0;
	int n = 0;
	int random_value;
	
	if (iListAlarmFilesCount != 0)
	do
	{
		//random_value = (rand() % iListFilesCount);
		random_value = GenRandomInt(iListAlarmFilesCount-1);
		//printf("random value: %i\n", random_value);
		if (random_value < iListAlarmFilesCount)
		{			
			//printf("file: %s\n", cBuff);
			ret = GetFileType(cListAlarmFiles[random_value].Name);
			if (ret != 0)
			{
				memset(CurrentFile, 0, SizeBuff);
				strcpy(CurrentFile, cCurrentAlarmDir);
				strcat(CurrentFile, "/");
				strcat(CurrentFile, cListAlarmFiles[random_value].Name);	
				break;
			}
			else n++;
			if (n >= iListAlarmFilesCount) break;
		}
		else break;		
	} while (1);
	DBG_LOG_OUT();
	return ret;
}

int GetRandomFile(char *cDir, char *cFile, unsigned int uiTimeFrom, unsigned int uiTimeTo)
{
	DIR *dir;
	struct dirent *dp;	
	int result = 0;
	
	dir = opendir(cDir);
	if (dir != NULL)
	{
		int iCnt = 0;
		while((dp=readdir(dir)) != NULL)
			if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0)) 
			{
				char cPath[MAX_FILE_LEN];
				memset(cPath, 0, MAX_FILE_LEN);
				strcpy(cPath, cDir);
				strcat(cPath, dp->d_name);
				
				struct tm file_time;
				if ((uiTimeFrom == uiTimeTo) || (GetFileCreationTime(cPath, &file_time)))
				{
					int iTimeFile = file_time.tm_hour * 100 + file_time.tm_min;
					if (((uiTimeFrom < uiTimeTo) && (uiTimeFrom <= iTimeFile) && (uiTimeTo >= iTimeFile)) ||
						((uiTimeFrom > uiTimeTo) && (uiTimeFrom >= iTimeFile) && (uiTimeTo <= iTimeFile)) ||
						(uiTimeFrom == uiTimeTo))
						{
							iCnt++;
						}
				}
			}
		closedir(dir);
		
		if (iCnt == 0) return 0;
			
		int iFilePos = GenRandomInt(iCnt-1);
		if ((iFilePos < 0) || (iFilePos >= iCnt))
		{
			dbgprintf(2, "Out of range GenRandomInt(%i) = %i\n", iCnt-1, iFilePos);
		}
		else
		{
			iCnt = 0;
			dir = opendir(cDir);
			if (dir != NULL)
			{
				while((dp=readdir(dir)) != NULL)
				{
					if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
					{
						char cPath[MAX_FILE_LEN];
						memset(cPath, 0, MAX_FILE_LEN);
						strcpy(cPath, cDir);
						strcat(cPath, dp->d_name);
				
						struct tm file_time;
						if ((uiTimeFrom == uiTimeTo) || (GetFileCreationTime(cPath, &file_time)))
						{
							int iTimeFile = file_time.tm_hour * 100 + file_time.tm_min;
							if ((uiTimeFrom == uiTimeTo) ||
								((uiTimeFrom < uiTimeTo) && (uiTimeFrom <= iTimeFile) && (uiTimeTo >= iTimeFile)) ||
								((uiTimeFrom > uiTimeTo) && (uiTimeFrom >= iTimeFile) && (uiTimeTo <= iTimeFile)))
								{
									if (iCnt == iFilePos)
									{
										strcpy(cFile, dp->d_name);
										result = 1;
										break;
									}
									iCnt++;
								}
						}
					}
				}
				closedir(dir);
			}
		}
	} else dbgprintf(2, "GetRandomFile: error, is not dir:%s\n", cDir);
	return result;
}

int GetMaxSizeFile(char *cDir, char *cFile, unsigned int uiTimeFrom, unsigned int uiTimeTo)
{
	DIR *dir;
	struct dirent *dp;	
	int result = 0;
	
	dir = opendir(cDir);
	if (dir != NULL)
	{
		unsigned int iSize = 0;
		unsigned int iMaxSize = 0;
		
		while((dp=readdir(dir)) != NULL)
		{
			if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
			{
				char cPath[MAX_FILE_LEN];
				memset(cPath, 0, MAX_FILE_LEN);
				strcpy(cPath, cDir);
				strcat(cPath, dp->d_name);				
				
				struct tm file_time;
				if ((uiTimeFrom == uiTimeTo) || (GetFileCreationTime(cPath, &file_time)))
				{
					int iTimeFile = file_time.tm_hour * 100 + file_time.tm_min;
					if ((uiTimeFrom == uiTimeTo) ||
						((uiTimeFrom < uiTimeTo) && (uiTimeFrom <= iTimeFile) && (uiTimeTo >= iTimeFile)) ||
						((uiTimeFrom > uiTimeTo) && (uiTimeFrom >= iTimeFile) && (uiTimeTo <= iTimeFile)))
						{
							iSize = GetFileSize(cPath);
							if (iSize > iMaxSize)
							{
								memset(cFile, 0, MAX_FILE_LEN);
								strcpy(cFile, dp->d_name);
								iMaxSize = iSize;
								result = 1;
							}
						}
				}
			}
		}
		closedir(dir);
	}
	return result;
}

int GetMinSizeFile(char *cDir, char *cFile, unsigned int uiTimeFrom, unsigned int uiTimeTo)
{
	DIR *dir;
	struct dirent *dp;	
	int result = 0;
	
	dir = opendir(cDir);
	if (dir != NULL)
	{
		unsigned int iSize = 0;
		unsigned int iMinSize = -1;
		
		while((dp=readdir(dir)) != NULL)
		{
			if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
			{
				char cPath[MAX_FILE_LEN];
				memset(cPath, 0, MAX_FILE_LEN);
				strcpy(cPath, cDir);
				strcat(cPath, dp->d_name);				
				
				struct tm file_time;
				if (GetFileCreationTime(cPath, &file_time))
				{
					int iTimeFile = file_time.tm_hour * 100 + file_time.tm_min;
					if ((uiTimeFrom == uiTimeTo) ||
						((uiTimeFrom < uiTimeTo) && (uiTimeFrom <= iTimeFile) && (uiTimeTo >= iTimeFile)) ||
						((uiTimeFrom > uiTimeTo) && (uiTimeFrom >= iTimeFile) && (uiTimeTo <= iTimeFile)))
						{
							iSize = GetFileSize(cPath);
							if (iSize < iMinSize)
							{
								memset(cFile, 0, MAX_FILE_LEN);
								strcpy(cFile, dp->d_name);
								iMinSize = iSize;
								result = 1;
							}
						}
				}
			}
		}
		closedir(dir);
	}
	return result;
}

void SendToArchive(STREAM_INFO *siArchive, struct tm *prev_time, unsigned int uiMode, unsigned int uiTimeFrom, 
							unsigned int uiTimeTo, char *cMainPath, char *cPrePath, char *cArchivePath, unsigned int uiDestType)
{
	if (uiMode == 0) return;
	
	{
		char sl[2];
		sl[0] = 0;
		sl[1] = 0;
		int iLen = strlen(cPrePath);
		if (iLen && (cPrePath[iLen - 1] != 47)) sl[0] = 47;
		char cDirName[MAX_FILE_LEN];
		memset(cDirName, 0, MAX_FILE_LEN);		
		sprintf(cDirName, "%s%s%s%04i/%s/%02i/", cMainPath, cPrePath, sl, prev_time->tm_year+1900, GetMonthName(prev_time->tm_mon+1),prev_time->tm_mday);
		
		char cArchName[MAX_FILE_LEN];
		memset(cArchName, 0, MAX_FILE_LEN);
		sprintf(cArchName, "%s%s%s%04i/%s/%02i/", cArchivePath, cPrePath, sl, prev_time->tm_year+1900, GetMonthName(prev_time->tm_mon+1),prev_time->tm_mday);
		char cFileName[MAX_FILE_LEN];
		memset(cFileName, 0, MAX_FILE_LEN);
		
		switch(uiMode)
		{
			case 1:	//random
				if (GetRandomFile(cDirName, cFileName, uiTimeFrom, uiTimeTo)) 
				{
					strcat(cDirName, cFileName);
					strcat(cArchName, cFileName);
					CopyFile(cDirName, cArchName, NULL, 0, (ucBackUpModeArchive == 2) ? 1 : 0, siArchive, 
								ucBackUpOrderLmtArchive, 
								ucBackUpOrderEnableArchive ? ucBackUpOrderAddrArchive : NULL, 
								uiBackUpOrderWaitCnclArchive, uiBackUpOrderWaitMessArchive, uiDestType);
				}				
				else
				{
					dbgprintf(2, "Error random copy to archive, file not found\n");
					dbgprintf(3, "Time interval: %i - %i\n", uiTimeFrom, uiTimeTo);
					dbgprintf(3, "Path: '%s'\n", cDirName);		
				}
				break;
			case 2: //maxsize
				if (GetMaxSizeFile(cDirName, cFileName, uiTimeFrom, uiTimeTo))
				{
					strcat(cDirName, cFileName);
					strcat(cArchName, cFileName);
					CopyFile(cDirName, cArchName, NULL, 0, (ucBackUpModeArchive == 2) ? 1 : 0, siArchive, 
								ucBackUpOrderLmtArchive, 
								ucBackUpOrderEnableArchive ? ucBackUpOrderAddrArchive : NULL, 
								uiBackUpOrderWaitCnclArchive, uiBackUpOrderWaitMessArchive, uiDestType);
				}
				else
				{
					dbgprintf(2, "Error maxsize copy to archive, file not found\n");
					dbgprintf(3, "Time interval: %i - %i\n", uiTimeFrom, uiTimeTo);
					dbgprintf(3, "Path: '%s'\n", cDirName);		
				}
				break;
			case 3: //minsize
				if (GetMinSizeFile(cDirName, cFileName, uiTimeFrom, uiTimeTo))
				{
					strcat(cDirName, cFileName);
					strcat(cArchName, cFileName);									
					CopyFile(cDirName, cArchName, NULL, 0, (ucBackUpModeArchive == 2) ? 1 : 0, siArchive, 
								ucBackUpOrderLmtArchive, 
								ucBackUpOrderEnableArchive ? ucBackUpOrderAddrArchive : NULL, 
								uiBackUpOrderWaitCnclArchive, uiBackUpOrderWaitMessArchive, uiDestType);
				}
				else
				{
					dbgprintf(2, "Error minsize copy to archive, file not found\n");
					dbgprintf(3, "Time interval: %i - %i\n", uiTimeFrom, uiTimeTo);
					dbgprintf(3, "Path: '%s'\n", cDirName);		
				}
				break;
			default:
				break;				
		}
	}
}

void SendFilesToArchive(STREAM_INFO *siArchive, struct tm *prev_time)
{
	time_t rawtime;
	struct tm now_time;
	time(&rawtime);
	localtime_r(&rawtime, &now_time);
	
	if ((now_time.tm_year - prev_time->tm_year) > 1) memcpy(prev_time, &now_time, sizeof(struct tm));
	
	if ((now_time.tm_mday != prev_time->tm_mday) && now_time.tm_hour)
	{
		DBG_MUTEX_LOCK(&system_mutex);
		if (ucBackUpModeArchive != 0)
		{
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeFull, ucMediaArchiveTimeFromFull, 
							ucMediaArchiveTimeToFull, cMediaCapturePathFull, ucCaptureStartPathFull, 
							cMediaArchivePathFull, WRT_TYPE_ARCHIVE_FULL);
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeSlow, ucMediaArchiveTimeFromSlow, 
							ucMediaArchiveTimeToSlow, cMediaCapturePathSlow, ucCaptureStartPathSlow, 
							cMediaArchivePathSlow, WRT_TYPE_ARCHIVE_SLOW);
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeDiff, ucMediaArchiveTimeFromDiff, 
							ucMediaArchiveTimeToDiff, cMediaCapturePathDiff, ucCaptureStartPathDiff, 
							cMediaArchivePathDiff, WRT_TYPE_ARCHIVE_DIFF);
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeAudio, ucMediaArchiveTimeFromAudio, 
							ucMediaArchiveTimeToAudio, cMediaCapturePathAudio, ucCaptureStartPathAudio, 
							cMediaArchivePathAudio, WRT_TYPE_ARCHIVE_AUDIO);
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeStatuses, ucMediaArchiveTimeFromStatuses, 
							ucMediaArchiveTimeToStatuses, cMediaCapturePathStatuses, ucCaptureStartPathStatuses, 
							cMediaArchivePathStatuses, WRT_TYPE_ARCHIVE_STATUSES);
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeEvents, ucMediaArchiveTimeFromEvents, 
							ucMediaArchiveTimeToEvents, cMediaCapturePathEvents, ucCaptureStartPathEvents, 
							cMediaArchivePathEvents, WRT_TYPE_ARCHIVE_EVENTS);
			SendToArchive(siArchive, prev_time, ucMediaArchiveModeActions, ucMediaArchiveTimeFromActions, 
							ucMediaArchiveTimeToActions, cMediaCapturePathActions, ucCaptureStartPathActions, 
							cMediaArchivePathActions, WRT_TYPE_ARCHIVE_ACTIONS);
		}
		DBG_MUTEX_UNLOCK(&system_mutex);
		memcpy(prev_time, &now_time, sizeof(struct tm));
	}
}

void* thread_CopyFile(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	misc_buffer *mBuff = (misc_buffer*)pData;
	if (mBuff->clock == 0) 
	{
		DBG_MUTEX_LOCK(&system_mutex);
		uiThreadCopyStatus++;
		DBG_MUTEX_UNLOCK(&system_mutex);
	}
	char *cAddrOrderer = mBuff->void_data2;
	struct sockaddr_in  sAddress;
		
	//int iStreamID = (int)mBuff->uidata[0];
	STREAM_INFO *pStream = mBuff->void_data3;
	
	unsigned int iStreamID;
	unsigned int uiOrderNum = 0;
	int iMainStatus = 1;
	int iSendedPackets = 0;
	unsigned int uiData[8];
	unsigned int uiFrameNum = 0;
	unsigned int uiFullFileSize = GetFileSize(mBuff->data);
	
	unsigned int uiMaxWaitTime = mBuff->uidata[2];
	unsigned int uiMessWaitTime = mBuff->uidata[3];
	unsigned int uiDestType = mBuff->uidata[4];
	int iTimeout = 0;
	int ret;	
	
	while (cAddrOrderer && pStream)
	{
		DBG_MUTEX_LOCK(&pStream->mutex);
		iStreamID = pStream->Num;
		DBG_MUTEX_UNLOCK(&pStream->mutex);
		sAddress.sin_addr.s_addr = inet_addr(cAddrOrderer);
		sAddress.sin_port = htons(TCP_PORT);
		sAddress.sin_family = AF_INET;
		unsigned char ucOrderLmt = mBuff->uidata[1];	
		DBG_MUTEX_LOCK(&pStream->CopyMutex);
		if (ucOrderLmt < (pStream->CopyCurOrder - pStream->CopyNextOrder))
		{
			dbgprintf(2, "Order copy overfull (%i/%i), cancel copy\n", ucOrderLmt, pStream->CopyCurOrder - pStream->CopyNextOrder);
			DBG_MUTEX_UNLOCK(&pStream->CopyMutex);
			iMainStatus = 0;
			break;
		}
		uiOrderNum = pStream->CopyCurOrder;
		pStream->CopyCurOrder++;		
		
		if ((pStream->CopyStatus != 0) || (uiOrderNum != pStream->CopyNextOrder)) 
			dbgprintf(3, "Wait copy order my:%i current:%i\n", uiOrderNum, pStream->CopyNextOrder);
			
		while (1)
		{
			if ((pStream->CopyStatus == 0) && (uiOrderNum == pStream->CopyNextOrder)) 
			{
				pStream->CopyStatus = 1;
				break;
			}
			DBG_MUTEX_UNLOCK(&pStream->CopyMutex);
			usleep(1000000);
			DBG_MUTEX_LOCK(&pStream->CopyMutex);
			iTimeout++;
			if (iTimeout >= uiMaxWaitTime) 
			{
				pStream->CopyNextOrder++;
				break;
			}
		}
		
		DBG_MUTEX_UNLOCK(&pStream->CopyMutex);
		
		if (iTimeout >= uiMaxWaitTime)
		{
			dbgprintf(2, "Timeout wait copy local order, break copy %i>%i\n", iTimeout, uiMaxWaitTime);
			dbgprintf(3, "\t uiOrderNum %i\n", uiOrderNum);
			dbgprintf(3, "\t pStream->CopyNextOrder %i\n", pStream->CopyNextOrder);
			iMainStatus = 0;
			break;
		}
		if (iTimeout >= uiMessWaitTime) dbgprintf(2, "Long time wait copy local order %i>%i\n", iTimeout, uiMessWaitTime);
		
		//dbgprintf(3, "Begin copy order exist\n");
				
		tx_eventer_clear(&pStream->CopyEvent);		
		unsigned int uiEvent = 0;
		
		while(1)
		{
			//dbgprintf(3, "CopeFile: request priority\n");
			uiData[0] = iStreamID;
			uiData[1] = uiFullFileSize;
			uiData[2] = uiDestType;
			if (SendTCPMessage(TYPE_MESSAGE_COPY_FILE_GET_ORDER, (char*)uiData, sizeof(unsigned int) * 3, (char*)mBuff->void_data, strlen((char*)mBuff->void_data), &sAddress))
			{
				uiEvent = 0;
				ret = tx_eventer_recv(&pStream->CopyEvent, &uiEvent, 15000, 0);
				if (ret != 0)
				{
					if (uiEvent == WRT_EVENT_COPY_FILE_REJECT) break;
					if (uiEvent == WRT_EVENT_COPY_FILE_ACCEPT) break;
					if (uiEvent == WRT_EVENT_COPY_FILE_BUSY) 
					{
						//dbgprintf(3, "EVENT_BUSY_COPY_ORDER\n");
						iTimeout += 3;
						usleep(3000000);
					}
					//iTimeout--;
				}
				else
				{
					dbgprintf(2, "No response on COPY_FILE_GET_ORDER\n");
					iTimeout += 15;
					if (iTimeout >= uiMaxWaitTime) break;
				}
			} 
			else 
			{
				usleep(10000000);
				iTimeout = uiMaxWaitTime;
			}
			if (iTimeout >= uiMaxWaitTime) break;
		}
		if (iTimeout >= uiMaxWaitTime) 
		{
			DBG_MUTEX_LOCK(&pStream->CopyMutex);
			pStream->CopyNextOrder++;
			pStream->CopyStatus = 0;
			DBG_MUTEX_UNLOCK(&pStream->CopyMutex);	
			uiEvent = 0;
			cAddrOrderer = NULL;
			iMainStatus = 0;
			SendTCPMessage(TYPE_MESSAGE_COPY_FILE_RELEASE, (char*)&iStreamID, 4, NULL, 0, &sAddress);
			dbgprintf(2, "Timeout wait copy server order, break copy\n");
		}
		if (iTimeout != uiMaxWaitTime) dbgprintf((iTimeout > uiMessWaitTime) ? 2 : 4, "Time wait copy server order: %i sec.\n", iTimeout);
			
		
		if (uiEvent == WRT_EVENT_COPY_FILE_REJECT)
		{			
			dbgprintf(2, "Reject copy main order, break copy\n");
			
			DBG_MUTEX_LOCK(&pStream->CopyMutex);
			pStream->CopyNextOrder++;
			pStream->CopyStatus = 0;
			DBG_MUTEX_UNLOCK(&pStream->CopyMutex);			
			
			iMainStatus = 0;
			break;
		}
		break;
	}
	
	
	FILE *fFrom;
	FILE *fTo;
		
	if (iMainStatus)
	{
		if ((fFrom = fopen(mBuff->data,"rb")) == NULL)
		{
			dbgprintf(1,"Error open read file for copy:%s\n", mBuff->data);
						
			if (cAddrOrderer && pStream)
			{
				DBG_MUTEX_LOCK(&pStream->CopyMutex);
				pStream->CopyNextOrder++;
				pStream->CopyStatus = 0;
				DBG_MUTEX_UNLOCK(&pStream->CopyMutex);
				SendTCPMessage(TYPE_MESSAGE_COPY_FILE_RELEASE, (char*)&iStreamID, 4, NULL, 0, &sAddress);
			}				
			iMainStatus = 0;
		}
	}
	
	
	if (iMainStatus && (!cAddrOrderer || !pStream))
	{
		CreateDirectoryFromPath(mBuff->void_data, 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		
		if ((fTo = fopen(mBuff->void_data,"wb")) == NULL)
		{
			dbgprintf(1,"Error open write file for copy:%s\n", (char*)mBuff->void_data);
			fclose (fFrom);
			iMainStatus = 0;
		}		
	}
	
	char *cCopyBuff;
	if (iMainStatus) cCopyBuff = DBG_MALLOC(FILE_PACKET_BUFFER_SIZE);
	int len, wrtd, transfertimeout;
	unsigned int uiEvent;
	
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	len = 1;
	while ((iMainStatus == 1) && (len || iSendedPackets))
	{		
		previous_ms = 0;
		get_ms(&previous_ms);
		len = fread(cCopyBuff, 1, FILE_PACKET_BUFFER_SIZE, fFrom);
		wrtd = 0;
		transfertimeout = iTimeout;
		
		while(len - wrtd)  
		{	
			if (cAddrOrderer && pStream)
			{
				uiData[0] = iStreamID;
				uiData[1] = uiFrameNum;
				if (!SendTCPMessage(TYPE_MESSAGE_COPY_FILE_DATA, (char*)uiData, sizeof(unsigned int)*2, cCopyBuff, len, &sAddress))
				{					
					//transfertimeout--;
					iTimeout++;					
					if (iTimeout > uiMessWaitTime)
					{
						len = 0;
						iSendedPackets = 0;
						dbgprintf(2, "Error transfer file, timeout %is\n", uiMessWaitTime);
						break;
					}
					usleep(1000000);
				} 
				else 
				{
					//printf("Sended %i\n", uiFrameNum);				
					wrtd += len;
					iSendedPackets++;
					uiFrameNum++;
				}
			}
			else
			{
				ret = fwrite(&cCopyBuff[wrtd], 1, len - wrtd, fTo);
				if (ret == 0) break;
				wrtd += ret;
			}
		} 
		if (transfertimeout != iTimeout) dbgprintf(3, "Transfer file slowly, waittime:%is\n", iTimeout - transfertimeout);
		
		//printf("Sended DATA %i %i\n", wrtd, len);
		if (cAddrOrderer && pStream && iSendedPackets && ((iSendedPackets == 2) || ((iSendedPackets < 2) && (len == 0))))
		{			
			//printf("WAIT_TRANSFER Num:%i Readed:%ims 60s %i\n", uiFrameNum - 1, (unsigned int)get_ms(&previous_ms), (int)pStream);										
			previous_ms = 0;
			get_ms(&previous_ms);
			uiEvent = 0;
			ret = tx_eventer_recv(&pStream->CopyEvent, &uiEvent, 60000, 0);
			if (ret != 0)
			{
				if (uiEvent == WRT_EVENT_COPY_FILE_REJECT)
				{
					dbgprintf(2, "thread_CopyFile: Error copy file, aborted\n");
					iMainStatus = 2;
					break;
				}
				else
				if (uiEvent == WRT_EVENT_COPY_FILE_DONE)
				{
					//printf("TRANSFER_DONE Pack %i  %ims\n", iSendedPackets, (unsigned int)get_ms(&previous_ms));
					iSendedPackets--;
					//printf("Ok copy file %i bytes\n", wrtd);
				} 
				else
				if (uiEvent == WRT_EVENT_COPY_FILE_BUSY) 
				{
					dbgprintf(2, "thread_CopyFile: skipped WRT_EVENT_COPY_FILE_BUSY\n");
				}
				else
				{
					dbgprintf(2, "thread_CopyFile: Unknown evnt transfer data:%i\n", uiEvent);
					dbgprintf(3, "\t cAddrOrderer:%s\n", cAddrOrderer);
					dbgprintf(3, "\t pStream:%i\n", (int)pStream);
					dbgprintf(3, "\ti iSendedPackets:%i\n", iSendedPackets);
					dbgprintf(3, "\t len:%i\n", len);
					if ((len == 0) || (iSendedPackets == 0))
					{
						iMainStatus = 2;
						break;
					}
				}
			}
			else
			{
				dbgprintf(2, "Timeout %i %i copy file %ims (slow connect?), get cancel\n", uiFrameNum - 1, iSendedPackets, (unsigned int)get_ms(&previous_ms));
				iMainStatus = 2;
				break;
			}
		}
	}
	
	if (iMainStatus) DBG_FREE(cCopyBuff);
	
	if (iMainStatus)
	{
		if (len || (feof(fFrom) == 0)) iMainStatus = 0;
		fclose(fFrom);
		if (!cAddrOrderer || !pStream) fclose(fTo);
		//printf("Done copy %s %s\n", mBuff->data, mBuff->void_data);
	}
		
	if (mBuff->flag)
	{
		if (iMainStatus == 1) 
		{
			if (remove(mBuff->data) != 0) dbgprintf(2, "Error delete %s '%s'\n", (errno == ENOENT) ? "not found" : "no access", mBuff->data);
		}
		else
		{
			if (mBuff->void_data4)
			{
				dbgprintf(3, "Error transfer file, save to spec place\n");
				CreateDirectoryFromPath((char*)mBuff->void_data4, 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				if (link(mBuff->data, mBuff->void_data4) == 0)
				{
					unlink(mBuff->data);
					dbgprintf(3, "\tFrom:'%s'\n", mBuff->data);
					dbgprintf(3, "\tTo:'%s'\n", (char*)mBuff->void_data4);
				}
				else 
				{
					dbgprintf(2, "\tError save to spec place %i, %s\n", errno, strerror(errno));
					dbgprintf(3, "\tFile:'%s'\n", mBuff->data);
					dbgprintf(3, "\tPlace:'%s'\n", (char*)mBuff->void_data4);
				}
			}
		}
	}
		
	DBG_FREE(mBuff->data);
	DBG_FREE(mBuff->void_data);
	if (mBuff->void_data2) DBG_FREE(mBuff->void_data2);
	if (mBuff->void_data4) DBG_FREE(mBuff->void_data4);
	
	if (iMainStatus && cAddrOrderer && pStream)
	{
		DBG_MUTEX_LOCK(&pStream->CopyMutex);
		pStream->CopyNextOrder++;
		pStream->CopyStatus = 0;
		DBG_MUTEX_UNLOCK(&pStream->CopyMutex);
		SendTCPMessage(TYPE_MESSAGE_COPY_FILE_RELEASE, (char*)&iStreamID, 4, NULL, 0, &sAddress);
	}
		
	if (mBuff->clock == 0)
	{
		DBG_MUTEX_LOCK(&system_mutex);
		uiThreadCopyStatus--;
		DBG_MUTEX_UNLOCK(&system_mutex);
	}
	DBG_FREE(mBuff);
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)iMainStatus;
}

void AddMessageInList(char *cMessage, int iMessageLen, unsigned int cAddr)
{
	char * cTempBuffer1 = (char*) DBG_MALLOC(iMessageLen + 1);
	char * cTempBuffer2 = (char*) DBG_MALLOC(iMessageLen + 64);
	memset(cTempBuffer1, 0, iMessageLen + 1);
	memset(cTempBuffer2, 0, iMessageLen + 64);
	memcpy(cTempBuffer1, cMessage, iMessageLen);
	unsigned int uiSoundID = 0;
	int iPlaySound = 0;
	int iAddressNum = cAddr >> 24;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	
	
	//sprintf(cTempBuffer2, "%s", cTempBuffer1);
	if (cAddr != 0) 
		sprintf(cTempBuffer2, "[%i:%i](%i) %s", timeinfo.tm_hour,timeinfo.tm_min, iAddressNum, cTempBuffer1);
		else sprintf(cTempBuffer2, "[%i:%i] %s", timeinfo.tm_hour,timeinfo.tm_min, cTempBuffer1);
/*	}
	else
	{
		sprintf(cTempBuffer2, "[%i.%i.%i %i:%i:%i] %s", timeinfo.tm_mday,
																				timeinfo.tm_mon+1,
																				timeinfo.tm_year+1900,
																				timeinfo.tm_hour,
																				timeinfo.tm_min,
																				timeinfo.tm_sec,
																				cTempBuffer1);
	}*/
	int ret = strlen(cTempBuffer2);							
	DBG_MUTEX_LOCK(&message_mutex);
	if (iMessagesListCnt < MESSAGES_MAX_CNT)
	{
		iMessagesListCnt++;		
		cMessagesList = (char*)DBG_REALLOC(cMessagesList, iMessagesListCnt * MESSAGES_MAX_LEN);								
	} else memmove(cMessagesList, &cMessagesList[MESSAGES_MAX_LEN], MESSAGES_MAX_LEN * (MESSAGES_MAX_CNT-1));
	memset(&cMessagesList[(iMessagesListCnt-1) * MESSAGES_MAX_LEN],0, MESSAGES_MAX_LEN);
	if (ret >= MESSAGES_MAX_LEN) memcpy(&cMessagesList[(iMessagesListCnt-1) * MESSAGES_MAX_LEN], cTempBuffer2, MESSAGES_MAX_LEN - 1);
		else memcpy(&cMessagesList[(iMessagesListCnt-1) * MESSAGES_MAX_LEN], cTempBuffer2, ret);
	iMessageListChanged = 1;
	if ((unsigned int)get_ms(&iSoundMessageTimer) > uiSoundMessagePauseSize)
	{		
		iSoundMessageTimer = 0;
		get_ms(&iSoundMessageTimer);
		uiSoundID = uiSoundMessageID;
		iPlaySound = 1;
	}
	DBG_MUTEX_UNLOCK(&message_mutex);
	if (cTempBuffer2[strlen(cTempBuffer2) - 1] < 32) 
		dbgprintf(4,"Message: %s",cTempBuffer2);
		else dbgprintf(4,"Message: %s\n",cTempBuffer2);
	//log_print(cTempBuffer2);
	DBG_FREE(cTempBuffer1);
	DBG_FREE(cTempBuffer2);
	if ((uiSoundID != 0) && iPlaySound) Action_PlaySound(uiSoundID, 0);
	ShowNewMessage(cMessage);
}

void CreateModuleEvent(MODULE_INFO *pModule, int iStatusNum, int iValue)
{
	int i = 1 << iStatusNum;
	AddModuleEventInList(pModule->ID, iStatusNum + 1, iValue, NULL, 0, 0);
	if (pModule->Global & i)
	{
		DBG_MUTEX_LOCK(&system_mutex);
		unsigned char cBcTCP = ucBroadCastTCP;
		DBG_MUTEX_UNLOCK(&system_mutex);
		pModule->Status[iStatusNum] = iValue;
		SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)pModule, sizeof(MODULE_INFO_TRANSFER), (char*)&i, sizeof(i));
		pModule->Status[iStatusNum] = 0;									
	}
}

void AddModuleEventInList(unsigned int uiID, int iNumSens, int iStatus, char* cName, unsigned int uiNameLen, char cExecNow)
{
	DBG_LOG_IN();
	int iActCnt;
	DBG_MUTEX_LOCK(&evntaction_mutex);
	iActCnt = iActionCnt;
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	
	if (iActCnt == 0)
	{
		DBG_MUTEX_LOCK(&skipevent_mutex);
		if (iSkipEventListCnt < iSkipEventMaxCnt) iActCnt = 1;
		DBG_MUTEX_UNLOCK(&skipevent_mutex);
	}
		
	if ((iActCnt != 0) || (cExecNow))
	{
		DBG_MUTEX_LOCK(&modevent_mutex);	
		int n;
		for (n = 0; n != iModuleEventCnt; n++) if (meModuleEventList[n].New == 0) break;
		if ((n == iModuleEventCnt) && (iModuleEventCnt < MODULE_ENENT_CNT_MAX))
		{
			iModuleEventCnt++;
			meModuleEventList = (MODULE_EVENT*)DBG_REALLOC(meModuleEventList, sizeof(MODULE_EVENT)*iModuleEventCnt);
			memset(&meModuleEventList[iModuleEventCnt-1], 0, sizeof(MODULE_EVENT));
		}
		if (n == iModuleEventCnt)
		{
			dbgprintf(2, "Overfull event list, drop event\n");
		}
		else
		{
			meModuleEventList[n].New = 1;
			meModuleEventList[n].ID = uiID;
			meModuleEventList[n].SubNumber = iNumSens;
			meModuleEventList[n].Status = iStatus;
			memset(meModuleEventList[n].Name, 0, 64);
			if ((cName) && (uiNameLen > 0))
			{
				if (uiNameLen > 63) memcpy(meModuleEventList[n].Name, cName, 63);
					else memcpy(meModuleEventList[n].Name, cName, uiNameLen);
			}
			meModuleEventList[n].ExecNow = cExecNow;
		}
		DBG_MUTEX_UNLOCK(&modevent_mutex);	
	}
	tx_eventer_send_event(&modevent_evnt, EVENT_START);
	
	DBG_LOG_OUT();	
}

void FillModuleEventList(MODULE_EVENT **meList, unsigned int *uiCount, unsigned int uiID, int iNumSens, int iStatus, char* cName, unsigned int uiNameLen, char cExecNow)
{
	int iLen = *uiCount;
	MODULE_EVENT *pList = *meList;
	iLen++;
	pList = (MODULE_EVENT*)DBG_REALLOC(pList, sizeof(MODULE_EVENT)*iLen);
	memset(&pList[iLen-1], 0, sizeof(MODULE_EVENT));
	
	pList[iLen-1].New = 1;
	pList[iLen-1].ID = uiID;
	pList[iLen-1].SubNumber = iNumSens;
	pList[iLen-1].Status = iStatus;
	pList[iLen-1].ExecNow = cExecNow;
	
	memset(pList[iLen-1].Name, 0, 64);
	if ((cName) && (uiNameLen > 0))
	{
		if (uiNameLen > 63) memcpy(pList[iLen-1].Name, cName, 63);
			else memcpy(pList[iLen-1].Name, cName, uiNameLen);
	}
	
	*uiCount = iLen;
	*meList = pList;
}

void AddModuleEvents(MODULE_EVENT *meList, unsigned int uiCount)
{
	DBG_LOG_IN();
	int iActCnt;
	DBG_MUTEX_LOCK(&evntaction_mutex);
	iActCnt = iActionCnt;
	DBG_MUTEX_UNLOCK(&evntaction_mutex);
	
	if (iActCnt == 0)
	{
		DBG_MUTEX_LOCK(&skipevent_mutex);
		if (iSkipEventListCnt < iSkipEventMaxCnt) iActCnt = 1;
		DBG_MUTEX_UNLOCK(&skipevent_mutex);
	}
	
	int clk;
	int n;
			
	DBG_MUTEX_LOCK(&modevent_mutex);		
	for (clk = 0; clk < uiCount; clk++)
	{
		if ((iActCnt != 0) || (meList[clk].ExecNow))
		{
			for (n = 0; n < iModuleEventCnt; n++) if (meModuleEventList[n].New == 0) break;
			if ((n == iModuleEventCnt) && (iModuleEventCnt < MODULE_ENENT_CNT_MAX))
			{
				iModuleEventCnt++;
				meModuleEventList = (MODULE_EVENT*)DBG_REALLOC(meModuleEventList, sizeof(MODULE_EVENT)*iModuleEventCnt);
				memset(&meModuleEventList[iModuleEventCnt-1], 0, sizeof(MODULE_EVENT));
			}
			if (n == iModuleEventCnt)
			{
				dbgprintf(2, "Overfull event list, drop event\n");
				break;
			}
			meModuleEventList[n].New = 1;
			meModuleEventList[n].ID = meList[clk].ID;
			meModuleEventList[n].SubNumber = meList[clk].SubNumber;
			meModuleEventList[n].Status = meList[clk].Status;
			memcpy(meModuleEventList[n].Name, meList[clk].Name, 64);
			meModuleEventList[n].ExecNow = meList[clk].ExecNow;
		}
	}
	DBG_MUTEX_UNLOCK(&modevent_mutex);	
	
	tx_eventer_send_event(&modevent_evnt, EVENT_START);
	
	DBG_LOG_OUT();	
}

void ClearSkipIrCodeList()
{
	DBG_MUTEX_LOCK(&skipircode_mutex);
	if (iSkipIrCodeListCnt != 0) 
	{
		DBG_FREE(cSkipIrCodeList);
		cSkipIrCodeList = NULL;
		iSkipIrCodeListCnt = 0;	
	}
	DBG_MUTEX_UNLOCK(&skipircode_mutex);	
}

void AddSkipIrCodeInList(uint16_t *pCode, unsigned int uiLen)
{
	DBG_MUTEX_LOCK(&skipircode_mutex);
	if (uiLen <= MAX_IRCOMMAND_LEN)
	{
		if (iSkipIrCodeListCnt < iSkipIrCodeMaxCnt)
		{
			iSkipIrCodeListCnt++;		
			cSkipIrCodeList = (IR_COMMAND_TYPE*)DBG_REALLOC(cSkipIrCodeList, iSkipIrCodeListCnt * sizeof(IR_COMMAND_TYPE));								
			memset(&cSkipIrCodeList[iSkipIrCodeListCnt-1], 0, sizeof(IR_COMMAND_TYPE));
			cSkipIrCodeList[iSkipIrCodeListCnt-1].Len = uiLen;
			int n;
			for (n = 0; n < uiLen; n++)
				cSkipIrCodeList[iSkipIrCodeListCnt-1].Code[n] = pCode[n];
		}
	//	else memmove(cSkipIrCodeList, &cSkipIrCodeList[1], sizeof(SECURITY_KEY_INFO) * (ALIENKEY_MAX_CNT-1));	
	}
	DBG_MUTEX_UNLOCK(&skipircode_mutex);	
}
	
void ClearPtzSettingsList()
{
	DBG_MUTEX_LOCK(&ptz_mutex);
	if (iPtzSettingsListCnt != 0) 
	{
		DBG_FREE(psiPtzSettingsList);
		psiPtzSettingsList = NULL;
		iPtzSettingsListCnt = 0;	
	}
	DBG_MUTEX_UNLOCK(&ptz_mutex);	
}

int AddPtzSettingInList(char *cName, float fX, float fY, float fZoom, float fFocus)
{
	int result = 0;
	DBG_MUTEX_LOCK(&ptz_mutex);
	if (iPtzSettingsListCnt < PTZ_SETTINGS_MAX_CNT)
	{
		int i;
		for (i = 0; i < iPtzSettingsListCnt; i++)
		{
			if (psiPtzSettingsList[i].Used == 0) break;
		}
		if (i == iPtzSettingsListCnt)
		{
			iPtzSettingsListCnt++;		
			psiPtzSettingsList = (PTZ_SET_INFO*)DBG_REALLOC(psiPtzSettingsList, iPtzSettingsListCnt * sizeof(PTZ_SET_INFO));
		}			
		memset(&psiPtzSettingsList[i], 0, sizeof(PTZ_SET_INFO));
		int len = strlen(cName);
		memset(psiPtzSettingsList[i].Name, 0, 64);
		if (len < 64) memcpy(psiPtzSettingsList[i].Name, cName, len);
			else memcpy(psiPtzSettingsList[i].Name, cName, 63);
		psiPtzSettingsList[i].Used = 1;
		psiPtzSettingsList[i].PanTiltX = fX;
		psiPtzSettingsList[i].PanTiltY = fY;
		psiPtzSettingsList[i].Zoom = fZoom;
		psiPtzSettingsList[i].Focus = fFocus;
		result = 1;
	}	
	DBG_MUTEX_UNLOCK(&ptz_mutex);
	if (result) SavePTZs();
	return result;
}

int SetPtzSettingInList(char *cName, float fX, float fY, float fZoom, float fFocus)
{
	int result = -1;
	DBG_MUTEX_LOCK(&ptz_mutex);
	
	int i;
	for (i = 0; i < iPtzSettingsListCnt; i++)
	{
		if (psiPtzSettingsList[i].Used)
		{
			if (strcmp(cName, psiPtzSettingsList[i].Name) == 0) 
			{
				psiPtzSettingsList[i].PanTiltX = fX;
				psiPtzSettingsList[i].PanTiltY = fY;
				psiPtzSettingsList[i].Zoom = fZoom;
				psiPtzSettingsList[i].Focus = fFocus;
				result = i;
				break;	
			}					
		}
	}
	if ((result == -1) && (iPtzSettingsListCnt < PTZ_SETTINGS_MAX_CNT))	
	{
		for (i = 0; i < iPtzSettingsListCnt; i++)
		{
			if (psiPtzSettingsList[i].Used == 0) break;
		}
		if (i == iPtzSettingsListCnt)
		{
			iPtzSettingsListCnt++;		
			psiPtzSettingsList = (PTZ_SET_INFO*)DBG_REALLOC(psiPtzSettingsList, iPtzSettingsListCnt * sizeof(PTZ_SET_INFO));
		}			
		memset(&psiPtzSettingsList[i], 0, sizeof(PTZ_SET_INFO));
		int len = strlen(cName);
		memset(psiPtzSettingsList[i].Name, 0, 64);
		if (len < 64) memcpy(psiPtzSettingsList[i].Name, cName, len);
			else memcpy(psiPtzSettingsList[i].Name, cName, 63);
		psiPtzSettingsList[i].Used = 1;
		psiPtzSettingsList[i].PanTiltX = fX;
		psiPtzSettingsList[i].PanTiltY = fY;
		psiPtzSettingsList[i].Zoom = fZoom;
		psiPtzSettingsList[i].Focus = fFocus;
		result = i;
	}		
		
	DBG_MUTEX_UNLOCK(&ptz_mutex);
	
	if (result != -1) SavePTZs();
	return result;
}

int DelPtzSettingInList(char *cName)
{
	int lenname = strlen(cName);
	int lenset = strlen("PRESET_");
	if (lenname <= lenset) return 0;
	unsigned int iNumSet = Str2Int(&cName[lenset]);
	
	int result = 0;
	DBG_MUTEX_LOCK(&ptz_mutex);
	if (iNumSet < iPtzSettingsListCnt)
	{
		if (psiPtzSettingsList[iNumSet].Used)
		{
			psiPtzSettingsList[iNumSet].Used = 0;
			result = 1;	
		}
	}	
	DBG_MUTEX_UNLOCK(&ptz_mutex);
	if (result) SavePTZs();
	return result;
}

void ClearAlienKeyList()
{
	DBG_MUTEX_LOCK(&alienkey_mutex);
	if (iAlienKeyListCnt != 0) 
	{
		DBG_FREE(cAlienKeyList);
		cAlienKeyList = NULL;
		iAlienKeyListCnt = 0;	
	}
	DBG_MUTEX_UNLOCK(&alienkey_mutex);	
}

void AddAlienKeyInList(unsigned char *Serial, unsigned int Len)
{
	DBG_MUTEX_LOCK(&alienkey_mutex);
	if (iAlienKeyListCnt < iAlienKeyMaxCnt)
	{
		int i, k;
		for (i = 0; i < iAlienKeyListCnt; i++)
		{
			for (k = 0; k < Len; k++)
			{
				if (cAlienKeyList[i].Serial[k] != Serial[k]) break;
			}
			if (k == Len) break;
		}
		if (i == iAlienKeyListCnt)
		{
			iAlienKeyListCnt++;		
			cAlienKeyList = (SECURITY_KEY_INFO*)DBG_REALLOC(cAlienKeyList, iAlienKeyListCnt * sizeof(SECURITY_KEY_INFO));								
			memset(&cAlienKeyList[iAlienKeyListCnt-1], 0, sizeof(SECURITY_KEY_INFO));
			if (Len)
			{
				cAlienKeyList[iAlienKeyListCnt-1].SerialLength = Len;
				memcpy(cAlienKeyList[iAlienKeyListCnt-1].Serial, Serial, Len);
			}
		}
	}
//	else memmove(cAlienKeyList, &cAlienKeyList[1], sizeof(SECURITY_KEY_INFO) * (ALIENKEY_MAX_CNT-1));	
	DBG_MUTEX_UNLOCK(&alienkey_mutex);	
}

void CameraListClear(GL_STATE_T *pstate, CAMERA_LIST_INFO * clCamList, int *iCamCnt, int iRelease)
{
	int iCameraListCnt = *iCamCnt;
	int n;
	for(n = 0; n < iCameraListCnt; n++)
	{
		if (iRelease && (clCamList[n].Status > 0) && (clCamList[n].ID != 0))
		{
			glDeleteTextures(1, &clCamList[n].Texture);
			if (clCamList[n].Image && (!eglDestroyImageKHR(pstate->display, (EGLImageKHR)clCamList[n].Image)))
				dbgprintf(1,"CameraListClear: eglDestroyImageKHR failed.\n");			
		}
	}
	memset(&clCamList[0], 0, sizeof(CAMERA_LIST_INFO) * MAX_CAMERA_LIST_CNT);
	*iCamCnt = 0;	
}

void CameraListAdd(unsigned int uiID, CAMERA_LIST_INFO * clCamList, int *iCamCnt)
{
	int iCameraListCnt = *iCamCnt;
	if (iCameraListCnt < MAX_CAMERA_LIST_CNT)
	{
		clCamList[iCameraListCnt].ID = uiID;
		clCamList[iCameraListCnt].Status = 0;
		iCameraListCnt++;		
	}	
	*iCamCnt = iCameraListCnt;
}

void DisplayContentListClear()
{
	DBG_MUTEX_LOCK(&dyspcont_mutex);
	if ((iDisplayContentCurrent == 0) && (iDisplayContentCnt1 != 0))
	{
		DBG_FREE(dciDisplayContent1);
		dciDisplayContent1 = NULL;
		iDisplayContentCnt1 = 0;	
		iDisplayContentLiveTime = 0;
	}
	if ((iDisplayContentCurrent) && (iDisplayContentCnt2 != 0))
	{
		DBG_FREE(dciDisplayContent2);
		dciDisplayContent2 = NULL;
		iDisplayContentCnt2 = 0;	
		iDisplayContentLiveTime = 0;
	}
	DBG_MUTEX_UNLOCK(&dyspcont_mutex);	
}

void DisplayContentListAdd(DISPLAY_CONTENT_INFO *pDyspContNew)
{
	DBG_MUTEX_LOCK(&dyspcont_mutex);
	int Cnt;
	DISPLAY_CONTENT_INFO *pDyspContCur;
	if (iDisplayContentCurrent == 0)
	{
		Cnt = iDisplayContentCnt1;
		pDyspContCur = dciDisplayContent1;
	}
	else
	{
		Cnt = iDisplayContentCnt2;
		pDyspContCur = dciDisplayContent2;
	}
	if (Cnt < MAX_DISPLAY_CONTENTS)
	{
		Cnt++;	
		iDisplayContentLiveTime = 0;
		pDyspContCur = (DISPLAY_CONTENT_INFO*)DBG_REALLOC(pDyspContCur, Cnt * sizeof(DISPLAY_CONTENT_INFO));								
		memcpy(&pDyspContCur[Cnt-1], pDyspContNew, sizeof(DISPLAY_CONTENT_INFO));
	}
	if (iDisplayContentCurrent == 0)
	{
		iDisplayContentCnt1 = Cnt;
		dciDisplayContent1 = pDyspContCur;
	}
	else
	{
		iDisplayContentCnt2 = Cnt;
		dciDisplayContent2 = pDyspContCur;
	}
	DBG_MUTEX_UNLOCK(&dyspcont_mutex);	
}

void DisplayContentListSwitch()
{
	DBG_MUTEX_LOCK(&dyspcont_mutex);
	if (iDisplayContentCurrent) iDisplayContentCurrent = 0; else iDisplayContentCurrent = 1;
	if ((iDisplayContentCurrent == 0) && (iDisplayContentCnt1 != 0))
	{
		DBG_FREE(dciDisplayContent1);
		dciDisplayContent1 = NULL;
		iDisplayContentCnt1 = 0;	
		iDisplayContentLiveTime = 0;
	}
	if ((iDisplayContentCurrent) && (iDisplayContentCnt2 != 0))
	{
		DBG_FREE(dciDisplayContent2);
		dciDisplayContent2 = NULL;
		iDisplayContentCnt2 = 0;	
		iDisplayContentLiveTime = 0;
	}
	DBG_MUTEX_UNLOCK(&dyspcont_mutex);	
}

void ClearSkipEventList()
{
	DBG_MUTEX_LOCK(&skipevent_mutex);
	if (iSkipEventListCnt != 0) 
	{
		DBG_FREE(cSkipEventList);
		cSkipEventList = NULL;
		iSkipEventListCnt = 0;	
	}
	DBG_MUTEX_UNLOCK(&skipevent_mutex);	
}

void AddSkipEventInList(unsigned int uiID, unsigned int SubNumber, unsigned int Status)
{
	DBG_MUTEX_LOCK(&skipevent_mutex);
	if ((iSkipEventListCnt < iSkipEventMaxCnt)
		&& ((uiSkipEventListFilter == 0) || (uiSkipEventListFilter == uiID))
		&& ((uiSkipEventNumberFilter == 0) || (uiSkipEventNumberFilter == SubNumber)))
	{
		iSkipEventListCnt++;		
		cSkipEventList = (MODULE_EVENT*)DBG_REALLOC(cSkipEventList, iSkipEventListCnt * sizeof(MODULE_EVENT));
		memset(&cSkipEventList[iSkipEventListCnt-1], 0, sizeof(MODULE_EVENT));
		GetDateTimeStr(cSkipEventList[iSkipEventListCnt-1].Date, 64);
		cSkipEventList[iSkipEventListCnt-1].ID = uiID;
		cSkipEventList[iSkipEventListCnt-1].SubNumber = SubNumber;
		cSkipEventList[iSkipEventListCnt-1].Status = Status;		
	}
	//else memmove(cAlienKeyList, &cAlienKeyList[1], sizeof(MODULE_EVENT) * (SKIPEVENT_MAX_CNT-1));	
	DBG_MUTEX_UNLOCK(&skipevent_mutex);	
}
/*
int GetSmartCardID(uint8_t *serial, SECURITY_KEY_INFO *ski)
{
	int m;
	DBG_MUTEX_LOCK(&securitylist_mutex);
	for (m = 0; m < iSecurityKeyCnt; m++)
	{
		//printf("search card: %i, %i, %i, %i\n", skiSecurityKeys[m].Serial[0], skiSecurityKeys[m].Serial[1], skiSecurityKeys[m].Serial[2], skiSecurityKeys[m].Serial[3]);
		if ((skiSecurityKeys[m].Serial[0] == serial[0])
			&& (skiSecurityKeys[m].Serial[1] == serial[1])
			&& (skiSecurityKeys[m].Serial[2] == serial[2])
			&& (skiSecurityKeys[m].Serial[3] == serial[3])) break;
	}
	if (m != iSecurityKeyCnt)
			memcpy(ski, &skiSecurityKeys[m], sizeof(SECURITY_KEY_INFO));		
	
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	if (m != iSecurityKeyCnt) return 1; else return 0;
}
*/
int CreateEventsForSmartCard(MODULE_INFO *pModule, unsigned int sensenum, uint8_t *serial, unsigned int slength, unsigned int *uiIdResult)
{
	//uint8_t keya[16] = {255,255,255,255,255,255,255,7,128,105,255,255,255,255,255,255};
	*uiIdResult = 0;
	DBG_MUTEX_LOCK(&securitylist_mutex);
	unsigned int kListCnt = iSecurityKeyCnt;
	if (kListCnt == 0)
	{
		DBG_MUTEX_UNLOCK(&securitylist_mutex);
		return 0;
	}
	SECURITY_KEY_INFO *keyList = (SECURITY_KEY_INFO*)DBG_MALLOC(iSecurityKeyCnt * sizeof(SECURITY_KEY_INFO));
	memcpy(keyList, skiSecurityKeys, iSecurityKeyCnt * sizeof(SECURITY_KEY_INFO));	
	DBG_MUTEX_UNLOCK(&securitylist_mutex);
	int resp = 0;
	int sel = 1;
	int i, k;
	if (pModule->Type == MODULE_TYPE_RC522)	sel = MFRC522_selectTag(pModule->InitParams[0], serial);
	
	if (sel)
	{
		for (i = 0; i < kListCnt; i++)
		{
			if (!keyList[i].Enabled) continue;
			if (slength != keyList[i].SerialLength) continue;
			for (k = 0; k < slength; k++)
				if (keyList[i].Serial[k] != serial[k]) break;
			if (k != slength) continue;
			{
				int res = 0;
				int ret = 3;
				resp = 1;
				*uiIdResult = keyList[i].ID;
				
				if ((pModule->Type == MODULE_TYPE_USB_GPIO) && (keyList[i].VerifyKeyA || keyList[i].VerifyKeyB))
				{
					res = usb_gpio_authenticate_card(pModule, sensenum, keyList[i].Sector * 4, (void*)&keyList[i], sizeof(SECURITY_KEY_INFO));
					if (res <= 0) res = 0;
										
					/*if (res & 1) printf("Authenticate1 OK\n");
							else printf("Authenticate1 FAIL\n");
					if (res & 2) printf("Authenticate2 OK\n");
							else printf("Authenticate2 FAIL\n");*/
				}
				else
				{
					if (keyList[i].VerifyKeyA)
					{					
						if (pModule->Type == MODULE_TYPE_RC522)
						{
							/*for (k = 0; k < keyList[i].SerialLength; k++) 
								printf("%i) %i %i\n", k, keyList[i].Serial[k], keyList[i].Serial[k]);
							for (k = 0; k < 6; k++) 
								printf("%i) %i %i\n", k, keyList[i].KeyA[k], keyList[i].KeyB[k]);
							*/
							if (MFRC522_authenticate(pModule->InitParams[0], MF1_AUTHENT1A, keyList[i].Sector * 4, (uint8_t*)keyList[i].KeyA, keyList[i].Serial) == MI_OK) 
								res |= 1;				
						}
						if (pModule->Type == MODULE_TYPE_PN532)
						{
							if (PN532_mifareclassic_AuthenticateBlock(pModule->InitParams[0], keyList[i].Serial, 4, keyList[i].Sector * 4, 0, keyList[i].KeyA))
								res |= 1;
						}
						//if (res & 1) 
						//{
						//	printf("Authenticate1 OK\n");
							/*char data[32];
							if (MFRC522_readFromTag(keyList[i].Sector * 4 + 3, (uint8_t*)data) == MI_OK)
							{
								printf("Block %i: ", keyList[i].Sector * 4 + 3);
								for (k = 0; k < 16; k++) printf("%i,", data[k]);
								printf("\n");
							}	*/						
						//} else printf("Authenticate1 FAIL\n");
					} else res |= 1;
					if (keyList[i].VerifyKeyB && (res & 1))
					{
						if (pModule->Type == MODULE_TYPE_RC522)
						{
							if (MFRC522_authenticate(pModule->InitParams[0], MF1_AUTHENT1B, keyList[i].Sector * 4, (uint8_t*)keyList[i].KeyB, keyList[i].Serial) == MI_OK) 
								res |= 2;
						}
						if (pModule->Type == MODULE_TYPE_PN532)
						{
							if (PN532_mifareclassic_AuthenticateBlock(pModule->InitParams[0], keyList[i].Serial, 4, keyList[i].Sector * 4, 1, keyList[i].KeyB))
								res |= 2;
						}
						//if (res & 2) 
						//{
						//	printf("Authenticate2 OK\n");
							/*char data[32];
							if (MFRC522_readFromTag(keyList[i].Sector * 4, (uint8_t*)data) == MI_OK)
							{
								printf("Block %i: ", keyList[i].Sector * 4);
								for (k = 0; k < 16; k++) printf("%i,", data[k]);
								printf("\n");
							}
							if (MFRC522_readFromTag(keyList[i].Sector * 4 + 3, (uint8_t*)data) == MI_OK)
							{
								printf("Block %i: ", keyList[i].Sector * 4 + 3);
								for (k = 0; k < 16; k++) printf("%i,", data[k]);
								printf("\n");
							}	*/	
						//}
						//	else printf("Authenticate2 FAIL\n");
					} else res |= 2;
				}					
				
				if (res == ret) 
				{
					/*if (keyList[i].EventEnabled) 
					{
						AddModuleEventInList(keyList[i].ID, 0, 1, NULL, 0, 0);
						//CreateModuleEvent(pModule, 0, keyList[i].ID);
					}*/
					resp = 2;
					if (keyList[i].VerifyKeyA || keyList[i].VerifyKeyB) break;
				}
				else 
				{
					resp = -1;
					dbgprintf(3, "Wrong Authenticate smartcard ID:%.4s Name:%s\n", (char*)&keyList[i].ID, keyList[i].Name);	
					break;
				}
				//if (res == ret) printf("Authenticate OK smartcard ID:%.4s Name:%s %i %i\n", (char*)&keyList[i].ID, keyList[i].Name, 
					//		keyList[i].VerifyKeyA, keyList[i].VerifyKeyB);
			}		
		}
	} else resp = -2;
	if ((pModule->Type == MODULE_TYPE_RC522) && (sel)) MFRC522_haltTag(pModule->InitParams[0]);
	DBG_FREE(keyList);
	return resp;
}

float get_sys_temp()
{
	float ret = 10000;
	char ccBuff[32];
	memset(ccBuff, 0, 32);
	vc_gencmd(ccBuff, 32, "measure_temp");
	ccBuff[4] = 0;
	if (strcmp(ccBuff, "temp") == 0) ret = Str2Float(&ccBuff[5]);
	return ret;
}

float get_sys_volt(int iType)
{
	float ret = 0;
	char ccBuff[32];
	memset(ccBuff, 0, 32);
	if (iType == 0) vc_gencmd(ccBuff, 32, "measure_volts core");
	if (iType == 1) vc_gencmd(ccBuff, 32, "measure_volts sdram_c");
	if (iType == 2) vc_gencmd(ccBuff, 32, "measure_volts sdram_i");
	if (iType == 3) vc_gencmd(ccBuff, 32, "measure_volts sdram_p");
	ccBuff[4] = 0;
	if (strcmp(ccBuff, "volt") == 0) ret = Str2Float(&ccBuff[5]);
	return ret;
}

int get_sys_core()
{
	int ret = 0;
	char ccBuff[32];
	memset(ccBuff, 0, 32);
	vc_gencmd(ccBuff, 32, "get_throttled");
	ccBuff[9] = 0;
	if (strcmp(ccBuff, "throttled") == 0) ret = Hex2Int(&ccBuff[12]);
	return ret;
}

long get_fs_size(const char *anyfile)
{
  struct statfs buf;
  statfs(anyfile, &buf);
  return buf.f_blocks;
}

long get_fs_free(const char *anyfile)
{
  struct statfs buf;
  statfs(anyfile, &buf);
  return buf.f_bfree;
}

int get_fs_id(const char *anyfile, int *pID)
{
  struct statfs buf;
  if (statfs(anyfile, &buf) != 0) return 0;
  pID[0] = buf.f_fsid.__val[0];
  pID[1] = buf.f_fsid.__val[1];
  return 1;
}

long get_fs_free_mbytes(const char *anyfile)
{
	struct statfs buf;
 // struct stat st;
 // stat(anyfile, &st);
	statfs(anyfile, &buf);
 // printf("get_fs_free_mbytes %s, %i, %i, %i, %i, %i\n", anyfile, (unsigned int)buf.f_bfree, (unsigned int)st.st_blksize, (unsigned int)buf.f_bavail, (unsigned int)buf.f_blocks, (unsigned int)buf.f_bsize);
	return buf.f_bavail * buf.f_bsize / 1048576;
}

void FreeFsGroup(FS_GROUP *fs_group, unsigned int fsCount)
{
	int i, n;
	for (i = 0; i < fsCount; i++)
	{
		for (n = 0; n < fs_group[i].Count; n++) 
			DBG_FREE(fs_group[i].Paths[n]);	
		if (fs_group[i].Count) DBG_FREE(fs_group[i].Paths);	
	}		
	if (fsCount) DBG_FREE(fs_group);
}

int FillFsGroup(FS_GROUP **fs_group, unsigned int *fsCount, char *cPath, unsigned int uiMinFree)
{
	if (uiMinFree == 0) return 0;
	int i;
	int iCnt = *fsCount;
	int iID[2];
	if (!get_fs_id(cPath, iID)) 
	{
		dbgprintf(2, "Error get ID fs path:%s\n", cPath);				
		return 0;
	}
	//dbgprintf(3, "ID fs path:%s %i %i\n", cPath, iID[0], iID[1]);	
	FS_GROUP *group = *fs_group;
	
	int len = strlen(cPath);
	char *bb = (char*)DBG_MALLOC(len+1);
	memcpy(bb, cPath, len);
	bb[len] = 0;
	int res = 0;
	for (i = 0; i < iCnt; i++)
	{
		if ((group[i].ID[0] == iID[0]) &&
			(group[i].ID[1] == iID[1]))
			{
				group[i].Count++;
				group[i].Paths = (char**)DBG_REALLOC(group[i].Paths, sizeof(char*)*group[i].Count);				
				group[i].Paths[group[i].Count-1] = bb;
				if (uiMinFree > group[i].MinFree) group[i].MinFree = uiMinFree;
				res = 1;
				break;
			}			
	}
	
	if (res == 0)
	{
		group = (FS_GROUP*)DBG_REALLOC(group, sizeof(FS_GROUP)*(iCnt+1));
		group[iCnt].Count = 1;
		group[iCnt].Paths = (char**)DBG_MALLOC(sizeof(char*));				
		group[iCnt].Paths[0] = bb;
		group[iCnt].ID[0] = iID[0];
		group[iCnt].ID[1] = iID[1];
		group[iCnt].MinFree = uiMinFree;
		iCnt++;
		*fsCount = iCnt;
		*fs_group = group;
	}
	
	return 1;
}

size_t get_fs_blksize(const char *anyfile)
{
  struct stat st;
  stat(anyfile, &st);
  return st.st_blksize;
}

void ShowNewMessage(char *cText)
{
	DBG_MUTEX_LOCK(&system_mutex);	
	memset(cEventMessageText, 0, 128);
	if (strlen(cText) < 128) memcpy(cEventMessageText, cText, strlen(cText));
	else memcpy(cEventMessageText, cText, 127);								
	cEventMessageChanged = 1;
	DBG_MUTEX_UNLOCK(&system_mutex);
}

void GetIntVersion(unsigned int *ver)
{
	int n, i = 0, m = 0;
	memset(ver, 0, 4 * sizeof(int));
	unsigned int ret = strlen(VERSION);
	for (n = 0; n < ret; n++)
	{
		if ((VERSION[n] == 46) || (n == (ret - 1)))
		{
			if (VERSION[n] == 46) 
			{				
				ver[m] = Str2IntLimit(&VERSION[i], n - i);
				i = n + 1;
			}
			else ver[m] = Str2IntLimit(&VERSION[i], n - i + 1);
			m++;
		}
	}
}
	
/*
char * get_my_path()
{
	char *pathname,*p;
	//struct task_struct *task;
	struct mm_struct *mm;
 	mm = current->mm;
	if (mm) 
	{
		down_read(&mm->mmap_sem);
		if (mm->exe_file) {
                pathname = kmalloc(PATH_MAX, GFP_ATOMIC);
                if (pathname) {
                      p = d_path(&mm->exe_file->f_path, pathname, PATH_MAX);
                    //Now you have the path name of exe in p
                }
            }
		up_read(&mm->mmap_sem);
	}
	return pathname;
}*/

int CompareStr(char *cStr1, char *cStr2)
{
	char cString1[MAX_FILE_LEN];
	char cString2[MAX_FILE_LEN];
	int len1 = strlen(cStr1);
	int len2 = strlen(cStr2);
	if ((len1 >= MAX_FILE_LEN) || (len2 >= MAX_FILE_LEN)) return -1;
	memset(cString1, 0, MAX_FILE_LEN);
	memset(cString2, 0, MAX_FILE_LEN);
	memcpy(cString1, cStr1, len1);
	memcpy(cString2, cStr2, len2);
	int clk;
	int len0 = len2;
	if (len1 > len2) len0 = len1;
	
	for (clk = 0; clk != len0; clk++)
	{
		if (cString1[clk] < cString2[clk]) return 1;
		if (cString1[clk] > cString2[clk]) return 2;
	}
	return 0;
}

int GetFirstFile(char *cCurrDir, char *cFindedDir, char cDeleteEmptyDir)
{
	//printf("GetFirstFile %s %s\n", cCurrDir, cFindedDir);
	int iStatus = 0;
	char cCurrDir2[MAX_FILE_LEN];
	DIR *dir;
	struct dirent *dp;	
	int iCnt = 0;
	int iLen = strlen(cCurrDir);
	if (iLen == 0) return 0;
	
	dir = opendir(cCurrDir);
	if (dir != NULL)
	{
		while((dp=readdir(dir)) != NULL)
		{
			if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
			{
				memcpy(cCurrDir2, cCurrDir, MAX_FILE_LEN);
				if (cCurrDir2[iLen-1] != 47) strcat(cCurrDir2, "/");
				strcat(cCurrDir2, dp->d_name);
				if (dp->d_name[0] != 46)
				{
					int ret = GetFirstFile(cCurrDir2, cFindedDir, cDeleteEmptyDir);
					if (ret > 0) iStatus += ret;
				}
				iCnt++;
			}
		}
		closedir(dir);
		
		if ((iCnt == 0) && cDeleteEmptyDir) 
		{
			dbgprintf(5,"Remove empty Folder %s\n",cCurrDir);
			if (remove(cCurrDir) != 0) dbgprintf(2, "Error delete %s '%s'\n", (errno == ENOENT) ? "not found" : "no access", cCurrDir);
		}
	}
	else
	{
		int res = 0;
		DBG_MUTEX_LOCK(&system_mutex);
		int cnt = iStreamCnt;
		DBG_MUTEX_UNLOCK(&system_mutex);
		int n, i;
		for (n = 0; n < cnt; n++) 
		{	
			DBG_MUTEX_LOCK(&SyncStream[n]->mutex);
			i = strcmp(SyncStream[n]->CurrentPath, cCurrDir);
			DBG_MUTEX_UNLOCK(&SyncStream[n]->mutex);
			if (i == 0)
			{
				res = 1;
				break;
			}
		}
		if (res)
		{
			//dbgprintf(4,"file %s find, in work\n",cCurrDir);
			return -2;
		}
		int k;
		int f1 = -1;
		int f2 = -1;
		int iLen = strlen(cCurrDir);
		for (k = 0; k < iLen; k++) if (cCurrDir[k] == 47) f1 = k;
		iLen = strlen(cFindedDir);
		for (k = 0; k < iLen; k++) if (cFindedDir[k] == 47) f2 = k;
		//printf("CompareStr %s %s\n", &cCurrDir[f1+1], &cFindedDir[f2+1]);
		k = CompareStr(&cCurrDir[f1+1], &cFindedDir[f2+1]);
		if (k == 1) memcpy(cFindedDir, cCurrDir, MAX_FILE_LEN);
		//printf("GetFirstFile result %i\n", k);
		iStatus = 1;
	}
	return iStatus;
}

int ClearDir(char *cPath)
{
	DIR *dir;
	struct dirent *dp;	
	char cFileDir[MAX_FILE_LEN];
	int iCnt = 0;
	
	dir = opendir(cPath);
	if (dir != NULL)
	{
		while((dp=readdir(dir)) != NULL)
		{
			memset(cFileDir, 0, MAX_FILE_LEN);
			strcpy(cFileDir, cPath);
			strcat(cFileDir, "/");
			strcat(cFileDir, dp->d_name);
			
			if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0) 
				&& !Its_Directory(cFileDir))
			{
				if (remove(cFileDir) != 0) 
					printf("Error delete '%s'\n", cFileDir); 
					else 
					{
						printf("Deleted '%s'\n", cFileDir);
						iCnt++;
					}
			}
		}
		closedir(dir);
	}
	return iCnt;
}

int ConnectToCamera(unsigned int uiNum, unsigned int uiID, GL_STATE_T *pstate, GLuint *texture, void *eglImg, int *sW, int *sH, char cType)
{
	int ret = 0;
	*sW = pstate->screen_width;
	*sH = pstate->screen_height;
	glDeleteTextures(1, texture);
	glGenTextures(1,texture);
	func_link * f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
	memset(f_link, 0, sizeof(func_link));
	f_link->FuncRecv = RecvVideoFrame;
	f_link->ConnectNum = 1;
	f_link->DeviceNum = uiNum;
	struct sockaddr_in *pAddr = NULL;
						
	DBG_MUTEX_LOCK(&modulelist_mutex);
	pAddr = ModuleIdToAddress(uiID, 2);
	if (pAddr) dbgprintf(5, "conn to %s:%.4s\n", inet_ntoa(pAddr->sin_addr), (char*)&uiID);
						
	memset(pScreenMenu[0].Options[1].Name, 0, 64);
	if (cType < 2) 
		strcpy(pScreenMenu[0].Options[1].Name, "Камера: ");
		else
		strcpy(pScreenMenu[0].Options[1].Name, "Файл: ");
	pScreenMenu[0].Options[1].MiscData = uiID;
	ret = ModuleIdToNum(uiID, 2);
	if (ret >= 0)
	{							
		if (strlen((char*)miModuleList[ret].Name) < 55) strcat(pScreenMenu[0].Options[1].Name, (char*)miModuleList[ret].Name); 
				else memcpy(pScreenMenu[0].Options[1].Name, miModuleList[ret].Name, 55);
	}		
	if (pAddr) memcpy(&f_link->RecvAddress, pAddr, sizeof(f_link->RecvAddress));
	
	ret = 0;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	if (pAddr)
	{	
		int omx_res = omx_play_video_on_egl_from_func(f_link, *texture, &pstate->display, &pstate->context, eglImg, sW, sH, 0, cType);
		if (omx_res)
		{			
			ret = 1;
		} 
		else 
		{
			if (omx_res == 100) 
				dbgprintf(2,"warning OpenMax BUSY (resource priority low)\n");
				else
				dbgprintf(1,"error SHOW_TYPE_CAMERA omx_play_video_on_egl_from_func\n");
		}
	} 
	else 
	{
		dbgprintf(1,"not found CAMERA/FILE ID '%.4s'\n", (char*)&uiID);
		DBG_FREE(f_link);
	}
	//printf("omx_play_video_on_egl_from_func %i\n", ret);		
	return ret;
}

void PrintIRCmd(uint16_t *cOutBuffer, unsigned int uiLen)
{
	char tempBuff[256];
	char tempBuff2[32];
	memset(tempBuff, 0, 256);
	memset(tempBuff2, 0, 32);
	dbgprintf(5,"Unknown IR command len %i\n", uiLen);
	int n2;
	for (n2 = 0; n2 < uiLen; n2++) 
	{
		if (n2 < 10)
		{
			memset(tempBuff2, 0, 32);
			sprintf(tempBuff2, "%i", cOutBuffer[n2]);
			strcat(tempBuff, tempBuff2);
			if ((n2 == 9) && (uiLen > 9)) { strcat(tempBuff, " ..."); break;}
			if (n2 < (uiLen-1)) strcat(tempBuff, ".");	
		} else break;
	}								
	dbgprintf(5,"Unknown IR command: %s\n", tempBuff);
}

void PrintCardSerial(uint8_t *cOutBuffer, unsigned int uiLen)
{
	char tempBuff[256];
	char tempBuff2[32];
	memset(tempBuff, 0, 256);
	memset(tempBuff2, 0, 32);
	dbgprintf(5,"Unknown SmartCard	len %i\n", uiLen);
	int n2;
	for (n2 = 0; n2 < uiLen; n2++) 
	{
		if (n2 < 10)
		{
			memset(tempBuff2, 0, 32);
			sprintf(tempBuff2, "%i", cOutBuffer[n2]);
			strcat(tempBuff, tempBuff2);
			if ((n2 == 9) && (uiLen > 9)) { strcat(tempBuff, " ..."); break;}
			if (n2 < (uiLen-1)) strcat(tempBuff, ".");	
		} else break;
	}								
	dbgprintf(5,"Unknown SmartCard: %s\n", tempBuff);
}

int GetIrDataID(uint16_t *cOutBuffer, unsigned int uiLen, unsigned int *uiOutID)
{
	*uiOutID = 0;
	int ret = 0;
	int n2;
	DBG_MUTEX_LOCK(&ircode_mutex);
	for (n2 = 0; n2 < iIRCommandCnt; n2++)
	{
		if (uiLen == mIRCommandList[n2].Len)
		{
			if (SearchDataCombine((uint16_t*)cOutBuffer, uiLen, (uint16_t*)mIRCommandList[n2].Code, mIRCommandList[n2].Len, 0) != -1) 
			{
				*uiOutID = mIRCommandList[n2].ID;
				ret = 1;
				break;
			}
		}
	}
	DBG_MUTEX_UNLOCK(&ircode_mutex);
				
	if (ret) return 1; else return 0;
}

int FindFirstFile(char *cDir, char *cAddPath, char *cFile, char cDeleteEmptyDir)
{
	int iLen = strlen(cDir);
	if (iLen >= MAX_FILE_LEN) return -1;
	char cCurrentPath[MAX_FILE_LEN];
	char cResultPath[MAX_FILE_LEN];
	memset(cCurrentPath, 0, MAX_FILE_LEN);
	memset(cResultPath, 255, MAX_FILE_LEN);
	memcpy(cCurrentPath, cDir, iLen);
	cResultPath[MAX_FILE_LEN-1] = 0;
		
	int result = GetFirstFile(cCurrentPath, cResultPath, cDeleteEmptyDir);
	//printf("GetFirstFile done: %i %s\n", result, cCurrentPath);
	if (result >= 0)
	{		
		memset(cAddPath, 0, MAX_FILE_LEN);
		memset(cFile, 0, MAX_FILE_LEN);
		int i;
		int iResLen = strlen(cResultPath);
		int iSpltPos = -1;
		for (i = 0; i < iResLen; i++)
			if ((cResultPath[i] == 47) && (i != (iResLen-1))) iSpltPos = i;
		if (iSpltPos != -1)
		{
			int iAddLen = iSpltPos - iLen;
			int iFileLen = iResLen - iSpltPos - 1;
			if ((iAddLen >= MAX_FILE_LEN) || (iFileLen >= MAX_FILE_LEN))
			{
				dbgprintf(2,"GetFirstFile: error fill result\n");
				DBG_FREE(cResultPath);
				return -3;
			}
			//printf("FindFirstFile done: %i %i %i %i\n", result, iLen, iAddLen, iSpltPos);
			memcpy(cAddPath, &cResultPath[iLen], iAddLen);
			memcpy(cFile, &cResultPath[iSpltPos + 1], iFileLen);
			//printf("FindFirstFile done: %s %s\n", cAddPath, cFile);
		}
	}
	return result;
}

int CopyFile(char *cFrom, char *cTo, char *cToError, char cWait, char cRemoveAfter, STREAM_INFO *pStream, unsigned char ucOrderLmt, char* cAddrOrderer, 
		unsigned int uiMaxWaitTime, unsigned int uiMessWaitTime, unsigned int uiDestType)
{
	dbgprintf(3, "CopyFile:\n");
	dbgprintf(3, "\tFrom:'%s'\n",cFrom);
	dbgprintf(3, "\tTo:'%s'\n",cTo);
	dbgprintf(3, "\tRemoveAfter:'%i'\n",cRemoveAfter);
	dbgprintf(3, "\tAddress:'%s'\n",cAddrOrderer ? cAddrOrderer : "Path");
	dbgprintf(3, "\tDestType:'%i'\n",uiDestType);
	int ret = 1;
	misc_buffer *mBuff = (misc_buffer*)DBG_CALLOC(sizeof(misc_buffer),1);
	mBuff->data = (char*)DBG_CALLOC(256,1);
	if (strlen(cFrom) < 256) strcpy(mBuff->data, cFrom); else dbgprintf(2, "Big length CopyFile link From %i, max 256", strlen(cFrom));
	mBuff->void_data = (char*)DBG_CALLOC(256,1);
	if (strlen(cTo) < 256) strcpy(mBuff->void_data, cTo); else dbgprintf(2, "Big length CopyFile link To %i, max 256", strlen(cTo));	
	if (cToError)
	{
		mBuff->void_data4 = (char*)DBG_CALLOC(256,1);
		if (strlen(cToError) < 256) strcpy(mBuff->void_data4, cToError); else dbgprintf(2, "Big length CopyFile link Error %i, max 256", strlen(cToError));	
	}
	if (cAddrOrderer)
	{
		mBuff->void_data2 = (char*)DBG_CALLOC(64,1);
		if (strlen(cAddrOrderer) < 64) strcpy(mBuff->void_data2, cAddrOrderer); else dbgprintf(2, "Big length CopyFile AddOrderer %i, max 64", strlen(cAddrOrderer));	
	} else mBuff->void_data2 = NULL;
	mBuff->flag = cRemoveAfter;
	mBuff->clock = cWait;
	mBuff->void_data3 = pStream;
	mBuff->uidata[1] = ucOrderLmt;
	mBuff->uidata[2] = uiMaxWaitTime;
	mBuff->uidata[3] = uiMessWaitTime;
	mBuff->uidata[4] = uiDestType;
	
	if (cWait) 
	{
		ret = (int)thread_CopyFile((void*)mBuff);
	}
	else pthread_create(&threadFileIO, &tattrFileIO, thread_CopyFile, (void*)mBuff);
	return ret;
}

char ClearSpace(FS_GROUP *fs_group, unsigned int fsCount)
{
	//printf("ClearSpace %i\n", fsCount);
	char cPrev[MAX_FILE_LEN];
	char cCurr[MAX_FILE_LEN];
	char cSubPath[MAX_FILE_LEN];
	char cResSubPath[MAX_FILE_LEN];
	memset(cResSubPath, 0, MAX_FILE_LEN);
	unsigned int uiPathPos = 0;
	//unsigned int uiPrevPos = 0;
	char cStatus = 0;
	char result = 0;
	int i, m, n, k, clk, finded_clk;
	for (i = 0; i < fsCount; i++)
	{
		//printf("Space %i\n", i);
		if (fs_group[i].Count == 0) continue;
		clk = MAX_FILE_DELETE;		
		while(clk &&(fs_group[i].MinFree > get_fs_free_mbytes(fs_group[i].Paths[0])))
		{
			memset(cPrev, 0, MAX_FILE_LEN);
			cPrev[0] = 255;
			cStatus = 0;
			uiPathPos = 0;
			//uiPrevPos = 0;
			finded_clk = 0;
			for (m = 0; m < fs_group[i].Count; m++)
			{
				//printf("Path %i\n", m);
				//printf("Path %s\n", fs_group[i].Paths[m]);
				n = FindFirstFile(fs_group[i].Paths[m], cSubPath, cCurr, 1);
				//printf("FindFirstFile done %i\n", n);
				if (n <= 0) 
				{
					if (n == -1) 
					{
						result = -1;
						break;
					}
					dbgprintf(3, "no file for delete in %s (need space %i Mb) now: %i\n", fs_group[i].Paths[m], fs_group[i].MinFree, get_fs_free_mbytes(fs_group[i].Paths[m]));
					cStatus = 0;
				} 
				else 
				{	
					finded_clk++;
					//printf("FindFirstFile %s\n", fs_group[i].Paths[m]);
					//printf("FindFirstFile %s\n", cSubPath);
					//printf("FindFirstFile %s\n", cCurr);
					dbgprintf(5, "finded for delete %s\n", cCurr);
					/*if (n > uiPrevPos)
					{
						memcpy(cPrev, cCurr, MAX_FILE_LEN);
						uiPrevPos = n;
						uiPathPos = m;
						cStatus = 1;
					}
					if (n == uiPrevPos)*/
					{
						dbgprintf(5, "CompareStr with %s\n", cPrev);
						k = CompareStr(cPrev, cCurr);
						if (k == 2) 
						{
							memcpy(cPrev, cCurr, MAX_FILE_LEN);
							memcpy(cResSubPath, cSubPath, MAX_FILE_LEN);
							//uiPrevPos = n;
							uiPathPos = m;
							cStatus = 1;
						}
					}
					dbgprintf(5, "result for delete %s\n", cPrev);					
				}
			}
			if (finded_clk == 0)
			{
				clk = 0;	
				break;
			}
			if (cStatus)
			{		
				dbgprintf(3, "Delete '%s'\n", cPrev);
				int iLen = strlen(fs_group[i].Paths[uiPathPos]) + strlen(cResSubPath)+strlen(cPrev) + 1;
				char *cFullPath = (char*)DBG_MALLOC(iLen + 1);
				memset(cFullPath, 0, iLen + 1);
				strcpy(cFullPath, fs_group[i].Paths[uiPathPos]);
				strcat(cFullPath, cResSubPath);
				strcat(cFullPath, "/");
				strcat(cFullPath, cPrev);
				dbgprintf(3, "Delete '%s'\n", cFullPath);
				if (remove(cFullPath) != 0)
				{
					dbgprintf(2, "Error delete %s '%s'\n", (errno == ENOENT) ? "not found" : "no access", cFullPath);
					result = -2;
					DBG_FREE(cFullPath);					
					break;
				}
				DBG_FREE(cFullPath);				
			}
			clk--;
		}
		if (result >= 0)
		{
			if (!clk && (fs_group[i].MinFree > get_fs_free_mbytes(fs_group[i].Paths[0])))
			{
				dbgprintf(2, "Error clean disk, need more space '%i>%i'\n", fs_group[i].MinFree, get_fs_free_mbytes(fs_group[i].Paths[0]));
				for (m = 0; m < fs_group[i].Count; m++)
					dbgprintf(3, "\tfrom path: '%s'\n", fs_group[i].Paths[m]);
			}
			else
				result++;
		}
	}
	return result;
}

int SmartCardModuleInit(MODULE_INFO * pModule)
{
	if (pModule->Type == MODULE_TYPE_RC522)	
	{
		MFRC522_deinit(pModule->InitParams[0]);
		pModule->InitParams[0] = MFRC522_init(pModule->Settings[1]);
	}
	if (pModule->Type == MODULE_TYPE_PN532)	
	{
		PN532_deinit(pModule->InitParams[0]);
		pModule->InitParams[0] = PN532_init(pModule->Settings[1]);
	}
	
	return pModule->InitParams[0] ? 1 : 0;
}

int SmartCardModuleGetSerial(MODULE_INFO * pModule, uint8_t *serial, unsigned int *uiLength)
{
	int res = 0;
	uint8_t ret;
	int status;
	uint8_t data[16];
	
	if (pModule->Type == MODULE_TYPE_RC522)
	{
		status = MFRC522_requestTag(pModule->InitParams[0], MF1_REQALL, data);
		if (status == MI_OK)
		{
			status = MFRC522_antiCollision(pModule->InitParams[0], data);
			if (status == MI_OK)
			{
				*uiLength = 5;
				memcpy(serial, data, 5);
				res = 1;
			}
		} 
		else 
		{
			if (status == MI_TIMEOUT) res = -1;
		}
	}
	
	if (pModule->Type == MODULE_TYPE_PN532)
	{		
		status = PN532_readPassiveTargetID(pModule->InitParams[0], PN532_MIFARE_ISO14443A, data, &ret, 300); 
		if (status == -1) res = -1;
		if (status > 0)
		{
			*uiLength = ret;
			memcpy(serial, data, ret);
			res = 1;
		}
	}
	return res;
}
		

void * CardReader(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	DBG_LOG_IN();
	misc_buffer * mBuff = (misc_buffer*)pData;
	MODULE_INFO * pModules = (MODULE_INFO *)mBuff->void_data;
	unsigned int iModulesCount = mBuff->data_size;
	char *cCapturePath = mBuff->data;
	char *cBackUpPath = mBuff->data2;
	unsigned int uiCaptureSizeLimit = mBuff->uidata[0];
	unsigned int uiCaptureTimeLimit = mBuff->uidata[1];
	unsigned int ucCaptOrderLmt = mBuff->uidata[2];
	unsigned int ucBackUpCaptured = mBuff->uidata[3];	
	char cCaptEnabled = mBuff->uidata[4];
	unsigned int uiMaxWaitCopyTime = mBuff->uidata[5];
	unsigned int uiMessWaitCopyTime = mBuff->uidata[6];
	char*	cCaptOrderAddr = mBuff->void_data2;
	char*	cCaptPrePath = mBuff->void_data3;
	DBG_FREE(mBuff);
	
	char * pCurStatus = (char *)DBG_MALLOC(iModulesCount);
	memset(pCurStatus, 0, iModulesCount);
	char *mess_stat = (char *)DBG_MALLOC(iModulesCount);
	memset(mess_stat, 0, iModulesCount);
	
	uint8_t serial[7];
	uint8_t prev_serial[7];
	unsigned int n, k, iDevFinded, seriallength, prev_seriallength, res, uiID;	
	int status;
	DBG_MUTEX_LOCK(&system_mutex);	
	int iIntervalScan = iIntervalRescanCard;
	cThreadReaderStatus = 1;
	char cLoop = 1;
	DBG_MUTEX_UNLOCK(&system_mutex);

	capture_settings *capt_set = (capture_settings*)DBG_MALLOC(iModulesCount*sizeof(capture_settings));
	memset(capt_set, 0, iModulesCount*sizeof(capture_settings));
	
	unsigned int result, result_evn;
	//iTimeReset = 0;
	for (n = 0; n != iModulesCount; n++)
		if ((pModules[n].Enabled & 1) 
				&& (pModules[n].ScanSet != 0) 
				&& ((pModules[n].Type == MODULE_TYPE_PN532) || (pModules[n].Type == MODULE_TYPE_RC522))
				&& (pModules[n].SaveChanges != 0) && cCaptEnabled)
		{
					capt_set[n].FileData = file_init(1048576, 1024);
					capt_set[n].Stream = AddCaptureStream(CAPTURE_TYPE_MODULE, 0, 0);
					SaveCapturedData(4, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
										ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
										uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);					
		}	
	
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	while(cLoop)
	{
		//if (get_ms(&previous_ms) > 1000)
		{			
			iDevFinded = 0;
			for (n = 0; n != iModulesCount; n++)
			{
				if ((pModules[n].Enabled & 1) && ((pModules[n].Type == MODULE_TYPE_RC522) || (pModules[n].Type == MODULE_TYPE_PN532)))
				{
					if ((pModules[n].Settings[0] != 0) && (pModules[n].SubModule >= 0))
					{
						if (pModules[n].Settings[4]) usleep(pModules[n].Settings[4]);						
						gpio_switch_on_module(&pModules[pModules[n].SubModule]);
						if (pModules[n].Settings[4]) usleep(pModules[n].Settings[4]);						
					}
					if (pCurStatus[n] == 0)
					{
						if (SmartCardModuleInit(&pModules[n]))
						{
							pCurStatus[n] = 100;
							dbgprintf(3,"Digital init by %s ID:%.4s OK.\n", GetModuleTypeName(pModules[n].Type), (char*)&pModules[n].ID);
							mess_stat[n] = 0;							
						}
						else
						{
							pCurStatus[n] = 30;
							if (mess_stat[n] == 0) dbgprintf(1,"Digital init by %s ID:%.4s failed.\n", GetModuleTypeName(pModules[n].Type), (char*)&pModules[n].ID);
							mess_stat[n] = 1;
						}			
					}
					if (pCurStatus[n] == 30) CreateModuleEvent(&pModules[n], 0, 1);
					if ((pCurStatus[n] > 0) && (pCurStatus[n] < 100)) pCurStatus[n]--;
					if (pCurStatus[n] >= 100)
					{
						iDevFinded = 1;
						status = SmartCardModuleGetSerial(&pModules[n], serial, &seriallength);	
						if (status < 0) pCurStatus[n] = 0;
						if (status == 1)
						{
							if (pCurStatus[n] == 100) 
							{
								pCurStatus[n] = 101;
								CreateModuleEvent(&pModules[n], 0, 2);
							}	
							
							res = 0;
							if (seriallength == prev_seriallength)
							{
								for (k = 0; k < seriallength; k++)
								{
									if (prev_serial[k] != serial[k]) break;
								}
								if (k == seriallength) res = 1;
							}
							if (res	&& ((unsigned int)get_ms(&previous_ms) < iIntervalScan))
							{
								status = 0;
								if (pModules[n].Type == MODULE_TYPE_RC522) MFRC522_haltTag(pModules[n].InitParams[0]);
							}
							else 
							{
								prev_seriallength = seriallength;
								memcpy(prev_serial, serial, seriallength);							
								previous_ms = 0;
								get_ms(&previous_ms);			
							}							
						}
						else 
						{
							if (pCurStatus[n] == 101)
							{
								pCurStatus[n] = 100;
								CreateModuleEvent(&pModules[n], 0, 3);
								memset(prev_serial, 0, 4);							
								previous_ms = 0;
								get_ms(&previous_ms);	
							}
						}
						if (status) 
						{
							if (pModules[n].Type == MODULE_TYPE_RC522)
								dbgprintf(4, "The serial of card: %i, %i, %i, %i, %i\n", 
											serial[0], serial[1], serial[2], serial[3], serial[4]);
								else
								dbgprintf(4, "The serial of card: %i, %i, %i, %i\n", 
											serial[0], serial[1], serial[2], serial[3]);
							
							DBG_MUTEX_LOCK(&securitylist_mutex);
							int iUpdateStatus = 0;
							unsigned int iUpdaterID = 0;
							SECURITY_KEY_INFO skiUpdateInfo;
							
							if (iUpdateKeyInfoAction && ((unsigned int)get_ms(&iUpdateKeyInfoTimer) > 3000)) iUpdateKeyInfoAction = 0;
							if (iUpdateKeyInfoAction && (pModules[n].ID == uiUpdateKeyInfoReader)) 
							{
								int p = seriallength + 1;											
								if (seriallength == skiUpdateKeyInfo.SerialLength)
									for (p = 0; p < seriallength;p++) if (serial[p] != skiUpdateKeyInfo.Serial[p]) break;												
								if (p == seriallength)
								{
									iUpdateStatus = iUpdateKeyInfoAction;
									iUpdaterID = uiUpdateKeyInfoReader;
									memcpy(&skiUpdateInfo, &skiUpdateKeyInfo, sizeof(SECURITY_KEY_INFO));
								}
								iUpdateKeyInfoAction = 0;
							}
							DBG_MUTEX_UNLOCK(&securitylist_mutex);
							if (iUpdateStatus && (pModules[n].ID == iUpdaterID)) 
							{
								UpdateSecurityKey(iUpdateStatus, &pModules[n], &skiUpdateInfo, 0);								
							}
							else
							{
								result = 1;
								result_evn = pModules[n].GenEvents & result;
											
								status = CreateEventsForSmartCard(&pModules[n], 0, serial, seriallength, &uiID);
								if (status == 0)							
								{
									if (result_evn) CreateModuleEvent(&pModules[n], 0, 4);
									AddAlienKeyInList(serial, seriallength);
									dbgprintf(2,"New smartcard. Ignore\n");				
								}
								if (status == -1)							
								{
									if (result_evn) CreateModuleEvent(&pModules[n], 0, 5);
									dbgprintf(2,"Error auth smartcard. Ignore\n");				
								}	
								if (status > 0)
								{
									if (result_evn) CreateModuleEvent(&pModules[n], 1, uiID);
									if ((pModules[n].SaveChanges & result) && cCaptEnabled)
									{
										pModules[n].Status[1] = uiID;
										SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
																ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
																uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
										pModules[n].Status[1] = 0;
									}
									dbgprintf(6, "Card ID: %.4s from module: %.4s\n", (char*)&uiID, (char*)&pModules[n].ID);
								}
							}
							if (pModules[n].Type == MODULE_TYPE_RC522) MFRC522_haltTag(pModules[n].InitParams[0]);
						}
					}
					if ((pModules[n].Settings[0] != 0) && (pModules[n].SubModule >= 0))
					{
						if (pModules[n].Settings[4]) usleep(pModules[n].Settings[4]);
						gpio_switch_off_module(&pModules[pModules[n].SubModule]);
						if (pModules[n].Settings[4]) usleep(pModules[n].Settings[4]);						
					}				
				}
			}
			if (iDevFinded == 1) usleep(50000); 
			else usleep(300000);
			DBG_MUTEX_LOCK(&system_mutex);
			cLoop = cThreadReaderStatus;
			DBG_MUTEX_UNLOCK(&system_mutex);	
			//dbgprintf(3,"card reader loop\n");
		}
	}
	dbgprintf(3,"DONE card reader thread\n");
	
	for (n = 0; n != iModulesCount; n++)
		if ((pModules[n].Enabled & 1)  
				&& (pModules[n].ScanSet != 0) 
				&& ((pModules[n].Type == MODULE_TYPE_PN532) || (pModules[n].Type == MODULE_TYPE_RC522))
				&& (pModules[n].SaveChanges != 0)
				&& (pModules[n].ScanSet != 0) && (pModules[n].SaveChanges != 0) && cCaptEnabled)
		{
			if (capt_set[n].FileData->opened == 1)
				SaveCapturedData(2, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
									ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
									uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
			file_deinit(capt_set[n].FileData);			
		}
	
	DBG_FREE(cCaptPrePath);
	if (cCaptOrderAddr) DBG_FREE(cCaptOrderAddr);
	DBG_FREE(cCapturePath);
	DBG_FREE(cBackUpPath);
	DBG_FREE(capt_set);
	DBG_FREE(pCurStatus);
	DBG_FREE(mess_stat);	
	DBG_FREE(pModules);
	
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadReaderStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)1;
}

int SaveActions(int iMode, void *pSaveData, void *pCaptSet)
{
	capture_settings *capt_set = (capture_settings*)pCaptSet;
	file_struct *file_data = capt_set->FileData;
	ACTION_INFO *maActInf = (ACTION_INFO*)pSaveData;
	char buffpath[MAX_PATH*2];
		
	if (iMode == 0)
	{
		memset(buffpath, 0, MAX_PATH*2);
		sprintf(buffpath, "Actions\r\n\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
		
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "Time;Ms;ActionID;SrcID;SrcSubNumber;SrcSubNumber;SrcStatus;SrcStatus;DestID;DestSubNumber;DestSubNumber;DestStatus;DestStatus;\r\n\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
	}
	
	if (iMode == 1)
	{
		//dbgprintf(3, "ExecuteAction %.4s, %.4s, %s\n", (char*)&maActInf->ActionID, (char*)&maActInf->DestID, maActInf->Name);
		char SrcSubNumberBuff[64];
		char SrcSubNumberBuff2[64];
		char SrcStatusBuff[64];
		char SrcStatusBuff2[64];
		char DestSubNumberBuff[64];
		char DestSubNumberBuff2[64];
		char DestStatusBuff[64];
		char DestStatusBuff2[64];
		
		time_t rawtime;
		struct tm timeinfo;
		struct timespec ts;
		int64_t tms;
		
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		clock_gettime(CLOCK_MONOTONIC, &ts);
		tms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
		//printf("Act %.4s %.4s %i %i\n", (char*)&maActInf->ActionID, (char*)&maActInf->DestID, maActInf->DestSubNumber, maActInf->DestStatus);
		memset(buffpath, 0, MAX_PATH*2);
		sprintf(buffpath, "%02i:%02i:%02i;%i;%.4s;%.4s;%s;%s;%s;%s;%.4s;%s;%s;%s;%s;\r\n",
						timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(unsigned int)tms, (char*)&maActInf->ActionID, 
						(char*)&maActInf->SrcID, 
						GetActionCodeName(maActInf->SrcSubNumber, SrcSubNumberBuff, 64, 4),
						GetActionCodeName(maActInf->SrcSubNumber, SrcSubNumberBuff2, 64, 2),
						GetActionCodeName(maActInf->SrcStatus, SrcStatusBuff, 64, 4),
						GetActionCodeName(maActInf->SrcStatus, SrcStatusBuff2, 64, 2),
						(char*)&maActInf->DestID, 
						GetActionCodeName(maActInf->DestSubNumber, DestSubNumberBuff, 64, 4),
						GetActionCodeName(maActInf->DestSubNumber, DestSubNumberBuff2, 64, 2),
						GetActionCodeName(maActInf->DestStatus, DestStatusBuff, 64, 4),
						GetActionCodeName(maActInf->DestStatus, DestStatusBuff2, 64, 2));			
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
	}
	return 1;
}

int SaveEvents(int iMode, void *pSaveData, void *pCaptSet)
{
	capture_settings *capt_set = (capture_settings*)pCaptSet;
	file_struct *file_data = capt_set->FileData;
	MODULE_EVENT *pEvent = (MODULE_EVENT*)pSaveData;
	char buffpath[MAX_PATH*2];
		
	if (iMode == 0)
	{
		memset(buffpath, 0, MAX_PATH*2);
		sprintf(buffpath, "Events\r\n\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
		
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "Time;Ms;ID;SubNumber;SubNumber;Status;Status;ExecNow;\r\n\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
	}
	
	if (iMode == 1)
	{
		char SubNumberBuff[64];
		char SubNumberBuff2[64];
		char StatusBuff[64];
		char StatusBuff2[64];
		
		time_t rawtime;
		struct tm timeinfo;
		struct timespec ts;
		int64_t tms;
		
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		clock_gettime(CLOCK_MONOTONIC, &ts);
		tms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
		
		memset(buffpath, 0, MAX_PATH*2);
		sprintf(buffpath, "%02i:%02i:%02i;%i;%.4s;%s;%s;%s;%s;%i;\r\n",
						timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(unsigned int)tms, (char*)&pEvent->ID, 
						GetActionCodeName(pEvent->SubNumber, SubNumberBuff, 64, 4),
						GetActionCodeName(pEvent->SubNumber, SubNumberBuff2, 64, 2),
						GetActionCodeName(pEvent->Status, StatusBuff, 64, 4),
						GetActionCodeName(pEvent->Status, StatusBuff2, 64, 2),
						pEvent->ExecNow);			
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
	}
	return 1;
}

int SaveModuleStatuses(int iMode, void *pSaveData, void *pCaptSet)
{
	capture_settings *capt_set = (capture_settings*)pCaptSet;
	file_struct *file_data = capt_set->FileData;
	MODULE_INFO *pModule = (MODULE_INFO*)pSaveData;
				
	char buffpath[MAX_PATH*2];
	char buffpath2[64];
	char cBufferName[64];
			
	if (iMode == 0)
	{
		char cBufferValueType[32];
		int i;
		memset(buffpath, 0, MAX_PATH*2);
		sprintf(buffpath, "Module;ID: %.4s;Type:%s;Name: %s;IP: %s\r\n\r\n", (char*)&pModule->ID, GetModuleTypeName(pModule->Type), pModule->Name, inet_ntoa(pModule->Address.sin_addr));
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
				
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "Settings:;");
		for (i = 0; i < MAX_MODULE_SETTINGS; i++)
		{
			memset(buffpath2, 0, 64);	
			sprintf(buffpath2, "%i;", pModule->Settings[i]);					
			strcat(buffpath, buffpath2);
		}
		strcat(buffpath, "\r\n\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
		
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "Time;Ms;");
		
		int k = 1;			
		for (i = 0; i < MAX_MODULE_STATUSES; i++)
		{
			if (!GetModuleStatusEn(pModule->Type, i)) break;
			if (pModule->SaveChanges & k)
			{
				memset(buffpath2, 0, 64);	
				sprintf(buffpath2, "%s,%s;", GetModuleStatusName(pModule->Type, i, cBufferName, 64, 0), GetModuleStatusValueType(pModule->Type, i, cBufferValueType, 32));					
				strcat(buffpath, buffpath2);
			}
			k <<= 1;
		}
		strcat(buffpath, "\r\n\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
		memset(buffpath, 0, MAX_PATH*2);
		strcat(buffpath, "\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
	}
	
	if (iMode == 1)
	{
		time_t rawtime;
		struct tm timeinfo;
		struct timespec ts;
		int64_t tms;
		
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		clock_gettime(CLOCK_MONOTONIC, &ts);
		tms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
				
		memset(buffpath, 0, MAX_PATH*2);
		sprintf(buffpath, "%02i:%02i:%02i;%i;",
						timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(unsigned int)tms);		
		int i;
		int k = 1;
		for (i = 0; i < MAX_MODULE_STATUSES; i++)
		{
			if (!GetModuleStatusEn(pModule->Type, i)) break;
			if (pModule->SaveChanges & k)	
			{
				memset(buffpath2, 0, 64);	
				sprintf(buffpath2, "%s;", GetModuleStatusValue(pModule->Type, i, pModule->Status[i], cBufferName, 64));
				strcat(buffpath, buffpath2);
			}
			k <<= 1;
		}
		strcat(buffpath, "\r\n");
		file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);
	}
	return 1;
}

int SaveCapturedData(char cSplit,
						capture_settings *capt_set, 
						char *cCapturePath, 
						char *cBackUpPath, 
						unsigned int uiCaptureSizeLimit, 
						unsigned int uiCaptureTimeSize, 
						unsigned char ucCaptOrderLmt,
						unsigned int ucBackUpCaptured, 
						char* cAddrOrderer,
						char* cPrePath,
						char* cType,
						unsigned int uiID,
						void *SFunc,
						void *pSaveData,
						unsigned int uiMaxWaitCopyTime, 
						unsigned int uiMessWaitCopyTime,
						unsigned int uiDestType)
{	
	file_struct *file_data = capt_set->FileData;
	STREAM_INFO *pStream = capt_set->Stream;
	tfSaveFunc SaveFunc = (tfSaveFunc)SFunc;
						
	
	char buffpath[MAX_PATH*2];
	char errorpath[MAX_PATH*2];
	char backuppath[MAX_PATH*2];
	time_t rawtime;
	struct tm timeinfo;
	struct timespec ts;
	int64_t tms;
	time(&rawtime);
	
	if ((file_data->opened == 1) && (cSplit ||
		((file_data->filelen / 1024) >= uiCaptureSizeLimit) ||
		((unsigned int)difftime(rawtime, file_data->createtime) >= uiCaptureTimeSize)))
	{
		file_close(capt_set->filehandle, file_data);
		
		if (ucBackUpCaptured)
			CopyFile(capt_set->FilePath, capt_set->BackUpFile, capt_set->ErrorFile, 0, (ucBackUpCaptured == 2) ? 1 : 0, pStream, ucCaptOrderLmt, cAddrOrderer,
					uiMaxWaitCopyTime, uiMessWaitCopyTime, uiDestType);
			
		DBG_FREE(capt_set->FilePath);
		if (capt_set->BackUpFile) DBG_FREE(capt_set->BackUpFile);
		capt_set->BackUpFile = NULL;
		if (capt_set->ErrorFile) DBG_FREE(capt_set->ErrorFile);
		capt_set->ErrorFile = NULL;
		DBG_MUTEX_LOCK(&pStream->mutex);
		memset(pStream->CurrentPath, 0, MAX_PATH);
		DBG_MUTEX_UNLOCK(&pStream->mutex);
	}
	
	if ((cSplit & 2) == 0)
	{
		if (file_data->opened == 0)
		{
			int i;
			
			localtime_r(&rawtime, &timeinfo);
			
			memset(errorpath, 0, MAX_PATH*2);					
			memset(buffpath, 0, MAX_PATH*2);			
			memset(backuppath, 0, MAX_PATH*2);
					
			if (ucBackUpCaptured != 2)
			{
				sprintf(buffpath, "%s%s%04i/%s/%02i/%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.CSV",
							cCapturePath, cPrePath,
							timeinfo.tm_year+1900,GetMonthName(timeinfo.tm_mon+1),timeinfo.tm_mday,
							timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
							(char*)&uiID, cType);
			}
			else
			{
				sprintf(buffpath, "%s%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.CSV",
							cCapturePath,
							timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
							(char*)&uiID, cType);
				sprintf(errorpath, "%s%s%04i/%s/%02i/%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.CSV",
							cCapturePath, cPrePath,
							timeinfo.tm_year+1900,GetMonthName(timeinfo.tm_mon+1),timeinfo.tm_mday,
							timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
							(char*)&uiID, cType);
			}
			
			sprintf(backuppath, "%s%s%04i/%s/%02i/%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.CSV",
							cAddrOrderer ? "" : cBackUpPath,
							cPrePath,
							timeinfo.tm_year+1900,GetMonthName(timeinfo.tm_mon+1),timeinfo.tm_mday,
							timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
							(char*)&uiID, cType);
			
			CreateDirectoryFromPath(buffpath, 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			capt_set->filehandle = file_open(buffpath, file_data);
			if (capt_set->filehandle)
			{
				DBG_MUTEX_LOCK(&pStream->mutex);
				memset(pStream->CurrentPath, 0, MAX_PATH);
				strcpy(pStream->CurrentPath, buffpath);
				DBG_MUTEX_UNLOCK(&pStream->mutex);
				
				i = strlen(buffpath);
				capt_set->FilePath = (char*)DBG_MALLOC(i + 1);
				memcpy(capt_set->FilePath, buffpath, i);
				capt_set->FilePath[i] = 0;
				
				if (ucBackUpCaptured)
				{
					i = strlen(errorpath);
					capt_set->ErrorFile = (char*)DBG_MALLOC(i + 1);
					memcpy(capt_set->ErrorFile, errorpath, i);
					capt_set->ErrorFile[i] = 0;
				
					i = strlen(backuppath);
					capt_set->BackUpFile = (char*)DBG_MALLOC(i + 1);
					memcpy(capt_set->BackUpFile, backuppath, i);
					capt_set->BackUpFile[i] = 0;
				}
				
				localtime_r(&rawtime, &timeinfo);
				clock_gettime(CLOCK_MONOTONIC, &ts);
				tms = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000000;
			
				memset(buffpath, 0, MAX_PATH*2);
				sprintf(buffpath, "Date;%04i.%02i.%02i;%02i:%02i:%02i;%i\r\n",
							timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
							(unsigned int)tms);		
				file_write(buffpath, strlen(buffpath), capt_set->filehandle, file_data);	
				
				SaveFunc(0, pSaveData, capt_set);				
			}
			else
			{
				file_data->opened = -1;
				dbgprintf(2, "Error create file '%s'\n", buffpath);
			}
		}
		if (((cSplit & 4) == 0) && (file_data->opened == 1))
		{
			SaveFunc(1, pSaveData, capt_set);
		}
	}
	
	return 1;
}

unsigned int GetCurrentSec()
{
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	unsigned int res;
	res = timeinfo.tm_year * 3214080;
	res += timeinfo.tm_mon * 267840;
	res += timeinfo.tm_mday * 8640;
	res += timeinfo.tm_hour * 360;
	res += timeinfo.tm_min * 60;
	return 	res;
}

void * thread_Eventer(void *pData)
{
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	misc_buffer * mBuff = (misc_buffer*)pData;
	char *cCapturePathEvents = &mBuff->data[0];
	char *cCapturePathActions = &mBuff->data[256];
	char *cBackUpPathEvents = &mBuff->data2[0];
	char *cBackUpPathActions = &mBuff->data2[256];
	
	unsigned int uiCaptEvntSizeLimit = mBuff->uidata[0];
	unsigned int uiCaptEvntTimeLimit = mBuff->uidata[1];
	unsigned int ucCaptEvntOrderLmt = mBuff->uidata[2];
	unsigned int ucBackUpEvntCaptured = mBuff->uidata[3];
	unsigned int ucCaptEvntEnabled = mBuff->uidata[4];
	unsigned int uiCaptActSizeLimit = mBuff->uidata[5];
	unsigned int uiCaptActTimeLimit = mBuff->uidata[6];
	unsigned int ucCaptActOrderLmt = mBuff->uidata[7];
	unsigned int ucBackUpActCaptured = mBuff->uidata[8];
	unsigned int ucCaptActEnabled = mBuff->uidata[9];
	unsigned int uiMaxWaitCopyTimeEvents = mBuff->uidata[10];
	unsigned int uiMessWaitCopyTimeEvents = mBuff->uidata[11];
	unsigned int uiMaxWaitCopyTimeActions = mBuff->uidata[12];
	unsigned int uiMessWaitCopyTimeActions = mBuff->uidata[13];

	char*	cCaptEvntOrderAddr = mBuff->void_data2;
	char*	cCaptEvntPrePath = mBuff->void_data3;
	char*	cCaptActOrderAddr = mBuff->void_data4;
	char*	cCaptActPrePath = mBuff->void_data5;
	
	int n, b, k, ret;
	int NewDay = 0;
	unsigned int iBuff[4];
	char cNameBuff[64];
	unsigned int uiID;
	ACTION_INFO maActInf;
	struct sockaddr_in ModAddress;
	struct sockaddr_in *pModAddress;
	unsigned int uiType = -1;
	char cSplitCaptures = 0;
	time_t rawtime;
	struct tm timeinfo;
	struct tm prev_timeinfo;
	memset(&timeinfo, 0, sizeof(struct tm));
	memset(&prev_timeinfo, 0, sizeof(struct tm));
	int NowTime;
	unsigned char NowDay;
	unsigned int uiEvent = 0;
	unsigned int uiLocalSysID;
	unsigned int uiLocalSysEvents;
	int iTestModuleStatus, iTest3ModuleStatus;
	int64_t iDiffTimer = 0;
	unsigned int uiBetweenTime;
	int64_t iLocalTimerEvent = 0;
	get_ms(&iLocalTimerEvent);
	
	MODULE_INFO tMI_Unknown;
	
	DBG_MUTEX_LOCK(&system_mutex);
	unsigned char cBcTCP = ucBroadCastTCP;
	cThreadEventerStatus = 1;
	get_ms(&iAccessTimer);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	b = ModuleTypeToNum(MODULE_TYPE_SYSTEM, 1);
	if (b >= 0)	
	{
		uiLocalSysID = miModuleList[b].ID; 
		uiLocalSysEvents = miModuleList[b].GenEvents & (1 << 14); 
	} else dbgprintf(1, "Eventer: error search sys type module\n");
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
				
	capture_settings capt_evnt_set;
	if (ucCaptEvntEnabled)
	{
		capt_evnt_set.FileData = file_init(1048576, 1024);
		capt_evnt_set.Stream = AddCaptureStream(CAPTURE_TYPE_EVENT, 0, 0);
		SaveCapturedData(4, &capt_evnt_set, cCapturePathEvents, cBackUpPathEvents, uiCaptEvntSizeLimit, uiCaptEvntTimeLimit, ucCaptEvntOrderLmt, ucBackUpEvntCaptured, cCaptEvntOrderAddr, cCaptEvntPrePath, 
						DIR_EVNT, uiLocalSysID, SaveEvents, NULL, uiMaxWaitCopyTimeEvents, uiMessWaitCopyTimeEvents, WRT_TYPE_BACKUP_EVENTS);	
	}
	
	capture_settings capt_act_set;
	if (ucCaptActEnabled)
	{
		capt_act_set.FileData = file_init(1048576, 1024);
		capt_act_set.Stream = AddCaptureStream(CAPTURE_TYPE_ACTION, 0, 0);
		SaveCapturedData(4, &capt_act_set, cCapturePathActions, cBackUpPathActions, uiCaptActSizeLimit, uiCaptActTimeLimit, ucCaptActOrderLmt, ucBackUpActCaptured, cCaptActOrderAddr, cCaptActPrePath, 
						DIR_ACT, uiLocalSysID, SaveActions, NULL, uiMaxWaitCopyTimeActions, uiMessWaitCopyTimeActions, WRT_TYPE_BACKUP_ACTIONS);	
	}
	
	tx_eventer_add_event(&modevent_evnt, EVENT_START);
	tx_eventer_add_event(&modevent_evnt, EVENT_STOP);
	tx_eventer_add_event(&modevent_evnt, EVENT_SPLIT_EVENTS);
	tx_eventer_add_event(&modevent_evnt, EVENT_SPLIT_ACTIONS);	
	while(1)
	{
		if (ucCaptEvntEnabled || ucCaptActEnabled || uiLocalSysEvents)
		{
			memcpy(&prev_timeinfo, &timeinfo, sizeof(struct tm));
			time(&rawtime);
			localtime_r(&rawtime, &timeinfo);
			NowTime = timeinfo.tm_hour * 10000;
			NowTime += (timeinfo.tm_min) * 100;
			NowTime += timeinfo.tm_sec;	
			NowDay = 1 << timeinfo.tm_wday;
			if (NowDay == 1) NowDay |= 128;
			NowDay = NowDay >> 1;
			if (NowTime == 0) NewDay++; else NewDay = 0;			
		}
		if (ucCaptEvntEnabled && ((NewDay == 1) || (cSplitCaptures == 1)))
		{
			dbgprintf(4, "Split event files\n");
			cSplitCaptures = 0;
			if (capt_evnt_set.FileData->opened == 1)
				SaveCapturedData(5, &capt_evnt_set, cCapturePathEvents, cBackUpPathEvents, uiCaptEvntSizeLimit, uiCaptEvntTimeLimit, ucCaptEvntOrderLmt, ucBackUpEvntCaptured, cCaptEvntOrderAddr, cCaptEvntPrePath, 
						DIR_EVNT, uiLocalSysID, SaveEvents, NULL, uiMaxWaitCopyTimeEvents, uiMessWaitCopyTimeEvents, WRT_TYPE_BACKUP_EVENTS);
		}
		if (ucCaptActEnabled && ((NewDay == 1) || (cSplitCaptures == 2)))
		{
			dbgprintf(4, "Split action files\n");
			cSplitCaptures = 0;
			if (capt_act_set.FileData->opened == 1)
				SaveCapturedData(5, &capt_act_set, cCapturePathActions, cBackUpPathActions, uiCaptActSizeLimit, uiCaptActTimeLimit, ucCaptActOrderLmt, ucBackUpActCaptured, cCaptActOrderAddr, cCaptActPrePath, 
						DIR_ACT, uiLocalSysID, SaveActions, NULL, uiMaxWaitCopyTimeActions, uiMessWaitCopyTimeActions, WRT_TYPE_BACKUP_ACTIONS);
		}
		
		uiEvent = 0;
		ret = tx_eventer_recv(&modevent_evnt, &uiEvent, 500, 0);
		if (ret != 0)
		{
			if (uiEvent == EVENT_STOP) break;
			if (uiEvent == EVENT_SPLIT_EVENTS) 
			{
				cSplitCaptures = 1;
				ret = 0;
			}
			if (uiEvent == EVENT_SPLIT_ACTIONS) 
			{
				cSplitCaptures = 2;
				ret = 0;
			}			
		}
		if (ret != 0)		
		{				
			DBG_MUTEX_LOCK(&modevent_mutex);
			for (n = 0; n < iModuleEventCnt; n++)
			{
				if (meModuleEventList[n].New != 0)
				{
					if (meModuleEventList[n].ExecNow != 0)
					{
						char cAllID = 0;
						if (meModuleEventList[n].ID == 0) cAllID = 1;
						iBuff[0] = meModuleEventList[n].ID;
						iBuff[1] = meModuleEventList[n].SubNumber;
						iBuff[2] = meModuleEventList[n].Status;
						memcpy(cNameBuff, meModuleEventList[n].Name, 64);
						DBG_MUTEX_UNLOCK(&modevent_mutex);
						
						DBG_MUTEX_LOCK(&modulelist_mutex);	
						if (cAllID) iBuff[0] = uiLocalSysID;
						b = ModuleIdToNum(iBuff[0], 2);
						if (b >= 0)
						{
							//if (miModuleList[b].Local)
							{
								ModuleAction(iBuff[0], iBuff[1], iBuff[2], cNameBuff);
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
							}
							/*else							
							{
								memcpy(&ModAddress, &miModuleList[b].Address, sizeof(ModAddress));	
								pModAddress = &ModAddress;
															
								DBG_MUTEX_UNLOCK(&modulelist_mutex);											
								SendTCPMessage(TYPE_MESSAGE_MODULE_SET, (char*)iBuff, 3*sizeof(unsigned int), cNameBuff, strlen(cNameBuff), pModAddress);
							}*/
						}
						else
						{
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_SET, (char*)iBuff, 3*sizeof(unsigned int), cNameBuff, strlen(cNameBuff));
							dbgprintf(3, "Exec Unknown ID:%.4s\n", (char*)&iBuff[0]);
						}	
						if (cAllID)
						{
							DBG_MUTEX_LOCK(&modulelist_mutex);
							for (b = 0; b < iModuleCnt; b++)
							{
								if ((miModuleList[b].Enabled & 1) && (miModuleList[b].Local == 0) && (miModuleList[b].Type == MODULE_TYPE_SYSTEM))
								{									
									iBuff[0] = miModuleList[b].ID;
									memcpy(&ModAddress, &miModuleList[b].Address, sizeof(ModAddress));	
									pModAddress = &ModAddress;
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									SendTCPMessage(TYPE_MESSAGE_MODULE_SET, (char*)iBuff, 3*sizeof(unsigned int), cNameBuff, strlen(cNameBuff), pModAddress);
									DBG_MUTEX_LOCK(&modulelist_mutex);
								}
							}
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
						}
						DBG_MUTEX_LOCK(&modevent_mutex);
					}
					else
					{
						time(&rawtime);
						localtime_r(&rawtime, &timeinfo);
						NowTime = timeinfo.tm_hour * 10000;
						NowTime += (timeinfo.tm_min) * 100;
						NowTime += timeinfo.tm_sec;					
						NowDay = 1 << timeinfo.tm_wday;
						if (NowDay == 1) NowDay |= 128;
						NowDay = NowDay >> 1;
						
						uiBetweenTime = (unsigned int)get_ms(&iDiffTimer);
						iDiffTimer = 0;
						get_ms(&iDiffTimer);
						
						ret = 0;
						DBG_MUTEX_LOCK(&evntaction_mutex);	
						for (b = 0; b < iActionCnt; b++)
						{
							//printf("1) %i, %i\n", ReqModuleStatus(maActionInfo[b].TestModuleID), maActionInfo[b].TestModuleStatus);
							//if (b==0) dbgprintf(2,"2) %i, %i\n", meModuleEventList[n].ID, maActionInfo[b].SrcID);
							//if (b==0) dbgprintf(2,"3) %i, %i\n", maActionInfo[b].SrcStatus, meModuleEventList[n].Status);
							/*printf("%i, %i,  %i, %i,  %i, %i,  %i, %i\n",meModuleEventList[n].ID , maActionInfo[b].SrcID,
														meModuleEventList[n].Status , maActionInfo[b].SrcStatus,
														meModuleEventList[n].SubNumber, maActionInfo[b].SrcSubNumber,
														maActionInfo[b].TestType, ACTION_TEST_TYPE_AND);*/
							//if (maActionInfo[b].Enabled)
								//printf("1.0) OffStat:%i WaitBeforeStatus:%i BetweenTime:%i WaitBeforeDone:%i\n", 
									//maActionInfo[b].TimerOffStatus, maActionInfo[b].TimerWaitBeforeStatus, uiBetweenTime, maActionInfo[b].TimerWaitBeforeDone);
							if (maActionInfo[b].TimerOffStatus) 
							{								
								if (maActionInfo[b].TimerOffStatus > uiBetweenTime) 
									maActionInfo[b].TimerOffStatus -= uiBetweenTime;
									else 
									{
										maActionInfo[b].TimerOffStatus = 0;
										maActionInfo[b].TimerWaitBeforeDone = 0;
									}
							}
							if (maActionInfo[b].TimerWaitBeforeStatus)
							{
								if (maActionInfo[b].TimerWaitBeforeStatus > uiBetweenTime) 
									maActionInfo[b].TimerWaitBeforeStatus -= uiBetweenTime;
									else 
									{
										maActionInfo[b].TimerWaitBeforeStatus = 0;
										maActionInfo[b].TimerWaitBeforeDone = 1;
									}
							}
							//if (maActionInfo[b].Enabled)
								//printf("1.1) OffStat:%i WaitBeforeStatus:%i BetweenTime:%i WaitBeforeDone:%i\n", 
									//maActionInfo[b].TimerOffStatus, maActionInfo[b].TimerWaitBeforeStatus, uiBetweenTime, maActionInfo[b].TimerWaitBeforeDone);
							
							//if (b == 0) printf("TimerOffStatus0: %i \n", maActionInfo[b].TimerOffStatus);
							if ((maActionInfo[b].TimerWaitBeforeDone == 1)
								||
								((maActionInfo[b].Enabled == 1)
									&&
									(
											(
												(
													(
														(((meModuleEventList[n].ID == maActionInfo[b].SrcID) || (maActionInfo[b].SrcID == 0)) 
															&& ((meModuleEventList[n].SubNumber == maActionInfo[b].SrcSubNumber) || (maActionInfo[b].SrcSubNumber == 0))
														)
														||
														((maActionInfo[b].SrcID == 0) && (maActionInfo[b].SrcSubNumber == 0))											
													)
													&&
													(
														((maActionInfo[b].TestType == ACTION_TEST_TYPE_EQUALLY) && (meModuleEventList[n].Status == maActionInfo[b].SrcStatus))
														||
														((maActionInfo[b].TestType == ACTION_TEST_TYPE_LESS) && (meModuleEventList[n].Status < maActionInfo[b].SrcStatus))
														||
														((maActionInfo[b].TestType == ACTION_TEST_TYPE_MORE) && (meModuleEventList[n].Status > maActionInfo[b].SrcStatus))
														||
														((maActionInfo[b].TestType == ACTION_TEST_TYPE_AND) && (meModuleEventList[n].Status & maActionInfo[b].SrcStatus))
														||
														((maActionInfo[b].TestType == ACTION_TEST_TYPE_NOT) && (meModuleEventList[n].Status != maActionInfo[b].SrcStatus))
														|| (maActionInfo[b].SrcSubNumber == 0)
													)
												)									
											)
											&& 
											(NowDay & maActionInfo[b].WeekDays)
											&& 
											(	((maActionInfo[b].NotBeforeTime <= maActionInfo[b].NotAfterTime) 
												&& (maActionInfo[b].NotBeforeTime <= NowTime) 
												&& (maActionInfo[b].NotAfterTime >= NowTime))
												||
												((maActionInfo[b].NotBeforeTime > maActionInfo[b].NotAfterTime) 
												&& ((maActionInfo[b].NotBeforeTime <= NowTime) 
												|| (maActionInfo[b].NotAfterTime >= NowTime)))
											)
											//&& (GetModuleStatus(maActionInfo[b].TestModuleID, maActionInfo[b].TestModuleSubNumber) == maActionInfo[b].TestModuleStatus)
										)
									)
								)
							{
								if (maActionInfo[b].TimerWaitBeforeDone == 0) 
								{
									iTestModuleStatus = GetModuleStatus(maActionInfo[b].TestModuleID, maActionInfo[b].TestModuleSubNumber);
									iTest3ModuleStatus = GetModuleStatus(maActionInfo[b].Test3ModuleID, maActionInfo[b].Test3ModuleSubNumber);
								}
								if ((maActionInfo[b].TimerWaitBeforeDone == 1) 
									||
									(
										(
											((maActionInfo[b].Test2Type == ACTION_TEST_TYPE_EQUALLY) && (iTestModuleStatus == maActionInfo[b].TestModuleStatus))
											||
											((maActionInfo[b].Test2Type == ACTION_TEST_TYPE_LESS) && (iTestModuleStatus < maActionInfo[b].TestModuleStatus))
											||
											((maActionInfo[b].Test2Type == ACTION_TEST_TYPE_MORE) && (iTestModuleStatus > maActionInfo[b].TestModuleStatus))
											||
											((maActionInfo[b].Test2Type == ACTION_TEST_TYPE_AND) && (iTestModuleStatus & maActionInfo[b].TestModuleStatus))
											||
											((maActionInfo[b].Test2Type == ACTION_TEST_TYPE_NOT) && (iTestModuleStatus != maActionInfo[b].TestModuleStatus))
											|| (maActionInfo[b].TestModuleSubNumber == 0)
										)
										&&
										(
											((maActionInfo[b].Test3Type == ACTION_TEST_TYPE_EQUALLY) && (iTest3ModuleStatus == maActionInfo[b].Test3ModuleStatus))
											||
											((maActionInfo[b].Test3Type == ACTION_TEST_TYPE_LESS) && (iTest3ModuleStatus < maActionInfo[b].Test3ModuleStatus))
											||
											((maActionInfo[b].Test3Type == ACTION_TEST_TYPE_MORE) && (iTest3ModuleStatus > maActionInfo[b].Test3ModuleStatus))
											||
											((maActionInfo[b].Test3Type == ACTION_TEST_TYPE_AND) && (iTest3ModuleStatus & maActionInfo[b].Test3ModuleStatus))
											||
											((maActionInfo[b].Test3Type == ACTION_TEST_TYPE_NOT) && (iTest3ModuleStatus != maActionInfo[b].Test3ModuleStatus))
											|| (maActionInfo[b].Test3ModuleSubNumber == 0)
										)
									)
									)								
								{	
									//if (maActionInfo[b].Enabled)
										//printf("2.0) WaitBeforeDone:%i OffStatus:%i WaitBeforeStatus:%i\n", 
											//	maActionInfo[b].TimerWaitBeforeDone, maActionInfo[b].TimerOffStatus, maActionInfo[b].TimerWaitBeforeStatus);
									if (maActionInfo[b].TimerWaitBeforeDone == 0)
									{
										if ((maActionInfo[b].TimerWaitBefore) && ((maActionInfo[b].TimerWaitBeforeStatus == 0) || (maActionInfo[b].RestartTimer & 1)))
													maActionInfo[b].TimerWaitBeforeStatus = maActionInfo[b].TimerWaitBefore;										
									}
									if (maActionInfo[b].TimerWaitBeforeDone == 2)
									{
										if ((!(maActionInfo[b].AfterAccept & PA_OFF)) && (maActionInfo[b].TimerOff) && (maActionInfo[b].RestartTimer & 2))
											maActionInfo[b].TimerOffStatus = maActionInfo[b].TimerOff;										
									}
									
									//printf("TimerWaitBeforeDone %i  TimerWaitBeforeStatus %i\n", maActionInfo[b].TimerWaitBeforeDone, maActionInfo[b].TimerWaitBeforeStatus);
									//if (b == 0) printf("TimerOffStatus2 %i \n",maActionInfo[b].TimerOffStatus);
									if ((maActionInfo[b].TimerWaitBeforeDone == 1) || 
										((maActionInfo[b].TimerWaitBeforeDone == 0) && (maActionInfo[b].TimerWaitBeforeStatus == 0) && (maActionInfo[b].TimerOffStatus == 0)))
									{
										if (maActionInfo[b].TimerWaitBeforeDone == 1) 
										{
											if (maActionInfo[b].TimerOffStatus) maActionInfo[b].TimerWaitBeforeDone = 2;
												else maActionInfo[b].TimerWaitBeforeDone = 3;
										}
										//if (b == 0) printf("TimerOffStatus3 %i \n",maActionInfo[b].TimerOffStatus);
									
										ret = 1;
										memcpy(&maActInf, &maActionInfo[b], sizeof(ACTION_INFO));
										uiID = meModuleEventList[n].ID;
										DBG_MUTEX_UNLOCK(&evntaction_mutex);
										DBG_MUTEX_UNLOCK(&modevent_mutex);
										if ((maActInf.SrcID == 0) && (maActInf.SrcStatus != 0)) 
										{
											if (uiID != 0)
											{
												DBG_MUTEX_LOCK(&modulelist_mutex);	
												k = ModuleIdToNum(uiID, 2);
												if (k >= 0) uiID = miModuleList[k].Type; else uiID = MODULE_TYPE_UNKNOWN;										
												DBG_MUTEX_UNLOCK(&modulelist_mutex);
												if ((uiID != MODULE_TYPE_SYSTEM) && (uiID != MODULE_TYPE_UNKNOWN)) ret = 0;
											}
										}
										if (ret == 1)
										{
											//if (maActionInfo[b].TimerOff) maActionInfo[b].TimerOffStatus = maActionInfo[b].TimerOff;
												dbgprintf(6, "ExecuteAction %.4s, %.4s, %s\n", (char*)&maActInf.ActionID, (char*)&maActInf.DestID, maActInf.Name);
												//printf("ExecuteAction %.4s, %.4s, %s\n", (char*)&maActInf.ActionID, (char*)&maActInf.DestID, maActInf.Name);
												//if (b == 0) printf("%i, %i,   %i, %i\n",maActInf.SrcID,maActInf.SrcStatus, maActInf.DestID , maActInf.DestStatus);
												maActInf.CounterExec++;
												pModAddress = NULL;
												uiType = -1;
												if (maActInf.DestID != 0)
												{
													
													DBG_MUTEX_LOCK(&modulelist_mutex);					
													for (k = 0; k < iModuleCnt; k++) 
													{
														if ((miModuleList[k].Enabled & 1) && (miModuleList[k].ID == maActInf.DestID))
														{
															memcpy(&ModAddress, &miModuleList[k].Address, sizeof(ModAddress));	
															pModAddress = &ModAddress;
															uiType = miModuleList[k].Type;
															break;
														}
													}
													k = ModuleIdToNum(maActInf.DestID, 1);
													if (k >= 0)
													{
														ModuleAction(maActInf.DestID, maActInf.DestSubNumber, maActInf.DestStatus, maActInf.Name);
														DBG_MUTEX_UNLOCK(&modulelist_mutex);								
													}
													else
													{
														DBG_MUTEX_UNLOCK(&modulelist_mutex);											
														if ((pModAddress) && (uiType == MODULE_TYPE_SYSTEM) && (maActInf.DestStatus == SYSTEM_CMD_REDIRECT))
														{
															tMI_Unknown.ID = maActInf.SrcID;															
															if ((maActInf.SrcSubNumber <= 0) || (maActInf.SrcSubNumber > MAX_MODULE_STATUSES)) k = 1;
																else k = maActInf.SrcSubNumber;
															memset(tMI_Unknown.Status, 0, MAX_MODULE_STATUSES * sizeof(tMI_Unknown.Status[0]));
															tMI_Unknown.Status[k - 1] = maActInf.SrcStatus;
															uiType = TYPE_MESSAGE_MODULE_STATUS_CHANGED;															
														}
														else
														{
															iBuff[0] = maActInf.DestID;
															iBuff[1] = maActInf.DestSubNumber;
															iBuff[2] = maActInf.DestStatus;
															uiType = TYPE_MESSAGE_MODULE_SET;
														}
														if (pModAddress)
														{
															if (uiType == TYPE_MESSAGE_MODULE_SET) 
																SendTCPMessage(uiType, (char*)iBuff, 3*sizeof(unsigned int), maActInf.Name, strlen(maActInf.Name), pModAddress);
																else
																SendTCPMessage(uiType, (char*)&tMI_Unknown, sizeof(MODULE_INFO_TRANSFER), (char*)&k, sizeof(k), pModAddress);
														}
														else
														{
															if (uiType == TYPE_MESSAGE_MODULE_SET) 
																SendBroadCastMessage(cBcTCP, uiType, (char*)iBuff, 3*sizeof(unsigned int), maActInf.Name, strlen(maActInf.Name));
																else
																SendBroadCastMessage(cBcTCP, uiType, (char*)&tMI_Unknown, sizeof(MODULE_INFO_TRANSFER), (char*)&k, sizeof(k));
															dbgprintf(3, "Unknown ID:%.4s in Action:%.4s(%i)\n", (char*)&iBuff[0], &maActInf.ActionID, b);
														}
													}
												}
												else
												{																												
													memset(tMI_Unknown.Status, 0, MAX_MODULE_STATUSES * sizeof(tMI_Unknown.Status[0]));
													switch(maActInf.DestSubNumber)
													{
														case 2:
															if ((maActInf.TestModuleSubNumber > 0) && (maActInf.TestModuleSubNumber <= MAX_MODULE_STATUSES)) 
																k = maActInf.TestModuleSubNumber; else k = 1;
															tMI_Unknown.ID = maActInf.TestModuleID;
															tMI_Unknown.Status[k-1] = iTestModuleStatus;
															break;
														case 3:
															if ((maActInf.Test3ModuleSubNumber > 0) && (maActInf.Test3ModuleSubNumber <= MAX_MODULE_STATUSES)) 
																k = maActInf.Test3ModuleSubNumber; else k = 1;
															tMI_Unknown.ID = maActInf.Test3ModuleID;
															tMI_Unknown.Status[k-1] = iTest3ModuleStatus;
															break;
														case 0:
														case 1:
														default:
															if ((maActInf.SrcSubNumber > 0) && (maActInf.SrcSubNumber <= MAX_MODULE_STATUSES)) 
																k = maActInf.SrcSubNumber; else k = 1;
															tMI_Unknown.ID = maActInf.SrcID;
															tMI_Unknown.Status[k-1] = maActInf.SrcStatus;
															break;															
													}
													SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&tMI_Unknown, sizeof(MODULE_INFO_TRANSFER), (char*)&k, sizeof(k));
												}	
												if (maActInf.AfterAccept & PA_STOP) ret = 2;
												if (maActInf.AfterAccept & PA_OFF) maActInf.Enabled = 0;
												
												if (ucCaptActEnabled)
														SaveCapturedData(0, &capt_act_set, cCapturePathActions, cBackUpPathActions, uiCaptActSizeLimit, uiCaptActTimeLimit, ucCaptActOrderLmt, 
																			ucBackUpActCaptured, cCaptActOrderAddr, cCaptActPrePath, 
																			DIR_ACT, uiLocalSysID, SaveActions, &maActInf, uiMaxWaitCopyTimeActions, uiMessWaitCopyTimeActions, WRT_TYPE_BACKUP_ACTIONS);	
											
										}									
										DBG_MUTEX_LOCK(&modevent_mutex);
										DBG_MUTEX_LOCK(&evntaction_mutex);
										maActionInfo[b].Enabled = maActInf.Enabled;
										maActionInfo[b].CounterExec = maActInf.CounterExec;
										if (ret == 0)
										{
											maActionInfo[b].TimerWaitBeforeDone = 0;
											maActionInfo[b].TimerOffStatus = 0;
											maActionInfo[b].TimerWaitBeforeDone = 0;
										}
										//if (ret == 2) break;
									}
									if (maActionInfo[b].TimerWaitBeforeDone == 0)
									{
										if ((!(maActionInfo[b].AfterAccept & PA_OFF)) && (maActionInfo[b].TimerOff) && ((maActionInfo[b].TimerOffStatus == 0) || (maActionInfo[b].RestartTimer & 3)))
											maActionInfo[b].TimerOffStatus = maActionInfo[b].TimerOff;																			
									}
									if (maActionInfo[b].TimerWaitBeforeDone == 3) maActionInfo[b].TimerWaitBeforeDone = 0;
									if (ret == 2) break;
								}
							}
						}
						DBG_MUTEX_UNLOCK(&evntaction_mutex);
						if (ret == 0) 
						{
							AddSkipEventInList(meModuleEventList[n].ID, meModuleEventList[n].SubNumber, meModuleEventList[n].Status);
							dbgprintf(6, "Skipped Event (no actions) ID:%.4s, SubNum:%i, Status:%i(%.4s)\n",(char*)&meModuleEventList[n].ID, meModuleEventList[n].SubNumber, meModuleEventList[n].Status, (char*)&meModuleEventList[n].Status);							
						}
					}
					if (ucCaptEvntEnabled)
						SaveCapturedData(0, &capt_evnt_set, cCapturePathEvents, cBackUpPathEvents, uiCaptEvntSizeLimit, uiCaptEvntTimeLimit, ucCaptEvntOrderLmt, ucBackUpEvntCaptured, cCaptEvntOrderAddr, cCaptEvntPrePath, 
										DIR_EVNT, uiLocalSysID, SaveEvents, &meModuleEventList[n], uiMaxWaitCopyTimeEvents, uiMessWaitCopyTimeEvents, WRT_TYPE_BACKUP_EVENTS);	
					meModuleEventList[n].New = 0;
				}
			}
			DBG_MUTEX_UNLOCK(&modevent_mutex);
		}
		if ((unsigned int)get_ms(&iAccessTimer) > 10000)
		{
			DBG_MUTEX_LOCK(&system_mutex);
			iAccessLevel = iDefAccessLevel;
			iAccessTimer = 0;
			get_ms(&iAccessTimer);
			DBG_MUTEX_UNLOCK(&system_mutex);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);	
			n = ModuleTypeToNum(MODULE_TYPE_SYSTEM, 1);
			if (n >= 0) miModuleList[n].Status[1] = iAccessLevel;
			DBG_MUTEX_UNLOCK(&modulelist_mutex);		
			//printf("Access down = %i\n",0);			
		}
		if (uiLocalSysEvents)
		{
			if ((unsigned int)get_ms(&iLocalTimerEvent) >= 500)
			{
				AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_EVENT_TIMER, NULL, 0, 0);	
				iLocalTimerEvent = 0;
				get_ms(&iLocalTimerEvent);
			}
			if (timeinfo.tm_hour != prev_timeinfo.tm_hour) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_EVENT_HOURS, NULL, 0, 0);
			if (timeinfo.tm_min != prev_timeinfo.tm_min) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_EVENT_MINUTES, NULL, 0, 0);
			if (timeinfo.tm_sec != prev_timeinfo.tm_sec) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_EVENT_SECONDS, NULL, 0, 0);
		}
	}
	if (ucCaptEvntEnabled && (capt_evnt_set.FileData->opened == 1))
	{
		SaveCapturedData(2, &capt_evnt_set, cCapturePathEvents, cBackUpPathEvents, uiCaptEvntSizeLimit, uiCaptEvntTimeLimit, ucCaptEvntOrderLmt, ucBackUpEvntCaptured, cCaptEvntOrderAddr, cCaptEvntPrePath, 
				DIR_EVNT, uiLocalSysID, SaveEvents, NULL, uiMaxWaitCopyTimeEvents, uiMessWaitCopyTimeEvents, WRT_TYPE_BACKUP_EVENTS);
		file_deinit(capt_evnt_set.FileData);			
	}
	if (ucCaptActEnabled && (capt_act_set.FileData->opened == 1))
	{
		SaveCapturedData(2, &capt_act_set, cCapturePathActions, cBackUpPathActions, uiCaptActSizeLimit, uiCaptActTimeLimit, ucCaptActOrderLmt, ucBackUpActCaptured, cCaptActOrderAddr, cCaptActPrePath, 
				DIR_ACT, uiLocalSysID, SaveActions, NULL, uiMaxWaitCopyTimeActions, uiMessWaitCopyTimeActions, WRT_TYPE_BACKUP_ACTIONS);
		file_deinit(capt_act_set.FileData);			
	}
	
	DBG_FREE(cCaptEvntPrePath);
	if (cCaptEvntOrderAddr) DBG_FREE(cCaptEvntOrderAddr);
	DBG_FREE(cCaptActPrePath);
	if (cCaptActOrderAddr) DBG_FREE(cCaptActOrderAddr);
	
	DBG_FREE(mBuff->data);
	DBG_FREE(mBuff->data2);
	DBG_FREE(mBuff);
	
	DBG_LOG_OUT();
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadEventerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)1;
}

void * thread_Scaner(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	misc_buffer * mBuff = (misc_buffer*)pData;
	MODULE_INFO * pModules = (MODULE_INFO *)mBuff->void_data;
	MODULE_INFO * pLocalModule = NULL;
	unsigned int iModulesCount = mBuff->data_size;
	char *cCapturePath = mBuff->data;
	char *cBackUpPath = mBuff->data2;
	unsigned int uiCaptureSizeLimit = mBuff->uidata[0];
	unsigned int uiCaptureTimeLimit = mBuff->uidata[1];
	unsigned int ucCaptOrderLmt = mBuff->uidata[2];
	unsigned int ucBackUpCaptured = mBuff->uidata[3];
	char cCaptStatEnabled = mBuff->uidata[4];
	unsigned int uiMaxWaitCopyTime = mBuff->uidata[5];
	unsigned int uiMessWaitCopyTime = mBuff->uidata[6];

	char*	cCaptOrderAddr = mBuff->void_data2;
	char*	cCaptPrePath = mBuff->void_data3;	
	DBG_FREE(mBuff);
	
	DBG_MUTEX_LOCK(&system_mutex);
	char cLoop = 1;
	unsigned char cBcTCP = ucBroadCastTCP;
	cThreadScanerStatus = 1;
	cThreadScanerStatusUsbio = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
			
	int i, oclk, subnum;
	char cSplitCaptures = 0;
	int iOutBuffLen = 256;
	uint16_t *cOutBuffer = DBG_MALLOC(iOutBuffLen * sizeof(uint16_t));
		
	time_t rawtime;
	struct tm timeinfo;
	int NowTime;
	int NewDay = 0;
	unsigned char NowDay;
	
	unsigned int uiIrNoiseTimer = 0;
	unsigned int uiPrevID = 0;
	
	int n, prevstat[MAX_MODULE_STATUSES], uiID;
	unsigned int *uiScanTimer = (unsigned int*)DBG_MALLOC(iModulesCount*sizeof(unsigned int));
	unsigned int *uiScanFirst = (unsigned int*)DBG_MALLOC(iModulesCount*sizeof(unsigned int));
	capture_settings *capt_set = (capture_settings*)DBG_MALLOC(iModulesCount*sizeof(capture_settings));
	memset(capt_set, 0, iModulesCount*sizeof(capture_settings));
	
	unsigned int result, result_evn;
	char cUsbIoSleep = 0;
	int keyboard_exist = 0;
	//iTimeReset = 0;
	for (n = 0; n != iModulesCount; n++)
		if (pModules[n].Enabled & 1) 
		{
			if (pModules[n].ScanSet != 0)
			{
				for (i = 0; i != MAX_MODULE_STATUSES; i++) pModules[n].Status[i] = 0;
				uiScanTimer[n] = 0; //pModules[n].ScanSet;
				uiScanFirst[n] = 1;
				if (pModules[n].Type == MODULE_TYPE_KEYBOARD) keyboard_exist = 1;
				
				if ((pModules[n].SaveChanges != 0) && cCaptStatEnabled)
				{
					capt_set[n].FileData = file_init(1048576, 1024);
					capt_set[n].Stream = AddCaptureStream(CAPTURE_TYPE_MODULE, 0, 0);
					SaveCapturedData(4, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
										ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
										uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
				}
			}			
		}
	if (keyboard_exist) init_keyboard_scan();
	
	while(cLoop)
	{
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		NowTime = timeinfo.tm_hour * 10000;
		NowTime += (timeinfo.tm_min) * 100;
		NowTime += timeinfo.tm_sec;	
		NowDay = 1 << timeinfo.tm_wday;
		if (NowDay == 1) NowDay |= 128;
		NowDay = NowDay >> 1;
		if (NowTime == 0) NewDay++; else NewDay = 0;
		
		for (n = 0; n != iModulesCount; n++)
		{
			if ((pModules[n].Enabled & 1) && (pModules[n].ScanSet != 0))
			{
				uiScanTimer[n]++;
				if ((pModules[n].ScanSet <= uiScanTimer[n]) || (pModules[n].Type == MODULE_TYPE_CAMERA))
				{
					uiScanTimer[n] = 0;
					for (i = 0; i < MAX_MODULE_STATUSES; i++) prevstat[i] = pModules[n].Status[i];
					if (pModules[n].Type == MODULE_TYPE_RS485) prevstat[0] = -2;
					
					UpdateModuleStatus(&pModules[n], 0);
					result = 0;
					if ((pModules[n].Type == MODULE_TYPE_USB_GPIO) ||
						(pModules[n].Type == MODULE_TYPE_GPIO) ||
						(pModules[n].Type == MODULE_TYPE_TEMP_SENSOR) ||
						(pModules[n].Type == MODULE_TYPE_ADS1015) ||
						(pModules[n].Type == MODULE_TYPE_MCP3421) ||
						(pModules[n].Type == MODULE_TYPE_AS5600) ||
						(pModules[n].Type == MODULE_TYPE_HMC5883L) ||
						(pModules[n].Type == MODULE_TYPE_CAMERA) ||
						(pModules[n].Type == MODULE_TYPE_MEMORY) ||
						(pModules[n].Type == MODULE_TYPE_COUNTER) ||
						(pModules[n].Type == MODULE_TYPE_MIC) ||
						(pModules[n].Type == MODULE_TYPE_SYSTEM) ||
						(pModules[n].Type == MODULE_TYPE_RS485) ||
						(pModules[n].Type == MODULE_TYPE_KEYBOARD) ||
						(pModules[n].Type == MODULE_TYPE_EXTERNAL) ||
						(pModules[n].Type == MODULE_TYPE_SPEAKER))						
					{	
						if ((pModules[n].Type != MODULE_TYPE_CAMERA) && (pModules[n].Type != MODULE_TYPE_MIC) && 
							(pModules[n].Type != MODULE_TYPE_MEMORY) && (pModules[n].Type != MODULE_TYPE_COUNTER)) 
						{
							DBG_MUTEX_LOCK(&modulelist_mutex);
							pLocalModule = ModuleIdToPoint(pModules[n].ID, 1);
							if (pLocalModule != NULL)
							{						
								//for (i = 0; i < MAX_MODULE_STATUSES; i++) pLocalModule->Status[i] = pModules[n].Status[i];
								memcpy(pLocalModule->Status, pModules[n].Status, sizeof(pLocalModule->Status[0]) * MAX_MODULE_STATUSES);															
							} 
							else 
							{
								dbgprintf(1, "Error in Scaner, not finded local module ID:%.4s\n", (char*)&pModules[n].ID);
								pModules[n].Enabled ^= 1;
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								continue;
							}
							DBG_MUTEX_UNLOCK(&modulelist_mutex);						
						}
						for (i = 0; i < MAX_MODULE_STATUSES; i++) 
							if ((prevstat[i] != pModules[n].Status[i]) || uiScanFirst[n]) result |= (1 << i);
						if (uiScanFirst[n]) uiScanFirst[n] = 0;
						
						result_evn = pModules[n].GenEvents & result;
						if (result_evn != 0)				
						{
							dbgprintf(5, "Scaner: change module status ID:%.4s Statuses:%i\n", (char*)&pModules[n].ID, result_evn);
							//printf("scan %i, %i\n", result_evn, pModules[n].Status[2]);
							for (i = 0; i < MAX_MODULE_STATUSES; i++)
								if (result_evn & (1 << i)) 
									AddModuleEventInList(pModules[n].ID, i+1, pModules[n].Status[i], NULL, 0, 0);
						}
						result_evn = pModules[n].Global & result;
						if (result_evn != 0)				
							SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&pModules[n], sizeof(MODULE_INFO_TRANSFER), (char*)&result_evn, sizeof(result_evn));
						
						if ((pModules[n].SaveChanges & result) && cCaptStatEnabled)
								SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
												ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
												uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
					}
					if (pModules[n].Type == MODULE_TYPE_TIMER)
					{
						if (pModules[n].Settings[2] != 0) pModules[n].Settings[2]--;
						
						if ((pModules[n].Settings[2] == 0) && (pModules[n].Settings[1] == NowTime) && (NowDay & pModules[n].Settings[0]))
						{
							dbgprintf(5, "Scaner: MODULE_TYPE_TIMER :%.4s\n", (char*)&pModules[n].ID);							
							result = 1;
							result_evn = pModules[n].GenEvents & result;
							if (result_evn) 
								AddModuleEventInList(pModules[n].ID, 1, result_evn, NULL, 0, 0);
							result_evn = pModules[n].Global & result;
							if (result_evn)
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&pModules[n], sizeof(MODULE_INFO_TRANSFER), (char*)&result_evn, sizeof(result_evn));
							if ((pModules[n].SaveChanges & result) && cCaptStatEnabled) 
								SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
													ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
													uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
							pModules[n].Settings[2] = 30;
						}								
					}
					if ((pModules[n].Type == MODULE_TYPE_USB_GPIO) && (cUsbIoSleep == 0))
					{		
						for (i = 0; i < pModules[n].SettingsCount; i++)
						{
							//printf("MODULE_TYPE_USB_GPIO %i %i\n", pModules[n].SettingList[i].Enabled, pModules[n].Status[i]);						
							if ((pModules[n].SettingList[i].Enabled) && (pModules[n].Status[i]))
							{
								if (pModules[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_IR_RECIEVER)
								{
									uiID = pModules[n].SettingList[i].ID;
									if ((pModules[n].ParamList[uiID].Enabled) && (pModules[n].ParamList[uiID].CurrentType == USBIO_PIN_SETT_TYPE_IR_RECIEVER))
									{									
										if (usb_gpio_get_ir_data(&pModules[n], cOutBuffer, iOutBuffLen, (unsigned int*)&oclk))
										{
											if (GetIrDataID(cOutBuffer, oclk, (unsigned int*)&uiID))
											{
												result = (1 << i);
												result_evn = pModules[n].GenEvents & result;
												if (result_evn) 
													AddModuleEventInList(pModules[n].ID, i+1, uiID, NULL, 0, 0);
												result_evn = pModules[n].Global & result;
												if (result_evn)
													SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&pModules[n], sizeof(MODULE_INFO_TRANSFER), (char*)&result_evn, sizeof(result_evn));
												if ((pModules[n].SaveChanges & result) && cCaptStatEnabled)
												{
													pModules[n].Status[i] = uiID;
													SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
																			ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
																			uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
												}						
												dbgprintf(6, "IR command: %.4s, %.4s\n", (char*)&pModules[n].ID, (char*)&uiID);
												//printf("IR command: %.4s, %.4s\n", (char*)&pModules[n].ID, (char*)&uiID);
											}
											else
											{
												AddSkipIrCodeInList(cOutBuffer, oclk);
												PrintIRCmd(cOutBuffer, oclk);
											}										
										}
										pModules[n].Status[i] = 0;
									}								
								}
								if ((pModules[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_RS485_RC522) ||
									(pModules[n].SettingList[i].CurrentType == USBIO_PIN_SETT_TYPE_RS485_PN532))
								{
									subnum = pModules[n].SettingList[i].ID;
									if ((pModules[n].ParamList[subnum].Enabled) && 
										((pModules[n].ParamList[subnum].CurrentType == USBIO_PIN_SETT_TYPE_RS485_RC522) ||
										(pModules[n].ParamList[subnum].CurrentType == USBIO_PIN_SETT_TYPE_RS485_PN532)))
									{
										uint8_t serial[MAX_SECURITY_SERIAL_LEN];
										uint8_t sensnum;
										unsigned int seriallength;
										if (usb_gpio_get_card_serial(&pModules[n], &sensnum, serial, MAX_SECURITY_SERIAL_LEN, &seriallength))
										{
											dbgprintf(4, "The serial of card (USBIO %i): %i, %i, %i, %i, %i\n", 
														i, serial[0], serial[1], serial[2], serial[3], serial[4]);
														
											DBG_MUTEX_LOCK(&securitylist_mutex);
											int iUpdateStatus = 0;
											unsigned int iUpdaterID;
											SECURITY_KEY_INFO skiUpdateInfo;
											if (iUpdateKeyInfoAction && ((unsigned int)get_ms(&iUpdateKeyInfoTimer) > 3000)) iUpdateKeyInfoAction = 0;										
											if (iUpdateKeyInfoAction && (pModules[n].ID == uiUpdateKeyInfoReader)) 
											{
												int p = seriallength + 1;											
												if (seriallength == skiUpdateKeyInfo.SerialLength)
													for (p = 0; p < seriallength;p++) if (serial[p] != skiUpdateKeyInfo.Serial[p]) break;												
												if (p == seriallength)
												{
													iUpdateStatus = iUpdateKeyInfoAction;
													iUpdaterID = uiUpdateKeyInfoReader;
													memcpy(&skiUpdateInfo, &skiUpdateKeyInfo, sizeof(SECURITY_KEY_INFO));
												}
												iUpdateKeyInfoAction = 0;
											}
											DBG_MUTEX_UNLOCK(&securitylist_mutex);
								
											if (iUpdateStatus && (pModules[n].ID == iUpdaterID)) 
											{
												UpdateSecurityKey(iUpdateStatus, &pModules[n], &skiUpdateInfo, sensnum);								
											}
											else
											{	
												unsigned int uiID = 0;
												result = (1 << i);
												result_evn = pModules[n].GenEvents & result;
												int status = CreateEventsForSmartCard(&pModules[n], i, serial, seriallength, &uiID);
												if (status == 0)							
												{
													if (result_evn) CreateModuleEvent(&pModules[n], i, 4);
													AddAlienKeyInList(serial, seriallength);
													dbgprintf(2,"New smartcard (usbio %i). Ignore\n", i);
													PrintCardSerial(serial, seriallength);
												}	
												if (status == -1)							
												{
													if (result_evn) CreateModuleEvent(&pModules[n], i, 5);
													dbgprintf(2,"Error auth smartcard (usbio %i). Ignore\n", i);	
												}
												if (status > 0)
												{
													if (result_evn) CreateModuleEvent(&pModules[n], i, uiID);												
													if ((pModules[n].SaveChanges & result) && cCaptStatEnabled)
													{
														pModules[n].Status[i] = uiID;													
														SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
																			ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
																			uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);																	
													}
													dbgprintf(6, "Card ID: %.4s from module: %.4s(%i)\n", (char*)&uiID, (char*)&pModules[n].ID, subnum);
												}
											}										
											
											/*if (GetSmartCardID(cOutBuffer, oclk, (unsigned int*)&uiID))
											{
												result = (1 << i);
												result_evn = pModules[n].GenEvents & result;
												if (result_evn) 
												{
													AddModuleEventInList(pModules[n].ID, i+1, uiID, NULL, 0, 0);
													if (pModules[n].Global & result)
													{
														clk = 1;
														SendUDPMessage(TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&pModules[n], sizeof(MODULE_INFO_TRANSFER), (char*)&clk, sizeof(clk), NULL);			
													}
												}
												if ((pModules[n].SaveChanges & result) && cCaptStatEnabled)
														SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
																			ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
																			uiMaxWaitCopyTime, uiMessWaitCopyTime);
																			
												dbgprintf(6, "Card ID: %.4s from module: %.4s(%i)\n", (char*)&uiID, (char*)&pModules[n].ID, subnum);
											}
											else
											{
												AddAlienKeyInList(cOutBuffer[0], cOutBuffer[1], cOutBuffer[2], cOutBuffer[3]);
												PrintCardSerial(cOutBuffer, oclk);
											}	*/									
										}
										pModules[n].Status[i] = 0;
									}								
								}
							}
						}
					}
					if (pModules[n].Type == MODULE_TYPE_IR_RECEIVER)
					{
						if (uiIrNoiseTimer < pModules[n].Settings[1]) uiIrNoiseTimer++;
						oclk = ir_read(cOutBuffer, iOutBuffLen);
						if (oclk > 0)
						{
							if (GetIrDataID(cOutBuffer, oclk, (unsigned int*)&uiID))
							{
								if ((uiPrevID != uiID) || (uiIrNoiseTimer >= pModules[n].Settings[1]))
								{
									uiIrNoiseTimer = 0;
									pModules[n].Status[0] = uiID;
									result = 1;
									result_evn = pModules[n].GenEvents & result;
									if (result_evn) 
										AddModuleEventInList(pModules[n].ID, 1, uiID, NULL, 0, 0);
									result_evn = pModules[n].Global & result;
									if (result_evn)
										SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&pModules[n], sizeof(MODULE_INFO_TRANSFER), (char*)&result_evn, sizeof(result_evn));			
									if ((pModules[n].SaveChanges & result) && cCaptStatEnabled) 
											SaveCapturedData(0, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
															ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
															uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
															
									pModules[n].Status[0] = 0;
									dbgprintf(6, "IR command: %.4s, %.4s\n", (char*)&pModules[n].ID, (char*)&uiID);
								}
								uiPrevID = uiID;
							}
							else
							{
								AddSkipIrCodeInList(cOutBuffer, oclk);
								PrintIRCmd(cOutBuffer, oclk);
							}
						}
					}
				}
				else
				{
					//if (pModules[n].Type == MODULE_TYPE_IR_RECEIVER) ir_read(cInBuffer, iInBuffLen, 0);					
				}
			}
		}
		usleep(50000);
		DBG_MUTEX_LOCK(&system_mutex);
		cLoop = cThreadScanerStatus;
		if (cThreadScanerStatus == 2) cLoop = 0; else cLoop = 1;
		if (cThreadScanerStatus == 3)
		{
			cSplitCaptures = 1;
			cThreadScanerStatus = 1;
		}
		if (cThreadScanerStatusUsbio == 1) 
		{
			cUsbIoSleep = 1;
			cThreadScanerStatusUsbio = 2;
		}
		if ((cThreadScanerStatusUsbio == 0) && cUsbIoSleep)	cUsbIoSleep = 0;
		
		DBG_MUTEX_UNLOCK(&system_mutex);
		
		if ((NewDay == 1) || cSplitCaptures) 
		{
			dbgprintf(4, "Split module files\n");
			cSplitCaptures = 0;
			for (n = 0; n < iModulesCount; n++)
				if ((pModules[n].Enabled & 1) && 
					(pModules[n].ScanSet != 0) && 
					(pModules[n].SaveChanges != 0) && cCaptStatEnabled &&
					(capt_set[n].FileData->opened == 1))
					SaveCapturedData(1, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
										ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
										uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
		}
	}	
	
	if (keyboard_exist) close_keyboard_scan();	
	
	for (n = 0; n != iModulesCount; n++)
		if ((pModules[n].Enabled & 1) && (pModules[n].ScanSet != 0) && (pModules[n].SaveChanges != 0) && cCaptStatEnabled)
		{
			if (capt_set[n].FileData->opened == 1)
				SaveCapturedData(2, &capt_set[n], cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
									ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModules[n].ID, SaveModuleStatuses, &pModules[n],
									uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
			file_deinit(capt_set[n].FileData);			
		}
	
	DBG_FREE(cCaptPrePath);
	if (cCaptOrderAddr) DBG_FREE(cCaptOrderAddr);
	DBG_FREE(cCapturePath);
	DBG_FREE(cBackUpPath);
	DBG_FREE(uiScanTimer);
	DBG_FREE(uiScanFirst);
	DBG_FREE(capt_set);
	DBG_FREE(pModules);
	DBG_FREE(cOutBuffer);
	for (n = 0; n < iModulesCount; n++)
	{
		if (pModules[n].ParamsCount) DBG_FREE(pModules[n].ParamList);
		if (pModules[n].SettingsCount) DBG_FREE(pModules[n].SettingList);
	}
	
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadScanerStatus = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_OUT();	
	return (void*)1;
}

void * thread_Finger(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	misc_buffer * mBuff = (misc_buffer*)pData;
	MODULE_INFO * pModule = (MODULE_INFO *)mBuff->void_data;
	char *cCapturePath = mBuff->data;
	char *cBackUpPath = mBuff->data2;
	unsigned int uiCaptureSizeLimit = mBuff->uidata[0];
	unsigned int uiCaptureTimeLimit = mBuff->uidata[1];
	unsigned int ucCaptOrderLmt = mBuff->uidata[2];
	unsigned int ucBackUpCaptured = mBuff->uidata[3];
	char cCaptStatEnabled = mBuff->uidata[4];
	unsigned int uiMaxWaitCopyTime = mBuff->uidata[5];
	unsigned int uiMessWaitCopyTime = mBuff->uidata[6];

	char*	cCaptOrderAddr = mBuff->void_data2;
	char*	cCaptPrePath = mBuff->void_data3;	
	DBG_FREE(mBuff);
	
	DBG_MUTEX_LOCK(&system_mutex);
	char cLoop = 1;
	unsigned char cBcTCP = ucBroadCastTCP;
	cThreadFingerStatus++;
	DBG_MUTEX_UNLOCK(&system_mutex);
			
	int i;
		
	time_t rawtime;
	struct tm timeinfo;
	int NowTime;
	int NewDay = 0;
	unsigned char NowDay;
		
	capture_settings *capt_set = (capture_settings*)DBG_MALLOC(sizeof(capture_settings));
	memset(capt_set, 0, sizeof(capture_settings));
	
	unsigned int result, result_evn;
	//iTimeReset = 0;
	if (pModule->Enabled & 1) 
	{
			if (pModule->ScanSet != 0)
			{
				for (i = 0; i != MAX_MODULE_STATUSES; i++) pModule->Status[i] = 0;
				
				if ((pModule->SaveChanges != 0) && cCaptStatEnabled)
				{
					capt_set->FileData = file_init(1048576, 1024);
					capt_set->Stream = AddCaptureStream(CAPTURE_TYPE_MODULE, 0, 0);
					SaveCapturedData(4, capt_set, cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
										ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModule->ID, SaveModuleStatuses, pModule,
										uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
				}
			}			
	}
	
	while(cLoop)
	{
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		NowTime = timeinfo.tm_hour * 10000;
		NowTime += (timeinfo.tm_min) * 100;
		NowTime += timeinfo.tm_sec;	
		NowDay = 1 << timeinfo.tm_wday;
		if (NowDay == 1) NowDay |= 128;
		NowDay = NowDay >> 1;
		if (NowTime == 0) NewDay++; else NewDay = 0;
		
		if ((pModule->Enabled & 1) && (pModule->ScanSet != 0))
		{
			if (pModule->Type == MODULE_TYPE_TFP625A)
			{
				result = TFP625A_AutoIdentify(pModule);
				result_evn = pModule->GenEvents & result;
				if (result_evn)				
				{
					dbgprintf(5, "Scaner: change module status ID:%.4s Statuses:%i\n", (char*)&pModule->ID, result_evn);
					//printf("scan %i, %i\n", result_evn, pModule->Status[2]);
					for (i = 0; i < MAX_MODULE_STATUSES; i++)
						if (result_evn & (1 << i)) 
								AddModuleEventInList(pModule->ID, i+1, pModule->Status[i], NULL, 0, 0);
				}	
				result_evn = pModule->Global & result;
				if (result_evn)
					SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)pModule, sizeof(MODULE_INFO_TRANSFER), (char*)&result_evn, sizeof(result_evn));
					
				if ((pModule->SaveChanges & result) && cCaptStatEnabled)
							SaveCapturedData(0, capt_set, cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
											ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModule->ID, SaveModuleStatuses, pModule,
											uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);		
			}
		}
		DBG_MUTEX_LOCK(&system_mutex);
		cLoop = cThreadFingerRun;
		DBG_MUTEX_UNLOCK(&system_mutex);
		
		if ((NewDay == 1)) 
		{
			dbgprintf(4, "Split finger files\n");
			if ((pModule->Enabled & 1) && 
					(pModule->ScanSet != 0) && 
					(pModule->SaveChanges != 0) && cCaptStatEnabled &&
					(capt_set->FileData->opened == 1))
					SaveCapturedData(1, capt_set, cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
										ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModule->ID, SaveModuleStatuses, pModule,
										uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
		}
	}	
		
	if ((pModule->Enabled & 1) && (pModule->ScanSet != 0) && (pModule->SaveChanges != 0) && cCaptStatEnabled)
	{
		if (capt_set->FileData->opened == 1)
				SaveCapturedData(2, capt_set, cCapturePath, cBackUpPath, uiCaptureSizeLimit, uiCaptureTimeLimit, ucCaptOrderLmt, 
									ucBackUpCaptured, cCaptOrderAddr, cCaptPrePath, DIR_STAT, pModule->ID, SaveModuleStatuses, pModule,
									uiMaxWaitCopyTime, uiMessWaitCopyTime, WRT_TYPE_BACKUP_STATUSES);
		file_deinit(capt_set->FileData);			
	}
	
	DBG_FREE(cCaptPrePath);
	if (cCaptOrderAddr) DBG_FREE(cCaptOrderAddr);
	DBG_FREE(cCapturePath);
	DBG_FREE(cBackUpPath);
	DBG_FREE(capt_set);
	DBG_FREE(pModule);
	
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadFingerStatus--;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_OUT();	
	return (void*)1;
}

void * Shower()
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	//vc_dispman_init();
	//az_bcm_host_init();	
		
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadShowerStatus = 1;
	char cLoop = 1;
	float fMSize = fMenuSize;
	int iVertSync = VSync;
	unsigned int uiSMode = uiShowMode;
	unsigned int uiLocalSysID = 0;
	uiCurCameraID = 0;
	uiCurMicID = 0;
	char cIncConn = 0;
	char cZoomSet = cZoom;
	char cRandomFile = cSettRandomFile;
	char cRandomDir = cSettRandomDir;
	cCurRandomFile = cSettRandomFile;
	cCurRandomDir = cSettRandomDir;
	int iAlrmVol = iAlarmVolume;
	int iBasicVol = iBasicVolume;
	int iCamTimeMax = iCameraTimeMaxSet;
	int iSlideShowTimer = iSlideShowTimerSet;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	unsigned int uiDevType = uiDeviceType;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	char cStatusMessage[64];
	memset(cStatusMessage, 0, 64);
	
	int iShowFileNameClk = 0;
	int iShowFileNameStatus = 0;
	int iShowFileNamePos = 0;
	char cShowFileNameText[128];
	memset(cShowFileNameText, 0, 128);
	
	int iEventMessClk = 0;
	int iEventMessStatus = 0;
	int iEventMessPos = 0;
	char cEventMessText[128];
	memset(cEventMessText, 0, 128);
	
	char cShowDirNameText[128];
	memset(cShowDirNameText, 0, 128);
	memset(cSettShowDirNameText, 0, 128);
	char cDyspMessText[128];
	memset(cDyspMessText, 0, 128);
	
		
	cListFiles = NULL;
	cListDirs = NULL;
	iListDirsCount = 0;
	iListFilesCount = 0;
	iListDirsCurPos = 0;
	iListFilesCurPos = 0;
	cListAlarmFiles = NULL;
	iListAlarmFilesCount = 0;
	func_link *f_link;							
	char *pLinkNextShow;
	
	dbgprintf(3,"Init_GL\n");   
	memset(state, 0, sizeof(GL_STATE_T));
	state->menu_terminal_enabled = 0;
	state->menu_terminal_id = uiTerminalMenuID;
	state->menu_width = uiMenuWidth;
	state->menu_height = uiMenuHeight;
	if (state->menu_width == 0) state->menu_width = state->screen_width;
	if (state->menu_height == 0) state->menu_height = state->screen_height;
	state->menu_skip_timer = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);		
	struct sockaddr_in *m_addr = ModuleIdToAddress(state->menu_terminal_id, 2);
	if (m_addr != NULL) 
	{
		memcpy(&state->menu_terminal_addr, m_addr, sizeof(struct sockaddr_in));
		state->menu_terminal_enabled = 1;
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);		
	
	Init_GL(state);
	
	dbgprintf(3,"Init_Ortho\n");  
	Init_Ortho(state);
	dbgprintf(3,"InitGLText Color: %06x, Accelerator: %s\n", uiTextColor, cAccelerateTextRender ? "Yes" : "No");  	
	if (InitGLText(uiTextColor, 1.0f, cAccelerateTextRender) != 1) dbgprintf(1,"Error text init\n"); else dbgprintf(3,"OK text init\n");
	dbgprintf(3,"Init_Textures\n");  		   
	Init_Textures(state, cZoomSet);   
	
	int ret, ret2, ret3, ret4, n, i;
	
	DBG_MUTEX_LOCK(&modulelist_mutex);					
	for (n = 0; n < iModuleCnt; n++)
	{
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) && (miModuleList[n].Type == MODULE_TYPE_DISPLAY)) 
		{
			miModuleList[n].Settings[0] = state->screen_width;
			miModuleList[n].Settings[1] = state->screen_height;
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	
	dbgprintf(5,"Init Widgets\n");
	ret = 0;
	ret2 = 0;
	DBG_MUTEX_LOCK(&widget_mutex);
	for (n = 0; n < iWidgetsCnt; n++)
	{	
		if (!wiWidgetList[n].Enabled)
		{
			wiWidgetList[n].Status = -1;
			continue;
		}		
		wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
		
		//if ((wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) && (wiWidgetList[n].ShowMode != 0)) glGenTextures(1,&wiWidgetList[n].Texture);
		if ((wiWidgetList[n].Type > WIDGET_TYPE_SENSOR_MIN) &&
				(wiWidgetList[n].Type < WIDGET_TYPE_SENSOR_MAX) && 
				(wiWidgetList[n].ShowMode != 0))
		{
			dbgprintf(3,"Init Sensor widget, Color:%06x\n", Hex2Int(wiWidgetList[n].Color)); 
			if (CreateWidget(&wiWidgetList[n]) != 1) 
			{
				dbgprintf(1,"Error show sensor init\n");
				wiWidgetList[n].Status = -1;				
			}
			else 
			{
				wiWidgetList[n].Status = 1;
				dbgprintf(3,"OK show sensor init\n");										
			}		
		}		
		
		if (wiWidgetList[n].Type == WIDGET_TYPE_WEATHER)
		{
			if (wiWidgetList[n].ShowMode != 0)
			{
				dbgprintf(3,"Load Weather\n"); 
				if (InitGLWeather(state->screen_width, state->screen_height, (GLfloat)(wiWidgetList[n].Settings[0]) / 100) != 1)
				{
					dbgprintf(1,"Error Weather init\n");
					wiWidgetList[n].Status = -1;
				}
				else dbgprintf(4,"OK Weather  init\n");
			}
		}
		if ((wiWidgetList[n].Type > WIDGET_TYPE_CLOCK_MIN) &&
			(wiWidgetList[n].Type < WIDGET_TYPE_CLOCK_MAX))
		{
			dbgprintf(3,"Init Clock Widget, Color:%06x\n", Hex2Int(wiWidgetList[n].Color)); 
			if (CreateWidget(&wiWidgetList[n]) != 1) 
			{
				dbgprintf(1,"Error show clock init\n");
				wiWidgetList[n].Status = -1;
			}
			else 
			{
				wiWidgetList[n].Status = 1;
				dbgprintf(3,"OK show clock  init\n");										
			}						
		}
	}
	dbgprintf(5,"UpdateWidgets\n");
	UpdateWidgets(state, 1);
	DBG_MUTEX_UNLOCK(&widget_mutex);
	
	dbgprintf(4,"UpdateListFiles\n");
	DBG_MUTEX_LOCK(&system_mutex);
	ret = 0;
	if ((UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_FULL) == 0) && (uiSMode >= 2)) 
	{
		uiSMode = 1;
		uiShowModeCur = 1;								
		dbgprintf(3, "No files in Shower '%s'\n", cCurrentFileLocation);
		ret = 1;		
	}
	char cCurrentAlarmPath[MAX_FILE_LEN];
	memset(cCurrentAlarmPath, 0, MAX_FILE_LEN);
	strcpy(cCurrentAlarmPath, cFileAlarmLocation);	
	DBG_MUTEX_UNLOCK(&system_mutex);
	if (ret) ShowNewMessage("Нет файлов для воспроизведения");
	ret = 0;
	
	if (UpdateAlarmFiles(cCurrentAlarmPath) == 0) dbgprintf(2,"Not finded alarm files\n");
	
	int sW = 0;
	int sH = 0;
	GLfloat fSizeW = 0;
	GLfloat fSizeH = 0;
	GLfloat fPosW = 0;
	GLfloat fPosH = 0;
	iTimerToNextShow = 0;
	int iTimer = 0;
	int iTimer2 = 0;
	int iTimerH24 = 0;
	int iTimer05 = 0;
	int iTimer10 = 0;
	char cDateStr[128];	
	char cShowModeStr[64];
	
	unsigned int uiTimerShowPlaySlide = SHOW_PLAYSLIDE_TIME;
	
	DBG_MUTEX_LOCK(&systemlist_mutex);
	iShowerLiveControl = uiShowerLiveCtrlTime;
	DBG_MUTEX_UNLOCK(&systemlist_mutex);
				
	uiTimerShowVolume = SHOW_VOLUME_TIME;
	uiTimerShowTimer = SHOW_TIMER_TIME;
	
	iRefreshClk = 0;
	char CurrentFile[256];
	int	iFade_Direct = 0;
	int iShowStatus = SHOW_ENABLED;
	int iForceShowStatus = SHOW_DEFAULT;
	int iSlideShowRenderStatus = 0;
	int iSysStauses[10];
	memset(iSysStauses, 0, sizeof(int)*10);
	
	DBG_MUTEX_LOCK(&system_mutex);			
	iSysStauses[7] = get_mem_gpu_mb();
	iSysStauses[9] = get_mem_cpu_mb();
	iSysStauses[10] = (int)(get_sys_volt(0) * 1000);
	iSysStauses[11] = (int)(get_sys_volt(1) * 1000);
	iSysStauses[12] = (int)(get_sys_volt(2) * 1000);
	iSysStauses[13] = (int)(get_sys_volt(3) * 1000);
	iAlarmEvents = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	float fChangeVolumeStep = 0;
	float fChangeVolumeCurrent = iCurrentVolume;
	int iChangeVolumeNext = iCurrentVolume;
	int iChangeVolumeSoft = 0;	
	
	iCurrentVolume = iBasicVol;
	
	time_t rawtime;
	struct tm timeinfo;
	int NowTime;
	int iPrevShowMenu = 0;
	int iNewPosSlide = 0;
	char cKeyStatus = 0;
	int iNewShow, iCurShow;
	cKeyMem = 0;
	cKeyUpClock = 0;
	cMenuBackClk = 0;
	cMenuForwClk = 0;
	
	SetCurShowType(SHOW_TYPE_NA);
	SetNewShowType(SHOW_TYPE_NA);
	SetChangeShowNow(1);
	
	SetShowTimeMax(20);	
	int iRadioTimerOff = 0;
	iRadioTimeMax = 0;
	cSeek = 0;
	int iMiscData[10];
	unsigned int uiMiscData[6];
	
	int iCameraListCurCnt = 0;
	int iCameraListNewCnt = 0;
	int iCameraListShowed = 1;
	int iCameraReconnectTimer = 0;
	CAMERA_LIST_INFO * clCameraListCur = (CAMERA_LIST_INFO*)DBG_MALLOC(sizeof(CAMERA_LIST_INFO) * MAX_CAMERA_LIST_CNT);
	CAMERA_LIST_INFO * clCameraListNew = (CAMERA_LIST_INFO*)DBG_MALLOC(sizeof(CAMERA_LIST_INFO) * MAX_CAMERA_LIST_CNT);
	CameraListClear(state, clCameraListCur, &iCameraListCurCnt, 0);
	CameraListClear(state, clCameraListNew, &iCameraListNewCnt, 0);
	
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	
	int64_t previous_ms;
	unsigned int diff_ms;
	struct sockaddr_in *pAddr;
	struct sockaddr_in sFileAddr;						
	MODULE_INFO* pModule;

	int iCameraMenuStatus = 0;
	int iRemStatus = 0;
	DBG_MUTEX_LOCK(&system_mutex);	
	rsiRemoteStatus.Status = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
		
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);	
	dbgprintf(4,"GO GO GO in %i:%i:%i\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); 	
	
	//DBG_ON(DBG_MODE_LOG_FUNC | DBG_MODE_MEMORY_ERROR | DBG_MODE_TRACK_LOCKED_MUTEX | DBG_MODE_TRACK_FUNC_THREAD); // | DBG_MODE_TRACK_LOCKED_MUTEX);
	
	//char *pSurface = DBG_MALLOC(state->screen_width * state->screen_height * 4);
	//omx_jpegfile_to_surface("/mnt/FLASH/Shows/misc/WallPaper1/64118-1920x1080.jpg", state->screen_width, state->screen_height, pSurface);
	//printf("%ix%i\n", state->screen_width, state->screen_height);
	//return 0;
	previous_ms = 0;
	get_ms(&previous_ms);
		
	while (cLoop)
	{		
			previous_ms = 0;
			get_ms(&previous_ms);
			
			state->menu_render_count = 0;
			state->menu_skip_timer++;
			if (state->menu_skip_timer > TERMINAL_MENU_SKIP_FRAMES) state->menu_skip_timer = 0;
			
			if (iShowFileNameStatus != 0)
			{
				if (iShowFileNameStatus == 1)
				{
					iShowFileNamePos = -50*fMSize;
					iShowFileNameStatus++;
					iShowFileNameClk = 0;					
				}
				if (iShowFileNameStatus == 2)
				{
					iShowFileNamePos++;
					if (iShowFileNamePos >= (50*fMSize)) 
					{
						iShowFileNameStatus++;
						iShowFileNameClk = 0;
					}
				}
				if ((iShowFileNameStatus == 3) && (iShowFileNameClk == 150))
				{
					iShowFileNameStatus++;
					iShowFileNameClk = 0;
				}
				if (iShowFileNameStatus == 4)
				{
					if (iShowFileNamePos > (-50*fMSize)) iShowFileNamePos--; else iShowFileNameStatus = 0;
				}
				iShowFileNameClk++;
			}	
			
			if (iEventMessStatus != 0)
			{
				if (iEventMessStatus == 1)
				{
					iEventMessPos = -50*fMSize;
					iEventMessStatus++;
					iEventMessClk = 0;					
				}
				if (iEventMessStatus == 2)
				{
					iEventMessPos += 7;
					if (iEventMessPos >= (20*fMSize)) 
					{
						iEventMessStatus++;
						iEventMessClk = 0;
					}
				}
				if ((iEventMessStatus == 3) && (iEventMessClk == 50))
				{
					iEventMessStatus++;
					iEventMessClk = 0;
				}
				if (iEventMessStatus == 4)
				{
					if (iEventMessPos > (-50*fMSize)) iEventMessPos -= 7; else iEventMessStatus = 0;
				}
				iEventMessClk++;
			}		
			
			/////////////iChangeDirTime//////////////
			iTimer2++;
			//iChangeDirTime
			///////////// 1 sec//////////////		  	
			iTimer++;
			if (iTimer == 25) 
			{	
				DBG_MUTEX_LOCK(&widget_mutex);
				UpdateWidgets(state, 0);
				DBG_MUTEX_UNLOCK(&widget_mutex);
			
				iTimer = 0;	
				iTimer10++;				
				
				if (iAccessLevelCopy > 0)
				{
					DBG_MUTEX_LOCK(&system_mutex);			
					iSysStauses[0] = (int)(get_sys_temp() * 10);
					iSysStauses[2] = total_cpu_load_value() * 10;
					iSysStauses[3] = my_cpu_load_value() * 10;
					get_memory_ram((unsigned int*)&iSysStauses[4], (unsigned int*)&iSysStauses[5], (unsigned int*)&iSysStauses[6]);
					iSysStauses[10] = (int)(get_sys_volt(0) * 1000);
					iSysStauses[11] = (int)(get_sys_volt(1) * 1000);
					iSysStauses[12] = (int)(get_sys_volt(2) * 1000);
					iSysStauses[13] = (int)(get_sys_volt(3) * 1000);
					DBG_MUTEX_UNLOCK(&system_mutex);
					cIncConn = TestIncomeConnects();
				}
				iTimerH24++;			
								
				DBG_MUTEX_LOCK(&systemlist_mutex);
				iShowerLiveControl = uiShowerLiveCtrlTime;
				DBG_MUTEX_UNLOCK(&systemlist_mutex);
				//DBG_MUTEX_PRINT_LOCKED();
				//DBG_LOG_MEM();
				if (iChangeVolumeSoft)
				{
					if (fChangeVolumeStep == 0) 
					{
						iCurrentVolume = iChangeVolumeNext;
						MusicVolumeSet(iCurrentVolume);
						iChangeVolumeSoft = 0;
					}
					else
					{
						if (iChangeVolumeNext > iCurrentVolume)
						{			
							fChangeVolumeCurrent += fChangeVolumeStep;
							if (iCurrentVolume != (int)fChangeVolumeCurrent) MusicVolumeSet(iCurrentVolume);
							iCurrentVolume = (int)fChangeVolumeCurrent;
							if (iCurrentVolume > iChangeVolumeNext) iCurrentVolume = iChangeVolumeNext;							
						}
						if (iChangeVolumeNext < iCurrentVolume)
						{
							fChangeVolumeCurrent -= fChangeVolumeStep;
							if (iCurrentVolume != (int)fChangeVolumeCurrent) MusicVolumeSet(iCurrentVolume);
							iCurrentVolume = (int)fChangeVolumeCurrent;
							if (iCurrentVolume < iChangeVolumeNext) iCurrentVolume = iChangeVolumeNext;							
						}
						if (iChangeVolumeNext == iCurrentVolume) iChangeVolumeSoft = 0;
					}
				}
				if (uiTimerShowVolume < SHOW_VOLUME_TIME) uiTimerShowVolume++;
				if (uiTimerShowTimer <= SHOW_TIMER_TIME) 
				{
					if (uiTimerShowTimer < SHOW_TIMER_TIME) uiTimerShowTimer++;
					else
					{						
						DBG_MUTEX_LOCK(&system_mutex);								
						if ((iSlideShowTimer != 0) && (iSlideShowTimer != iSlideShowTimerSet)) 
						{
							iSlideShowTimerSet = iSlideShowTimer; 
							if ((cCurShowType & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE | SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM)) == 0)
								iShowTimeMax = iSlideShowTimer;
						}
						if ((iCamTimeMax != 0) && (iCamTimeMax != iCameraTimeMaxSet)) 
						{
							iCameraTimeMaxSet = iCamTimeMax; 
							if (cCurShowType & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE)) iShowTimeMax = iCamTimeMax;
						}
						if ((iRadioTimeMax != 0) && (iRadioTimeMax != iStreamTimeMaxSet)) iStreamTimeMaxSet = iRadioTimeMax;
						DBG_MUTEX_UNLOCK(&system_mutex);
					}
				}
				if (uiTimerShowPlaySlide < SHOW_PLAYSLIDE_TIME) uiTimerShowPlaySlide++;
				
				iCurShow = GetCurShowType();
				ret3 = GetChangeShowNow();
				//printf("### %i %i %i\n", GetChangeShowNow(), iTimerToNextShow, GetShowTimeMax());
				if ((iCurShow & (SHOW_TYPE_FILE | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE)) == 0) iTimerToNextShow++; 
				
				if ((((iCurShow & SHOW_TYPE_CAMERA_FILE) && omx_IsFree_Video()) || 
					((iCurShow & SHOW_TYPE_MIC_FILE) && Audio_IsFree()))
					&& (ret3 == 0))
				{
					pScreenMenu[0].Options[1].Show = 0; //File menu					
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NA);	
					DelCurShowType(SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_FILE);
					DBG_MUTEX_LOCK(&systemlist_mutex);
					uiLocalSysID = GetLocalSysID();
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					
					omx_stop_video(0);
					Audio_Stop(0);					
					CloseServerFile(5000);
					omx_stop_video(1);
					Audio_Stop(1);
					
					if (iCurShow & SHOW_TYPE_CAMERA_FILE)
						AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);
					if (iCurShow & SHOW_TYPE_MIC_STREAM)
						AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);					
				}
				if ((iCurShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_LIST)) && omx_IsFree_Video() && (ret3 == 0))
				{
					pScreenMenu[0].Options[1].Show = 0; //Camera menu					
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NA);	
					DelCurShowType(SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_LIST);
					DBG_MUTEX_LOCK(&systemlist_mutex);
					uiLocalSysID = GetLocalSysID();
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);
				}
				if ((iCurShow & SHOW_TYPE_MIC_STREAM) && Audio_IsFree() && (ret3 == 0))
				{
					pScreenMenu[0].Options[6].Show = 0; //Audio menu					
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NA);	
					DelCurShowType(SHOW_TYPE_MIC_STREAM);
					DBG_MUTEX_LOCK(&systemlist_mutex);
					uiLocalSysID = GetLocalSysID();
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);
				}
				if ((iCurShow & SHOW_TYPE_FILE) && MediaPlay_IsFree() && (ret3 == 0))					
				{
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NA);	
					DBG_MUTEX_LOCK(&systemlist_mutex);
					uiLocalSysID = GetLocalSysID();
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					if (iCurShow & SHOW_TYPE_VIDEO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);
					if (iCurShow & SHOW_TYPE_AUDIO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);
					DelCurShowType(SHOW_TYPE_AUDIO | SHOW_TYPE_VIDEO | SHOW_TYPE_FILE | SHOW_TYPE_WEB_STREAM);	
					pScreenMenu[0].Options[3].Show = 0; //Media menu									
				}
				if ((iCurShow & SHOW_TYPE_WEB_STREAM) && MediaPlay_IsFree() && (ret3 == 0))
				{
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NEW);
					DBG_MUTEX_LOCK(&systemlist_mutex);
					uiLocalSysID = GetLocalSysID();
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					if (iCurShow & SHOW_TYPE_VIDEO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);
					if (iCurShow & SHOW_TYPE_AUDIO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);					
					DelCurShowType(SHOW_TYPE_AUDIO | SHOW_TYPE_VIDEO | SHOW_TYPE_WEB_STREAM);	
					iRadioTimeMax = 0;
					iRadioTimerOff = 0;
					pScreenMenu[0].Options[20].Show = 0; //Stream menu	
				}
				if ((iRadioTimeMax != 0) && (ret3 == 0)) 
				{
					iRadioTimerOff++;
					if (iRadioTimerOff >= iRadioTimeMax) 
					{
						iRadioTimeMax = 0;
						iRadioTimerOff = 0;
						if (iCurShow & SHOW_TYPE_WEB_STREAM)	
							add_sys_cmd_in_list(SYSTEM_CMD_STREAM_OFF, 0);
						else
							add_sys_cmd_in_list(SYSTEM_CMD_RADIO_OFF, 0);
					}
				}
			}	// 1 sec
			
			///////////// 10 sec//////////////
			if (iTimer10 >= 10)
			{
				iTimer10 = 0;
				DBG_MUTEX_LOCK(&modulelist_mutex);		
				m_addr = ModuleIdToAddress(state->menu_terminal_id, 2);
				if (m_addr != NULL) 
				{
					memcpy(&state->menu_terminal_addr, m_addr, sizeof(struct sockaddr_in));
					if (state->menu_terminal_enabled < 1) state->menu_terminal_enabled++;
				} else state->menu_terminal_enabled = 0;
				DBG_MUTEX_UNLOCK(&modulelist_mutex);				
			}
			///////////// 0.5 sec//////////////		  	
			iTimer05++;
			if (iTimer05 == 13) 
			{				
				iTimer05 = 0;
				DBG_MUTEX_LOCK(&system_mutex);
				iCameraMenuStatus = rsiRemoteStatus.Status;
				fMSize = fMenuSize;
				iVertSync = VSync;
				uiSMode = uiShowModeCur;
				cZoomSet = cZoom;
				cRandomDir = cCurRandomDir;
				cRandomFile = cCurRandomFile;
				iAlrmVol = iAlarmVolume;
				iBasicVol = iBasicVolume;
				if (iShowFileNameStatus) memcpy(cShowDirNameText, cSettShowDirNameText, 128);
				
				//iCamTimeMax = iCameraTimeMaxSet;  
				state->menu_terminal_id = uiTerminalMenuID;
				state->menu_width = uiMenuWidth;
				state->menu_height = uiMenuHeight;
				if (state->menu_width == 0) state->menu_width = state->screen_width;
				if (state->menu_height == 0) state->menu_height = state->screen_height;					
				
				if (uiSMode > 0)
				{
					if ((iAlarmEvents & ALARM_TYPE_CLOCK) == 0)
					{						
						time(&rawtime);
						localtime_r(&rawtime, &timeinfo);
						NowTime = timeinfo.tm_hour * 10000;
						NowTime += (timeinfo.tm_min) * 100;
						NowTime += timeinfo.tm_sec;						
								
						for (ret = 0; ret < iAlarmClocksCnt; ret++)
						{
							if (actAlarmClockInfo[ret].TimerAlarmPause > 4)
							{	
								if ((actAlarmClockInfo[ret].Enabled == 1) && (actAlarmClockInfo[ret].Time == NowTime))
								{
									ret2 = 1 << timeinfo.tm_wday;
									if (ret2 == 1) ret2 |= 128;
									ret2 = ret2 >> 1;
									if (ret2 & actAlarmClockInfo[ret].Days)
									{									
										if (actAlarmClockInfo[ret].Skip == 0)
										{
											if (strlen(actAlarmClockInfo[ret].Path) == 0)
											{
												if (strcmp(cCurrentAlarmPath, cFileAlarmLocation) != 0)
												{
													memset(cCurrentAlarmPath, 0, MAX_FILE_LEN);
													strcpy(cCurrentAlarmPath, cFileAlarmLocation);
													dbgprintf(5, "Change alarm path to default:'%s'\n",cCurrentAlarmPath);
													if (UpdateAlarmFiles(cCurrentAlarmPath) == 0) dbgprintf(2,"Not finded alarm files\n");												
												}
											}
											else
											{
												if (strcmp(cCurrentAlarmPath, actAlarmClockInfo[ret].Path) != 0)
												{
													memset(cCurrentAlarmPath, 0, MAX_FILE_LEN);
													strcpy(cCurrentAlarmPath, actAlarmClockInfo[ret].Path);
													dbgprintf(5, "Change alarm path to spec:'%s'\n",cCurrentAlarmPath);												
													if (UpdateAlarmFilesNoSub(cCurrentAlarmPath) == 0) dbgprintf(2,"Not finded alarm files\n");
												}
											}
																					
											if (actAlarmClockInfo[ret].TimeSetVolume) 
											{
												iChangeVolumeSoft = 1;
												iChangeVolumeNext = iAlrmVol;
												fChangeVolumeCurrent = actAlarmClockInfo[ret].BeginVolume;
												iCurrentVolume = fChangeVolumeCurrent;
												if (actAlarmClockInfo[ret].BeginVolume < iAlrmVol) 
													fChangeVolumeStep = iAlrmVol - actAlarmClockInfo[ret].BeginVolume;
													else
													fChangeVolumeStep = actAlarmClockInfo[ret].BeginVolume - iAlrmVol;
												fChangeVolumeStep = fChangeVolumeStep / actAlarmClockInfo[ret].TimeSetVolume;
											} else iCurrentVolume = iAlrmVol;
											//printf("soft:%i step:%g dest:%i cur:%i\n", iChangeVolumeSoft, fChangeVolumeStep, iChangeVolumeNext, iCurrentVolume);
											if (actAlarmClockInfo[ret].Limit != 0)
											{
												actAlarmClockInfo[ret].Limit--;
												if (actAlarmClockInfo[ret].Limit == 0) actAlarmClockInfo[ret].Enabled = 0;
												DBG_MUTEX_UNLOCK(&system_mutex);
												SaveAlarms();
												DBG_MUTEX_LOCK(&system_mutex);
											}
											cNewShowType = SHOW_TYPE_ALARM1;
											cChangeShowNow = 1;						
											iAlarmEvents |= ALARM_TYPE_CLOCK;
											dbgprintf(4,"ALARM_TYPE_CLOCK: num=%i enbl=%i time=%i\n", ret, actAlarmClockInfo[ret].Enabled, actAlarmClockInfo[ret].Time);
										} 
										else 
										{
											actAlarmClockInfo[ret].Skip--;
											DBG_MUTEX_UNLOCK(&system_mutex);
											SaveAlarms();
											DBG_MUTEX_LOCK(&system_mutex);
										}
										actAlarmClockInfo[ret].TimerAlarmPause = 0;
										if (iAlarmEvents & ALARM_TYPE_CLOCK) 
										{
											for (n = 0; n < iAlarmClocksCnt; n++) actAlarmClockInfo[n].TimerAlarmPause = 0;
											break;
										}
									}
								}	
							} else if (actAlarmClockInfo[ret].TimerAlarmPause < 30) actAlarmClockInfo[ret].TimerAlarmPause++;
						}
					}				
				}				
				DBG_MUTEX_UNLOCK(&system_mutex);
				if (iTimer < 15) 
				{
					GetDateTimeStr(cDateStr, 128);
					GetShowModeStr(uiSMode, cShowModeStr, 64);
				}
			}	// 0.5 sec
			//if ((cNewShowType & SHOW_TYPE_CAMERA) && (iTimerToNextShow >= iShowTimeMax)) ResetShower(-1);
			
			//////////// Next Photo//////////////
			//printf("%i %i %i\n", GetChangeShowNow(), iTimerToNextShow, GetShowTimeMax());
			if ((GetChangeShowNow() == 1) || (iTimerToNextShow >= GetShowTimeMax())) 
			{	//(omx_IsFree() == 1) && (audio_IsFree() > 0) && 
				dbgprintf(5,"Change show %i, %i, %i\n", GetChangeShowNow(), iTimerToNextShow, GetShowTimeMax());
				
				DBG_MUTEX_LOCK(&modulelist_mutex);
				memset(cCurrentPlayType, 0, 32);
				memset(cCurrentPlayName, 0, 256);
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
							
				if (GetNewShowType() == SHOW_TYPE_ALARM1)
				{
					if ((iListAlarmFilesCount == 0) && strlen(cCurrentAlarmPath) && ((cCurrentAlarmPath[strlen(cCurrentAlarmPath)-1] == 47) || (Its_Directory(cCurrentAlarmPath) == 1)))
						UpdateAlarmFiles(cCurrentAlarmPath);
					if ((iListAlarmFilesCount == 0) && strlen(cCurrentAlarmPath) && ((cCurrentAlarmPath[strlen(cCurrentAlarmPath)-1] == 47) || (Its_Directory(cCurrentAlarmPath) == 1)))
					{
						iCurrentVolume = iAlrmVol;
						audio_set_playback_volume(iCurrentVolume);			
						PlayAlarmSound();
						SetChangeShowNow(0);
						ClockAlarmStop(0);
						if (iChangeVolumeSoft) iChangeVolumeSoft = 0;
						SetNewShowType(SHOW_TYPE_NA);
					}				
				}				
				/*DBG_MUTEX_LOCK(&system_mutex);				
				if ((iAlarmEvents & ALARM_TYPE_CLOCK) && (!(cNewShowType & SHOW_TYPE_ALARM1)))
				{
					iAlarmEvents ^= ALARM_TYPE_CLOCK;
					DBG_MUTEX_UNLOCK(&system_mutex);
					ClockAlarmStop(0);					
					dbgprintf(5,"ClockAlarmStop\n");
				} else DBG_MUTEX_UNLOCK(&system_mutex);*/
								
				pScreenMenu[0].Options[6].Show = 0;
				pScreenMenu[0].Options[2].Show = 0;
				pScreenMenu[0].Options[3].Show = 0;
				//if ((pScreenMenu[0].SelectedOption == 2) ||
					//(pScreenMenu[0].SelectedOption == 3) ||
					//(pScreenMenu[0].SelectedOption == 6)) pScreenMenu[0].SelectedOption = 0;
				
				if (GetChangeShowNow() == 0) 
				{
					SetNewShowType(SHOW_TYPE_NA);
					SetChangeShowNow(2);
				} else SetChangeShowNow(3);
						
				//if ((iShowStatus == 1) || (cNewShowType != 0)) 
				iFade_Direct = 1;
				
				time(&rawtime);
				localtime_r(&rawtime, &timeinfo);
				NowTime = timeinfo.tm_hour * 10000;
				NowTime += (timeinfo.tm_min)*100;
				NowTime += timeinfo.tm_sec;	
				
				DBG_MUTEX_LOCK(&system_mutex);	
				if (iShowStatus == SHOW_DISABLED)
				{
					if (((iSlideShowOffTime < iSlideShowOnTime) && ((NowTime > iSlideShowOnTime) || (NowTime < iSlideShowOffTime))) 
						|| ((iSlideShowOffTime > iSlideShowOnTime) && (NowTime > iSlideShowOnTime) && (NowTime < iSlideShowOffTime))) 
						{
							iShowStatus = SHOW_ENABLED;
							iForceShowStatus = SHOW_DEFAULT;
						}
				}
				if ((iShowStatus == SHOW_ENABLED) && (iForceShowStatus != SHOW_DISABLED))
				{
					if (((iSlideShowOffTime < iSlideShowOnTime) && (NowTime > iSlideShowOffTime) && (NowTime < iSlideShowOnTime))
						|| ((iSlideShowOffTime > iSlideShowOnTime) && ((NowTime > iSlideShowOffTime) || (NowTime < iSlideShowOnTime))))
						{
							iShowStatus = SHOW_DISABLED;
							iForceShowStatus = SHOW_DEFAULT;							
						}
				}
				DBG_MUTEX_UNLOCK(&system_mutex);

				iTimerToNextShow = 0;								
				//printf("Next Photo %i,%i,%i\n",iFade_Direct,iShowStatus,cNewShowType);
			}	// Next Photo
			
			if (iFade_Direct == 1) 
			{// && ((omx_IsFree() == 1) || (cNewShowType & SHOW_TYPE_CAMERA))
				//if ((GetShowType() & (SHOW_TYPE_IMAGE | SHOW_TYPE_FILE)) == SHOW_TYPE_IMAGE) iFade += 3; else iFade+=20; 
				DBG_MUTEX_LOCK(&system_mutex);
				uiSMode = uiShowModeCur;
				DBG_MUTEX_UNLOCK(&system_mutex);
					
				if (uiSMode >= 2)
				{
					if (GetChangeShowNow() == 2) iFade += 3; else iFade+=20;
				} else iFade = 255;
				if (iFade >= 255)
				{
					if (iTimer2 >= iChangeDirTime) 
					{
						iTimer2 = 0; 
						n = 0;
						DBG_MUTEX_LOCK(&system_mutex);							
						if (uiSMode >= 2)
						{
							if (UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_NEXT) == 0)
							{
								uiSMode = 1;
								uiShowModeCur = 1;
								dbgprintf(3, "No files in Shower2 '%s'\n", cCurrentFileLocation);
								n = 1;								
							}							
						}
						DBG_MUTEX_UNLOCK(&system_mutex);
						if (n) ShowNewMessage("Нет файлов для воспроизведения");
					}
					
					if (iTimerH24 >= 86400) 
					{
						iTimerH24 = 0; 
						dbgprintf(4,"UpdateAlarmFiles\n");
						DBG_MUTEX_LOCK(&system_mutex);							
						if (uiSMode >= 1) 
						{
							memset(cCurrentAlarmPath, 0, MAX_FILE_LEN);
							strcpy(cCurrentAlarmPath, cFileAlarmLocation);												
							if (UpdateAlarmFiles(cCurrentAlarmPath) == 0) dbgprintf(2,"Not finded alarm files\n");
						}
						DBG_MUTEX_UNLOCK(&system_mutex);													
					}		
					iShowFileNameStatus = 0;
					iFade = 255;					
					iFade_Direct = 2;
					iCurShow = GetCurShowType();
					iNewShow = GetNewShowType();
					//printf("!!!!!!4 %i %i\n", iCurShow, iNewShow);
					/*if ((GetShowType() & SHOW_TYPE_AUDIO_STREAM)  (GetShowType() & SHOW_TYPE_BACK_STREAM))
					{
						SetShowType(GetShowType() & 255);
						iRadioTimeMax = 0;
					}*/
					if ((iNewShow & SHOW_TYPE_NEW) ||
						(iNewShow & SHOW_TYPE_FILE) ||
						((iNewShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_IMAGE | SHOW_TYPE_CLOSE_VIDEO)) && (iCurShow & SHOW_TYPE_AUDIO) && (iCurShow & SHOW_TYPE_VIDEO)) ||
						((iNewShow & (SHOW_TYPE_MIC_STREAM | SHOW_TYPE_CLOSE_AUDIO)) && (iCurShow & SHOW_TYPE_AUDIO) && (iCurShow & SHOW_TYPE_VIDEO)) ||
						(iCurShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE)) ||
						(iNewShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE)))
					{
						dbgprintf(5,"drop all %i %i\n", iNewShow, iCurShow);
						//printf("drop all %i %i\n", iNewShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE), iCurShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE));
						pScreenMenu[0].Options[1].Show = 0; //Camera menu
						pScreenMenu[0].Options[2].Show = 0; //Foto menu
						pScreenMenu[0].Options[3].Show = 0; //Media menu
						pScreenMenu[0].Options[6].Show = 0; //Audio menu
						pScreenMenu[0].Options[20].Show = 0; //Stream menu
						Media_StopPlay(1);
						omx_stop_video(0);
						Audio_Stop(0);	
						CloseServerFile(5000);						
						omx_stop_video(1);
						Audio_Stop(1);	
						CloseAllConnects(CONNECT_CLIENT, FLAG_VIDEO_STREAM | FLAG_AUDIO_STREAM, 0);
						iSlideShowRenderStatus = 0;
						CameraListClear(state, clCameraListCur, &iCameraListCurCnt, 1);
						uiCurCameraID = 0;
						uiCurMicID = 0;
						if (iCurShow & SHOW_TYPE_WEB_STREAM) 
						{
							iRadioTimeMax = 0;		
							iRadioTimerOff = 0;
						}						
						SetCurShowType(SHOW_TYPE_NA);

						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
						if (n >= 0) 
						{
							miModuleList[n].Status[0] = 0;
							miModuleList[n].Status[1] = 0;
						}
						n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
						if (n >= 0) miModuleList[n].Status[0] = 0;
						DBG_MUTEX_UNLOCK(&modulelist_mutex);						
						
						DBG_MUTEX_LOCK(&systemlist_mutex);
						uiLocalSysID = GetLocalSysID();
						DBG_MUTEX_UNLOCK(&systemlist_mutex);
						if (iCurShow & SHOW_TYPE_VIDEO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);		
						if (iCurShow & SHOW_TYPE_AUDIO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);	
						
						//printf("droped all %i %i\n", iNewShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE), iCurShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE));
						//printf("!!!!!!4 %i %i\n", iCurShow, iNewShow);
					}
					if ((iNewShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CLOSE_VIDEO | SHOW_TYPE_CAMERA_LIST)) 
								&& (!(iCurShow & SHOW_TYPE_AUDIO)) && (iCurShow & SHOW_TYPE_VIDEO))
					{
						dbgprintf(5,"drop Media and video\n");
						pScreenMenu[0].Options[1].Show = 0; //Camera menu
						pScreenMenu[0].Options[2].Show = 0; //Foto menu
						pScreenMenu[0].Options[3].Show = 0; //Media menu
						pScreenMenu[0].Options[20].Show = 0; //Stream menu
						Media_StopPlay(1);
						omx_stop_video(1);
						iSlideShowRenderStatus = 0;	
						CameraListClear(state, clCameraListCur, &iCameraListCurCnt, 1);
						uiCurCameraID = 0;
						if (iCurShow & SHOW_TYPE_WEB_STREAM) 
						{
							iRadioTimeMax = 0;			
							iRadioTimerOff = 0;		
						}
						DelCurShowType(SHOW_TYPE_VIDEO | SHOW_TYPE_FILE | SHOW_TYPE_IMAGE | SHOW_TYPE_WEB_STREAM | SHOW_TYPE_CLOSE_VIDEO);	
						
						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
						if (n >= 0) 
						{
							miModuleList[n].Status[0] = 0;
							miModuleList[n].Status[1] = 0;
						}
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						
						DBG_MUTEX_LOCK(&systemlist_mutex);
						uiLocalSysID = GetLocalSysID();
						DBG_MUTEX_UNLOCK(&systemlist_mutex);
						AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);
					}
					if ((iNewShow & (SHOW_TYPE_MIC_STREAM | SHOW_TYPE_CLOSE_AUDIO)) 
							&& (iCurShow & SHOW_TYPE_AUDIO) && (!(iCurShow & SHOW_TYPE_VIDEO)))
					{
						dbgprintf(5,"drop Media and audio\n");
						pScreenMenu[0].Options[2].Show = 0; //Foto menu
						pScreenMenu[0].Options[3].Show = 0; //Media menu
						pScreenMenu[0].Options[6].Show = 0; //Audio menu
						pScreenMenu[0].Options[20].Show = 0; //Stream menu
						Media_StopPlay(1);
						Audio_Stop(1);
						uiCurMicID = 0;
						if (iCurShow & SHOW_TYPE_WEB_STREAM) 
						{
							iRadioTimeMax = 0;		
							iRadioTimerOff = 0;
						}
						DelCurShowType(SHOW_TYPE_AUDIO | SHOW_TYPE_FILE | SHOW_TYPE_WEB_STREAM | SHOW_TYPE_CLOSE_AUDIO);
						
						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
						if (n >= 0) miModuleList[n].Status[0] = 0;
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						
						DBG_MUTEX_LOCK(&systemlist_mutex);
						uiLocalSysID = GetLocalSysID();
						DBG_MUTEX_UNLOCK(&systemlist_mutex);
						AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);
					}
					if (((iNewShow == SHOW_TYPE_NA) || (iNewShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_LIST))) 
							&& (iCurShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_LIST)))
					{
						dbgprintf(5, "drop video and connects\n");
						pScreenMenu[0].Options[1].Show = 0; //Camera menu
						omx_stop_video(1);
						CloseAllConnects(CONNECT_CLIENT, FLAG_VIDEO_STREAM, 0);
						iSlideShowRenderStatus = 0;		
						uiCurCameraID = 0;
						DelCurShowType(SHOW_TYPE_CAMERA | SHOW_TYPE_IMAGE | SHOW_TYPE_CAMERA_LIST);
						
						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
						if (n >= 0)
						{
							miModuleList[n].Status[0] = 0;
							miModuleList[n].Status[1] = 0;
						}						
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
								
						DBG_MUTEX_LOCK(&systemlist_mutex);
						uiLocalSysID = GetLocalSysID();
						DBG_MUTEX_UNLOCK(&systemlist_mutex);
						AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);
					}
					if (((iNewShow == SHOW_TYPE_NA) || (iNewShow & SHOW_TYPE_MIC_STREAM)) 
							&& (iCurShow & SHOW_TYPE_MIC_STREAM))
					{
						dbgprintf(5, "drop audio and connects\n");
						pScreenMenu[0].Options[6].Show = 0; //Audio menu
						Audio_Stop(1);
						CloseAllConnects(CONNECT_CLIENT, FLAG_AUDIO_STREAM, 0);
						uiCurMicID = 0;												
						DelCurShowType(SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE);
						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
						if (n >= 0) miModuleList[n].Status[0] = 0;
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						DBG_MUTEX_LOCK(&systemlist_mutex);
						uiLocalSysID = GetLocalSysID();
						DBG_MUTEX_UNLOCK(&systemlist_mutex);
						AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);
					}
					if (iCurShow & SHOW_TYPE_IMAGE) DelCurShowType(SHOW_TYPE_IMAGE);
						
					if (iNewShow & (SHOW_TYPE_CLOSE_VIDEO | SHOW_TYPE_CLOSE_AUDIO))	
							DelCurShowType(SHOW_TYPE_CLOSE_VIDEO | SHOW_TYPE_CLOSE_AUDIO);				
					//if (iNewShow & SHOW_TYPE_IMAGE) DelCurShowType(SHOW_TYPE_IMAGE);
					
					
					uiTimerShowPlaySlide = SHOW_PLAYSLIDE_TIME;
					
					DBG_MUTEX_LOCK(&system_mutex);	
					if (((iAlarmEvents & ALARM_TYPE_CLOCK) == 0) && (iCurrentVolume != iBasicVol)) 
					{
						iCurrentVolume = iBasicVol;
						audio_set_playback_volume(iCurrentVolume);
					}
					if ((cNewShowType & SHOW_TYPE_ALARM1) && (iChangeVolumeSoft == 0)) 
					{
						iCurrentVolume = iAlrmVol;
						audio_set_playback_volume(iCurrentVolume);
					}
					if ((!(cNewShowType & SHOW_TYPE_ALARM1)) && ((cCurShowType & SHOW_TYPE_AUDIO) == 0) && (iAlarmEvents & ALARM_TYPE_CLOCK))
					{
						iAlarmEvents ^= ALARM_TYPE_CLOCK;
						DBG_MUTEX_UNLOCK(&system_mutex);
						ClockAlarmStop(1);
						dbgprintf(5,"ClockAlarmStop done\n");
					} else DBG_MUTEX_UNLOCK(&system_mutex);
						
					/*DBG_MUTEX_LOCK(&widget_mutex);
					UpdateWidgets(state, 0);	
					DBG_MUTEX_UNLOCK(&widget_mutex);*/
				}
			}
			
			////////////ACCEPT NEXT SOURCE////////////	
			//printf("Fade direct %i iNewShow %i uiSMode %i ChangeNow() %i Status %i ForceStatus %i\n", iFade_Direct, iNewShow, uiSMode, GetChangeShowNow(), iShowStatus, iForceShowStatus);
			if (iFade_Direct == 2)				
			{
				//printf(">>> %i %i %i\n",iShowStatus == SHOW_ENABLED,uiSMode >= 2,
					//iNewShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_ALARM1 | SHOW_TYPE_FILE | SHOW_TYPE_URL));
				iNewShow = GetNewShowType();				
				if ((
					  (
						((iShowStatus == SHOW_ENABLED) && (iForceShowStatus != SHOW_DISABLED)) 
						|| 
						(iForceShowStatus == SHOW_ENABLED)
					  ) 
					  && 
					  (uiSMode >= 2)
					) 
					|| 
					(iNewShow & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE | SHOW_TYPE_ALARM1 | SHOW_TYPE_FILE | SHOW_TYPE_URL)))
				{
					//printf("Accept new show\n");
					iCurShow = GetCurShowType();				
					if (iNewShow & SHOW_TYPE_MIC_STREAM)
					{			
						SetShowTimeMax(iCamTimeMax);
						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						if (n >= 0)
						{
							ret = 0;
							DBG_MUTEX_LOCK(&system_mutex);
							if (uiCurMicID != uiNextMicID) uiCurMicID = uiNextMicID; else ret = 1;
							uiMiscData[0] = uiNextMicID;
							DBG_MUTEX_UNLOCK(&system_mutex);
							
							if (ret == 0)
							{
								memset(pScreenMenu[0].Options[6].Name, 0, 64);
								strcpy(pScreenMenu[0].Options[6].Name, "Микрофон: ");
								
								DBG_MUTEX_LOCK(&modulelist_mutex);	
								ret = ModuleIdToNum(uiMiscData[0], 2);
								if (ret >= 0)
								{							
									if (strlen((char*)miModuleList[ret].Name) < 50) strcat(pScreenMenu[0].Options[6].Name, (char*)miModuleList[ret].Name); 
										else memcpy(pScreenMenu[0].Options[6].Name, miModuleList[ret].Name, 50);
									memset(cCurrentPlayType, 0, 32);
									strcpy(cCurrentPlayType, "Микрофон");
									memset(cCurrentPlayName, 0, 256);
									if (strlen((char*)miModuleList[ret].Name) < 252) 
										memcpy(cCurrentPlayName, (char*)miModuleList[ret].Name, strlen((char*)miModuleList[ret].Name)); 
										else 
										memcpy(cCurrentPlayName, (char*)miModuleList[ret].Name, 252);
								}
								DBG_MUTEX_UNLOCK(&modulelist_mutex);								
								
								f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
								memset(f_link, 0, sizeof(func_link));
								f_link->FuncRecv = RecvAudioFrame;
								f_link->ConnectNum = 1;
								f_link->DeviceNum = 0;
								pAddr = NULL;
															
								DBG_MUTEX_LOCK(&modulelist_mutex);
								pAddr = ModuleIdToAddress(uiMiscData[0], 2);
								if (pAddr) memcpy(&f_link->RecvAddress, pAddr, sizeof(f_link->RecvAddress));
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								if (pAddr)
								{				
									f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
									DBG_MUTEX_LOCK(&modulelist_mutex);
									memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));								
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									pModule = f_link->pModule;
									if (pModule->Settings[0] == 0) 
									{
										f_link->pSubModule = NULL;
									}
									else
									{
										f_link->pSubModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
										DBG_MUTEX_LOCK(&modulelist_mutex);
										memcpy(f_link->pSubModule, &miModuleList[miModuleList[n].SubModule], sizeof(MODULE_INFO));	
										DBG_MUTEX_UNLOCK(&modulelist_mutex);					
									}
									audio_set_playback_volume(iBasicVol);		
									PlayAudioStream(f_link);	
									pScreenMenu[0].Options[6].Show = 1; //Audio menu								
									AddCurShowType(SHOW_TYPE_MIC_STREAM);	

									DBG_MUTEX_LOCK(&modulelist_mutex);
									n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
									if (n >= 0) miModuleList[n].Status[0] = uiMiscData[0];
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									DBG_MUTEX_LOCK(&systemlist_mutex);
									uiLocalSysID = GetLocalSysID();
									DBG_MUTEX_UNLOCK(&systemlist_mutex);
									AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_AUDIO, NULL, 0, 0);										
								}
								else 
								{
									DBG_MUTEX_LOCK(&system_mutex);
									iMiscData[0] = uiNextMicID;
									uiNextMicID = 0;
									uiCurMicID = 0;
									DBG_MUTEX_UNLOCK(&system_mutex);
									DBG_MUTEX_LOCK(&modulelist_mutex);
									n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
									if (n >= 0)	miModuleList[n].Status[0] = 0;
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									dbgprintf(1,"not found MIC ID %.4s\n", (char*)&iMiscData[0]);
									DBG_FREE(f_link);
								}
							}
						}				
					}
					
					if (iNewShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE))
					{
						//printf("OpenServerFile!!!\n");
						unsigned int uiData[2];
						DBG_MUTEX_LOCK(&system_mutex);
						memcpy(&sFileAddr, &rsiRemoteStatus.Address, sizeof(sFileAddr));
						uiMiscData[0] = rsiRemoteStatus.ID;
						uiData[0] = rsiRemoteStatus.Selected;
						uiData[1] = 0;
						if (iNewShow & SHOW_TYPE_CAMERA_FILE) uiData[1] |= 1;
						if (iNewShow & SHOW_TYPE_MIC_FILE) uiData[1] |= 2;
						DBG_MUTEX_UNLOCK(&system_mutex);
						
						DBG_MUTEX_LOCK(&modulelist_mutex);
						ret = ModuleIdToNum(uiMiscData[0], 2);
						if (ret >= 0)
						{	
							memset(cCurrentPlayType, 0, 32);
							strcpy(cCurrentPlayType, "Файл");
							memset(cCurrentPlayName, 0, 256);
							if (strlen((char*)miModuleList[ret].Name) < 252) 
								memcpy(cCurrentPlayName, (char*)miModuleList[ret].Name, strlen((char*)miModuleList[ret].Name)); 
								else 
								memcpy(cCurrentPlayName, (char*)miModuleList[ret].Name, 252);
						}
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						
						if (ret >= 0)
						{
							ret = OpenServerFile(&sFileAddr, uiData[0], uiData[1], 5000);							
							if (((ret & 1) == 0) && (iNewShow & SHOW_TYPE_CAMERA_FILE)) iNewShow ^= SHOW_TYPE_CAMERA_FILE;
							if (((ret & 2) == 0) && (iNewShow & SHOW_TYPE_MIC_FILE)) iNewShow ^= SHOW_TYPE_MIC_FILE;
							if ((iNewShow & (SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE)) == 0) iNewShow = SHOW_TYPE_ERROR;
						}
						else
						{
							DelCurShowType(SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE);
						}
					}
					
					if (iNewShow & SHOW_TYPE_MIC_FILE)
					{			
						SetShowTimeMax(iCamTimeMax);
						DBG_MUTEX_LOCK(&modulelist_mutex);
						n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						if (n >= 0)
						{
							f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
							memset(f_link, 0, sizeof(func_link));
							f_link->FuncRecv = RecvAudioFrame;
							f_link->ConnectNum = 1;
							if (iNewShow & SHOW_TYPE_CAMERA_FILE) f_link->DeviceNum = 1; else f_link->DeviceNum = 2;
							memcpy(&f_link->RecvAddress, &sFileAddr, sizeof(f_link->RecvAddress));
							f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
							DBG_MUTEX_LOCK(&modulelist_mutex);
							memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));								
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							pModule = f_link->pModule;
							if (pModule->Settings[0] == 0) 
							{
								f_link->pSubModule = NULL;
							}
							else 
							{
								f_link->pSubModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
								DBG_MUTEX_LOCK(&modulelist_mutex);
								memcpy(f_link->pSubModule, &miModuleList[miModuleList[n].SubModule], sizeof(MODULE_INFO));	
								DBG_MUTEX_UNLOCK(&modulelist_mutex);					
							}
							audio_set_playback_volume(iBasicVol);		
							PlayAudioStream(f_link);	
							pScreenMenu[0].Options[6].Show = 1; //Audio menu								
							AddCurShowType(SHOW_TYPE_MIC_FILE);	

							DBG_MUTEX_LOCK(&systemlist_mutex);
							uiLocalSysID = GetLocalSysID();
							DBG_MUTEX_UNLOCK(&systemlist_mutex);
							AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_AUDIO, NULL, 0, 0);										
							DBG_MUTEX_LOCK(&system_mutex);
							rsiRemoteStatus.Status = 3;
							DBG_MUTEX_UNLOCK(&system_mutex);
						}
						else 
						{
							DBG_MUTEX_LOCK(&system_mutex);
							rsiRemoteStatus.Status = 0;
							rsiRemoteStatus.Timer = 0;
							if (rsiRemoteStatus.FileOpened || (rsiRemoteStatus.Status >= 2)) ret = 2;
							DBG_MUTEX_UNLOCK(&system_mutex);
							DBG_MUTEX_LOCK(&modulelist_mutex);
							n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
							if (n >= 0) miModuleList[n].Status[0] = 0;
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							dbgprintf(1,"not found SPEAKER for REMOTE FILE ID %.4s\n", (char*)&iMiscData[0]);
							DBG_FREE(f_link);
							iNewShow ^= SHOW_TYPE_CAMERA_FILE;
						}										
					}
					
					if (iNewShow & SHOW_TYPE_CAMERA_FILE)
					{
						if ((ret >= 0) && (ConnectToCamera(0, uiMiscData[0], state, &state->tex[0], &eglImage, &sW, &sH, 2)))
						{
							iSlideShowRenderStatus = 1;
							SetShowTimeMax(iCamTimeMax);
							CreateScreenModel(state, sW,sH, 1, cZoomSet, 1);
							pScreenMenu[0].Options[1].Show = 1; //Camera menu									
							pScreenMenu[0].SelectedOption = 1;
							AddCurShowType(SHOW_TYPE_CAMERA_FILE);
							iCameraListShowed = 1;
							
							DBG_MUTEX_LOCK(&systemlist_mutex);
							uiLocalSysID = GetLocalSysID();
							DBG_MUTEX_UNLOCK(&systemlist_mutex);
							AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_VIDEO, NULL, 0, 0);
							DBG_MUTEX_LOCK(&system_mutex);
							rsiRemoteStatus.Status = 3;
							DBG_MUTEX_UNLOCK(&system_mutex);
						} 
						else 
						{
							ret = 0;
							DBG_MUTEX_LOCK(&system_mutex);
							rsiRemoteStatus.Status = 0;
							rsiRemoteStatus.Timer = 0;
							if (rsiRemoteStatus.FileOpened || (rsiRemoteStatus.Status >= 2)) ret = 2;
							DBG_MUTEX_UNLOCK(&system_mutex);
							dbgprintf(1,"not found REMOTE FILE ID %.4s\n", (char*)&iMiscData[0]);							
						}
					}
					
					if (iNewShow & SHOW_TYPE_CAMERA)
					{
						n = 0;
						DBG_MUTEX_LOCK(&system_mutex);
						//uiCurCameraID = 0;
						//if (uiCurCameraID == uiNextCameraID) n = 1;
						if (uiCurCameraID != uiNextCameraID) uiCurCameraID = uiNextCameraID; else n = 1;
						uiMiscData[0] = uiCurCameraID;
						DBG_MUTEX_UNLOCK(&system_mutex);
						if (n == 0)
						{
							DBG_MUTEX_LOCK(&modulelist_mutex);
							ret = ModuleIdToNum(uiMiscData[0], 2);
							if (ret >= 0)
							{							
								memset(cCurrentPlayType, 0, 32);
								strcpy(cCurrentPlayType, "Камера");
								memset(cCurrentPlayName, 0, 256);
								if (strlen((char*)miModuleList[ret].Name) < 252) 
									memcpy(cCurrentPlayName, (char*)miModuleList[ret].Name, strlen((char*)miModuleList[ret].Name)); 
									else 
									memcpy(cCurrentPlayName, (char*)miModuleList[ret].Name, 252);
							}
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							
							if ((ret >= 0) && (ConnectToCamera(0, uiMiscData[0], state, &state->tex[0], &eglImage, &sW, &sH, 0)))
							{
								iSlideShowRenderStatus = 1;
								SetShowTimeMax(iCamTimeMax);
								CreateScreenModel(state, sW,sH, 1, cZoomSet, 1);
								pScreenMenu[0].Options[1].Show = 1; //Camera menu									
								pScreenMenu[0].SelectedOption = 1;
								AddCurShowType(SHOW_TYPE_CAMERA);
								iCameraListShowed = 1;								
								
								DBG_MUTEX_LOCK(&modulelist_mutex);
								n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
								if (n >= 0) 
								{
									miModuleList[n].Status[0] = uiMiscData[0];		//Camera ID
									int iCamNum = ModuleIdToNum(uiMiscData[0], 2);
									if (iCamNum >= 0) 								//PTZ ID
										miModuleList[n].Status[1] = (miModuleList[iCamNum].Settings[38] & 1) ? miModuleList[iCamNum].Settings[39] : 0;
								}
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								
								DBG_MUTEX_LOCK(&systemlist_mutex);
								uiLocalSysID = GetLocalSysID();
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_VIDEO, NULL, 0, 0);

								DBG_MUTEX_LOCK(&system_mutex);
								uiCurCameraID = uiNextCameraID;	
								DBG_MUTEX_UNLOCK(&system_mutex);
							} 
							else 
							{
								DBG_MUTEX_LOCK(&system_mutex);
								uiCurCameraID = 0;
								uiNextCameraID = 0;
								DBG_MUTEX_UNLOCK(&system_mutex);
								
								DBG_MUTEX_LOCK(&modulelist_mutex);
								n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
								if (n >= 0)
								{
									miModuleList[n].Status[0] = 0;
									miModuleList[n].Status[1] = 0;
								}
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								dbgprintf(1,"not found CAMERA ID '%.4s'\n", (char*)&iMiscData[0]);
							}
						}
					}
					
					if (iNewShow & SHOW_TYPE_CAMERA_LIST)
					{
						if (iCameraListNewCnt != 0)
						{	
							DBG_MUTEX_LOCK(&modulelist_mutex);
							memset(cCurrentPlayType, 0, 32);
							strcpy(cCurrentPlayType, "Камеры");
							memset(cCurrentPlayName, 0, 256);							
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							
							CameraListClear(state, clCameraListCur, &iCameraListCurCnt, 1);
							memcpy(clCameraListCur, clCameraListNew, iCameraListNewCnt * sizeof(CAMERA_LIST_INFO));
							iCameraListCurCnt = iCameraListNewCnt;
							CameraListClear(state, clCameraListNew, &iCameraListNewCnt, 0);
							
							iCameraReconnectTimer = 0;
							iSlideShowRenderStatus = 1;								
							SetShowTimeMax(iCamTimeMax);
							fSizeW = state->screen_width; fSizeH = state->screen_height;
							if (iCameraListCurCnt > 1) {fSizeW = state->screen_width / 2; fSizeH = state->screen_height / 2;}
							if (iCameraListCurCnt > 4) {fSizeW = state->screen_width / 3; fSizeH = state->screen_height / 2;}
							if (iCameraListCurCnt > 6) {fSizeW = state->screen_width / 4; fSizeH = state->screen_height / 3;}
							if (iCameraListCurCnt > 12) {fSizeW = state->screen_width / 5; fSizeH = state->screen_height / 3;}
							if (iCameraListCurCnt > 20) {fSizeW = state->screen_width / 6; fSizeH = state->screen_height / 4;}
							if (iCameraListCurCnt > 24) {fSizeW = state->screen_width / 7; fSizeH = state->screen_height / 5;}
							CreateScreenModel(state, fSizeW,fSizeH, 1, 0, 0);
							fPosW = 0;
							fPosH = state->screen_height - fSizeH;
							for(n = 0; n < iCameraListCurCnt; n++)
							{
								clCameraListCur[n].PosW = fPosW;
								clCameraListCur[n].PosH = fPosH;
								//printf("%i %g %g\n", n, fPosW, fPosH);
								fPosW += fSizeW;
								if (fPosW >= state->screen_width) 
								{
									fPosW = 0;
									fPosH -= fSizeH;
									if (fPosH < 0) fPosH = state->screen_height - fSizeH;
								}
							}
							iCameraListShowed = 0;
							AddCurShowType(SHOW_TYPE_CAMERA_LIST);
							
							DBG_MUTEX_LOCK(&systemlist_mutex);
							uiLocalSysID = GetLocalSysID();
							DBG_MUTEX_UNLOCK(&systemlist_mutex);
							AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_VIDEO, NULL, 0, 0);			
						}
					}
					
					if (!(iNewShow & (SHOW_TYPE_ERROR | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE | SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST)))
					{
						memcpy(CurrentFile, cCurrentFile, 256);
						pLinkNextShow = CurrentFile;
						ret = 0;
						if (iNewShow & (SHOW_TYPE_FILE | SHOW_TYPE_URL | SHOW_TYPE_IMAGE))
						{
							if ((iNewShow & SHOW_TYPE_IMAGE)) ret = FILE_TYPE_IMAGE; else ret = FILE_TYPE_MEDIA;
							if (cPlayDirect == 4) 
							{
								cPlayDirect = 0;
								pLinkNextShow = cNextYouTubeURL;
							}
							else memcpy(CurrentFile, cCurrentFile, 256);
						}
						else
						{
							if (iNewShow & SHOW_TYPE_ALARM1) 
							{
								if (strlen(cCurrentAlarmPath) && (cCurrentAlarmPath[strlen(cCurrentAlarmPath)-1] != 47) && (Its_Directory(cCurrentAlarmPath) == 0))
								{
									memset(CurrentFile, 0, 256);
									if (strlen(cCurrentAlarmPath) < 256) strcpy(CurrentFile, cCurrentAlarmPath);
									ret = GetFileType(CurrentFile);
								}	
								else ret = GetNextAlarmFile(CurrentFile, 256);															
							}
							else
							{		
								int iFileNum = 0;
								int iFileOrderCnt = 0;
								if ((cPlayDirect == 0) || (cPlayDirect == 5)) ret = GetNextFile(CurrentFile, 256, cRandomFile, &iFileNum, &iFileOrderCnt);
								
								if (cPlayDirect == 1)  
								{
									ret = GetPrevFile(CurrentFile, 256, cRandomFile, &iFileNum, &iFileOrderCnt);
									cPlayDirect = 0;
								}
								
								if (cPlayDirect == 2) cPlayDirect = 0;
								if (cPlayDirect == 3)
								{
									ret = GetSpecFile(CurrentFile, 256, &iFileNum, &iFileOrderCnt);
									cPlayDirect = 0;
								}
								
								if ((cPlayDirect == 0) && (ret & FILE_TYPE_MEDIA) && (GetCurShowType() & (SHOW_TYPE_VIDEO | SHOW_TYPE_AUDIO)))
								{
									DelCurShowType(SHOW_TYPE_IMAGE);
									//printf("WaitNextFile %i %i %s\n", cPlayDirect, ret, CurrentFile);
									ret = 0;									
								}
								else 
								{
									AcceptFileInList(iFileNum, iFileOrderCnt);
									//printf("GetNextFile %i %i %s\n", cPlayDirect, ret, CurrentFile);								
								}
								if (cPlayDirect == 5) cPlayDirect = 0;
							}
						}
						
						dbgprintf(4,"Next file:'%s' %i\n", CurrentFile, ret);	
						if (ret & FILE_TYPE_MEDIA)
						{	
							if (GetCurShowType() & (SHOW_TYPE_FILE | SHOW_TYPE_VIDEO | SHOW_TYPE_AUDIO | SHOW_TYPE_WEB_STREAM))
							{
								iRadioTimerOff = 0;
								dbgprintf(5,"kick all\n");																		
								pScreenMenu[0].Options[1].Show = 0; //Camera menu
								pScreenMenu[0].Options[2].Show = 0; //Foto menu
								pScreenMenu[0].Options[3].Show = 0; //Media menu
								pScreenMenu[0].Options[6].Show = 0; //Audio menu
								pScreenMenu[0].Options[20].Show = 0; //Stream menu								
								Media_StopPlay(1);
								omx_stop_video(0);
								Audio_Stop(0);	
								omx_stop_video(1);
								Audio_Stop(1);	
								CloseAllConnects(CONNECT_CLIENT, FLAG_VIDEO_STREAM | FLAG_AUDIO_STREAM, 0);
								
								DBG_MUTEX_LOCK(&modulelist_mutex);
								n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
								if (n >= 0)
								{
									miModuleList[n].Status[0] = 0;
									miModuleList[n].Status[1] = 0;
								}
								n = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
								if (n >= 0) miModuleList[n].Status[0] = 0;
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								DBG_MUTEX_LOCK(&systemlist_mutex);
								uiLocalSysID = GetLocalSysID();
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								if (GetCurShowType() & SHOW_TYPE_VIDEO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_VIDEO, NULL, 0, 0);		
								if (GetCurShowType() & SHOW_TYPE_AUDIO) AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_STOPED_AUDIO, NULL, 0, 0);		
								
								DelCurShowType(SHOW_TYPE_FILE | SHOW_TYPE_VIDEO | SHOW_TYPE_AUDIO | SHOW_TYPE_IMAGE | SHOW_TYPE_WEB_STREAM);
							}
							//ret = GetTypesStreamsMediaFile(CurrentFile);
							//ret = GetTypesStreamsMediaFile("/mnt/FLASH/sz2.mkv");
							ret = PlayMediaFile(pLinkNextShow, (uiDevType & DEVICE_TYPE_VIDEO_OUT) ? 1 : 0, (uiDevType & DEVICE_TYPE_AUDIO_OUT) ? 1 : 0);								
							if (ret & (FILE_TYPE_VIDEO | FILE_TYPE_AUDIO)) 
							{									
								//PlayMediaFile("/mnt/FLASH/sz2.mkv");
								memset(pScreenMenu[0].Options[3].Name, 0, 64);
								if (iNewShow & SHOW_TYPE_URL) 
								{
									strcpy(pScreenMenu[0].Options[20].Name, "Поток: ");
									DBG_MUTEX_LOCK(&system_mutex);		
									if (iRadioCode < iInternetRadioCnt) 
									{
										if (strlen(irInternetRadio[iRadioCode].Name) < 55) 
											strcat(pScreenMenu[0].Options[20].Name, irInternetRadio[iRadioCode].Name);
											else
											memcpy(&pScreenMenu[0].Options[20].Name[7], irInternetRadio[iRadioCode].Name, 55);	
									}
									memset(cShowFileNameText, 0, 128);
									if (strlen(irInternetRadio[iRadioCode].Name) < 128) 
										memcpy(cShowFileNameText, irInternetRadio[iRadioCode].Name, strlen(irInternetRadio[iRadioCode].Name)); 
										else 
										memcpy(cShowFileNameText, irInternetRadio[iRadioCode].Name, 127);
									pScreenMenu[0].Options[20].Show = 1; //Stream menu								
									
									char cTempBuff[256];
									memset(cTempBuff, 0, 256);
									if (strlen(irInternetRadio[iRadioCode].Name) < 252) 
										memcpy(cTempBuff, irInternetRadio[iRadioCode].Name, strlen(irInternetRadio[iRadioCode].Name)); 
										else 
										memcpy(cTempBuff, irInternetRadio[iRadioCode].Name, 252);
									
									DBG_MUTEX_UNLOCK(&system_mutex);
									
									DBG_MUTEX_LOCK(&modulelist_mutex);
									memset(cCurrentPlayType, 0, 32);
									strcpy(cCurrentPlayType, "Поток");
									memcpy(cCurrentPlayName, cTempBuff, 256);
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									
									AddCurShowType(SHOW_TYPE_WEB_STREAM);
								} 
								else 
								{
									DBG_MUTEX_LOCK(&modulelist_mutex);
									strcpy(pScreenMenu[0].Options[3].Name, "Медиа: ");
									char cbuff[2048];
									unsigned int ilen = 0;
									int ist = 0;
									memset(cbuff, 0, 2048);
									ist = GetCharPos(CurrentFile, strlen(CurrentFile), 47, 1) + 1;									
									ilen = 128;
									utf8_to_cp866(&CurrentFile[ist], (strlen(&CurrentFile[ist]) < ilen) ? strlen(&CurrentFile[ist]) : 124, cShowFileNameText, &ilen);
									if (ilen < 55) strcat(pScreenMenu[0].Options[3].Name, cShowFileNameText); else strcat(pScreenMenu[0].Options[3].Name, &cShowFileNameText[ist - 55]);
									pScreenMenu[0].Options[3].Show = 1; //Media menu								
									
									memset(cCurrentPlayType, 0, 32);
									strcpy(cCurrentPlayType, "Медиа");
									memset(cCurrentPlayName, 0, 256);
									ilen = 252;
									utf8_to_cp866(CurrentFile, (strlen(CurrentFile) < 252) ? strlen(CurrentFile) : 252, cCurrentPlayName, &ilen);
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									
									AddCurShowType(SHOW_TYPE_FILE);
								}							
							} else SetNewShowType(SHOW_TYPE_NA);
							
							if (ret & FILE_TYPE_AUDIO) 
							{
								if (GetCurShowType() & (SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE))
								{
									//printf("kick audio\n");
									pScreenMenu[0].Options[6].Show = 0; //Audio menu
									Audio_Stop(1);	
									CloseAllConnects(CONNECT_CLIENT, FLAG_AUDIO_STREAM, 0);
									DelCurShowType(SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE);
								}
								ret ^= FILE_TYPE_AUDIO;
								SetShowTimeMax(iSlideShowTimerSet);						
								DBG_MUTEX_LOCK(&modulelist_mutex);					
								for (n = 0; n != iModuleCnt; n++)
								{
									if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_SPEAKER))
									{
										ret |= FILE_TYPE_AUDIO;
										//if (GetShowType() & SHOW_TYPE_VIDEO) SetShowType(GetShowType() | SHOW_TYPE_AUDIO); else SetShowType(SHOW_TYPE_BACK_STREAM);
										f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
										memset(f_link, 0, sizeof(func_link));
										f_link->FuncRecv = ReadAudioFrame;
										f_link->ConnectNum = 0;
										f_link->DeviceNum = 0;
										f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));						
										memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));	
										if (miModuleList[n].Settings[0] == 0) 
										{
											f_link->pSubModule = NULL; 
										}
										else 
										{
											f_link->pSubModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
											memcpy(f_link->pSubModule, &miModuleList[miModuleList[n].SubModule], sizeof(MODULE_INFO));								
										}
										audio_set_playback_volume(iCurrentVolume);
										PlayAudioStream(f_link);
										iShowFileNameStatus = 1;
										break;
									}
								}								
								DBG_MUTEX_UNLOCK(&modulelist_mutex);	
								if (ret & FILE_TYPE_AUDIO) 
								{
									AddCurShowType(SHOW_TYPE_AUDIO);
									
									DBG_MUTEX_LOCK(&systemlist_mutex);
									uiLocalSysID = GetLocalSysID();
									DBG_MUTEX_UNLOCK(&systemlist_mutex);
									AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_AUDIO, NULL, 0, 0);		
									//if (!(ret & FILE_TYPE_VIDEO)) AddCurShowType(SHOW_TYPE_IMAGE);
								}
							}
							//dbg_func_print_entered();
							if (ret & FILE_TYPE_VIDEO) 
							{	
								if (GetCurShowType() & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST))
								{
									//printf("kick video\n");
									pScreenMenu[0].Options[1].Show = 0; //Camera menu							
									omx_stop_video(1);
									CloseAllConnects(CONNECT_CLIENT, FLAG_VIDEO_STREAM, 0);
									DelCurShowType(SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST);
								}
								DelCurShowType(SHOW_TYPE_IMAGE);
								sW = state->screen_width;
								sH = state->screen_height;
								glDeleteTextures(1, &state->tex[0]);
								glGenTextures(1,&state->tex[0]);
								f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
								memset(f_link, 0, sizeof(func_link));
								f_link->FuncRecv = ReadVideoFrame;								
								f_link->ConnectNum = 0;
								
								int omx_res = omx_play_video_on_egl_from_func(f_link, state->tex[0], &state->display, &state->context, &eglImage, &sW, &sH, 1, 0);
								if (omx_res)
								{
									CreateScreenModel(state, sW,sH, 1, cZoomSet, 1);
									AddCurShowType(SHOW_TYPE_VIDEO);
									iSlideShowRenderStatus = 1;
									SetShowTimeMax(iSlideShowTimerSet);
									iShowFileNameStatus = 1;
									
									DBG_MUTEX_LOCK(&systemlist_mutex);
									uiLocalSysID = GetLocalSysID();
									DBG_MUTEX_UNLOCK(&systemlist_mutex);
									AddModuleEventInList(uiLocalSysID, 15, SYSTEM_CMD_PLAY_VIDEO, NULL, 0, 0);	
								}
								else 
								{
									if (omx_res == 100) 
										dbgprintf(2,"warning OpenMax BUSY (resource priority low)\n");
										else
										dbgprintf(1,"error FILE_TYPE_VIDEO omx_play_video_on_egl_from_func\n");
								}
							}
						}
						if ((iNewShow & SHOW_TYPE_ALARM1) && ((ret & FILE_TYPE_AUDIO) == 0)) 
						{
							iCurrentVolume = iAlrmVol;
							audio_set_playback_volume(iCurrentVolume);			
							PlayAlarmSound();	
							ClockAlarmStop(0);
							if (iChangeVolumeSoft) iChangeVolumeSoft = 0;
							//audio_set_playback_volume(iCurrentVolume);								
						}
						if (ret & FILE_TYPE_IMAGE) 
						{
							if (GetCurShowType() & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST))
							{
								pScreenMenu[0].Options[1].Show = 0;
								omx_stop_video(1);
								CloseAllConnects(CONNECT_CLIENT, FLAG_VIDEO_STREAM, 0);
								DelCurShowType(SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST);
							}
							if (GetCurShowType() & SHOW_TYPE_VIDEO)
							{
								dbgprintf(5,"Stop video to image\n");
								pScreenMenu[0].Options[3].Show = 0;
								pScreenMenu[0].Options[20].Show = 0;
								Media_StopPlay(1);
								omx_stop_video(0);
								if (GetCurShowType() & SHOW_TYPE_AUDIO) Audio_Stop(0);
								omx_stop_video(1);								
								if (GetCurShowType() & SHOW_TYPE_AUDIO) Audio_Stop(1);
								CloseAllConnects(CONNECT_CLIENT, FLAG_VIDEO_STREAM, 0);
								DelCurShowType(SHOW_TYPE_VIDEO | SHOW_TYPE_AUDIO | SHOW_TYPE_FILE | SHOW_TYPE_WEB_STREAM);
							}
							DelCurShowType(SHOW_TYPE_IMAGE);	
							//iRadioTimeMax = 0;
							unsigned int ilen;	
							SetShowTimeMax(iSlideShowTimer);
							memset(pScreenMenu[0].Options[2].Name, 0, 64);
							strcpy(pScreenMenu[0].Options[2].Name, "Фото: ");
							
							ilen = 0;
							int ist = 0;
							ist = GetCharPos(CurrentFile, strlen(CurrentFile), 47, 1) + 1;
							ilen = 55;
							utf8_to_cp866(&CurrentFile[ist], (strlen(&CurrentFile[ist]) < ilen) ? strlen(&CurrentFile[ist]) : 52, &pScreenMenu[0].Options[2].Name[6], &ilen);
							
							DBG_MUTEX_LOCK(&modulelist_mutex);
							memset(cCurrentPlayType, 0, 32);
							strcpy(cCurrentPlayType, "Фото");
							ilen = 252;
							utf8_to_cp866(CurrentFile, (strlen(CurrentFile) < 252) ? strlen(CurrentFile) : 252, cCurrentPlayName, &ilen);
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							
							sW = state->screen_width;
							sH = state->screen_height;
							glDeleteTextures(1, &state->tex[0]);
							glGenTextures(1,&state->tex[0]);
					
							if (omx_image_to_egl(CurrentFile, state->tex[0], &state->display, &state->context, &eglImage, &sW, &sH) == 1)
							{								
								/*glBindTexture (GL_TEXTURE_2D, state->tex[0]);
								omx_jpegfile_to_surface(CurrentFile, state->screen_width, state->screen_height, (char*)state->tex_main);
								glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, state->screen_width, state->screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->tex_main);
								*/
								CreateScreenModel(state, sW,sH, 1, cZoom, 1);
								iSlideShowRenderStatus = 1;
								pScreenMenu[0].Options[2].Show = 1; //Foto menu
								AddCurShowType(SHOW_TYPE_IMAGE);
							}
							else dbgprintf(1,"error SHOW_TYPE_IMAGE omx_image_to_egl\n");
						}	
						if (iCurShow & SHOW_TYPE_ALARM1) DelCurShowType(SHOW_TYPE_ALARM1);
					}
				}
				SetNewShowType(SHOW_TYPE_NA);
				/*else
				{
					iSlideShowRenderStatus = 0;
				}	*/	  
				iFade_Direct = 3;
			}//DECODE NEXT PHOTO			
				    
			if (iFade_Direct == 3) 
			{
				//if ((GetShowType() & (SHOW_TYPE_IMAGE | SHOW_TYPE_FILE)) == SHOW_TYPE_IMAGE) iFade -= 2; else iFade-=20; 
				if (uiSMode >= 2)
				{
					if (GetChangeShowNow() == 2) iFade -= 2; else iFade-=20;  
				} else iFade = 0;
				if (iFade <= 0) 
				{
					iFade = 0;
					iFade_Direct = 0;
					SetChangeShowNow(0);					
				}
			}	

			if ((iCameraListShowed == 0) && (iCameraListCurCnt != 0) && (GetCurShowType() & SHOW_TYPE_CAMERA_LIST))
			{
				if (iCameraReconnectTimer) iCameraReconnectTimer--;
				if (iCameraReconnectTimer == 0)
				{
					iCameraReconnectTimer = CAMERA_RECONNECT_TIME;
					ret = 0;
					for (n = 0; n < iCameraListCurCnt; n++)
					{
						if (clCameraListCur[n].Status == 0)
						{
							if (clCameraListCur[n].ID != 0)
							{
								//printf("Try Connect to %.4s\n", (char*)&clCameraListCur[n].ID);
								if (ConnectToCamera(n, clCameraListCur[n].ID, state, &clCameraListCur[n].Texture, &clCameraListCur[n].Image, &sW, &sH, 1))
								{
									clCameraListCur[n].Status = 1;
									//printf("Connected to %.4s\n", (char*)&clCameraListCur[n].ID);
								} 
								else 
								{
									glDeleteTextures(1, &clCameraListCur[n].Texture);
									ret++;
								}
							} else clCameraListCur[n].Status = 1;
						}
					}
					if (ret == 0) iCameraListShowed = 1;
				}
			}
		
			RenderStart();
			ret = GetCurShowType();
			if //(((uiSMode >= 2) && (iShowStatus == SHOW_ENABLED)) || 
				((uiSMode >= 1) && (ret & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_VIDEO | SHOW_TYPE_IMAGE))) 
					Rendering(state, iSlideShowRenderStatus, GetCurShowType(), iVertSync, clCameraListCur, iCameraListCurCnt);
			if ((uiSMode >= 1) & (!(ret & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_VIDEO))))
			{
				RenderWidgets(state, iSlideShowRenderStatus);
			}
			
			DBG_MUTEX_LOCK(&system_mutex);	
			iRemStatus = 0;
			ret2 = iAlarmEvents;
			if (iSysMessageCnt)
			{
				ret = smiSysMessage[0].ID;
				ret4 = smiSysMessage[0].Data;
				iSysMessageCnt--;			
				if (iSysMessageCnt) memmove(smiSysMessage, &smiSysMessage[1], iSysMessageCnt*sizeof(SYS_MESSAGE_INFO));
			}
			else
			{
				ret = SYSTEM_CMD_NULL;
				ret4 = 0;
			}
			
			if (((ret == SYSTEM_CMD_MENU_KEY_LEFT) ||
				(ret == SYSTEM_CMD_MENU_KEY_RIGHT) ||
				(ret == SYSTEM_CMD_MENU_KEY_UP) ||
				(ret == SYSTEM_CMD_MENU_KEY_DOWN)) && (rsiRemoteStatus.Status == 0) 
				&& (cCurShowType & (SHOW_TYPE_CAMERA | SHOW_TYPE_MIC_STREAM)) 
				&& (cShowMenu == 0)
				&& ((uiCurCameraID != 0) || uiNextMicID != 0))
				{
					unsigned int iRemId = uiCurCameraID;
					if (uiCurCameraID) iRemId = uiCurCameraID; else iRemId = uiNextMicID;		
					DBG_MUTEX_UNLOCK(&system_mutex);
					DBG_MUTEX_LOCK(&modulelist_mutex);
					int iRemNum = ModuleIdToNum(iRemId, 2);
					struct sockaddr_in sAddr;						
					if (iRemNum >= 0)
					{							
						iRemId = miModuleList[iRemNum].ParentID;
						iRemNum = ModuleIdToNum(iRemId, 2);
						if (iRemNum >= 0)
							memcpy(&sAddr, &miModuleList[iRemNum].Address, sizeof(miModuleList[iRemNum].Address));						
							else
							iRemNum	= -1;						
					}
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					DBG_MUTEX_LOCK(&system_mutex);
					if (iRemNum >= 0)
					{			
						OpenRemoteList(iRemId, &sAddr, 1);
					}
				}
			if ((rsiRemoteStatus.Status) && ((ret == SYSTEM_CMD_MENU_KEY_MENU))) 
			{
				rsiRemoteStatus.Status = 0;
				rsiRemoteStatus.Timer = 0;
				if (rsiRemoteStatus.FileOpened || (rsiRemoteStatus.Status >= 2)) iRemStatus = 2;
			}
			if (rsiRemoteStatus.Status)
			{			
				RenderText(state, 30*fMSize, 30*fMSize, 90, 0, rsiRemoteStatus.Path);	
				if (RenderRemoteFileMenu(state, ret, fMSize)) iRemStatus = 1;
				RenderRemoteFileStatus(state, fMSize);
			}
			if (rsiRemoteStatus.Status)
			{
				if (cShowMenu == 0) //&& (cCurShowType & SHOW_TYPE_CAMERA_FILE)					
				{
					if ((ret == SYSTEM_CMD_MENU_KEY_UP) ||
						(ret == SYSTEM_CMD_MENU_KEY_DOWN) ||
						(ret == SYSTEM_CMD_MENU_KEY_LEFT) ||
						(ret == SYSTEM_CMD_MENU_KEY_RIGHT) ||
						(ret == SYSTEM_CMD_MENU_KEY_OK) ||
						(ret == SYSTEM_CMD_SHOW_PAUSE) ||
						(ret == SYSTEM_CMD_SHOW_PLAY) ||
						(ret == SYSTEM_CMD_SHOW_STOP_AV) ||
						(ret == SYSTEM_CMD_MENU_KEY_BACK))
					{
						if (rsiRemoteStatus.Status == 1) rsiRemoteStatus.Timer = FILE_VIEW_TIME;
						if (rsiRemoteStatus.Status == 2) rsiRemoteStatus.Timer = FILE_SHOW_TIME;
						ret = SYSTEM_CMD_NULL;							
					}
					else 
					{
						if ((rsiRemoteStatus.Status == 1) || (rsiRemoteStatus.Status == 2)) 
						{
							rsiRemoteStatus.Timer--;
							if (rsiRemoteStatus.Timer == 0) 
							{
								if (rsiRemoteStatus.FileOpened || (rsiRemoteStatus.Status >= 2)) iRemStatus = 2;		
								rsiRemoteStatus.Status = 0;
							}
					//		printf("Time %i %i %i\n", rsiRemoteStatus.Timer, rsiRemoteStatus.Status, rsiRemoteStatus.FileOpened);						
						}
					}						
				}
				else 
				{
					rsiRemoteStatus.Status = 0;
					rsiRemoteStatus.Timer = 0;
					if (rsiRemoteStatus.FileOpened || (rsiRemoteStatus.Status >= 2)) iRemStatus = 2;
				}
			}	
			
			DBG_MUTEX_UNLOCK(&system_mutex);
			if (iRemStatus == 1)
			{
				if (uiDevType & DEVICE_TYPE_AUDIO_OUT) 
					SetNewShowType(SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_MIC_FILE);
					else
					SetNewShowType(SHOW_TYPE_CAMERA_FILE);
				SetChangeShowNow(1);
				//printf("$$$$ %i\n", GetNewShowType());
			}
			if (iRemStatus == 2)
			{
				SetNewShowType(SHOW_TYPE_NEW);
				SetChangeShowNow(1);
			}
			if (ret == SYSTEM_CMD_TIMERS_INCREASE)
			{
				iMiscData[6] = GetCurShowType();
				if (iMiscData[6] & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE)) 
				{
					iCamTimeMax += ret4;
					if (iCamTimeMax > 10000) iCamTimeMax = 10000;
				}
				else
				if (iMiscData[6] & (SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM))
				{
					iRadioTimeMax += ret4;
					if (iRadioTimeMax > 10000) iRadioTimeMax = 10000;
				}
				else
				{
					iSlideShowTimer += ret4;
					if (iSlideShowTimer > 10000) iSlideShowTimer = 10000;
				}
				uiTimerShowTimer = 0;
			}
			if (ret == SYSTEM_CMD_TIMERS_DECREASE)
			{
				iMiscData[6] = GetCurShowType();
				if (iMiscData[6] & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE)) 
				{
					iCamTimeMax -= ret4;
					if (iCamTimeMax < 15) iCamTimeMax = 15;
				}
				else
				if (iMiscData[6] & (SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM))
				{
					iRadioTimeMax -= ret4;
					if (iRadioTimeMax < 60) iRadioTimeMax = 60;
				}
				else
				{
					iSlideShowTimer -= ret4;
					if (iSlideShowTimer < 15) iSlideShowTimer = 15;
				}
				uiTimerShowTimer = 0;
			}
			if (ret == SYSTEM_CMD_TIMERS_UPDATE)
			{
				DBG_MUTEX_LOCK(&system_mutex);								
				iSlideShowTimer = iSlideShowTimerSet;
				iCamTimeMax = iCameraTimeMaxSet;
				iRadioTimeMax = iStreamTimeMaxSet;
				DBG_MUTEX_UNLOCK(&system_mutex);	
			}
			
			if (ret == SYSTEM_CMD_RESET_TIMER) 
			{
				if (ret4 & 1) iTimerToNextShow = 0;
				if (ret4 & 2) iRadioTimerOff = 0;
			}
			
			if (ret == SYSTEM_CMD_RADIO_VOLUME_DOWN) Menu_RadioVolumeDown(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_RADIO_VOLUME_UP) Menu_RadioVolumeUp(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_RADIO_STATION_NEXT) Menu_NextRadioStation(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_RADIO_STATION_PREV) Menu_PrevRadioStation(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_SOUND_VOLUME_DEC) {Menu_MusicVolumeDec(pScreenMenu, ret4); iChangeVolumeSoft = 0;}
			if (ret == SYSTEM_CMD_SOUND_VOLUME_INC) {Menu_MusicVolumeInc(pScreenMenu, ret4); iChangeVolumeSoft = 0;}
			if (ret == SYSTEM_CMD_SOUND_VOLUME_DOWN) {Menu_MusicVolumeDown(pScreenMenu, 0); iChangeVolumeSoft = 0;}
			if (ret == SYSTEM_CMD_SOUND_VOLUME_UP) {Menu_MusicVolumeUp(pScreenMenu, 0); iChangeVolumeSoft = 0;}
			if (ret == SYSTEM_CMD_SOUND_VOLUME_MUTE) {Menu_MusicVolumeMute(pScreenMenu, 0); iChangeVolumeSoft = 0;}
			if (ret == SYSTEM_CMD_SOUND_VOLUME_SET) {MusicVolumeSet(ret4); iChangeVolumeSoft = 0;}
			if (ret == SYSTEM_CMD_ALARM_VOLUME_DEC) Menu_AlarmVolumeDec(pScreenMenu, ret4);
			if (ret == SYSTEM_CMD_ALARM_VOLUME_INC) Menu_AlarmVolumeInc(pScreenMenu, ret4);
			if (ret == SYSTEM_CMD_ALARM_VOLUME_DOWN) Menu_AlarmVolumeDown(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_ALARM_VOLUME_UP) Menu_AlarmVolumeUp(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_ALARM_VOLUME_SET) Menu_AlarmVolumeSet(pScreenMenu, ret4);
			if (ret == SYSTEM_CMD_SOFT_VOLUME_SET) {iChangeVolumeNext = ret4; iChangeVolumeSoft = 1; fChangeVolumeCurrent = iCurrentVolume;}
			if (ret == SYSTEM_CMD_SOFT_VOLUME_STEP) fChangeVolumeStep = (float)ret4/1000;
			if (ret == SYSTEM_CMD_CLEAR_MESSAGE) Menu_ClearMessageList(pScreenMenu, 12);
			if (ret == SYSTEM_CMD_RADIO_ON) Menu_OnRadioStation(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_PLAY_NEXT_DIR) Menu_PlayNextDir(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_PLAY_PREV_DIR) Menu_PlayPrevDir(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_STREAM_ON) StreamOn(ret4);
			if (ret == SYSTEM_CMD_STREAM_ON_LAST) StreamOnLast();			
			if (ret == SYSTEM_CMD_STREAM_ON_NEXT) StreamOnNext();
			if (ret == SYSTEM_CMD_STREAM_ON_PREV) StreamOnPrev();
			if (ret == SYSTEM_CMD_STREAM_TYPE_ON) StreamTypeOn(ret4);
			if (ret == SYSTEM_CMD_STREAM_TYPE_RND_ON) StreamTypeOnRandom(ret4);
			if (ret == SYSTEM_CMD_STREAM_RND_ON) StreamOnRandom();
			if (ret == SYSTEM_CMD_SET_STATUS_MESSAGE) 
			{
				DBG_MUTEX_LOCK(&modulelist_mutex);
				memcpy(cStatusMessage, cSysStatus, 64);
				DBG_MUTEX_UNLOCK(&modulelist_mutex);				
			}
			if (ret == SYSTEM_CMD_RANDOM_FILE_ON) 
			{
				DBG_MUTEX_LOCK(&system_mutex);		
				cCurRandomFile = 1;
				DBG_MUTEX_UNLOCK(&system_mutex);
				cRandomFile = 1;				
			}
			if (ret == SYSTEM_CMD_RANDOM_FILE_OFF)
			{
				DBG_MUTEX_LOCK(&system_mutex);		
				cCurRandomFile = 0;
				DBG_MUTEX_UNLOCK(&system_mutex);
				cRandomFile = 0;
			}
			if (ret == SYSTEM_CMD_RANDOM_DIR_ON) 
			{
				DBG_MUTEX_LOCK(&system_mutex);		
				cCurRandomDir = 1;
				DBG_MUTEX_UNLOCK(&system_mutex);
				cRandomDir = 1;				
			}
			if (ret == SYSTEM_CMD_RANDOM_DIR_OFF)
			{
				DBG_MUTEX_LOCK(&system_mutex);		
				cCurRandomDir = 0;
				DBG_MUTEX_UNLOCK(&system_mutex);
				cRandomDir = 0;
			}
			if (ret == SYSTEM_CMD_CAMLIST_CLEAR) CameraListClear(state, clCameraListNew, &iCameraListNewCnt, 0);										
			if (ret == SYSTEM_CMD_CAMLIST_ADD) CameraListAdd(ret4, clCameraListNew, &iCameraListNewCnt);
			if (ret == SYSTEM_CMD_CAMLIST_SHOW) 
			{
				SetNewShowType(SHOW_TYPE_CAMERA_LIST);
				SetChangeShowNow(1);
			}
			if (ret == SYSTEM_CMD_OPENED_FILE)
			{
				//reset_sys_cmd_list();
				//SetNewShowType(SHOW_TYPE_CAMERA_FILE);
				//SetChangeShowNow(1);
				
				/*DBG_MUTEX_LOCK(&system_mutex);
				//rsiRemoteStatus.PlayStatus = 1;
				//rsiRemoteStatus.FileOpened = 1;
				printf("SYSTEM_CMD_OPENED_FILE %i %i %i %i\n", rsiRemoteStatus.flv_info.AudioEnabled, 
														rsiRemoteStatus.flv_info.VideoEnabled,
														rsiRemoteStatus.flv_info.AudioStream,
														rsiRemoteStatus.flv_info.VideoStream);
				DBG_MUTEX_UNLOCK(&system_mutex);*/
			}	
			if (ret == SYSTEM_CMD_CLOSED_FILE)
			{
				//DBG_MUTEX_LOCK(&system_mutex);
				//rsiRemoteStatus.FileOpened = 0;
				//rsiRemoteStatus.PlayStatus = 0;
				/*if (rsiRemoteStatus.Status) 
				{
					rsiRemoteStatus.Status = 0;
					DBG_MUTEX_UNLOCK(&system_mutex);
					//SetNewShowType(SHOW_TYPE_NEW);
					//SetChangeShowNow(1);				
				} else */
				//DBG_MUTEX_UNLOCK(&system_mutex);
				
			}
			if (ret == SYSTEM_CMD_ERROR_FILE)
			{
				/*DBG_MUTEX_LOCK(&system_mutex);
				rsiRemoteStatus.FileOpened = 0;
				rsiRemoteStatus.PlayStatus = 0;
				if (rsiRemoteStatus.Status) 
				{
					rsiRemoteStatus.Status = 0;
					DBG_MUTEX_UNLOCK(&system_mutex);
					SetNewShowType(SHOW_TYPE_NEW);
					SetChangeShowNow(1);				
				} else DBG_MUTEX_UNLOCK(&system_mutex);	*/			
			}
			if (ret == SYSTEM_CMD_DONE_FILE)
			{
				DBG_MUTEX_LOCK(&system_mutex);
				if (rsiRemoteStatus.Status > 2) 
				{
					rsiRemoteStatus.PlayStatus = 0;
					rsiRemoteStatus.Status = 2;
					//rsiRemoteStatus.Direct = 21;
				}
				DBG_MUTEX_UNLOCK(&system_mutex);				
			}
			if (ret == SYSTEM_CMD_BUSY_FILE)
			{
				DBG_MUTEX_LOCK(&system_mutex);
				if (rsiRemoteStatus.Status > 2) 
				{
					rsiRemoteStatus.PlayStatus = 3;
					rsiRemoteStatus.Status = 2;
					rsiRemoteStatus.Timer = FILE_SHOW_TIME;
				}
				DBG_MUTEX_UNLOCK(&system_mutex);
			}
			if (ret == SYSTEM_CMD_FREE_FILE)
			{
				DBG_MUTEX_LOCK(&system_mutex);
				if ((rsiRemoteStatus.Status >= 2) && (rsiRemoteStatus.PlayStatus == 3))
				{
					omx_player_play();
					Audio_Play();
					rsiRemoteStatus.PlayStatus = 1;
					rsiRemoteStatus.Status = 3;
					rsiRemoteStatus.Timer = FILE_SHOW_TIME;
				}
				DBG_MUTEX_UNLOCK(&system_mutex);
			}
			
			if (ret == SYSTEM_CMD_NEW_FILE_POS)
			{
				/*omx_player_play();
				DBG_MUTEX_LOCK(&system_mutex);
				rsiRemoteStatus.PlayStatus = 1;
				DBG_MUTEX_UNLOCK(&system_mutex);*/
			}
			if (ret == SYSTEM_CMD_VIDEO_ERROR) Menu_CloseVideo(NULL, -1);
			if (ret == SYSTEM_CMD_AUDIO_ERROR) Menu_CloseAudio(NULL, -1);
			if (ret == SYSTEM_CMD_CAMERA_ERROR) 
			{
				if (GetChangeShowNow() == 0)
				{
					if (GetCurShowType() & SHOW_TYPE_CAMERA)
					{	
						SetNewShowType(SHOW_TYPE_NEW);
						SetChangeShowNow(1);
					}
					if (GetCurShowType() & SHOW_TYPE_CAMERA_LIST)
					{	
						//printf("### %i %i %i\n", ret4, iCameraListCurCnt, clCameraListCur[ret4].Status);
						if ((ret4 < iCameraListCurCnt) && (clCameraListCur[ret4].Status))
						{
							omx_stop_video_num(ret4, 1);
							glDeleteTextures(1, &clCameraListCur[ret4].Texture);
							if (clCameraListCur[ret4].Image && (!eglDestroyImageKHR(state->display, (EGLImageKHR)clCameraListCur[ret4].Image)))
								dbgprintf(1,"CameraListClear: eglDestroyImageKHR failed.\n");			
							iCameraListShowed = 0;
							iCameraReconnectTimer = CAMERA_RECONNECT_TIME;
							clCameraListCur[ret4].Status = 0;
							clCameraListCur[ret4].Image = NULL;
						}
						else
						{
							SetNewShowType(SHOW_TYPE_NEW);
							SetChangeShowNow(1);
						}
					}
				}
			}
			if (ret == SYSTEM_CMD_SHOW_NEXT) Menu_GetNext(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_SHOW_PREV) Menu_GetPrev(pScreenMenu, 0);
			if (ret == SYSTEM_CMD_SHOW_AGAIN) Menu_GetAgain(pScreenMenu, 0);
			
			if (ret == SYSTEM_CMD_MENU_NEXT) 
			{
				if ((GetCurShowType() & (SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM)))
				StreamOnNext();
				else Menu_PlayNextDir(pScreenMenu, 0);
			}
			if (ret == SYSTEM_CMD_MENU_PREV) 
			{
				if ((GetCurShowType() & (SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM)))
				StreamOnPrev();
				else Menu_PlayPrevDir(pScreenMenu, 0);
			}
			
			if (ret == SYSTEM_CMD_SET_ZOOM_MODE) 
			{
				DBG_MUTEX_LOCK(&system_mutex);		
				if (ret4 < 2) cZoom = (char)ret4; 
				else 
				{
					if (cZoom == 0) cZoom = 1; else cZoom = 0;
				}
				cZoomSet = cZoom;
				DBG_MUTEX_UNLOCK(&system_mutex);	
				CreateScreenModel(state, sW,sH, 1, cZoomSet, 1);
			}
			if (ret == SYSTEM_CMD_RADIO_OFF)
			{
				if (!(GetCurShowType() & SHOW_TYPE_RADIO_STREAM))
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);							
					ret = ModuleTypeToNum(MODULE_TYPE_SPEAKER, 1);
					ret3 = ModuleTypeToNum(MODULE_TYPE_RADIO, 1);
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					if ((ret != -1) && (ret3 != -1))
					{
						RDA5807M_setMute(1);
						DBG_MUTEX_LOCK(&modulelist_mutex);
						MODULE_INFO* SpkMod = ModuleTypeToPoint(MODULE_TYPE_SPEAKER, 1);
						if ((SpkMod) && (SpkMod->SubModule >= 0)) gpio_switch_off_module(&miModuleList[SpkMod->SubModule]);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);	
					}
					ret = 0;
				}	
			}
			if (ret == SYSTEM_CMD_STREAM_OFF)
			{
				if (GetCurShowType() & SHOW_TYPE_WEB_STREAM)	
				{
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NEW);
					pScreenMenu[0].Options[20].Show = 0;				
				}	
			}
			if (ret == SYSTEM_CMD_SHOW_STOP_VIDEO) Menu_CloseVideo(NULL, -1);
			if (ret == SYSTEM_CMD_SHOW_STOP_AUDIO) Menu_CloseAudio(NULL, -1);
			if (ret == SYSTEM_CMD_SET_SHOW_MODE)
			{
				DBG_MUTEX_LOCK(&system_mutex);
				iForceShowStatus = ret4;
				DBG_MUTEX_UNLOCK(&system_mutex);
			}
			if (ret == SYSTEM_CMD_SET_SHOW_MODE)
			{
				if (uiSMode != ret4) 
				{
					uiSMode = ret4;
					SetShowMode(ret4);		
				}
			}
			if (ret == SYSTEM_CMD_SHOW_PAUSE) Media_Pause();
			if (ret == SYSTEM_CMD_SHOW_PLAY) Media_Play();			
			if (ret == SYSTEM_CMD_SHOW_INFO) iShowFileNameStatus = 1;
			if (ret == SYSTEM_CMD_SHOW_FORWARD) 
			{
				if (Media_Forward(ret4))
				{
					uiTimerShowPlaySlide = 0;
					SetShowMenu(0);
				}
			}
			if (ret == SYSTEM_CMD_SHOW_BACKWARD)
			{
				if (Media_Backward(ret4))
				{
					SetShowMenu(0);
					uiTimerShowPlaySlide = 0;
				}
			}
			if (ret == SYSTEM_CMD_PLAY_NEW_POS) 
			{
				if (Media_SetNewPos(ret4))
				{
					SetShowMenu(0);
					uiTimerShowPlaySlide = 0;
				}
			}
			
			if ((iCameraMenuStatus == 0) && (GetShowMenu() == 0))
			{
				if ((ret == SYSTEM_CMD_MENU_KEY_MENU) && (uiTimerShowPlaySlide < SHOW_PLAYSLIDE_TIME)) 
				{
					uiTimerShowPlaySlide = SHOW_PLAYSLIDE_TIME;
					ret = SYSTEM_CMD_NULL;
				}
				if ((ret == SYSTEM_CMD_MENU_KEY_OK) && (uiTimerShowPlaySlide < SHOW_PLAYSLIDE_TIME))
				{
					Media_SetNewPos(iNewPosSlide);
					uiTimerShowPlaySlide = 0;	
					ret = SYSTEM_CMD_NULL;
				}
				if ((ret == SYSTEM_CMD_MENU_FORWARD) || (ret == SYSTEM_CMD_MENU_BACKWARD))
				{
					if (uiTimerShowPlaySlide >= SHOW_PLAYSLIDE_TIME)
					{
						GetPlayBufferStatus(&uiMiscData[0], &uiMiscData[1], &uiMiscData[2]);
						if ((int)uiMiscData[2] != -1) 
						{
							iNewPosSlide = uiMiscData[2];
							uiTimerShowPlaySlide = 0;
						}
					}
					else
					{
						if (ret == SYSTEM_CMD_MENU_FORWARD)
						{
							iNewPosSlide += ret4;
							if (iNewPosSlide > 995) iNewPosSlide = 995;
							uiTimerShowPlaySlide = 0;	
						}
						if (ret == SYSTEM_CMD_MENU_BACKWARD)
						{
							iNewPosSlide -= ret4;
							if (iNewPosSlide < 0) iNewPosSlide = 0;
							uiTimerShowPlaySlide = 0;		
						}
					}
				}
			}
			if (ret == SYSTEM_CMD_SHOW_FULL_NEXT) 
			{
				if (GetChangeShowNow() == 0) 
				{						
					SetNewShowType(SHOW_TYPE_NEW);
					cPlayDirect = 5;
					SetChangeShowNow(1);
					pScreenMenu[0].Options[20].Show = 0;
				}
			}			
			if (ret == SYSTEM_CMD_WIDGET_STATUS_OFF) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
				{
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0)) 
					{							
						if ((wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) && (wiWidgetList[n].Status != 0))
							ReleaseWidget(&wiWidgetList[n]);
						wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
						wiWidgetList[n].ShowMode = 0;
						UpdateWidgets(state, 0);						
					}
				}
				DBG_MUTEX_UNLOCK(&widget_mutex);	
			}
			if (ret == SYSTEM_CMD_WIDGET_STATUS_ALLWAYS) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
				{
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0)) 
					{
						if ((wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) && (wiWidgetList[n].Status == 0))
							CreateWidget(&wiWidgetList[n]);
						wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
						wiWidgetList[n].ShowMode |= WIDGET_SHOWMODE_ALLWAYS;
						UpdateWidgets(state, 0);
					}
				}
				DBG_MUTEX_UNLOCK(&widget_mutex);	
			}
			if (ret == SYSTEM_CMD_WIDGET_STATUS_MENU) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
				{
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0)) 
					{
						if ((wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) && (wiWidgetList[n].Status == 0))
							CreateWidget(&wiWidgetList[n]);
						wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
						wiWidgetList[n].ShowMode |= WIDGET_SHOWMODE_MENU;
						UpdateWidgets(state, 0);
					}
				}
				DBG_MUTEX_UNLOCK(&widget_mutex);	
			}
			if (ret == SYSTEM_CMD_WIDGET_STATUS_TIMEOUT) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
				{
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0)) 
					{
						if ((wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) && (wiWidgetList[n].Status == 0))
							CreateWidget(&wiWidgetList[n]);
						wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
						wiWidgetList[n].ShowMode |= WIDGET_SHOWMODE_TIMEOUT;
						UpdateWidgets(state, 0);
					}
				}
				DBG_MUTEX_UNLOCK(&widget_mutex);	
			}
			if (ret == SYSTEM_CMD_WIDGET_STATUS_DAYTIME) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
				{
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0)) 
					{
						if ((wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) && (wiWidgetList[n].Status == 0))
							CreateWidget(&wiWidgetList[n]);
						wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
						wiWidgetList[n].ShowMode |= WIDGET_SHOWMODE_DAYTIME;
						UpdateWidgets(state, 0);
					}
				}
				DBG_MUTEX_UNLOCK(&widget_mutex);	
			}			
			if (ret == SYSTEM_CMD_WIDGET_TIMEUP) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0))
						wiWidgetList[n].ShowTimer = wiWidgetList[n].ShowTime;
				DBG_MUTEX_UNLOCK(&widget_mutex);
			}
			if (ret == SYSTEM_CMD_WIDGET_TIMEDOWN) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0))
						wiWidgetList[n].ShowTimer = 0;
				DBG_MUTEX_UNLOCK(&widget_mutex);
			}
			if (ret == SYSTEM_CMD_WIDGET_UPDATE) 
			{
				DBG_MUTEX_LOCK(&widget_mutex);	
				for (n = 0; n < iWidgetsCnt; n++)
				{
					if (wiWidgetList[n].Enabled && (wiWidgetList[n].WidgetID == ret4) && (wiWidgetList[n].Status >= 0))
					{
						wiWidgetList[n].Timer = wiWidgetList[n].Refresh;
						UpdateWidgets(state, 1);
					}
				}
				DBG_MUTEX_UNLOCK(&widget_mutex);
			}
			if (ret == SYSTEM_CMD_PLAY_DIR) 
			{
				DBG_MUTEX_LOCK(&system_mutex);	
				n = UpdateListFiles(cNextPlayPath, UPDATE_LIST_SCANTYPE_CUR);
				DBG_MUTEX_UNLOCK(&system_mutex);	
				if (n) SetChangeShowNow(1); 
				else 
				{
					dbgprintf(3, "No files in Shower3 '%s'\n", cNextPlayPath);
					DBG_MUTEX_LOCK(&system_mutex);
					UpdateListFiles(NULL, UPDATE_LIST_SCANTYPE_FULL);
					DBG_MUTEX_UNLOCK(&system_mutex);
					ShowNewMessage("Нет файлов для воспроизведения");					
				}
			}
			if (ret == SYSTEM_CMD_PLAY_FILE) 
			{
				DBG_MUTEX_LOCK(&system_mutex);	
				memcpy(cCurrentFile, cNextPlayPath, 256);
				DBG_MUTEX_UNLOCK(&system_mutex);	
				SetNewShowType(SHOW_TYPE_FILE);
				SetChangeShowNow(1);
			}
			if (ret == SYSTEM_CMD_PLAY_YOUTUBE_JWZ) 
			{
				SetNewShowType(SHOW_TYPE_FILE);
				SetChangeShowNow(1);
				cPlayDirect = 4;
			}			
			if (ret == SYSTEM_CMD_PLAY_DIR_RND_FILE) 
			{
				DBG_MUTEX_LOCK(&system_mutex);	
				n = UpdateListFiles(cNextPlayPath, UPDATE_LIST_SCANTYPE_CUR);
				DBG_MUTEX_UNLOCK(&system_mutex);
				int iFileNum = 0;
				int iFileOrderCnt = 0;				
				if (n)
				{
					GetNextFile(cCurrentFile, 256, 1, &iFileNum, &iFileOrderCnt); 
					AcceptFileInList(iFileNum, iFileOrderCnt);									
				}
				else 
				{
					dbgprintf(3, "No files in Shower4 '%s'\n", cNextPlayPath);
					ShowNewMessage("Нет файлов для воспроизведения");
				}
				i = 0;
				DBG_MUTEX_LOCK(&system_mutex);	
				if (UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_FULL) == 0)
				{
					if (uiShowModeCur >= 2) 
					{
						uiSMode = 1;
						uiShowModeCur = 1;
					}
					dbgprintf(3, "No files in Shower4 '%s'\n", cCurrentFileLocation);
					i = 1;
				}
				DBG_MUTEX_UNLOCK(&system_mutex);
				if (i) ShowNewMessage("Нет файлов для воспроизведения");
				if (n) 
				{
					SetNewShowType(SHOW_TYPE_FILE);
					SetChangeShowNow(1);
				}
			}
			if (ret == SYSTEM_CMD_SET_SHOW_PATH)
			{
				Menu_PlaySubDirectories(NULL, 0);
			}
			if (ret == SYSTEM_CMD_SET_DIRLIST_POS)
			{
				i = 0;
				DBG_MUTEX_LOCK(&system_mutex);
				if ((ret4 >= 0) && (ret4 < iListDirsCount)) iListDirsCurPos = ret4;
				int ret = UpdateListFiles(cCurrentFileLocation, UPDATE_LIST_SCANTYPE_CHANGE);
				if (ret == 0) 
				{		
					if (uiShowModeCur >= 2) uiShowModeCur = 1;
					dbgprintf(3, "No files in Menu_PlaySelectedDir '%s'\n", cCurrentFileLocation);
					i = 1;
				}
				else
				{
					if (uiShowModeCur < 2) uiShowModeCur = 2;		
				}
				DBG_MUTEX_UNLOCK(&system_mutex);
				if (i)
				{
					SetChangeShowNow(1);
					SetNewShowType(SHOW_TYPE_NEW);
					cPlayDirect = 3;
					pScreenMenu[0].Options[20].Show = 0;
					ShowNewMessage("Нет файлов для воспроизведения");					
				}
			}
			if (ret == SYSTEM_CMD_SET_PLAYLIST_POS)
			{
				DBG_MUTEX_LOCK(&system_mutex);
				if ((ret4 >= 0) && (ret4 < iListFilesCount)) iListFilesCurPos = ret4;
				DBG_MUTEX_UNLOCK(&system_mutex);
				SetChangeShowNow(1);
				cPlayDirect = 3;
				pScreenMenu[0].Options[20].Show = 0;
				if (uiShowModeCur < 2) uiShowModeCur = 2;
			}
			if (ret == SYSTEM_CMD_SHOW_MENU) 
			{
				if ((ret4 > 0) && (ret4 < MAX_MENU_PAGES))
				{
					SetShowMenu(1);
					iCurrentMenuPage = ret4 - 1;
					pScreenMenu[iCurrentMenuPage].SelectedOption = 0;	
					if (pScreenMenu[iCurrentMenuPage].OpenFunc != NULL) pScreenMenu[iCurrentMenuPage].OpenFunc(pScreenMenu, iCurrentMenuPage);
					if (pScreenMenu[iCurrentMenuPage].CountOptions == 0) iCurrentMenuPage = 0;
					DBG_MUTEX_LOCK(&system_mutex);						
					iSysStauses[8] = get_used_mem_gpu_mb();
					DBG_MUTEX_UNLOCK(&system_mutex);
					GetDateTimeStr(cDateStr, 128);										
				}
			}
			if (ret == SYSTEM_CMD_HIDE_MENU) SetShowMenu(0);
						
			if (((cKeyStatus == 0) && (ret == SYSTEM_CMD_MENU_KEY_ON)) || 
				((iCameraMenuStatus == 0) && ((ret == SYSTEM_CMD_MENU_KEY_MENU) || (ret == SYSTEM_CMD_MENU_KEY_OK))))
			{	
				if (ret2 & ALARM_TYPE_CLOCK)
				{
					if (iChangeVolumeSoft) iChangeVolumeSoft = 0;
					iCurrentVolume -= iAlrmVol / 3;
					if (iCurrentVolume < 5) 
					{
						iCurrentVolume = 0; 
						ClockAlarmStop(1);
					}
					audio_set_playback_volume(iCurrentVolume);															
				}
				else
				{
					if (GetShowMenu() == 0)
					{
						SetShowMenu(1);
						iCurrentMenuPage = 0;
						pScreenMenu[iCurrentMenuPage].SelectedOption = 0;
						cMenuBackClk = 0;
						cMenuForwClk = 0;
						cKeyUpClock = 0;
						cKeyDownClock = 0;
						if (ret == SYSTEM_CMD_MENU_KEY_OK) ret = SYSTEM_CMD_MENU_KEY_MENU;
						DBG_MUTEX_LOCK(&system_mutex);						
						iSysStauses[8] = get_used_mem_gpu_mb();
						DBG_MUTEX_UNLOCK(&system_mutex);		
						GetDateTimeStr(cDateStr, 128);
					}
					else
					{
						if (ret == SYSTEM_CMD_MENU_KEY_MENU) SetShowMenu(0);						
					}
				}
				cKeyStatus = 1;
				cKeyDownClock = 0;	
				cKeyUpClock = 0; 				
			}
			if ((cKeyStatus == 1) && (ret == SYSTEM_CMD_MENU_KEY_OFF)) 
			{
				cKeyStatus = 0;
				cKeyUpClock = 0;				
			}
			
			DBG_MUTEX_LOCK(&system_mutex);	
			cLoop = cThreadShowerStatus;	
			iMiscData[0] = iAlarmEvents;
			iMiscData[1] = iCurrentVolume;
			iMiscData[2] = iAccessLevel;
			iMiscData[3] = iMessagesListCnt;
			iMiscData[4] = cShowMenu;
			iMiscData[5] = iDefAccessLevel;
			iMiscData[6] = cCurShowType;
			iMiscData[7] = cStatusMessage[0];
			if (cDisplayMessageChanged)
			{
				memcpy(cDyspMessText, cDisplayMessageText, 127);
				cDisplayMessageChanged = 0;
			}
			if (cEventMessageChanged)
			{
				memcpy(cEventMessText, cEventMessageText, 127);
				cEventMessageChanged = 0;
				iEventMessStatus = 1;
			}
			DBG_MUTEX_UNLOCK(&system_mutex);
		
			iAccessLevelCopy = iMiscData[2];
			if (iMiscData[4] == 1) RenderMenu(pScreenMenu, state, cKeyStatus, ret, fMSize);
				
			if ((ret != SYSTEM_CMD_NULL) && (ret != SYSTEM_CMD_MENU_KEY_OFF) && (ret != SYSTEM_CMD_MENU_KEY_ON))
			{
				iMenuKeyCode = SYSTEM_CMD_NULL;
			}
			
			if ((iMiscData[0] & ALARM_TYPE_CLOCK) && (iTimer < 15)) RenderText(state, 36*fMSize, 10*fMSize, 10*fMSize, 1,"Будильник громкость: %i", iMiscData[1]);
			if ((iAccessLevelCopy != iMiscData[5]) && (iTimer < 15)) RenderText(state, 36*fMSize, 10*fMSize, 50*fMSize, 1,"Уровень доступа: %i", iMiscData[2]);			
			
			if (iTimer < 15)
			{
				if (iMiscData[3] != 0) RenderText(state, 36*fMSize, 10*fMSize, 80*fMSize, 1,"Сообщений: %i", iMiscData[3]);
				else
				{
					if (cDyspMessText[0]) RenderText(state, 36*fMSize, 10*fMSize, 80*fMSize, 1, cDyspMessText);
				}
			}
			
			if ((iMiscData[7] != 0) && (iTimer < 15)) 
			{
				DBG_MUTEX_LOCK(&system_mutex);
				RenderText(state, 36*fMSize, 10*fMSize, 140*fMSize, 1, cStatusMessage);
				DBG_MUTEX_UNLOCK(&system_mutex);
			}
			
			if ((iAccessLevelCopy != 0) && (cIncConn & 2) && (iTimer < 15)) RenderText(state, 36*fMSize, 300*fMSize, 80*fMSize, 1,"Камера подключена");			
			if ((iAccessLevelCopy != 0) && (cIncConn & 1) && (iTimer >= 15)) RenderText(state, 36*fMSize, 550*fMSize, 80*fMSize, 1,"Микрофон подключен");			
			if ((iMiscData[4] == 0) && (iMiscData[6] & SHOW_TYPE_CAMERA))
			{
				int iPan = 0;
				int iTilt = 0;
				int iZoom = 0;
				int iFocus = 0;
				int iPtzEn = 0;
				DBG_MUTEX_LOCK(&modulelist_mutex);
				n = ModuleTypeToNum(MODULE_TYPE_DISPLAY, 1);
				if (n >= 0)
				{
					if (miModuleList[n].Status[1])
					{
						n = ModuleIdToNum(miModuleList[n].Status[1], 2);
						if (n >= 0)
						{
							iPtzEn = 1;
							iPan = miModuleList[n].Status[5] / 1000;
							iTilt = miModuleList[n].Status[6] / 1000;
							iZoom = miModuleList[n].Status[8] / 1000;
							iFocus = miModuleList[n].Status[7] / 1000;
						}
					}
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
								 
				if (iPtzEn) RenderText(state, 26*fMSize, 0*fMSize, 170*fMSize, 0,"PAN:%i  TILT:%i  ZOOM:%i FOCUS:%i", iPan, iTilt, iZoom, iFocus);	
			}
			if ((iMiscData[4] == 1) && (iAccessLevelCopy != 0))
			{
				if (iPrevShowMenu == 1)
				{
					GetTransferDataCnt(&uiMiscData[0], &uiMiscData[1]);
					GetConnBufferStatus(&uiMiscData[3],&uiMiscData[4]);				
					RenderText(state, 26*fMSize, 333*fMSize, 170*fMSize, 0,">LAN> V A CPU CRAM GRAM Напр. Темп.");		
					///LAN mem buffer///
					RenderGraph(state, 345*fMSize, 300*fMSize, 60.0f*fMSize, 1.0f*fMSize);
					RenderGraph(state, 347*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(uiMiscData[0]/1048576)/((float)iSysStauses[9]/100)*fMSize);
					RenderGraph(state, 362*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(uiMiscData[3]/1048576)/((float)iSysStauses[9]/100)*fMSize);
					RenderGraph(state, 377*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(uiMiscData[4]/1048576)/((float)iSysStauses[9]/100)*fMSize);
					RenderGraph(state, 392*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(uiMiscData[1]/1048576)/((float)iSysStauses[9]/100)*fMSize);					
					///AV mem buffer///
					GetPlayBufferStatus(&uiMiscData[0], &uiMiscData[1], &uiMiscData[2]);				
					RenderGraph(state, 430*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(uiMiscData[0]/1048576)/((float)iSysStauses[9]/100)*fMSize);
					RenderGraph(state, 450*fMSize, 200*fMSize, 2.0f*fMSize, 1.0f*fMSize + (float)(100)*fMSize);
					RenderGraph(state, 428*fMSize, 300*fMSize, 45.0f*fMSize, 1.0f*fMSize);
					RenderGraph(state, 461*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(uiMiscData[1]/1048576)/((float)iSysStauses[9]/100)*fMSize);
					///CPU load///
					RenderGraph(state, 497*fMSize, 200*fMSize, 2.0f*fMSize, 1.0f*fMSize + (float)(100)*fMSize);
					RenderGraph(state, 497*fMSize, 300*fMSize, 35.0f*fMSize, 1.0f*fMSize);
					RenderGraph(state, 504*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(iSysStauses[2] / 10)*fMSize);
					RenderGraph(state, 519*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + (float)(iSysStauses[3] / 10)*fMSize);
					//CPU RAM///
					//RenderGraph(state, 477*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + iSysStauses[4]*fMSize);
					RenderGraph(state, 565*fMSize, 200*fMSize, 2.0f*fMSize, 1.0f*fMSize + 100*fMSize);
					RenderGraph(state, 565*fMSize, 300*fMSize, 35.0f*fMSize, 1.0f*fMSize);
					RenderGraph(state, 572*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + iSysStauses[5]/((float)iSysStauses[9]/100)*fMSize);
					//RenderGraph(state, 507*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + iSysStauses[6]*fMSize);
					RenderGraph(state, 587*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + DBG_USE_MEM()/1000000/((float)iSysStauses[9]/100)*fMSize);
					///GPU RAM///					
					RenderGraph(state, 653*fMSize, 200*fMSize, 2.0f*fMSize, 1.0f*fMSize + 100*fMSize);
					RenderGraph(state, 653*fMSize, 300*fMSize, 25.0f*fMSize, 1.0f*fMSize);
					RenderGraph(state, 660*fMSize, 200*fMSize, 10.0f*fMSize, 1.0f*fMSize + iSysStauses[8]/((float)iSysStauses[9]/100)*fMSize);
					///Volt///
					RenderText(state, 26*fMSize, 710*fMSize, 200*fMSize, 0,"%.1fV", (float)(iSysStauses[10]) / 1000);
					///Temp///
					RenderText(state, 26*fMSize, 800*fMSize, 200*fMSize, 0,"%.1f`C", (float)(iSysStauses[0] / 10));							
				}
			}
			
			if ((iMiscData[4] == 1) || (uiTimerShowVolume < SHOW_VOLUME_TIME))
			{
				RenderText(state, 26*fMSize, 10*fMSize, 170*fMSize, 0,"Громк.");		
				RenderGraph(state, 15*fMSize, 200*fMSize, 30.0f*fMSize, iCurrentVolume*fMSize);
				RenderGraph(state, 50*fMSize, 200*fMSize, 10.0f*fMSize, 100.0f*fMSize);
				RenderGraph(state, 65*fMSize, 200*fMSize, 30.0f*fMSize, iAlrmVol*fMSize);
			}
			if ((iMiscData[4] == 1) || (uiTimerShowTimer < SHOW_TIMER_TIME))
			{
				if (iMiscData[6] & (SHOW_TYPE_CAMERA | SHOW_TYPE_CAMERA_FILE | SHOW_TYPE_CAMERA_LIST | SHOW_TYPE_MIC_STREAM | SHOW_TYPE_MIC_FILE))
				{
					RenderText(state, 26*fMSize, 110*fMSize, 170*fMSize, 0,"Таймер камеры");	
					RenderText(state, 90*fMSize, 140*fMSize, 180*fMSize, 0,"%.1f", (float)iCamTimeMax / 60);
				}
				else
				if (iMiscData[6] & (SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM))
				{
					RenderText(state, 26*fMSize, 110*fMSize, 170*fMSize, 0,"Таймер потока");	
					RenderText(state, 90*fMSize, 140*fMSize, 180*fMSize, 0,"%.1f", (float)iRadioTimeMax / 60);
				}
				else
				{
					RenderText(state, 26*fMSize, 110*fMSize, 170*fMSize, 0,"Таймер показа");	
					RenderText(state, 90*fMSize, 140*fMSize, 180*fMSize, 0,"%.1f", (float)iSlideShowTimer / 60);
				}
			}
			if (uiTimerShowPlaySlide < SHOW_PLAYSLIDE_TIME)
			{
				GetPlayBufferStatus(&uiMiscData[0], &uiMiscData[1], &uiMiscData[2]);
				if ((int)uiMiscData[2] >= 0)
				{
					RenderGraph(state, 50*fMSize, 							30.0f*fMSize, 	505.0f*fMSize, 					3.0f*fMSize);
					RenderGraph(state, 50*fMSize, 							0.0f, 			505.0f*fMSize, 					3.0f*fMSize);
					RenderGraph(state, 50*fMSize, 							0.0f, 			5.0f*fMSize, 					30.0f*fMSize);
					RenderGraph(state, 550*fMSize, 							0.0f, 			5.0f*fMSize, 					30.0f*fMSize);
					if (iNewPosSlide) 
						RenderGraph(state, (50 + (float)iNewPosSlide/2)*fMSize, 5.0f*fMSize, 	10.0f*fMSize, 					23.0f*fMSize);
					RenderGraph(state, 55*fMSize, 							11.0f*fMSize, 	(float)uiMiscData[2]/2*fMSize, 	10.0f*fMSize);
				} else uiTimerShowPlaySlide = SHOW_PLAYSLIDE_TIME;				
			}
			if (iMiscData[4] == 1)
			{
				RenderText(state, 26*fMSize, 0, 140*fMSize, 0,"Статус:%s, Порядок Папки:%s, Файлы:%s",
							cShowModeStr,
							cRandomDir ? "?" : "+1",
							cRandomFile ? "?" : "+1");
				RenderText(state, 26*fMSize, 0, 115*fMSize, 0,"Таймер:%i/%i, %i/%i  Дата:%s",
							iTimerToNextShow, GetShowTimeMax(), iRadioTimerOff, iRadioTimeMax, cDateStr);
			}
			if (iShowFileNameStatus > 1)
			{
				RenderText(state, 54*fMSize, 0, iShowFileNamePos, 0, "%s", cShowFileNameText);
				//printf("ggg pos %i %i '%s'\n", iShowFileNamePos, state->menu_height - iShowFileNamePos + (int)(50*fMSize), cShowDirNameText);
				if ((!(iMiscData[6] & (SHOW_TYPE_RADIO_STREAM | SHOW_TYPE_WEB_STREAM)))
					&&
					(!(iMiscData[0] & ALARM_TYPE_CLOCK)))
					RenderText(state, 54*fMSize, 0, state->menu_height - iShowFileNamePos - 50*fMSize, 0,"%s", cShowDirNameText);
			}
			if (iEventMessStatus > 1)
			{
				RenderText(state, 36*fMSize, 0, iEventMessPos, 0,"%s", cEventMessText);
			}
			
			if (state->menu_render_count) RenderMenuDone(state);
			if ((!state->menu_render_count) && 
				(iMiscData[4] == 0) && 
				(iMiscData[3] == 0) && 
				(iMiscData[7] == 0) &&
				(iShowFileNameStatus == 0) &&
				(iEventMessStatus == 0) &&
				(uiTimerShowPlaySlide >= SHOW_PLAYSLIDE_TIME) &&
				(!(iMiscData[0] & ALARM_TYPE_CLOCK)) &&
				(iAccessLevelCopy == iMiscData[5]) &&
				(cDyspMessText[0] == 0)) 
				RenderDisplayContent(state, iTimer < 15);
			
			glFinish();
			glFlush();
			
			if ((iVertSync != 1) || ((GetCurShowType() & SHOW_TYPE_VIDEO) == 0))
			{
				diff_ms = (unsigned int)get_ms(&previous_ms);
				if (diff_ms < 40)
				{
					//printf("render: %i,\twait: %i\n", diff_ms, 40-diff_ms);
					usleep((40 - diff_ms)*1000); 	
				}
				//else printf("long render : %i\n", diff_ms);
			}
			
			RenderDone(state);	
			iPrevShowMenu = iMiscData[4];
	}
	
	DBG_FREE(clCameraListCur);
	DBG_FREE(clCameraListNew);
	DisplayContentListClear();
	DisplayContentListSwitch();	
	
	dbgprintf(5,"DeInit Widgets\n");
	DBG_MUTEX_LOCK(&widget_mutex);
	for (n = 0; n < iWidgetsCnt; n++)
	{		
		if ((wiWidgetList[n].Type > WIDGET_TYPE_SENSOR_MIN) || 
			(wiWidgetList[n].Type < WIDGET_TYPE_SENSOR_MAX)) ReleaseWidget(&wiWidgetList[n]);
		if (wiWidgetList[n].Type == WIDGET_TYPE_WEATHER) DeInitGLWeather();		
		if ((wiWidgetList[n].Type > WIDGET_TYPE_CLOCK_MIN) || 
			(wiWidgetList[n].Type < WIDGET_TYPE_CLOCK_MAX)) ReleaseWidget(&wiWidgetList[n]);	
		if (wiWidgetList[n].Type == WIDGET_TYPE_IMAGE) 
		{
			if (wiWidgetList[n].ShowMode != 0) ReleaseWidget(&wiWidgetList[n]);
		}
	}
	DBG_MUTEX_UNLOCK(&widget_mutex);	
	
	DeInit_GL(state);
	
	DBG_MUTEX_LOCK(&system_mutex);	
	cThreadShowerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);	
		
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());
	return 0;
}

int main(int argc, char *argv[])
{	
	if (GetProcessNameCount("azad") > 1)
	{
		printf("azad process executing. exit.\n");
		return -1;
	}
	
	int iResetFlag = 0;
	int iDebugFlag = 0;	
	int iLogFlag = -1;	
	int iSafeFlag = 0;	
	int iNoDieFlag = 0;	
	int iPauseFlag = 0;	
	
	int y;
	int reslt = 0;
	for (y = 1; y < argc; y++) 
	{
		if (strcasecmp(argv[y], "pause") == 0) {iPauseFlag = 1; continue;}
		if (strcasecmp(argv[y], "nodie") == 0) {iNoDieFlag = 1; continue;}
		if (strcasecmp(argv[y], "safe") == 0) {iSafeFlag = 1; continue;}
		if (strcasecmp(argv[y], "reset") == 0) {iResetFlag = 1; continue;}
		if (strcasecmp(argv[y], "debug") == 0) {iDebugFlag = 1; continue;}
		if ((strcasecmp(argv[y], "log") == 0) && ((y + 1) < argc))
		{
			y++;
			iLogFlag = Str2Int(argv[y]);
			continue;
		}
		printf("Unknown attr: %s\n", argv[y]);
		reslt = 1;
	}
	if (reslt) 
	{
		printf("\tsupport only:nodie, safe, reset, debug, log [num]\n");
		exit(0);
	}
	
	if (iPauseFlag)
	{
		printf("\nPause before start AZAD\n");
		usleep(3000000);
	}
	
	bcm_host_init();
	
	//az_bcm_host_init();	
	//setlocale(LC_ALL, 0);
	emergency_stop();
	
	
	//char * cMyPath = get_my_path();
	//printf("%s\n", cMyPath);	
	GetIntVersion(version);
	
	uiStartMode = 0;
	if (iResetFlag) uiStartMode |= 0x0001;
	if (iDebugFlag) uiStartMode |= 0x0002;
	if (iSafeFlag) uiStartMode |= 0x0004;
	if (iNoDieFlag) uiStartMode |= 0x0008;
	if (iPauseFlag) uiStartMode |= 0x0010;
	
	//printf("\n@@@ %i %i %i %i\n", iResetFlag, iDebugFlag, iSafeFlag, iNoDieFlag);
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);	
	localtime_r(&rawtime, &timeinfo);
	memset(cStartTime, 0, 64);
	strftime(cStartTime, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
	
	iFileLog = 0;
	iScreenLog = 0;
	iMessageLog = 0;
	iLocalMessageLog = 0;
	iEmailLog = 0;
	uiLogEmailPauseSize = 300000;
	memset(cLogIP, 0, 256);
	memset(&cLogMlList, 0, sizeof(MAIL_INFO));
	iLogEmailTimer = 0;
	get_ms(&iLogEmailTimer);
	
	iSoundMessageTimer = 0;
	get_ms(&iSoundMessageTimer);
	uiSoundMessagePauseSize = 10000;
	uiSoundMessageID = 0;
	DBG_INIT(0, 100);
	if (iDebugFlag)
	{
		//DBG_ON(DBG_MODE_MEMORY_ERROR | DBG_MODE_DBG);DBG_MODE_TRACK_FUNC | DBG_MODE_TRACK_FUNC_THREAD
		//DBG_MODE_MEMORY_INFO
		DBG_ON(DBG_MODE_LOG_FUNC | DBG_MODE_MEMORY_ERROR | DBG_MODE_TRACK_LOCKED_MUTEX | DBG_MODE_LOG_MEM | DBG_MODE_TRACK_LONG_MUTEX); // | DBG_MODE_TRACK_LOCKED_MUTEX);
		//DBG_ON(DBG_MODE_LOG_FUNC | DBG_MODE_MEMORY_ERROR | DBG_MODE_TRACK_LOCKED_MUTEX | DBG_MODE_TRACK_FUNC_THREAD); // | DBG_MODE_TRACK_LOCKED_MUTEX);
	}
	DBG_LOG_IN();
	int n, i;
	unsigned int ret;
	char bb[256];
	unsigned int uiSystemID = 0;
	
	ucTimeUpdated = 0;
	iMessageListChanged = 0;
	iMessagesListCnt = 0;
	cMessagesList = NULL;
	uiSendDataCnt = 0;
	iSystemListCnt = 1;
	miSystemList = (SYSTEM_INFO*)DBG_MALLOC(sizeof(SYSTEM_INFO));
	memset(miSystemList, 0, sizeof(SYSTEM_INFO));
	miSystemList[0].Local = 1;
	memset(miSystemList[0].Version, 0, 64);
	if (strlen(VERSION) < 64) memcpy(miSystemList[0].Version, VERSION, strlen(VERSION));
		else memcpy(miSystemList[0].Version, VERSION, 63);
	miSystemList[0].IntVersion[0] = version[0];
	miSystemList[0].IntVersion[1] = version[1];
	miSystemList[0].IntVersion[2] = version[2];
	miSystemList[0].IntVersion[3] = version[3];
	
	iDefAccessLevel = DEFAULT_ACCESS_LEVEL;
	iAccessLevel = iDefAccessLevel;
	
	InitSettings();	
	
	printf("VERSION: %s\n", VERSION);
	
	total_cpu_load_init();
	my_cpu_load_init();
	
	if (iResetFlag)
	{
		printf("Set default config files\n");
		iScreenLog = 3;		
		SaveSettings();
		SaveRadios();
		SaveUsers();
		SaveDirectories();
		SaveCamRectangles();
		SaveMnlActions();
		SaveEvntActions();
		SaveIrCodes();
		SaveKeys();
		SaveWidgets();
		SaveStreamTypes();
		SaveStreams();
		SavePTZs();
		SaveMails();
		SaveSounds();
		SaveAlarms();
		SaveModules();
	}
	
	FillConfigPath(bb, 256, cConfigFile, 0);	
	printf("Loading config file: %s\n", bb);		
	if (LoadSettings(bb) == 0)
	{
		printf("Error load config file: %s\n", bb);	
		SaveSettings();
		//exit(0);
	}
	
	if (iLogFlag >= 0) iScreenLog = iLogFlag;
	
	if (strlen(cRadioFile))
	{
		FillConfigPath(bb, 256, cRadioFile, 0);	
		if (LoadRadios(bb) == 0) 
		{
			dbgprintf(1,"Error load RadioFile file: %s\n", bb);
			SaveRadios();
		}
	}
	if (strlen(cUserFile))
	{
		FillConfigPath(bb, 256, cUserFile, 0);	
		if (LoadUsers(bb) == 0) 
		{
			dbgprintf(1,"Error load UserFile file: %s\n", bb);
			SaveUsers();
		}
	}
	if (strlen(cDirectoryFile))
	{
		FillConfigPath(bb, 256, cDirectoryFile, 0);	
		if (LoadDirectories(bb) == 0) 
		{
			dbgprintf(1,"Error load DirectoryFile file: %s\n", bb);
			SaveDirectories();
		}
	}
	
	if (strlen(cCamRectangleFile))
	{
		FillConfigPath(bb, 256, cCamRectangleFile, 0);	
		if (LoadCamRectangles(bb) == 0) 
		{
			dbgprintf(1,"Error load CamRectangleFile file: %s\n", bb);
			SaveCamRectangles();
		}
	}
	if (strlen(cMnlActionFile))
	{
		FillConfigPath(bb, 256, cMnlActionFile, 0);	
		if (LoadMnlActions(bb) == 0)
		{
			dbgprintf(1,"Error load MnlActionFile file: %s\n", bb);
			SaveMnlActions();
		}
	}
	if (strlen(cEvntActionFile))
	{
		FillConfigPath(bb, 256, cEvntActionFile, 0);	
		if (LoadEvntActions(bb) == 0) 
		{
			dbgprintf(1,"Error load EvntActionFile file: %s\n", bb);
			SaveEvntActions();
		}
	}
	
	if (strlen(cIrCodeFile))
	{
		FillConfigPath(bb, 256, cIrCodeFile, 0);	
		if (LoadIrCodes(bb) == 0) 
		{
			dbgprintf(1,"Error load IrCodeFile file: %s\n", bb);
			SaveIrCodes();
		}
	}
	if (strlen(cKeyFile))
	{
		FillConfigPath(bb, 256, cKeyFile, 0);	
		if (LoadKeys(bb) == 0) 
		{
			dbgprintf(1,"Error load KeyFile file: %s\n", bb);
			SaveKeys();
		}
	}
	if (strlen(cWidgetFile))
	{
		FillConfigPath(bb, 256, cWidgetFile, 0);	
		if (LoadWidgets(bb) == 0) 
		{
			dbgprintf(1,"Error load WidgetFile file: %s\n", bb);
			SaveWidgets();
		}
	}
	
	if (strlen(cStreamTypeFile))
	{
		FillConfigPath(bb, 256, cStreamTypeFile, 0);	
		if (LoadStreamTypes(bb) == 0) 
		{
			dbgprintf(1,"Error load StreamTypeFile file: %s\n", bb);
			SaveStreamTypes();
		}
	}
	if (strlen(cStreamFile))
	{
		FillConfigPath(bb, 256, cStreamFile, 0);	
		if (LoadStreams(bb) == 0) 
		{
			dbgprintf(1,"Error load StreamFile file: %s\n", bb);
			SaveStreams();
		}
	}
	if (strlen(cPtzFile))
	{
		FillConfigPath(bb, 256, cPtzFile, 0);	
		if (LoadPTZs(bb) == 0) 
		{
			dbgprintf(1,"Error load PtzFile file: %s\n", bb);
			SavePTZs();
		}
	}
	if (strlen(cMailFile))
	{
		FillConfigPath(bb, 256, cMailFile, 0);	
		if (LoadMails(bb) == 0) 
		{
			dbgprintf(1,"Error load MailFile file: %s\n", bb);
			SaveMails();
		}
	}
	
	if (strlen(cSoundFile))
	{
		FillConfigPath(bb, 256, cSoundFile, 0);	
		if (LoadSounds(bb) == 0) 
		{
			dbgprintf(1,"Error load SoundFile file: %s\n", bb);
			SaveSounds();
		}
	}
	if (strlen(cAlarmFile))
	{
		FillConfigPath(bb, 256, cAlarmFile, 0);	
		if (LoadAlarms(bb) == 0) 
		{
			dbgprintf(1,"Error load AlarmFile file: %s\n", bb);
			SaveAlarms();
		}
	}
	if (strlen(cModuleFile))
	{
		FillConfigPath(bb, 256, cModuleFile, 0);	
		if (LoadModules(bb) == 0) 
		{
			dbgprintf(1,"Error load ModuleFile file: %s\n", bb);
			SaveModules();
		}
	}
	
	TestSettings(0);
	if (uiCaptureFileSizeFull < 10) uiCaptureFileSizeFull = 100;
	if (uiCaptureTimeLimitFull < 10) uiCaptureTimeLimitFull = 300;
	if (uiCaptureFileSizeSlow < 10) uiCaptureFileSizeSlow = 100;
	if (uiCaptureTimeLimitSlow < 10) uiCaptureTimeLimitSlow = 300;
	if (uiCaptureFileSizeDiff < 10) uiCaptureFileSizeDiff = 100;
	if (uiCaptureTimeLimitDiff < 10) uiCaptureTimeLimitDiff = 300;
	if (uiCaptureFileSizeAudio < 1) uiCaptureFileSizeAudio = 10;
	if (uiCaptureTimeLimitAudio < 10) uiCaptureTimeLimitAudio = 300;
	if (uiCaptureFileSizeStatuses < 1) uiCaptureFileSizeStatuses = 10;
	if (uiCaptureTimeLimitStatuses < 10) uiCaptureTimeLimitStatuses = 3000;
	if (uiCaptureFileSizeEvents < 1) uiCaptureFileSizeEvents = 10;
	if (uiCaptureTimeLimitEvents < 10) uiCaptureTimeLimitEvents = 3000;
	if (uiCaptureFileSizeActions < 1) uiCaptureFileSizeActions = 10;
	if (uiCaptureTimeLimitActions < 10) uiCaptureTimeLimitActions = 3000;
	
	memcpy(cLogMailAddress, cMailAddress, 64);
	memcpy(cLogMailServer, cMailServer, 64);
	memcpy(cLogMailLogin, cMailLogin, 64);
	memcpy(cLogMailPassword, cMailPassword, 64);
	memcpy(cLogMailAuth, cMailAuth, 64);
	
	uiShowModeCur = uiShowMode;
	
	if (iStreamTypeCnt == 0)
	{
		iStreamTypeCnt++;
		stStreamType = (STREAM_TYPE*)DBG_REALLOC(stStreamType, sizeof(STREAM_TYPE)*iStreamTypeCnt);
		memset(&stStreamType[iStreamTypeCnt-1], 0, sizeof(STREAM_TYPE));
		strcpy(stStreamType[iStreamTypeCnt-1].Name, "ALL");
	}
	for (n = 0; n < iInternetRadioCnt; n++) if (irInternetRadio[n].Type >= iStreamTypeCnt) irInternetRadio[n].Type = 0;
	
	
	//char cBackUpPath[MAX_PATH];
	//char cCapturePath[MAX_PATH];
	char cCapturePathFull[MAX_PATH];
	char cCapturePathSlow[MAX_PATH];
	char cCapturePathDiff[MAX_PATH];
	char cCapturePathAudio[MAX_PATH];
	char cCapturePathStatuses[MAX_PATH];
	char cCapturePathEvents[MAX_PATH];
	char cCapturePathActions[MAX_PATH];
	
	char cBackUpNormPath[MAX_PATH];	
	char cBackUpSlowPath[MAX_PATH];
	char cBackUpDiffPath[MAX_PATH];
	char cBackUpAudioPath[MAX_PATH];	
	char cBackUpStatusesPath[MAX_PATH];
	char cBackUpEventsPath[MAX_PATH];
	char cBackUpActionsPath[MAX_PATH];
	
	char cArchiveNormPath[MAX_PATH];	
	char cArchiveSlowPath[MAX_PATH];
	char cArchiveDiffPath[MAX_PATH];
	char cArchiveAudioPath[MAX_PATH];	
	char cArchiveStatusesPath[MAX_PATH];
	char cArchiveEventsPath[MAX_PATH];
	char cArchiveActionsPath[MAX_PATH];
	
	//memset(cBackUpPath, 0, MAX_PATH);
	//memset(cCapturePath, 0, MAX_PATH);
	
	memset(cBackUpNormPath, 0, MAX_PATH);
	memset(cBackUpSlowPath, 0, MAX_PATH);
	memset(cBackUpDiffPath, 0, MAX_PATH);
	memset(cBackUpAudioPath, 0, MAX_PATH);
	memset(cBackUpStatusesPath, 0, MAX_PATH);	
	memset(cBackUpEventsPath, 0, MAX_PATH);	
	memset(cBackUpActionsPath, 0, MAX_PATH);	
	
	memset(cCapturePathFull, 0, MAX_PATH);
	memset(cCapturePathSlow, 0, MAX_PATH);
	memset(cCapturePathDiff, 0, MAX_PATH);
	memset(cCapturePathAudio, 0, MAX_PATH);
	memset(cCapturePathStatuses, 0, MAX_PATH);
	memset(cCapturePathEvents, 0, MAX_PATH);
	memset(cCapturePathActions, 0, MAX_PATH);
	
	memset(cArchiveNormPath, 0, MAX_PATH);
	memset(cArchiveSlowPath, 0, MAX_PATH);
	memset(cArchiveDiffPath, 0, MAX_PATH);
	memset(cArchiveAudioPath, 0, MAX_PATH);
	memset(cArchiveStatusesPath, 0, MAX_PATH);
	memset(cArchiveEventsPath, 0, MAX_PATH);
	memset(cArchiveActionsPath, 0, MAX_PATH);
	
	if ((strlen(cMediaCapturePathFull) < (MAX_PATH - 1)) &&
		(cMediaCapturePathFull[strlen(cMediaCapturePathFull)-1] != 47))	cMediaCapturePathFull[strlen(cMediaCapturePathFull)] = 47;
	if ((strlen(cMediaCapturePathSlow) < (MAX_PATH - 1)) &&
		(cMediaCapturePathSlow[strlen(cMediaCapturePathSlow)-1] != 47))	cMediaCapturePathSlow[strlen(cMediaCapturePathSlow)] = 47;
	if ((strlen(cMediaCapturePathDiff) < (MAX_PATH - 1)) &&
		(cMediaCapturePathDiff[strlen(cMediaCapturePathDiff)-1] != 47))	cMediaCapturePathDiff[strlen(cMediaCapturePathDiff)] = 47;
	if ((strlen(cMediaCapturePathAudio) < (MAX_PATH - 1)) &&
		(cMediaCapturePathAudio[strlen(cMediaCapturePathAudio)-1] != 47)) cMediaCapturePathAudio[strlen(cMediaCapturePathAudio)] = 47;
	if ((strlen(cMediaCapturePathStatuses) < (MAX_PATH - 1)) &&
		(cMediaCapturePathStatuses[strlen(cMediaCapturePathStatuses)-1] != 47))	cMediaCapturePathStatuses[strlen(cMediaCapturePathStatuses)] = 47;
	if ((strlen(cMediaCapturePathEvents) < (MAX_PATH - 1)) &&
		(cMediaCapturePathEvents[strlen(cMediaCapturePathEvents)-1] != 47))	cMediaCapturePathEvents[strlen(cMediaCapturePathEvents)] = 47;
	if ((strlen(cMediaCapturePathActions) < (MAX_PATH - 1)) &&
		(cMediaCapturePathActions[strlen(cMediaCapturePathActions)-1] != 47)) cMediaCapturePathActions[strlen(cMediaCapturePathActions)] = 47;
	
	if ((strlen(cMediaBackUpPathFull) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathFull[strlen(cMediaBackUpPathFull)-1] != 47)) cMediaBackUpPathFull[strlen(cMediaBackUpPathFull)] = 47;
	if ((strlen(cMediaBackUpPathSlow) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathSlow[strlen(cMediaBackUpPathSlow)-1] != 47)) cMediaBackUpPathSlow[strlen(cMediaBackUpPathSlow)] = 47;
	if ((strlen(cMediaBackUpPathDiff) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathDiff[strlen(cMediaBackUpPathDiff)-1] != 47)) cMediaBackUpPathDiff[strlen(cMediaBackUpPathDiff)] = 47;
	if ((strlen(cMediaBackUpPathAudio) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathAudio[strlen(cMediaBackUpPathAudio)-1] != 47))	cMediaBackUpPathAudio[strlen(cMediaBackUpPathAudio)] = 47;
	if ((strlen(cMediaBackUpPathStatuses) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathStatuses[strlen(cMediaBackUpPathStatuses)-1] != 47)) cMediaBackUpPathStatuses[strlen(cMediaBackUpPathStatuses)] = 47;
	if ((strlen(cMediaBackUpPathEvents) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathEvents[strlen(cMediaBackUpPathEvents)-1] != 47)) cMediaBackUpPathEvents[strlen(cMediaBackUpPathEvents)] = 47;
	if ((strlen(cMediaBackUpPathActions) < (MAX_PATH - 1)) &&
		(cMediaBackUpPathActions[strlen(cMediaBackUpPathActions)-1] != 47))	cMediaBackUpPathActions[strlen(cMediaBackUpPathActions)] = 47;
		
	if ((strlen(cMediaArchivePathFull) < (MAX_PATH - 1)) &&
		(cMediaArchivePathFull[strlen(cMediaArchivePathFull)-1] != 47))	cMediaArchivePathFull[strlen(cMediaArchivePathFull)] = 47;
	if ((strlen(cMediaArchivePathSlow) < (MAX_PATH - 1)) &&
		(cMediaArchivePathSlow[strlen(cMediaArchivePathSlow)-1] != 47))	cMediaArchivePathSlow[strlen(cMediaArchivePathSlow)] = 47;
	if ((strlen(cMediaArchivePathDiff) < (MAX_PATH - 1)) &&
		(cMediaArchivePathDiff[strlen(cMediaArchivePathDiff)-1] != 47))	cMediaArchivePathDiff[strlen(cMediaArchivePathDiff)] = 47;
	if ((strlen(cMediaCapturePathAudio) < (MAX_PATH - 1)) &&
		(cMediaArchivePathAudio[strlen(cMediaArchivePathAudio)-1] != 47)) cMediaArchivePathAudio[strlen(cMediaArchivePathAudio)] = 47;
	if ((strlen(cMediaArchivePathStatuses) < (MAX_PATH - 1)) &&
		(cMediaArchivePathStatuses[strlen(cMediaArchivePathStatuses)-1] != 47))	cMediaArchivePathStatuses[strlen(cMediaArchivePathStatuses)] = 47;
	if ((strlen(cMediaArchivePathEvents) < (MAX_PATH - 1)) &&
		(cMediaArchivePathEvents[strlen(cMediaArchivePathEvents)-1] != 47))	cMediaArchivePathEvents[strlen(cMediaArchivePathEvents)] = 47;
	if ((strlen(cMediaArchivePathActions) < (MAX_PATH - 1)) &&
		(cMediaArchivePathActions[strlen(cMediaArchivePathActions)-1] != 47)) cMediaArchivePathActions[strlen(cMediaArchivePathActions)] = 47;
	
	if ((strlen(cLocalBackUpPathFull) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathFull[strlen(cLocalBackUpPathFull)-1] != 47)) cLocalBackUpPathFull[strlen(cLocalBackUpPathFull)] = 47;
	if ((strlen(cLocalBackUpPathSlow) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathSlow[strlen(cLocalBackUpPathSlow)-1] != 47)) cLocalBackUpPathSlow[strlen(cLocalBackUpPathSlow)] = 47;
	if ((strlen(cLocalBackUpPathDiff) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathDiff[strlen(cLocalBackUpPathDiff)-1] != 47)) cLocalBackUpPathDiff[strlen(cLocalBackUpPathDiff)] = 47;
	if ((strlen(cLocalBackUpPathAudio) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathAudio[strlen(cLocalBackUpPathAudio)-1] != 47))	cLocalBackUpPathAudio[strlen(cLocalBackUpPathAudio)] = 47;
	if ((strlen(cLocalBackUpPathStatuses) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathStatuses[strlen(cLocalBackUpPathStatuses)-1] != 47)) cLocalBackUpPathStatuses[strlen(cLocalBackUpPathStatuses)] = 47;
	if ((strlen(cLocalBackUpPathEvents) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathEvents[strlen(cLocalBackUpPathEvents)-1] != 47)) cLocalBackUpPathEvents[strlen(cLocalBackUpPathEvents)] = 47;
	if ((strlen(cLocalBackUpPathActions) < (MAX_PATH - 1)) &&
		(cLocalBackUpPathActions[strlen(cLocalBackUpPathActions)-1] != 47))	cLocalBackUpPathActions[strlen(cLocalBackUpPathActions)] = 47;
		
	if ((strlen(cLocalArchivePathFull) < (MAX_PATH - 1)) &&
		(cLocalArchivePathFull[strlen(cLocalArchivePathFull)-1] != 47))	cLocalArchivePathFull[strlen(cLocalArchivePathFull)] = 47;
	if ((strlen(cLocalArchivePathSlow) < (MAX_PATH - 1)) &&
		(cLocalArchivePathSlow[strlen(cLocalArchivePathSlow)-1] != 47))	cLocalArchivePathSlow[strlen(cLocalArchivePathSlow)] = 47;
	if ((strlen(cLocalArchivePathDiff) < (MAX_PATH - 1)) &&
		(cLocalArchivePathDiff[strlen(cLocalArchivePathDiff)-1] != 47))	cLocalArchivePathDiff[strlen(cLocalArchivePathDiff)] = 47;
	if ((strlen(cLocalArchivePathAudio) < (MAX_PATH - 1)) &&
		(cLocalArchivePathAudio[strlen(cLocalArchivePathAudio)-1] != 47)) cLocalArchivePathAudio[strlen(cLocalArchivePathAudio)] = 47;
	if ((strlen(cLocalArchivePathStatuses) < (MAX_PATH - 1)) &&
		(cLocalArchivePathStatuses[strlen(cLocalArchivePathStatuses)-1] != 47))	cLocalArchivePathStatuses[strlen(cLocalArchivePathStatuses)] = 47;
	if ((strlen(cLocalArchivePathEvents) < (MAX_PATH - 1)) &&
		(cLocalArchivePathEvents[strlen(cLocalArchivePathEvents)-1] != 47))	cLocalArchivePathEvents[strlen(cLocalArchivePathEvents)] = 47;
	if ((strlen(cLocalArchivePathActions) < (MAX_PATH - 1)) &&
		(cLocalArchivePathActions[strlen(cLocalArchivePathActions)-1] != 47)) cLocalArchivePathActions[strlen(cLocalArchivePathActions)] = 47;
	
	strcpy(cCapturePathFull, cMediaCapturePathFull);
	//strcat(cCapturePathFull, DIR_NORM);
	strcpy(cCapturePathSlow, cMediaCapturePathSlow);
	//strcat(cCapturePathSlow, DIR_SLOW);
	strcpy(cCapturePathDiff, cMediaCapturePathDiff);
	//strcat(cCapturePathDiff, DIR_DIFF);
	strcpy(cCapturePathAudio, cMediaCapturePathAudio);
	//strcat(cCapturePathAudio, DIR_AUD);
	strcpy(cCapturePathStatuses, cMediaCapturePathStatuses);
	//strcat(cCapturePathStatuses, DIR_STAT);
	strcpy(cCapturePathEvents, cMediaCapturePathEvents);
	//strcat(cCapturePathEvents, DIR_EVNT);
	strcpy(cCapturePathActions, cMediaCapturePathActions);
	//strcat(cCapturePathActions, DIR_ACT);
	
	strcpy(cBackUpNormPath, cLocalBackUpPathFull);
	//strcat(cBackUpNormPath, DIR_NORM);
	strcpy(cBackUpSlowPath, cLocalBackUpPathSlow);
	//strcat(cBackUpSlowPath, DIR_SLOW);
	strcpy(cBackUpDiffPath, cLocalBackUpPathDiff);
	//strcat(cBackUpDiffPath, DIR_DIFF);
	strcpy(cBackUpAudioPath, cLocalBackUpPathAudio);
	//strcat(cBackUpAudioPath, DIR_AUD);
	strcpy(cBackUpStatusesPath, cLocalBackUpPathStatuses);
	//strcat(cBackUpStatusesPath, DIR_STAT);
	strcpy(cBackUpEventsPath, cLocalBackUpPathEvents);
	//strcat(cBackUpEventsPath, DIR_EVNT);
	strcpy(cBackUpActionsPath, cLocalBackUpPathActions);
	//strcat(cBackUpActionsPath, DIR_ACT);
		
	strcpy(cArchiveNormPath, cLocalArchivePathFull);
	//strcat(cArchiveNormPath, DIR_NORM);
	strcpy(cArchiveSlowPath, cLocalArchivePathSlow);
	//strcat(cArchiveSlowPath, DIR_SLOW);
	strcpy(cArchiveDiffPath, cLocalArchivePathDiff);
	//strcat(cArchiveDiffPath, DIR_DIFF);
	strcpy(cArchiveAudioPath, cLocalArchivePathAudio);
	//strcat(cArchiveAudioPath, DIR_AUD);
	strcpy(cArchiveStatusesPath, cLocalArchivePathStatuses);
	//strcat(cArchiveStatusesPath, DIR_STAT);
	strcpy(cArchiveEventsPath, cLocalArchivePathEvents);
	//strcat(cArchiveEventsPath, DIR_EVNT);
	strcpy(cArchiveActionsPath, cLocalArchivePathActions);
	//strcat(cArchiveActionsPath, DIR_ACT);
	
	if ((strlen(cWriterServicePath) < (MAX_PATH - 1)) &&
		(cWriterServicePath[strlen(cWriterServicePath)-1] != 47))	cWriterServicePath[strlen(cWriterServicePath)] = 47;
	
	vc_gencmd_init();	
	
	iAccessLevel = iDefAccessLevel;
	if (iSlideShowTimerSet == 0) iSlideShowTimerSet = 60;
	
	n = ModuleTypeToNum(MODULE_TYPE_SYSTEM, 2);
	if (n >= 0)
	{
		miModuleList[n].Enabled = 1;
		miModuleList[n].Status[1] = iAccessLevel;
		miSystemList[0].ID = miModuleList[n].ID;
		memcpy(miSystemList[0].Name, miModuleList[n].Name, 64);
		sprintf(cLogMlList.MainText, "%.30s(%.4s) Log message", miSystemList[0].Name, (char*)&miSystemList[0].ID);	
	}
	else
	{
		iModuleCnt++;
		miModuleList = (MODULE_INFO*)DBG_REALLOC(miModuleList, sizeof(MODULE_INFO)*iModuleCnt);
		memset(&miModuleList[iModuleCnt-1], 0, sizeof(MODULE_INFO));
		miModuleList[iModuleCnt-1].Enabled = 1;
		miModuleList[iModuleCnt-1].ID = miSystemList[0].ID;	
		miModuleList[iModuleCnt-1].Status[1] = iAccessLevel;		
		memcpy(miModuleList[iModuleCnt-1].Name, miSystemList[0].Name, 64);
		miModuleList[iModuleCnt-1].Type = MODULE_TYPE_SYSTEM;
		/*if ((strlen((char*)miModuleList[iModuleCnt-1].Name) + strlen(miSystemList[0].Version + 3)) < 64)
		{
			strcat((char*)miModuleList[iModuleCnt-1].Name, " (");
			strcat((char*)miModuleList[iModuleCnt-1].Name, miSystemList[0].Version);
			strcat((char*)miModuleList[iModuleCnt-1].Name, ")");			
		}*/
		sprintf(cLogMlList.MainText, "%.35s(%.4s) Log message", miSystemList[0].Name, (char*)&miSystemList[0].ID);
		n = iModuleCnt-1;
	}
	
	miModuleList[n].Settings[0] = cDateTimeReference;
	miModuleList[n].Settings[1] = 0;
	miModuleList[n].Settings[2] = 0;
	miModuleList[n].Settings[3] = 0;
	miModuleList[n].Settings[4] = cCaptureFilesView | cBackUpFilesView;
	miModuleList[n].Settings[5] = cCaptureFilesLevel;
	miModuleList[n].Settings[6] = cBackUpFilesLevel;
	miModuleList[n].Status[7] = get_mem_gpu_mb();
	miModuleList[n].Status[9] = get_mem_cpu_mb();
	miModuleList[n].Status[10] = (int)(get_sys_volt(0) * 1000);
	miModuleList[n].Status[11] = (int)(get_sys_volt(1) * 1000);
	miModuleList[n].Status[12] = (int)(get_sys_volt(2) * 1000);
	miModuleList[n].Status[13] = (int)(get_sys_volt(3) * 1000);
	miModuleList[n].Version[0] = version[0];
	miModuleList[n].Version[1] = version[1];
	miModuleList[n].Version[2] = version[2];
	miModuleList[n].Version[3] = version[3];
	
		
	//int iDetectLocalAddr = 
	GetLocalAddr(&miSystemList[0].Address);
	miSystemList[0].Type = uiDeviceType;
	miSystemList[0].Active = MAX_SYSTEM_INFO_LIVE;	
	uiSystemID = miSystemList[0].ID;
	
	rsiRemoteStatus.ID = 0;
	rsiRemoteStatus.Address.sin_addr.s_addr = inet_addr("0.0.0.0");
	rsiRemoteStatus.Address.sin_family = AF_INET;
    rsiRemoteStatus.Address.sin_port = htons(TCP_PORT);
    
	int iIrPin = -1;
	for (n = 0; n < iModuleCnt; n++)
	{
		if (((miModuleList[n].Type == MODULE_TYPE_IR_RECEIVER) && (miModuleList[n].Enabled & 1)) && (iIrPin != -1)) miModuleList[n].Enabled = 0;
		if ((miModuleList[n].Type == MODULE_TYPE_IR_RECEIVER) && (miModuleList[n].Enabled & 1)) iIrPin = miModuleList[n].Settings[2];
	}
		
	for (n = 0; n < iModuleCnt; n++) 
	{
		if (miModuleList[n].Enabled & 1) miModuleList[n].Enabled |= 2;
		miModuleList[n].Local = 1; 
		miModuleList[n].ParentID = miSystemList[0].ID;
		miModuleList[n].Address.sin_addr.s_addr = inet_addr("127.0.0.1");
		miModuleList[n].Address.sin_port = htons(TCP_PORT);
		miModuleList[n].Address.sin_family = AF_INET;
		if ((miModuleList[n].Type == MODULE_TYPE_COUNTER) ||
			(miModuleList[n].Type == MODULE_TYPE_MEMORY))
		{
			for (i = 0; i < MAX_MODULE_SETTINGS; i++)
				if (i < MAX_MODULE_STATUSES) miModuleList[n].Status[i] = miModuleList[n].Settings[i];
					else break;
		}
		if ((iIrPin != -1) && (miModuleList[n].Type == MODULE_TYPE_GPIO) && 
			(miModuleList[n].Enabled & 1) && 
			(miModuleList[n].Settings[1] == iIrPin)) miModuleList[n].Enabled = 0;		
	}
	
	for (n = 0; n < iSecurityKeyCnt; n++) {skiSecurityKeys[n].Local = 1; skiSecurityKeys[n].ParentID = miSystemList[0].ID;}
	SortModules(miModuleList, iModuleCnt);
	PrintSettings();	
	
	pthread_attr_init(&tattrFileIO);   
	pthread_attr_setdetachstate(&tattrFileIO, PTHREAD_CREATE_DETACHED);
				
    pthread_mutex_init(&ircode_mutex, NULL);
	pthread_mutex_init(&rectangle_mutex, NULL);
	pthread_mutex_init(&modulelist_mutex, NULL);
	pthread_mutex_init(&securitylist_mutex, NULL);	
	pthread_mutex_init(&systemlist_mutex, NULL);	
	pthread_mutex_init(&modevent_mutex, NULL);
	pthread_mutex_init(&system_mutex, NULL);
	pthread_mutex_init(&stream_mutex, NULL);
	pthread_mutex_init(&widget_mutex, NULL);	
	pthread_mutex_init(&message_mutex, NULL);
	pthread_mutex_init(&evntaction_mutex, NULL);
	pthread_mutex_init(&mnlaction_mutex, NULL);
	pthread_mutex_init(&alienkey_mutex, NULL);
	pthread_mutex_init(&ptz_mutex, NULL);
	pthread_mutex_init(&skipevent_mutex, NULL);
	pthread_mutex_init(&skipircode_mutex, NULL);
	pthread_mutex_init(&user_mutex, NULL);
	pthread_mutex_init(&Network_Mutex, NULL);
	pthread_mutex_init(&dyspcont_mutex, NULL);	
	
	tx_eventer_create(&modevent_evnt, 1);
	tx_eventer_create(&strmevent_evnt, 1);
	tx_eventer_create(&writeevent_evnt, 1);
	tx_eventer_create(&main_thread_evnt, 1);
	tx_eventer_add_event(&main_thread_evnt, EVENT_STOP);
	
	if (iSafeFlag && (uiWEBServer == 0))
	{
		WebAuth = 0;
		WebMaxTimeIdle = 3600;
		uiWebPort = 80;
	}
	
	if ((uiWEBServer == 1) || iSafeFlag)
	{
		CreateMenu(&pWebMenu);
		if (strlen(cManualFile))
		{
			memset(bb, 0, 256);
			getcwd(bb, 256);
			strcat(bb, "/");
			strcat(bb, cManualFile);
			if (load_file_in_buff(bb, &mbManual) == 0) dbgprintf(1,"Error load Manual file: %s\n", bb);			
		}
	}
	
	unsigned int uiDevType = uiDeviceType;
	unsigned char ucFastCaptVid = 0;
	unsigned char ucSlowCaptVid = 0;
	unsigned char ucDiffCaptVid = 0;
	unsigned char ucCaptAud = 0;
	char cScannerEnabled = 0;
	unsigned int cFingerEnabled = 0;
	
			
	time(&rawtime);	
	localtime_r(&rawtime, &timeinfo);
	char timebuff[64];
	memset(timebuff, 0, 64);
	int cnt;
	
	if (iSafeFlag == 0)
	{
		//if (cDateTimeReference) ucTimeUpdated = 2;
					
		if (uiDeviceType & (DEVICE_TYPE_GPIO | DEVICE_TYPE_SPI | DEVICE_TYPE_I2C))
		{
			if ((geteuid() != 0) || (!bcm2835_init()))
			{
				dbgprintf(1,"Error init bcm2835\n");
				if (uiDeviceType & DEVICE_TYPE_GPIO) uiDeviceType ^= DEVICE_TYPE_GPIO;
				if (uiDeviceType & DEVICE_TYPE_SPI) uiDeviceType ^= DEVICE_TYPE_SPI;
				if (uiDeviceType & DEVICE_TYPE_I2C) uiDeviceType ^= DEVICE_TYPE_I2C;
			}
		}
		if (uiDeviceType & DEVICE_TYPE_VIDEO_OUT) 
		{
			CreateMenu(&pScreenMenu);
			//pthread_attr_init(&tattrPlayMedia);   
			//pthread_attr_setdetachstate(&tattrPlayMedia, PTHREAD_CREATE_DETACHED);		
		} else SetCurShowType(SHOW_TYPE_OFF);
		iCurrentVolume = iBasicVolume;
		
		if (uiDeviceType & DEVICE_TYPE_UART)
		{
			for (n = 0; n < iModuleCnt; n++) 
			{
				if (miModuleList[n].Enabled & 1) 
				{
					if (miModuleList[n].Type == MODULE_TYPE_RS485)
					{
						dbgprintf(3,"Init_UART %i for RS485\n", n); 
						miModuleList[n].InitParams[0] = uart_init_port(miModuleList[n].Settings[1], miModuleList[n].Settings[3]);
					}
					if (miModuleList[n].Type == MODULE_TYPE_EXTERNAL)
					{
						dbgprintf(3,"Init_UART %i for EXTERNAL\n", n); 
						miModuleList[n].InitParams[0] = uart_init_port(miModuleList[n].Settings[3], miModuleList[n].Settings[2]);
					}
				}
			}		
		}	
		 
		if (uiDeviceType & DEVICE_TYPE_I2C) 
		{
			i2c_init();			
			for (n = 0; n < iModuleCnt; n++)
			{
				if (miModuleList[n].Enabled & 1)
				{					
					if ((miModuleList[n].Type == MODULE_TYPE_TEMP_SENSOR) || 
						(miModuleList[n].Type == MODULE_TYPE_RTC) || 
						(miModuleList[n].Type == MODULE_TYPE_RADIO) || 
						(miModuleList[n].Type == MODULE_TYPE_ADS1015) || 
						(miModuleList[n].Type == MODULE_TYPE_MCP3421) || 
						(miModuleList[n].Type == MODULE_TYPE_AS5600) || 
						(miModuleList[n].Type == MODULE_TYPE_HMC5883L))
								miModuleList[n].InitParams[0] = i2c_open(100000);					
					if (miModuleList[n].Type == MODULE_TYPE_MCP3421) 
						miModuleList[n].Settings[8] = MCP3421_init(miModuleList[n].InitParams[0], I2C_ADDRESS_MCP3421 | miModuleList[n].Settings[0], 
														miModuleList[n].Settings[1], miModuleList[n].Settings[2]);
					if (miModuleList[n].Type == MODULE_TYPE_ADS1015) 
						miModuleList[n].Settings[8] = ADS1015_init(miModuleList[n].InitParams[0], I2C_ADDRESS_ADS1015 | miModuleList[n].Settings[0], 
														miModuleList[n].Settings[2]);
					if ((miModuleList[n].Type == MODULE_TYPE_EXTERNAL) && (miModuleList[n].Settings[0] == 1))
							miModuleList[n].InitParams[0] = i2c_open(get_i2c_code_to_speed(miModuleList[n].Settings[2]));
					if ((miModuleList[n].Type == MODULE_TYPE_ADS1015) && !i2c_echo(miModuleList[n].InitParams[0], I2C_ADDRESS_ADS1015 | miModuleList[n].Settings[0]))
							dbgprintf(1,"I2C ADS1015 NOT finded\n");
					if ((miModuleList[n].Type == MODULE_TYPE_MCP3421) && !i2c_echo(miModuleList[n].InitParams[0], I2C_ADDRESS_MCP3421 | miModuleList[n].Settings[0]))
							dbgprintf(1,"I2C MCP3421 NOT finded\n");
					if ((miModuleList[n].Type == MODULE_TYPE_AS5600) && !i2c_echo(miModuleList[n].InitParams[0], I2C_ADDRESS_AS5600 | miModuleList[n].Settings[0]))
							dbgprintf(1,"I2C AS5600 NOT finded\n");	
					if ((miModuleList[n].Type == MODULE_TYPE_HMC5883L) && !i2c_echo(miModuleList[n].InitParams[0], I2C_ADDRESS_HMC5883L))
							dbgprintf(1,"I2C HMC5883L NOT finded\n");	
				}
			}
		}		
		if (uiDeviceType & DEVICE_TYPE_I2C)
		{
			//i2c_scan();
			if (uiDeviceType & DEVICE_TYPE_CLOCK)
			{
				if (i2c_echo(0, I2C_ADDRESS_CLOCK) == 0)
				{
					uiDeviceType ^= DEVICE_TYPE_CLOCK;
					dbgprintf(1,"I2C clock NOT finded\n");
				}
				else
				{
					dbgprintf(3,"I2C clock finded\n");
					if (cNTPServer[0] == 0) ucTimeUpdated = 2; else ucTimeUpdated = 1;
					i2c_read_timedate3231(I2C_ADDRESS_CLOCK);
				}
			}
			if (uiDeviceType & DEVICE_TYPE_RADIO)
			{
				if (i2c_echo(0, I2C_ADDRESS_RADIO) == 0)
				{
					uiDeviceType ^= DEVICE_TYPE_RADIO;
					dbgprintf(1,"I2C radio NOT finded\n");					
				}
				else 
				{
					dbgprintf(3,"I2C radio finded\n");
					RDA5807M_init();
					//RDA5807M_setVolume(4);
					//OnRadioStation(6);
				}			
			}	
			if (uiDeviceType & DEVICE_TYPE_TEMP)
			{
				for (n = 0; n < iModuleCnt; n++)
				{
					if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_TEMP_SENSOR))
					{
						miModuleList[n].Status[0] = 10000;
						miModuleList[n].Status[1] = 10000;
							
						ret = miModuleList[n].Settings[0] | miModuleList[n].Settings[1];
						if (i2c_echo(miModuleList[n].InitParams[0], ret) == 0)
						{
							//uiDeviceType ^= DEVICE_TYPE_TEMP;
							dbgprintf(1,"I2C temp sensor NOT finded\n");				
						}
						else
						{
							miModuleList[n].Status[0] = 10000;
							miModuleList[n].Status[1] = 10000;
							if (miModuleList[n].Settings[0] == I2C_ADDRESS_AM2320) dbgprintf(3,"I2C temp sensor AM2320 finded: %s\n", miModuleList[n].Name);
							else 
							if (miModuleList[n].Settings[0] == I2C_ADDRESS_LM75) dbgprintf(3,"I2C temp sensor LM75 finded: %s\n", miModuleList[n].Name);
							else dbgprintf(2,"I2C temp sensor UNKNOWN finded: %s\n", miModuleList[n].Name);
							
							//int iTemp, iHumid;
							//iTemp = 255;
							//LM75_read(ret, &iTemp);
							
							//if (AM2320_read(&iTemp, &iHumid) == 0) dbgprintf(2,"I2C temp sensor NOT readed\n");
							//dbgprintf(1,"Temp:%g, Hum:%i\n",(double)iTemp/10, iHumid);
						}					
					}
				}			
			}
		}
		
		if (uiDeviceType & DEVICE_TYPE_SPI) {dbgprintf(3,"Init_SPI\n"); if (spi_init(2000000) == 0) uiDeviceType ^= DEVICE_TYPE_SPI;}
		
		if (uiDeviceType & DEVICE_TYPE_GPIO)
		{
			dbgprintf(3,"Init_BCM2835\n"); 
			
			dbgprintf(3,"Init_GPIO\n"); 
			if (gpio_init(miModuleList, iModuleCnt) == 0) uiDeviceType ^= DEVICE_TYPE_GPIO;
		}

		DBG_MUTEX_LOCK(&modulelist_mutex);					
		for (i = 0; i < iModuleCnt; i++)
		{
			if (miModuleList[i].Enabled & 1) 
			{
				if (miModuleList[i].Type == MODULE_TYPE_USB_GPIO)
				{
					dbgprintf(3,"Init_USB_GPIO\n"); 
					if (usb_gpio_init(&miModuleList[i]) <= 0) 
					{
						miModuleList[i].Enabled ^= 1;
						dbgprintf(2,"Error Init_USB_GPIO '%.4s'\n", (char*)&miModuleList[i].ID);
					}
					else 
						dbgprintf(3,"Init_USB_GPIO '%.4s' done, submodules:%i\n", (char*)&miModuleList[i].ID, (miModuleList[i].Settings[0] >> 8) & 0xFF);					
				}
				if (miModuleList[i].Type == MODULE_TYPE_EXTERNAL)
				{
					dbgprintf(3,"Init_EXTERNAL\n"); 
					if (external_init(&miModuleList[i]) <= 0) 
					{
						miModuleList[i].Enabled ^= 1;
						dbgprintf(2,"Error Init_EXTERNAL '%.4s'\n", (char*)&miModuleList[i].ID);
					}
					else
						dbgprintf(3,"Init_EXTERNAL '%.4s' done, submodules:%i\n", (char*)&miModuleList[i].ID, (miModuleList[i].Settings[0] >> 8) & 0xFF);					
				}
				if (miModuleList[i].Type == MODULE_TYPE_TFP625A)
				{
					dbgprintf(3,"Init_TFP625A\n"); 
					if (TFP625A_init(&miModuleList[i]) <= 0) 
					{
						miModuleList[i].Enabled ^= 1;
						dbgprintf(2,"Error Init_TFP625A '%.4s'\n", (char*)&miModuleList[i].ID);	
					}
					else 
						dbgprintf(3,"Init_TFP625A '%.4s' done\n", (char*)&miModuleList[i].ID);					
				}
			}
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);			
		
		dbgprintf(3,"Init_OMX\n");   
		omx_Start();
		
		dbgprintf(3,"Init_MEDIA\n"); 
		audio_init();
		
		for (n = 0; n < iModuleCnt; n++)
		{
			if (miModuleList[n].Enabled & 1)
			{
				if ((miModuleList[n].Type == MODULE_TYPE_CAMERA) && ucCaptEnabledVideo)
				{
					if (miModuleList[n].Settings[8] & 8) ucSlowCaptVid = 1;
					if (miModuleList[n].Settings[8] & 16) ucDiffCaptVid = 1;
					if (miModuleList[n].Settings[8] & 32) ucFastCaptVid = 1;
				}
				if ((miModuleList[n].Type == MODULE_TYPE_RS485) ||
					(miModuleList[n].Type == MODULE_TYPE_SPEAKER) ||
					(miModuleList[n].Type == MODULE_TYPE_RC522) ||
					(miModuleList[n].Type == MODULE_TYPE_PN532) ||
					(miModuleList[n].Type == MODULE_TYPE_CAMERA) ||
					(miModuleList[n].Type == MODULE_TYPE_TFP625A))
				{
					miModuleList[n].SubModule = ModuleIdToNum(miModuleList[n].Settings[0], 1);			
				}
				if (miModuleList[n].Type == MODULE_TYPE_SPEAKER) 
				{
					SetAudioPlayDeviceName(miModuleList[n].Settings[1]);
					audio_set_playback_volume(iBasicVolume);
				}
				if ((miModuleList[n].Type == MODULE_TYPE_MIC) && ucCaptEnabledAudio)
				{
					if (miModuleList[n].Settings[7] & 1) ucCaptAud |= 1;	//NORM
					if (miModuleList[n].Settings[10]) ucCaptAud |= 2;		//AUDIO DIFF
					if (miModuleList[n].Settings[7] & 2) ucCaptAud |= 4;	//VIDEO DIFF
					audio_set_capture_volume(miModuleList[n].Settings[1], miModuleList[n].Settings[15]);
					audio_set_capture_agc(miModuleList[n].Settings[1], (miModuleList[n].Settings[7] & 4) ? 1 : 0);
				}
			}
		}
		
		dbgprintf(3,"Starting LAN service\n");			
		if (InitLAN() <= 0) {dbgprintf(1,"Starting LAN service FAILED\n");}
		  
		if (uiDevType & DEVICE_TYPE_IR_RECEIVER) 
		{
			dbgprintf(3,"Open IR device %i\n", iIrPin);
			ir_init(iIrPin);
		}	
		
		///////////Launch control modules//////////
		DBG_MUTEX_LOCK(&modulelist_mutex);					
		for (n = 0; n != iModuleCnt; n++)
		{
			if ((miModuleList[n].Enabled & 1) && (miModuleList[n].ScanSet != 0) && 
						((miModuleList[n].Type == MODULE_TYPE_USB_GPIO) ||
						(miModuleList[n].Type == MODULE_TYPE_GPIO) ||
						(miModuleList[n].Type == MODULE_TYPE_TEMP_SENSOR) ||						
						(miModuleList[n].Type == MODULE_TYPE_ADS1015) ||
						(miModuleList[n].Type == MODULE_TYPE_MCP3421) ||
						(miModuleList[n].Type == MODULE_TYPE_AS5600) ||
						(miModuleList[n].Type == MODULE_TYPE_HMC5883L) ||
						(miModuleList[n].Type == MODULE_TYPE_IR_RECEIVER) ||
						(miModuleList[n].Type == MODULE_TYPE_CAMERA) ||
						(miModuleList[n].Type == MODULE_TYPE_MIC) ||
						(miModuleList[n].Type == MODULE_TYPE_RS485) ||
						(miModuleList[n].Type == MODULE_TYPE_KEYBOARD) ||
						(miModuleList[n].Type == MODULE_TYPE_COUNTER) ||
						(miModuleList[n].Type == MODULE_TYPE_MEMORY) ||
						(miModuleList[n].Type == MODULE_TYPE_SYSTEM) ||
						(miModuleList[n].Type == MODULE_TYPE_EXTERNAL) ||
						(miModuleList[n].Type == MODULE_TYPE_TFP625A)))
			{			
				dbgprintf(3,"Starting module scaner service\n");
				misc_buffer *mBuff = (misc_buffer *)DBG_MALLOC(sizeof(misc_buffer));
				mBuff->void_data = DBG_MALLOC(sizeof(MODULE_INFO)*iModuleCnt);
				memcpy(mBuff->void_data, miModuleList, sizeof(MODULE_INFO)*iModuleCnt);
				mBuff->data_size = iModuleCnt;
				mBuff->data = DBG_CALLOC(strlen(cMediaCapturePathStatuses) + 1, 1);
				memcpy(mBuff->data, cMediaCapturePathStatuses, strlen(cMediaCapturePathStatuses));
				mBuff->data2 = DBG_CALLOC(strlen(cMediaBackUpPathStatuses) + 1, 1);
				memcpy(mBuff->data2, cMediaBackUpPathStatuses, strlen(cMediaBackUpPathStatuses));				
				
				MODULE_INFO *mi = mBuff->void_data;
				for (i = 0; i < iModuleCnt; i++)
				{
					if (mi[i].ParamsCount)
					{
						mi[i].ParamList = (Sensor_Params*)DBG_MALLOC(sizeof(Sensor_Params) * mi[i].ParamsCount);
						memcpy(mi[i].ParamList, miModuleList[i].ParamList, sizeof(Sensor_Params) * mi[i].ParamsCount);
					}
					if (mi[i].SettingsCount)
					{
						mi[i].SettingList = (Sensor_Params*)DBG_MALLOC(sizeof(Sensor_Params) * mi[i].SettingsCount);
						memcpy(mi[i].SettingList, miModuleList[i].SettingList, sizeof(Sensor_Params) * mi[i].SettingsCount);
					}
				}
				
				mBuff->uidata[0] = uiCaptureFileSizeStatuses;
				mBuff->uidata[1] = uiCaptureTimeLimitStatuses;
				mBuff->uidata[2] = ucBackUpOrderLmtStatuses;
				mBuff->uidata[3] = ucBackUpModeStatuses;
				mBuff->uidata[4] = ucCaptEnabledStatuses;
				mBuff->uidata[5] = uiBackUpOrderWaitCnclStatuses;
				mBuff->uidata[6] = uiBackUpOrderWaitMessStatuses;
				mBuff->void_data2 = NULL;
				if (ucBackUpOrderEnableStatuses) 
				{
					mBuff->void_data2 = DBG_MALLOC(32);
					memcpy(mBuff->void_data2, ucBackUpOrderAddrStatuses, 32);
				}
				i = strlen(ucCaptureStartPathStatuses);
				mBuff->void_data3 = DBG_MALLOC(i + 1);
				memset(mBuff->void_data3, 0, i + 1);
				memcpy(mBuff->void_data3, ucCaptureStartPathStatuses, i);
				
				pthread_attr_init(&tattrScaner);   
				pthread_attr_setdetachstate(&tattrScaner, PTHREAD_CREATE_DETACHED);
				pthread_create(&threadScaner, &tattrScaner, thread_Scaner, mBuff);
				cScannerEnabled = 1;
				//dbgprintf(3,"Started module scaner service\n");	
				break;
			}			
		}  
		for (n = 0; n != iModuleCnt; n++)
		{
			if ((miModuleList[n].Enabled & 1) 
				&& (miModuleList[n].ScanSet != 0) 
				&& (miModuleList[n].Type == MODULE_TYPE_TFP625A))
				{
					dbgprintf(3,"Starting TFP625A module scaner thread\n");
					misc_buffer *mBuff = (misc_buffer *)DBG_MALLOC(sizeof(misc_buffer));
					mBuff->void_data = DBG_MALLOC(sizeof(MODULE_INFO));
					memcpy(mBuff->void_data, &miModuleList[n], sizeof(MODULE_INFO));
					mBuff->data = DBG_CALLOC(strlen(cMediaCapturePathStatuses) + 1, 1);
					memcpy(mBuff->data, cMediaCapturePathStatuses, strlen(cMediaCapturePathStatuses));
					mBuff->data2 = DBG_CALLOC(strlen(cMediaBackUpPathStatuses) + 1, 1);
					memcpy(mBuff->data2, cMediaBackUpPathStatuses, strlen(cMediaBackUpPathStatuses));				
												
					mBuff->uidata[0] = uiCaptureFileSizeStatuses;
					mBuff->uidata[1] = uiCaptureTimeLimitStatuses;
					mBuff->uidata[2] = ucBackUpOrderLmtStatuses;
					mBuff->uidata[3] = ucBackUpModeStatuses;
					mBuff->uidata[4] = ucCaptEnabledStatuses;
					mBuff->uidata[5] = uiBackUpOrderWaitCnclStatuses;
					mBuff->uidata[6] = uiBackUpOrderWaitMessStatuses;
					mBuff->void_data2 = NULL;
					if (ucBackUpOrderEnableStatuses) 
					{
						mBuff->void_data2 = DBG_MALLOC(32);
						memcpy(mBuff->void_data2, ucBackUpOrderAddrStatuses, 32);
					}
					i = strlen(ucCaptureStartPathStatuses);
					mBuff->void_data3 = DBG_MALLOC(i + 1);
					memset(mBuff->void_data3, 0, i + 1);
					memcpy(mBuff->void_data3, ucCaptureStartPathStatuses, i);
				
					pthread_attr_init(&tattrFinger);   
					pthread_attr_setdetachstate(&tattrFinger, PTHREAD_CREATE_DETACHED);
					pthread_create(&threadFinger, &tattrFinger, thread_Finger, mBuff);
					cFingerEnabled++;
				}
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);				
		///////////Launch control modules//////////	
		
		////////////Launch module event controller//////////
		dbgprintf(3,"Starting Eventer service\n");
		
		misc_buffer *mBuff = (misc_buffer *)DBG_MALLOC(sizeof(misc_buffer));
			
		mBuff->data = (void*)DBG_CALLOC(MAX_PATH*2, 1);	
		memcpy(&mBuff->data[0], cMediaCapturePathEvents, MAX_PATH);
		memcpy(&mBuff->data[256], cMediaCapturePathActions, MAX_PATH);
		
		mBuff->data2 = (void*)DBG_CALLOC(MAX_PATH*2, 1);	
		memcpy(&mBuff->data2[0], cMediaBackUpPathEvents, MAX_PATH);
		memcpy(&mBuff->data2[256], cMediaBackUpPathActions, MAX_PATH);
				
		mBuff->uidata[0] = uiCaptureFileSizeEvents;
		mBuff->uidata[1] = uiCaptureTimeLimitEvents;
		mBuff->uidata[2] = ucBackUpOrderLmtEvents;
		mBuff->uidata[3] = ucBackUpModeEvents;
		mBuff->uidata[4] = ucCaptEnabledEvents;
		mBuff->uidata[5] = uiCaptureFileSizeActions;
		mBuff->uidata[6] = uiCaptureTimeLimitActions;
		mBuff->uidata[7] = ucBackUpOrderLmtActions;
		mBuff->uidata[8] = ucBackUpModeActions;
		mBuff->uidata[9] = ucCaptEnabledActions;
		mBuff->uidata[10] = uiBackUpOrderWaitCnclEvents;
		mBuff->uidata[11] = uiBackUpOrderWaitMessEvents;
		mBuff->uidata[12] = uiBackUpOrderWaitCnclActions;
		mBuff->uidata[13] = uiBackUpOrderWaitMessActions;
		mBuff->void_data2 = NULL;
		if (ucBackUpOrderEnableEvents) 
		{
			mBuff->void_data2 = DBG_MALLOC(32);
			memcpy(mBuff->void_data2, ucBackUpOrderAddrEvents, 32);
		}
		i = strlen(ucCaptureStartPathEvents);
		mBuff->void_data3 = DBG_MALLOC(i + 1);
		memset(mBuff->void_data3, 0, i + 1);
		memcpy(mBuff->void_data3, ucCaptureStartPathEvents, i);
		
		mBuff->void_data4 = NULL;
		if (ucBackUpOrderEnableActions) 
		{
			mBuff->void_data4 = DBG_MALLOC(32);
			memcpy(mBuff->void_data4, ucBackUpOrderAddrActions, 32);
		}
		i = strlen(ucCaptureStartPathActions);
		mBuff->void_data5 = DBG_MALLOC(i + 1);
		memset(mBuff->void_data5, 0, i + 1);
		memcpy(mBuff->void_data5, ucCaptureStartPathActions, i);
				
		pthread_attr_init(&tattrEventer);   
		pthread_attr_setdetachstate(&tattrEventer, PTHREAD_CREATE_DETACHED);
		pthread_create(&threadEventer, &tattrEventer, thread_Eventer, mBuff);
		//dbgprintf(3,"Started Eventer service\n");			
		////////////Launch module event controller//////////	
						
	//	psemIP = DBG_MALLOC(sizeof(TX_SEMAPHORE));
		//tx_semaphore_create(psemIP, NULL, 0);	
		func_link *f_link = NULL;
		omx_egl_link *e_link = NULL;
		
		if (uiDevType & DEVICE_TYPE_VIDEO_IN) 
		{
			unsigned int uiID1 = 0;
			unsigned int uiID2 = 0;		
			unsigned int uiAudioCodec = 0;		
			n = 0;
			i = 0;
			DBG_MUTEX_LOCK(&modulelist_mutex);					
			for (n = 0; n < iModuleCnt; n++)
			{
				if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_CAMERA)) 
				{
					uiID1 = miModuleList[n].ID;
					i = miModuleList[n].Settings[8];
					break;	
				}
			}	
			for (n = 0; n < iModuleCnt; n++)
				if ((miModuleList[n].Enabled & 1) 
					&& (miModuleList[n].Type == MODULE_TYPE_MIC)
					&& (miModuleList[n].Settings[7] & 2)
					&& (miModuleList[n].Settings[16] < 6))
					{
						uiID2 = miModuleList[n].ID;
						uiAudioCodec = miModuleList[n].Settings[16];
						break;
					}
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
					
			if (ucSlowCaptVid) 
			{
				STREAM_INFO *si = AddCaptureStream(CAPTURE_TYPE_SLOW, uiID1, 0);
				SaveCapturedStreams(cMediaCapturePathSlow, si, uiCaptureFileSizeSlow, uiCaptureTimeLimitSlow, 1, 0, 
									ucBackUpModeSlow, cMediaBackUpPathSlow, i & 2, i & 1, 0, ucBackUpOrderLmtSlow, 
									ucBackUpOrderEnableSlow ? ucBackUpOrderAddrSlow : NULL,
									ucCaptureStartPathSlow, uiFlvBufferSize,
									uiBackUpOrderWaitCnclSlow, uiBackUpOrderWaitMessSlow, WRT_TYPE_BACKUP_SLOW);
			}
			if (ucDiffCaptVid) 
			{
				STREAM_INFO *si = AddCaptureStream(CAPTURE_TYPE_DIFF, uiID1, uiID2);							
				SaveCapturedStreams(cMediaCapturePathDiff, si, uiCaptureFileSizeDiff, uiCaptureTimeLimitDiff,
					1, 
					uiID2 ? 1 : 0, 
					ucBackUpModeDiff, cMediaBackUpPathDiff, 1, 0, uiAudioCodec, ucBackUpOrderLmtDiff, 
					ucBackUpOrderEnableDiff ? ucBackUpOrderAddrDiff : NULL,
					ucCaptureStartPathDiff, uiFlvBufferSize,
					uiBackUpOrderWaitCnclDiff, uiBackUpOrderWaitMessDiff, WRT_TYPE_BACKUP_DIFF);
			}
		}
		
		if (uiDevType & (DEVICE_TYPE_AUDIO_IN | DEVICE_TYPE_VIDEO_IN))
		{
			unsigned int uiID1;
			unsigned int uiID2;
			unsigned int uiAudioCodec = 0;
			
			if (((uiDevType & DEVICE_TYPE_VIDEO_IN) && (ucFastCaptVid == 1)) 
				|| ((uiDevType & DEVICE_TYPE_AUDIO_IN)  && (ucCaptAud & 1)))
			{	
				uiID1 = 0;
				uiID2 = 0;
				DBG_MUTEX_LOCK(&modulelist_mutex);
				if ((uiDevType & DEVICE_TYPE_VIDEO_IN) && (ucFastCaptVid == 1))
				{
					uiID1 = miModuleList[ModuleTypeToNum(MODULE_TYPE_CAMERA, 1)].ID;
				}
				if ((uiDevType & DEVICE_TYPE_AUDIO_IN) && (ucCaptAud & 1))
				{
					for (n = 0; n < iModuleCnt; n++)
					{
						if ((miModuleList[n].Enabled & 1) 
							&& (miModuleList[n].Type == MODULE_TYPE_MIC)
							&& (miModuleList[n].Settings[7] & 1))
							{
								uiID2 = miModuleList[n].ID;
								uiAudioCodec = miModuleList[n].Settings[16];
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								if ((miModuleList[n].Settings[16] < 6) && uiID1)
								{
									STREAM_INFO *si = AddCaptureStream(CAPTURE_TYPE_FULL, uiID1, uiID2);
									SaveCapturedStreams(cMediaCapturePathFull, si, uiCaptureFileSizeFull, uiCaptureTimeLimitFull, 
											uiID1 ? 1 : 0, 
											1,
											ucBackUpModeFull, cMediaBackUpPathFull, 0, 0, uiAudioCodec, ucBackUpOrderLmtFull,
											ucBackUpOrderEnableFull ? ucBackUpOrderAddrFull : NULL,
											ucCaptureStartPathFull, uiFlvBufferSize,
											uiBackUpOrderWaitCnclFull, uiBackUpOrderWaitMessFull, WRT_TYPE_BACKUP_FULL);
									uiID1 = 0;
								}
								else
								{
									/*if (uiID1)
									{
										ret = AddCaptureStream(CAPTURE_TYPE_FULL, uiID1, uiID2);
										SaveCapturedStreams(cMediaNormCapturePath, ret, uiCaptureFileSizeFull, uiCaptureTimeLimitFull, 
										1, 0, ucBackUpModeFull, cMediaBackUpPathFull, 0, 0, uiAudioCodec);
									}*/
									STREAM_INFO *si = AddCaptureStream(CAPTURE_TYPE_FULL, uiID1, uiID2);
									SaveCapturedStreams(cMediaCapturePathFull, si, uiCaptureFileSizeFull, uiCaptureTimeLimitFull, 
										0, 1, ucBackUpModeFull, cMediaBackUpPathFull, 0, 0, uiAudioCodec, ucBackUpOrderLmtFull, 
										ucBackUpOrderEnableFull ? ucBackUpOrderAddrFull : NULL,
										ucCaptureStartPathFull, uiFlvBufferSize,
										uiBackUpOrderWaitCnclFull, uiBackUpOrderWaitMessFull, WRT_TYPE_BACKUP_FULL);
								}
								DBG_MUTEX_LOCK(&modulelist_mutex);							
							}
					}
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				if (uiID1)
				{
					STREAM_INFO *si = AddCaptureStream(CAPTURE_TYPE_FULL, uiID1, 0);
					SaveCapturedStreams(cMediaCapturePathFull, si, uiCaptureFileSizeFull, uiCaptureTimeLimitFull, 
						1, 0, ucBackUpModeFull, cMediaBackUpPathFull, 0, 0, 0, ucBackUpOrderLmtFull, 
						ucBackUpOrderEnableFull ? ucBackUpOrderAddrFull : NULL,
						ucCaptureStartPathFull, uiFlvBufferSize, uiBackUpOrderWaitCnclFull, uiBackUpOrderWaitMessFull,
						WRT_TYPE_BACKUP_FULL);
				}
			}
			if ((uiDevType & DEVICE_TYPE_AUDIO_IN) && (ucCaptAud & 2))
			{
				uiID2 = 0;
				DBG_MUTEX_LOCK(&modulelist_mutex);
				for (n = 0; n < iModuleCnt; n++)
				{
					if ((miModuleList[n].Enabled & 1) 
						&& (miModuleList[n].Type == MODULE_TYPE_MIC)
						&& (miModuleList[n].Settings[10]))	//Diff AUDIO
						{
							uiID2 = miModuleList[n].ID;
							uiAudioCodec = miModuleList[n].Settings[16];
							DBG_MUTEX_UNLOCK(&modulelist_mutex);	
							STREAM_INFO *si = AddCaptureStream(CAPTURE_TYPE_AUDIO, 0, uiID2);
							SaveCapturedStreams(cMediaCapturePathAudio, si, uiCaptureFileSizeAudio, uiCaptureTimeLimitAudio, 
								0, 2,
								ucBackUpModeAudio, cMediaBackUpPathAudio, 0, 0, uiAudioCodec, ucBackUpOrderLmtAudio, 
								ucBackUpOrderEnableAudio ? ucBackUpOrderAddrAudio : NULL,
								ucCaptureStartPathAudio, uiFlvBufferSize,
								uiBackUpOrderWaitCnclAudio, uiBackUpOrderWaitMessAudio, WRT_TYPE_BACKUP_AUDIO);
							DBG_MUTEX_LOCK(&modulelist_mutex);
						}
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);			
			}
			if (uiDevType & DEVICE_TYPE_VIDEO_IN) 
			{
				dbgprintf(3,"Starting Video Capture service\n");
				e_link = (omx_egl_link*)DBG_MALLOC(sizeof(omx_egl_link));
				memset(e_link, 0, sizeof(omx_egl_link));
				
				e_link->FuncSend = SendVideoFrame;
				e_link->FuncSave = SaveVideoFrame;
				e_link->FuncSaveSlow = SaveVideoKeyFrame;
				e_link->FuncSaveDiff = SaveDiffVideoFrame;
				e_link->FuncRTSP = RTSP_SendVideoFrame;		
				n = 0;
				DBG_MUTEX_LOCK(&modulelist_mutex);					
				for (n = 0; n != iModuleCnt; n++)
				{
					if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_CAMERA)) 
					{
						e_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));						
						memcpy(e_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));	
						break;	
					}
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				
				if (!ucCaptEnabledVideo)
				{
					dbgprintf(3,"Disabled Save Video on Settings\n");
					MODULE_INFO *mi = e_link->pModule;
					if (mi->Settings[8] & 8) mi->Settings[8] ^= 8;
					if (mi->Settings[8] & 16) mi->Settings[8] ^= 16;
					if (mi->Settings[8] & 32) mi->Settings[8] ^= 32;
				}
						
				pthread_attr_init(&tattrVideoCapture);   
				pthread_attr_setdetachstate(&tattrVideoCapture, PTHREAD_CREATE_DETACHED);
				pthread_create(&threadVideoCapture, &tattrVideoCapture, thread_omx_video_capture_send, (void*)e_link);			
			}
			
			if (uiDevType & DEVICE_TYPE_AUDIO_IN) 
			{				
				DBG_MUTEX_LOCK(&modulelist_mutex);					
				for (n = 0; n != iModuleCnt; n++)
				{
					if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Type == MODULE_TYPE_MIC))
					{
						dbgprintf(3,"Starting Audio Capture service module:%i\n", n);
						f_link = (func_link*)DBG_MALLOC(sizeof(func_link));
						memset(f_link, 0, sizeof(func_link));
						f_link->pModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));						
						f_link->FuncSend = SendAudioFrame;
						f_link->FuncSave = SaveAudioFrame;
						f_link->FuncRTSP = RTSP_SendAudioFrame;
						memcpy(f_link->pModule, &miModuleList[n], sizeof(MODULE_INFO));	
						if (!ucCaptEnabledAudio)
						{
							dbgprintf(3,"Disabled Save Audio on Settings\n");
							MODULE_INFO *mi = f_link->pModule;
							if (mi->Settings[7] & 1) mi->Settings[7] ^= 1;
							mi->Settings[10] = 0;
							if (mi->Settings[7] & 2) mi->Settings[7] ^= 2;
						}	
						CaptureAudioStream((void*)f_link);	
					}
				}	
				DBG_MUTEX_UNLOCK(&modulelist_mutex);					
			}
			
			if (uiRTSPStream == 1)
			{
				unsigned int uiFrameRate = 0;
				unsigned int uiMicListCnt = 0;
				MIC_LIST *rmlList = NULL;
				DBG_MUTEX_LOCK(&modulelist_mutex);					
				if (uiDevType & DEVICE_TYPE_VIDEO_IN) uiFrameRate = miModuleList[ModuleTypeToNum(MODULE_TYPE_CAMERA, 1)].Settings[2];
				if (uiDevType & DEVICE_TYPE_AUDIO_IN) 
				{
					for (n = 0; n != iModuleCnt; n++)
						if ((miModuleList[n].Enabled & 1)
							&& (miModuleList[n].Local == 1) 						
							&& (miModuleList[n].Type == MODULE_TYPE_MIC))
							{
								uiMicListCnt++;
								rmlList = (MIC_LIST*)DBG_REALLOC(rmlList, uiMicListCnt * sizeof(MIC_LIST));
								memset(&rmlList[uiMicListCnt - 1], 0, sizeof(MIC_LIST));
								rmlList[uiMicListCnt - 1].ID = miModuleList[n].ID;
								rmlList[uiMicListCnt - 1].Freq = miModuleList[n].Settings[3];
								rmlList[uiMicListCnt - 1].Channels = miModuleList[n].Settings[0];
								rmlList[uiMicListCnt - 1].CodecNum = miModuleList[n].Settings[16];
							}
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				rtsp_start((uiDevType & DEVICE_TYPE_VIDEO_IN) ? 1 : 0, (uiDevType & DEVICE_TYPE_AUDIO_IN) ? 1 : 0, uiFrameRate, rmlList, uiMicListCnt, uiRTSPPort, uiRTSPForceAudio);			
			}			
		}
	}
	if ((uiWEBServer == 1) || iSafeFlag) web_start(uiWebPort);
	
	if (uiOnvifStream == 1)
	{
		MODULE_INFO *miCamCopy = NULL;
		unsigned int uiMicListCnt = 0;
		MIC_LIST *rmlList = NULL;
		int uiPTZnum = -1;	
		DBG_MUTEX_LOCK(&modulelist_mutex);
		if (uiDevType & DEVICE_TYPE_VIDEO_IN) 
		{
			MODULE_INFO* miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
			if (miCamera)
			{
				miCamCopy = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
				memcpy(miCamCopy, miCamera, sizeof(MODULE_INFO));
				char uiPtzEn = miCamera->Settings[38] & 1;
				unsigned int uiPtzID = miCamera->Settings[39];
		
				if (uiPtzEn && uiPtzID)
					for (n = 0; n < iModuleCnt; n++)
						if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) && (miModuleList[n].ID == uiPtzID) &&
							(miModuleList[n].Type == MODULE_TYPE_USB_GPIO) && (miModuleList[n].SubType == MODULE_SUBTYPE_PTZ)) 
						{
							uiPTZnum = n;
							break;
						}
			}
		}
		if (uiDevType & DEVICE_TYPE_AUDIO_IN) 
		{
			for (n = 0; n != iModuleCnt; n++)
				if ((miModuleList[n].Enabled & 1)
					&& (miModuleList[n].Local == 1) 						
					&& (miModuleList[n].Type == MODULE_TYPE_MIC))
					{
						uiMicListCnt++;
						rmlList = (MIC_LIST*)DBG_REALLOC(rmlList, uiMicListCnt * sizeof(MIC_LIST));
						memset(&rmlList[uiMicListCnt - 1], 0, sizeof(MIC_LIST));
						rmlList[uiMicListCnt - 1].ID = miModuleList[n].ID;
						rmlList[uiMicListCnt - 1].Freq = miModuleList[n].Settings[3];
						rmlList[uiMicListCnt - 1].Channels = miModuleList[n].Settings[0];
						rmlList[uiMicListCnt - 1].CodecNum = miModuleList[n].Settings[16];
						rmlList[uiMicListCnt - 1].BitRate = miModuleList[n].Settings[4];
					}
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		onvif_start(uiOnvifPort, OnvifAuth, uiRTSPPort, miCamCopy, rmlList, uiMicListCnt, uiPTZnum);			
	}
	
	STREAM_INFO *siArchive = NULL;
			
	if (iSafeFlag == 0)
	{
		////////////Launch file access service//////////
		if (cCaptureFilesView || cBackUpFilesView)
		{
			dbgprintf(3,"Starting file access service\n");
			misc_buffer *mBuff = (misc_buffer *)DBG_MALLOC(sizeof(misc_buffer));
			char* pP;
			
			mBuff->void_data = (void*)DBG_CALLOC(MAX_PATH*4, 1);	
			pP = (char*)mBuff->void_data;
			memcpy(&pP[0], cMediaCapturePathFull, MAX_PATH);
			memcpy(&pP[256], cMediaCapturePathSlow, MAX_PATH);
			memcpy(&pP[512], cMediaCapturePathDiff, MAX_PATH);
			memcpy(&pP[768], cMediaCapturePathAudio, MAX_PATH);
			
			mBuff->void_data2 = (void*)DBG_CALLOC(MAX_PATH*4, 1);
			pP = (char*)mBuff->void_data2;
			memcpy(&pP[0], cLocalBackUpPathFull, MAX_PATH);
			memcpy(&pP[256], cLocalBackUpPathSlow, MAX_PATH);
			memcpy(&pP[512], cLocalBackUpPathDiff, MAX_PATH);
			memcpy(&pP[768], cLocalBackUpPathAudio, MAX_PATH);
			
				mBuff->clock = cCaptureFilesViewDef;
				mBuff->uidata[0] = cCaptureFilesView;
				mBuff->uidata[1] = cBackUpFilesView;
		
				pthread_attr_init(&tattrStreamer);   
				pthread_attr_setdetachstate(&tattrStreamer, PTHREAD_CREATE_DETACHED);
				pthread_create(&threadStreamer, &tattrStreamer, Streamer, (void*)(mBuff));
				//dbgprintf(3,"Started file access service\n");	
		}
		////////////Launch file access service//////////
		
		////////////Launch file writer service//////////
		if (cFileWriterService)
		{
			dbgprintf(3,"Starting file writer service\n");
			if ((strlen(cWriterServicePath) < MAX_PATH))
			{
				misc_buffer *mBuff = (misc_buffer *)DBG_MALLOC(sizeof(misc_buffer));
				mBuff->void_data = (void*)DBG_CALLOC(strlen(cWriterServicePath)+2, 1);
				memcpy(mBuff->void_data, cWriterServicePath, strlen(cWriterServicePath));
				char *buff = mBuff->void_data;
				if (buff[strlen(buff) - 1] != 47) buff[strlen(buff)] = 47;
				pthread_attr_init(&tattrWriter);   
				pthread_attr_setdetachstate(&tattrWriter, PTHREAD_CREATE_DETACHED);
				pthread_create(&threadWriter, &tattrWriter, Writer, (void*)(mBuff));
				//dbgprintf(3,"Started file writer service\n");	
			} else dbgprintf(2,"Big length Writer path\n");
		}
		////////////Launch file writer service//////////
				
		if (uiDevType & DEVICE_TYPE_VIDEO_OUT) 
		{
			dbgprintf(3,"Starting Shower service\n");
			pthread_attr_init(&tattrShower);   
			pthread_attr_setdetachstate(&tattrShower, PTHREAD_CREATE_DETACHED);
			pthread_create(&threadShower, &tattrShower, Shower, NULL);
			//dbgprintf(3,"Started Shower service\n");		
		}	
		
		////////////Launch module card controller//////////
		DBG_MUTEX_LOCK(&modulelist_mutex);					
		if (uiDevType & (DEVICE_TYPE_RC522 | DEVICE_TYPE_PN532)) 
		{
			dbgprintf(3,"Starting card control service\n");
			misc_buffer *mBuff = (misc_buffer *)DBG_MALLOC(sizeof(misc_buffer));
			mBuff->void_data = DBG_MALLOC(sizeof(MODULE_INFO)*iModuleCnt);
			memcpy(mBuff->void_data, miModuleList, sizeof(MODULE_INFO)*iModuleCnt);
			mBuff->data_size = iModuleCnt;
			mBuff->data = DBG_CALLOC(strlen(cMediaCapturePathStatuses) + 1, 1);
			memcpy(mBuff->data, cMediaCapturePathStatuses, strlen(cMediaCapturePathStatuses));
			mBuff->data2 = DBG_CALLOC(strlen(cMediaBackUpPathStatuses) + 1, 1);
			memcpy(mBuff->data2, cMediaBackUpPathStatuses, strlen(cMediaBackUpPathStatuses));				
			mBuff->uidata[0] = uiCaptureFileSizeStatuses;
			mBuff->uidata[1] = uiCaptureTimeLimitStatuses;
			mBuff->uidata[2] = ucBackUpOrderLmtStatuses;
			mBuff->uidata[3] = ucBackUpModeStatuses;
			mBuff->uidata[4] = ucCaptEnabledStatuses;
			mBuff->uidata[5] = uiBackUpOrderWaitCnclStatuses;
			mBuff->uidata[6] = uiBackUpOrderWaitMessStatuses;
			mBuff->void_data2 = NULL;
			if (ucBackUpOrderEnableStatuses) 
			{
				mBuff->void_data2 = DBG_MALLOC(32);
				memcpy(mBuff->void_data2, ucBackUpOrderAddrStatuses, 32);
			}
			i = strlen(ucCaptureStartPathStatuses);
			mBuff->void_data3 = DBG_MALLOC(i + 1);
			memset(mBuff->void_data3, 0, i + 1);
			memcpy(mBuff->void_data3, ucCaptureStartPathStatuses, i);
			
			pthread_attr_init(&tattrCardReader);   
			pthread_attr_setdetachstate(&tattrCardReader, PTHREAD_CREATE_DETACHED);
			pthread_create(&threadCardReader, &tattrCardReader, CardReader, (void*)mBuff);
			//dbgprintf(3,"Started card control service\n");	
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);					
		////////////Launch module card controller//////////
		
		siArchive = AddCaptureStream(CAPTURE_TYPE_ARCHIVE, 0, 0);
		
		SendUDPMessage(TYPE_MESSAGE_DEVICE_STARTED, (char*)&uiSystemID, sizeof(uiSystemID), NULL, 0, NULL);
		
		dbgprintf(3,"INIT SERVICES DONE\n");	
		
		DBG_MUTEX_LOCK(&system_mutex);
		if ((uiCaptureMinFreeSpaceFull != 0) && (uiCaptureMinFreeSpaceFull < 100)) uiCaptureMinFreeSpaceFull = 300;
		if ((uiCaptureMinFreeSpaceSlow != 0) && (uiCaptureMinFreeSpaceSlow < 100)) uiCaptureMinFreeSpaceSlow = 300;
		if ((uiCaptureMinFreeSpaceDiff != 0) && (uiCaptureMinFreeSpaceDiff < 100)) uiCaptureMinFreeSpaceDiff = 300;
		if ((uiCaptureMinFreeSpaceAudio != 0) && (uiCaptureMinFreeSpaceAudio < 100)) uiCaptureMinFreeSpaceAudio = 300;
		if ((uiCaptureMinFreeSpaceStatuses != 0) && (uiCaptureMinFreeSpaceStatuses < 100)) uiCaptureMinFreeSpaceStatuses = 100;
		if ((uiCaptureMinFreeSpaceEvents != 0) && (uiCaptureMinFreeSpaceEvents < 100)) uiCaptureMinFreeSpaceEvents = 100;
		if ((uiCaptureMinFreeSpaceActions != 0) && (uiCaptureMinFreeSpaceActions < 100)) uiCaptureMinFreeSpaceActions = 100;
		
		if ((uiBackUpMinFreeSpaceFull != 0) && (uiBackUpMinFreeSpaceFull < 100)) uiBackUpMinFreeSpaceFull = 300;
		if ((uiBackUpMinFreeSpaceSlow != 0) && (uiBackUpMinFreeSpaceSlow < 100)) uiBackUpMinFreeSpaceSlow = 300;
		if ((uiBackUpMinFreeSpaceDiff != 0) && (uiBackUpMinFreeSpaceDiff < 100)) uiBackUpMinFreeSpaceDiff = 300;
		if ((uiBackUpMinFreeSpaceAudio != 0) && (uiBackUpMinFreeSpaceAudio < 100)) uiBackUpMinFreeSpaceAudio = 300;
		if ((uiBackUpMinFreeSpaceStatuses != 0) && (uiBackUpMinFreeSpaceStatuses < 100)) uiBackUpMinFreeSpaceStatuses = 100;
		if ((uiBackUpMinFreeSpaceEvents != 0) && (uiBackUpMinFreeSpaceEvents < 100)) uiBackUpMinFreeSpaceEvents = 100;
		if ((uiBackUpMinFreeSpaceActions != 0) && (uiBackUpMinFreeSpaceActions < 100)) uiBackUpMinFreeSpaceActions = 100;
		
		if ((uiArchiveMinFreeSpaceFull != 0) && (uiArchiveMinFreeSpaceFull < 100)) uiArchiveMinFreeSpaceFull = 300;
		if ((uiArchiveMinFreeSpaceSlow != 0) && (uiArchiveMinFreeSpaceSlow < 100)) uiArchiveMinFreeSpaceSlow = 300;
		if ((uiArchiveMinFreeSpaceDiff != 0) && (uiArchiveMinFreeSpaceDiff < 100)) uiArchiveMinFreeSpaceDiff = 300;
		if ((uiArchiveMinFreeSpaceAudio != 0) && (uiArchiveMinFreeSpaceAudio < 100)) uiArchiveMinFreeSpaceAudio = 300;
		if ((uiArchiveMinFreeSpaceStatuses != 0) && (uiArchiveMinFreeSpaceStatuses < 100)) uiArchiveMinFreeSpaceStatuses = 100;
		if ((uiArchiveMinFreeSpaceEvents != 0) && (uiArchiveMinFreeSpaceEvents < 100)) uiArchiveMinFreeSpaceEvents = 100;
		if ((uiArchiveMinFreeSpaceActions != 0) && (uiArchiveMinFreeSpaceActions < 100)) uiArchiveMinFreeSpaceActions = 100;
		
		if (ucMediaArchiveTimeFromFull > 2359) ucMediaArchiveTimeFromFull = 2359;
		if (ucMediaArchiveTimeToFull > 2359) ucMediaArchiveTimeToFull = 2359;
		if (ucMediaArchiveTimeFromSlow > 2359) ucMediaArchiveTimeFromSlow = 2359;
		if (ucMediaArchiveTimeToSlow > 2359) ucMediaArchiveTimeToSlow = 2359;
		if (ucMediaArchiveTimeFromDiff > 2359) ucMediaArchiveTimeFromDiff = 2359;
		if (ucMediaArchiveTimeToDiff > 2359) ucMediaArchiveTimeToDiff = 2359;
		if (ucMediaArchiveTimeFromAudio > 2359) ucMediaArchiveTimeFromAudio = 2359;
		if (ucMediaArchiveTimeToAudio > 2359) ucMediaArchiveTimeToAudio = 2359;
		if (ucMediaArchiveTimeFromStatuses > 2359) ucMediaArchiveTimeFromStatuses = 2359;
		if (ucMediaArchiveTimeToStatuses > 2359) ucMediaArchiveTimeToStatuses = 2359;
		if (ucMediaArchiveTimeFromEvents > 2359) ucMediaArchiveTimeFromEvents = 2359;
		if (ucMediaArchiveTimeToEvents > 2359) ucMediaArchiveTimeToEvents = 2359;
		if (ucMediaArchiveTimeFromActions > 2359) ucMediaArchiveTimeFromActions = 2359;
		if (ucMediaArchiveTimeToActions > 2359) ucMediaArchiveTimeToActions = 2359;
		DBG_MUTEX_UNLOCK(&system_mutex);
		
		//if (ucBackUpModeFull == 1) iBackUpSpaceOrderMax++;
		//if (ucBackUpModeSlow == 1) iBackUpSpaceOrderMax++;
		//if (ucBackUpModeDiff == 1) iBackUpSpaceOrderMax++;
		
		DBG_MUTEX_LOCK(&modulelist_mutex);	
		cnt = ModuleTypeToNum(MODULE_TYPE_SYSTEM, 1);
		if (cnt >= 0) ret = miModuleList[cnt].ID;
		DBG_MUTEX_UNLOCK(&modulelist_mutex);	
		AddModuleEventInList(ret, 15, SYSTEM_CMD_EVENT_START, NULL, 0, 0);
	}
	
	strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
	dbgprintf(3 | 128,"Now time '%s'\n", timebuff);
	
	FS_GROUP *fsFSGroup = NULL;
	unsigned int fsFSGroupCount = 0;
	
	DBG_MUTEX_LOCK(&system_mutex);
	if (ucFastCaptVid || (ucCaptAud & 1))
								FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathFull, uiCaptureMinFreeSpaceFull);
	if (ucSlowCaptVid) 			FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathSlow, uiCaptureMinFreeSpaceSlow);
	if (ucDiffCaptVid)	 		FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathDiff, uiCaptureMinFreeSpaceDiff);
	if (ucCaptAud & 2) 			FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathAudio, uiCaptureMinFreeSpaceAudio);
	if (ucCaptEnabledStatuses) 	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathStatuses, uiCaptureMinFreeSpaceStatuses);
	if (ucCaptEnabledEvents) 	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathEvents, uiCaptureMinFreeSpaceEvents);
	if (ucCaptEnabledActions) 	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cMediaCapturePathActions, uiCaptureMinFreeSpaceActions);
	
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathFull, uiBackUpMinFreeSpaceFull);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathSlow, uiBackUpMinFreeSpaceSlow);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathDiff, uiBackUpMinFreeSpaceDiff);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathAudio, uiBackUpMinFreeSpaceAudio);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathStatuses, uiBackUpMinFreeSpaceStatuses);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathEvents, uiBackUpMinFreeSpaceEvents);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalBackUpPathActions, uiBackUpMinFreeSpaceActions);
	
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathFull, uiArchiveMinFreeSpaceFull);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathSlow, uiArchiveMinFreeSpaceSlow);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathDiff, uiArchiveMinFreeSpaceDiff);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathAudio, uiArchiveMinFreeSpaceAudio);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathStatuses, uiArchiveMinFreeSpaceStatuses);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathEvents, uiArchiveMinFreeSpaceEvents);
	FillFsGroup(&fsFSGroup, &fsFSGroupCount, cLocalArchivePathActions, uiArchiveMinFreeSpaceActions);	
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	unsigned int uiLastMemUse = DBG_MAX_MEM();
	unsigned int uiEvent;
	
	unsigned int uiMemVar;
	unsigned int  uiFreeSpaceWait = 0;
	
	unsigned int uiTimeUpdateTimeout = 1000;
	unsigned int uiTimeSaveTimeout = 0;
	unsigned int timer_request_datetime = 0;
	
	struct tm archive_time;
	time(&rawtime);
	localtime_r(&rawtime, &archive_time);
	
	while (1)
	{
		if (iSafeFlag == 0)
		{
			if (uiDeviceType & DEVICE_TYPE_CLOCK)
			{
				uiTimeSaveTimeout++;
				if (uiTimeSaveTimeout >= 240)
				{
					uiTimeSaveTimeout = 0;
					ret = 0;
					DBG_MUTEX_LOCK(&system_mutex);
					if ((cNTPServer[0] != 0) && (ucTimeUpdated == 2)) ret = 1;					
					DBG_MUTEX_UNLOCK(&system_mutex);
					if (ret) i2c_write_timedate3231(I2C_ADDRESS_CLOCK);
				}
			}
				
			uiTimeUpdateTimeout++;
			if (uiTimeUpdateTimeout >= 4)
			{
				uiTimeUpdateTimeout = 0;
				
				char cTimeDone = 0;
				ret = 0;
				DBG_MUTEX_LOCK(&system_mutex);
				if ((cNTPServer[0] == 0) && cDateTimeReference && (ucTimeUpdated != 2)) ucTimeUpdated = 2;
				
				cTimeDone = ucTimeUpdated;
				if ((cNTPServer[0] != 0) && (ucTimeUpdated != 2))
				{
					char cntppath[64];
					memcpy(cntppath, cNTPServer, 64);
					DBG_MUTEX_UNLOCK(&system_mutex);
					time_t rawt;	
					if (GetNtpServerTime(cntppath, &rawt))
					{
						stime(&rawt);
						DBG_MUTEX_LOCK(&system_mutex);
						ucTimeUpdated = 2;
						cTimeDone = ucTimeUpdated;
						DBG_MUTEX_UNLOCK(&system_mutex);
						ret = 1;
						char cTimeStr[64];
						dbgprintf(3, "Updated DateTime from NTP: '%s'\n", GetCurrDateTimeStr(cTimeStr, 64));
					} else dbgprintf(3, "Error load Date from : '%s'\n", cntppath);
				} else DBG_MUTEX_UNLOCK(&system_mutex);	
				
				if (ret && (uiDeviceType & DEVICE_TYPE_CLOCK)) i2c_write_timedate3231(I2C_ADDRESS_CLOCK);
			
				timer_request_datetime++;
				
				if ((cTimeDone != 2) || ((ret == 0) && (timer_request_datetime >= 5)))
				{
					timer_request_datetime = 0;
					ret = 0;
					struct sockaddr_in 	sTimeAddress;
					DBG_MUTEX_LOCK(&modulelist_mutex);
					for (i = 0; i < iModuleCnt; i++) 
						if ((miModuleList[i].Enabled & 1) && (miModuleList[i].Type == MODULE_TYPE_SYSTEM) && miModuleList[i].Settings[0])
						{
							ret = 1;
							memcpy(&sTimeAddress, &miModuleList[i].Address, sizeof(sTimeAddress));
							break;
						}
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					if (ret) SendTCPMessage(TYPE_MESSAGE_REQUEST_DATETIME, NULL, 0, NULL, 0, &sTimeAddress);
				}
			}
			
			//if (iDetectLocalAddr != 1) iDetectLocalAddr = GetLocalAddr(&miSystemList[0].Address);

			DBG_MUTEX_LOCK(&systemlist_mutex);
			for (cnt = 0; cnt < iSystemListCnt; cnt++)
			{
				if (miSystemList[cnt].Active == 0)
				{
					dbgprintf(4,"SYSTEM LOSTED: %s\n", inet_ntoa(miSystemList[cnt].Address.sin_addr));
					ret = miSystemList[cnt].ID;
					DBG_MUTEX_UNLOCK(&systemlist_mutex);
					AddModuleEventInList(ret, 15, SYSTEM_CMD_LOST, NULL, 0, 0);
					ClearSecurityKey(miSystemList[cnt].ID);
					ClearModuleList(miSystemList[cnt].ID);
					ClearSystemList(miSystemList[cnt].ID);
					DBG_MUTEX_LOCK(&modulelist_mutex);
					SortModules(miModuleList, iModuleCnt);
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					DBG_MUTEX_LOCK(&systemlist_mutex);													
				}
				else 
				{
					if (miSystemList[cnt].Local == 0) 
					{
						if (miSystemList[cnt].Active != MAX_SYSTEM_INFO_LIVE) dbgprintf(5,"SYSTEM LOSTING(%i): %s\n", miSystemList[cnt].Active - 1, inet_ntoa(miSystemList[cnt].Address.sin_addr));
						miSystemList[cnt].Active--;						
					}
					else
					{
						if (uiDevType & DEVICE_TYPE_VIDEO_OUT)
						{
							if (iShowerLiveControl == 0)
							{
								dbgprintf(1,"VIDEO SYSTEM LOSTED: %s\n", inet_ntoa(miSystemList[cnt].Address.sin_addr));
								DBG_MUTEX_PRINT_LOCKED(0, 1); 
								dbg_death_signal(SIGKILL);
							}
							if (iShowerLiveControl) iShowerLiveControl--;
						}
					}
				}
			}
			DBG_MUTEX_UNLOCK(&systemlist_mutex);
			
			//dbgprintf(5, "SEND SYSTEM SIGNAL\n");
									
			SendUDPMessage(TYPE_MESSAGE_SYSTEM_SIGNAL, (char*)&uiSystemID, sizeof(uiSystemID), NULL, 0, NULL);
			
			if (uiFreeSpaceWait == 0)
			{
				ClearSpace(fsFSGroup, fsFSGroupCount);
				SendFilesToArchive(siArchive, &archive_time);
				uiFreeSpaceWait = TIMEWAIT_FREE_SPACE_REPEAT;
			} else uiFreeSpaceWait--;							
		}
		
		uiEvent = 0;
		ret = tx_eventer_recv(&main_thread_evnt, &uiEvent, 15000, 0);
		if ((ret != 0) && (uiEvent == EVENT_STOP)) break;
		
		//SendDbgUDPMessage(TYPE_MESSAGE_TEXT, "Testing", sizeof("Testing\0"), "127.0.0.1");
		DBG_MUTEX_PRINT_LOCKED(100, 0);
		dbg_mem_test_allocated(0);
		uiMemVar = DBG_MAX_MEM()/1024;
		if ((uiMemVar > 200000) && (uiMemVar > uiLastMemUse))
		{
			dbgprintf(2, "Detect more memory use: %iMB (%iMB)\n", uiMemVar, uiMemVar - uiLastMemUse);
			dbg_mem_print_allocated(3);
			uiLastMemUse = uiMemVar;
			uiMemVar = DBG_USE_MEM();
			if (uiMemVar > 250000) 
			{
				dbgprintf(2, "Detect big memory use: %iMB\n", uiMemVar);
				dbg_mem_print_allocated(2);
			}
		}
		//dbg_mem_print_allocated();
		//dbg_log_mem_short();
		//dbg_mem_print_allocated_short();
	}
	
	dbgprintf(3,"Shutdown AZAD (main)\n");	
	/*if (cThreadMainStatus == 6)
	{
		dbgprintf(3, "Closing WEB threads\n");
		web_stop();
		printf("\nRestart in emergency mode\n");
		system("./azad debug safe &");	
		exit(2);
	}*/
	char cExeStr[256];
			
	if (cRebootAfterUpdate)
	{
		if ((cThreadMainStatus == 7) && (cNewSourcePath[0] != 0)) //Update
		{
			memset(cExeStr, 0, 256);
			snprintf(cExeStr, 255, "./updater 1 %s %s %s %s &", cNewSourcePath, cNewSourceLogin, cNewSourcePass, cNewSourceFile);	
			system(cExeStr);
			exit(1);
		}
	}
	
	dbgprintf(4, "Clear FS Group\n");
	if (fsFSGroupCount) FreeFsGroup(fsFSGroup, fsFSGroupCount);
	
	dbgprintf(4, "Closing Scaner thread\n");
	DBG_MUTEX_LOCK(&system_mutex);
	if (cThreadScanerStatus) cThreadScanerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadScanerStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 0) usleep(50000);
	} while(ret);
	
	dbgprintf(4, "Closing Shower thread\n");
	DBG_MUTEX_LOCK(&system_mutex);
	if (cThreadShowerStatus == 1) cThreadShowerStatus = 0; else cThreadShowerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadShowerStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 2) usleep(50000);
	} while(ret != 2);
	
	dbgprintf(4, "Closing Eventer thread\n");
	DBG_MUTEX_LOCK(&system_mutex);
	if (cThreadEventerStatus == 1) 
	{
		tx_eventer_send_event(&modevent_evnt, EVENT_STOP);
		cThreadEventerStatus = 0; 
	}
	else cThreadEventerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadEventerStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 2) usleep(50000);
	} while(ret != 2);
	
	dbgprintf(4, "Closing Streamer thread\n");
	DBG_MUTEX_LOCK(&system_mutex);
	if (cThreadStreamerStatus == 1) 
	{
		tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_EXIT);
		cThreadStreamerStatus = 0;
	}
	else cThreadStreamerStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadStreamerStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 2) usleep(50000);
	} while(ret != 2);
	
	dbgprintf(4, "Closing Reader thread\n");
	DBG_MUTEX_LOCK(&system_mutex);
	if (cThreadReaderStatus == 1) cThreadReaderStatus = 0; else cThreadReaderStatus = 2;
	DBG_MUTEX_UNLOCK(&system_mutex);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadReaderStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 2) usleep(50000);
	} while(ret != 2);
	
	dbgprintf(4, "Closing Finger threads\n");
	DBG_MUTEX_LOCK(&system_mutex);
	if (cThreadFingerRun == 1) cThreadFingerRun = 0;
	DBG_MUTEX_UNLOCK(&system_mutex);
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = cThreadFingerStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret > 0) usleep(50000);
	} while(ret > 0);
	
	if ((uiErrorVideoRestart != 100) || (uiErrorCameraRestart != 100) || (uiErrorAudioRestart != 100))
	{
		dbgprintf(4, "Closing Plays threads\n");	
		Media_StopPlay(0);
		Media_StopPlay(1);
		omx_stop_video(0);
		Audio_Stop(0);	
		omx_stop_video(1);
		Audio_Stop(1);
	
		dbgprintf(4, "Closing VideoAudioRecord thread\n");	
		Media_StopRec(1);
		dbgprintf(4, "Closing VideoCapture thread\n");	
		omx_stop_capture(1);
		dbgprintf(4, "Closing OMX\n");	
		omx_Stop();
		dbgprintf(4, "Closing AudioCapture thread\n");	
		Audio_StopCapture(1);
		dbgprintf(4, "Closing Audio\n");	
		audio_close();
		dbgprintf(4, "Closing GLText\n");	
		DeInitGLText();		
	}	
	dbgprintf(4, "Closing ONVIF threads\n");
	onvif_stop();
	dbgprintf(4, "Closing RTSP threads\n");
	rtsp_stop();
	dbgprintf(4, "Closing WEB threads\n");
	if (web_stop()) 
	{
		ReleaseMenu(pWebMenu);    	
		if (mbManual.void_data) DBG_FREE(mbManual.void_data);
	}	
	
	dbgprintf(4, "Wait Copy threads\n");		
	do
	{
		DBG_MUTEX_LOCK(&system_mutex);
		ret = uiThreadCopyStatus;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);	
	
	dbgprintf(4, "Closing Writer thread\n");		
	CloseWriter();
	
	dbgprintf(4, "Closing LAN threads\n");
	if (iSafeFlag == 0) DelLAN();
	
	dbgprintf(4, "Closing Capture Streams\n");
	ReleaseCaptureStreams();
	
	dbgprintf(4, "Closing RDA5807M\n");
	if (uiDevType & DEVICE_TYPE_RADIO) RDA5807M_term();
	DBG_MUTEX_LOCK(&modulelist_mutex);					
	for (i = 0; i < iModuleCnt; i++)
	{
		if ((miModuleList[i].Enabled & 1) && miModuleList[i].Local)
		{	
			dbgprintf(4, "Closing module: %.4s\n", (char*)&miModuleList[i].ID);	
			if (miModuleList[i].Type == MODULE_TYPE_RC522) MFRC522_deinit(miModuleList[i].InitParams[0]);
			if (miModuleList[i].Type == MODULE_TYPE_PN532) PN532_deinit(miModuleList[i].InitParams[0]); 
			if (miModuleList[i].Type == MODULE_TYPE_RS485) uart_close_port(miModuleList[i].InitParams[0]);
			if (miModuleList[i].Type == MODULE_TYPE_EXTERNAL) external_close(&miModuleList[i]);
			if (miModuleList[i].Type == MODULE_TYPE_TFP625A) TFP625A_close(&miModuleList[i]);
			if ((miModuleList[n].Type == MODULE_TYPE_TEMP_SENSOR) ||
				(miModuleList[n].Type == MODULE_TYPE_ADS1015) ||
				(miModuleList[n].Type == MODULE_TYPE_MCP3421) ||
				(miModuleList[n].Type == MODULE_TYPE_AS5600) ||
				(miModuleList[n].Type == MODULE_TYPE_HMC5883L))
														i2c_close(miModuleList[i].InitParams[0]);				
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	dbgprintf(4, "Closing I2C\n");
	if (uiDevType & DEVICE_TYPE_I2C) i2c_deinit();	
	dbgprintf(4, "Closing GPIO\n");
	if (uiDevType & DEVICE_TYPE_GPIO) gpio_close();   
	dbgprintf(4, "Closing USB GPIO\n");
	if (uiDevType & DEVICE_TYPE_USB_GPIO) 
	{
		DBG_MUTEX_LOCK(&modulelist_mutex);					
		for (i = 0; i < iModuleCnt; i++)
		{
			if ((miModuleList[i].Enabled & 1) && (miModuleList[i].Type == MODULE_TYPE_USB_GPIO) && miModuleList[i].Local)
					usb_gpio_close(&miModuleList[i]);
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);		
	}	
	
	if (uiDeviceType & (DEVICE_TYPE_GPIO | DEVICE_TYPE_SPI | DEVICE_TYPE_I2C))
	{
		if (!bcm2835_close()) dbgprintf(2,"Error close gpio\n");
	}

	dbgprintf(4, "Closing IR RECV\n");
	if (uiDevType & DEVICE_TYPE_IR_RECEIVER) ir_close();
	dbgprintf(4, "Clear SystemList\n");	
	DBG_FREE(miSystemList);
	dbgprintf(4, "Clear RadioStation\n");	
	if (riRadioStation) DBG_FREE(riRadioStation);
	dbgprintf(4, "Clear UserList\n");	
	if (uiUserList) DBG_FREE(uiUserList);
	dbgprintf(4, "Clear DirList\n");	
	if (diDirList) DBG_FREE(diDirList);
	dbgprintf(4, "Clear RectangleList\n");	
	if (riRectangleList) DBG_FREE(riRectangleList);
	dbgprintf(4, "Clear ActionManual\n");	
	if (amiActionManual) DBG_FREE(amiActionManual);
	dbgprintf(4, "Clear ActionInfo\n");	
	if (maActionInfo) DBG_FREE(maActionInfo);
	dbgprintf(4, "Clear IRCommandList\n");	
	if (mIRCommandList) DBG_FREE(mIRCommandList);
	dbgprintf(4, "Clear SecurityKeys\n");	
	if (skiSecurityKeys) DBG_FREE(skiSecurityKeys);
	dbgprintf(4, "Clear WidgetList\n");	
	if (wiWidgetList) DBG_FREE(wiWidgetList);	
	dbgprintf(4, "Clear StreamType\n");	
	if (stStreamType) DBG_FREE(stStreamType);
	dbgprintf(4, "Clear InternetRadio\n");	
	if (irInternetRadio) DBG_FREE(irInternetRadio);
	dbgprintf(4, "Clear MailList\n");	
	if (miMailList) DBG_FREE(miMailList);
	dbgprintf(4, "Clear SoundList\n");	
	if (iSoundListCnt)
	{
		for(n = 0; n < iSoundListCnt; n++) if (mSoundList[n].Data) DBG_FREE(mSoundList[n].Data);
		DBG_FREE(mSoundList);
	}
	dbgprintf(4, "Clear AlarmClockInfo\n");	
	if (actAlarmClockInfo) DBG_FREE(actAlarmClockInfo);
	dbgprintf(4, "Clear ModuleList\n");	
	if (miModuleList) DBG_FREE(miModuleList);
	dbgprintf(4, "Clear ModuleEventList\n");	
	if (iModuleEventCnt) DBG_FREE(meModuleEventList);
	dbgprintf(4, "Clear SkipIrCodeList\n");	
	if (iSkipIrCodeListCnt) DBG_FREE(cSkipIrCodeList);
	dbgprintf(4, "Clear SkipEventList\n");	
	if (cSkipEventList) DBG_FREE(cSkipEventList);
	dbgprintf(4, "Clear AlienKeyList\n");	
	if (iAlienKeyListCnt) DBG_FREE(cAlienKeyList);
	dbgprintf(4, "Clear PTZList\n");	
	if (iPtzSettingsListCnt) DBG_FREE(psiPtzSettingsList);
	dbgprintf(4, "Clear ListFiles\n");	
	if (cListFiles) DBG_FREE(cListFiles);
	dbgprintf(4, "Clear ListDirs\n");	
	if (cListDirs) DBG_FREE(cListDirs);	
	dbgprintf(4, "Clear ListAlarmFiles\n");	
	if (cListAlarmFiles) DBG_FREE(cListAlarmFiles);
	dbgprintf(4, "Clear ConnectsHistory\n");	
	if (whWebHistory) DBG_FREE(whWebHistory);
	dbgprintf(4, "Clear ClearMessageList\n");	
	Menu_ClearMessageList(NULL, 0);
	dbgprintf(4, "Clear ClearRemoteFileList\n");	
	if (rsiRemoteStatus.FileCnt) DBG_FREE(rsiRemoteStatus.FileList);
	if (rsiRemoteStatus.NewFileCnt) DBG_FREE(rsiRemoteStatus.NewFileList);
	dbgprintf(4, "Closing GL\n");		
	Exit_Func();
	
	dbgprintf(4, "Closing MISC\n");
	if (uiDevType & DEVICE_TYPE_VIDEO_OUT) 
	{
		pthread_attr_destroy(&tattrShower);
		//pthread_attr_destroy(&tattrPlayMedia);
		if (iSafeFlag == 0) ReleaseMenu(pScreenMenu);
	}
	if (cCaptureFilesView || cBackUpFilesView) pthread_attr_destroy(&tattrStreamer);
	if (cFileWriterService) pthread_attr_destroy(&tattrWriter);
	if (uiDevType & DEVICE_TYPE_VIDEO_IN) pthread_attr_destroy(&tattrVideoCapture);
	if (cScannerEnabled) pthread_attr_destroy(&tattrScaner);
	if (cFingerEnabled) pthread_attr_destroy(&tattrFinger);
	pthread_attr_destroy(&tattrEventer);
	pthread_attr_destroy(&tattrFileIO);
	
	if (uiDevType & (DEVICE_TYPE_RC522 | DEVICE_TYPE_PN532)) pthread_attr_destroy(&tattrCardReader);

	dbgprintf(4, "Closing Mutexes\n");	
	pthread_mutex_destroy(&ircode_mutex);
	pthread_mutex_destroy(&rectangle_mutex);
	pthread_mutex_destroy(&modulelist_mutex);
	pthread_mutex_destroy(&securitylist_mutex);
	pthread_mutex_destroy(&systemlist_mutex);	
	pthread_mutex_destroy(&modevent_mutex);
	pthread_mutex_destroy(&system_mutex);
	pthread_mutex_destroy(&stream_mutex);
	pthread_mutex_destroy(&widget_mutex);	
	pthread_mutex_destroy(&message_mutex);
	pthread_mutex_destroy(&evntaction_mutex);
	pthread_mutex_destroy(&mnlaction_mutex);
	pthread_mutex_destroy(&alienkey_mutex);
	pthread_mutex_destroy(&ptz_mutex);
	pthread_mutex_destroy(&skipevent_mutex);
	pthread_mutex_destroy(&skipircode_mutex);
	pthread_mutex_destroy(&user_mutex);
	pthread_mutex_destroy(&Network_Mutex);	
	pthread_mutex_destroy(&dyspcont_mutex);
	
	tx_eventer_delete(&modevent_evnt);
	tx_eventer_delete(&strmevent_evnt);
	tx_eventer_delete(&writeevent_evnt);
	tx_eventer_delete(&main_thread_evnt);
	
	//vc_gencmd_stop();
	
	DBG_LOG_OUT();
	
	dbg_func_print_entered();
	dbg_mem_print_allocated(2);
	
	dbgprintf(4, "Closing VCHI\n");	
	//bcm_host_deinit();
	vc_dispmanx_stop();
	vc_vchi_tv_stop();
	vc_vchi_cec_stop();
	vc_gencmd_stop();
	//vchi_exit();
	
	DBG_CLOSE();	
	
	if ((cThreadMainStatus == 7) && (cNewSourcePath[0] != 0)) //Update 
	{
		memset(cExeStr, 0, 256);
		snprintf(cExeStr, 255, "./updater 0 %s %s %s %s &", cNewSourcePath, cNewSourceLogin, cNewSourcePass, cNewSourceFile);	
		system(cExeStr);
	}
	
	memset(cExeStr, 0, 256);
	strcpy(cExeStr, "./azad");
	
	if (cThreadMainStatus == 3) strcat(cExeStr, " pause");
	if (cThreadMainStatus == 5) strcat(cExeStr, " pause");
	if (cThreadMainStatus == 6) strcat(cExeStr, " reset");
	if (iDebugFlag) strcat(cExeStr, " debug");
	if (iNoDieFlag) strcat(cExeStr, " nodie");
	strcat(cExeStr, " &");
	
	if (cThreadMainStatus == 1) system("shutdown -h now &");	
	if (cThreadMainStatus == 2) system("reboot &");	
	if ((cThreadMainStatus == 3) || (cThreadMainStatus == 6)) system(cExeStr);
	if (cThreadMainStatus == 5)
	{
		while (cExternalApp[strlen(cExternalApp) - 1] == 38) cExternalApp[strlen(cExternalApp) - 1] = 0;
		printf("Execute external app with wait: '%s'\n", cExternalApp);
		system(cExternalApp);
		system(cExeStr);
	}
	/*if (cThreadMainStatus == 3) 
	{
		if (fork() == 0) execl("azad", "", "pause", NULL);
	}*/
	//printf("gggg\n");
	return cThreadMainStatus;
}

