#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
//#include <linux/netdevice.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include "onvif.h"
#include <openssl/md5.h>
#include "main.h"
#include "network.h"
#include "weather.h"
#include "omx_client.h"
#include "web.h"
#include "gpio.h"
#include "weather.h"
#include "weather_func.h"
#include "rtsp.h"
#include "libavutil/base64.h"
#include <openssl/sha.h>

#define ONVIF_SERVER_NAME							"Azad Onvif Server"
#define ONVIF_COUNTRY_NAME							"russia"
#define ONVIF_MANUFACTURER							"Azat"

#define HEAD_MARK_HTTP 				" HTTP/"
#define HEAD_MARK_POST 				"POST "
#define HEAD_MARK_GET 				"GET "
#define HEAD_MARK_CONTENTTYPE 		"Content-Type: "
#define HEAD_MARK_HOST			 	"Host: "
#define HEAD_MARK_CONTENTLENGTH	 	"Content-Length: "
#define HEAD_MARK_ACCEPTENCODING	"Accept-Encoding: "
#define HEAD_MARK_CONNECTION		"Connection: "

#define ONVIF_TX_BUF_SIZE_MAX		2512000
#define ONVIF_RX_BUF_SIZE_MAX		2512000
#define ONVIF_RX_BUF_SIZE_DEF		16384

#define ONVIF_AUTH_LOCK_TIME		10
#define ONVIF_NONCE_CASH_SIZE		1024

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];

#define ONVIF_ACTION_NAME_DEVICE_GETDNS						"GetDNS"
#define ONVIF_ACTION_NAME_DEVICE_GETDEVICEINFORMATION		"GetDeviceInformation"
#define ONVIF_ACTION_NAME_DEVICE_GETSYSTEMDATEANDTIME		"GetSystemDateAndTime"
#define ONVIF_ACTION_NAME_DEVICE_SETSYSTEMDATEANDTIME		"SetSystemDateAndTime"
#define ONVIF_ACTION_NAME_DEVICE_GETCAPABILITIES			"GetCapabilities"
#define ONVIF_ACTION_NAME_DEVICE_GETSERVICES				"GetServices"
#define ONVIF_ACTION_NAME_DEVICE_GETSCOPES					"GetScopes"
#define ONVIF_ACTION_NAME_DEVICE_SETSCOPES					"SetScopes"
#define ONVIF_ACTION_NAME_DEVICE_GETUSERS					"GetUsers"
#define ONVIF_ACTION_NAME_DEVICE_GETDEVICEINFORMATION		"GetDeviceInformation"
#define ONVIF_ACTION_NAME_DEVICE_GETSERVICECAPABILITIES		"GetServiceCapabilities"
#define ONVIF_ACTION_NAME_DEVICE_GETZEROCONFIGURATION		"GetZeroConfiguration"
#define ONVIF_ACTION_NAME_DEVICE_SETZEROCONFIGURATION		"SetZeroConfiguration"
#define ONVIF_ACTION_NAME_DEVICE_GETNETWORKINTERFACES		"GetNetworkInterfaces"
#define ONVIF_ACTION_NAME_DEVICE_SETNETWORKINTERFACES		"SetNetworkInterfaces"
#define ONVIF_ACTION_NAME_DEVICE_GETCERTIFICATES			"GetCertificates"
#define ONVIF_ACTION_NAME_DEVICE_GETCERTIFICATESSTATUS		"GetCertificatesStatus"
#define ONVIF_ACTION_NAME_DEVICE_GETNTP						"GetNTP"
#define ONVIF_ACTION_NAME_DEVICE_SETNTP						"SetNTP"
#define ONVIF_ACTION_NAME_DEVICE_GETHOSTNAME				"GetHostname"
#define ONVIF_ACTION_NAME_DEVICE_SETHOSTNAME				"SetHostname"
#define ONVIF_ACTION_NAME_DEVICE_GETDISCOVERYMODE			"GetDiscoveryMode"
#define ONVIF_ACTION_NAME_DEVICE_GETNETWORKPROTOCOLS		"GetNetworkProtocols"
#define ONVIF_ACTION_NAME_DEVICE_GETNETWORKDEFAULTGATEWAY	"GetNetworkDefaultGateway"
#define ONVIF_ACTION_NAME_DEVICE_SYSTEMREBOOT				"SystemReboot"
#define ONVIF_ACTION_NAME_DEVICE_SETSYSTEMFACTORYDEFAULT	"SetSystemFactoryDefault"
#define ONVIF_ACTION_NAME_DEVICE_GETSYSTEMBACKUP			"GetSystemBackup"
#define ONVIF_ACTION_NAME_DEVICE_STARTFIRMWAREUPGRADE		"StartFirmwareUpgrade"
#define ONVIF_ACTION_NAME_DEVICE_GETRELAYOUTPUTS			"GetRelayOutputs"
#define ONVIF_ACTION_NAME_DEVICE_SETRELAYOUTPUTSTATE		"SetRelayOutputState"
#define ONVIF_ACTION_NAME_DEVICE_SETRELAYOUTPUTSETTINGS		"SetRelayOutputSettings"
#define ONVIF_ACTION_NAME_DEVICE_GETACCESSPOLICY			"GetAccessPolicy"
#define ONVIF_ACTION_NAME_DEVICE_SETACCESSPOLICY			"SetAccessPolicy"
#define ONVIF_ACTION_NAME_DEVICE_GETSYSTEMLOG				"GetSystemLog"

#define ONVIF_ACTION_NAME_DEVICE_SETNETWORKPROTOCOLS		"SetNetworkProtocols"
#define ONVIF_ACTION_NAME_DEVICE_SETUSER					"SetUser"
#define ONVIF_ACTION_NAME_DEVICE_CREATEUSERS				"CreateUsers"
#define ONVIF_ACTION_NAME_DEVICE_DELETEUSERS				"DeleteUsers"

#define ONVIF_ACTION_NAME_MEDIA_GETPROFILES					"GetProfiles"
#define ONVIF_ACTION_NAME_MEDIA_GETVIDEOSOURCES				"GetVideoSources"
#define ONVIF_ACTION_NAME_MEDIA_GETVIDEOENCODERS			"GetVideoEncoderConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETAUDIOENCODERS			"GetAudioEncoderConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETSNAPSHOTURI				"GetSnapshotUri"
#define ONVIF_ACTION_NAME_MEDIA_GETPROFILE					"GetProfile"
#define ONVIF_ACTION_NAME_MEDIA_GETSTREAMURI				"GetStreamUri"
#define ONVIF_ACTION_NAME_MEDIA_GETVIDEOSOURCECONF			"GetVideoSourceConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_GETVIDEOSOURCECONFS			"GetVideoSourceConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETVIDEOENCODERCONF			"GetVideoEncoderConfigurationOptions"
#define ONVIF_ACTION_NAME_MEDIA_GETAUDIOSOURCES				"GetAudioSources"
#define ONVIF_ACTION_NAME_MEDIA_CREATEPROFILE				"CreateProfile"
#define ONVIF_ACTION_NAME_MEDIA_DELETEPROFILE				"DeleteProfile"
#define ONVIF_ACTION_NAME_MEDIA_GETCOMPATIBLEVIDEOENCODER	"GetCompatibleVideoEncoderConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETCOMPATIBLEMETADATACONF	"GetCompatibleMetadataConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETAUDIOSOURCECONFS			"GetAudioSourceConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_SETVIDEOENCODERCONF			"SetVideoEncoderConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_GETCOMPVIDEOANALITCONF		"GetCompatibleVideoAnalyticsConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETCOMPAUDIOENCCONF			"GetCompatibleAudioEncoderConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETCOMPMETADATACONF			"GetCompatibleMetadataConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_GETCOMPVIDEOENCCONF			"GetCompatibleVideoEncoderConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_ADDVIDEOENCCONF				"AddVideoEncoderConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_ADDAUDIOENCCONF				"AddAudioEncoderConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_ADDVIDEOANALITCONF			"AddVideoAnalyticsConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_ADDMETADATACONFIGURATION	"AddMetadataConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_ADDPTZCONFIGURATION			"AddPTZConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_GETVIDEOANALITICSCONFS		"GetVideoAnalyticsConfigurations"
#define ONVIF_ACTION_NAME_MEDIA_REMOVEVIDEOENCCONF			"RemoveVideoEncoderConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_REMOVEAUDIOENCCONF			"RemoveAudioEncoderConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_REMOVEVIDEOANALITCONF		"RemoveVideoAnalyticsConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_REMOVEMETADATACONFIGURATION	"RemoveMetadataConfiguration"
#define ONVIF_ACTION_NAME_MEDIA_REMOVEPTZCONFIGURATION		"RemovePTZConfiguration"

#define ONVIF_ACTION_NAME_IMAGE_GETMOVEOPTIONS				"GetMoveOptions"
#define ONVIF_ACTION_NAME_IMAGE_GETIMAGINGSETTINGS			"GetImagingSettings"
#define ONVIF_ACTION_NAME_IMAGE_GETOPTIONS					"GetOptions"
#define ONVIF_ACTION_NAME_IMAGE_GETSTATUS					"GetStatus"
#define ONVIF_ACTION_NAME_IMAGE_SETIMAGINGSETTINGS			"SetImagingSettings"
#define ONVIF_ACTION_NAME_IMAGE_STOP						"Stop"
#define ONVIF_ACTION_NAME_IMAGE_MOVE						"Move"

#define ONVIF_ACTION_NAME_PTZ_GETNODE						"GetNode"
#define ONVIF_ACTION_NAME_PTZ_GETNODES						"GetNodes"
#define ONVIF_ACTION_NAME_PTZ_GETSTATUS						"GetStatus"
#define ONVIF_ACTION_NAME_PTZ_GETPRESETS					"GetPresets"
#define ONVIF_ACTION_NAME_PTZ_REMOVEPRESET					"RemovePreset"
#define ONVIF_ACTION_NAME_PTZ_GETCONFIGURATIONS				"GetConfigurations"
#define ONVIF_ACTION_NAME_PTZ_STOP							"Stop"
#define ONVIF_ACTION_NAME_PTZ_CONTINUOUSMOVE				"ContinuousMove"
#define ONVIF_ACTION_NAME_PTZ_RELATIVEMOVE					"RelativeMove"
#define ONVIF_ACTION_NAME_PTZ_GOTOHOMEPOSITION				"GotoHomePosition"
#define ONVIF_ACTION_NAME_PTZ_GOTOPRESET					"GotoPreset"
#define ONVIF_ACTION_NAME_PTZ_SETPRESET						"SetPreset"
#define ONVIF_ACTION_NAME_PTZ_ABSOLUTEMOVE					"AbsoluteMove"
#define ONVIF_ACTION_NAME_PTZ_SETHOMEPOSITION				"SetHomePosition"

#define ONVIF_ACTION_NAME_EVENT_CREATEPULLPOINTSUB			"CreatePullPointSubscription"
#define ONVIF_ACTION_NAME_EVENT_PULLMESSAGES				"PullMessages"
#define ONVIF_ACTION_NAME_EVENT_GETEVENTPROPERTIES			"GetEventProperties"
#define ONVIF_ACTION_NAME_EVENT_SUBSCRIBE					"Subscribe"
#define ONVIF_ACTION_NAME_EVENT_RENEWREQUEST				"Renew"
#define ONVIF_ACTION_NAME_EVENT_UNSUBSCRIBEREQUEST			"Unsubscribe"

#define ONVIF_ACTION_NAME_RECV_GETRECEIVERS					"GetReceivers"
#define ONVIF_ACTION_NAME_RECV_CREATERECEIVER				"CreateReceiver"
#define ONVIF_ACTION_NAME_RECV_CONFIGURERECEIVER			"ConfigureReceiver"
#define ONVIF_ACTION_NAME_RECV_DELETERECEIVER				"DeleteReceiver"

#define ONVIF_SERVICE_PATH_DEVICE							"/onvif/device_service"
#define ONVIF_SERVICE_PATH_EVENT							"/onvif/event_service"
#define ONVIF_SERVICE_PATH_EVENT_0							"/event_service/0"
#define ONVIF_SERVICE_PATH_MEDIA							"/onvif/media_service"
#define ONVIF_SERVICE_PATH_IMAGE							"/onvif/image_service"
#define ONVIF_SERVICE_PATH_PTZ								"/onvif/ptz_service"
#define ONVIF_SERVICE_PATH_SNAPSHOT							"snapshot/Camera"
#define ONVIF_SERVICE_PATH_RECV								"/onvif/receiver_service"

typedef enum
{
    ONVIF_ACTION_UNKNOWN,
	ONVIF_ACTION_TEARDOWN,
	ONVIF_ACTION_LOGOUT,
	ONVIF_ACTION_ERROR,
	
	ONVIF_ACTION_DEVICE_GETSCOPES = 100,
	ONVIF_ACTION_DEVICE_SETSCOPES,
	ONVIF_ACTION_DEVICE_GETDNS,
	ONVIF_ACTION_DEVICE_GETDEVICEINFORMATION,
	ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME,
	ONVIF_ACTION_DEVICE_SETSYSTEMDATEANDTIME,
	ONVIF_ACTION_DEVICE_GETCAPABILITIES,
	ONVIF_ACTION_DEVICE_GETSERVICES,
	ONVIF_ACTION_DEVICE_GETUSERS,
	ONVIF_ACTION_DEVICE_GETSERVICECAPABILITIES,
	ONVIF_ACTION_DEVICE_GETZEROCONFIGURATION,
	ONVIF_ACTION_DEVICE_SETZEROCONFIGURATION,
	ONVIF_ACTION_DEVICE_GETNETWORKINTERFACES,
	ONVIF_ACTION_DEVICE_SETNETWORKINTERFACES,
	ONVIF_ACTION_DEVICE_GETCERTIFICATES,
	ONVIF_ACTION_DEVICE_GETCERTIFICATESSTATUS,
	ONVIF_ACTION_DEVICE_GETNTP,
	ONVIF_ACTION_DEVICE_SETNTP,
	ONVIF_ACTION_DEVICE_GETHOSTNAME,
	ONVIF_ACTION_DEVICE_SETHOSTNAME,
	ONVIF_ACTION_DEVICE_GETDISCOVERYMODE,
	ONVIF_ACTION_DEVICE_GETNETWORKPROTOCOLS,
	ONVIF_ACTION_DEVICE_GETNETWORKDEFAULTGATEWAY,
	ONVIF_ACTION_DEVICE_SYSTEMREBOOT,
	ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT,
	ONVIF_ACTION_DEVICE_GETSYSTEMBACKUP,
	ONVIF_ACTION_DEVICE_STARTFIRMWAREUPGRADE,
	ONVIF_ACTION_DEVICE_GETRELAYOUTPUTS,
	ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSTATE,
	ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSETTINGS,
	ONVIF_ACTION_DEVICE_GETACCESSPOLICY,
	ONVIF_ACTION_DEVICE_SETACCESSPOLICY,
	ONVIF_ACTION_DEVICE_SETNETWORKPROTOCOLS,
	ONVIF_ACTION_DEVICE_SETUSER,
	ONVIF_ACTION_DEVICE_DELETEUSERS,
	ONVIF_ACTION_DEVICE_CREATEUSERS,
	ONVIF_ACTION_DEVICE_GETSYSTEMLOG,
	ONVIF_ACTION_EVENT_GETEVENTPROPERTIES,
	ONVIF_ACTION_EVENT_SUBSCRIBE,	
	ONVIF_ACTION_EVENT_RENEWREQUEST,
	ONVIF_ACTION_EVENT_UNSUBSCRIBEREQUEST,
	ONVIF_ACTION_MEDIA_GETPROFILES,
	ONVIF_ACTION_MEDIA_GETVIDEOSOURCES,
	ONVIF_ACTION_MEDIA_GETVIDEOENCODERS,
	ONVIF_ACTION_MEDIA_GETAUDIOENCODERS,
	ONVIF_ACTION_MEDIA_GETSNAPSHOTURI,
	ONVIF_ACTION_MEDIA_GETPROFILE,
	ONVIF_ACTION_MEDIA_GETSTREAMURI,
	ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONF,
	ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONFS,
	ONVIF_ACTION_MEDIA_GETVIDEOENCODERCONF,
	ONVIF_ACTION_MEDIA_GETAUDIOSOURCES,
	ONVIF_ACTION_MEDIA_CREATEPROFILE,
	ONVIF_ACTION_MEDIA_DELETEPROFILE,
	ONVIF_ACTION_MEDIA_GETCOMPATIBLEVIDEOENCODER,
	ONVIF_ACTION_MEDIA_GETCOMPATIBLEMETADATACONF,
	ONVIF_ACTION_MEDIA_GETAUDIOSOURCECONFS,
	ONVIF_ACTION_MEDIA_SETVIDEOENCODERCONF,
	ONVIF_ACTION_MEDIA_GETCOMPVIDEOANALITCONF,
	ONVIF_ACTION_MEDIA_GETCOMPAUDIOENCCONF,	
	ONVIF_ACTION_MEDIA_GETCOMPMETADATACONF,
	ONVIF_ACTION_MEDIA_GETCOMPVIDEOENCCONF,
	ONVIF_ACTION_MEDIA_ADDVIDEOENCCONF,
	ONVIF_ACTION_MEDIA_ADDAUDIOENCCONF,
	ONVIF_ACTION_MEDIA_ADDVIDEOANALITCONF,
	ONVIF_ACTION_MEDIA_ADDMETADATACONFIGURATION,
	ONVIF_ACTION_MEDIA_ADDPTZCONFIGURATION,
	ONVIF_ACTION_MEDIA_GETVIDEOANALITICSCONFS,
	ONVIF_ACTION_MEDIA_REMOVEVIDEOENCCONF,
	ONVIF_ACTION_MEDIA_REMOVEAUDIOENCCONF,
	ONVIF_ACTION_MEDIA_REMOVEVIDEOANALITCONF,
	ONVIF_ACTION_MEDIA_REMOVEMETADATACONFIGURATION,
	ONVIF_ACTION_MEDIA_REMOVEPTZCONFIGURATION,
	ONVIF_ACTION_IMAGE_GETMOVEOPTIONS,
	ONVIF_ACTION_IMAGE_GETIMAGINGSETTINGS,
	ONVIF_ACTION_IMAGE_GETOPTIONS,
	ONVIF_ACTION_IMAGE_GETSTATUS,
	ONVIF_ACTION_IMAGE_GETPREVIEWFILE,
	ONVIF_ACTION_IMAGE_SETIMAGINGSETTINGS,
	ONVIF_ACTION_IMAGE_STOP,
	ONVIF_ACTION_IMAGE_MOVE,
	ONVIF_ACTION_PTZ_GETNODE,
	ONVIF_ACTION_PTZ_GETNODES,
	ONVIF_ACTION_PTZ_GETSTATUS,
	ONVIF_ACTION_PTZ_GETPRESETS,
	ONVIF_ACTION_PTZ_GETCONFIGURATIONS,
	ONVIF_ACTION_PTZ_STOP,
	ONVIF_ACTION_PTZ_CONTINUOUSMOVE,
	ONVIF_ACTION_PTZ_RELATIVEMOVE,
	ONVIF_ACTION_PTZ_GOTOHOMEPOSITION,
	ONVIF_ACTION_PTZ_GOTOPRESET,
	ONVIF_ACTION_PTZ_SETPRESET,
	ONVIF_ACTION_PTZ_REMOVEPRESET,
	ONVIF_ACTION_PTZ_ABSOLUTEMOVE,
	ONVIF_ACTION_PTZ_SETHOMEPOSITION,
	ONVIF_ACTION_EVENT_CREATEPULLPOINTSUB,
	ONVIF_ACTION_EVENT_PULLMESSAGES,
	ONVIF_ACTION_RECV_GETRECEIVERS,
	ONVIF_ACTION_RECV_CREATERECEIVER,
	ONVIF_ACTION_RECV_CONFIGURERECEIVER,
	ONVIF_ACTION_RECV_DELETERECEIVER
} ONVIF_ACTION;

typedef enum
{
    ONVIF_SERVICE_UNKNOWN,
	ONVIF_SERVICE_DEVICE,
	ONVIF_SERVICE_EVENT,
	ONVIF_SERVICE_EVENT_0,
	ONVIF_SERVICE_MEDIA,
	ONVIF_SERVICE_SNAPSHOT,
	ONVIF_SERVICE_IMAGE,
	ONVIF_SERVICE_PTZ,
	ONVIF_SERVICE_RECV
} ONVIF_SERVICE;

typedef struct ONVIF_SESSION
{
	unsigned int 	client_ip;
	char			client_ip_str[16];
	unsigned int 	server_ip;
	char			server_ip_str[16];
	unsigned short  port;  
	unsigned short  onvif_port;
	unsigned short  rtsp_port;
	char			auth;
	char			login[64];
	char			password[64];
    int				socket;
	unsigned int	flag;
	char			media_url[256];
	unsigned int 	code;
	char			nonce[30];
	char			*head;
	MODULE_INFO		*CameraModule;
	MIC_LIST		*MicList;
	unsigned int 	MicCount;
	char			ptz_enabled;
	unsigned int	ptz_id;
	int				ptz_module_num;
	char			CameraPreviewFile[256];
} ONVIF_SESSION;

typedef struct PARAM_POINT
{
	char			*Point;
	unsigned int	Length;
} PARAM_POINT;

typedef struct CONTENT_TYPE
{
	unsigned int		Action;
	unsigned int		Charset;
	unsigned int		Type;
} CONTENT_TYPE;

typedef struct HTTP_HEAD
{
	char			Mode;
	char  			*Path; 
	char  			ServiceType; 
	char  			*ContentType; 
	CONTENT_TYPE	ContentTypeDetail;
	char			*Host;
	unsigned int	ContentLength;
	char			*AcceptEncoding;
	char			*Connection;
	char			*Body;
	unsigned int	BodyLength;
} HTTP_HEAD;

static pthread_t threadONVIF_income;
pthread_attr_t tattrONVIF_income;
static pthread_t threadONVIF_io;
pthread_attr_t tattrONVIF_io;

char cONVIF_init_done;
int cThreadOnvifStatus;
int Onvif_Pair[2];
pthread_mutex_t ONVIF_mutex;

long int ONVIF_Auth_lock_time;
char *ONVIF_Nonce_Buffer;
unsigned int ONVIF_Nonce_Buff_Pos;	
	
void * thread_ONVIF_income(void* pData);
void * thread_ONVIF_io(void* pData);
static char ONVIF_get_msg_type(char *pData, unsigned int uiDataLen, HTTP_HEAD *hhHead, char cTest);
static char ONVIF_describe_respond_401(ONVIF_SESSION *session, char *msg_rx, char *msg_tx);
static char* ONVIF_GetActionName(unsigned int uiAction);
static char* ONVIF_GetServiceTypeName(unsigned int uiServiceCode);
static char ONVIF_Auth(ONVIF_SESSION *session, char *msg_rx, unsigned int iReaded, HTTP_HEAD *pHEAD);
static char ONVIF_Respond_Get(ONVIF_SESSION *session, HTTP_HEAD *pHEAD, char *msg_tx);
int ONVIF_GetImage(ONVIF_SESSION *session, HTTP_HEAD *pHEAD, char *msg_tx);
static void ONVIV_Template_Error(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, char* strCode, char* strReason);
int ONVIF_get_node_param_from_url(char *cUrl, unsigned int uiUrlLen, char *node_name, char *param_name, char *param_value, int value_len);

void onvif_start(unsigned int uiOnvifPt, unsigned int uiOnvifAuth, unsigned int uiRtspPt, MODULE_INFO* miCamera, MIC_LIST *nlMicList, unsigned int uiMicCnt, int uiPTZnum)
{
	cONVIF_init_done = 0;
		
	pthread_attr_init(&tattrONVIF_io);   
	pthread_attr_setdetachstate(&tattrONVIF_io, PTHREAD_CREATE_DETACHED);
	
	pthread_mutex_init(&ONVIF_mutex, NULL);
		
	DBG_MUTEX_LOCK(&ONVIF_mutex);		
	cThreadOnvifStatus = 0;
	Onvif_Pair[0] = 0;
	Onvif_Pair[1] = 0;		
	DBG_MUTEX_UNLOCK(&ONVIF_mutex);	
	
	unsigned int *cSettings = (unsigned int*)DBG_MALLOC(sizeof(unsigned int)*10);
	cSettings[0] = uiOnvifPt;
	cSettings[1] = uiOnvifAuth;
	cSettings[2] = uiRtspPt;
	cSettings[3] = uiMicCnt;
	cSettings[4] = uiPTZnum;
	void **tr_data = (void**)DBG_MALLOC(sizeof(void*)*10);
	tr_data[0] = (void*)cSettings;
	tr_data[1] = (void*)miCamera;
	tr_data[2] = (void*)nlMicList;
	
	pthread_attr_init(&tattrONVIF_income);   
	pthread_attr_setdetachstate(&tattrONVIF_income, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadONVIF_income, &tattrONVIF_income, thread_ONVIF_income, tr_data); 
	
	cONVIF_init_done = 1;
}

void onvif_stop()
{
	if (cONVIF_init_done == 0) return;
	
	DBG_MUTEX_LOCK(&ONVIF_mutex);
	if (Onvif_Pair[0] != 0)
	{
		char cType = SIGNAL_CLOSE;
		int rv = write(Onvif_Pair[0], &cType, 1);
		if (rv < 1) dbgprintf(4, "onvif_stop: write socketpair signal %i (errno:%i, %s)\n", cType, errno, strerror(errno));	
	}
	DBG_MUTEX_UNLOCK(&ONVIF_mutex);
	int ret;
	do
	{
		DBG_MUTEX_LOCK(&ONVIF_mutex);
		ret = cThreadOnvifStatus;
		DBG_MUTEX_UNLOCK(&ONVIF_mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);
		
	pthread_attr_destroy(&tattrONVIF_income);
	pthread_attr_destroy(&tattrONVIF_io);
	pthread_mutex_destroy(&ONVIF_mutex);
}

void * thread_ONVIF_income(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "ONVIF_income");
	
	void **tr_data = (void**)pData;
	unsigned int *uiSettings = (unsigned int*)tr_data[0];
	MODULE_INFO* miCamera		= tr_data[1];
	MIC_LIST *mlMicList			= tr_data[2];
	char CameraPreviewFile[256];
	
	DBG_MUTEX_LOCK(&system_mutex);
	memcpy(CameraPreviewFile, cCameraSensorFile, 256);
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	unsigned int uiOnvifPt		= uiSettings[0];
	unsigned int uiOnvifAuth	= uiSettings[1];
	unsigned int uiRtspPt		= uiSettings[2];
	unsigned int uiMicCnt		= uiSettings[3];
	int uiPTZnum				= uiSettings[4];
	DBG_FREE(uiSettings);
	DBG_FREE(pData);
	
	struct sockaddr_in      client_addr;
    struct sockaddr_in      client_port;  
	struct sockaddr_in      server_port;  
		
	unsigned int            addr_len, n;
	
	int tcp_sock;
	ONVIF_SESSION *session;
	// Create socket. 
    int onvif_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(onvif_sock != -1);
	
	n = 1;
	if (setsockopt(onvif_sock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int)) < 0)
		dbgprintf(2, "ONVIF setsockopt(SO_REUSEADDR) failed\n");

	// Setup address. 
    client_addr.sin_family      = AF_INET;
    client_addr.sin_port        = htons(uiOnvifPt);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&client_addr.sin_zero, 0, 8);

    // Bind to address. 
    int rc = bind(onvif_sock,(struct sockaddr *) &client_addr, sizeof(client_addr)); 
    if (rc != 0)
	{
		dbgprintf(1, "thread_ONVIF_income: Error bind socket\n");
		close(onvif_sock);
		DBG_LOG_OUT();
		pthread_exit(0);
	}

    // Listening for traffic. 
    if (listen(onvif_sock, 5) == -1) 
	{
        dbgprintf(1,"Error listen Onvif socket(%i) %s\n", errno, strerror(errno));
        close(onvif_sock);
		DBG_LOG_OUT();
		pthread_exit(0);
    }
	
	int flags = fcntl(onvif_sock, F_GETFL, 0);
	if (fcntl(onvif_sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(1,"error set Onvif nonblock(%i) %s\n", errno, strerror(errno));
		close(onvif_sock);
		DBG_LOG_OUT();
        pthread_exit(0);
	}
	
	int iLoop = 1;
	char SignalBuff;
	
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, Onvif_Pair) < 0)
	{	
		dbgprintf(1,"error create socketpair\n");
		Onvif_Pair[0] = 0;
		Onvif_Pair[1] = 0;
		iLoop = 0;
	}
	else
	{
		flags = fcntl(Onvif_Pair[0], F_GETFL, 0);
		if (fcntl(Onvif_Pair[0], F_SETFL, flags | O_NONBLOCK) < 0)
		{
			dbgprintf(1,"error set socketpair nonblock\n");
			iLoop = 0;
		}
		else
		{
			flags = fcntl(Onvif_Pair[1], F_GETFL, 0);
			if (fcntl(Onvif_Pair[1], F_SETFL, flags | O_NONBLOCK) < 0)
			{
				dbgprintf(1,"error set socketpair nonblock\n");
				iLoop = 0;
			}
		}
	}
	
	DBG_MUTEX_LOCK(&ONVIF_mutex);
	cThreadOnvifStatus++;
	ONVIF_Auth_lock_time = 0;	
	ONVIF_Nonce_Buffer = (char*)DBG_MALLOC(ONVIF_NONCE_CASH_SIZE);
	ONVIF_Nonce_Buff_Pos = 0;
	memset(ONVIF_Nonce_Buffer, 0, ONVIF_NONCE_CASH_SIZE);
	DBG_MUTEX_UNLOCK(&ONVIF_mutex);	
	
	int max_fd = -1;
	fd_set rfds;
	char ReadSignalReady = 1;
	struct timeval tv;
	int ret;
	
	while(iLoop)
    {
		FD_ZERO(&rfds);		
		FD_SET(onvif_sock, &rfds);
		max_fd = onvif_sock;
		if (ReadSignalReady == 0)
		{
			FD_SET(Onvif_Pair[1], &rfds);
			if (Onvif_Pair[1] > max_fd) max_fd = Onvif_Pair[1];
			tv.tv_sec = 1;
		} else tv.tv_sec = 0;
		
		tv.tv_usec = 0;		
		
		ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		
		if ((ReadSignalReady == 0) && (FD_ISSET(Onvif_Pair[1], &rfds))) ReadSignalReady = 1;
		if (ReadSignalReady == 1)
		{	
			ret = read(Onvif_Pair[1], &SignalBuff, 1);
			if ((ret > 0) && (SignalBuff == SIGNAL_CLOSE))
			{
				iLoop = 0;
				//printf("SIGNAL_CLOSE %i\n", conn_num);
				dbgprintf(4, "Close ONVIF signal\n");	
				break;
			}
			else
			{
				if (errno == EAGAIN) ReadSignalReady = 0;
				else 
				{
					dbgprintf(2, "Close ONVIF socketpair (errno:%i, %s)\n",errno, strerror(errno));
					break;
				}
			}
		}
		if (ret != 0)
		{
			if (FD_ISSET(onvif_sock, &rfds))
			{				
				addr_len = sizeof(client_addr); 
				tcp_sock = accept(onvif_sock, (struct sockaddr *) &client_addr, &addr_len); 
				if (tcp_sock != -1)
				{
					session = (ONVIF_SESSION*)DBG_MALLOC(sizeof(ONVIF_SESSION));
					memset(session, 0, sizeof(ONVIF_SESSION));
					
					addr_len = sizeof(client_port);
					getpeername(tcp_sock, (struct sockaddr*)&client_port, (socklen_t*)&addr_len);
					addr_len = sizeof(server_port);
					getsockname(tcp_sock, (struct sockaddr*)&server_port, (socklen_t*)&addr_len);
					
					flags = fcntl(tcp_sock, F_GETFL, 0);
					if (fcntl(tcp_sock, F_SETFL, flags | O_NONBLOCK) == -1)
					{
						dbgprintf(1,"error set Onvif nonblock accepted socket(%i) %s\n", errno, strerror(errno));
						close(tcp_sock);
						tcp_sock = -1;
					}
					session->client_ip   	= client_port.sin_addr.s_addr;
					session->server_ip   	= server_port.sin_addr.s_addr;
					strcpy(session->client_ip_str, inet_ntoa(client_port.sin_addr));
					strcpy(session->server_ip_str, inet_ntoa(server_port.sin_addr));
					session->port			= client_port.sin_port;
					session->onvif_port		= uiOnvifPt;
					session->rtsp_port		= uiRtspPt;
					session->auth			= uiOnvifAuth;		
					session->ptz_module_num	= uiPTZnum;						
					memset(session->login, 0, 64);
					memset(session->password, 0, 64);			
					session->socket 		= tcp_sock; 
					memset(session->media_url, 0, 256);
					memcpy(session->CameraPreviewFile, CameraPreviewFile, 256);
					
					if (miCamera)
					{
						session->CameraModule = (MODULE_INFO*)DBG_MALLOC(sizeof(MODULE_INFO));
						memcpy(session->CameraModule, miCamera, sizeof(MODULE_INFO));
						session->ptz_enabled	= miCamera->Settings[38] & 1;
						session->ptz_id			= miCamera->Settings[39];
						if (session->ptz_module_num == -1) session->ptz_enabled = 0;
					}
					if (uiMicCnt && mlMicList)
					{
						session->MicCount = uiMicCnt;
						session->MicList = (MIC_LIST*)DBG_MALLOC(sizeof(MIC_LIST)*uiMicCnt);
						memcpy(session->MicList, mlMicList, sizeof(MIC_LIST)*uiMicCnt);
					}
					if (tcp_sock != -1)
						pthread_create(&threadONVIF_io, &tattrONVIF_io, thread_ONVIF_io, (void*)session); 					
					else DBG_FREE(session);
				}
			}
		}
    }
	
	DBG_MUTEX_LOCK(&ONVIF_mutex);
	if (Onvif_Pair[0] != 0) close(Onvif_Pair[0]);
	if (Onvif_Pair[1] != 0) close(Onvif_Pair[1]);
	Onvif_Pair[0] = 0;
	Onvif_Pair[1] = 0;
	cThreadOnvifStatus--;
	DBG_FREE(ONVIF_Nonce_Buffer);
	DBG_MUTEX_UNLOCK(&ONVIF_mutex);
	    
	close(onvif_sock);
	if (miCamera) DBG_FREE(miCamera);
	if (uiMicCnt && mlMicList) DBG_FREE(mlMicList);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)0;
}

void * thread_ONVIF_io(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "ONVIF_io");
	
	ONVIF_SESSION			*session = (ONVIF_SESSION*)pData;
	char                    *msg_rx = (char*)DBG_MALLOC(ONVIF_RX_BUF_SIZE_DEF);
    char                    *msg_tx = (char*)DBG_MALLOC(ONVIF_TX_BUF_SIZE_MAX);
    int                     ret; 
	int 					iReadCnt;
	int						iReaded;
	int						iReaderSize = ONVIF_RX_BUF_SIZE_DEF;
	int 					iPageSize = 0;
	HTTP_HEAD 				hhHEAD;
	
	memset(msg_tx, 0, ONVIF_TX_BUF_SIZE_MAX);
	memset(msg_rx, 0, ONVIF_RX_BUF_SIZE_DEF);
	
    dbgprintf(5, "thread_ONVIF_io: New ONVIF server thread.\n"); 
    dbgprintf(5, "thread_ONVIF_io: Client IP: %s\n", session->client_ip_str);

	session->head = (char*)DBG_MALLOC(4096);
	memset(session->head, 0, 4096);
	
	int iAuthLocked = 0;
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	
	DBG_MUTEX_LOCK(&ONVIF_mutex);
	iAuthLocked = (ONVIF_Auth_lock_time != 0) ? 1 : 0;
	
	if (ONVIF_Auth_lock_time && ((nanotime.tv_sec - ONVIF_Auth_lock_time) > ONVIF_AUTH_LOCK_TIME)) 
	{
		iAuthLocked = 0;
		ONVIF_Auth_lock_time = 0;
	}
	cThreadOnvifStatus++;
	DBG_MUTEX_UNLOCK(&ONVIF_mutex);
	
	do
    {
		memset(msg_rx, 0, ONVIF_RX_BUF_SIZE_DEF);
		iReaded = 0;
		iReadCnt = 100;
		iPageSize = 0;
		
		do
		{
			ret = read(session->socket, &msg_rx[iReaded], iReaderSize - iReaded); 
			if (ret < 1) 
			{
				if (ret == 0)
				{
					dbgprintf(5, "ONVIF_io: read error(closed)\n");	
					break; 
				}
				if (errno != EAGAIN)			
				{
					session->socket = 0;
					dbgprintf(5, "ONVIF_io: read error(errno:%i, %s)\n", errno, strerror(errno));
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
					if (iReaded >= ONVIF_RX_BUF_SIZE_MAX) break;
					iReaderSize += ONVIF_RX_BUF_SIZE_DEF;
					msg_rx = (char*)DBG_REALLOC(msg_rx, iReaderSize);
					//printf("up in buffer to %i  %i\n", iReaderSize, iReaded);
				}
				if (ONVIF_get_msg_type(msg_rx, iReaded, &hhHEAD, 1)) break; else iReadCnt = 100;
			}
		} while(iReadCnt);
		if (iReaded > 0)
		{
			DBG_MUTEX_LOCK(&ONVIF_mutex);
			//printf("ONVIF_io: ONVIF Request [session ID = %d]:\n", id); 
			memset(msg_tx, 0, ONVIF_TX_BUF_SIZE_MAX);
			
			ONVIF_get_msg_type(msg_rx, iReaded, &hhHEAD, 0);
			dbgprintf(5, "ONVIF_io: sevice=%s, type=%s\n", ONVIF_GetServiceTypeName(hhHEAD.ServiceType), ONVIF_GetActionName(hhHEAD.ContentTypeDetail.Action));
			if ((hhHEAD.ContentTypeDetail.Action == ONVIF_ACTION_UNKNOWN) || (hhHEAD.ServiceType == ONVIF_SERVICE_UNKNOWN))
				//|| (hhHEAD.ContentTypeDetail.Action == ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT))
			{
				dbgprintf(2, "ONVIF_io: Mode=%i, sevice=%s, type=%s\n", hhHEAD.Mode, ONVIF_GetServiceTypeName(hhHEAD.ServiceType), ONVIF_GetActionName(hhHEAD.ContentTypeDetail.Action));
				dbgprintf(3, "ONVIF_io: ServicePath: '%s'\n", hhHEAD.Path);
				dbgprintf(3, "ONVIF_io: Action: messlen:%i strlen:%i body:'%s'\n", hhHEAD.BodyLength, strlen(hhHEAD.Body), hhHEAD.Body);
				//omx_dump_data("onvif.dat", msg_rx, iReaded);
				//printf(">>>P '%s'\n", hhHEAD.Path);
				//printf(">>>CT '%s'\n", hhHEAD.ContentType);
				//printf("%s\n", &hhHEAD.Body[2]);
			}
			
			if ((session->auth == 0) || 
				(hhHEAD.ContentTypeDetail.Action == ONVIF_ACTION_IMAGE_GETPREVIEWFILE) || 
				(hhHEAD.ContentTypeDetail.Action == ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME) || 
				(hhHEAD.ContentTypeDetail.Action == ONVIF_ACTION_DEVICE_GETSCOPES) || 				
				((iAuthLocked == 0) && ONVIF_Auth(session, msg_rx, iReaded, &hhHEAD)))
			{
				//ONVIF_main_form(session->head, session->auth, session->access);
						
				dbgprintf(5, "ONVIF_io: type=%s\n", ONVIF_GetActionName(hhHEAD.ContentTypeDetail.Action));
				
				if (hhHEAD.Mode == 2)
				{
					switch(hhHEAD.ContentTypeDetail.Action)
					{
						case ONVIF_ACTION_IMAGE_GETPREVIEWFILE:
							iPageSize = ONVIF_GetImage(session, &hhHEAD, msg_tx);
							break;
						default:
							break;
					}
				}
				
				if (hhHEAD.Mode == 1)
				{
					switch(hhHEAD.ContentTypeDetail.Action)
					{																															
						case ONVIF_ACTION_EVENT_PULLMESSAGES:
							break;
						case ONVIF_ACTION_DEVICE_SETSCOPES:	
						case ONVIF_ACTION_DEVICE_GETUSERS:						
						case ONVIF_ACTION_DEVICE_GETSERVICECAPABILITIES:
						case ONVIF_ACTION_DEVICE_GETCERTIFICATES:
						case ONVIF_ACTION_DEVICE_GETCERTIFICATESSTATUS:	
						case ONVIF_ACTION_DEVICE_GETNTP:
						case ONVIF_ACTION_DEVICE_SETNTP:
						case ONVIF_ACTION_DEVICE_GETHOSTNAME:
						case ONVIF_ACTION_DEVICE_SETHOSTNAME:
						case ONVIF_ACTION_DEVICE_GETDISCOVERYMODE:
						case ONVIF_ACTION_DEVICE_GETNETWORKPROTOCOLS:
						case ONVIF_ACTION_DEVICE_GETNETWORKDEFAULTGATEWAY:
						case ONVIF_ACTION_EVENT_CREATEPULLPOINTSUB:
						case ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME: 						
						case ONVIF_ACTION_DEVICE_SETSYSTEMDATEANDTIME: 						
						case ONVIF_ACTION_DEVICE_GETSCOPES:
						case ONVIF_ACTION_DEVICE_GETNETWORKINTERFACES:
						case ONVIF_ACTION_DEVICE_SETNETWORKINTERFACES:
						case ONVIF_ACTION_DEVICE_GETDEVICEINFORMATION:
						case ONVIF_ACTION_DEVICE_GETDNS: 
						case ONVIF_ACTION_DEVICE_GETCAPABILITIES:
						case ONVIF_ACTION_DEVICE_GETSERVICES:
						case ONVIF_ACTION_DEVICE_SYSTEMREBOOT:
						case ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT:
						case ONVIF_ACTION_DEVICE_GETSYSTEMBACKUP:
						case ONVIF_ACTION_DEVICE_STARTFIRMWAREUPGRADE:
						case ONVIF_ACTION_DEVICE_GETZEROCONFIGURATION:
						case ONVIF_ACTION_DEVICE_SETZEROCONFIGURATION:
						case ONVIF_ACTION_DEVICE_GETRELAYOUTPUTS:
						case ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSTATE:
						case ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSETTINGS:
						case ONVIF_ACTION_DEVICE_GETACCESSPOLICY:
						case ONVIF_ACTION_DEVICE_SETACCESSPOLICY:
						case ONVIF_ACTION_DEVICE_SETNETWORKPROTOCOLS:
						case ONVIF_ACTION_DEVICE_SETUSER:
						case ONVIF_ACTION_DEVICE_DELETEUSERS:
						case ONVIF_ACTION_DEVICE_CREATEUSERS:
						case ONVIF_ACTION_DEVICE_GETSYSTEMLOG:
						case ONVIF_ACTION_EVENT_GETEVENTPROPERTIES:
						case ONVIF_ACTION_EVENT_SUBSCRIBE:						
						case ONVIF_ACTION_EVENT_RENEWREQUEST:
						case ONVIF_ACTION_EVENT_UNSUBSCRIBEREQUEST:
						case ONVIF_ACTION_MEDIA_GETPROFILES:
						case ONVIF_ACTION_MEDIA_GETVIDEOSOURCES:
						case ONVIF_ACTION_MEDIA_GETVIDEOENCODERS:
						case ONVIF_ACTION_MEDIA_GETAUDIOENCODERS:
						case ONVIF_ACTION_MEDIA_GETSNAPSHOTURI:
						case ONVIF_ACTION_MEDIA_GETPROFILE:
						case ONVIF_ACTION_MEDIA_GETSTREAMURI:
						case ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONF:
						case ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONFS:
						case ONVIF_ACTION_MEDIA_GETVIDEOENCODERCONF:
						case ONVIF_ACTION_MEDIA_GETAUDIOSOURCES:
						case ONVIF_ACTION_MEDIA_CREATEPROFILE:
						case ONVIF_ACTION_MEDIA_DELETEPROFILE:
						case ONVIF_ACTION_MEDIA_GETCOMPATIBLEVIDEOENCODER:
						case ONVIF_ACTION_MEDIA_GETCOMPATIBLEMETADATACONF:
						case ONVIF_ACTION_MEDIA_GETAUDIOSOURCECONFS:
						case ONVIF_ACTION_MEDIA_SETVIDEOENCODERCONF:
						case ONVIF_ACTION_MEDIA_GETCOMPVIDEOANALITCONF:
						case ONVIF_ACTION_MEDIA_GETCOMPAUDIOENCCONF:	
						case ONVIF_ACTION_MEDIA_GETCOMPMETADATACONF:
						case ONVIF_ACTION_MEDIA_GETCOMPVIDEOENCCONF:
						case ONVIF_ACTION_MEDIA_ADDVIDEOENCCONF:
						case ONVIF_ACTION_MEDIA_ADDAUDIOENCCONF:
						case ONVIF_ACTION_MEDIA_ADDVIDEOANALITCONF:
						case ONVIF_ACTION_MEDIA_ADDMETADATACONFIGURATION:
						case ONVIF_ACTION_MEDIA_ADDPTZCONFIGURATION:	
						case ONVIF_ACTION_MEDIA_GETVIDEOANALITICSCONFS:
						case ONVIF_ACTION_MEDIA_REMOVEVIDEOENCCONF:
						case ONVIF_ACTION_MEDIA_REMOVEAUDIOENCCONF:
						case ONVIF_ACTION_MEDIA_REMOVEVIDEOANALITCONF:
						case ONVIF_ACTION_MEDIA_REMOVEMETADATACONFIGURATION:
						case ONVIF_ACTION_MEDIA_REMOVEPTZCONFIGURATION:	
						case ONVIF_ACTION_IMAGE_GETMOVEOPTIONS:
						case ONVIF_ACTION_IMAGE_GETIMAGINGSETTINGS:
						case ONVIF_ACTION_IMAGE_GETOPTIONS:
						case ONVIF_ACTION_IMAGE_GETSTATUS:
						case ONVIF_ACTION_IMAGE_SETIMAGINGSETTINGS:
						case ONVIF_ACTION_IMAGE_STOP:
						case ONVIF_ACTION_IMAGE_MOVE:
						case ONVIF_ACTION_PTZ_GETNODE:
						case ONVIF_ACTION_PTZ_GETNODES:
						case ONVIF_ACTION_PTZ_GETSTATUS:
						case ONVIF_ACTION_PTZ_GETPRESETS:
						case ONVIF_ACTION_PTZ_GETCONFIGURATIONS:
						case ONVIF_ACTION_PTZ_STOP:
						case ONVIF_ACTION_PTZ_CONTINUOUSMOVE:
						case ONVIF_ACTION_PTZ_RELATIVEMOVE:
						case ONVIF_ACTION_PTZ_GOTOHOMEPOSITION:
						case ONVIF_ACTION_PTZ_GOTOPRESET:
						case ONVIF_ACTION_PTZ_SETPRESET:
						case ONVIF_ACTION_PTZ_ABSOLUTEMOVE:
						case ONVIF_ACTION_PTZ_SETHOMEPOSITION:
						case ONVIF_ACTION_PTZ_REMOVEPRESET:
						case ONVIF_ACTION_RECV_GETRECEIVERS:
						case ONVIF_ACTION_RECV_CREATERECEIVER:
						case ONVIF_ACTION_RECV_CONFIGURERECEIVER:
						case ONVIF_ACTION_RECV_DELETERECEIVER:
							ONVIF_Respond_Get(session, &hhHEAD, msg_tx);							
							break;
						case ONVIF_ACTION_LOGOUT: 
							ONVIF_describe_respond_401(session, msg_rx, msg_tx);
							break;			
						default:						
							hhHEAD.ContentTypeDetail.Action = ONVIF_ACTION_ERROR; 
							break; 
					}
				}
				if (!hhHEAD.Mode) hhHEAD.ContentTypeDetail.Action = ONVIF_ACTION_ERROR; 
			}
			else
			{
				if (session->auth) 
				{
					ONVIF_describe_respond_401(session, msg_rx, msg_tx);
					dbgprintf(4, "ONVIF_io: Error authentifycation\n");
				}
			}			
			if (!iPageSize) iPageSize = strlen(msg_tx);	
			if (iPageSize == 0) dbgprintf(2, "Response size is null (%i) %s\n", hhHEAD.ContentTypeDetail.Action, ONVIF_GetActionName(hhHEAD.ContentTypeDetail.Action));
			DBG_MUTEX_UNLOCK(&ONVIF_mutex);	
			
			if (hhHEAD.ContentTypeDetail.Action == ONVIF_ACTION_TEARDOWN)
			{
				dbgprintf(5, "ONVIF_ACTION_TEARDOWN \n"); 			
				break; 
			}
		}
		int iSended = 0;
		
		while(iPageSize)
		{
			ret = write(session->socket, &msg_tx[iSended], iPageSize);			
			if (ret < 1) 
			{
				if (errno == EAGAIN) 
				{
					dbgprintf(5, "ONVIF_io: write error (errno:%i, %s)\n", errno, strerror(errno));	
					//printf("No data on %i\n", n);
				}
				else
				{
					//session->socket = 0;
					dbgprintf(5, "ONVIF_io: Write TCP error. [client IP: %s]\n", session->client_ip_str); 
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
	
	dbgprintf(5, "ONVIF_io: Done. [client IP: %s]\n", session->client_ip_str); 
	
	if (session->socket > 0) 
	{
		close(session->socket);
		session->socket = 0;		
	}
	
	if (session->CameraModule) DBG_FREE(session->CameraModule);
	if (session->MicList && session->MicCount) DBG_FREE(session->MicList);
	DBG_FREE(session->head);
	DBG_FREE(session);
	DBG_FREE(msg_rx);
	DBG_FREE(msg_tx);
	DBG_MUTEX_LOCK(&ONVIF_mutex);
	cThreadOnvifStatus--;
	DBG_MUTEX_UNLOCK(&ONVIF_mutex);
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)0;
}

int ONVIF_load_limit_file_in_buff(char *filename, misc_buffer * buffer, unsigned int uiLen) 
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

unsigned int ONVIF_get_action_code(char *rx_data, unsigned int rx_len, unsigned int uiServiceType)
{
	if (uiServiceType == ONVIF_SERVICE_DEVICE)
	{
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETSCOPES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETSCOPES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETDNS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETDNS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETDEVICEINFORMATION, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETDEVICEINFORMATION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETSYSTEMDATEANDTIME, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETSYSTEMDATEANDTIME, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETSYSTEMDATEANDTIME;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETCAPABILITIES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETCAPABILITIES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETSERVICES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETSERVICES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETSCOPES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETSCOPES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETUSERS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETUSERS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETSERVICECAPABILITIES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETSERVICECAPABILITIES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETZEROCONFIGURATION, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETZEROCONFIGURATION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETZEROCONFIGURATION, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETZEROCONFIGURATION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETNETWORKINTERFACES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETNETWORKINTERFACES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETNETWORKINTERFACES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETNETWORKINTERFACES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETCERTIFICATES, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETCERTIFICATES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETCERTIFICATESSTATUS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETCERTIFICATESSTATUS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETNTP, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETNTP;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETNTP, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETNTP;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETHOSTNAME, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETHOSTNAME;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETHOSTNAME, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETHOSTNAME;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETDISCOVERYMODE, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETDISCOVERYMODE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETNETWORKPROTOCOLS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETNETWORKPROTOCOLS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETNETWORKDEFAULTGATEWAY, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETNETWORKDEFAULTGATEWAY;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SYSTEMREBOOT, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SYSTEMREBOOT;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETSYSTEMFACTORYDEFAULT, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETSYSTEMBACKUP, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETSYSTEMBACKUP;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_STARTFIRMWAREUPGRADE, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_STARTFIRMWAREUPGRADE;		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETRELAYOUTPUTS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETRELAYOUTPUTS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETRELAYOUTPUTSTATE, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSTATE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETRELAYOUTPUTSETTINGS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSETTINGS;
		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETACCESSPOLICY, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETACCESSPOLICY;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETACCESSPOLICY, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETACCESSPOLICY;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETNETWORKPROTOCOLS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETNETWORKPROTOCOLS;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_SETUSER, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_SETUSER;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_DELETEUSERS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_DELETEUSERS;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_CREATEUSERS, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_CREATEUSERS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_DEVICE_GETSYSTEMLOG, NULL, NULL, 0)) return ONVIF_ACTION_DEVICE_GETSYSTEMLOG;	
	}
	if (uiServiceType == ONVIF_SERVICE_MEDIA)
	{
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETPROFILES, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETPROFILES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETVIDEOSOURCES, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETVIDEOSOURCES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETVIDEOENCODERS, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETVIDEOENCODERS;		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETAUDIOENCODERS, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETAUDIOENCODERS;		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETSNAPSHOTURI, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETSNAPSHOTURI;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETPROFILE, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETPROFILE;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETSTREAMURI, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETSTREAMURI;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETVIDEOSOURCECONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETVIDEOSOURCECONFS, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONFS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETVIDEOENCODERCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETVIDEOENCODERCONF;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETAUDIOSOURCES, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETAUDIOSOURCES;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_CREATEPROFILE, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_CREATEPROFILE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_DELETEPROFILE, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_DELETEPROFILE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETCOMPATIBLEVIDEOENCODER, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETCOMPATIBLEVIDEOENCODER;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETCOMPATIBLEMETADATACONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETCOMPATIBLEMETADATACONF;		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETAUDIOSOURCECONFS, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETAUDIOSOURCECONFS;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_SETVIDEOENCODERCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_SETVIDEOENCODERCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETCOMPVIDEOANALITCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETCOMPVIDEOANALITCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETCOMPAUDIOENCCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETCOMPAUDIOENCCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETCOMPMETADATACONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETCOMPMETADATACONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETCOMPVIDEOENCCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETCOMPVIDEOENCCONF;		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_ADDVIDEOENCCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_ADDVIDEOENCCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_ADDAUDIOENCCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_ADDAUDIOENCCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_ADDVIDEOANALITCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_ADDVIDEOANALITCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_ADDMETADATACONFIGURATION, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_ADDMETADATACONFIGURATION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_ADDPTZCONFIGURATION, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_ADDPTZCONFIGURATION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_GETVIDEOANALITICSCONFS, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_GETVIDEOANALITICSCONFS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_REMOVEVIDEOENCCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_REMOVEVIDEOENCCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_REMOVEAUDIOENCCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_REMOVEAUDIOENCCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_REMOVEVIDEOANALITCONF, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_REMOVEVIDEOANALITCONF;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_REMOVEMETADATACONFIGURATION, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_REMOVEMETADATACONFIGURATION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_MEDIA_REMOVEPTZCONFIGURATION, NULL, NULL, 0)) return ONVIF_ACTION_MEDIA_REMOVEPTZCONFIGURATION;
		
	}
	if (uiServiceType == ONVIF_SERVICE_IMAGE)
	{
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_GETMOVEOPTIONS, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_GETMOVEOPTIONS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_GETIMAGINGSETTINGS, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_GETIMAGINGSETTINGS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_GETOPTIONS, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_GETOPTIONS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_GETSTATUS, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_GETSTATUS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_SETIMAGINGSETTINGS, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_SETIMAGINGSETTINGS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_STOP, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_STOP;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_IMAGE_MOVE, NULL, NULL, 0)) return ONVIF_ACTION_IMAGE_MOVE;
	}
	if (uiServiceType == ONVIF_SERVICE_PTZ)
	{
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GETNODE, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GETNODE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GETNODES, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GETNODES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GETSTATUS, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GETSTATUS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GETPRESETS, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GETPRESETS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_REMOVEPRESET, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_REMOVEPRESET;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GETCONFIGURATIONS, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GETCONFIGURATIONS;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_STOP, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_STOP;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_CONTINUOUSMOVE, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_CONTINUOUSMOVE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_RELATIVEMOVE, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_RELATIVEMOVE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GOTOHOMEPOSITION, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GOTOHOMEPOSITION;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_GOTOPRESET, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_GOTOPRESET;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_SETPRESET, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_SETPRESET;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_ABSOLUTEMOVE, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_ABSOLUTEMOVE;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_PTZ_SETHOMEPOSITION, NULL, NULL, 0)) return ONVIF_ACTION_PTZ_SETHOMEPOSITION;	
	}
	if (uiServiceType == ONVIF_SERVICE_EVENT)
	{
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_EVENT_CREATEPULLPOINTSUB, NULL, NULL, 0)) return ONVIF_ACTION_EVENT_CREATEPULLPOINTSUB;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_EVENT_PULLMESSAGES, NULL, NULL, 0)) return ONVIF_ACTION_EVENT_PULLMESSAGES;		
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_EVENT_GETEVENTPROPERTIES, NULL, NULL, 0)) return ONVIF_ACTION_EVENT_GETEVENTPROPERTIES;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_EVENT_SUBSCRIBE, NULL, NULL, 0)) return ONVIF_ACTION_EVENT_SUBSCRIBE;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_EVENT_RENEWREQUEST, NULL, NULL, 0)) return ONVIF_ACTION_EVENT_RENEWREQUEST;
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_EVENT_UNSUBSCRIBEREQUEST, NULL, NULL, 0)) return ONVIF_ACTION_EVENT_UNSUBSCRIBEREQUEST;
	}
	if (uiServiceType == ONVIF_SERVICE_RECV)
	{
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_RECV_GETRECEIVERS, NULL, NULL, 0)) return ONVIF_ACTION_RECV_GETRECEIVERS;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_RECV_CREATERECEIVER, NULL, NULL, 0)) return ONVIF_ACTION_RECV_CREATERECEIVER;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_RECV_CONFIGURERECEIVER, NULL, NULL, 0)) return ONVIF_ACTION_RECV_CONFIGURERECEIVER;	
		if (ONVIF_get_node_param_from_url(rx_data, rx_len, ONVIF_ACTION_NAME_RECV_DELETERECEIVER, NULL, NULL, 0)) return ONVIF_ACTION_RECV_DELETERECEIVER;			
	}
	return ONVIF_ACTION_UNKNOWN;
}

unsigned int ONVIF_get_service_code(char *service_path)
{	
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_DEVICE) == 0) return ONVIF_SERVICE_DEVICE;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_MEDIA) == 0) return ONVIF_SERVICE_MEDIA;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_EVENT) == 0) return ONVIF_SERVICE_EVENT;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_EVENT_0) == 0) return ONVIF_SERVICE_EVENT_0;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_SNAPSHOT) == 0) return ONVIF_SERVICE_SNAPSHOT;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_IMAGE) == 0) return ONVIF_SERVICE_IMAGE;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_PTZ) == 0) return ONVIF_SERVICE_PTZ;
	if (strcasecmp(service_path, ONVIF_SERVICE_PATH_RECV) == 0) return ONVIF_SERVICE_RECV;
	return ONVIF_SERVICE_UNKNOWN;
}

int ONVIF_get_xml_value(char *cParamName, char *cValue, unsigned int iValueLen, char* cData, unsigned int Data_len)
{
	memset(cValue, 0, iValueLen);
	unsigned int iParamLen = strlen(cParamName);
	char *cFullParamName = (char*)DBG_MALLOC(iParamLen + 2);
	memcpy(cFullParamName, cParamName, iParamLen);
	cFullParamName[iParamLen] = 62;
	iParamLen++;
	cFullParamName[iParamLen] = 0;
	
	int iFrom = SearchStrInDataCaseIgn(cData, Data_len, 0, cFullParamName);
	if (!iFrom) return 0;
	iFrom += iParamLen - 1; 
	int iTo = SearchStrInDataCaseIgn(cData, Data_len, iFrom, "</");
	if (!iTo) return 0;
	iParamLen = iTo - iFrom - 1;
	if (iParamLen > iValueLen) 
	{
		memcpy(cValue, &cData[iFrom], iValueLen);
		return 2;
	}
	else memcpy(cValue, &cData[iFrom], iParamLen);
	return 1;
}

int ONVIF_get_param_from_url(char *cUrl, unsigned int uiUrlLen, char *param_name, char *param_value, int value_len, char* cSplitter)
{
	memset(param_value, 0, value_len);
	int msg_len = uiUrlLen;
	char *msg = cUrl;
	char param[64];
	int n, ret;
	int paramlen = strlen(param_name);
	
	if (msg_len == 0) 
	{
		dbgprintf(2, "ONVIF_get_param_from_url: income str null lenght\n");
		return 0;
	}	
	if (paramlen >= 64)
	{
		dbgprintf(2, "ONVIF_get_param_from_url: big param lenght (%i)'%s'\n", strlen(param_name), param_name);
		return 0;
	}	
		
	if (paramlen == 0) ret = 0;
	else
	{
		memset(param, 0, 64);
		strcpy(param, param_name);
		strcat(param, "=");
	
		ret = SearchStrInDataCaseIgn(msg, msg_len, 0, param) - 1;
		if (ret == -1) 
		{
			dbgprintf(2, "ONVIF_get_param_from_url: param not finded '%s'\n", param);
			return 0;
		}
		if ((ret != 0) && (msg[ret-1] != ' '))  
		{
			dbgprintf(2, "ONVIF_get_param_from_url: income str bad\n");
			return 0;
		}
		ret += strlen(param);
	}	
	
	n = SearchStrInDataCaseIgn(msg, msg_len, ret, cSplitter) - 1;
	if (n == -1) n = msg_len;
	
	if ((n - ret) >= value_len) 
	{
		dbgprintf(2, "ONVIF_get_param_from_url: big len param value :%i maxlen: %i\n", n - ret, value_len);
		return 0;
	}
	if ((msg[ret] == 34) && (msg[n - 1]) == 34) { ret++; n--;}
	memcpy(param_value, &msg[ret], n - ret);
	//printf("WEB_get_param_from_url: '%s'='%s'\n", param_name, param_value);
	//return n - ret;
	return ret + (n - ret);
}

int ONVIF_get_node_param_from_url(char *cUrl, unsigned int uiUrlLen, char *node_name, char *param_name, char *param_value, int value_len)
{
	char node[64];
	int ret;
	
	memset(node, 0, 64);
	strcpy(node, "<");
	strcat(node, node_name);
	strcat(node, " />");
	
	ret = SearchStrInDataCaseIgn(cUrl, uiUrlLen, 0, node);
	if (ret == 0)
	{
		node[0] = 58;
		ret = SearchStrInDataCaseIgn(cUrl, uiUrlLen, 0, node);
	}
		
	if (ret)
	{
		if (param_value && value_len) memset(param_value, 0, value_len);
		return 1;
	}
		
	memset(node, 0, 64);
	strcpy(node, "<");
	strcat(node, node_name);
	strcat(node, " ");
		
	ret = SearchStrInDataCaseIgn(cUrl, uiUrlLen, 0, node);
	if (ret == 0)
	{
		node[0] = 58;
		ret = SearchStrInDataCaseIgn(cUrl, uiUrlLen, 0, node);
	}
		
	if ((ret == 0) && (!param_name))
	{
		memset(node, 0, 64);
		strcpy(node, "<");
		strcat(node, node_name);
		strcat(node, ">");
		ret = SearchStrInDataCaseIgn(cUrl, uiUrlLen, 0, node);
		if (ret == 0)
		{
			node[0] = 58;
			ret = SearchStrInDataCaseIgn(cUrl, uiUrlLen, 0, node);
		}
	}
		
	if (ret == 0) 
	{
		dbgprintf(5, "ONVIF_get_node_param_from_url: not finded START for node '%s'\n", node_name);
		return 0;
	}
	
	int n;
	unsigned int uiNodeLen;
	unsigned int uiStartNodePos = ret + strlen(node_name) + 1;
	
	if (cUrl[uiStartNodePos - 1] == 32)
	{
		for (n = uiStartNodePos; n < uiUrlLen; n++)
			if (cUrl[n] == 62) break;
	
		if (cUrl[n] != 62)
		{
			dbgprintf(3, "ONVIF_get_node_param_from_url: not finded END for node '%s'\n", node_name);
			return 0;
		}
		if (!param_name) uiStartNodePos = n + 1;
	}
	
	if (param_name)		
		return ONVIF_get_param_from_url(&cUrl[uiStartNodePos], n - uiStartNodePos, param_name, param_value, value_len, " ");
		
	int iCnt = 1;
	int iMode = 0;
	for (n = uiStartNodePos; n < uiUrlLen; n++)
	{		
		if ((iMode == 1) && (cUrl[n] != 47)) iCnt++;
		if ((iMode == 1) && (cUrl[n] == 47)) iCnt--;
		if (iMode == 1) iMode++; 
		if (iMode && (cUrl[n] == 62) && (n != uiStartNodePos) && (cUrl[n-1] == 47)) iCnt--;
		if (cUrl[n] == 60) iMode = 1;
		if (cUrl[n] == 62) iMode = 0;
		if (iCnt == 0) break;
	}
	if (iCnt)
	{
		dbgprintf(3, "ONVIF_get_node_param_from_url: not finded END value for node %i '%s'\n", iCnt, node_name);
		return 0;
	}
	uiNodeLen = n - uiStartNodePos - 1;
	
	if ((cUrl[uiStartNodePos] == 34) && (cUrl[uiStartNodePos + uiNodeLen - 1]) == 34) { uiStartNodePos++; uiNodeLen--;}
	
	if ((uiNodeLen > value_len) && param_value && value_len)
	{
		dbgprintf(3, "ONVIF_get_node_param_from_url: value for node '%s' very big, %i>%i\n", node_name, uiNodeLen, value_len);
		return 0;
	}
	
	if (param_value && value_len)
	{
		memset(param_value, 0, value_len);
		memcpy(param_value, &cUrl[uiStartNodePos], uiNodeLen);
	}
	
	return uiStartNodePos + uiNodeLen;
}

static char ONVIF_describe_respond_401(ONVIF_SESSION *session, char *msg_rx, char *msg_tx)
{
	char msg_body[2048];
	memset(msg_body, 0, 2048);
	strcpy(msg_body,"<html>\r\n"					
					"<head>\r\n"
					"<meta charset=\"cp866\""
					"</head>"
					"<body><a href=\"/\"><h1>401 Unauthorized.</h1></a></body>"
					"</html>\r\n");
    snprintf(msg_tx, 
             ONVIF_TX_BUF_SIZE_MAX, 
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

static char* ONVIF_GetActionName(unsigned int uiAction)
{
	switch(uiAction)
	{
		case ONVIF_ACTION_UNKNOWN: return "ONVIF_ACTION_UNKNOWN";
		case ONVIF_ACTION_TEARDOWN: return "ONVIF_ACTION_TEARDOWN";
		case ONVIF_ACTION_IMAGE_GETPREVIEWFILE: return "ONVIF_ACTION_IMAGE_GETPREVIEWFILE";
		case ONVIF_ACTION_LOGOUT: return "ONVIF_ACTION_LOGOUT";
		case ONVIF_ACTION_ERROR: return "ONVIF_ACTION_ERROR";
		
		case ONVIF_ACTION_DEVICE_GETSCOPES: return "ONVIF_ACTION_DEVICE_GETSCOPES";
		case ONVIF_ACTION_DEVICE_SETSCOPES: return "ONVIF_ACTION_DEVICE_SETSCOPES";
		case ONVIF_ACTION_DEVICE_GETDNS: return "ONVIF_ACTION_DEVICE_GETDNS";
		case ONVIF_ACTION_DEVICE_GETDEVICEINFORMATION: return "ONVIF_ACTION_DEVICE_GETDEVICEINFORMATION";
		case ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME: return "ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME";
		case ONVIF_ACTION_DEVICE_SETSYSTEMDATEANDTIME: return "ONVIF_ACTION_DEVICE_SETSYSTEMDATEANDTIME";
		case ONVIF_ACTION_DEVICE_GETCAPABILITIES: return "ONVIF_ACTION_DEVICE_GETCAPABILITIES";
		case ONVIF_ACTION_DEVICE_GETSERVICES: return "ONVIF_ACTION_DEVICE_GETSERVICES";
		case ONVIF_ACTION_DEVICE_GETUSERS: return "ONVIF_ACTION_DEVICE_GETUSERS";
		case ONVIF_ACTION_DEVICE_GETSERVICECAPABILITIES: return "ONVIF_ACTION_DEVICE_GETSERVICECAPABILITIES";
		case ONVIF_ACTION_DEVICE_GETZEROCONFIGURATION: return "ONVIF_ACTION_DEVICE_GETZEROCONFIGURATION";
		case ONVIF_ACTION_DEVICE_SETZEROCONFIGURATION: return "ONVIF_ACTION_DEVICE_SETZEROCONFIGURATION";
		case ONVIF_ACTION_DEVICE_GETNETWORKINTERFACES: return "ONVIF_ACTION_DEVICE_GETNETWORKINTERFACES";
		case ONVIF_ACTION_DEVICE_SETNETWORKINTERFACES: return "ONVIF_ACTION_DEVICE_SETNETWORKINTERFACES";
		case ONVIF_ACTION_DEVICE_GETCERTIFICATES: return "ONVIF_ACTION_DEVICE_GETCERTIFICATES";
		case ONVIF_ACTION_DEVICE_GETCERTIFICATESSTATUS: return "ONVIF_ACTION_DEVICE_GETCERTIFICATESSTATUS";
		case ONVIF_ACTION_DEVICE_GETNTP: return "ONVIF_ACTION_DEVICE_GETNTP";
		case ONVIF_ACTION_DEVICE_SETNTP: return "ONVIF_ACTION_DEVICE_SETNTP";
		case ONVIF_ACTION_DEVICE_GETHOSTNAME: return "ONVIF_ACTION_DEVICE_GETHOSTNAME";
		case ONVIF_ACTION_DEVICE_SETHOSTNAME: return "ONVIF_ACTION_DEVICE_SETHOSTNAME";
		case ONVIF_ACTION_DEVICE_GETDISCOVERYMODE: return "ONVIF_ACTION_DEVICE_GETDISCOVERYMODE";
		case ONVIF_ACTION_DEVICE_GETNETWORKPROTOCOLS: return "ONVIF_ACTION_DEVICE_GETNETWORKPROTOCOLS";
		case ONVIF_ACTION_DEVICE_GETNETWORKDEFAULTGATEWAY: return "ONVIF_ACTION_DEVICE_GETNETWORKDEFAULTGATEWAY";
		case ONVIF_ACTION_DEVICE_SYSTEMREBOOT: return "ONVIF_ACTION_DEVICE_SYSTEMREBOOT";
		case ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT: return "ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT";
		case ONVIF_ACTION_DEVICE_GETSYSTEMBACKUP: return "ONVIF_ACTION_DEVICE_GETSYSTEMBACKUP";		
		case ONVIF_ACTION_DEVICE_STARTFIRMWAREUPGRADE: return "ONVIF_ACTION_DEVICE_STARTFIRMWAREUPGRADE";
		case ONVIF_ACTION_DEVICE_GETRELAYOUTPUTS: return "ONVIF_ACTION_DEVICE_GETRELAYOUTPUTS";
		case ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSTATE: return "ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSTATE";
		case ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSETTINGS: return "ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSETTINGS";
		
		case ONVIF_ACTION_DEVICE_GETACCESSPOLICY: return "ONVIF_ACTION_DEVICE_GETACCESSPOLICY";	
		case ONVIF_ACTION_DEVICE_SETACCESSPOLICY: return "ONVIF_ACTION_DEVICE_SETACCESSPOLICY";	
		case ONVIF_ACTION_DEVICE_SETNETWORKPROTOCOLS: return "ONVIF_ACTION_DEVICE_SETNETWORKPROTOCOLS";	
		case ONVIF_ACTION_DEVICE_SETUSER: return "ONVIF_ACTION_DEVICE_SETUSER";	
		case ONVIF_ACTION_DEVICE_DELETEUSERS: return "ONVIF_ACTION_DEVICE_DELETEUSERS";
		case ONVIF_ACTION_DEVICE_CREATEUSERS: return "ONVIF_ACTION_DEVICE_CREATEUSERS";
		case ONVIF_ACTION_DEVICE_GETSYSTEMLOG: return "ONVIF_ACTION_DEVICE_GETSYSTEMLOG";	
		case ONVIF_ACTION_EVENT_GETEVENTPROPERTIES: return "ONVIF_ACTION_EVENT_GETEVENTPROPERTIES";
		case ONVIF_ACTION_EVENT_SUBSCRIBE: return "ONVIF_ACTION_EVENT_SUBSCRIBE";
		case ONVIF_ACTION_EVENT_RENEWREQUEST: return "ONVIF_ACTION_EVENT_RENEWREQUEST";
		case ONVIF_ACTION_EVENT_UNSUBSCRIBEREQUEST: return "ONVIF_ACTION_EVENT_UNSUBSCRIBEREQUEST";
		
		case ONVIF_ACTION_MEDIA_GETPROFILES: return "ONVIF_ACTION_MEDIA_GETPROFILES";	
		case ONVIF_ACTION_MEDIA_GETVIDEOSOURCES: return "ONVIF_ACTION_MEDIA_GETVIDEOSOURCES";
		case ONVIF_ACTION_MEDIA_GETVIDEOENCODERS: return "ONVIF_ACTION_MEDIA_GETVIDEOENCODERS";
		case ONVIF_ACTION_MEDIA_GETAUDIOENCODERS: return "ONVIF_ACTION_MEDIA_GETAUDIOENCODERS";
		case ONVIF_ACTION_MEDIA_GETSNAPSHOTURI: return "ONVIF_ACTION_MEDIA_GETSNAPSHOTURI";
		case ONVIF_ACTION_MEDIA_GETPROFILE: return "ONVIF_ACTION_MEDIA_GETPROFILE";
		case ONVIF_ACTION_MEDIA_GETSTREAMURI: return "ONVIF_ACTION_MEDIA_GETSTREAMURI";
		case ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONF: return "ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONF";
		case ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONFS: return "ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONFS";
		case ONVIF_ACTION_MEDIA_GETVIDEOENCODERCONF: return "ONVIF_ACTION_MEDIA_GETVIDEOENCODERCONF";
		case ONVIF_ACTION_MEDIA_GETAUDIOSOURCES: return "ONVIF_ACTION_MEDIA_GETAUDIOSOURCES";
		case ONVIF_ACTION_MEDIA_CREATEPROFILE: return "ONVIF_ACTION_MEDIA_CREATEPROFILE";
		case ONVIF_ACTION_MEDIA_DELETEPROFILE: return "ONVIF_ACTION_MEDIA_DELETEPROFILE";
		case ONVIF_ACTION_MEDIA_GETCOMPATIBLEVIDEOENCODER: return "ONVIF_ACTION_MEDIA_GETCOMPATIBLEVIDEOENCODER";
		case ONVIF_ACTION_MEDIA_GETCOMPATIBLEMETADATACONF: return "ONVIF_ACTION_MEDIA_GETCOMPATIBLEMETADATACONF";
		case ONVIF_ACTION_MEDIA_GETAUDIOSOURCECONFS: return "ONVIF_ACTION_MEDIA_GETAUDIOSOURCECONFS";
		case ONVIF_ACTION_MEDIA_SETVIDEOENCODERCONF: return "ONVIF_ACTION_MEDIA_SETVIDEOENCODERCONF";
		case ONVIF_ACTION_MEDIA_GETCOMPVIDEOANALITCONF: return "ONVIF_ACTION_MEDIA_GETCOMPVIDEOANALITCONF";
		case ONVIF_ACTION_MEDIA_GETCOMPAUDIOENCCONF: return "ONVIF_ACTION_MEDIA_GETCOMPAUDIOENCCONF";
		case ONVIF_ACTION_MEDIA_GETCOMPMETADATACONF: return "ONVIF_ACTION_MEDIA_GETCOMPMETADATACONF";
		case ONVIF_ACTION_MEDIA_GETCOMPVIDEOENCCONF: return "ONVIF_ACTION_MEDIA_GETCOMPVIDEOENCCONF";
		case ONVIF_ACTION_MEDIA_ADDVIDEOENCCONF: return "ONVIF_ACTION_MEDIA_ADDVIDEOENCCONF";
		case ONVIF_ACTION_MEDIA_ADDAUDIOENCCONF: return "ONVIF_ACTION_MEDIA_ADDAUDIOENCCONF";
		case ONVIF_ACTION_MEDIA_ADDVIDEOANALITCONF: return "ONVIF_ACTION_MEDIA_ADDVIDEOANALITCONF";
		case ONVIF_ACTION_MEDIA_ADDMETADATACONFIGURATION: return "ONVIF_ACTION_MEDIA_ADDMETADATACONFIGURATION";
		case ONVIF_ACTION_MEDIA_ADDPTZCONFIGURATION: return "ONVIF_ACTION_MEDIA_ADDPTZCONFIGURATION";
		case ONVIF_ACTION_MEDIA_GETVIDEOANALITICSCONFS: return "ONVIF_ACTION_MEDIA_GETVIDEOANALITICSCONFS";
		case ONVIF_ACTION_MEDIA_REMOVEVIDEOENCCONF: return "ONVIF_ACTION_MEDIA_REMOVEVIDEOENCCONF";
		case ONVIF_ACTION_MEDIA_REMOVEAUDIOENCCONF: return "ONVIF_ACTION_MEDIA_REMOVEAUDIOENCCONF";
		case ONVIF_ACTION_MEDIA_REMOVEVIDEOANALITCONF: return "ONVIF_ACTION_MEDIA_REMOVEVIDEOANALITCONF";
		case ONVIF_ACTION_MEDIA_REMOVEMETADATACONFIGURATION: return "ONVIF_ACTION_MEDIA_REMOVEMETADATACONFIGURATION";
		case ONVIF_ACTION_MEDIA_REMOVEPTZCONFIGURATION: return "ONVIF_ACTION_MEDIA_REMOVEPTZCONFIGURATION";
		
		case ONVIF_ACTION_IMAGE_GETMOVEOPTIONS: return "ONVIF_ACTION_IMAGE_GETMOVEOPTIONS";
		case ONVIF_ACTION_IMAGE_GETIMAGINGSETTINGS: return "ONVIF_ACTION_IMAGE_GETIMAGINGSETTINGS";
		case ONVIF_ACTION_IMAGE_GETOPTIONS: return "ONVIF_ACTION_IMAGE_GETOPTIONS";
		case ONVIF_ACTION_IMAGE_GETSTATUS: return "ONVIF_ACTION_IMAGE_GETSTATUS";
		case ONVIF_ACTION_IMAGE_SETIMAGINGSETTINGS: return "ONVIF_ACTION_IMAGE_SETIMAGINGSETTINGS";
		case ONVIF_ACTION_IMAGE_STOP: return "ONVIF_ACTION_IMAGE_STOP";
		case ONVIF_ACTION_IMAGE_MOVE: return "ONVIF_ACTION_IMAGE_MOVE";
		
		
		case ONVIF_ACTION_PTZ_GETNODE: return "ONVIF_ACTION_PTZ_GETNODE";
		case ONVIF_ACTION_PTZ_GETNODES: return "ONVIF_ACTION_PTZ_GETNODES";
		case ONVIF_ACTION_PTZ_GETSTATUS: return "ONVIF_ACTION_PTZ_GETSTATUS";
		case ONVIF_ACTION_PTZ_GETPRESETS: return "ONVIF_ACTION_PTZ_GETPRESETS";
		case ONVIF_ACTION_PTZ_REMOVEPRESET: return "ONVIF_ACTION_PTZ_REMOVEPRESET";
		case ONVIF_ACTION_PTZ_GETCONFIGURATIONS: return "ONVIF_ACTION_PTZ_GETCONFIGURATIONS";
		case ONVIF_ACTION_PTZ_STOP: return "ONVIF_ACTION_PTZ_STOP";
		case ONVIF_ACTION_PTZ_CONTINUOUSMOVE: return "ONVIF_ACTION_PTZ_CONTINUOUSMOVE";
		case ONVIF_ACTION_PTZ_RELATIVEMOVE: return "ONVIF_ACTION_PTZ_RELATIVEMOVE";
		case ONVIF_ACTION_PTZ_GOTOHOMEPOSITION: return "ONVIF_ACTION_PTZ_GOTOHOMEPOSITION";
		case ONVIF_ACTION_PTZ_GOTOPRESET: return "ONVIF_ACTION_PTZ_GOTOPRESET";
		case ONVIF_ACTION_PTZ_SETPRESET: return "ONVIF_ACTION_PTZ_SETPRESET";
		case ONVIF_ACTION_PTZ_SETHOMEPOSITION: return "ONVIF_ACTION_PTZ_SETHOMEPOSITION";		
		case ONVIF_ACTION_PTZ_ABSOLUTEMOVE: return "ONVIF_ACTION_PTZ_ABSOLUTEMOVE";
		
		case ONVIF_ACTION_EVENT_CREATEPULLPOINTSUB: return "ONVIF_ACTION_EVENT_CREATEPULLPOINTSUB";
		case ONVIF_ACTION_EVENT_PULLMESSAGES: return "ONVIF_ACTION_EVENT_PULLMESSAGES";

		case ONVIF_ACTION_RECV_GETRECEIVERS: return "ONVIF_ACTION_RECV_GETRECEIVERS";
		case ONVIF_ACTION_RECV_CREATERECEIVER: return "ONVIF_ACTION_RECV_CREATERECEIVER";
		case ONVIF_ACTION_RECV_CONFIGURERECEIVER: return "ONVIF_ACTION_RECV_CONFIGURERECEIVER";
		case ONVIF_ACTION_RECV_DELETERECEIVER: return "ONVIF_ACTION_RECV_DELETERECEIVER";
		default: return "ONVIF_ACTION_UNKNOWN";
	}
	return "ONVIF_ACTION_UNKNOWN";
}

static char* ONVIF_GetServiceTypeName(unsigned int uiServiceCode)
{
	switch(uiServiceCode)
	{
		case ONVIF_SERVICE_DEVICE: return "ONVIF_SERVICE_DEVICE";
		case ONVIF_SERVICE_EVENT: return "ONVIF_SERVICE_EVENT";
		case ONVIF_SERVICE_EVENT_0: return "ONVIF_SERVICE_EVENT_0";
		case ONVIF_SERVICE_MEDIA: return "ONVIF_SERVICE_MEDIA";		
		case ONVIF_SERVICE_SNAPSHOT: return "ONVIF_SERVICE_SNAPSHOT";
		case ONVIF_SERVICE_IMAGE: return "ONVIF_SERVICE_IMAGE";
		case ONVIF_SERVICE_PTZ: return "ONVIF_SERVICE_PTZ";
		default: return "ONVIF_SERVICE_UNKNOWN";
	}
	return "ONVIF_SERVICE_UNKNOWN";
}

static int ONVIF_get_line(char *pData, unsigned int uiDataLen, unsigned int uiPos, unsigned int *uiLineLen, char cSplit)
{
	int n;
	*uiLineLen = 0;
	if (uiPos >= uiDataLen) return 0;
	*uiLineLen = uiDataLen - uiPos;
	
	for (n = uiPos; n < uiDataLen; n++)
	{
		if ((cSplit) && (pData[n] == 10)) pData[n] = 0;
		if (pData[n] == 13)
		{
			*uiLineLen = n - uiPos;
			if (cSplit) pData[n] = 0;
			break;
		}
	}
	return 1;
}

static char ONVIF_get_msg_type(char *pData, unsigned int uiDataLen, HTTP_HEAD *hhHead, char cTest)
{
	unsigned int uiPos = 0;
	unsigned int uiLineLen = 0;
	int ret;
	memset(hhHead, 0, sizeof(HTTP_HEAD));
	
	while(1)
	{
		ret = ONVIF_get_line(pData, uiDataLen, uiPos, &uiLineLen, cTest ? 0 : 1);
		if (!ret || !uiLineLen) break;
		if (ret && uiLineLen)
		{
			if (!hhHead->Mode && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_POST) == 1)) hhHead->Mode = 1;
			if (!hhHead->Mode && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_GET) == 1)) hhHead->Mode = 2;
			if (!hhHead->Path && hhHead->Mode) 
			{
				hhHead->Path = &pData[uiPos + strlen(HEAD_MARK_POST)];
				int len = SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_HTTP) - 1;
				if ((cTest == 0) && (len >= 0)) pData[uiPos + len] = 0;
				hhHead->ServiceType = ONVIF_get_service_code(hhHead->Path);
			}
			if (!hhHead->ContentType && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_CONTENTTYPE) == 1)) 
				hhHead->ContentType = &pData[uiPos + strlen(HEAD_MARK_CONTENTTYPE)];			
			if (!hhHead->Host && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_HOST) == 1)) hhHead->Host = &pData[uiPos + strlen(HEAD_MARK_HOST)];
			if (!hhHead->ContentLength && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_CONTENTLENGTH) == 1)) 
				hhHead->ContentLength = Str2IntLimit(&pData[uiPos + strlen(HEAD_MARK_CONTENTLENGTH)], uiLineLen - strlen(HEAD_MARK_CONTENTLENGTH));
			if (!hhHead->AcceptEncoding && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_ACCEPTENCODING) == 1)) hhHead->AcceptEncoding = &pData[uiPos + strlen(HEAD_MARK_ACCEPTENCODING)];
			if (!hhHead->Connection && (SearchStrInDataCaseIgn(&pData[uiPos], uiLineLen, 0, HEAD_MARK_CONNECTION) == 1)) hhHead->Connection = &pData[uiPos + strlen(HEAD_MARK_CONNECTION)];			 
		} 		
		uiPos += uiLineLen + 2;
	}
	uiPos++;
	hhHead->Body = &pData[uiPos];
	hhHead->BodyLength = uiDataLen - uiPos;
	//printf("%i %i\n", hhHead->ContentLength, hhHead->BodyLength);
	if (hhHead->ContentLength > hhHead->BodyLength) return 0;
	//int i;
	//for (i = uiPos; i < uiDataLen; i++) if (pData[uiPos] < 32) pData[uiPos] = 32;
	
	hhHead->ContentTypeDetail.Action = ONVIF_get_action_code(hhHead->Body, hhHead->BodyLength, hhHead->ServiceType);	
	//hhHead->ContentTypeDetail.Action = ONVIF_get_action_code(pData, uiDataLen, hhHead->ServiceType);	
	if ((hhHead->ServiceType == ONVIF_SERVICE_SNAPSHOT) && (hhHead->ContentTypeDetail.Action == ONVIF_ACTION_UNKNOWN))
				hhHead->ContentTypeDetail.Action = ONVIF_ACTION_IMAGE_GETPREVIEWFILE;
			
	//printf("%s", pData);
	return 1;
}

int ONVIF_test_nonce(char *t_nonce)
{
	if (SearchStrInData(ONVIF_Nonce_Buffer, ONVIF_NONCE_CASH_SIZE, 0, t_nonce)) return 0;
	int len = strlen(t_nonce);
	if ((ONVIF_Nonce_Buff_Pos + len) >= ONVIF_NONCE_CASH_SIZE) ONVIF_Nonce_Buff_Pos = 0;
	if (len >= ONVIF_NONCE_CASH_SIZE) return 0;
	memcpy(&ONVIF_Nonce_Buffer[ONVIF_Nonce_Buff_Pos], t_nonce, len);
	ONVIF_Nonce_Buff_Pos += len;
	return 1;
}
	
int ONVIF_test_date(char *t_date)
{
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
	timeinfo.tm_mday = Str2IntLimit(&t_date[8], 2);
	timeinfo.tm_mon = Str2IntLimit(&t_date[5], 2) - 1;
	timeinfo.tm_year = Str2IntLimit(&t_date[0], 4) - 1900;
	timeinfo.tm_hour = Str2IntLimit(&t_date[11], 2);
	timeinfo.tm_min = Str2IntLimit(&t_date[14], 2);
	timeinfo.tm_sec = Str2IntLimit(&t_date[17], 2);
	time_t testtime1 = mktime(&timeinfo);	
	time_t testtime2 = mktime(&timeinfo) - timezone;	
	time_t nowtime;
	time(&nowtime);		
	
	/*struct tm tm_info;
	struct tm testinfo1;
	struct tm testinfo2;
	localtime_r(&nowtime, &tm_info);
	localtime_r(&testtime1, &testinfo1);
	localtime_r(&testtime2, &testinfo2);
	char buffer[256];
	memset(buffer, 0, 256);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", &tm_info);
	printf("%s\n", buffer);
	memset(buffer, 0, 256);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", &testinfo1);
	printf("%s\n", buffer);
	memset(buffer, 0, 256);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S !!", &testinfo2);
	printf("%s %i %i\n", buffer, nowtime - testtime1, nowtime-testtime2);*/
	if ((((nowtime + 10) >= testtime1) && ((nowtime - 10) <= testtime1)) ||
		(((nowtime + 10) >= testtime2) && ((nowtime - 10) <= testtime2))) return 1;
	return 0;
}

void ONVIF_SHA1(char * pszNonce, int iNonceLen, char * pszDate, char * pszPassword, char sResult[20])
{
	int len = iNonceLen + strlen(pszDate) + strlen(pszPassword);
	char *source_date = (char*)DBG_MALLOC(len);
	memcpy(source_date, pszNonce, iNonceLen);
	memcpy(&source_date[iNonceLen], pszDate, strlen(pszDate));
	memcpy(&source_date[iNonceLen + strlen(pszDate)], pszPassword, strlen(pszPassword));
    SHA1((unsigned char *)source_date, len, (unsigned char *)sResult);	
	DBG_FREE(source_date);
}

static char ONVIF_Auth(ONVIF_SESSION *session, char *msg_rx, unsigned int iReaded, HTTP_HEAD *pHEAD)
{
	char usernametoken[1024];	
	char username[64];	
	char password[64];	
	char nonce[64];	
	char nonce_raw[128];	
	char created[64];	
	usernametoken[1023] = 0;	
	username[63] = 0;	
	password[63] = 0;	
	nonce[63] = 0;	
	created[63] = 0;	
	memset(nonce_raw, 0, 128);
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	
	int ret = 0;
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "UsernameToken", NULL, usernametoken, 1023))
	{
		int len = strlen(usernametoken);
		if (ONVIF_get_node_param_from_url(usernametoken, len, "Username", NULL, username, 63) &&
			ONVIF_get_node_param_from_url(usernametoken, len, "Password", NULL, password, 63) &&
			ONVIF_get_node_param_from_url(usernametoken, len, "Nonce", NULL, nonce, 63) &&
			ONVIF_get_node_param_from_url(usernametoken, len, "Created", NULL, created, 63))
		{
			ret = 1;
		}
	}
	if (!ret) 
	{
		dbgprintf(2, "ONVIF_Auth: Not found node 'UsernameToken'\n");
		return 0;
	}
	if (!ONVIF_test_date(created))
	{
		ONVIF_Auth_lock_time = nanotime.tv_sec;
		dbgprintf(2, "ONVIF_Auth: wrong date '%s'\n", created);
		return 0;
	}
	if (!ONVIF_test_nonce(nonce))
	{
		ONVIF_Auth_lock_time = nanotime.tv_sec;
		dbgprintf(2, "ONVIF_Auth: repeat nonce\n");
		return 0;
	}
	
	DBG_MUTEX_LOCK(&user_mutex);
	unsigned int n;
	ret = 0;	
	for (n = 0; n < iUserCnt; n++)
	{
		if ((uiUserList[n].Enabled) && (uiUserList[n].Access & ACCESS_ONVIF) && (SearchStrInDataCaseIgn(uiUserList[n].Login, strlen(uiUserList[n].Login), 0, username)))
		{
			memset(session->login, 0, 64);
			memset(session->password, 0, 64);
			strcpy(session->login, uiUserList[n].Login);
			strcpy(session->password, uiUserList[n].Password);
			ret = 1;
			break;
		}
	}	
	DBG_MUTEX_UNLOCK(&user_mutex);
	
	
	if (ret != 1) 
	{
		ONVIF_Auth_lock_time = nanotime.tv_sec;
		dbgprintf(2, "ONVIF_Auth: wrong username '%s'\n", username);
		return 0;
	}
	
	if (strlen(nonce) > 32) 
	{
		dbgprintf(3, "ONVIF_Auth: Wrong nonce\n");
		return -1;
	}
	
	char digest[20]; 
    int nonce_raw_len = av_base64_decode((uint8_t *)nonce_raw, nonce, 128);
	ONVIF_SHA1(nonce_raw, nonce_raw_len, created, session->password, digest);
	
	char digest_response[64];
	memset(digest_response, 0, 64);
	av_base64_encode(digest_response, 64, (uint8_t *)digest, 20);

	if (strcmp(digest_response, password) != 0) 
	{
		dbgprintf(2, "ONVIF_Auth: Wrong password\n");
		ONVIF_Auth_lock_time = nanotime.tv_sec;
		return 0;
	}
	
	return 1;
}

static void ONVIV_Template_Head(char *buff, unsigned int buffsize, char *serv_name, char *content_type, unsigned int body_len, char *connection)
{
	snprintf(buff, buffsize, 
			"HTTP/1.1 200 OK\r\n"
			"Server: %s\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %i\r\n"
			"Connection: %s\r\n\r\n", serv_name, content_type, body_len, connection);
}

static void ONVIV_Template_Neck(char *buff, unsigned int buffsize)
{
	snprintf(buff, buffsize, 
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
			"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"\r\n" 
		//	"xmlns:e=\"http://www.w3.org/2003/05/soap-encoding\"\r\n"
			"xmlns:wsa=\"http://www.w3.org/2005/08/addressing\"\r\n"
			"xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"\r\n"
			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n"
			"xmlns:wsaw=\"http://www.w3.org/2006/05/addressing/wsdl\"\r\n"
			"xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\"\r\n"
			"xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\"\r\n" 
			"xmlns:wsntw=\"http://docs.oasis-open.org/wsn/bw-2\"\r\n" 
			"xmlns:wsrf-rw=\"http://docs.oasis-open.org/wsrf/rw-2\"\r\n"
			"xmlns:wsrf-r=\"http://docs.oasis-open.org/wsrf/r-2\"\r\n"
			"xmlns:wsrf-bf=\"http://docs.oasis-open.org/wsrf/bf-2\"\r\n"
			"xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl\"\r\n"
			"xmlns:wsoap12=\"http://schemas.xmlsoap.org/wsdl/soap12\"\r\n"
			"xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http\"\r\n"
			"xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"\r\n"
			"xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\"\r\n"
			"xmlns:tt=\"http://www.onvif.org/ver10/schema\"\r\n"
			"xmlns:tns1=\"http://www.onvif.org/ver10/topics\"\r\n"
			"xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\"\r\n"
			"xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\"\r\n"
		//	"xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\"\r\n"
			"xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\"\r\n"
			"xmlns:tst=\"http://www.onvif.org/ver10/storage/wsdl\"\r\n"
			"xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\"\r\n"
			"xmlns:pt=\"http://www.onvif.org/ver10/pacs\"\r\n"
			"xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\"\r\n"
			"xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\"\r\n"
		//	"xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\"\r\n"
		//	"xmlns:axt=\"http://www.onvif.org/ver20/analytics\"\r\n"
		//	"xmlns:trp=\"http://www.onvif.org/ver10/replay/wsdl\"\r\n"
		//	"xmlns:tse=\"http://www.onvif.org/ver10/search/wsdl\"\r\n"
		//	"xmlns:trc=\"http://www.onvif.org/ver10/recording/wsdl\"\r\n"
		//	"xmlns:tac=\"http://www.onvif.org/ver10/accesscontrol/wsdl\"\r\n"
			"xmlns:tdc=\"http://www.onvif.org/ver10/doorcontrol/wsdl\"\r\n"
			"xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\"\r\n"
			"xmlns:tth=\"http://www.onvif.org/ver10/thermal/wsdl\"\r\n"
			"xmlns:tcr=\"http://www.onvif.org/ver10/credential/wsdl\"\r\n"
		//	"xmlns:tar=\"http://www.onvif.org/ver10/accessrules/wsdl\"\r\n"
		//	"xmlns:tsc=\"http://www.onvif.org/ver10/schedule/wsdl\"\r\n"
		//	"xmlns:trv=\"http://www.onvif.org/ver10/receiver/wsdl\"\r\n"
		//	"xmlns:tpv=\"http://www.onvif.org/ver10/provisioning/wsdl\"\r\n"
			"xmlns:ter=\"http://www.onvif.org/ver10/error\">\r\n");
}

int ONVIF_GetImage(ONVIF_SESSION *session, HTTP_HEAD *pHEAD, char *msg_tx)
{
	int ret = 0;
	char cPath[512];
	memset(cPath, 0, 512);
	
	if (strlen(session->CameraPreviewFile))
	{
		misc_buffer mbuffer;
		memset(&mbuffer, 0, sizeof(misc_buffer));
		if (ONVIF_load_limit_file_in_buff(session->CameraPreviewFile, &mbuffer, ONVIF_TX_BUF_SIZE_MAX - 1024) == 1)
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
		} else dbgprintf(3, "No file or file big %s\n", session->CameraPreviewFile);
	} else dbgprintf(2, "Unknown file type request (action:%s)\n", pHEAD->ContentType);
	
	return ret;
}

static void ONVIV_Template_Dev_GetSystemDateAndTime(char *buff, unsigned int buffsize)
{
	struct tm timeinfo;
	time_t rawtime;
	time(&rawtime);	
	localtime_r(&rawtime, &timeinfo);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetSystemDateAndTimeResponse>\r\n"
			"<tds:SystemDateAndTime>\r\n"
			"<tt:DateTimeType>NTP</tt:DateTimeType>\r\n"
			"<tt:DaylightSavings>false</tt:DaylightSavings>\r\n"
			"<tt:TimeZone>\r\n"
			"<tt:TZ>PST4PDT</tt:TZ>\r\n"
			"</tt:TimeZone>\r\n"
			"<tt:UTCDateTime>\r\n"
			"<tt:Time>\r\n"
			"<tt:Hour>%i</tt:Hour>\r\n"
			"<tt:Minute>%i</tt:Minute>\r\n"
			"<tt:Second>%i</tt:Second>\r\n"
			"</tt:Time>\r\n"
			"<tt:Date>\r\n"
			"<tt:Year>%i</tt:Year>\r\n"
			"<tt:Month>%i</tt:Month>\r\n"
			"<tt:Day>%i</tt:Day>\r\n"
			"</tt:Date>\r\n"
			"</tt:UTCDateTime>\r\n"
			"</tds:SystemDateAndTime>\r\n"
			"</tds:GetSystemDateAndTimeResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday);
}

static void ONVIV_Template_Dev_SetSystemDateAndTime(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char datetimetype[32];
	char utcdatetime[256];
	char hour[32];
	char minute[32];
	char second[32];
	char year[32];
	char month[32];
	char day[32];
	
	datetimetype[31] = 0;	
	utcdatetime[255] = 0;	
	hour[31] = 0;	
	minute[31] = 0;
	second[31] = 0;	
	year[31] = 0;
	month[31] = 0;	
	day[31] = 0;
	
	int res = 0;
	res |= !ONVIF_get_xml_value("DateTimeType", datetimetype, 31, pHEAD->Body, pHEAD->BodyLength);
	res |= !ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "UTCDateTime", NULL, utcdatetime, 255);
	if (!res)
	{
		res |= !ONVIF_get_node_param_from_url(utcdatetime, strlen(utcdatetime), "Hour", NULL, hour, 31);
		res |= !ONVIF_get_node_param_from_url(utcdatetime, strlen(utcdatetime), "Minute", NULL, minute, 31);
		res |= !ONVIF_get_node_param_from_url(utcdatetime, strlen(utcdatetime), "Second", NULL, second, 31);
		res |= !ONVIF_get_node_param_from_url(utcdatetime, strlen(utcdatetime), "Year", NULL, year, 31);
		res |= !ONVIF_get_node_param_from_url(utcdatetime, strlen(utcdatetime), "Month", NULL, month, 31);
		res |= !ONVIF_get_node_param_from_url(utcdatetime, strlen(utcdatetime), "Day", NULL, day, 31);
	}
	
	if (!res)
	{
		if (strcasecmp(datetimetype, "Manual") == 0)
		{
			struct tm timeinfo;
			memset(&timeinfo, 0, sizeof(timeinfo));
			timeinfo.tm_mday = Str2Int(day);
			timeinfo.tm_mon = Str2Int(month) - 1;
			timeinfo.tm_year = Str2Int(year) - 1900;
			timeinfo.tm_hour = Str2Int(hour);
			timeinfo.tm_min = Str2Int(minute);
			timeinfo.tm_sec = Str2Int(second);
			time_t rawtime = mktime(&timeinfo);
			a_stime(&rawtime);
		}
	}

	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:SetSystemDateAndTimeResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_GetScopes(char *buff, unsigned int buffsize, char *country, MODULE_INFO* miCamera)
{
	unsigned int hardware_id = 0;
	if (miCamera) hardware_id = miCamera->ID;
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetScopesResponse>\r\n"
				"<tds:Scopes><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/Streaming</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/T</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/S</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/G</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/C</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/A</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Configurable</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/location/country/%s</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Configurable</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/type/video_encoder</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Configurable</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/name/%.4s</tt:ScopeItem></tds:Scopes>\r\n"
				"<tds:Scopes><tt:ScopeDef>Configurable</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/hardware/%.4s</tt:ScopeItem></tds:Scopes>\r\n"
			"</tds:GetScopesResponse></s:Body>\r\n"
			"</s:Envelope>\r\n",
			country, (char*)&hardware_id, (char*)&hardware_id);	
}

static void ONVIV_Template_Dev_GetNetworkInterfaces(char *buff, unsigned int buffsize)
{
	eth_config ifr;
	GetLocalNetIf(&ifr);
	unsigned int netmask;
	netmask = ifr.if_netmask.sa_data[2] << 24;
	netmask |= ifr.if_netmask.sa_data[3] << 16;
	netmask |= ifr.if_netmask.sa_data[4] << 8;
	netmask |= ifr.if_netmask.sa_data[5];
	int mask = 32;
	while(netmask && ((netmask & 1) == 0)) {mask--; netmask >>= 1;}
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetNetworkInterfacesResponse>\r\n"
				"<tds:NetworkInterfaces token=\"%s\">\r\n"
					"<tt:Enabled>true</tt:Enabled>\r\n"
					"<tt:Info>\r\n"
						"<tt:HwAddress>%02x:%02x:%02x:%02x:%02x:%02x</tt:HwAddress>\r\n"
						"<tt:MTU>%i</tt:MTU>\r\n"
					"</tt:Info>\r\n"
					"<tt:IPv4>\r\n"
						"<tt:Enabled>true</tt:Enabled>\r\n"
						"<tt:Config>\r\n"
							"<tt:Manual>\r\n"
								"<tt:Address>%i.%i.%i.%i</tt:Address>\r\n"
								"<tt:PrefixLength>%i</tt:PrefixLength>\r\n"
							"</tt:Manual>\r\n"
							"<tt:DHCP>false</tt:DHCP>\r\n"
						"</tt:Config>\r\n"
					"</tt:IPv4>\r\n"
				"</tds:NetworkInterfaces>\r\n"
			"</tds:GetNetworkInterfacesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>", 
			ifr.if_name, 
			(unsigned char)ifr.if_hwaddr.sa_data[0],
			(unsigned char)ifr.if_hwaddr.sa_data[1],
			(unsigned char)ifr.if_hwaddr.sa_data[2],
			(unsigned char)ifr.if_hwaddr.sa_data[3],
			(unsigned char)ifr.if_hwaddr.sa_data[4],
			(unsigned char)ifr.if_hwaddr.sa_data[5],
			ifr.if_mtu,
			(unsigned char)ifr.if_addr.sa_data[2],
			(unsigned char)ifr.if_addr.sa_data[3],
			(unsigned char)ifr.if_addr.sa_data[4],
			(unsigned char)ifr.if_addr.sa_data[5],
			mask);	
}

static void ONVIV_Template_Dev_SetNetworkInterfaces(char *buff, unsigned int buffsize)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:SetNetworkInterfacesResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>");
}

static void ONVIV_Template_Dev_GetDeviceInformation(char *buff, unsigned int buffsize, char *manufacturer, MODULE_INFO* miCamera)
{
	MODULE_INFO mi;
	MODULE_INFO *pmiOut = &mi;
	memset(&mi, 0, sizeof(MODULE_INFO));
	if (miCamera) pmiOut = miCamera;
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetDeviceInformationResponse>\r\n"
			"<tds:Manufacturer>%s</tds:Manufacturer>\r\n"
			"<tds:Model>%s</tds:Model>\r\n"
			"<tds:FirmwareVersion>%i.%i.%i.%i</tds:FirmwareVersion>\r\n"
			"<tds:SerialNumber>000000</tds:SerialNumber>\r\n"
			"<tds:HardwareId>%.4s</tds:HardwareId>\r\n"
			"</tds:GetDeviceInformationResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			manufacturer, pmiOut->Name, pmiOut->Version[0], pmiOut->Version[1], pmiOut->Version[2], pmiOut->Version[3], (char*)&pmiOut->ID);
}

static void ONVIV_Template_Dev_GetDNS(char *buff, unsigned int buffsize)
{
	char dns[64];
	
	GetLocalNetIfFromDhcpcd(NULL, NULL, NULL, dns, 64);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetDNSResponse>\r\n"
				"<tds:DNSInformation>\r\n"
					"<tt:FromDHCP>false</tt:FromDHCP>\r\n"
					"<tt:DNSManual>\r\n"
						"<tt:Type>IPv4</tt:Type>\r\n"
						"<tt:IPv4Address>%s</tt:IPv4Address>\r\n"
					"</tt:DNSManual>\r\n"
				"</tds:DNSInformation>\r\n"
			"</tds:GetDNSResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", dns);
}

static void ONVIV_Template_Dev_GetCapabilities(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	int iVideoSourcesCnt = session->CameraModule ? 1 : 0;
	int iVideoOutputsCnt = 0;
	int iAudioSourcesCnt = session->MicCount;
	int iAudioOutputsCnt = 0;
	int iRelayOutputsCnt = 0;
	int n;
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) 
				&& (miModuleList[n].Type == MODULE_TYPE_GPIO) 
				&& (miModuleList[n].Settings[0] & MODULE_SECSET_OUTPUT)) iRelayOutputsCnt++;		
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetCapabilitiesResponse>\r\n"
				"<tds:Capabilities>\r\n"
					/*"<tt:Analytics>\r\n"
						"<tt:XAddr>http://%s/onvif/analytics_service</tt:XAddr>\r\n"
						"<tt:RuleSupport>false</tt:RuleSupport>\r\n"
						"<tt:AnalyticsModuleSupport>false</tt:AnalyticsModuleSupport>\r\n"
					"</tt:Analytics>\r\n"*/
					"<tt:Device>\r\n"
						"<tt:XAddr>http://%s/onvif/device_service</tt:XAddr>\r\n"
						"<tt:Network>\r\n"
							"<tt:IPFilter>false</tt:IPFilter>\r\n"
							"<tt:ZeroConfiguration>true</tt:ZeroConfiguration>\r\n"
							"<tt:IPVersion6>false</tt:IPVersion6>\r\n"
							"<tt:DynDNS>false</tt:DynDNS>\r\n"
							"<tt:Extension>\r\n"
								"<tt:Dot11Configuration>true</tt:Dot11Configuration>\r\n"
							"</tt:Extension>\r\n"
						"</tt:Network>\r\n"
						"<tt:System>\r\n"
							"<tt:DiscoveryResolve>true</tt:DiscoveryResolve>\r\n"
							"<tt:DiscoveryBye>true</tt:DiscoveryBye>\r\n"
							"<tt:RemoteDiscovery>true</tt:RemoteDiscovery>\r\n"
							"<tt:SystemBackup>false</tt:SystemBackup>\r\n"
							"<tt:SystemLogging>true</tt:SystemLogging>\r\n"
							"<tt:FirmwareUpgrade>false</tt:FirmwareUpgrade>\r\n"
							"<tt:SupportedVersions>\r\n"
								"<tt:Major>0</tt:Major>\r\n"
								"<tt:Minor>0</tt:Minor>\r\n"
							"</tt:SupportedVersions>\r\n"
							"<tt:Extension>\r\n"
								"<tt:HttpFirmwareUpgrade>false</tt:HttpFirmwareUpgrade>\r\n"
								"<tt:HttpSystemBackup>false</tt:HttpSystemBackup>\r\n"
								"<tt:HttpSystemLogging>false</tt:HttpSystemLogging>\r\n"
								"<tt:HttpSupportInformation>false</tt:HttpSupportInformation>\r\n"
							"</tt:Extension>\r\n"
						"</tt:System>\r\n"
						"<tt:IO>\r\n"
							"<tt:InputConnectors>1</tt:InputConnectors>\r\n"
							"<tt:RelayOutputs>1</tt:RelayOutputs>\r\n"
							"<tt:Extension>\r\n"
								"<tt:Auxiliary>true</tt:Auxiliary>\r\n"
							"<tt:Extension/></tt:Extension>\r\n"
						"</tt:IO>\r\n"
					/*	"<tt:Security>\r\n"
							"<tt:TLS1.1>false</tt:TLS1.1>\r\n"
							"<tt:TLS1.2>false</tt:TLS1.2>\r\n"
							"<tt:OnboardKeyGeneration>false</tt:OnboardKeyGeneration>\r\n"
							"<tt:AccessPolicyConfig>true</tt:AccessPolicyConfig>\r\n"
							"<tt:X.509Token>false</tt:X.509Token>\r\n"
							"<tt:SAMLToken>false</tt:SAMLToken>\r\n"
							"<tt:KerberosToken>false</tt:KerberosToken>\r\n"
							"<tt:RELToken>false</tt:RELToken>\r\n"
							"<tt:Extension>\r\n"
								"<tt:TLS1.0>false</tt:TLS1.0>\r\n"
								"<tt:Extension>\r\n"
									"<tt:Dot1X>true</tt:Dot1X>\r\n"
									"<tt:SupportedEAPMethod>0</tt:SupportedEAPMethod>\r\n"
									"<tt:RemoteUserHandling>true</tt:RemoteUserHandling>\r\n"
								"</tt:Extension>\r\n"
							"</tt:Extension>\r\n"
						"</tt:Security>\r\n"*/
					"</tt:Device>\r\n"
					/*"<tt:Events>\r\n"
						"<tt:XAddr>http://%s/onvif/event_service</tt:XAddr>\r\n"
						"<tt:WSSubscriptionPolicySupport>false</tt:WSSubscriptionPolicySupport>\r\n"
						"<tt:WSPullPointSupport>false</tt:WSPullPointSupport>\r\n"
						"<tt:WSPausableSubscriptionManagerInterfaceSupport>false</tt:WSPausableSubscriptionManagerInterfaceSupport>\r\n"
					"</tt:Events>\r\n"*/
					"<tt:Imaging>\r\n"
						"<tt:XAddr>http://%s/onvif/image_service</tt:XAddr>\r\n"
					"</tt:Imaging>\r\n"
					"<tt:Media>\r\n"
						"<tt:XAddr>http://%s/onvif/media_service</tt:XAddr>\r\n"
						"<tt:StreamingCapabilities>\r\n"
							"<tt:RTPMulticast>false</tt:RTPMulticast>\r\n"
							"<tt:RTP_TCP>true</tt:RTP_TCP>\r\n"
							"<tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP>\r\n"
						"</tt:StreamingCapabilities>\r\n"
						"<tt:Extension>\r\n"
							"<tt:ProfileCapabilities>\r\n"
								"<tt:MaximumNumberOfProfiles>10</tt:MaximumNumberOfProfiles>\r\n"
							"</tt:ProfileCapabilities>\r\n"
						"</tt:Extension>\r\n"
					"</tt:Media>\r\n"
					"<tt:PTZ>\r\n"
						"<tt:XAddr>http://%s/onvif/ptz_service</tt:XAddr>\r\n"
					"</tt:PTZ>\r\n"
					"<tt:Extension>\r\n"
						"<tt:DeviceIO>\r\n"
							"<tt:XAddr>http://%s/onvif/deviceIO_service</tt:XAddr>\r\n"
							"<tt:VideoSources>%i</tt:VideoSources>\r\n"
							"<tt:VideoOutputs>%i</tt:VideoOutputs>\r\n"
							"<tt:AudioSources>%i</tt:AudioSources>\r\n"
							"<tt:AudioOutputs>%i</tt:AudioOutputs>\r\n"
							"<tt:RelayOutputs>%i</tt:RelayOutputs>\r\n"
						"</tt:DeviceIO>\r\n"
						"<tt:Recording>\r\n"
							"<tt:XAddr>http://%s/onvif/recording_service</tt:XAddr>\r\n"
							"<tt:ReceiverSource>false</tt:ReceiverSource>\r\n"
							"<tt:MediaProfileSource>true</tt:MediaProfileSource>\r\n"
							"<tt:DynamicRecordings>true</tt:DynamicRecordings>\r\n"
							"<tt:DynamicTracks>true</tt:DynamicTracks>\r\n"
							"<tt:MaxStringLength>256</tt:MaxStringLength>\r\n"
						"</tt:Recording>\r\n"
						"<tt:Search>\r\n"
							"<tt:XAddr>http://%s/onvif/search_service</tt:XAddr>\r\n"
							"<tt:MetadataSearch>true</tt:MetadataSearch>\r\n"
						"</tt:Search>\r\n"
						"<tt:Replay>\r\n"
							"<tt:XAddr>http://%s/onvif/replay_service</tt:XAddr>\r\n"
						"</tt:Replay>\r\n"
						/*"<tt:Receiver>\r\n"
							"<tt:XAddr>http://%s/onvif/receiver_service</tt:XAddr>\r\n"
							"<tt:RTP_Multicast>true</tt:RTP_Multicast>\r\n"
							"<tt:RTP_TCP>true</tt:RTP_TCP>\r\n"
							"<tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP>\r\n"
							"<tt:SupportedReceivers>10</tt:SupportedReceivers>\r\n"
							"<tt:MaximumRTSPURILength>256</tt:MaximumRTSPURILength>\r\n"
						"</tt:Receiver>\r\n"*/
					"</tt:Extension>\r\n"
				"</tds:Capabilities>\r\n"
			"</tds:GetCapabilitiesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", /*pHEAD->Host, pHEAD->Host,*/ pHEAD->Host, pHEAD->Host, pHEAD->Host,
								 pHEAD->Host, pHEAD->Host, 
								 iVideoSourcesCnt, iVideoOutputsCnt, iAudioSourcesCnt, iAudioOutputsCnt, iRelayOutputsCnt,
								 pHEAD->Host, pHEAD->Host, pHEAD->Host);
}

static void ONVIV_Template_Dev_GetServices(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetServicesResponse>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/device/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/device_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>20</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/events/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/event_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver20/media/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/media2_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/media/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/media_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver20/ptz/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/ptz_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>17</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver20/imaging/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/image_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver20/analytics/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/analytics_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/recording/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/recording_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>18</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/search/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/search_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>2</tt:Major><tt:Minor>4</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/replay/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/replay_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>18</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
	//			"<tds:Service>\r\n"
	//				"<tds:Namespace>http://www.onvif.org/ver10/accesscontrol/wsdl</tds:Namespace>\r\n"
	//				"<tds:XAddr>http://%s/onvif/accesscontrol_service</tds:XAddr>\r\n"
	//				"<tds:Version><tt:Major>19</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
	//			"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/doorcontrol/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/doorcontrol_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/deviceIO/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/deviceIO_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/thermal/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/thermal_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>17</tt:Major><tt:Minor>6</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/credential/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/credential_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>19</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/accessrules/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/accessrules_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>18</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/schedule/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/schedule_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>18</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/receiver/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/receiver_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>2</tt:Major><tt:Minor>1</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
				"<tds:Service>\r\n"
					"<tds:Namespace>http://www.onvif.org/ver10/provisioning/wsdl</tds:Namespace>\r\n"
					"<tds:XAddr>http://%s/onvif/provisioning_service</tds:XAddr>\r\n"
					"<tds:Version><tt:Major>18</tt:Major><tt:Minor>12</tt:Minor></tds:Version>\r\n"
				"</tds:Service>\r\n"
			"</tds:GetServicesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", pHEAD->Host, pHEAD->Host, pHEAD->Host, pHEAD->Host, pHEAD->Host,
								 pHEAD->Host, pHEAD->Host, pHEAD->Host, pHEAD->Host, pHEAD->Host,
								 pHEAD->Host, pHEAD->Host, pHEAD->Host, pHEAD->Host, pHEAD->Host,
								 pHEAD->Host, pHEAD->Host, pHEAD->Host
								 //, pHEAD->Host
								 );
}

static void ONVIV_Template_Evnt_EventService(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	struct tm timeinfo;
	time_t rawtime;
	time(&rawtime);	
	localtime_r(&rawtime, &timeinfo);
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
				"<wsa:MessageID>urn:uuid:95a31c47-3117-467f-aada-164334b33eb9</wsa:MessageID>\r\n"
				"<wsa:To>http://www.w3.org/2005/08/addressing/anonymous</wsa:To>\r\n"
				"<wsa:Action>http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse</wsa:Action>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tev:CreatePullPointSubscriptionResponse>\r\n"
				"<tev:SubscriptionReference>\r\n"
					"<wsa:Address>http://%s/event_service/0</wsa:Address>\r\n"
				"</tev:SubscriptionReference>\r\n"
				"<wsnt:CurrentTime>%i-%i-%iT%i:%i:%iZ</wsnt:CurrentTime>\r\n"
				"<wsnt:TerminationTime>%i-%i-%iT%i:%i:%iZ</wsnt:TerminationTime>\r\n"
			"</tev:CreatePullPointSubscriptionResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", pHEAD->Host,
								timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
								timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

static void ONVIV_Template_Med_GetVideoSources(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char subbuff[1024];
	strcpy(buff, "<s:Header>\r\n"				
					"</s:Header>\r\n"
					"<s:Body>\r\n"
					"<trt:GetVideoSourcesResponse>\r\n");
			
	DBG_MUTEX_LOCK(&modulelist_mutex);
	memset(subbuff, 0, 1024);
	snprintf(subbuff, 1024,			
				"<trt:VideoSources token=\"V_SRC_1\">\r\n"
					"<tt:Framerate>%i</tt:Framerate>\r\n"
					"<tt:Resolution>\r\n"
						"<tt:Width>%i</tt:Width>\r\n"
						"<tt:Height>%i</tt:Height>\r\n"
					"</tt:Resolution>\r\n"
					"<tt:Imaging>\r\n"
						"<tt:Brightness>%f</tt:Brightness>\r\n"
						"<tt:ColorSaturation>%f</tt:ColorSaturation>\r\n"
						"<tt:Contrast>%f</tt:Contrast>\r\n"
						"<tt:Sharpness>%f</tt:Sharpness>\r\n"
						"<tt:Exposure><tt:Mode>%s</tt:Mode></tt:Exposure>\r\n"
						"<tt:Focus><tt:AutoFocusMode>%s</tt:AutoFocusMode></tt:Focus>\r\n"
						"<tt:WhiteBalance><tt:Mode>%s</tt:Mode></tt:WhiteBalance>\r\n"
					"</tt:Imaging>\r\n"
				"</trt:VideoSources>\r\n",
			ispCameraImageSettings.MainVideo.video_frame_rate,
			ispCameraImageSettings.MainVideo.video_width,
			ispCameraImageSettings.MainVideo.video_height,
			ispCameraImageSettings.Brightness, ispCameraImageSettings.ColorSaturation, ispCameraImageSettings.Contrast, ispCameraImageSettings.Sharpness,
			(ispCameraImageSettings.Exposure.Mode == OMX_ExposureControlAuto) ? "AUTO" : "MANUAL",
			ispCameraImageSettings.Focus.AutoFocusMode ? "AUTO" : "MANUAL",
			(ispCameraImageSettings.WhiteBalance.Mode == OMX_WhiteBalControlAuto) ? "AUTO" : "MANUAL");
	strcat(buff, subbuff);
	if (ispCameraImageSettings.PreviewEnabled)
	{
		memset(subbuff, 0, 1024);
		snprintf(subbuff, 1024,			
					"<trt:VideoSources token=\"V_SRC_2\">\r\n"
						"<tt:Framerate>%i</tt:Framerate>\r\n"
						"<tt:Resolution>\r\n"
							"<tt:Width>%i</tt:Width>\r\n"
							"<tt:Height>%i</tt:Height>\r\n"
						"</tt:Resolution>\r\n"
						"<tt:Imaging>\r\n"
							"<tt:Brightness>%f</tt:Brightness>\r\n"
							"<tt:ColorSaturation>%f</tt:ColorSaturation>\r\n"
							"<tt:Contrast>%f</tt:Contrast>\r\n"
							"<tt:Sharpness>%f</tt:Sharpness>\r\n"
							"<tt:Exposure><tt:Mode>%s</tt:Mode></tt:Exposure>\r\n"
							"<tt:Focus><tt:AutoFocusMode>%s</tt:AutoFocusMode></tt:Focus>\r\n"
							"<tt:WhiteBalance><tt:Mode>%s</tt:Mode></tt:WhiteBalance>\r\n"
						"</tt:Imaging>\r\n"
					"</trt:VideoSources>\r\n",
				ispCameraImageSettings.PrevVideo.video_frame_rate,
				ispCameraImageSettings.PrevVideo.video_width,
				ispCameraImageSettings.PrevVideo.video_height,
				ispCameraImageSettings.Brightness, ispCameraImageSettings.ColorSaturation, ispCameraImageSettings.Contrast, ispCameraImageSettings.Sharpness,
				(ispCameraImageSettings.Exposure.Mode == OMX_ExposureControlAuto) ? "AUTO" : "MANUAL",
				ispCameraImageSettings.Focus.AutoFocusMode ? "AUTO" : "MANUAL",
				(ispCameraImageSettings.WhiteBalance.Mode == OMX_WhiteBalControlAuto) ? "AUTO" : "MANUAL");
		strcat(buff, subbuff);
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	strcat(buff, "</trt:GetVideoSourcesResponse>\r\n"
				"</s:Body>\r\n"
				"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_GetVideoEncoders(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	char subbuff[4096];
	strcpy(buff, "<s:Header>\r\n"				
					"</s:Header>\r\n"
					"<s:Body>\r\n"
					"<trt:GetVideoEncoderConfigurations>\r\n");	
	memset(subbuff, 0, 4096);				
	snprintf(subbuff, 4096, 
					"<trt:Configurations token=\"V_ENC_1\">\r\n"
						"<trt:Name>V_ENC_1</trt:Name>\r\n"
						"<trt:UseCount>1</trt:UseCount>\r\n"
						"<trt:Encoding>H264</trt:Encoding>\r\n"
						"<trt:Resolution>\r\n"
							"<trt:Width>%i</trt:Width>\r\n"
							"<trt:Height>%i</trt:Height>\r\n"
						"</trt:Resolution>\r\n"
						"<trt:Quality>%i</trt:Quality>\r\n"
						"<trt:RateControl>\r\n"
							"<trt:FrameRateLimit>%i</trt:FrameRateLimit>\r\n"
							"<trt:EncodingInterval>%i</trt:EncodingInterval>\r\n"
							"<trt:BitrateLimit>%i</trt:BitrateLimit>\r\n"
						"</trt:RateControl>\r\n"
						"<trt:H264>\r\n"
							"<trt:H264Profile>%s</trt:H264Profile>\r\n"
						"</trt:H264>\r\n"
					"</trt:Configurations>\r\n",
					ispCameraImageSettings.MainVideo.video_width,
					ispCameraImageSettings.MainVideo.video_height,
					ispCameraImageSettings.MainVideoAvcLevel,
					ispCameraImageSettings.MainVideo.video_frame_rate,
					ispCameraImageSettings.MainVideo.video_intra_frame,
					ispCameraImageSettings.MainVideo.video_bit_rate/1000,
					GetH264ProfileName(ispCameraImageSettings.MainVideoAvcProfile));
	strcat(buff, subbuff);
				
	if (ispCameraImageSettings.PreviewEnabled)
	{	
		memset(subbuff, 0, 4096);				
		snprintf(subbuff, 4096, 
				"<trt:Configurations token=\"V_ENC_2\">\r\n"
						"<trt:Name>V_ENC_2</trt:Name>\r\n"
						"<trt:UseCount>2</trt:UseCount>\r\n"
						"<trt:Encoding>H264</trt:Encoding>\r\n"
						"<trt:Resolution>\r\n"
							"<trt:Width>%i</trt:Width>\r\n"
							"<trt:Height>%i</trt:Height>\r\n"
						"</trt:Resolution>\r\n"
						"<trt:Quality>%i</trt:Quality>\r\n"
						"<trt:RateControl>\r\n"
							"<trt:FrameRateLimit>%i</trt:FrameRateLimit>\r\n"
							"<trt:EncodingInterval>%i</trt:EncodingInterval>\r\n"
							"<trt:BitrateLimit>%i</trt:BitrateLimit>\r\n"
						"</trt:RateControl>\r\n"
						"<trt:H264>\r\n"
							"<trt:H264Profile>%s</trt:H264Profile>\r\n"
						"</trt:H264>\r\n"
					"</trt:Configurations>\r\n",					
					ispCameraImageSettings.PrevVideo.video_width,
					ispCameraImageSettings.PrevVideo.video_height,
					ispCameraImageSettings.PrevVideoAvcLevel,
					ispCameraImageSettings.PrevVideo.video_frame_rate,
					ispCameraImageSettings.PrevVideo.video_intra_frame,
					ispCameraImageSettings.PrevVideo.video_bit_rate/1000,
					GetH264ProfileName(ispCameraImageSettings.PrevVideoAvcProfile));
		strcat(buff, subbuff);
	}		
	strcat(buff, "</trt:GetVideoEncoderConfigurations>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");			
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Med_GetAudioEncoders(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	char subbuff[4096];
	char subbuff_aud[2048];
	int n;
	
	strcpy(buff, "<s:Header>\r\n"				
					"</s:Header>\r\n"
					"<s:Body>\r\n"
					"<trt:GetAudioEncoderConfigurations>\r\n");	
						
	for (n = 0; n < session->MicCount; n++)
	{
		memset(subbuff_aud, 0, 2048);
		snprintf(subbuff_aud, 2048, 			
					"<tt:Configurations token=\"A_ENC_%.4s\">\r\n"
						"<tt:Name>A_ENC_%.4s</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:Encoding>%s</tt:Encoding>\r\n"
						"<tt:Bitrate>%i</tt:Bitrate>\r\n"
						"<tt:SampleRate>%i</tt:SampleRate>\r\n"
					"</tt:Configurations>\r\n",
					(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID,
					WEB_get_AudioCodecName(session->MicList[n].CodecNum),
					(session->MicList[n].BitRate > 10) ? session->MicList[n].BitRate / 1000 : session->MicList[n].BitRate,
					session->MicList[n].Freq);
		strcat(buff, subbuff);
		if (strlen(buff) > (buffsize + 2048)) break;			
	}
		
	strcat(buff, "</trt:GetAudioEncoderConfigurations>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");			
}

static void ONVIV_Template_Med_GetProfiles(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	char subbuff[4096];
	char subbuff_aud[2048];
	int n = 0;
	memset(subbuff_aud, 0, 2048);		
	for (n = 0; n < session->MicCount; n++)
	{
		snprintf(subbuff_aud, 2048, 			
						"<tt:AudioSourceConfiguration token=\"A_SRC_CFG_%.4s\">\r\n"
						"<tt:Name>A_SRC_CFG_%.4s</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:SourceToken>A_SRC_%.4s</tt:SourceToken>\r\n"
						"</tt:AudioSourceConfiguration>\r\n"
						"<tt:AudioEncoderConfiguration token=\"A_ENC_%.4s\">\r\n"
						"<tt:Name>A_ENC_%.4s</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:Encoding>%s</tt:Encoding>\r\n"
						"<tt:Bitrate>%i</tt:Bitrate>\r\n"
						"<tt:SampleRate>%i</tt:SampleRate>\r\n"
						"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
					"</tt:AudioEncoderConfiguration>\r\n",
					(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID, 
					(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID,
					WEB_get_AudioCodecName(session->MicList[n].CodecNum),
					(session->MicList[n].BitRate > 10) ? session->MicList[n].BitRate / 1000 : session->MicList[n].BitRate,
					session->MicList[n].Freq);				
	}
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	strcpy(buff, "<s:Header>\r\n"				
					"</s:Header>\r\n"
					"<s:Body>\r\n"
					"<trt:GetProfilesResponse>\r\n");	
	memset(subbuff, 0, 4096);				
	snprintf(subbuff, 4096, 
				"<trt:Profiles token=\"PROFILE_MAIN\" fixed=\"true\">\r\n"
					"<tt:Name>PROFILE_MAIN</tt:Name>\r\n"
					"<tt:VideoSourceConfiguration token=\"V_SRC_CFG_1\">\r\n"
						"<tt:Name>V_SRC_CFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:SourceToken>V_SRC_1</tt:SourceToken>\r\n"
						"<tt:Bounds x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" />\r\n"
					"</tt:VideoSourceConfiguration>\r\n"
					"<tt:VideoEncoderConfiguration token=\"V_ENC_1\">\r\n"
						"<tt:Name>V_ENC_1</tt:Name>\r\n"
						"<tt:UseCount>1</tt:UseCount>\r\n"
						"<tt:Encoding>H264</tt:Encoding>\r\n"
						"<tt:Resolution>\r\n"
							"<tt:Width>%i</tt:Width>\r\n"
							"<tt:Height>%i</tt:Height>\r\n"
						"</tt:Resolution>\r\n"
						"<tt:Quality>%i</tt:Quality>\r\n"
						"<tt:RateControl>\r\n"
							"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
							"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
							"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
						"</tt:RateControl>\r\n"
						"<tt:H264>\r\n"
							"<tt:H264Profile>%s</tt:H264Profile>\r\n"
						"</tt:H264>\r\n"
						"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
					"</tt:VideoEncoderConfiguration>\r\n",
					ispCameraImageSettings.FrameLeftCrop,
					ispCameraImageSettings.FrameUpCrop,
					100 - ispCameraImageSettings.FrameRightCrop - ispCameraImageSettings.FrameLeftCrop,
					100 - ispCameraImageSettings.FrameDownCrop -  ispCameraImageSettings.FrameUpCrop,
					ispCameraImageSettings.MainVideo.video_width,
					ispCameraImageSettings.MainVideo.video_height,
					ispCameraImageSettings.MainVideoAvcLevel,
					ispCameraImageSettings.MainVideo.video_frame_rate,
					ispCameraImageSettings.MainVideo.video_intra_frame,
					ispCameraImageSettings.MainVideo.video_bit_rate/1000,
					GetH264ProfileName(ispCameraImageSettings.MainVideoAvcProfile));
	strcat(buff, subbuff);
	strcat(buff, subbuff_aud);	
	strcat(buff, 		"<tt:PTZConfiguration token=\"PTZCFG_1\" MoveRamp=\"0\" PresetRamp=\"0\" PresetTourRamp=\"0\">\r\n"
						"<tt:Name>PTZCFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:NodeToken>PTZNODE_1</tt:NodeToken>\r\n"
						"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\r\n"
						"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\r\n"
						"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\r\n"
						"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\r\n"
						"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\r\n"
						"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\r\n"
						"<tt:DefaultPTZSpeed>\r\n"
							"<tt:PanTilt x=\"0.5\" y=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" />\r\n"
							"<tt:Zoom x=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" />\r\n"
						"</tt:DefaultPTZSpeed>\r\n"
						"<tt:DefaultPTZTimeout>PT5S</tt:DefaultPTZTimeout>\r\n"
						"<tt:PanTiltLimits>\r\n"
							"<tt:Range>\r\n"
								"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\r\n"
								"<tt:XRange>\r\n"
									"<tt:Min>-1.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:XRange>\r\n"
								"<tt:YRange>\r\n"
									"<tt:Min>-1.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:YRange>\r\n"
							"</tt:Range>\r\n"
						"</tt:PanTiltLimits>\r\n"
						"<tt:ZoomLimits>\r\n"
							"<tt:Range>\r\n"
								"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\r\n"
								"<tt:XRange>\r\n"
									"<tt:Min>0.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:XRange>\r\n"
							"</tt:Range>\r\n"
						"</tt:ZoomLimits>\r\n"
						"<tt:Extension>\r\n"
							"<tt:PTControlDirection>\r\n"
								"<tt:EFlip><tt:Mode>OFF</tt:Mode></tt:EFlip>\r\n"
								"<tt:Reverse><tt:Mode>OFF</tt:Mode></tt:Reverse>\r\n"
							"</tt:PTControlDirection>\r\n"
						"</tt:Extension>\r\n"
					"</tt:PTZConfiguration>\r\n"
				"</trt:Profiles>\r\n");
				
	if (ispCameraImageSettings.PreviewEnabled)
	{	
		memset(subbuff, 0, 4096);				
		snprintf(subbuff, 4096, 
				"<trt:Profiles token=\"PROFILE_PREV\" fixed=\"true\">\r\n"
					"<tt:Name>PROFILE_PREV</tt:Name>\r\n"
					"<tt:VideoSourceConfiguration token=\"V_SRC_CFG_1\">\r\n"
						"<tt:Name>V_SRC_CFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:SourceToken>V_SRC_2</tt:SourceToken>\r\n"
						"<tt:Bounds x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" />\r\n"
					"</tt:VideoSourceConfiguration>\r\n"
					"<tt:VideoEncoderConfiguration token=\"V_ENC_2\">\r\n"
						"<tt:Name>V_ENC_2</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:Encoding>H264</tt:Encoding>\r\n"
						"<tt:Resolution>\r\n"
							"<tt:Width>%i</tt:Width>\r\n"
							"<tt:Height>%i</tt:Height>\r\n"
						"</tt:Resolution>\r\n"
						"<tt:Quality>%i</tt:Quality>\r\n"
						"<tt:RateControl>\r\n"
							"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
							"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
							"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
						"</tt:RateControl>\r\n"
						"<tt:H264>\r\n"
							"<tt:H264Profile>%s</tt:H264Profile>\r\n"
						"</tt:H264>\r\n"
						"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
					"</tt:VideoEncoderConfiguration>\r\n",					
					ispCameraImageSettings.FrameLeftCrop,
					ispCameraImageSettings.FrameUpCrop,
					100 - ispCameraImageSettings.FrameRightCrop - ispCameraImageSettings.FrameLeftCrop,
					100 - ispCameraImageSettings.FrameDownCrop -  ispCameraImageSettings.FrameUpCrop,
					ispCameraImageSettings.PrevVideo.video_width,
					ispCameraImageSettings.PrevVideo.video_height,
					ispCameraImageSettings.PrevVideoAvcLevel,
					ispCameraImageSettings.PrevVideo.video_frame_rate,
					ispCameraImageSettings.PrevVideo.video_intra_frame,
					ispCameraImageSettings.PrevVideo.video_bit_rate/1000,
					GetH264ProfileName(ispCameraImageSettings.PrevVideoAvcProfile));
		strcat(buff, subbuff);	
		strcat(buff, subbuff_aud);	
		strcat(buff,	"<tt:PTZConfiguration token=\"PTZCFG_1\" MoveRamp=\"0\" PresetRamp=\"0\" PresetTourRamp=\"0\">\r\n"
						"<tt:Name>PTZCFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:NodeToken>PTZNODE_1</tt:NodeToken>\r\n"
						"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\r\n"
						"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\r\n"
						"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\r\n"
						"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\r\n"
						"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\r\n"
						"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\r\n"
						"<tt:DefaultPTZSpeed>\r\n"
							"<tt:PanTilt x=\"0.5\" y=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" />\r\n"
							"<tt:Zoom x=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" />\r\n"
						"</tt:DefaultPTZSpeed>\r\n"
						"<tt:DefaultPTZTimeout>PT5S</tt:DefaultPTZTimeout>\r\n"
						"<tt:PanTiltLimits>\r\n"
							"<tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\r\n"
								"<tt:XRange>\r\n"
									"<tt:Min>-1.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:XRange>\r\n"
								"<tt:YRange>\r\n"
									"<tt:Min>-1.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:YRange>\r\n"
							"</tt:Range>\r\n"
						"</tt:PanTiltLimits>\r\n"
						"<tt:ZoomLimits>\r\n"
							"<tt:Range>\r\n"
								"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\r\n"
								"<tt:XRange>\r\n"
									"<tt:Min>0.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:XRange>\r\n"
							"</tt:Range>\r\n"
						"</tt:ZoomLimits>\r\n"
						"<tt:Extension>\r\n"
							"<tt:PTControlDirection>\r\n"
								"<tt:EFlip><tt:Mode>OFF</tt:Mode></tt:EFlip>\r\n"
								"<tt:Reverse><tt:Mode>OFF</tt:Mode></tt:Reverse>\r\n"
							"</tt:PTControlDirection>\r\n"
						"</tt:Extension>\r\n"
					"</tt:PTZConfiguration>\r\n"
				"</trt:Profiles>\r\n");
	}		
	strcat(buff, "</trt:GetProfilesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");			
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Med_GetSnapshotUri(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetSnapshotUriResponse>\r\n"
				"<trt:MediaUri>\r\n"
					"<tt:Uri>http://%s/snapshot/Camera</tt:Uri>\r\n"
					"<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>\r\n"
					"<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>\r\n"
					"<tt:Timeout>PT60S</tt:Timeout>\r\n"
				"</trt:MediaUri>\r\n"
			"</trt:GetSnapshotUriResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", pHEAD->Host);
}

static void ONVIV_Template_Med_GetProfile(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	char cProfileName[64];
	cProfileName[63] = 0;
	ONVIF_get_xml_value("ProfileToken", cProfileName, 63, pHEAD->Body, pHEAD->BodyLength);
	
	
	char subbuff[4096];	
	char subbuff_aud[2048];
	int n;
	memset(subbuff_aud, 0, 2048);		
	for (n = 0; n < session->MicCount; n++)
	{
		snprintf(subbuff_aud, 2048, 			
						"<tt:AudioSourceConfiguration token=\"A_SRC_CFG_%.4s\">\r\n"
						"<tt:Name>A_SRC_CFG_%.4s</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:SourceToken>A_SRC_%.4s</tt:SourceToken>\r\n"
						"</tt:AudioSourceConfiguration>\r\n"
						"<tt:AudioEncoderConfiguration token=\"A_ENC_%.4s\">\r\n"
						"<tt:Name>A_ENC_%.4s</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:Encoding>%s</tt:Encoding>\r\n"
						"<tt:Bitrate>%i</tt:Bitrate>\r\n"
						"<tt:SampleRate>%i</tt:SampleRate>\r\n"
						"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
					"</tt:AudioEncoderConfiguration>\r\n",
					(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID,
					(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID,
					WEB_get_AudioCodecName(session->MicList[n].CodecNum),
					(session->MicList[n].BitRate > 10) ? session->MicList[n].BitRate / 1000 : session->MicList[n].BitRate,
					session->MicList[n].Freq);
		break;
	}
	
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	strcpy(buff, "<s:Header>\r\n"				
					"</s:Header>\r\n"
					"<s:Body>\r\n"
					"<trt:GetProfileResponse>\r\n");	
	
	memset(subbuff, 0, 4096);
					
	if (strcasecmp(cProfileName, "PROFILE_MAIN") == 0)
	{
		snprintf(subbuff, 4096, 
				"<trt:Profile token=\"PROFILE_MAIN\" fixed=\"true\">\r\n"
					"<tt:Name>PROFILE_MAIN</tt:Name>\r\n"
					"<tt:VideoSourceConfiguration token=\"V_SRC_CFG_1\">\r\n"
						"<tt:Name>V_SRC_CFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:SourceToken>V_SRC_1</tt:SourceToken>\r\n"
						"<tt:Bounds x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" />\r\n"
					"</tt:VideoSourceConfiguration>\r\n"
					"<tt:VideoEncoderConfiguration token=\"V_ENC_1\">\r\n"
						"<tt:Name>V_ENC_1</tt:Name>\r\n"
						"<tt:UseCount>1</tt:UseCount>\r\n"
						"<tt:Encoding>H264</tt:Encoding>\r\n"
						"<tt:Resolution>\r\n"
							"<tt:Width>%i</tt:Width>\r\n"
							"<tt:Height>%i</tt:Height>\r\n"
						"</tt:Resolution>\r\n"
						"<tt:Quality>%i</tt:Quality>\r\n"
						"<tt:RateControl>\r\n"
							"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
							"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
							"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
						"</tt:RateControl>\r\n"
						"<tt:H264>\r\n"
							"<tt:H264Profile>%s</tt:H264Profile>\r\n"
						"</tt:H264>\r\n"
						"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
					"</tt:VideoEncoderConfiguration>\r\n",
					ispCameraImageSettings.FrameLeftCrop,
					ispCameraImageSettings.FrameUpCrop,
					100 - ispCameraImageSettings.FrameRightCrop - ispCameraImageSettings.FrameLeftCrop,
					100 - ispCameraImageSettings.FrameDownCrop -  ispCameraImageSettings.FrameUpCrop,
					ispCameraImageSettings.MainVideo.video_width,
					ispCameraImageSettings.MainVideo.video_height,
					ispCameraImageSettings.MainVideoAvcLevel,
					ispCameraImageSettings.MainVideo.video_frame_rate,
					ispCameraImageSettings.MainVideo.video_intra_frame,
					ispCameraImageSettings.MainVideo.video_bit_rate/1000,
					GetH264ProfileName(ispCameraImageSettings.MainVideoAvcProfile));	
	}
	else
	{	
		snprintf(subbuff, 4096, 
				"<trt:Profile token=\"PROFILE_PREV\" fixed=\"true\">\r\n"
					"<tt:Name>PROFILE_PREV</tt:Name>\r\n"
					"<tt:VideoSourceConfiguration token=\"V_SRC_CFG_1\">\r\n"
						"<tt:Name>V_SRC_CFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:SourceToken>V_SRC_2</tt:SourceToken>\r\n"
						"<tt:Bounds x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" />\r\n"
					"</tt:VideoSourceConfiguration>\r\n"
					"<tt:VideoEncoderConfiguration token=\"V_ENC_2\">\r\n"
						"<tt:Name>V_ENC_2</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:Encoding>H264</tt:Encoding>\r\n"
						"<tt:Resolution>\r\n"
							"<tt:Width>%i</tt:Width>\r\n"
							"<tt:Height>%i</tt:Height>\r\n"
						"</tt:Resolution>\r\n"
						"<tt:Quality>%i</tt:Quality>\r\n"
						"<tt:RateControl>\r\n"
							"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
							"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
							"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
						"</tt:RateControl>\r\n"
						"<tt:H264>\r\n"
							"<tt:H264Profile>%s</tt:H264Profile>\r\n"
						"</tt:H264>\r\n"
						"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
					"</tt:VideoEncoderConfiguration>\r\n",					
					ispCameraImageSettings.FrameLeftCrop,
					ispCameraImageSettings.FrameUpCrop,
					100 - ispCameraImageSettings.FrameRightCrop - ispCameraImageSettings.FrameLeftCrop,
					100 - ispCameraImageSettings.FrameDownCrop -  ispCameraImageSettings.FrameUpCrop,
					ispCameraImageSettings.PrevVideo.video_width,
					ispCameraImageSettings.PrevVideo.video_height,
					ispCameraImageSettings.PrevVideoAvcLevel,
					ispCameraImageSettings.PrevVideo.video_frame_rate,
					ispCameraImageSettings.PrevVideo.video_intra_frame,
					ispCameraImageSettings.PrevVideo.video_bit_rate/1000,
					GetH264ProfileName(ispCameraImageSettings.PrevVideoAvcProfile));
	}
	strcat(buff, subbuff);	
	strcat(buff, subbuff_aud);	
	strcat(buff,	"<tt:PTZConfiguration token=\"PTZCFG_1\" MoveRamp=\"0\" PresetRamp=\"0\" PresetTourRamp=\"0\">\r\n"
						"<tt:Name>PTZCFG_1</tt:Name>\r\n"
						"<tt:UseCount>2</tt:UseCount>\r\n"
						"<tt:NodeToken>PTZNODE_1</tt:NodeToken>\r\n"
						"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\r\n"
						"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\r\n"
						"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\r\n"
						"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\r\n"
						"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\r\n"
						"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\r\n"
						"<tt:DefaultPTZSpeed>\r\n"
							"<tt:PanTilt x=\"0.5\" y=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" />\r\n"
							"<tt:Zoom x=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" />\r\n"
						"</tt:DefaultPTZSpeed>\r\n"
						"<tt:DefaultPTZTimeout>PT5S</tt:DefaultPTZTimeout>\r\n"
						"<tt:PanTiltLimits>\r\n"
							"<tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\r\n"
								"<tt:XRange>\r\n"
									"<tt:Min>-1.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:XRange>\r\n"
								"<tt:YRange>\r\n"
									"<tt:Min>-1.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:YRange>\r\n"
							"</tt:Range>\r\n"
						"</tt:PanTiltLimits>\r\n"
						"<tt:ZoomLimits>\r\n"
							"<tt:Range>\r\n"
								"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\r\n"
								"<tt:XRange>\r\n"
									"<tt:Min>0.0</tt:Min>\r\n"
									"<tt:Max>1.0</tt:Max>\r\n"
								"</tt:XRange>\r\n"
							"</tt:Range>\r\n"
						"</tt:ZoomLimits>\r\n"
						"<tt:Extension>\r\n"
							"<tt:PTControlDirection>\r\n"
								"<tt:EFlip><tt:Mode>OFF</tt:Mode></tt:EFlip>\r\n"
								"<tt:Reverse><tt:Mode>OFF</tt:Mode></tt:Reverse>\r\n"
							"</tt:PTControlDirection>\r\n"
						"</tt:Extension>\r\n"
					"</tt:PTZConfiguration>\r\n"
				"</trt:Profile>\r\n");
			
	strcat(buff, "</trt:GetProfileResponse>\r\n"
					"</s:Body>\r\n"
					"</s:Envelope>\r\n");	
		
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Med_GetStreamUri(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	char cProfileName[64];
	char *cVideoPath;
	char *cAudioPath;
	cProfileName[63] = 0;
	ONVIF_get_xml_value("ProfileToken", cProfileName, 63, pHEAD->Body, pHEAD->BodyLength);
	if (strcasecmp(cProfileName, "PROFILE_MAIN") == 0) cVideoPath = "video0"; else cVideoPath = "video1";
	if (session->MicCount) cAudioPath = "_audio0"; else cAudioPath = "";
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetStreamUriResponse>\r\n"
				"<trt:MediaUri>\r\n"
					"<tt:Uri>rtsp://%s:%i/%s%s</tt:Uri>\r\n"
					"<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>\r\n"
					"<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>\r\n"
					"<tt:Timeout>PT60S</tt:Timeout>\r\n"
				"</trt:MediaUri>\r\n"
			"</trt:GetStreamUriResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", session->server_ip_str, session->rtsp_port, cVideoPath, cAudioPath);
}

static void ONVIV_Template_Med_GetVideoSourceConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetVideoSourceConfigurationResponse>\r\n"
				"<trt:Configuration token=\"V_SRC_CFG_1\">\r\n"
					"<tt:Name>V_SRC_CFG_1</tt:Name>\r\n"
					"<tt:UseCount>2</tt:UseCount>\r\n"
					"<tt:SourceToken>V_SRC_1</tt:SourceToken>\r\n"
					"<tt:Bounds x=\"0\" y=\"0\" width=\"1280\" height=\"720\" />\r\n"
				"</trt:Configuration>\r\n"
			"</trt:GetVideoSourceConfigurationResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_GetVideoEncoderConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetVideoEncoderConfigurationOptionsResponse>\r\n"
				"<trt:Options>\r\n"
					"<tt:QualityRange>\r\n"
						"<tt:Min>0</tt:Min>\r\n"
						"<tt:Max>16</tt:Max>\r\n"
					"</tt:QualityRange>\r\n"
					"<tt:H264>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>2592</tt:Width>\r\n"
							"<tt:Height>1944</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>2048</tt:Width>\r\n"
							"<tt:Height>1536</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>1600</tt:Width>\r\n"
							"<tt:Height>1200</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>1400</tt:Width>\r\n"
							"<tt:Height>1050</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>1280</tt:Width>\r\n"
							"<tt:Height>960</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>1024</tt:Width>\r\n"
							"<tt:Height>768</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>800</tt:Width>\r\n"
							"<tt:Height>600</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>640</tt:Width>\r\n"
							"<tt:Height>480</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>480</tt:Width>\r\n"
							"<tt:Height>240</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>320</tt:Width>\r\n"
							"<tt:Height>240</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:ResolutionsAvailable>\r\n"
							"<tt:Width>160</tt:Width>\r\n"
							"<tt:Height>120</tt:Height>\r\n"
						"</tt:ResolutionsAvailable>\r\n"
						"<tt:GovLengthRange>\r\n"
							"<tt:Min>10</tt:Min>\r\n"
							"<tt:Max>60</tt:Max>\r\n"
						"</tt:GovLengthRange>\r\n"
						"<tt:FrameRateRange>\r\n"
							"<tt:Min>2</tt:Min>\r\n"
							"<tt:Max>30</tt:Max>\r\n"
						"</tt:FrameRateRange>\r\n"
						"<tt:EncodingIntervalRange>\r\n"
							"<tt:Min>5</tt:Min>\r\n"
							"<tt:Max>300</tt:Max>\r\n"
						"</tt:EncodingIntervalRange>\r\n"
						"<tt:H264ProfilesSupported>Baseline</tt:H264ProfilesSupported>\r\n"
						"<tt:H264ProfilesSupported>Main</tt:H264ProfilesSupported>\r\n"
						"<tt:H264ProfilesSupported>Extended</tt:H264ProfilesSupported>\r\n"
						"<tt:H264ProfilesSupported>High</tt:H264ProfilesSupported>\r\n"	
					"</tt:H264>\r\n"
				"</trt:Options>\r\n"
			"</trt:GetVideoEncoderConfigurationOptionsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Img_GetImagingSettings(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<timg:GetImagingSettingsResponse>\r\n"
				"<timg:ImagingSettings>\r\n"
					"<tt:Brightness>%f</tt:Brightness>\r\n"
					"<tt:ColorSaturation>%f</tt:ColorSaturation>\r\n"
					"<tt:Contrast>%f</tt:Contrast>\r\n"
					"<tt:Sharpness>%f</tt:Sharpness>\r\n"
					"<tt:Exposure><tt:Mode>%s</tt:Mode></tt:Exposure>\r\n"
					"<tt:Focus>\r\n"
						"<tt:AutoFocusMode>%s</tt:AutoFocusMode>\r\n"
						/*"<tt:DefaultSpeed>100.0</tt:DefaultSpeed>\r\n"
						"<tt:NearLimit>100.0</tt:NearLimit>\r\n"
						"<tt:FarLimit>1000.0</tt:FarLimit>\r\n"*/
					"</tt:Focus>\r\n"
					"<tt:WhiteBalance>\r\n"
						"<tt:Mode>%s</tt:Mode>\r\n"
						/*"<tt:CrGain>10.0</tt:CrGain>\r\n"
						"<tt:CbGain>10.0</tt:CbGain>\r\n"*/
					"</tt:WhiteBalance>\r\n"
				"</timg:ImagingSettings>\r\n"
			"</timg:GetImagingSettingsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			ispCameraImageSettings.Brightness, ispCameraImageSettings.ColorSaturation, ispCameraImageSettings.Contrast, ispCameraImageSettings.Sharpness,
			(ispCameraImageSettings.Exposure.Mode == OMX_ExposureControlAuto) ? "AUTO" : "MANUAL",
			ispCameraImageSettings.Focus.AutoFocusMode ? "AUTO" : "MANUAL",
			(ispCameraImageSettings.WhiteBalance.Mode == OMX_WhiteBalControlAuto) ? "AUTO" : "MANUAL");
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
}

static void ONVIV_Template_Img_GetStatus(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<timg:GetStatusResponse>\r\n"
				"<timg:Status>\r\n"					
					"<tt:FocusStatus20>\r\n"
						"<tt:Position >%i</tt:Position >\r\n"
						"<tt:MoveStatus >%s</tt:MoveStatus >\r\n"
					"</tt:FocusStatus20>\r\n"
				"</timg:Status>\r\n"
			"</timg:GetStatusResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			ispCameraImageSettings.Focus.Position,
			ispCameraImageSettings.Focus.Status ? "MOVING" : "IDLE");
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Img_GetOptions(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<timg:GetOptionsResponse>\r\n"
				"<timg:ImagingOptions>\r\n"					
					"<tt:Brightness>\r\n"
						"<tt:Min>0.0</tt:Min>\r\n"
						"<tt:Max>100.0</tt:Max>\r\n"
					"</tt:Brightness>\r\n"
					"<tt:ColorSaturation>\r\n"
						"<tt:Min>-100.0</tt:Min>\r\n"
						"<tt:Max>100.0</tt:Max>\r\n"
					"</tt:ColorSaturation>\r\n"
					"<tt:Contrast>\r\n"
						"<tt:Min>-100.0</tt:Min>\r\n"
						"<tt:Max>100.0</tt:Max>\r\n"
					"</tt:Contrast>\r\n"
					"<tt:Sharpness>\r\n"
						"<tt:Min>-100.0</tt:Min>\r\n"
						"<tt:Max>100.0</tt:Max>\r\n"
					"</tt:Sharpness>\r\n"
					"<tt:Exposure>\r\n"
						"<tt:Mode>AUTO</tt:Mode>\r\n"
						"<tt:Mode>MANUAL</tt:Mode>\r\n"
					"</tt:Exposure>\r\n"
					"<tt:Focus>\r\n"
						"<tt:AutoFocusModes>AUTO</tt:AutoFocusModes>\r\n"
						"<tt:AutoFocusModes>MANUAL</tt:AutoFocusModes>\r\n"
						"<tt:DefaultSpeed>\r\n"
							"<tt:Min>0.0</tt:Min>\r\n"
							"<tt:Max>100.0</tt:Max>\r\n"
						"</tt:DefaultSpeed>\r\n"
						"<tt:NearLimit>\r\n"
							"<tt:Min>0.0</tt:Min>\r\n"
							"<tt:Max>100.0</tt:Max>\r\n"
						"</tt:NearLimit>\r\n"
						"<tt:FarLimit>\r\n"
							"<tt:Min>0.0</tt:Min>\r\n"
							"<tt:Max>1000.0</tt:Max>\r\n"
						"</tt:FarLimit>\r\n"
					"</tt:Focus>\r\n"
					"<tt:IrCutFilterModes>ON</tt:IrCutFilterModes>\r\n"
					"<tt:IrCutFilterModes>OFF</tt:IrCutFilterModes>\r\n"
					"<tt:IrCutFilterModes>AUTO</tt:IrCutFilterModes>\r\n"					
					"<tt:WhiteBalance>\r\n"
						"<tt:Mode>AUTO</tt:Mode>\r\n"
						"<tt:Mode>MANUAL</tt:Mode>\r\n"
						"<tt:YrGain>\r\n"
							"<tt:Min>0.0</tt:Min>\r\n"
							"<tt:Max>100.0</tt:Max>\r\n"
						"</tt:YrGain>\r\n"
						"<tt:YbGain>\r\n"
							"<tt:Min>0.0</tt:Min>\r\n"
							"<tt:Max>100.0</tt:Max>\r\n"
						"</tt:YbGain>\r\n"
					"</tt:WhiteBalance>\r\n"
				"</timg:ImagingOptions>\r\n"
			"</timg:GetOptionsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Img_SetImagingSettings(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char videosourcetoken[32];
	char brightness[32];
	char colorsaturation[32];
	char contrast[32];
	char autofocusmode[32];
	char sharpness[32];
	char exposure[128];
	char whitebalance[128];
	char exposuremode[32];
	char whitebalancemode[32];
	
	videosourcetoken[31] = 0;	
	brightness[31] = 0;
	colorsaturation[31] = 0;	
	contrast[31] = 0;
	autofocusmode[31] = 0;	
	sharpness[31] = 0;
	exposure[127] = 0;	
	whitebalance[127] = 0;
	exposuremode[31] = 0;	
	whitebalancemode[31] = 0;
	
	int res = 0;
	res |= ONVIF_get_xml_value("VideoSourceToken", videosourcetoken, 31, pHEAD->Body, pHEAD->BodyLength) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Brightness", NULL, brightness, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "ColorSaturation", NULL, colorsaturation, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Contrast", NULL, contrast, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Sharpness", NULL, sharpness, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "AutoFocusMode", NULL, autofocusmode, 31) == 0;
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Exposure", NULL, exposure, 127))
		res |= !ONVIF_get_node_param_from_url(exposure, strlen(exposure), "Mode", NULL, exposuremode, 31); else res = 1;
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "WhiteBalance", NULL, whitebalance, 127))
		res |= !ONVIF_get_node_param_from_url(whitebalance, strlen(whitebalance), "Mode", NULL, whitebalancemode, 31); else res = 1;
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<trt:SetImagingSettings />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
			
	if (!res)
	{
		res = 0;
		DBG_MUTEX_LOCK(&modulelist_mutex);
		if (strcasecmp(videosourcetoken, "V_SRC_1") == 0)
		{
			ispCameraImageSettings.Updated = 1;
			ispCameraImageSettings.Brightness = Str2Float(brightness);
			ispCameraImageSettings.ColorSaturation = Str2Float(colorsaturation);
			ispCameraImageSettings.Contrast = Str2Float(contrast);
			ispCameraImageSettings.Sharpness = Str2Float(sharpness);
			if (strcasecmp(exposuremode, "AUTO") == 0) ispCameraImageSettings.Exposure.Mode = OMX_ExposureControlAuto;
			if (strcasecmp(exposuremode, "MANUAL") == 0) ispCameraImageSettings.Exposure.Mode = OMX_ExposureControlOff;
			if (strcasecmp(autofocusmode, "AUTO") == 0) ispCameraImageSettings.Focus.AutoFocusMode = 1;
			if (strcasecmp(autofocusmode, "MANUAL") == 0) ispCameraImageSettings.Focus.AutoFocusMode = 0;
			if (strcasecmp(whitebalancemode, "AUTO") == 0) ispCameraImageSettings.WhiteBalance.Mode = OMX_WhiteBalControlAuto;
			if (strcasecmp(whitebalancemode, "MANUAL") == 0) ispCameraImageSettings.WhiteBalance.Mode = OMX_WhiteBalControlOff;
			res = 1;
		}	
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		
		if (res) 
		{
			omx_set_image_sensor_camera_params(&ispCameraImageSettings);
			SaveModules();
		}
	}
}

static void ONVIV_Template_Img_Stop(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{	
	char focus[256];
	memset(focus, 0, 256);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<trt:Stop />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
			
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
	char uiPtzEn = 0;
	unsigned int uiPtzID = 0;
	if (miCamera)
	{
		uiPtzEn = miCamera->Settings[38] & 1;
		uiPtzID = miCamera->Settings[39];		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	if (uiPtzEn && uiPtzID)
		AddModuleEventInList(uiPtzID, PTZ_TARGET_STOP_FOCUS, 0, NULL, 0, 1);
}

static void ONVIV_Template_Img_Move(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{	
	char focus[256];
	memset(focus, 0, 256);
	char continuous[256];
	memset(continuous, 0, 256);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<trt:Move />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
			
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Focus", NULL, focus, 255))	
	{
		int len = strlen(focus);
		if (ONVIF_get_node_param_from_url(focus, len, "Continuous", NULL, continuous, 255))
		{
			len = strlen(continuous);
			char spd[32];
			memset(spd, 0, 32);
			if (ONVIF_get_node_param_from_url(continuous, len, "Speed", NULL, spd, 31))
			{
				float s = 0;
		
				s = Str2Float(spd);
					
				DBG_MUTEX_LOCK(&modulelist_mutex);
			
				MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
				char uiPtzEn = 0;
				unsigned int uiPtzID = 0;
				if (miCamera)
				{
					uiPtzEn = miCamera->Settings[38] & 1;
					uiPtzID = miCamera->Settings[39];		
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
					
				if (uiPtzEn && uiPtzID)
				{					
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_CONT_MS_MOVE_FOCUS, s * 100000, NULL, 0, 1);					
				}					
			}
		}
	}
}

static void ONVIV_Template_Med_GetAudioSources(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	char subbuff[256];
	int n;
	int iLen = 0;
	
	strcpy(buff, "<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetAudioSourcesResponse>\r\n");
			
	for (n = 0; n < session->MicCount; n++)
	{
		memset(subbuff, 0, 256);
		snprintf(subbuff, 256, 
							"<trt:AudioSources token=\"A_SRC_%.4s\">\r\n"
								"<tt:Channels>%i</tt:Channels>\r\n"
							"</trt:AudioSources>\r\n",
							(char*)&session->MicList[n].ID, session->MicList[n].Channels);
		strcat(buff, subbuff);
		iLen += strlen(subbuff);
		if ((iLen + 256) > ONVIF_TX_BUF_SIZE_MAX) break;
	}
	
	strcat(buff, "</trt:GetAudioSourcesResponse>\r\n"
					"</s:Body>\r\n"
					"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_CreateProfile(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, 16384, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_DeleteProfile(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_GetCompatibleVideoEncoder(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetCompatibleVideoEncoderConfigurationsResponse>\r\n"
				"<trt:Configurations token=\"V_ENC_1\">\r\n"
					"<tt:Name>V_ENC_1</tt:Name>\r\n"
					"<tt:UseCount>1</tt:UseCount>\r\n"
					"<tt:Encoding>H264</tt:Encoding>\r\n"
					"<tt:Resolution>\r\n"
						"<tt:Width>%i</tt:Width>\r\n"
						"<tt:Height>%i</tt:Height>\r\n"
					"</tt:Resolution>\r\n"
					"<tt:Quality>%i</tt:Quality>\r\n"
					"<tt:RateControl>\r\n"
						"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
						"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
						"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
					"</tt:RateControl>\r\n"
					"<tt:H264>\r\n"
						"<tt:H264Profile>%s</tt:H264Profile>\r\n"
					"</tt:H264>\r\n"
					"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
				"</trt:Configurations>\r\n"
				"<trt:Configurations token=\"V_ENC_2\">\r\n"
					"<tt:Name>V_ENC_2</tt:Name>\r\n"
					"<tt:UseCount>2</tt:UseCount>\r\n"
					"<tt:Encoding>H264</tt:Encoding>\r\n"
					"<tt:Resolution>\r\n"
						"<tt:Width>%i</tt:Width>\r\n"
						"<tt:Height>%i</tt:Height>\r\n"
					"</tt:Resolution>\r\n"
					"<tt:Quality>%i</tt:Quality>\r\n"
					"<tt:RateControl>\r\n"
						"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
						"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
						"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
					"</tt:RateControl>\r\n"
					"<tt:H264>\r\n"
						"<tt:H264Profile>%s</tt:H264Profile>\r\n"
					"</tt:H264>\r\n"
					"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
				"</trt:Configurations>\r\n"
			"</trt:GetCompatibleVideoEncoderConfigurationsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			ispCameraImageSettings.MainVideo.video_width,
			ispCameraImageSettings.MainVideo.video_height,
			ispCameraImageSettings.MainVideoAvcLevel,
			ispCameraImageSettings.MainVideo.video_frame_rate,
			ispCameraImageSettings.MainVideo.video_intra_frame,
			ispCameraImageSettings.MainVideo.video_bit_rate/1000,
			GetH264ProfileName(ispCameraImageSettings.MainVideoAvcProfile),
			ispCameraImageSettings.PrevVideo.video_width,
			ispCameraImageSettings.PrevVideo.video_height,
			ispCameraImageSettings.PrevVideoAvcLevel,
			ispCameraImageSettings.PrevVideo.video_frame_rate,
			ispCameraImageSettings.PrevVideo.video_intra_frame,
			ispCameraImageSettings.PrevVideo.video_bit_rate/1000,
			GetH264ProfileName(ispCameraImageSettings.PrevVideoAvcProfile));
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Med_GetCompatibleMetaDataConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetCompatibleMetadataConfigurationsResponse>\r\n"
				"<trt:Configurations token=\"MetadataToken_1\">\r\n"
					"<tt:Name>MetadataConfiguration_1</tt:Name>\r\n"
					"<tt:UseCount>0</tt:UseCount>\r\n"
					"<tt:PTZStatus>\r\n"
						"<tt:Status>true</tt:Status>\r\n"
						"<tt:Position>false</tt:Position>\r\n"
					"</tt:PTZStatus>\r\n"
					"<tt:Analytics>false</tt:Analytics>\r\n"
					"<tt:SessionTimeout>PT60S</tt:SessionTimeout>\r\n"
				"</trt:Configurations>\r\n"
			"</trt:GetCompatibleMetadataConfigurationsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_GetVideoSourceConfs(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);

	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetVideoSourceConfigurationsResponse>\r\n"
				"<trt:Configurations token=\"V_SRC_CFG_1\">\r\n"
					"<tt:Name>V_SRC_CFG_1</tt:Name>\r\n"
					"<tt:UseCount>2</tt:UseCount>\r\n"
					"<tt:SourceToken>V_SRC_1</tt:SourceToken>\r\n"
					"<tt:Bounds x=\"%i\" y=\"%i\" width=\"%i\" height=\"%i\" />\r\n"
				"</trt:Configurations>\r\n"
			"</trt:GetVideoSourceConfigurationsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			ispCameraImageSettings.FrameLeftCrop,
			ispCameraImageSettings.FrameUpCrop,
			100 - ispCameraImageSettings.FrameRightCrop - ispCameraImageSettings.FrameLeftCrop,
			100 - ispCameraImageSettings.FrameDownCrop -  ispCameraImageSettings.FrameUpCrop);
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Med_GetAudioSourceConfs(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	char subbuff[256];
	int n;
	int iLen = 0;
	
	strcpy(buff, "<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetAudioSourceConfigurationsResponse>\r\n");
		
	for (n = 0; n < session->MicCount; n++)
	{
		memset(subbuff, 0, 256);
		snprintf(subbuff, 256, 
							"<trt:Configurations token=\"A_SRC_CFG_%.4s\">\r\n"
								"<tt:Name>A_SRC_CFG_%.4s</tt:Name>\r\n"
								"<tt:UseCount>2</tt:UseCount>\r\n"
								"<tt:SourceToken>A_SRC_%.4s</tt:SourceToken>\r\n"
							"</trt:Configurations>\r\n",
							(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID);
		strcat(buff, subbuff);
		iLen += strlen(subbuff);
		if ((iLen + 256) > ONVIF_TX_BUF_SIZE_MAX) break;		
	}
	
	strcat(buff, "</trt:GetAudioSourceConfigurationsResponse>\r\n"
					"</s:Body>\r\n"
					"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_SetVideoEncoderConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char encoder[32];
	char quality[32];
	char width[32];
	char height[32];
	char frameratelimit[32];
	char encodinginterval[32];
	char bitratelimit[32];
	char h264profile[32];
	encoder[31] = 0;	
	quality[31] = 0;
	width[31] = 0;	
	height[31] = 0;
	frameratelimit[31] = 0;	
	encodinginterval[31] = 0;
	bitratelimit[31] = 0;	
	h264profile[31] = 0;
	int res = 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Configuration", "token", encoder, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Quality", NULL, quality, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Width", NULL, width, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Height", NULL, height, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "FrameRateLimit", NULL, frameratelimit, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "EncodingInterval", NULL, encodinginterval, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "BitrateLimit", NULL, bitratelimit, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "H264Profile", NULL, h264profile, 31) == 0;
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"	
			"<trt:SetVideoEncoderConfigurationResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
	
	if (!res)
	{
		res = 0;
		DBG_MUTEX_LOCK(&modulelist_mutex);
		if (strcasecmp(encoder, "V_ENC_1") == 0)
		{
			ispCameraImageSettings.Updated = 1;
			ispCameraImageSettings.MainVideo.video_frame_rate = Str2Int(frameratelimit);
			ispCameraImageSettings.MainVideo.video_width = Str2Int(width);
			ispCameraImageSettings.MainVideo.video_height = Str2Int(height);
			ispCameraImageSettings.MainVideo.video_bit_rate = Str2Int(bitratelimit) * 1000;
			ispCameraImageSettings.MainVideoAvcProfile = GetH264ProfileCode(h264profile);
			ispCameraImageSettings.MainVideoAvcLevel = (int)Str2Float(quality);
			ispCameraImageSettings.MainVideo.video_intra_frame = Str2Int(encodinginterval);
			res = 1;
		}
		if (strcasecmp(encoder, "V_ENC_2") == 0)
		{
			ispCameraImageSettings.Updated = 1;
			ispCameraImageSettings.PrevVideo.video_frame_rate = Str2Int(frameratelimit);
			ispCameraImageSettings.PrevVideo.video_width = Str2Int(width);
			ispCameraImageSettings.PrevVideo.video_height = Str2Int(height);
			ispCameraImageSettings.PrevVideo.video_bit_rate = Str2Int(bitratelimit) * 1000;
			ispCameraImageSettings.PrevVideoAvcProfile = GetH264ProfileCode(h264profile);
			ispCameraImageSettings.PrevVideoAvcLevel = (int)Str2Float(quality);
			ispCameraImageSettings.PrevVideo.video_intra_frame = Str2Int(encodinginterval);
			res = 1;
		}	
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		
		if (res) 
		{
			omx_set_image_sensor_camera_params(&ispCameraImageSettings);
			SaveModules();
		}
	}
}

static void ONVIV_Template_Img_GetMoveOptions(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<timg:GetMoveOptionsResponse>\r\n"
				"<timg:MoveOptions>\r\n"
					"<tt:Continuous>\r\n"
						"<tt:Speed>\r\n"
							"<tt:Min>1.00</tt:Min>\r\n"
							"<tt:Max>5.00</tt:Max>\r\n"
						"</tt:Speed>\r\n"
					"</tt:Continuous>\r\n"
				"</timg:MoveOptions>\r\n"
			"</timg:GetMoveOptionsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_GetConfigurations(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	/*DBG_MUTEX_LOCK(&modulelist_mutex);
				
	MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
	char uiPtzEn = 0;
	unsigned int uiPtzID = 0;
	if (miCamera)
	{
		uiPtzEn = miCamera->Settings[38] & 1;
		uiPtzID = miCamera->Settings[39];		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
				
	if (uiPtzEn && uiPtzID)		
	{*/
		snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:GetConfigurationsResponse>\r\n"
				"<tptz:PTZConfiguration token=\"PTZCFG_1\" MoveRamp=\"0\" PresetRamp=\"0\" PresetTourRamp=\"0\">\r\n"
					"<tt:Name>PTZCFG_1</tt:Name>\r\n"
					"<tt:UseCount>2</tt:UseCount>\r\n"
					"<tt:NodeToken>PTZNODE_1</tt:NodeToken>\r\n"
					"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\r\n"
					"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\r\n"
					"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\r\n"
					"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\r\n"
					"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\r\n"
					"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\r\n"
					"<tt:DefaultPTZSpeed>\r\n"
						"<tt:PanTilt x=\"0.5\" y=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" />\r\n"
						"<tt:Zoom x=\"0.5\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" />\r\n"
					"</tt:DefaultPTZSpeed>\r\n"
					"<tt:DefaultPTZTimeout>PT5S</tt:DefaultPTZTimeout>\r\n"
					"<tt:PanTiltLimits>\r\n"
						"<tt:Range>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:Range>\r\n"
					"</tt:PanTiltLimits>\r\n"
					"<tt:ZoomLimits>\r\n"
						"<tt:Range>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:Range>\r\n"
					"</tt:ZoomLimits>\r\n"
					"<tt:Extension>\r\n"
						"<tt:PTControlDirection>\r\n"
							"<tt:EFlip>\r\n"
								"<tt:Mode>OFF</tt:Mode>\r\n"
							"</tt:EFlip>\r\n"
							"<tt:Reverse>\r\n"
								"<tt:Mode>OFF</tt:Mode>\r\n"
							"</tt:Reverse>\r\n"
						"</tt:PTControlDirection>\r\n"
					"</tt:Extension>\r\n"
				"</tptz:PTZConfiguration>\r\n"
			"</tptz:GetConfigurationsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
	/*}
	else
	{
		snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:GetConfigurationsResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");	
	}*/
}

static void ONVIV_Template_Ptz_GetNode(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:GetNodeResponse>\r\n"
				"<tptz:PTZNode token=\"PTZNODE_1\" FixedHomePosition=\"false\" GeoMove=\"false\">\r\n"
					"<tt:Name>PTZNODE_1</tt:Name>\r\n"
					"<tt:SupportedPTZSpaces>\r\n"
						"<tt:AbsolutePanTiltPositionSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:AbsolutePanTiltPositionSpace>\r\n"
						"<tt:AbsoluteZoomPositionSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:AbsoluteZoomPositionSpace>\r\n"
						"<tt:RelativePanTiltTranslationSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:RelativePanTiltTranslationSpace>\r\n"
						"<tt:RelativeZoomTranslationSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:RelativeZoomTranslationSpace>\r\n"
						"<tt:ContinuousPanTiltVelocitySpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:ContinuousPanTiltVelocitySpace>\r\n"
						"<tt:ContinuousZoomVelocitySpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:ContinuousZoomVelocitySpace>\r\n"
						"<tt:PanTiltSpeedSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:PanTiltSpeedSpace>\r\n"
						"<tt:ZoomSpeedSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:ZoomSpeedSpace>\r\n"
					"</tt:SupportedPTZSpaces>\r\n"
					"<tt:MaximumNumberOfPresets>100</tt:MaximumNumberOfPresets>\r\n"
					"<tt:HomeSupported>true</tt:HomeSupported>\r\n"
					"<tt:AuxiliaryCommands>Wiper start</tt:AuxiliaryCommands>\r\n"
					"<tt:AuxiliaryCommands>Wiper stop</tt:AuxiliaryCommands>\r\n"
					"<tt:Extension>\r\n"
						"<tt:SupportedPresetTour>\r\n"
							"<tt:MaximumNumberOfPresetTours>10</tt:MaximumNumberOfPresetTours>\r\n"
							"<tt:PTZPresetTourOperation>Start</tt:PTZPresetTourOperation>\r\n"
							"<tt:PTZPresetTourOperation>Stop</tt:PTZPresetTourOperation>\r\n"
							"<tt:PTZPresetTourOperation>Pause</tt:PTZPresetTourOperation>\r\n"
						"</tt:SupportedPresetTour>\r\n"
					"</tt:Extension>\r\n"
				"</tptz:PTZNode>\r\n"
			"</tptz:GetNodeResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_GetNodes(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:GetNodesResponse>\r\n"
				"<tptz:PTZNode token=\"PTZNODE_1\" FixedHomePosition=\"false\" GeoMove=\"false\">\r\n"
					"<tt:Name>PTZNODE_1</tt:Name>\r\n"
					"<tt:SupportedPTZSpaces>\r\n"
						"<tt:AbsolutePanTiltPositionSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:AbsolutePanTiltPositionSpace>\r\n"
						"<tt:AbsoluteZoomPositionSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:AbsoluteZoomPositionSpace>\r\n"
						"<tt:RelativePanTiltTranslationSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:RelativePanTiltTranslationSpace>\r\n"
						"<tt:RelativeZoomTranslationSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:RelativeZoomTranslationSpace>\r\n"
						"<tt:ContinuousPanTiltVelocitySpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
							"<tt:YRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:YRange>\r\n"
						"</tt:ContinuousPanTiltVelocitySpace>\r\n"
						"<tt:ContinuousZoomVelocitySpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>-1.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:ContinuousZoomVelocitySpace>\r\n"
						"<tt:PanTiltSpeedSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:PanTiltSpeedSpace>\r\n"
						"<tt:ZoomSpeedSpace>\r\n"
							"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace</tt:URI>\r\n"
							"<tt:XRange>\r\n"
								"<tt:Min>0.0</tt:Min>\r\n"
								"<tt:Max>1.0</tt:Max>\r\n"
							"</tt:XRange>\r\n"
						"</tt:ZoomSpeedSpace>\r\n"
					"</tt:SupportedPTZSpaces>\r\n"
					"<tt:MaximumNumberOfPresets>100</tt:MaximumNumberOfPresets>\r\n"
					"<tt:HomeSupported>true</tt:HomeSupported>\r\n"
					"<tt:AuxiliaryCommands>Wiper start</tt:AuxiliaryCommands>\r\n"
					"<tt:AuxiliaryCommands>Wiper stop</tt:AuxiliaryCommands>\r\n"
					"<tt:Extension>\r\n"
						"<tt:SupportedPresetTour>\r\n"
							"<tt:MaximumNumberOfPresetTours>10</tt:MaximumNumberOfPresetTours>\r\n"
							"<tt:PTZPresetTourOperation>Start</tt:PTZPresetTourOperation>\r\n"
							"<tt:PTZPresetTourOperation>Stop</tt:PTZPresetTourOperation>\r\n"
							"<tt:PTZPresetTourOperation>Pause</tt:PTZPresetTourOperation>\r\n"
						"</tt:SupportedPresetTour>\r\n"
					"</tt:Extension>\r\n"
				"</tptz:PTZNode>\r\n"
			"</tptz:GetNodesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_GetStatus(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, int uiPTZnum)
{
	PTZ_SET_INFO psiPtzSettings;
	memset(&psiPtzSettings , 0, sizeof(psiPtzSettings));
		
	if (uiPTZnum >= 0)
	{
		DBG_MUTEX_LOCK(&modulelist_mutex);
		if (uiPTZnum < iModuleCnt)
		{
			psiPtzSettings.PanTiltX = (float)miModuleList[uiPTZnum].Status[5] / 100000;
			psiPtzSettings.PanTiltY = (float)miModuleList[uiPTZnum].Status[6] / 100000;
			psiPtzSettings.Focus = (float)miModuleList[uiPTZnum].Status[7] / 100000;
			psiPtzSettings.Zoom = (float)miModuleList[uiPTZnum].Status[8] / 100000;			
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
	}
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:GetStatusResponse>\r\n"
				"<tptz:PTZStatus>\r\n"
					"<tt:Position>\r\n"
						"<tt:PanTilt x=\"%g\" y=\"%g\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace\" />\r\n"
						"<tt:Zoom x=\"%g\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace\" />\r\n"
					"</tt:Position>\r\n"
					"<tt:MoveStatus>\r\n"
						"<tt:PanTilt>IDLE</tt:PanTilt>\r\n"
						"<tt:Zoom>IDLE</tt:Zoom>\r\n"
					"</tt:MoveStatus>\r\n"
					"<tt:UtcTime>2021-02-17T11:15:15Z</tt:UtcTime>\r\n"
				"</tptz:PTZStatus>\r\n"
			"</tptz:GetStatusResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			psiPtzSettings.PanTiltX, psiPtzSettings.PanTiltY, psiPtzSettings.Zoom);
}

static void ONVIV_Template_Ptz_GetPresets(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char subbuff[1024];
	strcpy(buff, "<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:GetPresetsResponse>\r\n");
			
	DBG_MUTEX_LOCK(&ptz_mutex);
	
	int i;
	for (i = 0; i < iPtzSettingsListCnt; i++)
	{
		if (psiPtzSettingsList[i].Used)
		{
			snprintf(subbuff, 1024, 
				"<tptz:Preset token=\"PRESET_%i\">\r\n"
					"<tt:Name>%s</tt:Name>\r\n"
					"<tt:PTZPosition>\r\n"
						"<tt:PanTilt x=\"%g\" y=\"%g\" />\r\n"
						"<tt:Zoom x=\"%g\" />\r\n"
					"</tt:PTZPosition>\r\n"
				"</tptz:Preset>\r\n",
				i, psiPtzSettingsList[i].Name, psiPtzSettingsList[i].PanTiltX, psiPtzSettingsList[i].PanTiltY, psiPtzSettingsList[i].Zoom);
			strcat(buff, subbuff);
		}
	}		
	DBG_MUTEX_UNLOCK(&ptz_mutex);
	
	strcat(buff, "</tptz:GetPresetsResponse>\r\n"
				"</s:Body>\r\n"
				"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_Stop(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, MODULE_INFO *miCamera)
{
	char pantilt[32];
	char zoom[32];
	pantilt[31] = 0;
	zoom[31] = 0;
	if ((ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "PanTilt", NULL, pantilt, 31)) &&
		(ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Zoom", NULL, zoom, 31)))
	{				
		DBG_MUTEX_LOCK(&modulelist_mutex);
	
		MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
		char uiPtzEn = 0;
		unsigned int uiPtzID = 0;
		if (miCamera)
		{
			uiPtzEn = miCamera->Settings[38] & 1;
			uiPtzID = miCamera->Settings[39];		
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		
		if (uiPtzEn && uiPtzID)
		{
			int pantilt_en = strcasecmp(pantilt, "true") == 0;
			int zoom_en = strcasecmp(zoom, "true") == 0;
			if (pantilt_en) 
			{
				AddModuleEventInList(uiPtzID, PTZ_TARGET_STOP_PAN, 0, NULL, 0, 1);
				AddModuleEventInList(uiPtzID, PTZ_TARGET_STOP_TILT, 0, NULL, 0, 1);
			}
			if (zoom_en) 
			{
				AddModuleEventInList(uiPtzID, PTZ_TARGET_STOP_ZOOM, 0, NULL, 0, 1);
				AddModuleEventInList(uiPtzID, PTZ_TARGET_STOP_FOCUS, 0, NULL, 0, 1);
			}
			if (pantilt_en || zoom_en)
			{
				DBG_MUTEX_LOCK(&modulelist_mutex);
				if (zoom_en) ispCameraImageSettings.StartAutofocus = -1;
				ispCameraImageSettings.ChangedPosition = 1;
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
			}
		}	
	}			
		
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:StopResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_ContinuousMove(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char velocity[512];
	velocity[512] = 0;		
		
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Velocity", NULL, velocity, 512))	
	{
		int len = strlen(velocity);
		char panx[32];
		char pany[32];
		char panz[32];
		memset(panx, 0, 32);
		memset(pany, 0, 32);
		memset(panz, 0, 32);
		char pantilt_en = (ONVIF_get_node_param_from_url(velocity, len, "PanTilt", "x", panx, 31)) &&
					(ONVIF_get_node_param_from_url(velocity, len, "PanTilt", "y", pany, 31));
		char zoom_en = (ONVIF_get_node_param_from_url(velocity, len, "Zoom", "x", panz, 31));
		
		float px = 0;
		float py = 0;
		float pz = 0;
		
		if (pantilt_en || zoom_en)
		{
				if (pantilt_en)
				{
					px = Str2Float(panx);
					py = Str2Float(pany);
				}
				if (zoom_en) pz = Str2Float(panz);
				if ((px == 0.0f) && (py == 0.0f)) pantilt_en = 0;
				if (pz == 0.0f) zoom_en = 0;
			
				DBG_MUTEX_LOCK(&modulelist_mutex);
	
				MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
				char uiPtzEn = 0;
				unsigned int uiPtzID = 0;
				if (miCamera)
				{
					uiPtzEn = miCamera->Settings[38] & 1;
					uiPtzID = miCamera->Settings[39];		
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				
				if (uiPtzEn && uiPtzID)
				{						
					if (pantilt_en) 
					{						
						AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, 0, NULL, 0, 1);
						AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, 0, NULL, 0, 1);
						AddModuleEventInList(uiPtzID, PTZ_TARGET_CONT_MS_MOVE_PAN, px * 100000, NULL, 0, 1);
						AddModuleEventInList(uiPtzID, PTZ_TARGET_CONT_MS_MOVE_TILT, py * 100000, NULL, 0, 1);
					}
					if (zoom_en) 
					{
						//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 1, NULL, 0, 1);
						//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, 100000, NULL, 0, 1);
						AddModuleEventInList(uiPtzID, PTZ_TARGET_CONT_MS_MOVE_ZOOM, pz * 100000, NULL, 0, 1);
						//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100, NULL, 0, 1);
						//AddModuleEventInList(uiPtzID, PTZ_TARGET_CONT_MS_MOVE_FOCUS, pz * 100, NULL, 0, 1);
						//printf("PTZ_TARGET_CONT_MS_MOVE_ZOOM %f\n", pz);
					}
					if (pantilt_en || zoom_en)
					{
						DBG_MUTEX_LOCK(&modulelist_mutex);
						if (zoom_en && (pz != 0.0f)) ispCameraImageSettings.StartAutofocus = (pz > 0) ? 1 : -1;
						ispCameraImageSettings.ChangedPosition = 1;
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
					}
				}	
			}
	}
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<tptz:ContinuousMoveResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_RelativeMove(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char translation[256];
	char speed[256];
	memset(translation, 0, 256);
	memset(speed, 0, 256);
	char trans_f = 0;
	char spd_f = 0;
	
	trans_f = ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Translation", NULL, translation, 255);
	spd_f = ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Speed", NULL, speed, 255);
	if (trans_f || spd_f)
	{
		int trlen = strlen(translation);
		int splen = strlen(speed);
		char panx[32];
		char pany[32];
		char panz[32];
		char speedx[32];
		char speedy[32];
		char speedz[32];
		memset(panx, 0, 32);
		memset(pany, 0, 32);
		memset(panz, 0, 32);
		memset(speedx, 0, 32);
		memset(speedy, 0, 32);
		memset(speedz, 0, 32);
		
		char pantilt_en = (ONVIF_get_node_param_from_url(translation, trlen, "PanTilt", "x", panx, 31)) &&
					(ONVIF_get_node_param_from_url(translation, trlen, "PanTilt", "y", pany, 31));
		char zoom_en = (ONVIF_get_node_param_from_url(translation, trlen, "Zoom", "x", panz, 31));
		
		char pantilt_spd = (ONVIF_get_node_param_from_url(speed, splen, "PanTilt", "x", speedx, 31)) &&
					(ONVIF_get_node_param_from_url(speed, splen, "PanTilt", "y", speedy, 31));
		char zoom_spd = (ONVIF_get_node_param_from_url(speed, splen, "Zoom", "x", speedz, 31));
		
		float px = 0;
		float py = 0;
		float pz = 0;
		float sx = 0.5;
		float sy = 0.5;
		float sz = 0.5;
		
		if (pantilt_en || zoom_en)
		{
			if (pantilt_en)
			{
				px = Str2Float(panx);
				py = Str2Float(pany);
			}
			if (pantilt_spd)
			{
				sx = Str2Float(speedx);
				sy = Str2Float(speedy);
			}
			if (zoom_en) pz = Str2Float(panz);
			if (zoom_spd) sz = Str2Float(speedz);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);
			
			MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
			char uiPtzEn = 0;
			unsigned int uiPtzID = 0;
			if (miCamera)
			{
				uiPtzEn = miCamera->Settings[38] & 1;
				uiPtzID = miCamera->Settings[39];		
			}
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			
			if (uiPtzEn && uiPtzID)
			{				
				if (pantilt_en) 
				{
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, sx * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, sy * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_STEP_MS_MOVE_PAN, px * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_STEP_MS_MOVE_TILT, py * 100000, NULL, 0, 1);
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_STEP_RAW_FOCUS, (py >= 0) ? 1 : -1, NULL, 0, 1);
				}
				if (zoom_en) 
				{
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 1, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, sz * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_STEP_MS_MOVE_ZOOM, pz * 100000, NULL, 0, 1);
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_STEP_MS_MOVE_ZOOM, (pz >= 0) ? 5000 : -5000, NULL, 0, 1);
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, sz * 100, NULL, 0, 1);
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_STEP_MS_MOVE_FOCUS, pz * 100, NULL, 0, 1);
				}
				if (pantilt_en || zoom_en)
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);
					if (zoom_en && (pz != 0.0f)) ispCameraImageSettings.StartAutofocus = (pz > 0) ? 1 : -1;
					ispCameraImageSettings.ChangedPosition = 1;
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
				}
			}	
		}
	}
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:RelativeMoveResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_AbsoluteMove(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char position[256];
	char speed[256];
	memset(position, 0, 256);
	memset(speed, 0, 256);
	char position_f = 0;
	char spd_f = 0;
	
	position_f = ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Position", NULL, position, 255);
	spd_f = ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "Speed", NULL, speed, 255);	
	if (position_f || spd_f)
	{
		int trlen = strlen(position);
		int splen = strlen(speed);
		char panx[32];
		char pany[32];
		char panz[32];
		char speedx[32];
		char speedy[32];
		char speedz[32];
		memset(panx, 0, 32);
		memset(pany, 0, 32);
		memset(panz, 0, 32);
		memset(speedx, 0, 32);
		memset(speedy, 0, 32);
		memset(speedz, 0, 32);
		
		char pantilt_en = (ONVIF_get_node_param_from_url(position, trlen, "PanTilt", "x", panx, 31)) &&
					(ONVIF_get_node_param_from_url(position, trlen, "PanTilt", "y", pany, 31));
		char zoom_en = (ONVIF_get_node_param_from_url(position, trlen, "Zoom", "x", panz, 31));
		
		char pantilt_spd = (ONVIF_get_node_param_from_url(speed, splen, "PanTilt", "x", speedx, 31)) &&
					(ONVIF_get_node_param_from_url(speed, splen, "PanTilt", "y", speedy, 31));
		char zoom_spd = (ONVIF_get_node_param_from_url(speed, splen, "Zoom", "x", speedz, 31));
		
		float px, py, pz;
		float sx = 0.5;
		float sy = 0.5;
		float sz = 0.5;		
		
		if (pantilt_en || zoom_en)
		{
			if (pantilt_en)
			{
				px = Str2Float(panx);
				py = Str2Float(pany);
			}
			if (pantilt_spd)
			{
				sx = Str2Float(speedx);
				sy = Str2Float(speedy);
			}
			if (zoom_en) pz = Str2Float(panz);
			if (zoom_spd) sz = Str2Float(speedz);
			
			DBG_MUTEX_LOCK(&modulelist_mutex);
			
			MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
			char uiPtzEn = 0;
			unsigned int uiPtzID = 0;
			if (miCamera)
			{
				uiPtzEn = miCamera->Settings[38] & 1;
				uiPtzID = miCamera->Settings[39];		
			}
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			
			if (uiPtzEn && uiPtzID)
			{				
				if (pantilt_en)
				{
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, sx * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, sy * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_PAN, px * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_TILT, py * 100000, NULL, 0, 1);
				}
				if (zoom_en) 
				{
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 1, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_ZOOM, pz * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, sz * 100000, NULL, 0, 1);
					//printf("s %i p %i\n", (int)(sz * 100000), (int)(pz * 100000));
					//printf("PTZ_TARGET_ABSLT_MS_MOVE_ZOOM4 %f\n", pz);
				}
				if (pantilt_en || zoom_en)
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);
					if (zoom_en) ispCameraImageSettings.StartAutofocus = 1;
					ispCameraImageSettings.ChangedPosition = 1;
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
				}
			}	
		}
	}
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<tptz:AbsoluteMoveResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

void onvif_GotoHomePosition(unsigned int uiPtzID)
{
	PTZ_SET_INFO psiPtzSettings;
	DBG_MUTEX_LOCK(&ptz_mutex);
	memcpy(&psiPtzSettings, &psiPtzHomeSettings, sizeof(psiPtzSettings));
	DBG_MUTEX_UNLOCK(&ptz_mutex);
		
	AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, 100000, NULL, 0, 1);
	AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, 100000, NULL, 0, 1);
	AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, 100000, NULL, 0, 1);
	AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);
	//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 2, NULL, 0, 1);
		
	AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_PAN, psiPtzSettings.PanTiltX * 100000, NULL, 0, 1);
	AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_TILT, psiPtzSettings.PanTiltY * 100000, NULL, 0, 1);	
	AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_ZOOM, psiPtzSettings.Zoom * 100000, NULL, 0, 1);	
	AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_FOCUS, psiPtzSettings.Focus * 100000, NULL, 0, 1);	
}

void onvif_GotoPresetPosition(unsigned int uiPtzID, unsigned int uiTarget)
{
	PTZ_SET_INFO psiPtzSettings;	
	memset(&psiPtzSettings, 0, sizeof(psiPtzSettings));
	
	if (uiTarget == 0)
	{
		onvif_GotoHomePosition(uiPtzID);
		return;
	}
	uiTarget--;
	
	int result = 0;
	DBG_MUTEX_LOCK(&ptz_mutex);
	if (uiTarget < iPtzSettingsListCnt)
	{
		if (psiPtzSettingsList[uiTarget].Used)
		{
			memcpy(&psiPtzSettings, &psiPtzSettingsList[uiTarget], sizeof(psiPtzSettings));
			result = 1;	
		}
	} else dbgprintf(2, "GotoPresetPosition: PTZ (%.4s) setting not found: %i\n", (char*)&uiPtzID, uiTarget + 1);	
	DBG_MUTEX_UNLOCK(&ptz_mutex);	
		
	if (result)
	{		
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);
		//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 2, NULL, 0, 1);
		
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_PAN, psiPtzSettings.PanTiltX * 100000, NULL, 0, 1);					
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_TILT, psiPtzSettings.PanTiltY * 100000, NULL, 0, 1);	
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_ZOOM, psiPtzSettings.Zoom * 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_FOCUS, psiPtzSettings.Focus * 100000, NULL, 0, 1);
		printf("PTZ_TARGET_ABSLT_MS_MOVE_ZOOM3 %f\n",psiPtzSettings.Zoom);
	}
}

static void ONVIV_Template_Ptz_GotoHomePosition(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
	char uiPtzEn = 0;
	unsigned int uiPtzID = 0;
	if (miCamera)
	{
		uiPtzEn = miCamera->Settings[38] & 1;
		uiPtzID = miCamera->Settings[39];		
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	if (uiPtzEn && uiPtzID)
	{
		PTZ_SET_INFO psiPtzSettings;
		DBG_MUTEX_LOCK(&ptz_mutex);
		memcpy(&psiPtzSettings, &psiPtzHomeSettings, sizeof(psiPtzSettings));
		DBG_MUTEX_UNLOCK(&ptz_mutex);
		
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);
		//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 2, NULL, 0, 1);
		
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_PAN, psiPtzSettings.PanTiltX * 100000, NULL, 0, 1);
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_TILT, psiPtzSettings.PanTiltY * 100000, NULL, 0, 1);	
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_ZOOM, psiPtzSettings.Zoom * 100000, NULL, 0, 1);	
		AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_FOCUS, psiPtzSettings.Focus * 100000, NULL, 0, 1);	
		//printf("PTZ_TARGET_ABSLT_MS_MOVE_ZOOM2 %f\n", psiPtzSettings.Zoom);
	}
		
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<tptz:GotoHomePositionResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_GotoPreset(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	PTZ_SET_INFO psiPtzSettings;	
	memset(&psiPtzSettings, 0, sizeof(psiPtzSettings));
	char cName[64];
	cName[63] = 0;
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "PresetToken", NULL, cName, 63))	
	{
		int lenname = strlen(cName);
		int lenset = strlen("PRESET_");
		if (lenname > lenset)
		{
			unsigned int iNumSet = Str2Int(&cName[lenset]);
			
			int result = 0;
			DBG_MUTEX_LOCK(&ptz_mutex);
			if (iNumSet < iPtzSettingsListCnt)
			{
				if (psiPtzSettingsList[iNumSet].Used)
				{
					memcpy(&psiPtzSettings, &psiPtzSettingsList[iNumSet], sizeof(psiPtzSettings));
					result = 1;	
				}
			}	
			DBG_MUTEX_UNLOCK(&ptz_mutex);	
		
			if (result)
			{
				DBG_MUTEX_LOCK(&modulelist_mutex);
				
				MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
				char uiPtzEn = 0;
				unsigned int uiPtzID = 0;
				if (miCamera)
				{
					uiPtzEn = miCamera->Settings[38] & 1;
					uiPtzID = miCamera->Settings[39];	
					//ispCameraImageSettings.StartAutofocus = 2;	
					ispCameraImageSettings.ChangedPosition = 1;
				}
				DBG_MUTEX_UNLOCK(&modulelist_mutex);
				
				if (uiPtzEn && uiPtzID)		
				{
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_PAN, 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_TILT, 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);
					//AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_MODE_FOCUS, 2, NULL, 0, 1);
					//printf("!!! %f %f %f %f\n", psiPtzSettings.PanTiltX, psiPtzSettings.PanTiltY, psiPtzSettings.Zoom, psiPtzSettings.Focus);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_PAN, psiPtzSettings.PanTiltX * 100000, NULL, 0, 1);					
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_AS_MOVE_TILT, psiPtzSettings.PanTiltY * 100000, NULL, 0, 1);	
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_ZOOM, psiPtzSettings.Zoom * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_ABSLT_MS_MOVE_FOCUS, psiPtzSettings.Focus * 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_ZOOM, 100000, NULL, 0, 1);
					AddModuleEventInList(uiPtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);
					
					//printf("PTZ_TARGET_ABSLT_MS_MOVE_ZOOM1 %f\n", psiPtzSettings.Zoom);
				}
			}
		}
	}
			
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"			
			"<tptz:GotoPresetResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_SetPreset(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	PTZ_SET_INFO psiPtzSettings;
	memset(&psiPtzSettings , 0, sizeof(psiPtzSettings));
	int result = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
	
	if (miCamera)
	{
		char uiPtzEn = miCamera->Settings[38] & 1;
		unsigned int uiPtzID = miCamera->Settings[39];
		
		if (uiPtzEn && uiPtzID)
		{
			int n;
			for (n = 0; n < iModuleCnt; n++)
				if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) && (miModuleList[n].ID == uiPtzID) &&
					(miModuleList[n].Type == MODULE_TYPE_USB_GPIO) && (miModuleList[n].SubType == MODULE_SUBTYPE_PTZ)) 
					{
						psiPtzSettings.PanTiltX = (float)miModuleList[n].Status[5] / 100000;
						psiPtzSettings.PanTiltY = (float)miModuleList[n].Status[6] / 100000;
						psiPtzSettings.Focus = (float)miModuleList[n].Status[7] / 100000;
						psiPtzSettings.Zoom = (float)miModuleList[n].Status[8] / 100000;
						result = 1;
						break;
					}				
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	if (result)
	{
		char cName[64];
		cName[63] = 0;
		int iSetNum = 0;
		if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "PresetName", NULL, cName, 63))	
				iSetNum = SetPtzSettingInList(cName, psiPtzSettings.PanTiltX, psiPtzSettings.PanTiltY, psiPtzSettings.Zoom, psiPtzSettings.Focus);
		snprintf(buff, buffsize, 
				"<s:Header>\r\n"
				"</s:Header>\r\n"
				"<s:Body>\r\n"
				"<tptz:SetPresetResponse>\r\n"
					"<tptz:PresetToken>PRESET_%i</tptz:PresetToken>\r\n"
				"</tptz:SetPresetResponse>\r\n"	
				"</s:Body>\r\n"
				"</s:Envelope>\r\n",
				iSetNum);
	}
}

static void ONVIV_Template_Ptz_RemovePreset(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char cToken[64];
	cToken[63] = 0;
	
	if (ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "PresetToken", NULL, cToken, 63))	
			DelPtzSettingInList(cToken);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:RemovePresetResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Ptz_SetHomePosition(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	PTZ_SET_INFO psiPtzSettings;
	memset(&psiPtzSettings , 0, sizeof(psiPtzSettings));
	int result = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	
	MODULE_INFO *miCamera = ModuleTypeToPoint(MODULE_TYPE_CAMERA, 1);
	
	if (miCamera)
	{
		char uiPtzEn = miCamera->Settings[38] & 1;
		unsigned int uiPtzID = miCamera->Settings[39];
		
		if (uiPtzEn && uiPtzID)
		{
			int n;
			for (n = 0; n < iModuleCnt; n++)
				if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) && (miModuleList[n].ID == uiPtzID) &&
					(miModuleList[n].Type == MODULE_TYPE_USB_GPIO) && (miModuleList[n].SubType == MODULE_SUBTYPE_PTZ)) 
					{
						psiPtzSettings.PanTiltX = (float)miModuleList[n].Status[5] / 100000;
						psiPtzSettings.PanTiltY = (float)miModuleList[n].Status[6] / 100000;
						psiPtzSettings.Focus = (float)miModuleList[n].Status[7] / 100000;
						psiPtzSettings.Zoom = (float)miModuleList[n].Status[8] / 100000;
						result = 1;
						break;
					}				
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	if (result)
	{
		DBG_MUTEX_LOCK(&ptz_mutex);
		memcpy(&psiPtzHomeSettings, &psiPtzSettings, sizeof(psiPtzHomeSettings));
		DBG_MUTEX_UNLOCK(&ptz_mutex);
	}
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tptz:SetHomePositionResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Recv_GetReceivers(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
	/*snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trv:GetReceiversResponse>\r\n"
				"<trv:Receivers>\r\n"
					"<tt:Token>ReceiverToken_3276771994</tt:Token>\r\n"
					"<tt:Configuration>\r\n"
						"<tt:Mode>AutoConnect</tt:Mode>\r\n"
						"<tt:MediaUri>3</tt:MediaUri>\r\n"
						"<tt:StreamSetup>\r\n"
							"<tt:Stream>RTP-Unicast</tt:Stream>\r\n"
							"<tt:Transport>\r\n"
								"<tt:Protocol>UDP</tt:Protocol>\r\n"
							"</tt:Transport>\r\n"
						"</tt:StreamSetup>\r\n"
					"</tt:Configuration>\r\n"
				"</trv:Receivers>\r\n"
			"</trv:GetReceiversResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");*/
}

static void ONVIV_Template_Recv_CreateReceiver(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
	/*snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trv:CreateReceiverResponse>\r\n"
				"<trv:Receiver>\r\n"
					"<tt:Token>ReceiverToken_3686880153</tt:Token>\r\n"
						"<tt:Configuration>\r\n"
							"<tt:Mode>AutoConnect</tt:Mode>\r\n"
							"<tt:MediaUri>video</tt:MediaUri>\r\n"
							"<tt:StreamSetup>\r\n"
								"<tt:Stream>RTP-Unicast</tt:Stream>\r\n"
								"<tt:Transport>\r\n"
									"<tt:Protocol>UDP</tt:Protocol>\r\n"
								"</tt:Transport>\r\n"
						"</tt:StreamSetup>\r\n"
					"</tt:Configuration>\r\n"
				"</trv:Receiver>\r\n"
			"</trv:CreateReceiverResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");*/
}

static void ONVIV_Template_Dev_SystemReboot(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:SystemRebootResponse>\r\n"
				"<tds:Message>Rebooting</tds:Message>\r\n"
			"</tds:SystemRebootResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
	System_Reboot(NULL, 0);
}

static void ONVIV_Template_Dev_SetSystemFactoryDefault(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char cResetType[32];
	cResetType[31] = 0;
	ONVIF_get_xml_value("FactoryDefault", cResetType, 31, pHEAD->Body, pHEAD->BodyLength);

	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:SetSystemFactoryDefaultResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
			
	if (strcasecmp(cResetType, "Hard") == 0) System_ResetDefault(NULL, 0);
	if (strcasecmp(cResetType, "Soft") == 0) 
	{
		int n;
		DBG_MUTEX_LOCK(&modulelist_mutex);
		for (n = 0; n < iModuleCnt; n++)
			if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1))
				SetModuleDefaultSettings(&miModuleList[n]);			
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		SaveModules();
		System_Reboot(NULL, 0);
	}
}

static void ONVIV_Template_Dev_SetScopes(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Dev_GetUsers(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	int iLen = buffsize - 256;
	if (iLen <= 0) return;
	iLen -= 256;
	int n;
	char subbuff[256];
	strcpy(buff, "<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetUsersResponse>\r\n");
			
	DBG_MUTEX_LOCK(&user_mutex);
	for (n = 0; n < iUserCnt; n++)
	{
		if (uiUserList[n].Enabled && (uiUserList[n].Access & ACCESS_ONVIF))
		{
			memset(subbuff, 0, 256);
			snprintf(subbuff, 255, "<tds:User>\r\n"
									"<tt:Username>%s</tt:Username>\r\n"
									"<tt:UserLevel>%s</tt:UserLevel>\r\n"
								 "</tds:User>\r\n", 
						uiUserList[n].Login,
						uiUserList[n].Level ? "Administrator" : "User");
			strcat(buff, subbuff);
			iLen -= strlen(subbuff);
			if (iLen <= 0) break;		
		}
	}	
	DBG_MUTEX_UNLOCK(&user_mutex);
	
	strcat(buff, "</tds:GetUsersResponse>\r\n"
				"</s:Body>\r\n"
				"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_GetServiceCapabilities(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetServiceCapabilitiesResponse>\r\n"
				"<tds:Capabilities>\r\n"
					"<tds:Network IPFilter=\"false\" ZeroConfiguration=\"true\" IPVersion6=\"false\" DynDNS=\"true\" Dot11Configuration=\"true\" Dot1XConfigurations=\"1\" HostnameFromDHCP=\"false\" NTP=\"2\" DHCPv6=\"false\"></tds:Network>\r\n"
					"<tds:Security TLS1.0=\"false\" TLS1.1=\"false\" TLS1.2=\"false\" OnboardKeyGeneration=\"false\" AccessPolicyConfig=\"true\" DefaultAccessPolicy=\"true\" Dot1X=\"true\" RemoteUserHandling=\"true\" X.509Token=\"false\" SAMLToken=\"false\" KerberosToken=\"false\" UsernameToken=\"false\" HttpDigest=\"true\" RELToken=\"false\" SupportedEAPMethods=\"0\" MaxUsers=\"10\" MaxUserNameLength=\"32\" MaxPasswordLength=\"32\"></tds:Security>\r\n"
					"<tds:System DiscoveryResolve=\"true\" DiscoveryBye=\"true\" RemoteDiscovery=\"false\" SystemBackup=\"false\" SystemLogging=\"true\" FirmwareUpgrade=\"false\" HttpFirmwareUpgrade=\"false\" HttpSystemBackup=\"false\" HttpSystemLogging=\"false\" HttpSupportInformation=\"true\" StorageConfiguration=\"false\" MaxStorageConfigurations=\"0\" GeoLocationEntries=\"0\" AutoGeo=\"\" StorageTypesSupported=\"\"></tds:System>\r\n"
					"<tds:Misc AuxiliaryCommands=\"\"></tds:Misc>\r\n"
				"</tds:Capabilities>\r\n"
			"</tds:GetServiceCapabilitiesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_GetZeroConfiguration(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	eth_config ifr;
	GetLocalNetIf(&ifr);

	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetZeroConfigurationResponse>\r\n"
				"<tds:ZeroConfiguration>\r\n"
					"<tt:InterfaceToken>%s</tt:InterfaceToken>\r\n"
					"<tt:Enabled>false</tt:Enabled>\r\n"
				"</tds:ZeroConfiguration>\r\n"
			"</tds:GetZeroConfigurationResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", ifr.if_name);
}

static void ONVIV_Template_Dev_SetZeroConfiguration(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Error(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, char* strCode, char* strReason)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<s:Fault>\r\n"
				"<s:Code>\r\n"
					"<s:Value>s:Receiver</s:Value>\r\n"
					"<s:Subcode>\r\n"
						"<s:Value>%s</s:Value>\r\n"
					"</s:Subcode>\r\n"
				"</s:Code>\r\n"
				"<s:Reason>\r\n"
					"<s:Text xml:lang=\"en\">%s</s:Text>\r\n"
				"</s:Reason>\r\n"
			"</s:Fault>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", strCode, strReason);
}

static void ONVIV_Template_Dev_GetNTP(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetNTPResponse>\r\n"
				"<tds:NTPInformation>\r\n"
					"<tt:FromDHCP>false</tt:FromDHCP>\r\n"
					"<tt:NTPManual>\r\n"
						"<tt:Type>DNS</tt:Type>\r\n"
						"<tt:DNSname></tt:DNSname>\r\n"
					"</tt:NTPManual>\r\n"
				"</tds:NTPInformation>\r\n"
			"</tds:GetNTPResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_SetNTP(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Dev_GetHostName(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char hostname[32];
	memset(hostname, 0, 32);
	gethostname(hostname, 32);
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetHostnameResponse>\r\n"
				"<tds:HostnameInformation>\r\n"
					"<tt:FromDHCP>false</tt:FromDHCP>\r\n"
					"<tt:Name>%s</tt:Name>\r\n"
				"</tds:HostnameInformation>\r\n"
			"</tds:GetHostnameResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", hostname);
}

static void ONVIV_Template_Dev_SetHostName(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Dev_GetDiscoveryMode(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetDiscoveryModeResponse>\r\n"
				"<tds:DiscoveryMode>Discoverable</tds:DiscoveryMode>\r\n"
			"</tds:GetDiscoveryModeResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_GetNetworkProtocols(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	DBG_MUTEX_LOCK(&system_mutex);			
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetNetworkProtocolsResponse>\r\n"
				"<tds:NetworkProtocols>\r\n"
					"<tt:Name>HTTP</tt:Name>\r\n"
					"<tt:Enabled>true</tt:Enabled>\r\n"
					"<tt:Port>%i</tt:Port>\r\n"
				"</tds:NetworkProtocols>\r\n"
				"<tds:NetworkProtocols>\r\n"
					"<tt:Name>RTSP</tt:Name>\r\n"
					"<tt:Enabled>true</tt:Enabled>\r\n"
					"<tt:Port>%i</tt:Port>\r\n"
				"</tds:NetworkProtocols>\r\n"
			"</tds:GetNetworkProtocolsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n", uiWebPort, uiRTSPPort);
	DBG_MUTEX_UNLOCK(&system_mutex);	
}

static void ONVIV_Template_Dev_GetNetworkDefaultGateway(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char route[64];
	
	GetLocalNetIfFromDhcpcd(NULL, route, NULL, NULL, 64);
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetNetworkDefaultGatewayResponse>\r\n"
				"<tds:NetworkGateway>\r\n"
					"<tt:IPv4Address>%s</tt:IPv4Address>\r\n"
				"</tds:NetworkGateway>\r\n"
			"</tds:GetNetworkDefaultGatewayResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			route);
}

static void ONVIV_Template_Dev_GetRelayOutputs(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char subbuff[2048];
	int n, i;
	strcpy(buff, "<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:GetRelayOutputsResponse>\r\n");
			
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1))
		{			
			if ((miModuleList[n].Type == MODULE_TYPE_GPIO) && (miModuleList[n].Settings[0] & MODULE_SECSET_OUTPUT)) 
			{
				snprintf(subbuff, 2048,			
							"<tds:RelayOutputs token=\"%.4s\">\r\n"
								"<tt:Properties>\r\n"
									"<tt:Mode>%s</tt:Mode>\r\n"
									"<tt:IdleState>%s</tt:IdleState>\r\n"
									"<tt:DelayTime>PT%iS</tt:DelayTime>\r\n"
								"</tt:Properties>\r\n"
							"</tds:RelayOutputs>\r\n",
							(char*)&miModuleList[n].ID,
							miModuleList[n].Settings[2] ? "Monostable" : "Bistable",  
							(miModuleList[n].Settings[0] & MODULE_SECSET_INVERSE) ? "closed" : "open",
							miModuleList[n].Settings[2]/1000);
				strcat(buff, subbuff);	
			}
			if ((miModuleList[n].Type == MODULE_TYPE_USB_GPIO) && miModuleList[n].ParamsCount) 
			{
				for (i = 0; i < miModuleList[n].SettingsCount; i++)
				{					
					int iPortSelected = miModuleList[n].SettingList[i].ID;
					if (iPortSelected >= miModuleList[n].ParamsCount) iPortSelected = 0;
						
					if (miModuleList[n].ParamList[iPortSelected].CurrentType == USBIO_PIN_SETT_TYPE_OUTPUT)
					{
						snprintf(subbuff, 2048,			
								"<tds:RelayOutputs token=\"%.4s_%i\">\r\n"
									"<tt:Properties>\r\n"
										"<tt:Mode>%s</tt:Mode>\r\n"
										"<tt:IdleState>%s</tt:IdleState>\r\n"
										"<tt:DelayTime>PT%iS</tt:DelayTime>\r\n"
									"</tt:Properties>\r\n"
								"</tds:RelayOutputs>\r\n",
								(char*)&miModuleList[n].ID, iPortSelected,
								(miModuleList[n].ParamList[iPortSelected].Mode == USBIO_CMD_MODE_STATIC) ? "Bistable" : "Monostable",  
								(miModuleList[n].ParamList[iPortSelected].DefaultValue ^ miModuleList[n].ParamList[iPortSelected].Inverted) ? "closed" : "open",
								miModuleList[n].ParamList[iPortSelected].ImpulseLen * 5 / 100);
						strcat(buff, subbuff);
					}
				}
			}
		}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);

	strcat(buff, "</tds:GetRelayOutputsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_SetRelayOutputState(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char token[32];
	char state[32];
	
	token[31] = 0;	
	state[31] = 0;
	int res = 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "RelayOutputToken", NULL, token, 31) == 0;
	res |= ONVIF_get_node_param_from_url(pHEAD->Body, pHEAD->BodyLength, "LogicalState", NULL, state, 31) == 0;

	if (!res)
	{
		unsigned int uiID = 0;
		unsigned int uiNum = 0;
				
		int len = strlen(token);
		int n, ret = 0;
		int status = 0;
		if (strcasecmp(state, "active") == 0) status = 1;
		for (n = 0; n < len; n++)
			if (token[n] == 95)
			{
				if (n && (n <= 4)) 
				{
					memcpy(&uiID, token, n);
					uiNum = Str2Int(&token[n + 1]);
					ret = 1;
					break;
				}
			}
		if ((ret == 0) && (len <= 4)) memcpy(&uiID, token, 4);
		
		ret = 0;
		DBG_MUTEX_LOCK(&modulelist_mutex);
		n = ModuleIdToNum(uiID, 1);
		if (n >= 0)
		{
			if ((miModuleList[n].Type == MODULE_TYPE_GPIO) && (miModuleList[n].Settings[0] & MODULE_SECSET_OUTPUT)) ret = 1;
			if ((miModuleList[n].Type == MODULE_TYPE_USB_GPIO) && miModuleList[n].ParamsCount && (uiNum < miModuleList[n].SettingsCount)) 
			{
				int iPortSelected = miModuleList[n].SettingList[uiNum].ID;
				if ((iPortSelected < miModuleList[n].ParamsCount) &&
					(miModuleList[n].ParamList[iPortSelected].CurrentType == USBIO_PIN_SETT_TYPE_OUTPUT)) ret = 1;
			}
		}
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		if (ret) AddModuleEventInList(uiID, uiNum, status, NULL, 0, 1);
	}
	
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:SetRelayOutputStateResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Dev_SetRelayOutputSettings(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Dev_GetSystemLog(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	if (buffsize <= 1024) return;
	char *log_body = (char*)DBG_MALLOC(buffsize - 1024);
	unsigned int dlen = load_last_dbg_data(log_body, buffsize - 1024);	
	
	strcpy(buff, "<s:Header>\r\n"
				"</s:Header>\r\n"
				"<s:Body>\r\n"
				"<tds:GetSystemLogResponse>\r\n"
					"<tds:SystemLog>\r\n"
					"<tt:String>");
	int n;
	for (n = 0; n < dlen; n++)
	{
		if ((log_body[n] == 38) || (log_body[n] == 60) || (log_body[n] == 62) || (log_body[n] == 34) || (log_body[n] == 39)) log_body[n] = 32;
		if ((log_body[n] < 32) && (log_body[n] != 10) && (log_body[n] != 13)) log_body[n] = 32;
		if (log_body[n] > 127) log_body[n] = 63;
	}
	
	unsigned int uiTx_pos = strlen(buff);	
	memcpy(&buff[uiTx_pos], log_body, dlen);	
	uiTx_pos += dlen;
	strcat(&buff[uiTx_pos], "</tt:String>\r\n"
							"</tds:SystemLog>\r\n"
							"</tds:GetSystemLogResponse>\r\n"
							"</s:Body>\r\n"
							"</s:Envelope>\r\n");
	DBG_FREE(log_body);
}

static void ONVIV_Template_Dev_CreateUsers(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Dev_DeleteUsers(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Dev_SetUser(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Dev_SetNetworkProtocols(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	char protocol[512];
	char name[32];
	char rtspport[32];
	char httpport[32];
	
	rtspport[31] = 0;	
	httpport[31] = 0;
	protocol[511] = 0;	
	name[31] = 0;
	
	int pos = 0;
	int ret;
	int iCntProt = 0;
	do
	{
		ret = ONVIF_get_node_param_from_url(&pHEAD->Body[pos], pHEAD->BodyLength - pos, "NetworkProtocols", NULL, protocol, 511);
		if (ret)
		{
			pos += ret;		
			iCntProt++;
			if (ONVIF_get_node_param_from_url(protocol, strlen(protocol), "Name", NULL, name, 31))
			{
				if (strcasecmp(name, "HTTP") == 0)
					ONVIF_get_node_param_from_url(protocol, strlen(protocol), "Port", NULL, httpport, 31);
				if (strcasecmp(name, "RTSP") == 0)
					ONVIF_get_node_param_from_url(protocol, strlen(protocol), "Port", NULL, rtspport, 31);
			}
		}		
	} while (ret && (iCntProt < 3));
			
	DBG_MUTEX_LOCK(&system_mutex);			
	uiRTSPPort = Str2Int(rtspport);
	uiWebPort = Str2Int(httpport);
	DBG_MUTEX_UNLOCK(&system_mutex);
	SaveSettings();
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tds:SetNetworkProtocolsResponse />\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Recv_ConfigureReceiver(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Recv_DeleteReceiver(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Evnt_Subscribe(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
						"<s:Header>\r\n"
							"<wsa:Action>http://docs.oasis-open.org/wsn/bw-2/NotificationProducer/SubscribeResponse</wsa:Action>\r\n"
						"</s:Header>\r\n"
						"<s:Body>\r\n"
						"<wsnt:SubscribeResponse>\r\n"
							"<wsnt:SubscriptionReference>\r\n"
								"<wsa:Address>http://www.example.org/SubscriptionManager</wsa:Address>\r\n"      
							"</wsnt:SubscriptionReference>\r\n"
						"</wsnt:SubscribeResponse>\r\n"
						"</s:Body>\r\n"
						"</s:Envelope>\r\n");
}

static void ONVIV_Template_Evnt_RenewRequest(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Evnt_UnsubscribeRequest(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");	
}

static void ONVIV_Template_Evnt_GetEventProperties(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<tev:GetEventPropertiesResponse>\r\n"
				"<tev:TopicNamespaceLocation>http://www.onvif.org/onvif/ver10/topics/topicns.xml</tev:TopicNamespaceLocation>\r\n"
				"<wsnt:FixedTopicSet>true</wsnt:FixedTopicSet>\r\n"
				"<wstop:TopicSet xmlns=\"\">\r\n"
					"<tns1:VideoSource wstop:topic=\"true\">\r\n"
						"<ImageTooBlurry wstop:topic=\"true\">\r\n"
							"<ImagingService wstop:topic=\"true\">\r\n"
								"<tt:MessageDescription IsProperty=\"true\">\r\n"
									"<tt:Source>\r\n"
										"<tt:SimpleItemDescription Name=\"Source\" Type=\"tt:ReferenceToken\"/>\r\n"
									"</tt:Source>\r\n"
									"<tt:Data>\r\n"
										"<tt:SimpleItemDescription Name=\"State\" Type=\"xs:boolean\"/>\r\n"
									"</tt:Data>\r\n"
								"</tt:MessageDescription>\r\n"
							"</ImagingService>\r\n"
						"</ImageTooBlurry>\r\n"
					"</tns1:VideoSource>\r\n"
				"</wstop:TopicSet>\r\n"
			"</tev:GetEventPropertiesResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_GetCompVideoAnalitConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_GetCompAudioEncConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD, ONVIF_SESSION *session)
{
	int n;
	for (n = 0; n < session->MicCount; n++)
	{
		snprintf(buff, buffsize, 
									"<s:Header>\r\n"
									"</s:Header>\r\n"
									"<s:Body>\r\n"
									"<trt:GetCompatibleAudioEncoderConfigurationsResponse>\r\n"
										"<trt:Configurations token=\"A_ENC_%.4s\">\r\n"
											"<tt:Name>A_ENC_%.4s</tt:Name>\r\n"
											"<tt:UseCount>2</tt:UseCount>\r\n"
											"<tt:Encoding>%s</tt:Encoding>\r\n"
											"<tt:Bitrate>%i</tt:Bitrate>\r\n"
											"<tt:SampleRate>%i</tt:SampleRate>\r\n"
											"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
										"</trt:Configurations>\r\n"
									"</trt:GetCompatibleAudioEncoderConfigurationsResponse>\r\n"
									"</s:Body>\r\n"
									"</s:Envelope>\r\n",
					(char*)&session->MicList[n].ID, (char*)&session->MicList[n].ID,
					WEB_get_AudioCodecName(session->MicList[n].CodecNum),
					(session->MicList[n].BitRate > 10) ? session->MicList[n].BitRate / 1000 : session->MicList[n].BitRate,
					session->MicList[n].Freq);		
	}
}

static void ONVIV_Template_Med_GetCompMetadataConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetCompatibleMetadataConfigurationsResponse>\r\n"
				"<trt:Configurations token=\"MetadataToken_1\">\r\n"
					"<tt:Name>MetadataConfiguration_1</tt:Name>\r\n"
					"<tt:UseCount>1</tt:UseCount>\r\n"
					"<tt:PTZStatus>\r\n"
						"<tt:Status>true</tt:Status>\r\n"
						"<tt:Position>false</tt:Position>\r\n"
					"</tt:PTZStatus>\r\n"
					"<tt:Analytics>false</tt:Analytics>\r\n"
					"<tt:SessionTimeout>PT60S</tt:SessionTimeout>\r\n"
				"</trt:Configurations>\r\n"
			"</trt:GetCompatibleMetadataConfigurationsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n");
}

static void ONVIV_Template_Med_GetCompVideoEncConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	DBG_MUTEX_LOCK(&modulelist_mutex);
	snprintf(buff, buffsize, 
			"<s:Header>\r\n"
			"</s:Header>\r\n"
			"<s:Body>\r\n"
			"<trt:GetCompatibleVideoEncoderConfigurationsResponse>\r\n"
				"<trt:Configurations token=\"V_ENC_1\">\r\n"
					"<tt:Name>V_ENC_1</tt:Name>\r\n"
					"<tt:UseCount>1</tt:UseCount>\r\n"
					"<tt:Encoding>H264</tt:Encoding>\r\n"
					"<tt:Resolution>\r\n"
						"<tt:Width>%i</tt:Width>\r\n"
						"<tt:Height>%i</tt:Height>\r\n"
					"</tt:Resolution>\r\n"
					"<tt:Quality>%i</tt:Quality>\r\n"
					"<tt:RateControl>\r\n"
						"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
						"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
						"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
					"</tt:RateControl>\r\n"
					"<tt:H264>\r\n"
						"<tt:H264Profile>%s</tt:H264Profile>\r\n"
					"</tt:H264>\r\n"
					"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
				"</trt:Configurations>\r\n"
				"<trt:Configurations token=\"V_ENC_2\">\r\n"
					"<tt:Name>V_ENC_2</tt:Name>\r\n"
					"<tt:UseCount>2</tt:UseCount>\r\n"
					"<tt:Encoding>H264</tt:Encoding>\r\n"
					"<tt:Resolution>\r\n"
						"<tt:Width>%i</tt:Width>\r\n"
						"<tt:Height>%i</tt:Height>\r\n"
					"</tt:Resolution>\r\n"
					"<tt:Quality>%i</tt:Quality>\r\n"
					"<tt:RateControl>\r\n"
						"<tt:FrameRateLimit>%i</tt:FrameRateLimit>\r\n"
						"<tt:EncodingInterval>%i</tt:EncodingInterval>\r\n"
						"<tt:BitrateLimit>%i</tt:BitrateLimit>\r\n"
					"</tt:RateControl>\r\n"
					"<tt:H264>\r\n"
						"<tt:H264Profile>%s</tt:H264Profile>\r\n"
					"</tt:H264>\r\n"
					"<tt:SessionTimeout>PT10S</tt:SessionTimeout>\r\n"
				"</trt:Configurations>\r\n"
			"</trt:GetCompatibleVideoEncoderConfigurationsResponse>\r\n"
			"</s:Body>\r\n"
			"</s:Envelope>\r\n",
			ispCameraImageSettings.MainVideo.video_width,
			ispCameraImageSettings.MainVideo.video_height,
			ispCameraImageSettings.MainVideoAvcLevel,
			ispCameraImageSettings.MainVideo.video_frame_rate,
			ispCameraImageSettings.MainVideo.video_intra_frame,
			ispCameraImageSettings.MainVideo.video_bit_rate/1000,
			GetH264ProfileName(ispCameraImageSettings.MainVideoAvcProfile),
			ispCameraImageSettings.PrevVideo.video_width,
			ispCameraImageSettings.PrevVideo.video_height,
			ispCameraImageSettings.PrevVideoAvcLevel,
			ispCameraImageSettings.PrevVideo.video_frame_rate,
			ispCameraImageSettings.PrevVideo.video_intra_frame,
			ispCameraImageSettings.PrevVideo.video_bit_rate/1000,
			GetH264ProfileName(ispCameraImageSettings.PrevVideoAvcProfile));
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
}

static void ONVIV_Template_Med_AddVideoEncConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_AddAudioEncConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_AddVideoAnalitConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_AddMetadataConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_AddPtzConfiguration(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}	

static void ONVIV_Template_Med_RemoveVideoEncConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_RemoveAudioEncConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_RemoveVideoAnalitConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_RemoveMetadataConf(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_RemovePtzConfiguration(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}	

static void ONVIV_Template_Dev_GetSystemBackup(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ActionNotSupported", "Need file settings");
}

static void ONVIV_Template_Dev_StartFirmwareUpgrade(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}	

static void ONVIV_Template_Dev_GetCertificates(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Dev_GetCertificatesStatus(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Dev_GetAccessPolicy(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Dev_SetAccessPolicy(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static void ONVIV_Template_Med_GetVideoAnaliticsConfs(char *buff, unsigned int buffsize, HTTP_HEAD *pHEAD)
{
	ONVIV_Template_Error(buff, buffsize, pHEAD, "ter:ActionNotSupported", "ActionNotSupported");
}

static char ONVIF_Respond_Get(ONVIF_SESSION *session, HTTP_HEAD *pHEAD, char *msg_tx)
{
	char *msg_head = (char*)DBG_MALLOC(2048);
	char *msg_neck = (char*)DBG_MALLOC(4096);
	char *msg_body = (char*)DBG_MALLOC(16384);
	unsigned int uiBodyLen = 0;
	memset(msg_head, 0, 2048);
	memset(msg_neck, 0, 4096);
	memset(msg_body, 0, 16384);
	
	switch(pHEAD->ContentTypeDetail.Action)
	{
		case ONVIF_ACTION_EVENT_CREATEPULLPOINTSUB: ONVIV_Template_Evnt_EventService(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_EVENT_GETEVENTPROPERTIES: ONVIV_Template_Evnt_GetEventProperties(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_EVENT_SUBSCRIBE: ONVIV_Template_Evnt_Subscribe(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_EVENT_RENEWREQUEST: ONVIV_Template_Evnt_RenewRequest(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_EVENT_UNSUBSCRIBEREQUEST: ONVIV_Template_Evnt_UnsubscribeRequest(msg_body, 16384, pHEAD); break;
		
		case ONVIF_ACTION_DEVICE_GETSYSTEMDATEANDTIME : ONVIV_Template_Dev_GetSystemDateAndTime(msg_body, 16384); break;
		case ONVIF_ACTION_DEVICE_SETSYSTEMDATEANDTIME : ONVIV_Template_Dev_SetSystemDateAndTime(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_GETSCOPES : ONVIV_Template_Dev_GetScopes(msg_body, 16384, ONVIF_COUNTRY_NAME, session->CameraModule); break;
		case ONVIF_ACTION_DEVICE_GETNETWORKINTERFACES : ONVIV_Template_Dev_GetNetworkInterfaces(msg_body, 16384); break;
		case ONVIF_ACTION_DEVICE_SETNETWORKINTERFACES : ONVIV_Template_Dev_SetNetworkInterfaces(msg_body, 16384); break;
		case ONVIF_ACTION_DEVICE_GETDEVICEINFORMATION : ONVIV_Template_Dev_GetDeviceInformation(msg_body, 16384, ONVIF_MANUFACTURER, session->CameraModule); break;
		case ONVIF_ACTION_DEVICE_GETDNS : ONVIV_Template_Dev_GetDNS(msg_body, 16384); break;
		case ONVIF_ACTION_DEVICE_GETCAPABILITIES : ONVIV_Template_Dev_GetCapabilities(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_DEVICE_GETSERVICES : ONVIV_Template_Dev_GetServices(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SYSTEMREBOOT: ONVIV_Template_Dev_SystemReboot(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETSYSTEMFACTORYDEFAULT: ONVIV_Template_Dev_SetSystemFactoryDefault(msg_body, 16384, pHEAD); break;	
		case ONVIF_ACTION_DEVICE_GETSYSTEMBACKUP: ONVIV_Template_Dev_GetSystemBackup(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_STARTFIRMWAREUPGRADE: ONVIV_Template_Dev_StartFirmwareUpgrade(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETSCOPES: ONVIV_Template_Dev_SetScopes(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETUSERS: ONVIV_Template_Dev_GetUsers(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETSERVICECAPABILITIES: ONVIV_Template_Dev_GetServiceCapabilities(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETZEROCONFIGURATION: ONVIV_Template_Dev_GetZeroConfiguration(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_SETZEROCONFIGURATION: ONVIV_Template_Dev_SetZeroConfiguration(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETCERTIFICATES: ONVIV_Template_Dev_GetCertificates(msg_body, 16384, pHEAD); break;	
		case ONVIF_ACTION_DEVICE_GETCERTIFICATESSTATUS: ONVIV_Template_Dev_GetCertificatesStatus(msg_body, 16384, pHEAD); break;	
		case ONVIF_ACTION_DEVICE_GETNTP: ONVIV_Template_Dev_GetNTP(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_SETNTP: ONVIV_Template_Dev_SetNTP(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETHOSTNAME: ONVIV_Template_Dev_GetHostName(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_SETHOSTNAME: ONVIV_Template_Dev_SetHostName(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETDISCOVERYMODE: ONVIV_Template_Dev_GetDiscoveryMode(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETNETWORKPROTOCOLS: ONVIV_Template_Dev_GetNetworkProtocols(msg_body, 16384, pHEAD, session); break;		
		case ONVIF_ACTION_DEVICE_GETNETWORKDEFAULTGATEWAY: ONVIV_Template_Dev_GetNetworkDefaultGateway(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_GETRELAYOUTPUTS: ONVIV_Template_Dev_GetRelayOutputs(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSTATE: ONVIV_Template_Dev_SetRelayOutputState(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_GETSYSTEMLOG: ONVIV_Template_Dev_GetSystemLog(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETRELAYOUTPUTSETTINGS: ONVIV_Template_Dev_SetRelayOutputSettings(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_DEVICE_GETACCESSPOLICY: ONVIV_Template_Dev_GetAccessPolicy(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETACCESSPOLICY: ONVIV_Template_Dev_SetAccessPolicy(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETNETWORKPROTOCOLS: ONVIV_Template_Dev_SetNetworkProtocols(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_SETUSER: ONVIV_Template_Dev_SetUser(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_DELETEUSERS: ONVIV_Template_Dev_DeleteUsers(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_DEVICE_CREATEUSERS: ONVIV_Template_Dev_CreateUsers(msg_body, 16384, pHEAD); break;
		
		case ONVIF_ACTION_MEDIA_GETPROFILES : ONVIV_Template_Med_GetProfiles(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETVIDEOENCODERS : ONVIV_Template_Med_GetVideoEncoders(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETAUDIOENCODERS : ONVIV_Template_Med_GetAudioEncoders(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETVIDEOSOURCES : ONVIV_Template_Med_GetVideoSources(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETSNAPSHOTURI : ONVIV_Template_Med_GetSnapshotUri(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETPROFILE: ONVIV_Template_Med_GetProfile(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETSTREAMURI: ONVIV_Template_Med_GetStreamUri(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONF : ONVIV_Template_Med_GetVideoSourceConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETVIDEOSOURCECONFS : ONVIV_Template_Med_GetVideoSourceConfs(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETVIDEOENCODERCONF : ONVIV_Template_Med_GetVideoEncoderConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETAUDIOSOURCES : ONVIV_Template_Med_GetAudioSources(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETAUDIOSOURCECONFS : ONVIV_Template_Med_GetAudioSourceConfs(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_CREATEPROFILE: ONVIV_Template_Med_CreateProfile(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_DELETEPROFILE: ONVIV_Template_Med_DeleteProfile(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETCOMPATIBLEVIDEOENCODER: ONVIV_Template_Med_GetCompatibleVideoEncoder(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETCOMPATIBLEMETADATACONF: ONVIV_Template_Med_GetCompatibleMetaDataConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_SETVIDEOENCODERCONF: ONVIV_Template_Med_SetVideoEncoderConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETCOMPVIDEOANALITCONF: ONVIV_Template_Med_GetCompVideoAnalitConf(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETCOMPAUDIOENCCONF: ONVIV_Template_Med_GetCompAudioEncConf(msg_body, 16384, pHEAD, session); break;
		case ONVIF_ACTION_MEDIA_GETCOMPMETADATACONF: ONVIV_Template_Med_GetCompMetadataConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETCOMPVIDEOENCCONF: ONVIV_Template_Med_GetCompVideoEncConf(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_MEDIA_ADDVIDEOENCCONF: ONVIV_Template_Med_AddVideoEncConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_ADDAUDIOENCCONF: ONVIV_Template_Med_AddAudioEncConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_ADDVIDEOANALITCONF: ONVIV_Template_Med_AddVideoAnalitConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_ADDMETADATACONFIGURATION: ONVIV_Template_Med_AddMetadataConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_ADDPTZCONFIGURATION: ONVIV_Template_Med_AddPtzConfiguration(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_GETVIDEOANALITICSCONFS: ONVIV_Template_Med_GetVideoAnaliticsConfs(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_REMOVEVIDEOENCCONF: ONVIV_Template_Med_RemoveVideoEncConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_REMOVEAUDIOENCCONF: ONVIV_Template_Med_RemoveAudioEncConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_REMOVEVIDEOANALITCONF: ONVIV_Template_Med_RemoveVideoAnalitConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_REMOVEMETADATACONFIGURATION: ONVIV_Template_Med_RemoveMetadataConf(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_MEDIA_REMOVEPTZCONFIGURATION: ONVIV_Template_Med_RemovePtzConfiguration(msg_body, 16384, pHEAD); break;
		
		case ONVIF_ACTION_IMAGE_GETMOVEOPTIONS: ONVIV_Template_Img_GetMoveOptions(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_IMAGE_GETIMAGINGSETTINGS: ONVIV_Template_Img_GetImagingSettings(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_IMAGE_GETOPTIONS: ONVIV_Template_Img_GetOptions(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_IMAGE_GETSTATUS: ONVIV_Template_Img_GetStatus(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_IMAGE_SETIMAGINGSETTINGS: ONVIV_Template_Img_SetImagingSettings(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_IMAGE_STOP: ONVIV_Template_Img_Stop(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_IMAGE_MOVE: ONVIV_Template_Img_Move(msg_body, 16384, pHEAD); break;
		
		case ONVIF_ACTION_PTZ_GETNODE: ONVIV_Template_Ptz_GetNode(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_GETNODES: ONVIV_Template_Ptz_GetNodes(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_GETSTATUS: ONVIV_Template_Ptz_GetStatus(msg_body, 16384, pHEAD, session->ptz_module_num); break;
		case ONVIF_ACTION_PTZ_GETPRESETS: ONVIV_Template_Ptz_GetPresets(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_GETCONFIGURATIONS: ONVIV_Template_Ptz_GetConfigurations(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_STOP: ONVIV_Template_Ptz_Stop(msg_body, 16384, pHEAD, session->CameraModule); break;
		case ONVIF_ACTION_PTZ_CONTINUOUSMOVE: ONVIV_Template_Ptz_ContinuousMove(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_RELATIVEMOVE: ONVIV_Template_Ptz_RelativeMove(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_GOTOHOMEPOSITION: ONVIV_Template_Ptz_GotoHomePosition(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_GOTOPRESET: ONVIV_Template_Ptz_GotoPreset(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_SETPRESET: ONVIV_Template_Ptz_SetPreset(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_REMOVEPRESET: ONVIV_Template_Ptz_RemovePreset(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_ABSOLUTEMOVE: ONVIV_Template_Ptz_AbsoluteMove(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_PTZ_SETHOMEPOSITION: ONVIV_Template_Ptz_SetHomePosition(msg_body, 16384, pHEAD); break;
		
		case ONVIF_ACTION_RECV_GETRECEIVERS: ONVIV_Template_Recv_GetReceivers(msg_body, 16384, pHEAD); break;
		case ONVIF_ACTION_RECV_CREATERECEIVER: ONVIV_Template_Recv_CreateReceiver(msg_body, 16384, pHEAD); break;		
		case ONVIF_ACTION_RECV_CONFIGURERECEIVER: ONVIV_Template_Recv_ConfigureReceiver(msg_body, 16384, pHEAD); break;	
		case ONVIF_ACTION_RECV_DELETERECEIVER: ONVIV_Template_Recv_DeleteReceiver(msg_body, 16384, pHEAD); break;	
		default: 
			dbgprintf(2, "No respond for ONVIF action (%i) %s\n", pHEAD->ContentTypeDetail.Action, ONVIF_GetActionName(pHEAD->ContentTypeDetail.Action));
			break;
	}
		
	ONVIV_Template_Neck(msg_neck, 4096);
	
	uiBodyLen = strlen(msg_neck) + strlen(msg_body);
	ONVIV_Template_Head(msg_head, 2048, ONVIF_SERVER_NAME, pHEAD->ContentType, uiBodyLen, pHEAD->Connection);
	
	strcpy(msg_tx, msg_head);	
	strcat(msg_tx, msg_neck);
	strcat(msg_tx, msg_body);
	
	/*if (pHEAD->ContentTypeDetail.Action == ONVIF_ACTION_MEDIA_GETPROFILES)
	{
		printf(msg_tx);
		printf("\n");
	}*/
		
	DBG_FREE(msg_head);
	DBG_FREE(msg_neck);
	DBG_FREE(msg_body);
	return 0;
}

int onvif_GetFocusPosition(int iZoomPos, int iMinFocusPos, int iMaxFocusPos)
{
	int iFocusPosition, iMaxCurrLimit, iMinCurrLimit;

	if (iZoomPos <= 1772)
		iFocusPosition = 0.00000007f*pow(iZoomPos, 3) - 0.000005f*pow(iZoomPos, 2) + 0.1126f*iZoomPos - 242.83f;
		else
		iFocusPosition = -0.0096f*pow(iZoomPos, 2) + 34.658f*iZoomPos - 30942.00f;

	if (iZoomPos > 1772)
	{
		iMaxCurrLimit = iFocusPosition + 300;
		iMinCurrLimit = iFocusPosition - 250;
	}
	else if (iZoomPos < 1544)
		{
			iMaxCurrLimit = iFocusPosition + 20;
			iMinCurrLimit = iFocusPosition - 20;
		}
		else
		{
			iMaxCurrLimit = iFocusPosition + 75;
			iMinCurrLimit = iFocusPosition - 55;
		}

	if (iMaxCurrLimit > iMaxFocusPos) iMaxCurrLimit = iMaxFocusPos;
	if (iMinCurrLimit < iMinFocusPos) iMinCurrLimit = iMinFocusPos;
	if (iFocusPosition > iMaxFocusPos) iFocusPosition = iMaxFocusPos;
	if (iFocusPosition < iMinFocusPos) iFocusPosition = iMinFocusPos;
	
	return iFocusPosition;
}

