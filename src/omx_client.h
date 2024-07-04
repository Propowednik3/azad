#pragma once

#ifndef _OMXCLIENT_H_
#define _OMXCLIENT_H_

#include <pthread.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_IVCommon.h>

#include <libavcodec/avcodec.h>

#include "pthread2threadx.h"
#include "main.h"
#include "network.h"


//#define VIDEO_WIDTH                     1600
//#define VIDEO_HEIGHT                    1200
//#define VIDEO_WIDTH32     			7	((VIDEO_WIDTH+31)&~31)
//#define VIDEO_HEIGHT16  				((VIDEO_HEIGHT+15)&~15)
//#define VIDEO_FRAME_SIZE      			((VIDEO_WIDTH * VIDEO_HEIGHT16 * 3)/2)
//#define VIDEO_FRAMERATE                 5
//#define VIDEO_INTRAPERIOD				60
//#define VIDEO_BITRATE                   5000000
#define VIDEO_COLOR_FORMAT				OMX_COLOR_FormatYUV420PackedPlanar

#define OMX_VIDEO_AVCLevelUnknown 0x6EFFFFFF
#define OMX_VIDEO_AVCProfileUnknown 0x6EFFFFFF

#define OMX_AVC_PROFILE 	OMX_VIDEO_AVCProfileBaseline
#define OMX_AVC_LEVEL 		OMX_VIDEO_AVCLevel11

#define OMX_BUFFERFLAG_EOS 0x00000001 
#define OMX_BUFFERFLAG_STARTTIME 0x00000002 
#define OMX_BUFFERFLAG_DECODEONLY 0x00000004 
#define OMX_BUFFERFLAG_DATACORRUPT 0x00000008 
#define OMX_BUFFERFLAG_ENDOFFRAME 0x00000010 
#define OMX_BUFFERFLAG_SYNCFRAME 0x00000020 
#define OMX_BUFFERFLAG_EXTRADATA 0x00000040 
#define OMX_BUFFERFLAG_CODECCONFIG 0x00000080 
#define OMX_BUFFERFLAG_TIMESTAMPINVALID 0x00000100 
#define OMX_BUFFERFLAG_READONLY 0x00000200 
#define OMX_BUFFERFLAG_ENDOFSUBFRAME 0x00000400 
#define OMX_BUFFERFLAG_SKIPFRAME 0x00000800 

//PORTS
#define	OMX_PORT_1			0
#define	OMX_PORT_2			1
#define	OMX_PORT_3			2
#define	OMX_PORT_4			3
#define	OMX_PORT_5			4
#define	OMX_PORT_6			5

//PORT TYPES
#define	OMX_PORT_UNKNOWN	0
#define	OMX_PORT_AUDIO		1
#define	OMX_PORT_IMAGE		2
#define	OMX_PORT_TEXT		3
#define	OMX_PORT_VIDEO		4
#define	OMX_PORT_CLOCK		5

#define	OMX_PORT_IN			0
#define	OMX_PORT_OUT		1
#define	OMX_LATER			0
#define	OMX_NOW				1


//COMPONENT TYPES
#define	OMX_COMP_UNKNOWN			0
#define	OMX_COMP_AUDIO_CAPTURE		1
#define	OMX_COMP_AUDIO_DECODE		2
#define	OMX_COMP_AUDIO_ENCODE		3
#define	OMX_COMP_AUDIO_LOWPOWER		4
#define	OMX_COMP_AUDIO_MIXER		5
#define	OMX_COMP_AUDIO_PROCESSOR	6
#define	OMX_COMP_AUDIO_RENDER		7
#define	OMX_COMP_AUDIO_SPLITTER		8
#define	OMX_COMP_IMAGE_DECODE		9
#define	OMX_COMP_IMAGE_ENCODE		10
#define	OMX_COMP_IMAGE_FX			11
#define	OMX_COMP_IMAGE_READ			12
#define	OMX_COMP_IMAGE_WRITE		13
#define	OMX_COMP_RESIZE				14
#define	OMX_COMP_SOURCE				15
#define	OMX_COMP_TRANSITION			16
#define	OMX_COMP_WRITE_STILL		17
#define	OMX_COMP_CLOCK				18
#define	OMX_COMP_NULL_SINK			19
#define	OMX_COMP_TEXT_SCHEDULER		20
#define	OMX_COMP_VISUALISATION		21
#define	OMX_COMP_READ_MEDIA			22
#define	OMX_COMP_WRITE_MEDIA		23
#define	OMX_COMP_CAMERA				24
#define	OMX_COMP_EGL_RENDER			25
#define	OMX_COMP_VIDEO_DECODE		26
#define	OMX_COMP_VIDEO_ENCODE		27
#define	OMX_COMP_VIDEO_RENDER		28
#define	OMX_COMP_VIDEO_SCHEDULER	29
#define	OMX_COMP_VIDEO_SPLITTER		30

#define	OMX_BUFFERS_MAX				32
#define	OMX_COMPONENTS_MAX			64

#define	OMX_AUTOFOCUS_WIDTH			159
#define	OMX_AUTOFOCUS_HEIGHT		119

#define	OMX_CommandAny			0X7FFFFFFE
#define	OMX_EventFillBuffer		0X7FFFFFFD
#define	OMX_EventEmptyBuffer	0X7FFFFFFC

#define VIDEO_CODER_BUFFER_SIZE 524288

enum OMX_EVENT 
{
	OMX_EVENT_STOP_VIDEO	= 200,
	OMX_EVENT_REPLAY_VIDEO,
	OMX_EVENT_BUSY_VIDEO,
	OMX_EVENT_STOP_AUDIO,
	OMX_EVENT_REPLAY_AUDIO,
	OMX_EVENT_BUSY_AUDIO,
	OMX_EVENT_SYNC_VIDEO,
	OMX_EVENT_WAITRENDER_VIDEO,
	OMX_EVENT_BUSY_CAPTURE,
	OMX_EVENT_STOP_CAPTURE,
	OMX_EVENT_STOP_PLAY_VIDEO,
	OMX_EVENT_BUSY_PLAY_VIDEO,
	OMX_EVENT_PLAYER_PAUSE,
	OMX_EVENT_PLAYER_PLAY,
	OMX_EVENT_PLAYER_SPEEDUP,
	OMX_EVENT_PLAYER_SPEEDDOWN
};

#define	CODECINFO_BUFFER_SIZE	1024
#define	DARK_SENSOR_BACKLASH	10
//#define	LIGHT_SENSOR_BACKLASH	15

#define OMX_TEST_ERROR(x) 		omx_test_error(x, __FILE__, __func__, __LINE__)	

#define DBG_OERR(cmd)        do {                                                \
										OMX_ERRORTYPE oerr = cmd;                \
										if (oerr != OMX_ErrorNone) 					\
										{                					\
											dbgprintf(1, "OpenMax error: %x, (%s)\n", oerr, omx_err2str(oerr));        \
											dbgprintf(4, #cmd                \
                                                " failed on line %d: %x, (%s)\n", \
                                                __LINE__, oerr, omx_err2str(oerr));   \
											pthread_exit(0);				\
											return 0;							\
										}                                     \
									} while (0)
	

typedef struct omx_ports
{
	int 					number;
	int						type;
	int						busy;
	OMX_U32					buffers_size;
	OMX_BUFFERHEADERTYPE 	*buffer[OMX_BUFFERS_MAX];
	void					*buffer_ptr[OMX_BUFFERS_MAX];
    int 					buffers_count;
    int						buffer_active;
    int						buffer_empty[OMX_BUFFERS_MAX];    
  /*  void					*source_buffer;
	int						source_buffer_data_size;
	int						source_buffer_position;
	int						source_buffer_flag;*/
	void 					*EglImage;
} omx_ports;

typedef struct omx_component
{
	int				type;
	OMX_HANDLETYPE  handle;
	int 			in_port_cnt;
	int 			out_port_cnt;
	omx_ports 		in_port[5];
	omx_ports		out_port[6];
	TX_SEMAPHORE 	*psem;
} omx_component;

typedef struct omx_buffer 
{
  char* data;
  unsigned int data_size;  
} omx_buffer;

typedef struct exposure_params 
{
  unsigned int Mode;
  float Priority;
  float MinExposureTime;
  float MaxExposureTime;
  float MinGain;
  float MaxGain;
  float MinIris;
  float MaxIris;
  float ExposureTime;
  float Gain;
  float Iris;
} exposure_params;

typedef struct focus_params 
{
  unsigned int AutoFocusMode;
  int Status;
  int Position;
  float DefaultSpeed;
  float NearLimit;
  float FarLimit;
} focus_params;

typedef struct whitebalance_params 
{
  unsigned int Mode;
  float CrGain;
  float CbGain;
} whitebalance_params;

typedef struct image_sensor_params 
{
	unsigned int			ModuleID;
	char					Updated;
	char					StartAutofocus;
	unsigned int 			BacklightCompensationMode;
	float					BacklightCompensationLevel;
	float					Brightness;
	float					ColorSaturation;
	float					Contrast;
	exposure_params 		Exposure;
	focus_params 			Focus;
	unsigned int 			IrCutFilter;
	float 					Sharpness;
	unsigned int 			WideDynamicRangeMode;
	float 					WideDynamicRangeLevel;
	whitebalance_params 	WhiteBalance;
	unsigned int			ImageFilter;
	VideoCodecInfo			MainVideo;
	VideoCodecInfo			PrevVideo;
	unsigned int			MainVideoAvcProfile;
	unsigned int			MainVideoAvcLevel;
	unsigned int			MainVideoConstantBitRate; 
	unsigned int			PrevVideoAvcProfile;
	unsigned int			PrevVideoAvcLevel;
	unsigned int			PrevVideoConstantBitRate; 
	unsigned int			KeepRatio;
	unsigned int			FrameLeftCrop;
	unsigned int			FrameRightCrop;
	unsigned int			FrameUpCrop;
	unsigned int			FrameDownCrop;
	unsigned int			PreviewEnabled;
	unsigned int			RecordEnabled;
	unsigned int			FlipHorisontal; 
	unsigned int			FlipVertical;
	unsigned int			RotateMode;
	unsigned int			AspectRatioW;
	unsigned int			AspectRatioH;

	unsigned int 			PrevColorLevel;
	unsigned int 			PrevBrigLevel;
	unsigned int 			PrevShowLevel;
	unsigned int 			PrevSettings;
	unsigned int 			MainSettings;
	unsigned int 			HighLightMode;
	unsigned int 			HighLightBright;
	unsigned int 			HighLightAmplif;
	unsigned int			AutoBrightControl;
	unsigned int			DestBrightControl;
	
	unsigned int			ISOControl;
	unsigned int			HardAutoISOControl;
	unsigned int			SoftAutoISOControl;
	unsigned int			DestAutoISOControl; 
	unsigned int			EVControl;
	unsigned int			HardAutoEVControl;
	unsigned int			SoftAutoEVControl;
	unsigned int			DestAutoEVControl; 
	unsigned int			ChangedPosition;
						
} image_sensor_params;

typedef struct omx_start_packet 
{
	char *BufferCodecInfo;
	int CodecInfoLen;
	int CodecInfoPos;
	char CodecInfoFilled;
	char *BufferStartFrame;
	int BufferStartSize;
	int StartFrameLen;
	unsigned int *StartFramesSizes;
	unsigned char *StartFramesFlags;
	int StartFramesCount;
	char StartFrameFilled;
	char HaveChangedFrames;
	VideoCodecInfo VideoParams;
	int SendType;
	int PacketType;
} omx_start_packet;

typedef struct omx_egl_link 
{
	GLuint texture_id;
	EGLDisplay * eglDisplay;
	EGLContext * eglContext;
	void * eglImage;	
	GLuint	sizeW;
	GLuint	sizeH;
	GLuint	attr;
	char *data;	
	TX_SEMAPHORE 	*psem_init;
	char errorcode;
	int (*FuncRecv)(void*, unsigned int*, int*, void*, char, unsigned int, unsigned int);
	int (*FuncSend)(unsigned int, char*, int, int, int, void*, void*, int);
	int (*FuncSave)(unsigned int, void*, int, void*, void*, void*);
	int (*FuncSaveSlow)(unsigned int, void*, int, void*, void*, void*);
	int (*FuncSaveDiff)(unsigned int, void*, int, void*, void*, void*);
	int (*FuncRTSP)(unsigned int, char*, unsigned int, void*);	
	unsigned int ConnectNum;
	unsigned int ConnectID;
	unsigned int DeviceNum;
	unsigned int Type;
	struct sockaddr_in Address;
	void *pModule;
	void *pSubModule;	
} omx_egl_link;

typedef struct omx_autofocus_params 
{
	int 		 Status;
	char		 Direct;
	char		 ScanCnt[4];
	unsigned char Mode;
	unsigned int SearchStep;
	unsigned int FullScanCounter;
	unsigned int MaxContrastVal[4];
	unsigned int LastContrast[4];
	int 		 CurrentFocusPos;	
	int 		 LastFocusStep;
	unsigned int ZoomMoveCounter;
	int 		 MaxContrastPos[4];
	unsigned int SensorPosX;
	unsigned int SensorPosY;
	unsigned int SensorWidth;
	unsigned int SensorHeight;
	unsigned int Enabled;
	unsigned int PtzEnabled;
	unsigned int PtzID;
	unsigned int PtzFocusMapEnabled;
	unsigned int PtzFocusMapChanged;
	uint16_t	 *PtzFocusMap;
	int			 PrevZoomStatus;
	int			 ZoomChanged;
	int			 FocusChanged;
	double		 CurrCoef;
	int			 MinFocusPos;
	int			 MaxFocusPos;
	int			 FullMinFocusPos;
	int			 FullMaxFocusPos;
	char		 GotoHomeAfterStart;
	char		 GotoHomeAfterTimeout;
	char		 GotoHomeDone;
	int		 	 GotoHomeTime;
	unsigned int GotoHomeCounter;
} omx_autofocus_params;

//EGLDisplay *eglDisplay;
//EGLContext *eglContext;

extern TX_SEMAPHORE 	psem_omx_sync;
extern TX_SEMAPHORE 	psem_omx_run;
extern int text_id;

extern image_sensor_params ispCameraImageSettings;	

//OMX_BUFFERHEADERTYPE *texture_bufferHeader;
//EGLImageKHR texture_mem_handle;

int omx_test_error(OMX_ERRORTYPE oerr, char *pFileName, const char *pFuncName, int iLine);
char* omx_err2str(int err);
char debug_dumpdata(char *cFileName, int iNum, int iCount, char *Buff, int iLen);
void omx_GetComponentName(int ComponentID, char *Name, int MaxLen);
int omx_send_cmd (int comp_num, int cmd, int param);
int omx_LoadComponent(int ComponentID, int iFunc);
int omx_InitComponent(int ComponentNum, OMX_CALLBACKTYPE *Calls);
int omx_Start(void);
int omx_Release();
int omx_IsFree_Video();
int omx_IsFree_Audio();
int omx_IsFree();
int omx_Stop(void);
void omx_rollback (int stage);
int omx_load_file (char *filename, omx_buffer * buffer);
int omx_play_video_file(char* cPath);
void omx_play_video_on_egl(char *FilePath, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH);
void* thread_omx_play_video_on_egl(void * pdata);
void* thread_omx_play_video_on_egl_from_func(void * pdata);
void* thread_omx_play_video_clk_on_egl_from_func(void * pdata);
void omx_image_to_egl_resize(char *FilePath, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH);
void* thread_omx_image_to_egl_resize(void * pdata);
int omx_play_video_on_egl_from_func(func_link *f_link, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH, int TimeCtrl, int iPrevType);
int omx_load_data_in_buffer(int CompNum, int port, void *DataBuff, int iDataSize, int iFlag, int iFlagAuto, int iTimeOut_ms);
int omx_create_egl_buffer(int CompNum, int width, int height, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext);
int omx_use_egl_buffer(int CompNum);
void omx_set_handle(int CompNum, OMX_HANDLETYPE hndl);
OMX_HANDLETYPE omx_get_handle(int CompNum);
int omx_set_tunnel(int Comp1, int Port1, int Comp2, int Port2, int iPortEnable);
int omx_empty_all_buffers(int CompNum, int port, int port_type);
void omx_get_semaphore(void **psem);
int omx_wait_exec_cmd(int CompNum);
int omx_wait_event_timeout(int CompNum, unsigned int event, unsigned int cmd, int timeout_ms);
int omx_wait_exec_spec_cmd(int CompNum);
int omx_wait_exec_cmd_timeout(int CompNum, int timeout_ms);
int omx_wait_exec_spec_cmd_timeout(int CompNum, int timeout_ms);
int omx_wait_list_empty(int CompNum);
int omx_wait_spec_list_empty(int CompNum);
int omx_reset_exec_cmd(int CompNum);
int omx_activate_buffers(int CompNum, int port, int port_type, int iFlags);
int omx_switch_next_buffer(int CompNum, int port, int port_type);
int omx_remove_buffers (int CompNum, int port, int port_type);
int omx_create_buffers2(int CompNum, int port, int port_type);
int omx_create_buffers(int CompNum, int port, int port_type);
int omx_create_buffers_exist(int CompNum, int port, int port_type, char* exbuff);
int omx_get_buffer_size(int CompNum, int port, int port_type, int *sizebuff);
int omx_set_count_buffers(int CompNum, int port, int port_type, int count, OMX_U32 sizebuff);
int omx_normalize_frame_format(int CompNum, int port, int port_type, int nFrameWidth, int nFrameHeight, int nMaxWidth, int nMaxHeight);
int omx_get_frame_resolution(int CompNum, int port, int port_type,  int *nFrameWidth, int *nFrameHeight);
int omx_set_frame_settings(int CompNum, int port, int port_type,  int nFrameWidth, int nFrameHeight, int nColorFormat);
int omx_set_video_compression_format(int CompNum, int port, int port_type, OMX_VIDEO_CODINGTYPE oCodeFormat);
int omx_set_image_compression_format(int CompNum, int port, int port_type, int oCodeFormat, OMX_COLOR_FORMATTYPE oColorFormat);
int omx_set_stride_slice_port(int CompNum, int port, int port_type, int stride, int slice);
int omx_set_state (int CompNum, int state_type, int now);
int omx_enable_port (int CompNum, int port, int port_type, int now);
int omx_disable_port (int CompNum, int port, int port_type, int now);
int omx_flush_port (int CompNum, int port, int port_type, int now);
int omx_delete_cmd_from_list(int CompNum, int event, int cmd);
int omx_print_wait_list(int CompNum);
int omx_add_cmd_in_list_spec(int CompNum, int event, int cmd, int NotLock);
int omx_add_cmd_in_list(int CompNum, int event, int cmd);
int omx_camera_init(int CompNum, int iFrameWidth, int iFrameHeight, int iFramerate, int iColorFormat, int iImage);
int omx_image_encoder_init(int CompNum, int iFrameWidth, int iFrameHeight, int iCompressionFormat);
int omx_video_encoder_init(int CompNum, int iFrameWidth, int iFrameHeight, int iFramerate, int iBitrate, int iCompressionFormat, int iIntraPeriod, int iInInit, int iBitType, int iProfile, int iLevel);
int omx_video_recorder(char *file_name);
int omx_video_capture(char *file_name);
int omx_image_capture_resize(char *file_name);
void* thread_omx_video_capture_send(void *pdata);
void omx_stop_video(int iWait);
void omx_stop_video_num(unsigned int uiNum, int iWait);
void omx_stop_capture(int iWait);
void omx_replay_video();
void omx_player_pause();
void omx_player_play();
void omx_player_speedup();
void omx_player_speeddown();
int omx_player_get_speed();
int omx_Release_Component(int iNum);
int omx_image_to_buffer(char *FilePath, void ** vBuffer, unsigned int *iSize, int *sW, int *sH);
int omx_image_to_egl(char *FilePath, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH);
int omx_copy_frame_settings(int CompNum, int port, int port_type,  int CompNumTo, int portTo, int port_typeTo);
int omx_set_frame_settings_ex(int CompNum, int port, int port_type,  int nFrameWidth, int nFrameHeight, 
								int eColorFormat, int nStride, int nSliceHeight, int eCompressionFormat, int bFlagErrorConcealment);
int omx_play_video_on_screen_from_func(func_link *f_link, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH, int TimeCtrl);
void* thread_omx_play_video_clk_on_screen_from_func(void * pdata);	
char* GetOmxProfileName(unsigned int uiVal);
unsigned int GetOmxLevelNum(unsigned int uiVal);
char* GetH264ProfileName(unsigned int uiVal);
int GetH264ProfileCode(char *cName);
char* GetNameExposure(unsigned int uiData);		
char* GetCodeNameExposure(unsigned int uiData);
unsigned int GetStandartExposure(unsigned int uiData);
char* GetNameImageFilter(unsigned int uiData);		
char* GetCodeNameImageFilter(unsigned int uiData);
unsigned int GetStandartImageFilter(unsigned int uiData);
char* GetNameWhiteBalance(unsigned int uiData);		
char* GetCodeNameWhiteBalance(unsigned int uiData);
unsigned int GetStandartWhiteBalance(unsigned int uiData);
void omx_set_image_sensor_module_params(image_sensor_params *ispCSI, MODULE_INFO *miModule);
void omx_get_image_sensor_module_params(image_sensor_params *ispCSI, MODULE_INFO *miModule);
int omx_set_image_sensor_camera_params(image_sensor_params *ispCSI);
int omx_dump_data(char *cName, char *cData, int iLen);
int omx_load_file_size(char *filename, char *buffer, unsigned int uiSize);
void GetResolutionFromMode(int iMode, int* pW, int *pH);

extern char cThreadOmxPlayStatus;
extern char cThreadOmxImageStatus;
extern char cThreadOmxCaptureStatus;
extern char cThreadOmxEncoderStatus;
extern pthread_mutex_t OMX_mutex;
extern int	omx_resource_priority;
extern int	omx_speed_play;

#endif