#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
//#include <alsa/asoundlib.h>

//#include <ao/ao.h>
//#include <mpg123.h>
#include "audio.h"
#include "omx_client.h"
#include "pthread2threadx.h"
#include "network.h"
#include "flv-muxer.h"
#include "gpio.h"
#include "weather_func.h"
#include "system.h"

#define BITS 							8
#define ALSA_PLAYBACK_BUFFER_MULTIPLY 	10
#define AUDIO_INBUF_SIZE   				20480
#define AUDIO_REFILL_THRESH   			4096
#define AVCODEC_MAX_AUDIO_FRAME_SIZE   	192000
#define MAX_SIZE_VIDEOPLAY_BUFFER		15000000
#define MAX_SIZE_AUDIOPLAY_BUFFER		5000000
#define MAX_PLAY_AUDIO_BLOCKS			600
#define MAX_PLAY_VIDEO_BLOCKS			60
#define MIN_FLV_BUFFER_SIZE				0x00500000

TX_EVENTER 	pevnt_audio_run;
TX_EVENTER 	pevnt_mplay_run;
TX_EVENTER 	pevnt_mcapt_run;
TX_EVENTER 	pevnt_mrec_run;
TX_EVENTER 	pevntRMV;
TX_EVENTER 	pevntRMA;

static pthread_t thread_capture_audio;
static pthread_t thread_play_audio;
static pthread_t thread_play_media;
static pthread_t thread_capture_sreams;
pthread_attr_t tattr_capture_audio;
pthread_attr_t tattr_play_audio;
pthread_attr_t tattr_play_media;
pthread_attr_t tattr_capture_sreams;
int iAudioStarted = 0;
int iPlaySoundCnt = 0;
int64_t iWaitPwrSoundTimer = 0;
//unsigned int uiWebVolume;
unsigned int uiMediaStepPos, uiMediaNewPos;
char cPlayCTLname[16];

char cThreadSoundsStatus;
pthread_mutex_t Sound_Mutex;
	
//misc_buffer *avFramePacket;
//misc_buffer *avSaveAudioPacket;
//misc_buffer *avSaveVideoPacket;

//int 					period_size;
//int 					audio_fd_count;
//struct pollfd 			*poll_fds; // file descriptors for polling audio

int audio_init()
{
	if (iAudioStarted != 0) return 0;
	
	//ao_initialize();
	//mpg123_init();
	memset(cPlayCTLname, 0, 16);
	strcpy(cPlayCTLname, "default");
	
	iAudioStarted = 1;
	iPlaySoundCnt = 0;	
	iWaitPwrSoundTimer = 0;
	uiMediaStepPos = 10;
	uiMediaNewPos = 0;
	//uiWebVolume = 0;
	uiNewMediaPos = -1;
	cThreadSoundsStatus = 0;
	
	pthread_mutex_init(&Play_Mutex, NULL);	
	pthread_mutex_init(&Sound_Mutex, NULL);	
	tx_eventer_create(&pevnt_mplay_run, 0);
	tx_eventer_create(&pevnt_mcapt_run, 0);
	tx_eventer_create(&pevnt_mrec_run, 0);
	tx_eventer_create(&pevnt_audio_run, 0);
	tx_eventer_create(&pevntRMV, 0);
	tx_eventer_create(&pevntRMA, 0);
	tx_eventer_create(&pevntRMS, 1);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_VIDEO);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_AUDIO);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_CODEC);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_NEXT);	
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_STOP);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_EXIT);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_EX_STOP);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_FORWARD);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_BACKWARD);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_AUDIO_SYNC);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_SET_NEW_POS);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_PLAY);
	tx_eventer_add_event(&pevntRMS, MEDIA_EVENT_PAUSE);
			
	pthread_attr_init(&tattr_capture_audio);
    pthread_attr_setdetachstate(&tattr_capture_audio, PTHREAD_CREATE_DETACHED);	
	pthread_attr_init(&tattr_play_media);
    pthread_attr_setdetachstate(&tattr_play_media, PTHREAD_CREATE_DETACHED);	
	pthread_attr_init(&tattr_play_audio);
    pthread_attr_setdetachstate(&tattr_play_audio, PTHREAD_CREATE_DETACHED);	
	pthread_attr_init(&tattr_capture_sreams);
    pthread_attr_setdetachstate(&tattr_capture_sreams, PTHREAD_CREATE_DETACHED);	
	return 1;
}

int audio_close(void)
{
	if (iAudioStarted != 1) return 0;
	//mpg123_exit();
    //ao_shutdown(); 
	
	DBG_MUTEX_LOCK(&Sound_Mutex);
	while (cThreadSoundsStatus != 0) 
	{
		DBG_MUTEX_UNLOCK(&Sound_Mutex);
		usleep(100000);
		DBG_MUTEX_LOCK(&Sound_Mutex);
	}
	DBG_MUTEX_UNLOCK(&Sound_Mutex);
	
	tx_eventer_recv(&pevnt_audio_run, NULL, TX_WAIT_FOREVER, 1);
	tx_eventer_delete(&pevnt_audio_run);
	tx_eventer_delete(&pevntRMV);
	tx_eventer_delete(&pevntRMA);
	tx_eventer_delete(&pevntRMS);
	tx_eventer_delete(&pevnt_mplay_run);
	tx_eventer_delete(&pevnt_mcapt_run);
	tx_eventer_delete(&pevnt_mrec_run);
	
	pthread_attr_destroy(&tattr_capture_sreams);
	pthread_attr_destroy(&tattr_play_media);
	pthread_attr_destroy(&tattr_play_audio);
	pthread_attr_destroy(&tattr_capture_audio);
	pthread_mutex_destroy(&Play_Mutex);
	pthread_mutex_destroy(&Sound_Mutex);
	iAudioStarted = 0;	
	return 1;
}

int CreateDirectoryFromPath(char* cPath, int iWithFile, mode_t mode)
{
	int iLen = strlen(cPath);
	int iLast = 0;
	int i;
	int resl = 0;
	int ret;
	char *path = (char*)DBG_MALLOC(iLen + 1);	
	char *name = (char*)DBG_MALLOC(iLen + 1);
	
	memcpy(path, cPath, iLen);
	path[iLen] = 0;
	if (path[iLen - 1] == 47) 
	{
		path[iLen - 1] = 0;
		iWithFile = 0;
	}
	
	for (i = 0; i < iLen; i++)
	{
		if ((path[i] != 47) || (i == 0)) 
		{
			iLast = i;
		}
		if (((path[i] == 47) && (i != 0)) || ((i == (iLen - 1)) && !iWithFile))
		{
			memset(name, 0, iLen + 1);	
			memcpy(name, path, iLast + 1);
			ret = mkdir(name, mode);
			//printf("Create dir %i '%s'\n", ret, name);
			if (ret == 0) resl++;
		}
	}
	
	DBG_FREE(path);
	DBG_FREE(name);
	return resl;
}

char* GetMonthName(int iMonth)
{
	switch(iMonth)
	{
		case 1: return "01_JAN";
		case 2: return "02_FEB";
		case 3: return "03_MAR";
		case 4: return "04_APR";
		case 5: return "05_MAY";
		case 6: return "06_JUN";
		case 7: return "07_JUL";
		case 8: return "08_AUG";
		case 9: return "09_SEP";
		case 10: return "10_OKT";
		case 11: return "11_NOV";
		case 12: return "12_DEC";
		default: return "00_Unknown";
	}
	return "0_Unknown";
}

void GetPlayBufferStatus(unsigned int *uiVideo, unsigned int *uiAudio, unsigned int *uiPosition)
{
	DBG_MUTEX_LOCK(&Play_Mutex);
	if (uiVideo) *uiVideo = uiVideoDataCnt;
	if (uiAudio) *uiAudio = uiAudioDataCnt;
	if (uiPosition) *uiPosition = uiNewMediaPos;
	DBG_MUTEX_UNLOCK(&Play_Mutex);
}

void SetAudioPlayDeviceName(int iDev)
{
	DBG_MUTEX_LOCK(&Play_Mutex);
	memset(cPlayCTLname, 0, 16);
	sprintf(cPlayCTLname, "hw:%i",iDev);
	DBG_MUTEX_UNLOCK(&Play_Mutex);
}

char TestDataChanged(time_t *tLastDate)
{
	char result = 0;
	time_t currtime;
	time(&currtime);
		
	if (abs(tLastDate[0] - currtime) > 3600) result = 1;
	memcpy(tLastDate, &currtime, sizeof(time_t));
	return result;
}

int Audio_IsFree()
{
	if (iAudioStarted == 0) return 2;	
	if (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_BUSY) == 0) return 1; else return 0;
}

int MediaRecord_IsFree(void* vStream)
{
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;
	if (iAudioStarted == 0) return 2;	
	if (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_BUSY_REC) == 0) return 1; else return 0;
}

int CaptureVideo_IsFree()
{
	if (iAudioStarted == 0) return 2;	
	if (tx_eventer_test_event(&pevnt_mrec_run, CAPTURE_EVENT_VIDEO) == 0) return 1; else return 0;
}

int CaptureAudio_IsFree()
{
	if (iAudioStarted == 0) return 2;	
	if (tx_eventer_test_event(&pevnt_mrec_run, CAPTURE_EVENT_AUDIO) == 0) return 1; else return 0;
}

int MediaPlay_IsFree()
{
	if (iAudioStarted == 0) return 2;	
	if (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) == 0) return 1; else return 0;
}

int Audio_Stop(int iWaitStop)
{
	if (iAudioStarted == 0) return -1;
	if (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_BUSY) != 0)
	{
		tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_STOP);
		//if (iWaitStop) tx_eventer_recv(&pevnt_audio_run, NULL, TX_WAIT_FOREVER, 1);
		if (iWaitStop) while (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_BUSY) != 0) usleep(10000);		
	}
	return 1;
}

int Audio_Play()
{
	if (iAudioStarted == 0) return -1;
	if (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_BUSY) != 0)
	{
		tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_PLAY);		
	}
	return 1;
}

int Audio_Pause()
{
	if (iAudioStarted == 0) return -1;
	if (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_BUSY) != 0)
	{
		tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_PAUSE);		
	}
	return 1;
}

int Media_Pause()
{
	int ret = 0;
	if (iAudioStarted == 0) return -1;
	if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
		&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
	{
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_PAUSE);
		ret = 1;
	}
	return ret;
}

int Media_Play()
{
	int ret = 0;
	if (iAudioStarted == 0) return -1;
	if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
		&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
	{
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_PLAY);
		ret = 1;
	}
	return ret;
}

int Media_Forward(unsigned int iDiffPos)
{
	int ret = 0;
	if (iAudioStarted == 0) return -1;
	if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
		&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
	{
		DBG_MUTEX_LOCK(&Play_Mutex);
		uiMediaStepPos = iDiffPos;
		DBG_MUTEX_UNLOCK(&Play_Mutex);
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_FORWARD);
		ret = 1;
	}
	return ret;
}

int Media_Backward(unsigned int iDiffPos)
{
	int ret = 0;
	if (iAudioStarted == 0) return -1;
	if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
		&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
	{
		DBG_MUTEX_LOCK(&Play_Mutex);
		uiMediaStepPos = iDiffPos;
		DBG_MUTEX_UNLOCK(&Play_Mutex);
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_BACKWARD);
		ret = 1;
	}
	return ret;
}

int Media_SetNewPos(unsigned int uiPos)
{
	int ret = 0;
	if (iAudioStarted == 0) return -1;
	if (uiPos < 1000)
	{
		if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
			&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
		{
			DBG_MUTEX_LOCK(&Play_Mutex);
			uiMediaNewPos = uiPos;
			DBG_MUTEX_UNLOCK(&Play_Mutex);
			tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_SET_NEW_POS);
			ret = 1;
		}
	}
	return ret;
}

int Media_StopPlay(int iWaitStop)
{
	if (iAudioStarted == 0) return -1;
	if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
		&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
	{
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_EX_STOP);
		if (iWaitStop) while (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0) usleep(10000);
	}
	return 1;
}

int Audio_StopCapture(int iWaitStop)
{
	if (iAudioStarted == 0) return -1;
	if (tx_eventer_test_event(&pevnt_mcapt_run, CAPTURE_EVENT_STOP) != 0)
	{
		tx_eventer_delete_event(&pevnt_mcapt_run, CAPTURE_EVENT_STOP);
		//if (iWaitStop) tx_eventer_recv(&pevnt_mcapt_run, NULL, TX_WAIT_FOREVER, 1);
		if (iWaitStop)
		{
			int ret = 1;
			while (ret) 
			{
				usleep(10000);
				DBG_MUTEX_LOCK(&system_mutex);
				ret = cThreadAudCaptStatus;
				DBG_MUTEX_UNLOCK(&system_mutex); 
			}
		}
	}
	return 1;
}

int Audio_MuteCapture(char cMute)
{
	if (iAudioStarted == 0) return -1;
	int iEvent = CAPTURE_EVENT_MUTE_ON;
	if (cMute == 0) iEvent = CAPTURE_EVENT_MUTE_OFF;
		
	if (tx_eventer_test_event(&pevnt_mcapt_run, iEvent) != 0)
		tx_eventer_delete_event(&pevnt_mcapt_run, iEvent);
		else return 0;		
	return 1;
}

int Audio_UnMuteCapture()
{
	if (iAudioStarted == 0) return -1;
	if (tx_eventer_test_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_OFF) != 0)
		tx_eventer_delete_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_OFF);
		else return 0;		
	return 1;
}

int Media_StopRec(int iWaitStop)
{
	if (iAudioStarted == 0) return -1;
	int n;
	DBG_MUTEX_LOCK(&system_mutex);
	int ui = iStreamCnt;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	for (n = 0; n < ui; n++)
	{
		DBG_MUTEX_LOCK(&SyncStream[n]->mutex);
	
		if (tx_eventer_test_event(&SyncStream[n]->run, MEDIA_EVENT_BUSY_REC) != 0)
		{
			tx_eventer_send_event(&SyncStream[n]->run, MEDIA_EVENT_STOP_REC);
			tx_eventer_send_event(&SyncStream[n]->pevntS, EVENT_START);
			if (SyncStream[n]->VidID) 
			{
				tx_eventer_send_data(&SyncStream[n]->pevntV, EVENT_VIDEO_INFO_DATA, NULL, 0, 0, 0);
				tx_eventer_send_data(&SyncStream[n]->pevntV, EVENT_START_VIDEO_FRAME_DATA, NULL, 0, 0, 0);
			}
			if (SyncStream[n]->AudID) tx_eventer_send_data(&SyncStream[n]->pevntA, EVENT_AUDIO_CODEC_INFO_DATA, NULL, 0, 0, 0);			
		}
		DBG_MUTEX_UNLOCK(&SyncStream[n]->mutex);	
	}	
	
	int res = 0;	
	do
	{
		res = 0;	
		for (n = 0; n < ui; n++)
			if (tx_eventer_test_event(&SyncStream[n]->run, MEDIA_EVENT_BUSY_REC) != 0) {res = 1; break;}			
		if (res) usleep(10000);
	} while (res);
	return 1;
}

int Media_NextRecID(unsigned int uiModuleID, unsigned int uiType, unsigned int uiCmd)
{
	if (iAudioStarted == 0) return -1;
	
	unsigned int n;
	DBG_MUTEX_LOCK(&system_mutex);
	int ui = iStreamCnt;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	for(n = 0; n < ui; n++)
	{
		DBG_MUTEX_LOCK(&SyncStream[n]->mutex);
	
		if ((SyncStream[n]->Type == uiType)
			&&
			((SyncStream[n]->VidID == uiModuleID)
			|| (SyncStream[n]->AudID == uiModuleID)
			|| (uiModuleID == 0)))
			{
				tx_eventer_send_event(&SyncStream[n]->run, MEDIA_EVENT_NEXT_REC + uiCmd);
				tx_eventer_send_event(&SyncStream[n]->pevntS, EVENT_START);
				if (SyncStream[n]->VidID) 
				{
					tx_eventer_send_data(&SyncStream[n]->pevntV, EVENT_VIDEO_INFO_DATA, NULL, 0, 0, 0);
					tx_eventer_send_data(&SyncStream[n]->pevntV, EVENT_START_VIDEO_FRAME_DATA, NULL, 0, 0, 0);
				}
				if (SyncStream[n]->AudID) tx_eventer_send_data(&SyncStream[n]->pevntA, EVENT_AUDIO_CODEC_INFO_DATA, NULL, 0, 0, 0);				
			}
		DBG_MUTEX_UNLOCK(&SyncStream[n]->mutex);
	}
	
	return 1;
}

int Media_NextRecNum(unsigned int uiNum, unsigned int uiType, unsigned int uiCmd)
{
	if (iAudioStarted == 0) return -1;
	
	DBG_MUTEX_LOCK(&system_mutex);
	int ui = iStreamCnt;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	if (uiNum < ui)
	{
		DBG_MUTEX_LOCK(&SyncStream[uiNum]->mutex);
		tx_eventer_send_event(&SyncStream[uiNum]->run, MEDIA_EVENT_NEXT_REC + uiCmd);
		tx_eventer_send_event(&SyncStream[uiNum]->pevntS, EVENT_START);
		if (SyncStream[uiNum]->VidID) 
		{
			tx_eventer_send_data(&SyncStream[uiNum]->pevntV, EVENT_VIDEO_INFO_DATA, NULL, 0, 0, 0);
			tx_eventer_send_data(&SyncStream[uiNum]->pevntV, EVENT_START_VIDEO_FRAME_DATA, NULL, 0, 0, 0);
		}
		if (SyncStream[uiNum]->AudID) tx_eventer_send_data(&SyncStream[uiNum]->pevntA, EVENT_AUDIO_CODEC_INFO_DATA, NULL, 0, 0, 0);
		DBG_MUTEX_UNLOCK(&SyncStream[uiNum]->mutex);
	}
	
	return 1;
}

/*unsigned int audio_get_volume()
{
	unsigned int ret;
	DBG_MUTEX_LOCK(&Play_Mutex);
	ret = uiWebVolume;
	DBG_MUTEX_UNLOCK(&Play_Mutex);
	
	return ret;
}*/
/*
static int show_selem(snd_mixer_t *handle, snd_mixer_selem_id_t *id, const char *space, int level)
{
	snd_mixer_selem_channel_id_t chn;
	long pmin = 0, pmax = 0;
	long cmin = 0, cmax = 0;
	long pvol, cvol;
	int psw, csw;
	int pmono, cmono, mono_ok = 0;
	long db;
	snd_mixer_elem_t *elem;
	
	elem = snd_mixer_find_selem(handle, id);
	if (!elem) {
		error("Mixer %s simple element not found", card);
		return -ENOENT;
	}

	if (level & LEVEL_BASIC) {
		printf("%sCapabilities:", space);
		if (snd_mixer_selem_has_common_volume(elem)) {
			printf(" volume");
			if (snd_mixer_selem_has_playback_volume_joined(elem))
				printf(" volume-joined");
		} else {
			if (snd_mixer_selem_has_playback_volume(elem)) {
				printf(" pvolume");
				if (snd_mixer_selem_has_playback_volume_joined(elem))
					printf(" pvolume-joined");
			}
			if (snd_mixer_selem_has_capture_volume(elem)) {
				printf(" cvolume");
				if (snd_mixer_selem_has_capture_volume_joined(elem))
					printf(" cvolume-joined");
			}
		}
		if (snd_mixer_selem_has_common_switch(elem)) {
			printf(" switch");
			if (snd_mixer_selem_has_playback_switch_joined(elem))
				printf(" switch-joined");
		} else {
			if (snd_mixer_selem_has_playback_switch(elem)) {
				printf(" pswitch");
				if (snd_mixer_selem_has_playback_switch_joined(elem))
					printf(" pswitch-joined");
			}
			if (snd_mixer_selem_has_capture_switch(elem)) {
				printf(" cswitch");
				if (snd_mixer_selem_has_capture_switch_joined(elem))
					printf(" cswitch-joined");
				if (snd_mixer_selem_has_capture_switch_exclusive(elem))
					printf(" cswitch-exclusive");
			}
		}
		if (snd_mixer_selem_is_enum_playback(elem)) {
			printf(" penum");
		} else if (snd_mixer_selem_is_enum_capture(elem)) {
			printf(" cenum");
		} else if (snd_mixer_selem_is_enumerated(elem)) {
			printf(" enum");
		}
		printf("\n");
		if (snd_mixer_selem_is_enumerated(elem)) {
			int i, items;
			unsigned int idx;
			char itemname[40];
			items = snd_mixer_selem_get_enum_items(elem);
			printf("  Items:");
			for (i = 0; i < items; i++) {
				snd_mixer_selem_get_enum_item_name(elem, i, sizeof(itemname) - 1, itemname);
				printf(" '%s'", itemname);
			}
			printf("\n");
			for (i = 0; !snd_mixer_selem_get_enum_item(elem, i, &idx); i++) {
				snd_mixer_selem_get_enum_item_name(elem, idx, sizeof(itemname) - 1, itemname);
				printf("  Item%d: '%s'\n", i, itemname);
			}
			return 0; // no more thing to do
		}
		if (snd_mixer_selem_has_capture_switch_exclusive(elem))
			printf("%sCapture exclusive group: %i\n", space,
			       snd_mixer_selem_get_capture_group(elem));
		if (snd_mixer_selem_has_playback_volume(elem) ||
		    snd_mixer_selem_has_playback_switch(elem)) {
			printf("%sPlayback channels:", space);
			if (snd_mixer_selem_is_playback_mono(elem)) {
				printf(" Mono");
			} else {
				int first = 1;
				for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++){
					if (!snd_mixer_selem_has_playback_channel(elem, chn))
						continue;
					if (!first)
						printf(" -");
					printf(" %s", snd_mixer_selem_channel_name(chn));
					first = 0;
				}
			}
			printf("\n");
		}
		if (snd_mixer_selem_has_capture_volume(elem) ||
		    snd_mixer_selem_has_capture_switch(elem)) {
			printf("%sCapture channels:", space);
			if (snd_mixer_selem_is_capture_mono(elem)) {
				printf(" Mono");
			} else {
				int first = 1;
				for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++){
					if (!snd_mixer_selem_has_capture_channel(elem, chn))
						continue;
					if (!first)
						printf(" -");
					printf(" %s", snd_mixer_selem_channel_name(chn));
					first = 0;
				}
			}
			printf("\n");
		}
		if (snd_mixer_selem_has_playback_volume(elem) ||
		    snd_mixer_selem_has_capture_volume(elem)) {
			printf("%sLimits:", space);
			if (snd_mixer_selem_has_common_volume(elem)) {
				snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
				snd_mixer_selem_get_capture_volume_range(elem, &cmin, &cmax);
				printf(" %li - %li", pmin, pmax);
			} else {
				if (snd_mixer_selem_has_playback_volume(elem)) {
					snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
					printf(" Playback %li - %li", pmin, pmax);
				}
				if (snd_mixer_selem_has_capture_volume(elem)) {
					snd_mixer_selem_get_capture_volume_range(elem, &cmin, &cmax);
					printf(" Capture %li - %li", cmin, cmax);
				}
			}
			printf("\n");
		}
		pmono = snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_MONO) &&
		        (snd_mixer_selem_is_playback_mono(elem) || 
			 (!snd_mixer_selem_has_playback_volume(elem) &&
			  !snd_mixer_selem_has_playback_switch(elem)));
		cmono = snd_mixer_selem_has_capture_channel(elem, SND_MIXER_SCHN_MONO) &&
		        (snd_mixer_selem_is_capture_mono(elem) || 
			 (!snd_mixer_selem_has_capture_volume(elem) &&
			  !snd_mixer_selem_has_capture_switch(elem)));
#if 0
		printf("pmono = %i, cmono = %i (%i, %i, %i, %i)\n", pmono, cmono,
				snd_mixer_selem_has_capture_channel(elem, SND_MIXER_SCHN_MONO),
				snd_mixer_selem_is_capture_mono(elem),
				snd_mixer_selem_has_capture_volume(elem),
				snd_mixer_selem_has_capture_switch(elem));
#endif
		if (pmono || cmono) {
			if (!mono_ok) {
				printf("%s%s:", space, "Mono");
				mono_ok = 1;
			}
			if (snd_mixer_selem_has_common_volume(elem)) {
				snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &pvol);
				printf(" %s", get_percent(pvol, pmin, pmax));
				if (!snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_MONO, &db)) {
					printf(" [");
					print_dB(db);
					printf("]");
				}
			}
			if (snd_mixer_selem_has_common_switch(elem)) {
				snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &psw);
				printf(" [%s]", psw ? "on" : "off");
			}
		}
		if (pmono && snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_MONO)) {
			int title = 0;
			if (!mono_ok) {
				printf("%s%s:", space, "Mono");
				mono_ok = 1;
			}
			if (!snd_mixer_selem_has_common_volume(elem)) {
				if (snd_mixer_selem_has_playback_volume(elem)) {
					printf(" Playback");
					title = 1;
					snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &pvol);
					printf(" %s", get_percent(pvol, pmin, pmax));
					if (!snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_MONO, &db)) {
						printf(" [");
						print_dB(db);
						printf("]");
					}
				}
			}
			if (!snd_mixer_selem_has_common_switch(elem)) {
				if (snd_mixer_selem_has_playback_switch(elem)) {
					if (!title)
						printf(" Playback");
					snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &psw);
					printf(" [%s]", psw ? "on" : "off");
				}
			}
		}
		if (cmono && snd_mixer_selem_has_capture_channel(elem, SND_MIXER_SCHN_MONO)) {
			int title = 0;
			if (!mono_ok) {
				printf("%s%s:", space, "Mono");
				mono_ok = 1;
			}
			if (!snd_mixer_selem_has_common_volume(elem)) {
				if (snd_mixer_selem_has_capture_volume(elem)) {
					printf(" Capture");
					title = 1;
					snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_MONO, &cvol);
					printf(" %s", get_percent(cvol, cmin, cmax));
					if (!snd_mixer_selem_get_capture_dB(elem, SND_MIXER_SCHN_MONO, &db)) {
						printf(" [");
						print_dB(db);
						printf("]");
					}
				}
			}
			if (!snd_mixer_selem_has_common_switch(elem)) {
				if (snd_mixer_selem_has_capture_switch(elem)) {
					if (!title)
						printf(" Capture");
					snd_mixer_selem_get_capture_switch(elem, SND_MIXER_SCHN_MONO, &csw);
					printf(" [%s]", csw ? "on" : "off");
				}
			}
		}
		if (pmono || cmono)
			printf("\n");
		if (!pmono || !cmono) {
			for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
				if ((pmono || !snd_mixer_selem_has_playback_channel(elem, chn)) &&
				    (cmono || !snd_mixer_selem_has_capture_channel(elem, chn)))
					continue;
				printf("%s%s:", space, snd_mixer_selem_channel_name(chn));
				if (!pmono && !cmono && snd_mixer_selem_has_common_volume(elem)) {
					snd_mixer_selem_get_playback_volume(elem, chn, &pvol);
					printf(" %s", get_percent(pvol, pmin, pmax));
					if (!snd_mixer_selem_get_playback_dB(elem, chn, &db)) {
						printf(" [");
						print_dB(db);
						printf("]");
					}
				}
				if (!pmono && !cmono && snd_mixer_selem_has_common_switch(elem)) {
					snd_mixer_selem_get_playback_switch(elem, chn, &psw);
					printf(" [%s]", psw ? "on" : "off");
				}
				if (!pmono && snd_mixer_selem_has_playback_channel(elem, chn)) {
					int title = 0;
					if (!snd_mixer_selem_has_common_volume(elem)) {
						if (snd_mixer_selem_has_playback_volume(elem)) {
							printf(" Playback");
							title = 1;
							snd_mixer_selem_get_playback_volume(elem, chn, &pvol);
							printf(" %s", get_percent(pvol, pmin, pmax));
							if (!snd_mixer_selem_get_playback_dB(elem, chn, &db)) {
								printf(" [");
								print_dB(db);
								printf("]");
							}
						}
					}
					if (!snd_mixer_selem_has_common_switch(elem)) {
						if (snd_mixer_selem_has_playback_switch(elem)) {
							if (!title)
								printf(" Playback");
							snd_mixer_selem_get_playback_switch(elem, chn, &psw);
							printf(" [%s]", psw ? "on" : "off");
						}
					}
				}
				if (!cmono && snd_mixer_selem_has_capture_channel(elem, chn)) {
					int title = 0;
					if (!snd_mixer_selem_has_common_volume(elem)) {
						if (snd_mixer_selem_has_capture_volume(elem)) {
							printf(" Capture");
							title = 1;
							snd_mixer_selem_get_capture_volume(elem, chn, &cvol);
							printf(" %s", get_percent(cvol, cmin, cmax));
							if (!snd_mixer_selem_get_capture_dB(elem, chn, &db)) {
								printf(" [");
								print_dB(db);
								printf("]");
							}
						}
					}
					if (!snd_mixer_selem_has_common_switch(elem)) {
						if (snd_mixer_selem_has_capture_switch(elem)) {
							if (!title)
								printf(" Capture");
							snd_mixer_selem_get_capture_switch(elem, chn, &csw);
							printf(" [%s]", csw ? "on" : "off");
						}
					}
				}
				printf("\n");
			}
		}
	}
	return 0;
}

static int selems(int level)
{
	int err;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_alloca(&sid);
	
	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		error("Mixer %s open error: %s", card, snd_strerror(err));
		return err;
	}
	if (smixer_level == 0 && (err = snd_mixer_attach(handle, card)) < 0) {
		error("Mixer attach %s error: %s", card, snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}
	if ((err = snd_mixer_selem_register(handle, smixer_level > 0 ? &smixer_options : NULL, NULL)) < 0) {
		error("Mixer register error: %s", snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}
	err = snd_mixer_load(handle);
	if (err < 0) {
		error("Mixer %s load error: %s", card, snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}
	for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
		snd_mixer_selem_get_id(elem, sid);
		if (!(level & LEVEL_INACTIVE) && !snd_mixer_selem_is_active(elem))
			continue;
		printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		show_selem(handle, sid, "  ", level);
	}
	snd_mixer_close(handle);
	return 0;
}
*/
void audio_set_playback_volume(int iDev, int iVolume)
{
	char cDevName[16];
		
	if (iDev == -1)
	{
		DBG_MUTEX_LOCK(&Play_Mutex);
		//uiWebVolume = iVolume;
		memcpy(cDevName, cPlayCTLname, 16);
		DBG_MUTEX_UNLOCK(&Play_Mutex);
	}
	else
	{
		memset(cDevName, 0, 16);
		sprintf(cDevName, "hw:%i",iDev);
	}
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	long midlevolume = 0;
	long min, max;
	
	dbgprintf(5, "audio_set_playback_volume:'%s' >>> %i\n", cDevName, iVolume);
	
	// Open an empty mixer
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	// Load the mixer elements
	snd_mixer_load(m_handle);

	// Configure the simple element we are looking for
	//snd_mixer_selem_id_t *simpleElemId; // mixer simple element
	//snd_mixer_selem_id_alloca(&simpleElemId);
	//snd_mixer_selem_id_set_index(simpleElemId, 0);	
	//snd_mixer_selem_id_set_name(simpleElemId, audio_device_name);
	//m_elem = snd_mixer_find_selem(m_handle, simpleElemId);

	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		//show_selem(m_handle, sid, "  ", 0);
		//printf("%i %i %i\n", snd_mixer_selem_is_active(m_elem), snd_mixer_selem_is_enum_playback(m_elem), snd_mixer_selem_has_playback_switch(m_elem));
		//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		
        if (snd_mixer_selem_is_active(m_elem) && (snd_mixer_selem_is_enum_playback(m_elem) || snd_mixer_selem_has_playback_switch(m_elem))) break;
			//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        //show_selem(m_handle, sid, "  ", level);
    }
	if (m_elem)
	{
		// Here is set the global system volume
		snd_mixer_selem_get_playback_volume_range(m_elem, &min, &max);
		midlevolume = max - min;
		//printf("min %i, max %i, mid %i, perc %i, vol %i\n", min, max, midlevolume, min + (midlevolume * iVolume / 100), iVolume);
		if (snd_mixer_selem_set_playback_volume_all(m_elem, min + (midlevolume * iVolume / 100)) != 0) 
			dbgprintf(2, "snd_mixer_selem_set_playback_volume_all ERROR\n");
	} else dbgprintf(2, "audio_set_volume: Not find audio playback device\n");
	
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
	
	/*long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, iVolume * max / 100);

    snd_mixer_close(handle);*/
}

void audio_set_capture_volume(int iDev, int iVolume)
{
	char cDevName[16];
	memset(cDevName, 0, 16);
	sprintf(cDevName, "hw:%i",iDev);	
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	long midlevolume = 0;
	long min, max;
	// Open an empty mixer
	dbgprintf(5, "audio_set_capture_volume:'%s' >>> %i\n", cDevName, iVolume);
	
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	// Load the mixer elements
	snd_mixer_load(m_handle);
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		//show_selem(m_handle, sid, "  ", 0);
		//printf("%i %i %i\n", snd_mixer_selem_is_active(m_elem), snd_mixer_selem_is_enum_capture(m_elem), snd_mixer_selem_has_capture_switch(m_elem));
		//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		
        if (snd_mixer_selem_is_active(m_elem) && (snd_mixer_selem_is_enum_capture(m_elem) || snd_mixer_selem_has_capture_switch(m_elem))) break;
			//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        //show_selem(m_handle, sid, "  ", level);
    }
	if (m_elem)
	{
		// Here is set the global system volume
		snd_mixer_selem_get_capture_volume_range(m_elem, &min, &max);
		midlevolume = max - min;
		//printf("min %i, max %i, mid %i, perc %i, vol %i\n", min, max, midlevolume, min + (midlevolume * iVolume / 100), iVolume);
		if (snd_mixer_selem_set_capture_volume_all(m_elem, min + (midlevolume * iVolume / 100)) != 0) 
			dbgprintf(2, "snd_mixer_selem_set_capture_volume_all ERROR\n");
	} else dbgprintf(2, "audio_set_volume: Not find audio capture device\n");
	
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
}

int audio_get_playback_volume()
{
	int res = -1;
	char cDevName[16];
	DBG_MUTEX_LOCK(&Play_Mutex);
	memcpy(cDevName, cPlayCTLname, 16);
	DBG_MUTEX_UNLOCK(&Play_Mutex);
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	long midlevolume = 0;
	long min, max;
	// Open an empty mixer
	dbgprintf(5, "audio_get_playback_volume:'%s'\n", cDevName);
	
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	// Load the mixer elements
	snd_mixer_load(m_handle);
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		//show_selem(m_handle, sid, "  ", 0);
		//printf("%i %i %i\n", snd_mixer_selem_is_active(m_elem), snd_mixer_selem_is_enum_capture(m_elem), snd_mixer_selem_has_capture_switch(m_elem));
		//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		
        if (snd_mixer_selem_is_active(m_elem) && (snd_mixer_selem_is_enum_playback(m_elem) || snd_mixer_selem_has_playback_switch(m_elem))) break;
			//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        //show_selem(m_handle, sid, "  ", level);
    }
	if (m_elem)
	{
		// Here is set the global system volume
		snd_mixer_selem_get_playback_volume_range(m_elem, &min, &max);
		midlevolume = max - min;
		//printf("min %i, max %i, mid %i, perc %i, vol %i\n", min, max, midlevolume, min + (midlevolume * iVolume / 100), iVolume);
		long ret = 0;
		if (snd_mixer_selem_get_playback_volume(m_elem, 0, &ret) != 0) 
			dbgprintf(2, "snd_mixer_selem_get_playback_volume ERROR\n");
			else
			{
				if ((ret - min) == 0) res = 0; else res = (int)((float)(ret - min) / ((float)(midlevolume) / 100));
			}
	} else dbgprintf(2, "audio_set_volume: Not find audio playback device\n");
	
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
	
	dbgprintf(5, "audio_get_playback_volume:'%s' >>> %i\n", cDevName, res);
	
	return (int)res;
}

int audio_get_capture_switch(int iDev)
{
	int res = -1;
	char cDevName[16];
	memset(cDevName, 0, 16);
	sprintf(cDevName, "hw:%i",iDev);	
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	int csw, chn;
	// Load the mixer elements
	snd_mixer_load(m_handle);
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		//show_selem(m_handle, sid, "  ", 0);
		printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		printf("\n%i %i %i %i %i %i\n", snd_mixer_selem_is_active(m_elem), 
							snd_mixer_selem_is_enum_capture(m_elem), 
							snd_mixer_selem_has_capture_switch(m_elem),
							snd_mixer_selem_has_common_switch(m_elem),
							snd_mixer_selem_has_capture_switch_joined(m_elem),
							snd_mixer_selem_has_capture_switch_exclusive(m_elem));
		
		/*if (snd_mixer_selem_has_common_switch(m_elem)) 
		{
			printf(" Common\n");					
			for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) 
			{				
				snd_mixer_selem_get_common_switch(m_elem, chn, &csw);
				printf(" [%s]\n", csw ? "on" : "off");
			}	
		}*/
		if (snd_mixer_selem_has_capture_switch(m_elem)) 
		{
			printf(" Capture\n");					
			for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) 
			{				
				snd_mixer_selem_get_capture_switch(m_elem, chn, &csw);
				printf(" [%s]\n", csw ? "on" : "off");
			}
		}
		snd_mixer_selem_set_capture_switch_all(m_elem, 1);
		/*if ((snd_mixer_selem_has_capture_switch(m_elem)) && (snd_mixer_selem_has_playback_switch(m_elem)))
		{
			printf(" Playback\n");					
			for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) 
			{				
				snd_mixer_selem_get_playback_switch(m_elem, chn, &csw);
				printf(" [%s]\n", csw ? "on" : "off");
				snd_mixer_selem_set_playback_switch_all(m_elem, 1);
			}
		}*/
		//if (snd_mixer_selem_is_active(m_elem) && (snd_mixer_selem_is_enum_capture(m_elem) || snd_mixer_selem_has_capture_switch(m_elem))) break;
			//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        //show_selem(m_handle, sid, "  ", level);
    }
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
	
	return (int)res;
}

int audio_get_capture_agc(int iDev)
{
	int res = -1;
	char cDevName[64];
	memset(cDevName, 0, 64);
	sprintf(cDevName, "hw:%i",iDev);	
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	int csw;
	// Load the mixer elements
	snd_mixer_load(m_handle);
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		memset(cDevName, 0, 64);
		if (strlen(snd_mixer_selem_id_get_name(sid)) < 64)
			strcpy(cDevName, snd_mixer_selem_id_get_name(sid)); 
			else
			memcpy(cDevName, snd_mixer_selem_id_get_name(sid), 63); 
		if ((SearchStrInDataCaseIgn(cDevName, strlen(cDevName), 0, "AGC") > 0) ||
			((SearchStrInDataCaseIgn(cDevName, strlen(cDevName), 0, "GAIN") > 0) &&
			(SearchStrInDataCaseIgn(cDevName, strlen(cDevName), 0, "CONTROL") > 0)))
		{
			if (snd_mixer_selem_has_capture_switch(m_elem)) 
			{
				snd_mixer_selem_get_capture_switch(m_elem, 0, &csw);
				if (csw) res = 1; else res = 0;
				//printf("snd_mixer_selem_Get_capture_switch %i\n", res);
				break;
			}
			if (snd_mixer_selem_has_playback_switch(m_elem))
			{
				snd_mixer_selem_get_playback_switch(m_elem, 0, &csw);
				if (csw) res = 1; else res = 0;
				//printf("snd_mixer_selem_Get_playback_switch %i\n", res);
				break;
			}
			if (snd_mixer_selem_has_common_switch(m_elem)) 
			{
				snd_mixer_selem_get_capture_switch(m_elem, 0, &csw);
				if (csw) res = 1; else res = 0;
				//printf("snd_mixer_selem_Get_common_switch %i\n", res);
				break;
			}			
		}
    }
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
	
	return (int)res;
}

int audio_set_capture_agc(int iDev, int iVal)
{
	int res = 0;
	char cDevName[64];
	memset(cDevName, 0, 64);
	sprintf(cDevName, "hw:%i",iDev);	
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	// Load the mixer elements
	snd_mixer_load(m_handle);
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		memset(cDevName, 0, 64);
		if (strlen(snd_mixer_selem_id_get_name(sid)) < 64)
			strcpy(cDevName, snd_mixer_selem_id_get_name(sid)); 
			else
			memcpy(cDevName, snd_mixer_selem_id_get_name(sid), 63); 
		if ((SearchStrInDataCaseIgn(cDevName, strlen(cDevName), 0, "AGC") > 0) ||
			((SearchStrInDataCaseIgn(cDevName, strlen(cDevName), 0, "GAIN") > 0) &&
			(SearchStrInDataCaseIgn(cDevName, strlen(cDevName), 0, "CONTROL") > 0)))
		{
			if (snd_mixer_selem_has_common_switch(m_elem)) 
			{
				snd_mixer_selem_set_capture_switch(m_elem, 0, iVal ? 1 : 0);
				snd_mixer_selem_set_playback_switch(m_elem, 0, iVal ? 1 : 0);
				//printf("snd_mixer_selem_Set_common_switch %i\n", iVal);
				res = 1;
				break;
			}
			if (snd_mixer_selem_has_capture_switch(m_elem)) 
			{
				snd_mixer_selem_set_capture_switch(m_elem, 0, iVal ? 1 : 0);
				//printf("snd_mixer_selem_Set_capture_switch %i\n", iVal);
				res = 1;
				break;
			}
			if (snd_mixer_selem_has_playback_switch(m_elem))
			{
				snd_mixer_selem_set_playback_switch(m_elem, 0, iVal ? 1 : 0);
				//printf("snd_mixer_selem_Set_playback_switch %i\n", iVal);
				res = 1;
				break;
			}						
		}
    }
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
	return (int)res;
}

int audio_get_capture_volume(int iDev)
{
	int res = -1;
	char cDevName[16];
	memset(cDevName, 0, 16);
	sprintf(cDevName, "hw:%i",iDev);	
	
	// ALSA mixer handle
	snd_mixer_t *m_handle;
	snd_mixer_elem_t* m_elem;
	long midlevolume = 0;
	long min, max;
	// Open an empty mixer
	dbgprintf(5, "audio_get_capture_volume:'%s'\n", cDevName);
	
	snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE);
	if (snd_mixer_attach(m_handle, cDevName) != 0) dbgprintf(2, "ERROR snd_mixer_attach, device:'%s'\n", cDevName);
	snd_mixer_selem_register(m_handle, NULL, NULL);

	// Load the mixer elements
	snd_mixer_load(m_handle);
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	for (m_elem = snd_mixer_first_elem(m_handle); m_elem; m_elem = snd_mixer_elem_next(m_elem)) 
	{
        snd_mixer_selem_get_id(m_elem, sid);
		//show_selem(m_handle, sid, "  ", 0);
		//printf("%i %i %i\n", snd_mixer_selem_is_active(m_elem), snd_mixer_selem_is_enum_capture(m_elem), snd_mixer_selem_has_capture_switch(m_elem));
		//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		
        if (snd_mixer_selem_is_active(m_elem) && (snd_mixer_selem_is_enum_capture(m_elem) || snd_mixer_selem_has_capture_switch(m_elem))) break;
			//printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        //show_selem(m_handle, sid, "  ", level);
    }
	if (m_elem)
	{
		// Here is set the global system volume
		snd_mixer_selem_get_capture_volume_range(m_elem, &min, &max);
		midlevolume = max - min;
		//printf("min %i, max %i, mid %i\n", min, max, midlevolume);
		long ret = 0;
		if (snd_mixer_selem_get_capture_volume(m_elem, SND_MIXER_SCHN_MONO, &ret) != 0) 
			dbgprintf(2, "snd_mixer_selem_get_capture_volume ERROR\n");
			else
			{
				if ((ret - min) == 0) res = 0; else res = (int)((float)(ret - min) / ((float)(midlevolume) / 100));
			}
	} else dbgprintf(2, "audio_get_volume: Not find audio capture device\n");
	
	snd_mixer_detach(m_handle, cDevName);
	snd_mixer_close(m_handle);
	
	dbgprintf(5, "audio_get_capture_volume:'%s' >>> %i\n", cDevName, res);
	
	return (int)res;
}

void Media_Replay()
{
	if ((tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY) != 0)
		&& (tx_eventer_test_event(&pevnt_mplay_run, MEDIA_EVENT_STOP) != 0))
	{
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_REPLAY);
	}		
}

int preconfig_audio_capture_device(snd_pcm_t *audio_capture_handle, snd_pcm_hw_params_t **alsa_hw_params, int audio_channels) 
{
	int err;

	// allocate an invalid snd_pcm_hw_params_t using standard malloc
	err = snd_pcm_hw_params_malloc(alsa_hw_params);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
		return -1;
	}
	// fill hw_params with a full configuration space for a PCM.
	err = snd_pcm_hw_params_any(audio_capture_handle, *alsa_hw_params);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot initialize hardware parameter structure (%s)\n",snd_strerror(err));
		return -2;
	}
	// set the number of channels
	err = snd_pcm_hw_params_set_channels(audio_capture_handle, *alsa_hw_params, audio_channels);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set channel count for capture device (%s)\n", snd_strerror(err));	
		return -3;
	}
	return 1;
}

int configure_audio_capture_device(snd_pcm_t *audio_capture_handle, snd_pcm_hw_params_t *alsa_hw_params, 
					struct pollfd **poll_fds, int *audio_fd_count, int *period_size,
					int audio_sample_rate, int audio_channels, int buffer_size) 
{
  int err;

  // libavcodec
/*	#if AUDIO_ONLY
		AVCodecContext *ctx = hls->format_ctx->streams[0]->codec;
	#else
		AVCodecContext *ctx = hls->format_ctx->streams[1]->codec;
	#endif*/
	//int buffer_size;

	// ALSA poll mmap
	snd_pcm_uframes_t real_buffer_size; // real buffer size in frames
	int dir;

	//buffer_size = buffer_size; //av_samples_get_buffer_size(NULL, ctx->channels, ctx->frame_size, ctx->sample_fmt, 0);
	*period_size = buffer_size / audio_channels / sizeof(short);

	// use mmap
	err = snd_pcm_hw_params_set_access(audio_capture_handle, alsa_hw_params,SND_PCM_ACCESS_MMAP_INTERLEAVED);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set access type (%s)\n", snd_strerror(err));
		return -1;
	}

	// SND_PCM_FORMAT_S16_LE => PCM 16 bit signed little endian
	err = snd_pcm_hw_params_set_format(audio_capture_handle, alsa_hw_params,  SND_PCM_FORMAT_S16_LE);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set sample format (%s)\n", snd_strerror(err));
		return -2;
	}

	// set the sample rate
	unsigned int rate = audio_sample_rate;
	err = snd_pcm_hw_params_set_rate_near(audio_capture_handle, alsa_hw_params, &rate, 0);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set sample rate (%s)\n", snd_strerror(err));
		return -3;
	}

	unsigned int actual_rate;
	int actual_dir;
	err = snd_pcm_hw_params_get_rate(alsa_hw_params, &actual_rate, &actual_dir);
	if (err < 0) 
	{
		dbgprintf(1,"error: failed to get sample rate from capture device (%s)\n", snd_strerror(err));
		return -4;
	}
	//dbgprintf(1,"actual sample rate=%u dir=%d\n", actual_rate, actual_dir);
	if (actual_rate != audio_sample_rate) 
	{
		dbgprintf(1,"error: failed to set sample rate for capture device to %d (got %d)\n",audio_sample_rate, actual_rate);
		return -5;
	}

	// set the buffer size
	int alsa_buffer_multiply = ALSA_BUFFER_MULTIPLY;
	err = snd_pcm_hw_params_set_buffer_size(audio_capture_handle, alsa_hw_params, buffer_size * alsa_buffer_multiply);
	while (err < 0) 
	{
		dbgprintf(1,"failed to set buffer size for capture device: buffer_size=%d multiply=%d\n", buffer_size, alsa_buffer_multiply);
		alsa_buffer_multiply /= 2;
		if (alsa_buffer_multiply == 0) break;
		dbgprintf(1,"trying smaller buffer size for capture device: buffer_size=%d multiply=%d\n", buffer_size, alsa_buffer_multiply);
		err = snd_pcm_hw_params_set_buffer_size(audio_capture_handle, alsa_hw_params, buffer_size * alsa_buffer_multiply);
	}
	if (err < 0) 
	{
		dbgprintf(1,"error: failed to set buffer size for capture device: buffer_size=%d multiply=%d (%s)\n", buffer_size, alsa_buffer_multiply, snd_strerror(err));
		return -6;
	}

	// check the value of the buffer size
	err = snd_pcm_hw_params_get_buffer_size(alsa_hw_params, &real_buffer_size);
	if (err < 0) 
	{
		dbgprintf(1,"error: failed to get buffer size from capture device (%s)\n", snd_strerror(err));
		return -7;
	}
	//dbgprintf(1,"capture device: buffer size: %d frames (buffer_size=%d multiply=%d)\n", (int)real_buffer_size, buffer_size, alsa_buffer_multiply);

	//dbgprintf(1,"capture device: setting period size to %d\n", period_size);
	dir = 0;
	// set the period size
	err = snd_pcm_hw_params_set_period_size_near(audio_capture_handle, alsa_hw_params, (snd_pcm_uframes_t *)period_size, &dir);
	if (err < 0) 
	{
		dbgprintf(1,"error: failed to set period size for capture device (%s)\n", snd_strerror(err));
		return -8;
	}

	snd_pcm_uframes_t actual_period_size;
	err = snd_pcm_hw_params_get_period_size(alsa_hw_params, &actual_period_size, &dir);
	if (err < 0) 
	{
		dbgprintf(1,"error: failed to get period size from capture device (%s)\n", snd_strerror(err));
		return -9;
	}
	//dbgprintf(1,"actual_period_size=%lu dir=%d\n", actual_period_size, dir);

	// apply the hardware configuration
	err = snd_pcm_hw_params (audio_capture_handle, alsa_hw_params);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set PCM hardware parameters for capture device (%s)\n", snd_strerror(err));
		return -10;
	}

	// end of configuration
	snd_pcm_hw_params_free(alsa_hw_params);

	err = snd_pcm_prepare(audio_capture_handle);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		return -11;
	}

	*audio_fd_count = snd_pcm_poll_descriptors_count(audio_capture_handle);
	if (*audio_fd_count <= 0) 
	{
		dbgprintf(1,"capture device error: invalid poll descriptors count: %i\n",*audio_fd_count);
		return *audio_fd_count;
	}
	*poll_fds = DBG_MALLOC(sizeof(struct pollfd) * (*audio_fd_count));
	if (*poll_fds == NULL) 
	{
		dbgprintf(1,"error: cannot allocate memory for poll_fds\n");
		pthread_exit(0);
	}
	// get poll descriptors
	err = snd_pcm_poll_descriptors(audio_capture_handle, *poll_fds, *audio_fd_count);
	if (err < 0) 
	{
		dbgprintf(1,"capture device error: unable to obtain poll descriptors for capture: %s\n", snd_strerror(err));
		return err;
	}
	//is_first_audio = 1; 

	// dump the configuration of capture_handle
	/*snd_output_t *output;
	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) 
	{
		dbgprintf(1,"snd_output_stdio_attach failed: %s\n", snd_strerror(err));
		return 0;
	}
	dbgprintf(1,"audio capture device:\n");
	snd_pcm_dump(audio_capture_handle, output);*/
	
	return 0;
}

void close_audio_capture_device(snd_pcm_t *audio_capture_handle, struct pollfd *poll_fds) 
{
	snd_pcm_close(audio_capture_handle);
	DBG_FREE(poll_fds);
}
  
int open_audio_capture_device(snd_pcm_t **audio_capture_handle, char *alsa_dev) 
{
	int err;

	//dbgprintf(1,"opening ALSA device for capture: %s\n", alsa_dev);
	err = snd_pcm_open(audio_capture_handle, alsa_dev, SND_PCM_STREAM_CAPTURE, 0);
	if (err < 0) 
	{
		if (err == -16) 
			dbgprintf(3,"error: cannot open audio capture device '%s': %s\n",alsa_dev, snd_strerror(err));
			else
			dbgprintf(1,"error: cannot open audio capture device '%s': %s\n",alsa_dev, snd_strerror(err));
		return -1;
	}
	return 0;
}

// Callback function that is called when an error has occurred
int xrun_recovery(snd_pcm_t *handle, int error) 
{
	switch(error) 
	{
		case -EPIPE: // Buffer overrun
			dbgprintf(4,"device error: buffer overrun\n");
			if ((error = snd_pcm_prepare(handle)) < 0) 
				dbgprintf(1,"device error: buffer overrrun cannot be recovered, snd_pcm_prepare failed: %s\n", snd_strerror(error));
			return 0;
			break;		
		case -ESTRPIPE: // capture device is suspended
			dbgprintf(1,"device error: suspended\n");
			// Wait until the suspend flag is cleared
			while ((error = snd_pcm_resume(handle)) == -EAGAIN) sleep(1);
            if (error < 0) 
			{
				if ((error = snd_pcm_prepare(handle)) < 0) 
					dbgprintf(1,"device error: suspend cannot be recovered, snd_pcm_prepare failed: %s\n", snd_strerror(error));
            }
			return 0;
			break;

		case -EBADFD: // PCM descriptor is wrong
			dbgprintf(1,"device error: EBADFD\n");
			break;

		default:
			dbgprintf(1,"device error: unknown, error = %d\n",error);
			break;
	}
  return error;
}

// Wait for data using poll
int wait_for_poll(snd_pcm_t *device, struct pollfd *target_fds, unsigned int audio_fd_count) 
{
	unsigned short revents;
	int avail_flags = 0;
	int ret;

	while (1) 
	{
		ret = poll(target_fds, audio_fd_count, -1); // -1 means block
		if (ret < 0) 
		{
			dbgprintf(1,"audio poll error: %d\n", ret);
			return ret;
		} 
		else 
		{
			snd_pcm_poll_descriptors_revents(device, target_fds, audio_fd_count, &revents);
			if (revents & POLLERR) return -EIO;
			if (revents & POLLIN) 
			{
				avail_flags |= AVAIL_AUDIO;
				return avail_flags;
			}
		}
	}
}

int recover_audio_capture_device(snd_pcm_t *audio_capture_handle, int *avail_flags, int *is_first_audio)
{ 
	if (snd_pcm_state(audio_capture_handle) & (SND_PCM_STATE_XRUN | SND_PCM_STATE_SUSPENDED)) 
	{
		*avail_flags = snd_pcm_state(audio_capture_handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
		if (xrun_recovery(audio_capture_handle, *avail_flags) < 0) 
		{
			dbgprintf(1,"capture device: write error: %s\n", snd_strerror(*avail_flags));
			return -1;
		}
		*is_first_audio = 1;
	} 
	else 
	{
		dbgprintf(1,"capture device error: wait for poll failed\n");				
	}
	return 0;
}

int read_audio_poll_mmap(snd_pcm_t *audio_capture_handle, int period_size, uint16_t *uiInBuffer, int audio_channels, int *is_first_audio) 
{
	const snd_pcm_channel_area_t 	*my_areas; // mapped memory area info
	snd_pcm_sframes_t 				avail, commitres; // aux for frames count
	snd_pcm_uframes_t 				offset, frames, size; //aux for frames count
	int 							error;	
	
	// check how many frames are ready to read or write
	avail = snd_pcm_avail_update(audio_capture_handle);
	if (avail < 0) 
	{
		if ((error = xrun_recovery(audio_capture_handle, avail)) < 0) 
		{
			dbgprintf(1,"capture device error: SUSPEND recovery failed: %s\n", snd_strerror(error));
			return -1;
		}
		*is_first_audio = 1;
		return error;
	}   
	
	if (avail < period_size) 
	{ // check if one period is ready to process
		if (*is_first_audio == 1) 
		{
			// if the capture from PCM is started (is_first_audio=1) and one period is ready to process,
			// the stream must start 
			*is_first_audio = 0;
			//dbgprintf(1,"[capture device started]\n");
			if ( (error = snd_pcm_start(audio_capture_handle)) < 0) 
			{
				dbgprintf(1,"error: cannot start capture device: %s\n", snd_strerror(error));
				return -2;
			}				
		}
		else
		{
			dbgprintf(5,"not first audio\n");
			// wait for pcm to become ready
			if ( (error = snd_pcm_wait(audio_capture_handle, -1)) < 0) 
			{
				if ((error = xrun_recovery(audio_capture_handle, error)) < 0) 
				{
					dbgprintf(1,"capture device error: snd_pcm_wait: %s\n", snd_strerror(error));
					return -3;
				}
				*is_first_audio = 1;
			}
		} 
		return -10;
	}
	
	int read_size = 0;
	size = period_size;
	while (size > 0) 
	{ // wait until we have period_size frames (in the most cases only one loop is needed)
		frames = size; // expected number of frames to be processed
		// frames is a bidirectional variable, this means that the real number of frames processed is written 
		// to this variable by the function.
		if ((error = snd_pcm_mmap_begin (audio_capture_handle, &my_areas, &offset, &frames)) < 0) 
		{
			if ((error = xrun_recovery(audio_capture_handle, error)) < 0) 
			{
				dbgprintf(1,"capture device error: mmap begin: %s\n", snd_strerror(error));
				return -4;
			}
			*is_first_audio = 1;
		} 
		size_t copy_size = frames * sizeof(short) * audio_channels;
		memcpy(uiInBuffer + read_size, (my_areas[0].addr)+(offset*sizeof(short)*audio_channels), copy_size);
		read_size += copy_size;

		commitres = snd_pcm_mmap_commit(audio_capture_handle, offset, frames);
		if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) 
		{
			if ((error = xrun_recovery(audio_capture_handle, commitres >= 0 ? commitres : -EPIPE)) < 0) 
			{
				dbgprintf(1,"capture device error: mmap commit: %s\n", snd_strerror(error));
				return -5;
			}
			*is_first_audio = 1;
		}
		size -= frames; // needed in the condition of the while loop to check if period is filled
	}

	return 0;
}

void audio_loop_poll_mmap(snd_pcm_t **ach, struct pollfd *poll_fds, int audio_fd_count, int period_size, AVFrame *av_frame, AVFormatContext *format_ctx, func_link *f_link, uint16_t *uiInBuffer, int buffer_size) 
{
	DBG_LOG_IN();	
	
	snd_pcm_t *audio_capture_handle = *ach;
	int avail_flags;
	int is_first_audio = 1;
	int iNumFrame = START_AUDIO_FRAME_NUMBER;
	int ret, n;	
	char cReinitCnt = 0;
	int iBufferSize = buffer_size;
	unsigned char *ucBuffer=DBG_MALLOC(iBufferSize);
	int iPacketSize;
	int iSaveFrame = 1;
//	FILE *dst_file = fopen("/home/pi/test.mp2", "wb");
//fwrite(ucBuffer, 1, iPacketSize, dst_file);
//fclose
	misc_buffer empty_encoded_frame;
	memset(&empty_encoded_frame, 0, sizeof(misc_buffer));
	
	MODULE_INFO *miModule = (MODULE_INFO *)(f_link->pModule);
	//////SENSOR SETTINGS/////
	char cNeedControl = 0;
	unsigned int uiSenceSkip = miModule->Settings[8];
	//unsigned int uiRecSplit = miModule->Settings[9] ? 1 : 0;
	unsigned int uiRecNorm = (miModule->Settings[7] & 1) ? 1 : 0;
	unsigned int uiRecVidDiff = (miModule->Settings[7] & 2) ? 1 : 0;
	unsigned int uiRecDiff = miModule->Settings[10] ? 1 : 0;
	unsigned int uiRecLevel = miModule->Settings[11] & 1023;
	//if (uiRecLevel > 100) uiRecLevel = 100;
	uiRecLevel = (unsigned int)(16384 / 1024 * uiRecLevel);
	unsigned int uiDiffSkip = miModule->Settings[12];
	unsigned int uiSaveFrames = miModule->Settings[13];
	unsigned int uiScanPeriod = miModule->Settings[14];
	if (uiScanPeriod == 0) uiScanPeriod = 1;
	unsigned int uiSaveFramesCnt = 0;
	
	unsigned int uiDigConvertMode = miModule->Settings[17];
	unsigned int uiDigLevel = 1 << miModule->Settings[18];

	if ((miModule->ScanSet != 0) && 
		((miModule->Global & 1) || (miModule->GenEvents & 1) || (miModule->SaveChanges & 1))) cNeedControl = 1;
	
	unsigned int ucScanGridSize = 1;
	ucScanGridSize = miModule->Settings[5] & 1023;
	if (ucScanGridSize == 0) ucScanGridSize = 1;
	//int16_t *iInBuffer = (int16_t*)uiInBuffer;
	
	unsigned int uiControlLevel = miModule->Settings[6] & 1023;
	//if (uiControlLevel > 100) uiControlLevel = 100;
	uiControlLevel = (unsigned int)(16384 / 1024 * uiControlLevel);
	unsigned int iTimeScanClk = 0;
	unsigned int iTimeDiffClk = 0;
	unsigned int iDiffResult = 0;
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	int iMicModuleNum = ModuleIdToNum(miModule->ID, 1);
	unsigned int uiEventsFlag = miModuleList[iMicModuleNum].GenEvents | miModuleList[iMicModuleNum].Global;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	int iSensorPrevStatus[8];	
	memset(&iSensorPrevStatus, 0, sizeof(int)*8);
	int iLastStatRtsp = 1;
	int iNeedSend;	
	
	STREAM_INFO *pCaptStreamNorm = NULL;
	STREAM_INFO *pCaptStreamAud = NULL;
	STREAM_INFO *pCaptStreamDiff = NULL;
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int ui = iStreamCnt;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	for (n = 0; n < ui; n++)
	{
		DBG_MUTEX_LOCK(&SyncStream[n]->mutex);
		if (miModule->ID == SyncStream[n]->AudID)
		{
			if (SyncStream[n]->Type == CAPTURE_TYPE_FULL) pCaptStreamNorm = SyncStream[n];
			if (SyncStream[n]->Type == CAPTURE_TYPE_AUDIO) pCaptStreamAud = SyncStream[n];
			if (SyncStream[n]->Type == CAPTURE_TYPE_DIFF) pCaptStreamDiff = SyncStream[n];
		}
		DBG_MUTEX_UNLOCK(&SyncStream[n]->mutex);
	}
	if ((uiRecNorm) && (pCaptStreamNorm == NULL)) {dbgprintf(1, "Error find audio CAPTURE_TYPE_FULL stream id\n"); uiRecNorm = 0;}
	if ((uiRecVidDiff) && (pCaptStreamDiff == NULL)) {dbgprintf(1, "Error find audio CAPTURE_TYPE_DIFF stream id\n"); uiRecVidDiff = 0;}
	if ((uiRecDiff) && (pCaptStreamAud == NULL)) {dbgprintf(1, "Error find audio CAPTURE_TYPE_AUDIO stream id\n"); uiRecDiff = 0;}
	
	
//	FILE *dst_file = fopen("/home/pi/test.ac3", "wb");
//	char zerobuff[8] = {33,33,33,33,33,33,33,33};
//fwrite(ucBuffer, 1, iPacketSize, dst_file);
//fclose
	/////////SET convert options
	int16_t *piCaptureBuffer=DBG_MALLOC(iBufferSize);	
	SwrContext *swr = swr_alloc();
	//printf("audio_sample_rate %i    audio_channels %i\n", f_link->codec_info.audio_sample_rate, f_link->codec_info.audio_channels);
	av_opt_set_int(swr, "in_sample_rate", (int64_t)f_link->codec_info.audio_sample_rate, 0);
	av_opt_set_int(swr, "out_sample_rate", (int64_t)f_link->codec_info.audio_sample_rate, 0);
	//av_opt_set_sample_fmt(swr, "in_sample_fmt", ctx->sample_fmt, 0);
	av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", av_frame->format, 0);
	
	if (f_link->codec_info.audio_channels == 1)
	{
		av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_MONO, 0);
		av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_MONO, 0);
	}
	else
	{
		av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
		av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
	}
	swr_init(swr);
	
	  
	//f_link->codec_info.audio_frame_size = buffer_size / 2;	
	f_link->codec_info.audio_raw_buffer_size = buffer_size;
	f_link->codec_info.audio_frame_rate = (int)(f_link->codec_info.audio_sample_rate / f_link->codec_info.audio_frame_size);
	//printf(">>> %i %i %i\n", f_link->codec_info.audio_frame_size, period_size, f_link->codec_info.audio_frame_size);
	
	unsigned int uiUpdateCounter = 0;
	float amplifCoefficient = 1.0f;
	int uiCalc;
	char cCriticalError = 0;
	char cMute = 0;
	int iMaxSignalLevel = 0;
	
	while (1) 
	{
		uiUpdateCounter++;
		if (uiUpdateCounter > 100)
		{
			uiUpdateCounter = 0;
			DBG_MUTEX_LOCK(&modulelist_mutex);	
			uiDigConvertMode = miModuleList[iMicModuleNum].Settings[17];
			uiDigLevel = 1 << miModuleList[iMicModuleNum].Settings[18];
			miModuleList[iMicModuleNum].Status[4] = iMaxSignalLevel >> 4;
			iMaxSignalLevel = 0;
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
		}
		
		if (tx_eventer_test_event(&pevnt_mcapt_run, CAPTURE_EVENT_STOP) == 0)
		{
			dbgprintf(5,"CAPTURE_EVENT_STOP\n");
			break;
		}
		if (tx_eventer_test_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_ON) == 0)
		{
			tx_eventer_add_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_ON);
			dbgprintf(5,"CAPTURE_EVENT_MUTE_ON\n");
			cMute = 1;
		}
		if (tx_eventer_test_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_OFF) == 0)
		{
			tx_eventer_add_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_OFF);
			dbgprintf(5,"CAPTURE_EVENT_MUTE_OFF\n");
			cMute = 0;
		}
		
		if (is_first_audio)
		{
			ret = read_audio_poll_mmap(audio_capture_handle, period_size, (uint16_t *)piCaptureBuffer, f_link->codec_info.audio_channels, &is_first_audio);
			if ((ret < 0) && (ret != -10))
			{
				dbgprintf(1,"Error load first data\n");
				break;
			}
			//dbgprintf(1,"Skip first data\n");
		}

		avail_flags = wait_for_poll(audio_capture_handle, poll_fds, audio_fd_count);
		if (avail_flags < 0) 
		{ 	
			//dbgprintf(1,"trying to recover from error\n");
			recover_audio_capture_device(audio_capture_handle, &avail_flags, &is_first_audio);
			dbgprintf(4,"recovery recover_audio_capture_device\n");	
		}
		if (avail_flags & AVAIL_AUDIO)
		{
			//printf(">>> %i", (unsigned int)get_ms(&testms));
			ret = read_audio_poll_mmap(audio_capture_handle, period_size, (uint16_t *)piCaptureBuffer, f_link->codec_info.audio_channels, &is_first_audio);	
			//printf(">>> %i\n", (unsigned int)get_ms(&testms));
			if (ret == 0) 
			{
				//////////BeforeFilter/////////
				if (uiDigConvertMode == 1)
				{
					for (n = 0; n < f_link->codec_info.audio_frame_size; n++) 
					{
						uiCalc = piCaptureBuffer[n] * uiDigLevel;
						if (uiCalc < -32767) uiCalc = -32767;
						if (uiCalc > 32767) uiCalc = 32767;
						piCaptureBuffer[n] = uiCalc;
					}
				}
				if (uiDigConvertMode == 2)
				{
					ret = 0;
					for (n = 0; n < f_link->codec_info.audio_frame_size; n += ucScanGridSize)
					{
						//if ((piCaptureBuffer[n] > 0) && (piCaptureBuffer[n] > ret)) ret = piCaptureBuffer[n];
						//if ((piCaptureBuffer[n] < 0) && (piCaptureBuffer[n] < (-ret))) ret = piCaptureBuffer[n];
						if ((piCaptureBuffer[n] > 0) && (ret < piCaptureBuffer[n])) ret = piCaptureBuffer[n];
						if ((piCaptureBuffer[n] < 0) && (ret < (-piCaptureBuffer[n]))) ret = -piCaptureBuffer[n];
					}
					if (ret)
					{	
						//printf("%i\t%i\t%i\t%i\t%f\n", ret, uiDigLevel, (int)(ret* amplifCoefficient), (int)(ret* amplifCoefficient) - uiDigLevel, amplifCoefficient);							
												
						ret *= amplifCoefficient;						
						ret = ret - uiDigLevel;
						if (ret < 1000) amplifCoefficient += 3;
						if (ret > 1000) amplifCoefficient -= 3;						
						if (amplifCoefficient != 1.0f)
						{
							///printf("%f\n", uiDigLevel, amplifCoefficient);							
							for (n = 0; n < f_link->codec_info.audio_frame_size; n ++) 
							{
								uiCalc = piCaptureBuffer[n] * amplifCoefficient;
								if (uiCalc < -32767) uiCalc = -32767;
								if (uiCalc > 32767) uiCalc = 32767;
								piCaptureBuffer[n] = uiCalc;
							}
						}
					}
				}
				//////////SENSOR>>>>>>>>>/////////////
				if (cNeedControl)
				{
					if (iTimeScanClk == 0)
					{
						iTimeScanClk = uiSenceSkip;	
						//SENSOR
						int iCurStatuses[8];
						iCurStatuses[0] = 0;
						iCurStatuses[1] = 0;
						
						for (n = 0; n < f_link->codec_info.audio_frame_size; n += ucScanGridSize)
						{
							if ((piCaptureBuffer[n] > 0) && (piCaptureBuffer[n] > uiControlLevel)) 
							{
								iCurStatuses[0] = 1;
								if (iCurStatuses[1] < piCaptureBuffer[n]) iCurStatuses[1] = piCaptureBuffer[n];
								if ((uiEventsFlag & 0b00100000) == 0) break;
							}
							if ((piCaptureBuffer[n] < 0) && (piCaptureBuffer[n] < (-uiControlLevel))) 
							{
								iCurStatuses[0] = 1;
								if (iCurStatuses[1] < (-piCaptureBuffer[n])) iCurStatuses[1] = -piCaptureBuffer[n];
								if ((uiEventsFlag & 0b00100000) == 0) break;
							}
							//if (ucCaptureBuffer[n] > 0) dbgprintf(3,"+ %i\n", ucCaptureBuffer[n]);
							//if (ucCaptureBuffer[n] < 0) dbgprintf(3,"- %i\n", -ucCaptureBuffer[n]);					
						}
						if ((iCurStatuses[0] != iSensorPrevStatus[0]) || (iCurStatuses[1] != iSensorPrevStatus[1]))
						{
							//dbgprintf(5,"Click\n");		
							DBG_MUTEX_LOCK(&modulelist_mutex);	
							miModuleList[iMicModuleNum].Status[0] = iCurStatuses[0];
							miModuleList[iMicModuleNum].Status[5] = iCurStatuses[1] >> 4;
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
							iSensorPrevStatus[0] = iCurStatuses[0];
							iSensorPrevStatus[1] = iCurStatuses[1];
						}												
					} else iTimeScanClk--;
				}
				if (uiRecDiff)
				{
					//int max = 0;
					if (iTimeDiffClk == 0)
					{
						iTimeDiffClk = uiDiffSkip;
						//DIFF TEST
						iDiffResult = 0;
						for (n = 0; n < f_link->codec_info.audio_frame_size; n += uiScanPeriod)
						{
							//if ((piCaptureBuffer[n] > 0) && (piCaptureBuffer[n] > max)) max = piCaptureBuffer[n];
							//if ((piCaptureBuffer[n] < 0) && (piCaptureBuffer[n] < (-max))) max = -piCaptureBuffer[n];
							if (piCaptureBuffer[n] > 0)
							{
								if (iMaxSignalLevel < piCaptureBuffer[n]) iMaxSignalLevel = piCaptureBuffer[n];
								if (piCaptureBuffer[n] > uiRecLevel) 
								{
									iDiffResult = 1; 									
									if ((uiEventsFlag & 0b00010000) == 0) break;
								}
							}
							if (piCaptureBuffer[n] < 0)
							{
								if (iMaxSignalLevel < (-piCaptureBuffer[n])) iMaxSignalLevel = -piCaptureBuffer[n];								
								if (piCaptureBuffer[n] < (-uiRecLevel)) 
								{
									iDiffResult = 1; 
									if ((uiEventsFlag & 0b00010000) == 0) break;
								}
							}
							//if (ucCaptureBuffer[n] > 0) printf("+ %i\n", ucCaptureBuffer[n]);
							//if (ucCaptureBuffer[n] < 0) printf("- %i\n", -ucCaptureBuffer[n]);					
						}
						if (iDiffResult)
						{
							uiSaveFramesCnt = uiSaveFrames; 
						}						
					} else iTimeDiffClk--;	
					//if (uiSaveFramesCnt) printf("SkipFrames:%i saving:%i limit:%i max:%i\n", iTimeDiffClk, uiSaveFramesCnt, uiRecLevel, max);
				}
			//////////<<<<<<<<<SENSOR/////////////
				if (iSaveFrame == 0)
				{
					
					if (uiRecNorm) UnSkipAudioFrame((void*)pCaptStreamNorm);
					if (uiRecDiff) UnSkipAudioFrame((void*)pCaptStreamAud);
					if (uiRecVidDiff) UnSkipAudioFrame((void*)pCaptStreamDiff);
					iSaveFrame = 1;
				}
				if ((f_link->codec_info.size_empty_frame == 0) || (cMute)) memset(piCaptureBuffer, 0, iBufferSize);
								
				//fwrite(piCaptureBuffer, 1, f_link->codec_info.audio_frame_size*2, dst_file);
				swr_convert(swr, (uint8_t**)&uiInBuffer, av_frame->nb_samples, (const uint8_t**)&piCaptureBuffer, av_frame->nb_samples);
				//fwrite(uiInBuffer, 1, iBufferSize * 2, dst_file);
				
				iNeedSend = ItsNetNeed(CONNECT_SERVER, TRAFFIC_AUDIO, FLAG_NEXT_AUDIO_FRAME | FLAG_AUDIO_STREAM | FLAG_AUDIO_CODEC_INFO, 0);
				if ((uiRecNorm) || (uiRecVidDiff) || (uiRecDiff && uiSaveFramesCnt) || (iNeedSend == 1) || (iLastStatRtsp > 0))
				{
					//printf("%i %i %i %i %i %i\n", uiRecNorm, uiRecVidDiff, uiRecDiff, uiSaveFramesCnt, iNeedSend, iLastStatRtsp);
					ret = encode_audio(av_frame, format_ctx, ucBuffer, iBufferSize, &iPacketSize);	
					
				}
				else ret = 1;
				
				
				//fwrite(ucBuffer, 1, iPacketSize, dst_file);
				//fwrite(zerobuff, 1, 8, dst_file);
					
				if (ret == 1)
				{
					if ((f_link->codec_info.size_empty_frame == 0) && (iPacketSize != 0))
					{
						f_link->codec_info.empty_frame = (char*)DBG_MALLOC(iPacketSize);
						f_link->codec_info.size_empty_frame = iPacketSize;
						memcpy(f_link->codec_info.empty_frame, ucBuffer, iPacketSize);
					}
										
					if (iNeedSend == 1)
						f_link->FuncSend(iNumFrame, FLAG_NEXT_AUDIO_FRAME, (char*)ucBuffer, iPacketSize, (void*)&f_link->codec_info, TRAFFIC_AUDIO);
					
					if ((uiRecNorm) && (MediaRecord_IsFree(pCaptStreamNorm) == 0))
						f_link->FuncSave(iNumFrame, (char*)ucBuffer, iPacketSize, (void*)&f_link->codec_info, (void*)pCaptStreamNorm);
					if ((uiRecVidDiff) && (MediaRecord_IsFree(pCaptStreamDiff) == 0))
						f_link->FuncSave(iNumFrame, (char*)ucBuffer, iPacketSize, (void*)&f_link->codec_info, (void*)pCaptStreamDiff);
					if ((uiRecDiff && uiSaveFramesCnt) && (MediaRecord_IsFree(pCaptStreamAud) == 0))
						f_link->FuncSave(iNumFrame, (char*)ucBuffer, iPacketSize, (void*)&f_link->codec_info, (void*)pCaptStreamAud);
					iLastStatRtsp = f_link->FuncRTSP(miModule->ID, (char*)ucBuffer, (unsigned int)iPacketSize);
					//fwrite(ucBuffer, 1, iPacketSize, dst_file);
					//		printf("sended audio data %i(%i)\n",iPacketSize,iBufferSize);					 
				} //else if (iNumFrame > (START_AUDIO_FRAME_NUMBER + 1)) dbgprintf(1,"not encode data\n");
				if (uiSaveFramesCnt && (iDiffResult == 0)) uiSaveFramesCnt--;
			} 
			else 
			{
				//if (ret < 0) 					
				dbgprintf(1,"error read_audio_poll_mmap\n");
				if (cReinitCnt) 
				{
					cCriticalError = 1;
					break;
				}
				
				close_audio_capture_device(audio_capture_handle, poll_fds);
				MODULE_INFO *miModule = (MODULE_INFO *)(f_link->pModule);
				char dev[32];
				memset(dev,0,32);
				sprintf(dev, "hw:%i,%i", miModule->Settings[1], miModule->Settings[2]);	
				
				ret = open_audio_capture_device(ach, dev);
				if (ret < 0)
				{
					if (ret == -1) dbgprintf(1,"warning: audio capturing is disabled (%s)\n",dev);
							else dbgprintf(1,"error: RE init_audio failed: %d\n", ret);
					break;
				} 
				audio_capture_handle = *ach;
				
				snd_pcm_hw_params_t *alsa_hw_params; 
				preconfig_audio_capture_device(audio_capture_handle, &alsa_hw_params, f_link->codec_info.audio_channels);
				ret = configure_audio_capture_device(audio_capture_handle, alsa_hw_params, &poll_fds, &audio_fd_count, &period_size,
										f_link->codec_info.audio_sample_rate, f_link->codec_info.audio_channels, buffer_size / 2);
				if (ret != 0) 
				{
					dbgprintf(1,"error: configure_audio_capture_device: ret=%d\n", ret);
					break;
				}
				
				cReinitCnt++;
			}
			iNumFrame++; 
			if (iNumFrame > END_AUDIO_FRAME_NUMBER) iNumFrame = START_AUDIO_FRAME_NUMBER + 1;
		}
		else 
		{
			if (iSaveFrame)
			{
				if (uiRecNorm) SkipAudioFrame((void*)pCaptStreamNorm);
				if (uiRecVidDiff) SkipAudioFrame((void*)pCaptStreamDiff);
				if (uiRecDiff) SkipAudioFrame((void*)pCaptStreamAud);
				iSaveFrame = 0;
			}
		}		
	} 
	//fclose(dst_file);
	if (f_link->codec_info.empty_frame) DBG_FREE(f_link->codec_info.empty_frame);
	DBG_FREE(ucBuffer);
	swr_free(&swr);
	DBG_FREE(piCaptureBuffer);

	DBG_MUTEX_LOCK(&system_mutex);
	if (uiErrorAudioRestart && cCriticalError) 
	{
		unsigned int uitWait = uiErrorAudioRestartWait;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (uitWait) usleep(uitWait * 1000);
		if (uiErrorAudioRestart == 1) System_Restart(NULL, 0); else System_Reboot(NULL, 0);
		uiErrorAudioRestart = 100;
	} else DBG_MUTEX_UNLOCK(&system_mutex);	
	
	DBG_LOG_OUT();		
}

int setup_av_frame(AVFormatContext *format_ctx, AudioCodecInfo *codec_set, AVFrame **av_frame, uint16_t **uiInBuffer, int *buffer_size, int *period_size, int *audio_pts_step_base) 
{
	DBG_LOG_IN();	
	
	AVCodecContext *audio_codec_ctx;
	int ret;
	
	audio_codec_ctx = format_ctx->streams[0]->codec;

	*av_frame = av_frame_alloc();
	if (!(*av_frame)) 
	{
		dbgprintf(1,"error: av_frame_alloc failed\n");
		DBG_LOG_OUT();	
		return -1;
	}
	if (audio_codec_ctx->frame_size == 0) audio_codec_ctx->frame_size = 2048;
	(*av_frame)->sample_rate = audio_codec_ctx->sample_rate;
	//dbgprintf(1,"sample_rate: %d\n", audio_codec_ctx->sample_rate);
	(*av_frame)->nb_samples = audio_codec_ctx->frame_size;
	//dbgprintf(1,"nb_samples: %d\n", audio_codec_ctx->frame_size);
	(*av_frame)->format = audio_codec_ctx->sample_fmt;
	//dbgprintf(1,"sample_fmt: %d\n", audio_codec_ctx->sample_fmt);
	(*av_frame)->channel_layout = audio_codec_ctx->channel_layout;
	//dbgprintf(1,"audio_codec_ctx->channel_layout: %" PRIu64 "\n", audio_codec_ctx->channel_layout);
	//dbgprintf(1,"av_frame->channel_layout: %" PRIu64 "\n", (*av_frame)->channel_layout);
	//dbgprintf(1,"audio_codec_ctx->channels: %d\n", audio_codec_ctx->channels);
	//dbgprintf(1,"av_frame->channels: %d\n", (*av_frame)->channels);

	*buffer_size = av_samples_get_buffer_size(NULL, audio_codec_ctx->channels, audio_codec_ctx->frame_size, audio_codec_ctx->sample_fmt, 0);
	*uiInBuffer = av_malloc(*buffer_size);
	if (!(*uiInBuffer)) 
	{
		dbgprintf(1,"error: av_malloc for samples failed for %i (ch:%i smpl:%i)\n", *buffer_size, audio_codec_ctx->channels, audio_codec_ctx->frame_size);
		DBG_LOG_OUT();	
		return -2;
	}
	*period_size = *buffer_size / codec_set->audio_channels / 4;
	*audio_pts_step_base = (*period_size) * 90000.0f / codec_set->audio_sample_rate;
	//printf("Audio buffer size %i %i %i", *buffer_size, *period_size,*audio_pts_step_base);
	//dbgprintf(1,"audio_pts_step_base: %d\n", *audio_pts_step_base);

	ret = avcodec_fill_audio_frame(*av_frame, audio_codec_ctx->channels, audio_codec_ctx->sample_fmt,
									(const uint8_t*)(*uiInBuffer), *buffer_size, 0);
	if (ret < 0) 
	{
		char errbuf[256];
		memset(errbuf, 0, 256);
		av_strerror(ret, errbuf, sizeof(errbuf));
		dbgprintf(1,"error: avcodec_fill_audio_frame failed: %s\n", errbuf);
		DBG_LOG_OUT();	
		return -3;
	}
	
	DBG_LOG_OUT();	
	return 1;
}

int is_sample_fmt_supported(AVCodec *codec, enum AVSampleFormat sample_fmt) 
{
	const enum AVSampleFormat *p = codec->sample_fmts;

	while (*p != AV_SAMPLE_FMT_NONE) 
	{
		if (*p == sample_fmt) return 1;
		p++;
	}
	return 0;
}


int select_sample_rate(AVCodec *codec, unsigned int uiCurRate)
{
    const int *p;
    int best_samplerate = 0;
    if (!codec->supported_samplerates)
        return 44100;
    p = codec->supported_samplerates;
    while (*p) 
	{
		if (*p == uiCurRate)
		{
			best_samplerate = uiCurRate;
			break;
		}
        best_samplerate = FFMAX(*p, best_samplerate);
        p++;
    }
    return best_samplerate;
}

int select_channel_layout(AVCodec *codec, int iCurChan)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;
	
	int cur_nb_channels = av_get_channel_layout_nb_channels(iCurChan);
	
    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;
    p = codec->channel_layouts;
    while (*p) 
	{
		int nb_channels = av_get_channel_layout_nb_channels(*p);
        if (nb_channels == cur_nb_channels)
		{
			best_ch_layout    = *p;
            best_nb_channels = nb_channels;
			break;
		}
        if (nb_channels > best_nb_channels) 
		{
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}

int setup_audio_stream(AVFormatContext *format_ctx, AudioCodecInfo *settings) 
{
	DBG_LOG_IN();	
	
	AVCodec *a_codec;
	AVCodecContext *audio_codec_ctx = NULL;
	AVStream *audio_stream;
	int ret;

	a_codec = avcodec_find_encoder(settings->audio_codec);
	if (!a_codec) 
	{		
		dbgprintf(1, "codec not found\n");
		DBG_LOG_OUT();	
		return -1;
	}
	
	audio_stream = avformat_new_stream(format_ctx, a_codec);
	if (!audio_stream) 
	{
		dbgprintf(1,"avformat_new_stream for audio error\n");
		DBG_LOG_OUT();	
		return -2;
	}
	audio_stream->id = format_ctx->nb_streams - 1;
	audio_codec_ctx = audio_stream->codec;

	audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	
	if ((settings->audio_codec == AV_CODEC_ID_MP2) || 
		(settings->audio_codec == AV_CODEC_ID_SPEEX) || 
		(settings->audio_codec == AV_CODEC_ID_PCM_S16LE) ||
		(settings->audio_codec == AV_CODEC_ID_ADPCM_SWF))
		audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	if (settings->audio_codec == AV_CODEC_ID_NELLYMOSER) 
		audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
	if (settings->audio_codec == AV_CODEC_ID_DTS) 
		audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_DBL;
		
		
	if (!is_sample_fmt_supported(a_codec, audio_codec_ctx->sample_fmt)) 
	{
		dbgprintf(1,"Sample format %s is not supported\n", av_get_sample_fmt_name(audio_codec_ctx->sample_fmt));
		DBG_LOG_OUT();	
		return -3;
	}

	audio_stream->time_base.num = 1;
	audio_stream->time_base.den = settings->audio_sample_rate;
	audio_codec_ctx->time_base.num = 1;
	audio_codec_ctx->time_base.den = settings->audio_sample_rate;
	audio_codec_ctx->ticks_per_frame = 1;
	audio_codec_ctx->bit_rate = settings->audio_bit_rate;
	audio_codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	audio_codec_ctx->profile = settings->audio_profile;
	audio_codec_ctx->sample_rate = settings->audio_sample_rate;
	
	if (settings->audio_bit_rate < 6)
	{
		audio_codec_ctx->bit_rate = 0;
		audio_codec_ctx->flags |= CODEC_FLAG_QSCALE;
		audio_codec_ctx->global_quality = settings->audio_bit_rate;
	}
	
	if (settings->audio_channels == 2) 
		audio_codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
		else 
		audio_codec_ctx->channel_layout = AV_CH_LAYOUT_MONO;
	
	audio_codec_ctx->channels = av_get_channel_layout_nb_channels(audio_codec_ctx->channel_layout);

	if (audio_codec_ctx->sample_rate != select_sample_rate(a_codec, audio_codec_ctx->sample_rate))
	{
		dbgprintf(2, "Changet sample rate from %i to %i\n", audio_codec_ctx->sample_rate, select_sample_rate(a_codec, audio_codec_ctx->sample_rate));
		audio_codec_ctx->sample_rate    = select_sample_rate(a_codec, audio_codec_ctx->sample_rate);    
	}
	/*if (audio_codec_ctx->channel_layout != select_channel_layout(a_codec, audio_codec_ctx->channel_layout))
	{
		dbgprintf(2, "Changet channel layout from %i to %i\n", audio_codec_ctx->channel_layout, select_channel_layout(a_codec, audio_codec_ctx->channel_layout));
		audio_codec_ctx->channel_layout = select_channel_layout(a_codec, audio_codec_ctx->channel_layout);    
	}*/
	if (audio_codec_ctx->channels != av_get_channel_layout_nb_channels(audio_codec_ctx->channel_layout))
	{
		dbgprintf(2, "Changet channels from %i to %i\n", audio_codec_ctx->channels, av_get_channel_layout_nb_channels(audio_codec_ctx->channel_layout));
		audio_codec_ctx->channels = av_get_channel_layout_nb_channels(audio_codec_ctx->channel_layout);    
	}
	
	ret = avcodec_open2(audio_codec_ctx, a_codec, NULL);
	if (ret < 0) 
	{
		char errbuf[256];
		memset(errbuf, 0, 256);
		av_strerror(ret, errbuf, sizeof(errbuf));
		dbgprintf(1,"avcodec_open2 failed: %s\n", errbuf);
		DBG_LOG_OUT();	
		return -4;
	}
	
	DBG_LOG_OUT();	
	return 1;
}

AVFormatContext * mpegts_create_context(AudioCodecInfo *settings) 
{
	DBG_LOG_IN();	
	
	AVFormatContext *format_ctx = NULL;
	AVOutputFormat *out_fmt;

	av_register_all();

	out_fmt = av_guess_format("mpegts", NULL, NULL);
	out_fmt->flags |= ~AVFMT_GLOBALHEADER;
	if (!out_fmt) 
	{
		dbgprintf(1,"av_guess_format failed\n");
		DBG_LOG_OUT();	
		return NULL;
	}

	format_ctx = avformat_alloc_context();
	if (!format_ctx) 
	{
		dbgprintf(1,"avformat_alloc_context failed\n");
		DBG_LOG_OUT();	
		return NULL;
	}
	format_ctx->oformat = out_fmt;
	if (setup_audio_stream(format_ctx, settings) <= 0) 
	{
		dbgprintf(1,"setup_audio_stream failed\n");
		DBG_LOG_OUT();	
		return NULL;
	}
	
	DBG_LOG_OUT();	
	return format_ctx;
}

int encode_audio(AVFrame *av_frame, AVFormatContext *format_ctx, unsigned char *ucBuffer, int iBufferSize, int *iPacketSize) 
{
	AVPacket pkt;
	int ret, got_output;
	AVCodecContext *ctx; // = hls->format_ctx->streams[0]->codec;
	ctx = format_ctx->streams[0]->codec;
	
	av_init_packet(&pkt);
	
	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;
	
	// encode the samples
	ret = avcodec_encode_audio2(ctx, &pkt, av_frame, &got_output);
	if (ret < 0) 
	{
		char errbuf[256];
		memset(errbuf, 0, 256);
		av_strerror(ret, errbuf, sizeof(errbuf));
		dbgprintf(1,"encode_audio error encoding audio frame: %s\n", errbuf);
		return -1;
	}
	ret = 0;
	if (got_output) 
	{
		//pkt.stream_index = hls->format_ctx->streams[0]->index; // This must be done after avcodec_encode_audio2	
		if (pkt.size <= iBufferSize)
		{
			*iPacketSize = pkt.size;
			memcpy(ucBuffer, pkt.data, pkt.size);
			ret = 1;
		}
		else dbgprintf(5,"av_packet size %i/%i\n",pkt.size,iBufferSize);
		av_free_packet(&pkt);
	} 
	else 
	{
		dbgprintf(5,"encode_audio error: not getting audio output\n");
	}
	
	return ret;
}

/*
int open_audio_play_device(void **audio_handle, char* audio_play_dev) 
{
	int err;
	snd_pcm_t *audio_play_handle;
	snd_pcm_hw_params_t *audio_play_params;

	dbgprintf(1,"opening ALSA device for playback (preview): %s\n", audio_play_dev);
	err = snd_pcm_open((snd_pcm_t**)audio_handle, audio_play_dev, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot open audio playback (preview) device '%s': %s\n", audio_play_dev, snd_strerror(err));
		return -1;
	}
	audio_play_handle = (snd_pcm_t*)(*audio_handle);
	err = snd_pcm_hw_params_DBG_MALLOC(&audio_play_params);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot allocate hardware parameter structure for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// fill hw_params with a full configuration space for a PCM.
	err = snd_pcm_hw_params_any(audio_play_handle, audio_play_params);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot initialize hardware parameter structure for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// enable rate resampling
	unsigned int enable_resampling = 1;
	err = snd_pcm_hw_params_set_rate_resample(audio_play_handle, audio_play_params, enable_resampling);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot enable rate resampling for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	err = snd_pcm_hw_params_set_access(audio_play_handle, audio_play_params, SND_PCM_ACCESS_MMAP_INTERLEAVED);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set access type for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// SND_PCM_FORMAT_FLOAT_LE => PCM FLOAT little endian
	err = snd_pcm_hw_params_set_format(audio_play_handle, audio_play_params, SND_PCM_FORMAT_FLOAT_LE);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set sample format for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	//audio_play_channels = audio_channels;
	err = snd_pcm_hw_params_set_channels(audio_play_handle, audio_play_params, 1);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set channel count for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// set the sample rate
	unsigned int rate = 48000;
	err = snd_pcm_hw_params_set_rate_near(audio_play_handle, audio_play_params, &rate, 0);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set sample rate for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// set the buffer size
	//err = snd_pcm_hw_params_set_buffer_size(audio_play_handle, audio_play_params, 4000 * ALSA_PLAYBACK_BUFFER_MULTIPLY);
	//if (err < 0) 
	//{
	//	dbgprintf(1,"error: failed to set buffer size for audio preview: audio_buffer_size=%d error=%s\n", 4000, snd_strerror(err));
	//	return -1;
	//}

	int dir;
	// set the period size
	err = snd_pcm_hw_params_set_period_size_near(audio_play_handle, audio_play_params, (snd_pcm_uframes_t *)&period_size, &dir);
	if (err < 0) 
	{
		dbgprintf(1,"error: failed to set period size for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// apply the hardware configuration
	err = snd_pcm_hw_params (audio_play_handle, audio_play_params);
	if (err < 0) 
	{
		dbgprintf(1,"error: cannot set PCM hardware parameters for audio preview: %s\n", snd_strerror(err));
		return -1;
	}

	// end of configuration
	snd_pcm_hw_paramsfree(audio_play_params);
	
	return 0;
}*/

int open_audio_play_device(void **audio_handle, char* audio_play_dev, int iChannels, int iRate)
{
	DBG_LOG_IN();	
	
	int err;
	snd_pcm_t *audio_play_handle;
	
	if ((err = snd_pcm_open((snd_pcm_t**)audio_handle, audio_play_dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
	{
        if (err == -16) 
			dbgprintf(2,"Playback open error: %i(snd_pcm_open): %s\n", err, snd_strerror(err));
			else
			dbgprintf(1,"Playback open error: %i(snd_pcm_open): %s\n", err, snd_strerror(err));
        DBG_LOG_OUT();	
		return -1;
    }
	audio_play_handle = (snd_pcm_t*)(*audio_handle);
    if ((err = snd_pcm_set_params(audio_play_handle,
										SND_PCM_FORMAT_S16_LE,
										SND_PCM_ACCESS_RW_INTERLEAVED,
										iChannels,
										iRate,
										1,
										500000)) < 0) 
	{   /* 0.5sec */
        dbgprintf(1,"Playback open error(snd_pcm_set_params): %s\n", snd_strerror(err));
        DBG_LOG_OUT();	
		return -2;
    }
	
	DBG_LOG_OUT();	
	return 1;
}

int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
	DBG_LOG_IN();	
		
	int ret, stream_index;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) 
	{
        dbgprintf(5,"(open_codec_context) Could not find %s stream\n", av_get_media_type_string(type));
        DBG_LOG_OUT();	
		return ret;
    } 
	else 
	{
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];
        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            dbgprintf(1,"Failed to find %s codec\n", av_get_media_type_string(type));
            DBG_LOG_OUT();	
			return AVERROR(EINVAL);
        }
        /* Init the decoders, with or without reference counting */
        //if (api_mode == API_MODE_NEW_API_REF_COUNT)
        //    av_dict_set(&opts, "refcounted_frames", "1", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) 
		{
            dbgprintf(1,"Failed to open %s codec\n", av_get_media_type_string(type));
            DBG_LOG_OUT();	
			return ret;
        }
        *stream_idx = stream_index;
    }
	
	DBG_LOG_OUT();		
    return 0;
}

int GetTypesStreamsMediaFile(char *cFileName)
{	
	DBG_LOG_IN();
	
	int ret = FILE_TYPE_NA;
	AVFormatContext *fmt_ctx = NULL;	
	AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;
	int video_stream_idx = -1, audio_stream_idx = -1;
	
    av_register_all(); /* register all formats and codecs */
	if (avformat_open_input(&fmt_ctx, cFileName, NULL, NULL) < 0) /* open input file, and allocate format context */
	{
        dbgprintf(1,"GetTypesStreamsMediaFile: Could not open source file %s\n", cFileName);
        DBG_LOG_OUT();	
		return ret;
    }	
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0)	/* retrieve stream information */
	{
        dbgprintf(1,"Could not find stream information\n");
        DBG_LOG_OUT();	
		return ret;
    }
    if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) 
	{
        ret |= FILE_TYPE_VIDEO;
		avcodec_close(video_dec_ctx);	
    }
	if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) 
	{
        ret |= FILE_TYPE_AUDIO;	
		avcodec_close(audio_dec_ctx);		
    }  
	avformat_close_input(&fmt_ctx);

	DBG_LOG_OUT();	    
    return ret;
}

int ReadVideoFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* StartPack, char cStream, unsigned int ConnectNum, unsigned int ConnectID)
{
	DBG_LOG_IN();
	
	int timeout = RECV_FRAME_TIMEOUT_MS;	
	int result = 1;
	misc_buffer *pRecvBuff;
	unsigned int uiRecvSize = 0;
	
	pthread_mutex_lock(&pevntRMV.mutex);
	tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_VIDEO);
	timeout = tx_eventer_recv_data_prelocked(&pevntRMV, EVENT_NEXT_VIDEO_FRAME_DATA, (void*)&pRecvBuff, &uiRecvSize, RECV_FRAME_TIMEOUT_MS);
	if (timeout != 0)
	{	
		if (uiRecvSize == sizeof(misc_buffer))
		{
			//printf("Recved frame %i, %i\n",*iNumFrame, RECV_FRAME_TIMEOUT_MS - timeout);
			pRecvBuff->flag = *Flag;
			if (pRecvBuff->uidata[0] == 1) 
			{
				*Flag = OMX_BUFFERFLAG_EOS;
				result = 3;
			}
			if (pRecvBuff->uidata[0] == 2) 
			{
				*Flag = 0;
				result = 4;
			}
			if (pRecvBuff->uidata[0] == 0)
			{				
				memcpy(AVBuffer, pRecvBuff->void_data, sizeof(AVPacket));	
				AVPacket *pkt = AVBuffer;
				pkt->pts = pRecvBuff->pts;
				if (pRecvBuff->clock == 0) *Flag = OMX_BUFFERFLAG_SYNCFRAME | OMX_BUFFERFLAG_STARTTIME;
				if (pRecvBuff->clock == 1) *Flag = OMX_BUFFERFLAG_TIME_UNKNOWN;
				if (pRecvBuff->clock == 2) *Flag = OMX_BUFFERFLAG_SYNCFRAME | OMX_BUFFERFLAG_TIME_UNKNOWN;
				*iNumFrame = (*iNumFrame) + 1;
			}
			if (pRecvBuff->clock == 2) 
			{
				*Flag |= OMX_BUFFERFLAG_EXTRADATA;
				pRecvBuff->clock = 1;
			}			
		} else dbgprintf(2,"ReadVideoFrame: Error size data EVENT_NEXT_VIDEO_FRAME_DATA\n");
		if (!tx_eventer_return_data(&pevntRMV, RECV_FRAME_TIMEOUT_MS))
			dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
	}
	else 
	{
		result = 0;
		dbgprintf(2,"timeout video frame\n");
	}
					
	DBG_LOG_OUT();	
	return result;
}

int ReadAudioFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* StartPack, char cStream, unsigned int ConnectNum, unsigned int ConnectID)
{
	DBG_LOG_IN();
	
	AudioCodecInfo *codec_info = (AudioCodecInfo*)StartPack;
	int timeout = RECV_FRAME_TIMEOUT_MS;	
	
	*Flag = 0;
	
	void* pData;
	misc_buffer *pRecvBuff;
	unsigned int uiRecvSize = 0;
	int ret = 1;
	
	if (codec_info->CodecInfoFilled == 0)
	{
		pthread_mutex_lock(&pevntRMA.mutex);
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_CODEC);
		timeout = tx_eventer_recv_data_prelocked(&pevntRMA, EVENT_AUDIO_CODEC_INFO_DATA, &pData, &uiRecvSize, RECV_FRAME_TIMEOUT_MS);
		if (timeout != 0)
		{
			if (uiRecvSize == sizeof(AudioCodecInfo))
			{
				memcpy(StartPack, pData, sizeof(AudioCodecInfo));
				codec_info->CodecInfoFilled = 1;
				*iNumFrame = 0;
				*Flag = FLAG_AUDIO_CODEC_INFO;				
			} else dbgprintf(2,"ReadAudioFrame: Error size data EVENT_AUDIO_CODEC_INFO_DATA\n");
			if (!tx_eventer_return_data(&pevntRMA, RECV_FRAME_TIMEOUT_MS))
				dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
		}
		else 
		{
			dbgprintf(2,"timeout audio codec info frame\n");
		}
		DBG_LOG_OUT();	
		return 1;
	}
	
	pthread_mutex_lock(&pevntRMA.mutex);
	tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_AUDIO);	
		
	timeout = tx_eventer_recv_data_prelocked(&pevntRMA, EVENT_NEXT_AUDIO_FRAME_DATA, (void*)&pRecvBuff, &uiRecvSize, RECV_FRAME_TIMEOUT_MS);
	
	if (timeout != 0)
	{
		if (uiRecvSize == sizeof(misc_buffer))
		{
			*Flag = 0;
			if (pRecvBuff->uidata[0] == 1) *Flag = FLAG_STOP_AUDIO;
			if (pRecvBuff->uidata[0] == 2) *Flag = FLAG_PAUSE_AUDIO;
			if (pRecvBuff->uidata[0] == 0)
			{
				memcpy(AVBuffer, pRecvBuff->void_data, sizeof(AVPacket));
				*Flag |= FLAG_NEXT_AUDIO_FRAME;
				*iNumFrame = (*iNumFrame) + 1;
				ret = 1;
			}		
		} else dbgprintf(2,"ReadAudioFrame: Error size data EVENT_NEXT_AUDIO_FRAME_DATA\n");
		if (!tx_eventer_return_data(&pevntRMA, RECV_FRAME_TIMEOUT_MS))
			dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
	}
	else 
	{
		dbgprintf(2,"timeout audio frame\n");
		ret = 0;
	}
	
	DBG_LOG_OUT();	
	return ret;
}

int SaveVideoFrame(unsigned int iNumFrame, void* omxbuffer, int BufferSize, void* tVideoInfo, void* StartPack, void *vStream)
{
	DBG_LOG_IN();
	OMX_BUFFERHEADERTYPE *omx_buff = (OMX_BUFFERHEADERTYPE*)omxbuffer;
	if (MediaRecord_IsFree(vStream) != 0) 
	{
		//dbgprintf(7,"Not finded record thread\n");
		DBG_LOG_OUT();	
		return -1;
	}
	//printf("SaveVideoFrame %i\n", iIdStream);
		
	/*if (iIdStream >= MAX_STREAM_INFO)
	{
		dbgprintf(1,"Error ID stream %i\n",iIdStream);
		DBG_LOG_OUT();	
		return -1;
	}*/
	
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;
	omx_start_packet *Start_Packet = (omx_start_packet*)StartPack;
	VideoCodecInfo *VideoInfo = (VideoCodecInfo*)tVideoInfo;
	int result = 1;
	
	if (tx_eventer_test_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA) == 0)
	{
		if (VideoInfo->Filled == 1)
			result = tx_eventer_send_data(&pStream->pevntV, EVENT_VIDEO_INFO_DATA, tVideoInfo, sizeof(VideoCodecInfo), 1, 0);	
	
		if ((Start_Packet->CodecInfoFilled == 1) && (Start_Packet->StartFrameFilled == 2))
			result = tx_eventer_send_data(&pStream->pevntV, EVENT_START_VIDEO_FRAME_DATA, StartPack, sizeof(omx_start_packet), 1, 0);		
		//if (result) printf("saved start frame %i\n", Start_Packet->StartFramesCount);		
	}
	else
	{
		misc_buffer avSaveVideoPacket;
		avSaveVideoPacket.void_data = (void*)(omx_buff);
		avSaveVideoPacket.data_size = omx_buff->nFilledLen;	
		
		pthread_mutex_lock(&pStream->pevntV.mutex);
		tx_eventer_send_event(&pStream->pevntS, EVENT_VIDEO);
		if (tx_eventer_send_data_prelocked(&pStream->pevntV, EVENT_NEXT_VIDEO_FRAME_DATA, &avSaveVideoPacket, sizeof(misc_buffer), 1, RECV_FRAME_TIMEOUT_MS) == 0)
		{
			dbgprintf(2,"timeout save norm video frame\n");
			result = 0;
		} //else printf("saved video frame %i\n", iNumFrame);		
	}
	
	DBG_LOG_OUT();	
	return result;
}

int SaveVideoKeyFrame(unsigned int iNumFrame, void* omxbuffer, int BufferSize, void* tVideoInfo, void* StartPack, void *vStream)
{
	DBG_LOG_IN();
	
	OMX_BUFFERHEADERTYPE *omx_buff = (OMX_BUFFERHEADERTYPE*)omxbuffer;
	if (MediaRecord_IsFree(vStream) != 0) 
	{
		//dbgprintf(7,"Not finded record thread\n");
		DBG_LOG_OUT();	
		return -1;
	}
	
	/*if (iIdStream >= MAX_STREAM_INFO)
	{
		dbgprintf(1,"Error ID stream %i\n",iIdStream);
		DBG_LOG_OUT();	
		return -1;
	}*/
	
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;
	omx_start_packet *Start_Packet = (omx_start_packet*)StartPack;
	VideoCodecInfo *VideoInfo = (VideoCodecInfo*)tVideoInfo;
	int result = 1;
		
	if (tx_eventer_test_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA) == 0)
	{
		if (VideoInfo->Filled == 1)
			result = tx_eventer_send_data(&pStream->pevntV, EVENT_VIDEO_INFO_DATA, tVideoInfo, sizeof(VideoCodecInfo), 1, 0);	
	
		if ((Start_Packet->CodecInfoFilled == 1) && (Start_Packet->StartFrameFilled == 2))
		{
			result = tx_eventer_send_data(&pStream->pevntV, EVENT_START_VIDEO_FRAME_DATA, StartPack, sizeof(omx_start_packet), 1, 0);
			/*omx_start_packet StartP;
			memcpy(&StartP, StartPack, sizeof(omx_start_packet));
			StartP.StartFrameLen = StartP.StartFramesSizes[0];
			StartP.StartFramesSizes[1] = 0;
			StartP.StartFramesFlags[1] = 0;
			StartP.StartFramesCount = 1;
			result = tx_eventer_send_data(&Stream[iIdStream].pevntV, EVENT_START_VIDEO_FRAME_DATA, &StartP, sizeof(omx_start_packet), 1, 0);*/
		}
	}
	else
	{
		if (omx_buff->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
		{			
			misc_buffer avSaveVideoPacket;
			avSaveVideoPacket.void_data = (void*)(omx_buff);
			avSaveVideoPacket.data_size = omx_buff->nFilledLen;
			
			pthread_mutex_lock(&pStream->pevntV.mutex);
			tx_eventer_send_event(&pStream->pevntS, EVENT_VIDEO);			
			if (tx_eventer_send_data_prelocked(&pStream->pevntV, EVENT_NEXT_VIDEO_FRAME_DATA, &avSaveVideoPacket, sizeof(misc_buffer), 1, RECV_FRAME_TIMEOUT_MS) == 0)
			{
				dbgprintf(2,"timeout save slow video frame\n");
				result = 0;
			}
		}
	}
	
	DBG_LOG_OUT();	
	return result;
}

int SaveDiffVideoFrame(unsigned int iNumFrame, void* omxbuffer, int BufferSize, void* tVideoInfo, void* StartPack, void *vStream)
{
	DBG_LOG_IN();	
	
	OMX_BUFFERHEADERTYPE *omx_buff = (OMX_BUFFERHEADERTYPE*)omxbuffer;
	if (MediaRecord_IsFree(vStream) != 0) 
	{
		//dbgprintf(7,"Not finded record thread\n");
		//printf("Not finded record thread\n");
		DBG_LOG_OUT();	
		return -1;
	}
	
	/*if (iIdStream >= MAX_STREAM_INFO)
	{
		dbgprintf(1,"Error ID stream %i\n",iIdStream);
		DBG_LOG_OUT();	
		return -1;
	}*/
	
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;
	omx_start_packet *Start_Packet = (omx_start_packet*)StartPack;
	VideoCodecInfo *VideoInfo = (VideoCodecInfo*)tVideoInfo;
	int result = 1;
	
	if (tx_eventer_test_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA) == 0)
	{
		if (VideoInfo->Filled == 1)
			result = tx_eventer_send_data(&pStream->pevntV, EVENT_VIDEO_INFO_DATA, tVideoInfo, sizeof(VideoCodecInfo), 1, 0);	
		if ((Start_Packet->CodecInfoFilled == 1) && (Start_Packet->StartFrameFilled == 2) && (Start_Packet->HaveChangedFrames != 0))
			result = tx_eventer_send_data(&pStream->pevntV, EVENT_START_VIDEO_FRAME_DATA, StartPack, sizeof(omx_start_packet), 1, 0);
	}
	else
	{
		misc_buffer avSaveVideoPacket;
		avSaveVideoPacket.void_data = (void*)(omx_buff);
		avSaveVideoPacket.data_size = omx_buff->nFilledLen;	
		avSaveVideoPacket.flag = Start_Packet->HaveChangedFrames;
		
		pthread_mutex_lock(&pStream->pevntV.mutex);
		tx_eventer_send_event(&pStream->pevntS, EVENT_VIDEO);
		if (tx_eventer_send_data_prelocked(&pStream->pevntV, EVENT_NEXT_VIDEO_FRAME_DATA, &avSaveVideoPacket, sizeof(misc_buffer), 1, RECV_FRAME_TIMEOUT_MS) == 0)
		{
			dbgprintf(2,"timeout save diff video frame\n");
			result = 0;
		}
	}
	
	DBG_LOG_OUT();	
	return result;
}

int SkipAudioFrame(void *vStream)
{
	/*if (iIdStream >= MAX_STREAM_INFO)
	{
		dbgprintf(1,"Error ID stream %i\n",iIdStream);
		return -1;
	}*/
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;
	tx_eventer_add_event(&pStream->pevntA, EVENT_SKIP_AUDIO_FRAME_DATA); 
	return 1;
}  

int UnSkipAudioFrame(void *vStream)
{
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;
	tx_eventer_delete_event(&pStream->pevntA, EVENT_SKIP_AUDIO_FRAME_DATA);
	return 1;
}	
		
int SaveAudioFrame(unsigned int iNumFrame, char* Buffer, int LenData, void* pData, void *vStream)
{
	if (MediaRecord_IsFree(vStream) != 0) return -1;
	
	/*if (iIdStream >= MAX_STREAM_INFO)
	{
		dbgprintf(1,"Error ID stream %i\n",iIdStream);
		return -1;
	}*/
	
	STREAM_INFO *pStream = (STREAM_INFO*)vStream;	
	AudioCodecInfo* tCodecInfo = (AudioCodecInfo*)pData;
	int result = 1;
	
	if (tx_eventer_test_event(&pStream->pevntA, EVENT_NEED_AUDIO_DATA) == 0)
	{
		result = tx_eventer_send_data(&pStream->pevntA, EVENT_AUDIO_CODEC_INFO_DATA, tCodecInfo, sizeof(AudioCodecInfo), 1, 0);	
	}
	else
	{	
		pthread_mutex_lock(&pStream->pevntA.mutex);
		tx_eventer_send_event(&pStream->pevntS, EVENT_AUDIO);
		if (tx_eventer_send_data_prelocked(&pStream->pevntA, EVENT_NEXT_AUDIO_FRAME_DATA, Buffer, LenData, 1, RECV_FRAME_TIMEOUT_MS) == 0)	
		{		
			dbgprintf(2,"timeout save AUDIO frame\n");
			result = 0;
		}
	}
	
	return result;
}

int Add_Video_Stream(AVFormatContext *ost, AVStream **v_st, VideoCodecInfo *tVideoInfo)
{
    AVCodecContext *cc;
    AVStream *video_st;
	video_st = avformat_new_stream(ost, NULL);
    if (!video_st) 
	{
        dbgprintf(1,"Could not allocate stream\n");
        return 0;
    }
	*v_st = video_st;
	video_st->id = ost->nb_streams-1;
    video_st->time_base.den = tVideoInfo->video_frame_rate;
	video_st->time_base.num = 1;
	video_st->r_frame_rate.num = tVideoInfo->video_frame_rate;
	video_st->r_frame_rate.den = 1;
	video_st->start_time = AV_NOPTS_VALUE;
	
	cc = video_st->codec;
	cc->codec_type = AVMEDIA_TYPE_VIDEO; 
	cc->codec_id = tVideoInfo->video_codec;
	//cc->profile = FF_PROFILE_H264_HIGH;
	//cc->level = 41;
	//cc->profile       = FF_PROFILE_H264_CONSTRAINED_BASELINE;
	//cc->level         = 31;  // Level 3.1
	cc->bit_rate = tVideoInfo->video_bit_rate;
	cc->width    = tVideoInfo->video_width;
	cc->height   = tVideoInfo->video_height;
	cc->time_base.den = tVideoInfo->video_frame_rate;
	cc->time_base.num = 1;	
	cc->ticks_per_frame = 2;

	cc->gop_size      = tVideoInfo->video_intra_frame; // emit one intra frame every twelve frames at most 
	cc->pix_fmt       = AV_PIX_FMT_YUV420P;
	cc->sample_aspect_ratio.num = video_st->sample_aspect_ratio.num;
	cc->sample_aspect_ratio.den = video_st->sample_aspect_ratio.den;
	cc->has_b_frames  = 0;
  
	if (cc->codec_id == AV_CODEC_ID_MPEG2VIDEO) cc->max_b_frames = 2;
	if (cc->codec_id == AV_CODEC_ID_MPEG1VIDEO) cc->mb_decision = 2;
	if (ost->oformat->flags & AVFMT_GLOBALHEADER) cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	return 1;
}

int Add_Audio_Stream(AVFormatContext *oc, AVStream **a_st, AudioCodecInfo *tAudioInfo)
{
	AVCodecContext *cc;
    AVStream *audio_st;
	AVCodec *codec = avcodec_find_encoder(tAudioInfo->audio_codec);
    if (!codec)
	{
        dbgprintf(1,"Could not find encoder for '%s'\n", avcodec_get_name(tAudioInfo->audio_codec));
        return 0;
    }
    audio_st = avformat_new_stream(oc, codec);
    if (!audio_st) 
	{
        dbgprintf(1,"Could not allocate stream\n");
        return 0;
    }
	*a_st = audio_st;
    audio_st->id = oc->nb_streams-1;
    cc = audio_st->codec;
	cc->codec_type = AVMEDIA_TYPE_AUDIO;
	cc->codec_id = tAudioInfo->audio_codec;
	cc->profile = tAudioInfo->audio_profile;
	cc->sample_fmt  = codec->sample_fmts[0]; //AV_SAMPLE_FMT_FLTP;
	cc->bit_rate    = tAudioInfo->audio_bit_rate;
	cc->sample_rate = tAudioInfo->audio_sample_rate;
	cc->channels        = av_get_channel_layout_nb_channels(cc->channel_layout);
	if (tAudioInfo->audio_channels == 1) cc->channel_layout = AV_CH_LAYOUT_MONO;
	if (tAudioInfo->audio_channels == 2) cc->channel_layout = AV_CH_LAYOUT_STEREO;
	cc->channels        = av_get_channel_layout_nb_channels(cc->channel_layout);
	audio_st->time_base = (AVRational){ 1, cc->sample_rate };
	
	audio_st->time_base.den = tAudioInfo->audio_frame_rate;
	audio_st->time_base.num = 1;
	cc->time_base.den = tAudioInfo->audio_frame_rate;
	cc->time_base.num = 1;	
				
    // Some formats want stream headers to be separate. 
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	int ret;
    ret = avcodec_open2(audio_st->codec, codec, NULL);
	if (ret < 0) 
	{
        dbgprintf(1,"Could not allocate stream\n");
        return 0;
    }
    return 1;
}

int Write_Video_Frame(AVFormatContext *oc, AVStream *video_st, VideoCodecInfo *tVideoInfo, void *omxbuffer)
{	
	int ret = -1;
	AVPacket pkt = { 0 };
	OMX_BUFFERHEADERTYPE *omx_buff = (OMX_BUFFERHEADERTYPE*)omxbuffer;
    AVCodecContext *c = video_st->codec;
	
/*	if (av_compare_ts(tVideoInfo->FrameNum, video_st->codec->time_base,
                      10, (AVRational){ 1, 1 }) >= 0)
	{
		dbgprintf(1,"error av_compare_ts\n");
        return 0;
	}*/
	av_init_packet(&pkt);
	
	//cBuff = (char*)DBG_MALLOC(omx_buff->nFilledLen);
	//memcpy(cBuff, omx_buff->pBuffer, omx_buff->nFilledLen);
	
	pkt.data = omx_buff->pBuffer;
	pkt.size = omx_buff->nFilledLen;
	pkt.stream_index = video_st->index;	
	if (omx_buff->nFlags & (OMX_BUFFERFLAG_ENDOFFRAME | EVENT_VIDEO_CODEC_INFO_DATA)) 
	{
		//printf("OMX_BUFFERFLAG_ENDOFFRAME\n");
		//*iPts = (*iPts) + 1;		
		tVideoInfo->FrameNum++;
	}
	pkt.dts = pkt.pts = av_rescale_q(tVideoInfo->FrameNum, c->time_base, video_st->time_base);
	
	if (omx_buff->nFlags & (OMX_BUFFERFLAG_SYNCFRAME | OMX_BUFFERFLAG_STARTTIME | EVENT_VIDEO_CODEC_INFO_DATA)) 
		pkt.flags |= AV_PKT_FLAG_KEY;
	/*if (omx_buff->nFlags & EVENT_VIDEO_CODEC_INFO_DATA) 
	{
		pkt.dts = pkt.pts = 0;
		tVideoInfo->FrameNum = 0;
	}*/
	//fwrite(cBuff, 1, omx_buff->nFilledLen, outfile2);
//	dbgprintf(3,"%i\n",omx_buff->nFilledLen);	
	//omxtimebase.num = 1;
	//omxtimebase.den = tVideoInfo->video_frame_rate;
	//OMX_TICKS tick = omx_buff->nTimeStamp;
	//pkt.dts = pkt.pts = av_rescale_q(tVideoInfo->FrameNum, 
      //                 omxtimebase, oc->streams[video_st->index]->time_base);
	//pkt.dts = pkt.pts = av_rescale_q(((((uint64_t)tick.nHighPart)<<32) | tick.nLowPart), 
      //                 omxtimebase, oc->streams[video_st->index]->time_base);

	//pkt.pts = tVideoInfo->FrameNum * 10 / tVideoInfo->video_frame_rate;
	//pkt.dts = AV_NOPTS_VALUE;
	//printf("1 pkt.dts %i, pts %i /// omxH %i, omxL %i /// codecH %i, codec L %i\n", pkt.dts, pkt.pts, omx_buff->nTimeStamp.nHighPart, omx_buff->nTimeStamp.nLowPart, c->time_base.num , c->time_base.den);
	//pkt.pts = pkt.dts = *iPts;
	
    
	/*ret = av_write_frame(oc, &pkt);
    if (ret < 0) 
	{
        dbgprintf(3,"Error while writing frame: %s\n", av_err2str(ret));
        return 0;
    }
	*/
	//printf("2 pkt.dts %i, pts %i /// omxH %i, omxL %i /// codecH %i, codec L %i\n", pkt.dts, pkt.pts, omx_buff->nTimeStamp.nHighPart, omx_buff->nTimeStamp.nLowPart, c->time_base.num , c->time_base.den);
	//av_packet_rescale_ts(&pkt, c->time_base, video_st->time_base);
    ret = av_interleaved_write_frame(oc, &pkt);
	//ret = av_write_frame(oc, &pkt);
   //DBG_FREE(cBuff);
	if (ret < 0) 
	{
        dbgprintf(1,"Error av_interleaved_write_frame: %s\n", av_err2str(ret));
        return 0;
    }	
	return 1;
}

int Write_Audio_Frame(AVFormatContext *oc, AVStream *audio_st, AudioCodecInfo *tAudioInfo, char* pBuffer, int iDataLen)
{
    int ret = -1;
    AVCodecContext *c;
    c = audio_st->codec;
    /* a hack to avoid data copy with some raw muxers */
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.flags        |= AV_PKT_FLAG_KEY;
    pkt.stream_index  = audio_st->index;
    pkt.data          = (uint8_t *)pBuffer;
    pkt.size          = iDataLen;
    //*iPts = (*iPts) + 1;
    //pkt.pts = pkt.dts = *iPts;
	tAudioInfo->FrameNum++;
	pkt.dts = pkt.pts = av_rescale_q(tAudioInfo->FrameNum, c->time_base, audio_st->time_base);
	//av_packet_rescale_ts(&pkt, c->time_base, audio_st->time_base);
    
	ret = av_interleaved_write_frame(oc, &pkt);
    //ret = av_write_frame(oc, &pkt);
    if (ret < 0) 
	{
        dbgprintf(1,"Error while writing frame: %s\n", av_err2str(ret));
        return 0;
    }	
    return 1;
}

void CloseStream(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
}

int PlayAudioSound(func_link *f_link)
{
	DBG_LOG_IN();
	MODULE_INFO *miModule = f_link->pModule;
	
	if (iAudioStarted == 0)
	{
		dbgprintf(1,"error: PlayAudioSound, audio not init\n");
		return 0;
	}
	if (!(miModule->Type & MODULE_TYPE_SPEAKER))
	{
		dbgprintf(1,"error: PlayAudioSound, wrong module type\n");
		return 0;
	}
	
	if (f_link->mbuffer.data_size == 0)
	{
		dbgprintf(1,"error: PlayAudioSound, wrong sound type\n");
		return 0;
	}
	
	DBG_MUTEX_LOCK(&Play_Mutex);	
	if (iPlaySoundCnt < (miModule->Settings[4]))
	{
		iPlaySoundCnt++;
		DBG_MUTEX_UNLOCK(&Play_Mutex);
		if (f_link->ConnectNum == -1)
		{
			f_link->ConnectNum = 0;
			f_link->mbuffer.void_data2 = f_link->mbuffer.void_data;
		}
		else
		{
			f_link->mbuffer.void_data2 = (char*)DBG_MALLOC(f_link->mbuffer.data_size);
			memcpy(f_link->mbuffer.void_data2, f_link->mbuffer.void_data, f_link->mbuffer.data_size);
			int iVolumePercent = f_link->mbuffer.uidata[0];
			if ((iVolumePercent <= 0) || (iVolumePercent > 500)) iVolumePercent = 100;
			if (iVolumePercent != 100)
			{
				float fVolume = (float)iVolumePercent / 100;
				int16_t *pSoundData = (int16_t *)f_link->mbuffer.void_data2;
				unsigned int uiDataLen = f_link->mbuffer.data_size / 2;			
				int i;
				int temp;
				for (i = 0; i < uiDataLen; i++) 
				{
					temp = pSoundData[i] * fVolume;
					if (temp > 32767) temp = 32767;
					if (temp < -32767) temp = -32767;
					pSoundData[i] = temp;
				}
			}
		}
		pthread_create(&thread_play_audio, &tattr_play_audio, thread_PlayAudioSound, (void*)f_link);
	} 
	else 
	{
		DBG_MUTEX_UNLOCK(&Play_Mutex);
		dbgprintf(4,"thread_PlayAudioSound, busy\n");
		if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);
		DBG_FREE(f_link->pModule);
		DBG_FREE(f_link);
		
		DBG_LOG_OUT();
		return 0;
	}
	
	DBG_LOG_OUT();
	return 1;
}

int PlayAudioStream(func_link *f_link)
{	
	DBG_LOG_IN();
	MODULE_INFO *miModule = f_link->pModule;
	
	if (iAudioStarted == 0)
	{
		dbgprintf(1,"error: audio_play_aac_stream, audio not init\n");
		DBG_LOG_OUT();
		return 0;
	}
	if (!(miModule->Type & MODULE_TYPE_SPEAKER))
	{
		dbgprintf(1,"error: audio_play_aac_stream, wrong module type\n");
		DBG_LOG_OUT();
		return 0;
	}
	if (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_BUSY))
	{
		dbgprintf(1,"Play audio stream BUSY\n");
		DBG_LOG_OUT();
		return 0;
	}
	if (f_link->ConnectNum == 1)
	{
		if (f_link->DeviceNum == 0)
				f_link->ConnectNum = TCP_Client(&f_link->RecvAddress, TRAFFIC_AUDIO, FLAG_AUDIO_CODEC_INFO | FLAG_AUDIO_STREAM, 0, &f_link->ConnectID);
		if (f_link->DeviceNum)
				f_link->ConnectNum = TCP_Client(&f_link->RecvAddress, TRAFFIC_REMOTE_AUDIO, FLAG_AUDIO_CODEC_INFO | FLAG_AUDIO_STREAM, 0, &f_link->ConnectID);		
		//if (f_link->DeviceNum == 2)
			//	f_link->ConnectNum = TCP_Client(&f_link->RecvAddress, TRAFFIC_REMOTE_AUDIO, FLAG_AUDIO_CODEC_INFO, 0, &f_link->ConnectID);		
		
		if (f_link->ConnectNum <= 0)
		{
			dbgprintf(1,"Error connect to AudioModule\n");
			DBG_LOG_OUT();
			return 0;
		}
	}	
	//tx_eventer_recv(&pevnt_audio_run, NULL, TX_WAIT_FOREVER, 1);
	pthread_create(&thread_play_audio, &tattr_play_audio, thread_PlayAudioStream, (void*)f_link);
	DBG_LOG_OUT();
	return 1;
}

int CaptureAudioStream(void *flink)
{
	DBG_LOG_IN();
	pthread_create(&thread_capture_audio, &tattr_capture_audio, thread_CaptureAudioStream, (void*)flink); 		
	DBG_LOG_OUT();
	return 1;
}

int SaveCapturedStreams(char *cPath, STREAM_INFO *pStream, int iMaxFileSize, int MaxDuration, int iHaveVideo, int iHaveAudio, 
	char cCopyFile, char* cSavePath, unsigned int uiDiff, unsigned int uiSelfEnc, unsigned int uiAudioCodec, unsigned char ucOrderLmt, 
	char *cAddrOrderer, char *cPrePath, unsigned int uiFlvBuffSize, unsigned int uiMaxWaitCopyTime, unsigned int uiMessWaitCopyTime,
	unsigned int uiBackUpType)
{
	DBG_LOG_IN();
	if ((iHaveVideo == 0) && (iHaveAudio == 0)) return 0;
	misc_buffer *mBuff = (misc_buffer*)DBG_CALLOC(sizeof(misc_buffer),1);
	
	int i;
	char *buff;
	
	i = strlen(cPath);
	mBuff->data = (char*)DBG_CALLOC(i + 2,1);	
	if (i < MAX_PATH) 
	{
		strcpy(mBuff->data, cPath);
		if (mBuff->data[i - 1] != 47) mBuff->data[i] = 47;
	}
	else dbgprintf(2, "Big length SaveCapturedStreams link cPath(%i) %i, max %i", (intptr_t)pStream, strlen(cPath), MAX_PATH);
	
	i = strlen(cSavePath);
	mBuff->void_data = (char*)DBG_CALLOC(i + 2,1);
	if (i < MAX_PATH) 
	{
		strcpy(mBuff->void_data, cSavePath);
		buff = mBuff->void_data;
		if (buff[i - 1] != 47) buff[i] = 47;
	}
	else dbgprintf(2, "Big length SaveCapturedStreams link cSavePath(%i) %i, max %i", (intptr_t)pStream, strlen(cSavePath), MAX_PATH);
	
	if (cAddrOrderer && (strlen(cAddrOrderer) < 8)) cAddrOrderer = NULL;
	
	if (cAddrOrderer)
	{
		mBuff->void_data2 = (char*)DBG_CALLOC(64,1);
		if (strlen(cAddrOrderer) < 64) strcpy(mBuff->void_data2, cAddrOrderer); 
			else dbgprintf(2, "Big length SaveCapturedStreams AddOrderer(%i) %i, max 64", (intptr_t)pStream, strlen(cAddrOrderer));	
	} else mBuff->void_data2 = NULL;
	
	i = strlen(cPrePath);
	mBuff->void_data4 = (char*)DBG_CALLOC(i + 2,1);
	if (i < MAX_PATH) 
	{
		strcpy(mBuff->void_data4, cPrePath);
		buff = mBuff->void_data4;
		if (strlen(cPrePath) && (buff[i - 1] != 47)) buff[i] = 47;
	} else dbgprintf(2, "Big length SaveCapturedStreams link cPrePath(%i) %i, max %i", (intptr_t)pStream, strlen(cPrePath), MAX_PATH);
	
	mBuff->void_data3 = pStream;
	mBuff->data_size = iMaxFileSize * 1000000;
	mBuff->dsize = MaxDuration;
	mBuff->flag = 0;
	mBuff->frsize = cCopyFile;
	mBuff->uidata[0] = uiAudioCodec;
	mBuff->uidata[1] = ucOrderLmt;
	mBuff->uidata[2] = uiFlvBuffSize;
	mBuff->uidata[3] = uiMaxWaitCopyTime;
	mBuff->uidata[4] = uiMessWaitCopyTime;
	mBuff->uidata[5] = uiBackUpType;
	
	if (iHaveVideo) 		mBuff->flag |= 1;
	if (iHaveAudio) 		mBuff->flag |= 2;
	if (uiDiff)				mBuff->flag |= 4;
	if (uiSelfEnc)			mBuff->flag |= 8;
	
	pthread_create(&thread_capture_sreams, &tattr_capture_sreams, thread_SaveFLVFile, (void*)mBuff); 		
	
	DBG_LOG_OUT();
	return 1;
}

int PlayMediaFile(char *cPath, int video_enable, int audio_enable, char overTCP)
{
	DBG_LOG_IN();
	if (MediaPlay_IsFree() == 0)
	{
		dbgprintf(2, "MediaPlay busy\n");
		DBG_LOG_OUT();
		return 0;
	}
	if ((video_enable == 0) && (audio_enable == 0)) return 0;
	misc_buffer *mBuff = (misc_buffer*)DBG_CALLOC(sizeof(misc_buffer),1);
	mBuff->data = (char*)DBG_CALLOC(1024,1);
	strcpy(mBuff->data, cPath);
	
	mBuff->flag = 0;
	if (video_enable) mBuff->flag |= 1;
	if (audio_enable) mBuff->flag |= 2;
	TX_EVENTER 	*pevnt_init;
	pevnt_init = DBG_MALLOC(2*sizeof(TX_EVENTER));	
    mBuff->void_data = (void*)pevnt_init;
	mBuff->dsize = 0;
	mBuff->uidata[0] = overTCP;
	tx_eventer_clear(&pevntRMS);	
	tx_eventer_create(&pevnt_init[0], 0); 
    tx_eventer_create(&pevnt_init[1], 0); 
    tx_eventer_add_event(&pevnt_init[0], 2);  
	pthread_create(&thread_play_media, &tattr_play_media, thread_PlayMediaFile, (void*)mBuff); 		
	tx_eventer_recv_event(&pevnt_init[0], 2, TX_WAIT_FOREVER);
	int ret = mBuff->dsize;
	tx_eventer_send_event(&pevnt_init[1], 1);  
	DBG_LOG_OUT();
	return ret;
}

unsigned int GetCodeFromSampleRate(unsigned int uiSampleRate)
{
	int ret = SAMPLE_RATE_UNKNOWN;
	switch(uiSampleRate)
	{
		case 8000:
			ret = SAMPLE_RATE_8000;
			break;
		case 11025:
			ret = SAMPLE_RATE_11025;
			break;
		case 16000:
			ret = SAMPLE_RATE_16000;
			break;
		case 22050:
			ret = SAMPLE_RATE_22050;
			break;
		case 44100:
			ret = SAMPLE_RATE_44100;
			break;
		case 48000:
			ret = SAMPLE_RATE_48000;
			break;
		case 96000:
			ret = SAMPLE_RATE_96000;
			break;
		default:
			ret = SAMPLE_RATE_UNKNOWN;
			break;
	}
	return ret;
}

unsigned int GetSampleRateFromCode(unsigned int uiCode)
{
	int ret = 0;
	if (uiCode & SAMPLE_RATE_8000) ret = 8000;
	if (uiCode & SAMPLE_RATE_11025) ret = 11025;
	if (uiCode & SAMPLE_RATE_16000) ret = 16000;
	if (uiCode & SAMPLE_RATE_22050) ret = 22050;
	if (uiCode & SAMPLE_RATE_44100) ret = 44100;
	if (uiCode & SAMPLE_RATE_48000) ret = 48000;
	if (uiCode & SAMPLE_RATE_96000) ret = 96000;
	
	return ret;
}

unsigned int GetSupportSampleRate(unsigned int uiCurrentRate, unsigned int uiSupportRates)
{
	unsigned int res = 0;
	if (uiSupportRates == SAMPLE_RATE_ANY) return uiCurrentRate;
	int ret = GetCodeFromSampleRate(uiCurrentRate);
	if (ret == 0) 
	{
		ret = SAMPLE_RATE_UNKNOWN;
		do
		{
			ret = ret >> 1;
		} while (((uiSupportRates & ret) == 0) && (ret));
		if (ret == 0)
		{
			dbgprintf(1,"Failed to calculate samplerate supported:%i for %i\n", uiSupportRates, uiCurrentRate);
			return 0;
		}
		res = GetSampleRateFromCode(uiSupportRates & ret);
	}
	else
	{
		do
		{
			ret = ret << 1;							
		} while (((uiSupportRates & ret) == 0) && (ret < SAMPLE_RATE_UNKNOWN));
		if (ret >= SAMPLE_RATE_UNKNOWN)
		{
			ret = GetCodeFromSampleRate(uiCurrentRate);
			do
			{
				ret = ret >> 1;							
			} while (((uiSupportRates & ret) == 0) && (ret));
		}
		if ((ret == 0) || (ret >= SAMPLE_RATE_UNKNOWN))
		{
			dbgprintf(1,"Not finded supported samplerate:%i for %i\n", uiSupportRates, uiCurrentRate);
			return 0;
		}
		res = GetSampleRateFromCode(ret);
	}	
	return res;
}

int GetDoneSoundPlay(void* pData, unsigned int uiTimeWait)
{	
	int ret = 0;
	int res = 0;
	unsigned int uiTimeMs;
	DBG_MUTEX_LOCK(&Play_Mutex);
	if (iPlaySoundCnt > 0) iPlaySoundCnt--;
	if ((pData != NULL) && (iPlaySoundCnt == 0) && (uiTimeWait != 0))
	{
		if (iWaitPwrSoundTimer == 0) ret = 1; 
		else 
		{
			ret = 2;
			iWaitPwrSoundTimer = 0;		
		}
		get_ms(&iWaitPwrSoundTimer);
	}
	DBG_MUTEX_UNLOCK(&Play_Mutex);
	
	if (ret == 0) 
	{
		//dbgprintf(3,"IN WORK %i\n", gettid());
		return 0;
	}
	if (ret == 2)
	{
		//dbgprintf(3,"SKIP WAIT %i\n", gettid());
		return 0;
	}
	if (ret == 1)
	{
		uiTimeWait *= 1000;
		uiTimeMs = uiTimeWait;
		ret = 0;
		//dbgprintf(3,"GET WAIT %i %i\n", gettid(), uiTimeWait);			
		do
		{
			usleep(uiTimeMs * 1000);		
			DBG_MUTEX_LOCK(&Play_Mutex);
			if (iPlaySoundCnt > 0) 
			{
				iWaitPwrSoundTimer = 0;
				ret = 1;
				//dbgprintf(3,"BREAK WAIT %i\n", gettid());
			}
			else
			{
				uiTimeMs = (unsigned int)get_ms(&iWaitPwrSoundTimer);
				if (uiTimeWait <= uiTimeMs)
				{
					iWaitPwrSoundTimer = 0;
					ret = 2;
					//dbgprintf(3,"DONE WAIT %i\n", gettid(), uiTimeMs);
				}
				else
				{
					uiTimeMs = uiTimeWait - uiTimeMs;
					//dbgprintf(3,"NEED MORE WAIT %i\n", gettid());		
				}
			}
			DBG_MUTEX_UNLOCK(&Play_Mutex);	
		} while (ret == 0);
		if (ret == 1) res = 0;
		if (ret == 2) res = 1;
	}
	return res;
}

int GetAudioCodecCode(int iCodecNum)
{
	int res = AV_CODEC_ID_AAC;
	switch(iCodecNum)
	{
		case 0:	res = AV_CODEC_ID_AAC; break;
		case 1: res = AV_CODEC_ID_MP3; break;
		case 2: res = AV_CODEC_ID_SPEEX; break;
		case 3: res = AV_CODEC_ID_PCM_S16LE; break;
		case 4: res = AV_CODEC_ID_ADPCM_SWF; break;
		case 5: res = AV_CODEC_ID_NELLYMOSER; break;
		case 6: res = AV_CODEC_ID_MP2; break;
		case 7: res = AV_CODEC_ID_WMAV1; break;
		case 8: res = AV_CODEC_ID_WMAV2; break;
		case 9: res = AV_CODEC_ID_WMAVOICE; break;
		case 10: res = AV_CODEC_ID_WMAPRO; break;
		case 11: res = AV_CODEC_ID_WMALOSSLESS; break;
		case 12: res = AV_CODEC_ID_MP1; break;
		case 13: res = AV_CODEC_ID_MPEG2TS; break;
		case 14: res = AV_CODEC_ID_AC3; break;
		case 15: res = AV_CODEC_ID_DTS; break;
		case 16: res = AV_CODEC_ID_VORBIS; break;
		case 17: res = AV_CODEC_ID_WAVPACK; break;
		case 18: res = AV_CODEC_ID_EAC3; break;
		default: res = AV_CODEC_ID_AAC; break;
	}
	return res;
}

int AudioFileInBuffer(char *cFileName, void **pBufferOut, unsigned int *uiBufferSize, unsigned int uiSampleRateFilter, unsigned int audio_channels)
{
	DBG_LOG_IN();
	
	AVFormatContext *fmt_ctx = NULL;	
	AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;
	AVStream *audio_stream = NULL;
	AVStream *video_stream = NULL;
	
	int video_stream_idx = -1, audio_stream_idx = -1;
	AVFrame *frame = NULL;
	int errnum;
	
	char audio_enable = 1;
	char cErrbuff[256];
	int iTypeStream = 0;
	if (SearchData("://", 3, cFileName, strlen(cFileName), 0) > 0) iTypeStream = 1;
	
	av_register_all(); /* register all formats and codecs */
	avcodec_register_all();
    if (iTypeStream == 1) avformat_network_init();
	
	AVCodec *codec;
	//AVCodecContext *ctx;
	AVDictionary *stream_opts = 0;
	av_dict_set_int(&stream_opts, "timeout", 15000000, 0);
	
	*uiBufferSize = 0;
	*pBufferOut = NULL;
	void *pSoundBuffer = NULL;
	void* pDecodedBufferOut = NULL;
	
	errnum = avformat_open_input(&fmt_ctx, (char*)cFileName, NULL, &stream_opts);
	if (errnum < 0) /* open input file, and allocate format context */
	{		
		if (iTypeStream == 0) 
			dbgprintf(2,"AudioFileInBuffer: Could not open source file (0) %s\n", (char*)cFileName);
			else 
			{
				dbgprintf(2,"AudioFileInBuffer: Could not open source file\n");
				printf("Could not open source file %s\n", (char*)cFileName);
			}
		av_strerror(errnum, cErrbuff, 256);
		dbgprintf(2,"AudioFileInBuffer: %s\n", cErrbuff);
		if (iTypeStream == 1) avformat_network_deinit();
		DBG_LOG_OUT();
		return -1;
    }	
	errnum = avformat_find_stream_info(fmt_ctx, NULL);
	if (errnum < 0)	/* retrieve stream information */
	{
        av_strerror(errnum, cErrbuff, 256);
		dbgprintf(3,"AudioFileInBuffer: %s\n", cErrbuff);
		dbgprintf(1,"AudioFileInBuffer: Could not find stream information\n");
        if (iTypeStream == 1) avformat_network_deinit();
		DBG_LOG_OUT();
		return -2;
    }
	if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) 
	{
		video_stream = fmt_ctx->streams[video_stream_idx];  
        video_dec_ctx = video_stream->codec;
    }
	
	if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) 
	{
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dec_ctx = audio_stream->codec; 
    } else audio_enable = 0;
	
	if ((!audio_stream) || (audio_enable == 0)) 
	{
        dbgprintf(1,"AudioFileInBuffer: Could not find audio stream, aborting\n");
		dbgprintf(1,"File '%s'\n", cFileName);
		
        avcodec_close(video_dec_ctx);
		avcodec_close(audio_dec_ctx);
		avformat_close_input(&fmt_ctx);   
		if (iTypeStream == 1) avformat_network_deinit();
        DBG_LOG_OUT();
		return -3;
    }
	
	frame = av_frame_alloc();
    if (!frame) 
	{
        dbgprintf(1,"AudioFileInBuffer: Could not allocate frame\n");
        avcodec_close(video_dec_ctx);
		avcodec_close(audio_dec_ctx);
		avformat_close_input(&fmt_ctx); 
		if (iTypeStream == 1) avformat_network_deinit();
        DBG_LOG_OUT();
		return -4;
    }
	 
	if (audio_stream)
	{
		if (audio_dec_ctx->frame_size == 0) 
		{
			dbgprintf(2, "ffmpeg frame size is zero\n");
			avcodec_close(video_dec_ctx);
			avcodec_close(audio_dec_ctx);
			avformat_close_input(&fmt_ctx); 
			if (iTypeStream == 1) avformat_network_deinit();
			DBG_LOG_OUT();
			return -5;
		}
	}
	
	
	av_register_all();
	codec = avcodec_find_decoder(audio_dec_ctx->codec_id);
	if (!codec) 
	{
		dbgprintf(2,"Codec not found %i\n", audio_dec_ctx->codec_id);
		avcodec_close(video_dec_ctx);
		avcodec_close(audio_dec_ctx);
		avformat_close_input(&fmt_ctx); 
		if (iTypeStream == 1) avformat_network_deinit();
        DBG_LOG_OUT();
		return -6;
	}
	
	if ((audio_channels == 0) && (audio_dec_ctx->channel_layout == AV_CH_LAYOUT_MONO)) audio_channels = 1;
	if (audio_channels != 1) audio_channels = 2;
		
	AVFrame *decoded_frame = NULL;	
	decoded_frame = av_frame_alloc();
	if (!decoded_frame) 
	{
		dbgprintf(1,"Could not allocate audio frame\n");
		//avcodec_close(ctx);
		//av_free(ctx);
		avcodec_close(video_dec_ctx);
		avcodec_close(audio_dec_ctx);
		avformat_close_input(&fmt_ctx); 
		if (iTypeStream == 1) avformat_network_deinit();
        DBG_LOG_OUT();
		return -7;
	}
	decoded_frame->nb_samples = audio_dec_ctx->frame_size;
	decoded_frame->format = audio_dec_ctx->sample_fmt;
	decoded_frame->channel_layout = audio_dec_ctx->channel_layout;
	decoded_frame->sample_rate = audio_dec_ctx->sample_rate;
	decoded_frame->channels = audio_dec_ctx->channels; 
					
	float ml_sample = 1.0f;
	unsigned int audio_sample_rate = audio_dec_ctx->sample_rate;
			
	if ((uiSampleRateFilter != SAMPLE_RATE_ANY) && ((GetCodeFromSampleRate(decoded_frame->sample_rate) & uiSampleRateFilter) == 0))
	{
		audio_sample_rate = GetSupportSampleRate(decoded_frame->sample_rate, uiSampleRateFilter);
		if (audio_sample_rate != decoded_frame->sample_rate) 
				ml_sample = (float)audio_sample_rate / decoded_frame->sample_rate;
		//printf("Set sample rate: %i from %i\n", audio_sample_rate, decoded_frame->sample_rate);
	}
				
	unsigned int uiDecBuffLen = 0;
	unsigned int data_size = 0;
	
	/////////SET convert options
	SwrContext *swr = swr_alloc();	
	av_opt_set_int(swr, "in_sample_rate", (int64_t)audio_dec_ctx->sample_rate, 0);
	av_opt_set_int(swr, "out_sample_rate", (int64_t)audio_sample_rate, 0);
	//av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
	av_opt_set_sample_fmt(swr, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	
	if (decoded_frame->channels == 1)		
		av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_MONO, 0);
		else av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
	
	if (audio_channels == 1) 
		av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_MONO, 0);	
		else av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
		
	swr_init(swr);	
	
	AVPacket pkt;
	unsigned int iCnt = 0;
	unsigned int uiFullLen = 0;
	int ret, got_frame;
	
	while(1)
	{
		av_init_packet(&pkt);
		ret = av_read_frame(fmt_ctx, &pkt);
		if (ret == AVERROR(EAGAIN))
		{
			dbgprintf(4,"av_read_frame EAGAIN\n");
			usleep(10000);
			continue;
		}
		
		if (ret < 0) 
		{
			char vvv[256];
			memset(vvv,0,256);
			av_make_error_string(vvv, 256, ret);
			dbgprintf(4,"av_read_frame failed: %i(%s)\n", ret, vvv);
			break;
		}
		
		if (pkt.stream_index != audio_stream_idx) continue;
		
		//printf("%i %i   %i\n", iCnt, uiFullLen, pkt.size);
		
		got_frame = 0;
		pkt.dts = AV_NOPTS_VALUE;
		pkt.pts = AV_NOPTS_VALUE;
								
		ret = avcodec_decode_audio4(audio_dec_ctx, decoded_frame, &got_frame, &pkt);
		if (ret <= 0) 
		{
			dbgprintf(1,"Error while decoding\n");
			break;
		}
		
		data_size = decoded_frame->nb_samples * 2 * audio_channels * ml_sample;
		
		if (uiDecBuffLen < data_size)
		{
			uiDecBuffLen = data_size;
			pDecodedBufferOut = DBG_REALLOC(pDecodedBufferOut, data_size);
		}

		swr_convert(swr, (uint8_t**)&pDecodedBufferOut, decoded_frame->nb_samples * ml_sample, (const uint8_t**)&decoded_frame->extended_data[0], decoded_frame->nb_samples);
		
		
		pSoundBuffer = DBG_REALLOC(pSoundBuffer, uiFullLen + data_size);
		memcpy(pSoundBuffer + uiFullLen, pDecodedBufferOut, data_size);
		uiFullLen += data_size;
				
		//printf("%i %i >>>  %i\n", iCnt, data_size, uiFullLen);		
		iCnt++;
		
		//if (pkt.flags & 1) av_free_packet(&pkt);		
		av_free_packet(&pkt);		
	} 

	if (uiFullLen)
	{
		*uiBufferSize = uiFullLen;
		*pBufferOut = pSoundBuffer;
	}
	
	avcodec_close(video_dec_ctx);
    avcodec_close(audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
	swr_free(&swr);
	if (iTypeStream == 1) avformat_network_deinit();
	if (pDecodedBufferOut) DBG_FREE(pDecodedBufferOut);
	
	DBG_LOG_OUT();
	return 1;
}

void* thread_PlayAudioStream(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "play_aud_stream");
	
	func_link *f_link = (func_link*)pData;
	MODULE_INFO *miModule = f_link->pModule;
	
	tx_eventer_add_event(&pevnt_audio_run, AUDIO_EVENT_STOP);    
    tx_eventer_add_event(&pevnt_audio_run, AUDIO_EVENT_BUSY);
	if (f_link->DeviceNum) 
	{
		tx_eventer_add_event(&pevnt_audio_run, AUDIO_EVENT_PLAY);
		tx_eventer_add_event(&pevnt_audio_run, AUDIO_EVENT_PAUSE);
	}
	DBG_MUTEX_LOCK(&Sound_Mutex);
	cThreadSoundsStatus++;
	DBG_MUTEX_UNLOCK(&Sound_Mutex);
		
	
	DBG_MUTEX_LOCK(&Play_Mutex);	
	iPlaySoundCnt++;
	DBG_MUTEX_UNLOCK(&Play_Mutex);
			
	if (f_link->pSubModule != NULL) gpio_switch_on_module((MODULE_INFO*)f_link->pSubModule);
	
	unsigned int iNumFrame = 0;
	int Flag, iLoop, err, got_frame, ret, res;
	int data_size = 0;
	AudioCodecInfo codec_info;	
	AVCodec *codec;
    AVCodecContext *ctx= NULL;
    int len;
    AVPacket avpkt;
    AVFrame *decoded_frame = NULL;
    av_init_packet(&avpkt);
	avpkt.size = 0;

	memset(&codec_info, 0, sizeof(AudioCodecInfo));
	codec_info.audio_raw_buffer_size = AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
	void *RecvBuffer = DBG_MALLOC(codec_info.audio_raw_buffer_size);
    
	iLoop = 0;
	err = 0;
	
	//int64_t previous_ms = 0;
	//int64_t prev_ms = 0;
	
	char *pBufferOut = NULL;
	char cAlsaInit = 0;
	char cClearedBuff = 0;
	snd_pcm_sframes_t frames;
	SwrContext *swr = swr_alloc();
	char cStopped = 0;
	int iPlayStatus = 1;
	int iFrameDone = 1;
	
	while (1)
	{
		if (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_STOP) == 0)
		{
			cStopped = 1;
			dbgprintf(5,"AUDIO_EVENT_STOP\n");
			break;
		}
		if (f_link->DeviceNum && (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_PLAY) == 0))
		{
			tx_eventer_add_event(&pevnt_audio_run, AUDIO_EVENT_PLAY);
			iPlayStatus = 1;
			//printf("Audio Play\n");
		}
		
		if (f_link->DeviceNum && (tx_eventer_test_event(&pevnt_audio_run, AUDIO_EVENT_PAUSE) == 0))
		{
			tx_eventer_add_event(&pevnt_audio_run, AUDIO_EVENT_PAUSE);
			iPlayStatus = 0;
			//printf("Audio Pause\n");
		}
		
		if (iPlayStatus == 0)
		{
			usleep(30000);
			continue;
		}
		
		//if (cAlsaInit != 0) printf("$$$$$$$$$$$$$$ %i\n", snd_pcm_state(f_link->Handle));
		Flag = 0;
		len = 0;
		iNumFrame = 0;

		avpkt.data = RecvBuffer;
		avpkt.size = codec_info.audio_raw_buffer_size;			
		//printf("!!!!!!!!1 %i %i %i\n", f_link->DeviceNum, iPlayStatus, iFrameDone);
		if ((f_link->DeviceNum == 0) || iPlayStatus)
		{
			//if (e_link2.Type == 2) && port_settings_changed) SendRequestNextFrame(e_link2.ConnectNum, e_link2.ConnectID, &e_link2.Address);
			if (f_link->DeviceNum && iFrameDone && cAlsaInit)
			{
				//printf("Request new audio packet\n");
				iFrameDone = 0;
				err = f_link->FuncRecv(&avpkt, &iNumFrame, &Flag, &codec_info, 0, f_link->ConnectNum, f_link->ConnectID);	
			} else err = f_link->FuncRecv(&avpkt, &iNumFrame, &Flag, &codec_info, 1, f_link->ConnectNum, f_link->ConnectID);			
		}
		else
		{
			err = 1;
			Flag = FLAG_PAUSE_AUDIO;
			avpkt.size = 0;
		}
		//printf("Recv audio packet %i %i %i %i\n", err, avpkt.size, iNumFrame, Flag);
		if (((err) && (avpkt.size == 0) && (iNumFrame == 0) && (Flag != FLAG_PAUSE_AUDIO) && (!(Flag & FLAG_AUDIO_CODEC_INFO)) && (!(Flag & FLAG_DONE_AUDIO))) 
			|| (Flag & (FLAG_STOP_AUDIO | FLAG_DATA_CORRUPT)))
		{
			dbgprintf(5,"Stop audio playing %i\n", Flag);
			err = 0;
			if (cAlsaInit) snd_pcm_drain(f_link->Handle);
			break;
		}
				
		if (Flag & FLAG_DONE_AUDIO_FRAME)
		{
			iFrameDone = 1;
			//dbgprintf(5,"Audio Frame Done\n");
		}
		
		if (Flag & FLAG_DONE_AUDIO) 
		{
			iPlayStatus = 0;
			iFrameDone = 1;
			dbgprintf(5,"Audio Done >> Pause\n");
		}
		
		if (err)
		{			
			if ((cAlsaInit == 0) && (Flag & FLAG_AUDIO_CODEC_INFO))
			{
				dbgprintf(5,"thread_PlayAudioStream Codec audio recv %i\n", codec_info.CodecInfoFilled);
				cAlsaInit = 1;
				if (codec_info.audio_raw_buffer_size > (AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE)) 
						avpkt.data = DBG_REALLOC(avpkt.data, codec_info.audio_raw_buffer_size);
				/* find the mpeg audio decoder */
				av_register_all();
				codec = avcodec_find_decoder(codec_info.audio_codec);
				if (!codec) 
				{
					dbgprintf(1,"Codec not found %i\n", codec_info.audio_codec);
					break;
				}
				ctx = avcodec_alloc_context3(codec);
				if (!ctx) 
				{
					dbgprintf(1,"Could not allocate audio codec context\n");
					break;
				}
				iLoop |= 2;					
				//ctx->sample_fmt = AV_SAMPLE_FMT_S16;
				ctx->bit_rate = codec_info.audio_bit_rate;
				ctx->sample_rate = codec_info.audio_sample_rate;
				ctx->channels = codec_info.audio_channels;
				if (codec_info.audio_channels == 1) ctx->channel_layout = AV_CH_LAYOUT_MONO;
				if (codec_info.audio_channels == 2) ctx->channel_layout = AV_CH_LAYOUT_STEREO;				
				ctx->codec_type = AVMEDIA_TYPE_AUDIO;
				ctx->profile = codec_info.audio_profile;
				ctx->frame_size = codec_info.audio_frame_size;
						
				/* open it */
				if (avcodec_open2(ctx, codec, NULL) < 0) 
				{
					dbgprintf(1,"Could not open codec\n");
					break;
				}
				iLoop |= 4;	
				if (!decoded_frame) 
				{
					decoded_frame = av_frame_alloc();
					if (!decoded_frame) 
					{
						dbgprintf(1,"Could not allocate audio frame\n");
						break;
					} 
					iLoop |= 8;						
					decoded_frame->nb_samples = ctx->frame_size;
					decoded_frame->format = ctx->sample_fmt;
					decoded_frame->channel_layout = ctx->channel_layout;
					decoded_frame->sample_rate = ctx->sample_rate;
					decoded_frame->channels = ctx->channels; 
				}
				
				codec_info.nb_samples = ctx->frame_size;	
				if ((miModule->Settings[6] != SAMPLE_RATE_ANY) && ((GetCodeFromSampleRate(decoded_frame->sample_rate) & miModule->Settings[6]) == 0))
				{
					codec_info.audio_sample_rate = GetSupportSampleRate(decoded_frame->sample_rate, miModule->Settings[6]);
					if (codec_info.audio_sample_rate != decoded_frame->sample_rate) 
						codec_info.nb_samples = codec_info.nb_samples * ((float)codec_info.audio_sample_rate / decoded_frame->sample_rate);
					//printf("Set sample rate: %i from %i(%i %i)\n", codec_info.audio_sample_rate, decoded_frame->sample_rate, codec_info.nb_samples, decoded_frame->nb_samples);
				}
				
				//////////calc size pcm buffer//////////
				if (codec_info.nb_samples)
				{
					if ((miModule->Settings[3] != 1) && (miModule->Settings[3] != 2)) ret = ctx->channels; else ret = miModule->Settings[3];				
					data_size = av_samples_get_buffer_size(NULL, ret /*ctx->channels*/, codec_info.nb_samples, AV_SAMPLE_FMT_S16, 1);
					if (data_size < 0) 
					{
						// This should not occur, checking just for paranoia 
						dbgprintf(1,"Failed to calculate data size error:%i, chan:%i, smpl:%i, frmt:%i\n", data_size, ret, codec_info.nb_samples, AV_SAMPLE_FMT_S16);
						break;
					}
					//////////Alloc pcm buffer//////////
					pBufferOut = DBG_MALLOC(data_size);
					cAlsaInit = 2;				
				}
				iLoop |= 16;	
				//////////INIT PCM CARD//////////
				char dev[32];
				memset(dev,0,32);
				sprintf(dev, "hw:%i,%i", miModule->Settings[1], miModule->Settings[2]);		
				if ((miModule->Settings[3] != 1) && (miModule->Settings[3] != 2)) ret = codec_info.audio_channels; else ret = miModule->Settings[3];
				res = open_audio_play_device(&f_link->Handle, dev, ret /*codec_info.audio_channels*/, codec_info.audio_sample_rate);
				if (res < 0) 
				{
					dbgprintf(1,"error: open_audio_play_device: '%s', open_audio_play_device chanels:%i\n",dev ,ret);
					if (res == -2) iLoop |= 32;
					break;
				}
				iLoop |= (32|64);	
				//printf("open_audio_play_device %i\n", codec_info.audio_sample_rate);
				/////////SET convert options				
				av_opt_set_int(swr, "in_sample_rate", (int64_t)ctx->sample_rate, 0);
				av_opt_set_int(swr, "out_sample_rate", (int64_t)codec_info.audio_sample_rate, 0);
				//av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
				av_opt_set_sample_fmt(swr, "in_sample_fmt", ctx->sample_fmt, 0);
				av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
				if ((miModule->Settings[3] != 1) && (miModule->Settings[3] != 2))
				{
					if (codec_info.audio_channels == 1)
					{
						av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_MONO, 0);
						av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_MONO, 0);
					}
					else
					{
						av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
						av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
					}				
				}
				if (miModule->Settings[3] == 1)
				{
					if (codec_info.audio_channels == 1)
					{
						av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_MONO, 0);
						av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_MONO, 0);
					}
					else
					{
						av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
						av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_MONO, 0);
					}				
				}
				if (miModule->Settings[3] == 2)
				{
					if (codec_info.audio_channels == 1)
					{
						av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_MONO, 0);
						av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
					}
					else
					{
						av_opt_set_int(swr, "in_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
						av_opt_set_int(swr, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
					}				
				}
				swr_init(swr);
			}
				
			if ((cAlsaInit == 2) && (Flag & FLAG_PAUSE_AUDIO))
			{
				if (cClearedBuff == 0)
				{
					memset(pBufferOut, 0, data_size);
					cClearedBuff = 1;
				}
				while(1)
				{
					frames = snd_pcm_writei(f_link->Handle, pBufferOut, codec_info.nb_samples);
					if (frames == -EAGAIN) 
					{
						dbgprintf(5,"fplay 2 EAGAIN\n");
						continue;
					}
					//printf("fplayed %i\n",err);
					if (frames < 0) 
					{
						if (xrun_recovery((snd_pcm_t*)f_link->Handle, frames) < 0) 
						{
							dbgprintf(4,"audio play error 2: %s\n", snd_strerror(frames));
							//break;
						}
						dbgprintf(4,"skip one period 2\n");
						//break; // skip one period
					}
					break;
				}
			}
			
			if ((cAlsaInit != 0) && ((Flag & FLAG_NEXT_AUDIO_FRAME) && (avpkt.size > 0)))
			{				
				if (cClearedBuff == 1) cClearedBuff = 0;
				got_frame = 0;
				avpkt.dts = AV_NOPTS_VALUE;
				avpkt.pts = AV_NOPTS_VALUE;
								
				len = avcodec_decode_audio4(ctx, decoded_frame, &got_frame, &avpkt);
				if (len <= 0) 
				{
					dbgprintf(1,"Error while decoding\n");
					break;
				}
				if (avpkt.flags & 1) av_free_packet(&avpkt);
				iLoop |= 128;				
				if (got_frame) 
				{
					if (cAlsaInit == 1)
					{
						if ((miModule->Settings[3] != 1) && (miModule->Settings[3] != 2)) ret = ctx->channels; else ret = miModule->Settings[3];				
						data_size = av_samples_get_buffer_size(NULL, ret /*ctx->channels*/, decoded_frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
						if (data_size < 0) 
						{
							// This should not occur, checking just for paranoia 
							dbgprintf(1,"Failed to calculate data size error:%i, chan:%i, smpl:%i, frmt:%i\n", data_size, ret, decoded_frame->nb_samples, AV_SAMPLE_FMT_S16);
							break;
						}
						codec_info.nb_samples = decoded_frame->nb_samples;
						//////////Alloc pcm buffer//////////
						pBufferOut = DBG_MALLOC(data_size);
						cAlsaInit = 2;		
						//printf("data_size %i   codec_info.nb_samples %i\n", data_size, codec_info.nb_samples);
					}
					swr_convert(swr, (uint8_t**)&pBufferOut, codec_info.nb_samples, (const uint8_t**)&decoded_frame->extended_data[0], decoded_frame->nb_samples);
					//prev_ms = get_ms(&previous_ms);
					//if (((unsigned int)prev_ms) > 120) dbgprintf(4,"Time snd_pcm_writei:%i\n", (unsigned int)get_ms(&previous_ms));	
					while(1)
					{
						//get_ms(&previous_ms);		
						if (frames == -EAGAIN) 
						{
							dbgprintf(5,"fplay EAGAIN\n");
							continue;
						}
						if (frames < 0) 
						{
							if (xrun_recovery((snd_pcm_t*)f_link->Handle, frames) < 0) 
							{
								dbgprintf(4,"audio play error: %s\n", snd_strerror(frames));
								//break;
							}
							dbgprintf(4,"skip one period\n");
						}
						break;
					}
					frames = snd_pcm_writei(f_link->Handle, pBufferOut, codec_info.nb_samples);
					//previous_ms = 0;						
				}					
			}			
		}	
		else
		{
			add_sys_cmd_in_list(SYSTEM_CMD_AUDIO_ERROR, 0);	
			if (cAlsaInit) snd_pcm_drain(f_link->Handle);
			break;
		}
		//if (iNumFrame > 2000) break;
	}
	dbgprintf(5,"closing audio thread\n");
	
	if (f_link->ConnectID != 0) CloseConnectID(f_link->ConnectID);
	
	/* clean up */
    //audio_stop();
	
	//fclose(file1);	
	//fclose(outfile);
	DBG_FREE(RecvBuffer);
	if (iLoop & 2)
	{
		avcodec_close(ctx);
		av_free(ctx);
	}
    if (iLoop & 8) av_frame_free(&decoded_frame);
	swr_free(&swr);
	if (iLoop & 32) 
	{
		snd_pcm_close(f_link->Handle);
		//printf("close_audio_play_device\n");
	}
	
	if (pBufferOut != NULL) DBG_FREE(pBufferOut);		
	
	if (cStopped == 0) tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_STOP);   
	tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_BUSY);
	if (f_link->DeviceNum) 
	{
		tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_PLAY);
		tx_eventer_send_event(&pevnt_audio_run, AUDIO_EVENT_PAUSE);
	}
	
	if (GetDoneSoundPlay(f_link->pSubModule, miModule->Settings[5])) gpio_switch_off_module((MODULE_INFO*)f_link->pSubModule);
	
    if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);
    DBG_FREE(f_link->pModule);
	DBG_FREE(f_link);	
	
	DBG_MUTEX_LOCK(&Sound_Mutex);
	cThreadSoundsStatus--;	
	DBG_MUTEX_UNLOCK(&Sound_Mutex);
    
	dbgprintf(5,"closed audio thread\n");
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return  (void*)0;
}

void* thread_CaptureAudioStream(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "audio_capture");
	
	func_link *f_link = (func_link*)pData;
	MODULE_INFO *miModule = (MODULE_INFO *)(f_link->pModule);
	
	snd_pcm_t *audio_capture_handle;
	snd_pcm_hw_params_t *alsa_hw_params; 
	AVFrame *av_frame;
	uint16_t *uiInBuffer = NULL;	
	int audio_buffer_size = 0;
	int audio_pts_step_base = 0;
	int ret;
	
	char dev[32];
	memset(dev,0,32);
	sprintf(dev, "hw:%i,%i", miModule->Settings[1], miModule->Settings[2]);	
	if (miModule->Settings[0] & MODULE_SECSET_STEREO) f_link->codec_info.audio_channels = 2;
		else f_link->codec_info.audio_channels = 1;
	
	f_link->codec_info.audio_codec = AV_CODEC_ID_AAC;
	f_link->codec_info.audio_profile = FF_PROFILE_UNKNOWN;
	f_link->codec_info.audio_sample_rate = miModule->Settings[3]; 
	f_link->codec_info.audio_bit_rate = miModule->Settings[4];    			
	
	f_link->codec_info.audio_codec = GetAudioCodecCode(miModule->Settings[16]);
	
	if (f_link->codec_info.audio_codec == AV_CODEC_ID_AAC)
			f_link->codec_info.audio_profile = FF_PROFILE_AAC_LOW;
	if (f_link->codec_info.audio_codec == AV_CODEC_ID_MP2)
	{
		f_link->codec_info.audio_profile = 2;
		if (f_link->codec_info.audio_bit_rate < 6) 
		{
			dbgprintf(2, "Wrong bitrate for MP2 codec, set 128K\n");
			f_link->codec_info.audio_bit_rate = 128000;
		}
	}
		
	ret = open_audio_capture_device(&audio_capture_handle, dev);
    if (ret < 0)
	{
        if (ret == -1) dbgprintf(1,"warning: audio capturing is disabled (%s)\n",dev);
			else dbgprintf(1,"error: init_audio failed: %d\n", ret);
		
		DBG_FREE(f_link->pModule);    
		DBG_FREE(f_link);
		DBG_LOG_OUT();		
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());
		return (void*)-1;
    } 
	//else dbgprintf(1,"audio capturing is opened\n");
	
	preconfig_audio_capture_device(audio_capture_handle, &alsa_hw_params, f_link->codec_info.audio_channels);
	
    AVFormatContext *format_ctx = mpegts_create_context(&f_link->codec_info);
	if (format_ctx == NULL) 
	{
		dbgprintf(1,"error: mpegts_create_context\n");
        DBG_FREE(f_link->pModule);    
		DBG_FREE(f_link);
		DBG_LOG_OUT();		
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return NULL;
	}
	ret = setup_av_frame(format_ctx, &f_link->codec_info, &av_frame, &uiInBuffer, &audio_buffer_size, &f_link->codec_info.audio_frame_size, &audio_pts_step_base);
	if (ret < 0) 
	{
        dbgprintf(1,"error: setup_av_frame: ret=%d\n", ret);
        DBG_FREE(f_link->pModule);    
		DBG_FREE(f_link);
		DBG_LOG_OUT();		
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)(intptr_t)ret;
    }
	
	int period_size = 0;
	int audio_fd_count = 0;
	struct pollfd *poll_fds = NULL; // file descriptors for polling audio
	ret = configure_audio_capture_device(audio_capture_handle, alsa_hw_params, &poll_fds, &audio_fd_count, &period_size,
						f_link->codec_info.audio_sample_rate, f_link->codec_info.audio_channels, audio_buffer_size / 2);
    if (ret != 0) 
	{
        dbgprintf(1,"error: configure_audio_capture_device: ret=%d\n", ret);
        DBG_FREE(f_link->pModule);    
		DBG_FREE(f_link);
		DBG_LOG_OUT();		
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)-2;
    }
	
	tx_eventer_add_event(&pevnt_mcapt_run, CAPTURE_EVENT_STOP);
	tx_eventer_add_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_ON);
	tx_eventer_add_event(&pevnt_mcapt_run, CAPTURE_EVENT_MUTE_OFF);
    
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadAudCaptStatus++;
	DBG_MUTEX_UNLOCK(&system_mutex); 
	
	audio_loop_poll_mmap(&audio_capture_handle, poll_fds, audio_fd_count, period_size, av_frame, format_ctx, f_link, uiInBuffer, audio_buffer_size);
	
	close_audio_capture_device(audio_capture_handle, poll_fds);
	
	DBG_FREE(f_link->pModule);    
	DBG_FREE(f_link);
	
	DBG_MUTEX_LOCK(&system_mutex);
	cThreadAudCaptStatus--;
	DBG_MUTEX_UNLOCK(&system_mutex); 
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)0;
}

void * thread_av_reader(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "av_reader");
	
	misc_buffer *mBuffRead = (misc_buffer*)pData;
	TX_EVENTER 	*pevnt_read = mBuffRead->void_data;
	AVFormatContext *fmt_ctx = mBuffRead->void_data2;
	AVPacket *pkt_read = mBuffRead->void_data3;
	int ret, ret2;
	int64_t previous_ms;
	unsigned int uiEvent;
	
	//tx_eventer_add_event(pevnt_read, MEDIA_EVENT_REPLAY);
	tx_eventer_add_event(pevnt_read, MEDIA_EVENT_STOP);
	tx_eventer_add_event(pevnt_read, MEDIA_EVENT_NEXT);
	//tx_eventer_add_event(pevnt_read, MEDIA_EVENT_BACKWARD);
	//tx_eventer_add_event(pevnt_read, MEDIA_EVENT_FORWARD);
	uiEvent = MEDIA_EVENT_NEXT;
	
	while(1)
	{
		if (uiEvent == MEDIA_EVENT_NEXT)
		{
			previous_ms = 0;
			get_ms(&previous_ms);
			
			av_init_packet(pkt_read);
			ret = av_read_frame(fmt_ctx, pkt_read);
			if (ret == AVERROR(EAGAIN))
			{
				dbgprintf(4,"av_read_frame EAGAIN\n");
				usleep(10000);
				continue;
			}
			uiEvent = 0;			
			if (ret < 0) 
			{
				pkt_read->size = 0;
				uiEvent = MEDIA_EVENT_STOP;
				char vvv[256];
				memset(vvv,0,256);
				av_make_error_string(vvv, 256, ret);
				dbgprintf(4,"av_read_frame failed: %i(%s)\n", ret, vvv);			
			}
			else
			{
				//printf("READED : %i %i\n",(unsigned int)pkt_read->size, pkt_read->stream_index);
				tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_NEXT);
			}
			ret2 = (unsigned int)get_ms(&previous_ms);	
			if (ret2 > 1000) dbgprintf(4, "LONG time frame read %i ms\n",ret2);
			if (ret2 > 4000) dbgprintf(2, "VERY LONG time frame read %i ms\n",ret2);			
		}
		if (uiEvent == MEDIA_EVENT_STOP)
			tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_STOP);
		uiEvent = 0;
		tx_eventer_recv(pevnt_read, &uiEvent, TX_WAIT_FOREVER, 0);
		//if (uiEvent == MEDIA_EVENT_STOP) printf(">>> MEDIA_EVENT_STOP\n");
		//if (uiEvent == MEDIA_EVENT_NEXT) printf("AV MEDIA_EVENT_NEXT\n");
		//if (pkt_read->data)	av_free_packet(pkt_read);
		if (uiEvent == MEDIA_EVENT_STOP) break;		
	}
		
	tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_EXIT);
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)0;
}

void* thread_PlayMediaFile(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "play_media_file");
	
	misc_buffer *mBuff = (misc_buffer*)pData;
	TX_EVENTER 	*pevnt_init = (TX_EVENTER*)mBuff->void_data;
	int video_enable = 0;
	int audio_enable = 0;
	char *cFileName = mBuff->data;
	char overTCP = mBuff->uidata[0];
	int errnum;
	//int64_t previous_ms = 0;
	DBG_MUTEX_LOCK(&system_mutex);
	unsigned int uiIoTimeout = uiMediaIoTimeout * 1000000;
	DBG_MUTEX_UNLOCK(&system_mutex);
	
	//if (SearchStrInDataCaseIgn(cFileName, strlen(cFileName), 0, "RTSP://") == 1) uiIoTimeout = 0;
	//if (SearchStrInDataCaseIgn(cFileName, strlen(cFileName), 0, "RTMP://") == 1) uiIoTimeout = 0;
	
	if (mBuff->flag & 1) video_enable = 1;
	if (mBuff->flag & 2) audio_enable = 1;    
	if ((video_enable == 0) && (audio_enable == 0)) 
	{
		tx_eventer_add_event(&pevnt_init[1], 1);  
		mBuff->dsize = 0;
		tx_eventer_send_event(&pevnt_init[0],2);	
		tx_eventer_recv_event(&pevnt_init[1], 1, TX_WAIT_FOREVER);
		tx_eventer_delete(&pevnt_init[0]);
		tx_eventer_delete(&pevnt_init[1]);
		DBG_FREE(pevnt_init);
		DBG_FREE(mBuff->data);
		DBG_FREE(mBuff);
		DBG_LOG_OUT();
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)0;
	}
	
	tx_eventer_add_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY);
	tx_eventer_add_event(&pevnt_mplay_run, MEDIA_EVENT_STOP);
	
	AVFormatContext *fmt_ctx = NULL;	
	AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;
	//int width, height;
	//enum AVPixelFormat pix_fmt;
	AVStream *video_stream = NULL, *audio_stream = NULL;
	//uint8_t *video_dst_data[4] = {NULL};
	//int      video_dst_linesize[4];
	//int video_dst_bufsize;
	int video_stream_idx = -1, audio_stream_idx = -1;
	AVFrame *frame = NULL;
	AudioCodecInfo codec_info;
	memset(&codec_info, 0, sizeof(AudioCodecInfo));
	//int video_frame_count = 0;
	//int audio_frame_count = 0;
	char cErrbuff[256];
	int iTypeStream = 0;
	if (SearchData("://", 3, cFileName, strlen(cFileName), 0) > 0) iTypeStream = 1;
	
	av_register_all(); /* register all formats and codecs */
	avcodec_register_all();
    if (iTypeStream == 1) avformat_network_init();
	
	AVDictionary *stream_opts = 0;
	if (uiIoTimeout > 0) av_dict_set_int(&stream_opts, "stimeout", uiIoTimeout, 0);
	if (overTCP) av_dict_set(&stream_opts, "rtsp_transport", "tcp", 0);
	
	errnum = avformat_open_input(&fmt_ctx, (char*)cFileName, NULL, &stream_opts);
	if (errnum < 0) /* open input file, and allocate format context */
	{		
		if (iTypeStream == 0) dbgprintf(2,"thread_PlayMediaFile: Could not open source file (0) %s\n", (char*)cFileName);
			else 
			{
				dbgprintf(2,"thread_PlayMediaFile: Could not open source file\n");
				printf("Could not open source file %s\n", (char*)cFileName);
			}
		av_strerror(errnum, cErrbuff, 256);
		dbgprintf(2,"thread_PlayMediaFile: %s\n", cErrbuff);
		if (iTypeStream == 1) avformat_network_deinit();
		tx_eventer_add_event(&pevnt_init[1], 1);  
		mBuff->dsize = 0;
		tx_eventer_send_event(&pevnt_init[0],2);	
		tx_eventer_recv_event(&pevnt_init[1], 1, TX_WAIT_FOREVER);
		tx_eventer_delete(&pevnt_init[0]);
		tx_eventer_delete(&pevnt_init[1]);
		DBG_FREE(pevnt_init);
		DBG_FREE(mBuff->data);
		DBG_FREE(mBuff);
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_STOP); 
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY);	
		DBG_LOG_OUT();
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)-1;
    }	
	errnum = avformat_find_stream_info(fmt_ctx, NULL);
	if (errnum < 0)	/* retrieve stream information */
	{
        av_strerror(errnum, cErrbuff, 256);
		dbgprintf(3,"thread_PlayMediaFile: %s\n", cErrbuff);
		dbgprintf(1,"Could not find stream information\n");
        if (iTypeStream == 1) avformat_network_deinit();
		tx_eventer_add_event(&pevnt_init[1], 1);  
		mBuff->dsize = 0;
		tx_eventer_send_event(&pevnt_init[0],2);	
		tx_eventer_recv_event(&pevnt_init[1], 1, TX_WAIT_FOREVER);
		tx_eventer_delete(&pevnt_init[0]);
		tx_eventer_delete(&pevnt_init[1]);
		DBG_FREE(pevnt_init);
		DBG_FREE(mBuff->data);
		DBG_FREE(mBuff);
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_STOP); 
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY);	
		DBG_LOG_OUT();
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)-1;
    }
	if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) 
	{
        video_stream = fmt_ctx->streams[video_stream_idx];  
		video_dec_ctx = video_stream->codec;
		if (video_dec_ctx->codec_id != AV_CODEC_ID_H264) video_enable = 0;
    } else video_enable = 0;
	
	if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) 
	{
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dec_ctx = audio_stream->codec; 
    } else audio_enable = 0;
	//av_dump_format(fmt_ctx, 0, cFileName, 0);  /* dump input information to stderr */
	
	//if (!audio_stream) audio_enable = 0;
	//if (!video_stream) video_enable = 0;
	
    if (((!audio_stream) || (audio_enable == 0)) && (!video_stream || (video_enable == 0))) 
	{
        dbgprintf(1,"Could not find audio and video stream or no output devices, aborting\n");
		dbgprintf(1,"File '%s'\n", cFileName);
		
        avcodec_close(video_dec_ctx);
		avcodec_close(audio_dec_ctx);
		avformat_close_input(&fmt_ctx);   
		if (iTypeStream == 1) avformat_network_deinit();
        tx_eventer_add_event(&pevnt_init[1], 1);  
		mBuff->dsize = 0;
		tx_eventer_send_event(&pevnt_init[0],2);	
		tx_eventer_recv_event(&pevnt_init[1], 1, TX_WAIT_FOREVER);
		tx_eventer_delete(&pevnt_init[0]);
		tx_eventer_delete(&pevnt_init[1]);
		DBG_FREE(pevnt_init);
		DBG_FREE(mBuff->data);
		DBG_FREE(mBuff);
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_STOP); 
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY); 	
		DBG_LOG_OUT();
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)-1;
    }
	
	frame = av_frame_alloc();
    if (!frame) 
	{
        dbgprintf(1,"Could not allocate frame\n");
        avcodec_close(video_dec_ctx);
		avcodec_close(audio_dec_ctx);
		avformat_close_input(&fmt_ctx); 
		if (iTypeStream == 1) avformat_network_deinit();
        tx_eventer_add_event(&pevnt_init[1], 1);  
		mBuff->dsize = 0;
		tx_eventer_send_event(&pevnt_init[0],2);	
		tx_eventer_recv_event(&pevnt_init[1], 1, TX_WAIT_FOREVER);
		tx_eventer_delete(&pevnt_init[0]);
		tx_eventer_delete(&pevnt_init[1]);
		DBG_FREE(pevnt_init);
		DBG_FREE(mBuff->data);
		DBG_FREE(mBuff);
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_STOP); 
		tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY); 	
		DBG_LOG_OUT();
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)-1;
    }	
	tx_eventer_add_event(&pevnt_init[1], 1);  
	mBuff->dsize = 0;
	if ((video_stream) && (video_enable)) mBuff->dsize |= FILE_TYPE_VIDEO;
	if ((audio_stream) && (audio_enable)) mBuff->dsize |= FILE_TYPE_AUDIO;
	tx_eventer_send_event(&pevnt_init[0],2);	
	tx_eventer_recv_event(&pevnt_init[1], 1, TX_WAIT_FOREVER);
	tx_eventer_delete(&pevnt_init[0]);
	tx_eventer_delete(&pevnt_init[1]);
	DBG_FREE(pevnt_init);
		
	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
    
	char *cBuff = NULL;
	int iBuffSize = 0;
	int ret;
	int iNeedFilter = 1;
	int iTimer = 10;
	int iDoneFile = 0;
	misc_buffer pkt_info;
			
	//const AVCodecDescriptor* codecDescriptor = avcodec_descriptor_get(video_dec_ctx->codec_id);
	//printf("video <%s>\n",codecDescriptor->name);
	//codecDescriptor = avcodec_descriptor_get(audio_dec_ctx->codec_id);
	//printf("audio <%s>\n",codecDescriptor->name);
	//FILE *dst_file = fopen("/mnt/FLASH/test4.h264", "wb");
	if (audio_stream)
	{
		if (audio_dec_ctx->frame_size == 0) 
		{
			dbgprintf(2, "ffmpeg frame size is zero\n");
		}
		codec_info.CodecInfoFilled 			= 1;
		codec_info.audio_sample_rate 		= audio_dec_ctx->sample_rate;
		codec_info.audio_bit_rate			= audio_dec_ctx->bit_rate;
		if (audio_dec_ctx->channel_layout == AV_CH_LAYOUT_MONO)	codec_info.audio_channels = 1;
		if (audio_dec_ctx->channel_layout == AV_CH_LAYOUT_STEREO) codec_info.audio_channels = 2;
		codec_info.audio_profile 			= audio_dec_ctx->profile; 
		codec_info.audio_codec				= audio_dec_ctx->codec_id;
		codec_info.audio_frame_size			= audio_dec_ctx->frame_size; 
		codec_info.audio_raw_buffer_size	= audio_dec_ctx->frame_size * 2 * codec_info.audio_channels;
	}
	pkt_info.clock = 0;	
	pkt_info.flag = 0;	

	array_buffer abVideoData;
	memset(&abVideoData, 0, sizeof(array_buffer));
	abVideoData.DataMaxCnt = MAX_PLAY_VIDEO_BLOCKS;
	
	if (video_enable)
	{
		abVideoData.AVBuffer = (AVPacket*)DBG_MALLOC(sizeof(AVPacket) * abVideoData.DataMaxCnt); 
		abVideoData.DataSize = (unsigned int*)DBG_MALLOC(sizeof(unsigned int) * abVideoData.DataMaxCnt); 
		memset(abVideoData.AVBuffer, 0, sizeof(AVPacket) * abVideoData.DataMaxCnt);
		memset(abVideoData.DataSize, 0, sizeof(unsigned int) * abVideoData.DataMaxCnt);
	}
	
	array_buffer abAudioData;
	memset(&abAudioData, 0, sizeof(array_buffer));
	abAudioData.DataMaxCnt = MAX_PLAY_AUDIO_BLOCKS;
	
	if (audio_enable)
	{
		abAudioData.AVBuffer = (AVPacket*)DBG_MALLOC(sizeof(AVPacket) * abAudioData.DataMaxCnt); 
		abAudioData.DataSize = (unsigned int*)DBG_MALLOC(sizeof(unsigned int) * abAudioData.DataMaxCnt); 
		memset(abAudioData.AVBuffer, 0, sizeof(AVPacket) * abAudioData.DataMaxCnt);
		memset(abAudioData.DataSize, 0, sizeof(unsigned int) * abAudioData.DataMaxCnt);
	}
	
	AVPacket pkt;
	pkt.data = NULL;
    pkt.size = 0;
	
	int iPreloadCnt, retprev, iPreloadMax, iPreloadWait; 
	unsigned int uiEvent = 0, uiEventPrev = 0;
	if (iTypeStream == 0) iPreloadMax = 50; else iPreloadMax = 20;
	if (!video_stream) iPreloadMax *= 10;
	iPreloadCnt = 0;
	iPreloadWait = 0;
	
	TX_EVENTER pevntSend;
	pthread_t threadFileIO;
	pthread_attr_t tattrFileIO;
	misc_buffer *mBuffRead = (misc_buffer*)DBG_MALLOC(sizeof(misc_buffer));
	mBuffRead->void_data = &pevntSend;
	mBuffRead->void_data2 = fmt_ctx;
	mBuffRead->void_data3 = &pkt;
	tx_eventer_create(&pevntSend, 1);
	
	pthread_attr_init(&tattrFileIO);   
	pthread_attr_setdetachstate(&tattrFileIO, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadFileIO, &tattrFileIO, thread_av_reader, (void*)mBuffRead);	

	DBG_MUTEX_LOCK(&Play_Mutex);
	uiAudioDataCnt = abAudioData.FullDataSize;
	uiVideoDataCnt = abVideoData.FullDataSize;
	uiNewMediaPos = 0;
	DBG_MUTEX_UNLOCK(&Play_Mutex);				
	
	unsigned int uiCurrentPos = 0;
	char pause_status = 0;
	unsigned int uiTwoFrameTime = 0;
	unsigned int uiNumVideoFrames = 0;
	float fAudioTimeBase = 1.0f;
	float fVideoTimeBase = 1.0f;
	char cTimerStop = 0;
	
	if (video_stream) fVideoTimeBase = (float)(fmt_ctx->streams[video_stream_idx]->time_base.num) * AV_TIME_BASE / (unsigned int)(fmt_ctx->streams[video_stream_idx]->time_base.den);
	if (audio_stream) fAudioTimeBase = (float)(fmt_ctx->streams[audio_stream_idx]->time_base.num) * AV_TIME_BASE / (unsigned int)(fmt_ctx->streams[audio_stream_idx]->time_base.den);
	
	unsigned int uiMediaLen = (unsigned int)(fmt_ctx->duration / 1000);
	//if (iTypeStream == 1) 
	/*if (audio_stream) dbgprintf(4, "A FullTimeLen:%i num: %i den: %i\n", 
		uiMediaLen, 
		fmt_ctx->streams[audio_stream_idx]->time_base.num, 
		fmt_ctx->streams[audio_stream_idx]->time_base.den);
	if (video_stream) dbgprintf(4, "V FullTimeLen:%i num: %i den: %i\n", 
		uiMediaLen, 
		fmt_ctx->streams[video_stream_idx]->time_base.num, 
		fmt_ctx->streams[video_stream_idx]->time_base.den);*/
	if (!audio_enable) audio_stream = NULL;
	if (!video_enable) video_stream = NULL;
	
	while ((iTimer) && (video_stream || audio_stream))/* read frames from the file */
	{	
		uiEventPrev = uiEvent;
		uiEvent = 0;
		ret = tx_eventer_recv(&pevntRMS, &uiEvent, 1000, 0);
		//if ((ret != 0) && (uiEvent > 8)) 
		//printf("evnt %i %i %i\n", ret, uiEvent, uiEventPrev);
		uiEvent |= uiEventPrev;	
		if (ret != 0)
		{
			if ((uiEvent & MEDIA_EVENT_PAUSE) && (!(uiEvent & (MEDIA_EVENT_CODEC | MEDIA_EVENT_AUDIO))))
			{
				dbgprintf(5, "MEDIA_EVENT_PAUSE %i\n", pause_status);
				if (pause_status == 0) pause_status = 1;	
				uiEvent ^= MEDIA_EVENT_PAUSE;
			}
			if (uiEvent & MEDIA_EVENT_PLAY)
			{
				dbgprintf(5, "MEDIA_EVENT_PLAY %i\n", pause_status);
				if (pause_status == 1) 
				{
					pause_status = 0;
					iPreloadWait = 0;
				}
				uiEvent ^= MEDIA_EVENT_PLAY;
			}	
			if (((uiEvent & MEDIA_EVENT_FORWARD) 
					|| (uiEvent & MEDIA_EVENT_BACKWARD) 
					|| (uiEvent & MEDIA_EVENT_SET_NEW_POS)
					|| (uiEvent & MEDIA_EVENT_SET_PAUSE_POS)) 
				&& (!(uiEvent & (MEDIA_EVENT_CODEC | MEDIA_EVENT_AUDIO))) 
				&& ((uiEvent & MEDIA_EVENT_NEXT) || (iDoneFile))
				&& ((audio_enable && audio_stream) || (!audio_enable))
				&& ((video_enable && video_stream) || (!video_enable)))
			{		
				ret = 1;
				int seek_flags = 0;
				unsigned int seek_target = 0;
				unsigned int uiCurPts;
				if (audio_enable) 
				{
					if (abAudioData.DataCnt == 0) uiCurPts = 0;
						else uiCurPts = (unsigned int)abAudioData.AVBuffer[abAudioData.PlayBlockNum].pts * fAudioTimeBase; 
				}
				else 
				{
					if (abVideoData.DataCnt == 0) uiCurPts = 0;
						else uiCurPts = (unsigned int)abVideoData.AVBuffer[abVideoData.PlayBlockNum].pts * fVideoTimeBase;
				}
				if (uiEvent & MEDIA_EVENT_BACKWARD)
				{
					DBG_MUTEX_LOCK(&Play_Mutex);
					seek_target = uiCurPts - (uiMediaStepPos * AV_TIME_BASE); 
					DBG_MUTEX_UNLOCK(&Play_Mutex);									
				}
				if (uiEvent & MEDIA_EVENT_FORWARD) 
				{
					DBG_MUTEX_LOCK(&Play_Mutex);
					seek_target = uiCurPts + (uiMediaStepPos * AV_TIME_BASE); 
					DBG_MUTEX_UNLOCK(&Play_Mutex);
				}
				if (uiEvent & MEDIA_EVENT_SET_NEW_POS) 
				{
					DBG_MUTEX_LOCK(&Play_Mutex);
					seek_target = uiMediaLen * uiMediaNewPos; 
					DBG_MUTEX_UNLOCK(&Play_Mutex);			
				}				
				if (seek_target <= 0) 
				{
					if (video_enable) 
					{
						seek_target = uiTwoFrameTime * fVideoTimeBase;
						if (seek_target == 0) ret = 0;
					} else seek_target = 0;
				}
				if (seek_target > fmt_ctx->duration) 
				{
					seek_target = uiCurPts; 				
					ret = 0;
				}
				if (uiCurPts > seek_target) seek_flags = AVSEEK_FLAG_BACKWARD; else seek_flags = 0;
				//if (!video_enable) seek_flags |= AVSEEK_FLAG_FRAME;
				//seek_target = fmt_ctx->duration / AV_TIME_BASE;
				if (ret)
				{
					dbgprintf(5, "SET_NEW_POS %i %i %i\n", seek_target, uiMediaLen, uiMediaNewPos);
					pause_status = 0;
					iPreloadWait = 0;
					//iPreloadWait = 1;
					//uiEvent |= MEDIA_EVENT_PAUSE;
					
					ret = av_seek_frame(fmt_ctx, -1, seek_target, seek_flags);
					if (ret < 0) 
						dbgprintf(2, "thread_av_reader: error while seeking\n");
					
					abVideoData.FullDataSize = 0;
					abVideoData.DataCnt = 0;
					abVideoData.PlayBlockNum = 0;
					abVideoData.LoadBlockNum = 0;
					if (video_enable)
					{
						for (ret = 0; ret < abVideoData.DataMaxCnt; ret++)
							if (abVideoData.DataSize[ret] != 0) 
							{
								if (iNeedFilter) free(abVideoData.AVBuffer[ret].data); else av_free_packet(&abVideoData.AVBuffer[ret]);
								abVideoData.DataSize[ret] = 0;
							}
					}
					
					abAudioData.FullDataSize = 0;
					abAudioData.DataCnt = 0;
					abAudioData.PlayBlockNum = 0;
					abAudioData.LoadBlockNum = 0;
					if (audio_enable)
					{
						for (ret = 0; ret < abAudioData.DataMaxCnt; ret++)
							if (abAudioData.DataSize[ret] != 0) 
							{
								av_free_packet(&abAudioData.AVBuffer[ret]);
								abAudioData.DataSize[ret] = 0;
							}
					}
					
					//abAudioData[0].Flag = 1;
					pkt_info.clock = 2;					
					iDoneFile = 0;
					if (uiEvent & MEDIA_EVENT_NEXT) 
					{
						av_free_packet(&pkt);
						uiEvent ^= MEDIA_EVENT_NEXT;
					}
					tx_eventer_send_event(&pevntSend, MEDIA_EVENT_NEXT);
				}
				
				if (uiEvent & MEDIA_EVENT_FORWARD) uiEvent ^= MEDIA_EVENT_FORWARD;
				if (uiEvent & MEDIA_EVENT_BACKWARD) uiEvent ^= MEDIA_EVENT_BACKWARD;
				if (uiEvent & MEDIA_EVENT_SET_NEW_POS) uiEvent ^= MEDIA_EVENT_SET_NEW_POS;
				if (uiEvent & MEDIA_EVENT_SET_PAUSE_POS) uiEvent ^= MEDIA_EVENT_SET_PAUSE_POS;
			}
			if (uiEvent & MEDIA_EVENT_STOP)			
			{
				dbgprintf(4,"Done file\n");
				iDoneFile = 1;
				iPreloadCnt = iPreloadMax;
				if (iPreloadWait == 1)
				{
					uiEvent |= MEDIA_EVENT_PLAY;
					iPreloadWait = 0;					
				}
				if (((abAudioData.DataCnt == 0) && (audio_enable)) || 
					((abVideoData.DataCnt == 0) && (video_enable))) uiEvent |= MEDIA_EVENT_EX_STOP;
				uiEvent ^= MEDIA_EVENT_STOP;
			}
			if (uiEvent & MEDIA_EVENT_EX_STOP)			
			{
				dbgprintf(4,"Play stop %i\n", uiEvent);
				cTimerStop = 1;
				if (audio_enable)
				{
					for (ret = 0; ret < abAudioData.DataMaxCnt; ret++)
							if (abAudioData.DataSize[ret] != 0) 
							{
								av_free_packet(&abAudioData.AVBuffer[ret]);	
								abAudioData.DataSize[ret] = 0;
							}
					abAudioData.DataCnt = 1;
					abAudioData.DataSize[0] = 0;
					abAudioData.AVBuffer[0].pts = 0;
					abAudioData.FullDataSize = 0;
					abAudioData.PlayBlockNum = 0;
					abAudioData.LoadBlockNum = 0;
					pkt_info.flag = 1;
				}
				
				if (video_enable)
				{
					for (ret = 0; ret < abVideoData.DataMaxCnt; ret++)
							if (abVideoData.DataSize[ret] != 0) 
							{
								if (iNeedFilter) free(abVideoData.AVBuffer[ret].data);
									else av_free_packet(&abVideoData.AVBuffer[ret]);
								abVideoData.DataSize[ret] = 0;
							}
					abVideoData.DataCnt = 1;
					abVideoData.DataSize[0] = 0;
					abVideoData.AVBuffer[0].pts = 0;
					abVideoData.FullDataSize = 0;
					abVideoData.PlayBlockNum = 0;
					abVideoData.LoadBlockNum = 0;							
				}
				
				pkt_info.clock = 2;
				iDoneFile = 1;
				iPreloadCnt = iPreloadMax;
				if (iPreloadWait == 1)
				{
					uiEvent |= MEDIA_EVENT_PLAY;
					iPreloadWait = 0;
				}
				if (uiEvent & MEDIA_EVENT_NEXT) 
				{
					av_free_packet(&pkt);
					uiEvent ^= MEDIA_EVENT_NEXT;
				}
				uiEvent ^= MEDIA_EVENT_EX_STOP;				
			}
			if (uiEvent & MEDIA_EVENT_VIDEO)			
			{	
				if ((video_enable) && (abVideoData.DataCnt > 0))
				{					
					//printf(">>>> %i\n",(unsigned int)get_ms(&previous_ms));			
					//printf("play video from num %i %i %i %i\n", abVideoData.PlayBlockNum, abVideoData.DataSize[abVideoData.PlayBlockNum], pause_status ,pkt_info.clock);
					pkt_info.void_data = &abVideoData.AVBuffer[abVideoData.PlayBlockNum];
					pkt_info.data_size = abVideoData.DataSize[abVideoData.PlayBlockNum];					
					pkt_info.pts = av_rescale_q(abVideoData.AVBuffer[abVideoData.PlayBlockNum].pts, fmt_ctx->streams[video_stream_idx]->time_base, AV_TIME_BASE_Q);
					if (iTypeStream) pkt_info.pts = 0;
					pkt_info.uidata[0] = 0;
					if (pause_status) pkt_info.uidata[0] = 2;
					if (abVideoData.DataSize[abVideoData.PlayBlockNum] == 0) pkt_info.uidata[0] = 1;
					retprev = pkt_info.flag;
					//printf("SEND VIDEO %i %i %i %i\n", abVideoData.PlayBlockNum, pkt_info.data_size, (unsigned int)pkt_info.pts, pkt_info.uidata[0]);
					ret = tx_eventer_send_data(&pevntRMV, EVENT_NEXT_VIDEO_FRAME_DATA, &pkt_info, sizeof(misc_buffer), 1, 200);
					if (ret != 0)
					{	
						iTimer = 10;			
						if (abVideoData.DataSize[abVideoData.PlayBlockNum] == 0) video_stream = NULL;
						if ((pause_status == 0) && (video_stream))
						{		
							if (pkt_info.clock == 0) pkt_info.clock++;									
							abVideoData.FullDataSize -= abVideoData.DataSize[abVideoData.PlayBlockNum];
							abVideoData.DataSize[abVideoData.PlayBlockNum] = 0;
							if (!audio_enable) uiCurrentPos = (unsigned int)abVideoData.AVBuffer[abVideoData.PlayBlockNum].pts * fVideoTimeBase / uiMediaLen;
							abVideoData.PlayBlockNum++;
							abVideoData.DataCnt--;
							if (abVideoData.PlayBlockNum >= abVideoData.DataMaxCnt) abVideoData.PlayBlockNum = 0;
							if (abVideoData.DataCnt == 0) 
							{
								if (iDoneFile == 0) 
									dbgprintf(4, "Slow speed load data for play (video)\n");
								else
								{
									abVideoData.DataCnt = 1;
									abVideoData.DataSize[0] = 0;
									abVideoData.AVBuffer[0].pts = 0;
									abVideoData.FullDataSize = 0;					
								}												
							}
							if (iPreloadCnt) iPreloadCnt--;	
						}
						if ((retprev != pkt_info.flag) && (!audio_enable)) tx_eventer_add_event(&pevnt_mplay_run, MEDIA_EVENT_STOP);
						//printf(">>> MEDIA_EVENT_VIDEO\n");
					} else dbgprintf(2, "timeout MEDIA_EVENT_VIDEO %i\n", abVideoData.DataCnt);	
					uiEvent ^= MEDIA_EVENT_VIDEO;
				} 
				else
				{
					//printf("play %i %i %i\n",	iDoneFile, video_enable, abVideoData.DataCnt);		
					if (iDoneFile) video_stream = NULL;					
				}
			}
			if ((audio_enable) && (uiEvent & MEDIA_EVENT_CODEC))
			{
				ret = tx_eventer_send_data(&pevntRMA, EVENT_AUDIO_CODEC_INFO_DATA, &codec_info, sizeof(AudioCodecInfo), 1, 200);
				if (ret != 0)
				{	
					tx_eventer_add_event(&pevnt_mplay_run, MEDIA_EVENT_STOP); 
				} else dbgprintf(2, "timeout MEDIA_EVENT_CODEC\n");	
				uiEvent ^= MEDIA_EVENT_CODEC;
			}
			if ((audio_enable) && (uiEvent & MEDIA_EVENT_AUDIO))
			{	
				if ((abAudioData.DataCnt > 0) && ((pkt_info.flag == 1) || (!video_stream)))
				{
					//printf("play audio from num %i %i\n", abAudioData.PlayBlockNum, abAudioData.DataSize[abAudioData.PlayBlockNum]);												
					pkt_info.void_data = &abAudioData.AVBuffer[abAudioData.PlayBlockNum];
					pkt_info.data_size = abAudioData.DataSize[abAudioData.PlayBlockNum];
					pkt_info.uidata[0] = 0;
					if (pause_status) pkt_info.uidata[0] = 2;
					if (abAudioData.DataSize[abAudioData.PlayBlockNum] == 0) pkt_info.uidata[0] = 1;						
					
					ret = tx_eventer_send_data(&pevntRMA, EVENT_NEXT_AUDIO_FRAME_DATA, &pkt_info, sizeof(misc_buffer), 1, 200);
					if (ret != 0)
					{
						iTimer = 10;			
						if (abAudioData.DataSize[abAudioData.PlayBlockNum] == 0) audio_stream = NULL;
						if ((pause_status == 0) && (audio_stream))
						{		
							abAudioData.FullDataSize -= abAudioData.DataSize[abAudioData.PlayBlockNum];
							abAudioData.DataSize[abAudioData.PlayBlockNum] = 0;
							uiCurrentPos = (unsigned int)abAudioData.AVBuffer[abAudioData.PlayBlockNum].pts * fAudioTimeBase / uiMediaLen;
							abAudioData.PlayBlockNum++;
							abAudioData.DataCnt--;
							if (abAudioData.PlayBlockNum >= abAudioData.DataMaxCnt) abAudioData.PlayBlockNum = 0;
							if (abAudioData.DataCnt == 0) 
							{
								if (iDoneFile == 0) 
									dbgprintf(4, "Slow speed load data for play (audio)\n");
									else
									{
										abAudioData.DataCnt = 1;
										abAudioData.DataSize[0] = 0;
										abAudioData.AVBuffer[0].pts = 0;
										abAudioData.FullDataSize = 0;					
									}												
							}
							if (iPreloadCnt) iPreloadCnt--;															
						}			
					} else dbgprintf(2, "timeout MEDIA_EVENT_AUDIO %i\n", abAudioData.DataCnt);	
					uiEvent ^= MEDIA_EVENT_AUDIO;
					// else dbgprintf(3, "no data MEDIA_EVENT_AUDIO %i\n", abAudioData.DataCnt); 
				}
			}
			if (uiEvent & MEDIA_EVENT_NEXT)			
			{				
				//if ((iDoneFile == 0) && (((abVideoData.DataCnt == 0) && (video_stream)) || ((abAudioData.DataCnt == 0) && (audio_stream))))				
				if ((iDoneFile == 0) && (iPreloadCnt == 0)) 
				{
					iPreloadWait = 1;
					uiEvent |= MEDIA_EVENT_PAUSE;
					//pause_status = 1;
				}
				iPreloadCnt++;
				if ((iPreloadWait == 1) && (iPreloadCnt >= iPreloadMax)) 
				{
					iPreloadWait = 0;
					uiEvent |= MEDIA_EVENT_PLAY;
				}
				if (iDoneFile == 0)
				{	
					ret = 0;
					if (pkt.stream_index == video_stream_idx)						
					{
						if (!video_enable)
						{
							av_free_packet(&pkt);
							ret = 1;
						}
						else
						{
							uiNumVideoFrames++;
							if (uiNumVideoFrames == 2) uiTwoFrameTime = (unsigned int)pkt.pts;
							if ((abVideoData.DataSize[abVideoData.LoadBlockNum] == 0) && (abVideoData.LoadBlockNum < abVideoData.DataMaxCnt))								
							{	
								if (iNeedFilter)
								{
									if (av_bitstream_filter_filter(h264bsfc, video_dec_ctx, NULL, (uint8_t**)&cBuff, &iBuffSize, pkt.data, pkt.size, 0) < 0)
									{
										//printf("pts V: %i %i\n", (unsigned int)pkt.dts, (unsigned int)pkt.pts);
										dbgprintf(4,"error av_bitstream_filter_filter video_dec_ctx\n");
										iNeedFilter = 0;										
									}			
								}
								memcpy(&abVideoData.AVBuffer[abVideoData.LoadBlockNum], &pkt, sizeof(AVPacket));
								if (iNeedFilter)
								{
									//if (cBuff == pkt.data) printf("????? %i %i %i\n", abVideoData.LoadBlockNum, iBuffSize, pkt.size);
									av_free_packet(&pkt);
									abVideoData.AVBuffer[abVideoData.LoadBlockNum].data = (uint8_t*)cBuff;
									abVideoData.AVBuffer[abVideoData.LoadBlockNum].size = iBuffSize;
									abVideoData.AVBuffer[abVideoData.LoadBlockNum].flags = 2;
								} else abVideoData.AVBuffer[abVideoData.LoadBlockNum].flags = 1;
								
								abVideoData.DataSize[abVideoData.LoadBlockNum] = abVideoData.AVBuffer[abVideoData.LoadBlockNum].size;
								abVideoData.FullDataSize += abVideoData.AVBuffer[abVideoData.LoadBlockNum].size;
								abVideoData.DataCnt++;
								ret = 1;
								//printf("#### Loaded video in %i %i\n", abVideoData.LoadBlockNum, abVideoData.DataSize[abVideoData.LoadBlockNum]);
								abVideoData.LoadBlockNum++;
								if (abVideoData.LoadBlockNum >= abVideoData.DataMaxCnt) abVideoData.LoadBlockNum = 0;											
							}
						}
					}
					if (pkt.stream_index == audio_stream_idx)
					{	
						if (!audio_enable)
						{
							av_free_packet(&pkt);
							ret = 1;
						}
						else
						{
							if ((abAudioData.DataSize[abAudioData.LoadBlockNum] == 0) && (abAudioData.LoadBlockNum < abAudioData.DataMaxCnt))								
							{						
								memcpy(&abAudioData.AVBuffer[abAudioData.LoadBlockNum], &pkt, sizeof(AVPacket));
								abAudioData.AVBuffer[abAudioData.LoadBlockNum].flags = 1;
								abAudioData.DataSize[abAudioData.LoadBlockNum] = abAudioData.AVBuffer[abAudioData.LoadBlockNum].size;
								abAudioData.FullDataSize += abAudioData.AVBuffer[abAudioData.LoadBlockNum].size;
								abAudioData.DataCnt++;
								ret = 1;
								//printf("#### Loaded audio in %i %i\n", abAudioData.LoadBlockNum, abAudioData.DataSize[abAudioData.LoadBlockNum]);
								abAudioData.LoadBlockNum++;
								if (abAudioData.LoadBlockNum >= abAudioData.DataMaxCnt) abAudioData.LoadBlockNum = 0;											
							}
						}
					}
					if (ret)
					{						
						tx_eventer_send_event(&pevntSend, MEDIA_EVENT_NEXT);
						uiEvent ^= MEDIA_EVENT_NEXT;
					}
				}			
			}
			//if (uiEvent) printf("NOT WORKED %i, iPreloadCnt: %i %i %i\n", uiEvent, iPreloadCnt, abVideoData.DataCnt, abAudioData.DataCnt);
			//iTimer = 10;
			DBG_MUTEX_LOCK(&Play_Mutex);
			uiAudioDataCnt = abAudioData.FullDataSize;
			uiVideoDataCnt = abVideoData.FullDataSize;
			uiNewMediaPos = uiCurrentPos;
			DBG_MUTEX_UNLOCK(&Play_Mutex);	
			//printf("Cnt: %i, %i Time: %i %i %i\n", abAudioData.FullDataSize, abVideoData.FullDataSize, uiCurrentPos, (unsigned int)FullTimeLen, FullTimeLen * uiCurrentPos / 1000);
		}
		else 
		{			
			iTimer--;
			if ((iTimer == 0) && (cTimerStop == 0))
			{
				iTimer = 10;
				cTimerStop = 1;
				uiEvent |= MEDIA_EVENT_EX_STOP;
			}
		}
    }
	if (iTimer == 0) dbgprintf(2,"timeout PlayMediaFile a:%i, v:%i\n", (intptr_t)audio_stream, (intptr_t)video_stream);
		//else dbgprintf(5,"PlayMediaFile succeeded done.\n");
	DBG_MUTEX_LOCK(&Play_Mutex);
	uiAudioDataCnt = 0;
	uiVideoDataCnt = 0;
	uiNewMediaPos = -1;
	DBG_MUTEX_UNLOCK(&Play_Mutex);	
	dbgprintf(5,"PlayMediaFile closing read thread.\n");
	tx_eventer_send_event(&pevntSend, MEDIA_EVENT_STOP);
	tx_eventer_recv_event(&pevntRMS, MEDIA_EVENT_EXIT, TX_WAIT_FOREVER);
	pthread_join(threadFileIO, NULL);
	pthread_attr_destroy(&tattrFileIO);
	tx_eventer_delete(&pevntSend);
	DBG_FREE(mBuffRead);
				
	avcodec_close(video_dec_ctx);
    avcodec_close(audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
	av_bitstream_filter_close(h264bsfc);
    //avfree(video_dst_data[0]);
	if (iTypeStream == 1) avformat_network_deinit();
	
	//fclose(dst_file);
	DBG_FREE(mBuff->data);
	DBG_FREE(mBuff);
	if (video_enable)
	{
		DBG_FREE(abVideoData.AVBuffer);
		DBG_FREE(abVideoData.DataSize);
	}
	if (audio_enable)
	{
		DBG_FREE(abAudioData.AVBuffer);
		DBG_FREE(abAudioData.DataSize);
	}
	
	tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_STOP); 
	tx_eventer_send_event(&pevnt_mplay_run, MEDIA_EVENT_BUSY_PLAY); 	
	 
	dbgprintf(5,"PlayMediaFile done.\n");
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)1;
}

void* thread_SaveFLVFile(void* pvData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "file_saver");
	
	misc_buffer *mBuff = (misc_buffer*)pvData;
	/*if (mBuff->clock >= MAX_STREAM_INFO)
	{
		dbgprintf(1,"Error ID stream %i\n",mBuff->clock);
		DBG_LOG_OUT();	
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)-1;
	}*/
	STREAM_INFO *pStream = (STREAM_INFO*)mBuff->void_data3;
	
	char cAudioCodecNum = mBuff->uidata[0];
	unsigned char ucOrderLmt = mBuff->uidata[1];
	unsigned int uiFlvBuffSize = MIN_FLV_BUFFER_SIZE + (mBuff->uidata[2] * MIN_FLV_BUFFER_SIZE);
	
	char* cAddrOrderer = (char*)mBuff->void_data2;
	char* cPrePath = (char*)mBuff->void_data4;
	
	unsigned int uiWaitCopyTimeBreak = mBuff->uidata[3];
	unsigned int uiWaitCopyTimeMess = mBuff->uidata[4];
	unsigned int uiBackUpType = mBuff->uidata[5];
	
	char *cFileEndName = "flv";	
	if (mBuff->flag & 2)
	{
		switch(cAudioCodecNum)
		{
			case 0: cFileEndName = "flv"; break;
			case 1: cFileEndName = "flv"; break;
			case 2: cFileEndName = "flv"; break;
			case 3: cFileEndName = "flv"; break;
			case 4: cFileEndName = "flv"; break;
			case 5: cFileEndName = "flv"; break;
			case 6: cFileEndName = "mp2"; break;
			case 7: cFileEndName = "wma"; break;
			case 8: cFileEndName = "wma"; break;
			case 9: cFileEndName = "wma"; break;
			case 10: cFileEndName = "wma"; break;
			case 11: cFileEndName = "wma"; break;
			case 12: cFileEndName = "mp1"; break;
			case 13: cFileEndName = "mpegts"; break;
			case 14: cFileEndName = "ac3"; break;
			case 15: cFileEndName = "dts"; break;
			case 16: cFileEndName = "ogg"; break;
			case 17: cFileEndName = "wavpack"; break;
			case 18: cFileEndName = "ac3"; break;
			default: cFileEndName = "flv"; break;
		}	
	}
	
	DBG_MUTEX_LOCK(&pStream->mutex);
	mBuff->uidata[0] = pStream->VidID;
	if (mBuff->uidata[0] == 0) mBuff->uidata[0] = pStream->AudID;
	unsigned int uiCaptType = pStream->Type;	
	mBuff->clock = pStream->Num;
	DBG_MUTEX_UNLOCK(&pStream->mutex);
			
	tx_eventer_add_event(&pStream->run, MEDIA_EVENT_STOP_REC);    
    tx_eventer_add_event(&pStream->run, MEDIA_EVENT_BUSY_REC);
	tx_eventer_add_event(&pStream->run, MEDIA_EVENT_NEXT_REC);
	tx_eventer_add_event(&pStream->run, MEDIA_EVENT_NEXT_REC + CMD_SAVE_COPY_OFFSET);
	
	OMX_BUFFERHEADERTYPE omx_buff;
	OMX_BUFFERHEADERTYPE *p_omx_buff;
	OMX_BUFFERHEADERTYPE omx_videobuff;
	omx_videobuff.pBuffer = DBG_MALLOC(1000000);
	omx_videobuff.nFilledLen = 0;
	int ret;
	int have_video = 0 , have_audio = 0;
	int iDiff = 0;
	int iSelfEncoder = 0;
	char cFileName[256];
	char cFileNameError[256];	
	char cFileType[32];
	memset(cFileName, 0, 256);
	memset(cFileNameError, 0, 256);
	memset(cFileType, 0, 32);
	char cBackUpFileName[256];
	memset(cBackUpFileName, 0, 256);
	if (uiCaptType == CAPTURE_TYPE_FULL) strcpy(cFileType, DIR_NORM);
	if (uiCaptType == CAPTURE_TYPE_SLOW) strcpy(cFileType, DIR_SLOW);
	if (uiCaptType == CAPTURE_TYPE_DIFF) strcpy(cFileType, DIR_DIFF);
	if (uiCaptType == CAPTURE_TYPE_AUDIO) strcpy(cFileType, DIR_AUD);
	
	time_t rawtime;
	struct tm timeinfo;
	
	if (mBuff->flag & 1) have_video = 1;
	if (mBuff->flag & 2) have_audio = 1;
	if (mBuff->flag & 4) iDiff = 1;
	if (mBuff->flag & 8) iSelfEncoder = 1;
    int64_t MaxDuration = mBuff->dsize * 1000;
	
	void *pData;
	misc_buffer *pRecvBuff;
	unsigned int uiDataSize;
		
	//FILE *outfile = fopen("/mnt/FLASH/videostream.h264", "wb");
	//FILE *outfile2 = fopen("/mnt/FLASH/audiostream.aac", "wb");
	FILE *flvfile = NULL;
	int iLevelLoadVideo = 0;
	int iLevelLoadAudio = 0;
	int iSpsPpsWrited = 0;
	int iFileSize = 0;
	int iCnt;
	//int iIdle;
	unsigned int uiEvent = 0;
	int64_t audio_pts = 0, video_pts = 0;
	AudioCodecInfo tAudioInfo;
	VideoCodecInfo tVideoInfo;
	memset(&tAudioInfo, 0, sizeof(AudioCodecInfo));
	memset(&tVideoInfo, 0, sizeof(VideoCodecInfo));
	if (have_video)	iLevelLoadVideo = 1;
	if (have_audio)	iLevelLoadAudio = 1;
	char *SpsPpsBuffer = NULL;
	int SpsPpsLen = 0;
	char cDateChanged = 0;
	
	char cFileWriteError = 0;
	char cFileWritePrevError = 0;
	int64_t iFileWriteErrorTimer = 0;
	get_ms(&iFileWriteErrorTimer);
	
	time_t tLastDate;
	time(&tLastDate);
	
	if (have_video) tx_eventer_add_event(&pevnt_mrec_run, CAPTURE_EVENT_VIDEO);
	if (have_audio && !have_video) tx_eventer_add_event(&pevnt_mrec_run, CAPTURE_EVENT_AUDIO);	
	
	//int64_t previous_ms = 0;
	//get_ms(&previous_ms);
	flv_fast_struct* ffs;
	if (uiCaptType != CAPTURE_TYPE_SLOW) ffs = flv_fast_writer_open(uiFlvBuffSize, 65536);
		else ffs = flv_fast_writer_open(uiFlvBuffSize >> 1, 65536);
	
	while (1) 
	{
		uiEvent = 0;
		if ((iLevelLoadAudio == 4) || (iLevelLoadVideo == 5) || cFileWriteError)
			ret = tx_eventer_recv(&pStream->pevntS, &uiEvent, 10000, 0);
			else ret = 0;
				
		//printf("file_saver %i %i  %i\n", iLevelLoadAudio, iLevelLoadVideo, ret);
		
		//if ((iLevelLoadAudio == 4) || (iLevelLoadVideo == 5)) printf("EVENT %i\n", uiEvent);
		//if (ret != 0)
		{
			//iIdle = 1;
			if (cFileWriteError && (((iLevelLoadVideo == 1) || (iLevelLoadVideo == 0)) && ((iLevelLoadAudio == 1) || (iLevelLoadAudio == 0))))
			{
				if (get_ms(&iFileWriteErrorTimer) > 30000)	//30sec.
				{
					dbgprintf(3,"Repeat create new file after %i sec\n", get_ms(&iFileWriteErrorTimer) / 1000);
					cFileWriteError = 0;
					cFileWritePrevError = 1;					
				}
			}
			
			if ((cFileWriteError == 0) && (((iLevelLoadVideo == 1) || (iLevelLoadVideo == 0)) && ((iLevelLoadAudio == 1) || (iLevelLoadAudio == 0))))
			{
				//printf("GET open file\n");			
				if ((iLevelLoadVideo == 0) && (iLevelLoadAudio == 0)) 
				{
					dbgprintf(1,"NO AV STREAMS %i\n", uiCaptType);
					break;
				}
				/*DBG_MUTEX_LOCK(&system_mutex);
				if (mBuff->clock == 0) memcpy(cMediaFileName, cFileName, 256);
					else memcpy(cMediaFileNameSlow, cFileName, 256);
				DBG_MUTEX_UNLOCK(&system_mutex);*/
				
				iFileSize = 0;
				// open the output file, if needed 
				time(&rawtime);
				localtime_r(&rawtime, &timeinfo);
				
				memset(cFileNameError, 0, 256);					
				memset(cFileName, 0, 256);			
						
				if (mBuff->frsize != 2)
				{
					sprintf(cFileName, "%s%s%04i/%s/%02i/%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.%s",
						mBuff->data, cPrePath,
						timeinfo.tm_year+1900,GetMonthName(timeinfo.tm_mon+1),timeinfo.tm_mday,
						timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(char*)mBuff->uidata, cFileType, cFileEndName);					
				}
				else
				{
					sprintf(cFileName, "%s%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.%s",
						mBuff->data,
						timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(char*)mBuff->uidata, cFileType, cFileEndName);
					sprintf(cFileNameError, "%s%s%04i/%s/%02i/%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.%s",
						mBuff->data, cPrePath,
						timeinfo.tm_year+1900,GetMonthName(timeinfo.tm_mon+1),timeinfo.tm_mday,
						timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(char*)mBuff->uidata, cFileType, cFileEndName);
				}
				CreateDirectoryFromPath(cFileName, 1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
							
				memset(cBackUpFileName, 0, 256);
				sprintf(cBackUpFileName, "%s%s%04i/%s/%02i/%04i_%02i_%02i__%02i_%02i_%02i_%.4s_%s.%s",
						(char*)mBuff->void_data,
						cPrePath,
						timeinfo.tm_year+1900,GetMonthName(timeinfo.tm_mon+1),timeinfo.tm_mday,
						timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,
						(char*)mBuff->uidata, cFileType, cFileEndName);
								
				if (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_NEXT_REC) == 0)
						tx_eventer_add_event(&pStream->run, MEDIA_EVENT_NEXT_REC);	
				if (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_NEXT_REC + CMD_SAVE_COPY_OFFSET) == 0)
						tx_eventer_add_event(&pStream->run, MEDIA_EVENT_NEXT_REC + CMD_SAVE_COPY_OFFSET);	
				
				DBG_MUTEX_LOCK(&pStream->mutex);
				memset(pStream->CurrentPath, 0, MAX_PATH);
				strcpy(pStream->CurrentPath, cFileName);
				DBG_MUTEX_UNLOCK(&pStream->mutex);
							
				//printf("%s\n",cFileName);			
				ret = flv_file_open(&flvfile, cFileName, ffs);
				if (ret == 0) 
				{
					if (!cFileWritePrevError) 
						dbgprintf(1,"thread_SaveFLVFile: Could not create '%s' (%i)\n", mBuff->data, errno);
					dbgprintf(3,"Error create file '%s'\n", cFileName);
					dbgprintf(3,"Error info (%i) %s\n", errno, strerror(errno));
					cFileWriteError = 2;
					iFileWriteErrorTimer = 0;
					get_ms(&iFileWriteErrorTimer);
				} 
				else 
				{
					dbgprintf(5,"Create file '%s'\n", cFileName);
					
					if (!flv_write_header(flvfile, have_audio, have_video, ffs, cAudioCodecNum))
						cFileWriteError = 1;
					else
					{
						if (cFileWritePrevError) dbgprintf(2,"Success restore to create file\n");
						cFileWritePrevError = 0;
						iSpsPpsWrited = 0;
						omx_videobuff.nFilledLen = 0;
						if (iLevelLoadVideo) iLevelLoadVideo++;	else iLevelLoadAudio++;	
					}
					//iIdle = 0;
				}
			}
			if (iLevelLoadVideo == 2)
			{
				//printf("GET EVENT_VIDEO_INFO_DATA %i\n", mBuff->clock);
				if (tVideoInfo.Filled == 0)
				{
					tx_eventer_recv_data(&pStream->pevntV, EVENT_VIDEO_INFO_DATA, &pData, &uiDataSize, TX_WAIT_FOREVER);
					if (uiDataSize != 0)
					{
						if (uiDataSize == sizeof(VideoCodecInfo)) 
						{
							//printf("saving EVENT_VIDEO_INFO_DATA\n");
							memcpy(&tVideoInfo, pData, sizeof(VideoCodecInfo));					
							tVideoInfo.Filled = 1;
						} else dbgprintf(1,"Error size VIDEO_INFO_DATA\n");
						if (!tx_eventer_return_data(&pStream->pevntV, RECV_FRAME_TIMEOUT_MS))
							dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
					}
				}  
				if (tVideoInfo.Filled == 1)
				{
					iLevelLoadVideo++; 
					if (iLevelLoadAudio) iLevelLoadAudio++;	else iLevelLoadVideo++; 
					tVideoInfo.FrameNum = 0;				
				}
				//iIdle = 0;
			}
			
			if (iLevelLoadAudio == 2)
			{
				if (tAudioInfo.CodecInfoFilled == 0)
				{
					//printf("GET EVENT_AUDIO_CODEC_INFO_DATA\n");			
					tx_eventer_recv_data(&pStream->pevntA, EVENT_AUDIO_CODEC_INFO_DATA, &pData, &uiDataSize, TX_WAIT_FOREVER);
					if (uiDataSize != 0)
					{
						if (uiDataSize == sizeof(AudioCodecInfo)) 
						{
							//printf("saving EVENT_AUDIO_CODEC_INFO_DATA\n");
							AudioCodecInfo *ci = (AudioCodecInfo*)pData;
							memcpy(&tAudioInfo, pData, sizeof(AudioCodecInfo));
							tAudioInfo.empty_frame = (char*)DBG_MALLOC(ci->size_empty_frame);
							memcpy(tAudioInfo.empty_frame, ci->empty_frame, ci->size_empty_frame);
							tAudioInfo.CodecInfoFilled = 1;				
						} else dbgprintf(1,"Error size AUDIO_CODEC_INFO_DATA\n");			
						if (!tx_eventer_return_data(&pStream->pevntA, RECV_FRAME_TIMEOUT_MS))
							dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
					}
				}
				if (tAudioInfo.CodecInfoFilled == 1)
				{
					tAudioInfo.FrameNum = 0;	
					iLevelLoadAudio++;		
					if (iLevelLoadVideo) iLevelLoadVideo++;	
					else 
					{
						iLevelLoadAudio++;	
						tx_eventer_add_event(&pStream->pevntA, EVENT_NEED_AUDIO_DATA);
					}
				}
				//iIdle = 0;
			}
			
			if (iLevelLoadVideo == 4)
			{
				//printf("GET EVENT_START_VIDEO_FRAME_DATA %i\n", mBuff->clock);
				ret = tx_eventer_recv_data(&pStream->pevntV, EVENT_START_VIDEO_FRAME_DATA, &pData, &uiDataSize, TX_WAIT_FOREVER);
				if (uiDataSize != 0)
				{
					if (uiDataSize == sizeof(omx_start_packet)) 
					{
						omx_start_packet *Start_Packet = (omx_start_packet*)pData;
						if (Start_Packet->CodecInfoLen == 0) dbgprintf(1,"error CodecInfoLen == 0\n");
						if (SpsPpsLen == 0)
						{
							SpsPpsBuffer = (char*)DBG_MALLOC(Start_Packet->CodecInfoLen);
							memcpy(SpsPpsBuffer, Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen);
							SpsPpsLen = Start_Packet->CodecInfoLen;						
							iFileSize += Start_Packet->CodecInfoLen;									
						}					
						omx_videobuff.nFilledLen = 0;
						ret = 0;
						iCnt = 0;
						if (iSpsPpsWrited == 0)
						{
							audio_pts = 0;
							video_pts = 0;
							if (!flv_write_avc_spspps(flvfile, (uint8_t*)SpsPpsBuffer, SpsPpsLen, 0, ffs))
								cFileWriteError = 1;
							//printf("spspps\n");
							iSpsPpsWrited = 1;
							//printf("flv_write_avc_spspps\n");
						}
						while ((tVideoInfo.video_intra_frame != ret) && (Start_Packet->StartFramesFlags[ret] == 2))
						{
							omx_buff.pBuffer = (uint8_t*)&Start_Packet->BufferStartFrame[iCnt];
							omx_buff.nFilledLen = Start_Packet->StartFramesSizes[ret];	
							iCnt += Start_Packet->StartFramesSizes[ret];
								
							if (iSpsPpsWrited == 1)
							{
								if (!flv_write_avc_first_frame(flvfile, (uint8_t*)SpsPpsBuffer, SpsPpsLen, (uint8_t*)omx_buff.pBuffer, omx_buff.nFilledLen, (uint32_t)(video_pts), ffs))
									cFileWriteError = 1;
								//printf("first_frame\n");
								iSpsPpsWrited = 2;
								//printf("flv_write_avc_first_frame\n");
							}
							else 
							{
								if (Start_Packet->StartFramesFlags[ret] == 2)
								{
									//printf("flv_write_avc_frame\n");
									//if (ret == 0) printf("S;(%i)", tVideoInfo.FrameNum);						
									if (ret == 0) 
									{
										if (!flv_write_avc_frame(flvfile, (uint8_t*)omx_buff.pBuffer, omx_buff.nFilledLen, (uint32_t)(video_pts), 1, ffs))
											cFileWriteError = 1;
									}
									else 
									{	
										if (!flv_write_avc_frame(flvfile, (uint8_t*)omx_buff.pBuffer, omx_buff.nFilledLen, (uint32_t)(video_pts), 0, ffs))
											cFileWriteError = 1;
									}
								}
								else
								{
									memcpy(&omx_videobuff.pBuffer[omx_videobuff.nFilledLen], omx_buff.pBuffer, omx_buff.nFilledLen);
									omx_videobuff.nFilledLen += omx_buff.nFilledLen;
								}						
							}
							iFileSize += omx_buff.nFilledLen;
							if (Start_Packet->StartFramesFlags[ret] == 2)
							{
								tVideoInfo.FrameNum++;
								video_pts += 1000 / tVideoInfo.video_frame_rate;	
							}
							ret++;
							//printf("video frame0 %i, pts: %i\n",tVideoInfo.FrameNum, (uint32_t)video_pts);
							//if (iLevelLoadAudio && (audio_pts < video_pts)) printf("audio frame0 empty %i, ptsa: %i, ptsv: %i\n",tAudioInfo.FrameNum, (uint32_t)audio_pts, (uint32_t)video_pts);								
							while (iLevelLoadAudio && (audio_pts < video_pts))
							{
								//printf("audio frame0 empty:%i size:%i, ptsv: %i, ptsa: %i\n",tAudioInfo.FrameNum, tAudioInfo.size_empty_frame, (uint32_t)video_pts, (uint32_t)audio_pts);
								if (!flv_write_audio_frame(flvfile, &tAudioInfo, (uint8_t*)tAudioInfo.empty_frame, tAudioInfo.size_empty_frame, (uint32_t)(audio_pts), ffs))
									cFileWriteError = 1;
								tAudioInfo.FrameNum++;
								iFileSize += tAudioInfo.size_empty_frame;	
								audio_pts += 1000 / tAudioInfo.audio_frame_rate;							
							}
							//printf("%i, %i\n", ret, tVideoInfo.FrameNum);
							if (ret >= tVideoInfo.video_intra_frame) break;
							if ((uiCaptType == CAPTURE_TYPE_SLOW) && (iSelfEncoder == 0)) break;
						} 
						//printf("ret : %i %i %i %i\n", ret, tVideoInfo.video_intra_frame, Start_Packet->StartFramesFlags[ret], Start_Packet->StartFramesCount);
						if (iLevelLoadAudio) 
						{
							if (iLevelLoadAudio < 4) iLevelLoadAudio++;
							tx_eventer_add_event(&pStream->pevntA, EVENT_NEED_AUDIO_DATA);
						}
						iLevelLoadVideo++;
						tx_eventer_add_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA);
					} else dbgprintf(1,"Error size start packet Need: %i, Recved: %i (%i)\n", sizeof(omx_start_packet), uiDataSize, ret);
					if (!tx_eventer_return_data(&pStream->pevntV, RECV_FRAME_TIMEOUT_MS))
						dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
				}
				//iIdle = 0;
			}
			
			if ((uiEvent == EVENT_VIDEO) && (iLevelLoadVideo == 5)) //&& ((!iLevelLoadAudio) || (audio_pts >= video_pts))
			{
				//printf("GET EVENT_NEXT_VIDEO_FRAME_DATA %i\n", mBuff->clock);
				if (tx_eventer_recv_data(&pStream->pevntV, EVENT_NEXT_VIDEO_FRAME_DATA, (void*)&pRecvBuff, &uiDataSize, 100))
				{	
					if (uiDataSize == sizeof(misc_buffer))
					{
						//if (iLevelLoadAudio && (audio_pts < video_pts)) printf("audio frame0 empty %i, ptsa: %i, ptsv: %i\n",tAudioInfo.FrameNum, (uint32_t)audio_pts, (uint32_t)video_pts);							
						while (iLevelLoadAudio && (audio_pts < video_pts))
						{
							//printf("audio frame0 empty:%i size:%i, ptsv: %i, ptsa: %i\n",tAudioInfo.FrameNum, tAudioInfo.size_empty_frame, (uint32_t)video_pts, (uint32_t)audio_pts);
							if (!flv_write_audio_frame(flvfile, &tAudioInfo, (uint8_t*)tAudioInfo.empty_frame, tAudioInfo.size_empty_frame, (uint32_t)(audio_pts), ffs))
								cFileWriteError = 1;
							tAudioInfo.FrameNum++;
							iFileSize += tAudioInfo.size_empty_frame;	
							audio_pts += 1000 / tAudioInfo.audio_frame_rate;							
						}
						if (pRecvBuff->data_size != 0) 
						{
							p_omx_buff = (OMX_BUFFERHEADERTYPE*)pRecvBuff->void_data;
							if ((iDiff) && 
								(p_omx_buff->nFlags & OMX_BUFFERFLAG_SYNCFRAME) && 
								(pRecvBuff->flag == 0)) 
								{
									if (iLevelLoadAudio) iLevelLoadVideo = 6; else iLevelLoadVideo = 4; 
									tx_eventer_delete_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA);
								}
							if (iLevelLoadVideo == 5)
							{
								//printf("video frame %i ptsv:%i, ptsa:%i\n",tVideoInfo.FrameNum, (uint32_t)(video_pts), (uint32_t)audio_pts);
								if (p_omx_buff->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
								{
									if (omx_videobuff.nFilledLen == 0) 
									{
										if (p_omx_buff->nFlags & OMX_BUFFERFLAG_SYNCFRAME) 
										{
											if (!flv_write_avc_frame(flvfile, (uint8_t*)p_omx_buff->pBuffer, p_omx_buff->nFilledLen, (uint32_t)(video_pts), 1, ffs))
												cFileWriteError = 1;
										}	
										else 
										{
											if (!flv_write_avc_frame(flvfile, (uint8_t*)p_omx_buff->pBuffer, p_omx_buff->nFilledLen, (uint32_t)(video_pts), 0, ffs))
												cFileWriteError = 1;
										}
										iFileSize += p_omx_buff->nFilledLen;
									}
									else
									{
										memcpy(&omx_videobuff.pBuffer[omx_videobuff.nFilledLen], p_omx_buff->pBuffer, p_omx_buff->nFilledLen);
										omx_videobuff.nFilledLen += p_omx_buff->nFilledLen;
										if (p_omx_buff->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
										{
											if (!flv_write_avc_frame(flvfile, (uint8_t*)omx_videobuff.pBuffer, omx_videobuff.nFilledLen, (uint32_t)(video_pts), 1, ffs))
												cFileWriteError = 1;
										}
										else 
										{
											if (!flv_write_avc_frame(flvfile, (uint8_t*)omx_videobuff.pBuffer, omx_videobuff.nFilledLen, (uint32_t)(video_pts), 0, ffs))
												cFileWriteError = 1;
										}
										iFileSize += omx_videobuff.nFilledLen;		
									}				
									//printf("video frame1\n");
							
									tVideoInfo.FrameNum++;						
									omx_videobuff.nFilledLen = 0;
									video_pts += 1000 / tVideoInfo.video_frame_rate;
									//iIdle = 0;
								}
								else
								{
									memcpy(&omx_videobuff.pBuffer[omx_videobuff.nFilledLen], p_omx_buff->pBuffer, p_omx_buff->nFilledLen);
									omx_videobuff.nFilledLen += p_omx_buff->nFilledLen;
								}
							}
						} else dbgprintf(2, "EMPTY next video packet\n");	
					} else dbgprintf(1,"Error size next video packet\n");
					if (!tx_eventer_return_data(&pStream->pevntV, RECV_FRAME_TIMEOUT_MS))
						dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
				} else dbgprintf(2, "TIMEOUT next video packet\n");			
			}
			if ((uiEvent == EVENT_AUDIO) && (iLevelLoadAudio == 4)) //&& ((!iLevelLoadVideo) || (audio_pts < video_pts)))		
			{				
				/*printf("GET EVENT_NEXT_AUDIO_FRAME_DATA\n");
				if (tx_eventer_test_event(&pStream->pevntA, EVENT_SKIP_AUDIO_FRAME_DATA) != 0) 
				{
					if (iLevelLoadVideo)
					{
						//printf("audio frame0 %i, pts: %i\n",tAudioInfo.FrameNum, (uint32_t)audio_pts);												
						flv_write_audio_frame(flvfile, &tAudioInfo, (uint8_t*)tAudioInfo.empty_frame, tAudioInfo.size_empty_frame, (uint32_t)(audio_pts), ffs);
						tAudioInfo.FrameNum++;
						iFileSize += tAudioInfo.size_empty_frame;	
						audio_pts += 1000 / tAudioInfo.audio_frame_rate;	
					}	
				}
				else*/
				//if (!iDiff || (iDiff && (audio_pts < video_pts)))
				
				{
					{
						if (tx_eventer_recv_data(&pStream->pevntA, EVENT_NEXT_AUDIO_FRAME_DATA, &pData, &uiDataSize, 100))
						{	
							if ((iDiff) && (iLevelLoadVideo == 6)) 
							{
								iLevelLoadVideo = 4;
								tx_eventer_delete_event(&pStream->pevntA, EVENT_NEED_AUDIO_DATA);
							}
							else
							{
								if (uiDataSize != 0)
								{
									//printf("audio frame1 %i, ptsv: %i, ptsa: %i\n",tAudioInfo.FrameNum, (uint32_t)video_pts, (uint32_t)audio_pts);												
									if (!flv_write_audio_frame(flvfile, &tAudioInfo, (uint8_t*)pData, uiDataSize, (uint32_t)(audio_pts), ffs))
										cFileWriteError = 1;
									iFileSize += uiDataSize;
									tAudioInfo.FrameNum++;
									audio_pts += 1000 / tAudioInfo.audio_frame_rate;	
									//iIdle = 0;										
								} else dbgprintf(1,"Error size next audio packet\n");			
							}								
							if (!tx_eventer_return_data(&pStream->pevntA, RECV_FRAME_TIMEOUT_MS))
								dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
						} else dbgprintf(2, "TIMEOUT next audio packet\n");				
					}
				}	
			}
			
			if (!cDateChanged) cDateChanged = TestDataChanged(&tLastDate);
			if ((omx_videobuff.nFilledLen == 0) &&
				(((iFileSize >= mBuff->data_size) && mBuff->data_size) 
				|| ((MaxDuration <= video_pts) && (iLevelLoadVideo) && MaxDuration) 
				|| ((MaxDuration <= audio_pts) && (iLevelLoadAudio) && MaxDuration)
				|| (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_STOP_REC) == 0)
				|| (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_NEXT_REC) == 0)
				|| (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_NEXT_REC + CMD_SAVE_COPY_OFFSET) == 0)
				|| cDateChanged
				|| (cFileWriteError == 1)))
			{
				if (cFileWriteError)
				{
					dbgprintf(2, "Close file after error:'%s'\n", cFileName);
					iFileWriteErrorTimer = 0;
					get_ms(&iFileWriteErrorTimer);
					cFileWriteError = 2;
				}
				
				if (cDateChanged) 
				{
					dbgprintf(3, "DateTime changed, file split:'%s'\n", cFileName);
					cDateChanged = 0;
				}
				iFileSize = 0;						
				if (have_video) iLevelLoadVideo = 1;
				if (have_audio) iLevelLoadAudio = 1;
				audio_pts = 0;
				video_pts = 0;
				tAudioInfo.FrameNum = 0;
				tVideoInfo.FrameNum = 0;
					
				tx_eventer_delete_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA);	
				tx_eventer_delete_event(&pStream->pevntA, EVENT_NEED_AUDIO_DATA);	
					
				if (tx_eventer_recv_data(&pStream->pevntA, EVENT_NEXT_AUDIO_FRAME_DATA, NULL, NULL, 0))
				{
					tx_eventer_clear_event(&pStream->pevntS, EVENT_AUDIO);
					if (!tx_eventer_return_data(&pStream->pevntA, RECV_FRAME_TIMEOUT_MS))
						dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
					dbgprintf(4,"Cancel recv audio data\n");
				}
						
				if (tx_eventer_recv_data(&pStream->pevntV, EVENT_NEXT_VIDEO_FRAME_DATA, NULL, NULL, 0))
				{
					tx_eventer_clear_event(&pStream->pevntS, EVENT_VIDEO);
					if (!tx_eventer_return_data(&pStream->pevntV, RECV_FRAME_TIMEOUT_MS))
						dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
					dbgprintf(4,"Cancel recv video data\n");
				}
					
				flv_file_close(flvfile, ffs);
				flvfile = NULL;
				dbgprintf(5,"Close file '%s'\n", cFileName);
				
				ret = 1;
				if ((mBuff->frsize != 0) || (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_NEXT_REC + CMD_SAVE_COPY_OFFSET) == 0))	
				{
					dbgprintf(4, "%s %s to %s\n",(mBuff->frsize == 2) ? "Remove" : "Copy", cFileName, cBackUpFileName);
					ret = CopyFile(cFileName, cBackUpFileName, cFileNameError, 0, (mBuff->frsize == 2) ? 1 : 0, pStream, ucOrderLmt, cAddrOrderer,
									uiWaitCopyTimeBreak, uiWaitCopyTimeMess, uiBackUpType);
				}
				if (tx_eventer_test_event(&pStream->run, MEDIA_EVENT_STOP_REC) == 0) break;	
					
				DBG_MUTEX_LOCK(&pStream->mutex);
				memset(pStream->CurrentPath, 0, MAX_PATH);
				DBG_MUTEX_UNLOCK(&pStream->mutex);
			}
			//if (iIdle) usleep(5000);
		}
    }
	flv_fast_writer_close(ffs);
	
	tx_eventer_delete_event(&pStream->pevntV, EVENT_NEED_VIDEO_DATA);
	tx_eventer_delete_event(&pStream->pevntA, EVENT_NEED_AUDIO_DATA);
	tx_eventer_delete_event(&pStream->run, MEDIA_EVENT_STOP_REC);    
    tx_eventer_delete_event(&pStream->run, MEDIA_EVENT_BUSY_REC);
	tx_eventer_delete_event(&pStream->run, MEDIA_EVENT_NEXT_REC);
	tx_eventer_delete_event(&pStream->run, MEDIA_EVENT_NEXT_REC + CMD_SAVE_COPY_OFFSET);
	
	DBG_FREE(mBuff->data);
	DBG_FREE(mBuff->void_data);
	if (mBuff->void_data2) DBG_FREE(mBuff->void_data2);
	DBG_FREE(mBuff->void_data4);
	
	DBG_FREE(mBuff);
	DBG_FREE(omx_videobuff.pBuffer);
	if (tAudioInfo.empty_frame) DBG_FREE(tAudioInfo.empty_frame);
	if (SpsPpsBuffer) DBG_FREE(SpsPpsBuffer);
	
	DBG_MUTEX_LOCK(&pStream->mutex);
	pStream->VidID = 0;
	pStream->AudID = 0;
	pStream->Type = 0;	
	DBG_MUTEX_UNLOCK(&pStream->mutex);
	
	dbgprintf(3, "Done save type:%i num:%i\n", uiCaptType, (intptr_t)pStream);
		
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)1;
}

void* thread_PlayAudioSound(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "play_sound");
	
	func_link *f_link = (func_link*)pData;
	MODULE_INFO *miModule = f_link->pModule;
	
	/*if (tx_eventer_count_event(&pevnt_audio_run, AUDIO_EVENT_BUSY_SOUND) > (miModule->Settings[4] - 1))
	{
		//if (tx_eventer_recv_event(&pevnt_audio_run, AUDIO_EVENT_BUSY_SOUND, 5000) == 0)
		{
			dbgprintf(4,"thread_PlayAudioSound, busy\n");
			if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);
			DBG_FREE(f_link->pModule);
			DBG_FREE(f_link);
			return 0;
		}
	}*/
	DBG_MUTEX_LOCK(&Sound_Mutex);
	cThreadSoundsStatus++;
	DBG_MUTEX_UNLOCK(&Sound_Mutex);
	
	if (f_link->pSubModule != NULL) gpio_switch_on_module((MODULE_INFO*)f_link->pSubModule);
	
	int err;
    int iLen, iWriteLen;
	unsigned char *pBuffer = f_link->mbuffer.void_data2;
	unsigned int iBufferSize = 0;
	unsigned int iCnannels = 2;
	unsigned int iSampleRate = 44100;
	unsigned int iSuppSampleRate = 44100;
	unsigned int iPeriodSize = 1024;
	float fPeriodDiff = 1;
	unsigned int iSrcPeriodSize = 1024;
	int iRepeats = f_link->ConnectNum;
	
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;
    char dev[32];
	memset(dev,0,32);
	sprintf(dev, "hw:%i,%i", miModule->Settings[1], miModule->Settings[2]);		
	if (miModule->Settings[3] == 1) iCnannels = 1;
	if ((miModule->Settings[6] != SAMPLE_RATE_ANY) && ((GetCodeFromSampleRate(iSampleRate) & miModule->Settings[6]) == 0))
		iSuppSampleRate = GetSupportSampleRate(iSampleRate, miModule->Settings[6]);			
	if ((err = snd_pcm_open(&handle, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
	{
        if (err == -16) 
			dbgprintf(3,"Playback open error2: %i(snd_pcm_open): %s\n", err, snd_strerror(err));
			else
			dbgprintf(1,"Playback open error2: %i(snd_pcm_open): %s\n", err, snd_strerror(err));
		
		if (GetDoneSoundPlay(f_link->pSubModule, miModule->Settings[5])) gpio_switch_off_module((MODULE_INFO*)f_link->pSubModule);
		
		DBG_MUTEX_LOCK(&Sound_Mutex);
		cThreadSoundsStatus--;
		DBG_MUTEX_UNLOCK(&Sound_Mutex);
		
        DBG_FREE(f_link->mbuffer.void_data2);
		if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);
		DBG_FREE(f_link->pModule);
		DBG_FREE(f_link);
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return 0;
    }
    if ((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                                      iCnannels,
                                      iSuppSampleRate,
                                      1,
                                      500000)) < 0) 
	{   /* 0.5sec */
        dbgprintf(1,"Playback open error2(snd_pcm_set_params): %s\n", snd_strerror(err));
		
		if (GetDoneSoundPlay(f_link->pSubModule, miModule->Settings[5])) gpio_switch_off_module((MODULE_INFO*)f_link->pSubModule);
		
		DBG_MUTEX_LOCK(&Sound_Mutex);
		cThreadSoundsStatus--;
		DBG_MUTEX_UNLOCK(&Sound_Mutex);
		
        DBG_FREE(f_link->mbuffer.void_data2);
		if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);
		DBG_FREE(f_link->pModule);
		DBG_FREE(f_link);
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return 0;
    }
	
	err = snd_pcm_get_params (handle, (snd_pcm_uframes_t*)&iBufferSize, (snd_pcm_uframes_t*)&iPeriodSize);
    if (err < 0) 
	{
        dbgprintf(1,"Unable to get period size for playback: %s\n", snd_strerror(err));
		if (GetDoneSoundPlay(f_link->pSubModule, miModule->Settings[5])) gpio_switch_off_module((MODULE_INFO*)f_link->pSubModule);

		DBG_MUTEX_LOCK(&Sound_Mutex);
		cThreadSoundsStatus--;
		DBG_MUTEX_UNLOCK(&Sound_Mutex);
		
		DBG_FREE(f_link->mbuffer.void_data2);
		if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);
		DBG_FREE(f_link->pModule);
		DBG_FREE(f_link);
		dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
		return (void*)(intptr_t)err;
    }
	iSrcPeriodSize = iPeriodSize; // / iCnannels * 2;
	int iPacketSize = iSrcPeriodSize * 4;	
	
	if (iSuppSampleRate != iSampleRate) 
	{
		fPeriodDiff = ((float)iSuppSampleRate / iSampleRate);
		iSrcPeriodSize = iSrcPeriodSize / fPeriodDiff;
		iPacketSize = iSrcPeriodSize * 4;
		//printf("Set sample rate: %i from %i(%i %i) %i\n", iSuppSampleRate, iSampleRate, iPeriodSize, iSrcPeriodSize, iBufferSize);
		iSampleRate = iSuppSampleRate;
	}	
	
	iLen = f_link->mbuffer.data_size;
	char *pBufferOut = (char*)DBG_MALLOC(iBufferSize);
	char *pLastPacket = (char*)DBG_MALLOC(iPacketSize);
	SwrContext *swr = NULL;
	if ((iCnannels == 1) || (iSampleRate != 44100))
	{
		swr = swr_alloc();
		/////////SET convert options				
		av_opt_set_int(swr, "in_sample_rate", (int64_t)44100, 0);
		av_opt_set_int(swr, "out_sample_rate", (int64_t)iSampleRate, 0);
		//av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
		av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		av_opt_set_int(swr, "in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
		av_opt_set_int(swr, "out_channel_layout", (iCnannels == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO, 0);
		swr_init(swr);		
	}
			
//	period_size = buffer_size / audio_channels / sizeof(short);
	//} else period_size = buffer_size / audio_channels / sizeof(short);
	//iSrcPeriodSize = 1024;
	//printf("# %i %i %i %i\n",iPacketSize, iSrcPeriodSize, iPeriodSize, iSampleRate);
	iWriteLen = iPacketSize;
	int iPerdSz = iPeriodSize;
	int iSrcPerdSz = iSrcPeriodSize;
	
	do
	{
		if (iLen < iPacketSize)
		{
			memcpy(pLastPacket, pBuffer, iLen);
			memset(&pLastPacket[iLen], 0, iPacketSize - iLen);
			pBuffer = (unsigned char*)pLastPacket;
			//iWriteLen = iLen;
			//iPerdSz = iWriteLen / 4 * fPeriodDiff;
			//iSrcPerdSz = iWriteLen / 4;
		}
		if ((iCnannels == 1) || (iSampleRate != 44100))
		{
			swr_convert(swr, (uint8_t**)&pBufferOut, iPerdSz, (const uint8_t**)&pBuffer, iSrcPerdSz);	
			frames = snd_pcm_writei(handle, pBufferOut, iPerdSz);
		}
		else frames = snd_pcm_writei(handle, pBuffer, iSrcPerdSz);
			
        if (frames < 0) frames = snd_pcm_recover(handle, frames, 0);
        if (frames < 0) 
		{
            dbgprintf(2,"snd_pcm_writei failed: %s\n", snd_strerror(frames));
            break;
        }
        if (frames > 0 && frames < (long)(iPerdSz))
                        dbgprintf(2,"Short write (expected %li, wrote %li)\n", (long)(iPerdSz), frames);
		pBuffer += iWriteLen;
		iLen -= iWriteLen;
		if ((iLen <= 0) && (iRepeats))
		{
			iRepeats--;
			pBuffer = f_link->mbuffer.void_data2;
			iLen = f_link->mbuffer.data_size;
			iWriteLen = iPacketSize;
			iPerdSz = iPeriodSize;
			iSrcPerdSz = iSrcPeriodSize;
		}
    } while (iLen > 0);
   
    if ((iCnannels == 1) || (iSampleRate != 44100)) swr_free(&swr);
	
	//snd_pcm_nonblock(handle, 0);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	
	DBG_FREE(pBufferOut);
	DBG_FREE(pLastPacket);
	DBG_FREE(f_link->mbuffer.void_data2);
    
	if (GetDoneSoundPlay(f_link->pSubModule, miModule->Settings[5])) gpio_switch_off_module((MODULE_INFO*)f_link->pSubModule);

	if (f_link->pSubModule != NULL) DBG_FREE(f_link->pSubModule);	
	DBG_FREE(f_link->pModule);
    DBG_FREE(f_link);
	
	DBG_MUTEX_LOCK(&Sound_Mutex);
	cThreadSoundsStatus--;
	DBG_MUTEX_UNLOCK(&Sound_Mutex);
	
	dbgprintf(5,"closed audio thread\n");	
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return  (void*)1;
}
