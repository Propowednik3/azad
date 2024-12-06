#ifndef _WEB_H_
#define _WEB_H_

#include "audio.h"
#include "debug.h"

#define WEB_TX_BUF_SIZE_MAX		2512000
#define WEB_RX_BUF_SIZE_MAX		2512000
#define WEB_RX_BUF_SIZE_DEF		16384
#define WEB_MAX_CLIENTS			10
#define WEB_MAX_TIME_IDLE		15
#define WEB_MAX_SIZE_FILE_VIEW	5000000

#define MARK_START      	"/"
#define MARK_GET_ICO		"/favicon.ico"
#define MARK_CONTROL   		"/control/"
#define MARK_YOUTUBE   		"/youtube/"
#define MARK_MENU   		"/menu/"
#define MARK_MEDIA   		"/media/"
#define MARK_ALARMS   		"/alarms/"
#define MARK_SOUNDS   		"/sounds/"
#define MARK_MODULES   		"/modules/"
#define MARK_MAILS   		"/mails/"
#define MARK_STREAMTYPES 	"/streamtypes/"
#define MARK_STREAMS 		"/streams/"
#define MARK_WIDGETS 		"/widgets/"
#define MARK_DIRECTORIES 	"/directories/"
#define MARK_MNLACTIONS 	"/mnlactions/"
#define MARK_EVNTACTIONS 	"/evntactions/"
#define MARK_KEYS		 	"/keys/"
#define MARK_SPECIALCODES 	"/specialcodes/"
#define MARK_SKIPSPECIALCODES 	"/skipspecialcodes/"
#define MARK_SKIPEVENTS 	"/skipevents/"
#define MARK_ALIENKEYS 		"/alienkeys/"
#define MARK_CAMRECTS		"/camrects/"
#define MARK_CAMERA			"/camera/"
#define MARK_MESSAGES		"/messages/"
#define MARK_SYSTEM			"/system/"
#define MARK_CONNECTS		"/connects/"
#define MARK_MIC			"/mic/"
#define MARK_IMAGES			"/images/"
#define MARK_USERS			"/users/"
#define MARK_RADIOS			"/radios/"
#define MARK_MANUAL			"/manual/"
#define MARK_SETTINGS		"/settings/"
#define MARK_VIEWER			"/viewer/"
#define MARK_MODSTATUSES	"/modstatuses/"
#define MARK_LOGOUT			"/logout/"
#define MARK_LOG			"/log/"
#define MARK_EXPLORER		"/explorer/"
#define MARK_HISTORY		"/history/"
#define MARK_GET_INFO  		"/get_info"
#define MARK_PLAY    		"/play"
#define MARK_PAUSE    		"/pause"
#define MARK_REQUEST_ID 	"req_index"
#define MARK_IMAGE_CAMRECT	"/image_camrect"
#define MARK_IMAGE_CAMPREV	"/image_camprev"
#define MARK_IMAGE_CAMMAIN	"/image_cammain"
#define MARK_NEXT   		"/next"
#define MARK_PREV    		"/prev"
#define MARK_STOP    		"/stop"
#define MARK_SHOW  			"/show"
#define MARK_SAVE  			"/save"
#define MARK_PAGE  			"/page"
#define MARK_OPEN  			"/open"
#define MARK_LOAD  			"/load"
#define MARK_FILTER			"/filters"
#define MARK_RESTART   		"/restart"
#define MARK_REBOOT    		"/reboot"
#define MARK_OFF    		"/shutdown"
#define MARK_CLOSE    		"/close"
#define MARK_EXIT    		"/exit"
#define MARK_GET_UP    		"/get_up"
#define MARK_GET_DOWN  		"/get_down"
#define MARK_WORK_REQUEST 	"/work_request"
#define MARK_WORK_ACCEPT 	"/work_accept"
#define MARK_CHANGE			"/change"
#define MARK_REFRESH		"/refresh"
#define MARK_ADD  			"/add"
#define MARK_LIST			"/list"
#define MARK_DELETE			"/delete"
#define MARK_EXECUTE		"/execute"
#define MARK_RECOVER		"/recover"
#define MARK_SET_VOLUME		"/set_volume"
#define MARK_SET_PLAY_POS  	"/set_play_pos"
#define MARK_UPDATE			"/update"

#define YOUTUBE_SERVER	"www.youtube.com"

typedef enum
{
    WEB_MSG_UNKNOWN, 
	WEB_MSG_START,
	WEB_MSG_GET_ICO,
	WEB_MSG_CONTROL,
	WEB_MSG_YOUTUBE,
	WEB_MSG_MEDIA,
	WEB_MSG_ERROR,
	WEB_MSG_TEARDOWN,
	WEB_MSG_MENU,
	WEB_MSG_ALARMS,
	WEB_MSG_MODULES,
	WEB_MSG_SOUNDS,
	WEB_MSG_MAILS,
	WEB_MSG_STREAMTYPES,
	WEB_MSG_STREAMS,
	WEB_MSG_WIDGETS,
	WEB_MSG_DIRECTORIES,
	WEB_MSG_MNLACTIONS,
	WEB_MSG_EVNTACTIONS,
	WEB_MSG_KEYS,
	WEB_MSG_SPECIALCODES,
	WEB_MSG_SKIPSPECIALCODES,
	WEB_MSG_CAMRECTS,
	WEB_MSG_ALIENKEYS,
	WEB_MSG_SKIPEVENTS,
	WEB_MSG_USERS,
	WEB_MSG_RADIOS,
	WEB_MSG_LOGOUT,
	WEB_MSG_MANUAL,
	WEB_MSG_MODSTATUSES,
	WEB_MSG_SETTINGS,
	WEB_MSG_LOG,
	WEB_MSG_EXPLORER,
	WEB_MSG_VIEWER,
	WEB_MSG_IMAGES,
	WEB_MSG_CAMERA,
	WEB_MSG_MIC,
	WEB_MSG_SYSTEM,
	WEB_MSG_CONNECTS,
	WEB_MSG_HISTORY,
	WEB_MSG_MESSAGES
} WEB_MSG; 

typedef enum
{
    WEB_ACT_NOTHING,
	WEB_ACT_PLAY,
	WEB_ACT_PAUSE,
	WEB_ACT_NEXT,
	WEB_ACT_PREV,
	WEB_ACT_STOP,
	WEB_ACT_SHOW,
	WEB_ACT_PAGE,
	WEB_ACT_SAVE,	
	WEB_ACT_ADD,
	WEB_ACT_DELETE,
	WEB_ACT_RECOVER,
	WEB_ACT_EXECUTE,
	WEB_ACT_SET_VOLUME,
	WEB_ACT_SET_PLAY_POS,
	WEB_ACT_GET_INFO,
	WEB_ACT_CHANGE,
	WEB_ACT_OPEN,
	WEB_ACT_REFRESH,
	WEB_ACT_WORK_ACCEPT,
	WEB_ACT_WORK_REQUEST,
	WEB_ACT_IMAGE_CAMRECT,
	WEB_ACT_IMAGE_CAMPREV,
	WEB_ACT_IMAGE_CAMMAIN,
	WEB_ACT_OFF,
	WEB_ACT_REBOOT,
	WEB_ACT_RESTART,
	WEB_ACT_EXIT,
	WEB_ACT_CONNECT,
	WEB_ACT_DISCONNECT,
	WEB_ACT_UP,
	WEB_ACT_DOWN,
	WEB_ACT_LOAD,
	WEB_ACT_CLOSE,
	WEB_ACT_FILTER,
	WEB_ACT_UPDATE
} WEB_ACT; 

typedef struct WEB_SESSION
{
	unsigned int 	ip;
	char			ip_str[16];
	unsigned short  port;  
	unsigned short  web_port;
	char			auth;
	unsigned int 	maxtimeidle;
	unsigned int 	access;
	unsigned int 	access_level;
	char			login[64];
	char			password[64];
    int				socket;
	unsigned int	flag;
	char			media_url[256];
	unsigned int 	code;
	char			nonce[30];
	char			*head;
	unsigned int	request_id;
	unsigned int	*p_request_id;
	char			display_enabled;
	char			speaker_enabled;
} WEB_SESSION;

typedef struct WEB_CLIENT_LIST
{
	unsigned int 	ip;
	char			ip_str[30];
	unsigned short  port;  
	int64_t			prev_time;
	unsigned int 	code;
	char			nonce[30];
	unsigned int 	status;
	unsigned int 	request_id;
} WEB_CLIENT_LIST;

typedef struct WEB_HISTORY_INFO
{
	char			Date[32];
	int				Prot;
	char			Addr[32];
	unsigned int	Type;
	unsigned int	Action;
	int				Auth;
	char			Login[96];
} WEB_HISTORY_INFO;

extern pthread_mutex_t WEB_mutex;
extern pthread_mutex_t WEB_req_mutex;
extern pthread_mutex_t WEB_explorer_mutex;
extern int iWebHistoryCnt;
extern WEB_HISTORY_INFO *whWebHistory;

void web_start(unsigned int uiPort);
int web_stop();
char WEB_test_youtube_content_H264(unsigned int itag);
void WEB_ClearMessageList();
void WEB_AddMessageInList(char *cMessage, ...);
void WEB_ClearWebHistory();
void WEB_AddWebHistory(int iProt, char *cAddr, unsigned int uiType, unsigned int uiAct, int Auth, char *cLogin);
char* WEB_get_AudioCodecName(unsigned int uiCodecNum);

#endif