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
#include "rtsp.h"
#include "flv-muxer.h"
#include "nal_to_rtp.h"
#include "omx_client.h"
#include "weather.h"
#include "network.h"
#include "weather_func.h"
#include "web.h"

#define CSEQ    		"CSeq: "
#define URL_MARK     	"rtsp:"
#define CLIENT_PORT 	"client_port="
#define INTERLEAVED 	"interleaved="
#define SESSION 		"Session: "
#define AUTH_MARK 		"Authorization: Digest "
#define USERNAME_MARK 	"username=\""
#define URI_MARK 		"uri=\""
#define RESPONSE_MARK 	"response=\""

#define RTSP_MAX_BAD_AUTH  		5


static pthread_t threadRTSP_income;
pthread_attr_t tattrRTSP_income;

static pthread_t threadRTSP_io;
pthread_attr_t tattrRTSP_io;
char cRTSP_init_done;
char cRTSP_init_done2;

pthread_mutex_t RTSP_mutex;

int RTP_Socket;

int Rtsp_Pair[2];
unsigned char RTSP_nonce_hex[30];
long int RTSP_nonce_time;

char *cVideoDataSPSMain;
char *cVideoDataPPSMain;
char cVideoDataFillMain;
char *cVideoDataSPSPrev;
char *cVideoDataPPSPrev;
char cVideoDataFillPrev;

int cThreadRtspStatus;

unsigned char RTSP_video_get_sps(omx_start_packet *Start_Packet, misc_buffer *m_data);
unsigned char RTSP_video_get_pps(omx_start_packet *Start_Packet, misc_buffer *m_data);
unsigned char RTSP_video_is_key_frame(misc_buffer *m_data);
char RTSP_Activate_RTP_session(RTSP_SESSION *session, char cType);
char RTSP_Teardown_RTP_session(unsigned int rtp_num);
unsigned int RTSP_get_new_rtp_session();
void * thread_RTSP_income(void* pData);
void * thread_RTSP_io(void* pData);


void CvtHex(HASH Bin, HASHHEX Hex)
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[HASHHEXLEN] = '\0';
};

void PrintRtspSession(RTSP_SESSION *sess)
{
	printf("RTSP session\n");
	printf("\t\tid:%i\n", sess->id);
	printf("\t\tip:%i\n", sess->ip);
	printf("\t\tip_str:%s\n", sess->ip_str);
	printf("\t\tport:%i\n", sess->port);  
	printf("\t\tvideo_enabled:%i\n", sess->video_enabled);
	printf("\t\taudio_enabled:%i\n", sess->audio_enabled);
	printf("\t\tframe_rate:%i\n", sess->frame_rate);
	printf("\t\tset_rtp_clnt_v_port:%i\n", sess->set_rtp_clnt_v_port);
	printf("\t\tset_rtp_clnt_a_port:%i\n", sess->set_rtp_clnt_a_port);
    printf("\t\tset_rtp_serv_v_port:%i\n", sess->set_rtp_serv_v_port);
	printf("\t\tset_rtp_serv_a_port:%i\n", sess->set_rtp_serv_a_port);
    printf("\t\trtp_video_port:%i\n", sess->rtp_video_port);     
	printf("\t\trtp_video_port_set:%i\n", sess->rtp_video_port_set);
    printf("\t\trtp_audio_port:%i\n", sess->rtp_audio_port); 
	printf("\t\trtp_audio_port_set:%i\n", sess->rtp_audio_port_set);
    printf("\t\trtsp_session_num:%i\n", sess->rtsp_session_num);
    printf("\t\trtsp_port:%i\n", sess->rtsp_port);     
    printf("\t\trtp_v_session_num:%i\n", sess->rtp_v_session_num);     
    printf("\t\trtp_a_session_num:%i\n", sess->rtp_a_session_num);     
    printf("\t\tsocket:%i\n", sess->socket);
	printf("\t\tflag:%i\n", sess->flag);
	//struct sockaddr_in  broadcast_addr;
	//struct sockaddr_in  local_addr;
	printf("\t\tbroadcastЖ%i\n", sess->broadcast);
	printf("\t\tinterleaved:%i\n", sess->interleaved);
	printf("\t\tvideo_channel_num:%i\n", sess->video_channel_num);
	printf("\t\taudio_channel_num:%i\n", sess->audio_channel_num);
	printf("\t\tOutBuffer:%i\n", (int)(intptr_t)sess->OutBuffer);
	printf("\t\tOutSize:%i\n", sess->OutSize);
	printf("\t\tOutLen:%i\n", sess->OutLen);
	printf("\t\tPassword:%s\n", sess->Password);
	printf("\t\tLogin:%s\n", sess->Login);
	printf("\t\tAuth:%i\n", sess->Auth);
	printf("\t\taudio_list_cnt:%i\n", sess->audio_list_cnt);
	printf("\t\taudio_list_pos:%i\n", sess->audio_list_pos);
	printf("\t\tvideo_channel:%i\n", sess->video_channel);
	printf("\t\tforce_audio_enabled:%i\n", sess->force_audio_enabled);
	printf("\t\tforce_audio_lenght:%i\n", sess->force_audio_lenght);
	printf("\n");
}

/* calculate H(A1) as per spec */
void DigestCalcHA1(char * pszAlg, char * pszUserName, char * pszRealm, char * pszPassword, char * pszNonce, char * pszCNonce, HASHHEX SessionKey)
{
		MD5_CTX Md5Ctx;
		HASH HA1;

		MD5_Init(&Md5Ctx);
		MD5_Update(&Md5Ctx, pszUserName, strlen(pszUserName));
		MD5_Update(&Md5Ctx, ":", 1);
		MD5_Update(&Md5Ctx, pszRealm, strlen(pszRealm));
		MD5_Update(&Md5Ctx, ":", 1);
		MD5_Update(&Md5Ctx, pszPassword, strlen(pszPassword));
		MD5_Final((unsigned char*)HA1, &Md5Ctx);
		if (strcasecmp (pszAlg, "md5-sess") == 0) 
		{
			MD5_Init(&Md5Ctx);
            MD5_Update(&Md5Ctx, HA1, HASHLEN);
            MD5_Update(&Md5Ctx, ":", 1);
            MD5_Update(&Md5Ctx, pszNonce, strlen(pszNonce));
            MD5_Update(&Md5Ctx, ":", 1);
            MD5_Update(&Md5Ctx, pszCNonce, strlen(pszCNonce));
            MD5_Final((unsigned char*)HA1, &Md5Ctx);
		};
		CvtHex(HA1, SessionKey);	
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    HASHHEX HA1,           /* H(A1) */
    char * pszNonce,       /* nonce from server */
    char * pszNonceCount,  /* 8 hex digits */
    char * pszCNonce,      /* client nonce */
    char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    char * pszMethod,      /* method from the request */
    char * pszDigestUri,   /* requested URL */
    HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    HASHHEX Response      /* request-digest or response-digest */
    )
{
		MD5_CTX Md5Ctx;
		HASH HA2;
		HASH RespHash;
		HASHHEX HA2Hex;

		// calculate H(A2)
		MD5_Init(&Md5Ctx);
		MD5_Update(&Md5Ctx, pszMethod, strlen(pszMethod));
		MD5_Update(&Md5Ctx, ":", 1);
		MD5_Update(&Md5Ctx, pszDigestUri, strlen(pszDigestUri));
		if ((pszQop) && (strcasecmp (pszQop, "auth-int") == 0))
		{
            MD5_Update(&Md5Ctx, ":", 1);
            MD5_Update(&Md5Ctx, HEntity, HASHHEXLEN);
		};
		MD5_Final((unsigned char*)HA2, &Md5Ctx);
		CvtHex(HA2, HA2Hex);
		// calculate response
		MD5_Init(&Md5Ctx);
		MD5_Update(&Md5Ctx, HA1, HASHHEXLEN);
		MD5_Update(&Md5Ctx, ":", 1);
		MD5_Update(&Md5Ctx, pszNonce, strlen(pszNonce));
		MD5_Update(&Md5Ctx, ":", 1);
		if (pszQop) 
		{
			MD5_Update(&Md5Ctx, pszNonceCount, strlen(pszNonceCount));
			MD5_Update(&Md5Ctx, ":", 1);
			MD5_Update(&Md5Ctx, pszCNonce, strlen(pszCNonce));
			MD5_Update(&Md5Ctx, ":", 1);
			MD5_Update(&Md5Ctx, pszQop, strlen(pszQop));
			MD5_Update(&Md5Ctx, ":", 1);
		};
		MD5_Update(&Md5Ctx, HA2Hex, HASHHEXLEN);
		MD5_Final((unsigned char*)RespHash, &Md5Ctx);
		CvtHex(RespHash, Response);
};

static inline bool is_base64(unsigned char c) 
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

int base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, unsigned char* data_out, unsigned int out_len) 
{
	char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i = 0;
	int j = 0;
	int ret = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	memset(data_out, 0, out_len);
	
	while (in_len--) 
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) 
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; i <4; i++)
			{
				data_out[ret] = base64_chars[char_array_4[i]];
				ret++;
			}
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++) char_array_3[j] = '\0';

		char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] =   char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++) 
		{
			data_out[ret] = base64_chars[char_array_4[j]];
			ret++;
		}

		while((i++ < 3))
		{
			data_out[ret] = '=';
			ret++;
		}
	}
	return ret;
}

int base64_decode(char *encoded_string, unsigned char* data_out, unsigned int out_len) 
{
	char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int len_chars = strlen(base64_chars);
	memset(data_out, 0, out_len);
	int in_len = strlen(encoded_string);
	int i = 0;
	int j = 0;
	int in_ = 0;
	int k;
	int res_len = 0;
			  
	unsigned char char_array_4[4], char_array_3[3];

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) 
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4)
		{
			for (i = 0; i <4; i++)
			{
				for (k = 0; k < len_chars; k++) if (base64_chars[k] == char_array_4[i]) {char_array_4[i] = k; break;}

				char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

				for (i = 0; (i < 3); i++) 
				{
					data_out[res_len] = char_array_3[i];
					res_len++;
				}
				i = 0;
			}
		}

		if (i) 
		{
			for (j = i; j <4; j++)  char_array_4[j] = 0;

			for (j = 0; j <4; j++)  
				for (k = 0; k < len_chars; k++) if (base64_chars[k] == char_array_4[i]) {char_array_4[i] = k; break;}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (j = 0; (j < i - 1); j++) 
			{
				data_out[res_len] = char_array_3[j];
				res_len++;
			}
		}
	}

	return res_len;
}
	
char RTSP_GetSamplingNum(unsigned int SamplingFrequencie)
{
	char ret = 0;
	switch (SamplingFrequencie)
	{
		case 96000:
			ret = 0;
			break;
		case 88200:
			ret = 1;
			break;
		case 64000:
			ret = 2;
			break;
		case 48000:
			ret = 3;
			break;
		case 44100:
			ret = 4;
			break;
		case 32000:
			ret = 5;
			break;
		case 24000:
			ret = 6;
			break;
		case 22050:
			ret = 7;
			break;
		case 16000:
			ret = 8;
			break;
		case 12000:
			ret = 9;
			break;
		case 11025:
			ret = 10;
			break;
		case 8000:
			ret = 11;
			break;
		case 7350:
			ret = 12;
			break;
		default:
			ret = 15;
			break;
	}
	return ret;
}

unsigned int RTSP_AudioSpecificConfig(char AudioObjectType, unsigned int SamplingFrequencie, char Channels)
{
	unsigned int ret = 0;
	ret = AudioObjectType & 31;
	ret = ret << 4;
	ret |= RTSP_GetSamplingNum(SamplingFrequencie);
	ret = ret << 4;
	ret |= Channels & 15;
	ret = ret << 3;
	return ret;	
}

static unsigned int RTSP_get_msg_type(char *msg_rx)
{
	//printf("RTSP_get_msg_type %s\n", msg_rx);
    if(strncmp(msg_rx, OPTIONS, strlen(OPTIONS)) == 0)
    {
        return RTSP_MSG_OPTIONS; 
    }
    else if(strncmp(msg_rx, DESCRIBE, strlen(DESCRIBE)) == 0)
    {
        return RTSP_MSG_DESCRIBE; 
    }
    else if(strncmp(msg_rx, SETUP, strlen(SETUP)) == 0)
    {
        return RTSP_MSG_SETUP; 
    }
    else if(strncmp(msg_rx, PLAY, strlen(PLAY)) == 0)
    {
        return RTSP_MSG_PLAY; 
    }
    else if(strncmp(msg_rx, TEARDOWN, strlen(TEARDOWN)) == 0)
    {
        return RTSP_MSG_TEARDOWN; 
    }
	else if(strncmp(msg_rx, DOLLAR, strlen(DOLLAR)) == 0)
    {
        return RTSP_MSG_DOLLAR; 
    }
	else if(strncmp(msg_rx, GET_PARAMETER, strlen(GET_PARAMETER)) == 0)
    {
        return RTSP_MSG_GET_PARAMETER; 
    }
	else if(strncmp(msg_rx, SET_PARAMETER, strlen(SET_PARAMETER)) == 0)
    {
        return RTSP_MSG_SET_PARAMETER; 
    }
    else
    {
		return RTSP_MSG_ERROR;
    }
}

unsigned int CalcCRC_1(char *buffer, unsigned int iLength)
{
	DBG_LOG_IN();
	
	unsigned int		CRC = 0;
	unsigned int		CRC2 = 0;
	unsigned int		Len;
	unsigned int		Arr[256];
	
	
	memset(Arr, 0, sizeof(Arr));
	for (Len = 0;Len != iLength; Len++)
	{		
		Arr[(unsigned char)buffer[Len]]++;
		CRC += (unsigned char)buffer[Len];
	}
	
	iLength = 256;
	for (Len = 0;Len != iLength; Len++)
	{
		CRC2 += Arr[Len];
	}
	
	DBG_LOG_OUT();	
	return (CRC ^ CRC2);
}

unsigned int CalcCRC_2(unsigned char *buffer, unsigned int iLength)
{
	DBG_LOG_IN();
	
	unsigned int		CRC = 0;
	unsigned char		*pCRC = (unsigned char*)&CRC;
	unsigned int		n, i;
	
	
	i = 0;
	for (n = 0; n != iLength; n++)
	{		
		pCRC[i] ^= buffer[n];
		i++;
		if (i == 4) i = 0;
	}
	
	DBG_LOG_OUT();	
	return CRC;
}

static int get_cseq(char *msg)
{
    char *temp = strstr(msg, CSEQ); 
	if (temp == NULL) return -1;
    temp += strlen(CSEQ); 
	
	char buff[10];
	memset(buff, 0, 10);
	if (strlen(temp) < 10) strcpy(buff, temp); else memcpy(buff, temp, 9);
	int n;
	int m = strlen(buff);
	for (n = 0; n < m; n++) if (buff[n] < 32) buff[n] = 0;
    m = Str2Int(buff);
	//printf("get_cseq %i\n", m);
    return m; 
}

static char * get_url(char   *msg, char *str)
{
    char *start = strstr(msg, URL_MARK);
    char *end = strstr(start, " ");
	if (start == NULL) return 0;
    if (end == NULL) return 0;
   
	memset(str, 0, 1024);
    
    memcpy(str, start, end-start); 
    str[end - start] = 0; 
    
    return str; 
}

static char RTSP_options_respond(char *msg_rx, char *msg_tx)
{
    int cseq = get_cseq(msg_rx); 
	if (cseq == -1)
	{
		dbgprintf(2, "RTSP_options_respond: Error get_cseq\n");
		return 0;
	}
	memset(msg_tx, 0, RTSP_BUF_SIZE_MAX);    
    snprintf(msg_tx, 
             RTSP_BUF_SIZE_MAX, 
             "RTSP/1.0 200 OK\r\n"
             //"Server: DSS/5.5.5 (Build/489.16; Platform/Linux; Release/Darwin; state/beta; )\r\n"
			 "CSeq: %d\r\n"
             "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n"
			// "Supported: play.basic, con.persistent\r\n\r\n",
			"\r\n",
             cseq); 

    return 1; 
}

static char RTSP_parameter_respond(RTSP_SESSION *session, char *msg_rx, char *msg_tx)
{
	int cseq = get_cseq(msg_rx); 
	if (cseq == -1)
	{
		dbgprintf(2, "RTSP_options_respond: Error get_cseq\n");
		return 0;
	}
	
	memset(msg_tx, 0, RTSP_BUF_SIZE_MAX);    
    snprintf(msg_tx, RTSP_BUF_SIZE_MAX,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"User-Agent: LibVLC/2.1.4 (LIVE555 Streaming Media v2014.01.21)\r\n"
		"Content-Length: 0\r\n"
		"Cache-Control: no-cache\r\n"
		"Session: %i\r\n\r\n",
		//,session->ip_str, session->rtsp_port, 
		cseq, session->rtsp_session_num);
		
	return 1;
}

unsigned int RTSP_get_profile_id(unsigned int AVC_Profile, unsigned int AVC_Level)
{
	unsigned int res = 0;
	switch(AVC_Profile)
	{
		case OMX_VIDEO_AVCProfileUnknown:
			res |= 0x424000;
			break;
		case OMX_VIDEO_AVCProfileBaseline:
			res |= 0x420000;
			break;
		case OMX_VIDEO_AVCProfileMain:
			res |= 0x4D0000;
			break;
		case OMX_VIDEO_AVCProfileExtended:
			res |= 0x580000;
			break;
		case OMX_VIDEO_AVCProfileHigh:
			res |= 0x640000;
			break;
		case OMX_VIDEO_AVCProfileHigh10:
			res |= 0x6E0000;
			break;
		case OMX_VIDEO_AVCProfileHigh422:
			res |= 0x7A0000;
			break;
		case OMX_VIDEO_AVCProfileHigh444:
			res |= 0xF40000;
			break;
		default:
			res |= 0x424000;
			break;
	}	
	switch(AVC_Level)
	{
		case OMX_VIDEO_AVCLevelUnknown:
			res |= 0x00000B;
			break;
		case OMX_VIDEO_AVCLevel1:
			res |= 0x00000A;
			break;
		case OMX_VIDEO_AVCLevel1b:
			res |= 0x00000B;
			break;
		case OMX_VIDEO_AVCLevel11:
			res |= 0x00000B;
			break;
		case OMX_VIDEO_AVCLevel12:
			res |= 0x00000C;
			break;
		case OMX_VIDEO_AVCLevel13:
			res |= 0x00000D;
			break;
		case OMX_VIDEO_AVCLevel2:
			res |= 0x000014;
			break;
		case OMX_VIDEO_AVCLevel21:
			res |= 0x000015;
			break;
		case OMX_VIDEO_AVCLevel22:
			res |= 0x000016;
			break;
		case OMX_VIDEO_AVCLevel3:
			res |= 0x00001E;
			break;
		case OMX_VIDEO_AVCLevel31:
			res |= 0x00001F;
			break;
		case OMX_VIDEO_AVCLevel32:
			res |= 0x000020;
			break;
		case OMX_VIDEO_AVCLevel4:
			res |= 0x000028;
			break;
		case OMX_VIDEO_AVCLevel41:
			res |= 0x000029;
			break;
		case OMX_VIDEO_AVCLevel42:
			res |= 0x00002A;
			break;
		case OMX_VIDEO_AVCLevel5:
			res |= 0x000032;
			break;
		case OMX_VIDEO_AVCLevel51:
			res |= 0x000033;
			break;
		default:
			res |= 0x00000B;
			break;
	}
	return res;
}

char RTSP_Auth(RTSP_SESSION *session, char *msg_rx)
{
	char *auth = strstr(msg_rx, AUTH_MARK);
	if (auth == NULL) {dbgprintf(4, "RTSP: Not find AUTH_MARK\n"); return 0;}
	auth += strlen(AUTH_MARK);
    char *uri_start = strstr(auth, URI_MARK);
	if (uri_start == NULL) {dbgprintf(4, "RTSP: Not find URI_MARK\n"); return 0;}
	uri_start += strlen(URI_MARK);
    char *uri_end = strstr(uri_start, "\"");
	if (uri_end == NULL) {dbgprintf(4, "RTSP: Not find end URI_MARK\n"); return 0;}
	int urilen = uri_end - uri_start;
	if (urilen >= 128) {dbgprintf(4, "RTSP: Big len URI\n"); return 0;}
	char uri[128];
	memset(uri, 0, 128);
    memcpy(uri, uri_start, urilen);
	
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
	if (resp_start == NULL) {dbgprintf(4, "RTSP: Not find RESPONSE_MARK\n"); return 0;}
	resp_start += strlen(RESPONSE_MARK);
    char *resp_end = strstr(resp_start, "\"");
	if (resp_end == NULL) {dbgprintf(4, "RTSP: Not find end RESPONSE_MARK\n"); return 0;}
	int resplen = resp_end - resp_start;
	if (resplen >= 128) {dbgprintf(4, "RTSP: Big len RESPONSE\n"); return 0;}
	char resp[128];
	memset(resp, 0, 128);
    memcpy(resp, resp_start, resplen);
	
	char nonce_hex[30];
	memset(nonce_hex, 0, 30);
	struct timespec nanotime;
	long int now_time, nonce_time;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	now_time = nanotime.tv_sec;
	
	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	memcpy(nonce_hex, RTSP_nonce_hex, 24);
	nonce_time = RTSP_nonce_time;	
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	
	if (((now_time - 10) > nonce_time) || ((now_time + 10) < nonce_time))
	{
		dbgprintf(4, "RTSP_Auth: nonce timeout\n", nonce_time, now_time); 
		return 0;
	}
		
	DBG_MUTEX_LOCK(&user_mutex);
	unsigned int n, ret = 0;	
	for (n = 0; n < iUserCnt; n++)
	{
		if ((uiUserList[n].Enabled) && (uiUserList[n].Access & ACCESS_RTSP) && (SearchStrInDataCaseIgn(uiUserList[n].Login, strlen(uiUserList[n].Login), 0, username)))
		{
			memset(session->Login, 0, 64);
			memset(session->Password, 0, 64);
			strcpy(session->Login, uiUserList[n].Login);
			strcpy(session->Password, uiUserList[n].Password);
			ret = 1;
			break;
		}
	}	
	DBG_MUTEX_UNLOCK(&user_mutex);
	if (ret != 1) 
	{
		dbg_printf(2, "RTSP_Auth: wrong username '%s'\n", username);		
		return -1;
	}
	
	HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;

    DigestCalcHA1("md5", session->Login, "Streaming Server", session->Password, nonce_hex, "0a4f113b", HA1);
    DigestCalcResponse(HA1, nonce_hex, "00000001", "0a4f113b", NULL, "DESCRIBE", uri, HA2, Response);
    //printf("RESPONSE SERV:'%s'\n", Response);
	if (strcmp(Response, resp) == 0) return 1; 
	else	
	{
		dbg_printf(2, "RTSP_Auth: wrong password\n");
		return -1;
	}
}

static char RTSP_describe_respond(RTSP_SESSION *session, char *msg_rx, char *msg_tx, char *cAVCsps, char *cAVCpps)
{
	//printf("RTSP_describe_respond\n");
	char    sdp_main[2048]; 
	char    url[1024]; 
	memset(sdp_main,0,2048);
	strcpy(sdp_main, msg_rx);
	int i;
	
	for (i = 0; i < 2048; i++) 
	{
		if (sdp_main[i] < 32)
		{
			sdp_main[i] = 0;
			UpperTextLimit(sdp_main, i);
			if ((session->audio_enabled == 1) && (SearchData("VIDEO", 3, sdp_main, i, 0) > 0) && (SearchData("AUDIO", 3, sdp_main, i, 0) < 0)) session->audio_enabled = 0;
			if ((session->video_enabled == 1) && (SearchData("VIDEO", 3, sdp_main, i, 0) > 0))
			{
				
				int n = SearchData("VIDEO", 3, sdp_main, i, 0) + 5;
				if ((sdp_main[n] > 47) && (sdp_main[n] < 58)) session->video_channel = sdp_main[n] - 48;
				if (session->video_channel > 1) session->video_channel = 0;
			}
			if ((session->video_enabled == 1) && (SearchData("AUDIO", 3, sdp_main, i, 0) > 0) && (SearchData("VIDEO", 3, sdp_main, i, 0) < 0)) session->video_enabled = 0;
			if ((session->audio_enabled == 1) && (SearchData("AUDIO", 3, sdp_main, i, 0) > 0))
			{
				
				int n = SearchData("AUDIO", 3, sdp_main, i, 0) + 5;
				if ((sdp_main[n] > 47) && (sdp_main[n] < 58)) session->audio_list_pos = sdp_main[n] - 48;
				if (session->audio_list_pos >= session->audio_list_cnt) session->audio_list_pos = 0;
			}
			break;
		}
	}
	if ((session->audio_enabled == 0) && (session->video_enabled == 0))
	{
		dbgprintf(4, "RTSP_describe_respond: No content for translate (%s)\n", sdp_main);
		return 0;
	}
	
	if ((session->audio_enabled == 0) && session->force_audio_enabled && session->force_audio_lenght) session->audio_enabled = 2;
					
	char   *sdp_main_str = 
				  "v=0\r\n"
                  "o=- %i %i IN IP4 %s\r\n"
                  "s=pi_encode.264\r\n"
                  "c=IN IP4 0.0.0.0\r\n"
				  "t=0 0\r\n"                  
				  "a=sdplang:en\r\n"
				  "a=range:npt=0-\r\n"
                  "a=control:*\r\n";
    char   *sdp_video_str = "m=video %i RTP/AVP %i\r\n"
							"a=rtpmap:%i H264/90000\r\n"
							"a=fmtp:%i packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s,%s\r\n"
							"a=framerate:%i.0\r\n"
							"a=control:track0\r\n"
							"b=AS:5000\r\n"
							"b=RS:0\r\n" 
							"b=RR:0\r\n";
	char   *sdp_audio_str = "m=audio %i RTP/AVP %i\r\n"
							"a=rtpmap:%i %s/%i/%i\r\n"
							"a=fmtp:%i %s\r\n"
							"a=control:track1\r\n"
							"b=AS:128\r\n"
							"b=RS:0\r\n" 
							"b=RR:0\r\n";       
	
	char sdp_audio_aac[256];
	memset(sdp_audio_aac, 0, 256);
	
	char *aud_codec_name = NULL;
	
	if ((session->audio_enabled == 1) && session->audio_list_cnt)
	{
		switch(session->audio_list_dt[session->audio_list_pos].CodecNum)
		{
			case 0: 
				{
					aud_codec_name = "mpeg4-generic"; 
					char sdp_audio[1024]; 
					memset(sdp_audio, 0, 1024);
					unsigned int ret = RTSP_AudioSpecificConfig(1 /*AAC MAIN*/, session->audio_list_dt[session->audio_list_pos].Freq, 
																				session->audio_list_dt[session->audio_list_pos].Channels);
					char cAudioConfig[10];
					memset(cAudioConfig, 0, 10);
					Char2Hex(cAudioConfig, (unsigned char*)&ret, 2, 1);
			
					sprintf(sdp_audio_aac, "streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s; sizeLength=13; indexLength=3; indexDeltaLength=3; Profile=1", cAudioConfig);
				}
				break;
			case 1: aud_codec_name = "mpa"; break;
			case 2: aud_codec_name = "speex"; break;
			case 3: aud_codec_name = "L16"; break;
			case 4: aud_codec_name = "DVI4"; break;
			case 5: aud_codec_name = "RED"; break;
			case 6: aud_codec_name = "MPA"; break;
			case 7: aud_codec_name = "RED"; break;
			case 8: aud_codec_name = "RED"; break;
			case 9: aud_codec_name = "RED"; break;
			case 10: aud_codec_name = "RED"; break;
			case 11: aud_codec_name = "RED"; break;
			case 12: aud_codec_name = "MPA"; break;
			case 13: aud_codec_name = "MP2T"; break;
			case 14: aud_codec_name = "ac3"; break;
			case 15: aud_codec_name = "RED"; break;
			case 16: aud_codec_name = "vorbis"; break;
			case 17: aud_codec_name = "RED"; break;			
			case 18: aud_codec_name = "ac3"; break;			
			default: aud_codec_name = "RED"; break;
		}
	}
	
	if (session->audio_enabled == 2)
	{
		aud_codec_name = "mpeg4-generic"; 
		char sdp_audio[1024]; 
		memset(sdp_audio, 0, 1024);
		unsigned int ret = RTSP_AudioSpecificConfig(1 /*AAC MAIN*/, 44100, 1);
		char cAudioConfig[10];
		memset(cAudioConfig, 0, 10);
		Char2Hex(cAudioConfig, (unsigned char*)&ret, 2, 1);
		sprintf(sdp_audio_aac, "streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s; sizeLength=13; indexLength=3; indexDeltaLength=3; Profile=1", cAudioConfig);
	}
	
	memset(sdp_main,0,2048);
    sprintf(sdp_main, sdp_main_str, session->rtsp_session_num, session->rtsp_session_num, session->ip_str); 
	
	if (session->video_enabled == 1) 
	{
		unsigned int ret = RTSP_get_profile_id(OMX_AVC_PROFILE, OMX_AVC_LEVEL);
		char cAVCConfig[10];
		memset(cAVCConfig, 0, 10);
		Char2Hex(cAVCConfig, (unsigned char*)&ret, 3, 1);
		char    sdp_video[1024]; 
		memset(sdp_video,0,1024);
		sprintf(sdp_video, sdp_video_str, session->set_rtp_clnt_v_port, session->video_channel_num, session->video_channel_num, 
						session->video_channel_num, cAVCConfig, cAVCsps, cAVCpps, session->frame_rate); 
		strcat(sdp_main, sdp_video);
	}
    if (session->audio_enabled == 1) 
	{
		char sdp_audio[1024]; 
		memset(sdp_audio, 0, 1024);
		sprintf(sdp_audio, sdp_audio_str, session->set_rtp_clnt_a_port, session->audio_channel_num, 
						session->audio_channel_num, aud_codec_name, session->audio_list_dt[session->audio_list_pos].Freq, 
						session->audio_list_dt[session->audio_list_pos].Channels, session->audio_channel_num, sdp_audio_aac); 
		strcat(sdp_main, sdp_audio);			
	}
	if (session->audio_enabled == 2) 
	{
		char sdp_audio[1024]; 
		memset(sdp_audio, 0, 1024);
		sprintf(sdp_audio, sdp_audio_str, session->set_rtp_clnt_a_port, session->audio_channel_num, 
						session->audio_channel_num, aud_codec_name, 44100, 1, session->audio_channel_num, sdp_audio_aac); 
		strcat(sdp_main, sdp_audio);			
	}
	
    int cseq = get_cseq(msg_rx); 
	if (cseq == -1)
	{
		dbgprintf(2, "RTSP_describe_respond: Error get_cseq\n");
		//printf("%s\n",msg_rx);
		return 0;
	}
	if (get_url(msg_rx, url) == 0)
	{
		dbgprintf(2, "RTSP_describe_respond: Error get_url\n");
		return 0;
	}
	
	memset(msg_tx, 0, RTSP_BUF_SIZE_MAX); 
	snprintf(msg_tx, 
             RTSP_BUF_SIZE_MAX, 
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "Server: AZ daemon\r\n"
			 "Cache-Control: no-cache\r\n"
			 "Content-Base: %s\r\n"
             "Content-Type: application/sdp\r\n"
             "Content-Length: %i\r\n"
             "Session: %i;timeout=60\r\n"
			 "\r\n"
			 "%s",
             cseq, 
             url,
             (int)strlen(sdp_main),
			 session->rtsp_session_num,
             sdp_main);
	return 1; 
}

static char RTSP_describe_respond_401(RTSP_SESSION *session, char *msg_rx, char *msg_tx)
{
    int cseq = get_cseq(msg_rx); 
	if (cseq == -1)
	{
		dbgprintf(2, "RTSP_describe_respond_401: Error get_cseq\n");
		return 0;
	}

	char nonce[30];
	char nonce_hex[30];
	memset(nonce, 0, 30);
	memset(nonce_hex, 0, 30);
	
	int n;
	for (n = 0; n < 12; n++) nonce[n] = GenRandomInt(255);
	Char2Hex(nonce_hex, (unsigned char *)nonce, 12, 0);
	LowerText(nonce_hex);
	
	memset(msg_tx, 0, RTSP_BUF_SIZE_MAX); 
	snprintf(msg_tx, 
             RTSP_BUF_SIZE_MAX, 
				"RTSP/1.0 401 Unauthorized\r\n"
				"CSeq: %d\r\n"
				"Server: AZ daemon\r\n"
				"WWW-Authenticate: Digest realm=\"Streaming Server\", nonce=\"%s\"\r\n\r\n",
				cseq, nonce_hex);
	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	memcpy(RTSP_nonce_hex, nonce_hex, 24);
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	RTSP_nonce_time = nanotime.tv_sec;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	return 1; 
}

static unsigned short get_interleaved(char   *msg)
{
    char *temp = strstr(msg, INTERLEAVED); 
	if (temp == NULL) return 1000;
    
	temp += strlen(INTERLEAVED);

    int port = 0; 
    do
    {
        // 64346 
        port *= 10; 
        port += *temp - '0'; 
        temp++; 
    } while (*temp != '-'); 
    return port; 
}

static unsigned short get_client_port(char   *msg)
{
    char *temp = strstr(msg, CLIENT_PORT); 
	if (temp == NULL) return 0;
    
	temp += strlen(CLIENT_PORT);

    int port = 0; 
    do
    {
        // 64346 
        port *= 10; 
        port += *temp - '0'; 
        temp++; 
    } while (*temp != '-'); 
    return port; 
}

/*static char get_session(char   *msg, char *session)
{
    char *temp = strstr(msg, SESSION);
    if (temp == NULL) return 0;
   
	memset(session, 0, 16);
    temp += strlen(SESSION); 

    memcpy(session, temp, 8); 
    session[8] = 0; 

    return 1; 
}*/

static char RTSP_setup_respond(RTSP_SESSION *session, char *msg_rx, char *msg_tx)
{
	//printf("RTSP_setup_respond\n");
	//printf(">>>>>>>>>> \n%s\n",msg_rx);
	int cseq = get_cseq(msg_rx); 
	if (cseq == -1)
	{
		dbgprintf(2, "RTSP_setup_respond: Error get_cseq\n");
		return 0;
	}
    unsigned short dst_port;
	unsigned short serv_port;
	//unsigned short channelnum = 0;
	
	if (SearchData("TCP", 3, msg_rx, strlen(msg_rx), 0) > 0) session->interleaved = 1;
	
	if ((session->audio_enabled) && (session->rtp_audio_port_set == 0))	 
	{
		if ((session->video_enabled == 0) || (session->rtp_video_port_set != 0))
		{
			if (session->interleaved)
			{
				//channelnum = session->video_channel_num;
				dst_port = get_interleaved(msg_rx);
				if (dst_port == 1000)
				{				
					dbgprintf(2, "RTSP_setup_respond: Error get_interleaved\n");
					return 0;
				}	
			}
			else
			{
				serv_port = session->set_rtp_serv_a_port;
				dst_port = get_client_port(msg_rx); 
				if (dst_port == 0)
				{				
					dbgprintf(2, "RTSP_setup_respond: Error get_client_port\n");
					return 0;
				}			
			}
			session->rtp_audio_port = dst_port; 
			session->rtp_audio_port_set = 1;
		} 
	}
	if ((session->video_enabled == 1) && (session->rtp_video_port_set == 0))
	{
		if (session->interleaved)
		{
			//channelnum = session->video_channel_num;
			dst_port = get_interleaved(msg_rx); 
			if (dst_port == 1000)
			{				
				dbgprintf(2, "RTSP_setup_respond: Error get_interleaved\n");
				return 0;
			}	
		}
		else
		{
			serv_port = session->set_rtp_serv_v_port;
			dst_port = get_client_port(msg_rx); 
			if (dst_port == 0)
			{
				dbgprintf(2, "RTSP_setup_respond: Error get_client_port\n");
				return 0;
			}
		}
		session->rtp_video_port = dst_port; 	
		session->rtp_video_port_set = 1; 	
	} 	
	
	memset(msg_tx, 0, RTSP_BUF_SIZE_MAX); 
    if (session->interleaved) 
	{
		snprintf(msg_tx, 
            RTSP_BUF_SIZE_MAX, 
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d;\r\n"
            "Session: %i\r\n\r\n",
            cseq, 
            dst_port,
            dst_port+1, 
			//channelnum,
            session->rtsp_session_num);
	}
	else
	{
		snprintf(msg_tx, 
            RTSP_BUF_SIZE_MAX, 
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n"
            "Session: %i\r\n\r\n",
            cseq, 
            session->ip_str, 
            inet_ntoa(session->local_addr.sin_addr), 
            dst_port,
            dst_port+1, 
            serv_port, 
            serv_port+1, 
            session->rtsp_session_num);
	}
	//printf("port src:%i, port1:%i, port2:%i\n",session->port, session->rtp_video_port, session->rtp_audio_port);
	//printf("<<<<<<<<<<<< \n%s\n",msg_tx);
    return 1; 
}

static char RTSP_play_respond(RTSP_SESSION *session, char *msg_rx, char *msg_tx)
{
    int cseq = get_cseq(msg_rx);
	if (cseq == -1)
	{
		dbgprintf(2, "RTSP_play_respond: Error get_cseq\n");
		return 0;
	}	
	/*char ses[16];
     
	if (get_session(msg_rx, ses) == 0)
	{
		dbgprintf(2, "RTSP_play_respond: Error get_session\n");
		return 0;
	}	*/
	//printf("Getted %s my %i\n",ses, session->rtsp_session_num);
	char url[1024];
	char url1[1024];
	char url2[1024];
	memset(url, 0, 1024);
	memset(url1, 0, 1024);
	memset(url2, 0, 1024);
	strcpy(url, "RTP-Info: ");
	if (session->video_enabled == 1) 
	{
		sprintf(url1, "url=rtsp://%s:%i/media/trackID=0;seq=1;rtptime=0", inet_ntoa(session->local_addr.sin_addr), session->rtsp_port); 
		if (session->audio_enabled == 1) strcat(url1, ", ");
	}
	if (session->audio_enabled == 1)
		sprintf(url2, "url=rtsp://%s:%i/media/trackID=1;seq=1;rtptime=0", inet_ntoa(session->local_addr.sin_addr), session->rtsp_port); 	
	strcat(url, url1);
	strcat(url, url2);
	//strcat(url, "\r\n");
	
    memset(msg_tx, 0, RTSP_BUF_SIZE_MAX); 
	// Fill in the response:
    snprintf(msg_tx, RTSP_BUF_SIZE_MAX,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "%s"
            "Session: %i\r\n"
            "%s\r\n\r\n",
            cseq, 
            "Range: npt=0.000-\r\n", 
            session->rtsp_session_num,
			url);
	//printf("%s\n",msg_tx);
    return 1; 
}

static char RTSP_teardown_respond(char *msg_rx, char *msg_tx)
{
    int cseq = get_cseq(msg_rx); 
	if (cseq == -1)
	{
		//printf("%s\n", msg_rx);
		dbgprintf(2, "RTSP_teardown_respond: Error get_cseq\n");
		//return 0;
	}	
    memset(msg_tx, 0, RTSP_BUF_SIZE_MAX); 
	// Fill in the response:
    snprintf(msg_tx, RTSP_BUF_SIZE_MAX,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "\r\n",
            cseq); 

    return 1; 
}

char RTSP_Activate_RTP_session(RTSP_SESSION *session, char cType)
{	
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	unsigned int rtp_num = RTSP_get_new_rtp_session();
	
	RTP_session[rtp_num].status = 1;
	RTP_session[rtp_num].rtsp_session = session;
	RTP_session[rtp_num].type = cType;
	if (cType == 0)
		RTP_session[rtp_num].rtp_port = session->rtp_video_port;
		else
		RTP_session[rtp_num].rtp_port = session->rtp_audio_port;
	RTP_session[rtp_num].id = session->id;
	RTP_session[rtp_num].peer_addr.sin_family       = AF_INET;
	RTP_session[rtp_num].peer_addr.sin_addr.s_addr  = session->ip;
	memset(&RTP_session[rtp_num].peer_addr.sin_zero, 0, 8);
	memcpy(&RTP_session[rtp_num].broad_addr, &session->broadcast_addr, sizeof(struct sockaddr_in));
	RTP_session[rtp_num].idr_observed   = 0;
	RTP_session[rtp_num].pps_sent       = 0;
	RTP_session[rtp_num].sps_sent       = 0;
	RTP_session[rtp_num].broadcast_enabled = session->broadcast;
	if (cType == 0) 
	{
		session->rtp_v_session_num = rtp_num;
		RTP_session[rtp_num].peer_addr.sin_port = htons(session->rtp_video_port);
		RTP_session[rtp_num].to_rtp_instance = 	sx_nal_to_rtp_util_create(0, session->video_channel_num, session->frame_rate);
	}
	else
	{
		RTP_session[rtp_num].idr_observed = 1;
		session->rtp_a_session_num = rtp_num;
		RTP_session[rtp_num].peer_addr.sin_port = htons(session->rtp_audio_port);
		RTP_session[rtp_num].to_rtp_instance = 	sx_nal_to_rtp_util_create(cType, session->audio_channel_num, 0);
	}    
    GetCurrDateTimeStr(RTP_session[rtp_num].DateConnect, 64);
	RTP_session[rtp_num].sended_bytes       = 0;
	RTP_session[rtp_num].recved_bytes       = 0;
	 
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	
	dbgprintf(6, "RTSP_Activate_RTP_session %i %i\n", rtp_num, cType);
	
	return 1;
}

char RTSP_Teardown_RTP_session(unsigned int rtp_num)
{
	dbgprintf(6, "RTSP_Teardown_RTP_session %i\n", rtp_num);
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	
	sx_nal_to_rtp_util_destroy(RTP_session[rtp_num].to_rtp_instance);
	
	memset(&RTP_session[rtp_num], 0, sizeof(RTP_SESSION));
	
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	return 1;
}

unsigned int RTSP_get_new_rtp_session()
{
	unsigned int i;
	for(i = 0; i < RTP_session_cnt; i++) 
		if (RTP_session[i].status == 0) 
		{
			memset(&RTP_session[i], 0, sizeof(RTP_SESSION));	
			break;
		}
	if (i == RTP_session_cnt)
	{
		RTP_session_cnt++;
		RTP_session = (RTP_SESSION*)DBG_REALLOC(RTP_session, RTP_session_cnt*sizeof(RTP_SESSION));
		memset(&RTP_session[RTP_session_cnt-1], 0, sizeof(RTP_SESSION));
	}
	
	return i;
}

static int RTSP_rtp_video_send(int sock, RTP_SESSION *session, unsigned char  *pData, unsigned int uiDataLen)
{
    sRTP_PKT_NODE  *head;
    sRTP_PKT_NODE  *temp;
	int res = 1;
	int iPackSize;
	if (session->rtsp_session->interleaved)
	{
		iPackSize = ((uiDataLen/RTP_PAYLOAD_SIZE)+1) * (8 + sizeof(sRTP_HEADER));
		iPackSize = uiDataLen + 10;
		if (iPackSize > (session->rtsp_session->OutSize - session->rtsp_session->OutLen))
		{
			dbgprintf(5, "RTSP_rtp_video_send: send buffer overfull\n");
			return 0;
		}
	}		
	
	//struct sockaddr_in *addr;
	//if (session->broadcast_status) addr = &session->broad_addr; else 
		//addr = &session->peer_addr;

    // Get the converted RTP chain.
    head = sx_nal_to_rtp_util_get(session->to_rtp_instance, pData, uiDataLen);
    temp = head;
	
	char *pSendData;
	int rv; 
	
	do
    {
        // Send the packet.
        //printf("send aud to %s\n", inet_ntoa(addr->sin_addr));
        if (session->rtsp_session->interleaved)
		{
			if ((session->rtsp_session->OutSize - session->rtsp_session->OutLen) < (temp->rtp_pkt_len + 4))
			{
				dbgprintf(1, "RTSP_rtp_video_send: error send data - buffer overfull\n");
				res = 0;
				break;
			}
			pSendData = &session->rtsp_session->OutBuffer[session->rtsp_session->OutLen];
			pSendData[0] = 36;
			pSendData[1] = session->rtp_port;
			pSendData[2] = (temp->rtp_pkt_len & 0xFF00) >> 8;						
			pSendData[3] = temp->rtp_pkt_len & 255;
			pSendData += 4;
			memcpy(pSendData, &temp->rtp_pkt, temp->rtp_pkt_len);
			session->rtsp_session->OutLen += temp->rtp_pkt_len + 4;
		}
		else 
		{
			rv = sendto(sock, &temp->rtp_pkt, temp->rtp_pkt_len, 0, (struct sockaddr *) &session->peer_addr, sizeof(struct sockaddr));
			if (rv != temp->rtp_pkt_len)
			{
				dbgprintf(2, "RTSP_rtp_video_send: error send data(%s)\n", strerror(errno));
				res = -1;
				break;
			}
		}

        temp = temp->next; 

    } while (temp); 

	if (session->rtsp_session->interleaved)
	{
		char cType = SIGNAL_WORK;
		rv = write(session->rtsp_session->socketpair[0], (char*)&cType, 1);
		if (rv < 1) dbgprintf(4, "RTSP_rtp_video_send: write socketpair signal (errno:%i, %s)\n", errno, strerror(errno));	
	}
    // Free the NAL unit.
    sx_nal_to_rtp_util_free(head); 
	return res;
}

static int RTSP_rtp_audio_send(int sock, RTP_SESSION *session, unsigned char  *pData, unsigned int uiDataLen, unsigned int uiCodecNum)
{
    sRTP_PKT_NODE  *head;
    sRTP_PKT_NODE  *temp;
	
	int res = 1;
	int iPackSize;
	if (session->rtsp_session->interleaved)
	{
		iPackSize = uiDataLen + 4 + sizeof(sRTP_HEADER) + 4 + 10;
		if (iPackSize > (session->rtsp_session->OutSize - session->rtsp_session->OutLen))
		{
			dbgprintf(5, "RTSP_rtp_audio_send: send buffer overfull\n");
			return 0;
		}
	}
	
	//struct sockaddr_in *addr;
	//if (session->broadcast_status) addr = &session->broad_addr; else 
		//addr = &session->peer_addr;

    // Get the converted RTP chain.
	
    switch(uiCodecNum)
	{
		case 14:
			head = sx_ac3_to_rtp_util_get(session->to_rtp_instance, pData, uiDataLen);
			break;
		case 18:
			head = sx_ac3_to_rtp_util_get(session->to_rtp_instance, pData, uiDataLen);
			break;
		default:
			head = sx_aac_to_rtp_util_get(session->to_rtp_instance, pData, uiDataLen);
			break;
	}
	
	temp = head; 
	
	char *pSendData;
	int rv; 
	
	do
    {
        // Send the packet.
        //printf("send aud to %s\n", inet_ntoa(addr->sin_addr));
		//printf("!!! %i %i\n", temp->rtp_pkt_len, uiCodecNum);			
        if (session->rtsp_session->interleaved)
		{
			if ((session->rtsp_session->OutSize - session->rtsp_session->OutLen) < (temp->rtp_pkt_len + 4))
			{
				dbgprintf(1, "RTSP_rtp_audio_send: error send data - buffer overfull\n");
				res = 0;
				break;
			}
			pSendData = &session->rtsp_session->OutBuffer[session->rtsp_session->OutLen];
			pSendData[0] = 36;
			pSendData[1] = session->rtp_port;
			pSendData[2] = (temp->rtp_pkt_len & 0xFF00) >> 8;						
			pSendData[3] = temp->rtp_pkt_len & 255;
			
			pSendData += 4;
			memcpy(pSendData, &temp->rtp_pkt, temp->rtp_pkt_len);
			session->rtsp_session->OutLen += temp->rtp_pkt_len + 4;
		}
		else 
		{
			rv = sendto(sock, &temp->rtp_pkt, temp->rtp_pkt_len, 0, (struct sockaddr *) &session->peer_addr, sizeof(struct sockaddr));
			if (rv != temp->rtp_pkt_len)
			{
				dbgprintf(2, "RTSP_rtp_audio_send: error send data(%s)\n", strerror(errno));
				res = -1;
				break;
			}
		}

        temp = temp->next; 

    } while (temp); 

	if (session->rtsp_session->interleaved)
	{
		char cType = SIGNAL_WORK;
		rv = write(session->rtsp_session->socketpair[0], (char*)&cType, 1);
		if (rv < 1) dbgprintf(4, "RTSP_rtp_audio_send: write socketpair signal (errno:%i, %s)\n", errno, strerror(errno));	
	}
    // Free the NAL unit.
    sx_nal_to_rtp_util_free(head);
	
	return res;	
}

int RTSP_SwitchBroadcast()
{
	int iCnt = 0;
	int iCnt2 = 0;
	int i;
	int broadcaststatus = 0;
	for(i = 0; i < RTP_session_cnt; i++)  
	{
		if ((RTP_session[i].status != 0) && (RTP_session[i].idr_observed == 0)) 
		{
			iCnt2++;
			broadcaststatus = 0;
			break;
		}
		if ((RTP_session[i].status != 0) && (RTP_session[i].idr_observed == 1) && (RTP_session[i].broadcast_enabled == 1))
		{
			iCnt++;
			if (iCnt == 1) broadcaststatus = RTP_session[i].broadcast_status;
		}
	}
	if ((broadcaststatus == 0) && (iCnt > 1) && (iCnt2 == 0))
	{
		broadcaststatus = 1;
		//printf("Switch ON broadcast\n");
		for (i = 0; i < RTP_session_cnt; i++) 
			if ((RTP_session[i].status != 0) && (RTP_session[i].idr_observed == 1) && (RTP_session[i].broadcast_enabled == 1))
					RTP_session[i].broadcast_status = broadcaststatus;
		if (setsockopt(RTP_Socket, SOL_SOCKET, SO_BROADCAST, &broadcaststatus, sizeof(broadcaststatus)) == -1) 
			dbgprintf(1,"setsockopt RTP (SO_BROADCAST) to 1\n");
	}
	if ((broadcaststatus == 1) && ((iCnt <= 1) || (iCnt2 != 0)))
	{
		broadcaststatus = 0;
		//printf("Switch OFF broadcast\n");
		for (i = 0; i < RTP_session_cnt; i++) 
			if ((RTP_session[i].status != 0) && (RTP_session[i].idr_observed == 1) && (RTP_session[i].broadcast_enabled == 1))
					RTP_session[i].broadcast_status = broadcaststatus;
		if (setsockopt(RTP_Socket, SOL_SOCKET, SO_BROADCAST, &broadcaststatus, sizeof(broadcaststatus)) == -1) 
			dbgprintf(1,"setsockopt RTP (SO_BROADCAST) to 1\n");
	}
	return broadcaststatus;
}

int ItsRTSPNeed(unsigned int uiType, unsigned int iID, char cCalc)
{
	DBG_LOG_IN();
	unsigned int  i;
	int res = 0;
	
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	for(i = 0; i < RTP_session_cnt; i++)
    {
        RTP_SESSION *session = &RTP_session[i];	
        if (session->status & 3)
        {	
			if ((session->type == 0) && (uiType == 0))
			{
				DBG_MUTEX_LOCK(&session->rtsp_session->mutex);
				if (session->rtsp_session->video_channel == iID) res++;
				DBG_MUTEX_UNLOCK(&session->rtsp_session->mutex);
			}
			if ((session->type > 0) && (uiType > 0))
			{				
				DBG_MUTEX_LOCK(&session->rtsp_session->mutex);
				if ((session->rtsp_session->audio_list_cnt) 
					&& (iID == session->rtsp_session->audio_list_dt[session->rtsp_session->audio_list_pos].ID))
						res++;
				DBG_MUTEX_UNLOCK(&session->rtsp_session->mutex);
			}
        }
		if ((cCalc == 0) && res) break;
    }
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	
	DBG_LOG_OUT();	
	return res;
}

int RTSP_SendVideoFrame(unsigned int uiModuleID, char* cFrameBuffer, unsigned int uiFrameLen, void* StartPack)
{
	DBG_LOG_IN();
	unsigned int  i;
	int res = 0;
	
	misc_buffer   data_to_send;
	int ret = 0;
	omx_start_packet *Start_Packet = (omx_start_packet*)StartPack;
	
	DBG_MUTEX_LOCK(&RTSP_mutex);	
	i = cRTSP_init_done2;
	if (i == 1) 
	{
		if ((Start_Packet->PacketType == 0) && (cVideoDataFillMain == 0))
		{
			unsigned char buff[256];
			if(RTSP_video_get_sps(Start_Packet, &data_to_send) != 0)
			{
				ret = base64_encode((unsigned char*)data_to_send.data, data_to_send.data_size, buff, 256);
				cVideoDataSPSMain = (char*)DBG_MALLOC(ret + 1);
				memcpy(cVideoDataSPSMain, buff, ret);
				cVideoDataSPSMain[ret] = 0;
				
				if(RTSP_video_get_pps(Start_Packet, &data_to_send) != 0)
				{
					ret = base64_encode((unsigned char*)data_to_send.data, data_to_send.data_size, buff, 256);
					cVideoDataPPSMain = (char*)DBG_MALLOC(ret + 1);
					memcpy(cVideoDataPPSMain, buff, ret);
					cVideoDataPPSMain[ret] = 0;
					cVideoDataFillMain = 1;
					//printf("filled VideoData\n");
				}
			}		
		}
		if ((Start_Packet->PacketType == 1) && (cVideoDataFillPrev == 0))
		{
			unsigned char buff[256];
			if(RTSP_video_get_sps(Start_Packet, &data_to_send) != 0)
			{
				ret = base64_encode((unsigned char*)data_to_send.data, data_to_send.data_size, buff, 256);
				cVideoDataSPSPrev = (char*)DBG_MALLOC(ret + 1);
				memcpy(cVideoDataSPSPrev, buff, ret);
				cVideoDataSPSPrev[ret] = 0;
				
				if(RTSP_video_get_pps(Start_Packet, &data_to_send) != 0)
				{
					ret = base64_encode((unsigned char*)data_to_send.data, data_to_send.data_size, buff, 256);
					cVideoDataPPSPrev = (char*)DBG_MALLOC(ret + 1);
					memcpy(cVideoDataPPSPrev, buff, ret);
					cVideoDataPPSPrev[ret] = 0;
					cVideoDataFillPrev = 1;
					//printf("filled VideoData\n");
				}
			}		
		}
	}
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	
	if (i != 1) 
	{
		DBG_LOG_OUT();
		return 0;
	}
    int iCnt = 0;
	ret = 0;
	//VideoCodecInfo *VideoParams = (VideoCodecInfo*)tVideoInfo;
	
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	//int broadcaststatus = RTSP_SwitchBroadcast();
	for(i = 0; i < RTP_session_cnt; i++)
    {
        RTP_SESSION *session = &RTP_session[i];
	
        if ((session->status & 3) && (session->type == 0))
        {			
			DBG_MUTEX_LOCK(&session->rtsp_session->mutex);
			//printf("Send frame to session: %i %i %i %i\n", i, session->status & 3, session->type, session->rtsp_session->audio_enabled);
			if (session->rtsp_session->video_channel == Start_Packet->PacketType)
			{
				while(1)
				{	
					if(!session->sps_sent)
					{
						//printf("Send SPS unit len %i\n", Start_Packet->CodecInfoLen);
						if (RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen) < 0) 
							session->status++;
							else
							res++;
						session->sps_sent = 1;
						session->sended_bytes += Start_Packet->CodecInfoLen;
					}
					//if ((Start_Packet->StartFramesCount != 1) && (!session->sps_sent)) break;
					/*if(!session->sps_sent)
					{
						if(RTSP_video_get_sps(Start_Packet, &data_to_send) == 0)
						{
							printf("mgmt_video_get_sps_nal_unit() returned 0!\n");
							break;
						}
						printf("Send SPS unit\n");
						session->sps_sent = 1;
						RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)data_to_send.data, data_to_send.data_size); 
					}
					if(!session->pps_sent)
					{
						printf("Send PPS unit\n");
						// Send PPS first.
						if(RTSP_video_get_pps(Start_Packet, &data_to_send) == 0)
						{
							printf("mgmt_video_get_pps_nal_unit() returned 0!\n");
							break;
						}					
						//printf("PPS unit len %d\n", data_to_send.data_size);
						//printf("Send PPS to session: %i\n", i);
						session->pps_sent = 1;
						RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)data_to_send.data, data_to_send.data_size);    
						break;
					}*/
					if(!session->idr_observed)
					{
						//if(!RTSP_video_is_key_frame(&in_data)) break;
						if (Start_Packet->StartFramesCount <= 1) 
						{
							session->idr_observed = 1; 
							session->cur_frame_num = 0;
							session->cur_frame_pos = 0;
						}
						if (Start_Packet->StartFramesFlags[session->cur_frame_num] == 2)
						//while (Start_Packet->StartFramesFlags[session->cur_frame_num] == 2)
						{
							data_to_send.data = &Start_Packet->BufferStartFrame[session->cur_frame_pos]+4;
							data_to_send.data_size = Start_Packet->StartFramesSizes[session->cur_frame_num]-4;
							session->cur_frame_pos += Start_Packet->StartFramesSizes[session->cur_frame_num];
							session->prev_frame_num = session->cur_frame_num;
							//printf("Send start frame to session: %i %i\n", i, ret);
							//printf("Strt %i unit (%i)\n", session->cur_frame_num, Start_Packet->StartFramesCount);
							
							if (RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)data_to_send.data, data_to_send.data_size) < 0)
								session->status++;
								else
								res++;
							/*if ((session->interleaved == 1) && (session->cur_frame_num == 0) && (session->idr_observed == 0))
							{
								printf("##############\n");
								RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)data_to_send.data, data_to_send.data_size);
								RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)data_to_send.data, data_to_send.data_size);
							}*/
							session->cur_frame_num++;	
							session->sended_bytes += data_to_send.data_size;
							//if (session->cur_frame_num >= Start_Packet->StartFramesCount) break;
						}										                  				
					} 
					else 
					{
						if (Start_Packet->StartFramesCount != 0)
						{
							ret = Start_Packet->StartFramesCount - 1;
							if ((Start_Packet->StartFramesFlags[ret] == 2) && (session->prev_frame_num != ret))
							{
								iCnt = Start_Packet->StartFrameLen - Start_Packet->StartFramesSizes[ret];
								data_to_send.data = &Start_Packet->BufferStartFrame[iCnt]+4;
								data_to_send.data_size = Start_Packet->StartFramesSizes[ret]-4;
								session->prev_frame_num = ret;
								//printf("Send %i unit (%i) len %i\n", ret, Start_Packet->StartFramesCount, data_to_send.data_size);
								//printf("Send Video frame to session: %i %i size %i\n", i, ret, data_to_send.data_size);
								if (RTSP_rtp_video_send(RTP_Socket, session, (unsigned char*)data_to_send.data, data_to_send.data_size) < 0)
									session->status++;
									else
									res++;
								session->sended_bytes += data_to_send.data_size;
							}
						}
					}
					break;
				}
			}				
			DBG_MUTEX_UNLOCK(&session->rtsp_session->mutex);
        }
		if ((session->status & 3) && (session->type > 0) && (session->rtsp_session->audio_enabled == 2))
        {
			DBG_MUTEX_LOCK(&session->rtsp_session->mutex);
			unsigned int uiCnt;
			for(uiCnt = 44 / session->rtsp_session->frame_rate; uiCnt; uiCnt--)
			{
				//printf("Send Audio frame to session: %i size %i\n", i, session->rtsp_session->force_audio_lenght);										
				if (RTSP_rtp_audio_send(RTP_Socket, session, (unsigned char*)session->rtsp_session->force_audio_packet, 
														session->rtsp_session->force_audio_lenght, 0) < 0)
						session->status++;
						else 
						res++;
					session->sended_bytes += uiFrameLen;
			}	
			DBG_MUTEX_UNLOCK(&session->rtsp_session->mutex);
        }
		//if (broadcaststatus) break;
    }
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	
	DBG_LOG_OUT();
	
	return res;
}

int RTSP_SendAudioFrame(unsigned int uiModuleID, char* cFrameBuffer, unsigned int uiFrameLen)
{
	DBG_LOG_IN();
	int i;
	int res = 0;
	
	DBG_MUTEX_LOCK(&RTSP_mutex);	
	i = cRTSP_init_done2;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);	
	
	if (i != 1) 
	{
		DBG_LOG_OUT();
		return 0;
	}
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	//int broadcaststatus = RTSP_SwitchBroadcast();
	
	for(i = 0; i < RTP_session_cnt; i++)
    {
        RTP_SESSION *session = &RTP_session[i];

        if ((session->status & 3) && (session->type > 0))
        {
			//printf("Send Audio frame to session: %i size %i\n", i, uiFrameLen);							
			DBG_MUTEX_LOCK(&session->rtsp_session->mutex);
			
			if ((session->rtsp_session->audio_list_cnt) 
				&& (uiModuleID == session->rtsp_session->audio_list_dt[session->rtsp_session->audio_list_pos].ID))
				{
					if (RTSP_rtp_audio_send(RTP_Socket, session, (unsigned char*)cFrameBuffer, uiFrameLen, 
												session->rtsp_session->audio_list_dt[session->rtsp_session->audio_list_pos].CodecNum) < 0)
						session->status++;
						else 
						res++;
					session->sended_bytes += uiFrameLen;
				}
			DBG_MUTEX_UNLOCK(&session->rtsp_session->mutex);
        }
		//if (broadcaststatus) break;
    }
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	
	DBG_LOG_OUT();
	
	return res;
}

unsigned char RTSP_video_get_sps(omx_start_packet *Start_Packet, misc_buffer *m_data)
{
	if (Start_Packet->CodecInfoFilled == 1)
	{
		int ret;
		char buff[4];
		memset(buff, 0, 3);
		buff[3] = 1;
		if (SearchData(buff, 4, Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen, 0) == 0)
		{
			if ((Start_Packet->BufferCodecInfo[4] & 0x1F) == 0x07)
			{
				ret = SearchData(buff, 4, Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen, 4);
				if (ret >= 0)
				{
					m_data->data = Start_Packet->BufferCodecInfo+4;
					m_data->data_size = ret-4;
					return 1;
				}
			}
		}
	}
	return 0;
}

unsigned char RTSP_video_get_pps(omx_start_packet *Start_Packet, misc_buffer *m_data)
{
	if (Start_Packet->CodecInfoFilled == 1)
	{
		int ret;
		char buff[4];
		memset(buff, 0, 3);
		buff[3] = 1;
		if (SearchData(buff, 4, Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen, 0) == 0)
		{
			ret = SearchData(buff, 4, Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen, 4);
			if (ret >= 0)
			{
				if ((Start_Packet->BufferCodecInfo[ret+4] & 0x1F) == 0x08)
				{				
					m_data->data = &Start_Packet->BufferCodecInfo[ret+4];
					m_data->data_size = Start_Packet->CodecInfoLen - (ret+4);
					return 1;
				}
			}
		}
	}
	return 0;
}

unsigned char RTSP_video_is_key_frame(misc_buffer *m_data)
{
	if (m_data->data_size != 0)
	{
		if((m_data->data[0] & 0x1F) == 0x05) 
		{
			return 1; 
		}
	}
    return 0; 
}

void rtsp_start(char cVideoOn, char cAudioOn, unsigned int uiVideoFrameRate, MIC_LIST *rmlList, unsigned int uiMicListCnt,
					unsigned int uRTSPPort, char cForceAudio)
{
	cRTSP_init_done = 0;
	cRTSP_init_done2 = 0;
	RTP_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	if (RTP_Socket == -1)
	{
		dbgprintf(1, "rtsp_start: Error create RTP socket\n");
		return;
	}
	int ttl = 60; /* max = 255 */
	if (setsockopt(RTP_Socket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1)
		dbgprintf(1, "rtsp_start: Error setsockopt IP_TTL\n");
	
		
	pthread_mutex_init(&RTSP_mutex, NULL);
	pthread_mutex_init(&RTP_session_mutex, NULL);
	
	pthread_attr_init(&tattrRTSP_io);   
	pthread_attr_setdetachstate(&tattrRTSP_io, PTHREAD_CREATE_DETACHED);
	
	DBG_MUTEX_LOCK(&RTSP_mutex);		
	cThreadRtspStatus = 0;
	Rtsp_Pair[0] = 0;
	Rtsp_Pair[1] = 0;		
	DBG_MUTEX_UNLOCK(&RTSP_mutex);	
	
	unsigned int *cSettings = (unsigned int*)DBG_MALLOC(11*sizeof(unsigned int));
	cSettings[0] = cVideoOn;
	cSettings[1] = cAudioOn;
	cSettings[2] = uiVideoFrameRate;
	cSettings[5] = uRTSPPort;
	
	misc_buffer *mBuff = (misc_buffer*)DBG_MALLOC(sizeof(misc_buffer));
	mBuff->void_data = cSettings;
	mBuff->void_data2 = rmlList;
	mBuff->data_size = uiMicListCnt;
	mBuff->uidata[0] = cForceAudio;
	if (cForceAudio) 
	{
		misc_buffer buffer;
		char cPath[MAX_PATH];
		FillConfigPath(cPath, MAX_PATH, "SilentSoundACCFrame.aac", 0);	
		if (load_file_in_buff(cPath, &buffer))
		{
			mBuff->uidata[1] = buffer.data_size;
			mBuff->void_data3 = buffer.void_data;
		} else mBuff->uidata[0] = 0;
	}
	
	pthread_attr_init(&tattrRTSP_income);   
	pthread_attr_setdetachstate(&tattrRTSP_income, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadRTSP_income, &tattrRTSP_income, thread_RTSP_income, (void*)mBuff); 
	cRTSP_init_done = 1;
	cVideoDataFillMain = 0;
	cVideoDataSPSMain = NULL;
	cVideoDataPPSMain = NULL;
	cVideoDataFillPrev = 0;
	cVideoDataSPSPrev = NULL;
	cVideoDataPPSPrev = NULL;
	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	cRTSP_init_done2 = 1;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
}

void rtsp_stop()
{
	if (cRTSP_init_done == 0) return;
	int i, ret;
	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	cRTSP_init_done2 = 0;
	if (Rtsp_Pair[0] != 0)
	{
		char cType = SIGNAL_CLOSE;
		int rv = write(Rtsp_Pair[0], &cType, 1);
		if (rv < 1) dbgprintf(4, "rtsp_stop: write socketpair signal %i (errno:%i, %s)\n", cType, errno, strerror(errno));	
	}
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	
	char cType;
	DBG_MUTEX_LOCK(&RTP_session_mutex);
	for(i = 0; i < RTP_session_cnt; i++)
    {
        RTP_SESSION *session = &RTP_session[i];	
        if ((session->status) && (session->rtsp_session->socketpair[0] != 0))
		{
			cType = SIGNAL_CLOSE;
			ret = write(session->rtsp_session->socketpair[0], (char*)&cType, 1);
			if (ret < 1) dbgprintf(4, "RTSP_rtp_video_send: write socketpair signal (errno:%i, %s)\n", errno, strerror(errno));
		}
	}
	DBG_MUTEX_UNLOCK(&RTP_session_mutex);
	
	do
	{
		DBG_MUTEX_LOCK(&RTSP_mutex);
		ret = cThreadRtspStatus;
		DBG_MUTEX_UNLOCK(&RTSP_mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);
	
	while(1)
	{
		ret = 0;			
		DBG_MUTEX_LOCK(&RTP_session_mutex);
		for (i = 0; i < RTP_session_cnt; i++)
			if (RTP_session[i].status != 0)	ret++;	
		DBG_MUTEX_UNLOCK(&RTP_session_mutex);
		if (ret) usleep(50000); else break;
	}
	dbgprintf(4, "Closed All Rtsp Connections\n");
	if (RTP_session_cnt) DBG_FREE(RTP_session);
	if (cVideoDataSPSMain) DBG_FREE(cVideoDataSPSMain);
	if (cVideoDataPPSMain) DBG_FREE(cVideoDataPPSMain);
	if (cVideoDataSPSPrev) DBG_FREE(cVideoDataSPSPrev);
	if (cVideoDataPPSPrev) DBG_FREE(cVideoDataPPSPrev);
	
	cRTSP_init_done = 0;
	pthread_mutex_destroy(&RTP_session_mutex);
	pthread_mutex_destroy(&RTSP_mutex);
	
	pthread_attr_destroy(&tattrRTSP_income);
	pthread_attr_destroy(&tattrRTSP_io);	
}

void * thread_RTSP_income(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "RTSP_income");
	
	misc_buffer *mBuff = (misc_buffer*)pData;
	unsigned int *uiSettings = mBuff->void_data;
	unsigned int uiMicListCnt = mBuff->data_size;
	MIC_LIST *rmlList = mBuff->void_data2;
	
	char cForceAudio = mBuff->uidata[0];
	char* pForceAudioData = mBuff->void_data3;
	unsigned int uiForceAudioLen = mBuff->uidata[1];
	
	unsigned int cVideoOn 			= uiSettings[0];
	unsigned int cAudioOn 			= uiSettings[1];
	unsigned int uiVideoFrameRate	= uiSettings[2];
	unsigned int uRTSPPort			= uiSettings[5];

	DBG_FREE(mBuff);
	DBG_FREE(uiSettings);
	
	struct sockaddr_in      client_addr;
    struct sockaddr_in      client_port;  
	struct sockaddr_in  	broadcast_addr;
	struct sockaddr_in  	local_addr;	
	
	unsigned int            addr_len;
	int tcp_sock;
	RTSP_SESSION *session;
    // Create socket. 
    int rtsp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(rtsp_sock != -1);
	
	int n = 1;
	if (setsockopt(rtsp_sock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int)) < 0)
		dbgprintf(2, "WEB setsockopt(SO_REUSEADDR) failed\n");

	int ttl = 60; /* max = 255 */
	if (setsockopt(rtsp_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1)
		dbgprintf(1, "thread_RTSP_income: Error setsockopt IP_TTL\n");
	
    // Setup address. 
    client_addr.sin_family      = AF_INET;
    client_addr.sin_port        = htons(uRTSPPort);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&client_addr.sin_zero, 0, 8);

    // Bind to address. 
    int rc = bind(rtsp_sock,(struct sockaddr *) &client_addr, sizeof(client_addr)); 
    if (rc != 0)
	{
		dbgprintf(1, "thread_RTSP_income: Error bind socket\n");
		close(rtsp_sock);
		DBG_LOG_OUT();
		pthread_exit(0);
	}

    // Listening for traffic. 
    if (listen(rtsp_sock, 5) == -1) 
	{
        dbgprintf(1,"Error listen rtsp socket(%i) %s\n", errno, strerror(errno));
        close(rtsp_sock);
		DBG_LOG_OUT();
		pthread_exit(0);
    }
	
	int flags = fcntl(rtsp_sock, F_GETFL, 0);
	if (fcntl(rtsp_sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(1,"error set rtsp nonblock(%i) %s\n", errno, strerror(errno));
		close(rtsp_sock);
		DBG_LOG_OUT();
        pthread_exit(0);
	}

	int iLocalcast = GetLocalAddr(&local_addr);
	int iBroadcast = GetBroadcastAddr(&broadcast_addr);
	int iLoop = 1;
	char SignalBuff;
	
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, Rtsp_Pair) < 0)
	{	
		dbgprintf(1,"error create socketpair\n");
		Rtsp_Pair[0] = 0;
		Rtsp_Pair[1] = 0;
		iLoop = 0;
	}
	else
	{
		flags = fcntl(Rtsp_Pair[0], F_GETFL, 0);
		if (fcntl(Rtsp_Pair[0], F_SETFL, flags | O_NONBLOCK) < 0)
		{
			dbgprintf(1,"error set socketpair nonblock\n");
			iLoop = 0;
		}
		else
		{
			flags = fcntl(Rtsp_Pair[1], F_GETFL, 0);
			if (fcntl(Rtsp_Pair[1], F_SETFL, flags | O_NONBLOCK) < 0)
			{
				dbgprintf(1,"error set socketpair nonblock\n");
				iLoop = 0;
			}
		}
	}
	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	cThreadRtspStatus++;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);	
	
	int max_fd = -1;
	fd_set rfds;
	char ReadSignalReady = 1;
	struct timeval tv;
	int ret;
		
    while(iLoop)
    {
		FD_ZERO(&rfds);		
		FD_SET(rtsp_sock, &rfds);
		max_fd = rtsp_sock;
		if (ReadSignalReady == 0)
		{
			FD_SET(Rtsp_Pair[1], &rfds);
			if (Rtsp_Pair[1] > max_fd) max_fd = Rtsp_Pair[1];
			tv.tv_sec = 1;
		} else tv.tv_sec = 0;
		
		tv.tv_usec = 0;		
		
		ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		
		if ((ReadSignalReady == 0) && (FD_ISSET(Rtsp_Pair[1], &rfds))) ReadSignalReady = 1;
		if (ReadSignalReady == 1)
		{	
			ret = read(Rtsp_Pair[1], &SignalBuff, 1);
			if ((ret > 0) && (SignalBuff == SIGNAL_CLOSE))
			{
				iLoop = 0;
				//printf("SIGNAL_CLOSE %i\n", conn_num);
				dbgprintf(4, "Close RTSP signal\n");	
				break;
			}
			else
			{
				if (errno == EAGAIN) ReadSignalReady = 0;
				else 
				{
					dbgprintf(2, "Close RTSP socketpair (errno:%i, %s)\n",errno, strerror(errno));
					break;
				}
			}
		}
		if (ret != 0)
		{
			if (FD_ISSET(rtsp_sock, &rfds))
			{
				// Accept new TCP connection from specified address. 
				addr_len = sizeof(client_addr); 
				tcp_sock = accept(rtsp_sock, (struct sockaddr *) &client_addr, &addr_len); 
				if (tcp_sock != -1)
				{
					//printf("RTSP_income: Received new TCP connection, initiating new RTSP sever instance...\n"); 
					//int session = rtsp_get_new_session(); 
					if (iLocalcast == 0) iLocalcast = GetLocalAddr(&local_addr);
					if (iBroadcast == 0) iBroadcast = GetBroadcastAddr(&broadcast_addr);
					
					session = (RTSP_SESSION*)DBG_MALLOC(sizeof(RTSP_SESSION));
					memset(session, 0, sizeof(RTSP_SESSION));
					
					pthread_mutex_init(&session->mutex, NULL);
					
					addr_len = sizeof(client_port);
					getsockname(tcp_sock, (struct sockaddr*)&client_port, (socklen_t*)&addr_len);
					
					session->rtsp_session_num = ((unsigned int)(intptr_t)(session)) & 0x7FFFFFFF;
					session->ip   			= client_addr.sin_addr.s_addr;			
					session->port			= client_port.sin_port;
					session->rtsp_port		= uRTSPPort;
					memset(session->Login, 0, 64);
					memset(session->Password, 0, 64);
					session->video_enabled 	= (char)cVideoOn;
					session->audio_enabled 	= (char)cAudioOn;
					session->frame_rate		= uiVideoFrameRate;
					session->audio_list_pos = 0;
					if ((session->audio_enabled) && (uiMicListCnt))
					{
						session->audio_list_cnt = uiMicListCnt;
						session->audio_list_dt = (MIC_LIST*)DBG_MALLOC(uiMicListCnt * sizeof(MIC_LIST));
						memcpy(session->audio_list_dt, rmlList, uiMicListCnt * sizeof(MIC_LIST));					
					}
					DBG_MUTEX_LOCK(&system_mutex);
					session->Auth					= RtspAuth;
					session->set_rtp_clnt_v_port	= uiRTPClntVidPort;
					session->set_rtp_clnt_a_port	= uiRTPClntAudPort;
					session->set_rtp_serv_v_port	= uiRTPServVidPort;
					session->set_rtp_serv_a_port	= uiRTPServAudPort;	
					DBG_MUTEX_UNLOCK(&system_mutex);
					session->video_channel_num		= 96; //105;
					session->audio_channel_num		= 97; //102;
					//session->localcast		= iLocalcast;
					session->broadcast		= iBroadcast;			
					if (iLocalcast != 0) memcpy(&session->local_addr, &local_addr, sizeof(struct sockaddr_in));
					if (iBroadcast != 0) memcpy(&session->broadcast_addr, &broadcast_addr, sizeof(struct sockaddr_in));			
					session->rtp_video_port = 0;
					session->rtp_audio_port = 0;
					session->socket 		= tcp_sock; 
					int ttl = 60; /* max = 255 */
					if (setsockopt(tcp_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1)
						dbgprintf(1, "thread_RTSP_income: Error income setsockopt IP_TTL\n");
					if (socketpair(AF_UNIX, SOCK_STREAM, 0, session->socketpair) >= 0) 
					{
						int flags;
						flags = fcntl(session->socketpair[1], F_GETFL, 0);
						if (fcntl(session->socketpair[1], F_SETFL, flags | O_NONBLOCK) < 0)
						{
							dbgprintf(1,"error set tcp pair nonblock\n");
							close(tcp_sock);
							DBG_FREE(session);
						}
						flags = fcntl(tcp_sock, F_GETFL, 0);
						if (fcntl(tcp_sock, F_SETFL, flags | O_NONBLOCK) < 0)
						{
							dbgprintf(1,"error set tcp nonblock\n");
							close(tcp_sock);
							DBG_FREE(session);
						} 
						else 
						{							
							session->force_audio_enabled = cForceAudio;
							if (cForceAudio && uiForceAudioLen) 
							{
								session->force_audio_lenght = uiForceAudioLen;
								session->force_audio_packet = (char*)DBG_MALLOC(uiForceAudioLen);
								memcpy(session->force_audio_packet, pForceAudioData, uiForceAudioLen);
							} else session->force_audio_lenght = 0;
							pthread_create(&threadRTSP_io, &tattrRTSP_io, thread_RTSP_io, (void*)session); 	
						}
					}
					else
					{
						dbgprintf(1,"error get socketpair\n");
						close(tcp_sock);
						DBG_FREE(session);
					}			
				}
			}
		}
    }	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	if (Rtsp_Pair[0] != 0) close(Rtsp_Pair[0]);
	if (Rtsp_Pair[1] != 0) close(Rtsp_Pair[1]);
	Rtsp_Pair[0] = 0;
	Rtsp_Pair[1] = 0;
	cThreadRtspStatus--;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	
	if (uiMicListCnt) DBG_FREE(rmlList);
	if (cForceAudio && uiForceAudioLen) DBG_FREE(pForceAudioData);
	close(rtsp_sock);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)0;
}

void * thread_RTSP_io(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "RTSP_io");
	
	RTSP_SESSION			*session = (RTSP_SESSION*)pData;
	unsigned int            id, msg_type; 
    char                    msg_rx[RTSP_BUF_SIZE_MAX]; 
    char                    msg_tx[RTSP_BUF_SIZE_MAX];
    int                     ret, ret2; 
	
	session->OutBuffer 		= (char*)DBG_MALLOC(RTSP_BUFFER_SIZE);
	session->OutLen			= 0;
	session->OutSize		= RTSP_BUFFER_SIZE;
	
	memset(msg_tx, 0, RTSP_BUF_SIZE_MAX);
	memset(msg_rx, 0, RTSP_BUF_SIZE_MAX);
	
	id = (unsigned int)(intptr_t)pData; 
	memset(session->ip_str, 0, 16);
	sprintf(session->ip_str, "%d.%d.%d.%d",
        ((unsigned char *) &session->ip)[0], ((unsigned char *) &session->ip)[1], ((unsigned char *) &session->ip)[2], ((unsigned char *) &session->ip)[3]); 	

    dbgprintf(5, "RTSP_io: New RTSP server thread. [id = %d]\n", id); 
    dbgprintf(4, "RTSP_io: Client IP: %s\n", session->ip_str); 
	
	fd_set rfds, wfds;
	struct timeval tv;
	int max_fd, res, i;
	char status = 0;
	char iSocketReadNotReady = 1; 
	char iSocketWriteNotReady = 1;
	char auth_bad_cnt = 0;
	char authented = 1;
	if (session->Auth == 1) authented = 0;
	
	char cWait;
	FD_ZERO(&rfds);			
	FD_ZERO(&wfds);			
	max_fd = session->socketpair[1];
	if (max_fd < session->socket) max_fd = session->socket;
	char cLoop = 1;
	
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int uiSysID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);							
	
	int iAuthLocked = 0;
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	
	DBG_MUTEX_LOCK(&RTSP_mutex);	
	cThreadRtspStatus++;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	WEB_AddWebHistory(0, session->ip_str, 0, WEB_ACT_CONNECT, session->Auth, "");
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	
	while(cLoop)
    {
		if ((iSocketReadNotReady == 1) || (iSocketWriteNotReady == 1)) 
		{
			FD_ZERO(&rfds);			
			FD_ZERO(&wfds);			
			FD_SET(session->socketpair[1], &rfds);	
			if (iSocketReadNotReady == 1) FD_SET(session->socket, &rfds);
			if (iSocketWriteNotReady == 1) FD_SET(session->socket, &wfds);	
			DBG_MUTEX_LOCK(&session->mutex);
			if ((iSocketReadNotReady == 1) && ((iSocketWriteNotReady == 1) || (session->OutLen == 0))) 
			{
				cWait = 1;
				tv.tv_sec = 60; 		
			}
			else 
			{
				cWait = 0;
				tv.tv_sec = 0; 		
			}			
			tv.tv_usec = 0;
			DBG_MUTEX_UNLOCK(&session->mutex);
			ret = select(max_fd + 1, &rfds, &wfds, NULL, &tv);
			if ((ret == 0) && (cWait)) 
			{
				dbgprintf(4, "RTSP_io: Timeout connection. [client IP: %s]\n", session->ip_str); 
				break;
			}
			if (ret > 0)
			{
				if (FD_ISSET(session->socketpair[1], &rfds))
				{
					while(1)
					{
						ret2 = read(session->socketpair[1], &msg_rx, 1024); 
						if (ret2 > 0)
						{
							for (i = 0; i < ret2; i++) if (msg_rx[i] == SIGNAL_CLOSE) {cLoop = 0; break;}
							if (cLoop == 0) break;
						}
						if ((ret2 < 1) && (errno == EAGAIN)) break;
					}
					if (cLoop == 0) break;
				}
				//if ((iSocketReadNotReady) && (FD_ISSET(session->socket, &rfds))) dbgprintf(4, "RTSP_io: iSocketRead Ready\n");	
				//if ((iSocketWriteNotReady) && (FD_ISSET(session->socket, &wfds))) dbgprintf(4, "RTSP_io: iSocketWrite Ready\n");	
				if ((iSocketReadNotReady) && (FD_ISSET(session->socket, &rfds))) iSocketReadNotReady = 0;
				if ((iSocketWriteNotReady) && (FD_ISSET(session->socket, &wfds))) iSocketWriteNotReady = 0;				
			}
			if (ret < 0) dbgprintf(1, "RTSP_io: select error(errno:%i, %s)\n", errno, strerror(errno));
		}
				
		if ((iSocketReadNotReady == 0) && (session->socket))
		{	
			memset(msg_rx, 0, RTSP_BUF_SIZE_MAX);
			ret = read(session->socket, msg_rx, RTSP_BUF_SIZE_MAX); 
			if (ret < 1) 
			{
				if (ret == 0)
				{
					dbgprintf(4, "RTSP_io: read error(closed)\n");	
					break; 
				}
				if (errno == EAGAIN)
				{
					iSocketReadNotReady = 1;
					//printf("EAGAIN %i\n", ret);
				}
				else
				{
					//session->socket = 0;
					dbgprintf(4, "RTSP_io: read error(errno:%i, %s)\n", errno, strerror(errno));	
					break; 
				}
			}
			if (ret > 0)
			{
				//printf("RTSP_io: RTSP Request [session ID = %d]:\n", id); 
				memset(msg_tx, 0, RTSP_BUF_SIZE_MAX);
				
				msg_type = RTSP_get_msg_type(msg_rx); 
				switch(msg_type)
				{
					case RTSP_MSG_OPTIONS: 
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_OPTIONS\n"); 
						//printf("%s\n", msg_rx); 
						//if (authented)
						{
							if (RTSP_options_respond(msg_rx, msg_tx) == 0)
							{
								msg_type = RTSP_MSG_ERROR; 
								dbgprintf(2, "RTSP_options_respond ERROR\n");
							}
						}
						//else RTSP_describe_respond_401(msg_rx, msg_tx);
						break; 
					}
					case RTSP_MSG_DESCRIBE:
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_DESCRIBE\n"); 
						//printf("%s\n", msg_rx); 
						if (authented)
						{
							if (session->video_channel == 0)
								ret = RTSP_describe_respond(session, msg_rx, msg_tx, cVideoDataSPSMain, cVideoDataPPSMain);
								else
								ret = RTSP_describe_respond(session, msg_rx, msg_tx, cVideoDataSPSPrev, cVideoDataPPSPrev);
							if (ret == 0)
							{
								msg_type = RTSP_MSG_ERROR;
								dbgprintf(2, "RTSP_describe_respond ERROR1\n");
							}
						}
						else
						{
							if (iAuthLocked == 0) res = RTSP_Auth(session, msg_rx); else res = 0;
							res = RTSP_Auth(session, msg_rx);
							if (res == 1)
							{
								if (session->video_channel == 0)
									ret = RTSP_describe_respond(session, msg_rx, msg_tx, cVideoDataSPSMain, cVideoDataPPSMain);
									else
									ret = RTSP_describe_respond(session, msg_rx, msg_tx, cVideoDataSPSPrev, cVideoDataPPSPrev);
								if (ret == 0)
								{
									msg_type = RTSP_MSG_ERROR;
									dbgprintf(2, "RTSP_describe_respond ERROR2\n");
								} else authented = 1;
							}
							if (res != 1)
							{
								if (res == -1) 
								{
									dbgprintf(3, "RTSP_Auth ERROR\n");
									auth_bad_cnt++;									
								}
								ret = RTSP_describe_respond_401(session, msg_rx, msg_tx);
								if ((auth_bad_cnt >= RTSP_MAX_BAD_AUTH) || (ret == 0))
								{
									msg_type = RTSP_MSG_ERROR;
									dbgprintf(2, "RTSP_describe_respond ERROR\n");
								}
							}
						}						
						break; 
					}
					case RTSP_MSG_SETUP:
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_SETUP\n");
						if (authented)
						{
							if (RTSP_setup_respond(session, msg_rx, msg_tx) == 0)
							{
								msg_type = RTSP_MSG_ERROR; 
								dbgprintf(2, "RTSP_options_respond ERROR\n");
							}
						} else msg_type = 999;
						break; 
					}
					case RTSP_MSG_PLAY:
					{						
						if (authented)
						{
							AddModuleEventInList(uiSysID, 11, SYSTEM_CMD_OPENED_RTSP, NULL, 0, 0);
							if (session->video_enabled) AddModuleEventInList(uiSysID, 11, SYSTEM_CMD_OPENED_CAMERA, NULL, 0, 0);
							if (session->audio_enabled) AddModuleEventInList(uiSysID, 11, SYSTEM_CMD_OPENED_MIC, NULL, 0, 0);
							
							DBG_MUTEX_LOCK(&WEB_mutex);
							WEB_AddWebHistory(0, session->ip_str, 0, WEB_ACT_PLAY, session->Auth, session->Login);
							DBG_MUTEX_UNLOCK(&WEB_mutex);
	
							dbgprintf(5, "RTSP_io: RTSP_MSG_PLAY\n"); 
							if (RTSP_play_respond(session, msg_rx, msg_tx) == 0)
							{
								msg_type = RTSP_MSG_ERROR; 
								dbgprintf(2, "RTSP_options_respond ERROR\n");
							}
						} else msg_type = 999;
						break; 
					}
					case RTSP_MSG_TEARDOWN:
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_TEARDOWN\n"); 
						RTSP_teardown_respond(msg_rx, msg_tx);
						break; 
					}	
					case RTSP_MSG_GET_PARAMETER:
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_GET_PARAMETER\n"); 
						RTSP_parameter_respond(session, msg_rx, msg_tx);
						//RTSP_teardown_respond(msg_rx, msg_tx);
						//msg_type = 999;
						break; 
					}	
					case RTSP_MSG_SET_PARAMETER:
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_SET_PARAMETER\n"); 
						//RTSP_teardown_respond(msg_rx, msg_tx);
						msg_type = 999;
						break; 
					}
					case RTSP_MSG_DOLLAR:
					{
						//usSize = msg_rx[3];
						//usSize |= msg_rx[2] << 8;
						
						//printf("RTSP_io: RTSP_MSG_DOLLAR ch: %i len %i\n", msg_rx[1], usSize); 
						//printf("RTSP_io: RTSP_MSG_DOLLAR ch: %i\n", msg_rx[1]);
						//
						msg_type = 999;
						break; 
					}
					default:
					{
						dbgprintf(5, "RTSP_io: RTSP_MSG_UNKNOWN len %i\n", ret); 
						//printf("%s\n", msg_rx);
						//printf("RTSP_io: RTSP_MSG_UNKNOWN len %i\n", ret); 
						msg_type = 999; //RTSP_MSG_ERROR; 
					}
				}
					
				if(msg_type == RTSP_MSG_ERROR) 
				{
					dbgprintf(2, "RTSP_MSG_ERROR\n");
					//printf("%s\n", msg_rx);
					RTSP_teardown_respond(msg_rx, msg_tx);
					msg_type = RTSP_MSG_TEARDOWN;
					break;
				}
				if (msg_type == RTSP_MSG_PLAY)
				{
					status = 1;
					if (session->video_enabled == 1) RTSP_Activate_RTP_session(session, 0);
					if (session->audio_enabled == 1) 
					{
						ret = 3;
						switch(session->audio_list_dt[session->audio_list_pos].CodecNum)
						{
							case 0: //aac
							case 1: //mpa
							case 6: //mpa
							case 12: //mpa
							case 13: //MP2T
								ret = 1; 
								break;									
							case 14: //ac3
							case 18: //eac3
								ret = 2; 
								break;
							default: 
								ret = 3; 
								break;
						}
						RTSP_Activate_RTP_session(session, ret);	
					}						
					if (session->audio_enabled == 2) RTSP_Activate_RTP_session(session, 3);	
				}
				if (msg_type == RTSP_MSG_TEARDOWN)
				{
					dbgprintf(5, "RTSP_MSG_TEARDOWN \n"); 			
					break; 
				}
				//printf("R########################\n");
				//printf("%s\n", msg_rx); 
				//printf("########################\n");
				
				if (msg_type != 999)
				{
					ret = strlen(msg_tx);
					if (ret)
					{
						DBG_MUTEX_LOCK(&session->mutex);
						if ((session->OutSize - session->OutLen) > 0)
						{
							memcpy(&session->OutBuffer[session->OutLen], msg_tx, ret);
							session->OutLen += ret;
						}
						DBG_MUTEX_UNLOCK(&session->mutex);
					}
				}
			}
		}
		
		DBG_MUTEX_LOCK(&session->mutex);
		if ((iSocketWriteNotReady == 0) && (session->socket) && (session->OutLen != 0))
		{
			ret = write(session->socket, session->OutBuffer, session->OutLen);
			if (ret < 1) 
			{
				if (errno == EAGAIN) 
				{
					dbgprintf(5, "RTSP_io: write error (errno:%i, %s)\n", errno, strerror(errno));	
					iSocketWriteNotReady = 1;
					//printf("No data on %i\n", n);
				}
				else
				{
					//session->socket = 0;
					dbgprintf(4, "RTSP_io: Write TCP error. [client IP: %s]\n", session->ip_str); 
					DBG_MUTEX_UNLOCK(&session->mutex);
					break; //EXIT
				}								
			}
			else
			{
				session->OutLen -= ret;
				memmove(session->OutBuffer, &session->OutBuffer[ret], session->OutLen);
			}				
		}
		DBG_MUTEX_UNLOCK(&session->mutex);		
    }
	dbgprintf(4, "RTSP_io: Done. [client IP: %s]\n", session->ip_str); 
	
	DBG_MUTEX_LOCK(&session->mutex);
	if (status == 1)
	{
		int ivenb = session->video_enabled;
		int iaenb = session->audio_enabled;
		int ivsnum = session->rtp_v_session_num;
		int iasnum = session->rtp_a_session_num;
		
		DBG_MUTEX_UNLOCK(&session->mutex);
		if (ivenb == 1) RTSP_Teardown_RTP_session(ivsnum);
		if (iaenb) RTSP_Teardown_RTP_session(iasnum);
		DBG_MUTEX_LOCK(&session->mutex);
	}
					
	if (session->socket > 0) 
	{
		close(session->socket);
		session->socket = 0;
	}
	close(session->socketpair[0]);
	close(session->socketpair[1]);
	DBG_MUTEX_UNLOCK(&session->mutex);
	
	DBG_MUTEX_LOCK(&WEB_mutex);
	WEB_AddWebHistory(0, session->ip_str, 0, WEB_ACT_DISCONNECT, session->Auth, "");
	DBG_MUTEX_UNLOCK(&WEB_mutex);
	
	AddModuleEventInList(uiSysID, 11, SYSTEM_CMD_CLOSED_RTSP, NULL, 0, 0);
	if (session->video_enabled) AddModuleEventInList(uiSysID, 11, SYSTEM_CMD_CLOSED_CAMERA, NULL, 0, 0);
	if (session->audio_enabled) AddModuleEventInList(uiSysID, 11, SYSTEM_CMD_CLOSED_MIC, NULL, 0, 0);
							
	
	pthread_mutex_destroy(&session->mutex);	
	DBG_FREE(session->OutBuffer);
	if (session->audio_list_cnt) DBG_FREE(session->audio_list_dt);
	DBG_FREE(session);
	
	DBG_MUTEX_LOCK(&RTSP_mutex);
	cThreadRtspStatus--;
	DBG_MUTEX_UNLOCK(&RTSP_mutex);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)0;
}
