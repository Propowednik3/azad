#ifndef _RTSP_H_
#define _RTSP_H_

#include "audio.h"
#include "debug.h"
#include <openssl/md5.h>

#define RTSP_BUF_SIZE_MAX   4096
#define RTSP_PORT      		8554
#define RTSP_BUFFER_SIZE	1024000

#define OPTIONS             "OPTIONS"
#define DESCRIBE            "DESCRIBE"
#define SETUP               "SETUP"
#define PLAY                "PLAY"
#define TEARDOWN            "TEARDOWN"
#define SET_PARAMETER		"SET_PARAMETER"
#define GET_PARAMETER		"GET_PARAMETER"
#define DOLLAR            	"$"

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];

typedef enum
{
    RTSP_MSG_OPTIONS, 
    RTSP_MSG_DESCRIBE, 
    RTSP_MSG_SETUP,
    RTSP_MSG_PLAY,
    RTSP_MSG_TEARDOWN, 
	RTSP_MSG_ERROR,
	RTSP_MSG_DOLLAR,
	RTSP_MSG_SET_PARAMETER,
	RTSP_MSG_GET_PARAMETER
} RTSP_MSG; 

typedef struct RTSP_SESSION
{
	unsigned int 	id;
	unsigned int 	ip;
	char			ip_str[16];
	unsigned short  port;  
	char			video_enabled;
	char			audio_enabled;
	unsigned int	frame_rate;
	unsigned int	set_rtp_clnt_v_port;
	unsigned int	set_rtp_clnt_a_port;
    unsigned int	set_rtp_serv_v_port;
	unsigned int	set_rtp_serv_a_port;
    unsigned short	rtp_video_port;     
	unsigned char	rtp_video_port_set;
    unsigned short	rtp_audio_port; 
	unsigned char	rtp_audio_port_set;
    unsigned int	rtsp_session_num;
    unsigned short  rtsp_port;     
    unsigned short  rtp_v_session_num;     
    unsigned short  rtp_a_session_num;     
    int				socket;
	unsigned int	flag;
	struct sockaddr_in  broadcast_addr;
	struct sockaddr_in  local_addr;
	int				broadcast;
	char			interleaved;
	unsigned short  video_channel_num;
	unsigned short  audio_channel_num;
	pthread_mutex_t mutex;
	int				socketpair[2];
	char			*OutBuffer;
	unsigned int	OutSize;
	unsigned int	OutLen;
	char			Password[64];
	char			Login[64];
	char			Auth;
	unsigned int	audio_list_cnt;
	MIC_LIST		*audio_list_dt;
	unsigned int	audio_list_pos;
	unsigned char	video_channel;
	char			force_audio_enabled;
	unsigned int	force_audio_lenght;
	char			*force_audio_packet;
} RTSP_SESSION;

typedef struct RTP_SESSION
{
    unsigned int 		type;
	unsigned int 		id;
	unsigned char       status; 
    struct sockaddr_in  peer_addr;
    struct sockaddr_in  broad_addr;
    unsigned char       sps_sent; 
    unsigned char       pps_sent; 
    unsigned char       idr_observed; 
	unsigned int 		prev_frame_num;
	unsigned int 		cur_frame_num;
	unsigned int 		cur_frame_pos;
    void               *to_rtp_instance;
	char 				broadcast_status;
	char 				broadcast_enabled;
	RTSP_SESSION		*rtsp_session;
	unsigned short		rtp_port;
	char				DateConnect[64];
	unsigned int 		sended_bytes;
	unsigned int 		recved_bytes;   
} RTP_SESSION;

typedef struct RTCP_PACKET
{
    unsigned char 		vpr;
	unsigned char 		type;
	unsigned short      len; 
    unsigned int 		ssrc;
	unsigned int 		msw_timestamp;
	unsigned int 		lsw_timestamp;
    unsigned int 		rtp_timestamp;
	unsigned int 		packet_cnt;
	unsigned int 		octet_cnt;
} RTCP_PACKET;

typedef struct RTCP_TCP_PACKET
{
    unsigned char 		magic;	
	unsigned char 		channel;
	unsigned short      len;	
	RTCP_PACKET 		body;
} RTCP_TCP_PACKET;

int RTSP_SendVideoFrame(unsigned int uiModuleID, char* cFrameBuffer, unsigned int uiFrameLen, void* StartPack);
int RTSP_SendAudioFrame(unsigned int uiModuleID, char* cFrameBuffer, unsigned int uiFrameLen);
void rtsp_start(char cVideoOn, char cAudioOn, unsigned int uiVideoFrameRate, MIC_LIST *rmlList, 
				unsigned int uiMicListCnt, unsigned int uRTSPPort, char cForceAudio);
void rtsp_stop();
int base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, unsigned char* data_out, unsigned int out_len);
void DigestCalcHA1(char * pszAlg, char * pszUserName, char * pszRealm, char * pszPassword, char * pszNonce, char * pszCNonce, HASHHEX SessionKey);
void DigestCalcResponse(HASHHEX HA1, char * pszNonce, char * pszNonceCount, char * pszCNonce, char * pszQop, 
		char * pszMethod, char * pszDigestUri, HASHHEX HEntity, HASHHEX Response);
int ItsRTSPNeed(unsigned int uiType, unsigned int iID, char cCalc);
pthread_mutex_t RTP_session_mutex;
RTP_SESSION *RTP_session;
unsigned int RTP_session_cnt;

#endif