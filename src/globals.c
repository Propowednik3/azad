#include "pthread2threadx.h"
#include "version.h"
#include "omx_client.h"

char* fullVersion = azad_VERSION;

image_sensor_params ispCameraImageSettings;	
TX_SEMAPHORE 	psem_omx_sync;
TX_SEMAPHORE 	psem_omx_run;
int text_id;
char cThreadOmxPlayStatus;
char cThreadOmxImageStatus;
char cThreadOmxCaptureStatus;
char cThreadOmxEncoderStatus;
pthread_mutex_t OMX_mutex;
int	omx_resource_priority;
int	omx_speed_play;

#include "network.h"

int iMessagesListCnt;
char *cMessagesList;
char iMessageListChanged;
unsigned int 		Connects_Count;
unsigned int 		Connects_Max_Active;
CONNECT_INFO 		*Connects_Info;
unsigned int iAlarmEvents;
unsigned int uiRecvDataCnt;
unsigned int uiSendDataCnt;

#include "main.h"

unsigned char ucTimeUpdated;
unsigned int iAccessLevel;
unsigned int iDefAccessLevel;

pthread_mutex_t modevent_mutex;
pthread_mutex_t system_mutex;
pthread_mutex_t stream_mutex;
pthread_mutex_t widget_mutex;
pthread_mutex_t message_mutex;
pthread_mutex_t modulelist_mutex;
pthread_mutex_t rectangle_mutex;
pthread_mutex_t specialcode_mutex;
pthread_mutex_t evntaction_mutex;
pthread_mutex_t mnlaction_mutex;
pthread_mutex_t alienkey_mutex;
pthread_mutex_t ptz_mutex;
pthread_mutex_t skipevent_mutex;
pthread_mutex_t skipspecialcode_mutex;
pthread_mutex_t securitylist_mutex;
pthread_mutex_t systemlist_mutex;
pthread_mutex_t user_mutex;
pthread_mutex_t Network_Mutex;
pthread_mutex_t WEB_mutex;
pthread_mutex_t dyspcont_mutex;

TX_EVENTER modevent_evnt;
TX_EVENTER strmevent_evnt;
TX_EVENTER main_thread_evnt;

//char cMediaFileName[256];
//char cMediaFileNameSlow[256];	
int iSystemListCnt;
SYSTEM_INFO *miSystemList;

int iModuleEventCnt;
MODULE_EVENT *meModuleEventList;

unsigned int iSoundListCnt;
SYS_SOUND_TYPE *mSoundList;

int iModuleCnt;
MODULE_INFO *miModuleList;

int iRectangleCnt;
RECT_INFO *riRectangleList;

int iCurrentWebMenuPage;
MENU_PAGE *pWebMenu;

int iAlarmClocksCnt;
ALARM_CLOCK_TYPE *actAlarmClockInfo;

int iMailCnt;
MAIL_INFO *miMailList;

int iActionManualCnt;
ACTION_MANUAL_INFO * amiActionManual;

int iStreamTypeCnt;
STREAM_TYPE *stStreamType;

int iInternetRadioCnt;
STREAM_SETT *irInternetRadio;

int iRadioStationsCnt;
RADIO_INFO *riRadioStation;

int iWidgetsCnt;
WIDGET_INFO *wiWidgetList;

int iSkipSpecialCodeMaxCnt;
int iSkipSpecialCodeListCnt;
SPECIAL_CODE_TYPE *cSkipSpecialCodeList;

int iAlienKeyMaxCnt;
int iAlienKeyListCnt;
SECURITY_KEY_INFO *cAlienKeyList;

int iPtzSettingsListCnt;
PTZ_SET_INFO *psiPtzSettingsList;
PTZ_SET_INFO psiPtzHomeSettings;

int iSkipEventMaxCnt;
int iSkipEventListCnt;
MODULE_EVENT *cSkipEventList;
unsigned int uiSkipEventListFilter;
unsigned int uiSkipEventNumberFilter;

int iDirsCnt;
DIRECTORY_INFO *diDirList;

int iActionCnt;
ACTION_INFO *maActionInfo;	

int iSecurityKeyCnt;
SECURITY_KEY_INFO *skiSecurityKeys;

SECURITY_KEY_INFO skiUpdateKeyInfo;
int iUpdateKeyInfoAction;
int iUpdateKeyInfoResult;
unsigned int uiUpdateKeyInfoReader;
int64_t iUpdateKeyInfoTimer;

unsigned int iSpecialCodeCnt;
SPECIAL_CODE_TYPE *mSpecialCodeList;

unsigned int iUserCnt;
USER_INFO	*uiUserList;

unsigned int 	iStreamCnt;
STREAM_INFO 	**SyncStream;

int 					iDisplayContentCurrent;
unsigned int 			iDisplayContentCnt1;
unsigned int 			iDisplayContentCnt2;
unsigned int 			iDisplayContentLiveTime;
DISPLAY_CONTENT_INFO 	*dciDisplayContent1;
DISPLAY_CONTENT_INFO 	*dciDisplayContent2;

misc_buffer mbManual;

unsigned int uiMaxFileCopyTimeBreakValue;
unsigned int uiMaxFileCopyTimeMessageValue;

char cThreadMainStatus;
char cThreadScanerStatus;
unsigned int cThreadFingerStatus;
char cThreadFingerRun;
char cThreadScanerStatusUsbio;
unsigned int uiThreadCopyStatus;
unsigned int iStreamTimeMaxSet;
unsigned int iCameraTimeMaxSet;
char OnvifAuth;
char RtspAuth;
char WebAuth;
unsigned int WebMaxTimeIdle;
int iCurrentShowRectangle;
char cCameraRectFile[256];
char cCameraShotFile[256];
char cCameraSensorFile[256];
char cNextYouTubeURL[4096];
char cAlarmFile[256];
char cModuleFile[256];
char cSoundFile[256];
char cMailFile[256];
char cStreamTypeFile[256];
char cStreamFile[256];
char cPtzFile[256];
char cWidgetFile[256];
char cKeyFile[256];
char cSpecialCodeFile[256];
char cEvntActionFile[256];
char cMnlActionFile[256];
char cDirectoryFile[256];
char cCamRectangleFile[256];
char cUserFile[256];
char cManualFile[256];
char cConfigFile[256];
char cRadioFile[256];
unsigned char iRadioVolume;
unsigned int uiPaddingSize;
unsigned int uiTextColor;
float fMenuSize;
unsigned char ucBroadCastTCP;
//char cClearStatusesFiles;
//char cClearEventsFiles;
//char cClearActionsFiles;
char cCaptureFilesView;
char cBackUpFilesView;
char cCaptureFilesLevel;
char cBackUpFilesLevel;
char cCaptureFilesViewDef;
unsigned int uiMediaSlowSettings;
unsigned int uiMediaSlowFramesSkip;

char cFileLocation[256];
char cCurrentFileLocation[256];
char cFileAlarmLocation[256];
char cMailAddress[64];
char cMailServer[64];
char cMailLogin[64];
char cMailPassword[64];
char cMailAuth[64];
char cExternalApp[256];
char cNTPServer[64];
char cUdpTargetAddress[64];
char cNewSourcePath[256];
char cNewSourceFile[256];
char cNewSourceLogin[256];
char cNewSourcePass[256];
char cRebootAfterUpdate;

unsigned int uiDeviceType;
unsigned int uiOnvifStream;
unsigned int uiRTSPStream;
unsigned int uiRTSPForceAudio;
unsigned int uiOnvifPort;
unsigned int uiRTSPPort;
unsigned int uiRTPClntVidPort;
unsigned int uiRTPClntAudPort;
unsigned int uiRTPServVidPort;
unsigned int uiRTPServAudPort;	
unsigned int uiWebPort;
unsigned int uiErrorCameraRestart;
unsigned int uiErrorCameraRestartWait;
unsigned int uiErrorAudioRestart;
unsigned int uiErrorAudioRestartWait;
unsigned int uiErrorVideoRestart;
unsigned int uiErrorVideoRestartWait;
int iTimeCor;
int VSync;
unsigned int uiShowerLiveCtrlTime;
unsigned int uiShowMode;
unsigned int uiShowModeCur;
unsigned int uiTimeSndPwrOff;
int iIntervalRescanCard;

unsigned char ucCaptEnabledVideo;
unsigned char ucCaptEnabledAudio;
unsigned char ucCaptEnabledStatuses;
unsigned char ucCaptEnabledEvents;
unsigned char ucCaptEnabledActions;

int cFileWriterService;
char cWriterServicePath[MAX_PATH];


int iSlideShowTimerSet;
int iSlideShowOnTime;
int iSlideShowOffTime;
int iBasicVolume;
int iAlarmVolume;
char cSettRandomFile, cSettRandomDir, cCurRandomFile, cCurRandomDir;
char cZoom;
char cDateTimeReference;
char cAccelerateTextRender;
unsigned int uiStartMode;
unsigned int uiTerminalMenuID;
unsigned int uiMenuWidth;
unsigned int uiMenuHeight;
unsigned int uiDefAlarmSound;
unsigned int uiDefAlarmRepeats;
unsigned int uiWEBServer;
unsigned int cCurShowType;
MAIL_INFO cLogMlList;

unsigned int uiMediaIoTimeout;

char cSysStatus[64];
unsigned int uiSoundMessageID;
int64_t iSoundMessageTimer;
unsigned int uiSoundMessagePauseSize;
unsigned int uiSoundMessageVolume;

unsigned int uiFlvBufferSize;

int iListFilesCurPos;
int iListFilesCount;
int iListDirsCurPos;
int iListDirsCount;
char cSettShowDirNameText[128];
	
char cCurrentPlayType[32];
char cCurrentPlayName[256];
char cCurrentPlayDir[256];
LIST_FILES_TYPE *cListFiles;
LIST_FILES_TYPE	*cListDirs;

char cDisplayMessageText[128];
char cDisplayMessageChanged;
char cEventMessageText[128];
char cEventMessageChanged;

char cStartTime[64];

//#include "debug.h"

int iFileLog, iScreenLog, iMessageLog, iLocalMessageLog, iEmailLog;
char cFileLogName[256];
char cLogIP[256];
int64_t iLogEmailTimer;
char cLogMailAddress[64];
char cLogMailServer[64];
char cLogMailLogin[64];
char cLogMailPassword[64];
char cLogMailAuth[64];
unsigned int uiLogEmailPauseSize;
pthread_mutex_t 	dbg_mutex;

//#include "audio.h"

pthread_mutex_t Play_Mutex;
unsigned int uiAudioDataCnt;
unsigned int uiVideoDataCnt;
unsigned int uiNewMediaPos;
unsigned int cThreadAudCaptStatus;
TX_EVENTER 	pevntRMS;

#include "rtsp.h"

pthread_mutex_t RTP_session_mutex;
RTP_SESSION *RTP_session;
unsigned int RTP_session_cnt;

#include "streamer.h"

char cThreadStreamerStatus;
unsigned int cStreamerExData[2];
REMOTE_STATUS_INFO	rsiRemoteStatus;

#include "gpio.h"

uint16_t *cRadioRegisters;
char cRadioStatus;

#include "web.h"

pthread_mutex_t WEB_mutex;
pthread_mutex_t WEB_req_mutex;
pthread_mutex_t WEB_explorer_mutex;
int iWebHistoryCnt;
WEB_HISTORY_INFO *whWebHistory;

#include "writer.h"

unsigned int cThreadWriterStatus;
TRANSFER_FILE_INFO tfTransferFileStatus;
TX_EVENTER writeevent_evnt;

