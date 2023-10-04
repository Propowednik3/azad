#ifndef _AUDIO_H_
#define _AUDIO_H_


#include <alsa/asoundlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavformat/avio.h>

#include "main.h"

#define MPA_FRAME_SIZE   1152

#define	AUDIO_EVENT_STOP				100
#define	AUDIO_EVENT_REPLAY				101
#define	AUDIO_EVENT_BUSY				102
#define	MEDIA_EVENT_BUSY_PLAY			103
#define	MEDIA_EVENT_STOP_PLAY			104
#define	MEDIA_EVENT_REPLAY				105
#define	MEDIA_EVENT_STOP_REC			106
#define	MEDIA_EVENT_BUSY_REC			107
#define	CAPTURE_EVENT_VIDEO				108
#define	CAPTURE_EVENT_AUDIO				109
#define	AUDIO_EVENT_BUSY_SOUND			110
#define	MEDIA_EVENT_DONE				111
//#define	AUDIO_EVENT_STOP_SOUND			112
#define	MEDIA_EVENT_NEXT_REC			113
#define	MEDIA_EVENT_NEXT_REC1			114
#define	MEDIA_EVENT_NEXT_REC2			115
#define	CAPTURE_EVENT_STOP				116
#define	CAPTURE_EVENT_BUSY				117
#define	AUDIO_EVENT_PLAY				118
#define	AUDIO_EVENT_PAUSE				119


#define	MEDIA_EVENT_STOP				1
#define	MEDIA_EVENT_NEXT				2
#define	MEDIA_EVENT_VIDEO				4
#define	MEDIA_EVENT_AUDIO				8
#define	MEDIA_EVENT_EXIT				16
#define	MEDIA_EVENT_CODEC				32
#define	MEDIA_EVENT_EX_STOP				64
#define	MEDIA_EVENT_FORWARD				128
#define	MEDIA_EVENT_BACKWARD			256
#define	MEDIA_EVENT_AUDIO_SYNC			512
#define	MEDIA_EVENT_SET_NEW_POS			1024
#define	MEDIA_EVENT_PLAY				2048
#define	MEDIA_EVENT_PAUSE				4096
#define	MEDIA_EVENT_SET_PAUSE_POS		8192


#define ALSA_BUFFER_MULTIPLY 100
#define AVAIL_AUDIO 2

int CreateDirectoryFromPath(char* cPath, int iWithFile, mode_t mode);
char* GetMonthName(int iMonth);

int audio_init();
int audio_close(void);

int Audio_IsFree(void);
int MediaPlay_IsFree();
int MediaRecord_IsFree();

int Audio_Stop(int iWaitStop);
int Audio_Play();
int Audio_Pause();
int Media_StopPlay(int iWaitStop);
void Media_Replay(void);
int Media_NextRecID(unsigned int uiModuleID, unsigned int uiType, unsigned int uiCmd);
int Media_NextRecNum(unsigned int uiNum, unsigned int uiType, unsigned int uiCmd);
int Media_Pause();
int Media_Play();
int Media_StopRec(int iWaitStop);
int Audio_StopCapture(int iWaitStop);
int Media_Forward(unsigned int iDiffPos);
int Media_Backward(unsigned int iDiffPos);
int Media_SetNewPos(unsigned int uiPos);
void audio_set_playback_volume(int iVolume);
void audio_set_capture_volume(int iDev, int iVolume);
int audio_get_playback_volume();
int audio_get_capture_volume(int iDev);
int audio_get_capture_agc(int iDev);
int audio_set_capture_agc(int iDev, int iVal);
unsigned int audio_get_volume();
void SetAudioPlayDeviceName(int iDev);
void SetAudioCaptDeviceName(int iDev);
void GetPlayBufferStatus(unsigned int *uiVideo, unsigned int *uiAudio, unsigned int *uiPosition);

int encode_audio(AVFrame *av_frame, AVFormatContext *format_ctx, unsigned char *ucBuffer, int iBufferSize, int *iPacketSize);
int open_audio_play_device(void **audio_play_handle, char* audio_play_dev, int iChannels, int iRate);
void audio_decode_example(const char *outfilename, const char *filename);
int AudioFileInBuffer(char *cFileName, void **pBufferOut, unsigned int *uiBufferSize, unsigned int uiSampleRateFilter, unsigned int audio_channels);

int ReadVideoFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* StartPack, char cStream, unsigned int ConnectNum, unsigned int ConnectID);
int ReadAudioFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* StartPack, char cStream, unsigned int ConnectNum, unsigned int ConnectID);
int SaveVideoFrame(unsigned int iNumFrame, void* omxbuffer, int BufferSize, void* tVideoInfo, void* StartPack, void *vStream);
int SaveDiffVideoFrame(unsigned int iNumFrame, void* omxbuffer, int BufferSize, void* tVideoInfo, void* StartPack, void *vStream);
int SaveVideoKeyFrame(unsigned int iNumFrame, void* omxbuffer, int BufferSize, void* tVideoInfo, void* StartPack, void *vStream);
int SaveAudioFrame(unsigned int iNumFrame, char* Buffer, int LenData, void* pData, void *vStream);
int SkipAudioFrame(void *vStream);
int UnSkipAudioFrame( void *vStream);

int GetTypesStreamsMediaFile(char *cFileName);
int CaptureAudioStream(void *flink);
int SaveCapturedStreams(char *cPath, STREAM_INFO *pStream, int iMaxFileSize, int MaxDuration, int iHaveVideo, int iHaveAudio, 
			char cCopyFile, char* cSavePath, unsigned int uiDiff, unsigned int uiSelfEnc, unsigned int uiAudioCodec, 
			unsigned char ucOrderLmt, char *cAddrOrderer, char *cPrePath, unsigned int uiFlvBuffSize,
			unsigned int uiMaxWaitCopyTime, unsigned int uiMessWaitCopyTime, unsigned int uiBackUpType);
int PlayMediaFile(char *cPath, int video_enable, int audio_enable, char overTCP);
int PlayAudioStream(func_link *f_link);
int PlayAudioSound(func_link *f_link);

void* thread_PlayAudioSound(void *pData);
void* thread_PlayAudioStream(void *pData);
void* thread_CaptureAudioStream(void *pData);
void* thread_PlayMediaFile(void *cFileName);
void* thread_SaveMediaFile(void* vFileName);
void* thread_SaveFLVFile(void* vFileName);

pthread_mutex_t Play_Mutex;
unsigned int uiAudioDataCnt;
unsigned int uiVideoDataCnt;
unsigned int uiNewMediaPos;
unsigned int cThreadAudCaptStatus;
TX_EVENTER 	pevntRMS;

#endif