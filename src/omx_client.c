#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <limits.h>
#include "omx_client.h"
#include "weather.h"
#include "weather_func.h"
#include "debug.h"
#include "audio.h"
#include "gpio.h"
#include "rtsp.h"
#include "onvif.h"

#define OMX_COLOR_Format32bitRGBA8888	0x7F000012

#define JPEG_QUALITY 75 //1 .. 100
#define JPEG_EXIF_DISABLE OMX_FALSE
#define JPEG_IJG_ENABLE OMX_FALSE
#define JPEG_THUMBNAIL_ENABLE OMX_TRUE
#define JPEG_THUMBNAIL_WIDTH 64 //0 .. 1024
#define JPEG_THUMBNAIL_HEIGHT 48 //0 .. 1024
#define JPEG_PREVIEW OMX_FALSE

//#define VIDEO_CODER_BUFFER_SIZE 32768
#define VIDEO_CODER_BUFFER_COUNT 1

#define VIDEO_CODEC						OMX_VIDEO_CodingAVC

#define CAM_DEVICE_NUMBER               0
#define CAM_SHARPNESS                   0                       // -100 .. 100
#define CAM_CONTRAST                    0                       // -100 .. 100
//#define CAM_BRIGHTNESS                60                      // 0 .. 100

#define CAM_BRIGHTNESS                  50                      // 0 .. 100
#define CAM_SATURATION                  0                       // -100 .. 100

#define CAM_FRAME_STABILISATION         OMX_FALSE
//#define CAM_WHITE_BALANCE_CONTROL       OMX_WhiteBalControlAuto // OMX_WHITEBALCONTROLTYPE
//#define CAM_WHITE_BALANCE_CONTROL       OMX_WhiteBalControlOff // OMX_WHITEBALCONTROLTYPE
//#define CAM_IMAGE_FILTER 				OMX_ImageFilterNone		//
#define CAM_IMAGE_FILTER 				OMX_ImageFilterNoise
#define CAM_FLIP_HORIZONTAL             OMX_FALSE
#define CAM_FLIP_VERTICAL               OMX_FALSE

#define CAM_METERING OMX_MeteringModeAverage 	// Center-weighted average metering. 
//#define CAM_METERING OMX_MeteringModeSpot  	// Spot (partial) metering. 
//#define CAM_METERING OMX_MeteringModeMatrix	// Matrix or evaluative metering. 
#define CAM_EXPOSURE_COMPENSATION 		0 						//-24 .. 24
#define CAM_SHUTTER_SPEED_AUTO 			OMX_TRUE
#define CAM_SHUTTER_SPEED 				0
#define CAM_EXPOSURE_ISO_SENSITIVITY    0						 //100 .. 800
#define CAM_EXPOSURE_AUTO_SENSITIVITY   OMX_TRUE

#define CAM_ROTATION 0 //0 90 180 270
#define CAM_COLOR_ENABLE OMX_FALSE
#define CAM_COLOR_U 128 //0 .. 255
#define CAM_COLOR_V 128 //0 .. 255
#define CAM_NOISE_REDUCTION OMX_TRUE
#define CAM_WHITE_BALANCE_CONTROL OMX_WhiteBalControlAuto
//The gains are used if the white balance is set to off
#define CAM_WHITE_BALANCE_RED_GAIN 1100 //0 ..
#define CAM_WHITE_BALANCE_BLUE_GAIN 100 //0 ..

//#define CAM_IMAGE_FILTER OMX_ImageFilterAntialias
#define CAM_DRC OMX_DynRangeExpOff

// Dunno where this is originally stolen from...
#define OMX_INIT_STRUCTURE(a) \
    memset(&(a), 0, sizeof(a)); \
    (a).nSize = sizeof(a); \
    (a).nVersion.nVersion = OMX_VERSION; \
    (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
    (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
    (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
    (a).nVersion.s.nStep = OMX_VERSION_STEP

#define PTZ_STATUSES_MAX	16
#define PTZ_START_STEP_SIZE	8

//#endif  
		
void RecalcRectangles(int iCnt, RECT_INFO *riList, int iMW, int iMH, int iSW, int iSH);
int omx_set_crop_value(int CompNum, unsigned int xLeft, unsigned int xRight, unsigned int xTop, unsigned int xDown);
int omx_set_flip_value(int CompNum, int iFlipHor, int iFlipVert, int iImage);
int omx_set_rotate_value(int CompNum, int iRotAngle, int iImage);
int omx_set_exposure_value(int CompNum, int eExposureControl);
int omx_set_iso_value(int CompNum, int iExpCompens, int iAutoISO, int iValueISO);
int omx_set_imagefilter_value(int CompNum, int eFilter);
int omx_set_whitebalance_value(int CompNum, int eBalance);
int omx_set_bright_value(int CompNum, int eBrightControl);
int omx_set_contrast_value(int CompNum, int eContrastControl);
int omx_set_sharpness_value(int CompNum, int eSharpnessControl);
int omx_set_saturation_value(int CompNum, int eSaturationControl);

void* thread_omx_play_video_noclk_on_egl_from_func(void * pdata);

TX_SEMAPHORE 	psem_components;
static pthread_t thread1;
pthread_attr_t tattr;
uint64_t gl_previous_ms;
    
static omx_component   m_oCompList[OMX_COMPONENTS_MAX];
int 			m_iStarted = 0;
int 			m_iFirstStart = 1;

//FILE 		    *in;
//char in2;
//static int want_quit = 0;

char* omx_err2str(int err) 
{
    switch (err) {
    case OMX_ErrorInsufficientResources: return "OMX_ErrorInsufficientResources";
    case OMX_ErrorUndefined: return "OMX_ErrorUndefined";
    case OMX_ErrorInvalidComponentName: return "OMX_ErrorInvalidComponentName";
    case OMX_ErrorComponentNotFound: return "OMX_ErrorComponentNotFound";
    case OMX_ErrorInvalidComponent: return "OMX_ErrorInvalidComponent";
    case OMX_ErrorBadParameter: return "OMX_ErrorBadParameter";
    case OMX_ErrorNotImplemented: return "OMX_ErrorNotImplemented";
    case OMX_ErrorUnderflow: return "OMX_ErrorUnderflow";
    case OMX_ErrorOverflow: return "OMX_ErrorOverflow";
    case OMX_ErrorHardware: return "OMX_ErrorHardware";
    case OMX_ErrorInvalidState: return "OMX_ErrorInvalidState";
    case OMX_ErrorStreamCorrupt: return "OMX_ErrorStreamCorrupt";
    case OMX_ErrorPortsNotCompatible: return "OMX_ErrorPortsNotCompatible";
    case OMX_ErrorResourcesLost: return "OMX_ErrorResourcesLost";
    case OMX_ErrorNoMore: return "OMX_ErrorNoMore";
    case OMX_ErrorVersionMismatch: return "OMX_ErrorVersionMismatch";
    case OMX_ErrorNotReady: return "OMX_ErrorNotReady";
    case OMX_ErrorTimeout: return "OMX_ErrorTimeout";
    case OMX_ErrorSameState: return "OMX_ErrorSameState";
    case OMX_ErrorResourcesPreempted: return "OMX_ErrorResourcesPreempted";
    case OMX_ErrorPortUnresponsiveDuringAllocation: return "OMX_ErrorPortUnresponsiveDuringAllocation";
    case OMX_ErrorPortUnresponsiveDuringDeallocation: return "OMX_ErrorPortUnresponsiveDuringDeallocation";
    case OMX_ErrorPortUnresponsiveDuringStop: return "OMX_ErrorPortUnresponsiveDuringStop";
    case OMX_ErrorIncorrectStateTransition: return "OMX_ErrorIncorrectStateTransition";
    case OMX_ErrorIncorrectStateOperation: return "OMX_ErrorIncorrectStateOperation";
    case OMX_ErrorUnsupportedSetting: return "OMX_ErrorUnsupportedSetting";
    case OMX_ErrorUnsupportedIndex: return "OMX_ErrorUnsupportedIndex";
    case OMX_ErrorBadPortIndex: return "OMX_ErrorBadPortIndex";
    case OMX_ErrorPortUnpopulated: return "OMX_ErrorPortUnpopulated";
    case OMX_ErrorComponentSuspended: return "OMX_ErrorComponentSuspended";
    case OMX_ErrorDynamicResourcesUnavailable: return "OMX_ErrorDynamicResourcesUnavailable";
    case OMX_ErrorMbErrorsInFrame: return "OMX_ErrorMbErrorsInFrame";
    case OMX_ErrorFormatNotDetected: return "OMX_ErrorFormatNotDetected";
    case OMX_ErrorContentPipeOpenFailed: return "OMX_ErrorContentPipeOpenFailed";
    case OMX_ErrorContentPipeCreationFailed: return "OMX_ErrorContentPipeCreationFailed";
    case OMX_ErrorSeperateTablesUsed: return "OMX_ErrorSeperateTablesUsed";
    case OMX_ErrorTunnelingUnsupported: return "OMX_ErrorTunnelingUnsupported";
    default: return "unknown error";
    }
}

int omx_test_error(OMX_ERRORTYPE oerr, char *pFileName, const char *pFuncName, int iLine)
{
	int res = 0;
	if (oerr != OMX_ErrorNone)
	{
		dbgprintf(1, "OpenMax error: %x, (%s) in %s\n", oerr, omx_err2str(oerr), pFuncName);
		dbgprintf(4, "Function failed on line %d: %x, (%s), file: %s\n", iLine, oerr, omx_err2str(oerr), pFileName);
		res = 1;
	}
	return res;
}

char debug_dumpdata(char *cFileName, int iNum, int iCount, char *Buff, int iLen)
{
	//DBG_LOG_IN();
	
    time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);

    char filebuffer[256];
	     	
    memset(filebuffer, 0 , 256);
    if (iCount == 0) sprintf(filebuffer,"%s_%i%i%i %i%i%i.dat", cFileName, timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
		else sprintf(filebuffer,"%s_%i_%i.dat", cFileName, iNum, iCount);

	FILE *f;
	if ((f = fopen(filebuffer,"wb+")) == NULL)
	{
		dbgprintf(1,"Error open log file:%s\n", filebuffer);
		return 0;
	}
	int iWriteLen = fwrite(Buff, 1, iLen, f);
	if(iWriteLen != iLen) 
	{
		dbgprintf(1,"Error write file:%s\n", filebuffer);
		return 0;
	}
	fclose(f);
	return 1;
}

int omx_dump_data(char *cName, char *cData, int iLen)
{
	//DBG_LOG_IN();
	
	FILE *file;
	if ((file = fopen(cName,"wb+")) == NULL)
	{
		dbgprintf(1,"Error create file:%s\n", cName);
		return 0;
	}
	int iWriteLen = fwrite(cData, 1, iLen, file);
	if(iWriteLen != iLen) 
	{
		dbgprintf(1,"Error write file:%s\n", cName);
		return 0;
	}
	fclose(file);
	return 1;
}    
	
/*static void signal_handler(int signal) 
{
    want_quit = 1;
	dbgprintf(1,("quiting...\n");	
}*/

static OMX_TICKS ToOMXTime(int64_t value)
{
    OMX_TICKS s;
    s.nLowPart  = value & 0xffffffff;
    s.nHighPart = value >> 32;
    return s;
}

static int64_t FromOMXTime(OMX_TICKS value)
{
    return (((int64_t)value.nHighPart) << 32) | value.nLowPart;
}
/*
static OMX_TICKS ToOMXTime(int64_t pts)
{
  OMX_TICKS ticks;
  ticks.nLowPart = pts & 0xffffffff;
  ticks.nHighPart = pts >> 32;
  return ticks;
}

static int64_t FromOMXTime(OMX_TICKS ticks)
{
  int64_t pts = ticks.nLowPart | ((uint64_t)(ticks.nHighPart) << 32);
  return pts;
}*/
	
static OMX_ERRORTYPE CallBackEmpty(
    OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, 
    OMX_OUT OMX_BUFFERHEADERTYPE *pBuffer)
{	
	DBG_LOG_IN();
	
	int CompNum;
	CompNum = (int)(intptr_t)pAppData;
	dbgprintf(8,"buffer empty %i %i\n", CompNum, (unsigned int)(intptr_t)(pBuffer->pAppPrivate));
	//printf("OMX_EventEmptyBuffer\n");		
	pBuffer->nFilledLen = 0;
	if (m_oCompList[CompNum].psem)
		tx_semaphore_go(m_oCompList[CompNum].psem, OMX_EventEmptyBuffer, (unsigned int)(intptr_t)(pBuffer->pAppPrivate));
		else dbgprintf(2, "CallBackEmpty: Critical warning %i\n", CompNum);
	
	DBG_LOG_OUT();
	return OMX_ErrorNone;
}

static OMX_ERRORTYPE CallBackFill(
    OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, 
    OMX_OUT OMX_BUFFERHEADERTYPE *pBuffer)
{
	DBG_LOG_IN();
	
	int CompNum;
	
	CompNum = (int)(intptr_t)pAppData;
    OMX_ERRORTYPE err = OMX_ErrorNone;
	
    /*printf("buffer fill %i\n", (uint32_t)get_ms(&gl_previous_ms));
	if ((uint32_t)get_ms(&gl_previous_ms) > 200) printf("slowly\n");
	gl_previous_ms = 0;
	get_ms(&gl_previous_ms);*/
	
   // m_oCompList[Comp].current_out_buffer = pBuffer;
	dbgprintf(8,"buffer fill %i\n", CompNum);
	
	if (m_oCompList[CompNum].psem)
		OMX_FillThisBuffer(m_oCompList[CompNum].handle, pBuffer);
		else dbgprintf(2, "CallBackFill: Critical warning %i\n", CompNum);
		
	if (m_oCompList[CompNum].psem)
		tx_semaphore_go(m_oCompList[CompNum].psem, OMX_EventFillBuffer, 0);
		else dbgprintf(2, "CallBackFill: Critical warning %i\n", CompNum);
	DBG_LOG_OUT();
	return err;
}

static OMX_ERRORTYPE CallBackFillSync(
    OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, 
    OMX_OUT OMX_BUFFERHEADERTYPE *pBuffer)
{
	DBG_LOG_IN();
	
	int CompNum, ret;
	CompNum = (int)(intptr_t)pAppData;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    //printf("!!!!!!!!! %i %i\n", (unsigned int)FromOMXTime(pBuffer->nTimeStamp), pBuffer->nTickCount);
			
   // m_oCompList[Comp].current_out_buffer = pBuffer;
    //debug_
	dbgprintf(8,"buffer fill sync %i\n", CompNum);
	
	if (pBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
		tx_eventer_send_event(&pevntRMS, MEDIA_EVENT_AUDIO_SYNC);
	if (tx_semaphore_exist_in_list(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY))
	{
		tx_semaphore_add_in_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);
		tx_semaphore_go(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY);
		ret = tx_semaphore_wait_event_timeout(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY, 100);
		if (ret == 0) 
		{
			tx_semaphore_delete_from_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);	
			dbgprintf(8,"timeout sync from fill\n");
		}
		else dbgprintf(8,"ok sync from fill %i\n", ret);
	}
	else dbgprintf(8,"skip sync from fill\n");
	
	//if (tx_semaphore_exist_in_list(&psem_omx_sync, OMX_EVENT_STOP_VIDEO, TX_ANY) == 0)	
	if (m_oCompList[CompNum].psem)
		OMX_FillThisBuffer(m_oCompList[CompNum].handle, pBuffer);
		else dbgprintf(2, "CallBackFillSync: Critical warning\n");
	
		//else dbgprintf(3,"skip OMX_FillThisBuffer\n");
		
	if (m_oCompList[CompNum].psem)
		tx_semaphore_go(m_oCompList[CompNum].psem, OMX_EventFillBuffer, 0);
		else dbgprintf(2, "CallBackFillSync: Critical warning %i\n", CompNum);
	
	DBG_LOG_OUT();
	return err;
}

static OMX_ERRORTYPE CallBackFillWait(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE *pBuffer)
{
	DBG_LOG_IN();
	
	int CompNum;
	CompNum = (int)(intptr_t)pAppData;
    OMX_ERRORTYPE err = OMX_ErrorNone;    
   // m_oCompList[Comp].current_out_buffer = pBuffer;
    dbgprintf(8,"buffer fill wait %i, %i\n", CompNum, (unsigned int)(intptr_t)(pBuffer->pAppPrivate));
	
	//printf("buffer fill wait %i, %i\n", CompNum, (unsigned int)(pBuffer->pAppPrivate));
	//omx_print_wait_list(CompNum);
    //OMX_FillThisBuffer(m_oCompList[CompNum].handle, pBuffer);	
    if (m_oCompList[CompNum].psem)
		tx_semaphore_go(m_oCompList[CompNum].psem, OMX_EventFillBuffer, (unsigned int)(intptr_t)(pBuffer->pAppPrivate));
		else dbgprintf(2, "CallBackFillWait: Critical warning %i\n", CompNum);
	DBG_LOG_OUT();	
	return err;
}

static OMX_ERRORTYPE CallBackEvent(
    OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, 
    OMX_OUT OMX_EVENTTYPE eEvent, OMX_OUT OMX_U32 Data1, 
    OMX_OUT OMX_U32 Data2, OMX_IN OMX_PTR pEventData)
{
    DBG_LOG_IN();
	
	int Comp;
	Comp = (int)(intptr_t)pAppData;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    //OMX_PARAM_PORTDEFINITIONTYPE param;

    //dbgprintf(8,"Hi there, I am in the %s callback, comp:%i\n", __func__, Comp);
	if (m_oCompList[Comp].psem == 0)
	{
		DBG_LOG_OUT();
		return err;
	}
		
	dbgprintf(8,"EVENT\n");
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            dbgprintf(8,"(%i) State changed to ", Comp);
            switch ((int)Data2)
            {
                case OMX_StateInvalid:
                    dbgprintf(8,"OMX_StateInvalid\n");
                    break;
                case OMX_StateLoaded:
                    dbgprintf(8,"OMX_StateLoaded\n");
                    break;
                case OMX_StateIdle:
                    dbgprintf(8,"OMX_StateIdle\n");
                    break;
                case OMX_StateExecuting:
                    dbgprintf(8,"OMX_StateExecuting\n");
                    break;
                case OMX_StatePause:
                    dbgprintf(8,"OMX_StatePause\n");
                    break;
                case OMX_StateWaitForResources:
                    dbgprintf(8,"OMX_StateWaitForResources\n");
                    break;
				default:
					dbgprintf(8,"unknown OMX_CommandStateSet\n");
					break;
            }
            tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);
        }
        else if (Data1 == OMX_CommandPortEnable)
        {
            dbgprintf(8,"OMX_CommandPortEnable %i, %i\n", Comp, Data1);
            tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);
        }
        else if (Data1 == OMX_CommandPortDisable)
        {
            dbgprintf(8,"OMX_CommandPortDisable %i, %i\n", Comp, Data1);
            tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);
        }
        else if (Data1 == OMX_CommandFlush)
        {
            dbgprintf(8,"OMX_CommandFlush %i, %i\n", Comp, Data1);
            tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);
        }
        else
		{
			dbgprintf(8,"OMX_EventCmdComplete %i, %i, Data2:%i\n", Comp, Data1, (int)Data2);
		}
    }
    else if (eEvent == OMX_EventPortSettingsChanged)
    {
        dbgprintf(8,"OMX_EventPortSettingsChanged %i\n", Comp );
        //g_AppMsg = MSG_PORT_SETTING_CHANGED;
        tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        dbgprintf(8,"In %i OMX_EventBufferFlag\n", Comp);
       // if ((int)Data2 == OMX_BUFFERFLAG_EOS)
        {
            //g_AppMsg = MSG_EOF;
            tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);
        }
    }
    else if (eEvent == OMX_EventParamOrConfigChanged)
    {
        dbgprintf(8,"In %i OMX_EventParamOrConfigChanged, %i\n", Comp, Data1);
        tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);        
    }
    else if (eEvent == OMX_EventMax)
    {
        dbgprintf(8,"In %s OMX_EventMax\n", __func__);
        tx_semaphore_go(m_oCompList[Comp].psem, eEvent, Data1);        
    }
    else
    {
        dbgprintf(2,"Param1 is %i (%s)\n", (int)Data1, omx_err2str(Data1));
        dbgprintf(3,"Param2 is %i %x\n", (int)Data2, (int)Data2);
        dbgprintf(3,"Params is undefined\n");
		//omx_err2str(Data1);
        tx_semaphore_go(m_oCompList[Comp].psem, eEvent, 123456789); 
    }
    
	DBG_LOG_OUT();
	return err;
}

/*static void say(const char* message, ...) {
    va_list args;
    char str[1024];
    memset(str, 0, sizeof(str));
    va_start(args, message);
    vsnprintf(str, sizeof(str) - 1, message, args);
    va_end(args);
    size_t str_len = strnlen(str, sizeof(str));
    if(str[str_len - 1] != '\n') {
        str[str_len] = '\n';
    }
    fprintf(stderr, str);
}*/
/*
static void die(const char* message, ...) {
    va_list args;
    char str[1024];
    memset(str, 0, sizeof(str));
    va_start(args, message);
    vsnprintf(str, sizeof(str), message, args);
    va_end(args);
    say(str);
    exit(1);
}*/
/*
static void omx_die(OMX_ERRORTYPE error, const char* message, ...) {
    va_list args;
    char str[1024];
    char *e;
    memset(str, 0, sizeof(str));
    va_start(args, message);
    vsnprintf(str, sizeof(str), message, args);
    va_end(args);
    switch(error) {
        case OMX_ErrorNone:                     e = "no error";                                      break;
        case OMX_ErrorBadParameter:             e = "bad parameter";                                 break;
        case OMX_ErrorIncorrectStateOperation:  e = "invalid state while trying to perform command"; break;
        case OMX_ErrorIncorrectStateTransition: e = "unallowed state transition";                    break;
        case OMX_ErrorInsufficientResources:    e = "insufficient resource";                         break;
        case OMX_ErrorBadPortIndex:             e = "bad port index, i.e. incorrect port";           break;
        case OMX_ErrorHardware:                 e = "hardware error";                                break;
        // That's all I've encountered during hacking so let's not bother with the rest... 
        default:                                e = "(no description)";
    }
    die("OMX error: %s: 0x%08x %s", str, error, e);
}*/

unsigned int GetOmxProfile(unsigned int uiVal)
{
	unsigned int ret = 0;
	switch(uiVal)
	{
		case 1: ret = OMX_VIDEO_AVCProfileBaseline;	break;
		case 2: ret = OMX_VIDEO_AVCProfileMain;		break;
		case 3: ret = OMX_VIDEO_AVCProfileExtended;	break;
		case 4: ret = OMX_VIDEO_AVCProfileHigh;		break;
		case 5: ret = OMX_VIDEO_AVCProfileHigh10;	break;
		case 6: ret = OMX_VIDEO_AVCProfileHigh422;	break;
		case 7: ret = OMX_VIDEO_AVCProfileHigh444;	break;
		default: ret = 0; 							break;
	}
	return ret;
}

unsigned int GetOmxProfileNum(unsigned int uiVal)
{
	unsigned int ret = 0;
	switch(uiVal)
	{
		case OMX_VIDEO_AVCProfileBaseline: 	ret = 1;	break;
		case OMX_VIDEO_AVCProfileMain: 		ret = 2;	break;
		case OMX_VIDEO_AVCProfileExtended: 	ret = 3;	break;
		case OMX_VIDEO_AVCProfileHigh: 		ret = 4;	break;
		case OMX_VIDEO_AVCProfileHigh10: 	ret = 5;	break;
		case OMX_VIDEO_AVCProfileHigh422: 	ret = 6;	break;
		case OMX_VIDEO_AVCProfileHigh444: 	ret = 7;	break;
		default: ret = 0; 								break;
	}
	return ret;
}

unsigned int GetOmxLevelNum(unsigned int uiVal)
{
	unsigned int ret = 0;
	switch(uiVal)
	{
		case OMX_VIDEO_AVCLevel1: 	ret = 1;	break;
		case OMX_VIDEO_AVCLevel1b:	ret = 2;	break;
		case OMX_VIDEO_AVCLevel11: 	ret = 3;	break;
		case OMX_VIDEO_AVCLevel12: 	ret = 4;	break;
		case OMX_VIDEO_AVCLevel13: 	ret = 5;	break;
		case OMX_VIDEO_AVCLevel2: 	ret = 6;	break;
		case OMX_VIDEO_AVCLevel21: 	ret = 7;	break;
		case OMX_VIDEO_AVCLevel22: 	ret = 8;	break;
		case OMX_VIDEO_AVCLevel3: 	ret = 9;	break;
		case OMX_VIDEO_AVCLevel31: 	ret = 10;	break;
		case OMX_VIDEO_AVCLevel32: 	ret = 11;	break;
		case OMX_VIDEO_AVCLevel4: 	ret = 12;	break;
		case OMX_VIDEO_AVCLevel41: 	ret = 13;	break;
		case OMX_VIDEO_AVCLevel42: 	ret = 14;	break;
		case OMX_VIDEO_AVCLevel5: 	ret = 15;	break;
		case OMX_VIDEO_AVCLevel51: 	ret = 16;	break;
		default: ret = 0; 						break;
	}
	return ret;
}

char* GetOmxProfileName(unsigned int uiVal)
{
	char* ret = "UNKNOWN";
	switch(uiVal)
	{
		case OMX_VIDEO_AVCProfileBaseline: 	ret = "OMX_VIDEO_AVCProfileBaseline";	break;
		case OMX_VIDEO_AVCProfileMain: 		ret = "OMX_VIDEO_AVCProfileMain";		break;
		case OMX_VIDEO_AVCProfileExtended: 	ret = "OMX_VIDEO_AVCProfileExtended";	break;
		case OMX_VIDEO_AVCProfileHigh: 		ret = "OMX_VIDEO_AVCProfileHigh";		break;
		case OMX_VIDEO_AVCProfileHigh10: 	ret = "OMX_VIDEO_AVCProfileHigh10";		break;
		case OMX_VIDEO_AVCProfileHigh422: 	ret = "OMX_VIDEO_AVCProfileHigh422";	break;
		case OMX_VIDEO_AVCProfileHigh444: 	ret = "OMX_VIDEO_AVCProfileHigh444";	break;
		default: ret = "UNKNOWN";						break;
	}
	return ret;
}

char* GetH264ProfileName(unsigned int uiVal)
{
	char* ret = "Main";
	switch(uiVal)
	{
		case OMX_VIDEO_AVCProfileBaseline: 	ret = "Baseline";	break;
		case OMX_VIDEO_AVCProfileMain: 		ret = "Main";		break;
		case OMX_VIDEO_AVCProfileExtended: 	ret = "Extended";	break;
		case OMX_VIDEO_AVCProfileHigh: 		ret = "High";		break;
		case OMX_VIDEO_AVCProfileHigh10: 	ret = "High";		break;
		case OMX_VIDEO_AVCProfileHigh422: 	ret = "High";		break;
		case OMX_VIDEO_AVCProfileHigh444: 	ret = "High";		break;
		default: ret = "Main";						break;
	}
	return ret;
}

int GetH264ProfileCode(char *cName)
{
	if (strcasecmp(cName, "Baseline") == 0) return OMX_VIDEO_AVCProfileBaseline;
	if (strcasecmp(cName, "Main") == 0) return OMX_VIDEO_AVCProfileMain;
	if (strcasecmp(cName, "Extended") == 0) return OMX_VIDEO_AVCProfileExtended;
	if (strcasecmp(cName, "High") == 0) return OMX_VIDEO_AVCProfileHigh;
	return OMX_VIDEO_AVCProfileBaseline;
}

unsigned int GetOmxLevel(unsigned int uiVal)
{
	unsigned int ret = 0;
	switch(uiVal)
	{
		case 1: ret = OMX_VIDEO_AVCLevel1;	break;
		case 2: ret = OMX_VIDEO_AVCLevel1b;	break;
		case 3: ret = OMX_VIDEO_AVCLevel11;	break;
		case 4: ret = OMX_VIDEO_AVCLevel12;	break;
		case 5: ret = OMX_VIDEO_AVCLevel13;	break;
		case 6: ret = OMX_VIDEO_AVCLevel2;	break;
		case 7: ret = OMX_VIDEO_AVCLevel21;	break;
		case 8: ret = OMX_VIDEO_AVCLevel22;	break;
		case 9: ret = OMX_VIDEO_AVCLevel3;	break;
		case 10: ret = OMX_VIDEO_AVCLevel31;break;
		case 11: ret = OMX_VIDEO_AVCLevel32;break;
		case 12: ret = OMX_VIDEO_AVCLevel4;	break;
		case 13: ret = OMX_VIDEO_AVCLevel41;break;
		case 14: ret = OMX_VIDEO_AVCLevel42;break;
		case 15: ret = OMX_VIDEO_AVCLevel5;	break;
		case 16: ret = OMX_VIDEO_AVCLevel51;break;
		default: ret = 0; 					break;
	}
	return ret;
}

char* GetOmxLevelName(unsigned int uiVal)
{
	char* ret = "UNKNOWN";
	switch(uiVal)
	{
		case OMX_VIDEO_AVCLevel1: 	ret = "OMX_VIDEO_AVCLevel1";	break;
		case OMX_VIDEO_AVCLevel1b: 	ret = "OMX_VIDEO_AVCLevel1b";	break;
		case OMX_VIDEO_AVCLevel11: 	ret = "OMX_VIDEO_AVCLevel11";	break;
		case OMX_VIDEO_AVCLevel12: 	ret = "OMX_VIDEO_AVCLevel12";	break;
		case OMX_VIDEO_AVCLevel13: 	ret = "OMX_VIDEO_AVCLevel13";	break;
		case OMX_VIDEO_AVCLevel2: 	ret = "OMX_VIDEO_AVCLevel2";	break;
		case OMX_VIDEO_AVCLevel21: 	ret = "OMX_VIDEO_AVCLevel21";	break;
		case OMX_VIDEO_AVCLevel22: 	ret = "OMX_VIDEO_AVCLevel22";	break;
		case OMX_VIDEO_AVCLevel3: 	ret = "OMX_VIDEO_AVCLevel3";	break;
		case OMX_VIDEO_AVCLevel31: 	ret = "OMX_VIDEO_AVCLevel31";	break;
		case OMX_VIDEO_AVCLevel32: 	ret = "OMX_VIDEO_AVCLevel32";	break;
		case OMX_VIDEO_AVCLevel4: 	ret = "OMX_VIDEO_AVCLevel4";	break;
		case OMX_VIDEO_AVCLevel41: 	ret = "OMX_VIDEO_AVCLevel41";	break;
		case OMX_VIDEO_AVCLevel42: 	ret = "OMX_VIDEO_AVCLevel42";	break;
		case OMX_VIDEO_AVCLevel5: 	ret = "OMX_VIDEO_AVCLevel5";	break;
		case OMX_VIDEO_AVCLevel51: 	ret = "OMX_VIDEO_AVCLevel51";	break;
		default: 					ret = "UNKNOWN";				break;
	}
	return ret;
}

char* GetNameExposure(unsigned int uiData)
{
	char* ret = "Unknown";
	switch(uiData)
	{
		case OMX_ExposureControlOff:				ret = "Off";				break;
		case OMX_ExposureControlAuto:				ret = "Auto";				break;
		case OMX_ExposureControlNight:				ret = "Night";				break;
		case OMX_ExposureControlBackLight:			ret = "BackLight";			break;
		case OMX_ExposureControlSpotLight:			ret = "SpotLight";			break;
		case OMX_ExposureControlSports:				ret = "Sports";				break;
		case OMX_ExposureControlSnow:				ret = "Snow";				break;
		case OMX_ExposureControlBeach:				ret = "Beach";				break;
		case OMX_ExposureControlLargeAperture:		ret = "LargeAperture";		break;
		case OMX_ExposureControlSmallAperture:		ret = "SmallAperture";		break;
		case OMX_ExposureControlVeryLong:			ret = "VeryLong";			break;
		case OMX_ExposureControlFixedFps:			ret = "FixedFps";			break;
		case OMX_ExposureControlNightWithPreview:	ret = "NightWithPreview";	break;
		case OMX_ExposureControlAntishake:			ret = "Antishake";			break;
		case OMX_ExposureControlFireworks:			ret = "Fireworks";			break;
		default:									ret = "Unknown";			break;
	}
	return ret;
}

char* GetShortNameExposure(unsigned int uiData)
{
	char* ret = "Unk";
	switch(uiData)
	{
		case OMX_ExposureControlOff:				ret = "Off";			break;
		case OMX_ExposureControlAuto:				ret = "Aut";			break;
		case OMX_ExposureControlNight:				ret = "Ngt";			break;
		case OMX_ExposureControlBackLight:			ret = "BkL";			break;
		case OMX_ExposureControlSpotLight:			ret = "SpL";			break;
		case OMX_ExposureControlSports:				ret = "Spt";			break;
		case OMX_ExposureControlSnow:				ret = "Snw";			break;
		case OMX_ExposureControlBeach:				ret = "Bch";			break;
		case OMX_ExposureControlLargeAperture:		ret = "LrA";			break;
		case OMX_ExposureControlSmallAperture:		ret = "SmA";			break;
		case OMX_ExposureControlVeryLong:			ret = "VrL";			break;
		case OMX_ExposureControlFixedFps:			ret = "FxF";			break;
		case OMX_ExposureControlNightWithPreview:	ret = "NgP";			break;
		case OMX_ExposureControlAntishake:			ret = "Ant";			break;
		case OMX_ExposureControlFireworks:			ret = "FrW";			break;
		default:									ret = "Unk";			break;
	}
	return ret;
}

unsigned int GetStandartExposure(unsigned int uiData)
{
	unsigned int ret = OMX_ExposureControlOff;
	switch(uiData)
	{
		case OMX_ExposureControlOff: 				ret = OMX_ExposureControlOff; break;
		case OMX_ExposureControlAuto: 				ret = OMX_ExposureControlAuto; break;
		case OMX_ExposureControlNight: 				ret = OMX_ExposureControlNight; break;
		case OMX_ExposureControlBackLight: 			ret = OMX_ExposureControlBackLight; break;
		case OMX_ExposureControlSpotLight: 			ret = OMX_ExposureControlSpotLight; break;
		case OMX_ExposureControlSports: 			ret = OMX_ExposureControlSports; break;
		case OMX_ExposureControlSnow: 				ret = OMX_ExposureControlSnow; break;
		case OMX_ExposureControlBeach: 				ret = OMX_ExposureControlBeach; break;
		case OMX_ExposureControlLargeAperture: 		ret = OMX_ExposureControlLargeAperture; break;
		case OMX_ExposureControlSmallAperture: 		ret = OMX_ExposureControlSmallAperture; break;
		case OMX_ExposureControlVeryLong: 			ret = OMX_ExposureControlVeryLong; break;
		case OMX_ExposureControlFixedFps: 			ret = OMX_ExposureControlFixedFps; break;
		case OMX_ExposureControlNightWithPreview: 	ret = OMX_ExposureControlNightWithPreview; break;
		case OMX_ExposureControlAntishake: 			ret = OMX_ExposureControlAntishake; break;
		case OMX_ExposureControlFireworks: 			ret = OMX_ExposureControlFireworks; break;
		
		case EXPOSURE_OFF: 							ret = OMX_ExposureControlOff; break;
		case EXPOSURE_AUTO: 						ret = OMX_ExposureControlAuto; break;
		case EXPOSURE_NIGHT: 						ret = OMX_ExposureControlNight; break;
		case EXPOSURE_BACKLIGHT: 					ret = OMX_ExposureControlBackLight; break;
		case EXPOSURE_SPOTLIGHT: 					ret = OMX_ExposureControlSpotLight; break;
		case EXPOSURE_SPORTS: 						ret = OMX_ExposureControlSports; break;
		case EXPOSURE_SNOW: 						ret = OMX_ExposureControlSnow; break;
		case EXPOSURE_BEACH: 						ret = OMX_ExposureControlBeach; break;
		case EXPOSURE_LARGEAPERTURE: 				ret = OMX_ExposureControlLargeAperture; break;
		case EXPOSURE_SMALLAPERTURE: 				ret = OMX_ExposureControlSmallAperture; break;
		case EXPOSURE_VERYLONG: 					ret = OMX_ExposureControlVeryLong; break;
		case EXPOSURE_FIXEDFPS: 					ret = OMX_ExposureControlFixedFps; break;
		case EXPOSURE_NIGHTWITHPREVIEW: 			ret = OMX_ExposureControlNightWithPreview; break;
		case EXPOSURE_ANTISHAKE: 					ret = OMX_ExposureControlAntishake; break;
		case EXPOSURE_FIREWORKS: 					ret = OMX_ExposureControlFireworks; break;
		
		default: 									ret = OMX_ExposureControlAuto;	break;
	}
	return ret;
}

char* GetCodeNameExposure(unsigned int uiData)
{
	switch(uiData)
	{
		case OMX_ExposureControlOff: 				return "EXPOSURE_OFF";
		case OMX_ExposureControlAuto: 				return "EXPOSURE_AUTO";
		case OMX_ExposureControlNight: 				return "EXPOSURE_NIGHT";
		case OMX_ExposureControlBackLight: 			return "EXPOSURE_BACKLIGHT";
		case OMX_ExposureControlSpotLight: 			return "EXPOSURE_SPOTLIGHT";
		case OMX_ExposureControlSports: 			return "EXPOSURE_SPORTS";
		case OMX_ExposureControlSnow: 				return "EXPOSURE_SNOW";
		case OMX_ExposureControlBeach: 				return "EXPOSURE_BEACH";
		case OMX_ExposureControlLargeAperture: 		return "EXPOSURE_LARGEAPERTURE";
		case OMX_ExposureControlSmallAperture: 		return "EXPOSURE_SMALLAPERTURE";
		case OMX_ExposureControlVeryLong: 			return "EXPOSURE_VERYLONG";
		case OMX_ExposureControlFixedFps: 			return "EXPOSURE_FIXEDFPS";
		case OMX_ExposureControlNightWithPreview: 	return "EXPOSURE_NIGHTWITHPREVIEW";
		case OMX_ExposureControlAntishake: 			return "EXPOSURE_ANTISHAKE";
		case OMX_ExposureControlFireworks: 			return "EXPOSURE_FIREWORKS";
		
		case EXPOSURE_OFF: 							return "EXPOSURE_OFF";
		case EXPOSURE_AUTO: 						return "EXPOSURE_AUTO";
		case EXPOSURE_NIGHT: 						return "EXPOSURE_NIGHT";
		case EXPOSURE_BACKLIGHT: 					return "EXPOSURE_BACKLIGHT";
		case EXPOSURE_SPOTLIGHT: 					return "EXPOSURE_SPOTLIGHT";
		case EXPOSURE_SPORTS: 						return "EXPOSURE_SPORTS";
		case EXPOSURE_SNOW: 						return "EXPOSURE_SNOW";
		case EXPOSURE_BEACH: 						return "EXPOSURE_BEACH";
		case EXPOSURE_LARGEAPERTURE: 				return "EXPOSURE_LARGEAPERTURE";
		case EXPOSURE_SMALLAPERTURE: 				return "EXPOSURE_SMALLAPERTURE";
		case EXPOSURE_VERYLONG: 					return "EXPOSURE_VERYLONG";
		case EXPOSURE_FIXEDFPS: 					return "EXPOSURE_FIXEDFPS";
		case EXPOSURE_NIGHTWITHPREVIEW: 			return "EXPOSURE_NIGHTWITHPREVIEW";
		case EXPOSURE_ANTISHAKE: 					return "EXPOSURE_ANTISHAKE";
		case EXPOSURE_FIREWORKS: 					return "EXPOSURE_FIREWORKS";
		
		default: 									return "EXPOSURE_OFF";
	}
	return "EXPOSURE_OFF";
}

char* GetNameImageFilter(unsigned int uiData)
{
	char* ret = "Unknown";
	switch(uiData)
	{
		case OMX_ImageFilterNone: 					ret = "None"; break;
		case OMX_ImageFilterNoise: 					ret = "Noise"; break;
		case OMX_ImageFilterEmboss:					ret = "Emboss";	break;
		case OMX_ImageFilterNegative:				ret = "Negative";	break;
		case OMX_ImageFilterSketch:					ret = "Sketch";	break;
		case OMX_ImageFilterOilPaint:				ret = "OilPaint";	break;
		case OMX_ImageFilterHatch:					ret = "Hatch";	break;
		case OMX_ImageFilterGpen:					ret = "Gpen";	break;
		case OMX_ImageFilterAntialias:				ret = "Antialias";	break;
		case OMX_ImageFilterDeRing:					ret = "DeRing";	break;
		case OMX_ImageFilterSolarize:				ret = "Solarize";	break;
		case OMX_ImageFilterWatercolor:				ret = "Watercolor";	break;
		case OMX_ImageFilterPastel:					ret = "Pastel";	break;
		case OMX_ImageFilterSharpen:				ret = "Sharpen";	break;
		case OMX_ImageFilterFilm:					ret = "Film";	break;
		case OMX_ImageFilterBlur:					ret = "Blur";	break;
		case OMX_ImageFilterSaturation:				ret = "Saturation";	break;
		case OMX_ImageFilterDeInterlaceLineDouble:	ret = "DeInterlaceLineDouble";	break;
		case OMX_ImageFilterDeInterlaceAdvanced:	ret = "DeInterlaceAdvanced";	break;
		case OMX_ImageFilterColourSwap:				ret = "ColourSwap";	break;
		case OMX_ImageFilterWashedOut:				ret = "WashedOut";	break;
		case OMX_ImageFilterColourPoint:			ret = "ColourPoint";	break;
		case OMX_ImageFilterPosterise:				ret = "Posterise";	break;
		case OMX_ImageFilterColourBalance:			ret = "ColourBalance";	break;
		case OMX_ImageFilterCartoon:				ret = "Cartoon";	break;
		case OMX_ImageFilterAnaglyph:				ret = "Anaglyph";	break;
		case OMX_ImageFilterDeInterlaceFast:		ret = "DeInterlaceFast";	break;
		default:									ret = "Unknown";	break;
	}
	return ret;
}

unsigned int GetStandartImageFilter(unsigned int uiData)
{
	unsigned int ret = OMX_ImageFilterNone;
	switch(uiData)
	{
		case OMX_ImageFilterNone: 				ret = OMX_ImageFilterNone; break;
		case OMX_ImageFilterNoise: 				ret = OMX_ImageFilterNoise; break;
		case OMX_ImageFilterEmboss: 			ret = OMX_ImageFilterEmboss; break;
		case OMX_ImageFilterNegative: 			ret = OMX_ImageFilterNegative; break;
		case OMX_ImageFilterSketch: 			ret = OMX_ImageFilterSketch; break;
		case OMX_ImageFilterOilPaint: 			ret = OMX_ImageFilterOilPaint; break;
		case OMX_ImageFilterHatch: 				ret = OMX_ImageFilterHatch; break;
		case OMX_ImageFilterGpen: 				ret = OMX_ImageFilterGpen; break;
		case OMX_ImageFilterAntialias: 			ret = OMX_ImageFilterAntialias; break;
		case OMX_ImageFilterDeRing: 			ret = OMX_ImageFilterDeRing; break;
		case OMX_ImageFilterSolarize: 			ret = OMX_ImageFilterSolarize; break;
		case OMX_ImageFilterWatercolor: 		ret = OMX_ImageFilterWatercolor; break;
		case OMX_ImageFilterPastel: 			ret = OMX_ImageFilterPastel; break;
		case OMX_ImageFilterSharpen: 			ret = OMX_ImageFilterSharpen; break;
		case OMX_ImageFilterFilm: 				ret = OMX_ImageFilterFilm; break;
		case OMX_ImageFilterBlur: 				ret = OMX_ImageFilterBlur; break;
		case OMX_ImageFilterSaturation: 			ret = OMX_ImageFilterSaturation; break;
		case OMX_ImageFilterDeInterlaceLineDouble: 	ret = OMX_ImageFilterDeInterlaceLineDouble; break;
		case OMX_ImageFilterDeInterlaceAdvanced: 	ret = OMX_ImageFilterDeInterlaceAdvanced; break;
		case OMX_ImageFilterColourSwap: 			ret = OMX_ImageFilterColourSwap; break;
		case OMX_ImageFilterWashedOut: 			ret = OMX_ImageFilterWashedOut; break;
		case OMX_ImageFilterColourPoint: 		ret = OMX_ImageFilterColourPoint; break;
		case OMX_ImageFilterPosterise: 			ret = OMX_ImageFilterPosterise; break;
		case OMX_ImageFilterColourBalance: 		ret = OMX_ImageFilterColourBalance; break;
		case OMX_ImageFilterCartoon: 			ret = OMX_ImageFilterCartoon; break;
		case OMX_ImageFilterAnaglyph: 			ret = OMX_ImageFilterAnaglyph; break;
		case OMX_ImageFilterDeInterlaceFast: 	ret = OMX_ImageFilterDeInterlaceFast; break;
		
		case FILTER_NONE: 					ret = OMX_ImageFilterNone; break;
		case FILTER_NOISE: 					ret = OMX_ImageFilterNoise; break;
		case FILTER_EMBOSS: 				ret = OMX_ImageFilterEmboss; break;
		case FILTER_NEGATIVE: 				ret = OMX_ImageFilterNegative; break;
		case FILTER_SKETCH: 				ret = OMX_ImageFilterSketch; break;
		case FILTER_OILPAINT: 				ret = OMX_ImageFilterOilPaint; break;
		case FILTER_HATCH: 					ret = OMX_ImageFilterHatch; break;
		case FILTER_GPEN: 					ret = OMX_ImageFilterGpen; break;
		case FILTER_ANTIALIAS: 				ret = OMX_ImageFilterAntialias; break;
		case FILTER_DERING: 				ret = OMX_ImageFilterDeRing; break;
		case FILTER_SOLARISE: 				ret = OMX_ImageFilterSolarize; break;
		case FILTER_WATERCOLOR: 			ret = OMX_ImageFilterWatercolor; break;
		case FILTER_PASTEL: 				ret = OMX_ImageFilterPastel; break;
		case FILTER_SHARPEN: 				ret = OMX_ImageFilterSharpen; break;
		case FILTER_FILM: 					ret = OMX_ImageFilterFilm; break;
		case FILTER_BLUR: 					ret = OMX_ImageFilterBlur; break;
		case FILTER_SATURATION: 			ret = OMX_ImageFilterSaturation; break;
		case FILTER_DEINTERLACELINEDOUBLE: 	ret = OMX_ImageFilterDeInterlaceLineDouble; break;
		case FILTER_DEINTERLACEADVANCED: 	ret = OMX_ImageFilterDeInterlaceAdvanced; break;
		case FILTER_COLOURSWAP: 			ret = OMX_ImageFilterColourSwap; break;
		case FILTER_WASHEDOUT: 				ret = OMX_ImageFilterWashedOut; break;
		case FILTER_COLOURPOINT: 			ret = OMX_ImageFilterColourPoint; break;
		case FILTER_POSTERIZE: 				ret = OMX_ImageFilterPosterise; break;
		case FILTER_COLOURBALANCE: 			ret = OMX_ImageFilterColourBalance; break;
		case FILTER_CARTOON: 				ret = OMX_ImageFilterCartoon; break;
		case FILTER_ANAGLYPH: 				ret = OMX_ImageFilterAnaglyph; break;
		case FILTER_DEINTERLACEFAST: 		ret = OMX_ImageFilterDeInterlaceFast; break;
		
		default: 							ret = OMX_ImageFilterNone;	break;
	}
	return ret;
}

char* GetCodeNameImageFilter(unsigned int uiData)
{
	switch(uiData)
	{
		case OMX_ImageFilterNone: 				return "FILTER_NONE";
		case OMX_ImageFilterNoise: 				return "FILTER_NOISE";
		case OMX_ImageFilterEmboss: 			return "FILTER_EMBOSS";
		case OMX_ImageFilterNegative: 			return "FILTER_NEGATIVE";
		case OMX_ImageFilterSketch: 			return "FILTER_SKETCH";
		case OMX_ImageFilterOilPaint: 			return "FILTER_OILPAINT";
		case OMX_ImageFilterHatch: 				return "FILTER_HATCH";
		case OMX_ImageFilterGpen: 				return "FILTER_GPEN";
		case OMX_ImageFilterAntialias: 			return "FILTER_ANTIALIAS";
		case OMX_ImageFilterDeRing: 			return "FILTER_DERING";
		case OMX_ImageFilterSolarize: 			return "FILTER_SOLARISE";
		case OMX_ImageFilterWatercolor: 		return "FILTER_WATERCOLOR";
		case OMX_ImageFilterPastel: 			return "FILTER_PASTEL";
		case OMX_ImageFilterSharpen: 			return "FILTER_SHARPEN";
		case OMX_ImageFilterFilm: 				return "FILTER_FILM";
		case OMX_ImageFilterBlur: 				return "FILTER_BLUR";
		case OMX_ImageFilterSaturation: 			return "FILTER_SATURATION";
		case OMX_ImageFilterDeInterlaceLineDouble: 	return "FILTER_DEINTERLACELINEDOUBLE";
		case OMX_ImageFilterDeInterlaceAdvanced: 	return "FILTER_DEINTERLACEADVANCED";
		case OMX_ImageFilterColourSwap: 			return "FILTER_COLOURSWAP";
		case OMX_ImageFilterWashedOut: 			return "FILTER_WASHEDOUT";
		case OMX_ImageFilterColourPoint: 		return "FILTER_COLOURPOINT";
		case OMX_ImageFilterPosterise: 			return "FILTER_POSTERIZE";
		case OMX_ImageFilterColourBalance: 		return "FILTER_COLOURBALANCE";
		case OMX_ImageFilterCartoon: 			return "FILTER_CARTOON";
		case OMX_ImageFilterAnaglyph: 			return "FILTER_ANAGLYPH";
		case OMX_ImageFilterDeInterlaceFast: 	return "FILTER_DEINTERLACEFAST";
		
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
		
		default: 							return "FILTER_NONE";
	}
	return "FILTER_NONE";
}

char* GetNameWhiteBalance(unsigned int uiData)
{
	char* ret = "Unknown";
	switch(uiData)
	{
		case OMX_WhiteBalControlOff: 				ret = "Off"; break;
		case OMX_WhiteBalControlAuto: 				ret = "Auto"; break;
		case OMX_WhiteBalControlSunLight:			ret = "SunLight";	break;
		case OMX_WhiteBalControlCloudy:				ret = "Cloudy";	break;
		case OMX_WhiteBalControlShade:				ret = "Shade";	break;
		case OMX_WhiteBalControlTungsten:			ret = "Tungsten";	break;
		case OMX_WhiteBalControlFluorescent:		ret = "Fluorescent";	break;
		case OMX_WhiteBalControlIncandescent:		ret = "Incandescent";	break;
		case OMX_WhiteBalControlFlash:				ret = "Flash";	break;
		case OMX_WhiteBalControlHorizon:			ret = "Horizon";	break;
		default:									ret = "Unknown";	break;
	}
	return ret;
}

char* GetCodeNameWhiteBalance(unsigned int uiData)
{
	switch(uiData)
	{
		case OMX_WhiteBalControlOff: 				return "WBALANCE_OFF";
		case OMX_WhiteBalControlAuto: 				return "WBALANCE_AUTO";
		case OMX_WhiteBalControlSunLight: 			return "WBALANCE_SUNLIGHT";
		case OMX_WhiteBalControlCloudy: 			return "WBALANCE_CLOUDY";
		case OMX_WhiteBalControlShade: 				return "WBALANCE_SHADE";
		case OMX_WhiteBalControlTungsten: 			return "WBALANCE_TUNGSTEN";
		case OMX_WhiteBalControlFluorescent: 		return "WBALANCE_FLUORESCENT";
		case OMX_WhiteBalControlIncandescent: 		return "WBALANCE_INCANDESCENT";
		case OMX_WhiteBalControlFlash: 				return "WBALANCE_FLASH";
		case OMX_WhiteBalControlHorizon: 			return "WBALANCE_HORIZON";
		
		case WBALANCE_OFF: 							return "WBALANCE_OFF";
		case WBALANCE_AUTO: 						return "WBALANCE_AUTO";
		case WBALANCE_SUNLIGHT: 					return "WBALANCE_SUNLIGHT";
		case WBALANCE_CLOUDY: 						return "WBALANCE_CLOUDY";
		case WBALANCE_SHADE: 						return "WBALANCE_SHADE";
		case WBALANCE_TUNGSTEN: 					return "WBALANCE_TUNGSTEN";
		case WBALANCE_FLUORESCENT: 					return "WBALANCE_FLUORESCENT";
		case WBALANCE_INCANDESCENT: 				return "WBALANCE_INCANDESCENT";
		case WBALANCE_FLASH: 						return "WBALANCE_FLASH";
		case WBALANCE_HORIZON: 						return "WBALANCE_HORIZON";
		
		default: 									return "WBALANCE_OFF";
	}
	return "WBALANCE_OFF";
}				


unsigned int GetStandartWhiteBalance(unsigned int uiData)
{
	unsigned int ret = OMX_WhiteBalControlAuto;
	switch(uiData)
	{
		case OMX_WhiteBalControlOff: 				ret = OMX_WhiteBalControlOff; break;
		case OMX_WhiteBalControlAuto: 				ret = OMX_WhiteBalControlAuto; break;
		case OMX_WhiteBalControlSunLight: 			ret = OMX_WhiteBalControlSunLight; break;
		case OMX_WhiteBalControlCloudy: 			ret = OMX_WhiteBalControlCloudy; break;
		case OMX_WhiteBalControlShade: 				ret = OMX_WhiteBalControlShade; break;
		case OMX_WhiteBalControlTungsten: 			ret = OMX_WhiteBalControlTungsten; break;
		case OMX_WhiteBalControlFluorescent: 		ret = OMX_WhiteBalControlFluorescent; break;
		case OMX_WhiteBalControlIncandescent: 		ret = OMX_WhiteBalControlIncandescent; break;
		case OMX_WhiteBalControlFlash: 				ret = OMX_WhiteBalControlFlash; break;
		case OMX_WhiteBalControlHorizon: 			ret = OMX_WhiteBalControlHorizon; break;
		
		case WBALANCE_OFF: 							ret = OMX_WhiteBalControlOff; break;
		case WBALANCE_AUTO: 						ret = OMX_WhiteBalControlAuto; break;
		case WBALANCE_SUNLIGHT: 					ret = OMX_WhiteBalControlSunLight; break;
		case WBALANCE_CLOUDY: 						ret = OMX_WhiteBalControlCloudy; break;
		case WBALANCE_SHADE: 						ret = OMX_WhiteBalControlShade; break;
		case WBALANCE_TUNGSTEN: 					ret = OMX_WhiteBalControlTungsten; break;
		case WBALANCE_FLUORESCENT: 					ret = OMX_WhiteBalControlFluorescent; break;
		case WBALANCE_INCANDESCENT: 				ret = OMX_WhiteBalControlIncandescent; break;
		case WBALANCE_FLASH: 						ret = OMX_WhiteBalControlFlash; break;
		case WBALANCE_HORIZON: 						ret = OMX_WhiteBalControlHorizon; break;
		
		default: 									ret = OMX_WhiteBalControlAuto;	break;
	}
	return ret;
}

void GetResolutionFromMode(int iMode, int* pW, int *pH)
{
	switch(iMode)
	{
		case RESOLUTION_TYPE_160X120:
			*pW = 160;
			*pH = 120;			
			break;
		case RESOLUTION_TYPE_320X240:
			*pW = 320;
			*pH = 240;			
			break;
		case RESOLUTION_TYPE_480X240:
			*pW = 480;
			*pH = 320;			
			break;
		case RESOLUTION_TYPE_640X480:
			*pW = 640;
			*pH = 480;			
			break;
		case RESOLUTION_TYPE_800X600:
			*pW = 800;
			*pH = 600;			
			break;
		case RESOLUTION_TYPE_1024X768:
			*pW = 1024;
			*pH = 768;			
			break;
		case RESOLUTION_TYPE_1280X960:
			*pW = 1280;
			*pH = 960;			
			break;
		case RESOLUTION_TYPE_1400X1050:
			*pW = 1400;
			*pH = 1050;			
			break;
		case RESOLUTION_TYPE_1600X1200:
			*pW = 1600;
			*pH = 1200;			
			break;
		case RESOLUTION_TYPE_2048X1536:
			*pW = 2048;
			*pH = 1536;			
			break;
		case RESOLUTION_TYPE_2592X1944:
			*pW = 2592;
			*pH = 1944;			
			break;
		default:
			*pW = 160;
			*pH = 120;
			break;
	}
}

void GetResolutionFromModule(int iSettings, int* pW, int *pH)
{
	if (iSettings < RESOLUTION_TYPE_MAX)
	{
		GetResolutionFromMode(iSettings, pW, pH);
		return;
	}
	
	*pW = (iSettings >> 16) & 0xFFFF;
	*pH = iSettings & 0xFFFF;	
}

int omx_ptz_get_focus_position(int iMinPos, int iMaxPos, int iZoom, double coefficient)
{
	double res;
	double result[7];
	
	if (iZoom < 1150)
	{
		result[0] = 0.00000000000024753002f * pow(iZoom,5);
		result[1] = 0.00000000058924190246f * pow(iZoom,4);
		result[2] = 0.00000044658664046409f * pow(iZoom,3);
		result[3] = 0.000195932410886704f * pow(iZoom,2);
		result[4] = 0.0878508528439463f * iZoom;
		res = -result[0] +result[1] -result[2] +result[3] +result[4] - 241.538470564908f;
		if (coefficient == 1.0f) res += 150;
	}
	else
	{
		result[0] = coefficient * 0.00000000000000674248f + 0.00000000000000628764f;
		result[1] = coefficient * 0.00000000006666166163f + 0.00000000004835326127f;
		result[2] = coefficient * 0.00000026792194617678f + 0.00000015421311031157f;
		result[3] = coefficient * 0.000563455736523909f + 0.000261108876261332f;
		result[4] = coefficient * 0.656181667705985f + 0.247294173933842f;
		result[5] = coefficient * 402.123458379113f + 124.399166725486f;
		result[6] = coefficient * 101473.0278843946f + 26126.7340117534f;
		res = -result[0]*pow(iZoom, 6)+result[1]*pow(iZoom, 5)-result[2]*pow(iZoom, 4)+result[3]*pow(iZoom, 3)-result[4]*pow(iZoom, 2)+result[5]*iZoom-result[6];
	}
	
	int ret = (int)res;
	if (ret < iMinPos) ret = iMinPos;
	if (ret > iMaxPos) ret = iMaxPos;
	
	return ret;
}

double omx_ptz_get_coefficient(int iZoom, int iFocus)
{
	double res;
	double result[7];
	
	result[0] = pow(iZoom, 6) * 0.00000000000000628764f;
	result[1] = pow(iZoom, 5) * 0.00000000004835326127f;
	result[2] = pow(iZoom, 4) * 0.00000015421311031157f;
	result[3] = pow(iZoom, 3) * 0.000261108876261332f;
	result[4] = pow(iZoom, 2) * 0.247294173933842f;
	result[5] = iZoom * 124.399166725486f;
	result[6] = iFocus + result[0] - result[1] + result[2] - result[3] + result[4] - result[5] + 26126.7340117534f;
	
	result[0] = pow(iZoom, 6) * 0.00000000000000674248f;
	result[1] = pow(iZoom, 5) * 0.00000000006666166163f;
	result[2] = pow(iZoom, 4) * 0.00000026792194617678f;
	result[3] = pow(iZoom, 3) * 0.000563455736523909f;
	result[4] = pow(iZoom, 2) * 0.656181667705985f;
	result[5] = iZoom * 402.123458379113f;
		
	res = result[6] / (-result[0] +result[1] -result[2] +result[3] -result[4] +result[5] -101473.0278843946f);
	
	return res;
}

void omx_Save_Autofocus(omx_autofocus_params *autofocus_pm, int iZoom)
{
	if (autofocus_pm->PtzFocusMapEnabled)
	{
		iZoom += 300;		
		if ((iZoom < 0) || (iZoom > 2299)) iZoom = 0;
		if (iZoom == 2300) iZoom = 2299;
		
		int iPtzFocusMapVal = (autofocus_pm->PtzFocusMap[iZoom] & 0xFFF) - 300;
		
		if (iPtzFocusMapVal != autofocus_pm->CurrentFocusPos)
		{
			//dbgprintf(2, "Sett focus TO mem zoom:%i focus:%i %i\n", iZoom, iPtzFocusMapVal, autofocus_pm->CurrentFocusPos);
			autofocus_pm->PtzFocusMap[iZoom] = (autofocus_pm->CurrentFocusPos + 300) & 0xFFF;
			autofocus_pm->PtzFocusMap[iZoom] |= 0x1000;
			autofocus_pm->PtzFocusMapChanged = 1;

			int iPos = iZoom - 1;
			for (;iPos >= 0; iPos--) if (autofocus_pm->PtzFocusMap[iPos] & 0x1000) break;
			if (iPos >= 0)
			{
				int iVal1 = autofocus_pm->PtzFocusMap[iZoom] & 0xFFF;
				int iVal2 = autofocus_pm->PtzFocusMap[iPos] & 0xFFF;
				//printf("Detect left val on %i (%i) from %i (%i)\n", iPos, iVal2, iZoom, iVal1);
				int iDiff = (iVal1 > iVal2) ? iVal1 - iVal2 : iVal2 - iVal1;
				float fStepSize = (float)(iDiff) / (iZoom - iPos);
				float fNewFocus = iVal2 + fStepSize;
				for (iPos++; iPos < iZoom; iPos++)
				{
					autofocus_pm->PtzFocusMap[iPos] = (int)fNewFocus & 0xFFF;
					//printf("Set val on %i: %i\n", iPos, autofocus_pm->PtzFocusMap[iPos]);
					autofocus_pm->PtzFocusMap[iPos] |= 0x2000;
					fNewFocus += fStepSize;
				}
			}
			
			iPos = iZoom + 1;
			for (;iPos < 2299; iPos++) if (autofocus_pm->PtzFocusMap[iPos] & 0x1000) break;
			if (iPos < 2299)
			{
				int iVal1 = autofocus_pm->PtzFocusMap[iZoom] & 0xFFF;
				int iVal2 = autofocus_pm->PtzFocusMap[iPos] & 0xFFF;
				//printf("Detect right val on %i (%i) from %i (%i)\n", iPos, iVal2, iZoom, iVal1);
				int iDiff = (iVal1 > iVal2) ? iVal1 - iVal2 : iVal2 - iVal1;
				float fStepSize = (float)(iDiff) / (iZoom - iPos);
				float fNewFocus = iVal2 + fStepSize;
				for (iPos--; iPos > iZoom; iPos--)
				{
					autofocus_pm->PtzFocusMap[iPos] = (int)fNewFocus & 0xFFF;
					//printf("Set val on %i: %i\n", iPos, autofocus_pm->PtzFocusMap[iPos]);
					autofocus_pm->PtzFocusMap[iPos] |= 0x2000;
					fNewFocus += fStepSize;
				}
			}				
		}		
	}
}

void omx_Stop_Autofocus(omx_autofocus_params *autofocus_pm)
{
	//printf("Stop AF\n");
	autofocus_pm->Direct = 0;
	autofocus_pm->Mode = 0;
	autofocus_pm->Status = 0;
	autofocus_pm->FullScanCounter = 0;
	autofocus_pm->ScanCnt[0] = 0;
	autofocus_pm->ScanCnt[1] = 0;
	autofocus_pm->ScanCnt[2] = 0;
	autofocus_pm->ScanCnt[3] = 0;
	autofocus_pm->MaxContrastVal[0] = 0;
	autofocus_pm->MaxContrastVal[1] = 0;
	autofocus_pm->MaxContrastVal[2] = 0;
	autofocus_pm->MaxContrastVal[3] = 0;
	autofocus_pm->LastContrast[0] = 0;
	autofocus_pm->LastContrast[1] = 0;
	autofocus_pm->LastContrast[2] = 0;
	autofocus_pm->LastContrast[3] = 0;
	autofocus_pm->MaxContrastPos[0] = -100000;
	autofocus_pm->MaxContrastPos[1] = -100000;
	autofocus_pm->MaxContrastPos[2] = -100000;
	autofocus_pm->MaxContrastPos[3] = -100000;
	autofocus_pm->SearchStep = PTZ_START_STEP_SIZE;
	autofocus_pm->CurrentFocusPos = -100000;
}

int omx_PrintFocusMapFilled(omx_autofocus_params *autofocus_pm)
{
	if ((autofocus_pm->PtzFocusMapEnabled) && autofocus_pm->PtzFocusMap)
	{
		int i;
		int iCnt = 0;
		int iCnt2 = 0;
		for (i = 0; i < 2299; i++)
		{
			if (autofocus_pm->PtzFocusMap[i] & 0x1000) iCnt++;
			if (autofocus_pm->PtzFocusMap[i] & 0x3000) iCnt2++;
		}
		iCnt /= 23;
		iCnt2 /= 23;
		dbgprintf(3, "Focus MAP filled:%i, preset:%i\n", iCnt, iCnt2);
	}
	return 0;
}

int omx_SetFocusFromMap(omx_autofocus_params *autofocus_pm, int iPtzModuleNum, int *iPtzStatuses, char cForce)
{
	if ((autofocus_pm->PtzFocusMapEnabled) && (iPtzModuleNum >= 0))
	{
		int iZoom = iPtzStatuses[4] + 300;			
		if ((iZoom < 0) || (iZoom >= 2300)) iZoom = 0;
		if (iZoom == 2300) iZoom = 2299;
		int iPtzFocusMapVal = (autofocus_pm->PtzFocusMap[iZoom] & 0xFFF) - 300;
		int iPtzFocusMapSet = autofocus_pm->PtzFocusMap[iZoom] & 0x1000;
		int iPtzFocusMapPreSet = autofocus_pm->PtzFocusMap[iZoom] & 0x2000;
		if ((iPtzFocusMapPreSet || iPtzFocusMapSet) && (autofocus_pm->CurrentFocusPos != -100000))
		{
			if (iPtzFocusMapVal != autofocus_pm->CurrentFocusPos)
			{
				//dbgprintf(4, "Sett focus from mem zoom:%i focus:%i\n", iZoom, iPtzFocusMapVal);
				if (cForce)
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);
					usb_gpio_set_focus(&miModuleList[iPtzModuleNum], iPtzFocusMapVal, 1, &autofocus_pm->CurrentFocusPos);
					DBG_MUTEX_UNLOCK(&modulelist_mutex);				
				}
				else
				{
					AddModuleEventInList(autofocus_pm->PtzID, PTZ_TARGET_ABSLT_RAW_FOCUS, iPtzFocusMapVal, NULL, 0, 1);	
					//AddModuleEventInList(autofocus_pm->PtzID, PTZ_TARGET_SET_SPEED_FOCUS, 100000, NULL, 0, 1);					
				}
			}
			if (iPtzFocusMapSet) return 1; else return 2;			
		}
		else dbgprintf(4, "Not sett focus FROM mem zoom:%i\n", iZoom);
	}
	return 0;
}

void omx_Start_Autofocus(omx_autofocus_params *autofocus_pm, int iDirect, int iPtzModuleNum, int *iPtzStatuses)
{
	int iSearchStep = PTZ_START_STEP_SIZE;
	if ((autofocus_pm->PtzFocusMapEnabled) && (iPtzModuleNum >= 0))
	{
		int res = omx_SetFocusFromMap(autofocus_pm, iPtzModuleNum, iPtzStatuses, 0);
		if (res == 1)
		{
			omx_Stop_Autofocus(autofocus_pm);
			return;
		}
		if (res == 2) iSearchStep = 3;
	}
	
	if (autofocus_pm->Status) return;
	//printf("Start AF\n");
	if (iDirect >= 0) autofocus_pm->Direct = iDirect;
	autofocus_pm->FullScanCounter = 0;
	autofocus_pm->Status = 1;	
	autofocus_pm->Mode = 0;
	autofocus_pm->ScanCnt[0] = 0;
	autofocus_pm->ScanCnt[1] = 0;
	autofocus_pm->ScanCnt[2] = 0;
	autofocus_pm->ScanCnt[3] = 0;
	autofocus_pm->MaxContrastVal[0] = 0;
	autofocus_pm->MaxContrastVal[1] = 0;
	autofocus_pm->MaxContrastVal[2] = 0;
	autofocus_pm->MaxContrastVal[3] = 0;
	autofocus_pm->LastContrast[0] = 0;
	autofocus_pm->LastContrast[1] = 0;
	autofocus_pm->LastContrast[2] = 0;
	autofocus_pm->LastContrast[3] = 0;
	autofocus_pm->MaxContrastPos[0] = -100000;
	autofocus_pm->MaxContrastPos[1] = -100000;
	autofocus_pm->MaxContrastPos[2] = -100000;
	autofocus_pm->MaxContrastPos[3] = -100000;
	autofocus_pm->SearchStep = iSearchStep;
	autofocus_pm->CurrentFocusPos = -100000;
}

void omx_SetFocusIndicator(omx_autofocus_params *autofocus_pm, 
									VideoCodecInfo *tVideoInfo, 
									unsigned char* pBuffer, int iCurrFocus, int iCurrZoom)
{	
	int i, n;
	unsigned int uiCurrPosX, uiMiddPosX;
	unsigned int uiLastPosY = autofocus_pm->SensorPosY + autofocus_pm->SensorHeight;
	unsigned int uiLastPosX = autofocus_pm->SensorPosX + autofocus_pm->SensorWidth;
	unsigned int uiFocusLimitMin, uiFocusLimitMax;
	iCurrFocus /= 1000;
	iCurrZoom /= 1000;
	uiMiddPosX = (uiLastPosX - autofocus_pm->SensorPosX) / 2 - 50;
	
	if (autofocus_pm->FullMaxFocusPos > autofocus_pm->FullMinFocusPos)
	{
		uiFocusLimitMin = (float)(autofocus_pm->MinFocusPos - autofocus_pm->FullMinFocusPos) / ((float)(autofocus_pm->FullMaxFocusPos - autofocus_pm->FullMinFocusPos)/100);
		uiFocusLimitMax = (float)(autofocus_pm->MaxFocusPos - autofocus_pm->FullMinFocusPos) / ((float)(autofocus_pm->FullMaxFocusPos - autofocus_pm->FullMinFocusPos)/100);
	}
	else
	{
		uiFocusLimitMin = 0;
		uiFocusLimitMax = 100;
	}
	
	//printf("@@ %i %i\n", uiFocusLimitMin, uiFocusLimitMax);
	unsigned char ucConturSize = 2;
	for (i = autofocus_pm->SensorPosY; i < uiLastPosY; i++)
	{
		uiCurrPosX = i * tVideoInfo->video_width32;
				
		//FOCUS
		if (i == autofocus_pm->SensorPosY + 8)
			for (n = uiMiddPosX; n <= (uiMiddPosX+100); n++) pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + n] = n & ucConturSize ? 255 : 0;
		if (i < (autofocus_pm->SensorPosY + 8))
		{
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX] = i & ucConturSize ? 255 : 0;
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX+100] = i & ucConturSize ? 255 : 0;
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX + iCurrFocus] = i & ucConturSize ? 255 : 0;
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX + uiFocusLimitMin] = i & ucConturSize ? 255 : 0;
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX + uiFocusLimitMax] = i & ucConturSize ? 255 : 0;
		}			
		//ZOOM
		if (i == (uiLastPosY - 8))
			for (n = uiMiddPosX; n <= (uiMiddPosX+100); n++) pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + n] = n & ucConturSize ? 255 : 0;	
		if (i > (uiLastPosY - 8))
		{
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX] = i & ucConturSize ? 255 : 0;
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX+100] = i & ucConturSize ? 255 : 0;
			pBuffer[uiCurrPosX + autofocus_pm->SensorPosX + uiMiddPosX + iCurrZoom] = i & ucConturSize ? 255 : 0;
		}
		//SQUARE
		if ((i == autofocus_pm->SensorPosY) || (i == (uiLastPosY-1)))
		{			
			for (n = autofocus_pm->SensorPosX; n < uiLastPosX; n++) pBuffer[uiCurrPosX + n] = n & ucConturSize ? 255 : 0;
		}
		pBuffer[uiCurrPosX + autofocus_pm->SensorPosX] = i & ucConturSize ? 255 : 0;
		pBuffer[uiCurrPosX + uiLastPosX] = i & ucConturSize ? 255 : 0;
	}
}

unsigned int omx_GetSquareContrast(omx_autofocus_params *autofocus_pm, 
									VideoCodecInfo *tVideoInfo, 
									unsigned char* pBuffer)
{
	unsigned int result = 0;
	
	int i, n;
	int res;
	unsigned char pCurrXY, pPrevX, pCurrY;
	unsigned int uiCurrPosX, uiPrevPosY;
	unsigned int uiLastPosY = autofocus_pm->SensorPosY + autofocus_pm->SensorHeight;
	unsigned int uiLastPosX = autofocus_pm->SensorPosX + autofocus_pm->SensorWidth;	
	
	for (i = autofocus_pm->SensorPosY; i < uiLastPosY; i+=1)
	{
		for (n = autofocus_pm->SensorPosX; n < uiLastPosX; n+=1)
		{
			uiCurrPosX = i * tVideoInfo->video_width32 + n;
			uiPrevPosY = (i-1) * tVideoInfo->video_width32 + n;
			pPrevX = pBuffer[uiCurrPosX - 1];
			pCurrXY = pBuffer[uiCurrPosX];
			pCurrY = pBuffer[uiPrevPosY];
			res = abs(pPrevX - pCurrXY);
			res *= res;
			result += res;
			//if (res > result) result = res;
			res = abs(pCurrY - pCurrXY);
			res *= res;
			result += res;
			//if (res > result) result = res;
			//pBuffer[uiCurrPosX] = 0; //WARNING
		}
	}
	//printf("AF    %i\n", result);
	return result;
}

unsigned int omx_GetContrast(unsigned int SensorWidth32, 
									unsigned int SensorWidth, 
									unsigned int SensorHeight,
									unsigned char* pBuffer)
{
	unsigned int result = 0;
	
	int i, n;
	int res;
	unsigned char pCurrXY, pPrevX, pCurrY;
	unsigned int uiCurrPosX, uiPrevPosY;
	
	
	for (i = 1; i < SensorHeight; i+=1)
	{
		for (n = 1; n < SensorWidth; n+=1)
		{
			uiCurrPosX = i * SensorWidth32 + n;
			uiPrevPosY = (i-1) * SensorWidth32 + n;
			pPrevX = pBuffer[uiCurrPosX - 1];
			pCurrXY = pBuffer[uiCurrPosX];
			pCurrY = pBuffer[uiPrevPosY];
			res = abs(pPrevX - pCurrXY);
			res *= res;
			result += res;
			//if (res > result) result = res;
			res = abs(pCurrY - pCurrXY);
			res *= res;
			result += res;
			//if (res > result) result = res;
			//pBuffer[uiCurrPosX] = 0; //WARNING
		}
	}
	//printf("AS    %i\n", result);
	return result;
}


void RecalcRectangles(int iCnt, RECT_INFO *riList, int iMW, int iMH, int iSW, int iSH)
{
	int res;
	for (res = 0; res < iCnt; res++)
	{
		riList[res].BX1 = (int)(((float)(iMW) / 100) * riList[res].X1);
		riList[res].BX2 = (int)(((float)(iMW) / 100) * riList[res].X2);
		riList[res].BY1 = (int)(((float)(iMH) / 100) * riList[res].Y1);
		riList[res].BY2 = (int)(((float)(iMH) / 100) * riList[res].Y2);
		riList[res].PX1 = (int)(((float)(iSW) / 100) * riList[res].X1);
		riList[res].PX2 = (int)(((float)(iSW) / 100) * riList[res].X2);
		riList[res].PY1 = (int)(((float)(iSH) / 100) * riList[res].Y1);
		riList[res].PY2 = (int)(((float)(iSH) / 100) * riList[res].Y2);	

		if (riList[res].BX2 >= iMW) riList[res].BX2 = iMW - 1;
		if (riList[res].BY2 >= iMH) riList[res].BY2 = iMH - 1;
		if (riList[res].PX2 >= iSW) riList[res].PX2 = iSW - 1;
		if (riList[res].PY2 >= iSH) riList[res].PY2 = iSH - 1;
	}
}

void CopyRectList(int iSrcCnt, RECT_INFO *SrcList, int *iDestCnt, RECT_INFO **DestList)
{
	DBG_MUTEX_LOCK(&rectangle_mutex);
	if ((*iDestCnt) != 0)
	{
		DBG_FREE(*DestList);
		*DestList = NULL;
		*iDestCnt = 0;
	}
	if (iSrcCnt != 0)
	{
		*DestList = (RECT_INFO*)DBG_MALLOC(sizeof(RECT_INFO) * iSrcCnt);
		memcpy(*DestList, SrcList, sizeof(RECT_INFO) * iSrcCnt);
		*iDestCnt = iSrcCnt;
	}
	DBG_MUTEX_UNLOCK(&rectangle_mutex);
}

int GetRectNum(int iCnt, RECT_INFO *riList, int iSizeW, int iCurPos)
{
	int iPosY = (int)floor(iCurPos/iSizeW);
	int iPosX = iCurPos - (iSizeW * iPosY);
	int n;
	int ret = 0;
	for(n = 0; n < iCnt; n++)
	{
		if ((riList[n].Enabled) && 
			(riList[n].PX1 <= iPosX) && (riList[n].PX2 >= iPosX) &&
			(riList[n].PY1 <= iPosY) && (riList[n].PY2 >= iPosY))
			{
				ret = riList[n].Group;
				break;
			}
	}
	return ret;
}

int MarkRects(int iCnt, RECT_INFO *riList, int iSizeW, int iCurPos, char cType)
{
	int iPosY = (int)floor(iCurPos/iSizeW);
	int iPosX = iCurPos - (iSizeW * iPosY);
	int n;
	for(n = 0; n < iCnt; n++)
	{
		if ((riList[n].Enabled) &&
			(riList[n].PX1 <= iPosX) && (riList[n].PX2 >= iPosX) &&
			(riList[n].PY1 <= iPosY) && (riList[n].PY2 >= iPosY))
			{
				switch (cType)
				{
					case 0:
						if (riList[n].Detected == 0) riList[n].Detected = 1;
						riList[n].DetectSize++;
						break;
					case 1:
						if (riList[n].ColorDetected == 0) riList[n].ColorDetected = 1;
						riList[n].ColorDetectSize++;
						break;
					case 2:
						if (riList[n].LongDetected == 0) riList[n].LongDetected = 1;
						riList[n].LongDetectSize++;
						break;
					case 3:
						if (riList[n].LongColorDetected == 0) riList[n].LongColorDetected = 1;
						riList[n].LongColorDetectSize++;
						break;
					default: 
						break;
				}
			}
	}
	return 1;
}

int ShowRectOnBuffer(int iCnt, RECT_INFO *riList, int iSizeW, char *pBuffer, int iBuffSize)
{
	int n, i, b;
	for(n = 0; n < iCnt; n++)
	{
		if (riList[n].Enabled &&
			(riList[n].BX2 > riList[n].BX1) &&
			(riList[n].BY2 > riList[n].BY1))
		{			
			i = riList[n].BY1 * iSizeW + riList[n].BX1;
			b = i + (riList[n].BX2 - riList[n].BX1);
			for (; i < b; i++) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
			i = riList[n].BY1 * iSizeW + riList[n].BX1;
			b = riList[n].BY2 * iSizeW + riList[n].BX1;
			for (; i < b; i += iSizeW) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
			i = riList[n].BY1 * iSizeW + riList[n].BX2;
			b = riList[n].BY2 * iSizeW + riList[n].BX2;
			for (; i < b; i += iSizeW) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
			i = riList[n].BY2 * iSizeW + riList[n].BX1;
			b = i + (riList[n].BX2 - riList[n].BX1);
			for (; i < b; i++) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
		}
	}
	return 1;
}

int ShowRectOnBufferPrev(int iCnt, RECT_INFO *riList, int iSizeW, char *pBuffer, int iBuffSize)
{
	int n, i, b;
	unsigned char res;
	
	for(n = 0; n < iCnt; n++)
	{
		if (riList[n].Enabled &&
			(riList[n].PX2 > riList[n].PX1) &&
			(riList[n].PY2 > riList[n].PY1))
		{
			res = 0;
			if (riList[n].Detected) res = 128;
			if (riList[n].ColorDetected) res = 255;
				
			i = riList[n].PY1 * iSizeW + riList[n].PX1;
			b = i + (riList[n].PX2 - riList[n].PX1);
			for (; i < b; i++) if (i < iBuffSize) { if (res == 0) pBuffer[i] ^= 255; else pBuffer[i] = res;} else break;
			i = riList[n].PY1 * iSizeW + riList[n].PX1;
			b = riList[n].PY2 * iSizeW + riList[n].PX1;
			for (; i < b; i += iSizeW) if (i < iBuffSize) { if (res == 0) pBuffer[i] ^= 255; else pBuffer[i] = res;} else break;
			i = riList[n].PY1 * iSizeW + riList[n].PX2;
			b = riList[n].PY2 * iSizeW + riList[n].PX2;
			for (; i < b; i += iSizeW) if (i < iBuffSize) { if (res == 0) pBuffer[i] ^= 255; else pBuffer[i] = res;} else break;
			i = riList[n].PY2 * iSizeW + riList[n].PX1;
			b = i + (riList[n].PX2 - riList[n].PX1);
			for (; i < b; i++) if (i < iBuffSize) { if (res == 0) pBuffer[i] ^= 255; else pBuffer[i] = res;} else break;
		}
	}
	return 1;
}

int ShowAllRectOnBufferPrev(int iCnt, RECT_INFO *riList, int iSizeW, char *pBuffer, int iBuffSize)
{
	int n, i, b;
	
	for(n = 0; n < iCnt; n++)
	{
		if (riList[n].Enabled &&
			(riList[n].PX2 > riList[n].PX1) &&
			(riList[n].PY2 > riList[n].PY1))
		{
			i = riList[n].PY1 * iSizeW + riList[n].PX1;
			b = i + (riList[n].PX2 - riList[n].PX1);
			for (; i < b; i++) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
			i = riList[n].PY1 * iSizeW + riList[n].PX1;
			b = riList[n].PY2 * iSizeW + riList[n].PX1;
			for (; i < b; i += iSizeW) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
			i = riList[n].PY1 * iSizeW + riList[n].PX2;
			b = riList[n].PY2 * iSizeW + riList[n].PX2;
			for (; i < b; i += iSizeW) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
			i = riList[n].PY2 * iSizeW + riList[n].PX1;
			b = i + (riList[n].PX2 - riList[n].PX1);
			for (; i < b; i++) if (i < iBuffSize) pBuffer[i] ^= 255; else break;
		}
	}
	return 1;
}

int ShowCurrRectOnBuffer(int iCnt, RECT_INFO *riList, int iSizeW, char *pBuffer, int iBuffSize, int iNum)
{
	int n, x, y;
	if (iCnt && (iNum < iCnt))
	{
		x = 0;
		y = 0;
		for(n = 0; n < iBuffSize; n++)
		{			
			if ((riList[iNum].PX2 >= riList[iNum].PX1) &&
					(riList[iNum].PY2 >= riList[iNum].PY1) && 
					(riList[iNum].PX1 <= x) && (x <= riList[iNum].PX2) &&
					(riList[iNum].PY1 <= y) && (y <= riList[iNum].PY2))
				{
					pBuffer[n] ^= 255; 
				}
			x++;
			if (x == iSizeW) {y++; x = 0;}
		}
	}
	return 1;
}

void text_draw_string(uint8_t *texture, uint8_t *canvas, int canvas_width, int canvas_height, unsigned char *cString, int iStrLen) 
{
	int pen_x = 0;
	int pen_y = 0;
	int row, col, charnum;
	int char_width = 16; 
	unsigned int width = 1626;
	unsigned int height = 22;	
	unsigned int bytes_per_pixel = 4;
	for (charnum = 0; charnum != iStrLen; charnum++) 
	{		
		if (cString[charnum] < 33) 
		{
			pen_x += char_width;
			continue;
		}
		for (col = (cString[charnum] - 32) * char_width; col < ((cString[charnum] - 32) * char_width + char_width); col++) 
		{
			if (pen_x < 0) 
			{
				pen_x++;
				continue;
			}
			if (pen_x >= canvas_width) break; // out of bounds
			for (row = 0; row < height; row++) 
			{
				if (col > width) break;
						
				if (pen_y + row < 0) continue;
				if (pen_y + row >= canvas_height) break; // out of bounds
						
				int offset = (row * width + col) * bytes_per_pixel;
				uint8_t opacity = texture[offset];
				uint8_t red = texture[offset + 1];
						
				// TODO: map colors to UV
				//uint8_t green = textdata->bitmap[offset + 2];
				//uint8_t blue = textdata->bitmap[offset + 3];
				if (opacity == 255) 
				{			
					canvas[(pen_y + row) * canvas_width + pen_x] = red;					
				} 
				else 			
				{
					if (opacity > 0) 
					{			
						// Blend colors
						uint8_t orig_color = canvas[(pen_y + row) * canvas_width + pen_x];
						float intensity = opacity / 255.0f;
						canvas[(pen_y + row) * canvas_width + pen_x] =
								orig_color * (1 - intensity) + red * intensity;						
					}			
				}						
			}
			pen_x++;
		}
	}
}

void text_draw_timestamp(uint8_t *texture, uint8_t *canvas, int canvas_width, int canvas_height, int iTemp, unsigned int uiExposure, int iFrameNum) 
{
	time_t rawtime;
	struct tm timeinfo;
	char str[64];

	time(&rawtime);

	localtime_r(&rawtime, &timeinfo);
	memset(str, 0, 64);
	if ((iTemp > -10000) && (iTemp < 10000)) sprintf(str, "%02i.%02i.%04i  %02i:%02i:%02i  t:%.1f`C  %s %i", 
													timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 
													(double)iTemp/10, GetShortNameExposure(uiExposure), iFrameNum);
	if (iTemp == -10000) sprintf(str, "%02i.%02i.%04i  %02i:%02i:%02i  %s %i", 
								timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 
								GetShortNameExposure(uiExposure), iFrameNum);
	if (iTemp == 10000) sprintf(str, "%02i.%02i.%04i  %02i:%02i:%02i  t:N/A %s %i", 
								timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 
								GetShortNameExposure(uiExposure), iFrameNum);
	str[63] = 0;			
	
	text_draw_string(texture, canvas, canvas_width, canvas_height, (unsigned char *)str, strlen(str)); 
}

void omx_GetComponentName(int ComponentID, char *Name, int MaxLen)
{
	DBG_LOG_IN();
	
	char buff[256];
	memset(Name, 0, MaxLen);
	memset(buff, 0, 256);
	switch(ComponentID)
	{
		case OMX_COMP_AUDIO_CAPTURE:
				strcpy(buff,"OMX.broadcom.audio_capture");
				break;
		case OMX_COMP_AUDIO_DECODE:
				strcpy(buff,"OMX.broadcom.audio_decode");
				break;
		case OMX_COMP_AUDIO_ENCODE:
				strcpy(buff,"OMX.broadcom.audio_encode");
				break;
		case OMX_COMP_AUDIO_LOWPOWER:
				strcpy(buff,"OMX.broadcom.audio_lowpower");
				break;
		case OMX_COMP_AUDIO_MIXER:
				strcpy(buff,"OMX.broadcom.audio_mixer");
				break;
		case OMX_COMP_AUDIO_PROCESSOR:
				strcpy(buff,"OMX.broadcom.audio_processor");
				break;
		case OMX_COMP_AUDIO_RENDER:
				strcpy(buff,"OMX.broadcom.audio_render");
				break;
		case OMX_COMP_AUDIO_SPLITTER:
				strcpy(buff,"OMX.broadcom.audio_splitter");
				break;
		case OMX_COMP_IMAGE_DECODE:
				strcpy(buff,"OMX.broadcom.image_decode");
				break;
		case OMX_COMP_IMAGE_ENCODE:
				strcpy(buff,"OMX.broadcom.image_encode");
				break;
		case OMX_COMP_IMAGE_FX:
				strcpy(buff,"OMX.broadcom.image_fx");
				break;
		case OMX_COMP_IMAGE_READ:
				strcpy(buff,"OMX.broadcom.image_read");
				break;
		case OMX_COMP_IMAGE_WRITE:
				strcpy(buff,"OMX.broadcom.image_write");
				break;
		case OMX_COMP_RESIZE:
				strcpy(buff,"OMX.broadcom.resize");
				break;
		case OMX_COMP_SOURCE:
				strcpy(buff,"OMX.broadcom.source");
				break;
		case OMX_COMP_TRANSITION:
				strcpy(buff,"OMX.broadcom.transition");
				break;
		case OMX_COMP_WRITE_STILL:
				strcpy(buff,"OMX.broadcom.write_still");
				break;
		case OMX_COMP_CLOCK:
				strcpy(buff,"OMX.broadcom.clock");
				break;
		case OMX_COMP_NULL_SINK:
				strcpy(buff,"OMX.broadcom.null_sink");
				break;
		case OMX_COMP_TEXT_SCHEDULER:
				strcpy(buff,"OMX.broadcom.text_scheduler");
				break;
		case OMX_COMP_VISUALISATION:
				strcpy(buff,"OMX.broadcom.visualisation");
				break;
		case OMX_COMP_READ_MEDIA:
				strcpy(buff,"OMX.broadcom.read_media");
				break;
		case OMX_COMP_WRITE_MEDIA:
				strcpy(buff,"OMX.broadcom.write_media");
				break;
		case OMX_COMP_CAMERA:
				strcpy(buff,"OMX.broadcom.camera");
				break;
		case OMX_COMP_EGL_RENDER:
				strcpy(buff,"OMX.broadcom.egl_render");
				break;
		case OMX_COMP_VIDEO_DECODE:
				strcpy(buff,"OMX.broadcom.video_decode");
				break;
		case OMX_COMP_VIDEO_ENCODE:
				strcpy(buff,"OMX.broadcom.video_encode");
				break;
		case OMX_COMP_VIDEO_RENDER:
				strcpy(buff,"OMX.broadcom.video_render");
				break;
		case OMX_COMP_VIDEO_SCHEDULER:
				strcpy(buff,"OMX.broadcom.video_scheduler");
				break;
		case OMX_COMP_VIDEO_SPLITTER:
				strcpy(buff,"OMX.broadcom.video_splitter");
				break;
		default:
			    break;
	}
	if (MaxLen>=256) memcpy(Name, buff, 255);
		else memcpy(Name, buff, MaxLen-1);
	
	DBG_LOG_OUT();	
}

void omx_print_flags(int nFlags)
{
	DBG_LOG_IN();
	
	if (nFlags & OMX_BUFFERFLAG_EOS) 			dbgprintf(3,"OMX_BUFFERFLAG_EOS\n"); 
	if (nFlags & OMX_BUFFERFLAG_STARTTIME)  	dbgprintf(3,"OMX_BUFFERFLAG_STARTTIME\n"); 
	if (nFlags & OMX_BUFFERFLAG_DATACORRUPT)  	dbgprintf(3,"OMX_BUFFERFLAG_DATACORRUPT\n"); 
	if (nFlags & OMX_BUFFERFLAG_DECODEONLY)  	dbgprintf(3,"OMX_BUFFERFLAG_DECODEONLY\n"); 
	if (nFlags & OMX_BUFFERFLAG_ENDOFFRAME)  	dbgprintf(3,"OMX_BUFFERFLAG_ENDOFFRAME\n"); 
	if (nFlags & OMX_BUFFERFLAG_SYNCFRAME)  	dbgprintf(3,"OMX_BUFFERFLAG_SYNCFRAME\n"); 
	if (nFlags & OMX_BUFFERFLAG_EXTRADATA)  	dbgprintf(3,"OMX_BUFFERFLAG_EXTRADATA\n"); 
	if (nFlags & OMX_BUFFERFLAG_CODECCONFIG)  	dbgprintf(3,"OMX_BUFFERFLAG_CODECCONFIG\n"); 
	if (nFlags & OMX_BUFFERFLAG_TIMESTAMPINVALID) dbgprintf(3,"OMX_BUFFERFLAG_TIMESTAMPINVALID\n");
	if (nFlags & OMX_BUFFERFLAG_READONLY)  		dbgprintf(3,"OMX_BUFFERFLAG_READONLY\n"); 
	if (nFlags & OMX_BUFFERFLAG_ENDOFSUBFRAME)  dbgprintf(3,"OMX_BUFFERFLAG_ENDOFSUBFRAME\n"); 
	if (nFlags & OMX_BUFFERFLAG_SKIPFRAME)  	dbgprintf(3,"OMX_BUFFERFLAG_SKIPFRAME\n"); 
	
	DBG_LOG_OUT();	
}

/*
static void block_until_state_changed(OMX_HANDLETYPE hComponent, OMX_STATETYPE wanted_eState) 
{
    OMX_STATETYPE eState;
    int i = 0;
    while(i++ == 0 || eState != wanted_eState) 
	{
		OMX_GetState(hComponent, &eState);
        if(eState != wanted_eState) 
		{
            if (i == 1) printf("Cur state:%i>>>%i\n",eState,wanted_eState);
			usleep(10000);
        }
    }
}

static void block_until_port_changed(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BOOL bEnabled) 
{
    OMX_ERRORTYPE r;
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    OMX_INIT_STRUCTURE(portdef);
    portdef.nPortIndex = nPortIndex;
    OMX_U32 i = 0;
    while(i++ == 0 || portdef.bEnabled != bEnabled) 
	{
        if((r = OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &portdef)) != OMX_ErrorNone) 
		{
            omx_die(r, "Failed to get port definition");
        }
        if(portdef.bEnabled != bEnabled) 
		{
            usleep(10000);
        }
    }
}
*/
/*static void block_until_flushed(appctx *ctx) {
    int quit;
    while(!quit) {
        vcos_semaphore_wait(&ctx->handler_lock);
        if(ctx->flushed) {
            ctx->flushed = 0;
            quit = 1;
        }
        vcos_semaphore_post(&ctx->handler_lock);
        if(!quit) {
            usleep(10000);
        }
    }
}*/

int omx_add_cmd_in_list(int CompNum, int event, int cmd)
{
	DBG_LOG_IN();	
	tx_semaphore_add_in_list(m_oCompList[CompNum].psem, event, cmd);
	DBG_LOG_OUT();	
	return 1;
}

int omx_add_cmd_in_list_spec(int CompNum, int event, int cmd, int NotLock)
{
	DBG_LOG_IN();	
	tx_semaphore_add_in_list_spec(m_oCompList[CompNum].psem, event, cmd, NotLock);
	DBG_LOG_OUT();	
	return 1;
}

int omx_print_wait_list(int CompNum)
{
	DBG_LOG_IN();	
	tx_semaphore_print_list(m_oCompList[CompNum].psem);
	DBG_LOG_OUT();	
	return 1;
}

int omx_delete_cmd_from_list(int CompNum, int event, int cmd)
{
	DBG_LOG_IN();	
	tx_semaphore_delete_from_list(m_oCompList[CompNum].psem, event, cmd);
	DBG_LOG_OUT();	
	return 1;
}

int omx_count_cmd_in_list(int CompNum, int event, int cmd)
{
	DBG_LOG_IN();	
	int ret = tx_semaphore_count_in_list(m_oCompList[CompNum].psem, event, cmd);
	DBG_LOG_OUT();	
	return ret;
}

int omx_send_cmd(int CompNum, int cmd, int param) 
{
    DBG_LOG_IN();
	
	if (CompNum>=OMX_COMPONENTS_MAX) {DBG_LOG_OUT();return 0;}
	omx_add_cmd_in_list(CompNum, OMX_EventCmdComplete, cmd);
    OMX_ERRORTYPE err = OMX_SendCommand (m_oCompList[CompNum].handle, cmd, param, NULL);
    if (err != OMX_ErrorNone)
    {
		dbgprintf(2,"error send command:Component:%i, Cmd:%i, Param:%i, ErrorCode:%i\n", CompNum, cmd, param, err);
		DBG_LOG_OUT();
		return -1;
	}	
	//sleep(2);	
    DBG_LOG_OUT();
	return 1;
}

int omx_flush_port (int CompNum, int port, int port_type, int now) 
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
    omx_send_cmd(CompNum, OMX_CommandFlush, port_num);
    if (now == OMX_NOW) tx_semaphore_wait(m_oCompList[CompNum].psem);
    
	DBG_LOG_OUT();
	return 1;
}

int omx_get_state_port(int CompNum, int port, int port_type) 
{
	DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;
	OMX_INIT_STRUCTURE(port_def);
	port_def.nPortIndex = port_num;
    OMX_ERRORTYPE oerr = OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def);
	if (OMX_TEST_ERROR(oerr))
	{
		DBG_LOG_OUT();
		return 0;
	}
	DBG_LOG_OUT();
	return port_def.bEnabled;
}

int omx_disable_port (int CompNum, int port, int port_type, int now) 
{
	DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
	dbgprintf(8,"close port:%i\n", port_num);    
	if (omx_get_state_port(CompNum, port, port_type) == 0) 
	{
		dbgprintf(8,"dont need close port:%i\n", port_num);
		DBG_LOG_OUT();
		return 1;
	}
    //if ((now != OMX_NOW) || (omx_wait_list_empty(CompNum) == 0))
	{
		omx_send_cmd(CompNum, OMX_CommandPortDisable, port_num);
	}
	/*else
	{
		//
		DBG_OERR(OMX_SendCommand (m_oCompList[CompNum].handle, OMX_CommandStateSet, OMX_CommandPortDisable, NULL));
		while (omx_get_state_port(CompNum, port, port_type) != 0) usleep(10000);
	}*/
	if (now == OMX_NOW) tx_semaphore_wait(m_oCompList[CompNum].psem);
	//if (now == OMX_NOW) block_until_port_changed(m_oCompList[CompNum].handle, port_num, OMX_FALSE);    
    DBG_LOG_OUT();
	return 1;
}

int omx_disable_port_no_event(int CompNum, int port, int port_type) 
{
	DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
	dbgprintf(8,"close port:%i\n", port_num);  

	DBG_OERR(OMX_SendCommand (m_oCompList[CompNum].handle, OMX_CommandPortDisable, port_num, NULL));
	while (omx_get_state_port(CompNum, port, port_type) != 0) usleep(30000);
	
	DBG_LOG_OUT();
	return 1;
}

int omx_enable_port(int CompNum, int port, int port_type, int now) 
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
	dbgprintf(8,"open port:%i\n", port_num); 
	if (omx_get_state_port(CompNum, port, port_type) != 0) 
	{
		dbgprintf(8,"dont need open port:%i\n", port_num);
		DBG_LOG_OUT();
		return 1;
	}
    //if ((now != OMX_NOW) || (omx_wait_list_empty(CompNum) == 0))
	{
		omx_send_cmd(CompNum, OMX_CommandPortEnable, port_num);
	}
	/*else
	{
		//tx_semaphore_wait(m_oCompList[CompNum].psem);
		DBG_OERR(OMX_SendCommand (m_oCompList[CompNum].handle, OMX_CommandStateSet, OMX_CommandPortEnable, NULL));
		while (omx_get_state_port(CompNum, port, port_type) == 0) usleep(10000);
	}*/
	if (now == OMX_NOW) tx_semaphore_wait(m_oCompList[CompNum].psem);
    //if (now == OMX_NOW) block_until_port_changed(m_oCompList[CompNum].handle, port_num, OMX_TRUE);
    DBG_LOG_OUT();
	return 1;
}

int omx_get_state(int CompNum) 
{
    DBG_LOG_IN();
	
	if (CompNum>=OMX_COMPONENTS_MAX) {DBG_LOG_OUT();return 0;}
    OMX_STATETYPE eState;
	OMX_GetState(m_oCompList[CompNum].handle, &eState);
	
	DBG_LOG_OUT();
	return eState;
}

int omx_set_state (int CompNum, int state_type, int now) 
{
	DBG_LOG_IN();
	
	int cur_state = omx_get_state(CompNum);
    if (CompNum>=OMX_COMPONENTS_MAX) {DBG_LOG_OUT();return 0;}
    if (cur_state != state_type) 
	{
		//if ((now != OMX_NOW) || (omx_wait_list_empty(CompNum) == 0))
		{
			omx_send_cmd(CompNum, OMX_CommandStateSet, state_type);
			dbgprintf(8,"omx_set_state:%i/%i>>>%i\n", CompNum, cur_state, state_type);
			if (now == OMX_NOW) 
			{
				tx_semaphore_wait(m_oCompList[CompNum].psem);
				dbgprintf(8,"state changed:%i/%i>>>%i\n", CompNum, cur_state, state_type);
			}	
		}
	/*	else
		{
			DBG_OERR(OMX_SendCommand (m_oCompList[CompNum].handle, OMX_CommandStateSet, state_type, NULL));
			while(omx_get_state(CompNum) != state_type) usleep(10000);
			//block_until_state_changed(m_oCompList[CompNum].handle, state_type);
		}*/
	}
    
	DBG_LOG_OUT();
	return 1;
}    

int omx_set_stride_slice_port(int CompNum, int port, int port_type, int stride, int slice)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
    OMX_PARAM_PORTDEFINITIONTYPE portdefa;
    
    portdefa.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdefa.nVersion.nVersion = OMX_VERSION;
    portdefa.nPortIndex = port_num;
    DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdefa));
	
	if (slice == -1) portdefa.format.image.nSliceHeight = portdefa.format.image.nFrameHeight;
		else portdefa.format.image.nSliceHeight = slice;     // multi of 16
	if (stride == -1) portdefa.format.image.nStride = (portdefa.format.image.nFrameWidth + portdefa.nBufferAlignment - 1) & (~(portdefa.nBufferAlignment - 1));		
		else portdefa.format.image.nStride = stride;  // multi of 32
    
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdefa));
    
	DBG_LOG_OUT();
	return 1;
}
  
int omx_LoadComponent(int ComponentID, int iFunc)
{	
	DBG_LOG_IN();
	
	OMX_CALLBACKTYPE m_Calls;
	m_Calls.EventHandler = CallBackEvent; //omx_event_handler_custom;
    m_Calls.EmptyBufferDone = CallBackEmpty; //event_handler_empty_buffer_refiller;
    m_Calls.FillBufferDone = CallBackFill; //omx_event_handler_filled_buffer; 
	//CallBackEmpty->pAppData = ComponentID;
    //if (ComponentID == OMX_COMP_IMAGE_DECODE) m_Calls.EmptyBufferDone = omx_set_stride_slice_port; //omx_event_handler_empty_buffer;
	if (iFunc == 1) m_Calls.FillBufferDone = CallBackFillWait;
	if (iFunc == 2) m_Calls.FillBufferDone = CallBackFillSync;
	int m_iComp = -1;
	int n;
	
	DBG_MUTEX_LOCK(&psem_components.mutex);	
	for (n = 0; n != OMX_COMPONENTS_MAX; n++)
		if (m_oCompList[n].psem == 0) break;
	if (n == OMX_COMPONENTS_MAX) {DBG_LOG_OUT();return -1;}
	m_iComp = n;
	m_oCompList[m_iComp].type = ComponentID;
	m_oCompList[m_iComp].psem = DBG_MALLOC(sizeof(TX_SEMAPHORE));
	m_oCompList[m_iComp].handle = NULL;
	DBG_MUTEX_UNLOCK(&psem_components.mutex);
	
	tx_semaphore_create(m_oCompList[m_iComp].psem, NULL, 0);
	omx_InitComponent(m_iComp, &m_Calls);	
	
	char cName[128];
	omx_GetComponentName(m_oCompList[m_iComp].type, cName, 128);
	if (OMX_GetHandle(&m_oCompList[m_iComp].handle, (char *)cName, (void *)(intptr_t)(m_iComp), &m_Calls) != OMX_ErrorNone) 
	{
		dbgprintf(1,"OMX error open component:%s\n", cName);
		omx_Release_Component(m_iComp);
		{DBG_LOG_OUT();return -2;}
	}
	for (n = 0; n != m_oCompList[m_iComp].in_port_cnt; n++)
	{
	/*	m_oCompList[m_iComp].in_port[n].source_buffer_data_size = 0;
		m_oCompList[m_iComp].in_port[n].source_buffer = NULL;	
    	m_oCompList[m_iComp].in_port[n].source_buffer_position = 0;
		m_oCompList[m_iComp].in_port[n].source_buffer_flag = 0;*/
		m_oCompList[m_iComp].in_port[n].buffer_active = 0;  
    	omx_disable_port(m_iComp, n, OMX_PORT_IN, OMX_LATER);    	
	}	
	for (n = 0; n != m_oCompList[m_iComp].out_port_cnt; n++)
	{
	/*	m_oCompList[m_iComp].out_port[n].source_buffer_data_size = 0;
		m_oCompList[m_iComp].out_port[n].source_buffer = NULL;	
    	m_oCompList[m_iComp].out_port[n].source_buffer_position = 0;
		m_oCompList[m_iComp].out_port[n].source_buffer_flag = 0;*/
		m_oCompList[m_iComp].out_port[n].buffer_active = 0; 	
    	omx_disable_port(m_iComp, n, OMX_PORT_OUT, OMX_LATER);	
	}
	//if (iFunc == 0) omx_set_state(m_iComp, OMX_StateIdle, OMX_LATER);
    tx_semaphore_wait(m_oCompList[m_iComp].psem);
	
	DBG_LOG_OUT();
	return m_iComp;
}

int omx_InitComponent(int ComponentNum, OMX_CALLBACKTYPE *m_Calls)
{	
	DBG_LOG_IN();
	
	if (ComponentNum>=OMX_COMPONENTS_MAX) {DBG_LOG_OUT();return 0;}
	switch(m_oCompList[ComponentNum].type)
	{
		case OMX_COMP_AUDIO_CAPTURE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[0].number=181;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=180;
				break;
		case OMX_COMP_AUDIO_DECODE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=120;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=121;
				break;
		case OMX_COMP_AUDIO_ENCODE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=160;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=161;
				break;
		case OMX_COMP_AUDIO_LOWPOWER:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=270;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_AUDIO_MIXER:
				m_oCompList[ComponentNum].in_port_cnt=5;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[0].number=230;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[1].number=232;
				m_oCompList[ComponentNum].in_port[2].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[2].number=233;
				m_oCompList[ComponentNum].in_port[3].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[3].number=234;
				m_oCompList[ComponentNum].in_port[4].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[4].number=235;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=231;
				break;
		case OMX_COMP_AUDIO_PROCESSOR:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=300;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=301;
				break;
		case OMX_COMP_AUDIO_RENDER:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=100;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[1].number=101;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_AUDIO_SPLITTER:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[0].number=260;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[1].number=261;
				m_oCompList[ComponentNum].out_port_cnt=4;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=262;
				m_oCompList[ComponentNum].out_port[1].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[1].number=263;
				m_oCompList[ComponentNum].out_port[2].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[2].number=264;
				m_oCompList[ComponentNum].out_port[3].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[3].number=265;
				break;
		case OMX_COMP_IMAGE_DECODE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=320;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=321;
				break;
		case OMX_COMP_IMAGE_ENCODE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=340;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=341;
				break;
		case OMX_COMP_IMAGE_FX:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=190;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=191;
				break;
		case OMX_COMP_IMAGE_READ:
				m_oCompList[ComponentNum].in_port_cnt=0;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=310;
				break;
		case OMX_COMP_IMAGE_WRITE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=330;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_RESIZE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=60;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=61;
				break;
		case OMX_COMP_SOURCE:
				m_oCompList[ComponentNum].in_port_cnt=0;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=20;
				break;
		case OMX_COMP_TRANSITION:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=210;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[1].number=211;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[0].number=212;
				break;
		case OMX_COMP_WRITE_STILL:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[0].number=30;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_CLOCK:
				m_oCompList[ComponentNum].in_port_cnt=0;
				m_oCompList[ComponentNum].out_port_cnt=6;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].out_port[0].number=80;
				m_oCompList[ComponentNum].out_port[1].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].out_port[1].number=81;
				m_oCompList[ComponentNum].out_port[2].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].out_port[2].number=82;
				m_oCompList[ComponentNum].out_port[3].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].out_port[3].number=83;
				m_oCompList[ComponentNum].out_port[4].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].out_port[4].number=84;
				m_oCompList[ComponentNum].out_port[5].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].out_port[5].number=85;
				break;
		case OMX_COMP_NULL_SINK:
				m_oCompList[ComponentNum].in_port_cnt=3;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=240;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].in_port[1].number=241;
				m_oCompList[ComponentNum].in_port[2].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[2].number=242;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_TEXT_SCHEDULER:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_TEXT;
				m_oCompList[ComponentNum].in_port[0].number=150;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[1].number=152;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_TEXT;
				m_oCompList[ComponentNum].out_port[0].number=151;
				break;
		case OMX_COMP_VISUALISATION:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=140;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[1].number=143;
				m_oCompList[ComponentNum].out_port_cnt=2;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=141;
				m_oCompList[ComponentNum].out_port[1].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[1].number=142;
				break;
		case OMX_COMP_READ_MEDIA:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[0].number=113;
				m_oCompList[ComponentNum].out_port_cnt=3;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].out_port[0].number=110;
				m_oCompList[ComponentNum].out_port[1].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[1].number=111;
				m_oCompList[ComponentNum].out_port[2].type=OMX_PORT_TEXT;
				m_oCompList[ComponentNum].out_port[2].number=112;
				break;
		case OMX_COMP_WRITE_MEDIA:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_AUDIO;
				m_oCompList[ComponentNum].in_port[0].number=170;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[1].number=171;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_CAMERA:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[0].number=73;
				m_oCompList[ComponentNum].out_port_cnt=3;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[0].number=70;
				m_oCompList[ComponentNum].out_port[1].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[1].number=71;
				m_oCompList[ComponentNum].out_port[2].type=OMX_PORT_IMAGE;
				m_oCompList[ComponentNum].out_port[2].number=72;
				break;
		case OMX_COMP_EGL_RENDER:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=220;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[0].number=221;
				break;
		case OMX_COMP_VIDEO_DECODE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=130;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[0].number=131;
				break;
		case OMX_COMP_VIDEO_ENCODE:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=200;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[0].number=201;
				break;
		case OMX_COMP_VIDEO_RENDER:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=90;
				m_oCompList[ComponentNum].out_port_cnt=0;
				break;
		case OMX_COMP_VIDEO_SCHEDULER:
				m_oCompList[ComponentNum].in_port_cnt=2;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=10;
				m_oCompList[ComponentNum].in_port[1].type=OMX_PORT_CLOCK;
				m_oCompList[ComponentNum].in_port[1].number=12;
				m_oCompList[ComponentNum].out_port_cnt=1;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[0].number=11;
				break;
		case OMX_COMP_VIDEO_SPLITTER:
				m_oCompList[ComponentNum].in_port_cnt=1;
				m_oCompList[ComponentNum].in_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].in_port[0].number=250;
				m_oCompList[ComponentNum].out_port_cnt=4;
				m_oCompList[ComponentNum].out_port[0].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[0].number=251;
				m_oCompList[ComponentNum].out_port[1].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[1].number=252;
				m_oCompList[ComponentNum].out_port[2].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[2].number=253;
				m_oCompList[ComponentNum].out_port[3].type=OMX_PORT_VIDEO;
				m_oCompList[ComponentNum].out_port[3].number=254;
				break;
		default:
			    break;
	}
    
	DBG_LOG_OUT();	
	return 1;
}

int omx_Start(void)
{
	DBG_LOG_IN();
	
	int err;	
	if (m_iStarted != 0) {DBG_LOG_OUT();return 0;}
		
	pthread_mutex_init(&OMX_mutex, NULL);	
	cThreadOmxPlayStatus = 0;
	cThreadOmxImageStatus = 0;
	cThreadOmxEncoderStatus = 0;
	cThreadOmxCaptureStatus = 0;
	
	omx_resource_priority = 0;
	
	err = OMX_Init();
	if (err != OMX_ErrorNone)
	{
		dbgprintf(8,"Error startt omx :%i\n", err);
		{DBG_LOG_OUT();return 0;}
	}
	m_iStarted = 1;
	omx_speed_play = 1;
	tx_semaphore_create(&psem_omx_sync, NULL, 0);
	tx_semaphore_create(&psem_omx_run, NULL, 0);
	tx_semaphore_create(&psem_components, NULL, 0);	
	pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	memset(m_oCompList, 0, sizeof(omx_component) * OMX_COMPONENTS_MAX);
	
	DBG_LOG_OUT();
	return 1;
}

int omx_load_file(char *filename, omx_buffer * buffer) 
{
	DBG_LOG_IN();
		
	int32_t pos;
	FILE *fp;
	  
	buffer->data_size = 0;
	buffer->data = NULL;
	fp = fopen (filename, "rb");
	if (!fp) {DBG_LOG_OUT();return 0;}
	if (fseek (fp, 0L, SEEK_END) < 0) 
	{
		fclose (fp);
		DBG_LOG_OUT();	
		return 0;
	};
	pos = ftell (fp);
	if (pos == LONG_MAX) 
	{
		fclose (fp);
		DBG_LOG_OUT();	
		return 0;
	};
	buffer->data_size = pos;
	fseek (fp, 0L, SEEK_SET);
	
	if (buffer->data_size > 10000000)
	{
		dbgprintf(2, "Error: OMX file size > 10000000\n");
		fclose (fp);
		DBG_LOG_OUT();	
		return 0;
	}
	
	buffer->data = DBG_MALLOC(buffer->data_size);
	fread (buffer->data, 1, buffer->data_size, fp);
	fclose (fp);
  
	DBG_LOG_OUT();
	return 1;
}

int omx_load_file_size(char *filename, char *buffer, unsigned int uiSize) 
{
	DBG_LOG_IN();
		
	int32_t pos;
	FILE *fp;
	  
	unsigned int uiFileSize = 0;
	
	fp = fopen (filename, "rb");
	if (!fp) {DBG_LOG_OUT();return 0;}
	if (fseek (fp, 0L, SEEK_END) < 0) 
	{
		fclose (fp);
		DBG_LOG_OUT();	
		return 0;
	};
	pos = ftell (fp);
	if (pos == LONG_MAX) 
	{
		fclose (fp);
		DBG_LOG_OUT();	
		return 0;
	};
	uiFileSize = pos;
	fseek (fp, 0L, SEEK_SET);
	
	if (uiFileSize != uiSize)
	{
		dbgprintf(2, "Error: OMX file size %i != %i\n", uiFileSize, uiSize);
		fclose (fp);
		DBG_LOG_OUT();	
		return 0;
	}
	
	fread (buffer, 1, uiFileSize, fp);
	fclose (fp);
  
	DBG_LOG_OUT();
	return 1;
}

int omx_get_image_sensor_current_params(int CompNum, image_sensor_params *ispCSI)
{
	int res = 1;
	OMX_ERRORTYPE oerr;
	
	OMX_CONFIG_EXPOSURECONTROLTYPE exposure_control_st;
	OMX_INIT_STRUCTURE (exposure_control_st);
	exposure_control_st.nPortIndex = OMX_ALL;
	oerr = OMX_GetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonExposure, &exposure_control_st);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	
	ispCSI->Exposure.Mode = exposure_control_st.eExposureControl;

	OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_control;
    OMX_INIT_STRUCTURE(white_balance_control);
    white_balance_control.nPortIndex = OMX_ALL;
    oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonWhiteBalance, &white_balance_control);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	
	OMX_CONFIG_CUSTOMAWBGAINSTYPE white_balance_gains_st;
	OMX_INIT_STRUCTURE (white_balance_gains_st);
	oerr = OMX_GetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCustomAwbGains, &white_balance_gains_st);	
	if (OMX_TEST_ERROR(oerr)) res = 0;
		
	ispCSI->WhiteBalance.Mode = white_balance_control.eWhiteBalControl;
	ispCSI->WhiteBalance.CrGain = (white_balance_gains_st.xGainR * 1000) >> 16;
	ispCSI->WhiteBalance.CbGain = (white_balance_gains_st.xGainB * 1000) >> 16;
	
	OMX_CONFIG_SHARPNESSTYPE sharpness;
    OMX_CONFIG_CONTRASTTYPE contrast;
    OMX_CONFIG_SATURATIONTYPE saturation;
    OMX_CONFIG_BRIGHTNESSTYPE brightness;
    OMX_INIT_STRUCTURE(contrast);
    OMX_INIT_STRUCTURE(saturation);
    OMX_INIT_STRUCTURE(brightness);
    OMX_INIT_STRUCTURE(sharpness);
    sharpness.nPortIndex = OMX_ALL;
    contrast.nPortIndex = OMX_ALL;
    saturation.nPortIndex = OMX_ALL;
    brightness.nPortIndex = OMX_ALL;
	
    oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonSharpness, &sharpness);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonContrast, &contrast);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonSaturation, &saturation);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonBrightness, &brightness);
	if (OMX_TEST_ERROR(oerr)) res = 0;	
	
	ispCSI->Brightness = brightness.nBrightness;
	ispCSI->ColorSaturation = saturation.nSaturation;
	ispCSI->Contrast = contrast.nContrast;
	ispCSI->Sharpness = sharpness.nSharpness;
	
	/*ispCSI->BacklightCompensationMode;
	ispCSI->BacklightCompensationLevel;
	ispCSI->IrCutFilter;
	ispCSI->WideDynamicRangeMode>
	ispCSI->WideDynamicRangeLevel;  
	ispCSI->Focus.AutoFocusMode;
	ispCSI->Focus.DefaultSpeed;
	ispCSI->Focus.NearLimit = 0;
	ispCSI->Focus.FarLimit = 0;*/
    
	return res;
}

void omx_get_image_sensor_module_params(image_sensor_params *ispCSI, MODULE_INFO *miModule)
{
	ispCSI->ModuleID = miModule->ID;
	ispCSI->Exposure.Mode = GetStandartExposure(miModule->Settings[24]);
	ispCSI->WhiteBalance.Mode = GetStandartWhiteBalance(miModule->Settings[29]);
	ispCSI->Brightness = (int)miModule->Settings[25];
	ispCSI->ColorSaturation = (int)miModule->Settings[32];
	ispCSI->Contrast = (int)miModule->Settings[30];
	ispCSI->Sharpness = (int)miModule->Settings[31];
	ispCSI->Focus.AutoFocusMode = (miModule->Settings[38] & 2) ? 1 : 0;
	ispCSI->Focus.DefaultSpeed = 1;
	ispCSI->ImageFilter = GetStandartImageFilter(miModule->Settings[28]);
	ispCSI->RotateMode = miModule->Settings[26] & 3;
	ispCSI->MainVideo.video_bit_rate = miModule->Settings[4]*1000;
	
	ispCSI->FrameLeftCrop = (miModule->Settings[10] & 0xFF000000) >> 24;
	ispCSI->FrameRightCrop = (miModule->Settings[10] & 0x00FF0000) >> 16;
	ispCSI->FrameUpCrop = (miModule->Settings[11] & 0xFF000000) >> 24;
	ispCSI->FrameDownCrop = (miModule->Settings[11] & 0x00FF0000) >> 16;
	
	GetResolutionFromModule(miModule->Settings[1], &ispCSI->MainVideo.video_width, &ispCSI->MainVideo.video_height);
	ispCSI->MainVideo.video_frame_rate = miModule->Settings[2];
	ispCSI->MainVideo.video_intra_frame = miModule->Settings[3];
	ispCSI->MainVideoAvcProfile = GetOmxProfile(miModule->Settings[19] & 255);
	ispCSI->MainVideoAvcLevel = GetOmxLevel(miModule->Settings[20] & 255);
	ispCSI->MainVideoConstantBitRate = (miModule->Settings[8] & 256) ? 1 : 0; 
	ispCSI->PrevVideoAvcProfile = GetOmxProfile((miModule->Settings[19] >> 8) & 255);
	ispCSI->PrevVideoAvcLevel = GetOmxLevel((miModule->Settings[20] >> 8) & 255);
	ispCSI->PrevVideoConstantBitRate = (miModule->Settings[8] & 512) ? 1 : 0; 
	ispCSI->PreviewEnabled = (miModule->Settings[8] & 4) ? 1 : 0;
	ispCSI->PrevVideo.video_bit_rate = miModule->Settings[17]*1000;
	GetResolutionFromModule(miModule->Settings[37], &ispCSI->PrevVideo.video_width, &ispCSI->PrevVideo.video_height);
	ispCSI->PrevVideo.video_frame_rate = ispCSI->MainVideo.video_frame_rate;
	ispCSI->PrevVideo.video_intra_frame = miModule->Settings[21];
	ispCSI->RecordEnabled = (miModule->Settings[8] & 32) ? 1 : 0;
	ispCSI->FlipHorisontal = (miModule->Settings[8] & 64) ? 1 : 0; 
	ispCSI->FlipVertical = (miModule->Settings[8] & 128) ? 1 : 0;
	ispCSI->AutoBrightControl  = (miModule->Settings[8] & 2048) ? 1 : 0;
	ispCSI->DestBrightControl = miModule->Settings[27];
	ispCSI->KeepRatio = (miModule->Settings[38] & 4) ? 1 : 0;
	
	ispCSI->ISOControl = miModule->Settings[40] & 0b1111111111;
	ispCSI->HardAutoISOControl = ((miModule->Settings[40] >> 10) & 1) ? 1 : 0;
	ispCSI->SoftAutoISOControl = ((miModule->Settings[40] >> 11) & 1) ? 1 : 0;
	ispCSI->DestAutoISOControl = (miModule->Settings[40] >> 12) & 127;
	if (ispCSI->DestAutoISOControl > 100) ispCSI->DestAutoISOControl = 50;
	if ((ispCSI->ISOControl < 100) || (ispCSI->ISOControl > 800)) ispCSI->ISOControl = 400;
	
	ispCSI->EVControl = miModule->Settings[41] & 0b11111;
	ispCSI->HardAutoEVControl = ((miModule->Settings[41] >> 5) & 1) ? 1 : 0;
	ispCSI->SoftAutoEVControl = ((miModule->Settings[41] >> 6) & 1) ? 1 : 0;
	ispCSI->DestAutoEVControl = (miModule->Settings[41] >> 7) & 127;
	if (ispCSI->DestAutoEVControl > 100) ispCSI->DestAutoEVControl = 50;
	if ((ispCSI->EVControl < -24) || (ispCSI->EVControl > 24)) ispCSI->EVControl = 0;	

	if (ispCSI->AutoBrightControl)
	{
		ispCSI->SoftAutoISOControl = 0;
		ispCSI->SoftAutoEVControl = 0;
	}
	if (ispCSI->SoftAutoISOControl) ispCSI->SoftAutoEVControl = 0;
	if (ispCSI->HardAutoISOControl) ispCSI->SoftAutoISOControl = 0;
	if (ispCSI->HardAutoEVControl) ispCSI->SoftAutoEVControl = 0;
	
	if (ispCSI->FrameLeftCrop > 100) ispCSI->FrameLeftCrop = 100;
	if (ispCSI->FrameRightCrop > 100) ispCSI->FrameRightCrop = 100;
	if (ispCSI->FrameUpCrop > 100) ispCSI->FrameUpCrop = 100;
	if (ispCSI->FrameDownCrop > 100) ispCSI->FrameDownCrop = 100;
	if ((ispCSI->FrameLeftCrop + ispCSI->FrameRightCrop) > 100)
	{
		dbgprintf(2, "Error: Left + Rigth Crop %i %i\n", ispCSI->FrameLeftCrop, ispCSI->FrameRightCrop);
		ispCSI->FrameLeftCrop = 0;
		ispCSI->FrameRightCrop = 0;		
	}
	if ((ispCSI->FrameUpCrop + ispCSI->FrameDownCrop) > 100)
	{
		dbgprintf(2, "Error: Up + Down Crop %i %i\n", ispCSI->FrameUpCrop, ispCSI->FrameDownCrop);
		ispCSI->FrameUpCrop = 0;
		ispCSI->FrameDownCrop = 0;		
	}
	
	ispCSI->AspectRatioW = ispCSI->MainVideo.video_width;
	ispCSI->AspectRatioH = ispCSI->MainVideo.video_height;
	
	if ((ispCSI->FrameLeftCrop != 0) ||
		(ispCSI->FrameRightCrop != 0) ||
		(ispCSI->FrameUpCrop != 0) ||
		(ispCSI->FrameDownCrop != 0))
		{
			if (ispCSI->KeepRatio)
			{
				if ((ispCSI->FrameLeftCrop + ispCSI->FrameRightCrop) >= 100) {ispCSI->FrameLeftCrop = 0; ispCSI->FrameRightCrop = 0;}
				if ((ispCSI->FrameUpCrop + ispCSI->FrameDownCrop) >= 100) {ispCSI->FrameUpCrop = 0; ispCSI->FrameDownCrop = 0;}
				unsigned int xWidth = 100 - ispCSI->FrameLeftCrop - ispCSI->FrameRightCrop;
				unsigned int xHeight = 100 - ispCSI->FrameUpCrop - ispCSI->FrameDownCrop;
				if (xWidth >= xHeight)
				{
					ispCSI->AspectRatioH = (float)(ispCSI->AspectRatioH) / ((float)(xWidth) / xHeight);
				}
				else
				{
					ispCSI->AspectRatioW = (float)(ispCSI->AspectRatioW) / ((float)(xHeight) / xWidth);
				}
			}
		} else ispCSI->KeepRatio = 0;
}

void omx_set_image_sensor_module_params(image_sensor_params *ispCSI, MODULE_INFO *miModule)
{
	miModule->Settings[24] = ispCSI->Exposure.Mode;
	miModule->Settings[29] = ispCSI->WhiteBalance.Mode;
	miModule->Settings[25] = ispCSI->Brightness;
	miModule->Settings[32] = ispCSI->ColorSaturation;
	miModule->Settings[30] = ispCSI->Contrast;
	miModule->Settings[31] = ispCSI->Sharpness;
	if (miModule->Settings[38] & 2) miModule->Settings[38] ^= 2;
	miModule->Settings[38] |= ispCSI->Focus.AutoFocusMode ? 2 : 0;
	miModule->Settings[38] |= ispCSI->KeepRatio ? 4 : 0;
	
	miModule->Settings[28] = ispCSI->ImageFilter;
	miModule->Settings[26] = (miModule->Settings[26] & 0xFFFFFFFC) | ispCSI->RotateMode;
	miModule->Settings[4] = ispCSI->MainVideo.video_bit_rate/1000;
	miModule->Settings[1] = ((ispCSI->MainVideo.video_width & 0xFFFF) << 16) | (ispCSI->MainVideo.video_height & 0xFFFF);
	miModule->Settings[2] = ispCSI->MainVideo.video_frame_rate;
	miModule->Settings[3] = ispCSI->MainVideo.video_intra_frame;
	miModule->Settings[19] = (miModule->Settings[19] & 0xFFFFFF00) | GetOmxProfileNum(ispCSI->MainVideoAvcProfile);
	miModule->Settings[20] = (miModule->Settings[20] & 0xFFFFFF00) | GetOmxLevelNum(ispCSI->MainVideoAvcLevel);
	miModule->Settings[19] = (miModule->Settings[19] & 0xFFFF00FF) | (GetOmxProfileNum(ispCSI->PrevVideoAvcProfile) << 8);
	miModule->Settings[20] = (miModule->Settings[20] & 0xFFFF00FF) | (GetOmxProfileNum(ispCSI->PrevVideoAvcLevel) << 8);
	miModule->Settings[10] = (miModule->Settings[10] & 0x00FFFFFF) | (ispCSI->FrameLeftCrop << 24);
	miModule->Settings[10] = (miModule->Settings[10] & 0xFF00FFFF) | (ispCSI->FrameRightCrop << 16);
	miModule->Settings[11] = (miModule->Settings[11] & 0x00FFFFFF) | (ispCSI->FrameUpCrop << 24);
	miModule->Settings[11] = (miModule->Settings[11] & 0xFF00FFFF) | (ispCSI->FrameDownCrop << 16);
	miModule->Settings[17] = ispCSI->PrevVideo.video_bit_rate / 1000;	
	miModule->Settings[37] = ((ispCSI->PrevVideo.video_width & 0xFFFF) << 16) | (ispCSI->PrevVideo.video_height & 0xFFFF);
	miModule->Settings[21] = ispCSI->PrevVideo.video_intra_frame;
	
	if (miModule->Settings[8] & 4) miModule->Settings[8] ^= 4;
	if (miModule->Settings[8] & 32) miModule->Settings[8] ^= 32;
	if (miModule->Settings[8] & 64) miModule->Settings[8] ^= 64;
	if (miModule->Settings[8] & 128) miModule->Settings[8] ^= 128;
	if (miModule->Settings[8] & 256) miModule->Settings[8] ^= 256;
	if (miModule->Settings[8] & 512) miModule->Settings[8] ^= 512;
	miModule->Settings[8] |= ispCSI->PreviewEnabled ? 4 : 0;	
	miModule->Settings[8] |= ispCSI->RecordEnabled ? 32 : 0;	
	miModule->Settings[8] |= ispCSI->FlipHorisontal ? 64 : 0;	
	miModule->Settings[8] |= ispCSI->FlipVertical ? 128 : 0;
	miModule->Settings[8] |= ispCSI->MainVideoConstantBitRate ? 256 : 0;	
	miModule->Settings[8] |= ispCSI->PrevVideoConstantBitRate ? 512 : 0;	
	miModule->Settings[8] |= ispCSI->AutoBrightControl ? 2048 : 0;	
	miModule->Settings[27] = ispCSI->DestBrightControl;	
	
	miModule->Settings[40] = ispCSI->ISOControl & 0b1111111111;
	miModule->Settings[40] |= (ispCSI->HardAutoISOControl & 1) << 10;
	miModule->Settings[40] |= (ispCSI->SoftAutoISOControl & 1) << 11;
	miModule->Settings[40] |= (ispCSI->DestAutoISOControl & 127) << 12;
	
	miModule->Settings[41] = ispCSI->EVControl & 0b11111;
	miModule->Settings[41] |= (ispCSI->HardAutoEVControl & 1) << 5;
	miModule->Settings[41] |= (ispCSI->SoftAutoEVControl & 1) << 6;
	miModule->Settings[41] |= (ispCSI->DestAutoEVControl & 127) << 7;			
}

int omx_set_image_sensor_camera_params(image_sensor_params *ispCSI)
{
	int ret = 0;
	int n;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (n = 0; n < iModuleCnt; n++)
		if ((miModuleList[n].Enabled & 1) && (miModuleList[n].Local == 1) && (miModuleList[n].Type == MODULE_TYPE_CAMERA))
		{			
			omx_set_image_sensor_module_params(ispCSI, &miModuleList[n]);
			ret = 1;
			break;
		}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	return ret;
}

void omx_camera_apply_params(int iCameraNum, image_sensor_params *oldCSI, image_sensor_params *newCSI, char cForce)
{
	if (cForce || (oldCSI->Exposure.Mode != newCSI->Exposure.Mode))
				omx_set_exposure_value(iCameraNum, GetStandartExposure(newCSI->Exposure.Mode));
	if (cForce || (oldCSI->ImageFilter != newCSI->ImageFilter))
				omx_set_imagefilter_value(iCameraNum, newCSI->ImageFilter);
	if (cForce || (oldCSI->WhiteBalance.Mode != newCSI->WhiteBalance.Mode))
				omx_set_whitebalance_value(iCameraNum, newCSI->WhiteBalance.Mode);
	if (cForce || (oldCSI->Brightness != newCSI->Brightness))
				omx_set_bright_value(iCameraNum, newCSI->Brightness);
	if (cForce || (oldCSI->ColorSaturation != newCSI->ColorSaturation))
				omx_set_saturation_value(iCameraNum, newCSI->ColorSaturation);
	if (cForce || (oldCSI->Contrast != newCSI->Contrast))
				omx_set_contrast_value(iCameraNum, newCSI->Contrast);
	if (cForce || (oldCSI->Sharpness != newCSI->Sharpness))
				omx_set_sharpness_value(iCameraNum, newCSI->Sharpness);
	if (cForce || (oldCSI->RotateMode != newCSI->RotateMode))
				omx_set_rotate_value(iCameraNum, newCSI->RotateMode, 0);
	if (cForce || ((oldCSI->FrameLeftCrop != newCSI->FrameLeftCrop) ||
					(oldCSI->FrameRightCrop != newCSI->FrameRightCrop) ||
					(oldCSI->FrameUpCrop != newCSI->FrameUpCrop) ||
					(oldCSI->FrameDownCrop != newCSI->FrameDownCrop)))
				omx_set_crop_value(iCameraNum, newCSI->FrameLeftCrop, newCSI->FrameRightCrop, newCSI->FrameUpCrop, newCSI->FrameDownCrop);	
	if (cForce || ((oldCSI->FlipHorisontal != newCSI->FlipHorisontal) ||
					(oldCSI->FlipVertical != newCSI->FlipVertical)))
				omx_set_flip_value(iCameraNum, newCSI->FlipHorisontal, newCSI->FlipVertical, 0);
	if (cForce || (oldCSI->ISOControl != newCSI->ISOControl) ||
					(oldCSI->EVControl != newCSI->EVControl) ||
				    (oldCSI->HardAutoISOControl != newCSI->HardAutoISOControl))
				omx_set_iso_value(iCameraNum, newCSI->EVControl, newCSI->HardAutoISOControl, newCSI->ISOControl);
	
	oldCSI->Exposure.Mode = newCSI->Exposure.Mode;
	oldCSI->ImageFilter = newCSI->ImageFilter;
	oldCSI->WhiteBalance.Mode = newCSI->WhiteBalance.Mode;
	oldCSI->Brightness = newCSI->Brightness;
	oldCSI->ColorSaturation = newCSI->ColorSaturation;
	oldCSI->Contrast = newCSI->Contrast;
	oldCSI->Sharpness = newCSI->Sharpness;
	oldCSI->RotateMode = newCSI->RotateMode;
	oldCSI->FrameLeftCrop = newCSI->FrameLeftCrop;
	oldCSI->FrameRightCrop = newCSI->FrameRightCrop;
	oldCSI->FrameUpCrop = newCSI->FrameUpCrop;
	oldCSI->FrameDownCrop = newCSI->FrameDownCrop;
	oldCSI->FlipHorisontal = newCSI->FlipHorisontal;
	oldCSI->FlipVertical = newCSI->FlipVertical;
	oldCSI->ISOControl = newCSI->ISOControl;
	oldCSI->EVControl = newCSI->EVControl;
}

int omx_set_image_compression_format(int CompNum, int port, int port_type, int oCodeFormat, OMX_COLOR_FORMATTYPE oColorFormat)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
    
	OMX_IMAGE_PARAM_PORTFORMATTYPE imagePortFormat;
    memset (&imagePortFormat, 0, sizeof (imagePortFormat));
    imagePortFormat.nSize = sizeof (imagePortFormat);
    imagePortFormat.nVersion.nVersion = OMX_VERSION;
    imagePortFormat.nPortIndex = port_num;
	OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamImagePortFormat, &imagePortFormat);	
    imagePortFormat.eCompressionFormat = oCodeFormat;
    if (oColorFormat) imagePortFormat.eColorFormat = oColorFormat;
    OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamImagePortFormat, &imagePortFormat);
    
	DBG_LOG_OUT();
	return 1;
}

int omx_set_video_compression_format(int CompNum, int port, int port_type, OMX_VIDEO_CODINGTYPE oCodeFormat)
{
   DBG_LOG_IN();
	
	int port_cnt, port_num;
   if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
   if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
   if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
     
   OMX_VIDEO_PARAM_PORTFORMATTYPE format;   
   memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = port_num;
   format.eCompressionFormat = oCodeFormat;
   DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoPortFormat, &format));
   
  /* OMX_VIDEO_PARAM_AVCTYPE avcformat;
   memset(&avcformat, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   avcformat.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   avcformat.nVersion.nVersion = OMX_VERSION;
   avcformat.nPortIndex = port_num;
   DBG_OERR(OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoAvc, &avcformat));
//    OMX_U32 nSize; 
//    OMX_VERSIONTYPE nVersion; 
//    OMX_U32 nPortIndex; 
//    OMX_U32 nSliceHeaderSpacing; 
//    OMX_U32 nPFrames; 
//    OMX_U32 nBFrames; 
//   OMX_BOOL bUseHadamard; 
//    OMX_U32 nRefFrames; 
//    OMX_U32 nRefIdx10ActiveMinus1; 
//    OMX_U32 nRefIdx11ActiveMinus1; 
//    OMX_BOOL bEnableUEP; 
//    OMX_BOOL bEnableFMO; 
//    OMX_BOOL bEnableASO; 
//    OMX_BOOL bEnableRS; 
//    OMX_VIDEO_AVCPROFILETYPE 
	if (iProfile) avcformat.eProfile = iProfile;
//   OMX_VIDEO_AVCLEVELTYPE 
	if (iLevel) avcformat.eLevel = iLevel;
//    OMX_U32 nAllowedPictureTypes; 
//    OMX_BOOL bFrameMBsOnly; 
//    OMX_BOOL bMBAFF; 
//    OMX_BOOL bEntropyCodingCABAC; 
//    OMX_BOOL bWeightedPPrediction; 
//    OMX_U32 nWeightedBipredicitonMode; 
//    OMX_BOOL bconstIpred ; 
//    OMX_BOOL bDirect8x8Inference; 
//    OMX_BOOL bDirectSpatialTemporal; 
//    OMX_U32 nCabacInitIdc; 
//    OMX_VIDEO_AVCLOOPFILTERTYPE eLoopFilterMode;
	if (iProfile || iLevel) DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoAvc, &avcformat));*/
/*	OMX_NALSTREAMFORMATTYPE nalformat;
	memset(&nalformat, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    nalformat.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	nalformat.nVersion.nVersion = OMX_VERSION;
	nalformat.nPortIndex = port_num;
	//DBG_OERR(OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamNalStreamFormat, &nalformat));
	nalformat.eNaluFormat = OMX_NaluFormatStartCodes;
	DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamNalStreamFormat, &nalformat));*/
	DBG_LOG_OUT();
	return 1;
}     

int omx_set_camera_capture_status(int CompNum, int iStatus, int iImage)
{
    DBG_LOG_IN();
	
	OMX_CONFIG_PORTBOOLEANTYPE capture;
    OMX_INIT_STRUCTURE(capture);
	if (iImage == OMX_FALSE)
		capture.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_2].number;
		else 
		capture.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_3].number;
    capture.bEnabled = iStatus;
    DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexConfigPortCapturing, &capture));
	
	DBG_LOG_OUT();
	return 1;
}

int omx_set_frame_settings(int CompNum, int port, int port_type,  int nFrameWidth, int nFrameHeight, int nColorFormat)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    if (pport[port].type == OMX_PORT_IMAGE)
    {
		portdef.format.image.nFrameWidth = nFrameWidth;
		portdef.format.image.nFrameHeight = nFrameHeight;
		portdef.format.image.nStride = (nFrameWidth+31)&~31;
		portdef.format.image.nSliceHeight = (nFrameHeight+15)&~15;
		if (nColorFormat) portdef.format.image.eColorFormat = nColorFormat;
		//portdef.format.image.nSliceHeight = portdef.format.image.nFrameHeight;
		//portdef.format.image.nStride = (portdef.format.image.nFrameWidth + portdef.nBufferAlignment - 1) & (~(portdef.nBufferAlignment - 1));	
	}
	else
	{
		portdef.format.video.nFrameWidth = nFrameWidth;
		portdef.format.video.nFrameHeight = nFrameHeight;
		portdef.format.video.nStride = (nFrameWidth+31)&~31;
		portdef.format.video.nSliceHeight = (nFrameHeight+15)&~15;
		if (nColorFormat) portdef.format.video.eColorFormat = nColorFormat;
		//portdef.format.video.nStride = nFrameWidth;
		//portdef.format.video.nSliceHeight = nFrameHeight;
	}
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    
	DBG_LOG_OUT();
	return 1;
}

int omx_set_frame_settings_ex(int CompNum, int port, int port_type,  int nFrameWidth, int nFrameHeight, 
								int eColorFormat, int nStride, int nSliceHeight, int eCompressionFormat, int bFlagErrorConcealment)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    if (pport[port].type == OMX_PORT_IMAGE)
    {
		if (nFrameWidth) portdef.format.image.nFrameWidth = nFrameWidth;
		if (nFrameHeight) portdef.format.image.nFrameHeight = nFrameHeight;
		portdef.format.image.nStride = nStride;
		portdef.format.image.nSliceHeight = nSliceHeight;
		if (eColorFormat) portdef.format.image.eColorFormat = eColorFormat;
		portdef.format.image.eCompressionFormat = eCompressionFormat;
		portdef.format.image.bFlagErrorConcealment = bFlagErrorConcealment;
		//portdef.format.image.nSliceHeight = portdef.format.image.nFrameHeight;
		//portdef.format.image.nStride = (portdef.format.image.nFrameWidth + portdef.nBufferAlignment - 1) & (~(portdef.nBufferAlignment - 1));	
	}
	else
	{
		if (nFrameWidth) portdef.format.video.nFrameWidth = nFrameWidth;
		if (nFrameHeight) portdef.format.video.nFrameHeight = nFrameHeight;
		portdef.format.video.nStride = nStride;
		portdef.format.video.nSliceHeight = nSliceHeight;
		if (eColorFormat) portdef.format.video.eColorFormat = eColorFormat;
		//portdef.format.video.nStride = nFrameWidth;
		//portdef.format.video.nSliceHeight = nFrameHeight;
	}
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    
	DBG_LOG_OUT();
	return 1;
}

int omx_set_frame_color(int CompNum, int port, int port_type,  int nColor)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    if (pport[port].type == OMX_PORT_IMAGE)
    {
		portdef.format.image.eColorFormat = nColor;
		portdef.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;	  
	}
	else
	{
		portdef.format.video.eColorFormat = nColor;
		portdef.format.video.eCompressionFormat = OMX_IMAGE_CodingUnused;
	}
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    
	DBG_LOG_OUT();
	return 1;
}

int omx_get_frame_color(int CompNum, int port, int port_type,  int *nColor)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    if (pport[port].type == OMX_PORT_IMAGE)
    {
		*nColor = portdef.format.image.eColorFormat;
	}
	else
	{
		*nColor = portdef.format.video.eColorFormat;
	}
    
	DBG_LOG_OUT();
	return 1;
}

int omx_copy_frame_settings(int CompNum, int port, int port_type,  int CompNumTo, int portTo, int port_typeTo)
{
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
	int port_cnt_to, port_num_to;
    if (port_typeTo == OMX_PORT_IN) port_cnt_to = m_oCompList[CompNumTo].in_port_cnt;
		else port_cnt_to = m_oCompList[CompNumTo].out_port_cnt;
    if ((CompNumTo>=OMX_COMPONENTS_MAX) || (port_cnt_to<=portTo)) {DBG_LOG_OUT();return 0;}
    omx_ports *pportTo;
    if (port_typeTo == OMX_PORT_IN) pportTo = m_oCompList[CompNumTo].in_port;
		else pportTo = m_oCompList[CompNumTo].out_port;    
    port_num_to = pportTo[portTo].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    
	portdef.nPortIndex = port_num_to;
    OMX_SetParameter (m_oCompList[CompNumTo].handle, OMX_IndexParamPortDefinition, &portdef);
    
	DBG_LOG_OUT();
	return 1;
}

int omx_get_frame_resolution(int CompNum, int port, int port_type,  int *nFrameWidth, int *nFrameHeight)
{	
	DBG_LOG_IN();
	
	int port_cnt, port_num;
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    if (pport[port].type == OMX_PORT_IMAGE)
    {		
		*nFrameWidth = portdef.format.image.nFrameWidth;
		*nFrameHeight = portdef.format.image.nFrameHeight;		
	}
	else
	{
		*nFrameWidth = portdef.format.video.nFrameWidth;
		*nFrameHeight = portdef.format.video.nFrameHeight;
	}
	//printf("%i, %i, %i, %i\n",portdef.format.image.nFrameWidth,portdef.format.image.nFrameHeight,portdef.format.image.nStride, portdef.format.image.nSliceHeight);
    //printf("%i, %i, %i, %i\n",portdef.format.video.nFrameWidth,portdef.format.video.nFrameHeight,portdef.format.video.nStride, portdef.format.video.nSliceHeight);
    
	DBG_LOG_OUT();
	return 1;
}

int omx_set_frame_resolution(int CompNum, int port, int port_type,  int nFrameWidth, int nFrameHeight, int nColorFormat)
{	
	DBG_LOG_IN();
	
	int port_cnt, port_num;
    omx_ports *pport;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
   // if (pport[port].type == OMX_PORT_IMAGE)
    // if (pport[port].type == OMX_PORT_IMAGE)
    {
		portdef.format.image.nFrameWidth = nFrameWidth;
		portdef.format.image.nFrameHeight = nFrameHeight;
		portdef.format.image.nStride = (nFrameWidth+31)&~31;
		portdef.format.image.nSliceHeight = (nFrameHeight+15)&~15;
		if (nColorFormat) portdef.format.image.eColorFormat = nColorFormat;
		//portdef.format.image.nSliceHeight = portdef.format.image.nFrameHeight;
		//portdef.format.image.nStride = (portdef.format.image.nFrameWidth + portdef.nBufferAlignment - 1) & (~(portdef.nBufferAlignment - 1));	
	}
	//else
	{
		portdef.format.video.nFrameWidth = nFrameWidth;
		portdef.format.video.nFrameHeight = nFrameHeight;
		portdef.format.video.nStride = (nFrameWidth+31)&~31;
		portdef.format.video.nSliceHeight = (nFrameHeight+15)&~15;
		if (nColorFormat) portdef.format.video.eColorFormat = nColorFormat;
	}
    DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
	DBG_LOG_OUT();
	return 1;
}

int omx_normalize_frame_format(int CompNum, int port, int port_type, int nFrameWidth, int nFrameHeight, int nMaxWidth, int nMaxHeight)
{   
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    //omx_ports *pport;    
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
   
    int res_width = nFrameWidth;
    int res_height= nFrameHeight;
    
    OMX_PARAM_RESIZETYPE portdefalt;
    portdefalt.nSize = sizeof (OMX_PARAM_RESIZETYPE);
    portdefalt.nVersion.nVersion = OMX_VERSION;
    portdefalt.nPortIndex = port_num;
        
    if (res_width > nMaxWidth) 
    {
      res_height = (unsigned int) (((double) nMaxWidth / (double) res_width) * (double) res_height);
      res_width = nMaxWidth;

    }
    if (res_height > nMaxHeight) 
    {
      res_width = (unsigned int) (((double) nMaxHeight / (double) res_height) * (double) res_width);
      res_height = nMaxHeight;
    } 
	//printf("normalize:%ix%i %ix%i\n", res_width, nFrameWidth, res_height, nFrameHeight);
    portdefalt.nMaxWidth = res_width;
    portdefalt.nMaxHeight = res_height;
	if ((res_height != nFrameHeight) || (res_width != nFrameWidth)) portdefalt.eMode = OMX_RESIZE_BOX; else portdefalt.eMode = OMX_RESIZE_NONE;
	portdefalt.bPreserveAspectRatio = OMX_TRUE;
    portdefalt.bAllowUpscaling = OMX_TRUE;
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamResize, &portdefalt));
    
	DBG_LOG_OUT();
	return 1;
}

int omx_set_ratio_value(int CompNum, unsigned int nX, unsigned int nY)
{
	OMX_ERRORTYPE oerr;
	OMX_CONFIG_POINTTYPE ratio_t;
	OMX_INIT_STRUCTURE (ratio_t);
	//ratio_t.nPortIndex = OMX_ALL;
	ratio_t.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
	//oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexParamBrcmPixelAspectRatio, &ratio_t);
	//printf("omx_set_ratio_value %i %i\n",ratio_t.nX, ratio_t.nY);
	ratio_t.nX = nX;
	ratio_t.nY = nY;
	oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexParamBrcmPixelAspectRatio, &ratio_t);
	
	if (OMX_TEST_ERROR(oerr)) return 0;	
	return 1;
}

int omx_set_fov_value(int CompNum, unsigned int xFieldOfViewHorizontal, unsigned int xFieldOfViewVertical)
{
	OMX_ERRORTYPE oerr;
	OMX_CONFIG_BRCMFOVTYPE fov_t;
	OMX_INIT_STRUCTURE (fov_t);
	//fov_t.nPortIndex = OMX_ALL;
	fov_t.nPortIndex = m_oCompList[CompNum].in_port[OMX_PORT_1].number;
	oerr = OMX_GetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigFieldOfView, &fov_t);
	printf("omx_set_fov_value %i %i %i %i\n",fov_t.xFieldOfViewHorizontal >> 16, fov_t.xFieldOfViewHorizontal & 65535, fov_t.xFieldOfViewVertical >> 16, fov_t.xFieldOfViewVertical & 65535);
	fov_t.xFieldOfViewHorizontal = xFieldOfViewHorizontal << 16;
	fov_t.xFieldOfViewVertical = xFieldOfViewVertical << 16;
	oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigFieldOfView, &fov_t);
	
	if (OMX_TEST_ERROR(oerr)) return 0;	
	return 1;
}

int omx_set_resize_format(int CompNum, int nMaxWidth, int nMaxHeight)
{   
    DBG_LOG_IN();
	
	int port_num;
    //omx_ports *pport;    
    port_num = m_oCompList[CompNum].out_port[0].number;
   
    OMX_PARAM_RESIZETYPE portdefalt;
    portdefalt.nSize = sizeof (OMX_PARAM_RESIZETYPE);
    portdefalt.nVersion.nVersion = OMX_VERSION;
    portdefalt.nPortIndex = port_num;
        
    portdefalt.nMaxWidth = nMaxWidth;
    portdefalt.nMaxHeight = nMaxHeight;
	portdefalt.eMode = OMX_RESIZE_BOX;
	portdefalt.bPreserveAspectRatio = OMX_TRUE;
    portdefalt.bAllowUpscaling = OMX_TRUE;
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamResize, &portdefalt));
    
	DBG_LOG_OUT();
	return 1;
}

int omx_get_resize_format(int CompNum, int port, int port_type)
{   
    DBG_LOG_IN();
	
	int port_cnt, port_num;
    //omx_ports *pport;    
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) port_num = m_oCompList[CompNum].in_port[port].number;
		else port_num = m_oCompList[CompNum].out_port[port].number;
   
    OMX_PARAM_RESIZETYPE portdefalt;
    portdefalt.nSize = sizeof (OMX_PARAM_RESIZETYPE);
    portdefalt.nVersion.nVersion = OMX_VERSION;
    portdefalt.nPortIndex = port_num;
    DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamResize, &portdefalt));
       
    dbgprintf(8,"Resize format, MaxW:%i, MaxH:%i, Mode:%i, Acpect:%i, UpScalling:%i\n",portdefalt.nMaxWidth, portdefalt.nMaxHeight, 
								portdefalt.eMode, portdefalt.bPreserveAspectRatio, portdefalt.bAllowUpscaling);
    
	DBG_LOG_OUT();
	return 1;
}

int omx_set_clock_state(int iClockNum, int iState, int iWaitMask)
{  
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	memset(&cstate, 0, sizeof(cstate));
	cstate.nSize = sizeof(cstate);
	cstate.nVersion.nVersion = OMX_VERSION;
	cstate.eState = iState;
	cstate.nWaitMask = iWaitMask;
	DBG_OERR(OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeClockState, &cstate));
	return 1;
}

int omx_set_clock_speed(int iClockNum, int iSpeed)
{  
	OMX_TIME_CONFIG_SCALETYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);

    scaleType.xScale = (iSpeed << 16) / 1000;
	DBG_OERR(OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeScale, &scaleType));
	return 1;
}

int omx_set_clock_seek(int iClockNum, int eType)
{  
	OMX_TIME_CONFIG_SEEKMODETYPE cseek;
	memset(&cseek, 0, sizeof(cseek));
	cseek.nSize = sizeof(cseek);
	cseek.nVersion.nVersion = OMX_VERSION;
	cseek.eType = eType;
	DBG_OERR(OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeSeekMode, &cseek));
	return 1;
}

int omx_set_clock_timepos(int iClockNum, int port, int64_t pts)
{  
	int port_cnt, port_num;
    //omx_ports *pport;    
    port_cnt = m_oCompList[iClockNum].out_port_cnt;
    if ((iClockNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    port_num = m_oCompList[iClockNum].out_port[port].number;
	
	OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
	OMX_INIT_STRUCTURE(timeStamp);
	timeStamp.nPortIndex = port_num;
	timeStamp.nTimestamp = ToOMXTime(pts);
	//DBG_OERR(OMX_GetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeCurrentMediaTime, &timeStamp));
	DBG_OERR(OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimePosition, &timeStamp));
	return 1;
}

int64_t omx_get_clock_time(int iClockNum, int port)
{
	int port_cnt, port_num;
    //omx_ports *pport;    
    port_cnt = m_oCompList[iClockNum].out_port_cnt;
    if ((iClockNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    port_num = m_oCompList[iClockNum].out_port[port].number;
   
	OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
	OMX_INIT_STRUCTURE(timeStamp);
	timeStamp.nPortIndex = port_num;
	DBG_OERR(OMX_GetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeClientStartTime, &timeStamp));	
    return FromOMXTime(timeStamp.nTimestamp);	;
}

int omx_set_clock_time(int iClockNum, int port, int64_t pts)
{
	int port_cnt, port_num;
    //omx_ports *pport;    
    port_cnt = m_oCompList[iClockNum].out_port_cnt;
    if ((iClockNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    port_num = m_oCompList[iClockNum].out_port[port].number;
   
	OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
	OMX_INIT_STRUCTURE(timeStamp);
	timeStamp.nPortIndex = port_num;
	timeStamp.nTimestamp = ToOMXTime(pts);
	DBG_OERR(OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeClientStartTime, &timeStamp));
    return 1;
}

OMX_TICKS omx_get_clock_time2(int iClockNum, int port)
{
	int port_num;
    //omx_ports *pport;    
    //port_cnt = m_oCompList[iClockNum].out_port_cnt;
    //if ((iClockNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) return 0;
    port_num = m_oCompList[iClockNum].out_port[port].number;
   
	OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
	OMX_INIT_STRUCTURE(timeStamp);
	timeStamp.nPortIndex = port_num;
	OMX_ERRORTYPE oerr = OMX_GetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeCurrentMediaTime, &timeStamp);
	if (oerr != OMX_ErrorNone)
	{
		dbgprintf(1, "OpenMax error: %x, (%s)\n", oerr, omx_err2str(oerr));
		dbgprintf(4, " failed on line %d: %x, (%s)\n",
                    __LINE__, oerr, omx_err2str(oerr));		
	}     
	
    return timeStamp.nTimestamp;
}

int omx_image_encoder_init(int CompNum, int iFrameWidth, int iFrameHeight, int iCompressionFormat)
{
	DBG_LOG_IN();
	
	OMX_PARAM_PORTDEFINITIONTYPE port_def;
	OMX_INIT_STRUCTURE (port_def);
	port_def.nPortIndex = m_oCompList[CompNum].in_port[OMX_PORT_1].number;
    DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def));
	port_def.format.image.nFrameWidth = iFrameWidth;
	port_def.format.image.nFrameHeight = iFrameHeight;	
	port_def.format.image.nStride      = (port_def.format.image.nFrameWidth+31)&~31;
	port_def.format.image.nSliceHeight = (port_def.format.image.nFrameHeight+15)&~15;
	port_def.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def));
	
	
	OMX_INIT_STRUCTURE (port_def);
	port_def.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
    DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def));
	port_def.format.image.nFrameWidth = iFrameWidth;
	port_def.format.image.nFrameHeight = iFrameHeight;	
	port_def.format.image.nStride      = (port_def.format.image.nFrameWidth+31)&~31;
	port_def.format.image.nSliceHeight = (port_def.format.image.nFrameHeight+15)&~15;
	port_def.format.image.eCompressionFormat = iCompressionFormat;
	port_def.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def));
	
    //Quality
    OMX_IMAGE_PARAM_QFACTORTYPE quality;
    OMX_INIT_STRUCTURE (quality);
    quality.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
    quality.nQFactor = JPEG_QUALITY;    
	DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamQFactor, &quality));
  
    //Disable EXIF tags
    OMX_CONFIG_BOOLEANTYPE exif;
    OMX_INIT_STRUCTURE (exif);
    exif.bEnabled = JPEG_EXIF_DISABLE;
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamBrcmDisableEXIF, &exif));
    
	//Enable IJG table
    OMX_PARAM_IJGSCALINGTYPE ijg;
    OMX_INIT_STRUCTURE (ijg);
    ijg.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
    ijg.bEnabled = JPEG_IJG_ENABLE;
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamBrcmEnableIJGTableScaling, &ijg));
  
    //Thumbnail
    OMX_PARAM_BRCMTHUMBNAILTYPE thumbnail;
    OMX_INIT_STRUCTURE (thumbnail);
    thumbnail.bEnable = JPEG_THUMBNAIL_ENABLE;
    thumbnail.bUsePreview = JPEG_PREVIEW;
    thumbnail.nWidth = JPEG_THUMBNAIL_WIDTH;
    thumbnail.nHeight = JPEG_THUMBNAIL_HEIGHT;
    DBG_OERR(OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamBrcmThumbnail, &thumbnail));
  
    //EXIF tags
    //See firmware/documentation/ilcomponents/image_decode.html for valid keys
    char key[] = "IFD0.Make";
    char value[] = "Raspberry Pi";  
    int key_length = strlen (key);
    int value_length = strlen (value);
  
    struct 
    {
      //These two fields need to be together
      OMX_CONFIG_METADATAITEMTYPE metadata_st;
      char metadata_padding[value_length];
    } item;
  
    OMX_INIT_STRUCTURE (item.metadata_st);
    item.metadata_st.nSize = sizeof (item);
    item.metadata_st.eScopeMode = OMX_MetadataScopePortLevel;
    item.metadata_st.nScopeSpecifier = 341;
    item.metadata_st.eKeyCharset = OMX_MetadataCharsetASCII;
    item.metadata_st.nKeySizeUsed = key_length;
    memcpy (item.metadata_st.nKey, key, key_length);
    item.metadata_st.eValueCharset = OMX_MetadataCharsetASCII;
    item.metadata_st.nValueMaxSize = sizeof (item.metadata_padding);
    item.metadata_st.nValueSizeUsed = value_length;
    memcpy (item.metadata_st.nValue, value, value_length);
  
    DBG_OERR(OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigMetadataItem, &item));
	
	DBG_LOG_OUT();
	return 1;
}

int omx_video_encoder_init(int CompNum, int iFrameWidth, int iFrameHeight, int iFramerate, int iBitrate, 
							int iCompressionFormat, int iIntraPeriod, int iInInit, int iBitType, int iProfile, int iLevel)
{
	DBG_LOG_IN();
	
	dbgprintf(3, "Init Video Encoder(%i) %ix%i FrameRate:%i BitRate:%i KeyFrame:%i BitType:%s Profile:%s Level:%s\n",
				CompNum,
				iFrameWidth, iFrameHeight, iFramerate, iBitrate, iIntraPeriod, iBitType ? "Const" : "Var", 
				GetOmxProfileName(iProfile), GetOmxLevelName(iLevel));
	
	OMX_PARAM_PORTDEFINITIONTYPE portdef;
    if (iInInit == OMX_TRUE)
	{
		OMX_INIT_STRUCTURE(portdef);
		portdef.nPortIndex = m_oCompList[CompNum].in_port[OMX_PORT_1].number;
		DBG_OERR(OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    
		portdef.format.video.nFrameWidth = iFrameWidth;
		portdef.format.video.nFrameHeight = iFrameHeight;
		portdef.format.video.xFramerate = iFramerate << 16;
		portdef.format.video.nStride = (iFrameWidth+31)&~31;
		portdef.format.video.nSliceHeight = (iFrameHeight+15)&~15;
		//portdef.format.video.nSliceHeight = portdef.format.video.nFrameHeight;
		//portdef.format.video.nStride = (portdef.format.video.nFrameWidth + portdef.nBufferAlignment - 1) & (~(portdef.nBufferAlignment - 1));
		//portdef.format.video.nStride = portdef.format.video.nFrameHeight;
		portdef.format.video.eColorFormat = VIDEO_COLOR_FORMAT;
		DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    }
    OMX_INIT_STRUCTURE(portdef);
	portdef.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
    DBG_OERR(OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    // Copy some of the encoder output port configuration
    // from camera output port
    portdef.format.video.nFrameWidth  = iFrameWidth;
    portdef.format.video.nFrameHeight = iFrameHeight;
    portdef.format.video.xFramerate   = iFramerate << 16;
    portdef.format.video.nStride = (iFrameWidth+31)&~31;
	portdef.format.video.nSliceHeight = (iFrameHeight+15)&~15;
	//portdef.format.video.nStride      = (portdef.format.video.nFrameWidth + portdef.nBufferAlignment - 1) & (~(portdef.nBufferAlignment - 1));
    // Which one is effective, this or the configuration just below?
    portdef.format.video.nBitrate     = iBitrate;
    DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));
    // Configure bitrate
    OMX_VIDEO_PARAM_BITRATETYPE bitrate;
    OMX_INIT_STRUCTURE(bitrate);
    if (iBitType == 0) 
		bitrate.eControlRate = OMX_Video_ControlRateVariable;
		else
		bitrate.eControlRate = OMX_Video_ControlRateConstant;
    bitrate.nTargetBitrate = portdef.format.video.nBitrate;
    bitrate.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
    DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoBitrate, &bitrate));
    // Configure format
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    OMX_INIT_STRUCTURE(format);
    format.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
    format.eCompressionFormat = iCompressionFormat; //OMX_VIDEO_CodingAVC;
    DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoPortFormat, &format));
	
	if (iIntraPeriod != 0)
	{
		// Configure intra period
		OMX_VIDEO_CONFIG_AVCINTRAPERIOD intraperiod;
		OMX_INIT_STRUCTURE(intraperiod);
		intraperiod.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
		intraperiod.nIDRPeriod = iIntraPeriod;
		intraperiod.nPFrames = iIntraPeriod;
		DBG_OERR(OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigVideoAVCIntraPeriod, &intraperiod));
	}
	
	OMX_VIDEO_PARAM_AVCTYPE avcformat;
	memset(&avcformat, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	avcformat.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	avcformat.nVersion.nVersion = OMX_VERSION;
	avcformat.nPortIndex = m_oCompList[CompNum].out_port[0].number;
	DBG_OERR(OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoAvc, &avcformat));
	
	if (iProfile) avcformat.eProfile = iProfile;
	if (iLevel) avcformat.eLevel = iLevel;
	if (iProfile || iLevel) 
		DBG_OERR(OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamVideoAvc, &avcformat));
	
	dbgprintf(3, "Init Video Encoder(%i) done\n", CompNum);
	
	DBG_LOG_OUT();
	return 1;
}

int omx_set_crop_value(int CompNum, unsigned int xLeft, unsigned int xRight, unsigned int xTop, unsigned int xDown)
{
	OMX_ERRORTYPE oerr;
	if ((xLeft + xRight) >= 100) {xLeft = 0; xRight = 0;}
	if ((xTop + xDown) >= 100) {xTop = 0; xDown = 0;}
	unsigned int xWidth = 100 - xLeft - xRight;
	unsigned int xHeight = 100 - xTop - xDown;
	OMX_CONFIG_INPUTCROPTYPE roi_st;
	OMX_INIT_STRUCTURE (roi_st);
	roi_st.nPortIndex = OMX_ALL;
	roi_st.xTop = (xTop << 16)/100;
	roi_st.xLeft = (xLeft << 16)/100;
	roi_st.xWidth = (xWidth << 16)/100;
	roi_st.xHeight = (xHeight << 16)/100;
	oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigInputCropPercentages, &roi_st);
	
	if (OMX_TEST_ERROR(oerr)) return 0;	
	return 1;
}

int omx_camera_init(int CompNum, int iFrameWidth, int iFrameHeight, int iFramerate, int iColorFormat, int iImage)
{    
	DBG_LOG_IN();
	int res = 1;
	
	OMX_ERRORTYPE oerr;
	omx_add_cmd_in_list(CompNum, OMX_EventParamOrConfigChanged, OMX_CommandAny);
	
	OMX_CONFIG_REQUESTCALLBACKTYPE cbtype;
	OMX_INIT_STRUCTURE(cbtype);
	cbtype.nPortIndex 	= OMX_ALL;
	cbtype.nIndex		= OMX_IndexParamCameraDeviceNumber;
	cbtype.bEnable		= OMX_TRUE;
	
	oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigRequestCallback, &cbtype);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	//omx_wait_exec_cmd(CompNum);
	
	OMX_PARAM_U32TYPE device;
	OMX_INIT_STRUCTURE(device);
	device.nPortIndex 	= OMX_ALL;
	device.nU32			= CAM_DEVICE_NUMBER;
	
	oerr = OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamCameraDeviceNumber, &device);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	
	omx_wait_exec_cmd(CompNum);	
	
	if (iImage == OMX_FALSE)
	{		
		OMX_PARAM_PORTDEFINITIONTYPE portdef;
		OMX_INIT_STRUCTURE(portdef);
		portdef.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
		oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
		portdef.format.video.nFrameWidth  = iFrameWidth;
		portdef.format.video.nFrameHeight = iFrameHeight;
		portdef.format.video.xFramerate   = iFramerate << 16;
		portdef.format.video.nStride = (iFrameWidth+31)&~31;
		portdef.format.video.nSliceHeight = (iFrameHeight+15)&~15;
		
		//portdef.format.video.nStride      = (portdef.format.video.nFrameWidth + portdef.nBufferAlignment - 1) & (~(portdef.nBufferAlignment - 1));
		portdef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
		portdef.format.video.eColorFormat = iColorFormat; //OMX_COLOR_FormatYUV420PackedPlanar;
		oerr = OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
		// Copy configure from 1 to 2 port
		OMX_INIT_STRUCTURE(portdef);
		portdef.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
		oerr = OMX_GetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
		portdef.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_2].number;
		oerr = OMX_SetParameter(m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
		// Configure frame rate
		OMX_CONFIG_FRAMERATETYPE framerate;
		OMX_INIT_STRUCTURE(framerate);
		framerate.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
		framerate.xEncodeFramerate = portdef.format.video.xFramerate;
		oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigVideoFramerate, &framerate);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
		framerate.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_2].number;
		oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigVideoFramerate, &framerate);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
		// Configure exposure value
	/*	OMX_CONFIG_EXPOSUREVALUETYPE exposure_value;
		OMX_INIT_STRUCTURE(exposure_value);
		exposure_value.nPortIndex = OMX_ALL;
		exposure_value.xEVCompensation = CAM_EXPOSURE_COMPENSATION;
		exposure_value.bAutoSensitivity = CAM_EXPOSURE_AUTO_SENSITIVITY;
		exposure_value.nSensitivity = CAM_EXPOSURE_ISO_SENSITIVITY;
		oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonExposureValue, &exposure_value);
		if (OMX_TEST_ERROR(oerr)) res = 0;		*/	
	}
	else
	{
	  /*// Configure Sensor Mode
	  OMX_PARAM_SENSORMODETYPE sensormode;
	  OMX_INIT_STRUCTURE(sensormode);
      sensormode.nPortIndex = OMX_ALL;
      sensormode.bOneShot = OMX_TRUE;
      sensormode.nFrameRate = portdef.format.video.xFramerate;
      oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexParamCommonSensorMode, &sensormode);
	  if (OMX_TEST_ERROR(oerr)) res = 0;	  */
	  OMX_PARAM_SENSORMODETYPE sensor;
      OMX_INIT_STRUCTURE (sensor);
      sensor.nPortIndex = OMX_ALL;
      OMX_INIT_STRUCTURE (sensor.sFrameSize);
      sensor.sFrameSize.nPortIndex = OMX_ALL;
      oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamCommonSensorMode, &sensor);
	  if (OMX_TEST_ERROR(oerr)) res = 0;
      sensor.bOneShot = OMX_TRUE;
      sensor.sFrameSize.nWidth = iFrameWidth;
      sensor.sFrameSize.nHeight = iFrameHeight;
      oerr = OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamCommonSensorMode, &sensor);
	  if (OMX_TEST_ERROR(oerr)) res = 0;
	  //}
	  OMX_PARAM_PORTDEFINITIONTYPE port_def_i;
	  OMX_INIT_STRUCTURE (port_def_i);
	  port_def_i.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_3].number;
      oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def_i);
	  if (OMX_TEST_ERROR(oerr)) res = 0;
	  port_def_i.format.image.nFrameWidth = iFrameWidth;
	  port_def_i.format.image.nFrameHeight = iFrameHeight;
	  port_def_i.format.image.nStride = (iFrameWidth+31)&~31;
	  port_def_i.format.image.nSliceHeight = (iFrameHeight+15)&~15;
	  port_def_i.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
	  port_def_i.format.image.eColorFormat = iColorFormat;
	  //Stride is byte-per-pixel*width, YUV has 1 byte per pixel, so the stride is
	  //the width (rounded up to the nearest multiple of 16).
	  //See mmal/util/mmal_util.c, mmal_encoding_width_to_stride()
	  port_def_i.format.image.nStride = (port_def_i.format.image.nFrameWidth + port_def_i.nBufferAlignment - 1) & (~(port_def_i.nBufferAlignment - 1));;
	  oerr = OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def_i);
	  if (OMX_TEST_ERROR(oerr)) res = 0;
	  port_def_i.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
	  port_def_i.format.video.nFrameWidth = 640;
	  port_def_i.format.video.nFrameHeight = 480;
	  port_def_i.format.video.nStride = (640+31)&~31;
	  port_def_i.format.video.nSliceHeight = (480+15)&~15;
	  port_def_i.format.video.eCompressionFormat = OMX_IMAGE_CodingUnused;
	  port_def_i.format.video.eColorFormat = VIDEO_COLOR_FORMAT;
	  //Setting the framerate to 0 unblocks the shutter speed from 66ms to 772ms
	  //The higher the speed, the higher the capture time
	  port_def_i.format.video.xFramerate = 0;
	  port_def_i.format.video.nStride = 640;
	  oerr = OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &port_def_i);
	  if (OMX_TEST_ERROR(oerr)) res = 0;
	} 
	
	//Exposure value
		OMX_CONFIG_EXPOSUREVALUETYPE exposure_value_st;
		OMX_INIT_STRUCTURE (exposure_value_st);
		exposure_value_st.nPortIndex = OMX_ALL;
		exposure_value_st.eMetering = CAM_METERING;
		exposure_value_st.xEVCompensation = (CAM_EXPOSURE_COMPENSATION << 16)/6;
		exposure_value_st.nShutterSpeedMsec = CAM_SHUTTER_SPEED;
		exposure_value_st.bAutoShutterSpeed = CAM_SHUTTER_SPEED_AUTO;
		exposure_value_st.nSensitivity = CAM_EXPOSURE_ISO_SENSITIVITY;
		exposure_value_st.bAutoSensitivity = CAM_EXPOSURE_AUTO_SENSITIVITY;
		exposure_value_st.nApertureFNumber = 0;
		exposure_value_st.bAutoAperture = 0;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonExposureValue, &exposure_value_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		
 	  /*	//Color enhancement
		OMX_CONFIG_COLORENHANCEMENTTYPE color_enhancement_st;
		OMX_INIT_STRUCTURE (color_enhancement_st);
		color_enhancement_st.nPortIndex = OMX_ALL;
		color_enhancement_st.bColorEnhancement = CAM_COLOR_ENABLE;
		color_enhancement_st.nCustomizedU = CAM_COLOR_U;
		color_enhancement_st.nCustomizedV = CAM_COLOR_V;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonColorEnhancement, &color_enhancement_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;
  
		//Denoise
		OMX_CONFIG_BOOLEANTYPE denoise_st;
		OMX_INIT_STRUCTURE (denoise_st);
		denoise_st.bEnabled = CAM_NOISE_REDUCTION;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigStillColourDenoiseEnable, &denoise_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;
  
		//DRC
		OMX_CONFIG_DYNAMICRANGEEXPANSIONTYPE drc_st;
		OMX_INIT_STRUCTURE (drc_st);
		drc_st.eMode = CAM_DRC;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigDynamicRangeExpansion, &drc_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;
	//}
	*/
  
	DBG_LOG_OUT();
	return res; 
}

int omx_set_iso_value(int CompNum, int iExpCompens, int iAutoISO, int iValueISO)
{
	int res = 1;
	OMX_CONFIG_EXPOSUREVALUETYPE exposure_value_st;
	OMX_INIT_STRUCTURE (exposure_value_st);
	exposure_value_st.nPortIndex = OMX_ALL;
	exposure_value_st.eMetering = CAM_METERING;
	exposure_value_st.xEVCompensation = (iExpCompens << 16)/6;
	exposure_value_st.nShutterSpeedMsec = CAM_SHUTTER_SPEED;
	exposure_value_st.bAutoShutterSpeed = CAM_SHUTTER_SPEED_AUTO;
	exposure_value_st.nSensitivity = iValueISO;
	exposure_value_st.bAutoSensitivity = iAutoISO;
	exposure_value_st.nApertureFNumber = 0;
	exposure_value_st.bAutoAperture = 0;
	OMX_ERRORTYPE oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonExposureValue, &exposure_value_st);
	if (OMX_TEST_ERROR(oerr)) res = 0;	
	return res;	
}

int omx_set_flip_value(int CompNum, int iFlipHor, int iFlipVert, int iImage)
{
	int res = 1;
	OMX_ERRORTYPE oerr;
	// Configure mirror
    OMX_MIRRORTYPE eMirror = OMX_MirrorNone;
    if(iFlipHor && !iFlipVert) {
        eMirror = OMX_MirrorHorizontal;
    } else if(!iFlipHor && iFlipVert) {
        eMirror = OMX_MirrorVertical;
    } else if(iFlipHor && iFlipVert) {
        eMirror = OMX_MirrorBoth;
    }
    OMX_CONFIG_MIRRORTYPE mirror;
    OMX_INIT_STRUCTURE(mirror);
    mirror.eMirror = eMirror;
    if (iImage == OMX_TRUE) 
	{
		mirror.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_3].number;
		oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonMirror, &mirror);
		if (OMX_TEST_ERROR(oerr)) res = 0;
	}
	else 
	{
		mirror.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
		oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonMirror, &mirror);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		mirror.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_2].number;
		oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonMirror, &mirror);
		if (OMX_TEST_ERROR(oerr)) res = 0;
	}    
	return res;
}

int omx_set_rotate_value(int CompNum, int iRotAngle, int iImage)
{
	int res = 1;
	OMX_ERRORTYPE oerr;
	//Rotation
	switch(iRotAngle)
	{
		case 0: iRotAngle = 0; break;
		case 1: iRotAngle = 90; break;
		case 2: iRotAngle = 180; break;
		case 3: iRotAngle = 270; break;
		case 90: 
		case 180: 
		case 270: 
			break;
		default: iRotAngle = 0; break;
	}
	
	OMX_CONFIG_ROTATIONTYPE rotation_st;
	OMX_INIT_STRUCTURE (rotation_st);
	rotation_st.nRotation = iRotAngle;
	if (iImage == OMX_TRUE) 
	{
		rotation_st.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_3].number;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonRotate, &rotation_st);	
		if (OMX_TEST_ERROR(oerr)) res = 0;
	}
	else 
	{
		rotation_st.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_1].number;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonRotate, &rotation_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;		
		rotation_st.nPortIndex = m_oCompList[CompNum].out_port[OMX_PORT_2].number;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonRotate, &rotation_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;
	}  
	return res;
}	

int omx_set_exposure_value(int CompNum, int eExposureControl)
{
	OMX_CONFIG_EXPOSURECONTROLTYPE exposure_control_st;
	OMX_INIT_STRUCTURE (exposure_control_st);
	exposure_control_st.nPortIndex = OMX_ALL;
	exposure_control_st.eExposureControl = eExposureControl;
	OMX_ERRORTYPE oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCommonExposure, &exposure_control_st);
	if (OMX_TEST_ERROR(oerr)) return 0;	
	return 1;
}

int omx_set_imagefilter_value(int CompNum, int eFilter)
{
	int res = 1;
	// Configure image filter
    OMX_CONFIG_IMAGEFILTERTYPE image_filter;
    OMX_INIT_STRUCTURE(image_filter);
    image_filter.nPortIndex = OMX_ALL;
    image_filter.eImageFilter = eFilter;
    OMX_ERRORTYPE oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonImageFilter, &image_filter);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	return res;
}

int omx_set_whitebalance_value(int CompNum, int eBalance)
{
	int res = 1;
	// Configure frame white balance control
    OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_control;
    OMX_INIT_STRUCTURE(white_balance_control);
    white_balance_control.nPortIndex = OMX_ALL;
    white_balance_control.eWhiteBalControl = eBalance;
    OMX_ERRORTYPE oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonWhiteBalance, &white_balance_control);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	 //White balance gains (if white balance is set to off)
    if (!eBalance)
	{
		OMX_CONFIG_CUSTOMAWBGAINSTYPE white_balance_gains_st;
		OMX_INIT_STRUCTURE (white_balance_gains_st);
		white_balance_gains_st.xGainR = (CAM_WHITE_BALANCE_RED_GAIN << 16)/1000;
		white_balance_gains_st.xGainB = (CAM_WHITE_BALANCE_BLUE_GAIN << 16)/1000;
		oerr = OMX_SetConfig (m_oCompList[CompNum].handle, OMX_IndexConfigCustomAwbGains, &white_balance_gains_st);
		if (OMX_TEST_ERROR(oerr)) res = 0;
	}    
	return res;
}

int omx_set_bright_value(int CompNum, int eBrightControl)
{
	OMX_CONFIG_BRIGHTNESSTYPE brightness;
    OMX_INIT_STRUCTURE(brightness);
    brightness.nPortIndex = OMX_ALL;
    brightness.nBrightness = eBrightControl;
    OMX_ERRORTYPE oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonBrightness, &brightness);
	if (OMX_TEST_ERROR(oerr)) return 0;
	return 1;
}

int omx_set_contrast_value(int CompNum, int eContrastControl)
{	
	// Configure contrast
	int res = 1;
    OMX_CONFIG_CONTRASTTYPE contrast;
    OMX_INIT_STRUCTURE(contrast);
    contrast.nPortIndex = OMX_ALL;
    contrast.nContrast = eContrastControl;
    OMX_ERRORTYPE oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonContrast, &contrast);
	if (OMX_TEST_ERROR(oerr)) res = 0;	
	
	return res;
}

int omx_set_sharpness_value(int CompNum, int eSharpnessControl)
{
	// Configure sharpness
	int res = 1;
    OMX_CONFIG_SHARPNESSTYPE sharpness;
    OMX_INIT_STRUCTURE(sharpness);
    sharpness.nPortIndex = OMX_ALL;
    sharpness.nSharpness = eSharpnessControl;
    OMX_ERRORTYPE oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonSharpness, &sharpness);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	
	return res;
}

int omx_set_saturation_value(int CompNum, int eSaturationControl)
{
	// Configure saturation
	int res = 1;
    OMX_CONFIG_SATURATIONTYPE saturation;
    OMX_INIT_STRUCTURE(saturation);
    saturation.nPortIndex = OMX_ALL;
    saturation.nSaturation = eSaturationControl;
    OMX_ERRORTYPE oerr = OMX_SetConfig(m_oCompList[CompNum].handle, OMX_IndexConfigCommonSaturation, &saturation);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	return res;
}

int omx_set_count_buffers(int CompNum, int port, int port_type, int count, OMX_U32 sizebuff)
{
	DBG_LOG_IN();
	int res = 1;
	int port_cnt, port_num, n;
	omx_ports *pport;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port) || (count>OMX_BUFFERS_MAX)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_ERRORTYPE oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
	if (OMX_TEST_ERROR(oerr)) res = 0;
	dbgprintf(8,"Get buffers comp:%i, count:%i, port:%i, size:%i\n",CompNum, portdef.nBufferCountActual, port_num, portdef.nBufferSize);
    
    if (sizebuff > portdef.nBufferSize) portdef.nBufferSize = sizebuff;
    if (count > 0) portdef.nBufferCountActual = count; 
	dbgprintf(8,"Set buffers comp:%i, count:%i, port:%i, size:%i\n",CompNum, count, port_num, portdef.nBufferSize);
    oerr = OMX_SetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    if (OMX_TEST_ERROR(oerr)) res = 0;
	pport[port].buffers_count = count;
    pport[port].buffers_size = portdef.nBufferSize;
    for (n= 0; n != count; n++) pport[port].buffer[n] = NULL;
	
	oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
	if (OMX_TEST_ERROR(oerr)) res = 0;
    
	dbgprintf(8,"Get buffers comp:%i, count:%i, port:%i, size:%i\n",CompNum, portdef.nBufferCountActual, port_num, portdef.nBufferSize);
    	
    DBG_LOG_OUT();
	return res;
}

int omx_get_buffer_size(int CompNum, int port, int port_type, int *sizebuff)
{
	DBG_LOG_IN();
	int res = 1;
	int port_cnt, port_num;
	omx_ports *pport;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_ERRORTYPE oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
	if (OMX_TEST_ERROR(oerr)) res = 0;
    *sizebuff = portdef.nBufferSize;	
    
	DBG_LOG_OUT();
	return res;
}

int omx_get_buffer_count(int CompNum, int port, int port_type, int *cnt)
{
	DBG_LOG_IN();
	
	int port_cnt, port_num;
	int res = 1;
	omx_ports *pport;
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_ERRORTYPE oerr = OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
	if (OMX_TEST_ERROR(oerr)) res = 0;
    *cnt = portdef.nBufferCountActual;	
    
	DBG_LOG_OUT();
	return res;
}

int omx_create_buffers(int CompNum, int port, int port_type)
{
	DBG_LOG_IN();
	
	int port_cnt, port_num, i;
	int res = 1;
	omx_ports *pport;
	dbgprintf(8,"Create buffer1:\n");
	
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    
    pport[port].buffers_count = portdef.nBufferCountActual;
    pport[port].buffers_size = portdef.nBufferSize;
  
    for (i = 0; i != portdef.nBufferCountActual; i++)
    {
		//if (pport[port].buffer[i] != NULL) return -1;
		dbgprintf(8,"Create buffer comp:%i, num:%i, port:%i, size:%i\n", CompNum, i+1, port_num, portdef.nBufferSize);
		OMX_ERRORTYPE oerr = OMX_AllocateBuffer(m_oCompList[CompNum].handle, &pport[port].buffer[i], port_num, NULL, portdef.nBufferSize);
		if (OMX_TEST_ERROR(oerr)) {res = 0; break;}
		//pport[port].buffer[i]->pAppPrivate = (void*)((port_num<<8)+i);
		pport[port].buffer[i]->pAppPrivate = (void*)(intptr_t)CompNum;
	}
    
	DBG_LOG_OUT();
	return res;
}

int omx_test_buffers(int CompNum, int port, int port_type, unsigned int iLine)
{
	DBG_LOG_IN();
	
	int port_cnt, port_num, i;
	omx_ports *pport;
	dbgprintf(8,"Create buffer1:\n");
	int res = 1; 
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
	unsigned int addr;
    for (i = 0; i != pport[port].buffers_count; i++)
    {
		addr = (unsigned int)(intptr_t)(pport[port].buffer[i]->pBuffer);
		if (addr < 100000) 
		{
			res = 0;
			dbgprintf(1,"Error buffer in line %i, comp %i, port %i, num %i, addr: %i, addrnow: %i\n", iLine, CompNum, port_num, i, addr, (unsigned int)(intptr_t)pport[port].buffer[i]->pBuffer);
		}
	} 
	DBG_LOG_OUT();
	return res;
}

int omx_create_buffers2(int CompNum, int port, int port_type)
{
	DBG_LOG_IN();
	
	int port_cnt, port_num, i;
	omx_ports *pport;
		
    if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    
    if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
    port_num = pport[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = port_num;
    OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef);
    
    pport[port].buffers_count = portdef.nBufferCountActual;
    pport[port].buffers_size = portdef.nBufferSize;
  
    for (i = 0; i != portdef.nBufferCountActual; i++)
    {
		dbgprintf(8,"Create buffer %i, port:%i, size:%i\n", i, port_num, portdef.nBufferSize);
		//if (pport[port].buffer[i] != NULL) return -1;
		posix_memalign(&pport[port].buffer_ptr[i], portdef.nBufferAlignment, portdef.nBufferSize);
		//assert(pport[port].buffer_ptr[i]==NULL);
		//pport[port].buffer_ptr[10] = DBG_MALLOC(portdef.nBufferSize);
		int err = OMX_UseBuffer(m_oCompList[CompNum].handle, &pport[port].buffer[i], port_num, pport, portdef.nBufferSize, pport[port].buffer_ptr[i]);
		if (err != OMX_ErrorNone)
		{
			dbgprintf(8,"Error OMX_UseBuffer : %i\n",err);
			DBG_LOG_OUT();
			return -2;
		}
		pport[port].buffer_empty[i] = 1;
	}    
    
	DBG_LOG_OUT();
	return 1;
} 

int omx_use_buffer(int CompNumOut, int portout, int CompNumIn, int portin)
{
	DBG_LOG_IN();
	
	omx_ports *pportin, *pportout;

    pportin = m_oCompList[CompNumIn].in_port;
	pportout = m_oCompList[CompNumOut].out_port;    
   //port_num = pportin[port].number;
    
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = pportin[portin].number;
    OMX_GetParameter (m_oCompList[CompNumIn].handle, OMX_IndexParamPortDefinition, &portdef);
    
    pportout[portout].buffers_count = portdef.nBufferCountActual;
    pportout[portout].buffers_size = portdef.nBufferSize;
    int i;
    for (i = 0; i != portdef.nBufferCountActual; i++)
    {
		dbgprintf(8,"Use buffer %i, port:%i, size:%i\n", i, pportout[portin].number, portdef.nBufferSize);
		//posix_memalign(&pport[port].buffer_ptr[i], portdef.nBufferAlignment, portdef.nBufferSize);
		//pportin[portin].buffer_ptr[i] = pportout[portout].buffer_ptr[i];
		int err = OMX_UseBuffer(m_oCompList[CompNumOut].handle, &pportout[portout].buffer[i], pportout[portout].number, pportout, portdef.nBufferSize, pportin[portin].buffer[i]->pBuffer);
		if (err != OMX_ErrorNone)
		{
			dbgprintf(8,"Error OMX_UseBuffer : %i\n",err);
			DBG_LOG_OUT();
			return -2;
		}
		pportout[portout].buffer_empty[i] = 1;
	}    
    
	DBG_LOG_OUT();
	return 1;
} 

int omx_remove_buffers (int CompNum, int port, int port_type) 
{
	DBG_LOG_IN();
	int res = 1;
	int port_cnt, port_num, i;
	omx_ports *pport;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
	if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
	if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
	port_num = pport[port].number;
	for (i = 0; i < pport[port].buffers_count; i++) 
	{
		dbgprintf(8,"Remove buffer comp:%i, num:%i, port:%i\n", CompNum, i+1, port_num);		
		OMX_ERRORTYPE oerr = OMX_FreeBuffer (m_oCompList[CompNum].handle, port_num, pport[port].buffer[i]);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		//vcos_DBG_FREE( (m_oCompList[CompNum].in_port[port].buffer[i]);
	}
	pport[port].buffers_count = 0;
  
	DBG_LOG_OUT();
	return res;
}

int omx_activate_buffers(int CompNum, int port, int port_type, int iFlags)
{
  DBG_LOG_IN();
	
	int port_cnt, i;
	int res = 1;
  omx_ports *pport;
  if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
  if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
  if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
  //port_num = pport[port].number;
  /*OMX_PARAM_PORTDEFINITIONTYPE portdef;
  portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  portdef.nVersion.nVersion = OMX_VERSION;
  portdef.nPortIndex = port_num;
  DBG_OERR(OMX_GetParameter (m_oCompList[CompNum].handle, OMX_IndexParamPortDefinition, &portdef));*/
  
	if (pport[port].buffers_count>OMX_BUFFERS_MAX) {DBG_LOG_OUT();return 0;}
	// for (i = 0; i < pport[port].buffers_count; i++)
	i = pport[port].buffer_active; 
  
	if (iFlags != 0) pport[port].buffer[i]->nFlags = iFlags;
	
	
	if (port_type == OMX_PORT_IN) 
	{			
		dbgprintf(8,"buffer activate to empty %i CompNum:%i App:%i\n",i,CompNum, (int)(intptr_t)pport[port].buffer[i]->pAppPrivate);
		//pport[port].buffer[i]->pAppPrivate = (void*)CompNum;
		//pport[port].buffer[i]->nFilledLen = 0;
		//printf("acc i %i, %i\n", i,pport[port].buffers_count);
		OMX_ERRORTYPE oerr = OMX_EmptyThisBuffer(m_oCompList[CompNum].handle, pport[port].buffer[i]);
		if (OMX_TEST_ERROR(oerr)) res = 0;
		//CallBackEmpty(NULL, (OMX_PTR)CompNum, pport[port].buffer[i]);
		//pport[port].buffer_active;
	}
	else
	{		
		dbgprintf(8,"buffer activate to fill %i CompNum:%i\n",i,CompNum);	//printf("acc o %i\n",pport[port].buffers_count);
		pport[port].buffer[i]->nFilledLen = 0;
		pport[port].buffer[i]->nOffset = 0;
		OMX_ERRORTYPE oerr = OMX_FillThisBuffer(m_oCompList[CompNum].handle, pport[port].buffer[i]);
		if (OMX_TEST_ERROR(oerr)) res = 0;				
	}	
	
	DBG_LOG_OUT();
	return res;
}

int omx_switch_next_buffer(int CompNum, int port, int port_type)
{
	DBG_LOG_IN();
	
	int port_cnt;
	omx_ports *pport;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
	if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
	if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
  
	if (pport[port].buffers_count > OMX_BUFFERS_MAX) {DBG_LOG_OUT();return 0;}
	
	pport[port].buffer_active++;
	if (pport[port].buffer_active >= pport[port].buffers_count) pport[port].buffer_active = 0;
	
	DBG_LOG_OUT();
	return 1;
}

OMX_BUFFERHEADERTYPE * omx_get_active_buffer(int CompNum, int port, int port_type, int iTimeOut_ms)
{
	DBG_LOG_IN();
	
	int port_cnt, i;
	int iTimeOut = iTimeOut_ms;
	omx_ports *pport;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
			else port_cnt = m_oCompList[CompNum].out_port_cnt;
	if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return NULL;}
	if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
			else pport = m_oCompList[CompNum].out_port;    
	//port_num = pport[port].number;
	//uint64_t previous_ms = 0;
	//get_ms(&previous_ms);
				
	if (pport[port].buffers_count>OMX_BUFFERS_MAX) {DBG_LOG_OUT();return NULL;}
	// for (i = 0; i < pport[port].buffers_count; i++)
	i = pport[port].buffer_active;
	
	if ((port_type == OMX_PORT_IN) && (iTimeOut_ms > 0))
	{
		int timer_clk = 0;
		//dbgprintf(8,"omx_get_active_buffer0 %i CompNum:%i\n", i, CompNum);	
		while (pport[port].buffer[i]->nFilledLen != 0) 
		{
			usleep(1000);			
			timer_clk++;
			if (timer_clk >= iTimeOut) 
			{
				dbgprintf(8,"Timeout get omx buffer\n");
				DBG_LOG_OUT();				
				return NULL;
			}
		}	
		//printf("wait done next buff comp %i, %ims from %ims\n", CompNum, timer_clk, iTimeOut_ms);
	}
	//uint64_t previous_ms = 0;
	//get_ms(&previous_ms);
	//printf("omx_get_active_buffer %i, %i ms\n", i, (uint32_t)get_ms(&previous_ms));
	
	dbgprintf(8,"omx_get_active_buffer1 %i CompNum:%i\n", i, CompNum);
	
	DBG_LOG_OUT();
	return pport[port].buffer[i];
}

int omx_IsFree_Encoder()
{
	//return tx_semaphore_wait_list_empty(&psem_omx_run);
	int ret;
	DBG_MUTEX_LOCK(&OMX_mutex);		
	ret = cThreadOmxEncoderStatus;
	DBG_MUTEX_UNLOCK(&OMX_mutex);		
	if (ret == 0) return 1; else return 0;
	//if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_CAPTURE, TX_ANY) == 0)  return 1;	else return 0;
}

int omx_IsFree_Capture()
{
	//return tx_semaphore_wait_list_empty(&psem_omx_run);
	int ret;
	DBG_MUTEX_LOCK(&OMX_mutex);		
	ret = cThreadOmxCaptureStatus;
	DBG_MUTEX_UNLOCK(&OMX_mutex);		
	if (ret == 0) return 1; else return 0;
	//if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_CAPTURE, TX_ANY) == 0)  return 1;	else return 0;
}

int omx_IsFree_Video()
{
	//return tx_semaphore_wait_list_empty(&psem_omx_run);
	int ret;
	DBG_MUTEX_LOCK(&OMX_mutex);		
	ret = cThreadOmxPlayStatus;
	DBG_MUTEX_UNLOCK(&OMX_mutex);		
	if (ret == 0) return 1; else return 0;
	//if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY) == 0) return 1;	else return 0;
}

int omx_IsFree_Image()
{
	//return tx_semaphore_wait_list_empty(&psem_omx_run);
	int ret;
	DBG_MUTEX_LOCK(&OMX_mutex);		
	ret = cThreadOmxImageStatus;
	DBG_MUTEX_UNLOCK(&OMX_mutex);		
	if (ret == 0) return 1; else return 0;
	//if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY) == 0) return 1;	else return 0;
}

int omx_IsFree_Audio()
{
	//return tx_semaphore_wait_list_empty(&psem_omx_run);
	if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_AUDIO, TX_ANY) == 0) return 1;	else return 0;
}

int omx_IsFree()
{
	if ((omx_IsFree_Video() == 1) && (omx_IsFree_Audio() == 1)) return 1;	else return 0;
}

int omx_wait_exec_cmd(int CompNum)
{	
	//printf("Wait comp %i\n",CompNum);
	//omx_print_wait_list(CompNum);
	return tx_semaphore_wait(m_oCompList[CompNum].psem);
}

int omx_wait_event_timeout(int CompNum, unsigned int event, unsigned int cmd, int timeout_ms)
{
	return tx_semaphore_wait_event_timeout(m_oCompList[CompNum].psem, event, cmd, timeout_ms);
}

int omx_wait_exec_cmd_timeout(int CompNum, int timeout_ms)
{	
	int timerclk = 0;
	while(1)
	{
		if (tx_semaphore_wait_list_empty(m_oCompList[CompNum].psem) == 0) 
		{
			usleep(1000); 
			timerclk++;
		} 
		else return 1;
		if (timerclk >= (timeout_ms*1000)) return 0;
	}
}

int omx_wait_exec_spec_cmd_timeout(int CompNum, int timeout_ms)
{	
	int timerclk = 0;
	while(1)
	{
		if (tx_semaphore_wait_spec_list_empty(m_oCompList[CompNum].psem) == 0) {usleep(1); timerclk++;} else return 1;
		if (timerclk >= (timeout_ms*1000)) return 0;
	}
}

int omx_wait_exec_spec_cmd(int CompNum)
{
	tx_semaphore_wait_spec(m_oCompList[CompNum].psem);
	return 1;
}

int omx_wait_list_empty(int CompNum)
{	
	return tx_semaphore_wait_list_empty(m_oCompList[CompNum].psem);
}

int omx_wait_spec_list_empty(int CompNum)
{	
	return tx_semaphore_wait_spec_list_empty(m_oCompList[CompNum].psem);
}

int omx_reset_exec_cmd(int CompNum)
{
	tx_semaphore_reset(m_oCompList[CompNum].psem);
	return 1;
}

int omx_empty_all_buffers(int CompNum, int port, int port_type)
{
	DBG_LOG_IN();
	
	int port_cnt, i;
	int res = 1;
	omx_ports *pport;
	if (port_type == OMX_PORT_IN) port_cnt = m_oCompList[CompNum].in_port_cnt;
		else port_cnt = m_oCompList[CompNum].out_port_cnt;
	if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
	if (port_type == OMX_PORT_IN) pport = m_oCompList[CompNum].in_port;
		else pport = m_oCompList[CompNum].out_port;    
	for (i = 0; i != pport[port].buffers_count; i++)
	{
		OMX_ERRORTYPE oerr = OMX_EmptyThisBuffer(m_oCompList[CompNum].handle, pport[port].buffer[i]);
		if (OMX_TEST_ERROR(oerr)) res = 0;
	}
	
	DBG_LOG_OUT();
	return res;
}

int omx_set_tunnel(int Comp1, int Port1, int Comp2, int Port2, int iPortEnable)
{
	DBG_LOG_IN();
	
	if ((Comp1>=OMX_COMPONENTS_MAX) 
		|| (m_oCompList[Comp1].out_port_cnt<=Port1) 
		|| (Comp2>=OMX_COMPONENTS_MAX)) {DBG_LOG_OUT();return 0;}
	if (Comp2 != -1) if (m_oCompList[Comp2].in_port_cnt<=Port2) {DBG_LOG_OUT();return 0;}
	if (Comp2 != -1) 
	{
			OMX_ERRORTYPE oerr = OMX_SetupTunnel (m_oCompList[Comp1].handle, m_oCompList[Comp1].out_port[Port1].number, m_oCompList[Comp2].handle, m_oCompList[Comp2].in_port[Port2].number);
			if (OMX_TEST_ERROR(oerr)) { DBG_LOG_OUT(); return 0;}
			if (iPortEnable == OMX_TRUE)
			{
				omx_enable_port(Comp1, Port1, OMX_PORT_OUT, OMX_LATER);   
				omx_enable_port(Comp2, Port2, OMX_PORT_IN, OMX_LATER);   
				//	omx_print_wait_list(Comp1);
				//omx_print_wait_list(Comp2);
				omx_wait_exec_cmd(Comp1);
				//	omx_delete_cmd_from_list(Comp1, OMX_EventCmdComplete, OMX_CommandPortEnable); 
				omx_wait_exec_cmd(Comp2);
			}
			dbgprintf(8,"Tunnel enabled\n");
	}
	else
	{
		OMX_ERRORTYPE oerr = OMX_SetupTunnel (m_oCompList[Comp1].handle, m_oCompList[Comp1].out_port[Port1].number, NULL, 0);  
		if (OMX_TEST_ERROR(oerr)) { DBG_LOG_OUT(); return 0;}
	}
	
	DBG_LOG_OUT();
	return 1;
} 

OMX_HANDLETYPE omx_get_handle(int CompNum)
{
	return m_oCompList[CompNum].handle;
}

void omx_set_handle(int CompNum, OMX_HANDLETYPE hndl)
{
	m_oCompList[CompNum].handle = hndl;
}
/*
void omx_enable_source_buffer(int CompNum, int port, void* buffer, int iSize, int iFlag)
{
	int port_cnt;
    omx_ports *pport;
    port_cnt = m_oCompList[CompNum].in_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) return 0;
    pport = m_oCompList[CompNum].in_port;
    DBG_MUTEX_LOCK(&m_oCompList[CompNum].psem->mutex);
    m_oCompList[CompNum].in_port[port].source_buffer = buffer;	
	m_oCompList[CompNum].in_port[port].source_buffer_data_size = iSize;
	m_oCompList[CompNum].in_port[port].source_buffer_flag = iFlag;
	m_oCompList[CompNum].in_port[port].source_buffer_position = 0;
	DBG_MUTEX_UNLOCK(&m_oCompList[CompNum].psem->mutex);  
}

void omx_disable_source_buffer(int CompNum, int port)
{
	int port_cnt;
    omx_ports *pport;
    port_cnt = m_oCompList[CompNum].in_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) return 0;
    pport = m_oCompList[CompNum].in_port;
    DBG_MUTEX_LOCK(&m_oCompList[CompNum].psem->mutex);
    m_oCompList[CompNum].in_port[port].source_buffer = NULL;	
	m_oCompList[CompNum].in_port[port].source_buffer_data_size = 0;
	m_oCompList[CompNum].in_port[port].source_buffer_flag = 0;
	m_oCompList[CompNum].in_port[port].source_buffer_position = 0;
	DBG_MUTEX_UNLOCK(&m_oCompList[CompNum].psem->mutex);
  
}*/

int omx_use_egl_buffer(int CompNum)
{
	DBG_LOG_IN();
	
	int port_cnt;
    int port = OMX_PORT_1;
	omx_ports *pport;
    port_cnt = m_oCompList[CompNum].out_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
    pport = m_oCompList[CompNum].out_port;
    //pport = &m_oCompList[CompNum].out_port[OMX_PORT_1];
	pport->buffers_count = 1;
    OMX_ERRORTYPE oerr = OMX_UseEGLImage (m_oCompList[CompNum].handle, &pport[port].buffer[0], pport[port].number, NULL, pport[port].EglImage);
  	if (OMX_TEST_ERROR(oerr)) { DBG_LOG_OUT(); return 0;}
	
	DBG_LOG_OUT();
	return 1;
}

int omx_create_egl_buffer(int CompNum, int width, int height, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext)
{
	DBG_LOG_IN();
	
	int port_cnt;
	int port = OMX_PORT_1;
	omx_ports *pport;
	port_cnt = m_oCompList[CompNum].out_port_cnt;
	if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
	pport = m_oCompList[CompNum].out_port;
	glBindTexture (GL_TEXTURE_2D, texture_id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//GL_LINEAR
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	dbgprintf(8,"create gl image Disp: %i, Cont:%i, Text:%i, W:%i,H:%i\n",(int)(intptr_t)(*eglDisplay), (int)(intptr_t)(*eglContext), texture_id,width,height);
	pport[port].EglImage = (void *)eglCreateImageKHR (*eglDisplay, *eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(intptr_t)texture_id, 0);
	if (pport[port].EglImage == EGL_NO_IMAGE_KHR)
	{
		dbgprintf(1,"dont create gl image Disp: %i, Cont:%i, Text:%i, W:%i,H:%i\n",(int)(intptr_t)(*eglDisplay), (int)(intptr_t)(*eglContext), texture_id,width,height);
		dbgprintf(1,"get egl errors : %i(w:%i,h:%i)\n",eglGetError(),width, height);		
		pport[port].EglImage = NULL;
		
		DBG_MUTEX_LOCK(&system_mutex);
		if (uiErrorVideoRestart) 
		{
			unsigned int uitWait = uiErrorVideoRestartWait;
			DBG_MUTEX_UNLOCK(&system_mutex);
			if (uitWait) usleep(uitWait * 1000);
			if (uiErrorVideoRestart == 1) System_Restart(NULL, 0); else System_Reboot(NULL, 0);
			uiErrorVideoRestart = 100;
		} else DBG_MUTEX_UNLOCK(&system_mutex);
		DBG_LOG_OUT();
		return 0;
	}
  
	DBG_LOG_OUT();
	return 1;
}

/*int omx_transfer_buffer(int CompNumOut, int portout, int CompNumIn, int portin, int iFlag)
{
	DBG_LOG_IN();
	
	omx_ports *pportout, *pportin;
    
	pportout = &m_oCompList[CompNumOut].out_port[portout];
    pportin = &m_oCompList[CompNumIn].in_port[portin];
    //omx_add_cmd_in_list_spec(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
    while (pportin->buffer[0]->nFilledLen!=0) usleep(10000);
	pportin->buffer[0]->nFilledLen = pportout->buffers_size;
	pportin->buffer[0]->nFlags = iFlag;
	pportin->buffer[0]->nOffset = 0;
	pportin->buffer[0]->pAppPrivate = &pportin->buffer_empty[0];
	OMX_EmptyThisBuffer(m_oCompList[CompNumIn].handle, pportin->buffer[0]);
	
	DBG_LOG_OUT();
  return 1;
}*/

/*int omx_copy_buffer(int CompNumOut, int portout, int CompNumIn, int portin, int iFlag)
{
	DBG_LOG_IN();
	
	omx_ports *pportout, *pportin;
    char *outbuff, *inbuff;
	pportout = &m_oCompList[CompNumOut].out_port[portout];
    pportin = &m_oCompList[CompNumIn].in_port[portin];
    //omx_add_cmd_in_list_spec(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
    while (pportin->buffer[0]->nFilledLen!=0) usleep(10000);
	pportin->buffer[0]->nFilledLen = pportout->buffer[0]->nFilledLen;
	pportin->buffer[0]->nFlags = iFlag;
	pportin->buffer[0]->nOffset = 0;
	pportin->buffer[0]->pAppPrivate = &pportin->buffer_empty[0];
	int i;
	outbuff = (char*)pportout->buffer[0]->pBuffer;
	inbuff = (char*)pportin->buffer[0]->pBuffer;
	for (i = 0; i != pportout->buffer[0]->nFilledLen; i++) inbuff[i] = 255;
	dbgprintf(8,"Copy data from comp:%i, port:%i to comp:%i, port:%i, size:%i\n",CompNumOut,pportout->number,CompNumIn,pportin->number, pportout->buffer[0]->nFilledLen);
	memcpy(inbuff, outbuff, pportout->buffer[0]->nFilledLen);
	OMX_EmptyThisBuffer(m_oCompList[CompNumIn].handle, pportin->buffer[0]);
	
	DBG_LOG_OUT();
  return 1;
}*/


int omx_load_data_in_buffer(int CompNum, int port, void *DataBuff, int iDataSize, int iFlag, int iFlagAuto, int iTimeOut_ms)
{
	DBG_LOG_IN();
	
	int port_cnt;
	int timer_clk = 0;
    omx_ports *pport;
    port_cnt = m_oCompList[CompNum].in_port_cnt;
    if ((CompNum>=OMX_COMPONENTS_MAX) || (port_cnt<=port)) {DBG_LOG_OUT();return 0;}
	//if (iDataSize == 0) return -2;
    pport = &m_oCompList[CompNum].in_port[port];
    //omx_add_cmd_in_list_spec(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
    int ret = 0;
    int iSize;
    int FirstPacket = 1;	
    int iPrevBuffer = pport->buffer_active;
    if (iPrevBuffer == 0) iPrevBuffer = pport->buffers_count;
    iPrevBuffer--;
    do
    {
		timer_clk = 0;
		//printf("buff %i\n", (int)pport->buffer[pport->buffer_active]);
		while (pport->buffer[pport->buffer_active]->nFilledLen != 0) 
		{
			usleep(10);			
			timer_clk++;
			if (timer_clk >= (iTimeOut_ms*100)) {DBG_LOG_OUT();return -1;}
		}
		iSize = pport->buffers_size;
		if (iFlagAuto == 1)
		{
			if (iFlag & OMX_BUFFERFLAG_TIME_UNKNOWN) iFlag = OMX_BUFFERFLAG_TIME_UNKNOWN;
			if ((FirstPacket == 1) && (iFlag & OMX_BUFFERFLAG_STARTTIME)) iFlag = OMX_BUFFERFLAG_STARTTIME;
			FirstPacket = 0;
			if ((iSize >= iDataSize) && (iFlag & OMX_BUFFERFLAG_EOS)) iFlag |= OMX_BUFFERFLAG_EOS;				
		}
		if (iSize >= iDataSize) iSize = iDataSize;	
		//memcpy(pport->buffer_ptr[pport->buffer_active], DataBuff, iSize);
		memcpy(pport->buffer[pport->buffer_active]->pBuffer, DataBuff, iSize);
		pport->buffer[pport->buffer_active]->nFilledLen = iSize;
		pport->buffer[pport->buffer_active]->nFlags = iFlag;
		pport->buffer[pport->buffer_active]->nOffset = 0;
		//pport->buffer[pport->buffer_active]->pAppPrivate = &pport->buffer_empty[pport->buffer_active];
		OMX_ERRORTYPE oerr = OMX_EmptyThisBuffer(m_oCompList[CompNum].handle, pport->buffer[pport->buffer_active]);
		if (OMX_TEST_ERROR(oerr)) {DBG_LOG_OUT();return -2;}
	
		//dbgprintf(8,"data loaded in buffer %i , len: %i, buffsize: %i\n",pport->buffer_active, iSize, pport->buffers_size);
		iDataSize -= iSize;	
		DataBuff +=  iSize;
		ret += iSize;
		pport->buffer_active++;
		if (pport->buffer_active >= pport->buffers_count) pport->buffer_active = 0;
		
    } while (iDataSize != 0);	
    
	DBG_LOG_OUT();
  return (pport->buffer_active);
}

int omx_play_video_file(char* cPath)
{
   DBG_LOG_IN();
	
	FILE *in;
	int res = -2;
   int iStatus = 0;
  // unsigned int data_len = 0; 
   if((in = fopen(cPath, "rb")) == NULL) {DBG_LOG_OUT();return -1;}
   dbgprintf(8,"File opened : %s\n", cPath);  
     
   if (m_iStarted == 0) omx_Start();
   dbgprintf(8,"m_iStarted:%i\n", m_iStarted);  
   if (m_iStarted == 0) {DBG_LOG_OUT();return -2;}
   dbgprintf(8,"OMX init\n");  
   
	int iDecoderNum = omx_LoadComponent(OMX_COMP_VIDEO_DECODE, 0);
	if (iDecoderNum < 0)
	{
		fclose(in);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_VIDEO_DECODE init\n");
	int iClockNum = omx_LoadComponent(OMX_COMP_CLOCK, 0);
	if (iClockNum < 0)
	{
		fclose(in);
		omx_Release_Component(iDecoderNum);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_CLOCK init\n");
	int iShedulerNum = omx_LoadComponent(OMX_COMP_VIDEO_SCHEDULER, 0);
	if (iShedulerNum < 0)
	{
		fclose(in);
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_VIDEO_SCHEDULER init\n");
	int iRenderNum = omx_LoadComponent(OMX_COMP_VIDEO_RENDER, 0);
	if (iRenderNum < 0)
	{
		fclose(in);
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		omx_Release_Component(iShedulerNum);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_VIDEO_RENDER init\n");
   
   dbgprintf(8,"Comps init done \n"); 
   
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   OMX_ERRORTYPE oerr = OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeClockState, &cstate);
	if (OMX_TEST_ERROR(oerr)) goto error_out;
	
   if (!omx_set_tunnel(iClockNum,OMX_PORT_1, iShedulerNum,OMX_PORT_2, OMX_TRUE))
	   goto error_out;

   dbgprintf(8,"Set state clock exec\n");   
   omx_set_state(iClockNum, OMX_StateExecuting, OMX_NOW);
   
   //printf("Set count buffers\n");
   //omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 20, 0);
      
   dbgprintf(8,"Set video format\n");
   omx_set_video_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_VIDEO_CodingAVC);
   //omx_set_frame_resolution(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT,  640, 480); 
  
   dbgprintf(8,"Set enable port 1 iDecoderNum\n");
   omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
   dbgprintf(8,"Set create buffers\n");
   if (!omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN)) goto error_out;
   omx_wait_exec_cmd(iDecoderNum);
   
   dbgprintf(8,"Set state\n");
   omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
   
   omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
   
   int real_output_width, real_output_height;
   omx_get_frame_resolution(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
   dbgprintf(8,"W:%i, H:%i\n",real_output_width,real_output_height);
   omx_get_frame_resolution(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
   dbgprintf(8,"W:%i, H:%i\n",real_output_width,real_output_height);
   
   int exist_data = 1;
   int data_len, iFlag, BufferSize;
   omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, &BufferSize);
   int port_settings_changed = 0;
   char * src_buff = (char*)DBG_MALLOC(BufferSize); 
   iFlag = OMX_BUFFERFLAG_STARTTIME | OMX_BUFFERFLAG_TIME_UNKNOWN;
   iFlag = OMX_BUFFERFLAG_STARTTIME;
  // dumpport(m_oCompList[iDecoderNum].handle, m_oCompList[iDecoderNum].in_port[0].number);
  // dumpport(m_oCompList[iDecoderNum].handle, m_oCompList[iDecoderNum].out_port[0].number);
  // sleep(1000);
  //data_len = fread(src_buff, 1, 20000000, in);
	   
    while (exist_data==1) 
    {
	    data_len = fread(src_buff, 1, BufferSize, in);
	    if (data_len == 0) 
	    {
		   iFlag = OMX_BUFFERFLAG_EOS;
		   exist_data = 0;
	    }
	    iStatus = omx_load_data_in_buffer(iDecoderNum, OMX_PORT_1, src_buff, data_len, iFlag, 1, 1000);
	    if (iStatus < 1) exist_data=-1;
	    iFlag = OMX_BUFFERFLAG_TIME_UNKNOWN;
	    if ((port_settings_changed == 0) && (omx_wait_list_empty(iDecoderNum) == 1))
        {
			if (exist_data == -1) exist_data = 1;
			dbgprintf(8,"Test wait %i\n", omx_wait_list_empty(iDecoderNum));
			port_settings_changed = 1;
			dbgprintf(8,"Set tunnel 2\n");
			if (!omx_set_tunnel(iDecoderNum,OMX_PORT_1, iShedulerNum,OMX_PORT_1, OMX_TRUE)) break;
			//dumpport(m_oCompList[iDecoderNum].handle, m_oCompList[iDecoderNum].out_port[0].number);
			dbgprintf(8,"Set state\n");
			omx_set_state(iShedulerNum, OMX_StateExecuting, OMX_NOW);
			dbgprintf(8,"Set tunnel 3\n");
			if (!omx_set_tunnel(iShedulerNum,OMX_PORT_1, iRenderNum,OMX_PORT_1, OMX_TRUE)) break;
			dbgprintf(8,"Set state\n");
			omx_set_state(iRenderNum, OMX_StateExecuting, OMX_NOW);
			omx_get_frame_resolution(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
			dbgprintf(8,"W:%i, H:%i\n",real_output_width,real_output_height);
			omx_add_cmd_in_list(iRenderNum, OMX_EventBufferFlag, OMX_CommandAny);
	    }
    } 
   omx_wait_exec_cmd(iRenderNum);
   
      
   DBG_FREE(src_buff);
   
   omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iRenderNum, OMX_StateIdle, OMX_NOW);
   
   omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
   omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
   omx_wait_exec_cmd(iDecoderNum);
   
   omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
   omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
   
   omx_disable_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
   omx_disable_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_LATER);
   
   omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);   
   omx_disable_port(iRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
   
   omx_wait_exec_cmd(iDecoderNum);
   omx_wait_exec_cmd(iShedulerNum);
   omx_wait_exec_cmd(iRenderNum);
   omx_wait_exec_cmd(iClockNum);
   
   if (!omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE)) goto error_out;
   if (!omx_set_tunnel(iClockNum, OMX_PORT_1, -1, 0, OMX_FALSE)) goto error_out;
   if (!omx_set_tunnel(iShedulerNum, OMX_PORT_1, -1, 0, OMX_FALSE)) goto error_out;
   
   omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
   omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
   omx_flush_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_LATER);
   omx_flush_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
   omx_flush_port(iRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
   omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
   omx_wait_exec_cmd(iDecoderNum);
   omx_wait_exec_cmd(iShedulerNum);
   omx_wait_exec_cmd(iRenderNum);
   omx_wait_exec_cmd(iClockNum);

   res = 1;
error_out:  
	fclose(in);
	omx_Release_Component(iDecoderNum);
	omx_Release_Component(iClockNum);
	omx_Release_Component(iShedulerNum);
	omx_Release_Component(iRenderNum);
	
   omx_Stop();
 //  sleep(5);
  
  DBG_LOG_OUT();
  return res;
}

void omx_play_video_on_egl(char *FilePath, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH)
{
	DBG_LOG_IN();
	
	omx_egl_link e_link;
	TX_SEMAPHORE 	*psem_init;
    e_link.eglDisplay = eglDisplay;
    e_link.eglContext = eglContext;
    e_link.texture_id = texture_id;
    e_link.data = DBG_MALLOC(strlen(FilePath) + 1);
	memset(e_link.data, 0, strlen(FilePath) + 1);
	memcpy(e_link.data, FilePath, strlen(FilePath) + 1);
    e_link.errorcode = 0;
    psem_init = DBG_MALLOC(sizeof(TX_SEMAPHORE));	
    e_link.psem_init = psem_init;
    tx_semaphore_create(psem_init, NULL, 0);	
    tx_semaphore_add_in_list(psem_init, 2, 2);     
    pthread_create(&thread1, &tattr, thread_omx_play_video_on_egl, &e_link);
    dbgprintf(8,"thread created\n");
    tx_semaphore_wait(psem_init);
    
    if (e_link.errorcode == 0) 
    {
	dbgprintf(8,"egl image creating\n");
    	if (*eglImage != NULL) eglDestroyImageKHR (*eglDisplay, *eglImage);
    	omx_create_egl_buffer(e_link.attr, e_link.sizeW, e_link.sizeH, texture_id, eglDisplay, eglContext);
	*eglImage = m_oCompList[e_link.attr].out_port[OMX_PORT_1].EglImage;
	*sW = e_link.sizeW;
	*sH = e_link.sizeH;
	dbgprintf(8,"egl image created\n");
    tx_semaphore_add_in_list(psem_init, 2, 2); 	
	tx_semaphore_go(psem_init,1,1);
	tx_semaphore_wait(psem_init);
	dbgprintf(8,"thread init done\n");
    } else dbgprintf(8,"thread error done\n");
    
	DBG_FREE(e_link.data);
    tx_semaphore_delete(psem_init);
    DBG_FREE(psem_init);

	DBG_LOG_OUT();  	
}

void* thread_omx_play_video_on_egl(void * pdata)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
    DBG_LOG_IN();
   
	pthread_setname_np(pthread_self(), "play_vid_on_egl");
	
	omx_egl_link *e_link = pdata;
   FILE *in;
   int res = -2;
   
	if (m_iStarted == 0) {DBG_LOG_OUT();return (void*)-2;}
	if((in = fopen(e_link->data, "rb")) == NULL) {DBG_LOG_OUT();return (void*)-1; }
	
   tx_semaphore_wait(&psem_omx_run);
   tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);    
   tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);    
   tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_REPLAY_VIDEO, TX_ANY);    
   
   int iDecoderNum = omx_LoadComponent(OMX_COMP_VIDEO_DECODE, 0);
   if (iDecoderNum < 0)
	{
		fclose(in);
		DBG_LOG_OUT();
		return (void*)-2;
	}
   int iClockNum = omx_LoadComponent(OMX_COMP_CLOCK, 0);
   if (iClockNum < 0)
	{
		fclose(in);
		omx_Release_Component(iDecoderNum);
		DBG_LOG_OUT();
		return (void*)-2;
	}
   int iShedulerNum = omx_LoadComponent(OMX_COMP_VIDEO_SCHEDULER, 0);
   if (iShedulerNum < 0)
	{
		fclose(in);
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		DBG_LOG_OUT();
		return (void*)-2;
	}
	int iEglRenderNum = omx_LoadComponent(OMX_COMP_EGL_RENDER, 0);  
	if (iEglRenderNum < 0)
	{
		fclose(in);
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		omx_Release_Component(iShedulerNum);
		DBG_LOG_OUT();
		return (void*)-2;
	}   
   dbgprintf(8,"Comps init done\n"); 
  
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
 
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	memset(&cstate, 0, sizeof(cstate));
	cstate.nSize = sizeof(cstate);
	cstate.nVersion.nVersion = OMX_VERSION;
	cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
	cstate.nWaitMask = 1;
	OMX_ERRORTYPE oerr = OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeClockState, &cstate);
	if (OMX_TEST_ERROR(oerr)) goto error_out;
	
	if (!omx_set_tunnel(iClockNum,OMX_PORT_1, iShedulerNum,OMX_PORT_2, OMX_TRUE)) goto error_out;

	dbgprintf(8,"Set state clock exec\n");   
	omx_set_state(iClockNum, OMX_StateExecuting, OMX_NOW);
   
	dbgprintf(8,"Set video format\n");
	omx_set_video_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_VIDEO_CodingAVC);
   
	omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
	if (!omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN)) goto error_out;
	omx_wait_exec_cmd(iDecoderNum);
	
	omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
	
	omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
	omx_add_cmd_in_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
					
	int exist_data = 1;
	int cur_buff = 0;
	int data_len, iFlag, BufferSize;
	omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, &BufferSize);
	int port_settings_changed = 0;
	char * src_buff = (char*)DBG_MALLOC(BufferSize); 
	iFlag = OMX_BUFFERFLAG_STARTTIME;
	while (exist_data == 1) 
	{
		if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_REPLAY_VIDEO, TX_ANY) == 0)
		{
			tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_REPLAY_VIDEO, TX_ANY);   
			iFlag = OMX_BUFFERFLAG_STARTTIME;
			rewind(in);
		}
		if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY) == 0)	data_len = 0;
			else data_len = fread(src_buff, 1, BufferSize, in);
		if (data_len == 0) 
		{
		   iFlag = OMX_BUFFERFLAG_EOS;
		   exist_data = 0;
		}
		//dbgprintf(8,"Read from file %i bytes\n", data_len);	
        cur_buff = omx_load_data_in_buffer(iDecoderNum, OMX_PORT_1, src_buff, data_len, iFlag, 1, 5000);
		if (cur_buff < 1)
		{
			cur_buff = 0;
			exist_data=-1;
			break;
		}    
		if (((exist_data==0) || (cur_buff==0)) && (port_settings_changed==0))
		{
            exist_data=-1;            
			break;
        }

	   dbgprintf(8,"current buffer %i\n", cur_buff);
	   

	   iFlag = OMX_BUFFERFLAG_TIME_UNKNOWN;
	   if ((exist_data !=-1) && (port_settings_changed == 0) && (omx_wait_list_empty(iDecoderNum) == 1))
       {
			if (exist_data == -1) exist_data = 1;
   
			dbgprintf(8,"Create tunnels\n");			
			if (!omx_set_tunnel(iDecoderNum,OMX_PORT_1, iShedulerNum,OMX_PORT_1, OMX_TRUE))	{exist_data=-1; break;}
			omx_set_state(iShedulerNum, OMX_StateExecuting, OMX_NOW);
			if (!omx_set_tunnel(iShedulerNum,OMX_PORT_1, iEglRenderNum,OMX_PORT_1, OMX_TRUE)) {exist_data=-1; break;}
			omx_set_state(iEglRenderNum, OMX_StateExecuting, OMX_NOW);
			omx_wait_exec_cmd(iEglRenderNum);
		     
			int real_output_width, real_output_height;
			omx_get_frame_resolution(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
			//dbgprintf(8,"%i,  %i, %i\n", e_link->eglDisplay, e_link->eglContext, e_link->texture_id);
			
			e_link->attr = iEglRenderNum;
			e_link->sizeW = real_output_width;
			e_link->sizeH = real_output_height;
			//omx_create_egl_buffer(iEglRenderNum, real_output_width, real_output_height, e_link->texture_id, e_link->eglDisplay, e_link->eglContext);
			dbgprintf(8,"%i,  %i\n",real_output_width,real_output_height);
			tx_semaphore_add_in_list_spec(e_link->psem_init,1,1,0);     
			tx_semaphore_go(e_link->psem_init,2,2);
			tx_semaphore_wait_spec(e_link->psem_init);
		   
			omx_enable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT,OMX_LATER);
			if (!omx_use_egl_buffer(iEglRenderNum)) {exist_data = -1; break;}
			omx_wait_exec_cmd(iEglRenderNum);
		  
			omx_add_cmd_in_list_spec(iEglRenderNum, OMX_EventFillBuffer, OMX_CommandAny, 0);
			if (!omx_activate_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, 0))
			{
				exist_data = -1;
				break;
			}
			omx_wait_exec_spec_cmd(iEglRenderNum);
		
			omx_add_cmd_in_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny); 
			
			tx_semaphore_go(e_link->psem_init,2,2);
			port_settings_changed = 1;
		}
	}
	if (exist_data == -1)
	{
		dbgprintf(8,"Not success\n");
        omx_delete_cmd_from_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
        omx_delete_cmd_from_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
        e_link->errorcode = 1;
		tx_semaphore_go(e_link->psem_init,2,2);
	}
	dbgprintf(8,"Play done1\n"); 
	if (exist_data == 0) omx_wait_exec_cmd(iEglRenderNum); else omx_delete_cmd_from_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny);
	dbgprintf(8,"Play done2\n");
   
	DBG_FREE(src_buff);
     
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	if (exist_data != -1) omx_set_tunnel(iClockNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	if (exist_data != -1) omx_set_tunnel(iShedulerNum, OMX_PORT_1, -1, 0, OMX_FALSE);	
	dbgprintf(8,"Play done3\n");
	
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);   
	omx_flush_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	
	dbgprintf(8,"Play done4\n");	
   
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
	omx_disable_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_LATER);   
	omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);   
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
   
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	if (port_settings_changed == 1) omx_remove_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT);
	
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateLoaded, OMX_NOW);
	
   //eglDestroyImageKHR (*(e_link->eglDisplay), m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage);
   res = 1;
error_out:    
   fclose(in);
   omx_Release_Component(iDecoderNum);
   omx_Release_Component(iClockNum);
   omx_Release_Component(iShedulerNum);
   omx_Release_Component(iEglRenderNum);
   tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);   
   tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);
	tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_REPLAY_VIDEO, TX_ANY);      
  dbgprintf(8,"Play done4\n");
  
  DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
  return (void*)(intptr_t)res;
}

void omx_stop_video(int iWait)
{
	//tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);
	int ret;
	do
	{
		ret = tx_semaphore_count_in_list(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, TX_ANY);
		if (ret)
		{
			if (ret == tx_semaphore_count_in_list(&psem_omx_run, OMX_EVENT_STOP_PLAY_VIDEO, TX_ANY))
				tx_semaphore_go(&psem_omx_run, OMX_EVENT_STOP_PLAY_VIDEO, TX_ANY);
			if ((iWait) || (ret > 0)) while (tx_semaphore_count_in_list(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, TX_ANY) == ret) usleep(10000);	
			//printf("Stopped thread %i\n", tx_semaphore_count_in_list(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, TX_ANY));
		}
	} while(ret);
	//if (iWait) while (omx_IsFree_Video() == 0) usleep(1000);
}

void omx_stop_video_num(unsigned int uiNum, int iWait)
{
	//tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);
	if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, uiNum))
	{
		tx_semaphore_go(&psem_omx_run, OMX_EVENT_STOP_PLAY_VIDEO, uiNum);
		if (iWait) while (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, uiNum)) usleep(10000);	
	}
	//if (iWait) while (omx_IsFree_Video() == 0) usleep(1000);
}

void omx_stop_capture(int iWait)
{
	tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_CAPTURE, TX_ANY);	
	if (iWait) while (omx_IsFree_Capture() == 0) usleep(1000);
}

void omx_replay_video()
{
	
	//tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_REPLAY_VIDEO, TX_ANY);	
}


void omx_player_pause()
{	
	tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_PAUSE, TX_ANY);	
}

void omx_player_play()
{
	tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_PLAY, TX_ANY);	
}

void omx_player_speedup()
{
	tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDUP, TX_ANY);
}

void omx_player_speeddown()
{	
	tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDDOWN, TX_ANY);
}

/*
int omx_play_video_on_screen_from_func(func_link *f_link, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH, int TimeCtrl)
{
	DBG_LOG_IN();
	
	char cMessages[3];
	int result = 1;
	cMessages[0] = TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO;
	cMessages[1] = TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME;
	cMessages[2] = TYPE_MESSAGE_REQUEST_VIDEO_STREAM;
	//dbgprintf(8,"Play video\n");
	if (f_link->ConnectPort == 1)
	{					
		f_link->ConnectPort = TCP_Client(&f_link->RecvAddress, cMessages, 3, FLAG_VIDEO_CODEC_INFO | FLAG_START_VIDEO_FRAME | FLAG_VIDEO_STREAM, 0);
		if (f_link->ConnectPort <= 0)
		{
			dbgprintf(1,"Error connect to CameraModule\n");
			DBG_FREE(f_link);
			return 0;
		}
	}
		
	omx_egl_link *e_link = DBG_CALLOC(sizeof(omx_egl_link), 1);
	e_link->errorcode = 0;
	e_link->FuncRecv = f_link->FuncRecv;
	e_link->sizeW = *sW;
    e_link->sizeH = *sH;
	e_link->ConnectPort = f_link->ConnectPort;
    DBG_FREE(f_link);	
    	
	pthread_create(&thread1, &tattr, thread_omx_play_video_clk_on_screen_from_func, e_link);
	
	dbgprintf(8,"thread 2 created\n");
    
	DBG_LOG_OUT();  
	return result;
}*/

int omx_player_get_speed()
{
	DBG_MUTEX_LOCK(&OMX_mutex);
	int ret = omx_speed_play;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	return ret;
}

int omx_play_video_on_egl_from_func(func_link *f_link, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH, int TimeCtrl, int iPrevType)
{
	DBG_LOG_IN();
	
	int ret = 0;
	DBG_MUTEX_LOCK(&OMX_mutex);
	if ((omx_resource_priority != 1) || omx_IsFree_Encoder()) ret++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	if (ret == 0) return 100;
	
	int result = 1;
	int iTraffType;
	
	f_link->ConnectID = 0;
	if (f_link->ConnectNum == 1)
	{					
		dbgprintf(4,"Connecting to video\n");	
		iTraffType = TRAFFIC_FULL_VIDEO;
		if (iPrevType == 1) iTraffType = TRAFFIC_PREV_VIDEO; 
		if (iPrevType == 2) iTraffType = TRAFFIC_REMOTE_VIDEO; 
		if (iPrevType == 2) 
				f_link->ConnectNum = TCP_Client(&f_link->RecvAddress, iTraffType, FLAG_VIDEO_PARAMS | FLAG_VIDEO_CODEC_INFO | FLAG_START_VIDEO_FRAME, 0, &f_link->ConnectID);
				else
				f_link->ConnectNum = TCP_Client(&f_link->RecvAddress, iTraffType, FLAG_VIDEO_PARAMS | FLAG_VIDEO_CODEC_INFO | FLAG_START_VIDEO_FRAME | FLAG_VIDEO_STREAM, 0, &f_link->ConnectID);
		if (f_link->ConnectNum <= 0)
		{
			dbgprintf(1,"Error connect to video\n");
			DBG_FREE(f_link);
			DBG_LOG_OUT();
			return 0;
		} else dbgprintf(4,"Connected to video\n");
	}
	omx_egl_link e_link;
	TX_SEMAPHORE 	*psem_init;
    e_link.eglDisplay = eglDisplay;
    e_link.eglContext = eglContext;
    e_link.texture_id = texture_id;
    e_link.errorcode = 0;
	e_link.FuncRecv = f_link->FuncRecv;
	e_link.sizeW = *sW;
    e_link.sizeH = *sH;
	e_link.ConnectNum = f_link->ConnectNum;
    e_link.ConnectID = f_link->ConnectID;
	e_link.DeviceNum = f_link->DeviceNum;
	e_link.Type = iPrevType;
	memcpy(&e_link.Address, &f_link->Address, sizeof(struct sockaddr_in));
    DBG_FREE(f_link);
	
    psem_init = DBG_MALLOC(sizeof(TX_SEMAPHORE));	
    e_link.psem_init = psem_init;
    tx_semaphore_create(psem_init, NULL, 0);	
 
    tx_semaphore_add_in_list(psem_init, 2, 2);     
    
	//dbgprintf(3,"!!!!!!!!! Start thread play video\n");
	if (TimeCtrl == 2)
		pthread_create(&thread1, &tattr, thread_omx_play_video_noclk_on_egl_from_func, &e_link);
	if (TimeCtrl == 1)
		pthread_create(&thread1, &tattr, thread_omx_play_video_clk_on_egl_from_func, &e_link);
	if (TimeCtrl == 0)
		pthread_create(&thread1, &tattr, thread_omx_play_video_on_egl_from_func, &e_link);
	
	dbgprintf(8,"thread 2 created\n");
    tx_semaphore_wait(psem_init);
    
    if (e_link.errorcode == 0) 
    {
		dbgprintf(8,"egl image creating\n");
    	if (*eglImage != NULL) eglDestroyImageKHR (*eglDisplay, *eglImage);
    	omx_create_egl_buffer(e_link.attr, e_link.sizeW, e_link.sizeH, texture_id, eglDisplay, eglContext);
		*eglImage = m_oCompList[e_link.attr].out_port[OMX_PORT_1].EglImage;
		*sW = e_link.sizeW;
		*sH = e_link.sizeH;
		dbgprintf(8,"egl image created w:%i, h:%i\n", e_link.sizeW, e_link.sizeH);
		tx_semaphore_add_in_list(psem_init, 2, 2); 	
		tx_semaphore_go(psem_init,1,1);
		tx_semaphore_wait(psem_init);
		dbgprintf(8,"thread init done\n");
    } 
	else 
	{
		dbgprintf(8,"thread error done\n");
		result = 0;
	}
    
    tx_semaphore_delete(psem_init);
    DBG_FREE(psem_init);
	
	DBG_LOG_OUT();  
	return result;
}
/*
void* thread_omx_play_video_clk_on_screen_from_func(void * pdata)
{
   DBG_LOG_IN();
	
	omx_egl_link *e_link = (omx_egl_link*)pdata;
   omx_egl_link e_link2;
   	
   memcpy(&e_link2, pdata, sizeof(omx_egl_link));  
   DBG_FREE(e_link);
   
   if (m_iStarted == 0) return (void*)-2;
   //tx_semaphore_wait(&psem_omx_run);
   tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);    
   tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);    
   
	int iDecoderNum = omx_LoadComponent(OMX_COMP_VIDEO_DECODE, 0);
	if (iDecoderNum < 0)
	{
		return (void*)-2;
	}  
	int iClockNum = omx_LoadComponent(OMX_COMP_CLOCK, 0);
	if (iClockNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		return (void*)-2;
	}  
	int iShedulerNum = omx_LoadComponent(OMX_COMP_VIDEO_SCHEDULER, 0);
	if (iShedulerNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		return (void*)-2;
	}  
	int iEglRenderNum = omx_LoadComponent(OMX_COMP_VIDEO_RENDER, 0);  
	if (iEglRenderNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		omx_Release_Component(iShedulerNum);
		return (void*)-2;
	}  	
   dbgprintf(8,"Comps init done\n"); 
  
   omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
 
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   DBG_OERR(OMX_SetParameter(m_oCompList[iClockNum].handle, OMX_IndexConfigTimeClockState, &cstate));

   omx_set_tunnel(iClockNum,OMX_PORT_1, iShedulerNum,OMX_PORT_2, OMX_TRUE);

   dbgprintf(8,"Set state clock exec\n");   
   omx_set_state(iClockNum, OMX_StateExecuting, OMX_NOW);
   
   dbgprintf(8,"Set video format\n");
   omx_set_video_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_VIDEO_CodingAVC);
   omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 20, VIDEO_CODER_BUFFER_SIZE);
   
   omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
   omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
   omx_wait_exec_cmd(iDecoderNum);
   
   omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
   
   omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
  // omx_add_cmd_in_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
   	      
   int status;
   int data_len, iFlag, BufferSize;
   omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, &BufferSize);
   int port_settings_changed = 0;
   //omx_start_packet StartPack;
   //memset(&StartPack, 0, sizeof(omx_start_packet));
   //StartPack.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
   //StartPack.BufferStartSize = ((VIDEO_WIDTH * VIDEO_HEIGHT * 3) / 2) * (VIDEO_INTRAPERIOD / 10);
   //StartPack.BufferStartFrame = (char*)DBG_MALLOC(StartPack.BufferStartSize);
  
   unsigned int iNumFrame = 0;
   int iTimeOut = 0;
   OMX_BUFFERHEADERTYPE *cur_buffer;
   while (1) 
   {
		status = 0; //WAIT
		cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 5000);		
		if (!cur_buffer)
		{
			dbgprintf(1,"thread_omx_play_video_clk_on_screen_from_func Error omx_get_active_buffer\n");
			status = -2; //EXIT
			Menu_ResetShower(-1);
		}
		
		if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY) == 0)
		{
			status = 2; //EXIT
			dbgprintf(8,"thread_omx_play_video_clk_on_screen_from_func OMX_EVENT_STOP_VIDEO\n");	
		} 
		
		if (status == 0)
		{
			iFlag = 0;
			status = e_link2.FuncRecv((char*)(cur_buffer->pBuffer + cur_buffer->nOffset), BufferSize - cur_buffer->nOffset, &iNumFrame, &data_len, &iFlag, NULL, 1, e_link2.ConnectPort);			
			cur_buffer->nFilledLen = data_len;		
			if (iFlag & OMX_BUFFERFLAG_EOS) dbgprintf(4,"OMX_BUFFERFLAG_EOS signal\n");
		}
			
		if (status == 0)
		{
			iTimeOut++;
			if (iTimeOut == 10)
			{
				status = 2;
				dbgprintf(8,"Stop video playing timout\n");	
			}	
		}	
		if (status > 0)
		{
			iTimeOut = 0;
			if (status == 2)
			{
				data_len = 0;
				iFlag |= OMX_BUFFERFLAG_EOS;
				dbgprintf(8,"Stop video playing\n");	
			}	
			//debug_
			//printf("Read from file %i bytes, [%i]\n", data_len,iDecoderNum);	
			//omx_add_cmd_in_list(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
			omx_activate_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, iFlag); 
			//ret = omx_wait_event_timeout(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny, 5000);
			//if (ret == 0) status = 3; 
			//else 
			//{
				//if (ret < 4057) 
			//	{
			//		dbgprintf(1,("wait empty buuf decoder %i 		%i\n", 5000 - ret, data_len);
			//	}
			//}
			//omx_wait_exec_cmd(iDecoderNum);
			omx_switch_next_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
			//dbgprintf(8,"Readed from file %i bytes\n", data_len);	
			if (iFlag & OMX_BUFFERFLAG_EOS) status = 2;
		}
		
		if (status >= 2) 
		{
			if (port_settings_changed == 0)
			{
				dbgprintf(8,"Not success\n");
				omx_delete_cmd_from_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
				//omx_delete_cmd_from_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
				e_link->errorcode = 1;
			}
			break;
        }
		
		//dbgprintf(8,"current buffer\n");	   
		if ((status == 1) && (port_settings_changed == 0) && (omx_wait_list_empty(iDecoderNum) == 1))
		{
			dbgprintf(8,"Create tunnels\n");
			port_settings_changed = 1;
			omx_set_tunnel(iDecoderNum,OMX_PORT_1, iShedulerNum,OMX_PORT_1, OMX_TRUE);
			omx_set_state(iShedulerNum, OMX_StateExecuting, OMX_NOW);
			omx_set_tunnel(iShedulerNum,OMX_PORT_1, iEglRenderNum,OMX_PORT_1, OMX_TRUE);
			omx_set_state(iEglRenderNum, OMX_StateExecuting, OMX_NOW);
			omx_wait_exec_cmd(iEglRenderNum);
				
			int real_output_width, real_output_height;
			omx_get_frame_resolution(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
			//printf("%i,  %i, %i\n", elink.eglDisplay, elink.eglContext, elink.texture_id);
				
			
			dbgprintf(4,"Play1\n"); 
		}
	}
	if (e_link2.ConnectPort != 0) CloseConnectID(e_link2.ConnectPort);
	
	dbgprintf(8,"Play done1\n"); 
	 
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	omx_wait_exec_cmd(iDecoderNum);
	
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
	omx_disable_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_LATER);   
	omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);   
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);	
	
	omx_wait_exec_cmd(iDecoderNum);
	omx_wait_exec_cmd(iShedulerNum);
	omx_wait_exec_cmd(iClockNum);
	omx_wait_exec_cmd(iEglRenderNum);
  
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	if (port_settings_changed != 0) omx_set_tunnel(iClockNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	if (port_settings_changed != 0) omx_set_tunnel(iShedulerNum, OMX_PORT_1, -1, 0, OMX_FALSE);	
	
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);   
	omx_flush_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	
	dbgprintf(8,"Play done4\n");		
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateLoaded, OMX_NOW);
	
   //eglDestroyImageKHR (*(e_link->eglDisplay), m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage);
   //DBG_FREE(StartPack.BufferCodecInfo);
   //DBG_FREE(StartPack.BufferStartFrame);  
   
   omx_Release_Component(iDecoderNum);
   omx_Release_Component(iClockNum);
   omx_Release_Component(iShedulerNum);
   omx_Release_Component(iEglRenderNum);
   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
  
   dbgprintf(8,"Play done5\n");
	
	DBG_LOG_OUT();  
	return (void*)1;
}*/

void* thread_omx_play_video_noclk_on_egl_from_func(void * pdata)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "play_video_noclk");
	
	int res = -2;
	
	omx_egl_link *e_link = (omx_egl_link*)pdata;
	omx_egl_link e_link2;
	memcpy(&e_link2, pdata, sizeof(omx_egl_link));  
   
	if (m_iStarted == 0) {DBG_LOG_OUT();return (void*)-2;}
	//tx_semaphore_wait(&psem_omx_run);
	
	DBG_MUTEX_LOCK(&OMX_mutex);
	cThreadOmxPlayStatus++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	int iDecoderNum = omx_LoadComponent(OMX_COMP_VIDEO_DECODE, 0);
	if (iDecoderNum < 0)
	{
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	
	int iEglRenderNum = omx_LoadComponent(OMX_COMP_EGL_RENDER, 2); 
	if (iEglRenderNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	   
	dbgprintf(8,"Comps init done\n"); 
	  
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);    
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);    
   
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	
	dbgprintf(8,"Set video format\n");
	omx_set_video_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_VIDEO_CodingAVC);
	omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 20, VIDEO_CODER_BUFFER_SIZE);
	 
	omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
	if (!omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN)) goto error_out;
	omx_wait_exec_cmd(iDecoderNum);
	   
	omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
	   
	omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
	omx_add_cmd_in_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
			  
	int status;
	int iFlag, BufferSize;
	omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, &BufferSize);
	int port_settings_changed = 0;
	
	//omx_start_packet StartPack;
	//memset(&StartPack, 0, sizeof(omx_start_packet));
	//StartPack.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
	//StartPack.BufferStartSize = ((VIDEO_WIDTH * VIDEO_HEIGHT * 3) / 2) * (VIDEO_INTRAPERIOD / 10);
	//StartPack.BufferStartFrame = (char*)DBG_MALLOC(StartPack.BufferStartSize);
	//int64_t prev_ms = 0;
	//get_ms(&prev_ms);
    unsigned int iNumFrame = 0;
    int iTimeOut = 0;
    unsigned char cLevel = 5;
    int ret;
    unsigned int uiOffSet = 0;
   
	int64_t iMaxTimeStamp = 0;
	//void *DecBuff;
	AVPacket avpkt;
	avpkt.data = NULL;
	avpkt.size = 0;
	avpkt.flags = 0;
	OMX_BUFFERHEADERTYPE *cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 5000);		
	if (!cur_buffer) dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: Error omx_get_active_buffer ERROR_STOP_VIDEO\n");
	//DecBuff = cur_buffer->pBuffer;
	
    char cClockState = 0;
    
	while (1) 
    {	   
		status = 0; //WAIT
		cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 5000);		
		if (!cur_buffer)
		{
			dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: Error omx_get_active_buffer ERROR_STOP_VIDEO\n");
			status = -1; //EXIT
			cLevel = 4;
			//Menu_ResetShower(-1);
		}
		if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY) == 0)
		{
			status = 3; //EXIT
			dbgprintf(5,"thread_omx_play_video_clk_on_egl_from_func: OMX_EVENT_STOP_VIDEO\n");	
		} 
		
		if (status == 0)
		{
			if (avpkt.size == uiOffSet)
			{
				if (avpkt.flags == 1) av_free_packet(&avpkt);
				if (avpkt.flags == 2) free(avpkt.data);				
				avpkt.data = cur_buffer->pBuffer + cur_buffer->nOffset;
				avpkt.size = BufferSize - cur_buffer->nOffset;
				avpkt.flags = 0;
				iFlag = port_settings_changed;		
				status = e_link2.FuncRecv((void*)&avpkt, &iNumFrame, &iFlag, NULL, 1, e_link2.ConnectNum, e_link2.ConnectID);
				if (status == 1)
				{
					if (avpkt.flags != 0)
					{
						//cur_buffer->pBuffer = avpkt.data;
						uiOffSet = 0;
						cur_buffer->nFilledLen = avpkt.size;
						memcpy(cur_buffer->pBuffer, avpkt.data, cur_buffer->nFilledLen);
					} else cur_buffer->nFilledLen = avpkt.size;		
				}
			}
			else
			{
				if (avpkt.flags != 0)
				{
					//cur_buffer->pBuffer = avpkt.data + uiOffSet;
					cur_buffer->nFilledLen = avpkt.size - uiOffSet;
					if (cur_buffer->nFilledLen > BufferSize) cur_buffer->nFilledLen = BufferSize;
					memcpy(cur_buffer->pBuffer, avpkt.data + uiOffSet, cur_buffer->nFilledLen);	
				}
				else 
				{
					add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, 0);	
					dbgprintf(2, "%s: error rework video data\n", __func__);
					status = 3;
				}
			}
			if (status == 1)
			{
				if (cur_buffer->nFilledLen > BufferSize) cur_buffer->nFilledLen = BufferSize;
				uiOffSet += cur_buffer->nFilledLen;
				cur_buffer->nOffset = 0;
				cur_buffer->nTimeStamp = ToOMXTime(avpkt.pts);
				if (avpkt.pts > iMaxTimeStamp) iMaxTimeStamp = avpkt.pts;
			}
		}
		if (status == 0)
		{
			iTimeOut++;
			if (iTimeOut == 10)
			{
				status = 2;
				dbgprintf(cLevel,"Stop video playing timout\n");	
			}	
		}
		if (status == 4) 
		{
			if ((port_settings_changed == 1) && (cClockState == 1)) 
			{
				uiOffSet = avpkt.size;
				//omx_set_clock_speed(iClockNum, 0);
				//omx_set_clock_state(iClockNum, OMX_TIME_ClockStateStopped, 0);				
				cClockState = 0;
				//omx_set_state(iClockNum, OMX_StatePause, OMX_NOW);		
				//omx_set_state(iDecoderNum, OMX_StatePause, OMX_NOW);				
			}
			if (tx_semaphore_exist_in_list(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY))
			{
				tx_semaphore_add_in_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);
				tx_semaphore_go(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY);
				ret = tx_semaphore_wait_event_timeout(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY, 30);
				if (ret == 0) 
				{
					tx_semaphore_delete_from_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);	
					dbgprintf(8,"timeout sync from thread_omx_play_video_clk_on_egl_from_func\n");
				}
				//else dbgprintf(8,"ok sync from thread_omx_play_video_clk_on_egl_from_func %i\n", ret);
			} 
			else 
			{
				usleep(10000);
			}
		}
		if ((status > 0) && (status != 4))
		{
			iTimeOut = 0;
			if (status == 3)
			{
				iFlag |= OMX_BUFFERFLAG_EOS;
				dbgprintf(cLevel,"Stop video playing\n");	
			}
			if (iFlag & OMX_BUFFERFLAG_EXTRADATA)
			{
				if (port_settings_changed == 1)
				{	
					if (iFlag & OMX_BUFFERFLAG_EOS)
					{
						dbgprintf(cLevel,"Extra stop video playing\n");	
						//omx_set_state(iClockNum, OMX_StatePause, OMX_NOW);
						//omx_set_clock_state(iClockNum, OMX_TIME_ClockStateStopped, 0);	
						omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
						//dbgprintf(cLevel,"omx_flush_port\n");	
					}
					else
					{
						dbgprintf(cLevel,"New position video playing\n");	
						//omx_set_state(iClockNum, OMX_StatePause, OMX_NOW);
						//omx_set_clock_state(iClockNum, OMX_TIME_ClockStateStopped, 0);	
						//omx_set_clock_time(iClockNum, OMX_PORT_1, avpkt.pts);
						iMaxTimeStamp = avpkt.pts;
						omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
						//omx_set_clock_state(iClockNum, OMX_TIME_ClockStateWaitingForStartTime, 1);
						//omx_set_state(iClockNum, OMX_StateExecuting, OMX_NOW);
						iFlag |= OMX_BUFFERFLAG_STARTTIME;
					}
				}
				iFlag ^= OMX_BUFFERFLAG_EXTRADATA;				
			}
			if (iFlag & OMX_BUFFERFLAG_EOS) cur_buffer->nTimeStamp = ToOMXTime(iMaxTimeStamp);
			
			if ((port_settings_changed == 1) && (cClockState == 0)) 
			{
				cClockState = 1;
				//omx_set_clock_speed(iClockNum, 1000);
			}
			
			//printf("DECODING %i %i %i %i %i %i\n", (unsigned int)(FromOMXTime(cur_buffer->nTimeStamp)/1000), cur_buffer->nFilledLen, iFlag, iFlag & OMX_BUFFERFLAG_STARTTIME, iFlag & OMX_BUFFERFLAG_TIME_UNKNOWN, iFlag & OMX_BUFFERFLAG_EOS);
			omx_add_cmd_in_list(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
			if (!omx_activate_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, iFlag))
			{
				add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, 0);	
				dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: TIMEOUT_STOP_VIDEO\n");
				status = 3;
			}
			else
			{
				ret = omx_wait_event_timeout(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny, 5000);
				if (ret == 0)
				{
					add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, 0);	
					dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: TIMEOUT_STOP_VIDEO\n");
					status = 3;
				}
				omx_switch_next_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
				//dbgprintf(8,"Readed from file %i bytes (status: %i)\n", data_len, status);	
				if ((iFlag & OMX_BUFFERFLAG_EOS) && (status < 3)) status = 3;
			}
		}		
		
		if ((status < 0) || (status == 3))
		{
			if (port_settings_changed == 0)
			{
				//dbgprintf(cLevel,"Not success\n");
				omx_delete_cmd_from_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
				omx_delete_cmd_from_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
				e_link->errorcode = 1;
				tx_semaphore_go(e_link->psem_init,2,2);
			}
			else
			{
				if (status < 0) omx_delete_cmd_from_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny);					
			}
			break;
        }
		
		//dbgprintf(8,"current buffer\n");	   
		if ((status == 1) && (port_settings_changed == 0) && (omx_wait_list_empty(iDecoderNum) == 1))
		{
			dbgprintf(8,"Create tunnels\n");
			cClockState = 1;
			omx_set_tunnel(iDecoderNum,OMX_PORT_1, iEglRenderNum,OMX_PORT_1, OMX_TRUE);
			omx_set_state(iEglRenderNum, OMX_StateExecuting, OMX_NOW);
			omx_wait_exec_cmd(iEglRenderNum);
				
			int real_output_width, real_output_height;
			omx_get_frame_resolution(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
			//printf("%i,  %i, %i\n", elink.eglDisplay, elink.eglContext, elink.texture_id); 
				
			e_link->attr = iEglRenderNum;
			e_link->sizeW = real_output_width;
			e_link->sizeH = real_output_height;
			//omx_create_egl_buffer(iEglRenderNum, real_output_width, real_output_height, e_link->texture_id, e_link->eglDisplay, e_link->eglContext);
			//dbgprintf(8,"%i,  %i\n",real_output_width,real_output_height);
			tx_semaphore_add_in_list_spec(e_link->psem_init,1,1,0);     
			tx_semaphore_go(e_link->psem_init,2,2);
			tx_semaphore_wait_spec(e_link->psem_init);
			
			omx_enable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT,OMX_LATER);
			omx_use_egl_buffer(iEglRenderNum);
			omx_wait_exec_cmd(iEglRenderNum);
			
			omx_add_cmd_in_list_spec(iEglRenderNum, OMX_EventFillBuffer, OMX_CommandAny, 0);
			if (!omx_activate_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, 0))
			{
				e_link->errorcode = 1;
				tx_semaphore_go(e_link->psem_init,2,2);
				break;
			}
			omx_wait_exec_spec_cmd(iEglRenderNum);
			
			omx_add_cmd_in_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny); 
			tx_semaphore_go(e_link->psem_init,2,2);
			port_settings_changed = 1;			
		}
	}
	
	if (e_link2.ConnectID != 0) CloseConnectID(e_link2.ConnectID);
	
	dbgprintf(8,"OMX Play done1 %i\n",status); 
	
	//cur_buffer->pBuffer = DecBuff;
	
	if (port_settings_changed == 1)
		omx_wait_exec_cmd(iEglRenderNum); 
		else 
		omx_delete_cmd_from_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny);
		
	dbgprintf(8,"OMX Play done2\n");
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	
	dbgprintf(8,"OMX Play done3\n");
	 
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	if (port_settings_changed == 1)
	{
		omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	}
	dbgprintf(8,"OMX Play done3.1\n");
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	omx_wait_exec_cmd(iDecoderNum);
	
	dbgprintf(8,"OMX Play done3.2\n"); 
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);	
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	if (port_settings_changed == 1) omx_remove_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT);
	
	omx_wait_exec_cmd(iDecoderNum);
	omx_wait_exec_cmd(iEglRenderNum);
	
	dbgprintf(8,"OMX Play done3.3\n");
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	
	dbgprintf(8,"OMX Play done4\n");		
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateLoaded, OMX_NOW);
	
   //eglDestroyImageKHR (*(e_link->eglDisplay), m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage);
   //DBG_FREE(StartPack.BufferCodecInfo);
   //DBG_FREE(StartPack.BufferStartFrame);  
   
   res = 1;
error_out:   
   omx_Release_Component(iDecoderNum);
   omx_Release_Component(iEglRenderNum);
   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
	//tx_semaphore_go(&psem_omx_sync, OMX_EVENT_STOP_VIDEO, TX_ANY);
	
   dbgprintf(cLevel,"OMX Play done5\n");
	
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxPlayStatus--;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	DBG_LOG_OUT();  
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)(intptr_t)res;
}

void* thread_omx_play_video_clk_on_egl_from_func(void * pdata)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "play_video_clk");
	
	int res = -2;
	
	omx_egl_link *e_link = (omx_egl_link*)pdata;
	omx_egl_link e_link2;
	memcpy(&e_link2, pdata, sizeof(omx_egl_link));  
   
	if (m_iStarted == 0) {DBG_LOG_OUT();return (void*)-2;}
	//tx_semaphore_wait(&psem_omx_run);
	
	DBG_MUTEX_LOCK(&OMX_mutex);
	cThreadOmxPlayStatus++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	int iDecoderNum = omx_LoadComponent(OMX_COMP_VIDEO_DECODE, 0);
	if (iDecoderNum < 0)
	{
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	
	int iClockNum = omx_LoadComponent(OMX_COMP_CLOCK, 0);
	if (iClockNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	
	int iShedulerNum = omx_LoadComponent(OMX_COMP_VIDEO_SCHEDULER, 0);
	if (iShedulerNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	
	int iEglRenderNum = omx_LoadComponent(OMX_COMP_EGL_RENDER, 2); 
	if (iEglRenderNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iClockNum);
		omx_Release_Component(iShedulerNum);
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	   
	dbgprintf(8,"Comps init done\n"); 
	  
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);    
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);    
   
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	
	omx_set_clock_state(iClockNum, OMX_TIME_ClockStateWaitingForStartTime, 1);
	if (!omx_set_tunnel(iClockNum,OMX_PORT_1, iShedulerNum,OMX_PORT_2, OMX_TRUE)) goto error_out;

	dbgprintf(8,"Set state clock exec\n");   
	omx_set_state(iClockNum, OMX_StateExecuting, OMX_NOW);
	  
	dbgprintf(8,"Set video format\n");
	omx_set_video_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_VIDEO_CodingAVC);
	omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 20, VIDEO_CODER_BUFFER_SIZE);
	 
	omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
	if (!omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN)) goto error_out;
	omx_wait_exec_cmd(iDecoderNum);
	   
	omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
	   
	omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
	omx_add_cmd_in_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
			  
	int status;
	int iFlag, BufferSize;
	omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, &BufferSize);
	int port_settings_changed = 0;
	
	//omx_start_packet StartPack;
	//memset(&StartPack, 0, sizeof(omx_start_packet));
	//StartPack.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
	//StartPack.BufferStartSize = ((VIDEO_WIDTH * VIDEO_HEIGHT * 3) / 2) * (VIDEO_INTRAPERIOD / 10);
	//StartPack.BufferStartFrame = (char*)DBG_MALLOC(StartPack.BufferStartSize);
	//int64_t prev_ms = 0;
	//get_ms(&prev_ms);
    unsigned int iNumFrame = 0;
    int iTimeOut = 0;
    unsigned char cLevel = 5;
    int ret;
    unsigned int uiOffSet = 0;
   
	int64_t iMaxTimeStamp = 0;
	//void *DecBuff;
	AVPacket avpkt;
	avpkt.data = NULL;
	avpkt.size = 0;
	avpkt.flags = 0;
	OMX_BUFFERHEADERTYPE *cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 5000);		
	if (!cur_buffer) dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: Error omx_get_active_buffer ERROR_STOP_VIDEO\n");
	//DecBuff = cur_buffer->pBuffer;
	
    char cClockState = 0;
    
	while (1) 
    {	   
		status = 0; //WAIT
		cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 5000);		
		if (!cur_buffer)
		{
			dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: Error omx_get_active_buffer ERROR_STOP_VIDEO\n");
			status = -1; //EXIT
			cLevel = 4;
			//Menu_ResetShower(-1);
		}
		if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY) == 0)
		{
			status = 3; //EXIT
			dbgprintf(5,"thread_omx_play_video_clk_on_egl_from_func: OMX_EVENT_STOP_VIDEO\n");	
		} 
		
		if (status == 0)
		{
			if (avpkt.size == uiOffSet)
			{
				if (avpkt.flags == 1) av_free_packet(&avpkt);
				if (avpkt.flags == 2) free(avpkt.data);				
				avpkt.data = cur_buffer->pBuffer + cur_buffer->nOffset;
				avpkt.size = BufferSize - cur_buffer->nOffset;
				avpkt.flags = 0;
				iFlag = port_settings_changed;		
				status = e_link2.FuncRecv((void*)&avpkt, &iNumFrame, &iFlag, NULL, 1, e_link2.ConnectNum, e_link2.ConnectID);
				if (status == 1)
				{
					if (avpkt.flags != 0)
					{
						//cur_buffer->pBuffer = avpkt.data;
						uiOffSet = 0;
						cur_buffer->nFilledLen = avpkt.size;
						memcpy(cur_buffer->pBuffer, avpkt.data, cur_buffer->nFilledLen);
					} else cur_buffer->nFilledLen = avpkt.size;		
				}
			}
			else
			{
				if (avpkt.flags != 0)
				{
					//cur_buffer->pBuffer = avpkt.data + uiOffSet;
					cur_buffer->nFilledLen = avpkt.size - uiOffSet;
					if (cur_buffer->nFilledLen > BufferSize) cur_buffer->nFilledLen = BufferSize;
					memcpy(cur_buffer->pBuffer, avpkt.data + uiOffSet, cur_buffer->nFilledLen);	
				}
				else 
				{
					add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, 0);	
					dbgprintf(2, "%s: error rework video data\n", __func__);
					status = 3;
				}
			}
			if (status == 1)
			{
				if (cur_buffer->nFilledLen > BufferSize) cur_buffer->nFilledLen = BufferSize;
				uiOffSet += cur_buffer->nFilledLen;
				cur_buffer->nOffset = 0;
				cur_buffer->nTimeStamp = ToOMXTime(avpkt.pts);
				if (avpkt.pts > iMaxTimeStamp) iMaxTimeStamp = avpkt.pts;
			}
		}
		if (status == 0)
		{
			iTimeOut++;
			if (iTimeOut == 10)
			{
				status = 2;
				dbgprintf(cLevel,"Stop video playing timout\n");	
			}	
		}
		if (status == 4) 
		{
			if ((port_settings_changed == 1) && (cClockState == 1)) 
			{
				uiOffSet = avpkt.size;
				omx_set_clock_speed(iClockNum, 0);
				//omx_set_clock_state(iClockNum, OMX_TIME_ClockStateStopped, 0);				
				cClockState = 0;
				//omx_set_state(iClockNum, OMX_StatePause, OMX_NOW);		
				//omx_set_state(iDecoderNum, OMX_StatePause, OMX_NOW);				
			}
			if (tx_semaphore_exist_in_list(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY))
			{
				tx_semaphore_add_in_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);
				tx_semaphore_go(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY);
				ret = tx_semaphore_wait_event_timeout(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY, 30);
				if (ret == 0) 
				{
					tx_semaphore_delete_from_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);	
					dbgprintf(8,"timeout sync from thread_omx_play_video_clk_on_egl_from_func\n");
				}
				//else dbgprintf(8,"ok sync from thread_omx_play_video_clk_on_egl_from_func %i\n", ret);
			} 
			else 
			{
				usleep(10000);
			}
		}
		if ((status > 0) && (status != 4))
		{
			iTimeOut = 0;
			if (status == 3)
			{
				iFlag |= OMX_BUFFERFLAG_EOS;
				dbgprintf(cLevel,"Stop video playing\n");	
			}
			if (iFlag & OMX_BUFFERFLAG_EXTRADATA)
			{
				if (port_settings_changed == 1)
				{	
					if (iFlag & OMX_BUFFERFLAG_EOS)
					{
						dbgprintf(cLevel,"Extra stop video playing\n");	
						//omx_set_state(iClockNum, OMX_StatePause, OMX_NOW);
						//omx_set_clock_state(iClockNum, OMX_TIME_ClockStateStopped, 0);	
						omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
						//dbgprintf(cLevel,"omx_flush_port\n");	
					}
					else
					{
						dbgprintf(cLevel,"New position video playing\n");	
						omx_set_state(iClockNum, OMX_StatePause, OMX_NOW);
						omx_set_clock_state(iClockNum, OMX_TIME_ClockStateStopped, 0);	
						omx_set_clock_time(iClockNum, OMX_PORT_1, avpkt.pts);
						iMaxTimeStamp = avpkt.pts;
						omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
						omx_set_clock_state(iClockNum, OMX_TIME_ClockStateWaitingForStartTime, 1);
						omx_set_state(iClockNum, OMX_StateExecuting, OMX_NOW);
						iFlag |= OMX_BUFFERFLAG_STARTTIME;
					}
				}
				iFlag ^= OMX_BUFFERFLAG_EXTRADATA;				
			}
			if (iFlag & OMX_BUFFERFLAG_EOS) cur_buffer->nTimeStamp = ToOMXTime(iMaxTimeStamp);
			
			if ((port_settings_changed == 1) && (cClockState == 0)) 
			{
				cClockState = 1;
				omx_set_clock_speed(iClockNum, 1000);
			}
			
			//printf("DECODING %i %i %i %i %i %i\n", (unsigned int)(FromOMXTime(cur_buffer->nTimeStamp)/1000), cur_buffer->nFilledLen, iFlag, iFlag & OMX_BUFFERFLAG_STARTTIME, iFlag & OMX_BUFFERFLAG_TIME_UNKNOWN, iFlag & OMX_BUFFERFLAG_EOS);
			omx_add_cmd_in_list(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
			if (!omx_activate_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, iFlag))
			{
				add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, 0);	
				dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: TIMEOUT_STOP_VIDEO\n");
				status = 3;
			}
			else
			{
				ret = omx_wait_event_timeout(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny, 5000);
				if (ret == 0)
				{
					add_sys_cmd_in_list(SYSTEM_CMD_VIDEO_ERROR, 0);	
					dbgprintf(1,"thread_omx_play_video_clk_on_egl_from_func: TIMEOUT_STOP_VIDEO\n");
					status = 3;
				}
				omx_switch_next_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
				//dbgprintf(8,"Readed from file %i bytes (status: %i)\n", data_len, status);	
				if ((iFlag & OMX_BUFFERFLAG_EOS) && (status < 3)) status = 3;
			}
		}		
		
		if ((status < 0) || (status == 3))
		{
			if (port_settings_changed == 0)
			{
				//dbgprintf(cLevel,"Not success\n");
				omx_delete_cmd_from_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
				omx_delete_cmd_from_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
				e_link->errorcode = 1;
				tx_semaphore_go(e_link->psem_init,2,2);
			}
			else
			{
				if (status < 0) omx_delete_cmd_from_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny);					
			}
			break;
        }
		
		//dbgprintf(8,"current buffer\n");	   
		if ((status == 1) && (port_settings_changed == 0) && (omx_wait_list_empty(iDecoderNum) == 1))
		{
			dbgprintf(8,"Create tunnels\n");
			cClockState = 1;
			omx_set_tunnel(iDecoderNum,OMX_PORT_1, iShedulerNum,OMX_PORT_1, OMX_TRUE);
			omx_set_state(iShedulerNum, OMX_StateExecuting, OMX_NOW);
			omx_set_tunnel(iShedulerNum,OMX_PORT_1, iEglRenderNum,OMX_PORT_1, OMX_TRUE);
			omx_set_state(iEglRenderNum, OMX_StateExecuting, OMX_NOW);
			omx_wait_exec_cmd(iEglRenderNum);
				
			int real_output_width, real_output_height;
			omx_get_frame_resolution(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
			//printf("%i,  %i, %i\n", elink.eglDisplay, elink.eglContext, elink.texture_id);
				
			e_link->attr = iEglRenderNum;
			e_link->sizeW = real_output_width;
			e_link->sizeH = real_output_height;
			//omx_create_egl_buffer(iEglRenderNum, real_output_width, real_output_height, e_link->texture_id, e_link->eglDisplay, e_link->eglContext);
			//dbgprintf(8,"%i,  %i\n",real_output_width,real_output_height);
			tx_semaphore_add_in_list_spec(e_link->psem_init,1,1,0);     
			tx_semaphore_go(e_link->psem_init,2,2);
			tx_semaphore_wait_spec(e_link->psem_init);
			
			omx_enable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT,OMX_LATER);
			omx_use_egl_buffer(iEglRenderNum);
			omx_wait_exec_cmd(iEglRenderNum);
			
			omx_add_cmd_in_list_spec(iEglRenderNum, OMX_EventFillBuffer, OMX_CommandAny, 0);
			if (!omx_activate_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, 0))
			{
				e_link->errorcode = 1;
				tx_semaphore_go(e_link->psem_init,2,2);
				break;
			}
			omx_wait_exec_spec_cmd(iEglRenderNum);
			
			omx_add_cmd_in_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny); 
			tx_semaphore_go(e_link->psem_init,2,2);
			port_settings_changed = 1;			
		}
	}
	
	if (e_link2.ConnectID != 0) CloseConnectID(e_link2.ConnectID);
	
	dbgprintf(8,"OMX Play done1 %i\n",status); 
	
	//cur_buffer->pBuffer = DecBuff;
	
	if (port_settings_changed == 1)
		omx_wait_exec_cmd(iEglRenderNum); 
		else 
		omx_delete_cmd_from_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny);
		
	dbgprintf(8,"OMX Play done2\n");
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);   
	omx_flush_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	
	dbgprintf(8,"OMX Play done3\n");
	 
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateIdle, OMX_NOW);
	if (port_settings_changed == 1)
	{
		omx_set_state(iShedulerNum, OMX_StateIdle, OMX_NOW);
		omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	}
	dbgprintf(8,"OMX Play done3.1\n");
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	omx_wait_exec_cmd(iDecoderNum);
	
	dbgprintf(8,"OMX Play done3.2\n"); 
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
	omx_disable_port(iClockNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iShedulerNum, OMX_PORT_2, OMX_PORT_IN, OMX_LATER);   
	omx_disable_port(iShedulerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);   
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);	
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	if (port_settings_changed == 1) omx_remove_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT);
	
	omx_wait_exec_cmd(iDecoderNum);
	omx_wait_exec_cmd(iShedulerNum);
	omx_wait_exec_cmd(iClockNum);
	omx_wait_exec_cmd(iEglRenderNum);
	
	dbgprintf(8,"OMX Play done3.3\n");
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	if (port_settings_changed == 1) omx_set_tunnel(iClockNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	if (port_settings_changed == 1) omx_set_tunnel(iShedulerNum, OMX_PORT_1, -1, 0, OMX_FALSE);	
	
	dbgprintf(8,"OMX Play done4\n");		
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iClockNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iShedulerNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateLoaded, OMX_NOW);
	
   //eglDestroyImageKHR (*(e_link->eglDisplay), m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage);
   //DBG_FREE(StartPack.BufferCodecInfo);
   //DBG_FREE(StartPack.BufferStartFrame);  
   
   res = 1;
error_out:   
   omx_Release_Component(iDecoderNum);
   omx_Release_Component(iClockNum);
   omx_Release_Component(iShedulerNum);
   omx_Release_Component(iEglRenderNum);
   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_STOP_VIDEO, TX_ANY);   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
	//tx_semaphore_go(&psem_omx_sync, OMX_EVENT_STOP_VIDEO, TX_ANY);
	
   dbgprintf(cLevel,"OMX Play done5\n");
	
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxPlayStatus--;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	DBG_LOG_OUT();  
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)(intptr_t)res;
}

void* thread_omx_play_video_on_egl_from_func(void * pdata)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
    DBG_LOG_IN();
    
    pthread_setname_np(pthread_self(), "play_vid_on_egl");
    
	int res = 0;
	omx_egl_link *e_link = (omx_egl_link*)pdata;
    omx_egl_link e_link2;
    memcpy(&e_link2, pdata, sizeof(omx_egl_link));  
   
    if (m_iStarted == 0) {DBG_LOG_OUT();return (void*)-2;}
   //tx_semaphore_wait(&psem_omx_run);
   
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxPlayStatus++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);	
	
	int iDecoderNum = omx_LoadComponent(OMX_COMP_VIDEO_DECODE, 0);
	if (iDecoderNum < 0)
	{
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	
	int iEglRenderNum = omx_LoadComponent(OMX_COMP_EGL_RENDER, 0);
	if (iEglRenderNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxPlayStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return (void*)-2;
	}  	   
   dbgprintf(8,"Comps init done\n"); 
  
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_STOP_PLAY_VIDEO, e_link2.DeviceNum);    
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, e_link2.DeviceNum);    
	if (e_link2.Type == 2)
	{
		tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_PAUSE, e_link2.DeviceNum); 
		tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_PLAY, e_link2.DeviceNum); 
		tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDUP, e_link2.DeviceNum); 
		tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDDOWN, e_link2.DeviceNum); 
	}
   
   omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
   omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW); 
   
   dbgprintf(8,"Set video format\n");
   omx_set_video_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_VIDEO_CodingAVC);
   omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 5, VIDEO_CODER_BUFFER_SIZE);
   
   omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   
   if (!omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN)) goto error_out; 
   omx_wait_exec_cmd(iDecoderNum);
   
   omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
   
   omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);   
   omx_add_cmd_in_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
   	      
   int 	status;
   int iFlag, BufferSize;
   omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, &BufferSize);
   int port_settings_changed = 0;
   omx_start_packet StartPack;
   memset(&StartPack, 0, sizeof(omx_start_packet));
   StartPack.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
   StartPack.BufferStartSize = 0;
   StartPack.BufferStartFrame = NULL;
  
   unsigned int iNumFrame = 0;
   unsigned char cLevel = 5;
   int ret;
   OMX_BUFFERHEADERTYPE *cur_buffer;
   AVPacket avpkt;
   iFlag = 0;
   
   int iPlaySpeed = 100;
   int iPlayStatus = 1;
    DBG_MUTEX_LOCK(&OMX_mutex);
	omx_speed_play = 1000/iPlaySpeed;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
				
   
	int iLoop = 1;
	//printf("thread_omx_play_video_on_egl_from_func IN\n");
    while (iLoop) 
    {
		if ((tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_STOP_PLAY_VIDEO, e_link2.DeviceNum) == 0))
		{
			dbgprintf(8,"thread_omx_play_video_on_egl_from_func: OMX_EVENT_STOP_VIDEO %i\n", status);
			break;
		}
		
		if (e_link2.Type == 2)
		{
			if ((tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_PLAYER_PLAY, e_link2.DeviceNum) == 0)) 
			{
				iPlayStatus = 1;
				tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_PLAY, e_link2.DeviceNum);   
			}
			if ((tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_PLAYER_PAUSE, e_link2.DeviceNum) == 0)) 
			{
				iPlayStatus = 0;
				tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_PAUSE, e_link2.DeviceNum);   
			}
			if ((tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDUP, e_link2.DeviceNum) == 0)) 
			{
				if (iPlaySpeed > 50) iPlaySpeed /= 2;			
				DBG_MUTEX_LOCK(&OMX_mutex);
				omx_speed_play = 1000/iPlaySpeed;
				if (omx_speed_play < 1) omx_speed_play = 1;
				DBG_MUTEX_UNLOCK(&OMX_mutex);
				tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDUP, e_link2.DeviceNum); 
			}
			if ((tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDDOWN, e_link2.DeviceNum) == 0))
			{
				if (iPlaySpeed < 2000) iPlaySpeed *= 2;									
				DBG_MUTEX_LOCK(&OMX_mutex);
				omx_speed_play = 1000/iPlaySpeed;
				DBG_MUTEX_UNLOCK(&OMX_mutex);
				tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDDOWN, e_link2.DeviceNum); 
			}
				
			if (iPlayStatus == 0)
			{
				usleep(100000);
				continue;
			}
		}
		
		status = 0; //WAIT			
		iFlag = 0;	
		
		cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 1000);		
		if (!cur_buffer)
		{
			dbgprintf(1,"thread_omx_play_video_on_egl_from_func: Error(timeout) omx_get_active_buffer ERROR_STOP_VIDEO\n");
			cLevel = 4;	
			break;
		}			
		
		if (iLoop == 1)
		{
			if ((e_link2.Type == 2) && (port_settings_changed != 0)) usleep(iPlaySpeed*1000);
		
			//if (e_link2.Type == 2) && port_settings_changed) SendRequestNextFrame(e_link2.ConnectNum, e_link2.ConnectID, &e_link2.Address);
			avpkt.data = cur_buffer->pBuffer + cur_buffer->nOffset;
			avpkt.size = BufferSize - cur_buffer->nOffset;			
			if (e_link2.Type == 2)
				status = e_link2.FuncRecv((void*)&avpkt, &iNumFrame, &iFlag, &StartPack, 0, e_link2.ConnectNum, e_link2.ConnectID);
			else
				status = e_link2.FuncRecv((void*)&avpkt, &iNumFrame, &iFlag, &StartPack, 1, e_link2.ConnectNum, e_link2.ConnectID);	
			cur_buffer->nFilledLen = avpkt.size;
			if ((e_link2.Type == 2) && (iFlag == OMX_BUFFERFLAG_EOS) && port_settings_changed) 
			{
				iPlayStatus = 0;
				continue;
			}
			if (iFlag == OMX_BUFFERFLAG_DATACORRUPT) 
			{
				break;
			}
			//printf("Recv %i %i %i %i %i %i\n", gettid(), status, iNumFrame, iFlag, avpkt.size, port_settings_changed);
		}		
		
		if (((status < 0) || (status == 3)) && (port_settings_changed == 0)) break;
		
		if ((iLoop != 1) || (status <= 0) || (status == 3))
		{
			if (port_settings_changed == 0) break;		
			if (iLoop < 3) 
			{
				add_sys_cmd_in_list(SYSTEM_CMD_CAMERA_ERROR, e_link2.DeviceNum);
				iLoop = 2;
			}
			iLoop++;
			usleep(200000);
			status = 0;			
		}
		
		if (status == 4)
		{
			if (tx_semaphore_exist_in_list(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY))
			{
				tx_semaphore_add_in_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);
				tx_semaphore_go(&psem_omx_sync, OMX_EVENT_SYNC_VIDEO, TX_ANY);
				ret = tx_semaphore_wait_event_timeout(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY, 30);
				if (ret == 0) 
				{
					tx_semaphore_delete_from_list(&psem_omx_sync, OMX_EVENT_WAITRENDER_VIDEO, TX_ANY);	
					dbgprintf(8,"timeout sync from thread_omx_play_video_on_egl_from_func\n");
				}
				//else dbgprintf(8,"ok sync from thread_omx_play_video_on_egl_from_func %i\n", ret);
			}
		}
		if (status)
		{
			//printf("Read from file %i bytes, [%i]\n", data_len,iDecoderNum);	
			omx_add_cmd_in_list(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);
			if (!omx_activate_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, iFlag))
			{
				status = 0;
				break;
			}
			ret = omx_wait_event_timeout(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny, 2000);
			if (ret == 0)
			{
				omx_delete_cmd_from_list(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny);  
				dbgprintf(2,"thread_omx_play_video_on_egl_from_func: TIMEOUT_STOP_VIDEO %i\n", e_link2.ConnectNum);
				add_sys_cmd_in_list(SYSTEM_CMD_CAMERA_ERROR, e_link2.DeviceNum);
				iLoop++;
				status = 0;			
			} 
			else omx_switch_next_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
			//dbgprintf(8,"Readed from file %i bytes\n", data_len);	
			//if (iFlag & OMX_BUFFERFLAG_EOS) status = 3;
		}
		
		//iFlag = OMX_BUFFERFLAG_TIME_UNKNOWN;
		if ((status == 1) && (port_settings_changed == 0) && (omx_wait_list_empty(iDecoderNum) == 1))
		{
			dbgprintf(8,"Create tunnels\n");
			
			omx_set_tunnel(iDecoderNum,OMX_PORT_1, iEglRenderNum, OMX_PORT_1, OMX_TRUE);
			omx_set_state(iEglRenderNum, OMX_StateExecuting, OMX_NOW);
			omx_wait_exec_cmd(iEglRenderNum);
		     
			int real_output_width, real_output_height;
			omx_get_frame_resolution(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, &real_output_width, &real_output_height);
			//printf("%i,  %i, %i\n", elink.eglDisplay, elink.eglContext, elink.texture_id);
			
			e_link->attr = iEglRenderNum;
			e_link->sizeW = real_output_width;
			e_link->sizeH = real_output_height;
			//omx_create_egl_buffer(iEglRenderNum, real_output_width, real_output_height, e_link->texture_id, e_link->eglDisplay, e_link->eglContext);
			//dbgprintf(8,"%i,  %i\n",real_output_width,real_output_height);
			tx_semaphore_add_in_list_spec(e_link->psem_init,1,1,0);     
			tx_semaphore_go(e_link->psem_init,2,2);
			tx_semaphore_wait_spec(e_link->psem_init);
		   
			omx_enable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT,OMX_LATER);
			omx_use_egl_buffer(iEglRenderNum);
			omx_wait_exec_cmd(iEglRenderNum);
		  
			omx_add_cmd_in_list_spec(iEglRenderNum, OMX_EventFillBuffer, OMX_CommandAny, 0);
			if (!omx_activate_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, 0))
			{
				e_link->errorcode = 1;
				tx_semaphore_go(e_link->psem_init,2,2);
				break;
			}
			omx_wait_exec_spec_cmd(iEglRenderNum);
		
			omx_add_cmd_in_list(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny); 
			tx_semaphore_go(e_link->psem_init,2,2);
			port_settings_changed = 1;			
		}
		if ((status == 1) && (port_settings_changed == 1) && (omx_wait_list_empty(iEglRenderNum) == 1))
		{
			//printf("Done Play %i\n", gettid());
			break;
		}
		//if (iLoop == 50) break;		
	}
	
	if (e_link2.ConnectID != 0) CloseConnectID(e_link2.ConnectID);
	
	//dbgprintf(8,"Play done1\n"); 
	/*if (port_settings_changed != 0)  
	{
		//omx_wait_exec_cmd(iEglRenderNum); 
		ret = omx_wait_event_timeout(iEglRenderNum, OMX_EventBufferFlag, OMX_CommandAny, 200);
		if (ret == 0) printf("OMX_EventBufferFlag timeout %i\n", gettid());
	}*/
	//dbgprintf(8,"Play done2\n");
	//printf("thread_omx_play_video_on_egl_from_func OUT\n");
	omx_reset_exec_cmd(iDecoderNum);
	omx_reset_exec_cmd(iEglRenderNum);	
	if (omx_wait_list_empty(iDecoderNum) == 0) omx_print_wait_list(iDecoderNum);
	if (omx_wait_list_empty(iEglRenderNum) == 0) omx_print_wait_list(iEglRenderNum);
	
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	omx_wait_exec_cmd(iDecoderNum);
	
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);	
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	if (port_settings_changed != 0)  omx_remove_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT);
	
	omx_wait_exec_cmd(iDecoderNum);
	omx_wait_exec_cmd(iEglRenderNum);
  	
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);	
	
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateLoaded, OMX_NOW);
	dbgprintf(8,"Play done4\n");		
	
   //eglDestroyImageKHR (*(e_link->eglDisplay), m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage);
   res = 1;
error_out:

   DBG_FREE(StartPack.BufferCodecInfo);
   if (StartPack.BufferStartFrame) DBG_FREE(StartPack.BufferStartFrame);  
   
   omx_Release_Component(iDecoderNum);
   omx_Release_Component(iEglRenderNum);
   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_STOP_PLAY_VIDEO, e_link2.DeviceNum);   
   tx_semaphore_go(&psem_omx_run, OMX_EVENT_BUSY_PLAY_VIDEO, e_link2.DeviceNum); 
	if (e_link2.Type == 2)
	{   
		tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_PLAY, e_link2.DeviceNum);   
		tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_PAUSE, e_link2.DeviceNum);   
		tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDUP, e_link2.DeviceNum);   
		tx_semaphore_go(&psem_omx_run, OMX_EVENT_PLAYER_SPEEDDOWN, e_link2.DeviceNum);   
	}
  
	if (port_settings_changed == 0)
	{
		e_link->errorcode = 1;
		tx_semaphore_go(e_link->psem_init,2,2);
	}
	
    dbgprintf(cLevel,"Play done5\n");
  
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxPlayStatus--;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	DBG_LOG_OUT();  
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return (void*)(intptr_t)res;
}

int omx_image_to_buffer(char *FilePath, void ** vBuffer, unsigned int *iSize, int *sW, int *sH)
{  
	DBG_LOG_IN();
	
	int res = 1;
	if (m_iStarted == 0) {DBG_LOG_OUT();return -2;}
	
	int ret = 0;
	DBG_MUTEX_LOCK(&OMX_mutex);
	if ((omx_resource_priority != 1) || omx_IsFree_Encoder()) ret++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	if (ret == 0) return 100;
	
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);    
    
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxImageStatus++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	//tx_semaphore_wait(&psem_omx_run);
    if (*vBuffer != NULL) 
	{
		DBG_FREE(*vBuffer);   
		*vBuffer = NULL;
	}
	*iSize = 0;
	
	OMX_IMAGE_CODINGTYPE codingtype = OMX_IMAGE_CodingUnused;
	int NameLen = strlen(FilePath);
	char *LowFilePath = (char*)DBG_MALLOC(NameLen+1);
	memset(LowFilePath, 0, NameLen+1);
	memcpy(LowFilePath, FilePath, NameLen);
	LowerText(LowFilePath);
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".jpg") != 0)) codingtype = OMX_IMAGE_CodingJPEG;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-5, ".jpeg") != 0)) codingtype = OMX_IMAGE_CodingJPEG;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-5, ".exif") != 0)) codingtype = OMX_IMAGE_CodingEXIF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".tif") != 0)) codingtype = OMX_IMAGE_CodingTIFF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-5, ".tiff") != 0)) codingtype = OMX_IMAGE_CodingTIFF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".png") != 0)) codingtype = OMX_IMAGE_CodingPNG;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".gif") != 0)) codingtype = OMX_IMAGE_CodingGIF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".lzw") != 0)) codingtype = OMX_IMAGE_CodingLZW;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".bmp") != 0)) codingtype = OMX_IMAGE_CodingBMP;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".tga") != 0)) codingtype = OMX_IMAGE_CodingTGA;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".ppm") != 0)) codingtype = OMX_IMAGE_CodingPPM;
	if (codingtype == OMX_IMAGE_CodingUnused)
	{
		dbgprintf(2,"error open file, unknown type: %s\n", FilePath);
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY); 
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxImageStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);	
		DBG_LOG_OUT();		
		return 0;
	}
	
	omx_buffer input_buffer;  
	int n;
		
	if (SearchStrInData(LowFilePath, NameLen, 0, "http://") == 1)
	{
		char cServerName[64];
		memset(cServerName, 0, 64);
		for (n = 7; n < NameLen; n++) if (LowFilePath[n] == 47) break;
		if ((n != NameLen) && ((n - 7) < 64)) 
		{
			memcpy(cServerName, &LowFilePath[7], n - 7);
			//printf("%s    %s\n", cServerName, &LowFilePath[n]);
			input_buffer.data_size = DownloadFileNB(cServerName, &LowFilePath[n], &input_buffer.data);
			if (input_buffer.data_size == 0)
			{
				dbgprintf(1,"Error load file %s\n", LowFilePath);
				tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
				DBG_MUTEX_LOCK(&OMX_mutex);		
				cThreadOmxImageStatus--;
				DBG_MUTEX_UNLOCK(&OMX_mutex);
				DBG_LOG_OUT();
				return -4;
			}
			//else printf("file:%s, len:%i\n", LowFilePath, input_buffer.data_size);
			//omx_dump_data("/var/log/omx.png",input_buffer.data,input_buffer.data_size); 
		} 
		else 
		{
			dbgprintf(1,"Long server name %s\n", LowFilePath);
			tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
			DBG_MUTEX_LOCK(&OMX_mutex);		
			cThreadOmxImageStatus--;
			DBG_MUTEX_UNLOCK(&OMX_mutex);
			DBG_LOG_OUT();
			return -3;
		}
	}
	else
	{
		if (omx_load_file(FilePath, &input_buffer) == 0)
		{
			dbgprintf(1,"error open file: %s\n", FilePath);
			DBG_FREE(LowFilePath);
			tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
			DBG_MUTEX_LOCK(&OMX_mutex);		
			cThreadOmxImageStatus--;
			DBG_MUTEX_UNLOCK(&OMX_mutex);
			DBG_LOG_OUT();
			return 0;
		}
	}
		
	int iDecoderNum = omx_LoadComponent(OMX_COMP_IMAGE_DECODE, 0);
	if (iDecoderNum < 0)
	{
		DBG_FREE(input_buffer.data); 
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxImageStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();		
		return -2;
	}
	dbgprintf(8,"OMX_COMP_IMAGE_DECODE init\n");   
	int iResizerNum = 0;  
		
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	
	omx_set_image_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, codingtype, 0);
	omx_set_frame_settings(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT,  160, 64, 0); 
	omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 2, input_buffer.data_size);

	omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	
	int resized = 0;
	
	res = 1;
	if (omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN))
	{
		omx_wait_exec_cmd(iDecoderNum);
    	omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
		omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
	} else res = -1;
	
	if (res == 1) 
	{
		if (omx_load_data_in_buffer(iDecoderNum, OMX_PORT_1, input_buffer.data, input_buffer.data_size, OMX_BUFFERFLAG_EOS, 1, 1000) < 1)
			res = -1;	
	}
	if (res == 1) 
	{
		dbgprintf(8,"data loaded in buffer (%ix%ibytes), wait decode\n",2,input_buffer.data_size);
		if (omx_wait_event_timeout(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny, 3000) == 0) 
		{ res = -1; dbgprintf(2,"Error decode image\n");}
	}
	if (res)
    {
		//omx_copy_frame_settings(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, iResizerNum, OMX_PORT_1, OMX_PORT_IN);
	
		//omx_set_stride_slice_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, 0,0);
 
		int iW, iH, iW16, iH16;
		int iBufferSize;
		OMX_BUFFERHEADERTYPE *cur_buffer;
		
		omx_get_frame_resolution(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, &iW, &iH);
		iW16 = ((iW)+15)&~15;
		iH16 = ((iH)+15)&~15;
		
		/*if ((*sW > iW) && (*sH > iH))
		{
			printf("111\n");
			omx_set_frame_settings_ex(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT,  0, 0, OMX_COLOR_Format32bitABGR8888, 
											0, 0, OMX_IMAGE_CodingUnused, OMX_FALSE);		
		
			omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
			if (omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT))
			{
				omx_wait_exec_cmd(iDecoderNum); 
				omx_add_cmd_in_list(iDecoderNum, OMX_EventFillBuffer, OMX_CommandAny);  
				if (omx_activate_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, 0))
				{
					omx_wait_exec_cmd(iDecoderNum);	
					omx_get_buffer_size(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, &iBufferSize);
					cur_buffer = omx_get_active_buffer(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, 0);
					if (cur_buffer == NULL) dbgprintf(1,"Error get buffer2\n");
				}
			}
		}
		else*/
		{
			resized = 1;
			//omx_dump_data("/var/log/bitmap2.raw",cur_buffer->pBuffer, iBufferSize); 
			//iH -= 2;
			//printf("W:%i, H:%i, %i\n",iW, iH, iBufferSize);
			iResizerNum = omx_LoadComponent(OMX_COMP_RESIZE, 0);
			if (iResizerNum < 0)
			{
				omx_Release_Component(iDecoderNum);
				DBG_FREE(input_buffer.data); 
				tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);  
				DBG_MUTEX_LOCK(&OMX_mutex);		
				cThreadOmxImageStatus--;
				DBG_MUTEX_UNLOCK(&OMX_mutex);
				DBG_LOG_OUT();
				return -2;
			}
			dbgprintf(8,"OMX_COMP_RESIZE init\n");   
			omx_set_state(iResizerNum, OMX_StateIdle, OMX_NOW);
 
			omx_add_cmd_in_list_spec(iResizerNum, OMX_EventPortSettingsChanged, OMX_CommandAny, 0);  
			omx_set_tunnel(iDecoderNum, OMX_PORT_1, iResizerNum, OMX_PORT_1, OMX_TRUE);
	  
			omx_wait_exec_spec_cmd(iResizerNum);
			omx_add_cmd_in_list_spec(iResizerNum, OMX_EventBufferFlag, OMX_CommandAny, 0);
			omx_set_frame_settings_ex(iResizerNum, OMX_PORT_1, OMX_PORT_OUT,  0, 0, OMX_COLOR_Format32bitABGR8888, 
										0, 0, OMX_IMAGE_CodingUnused, OMX_FALSE);	
			omx_set_state(iResizerNum, OMX_StateExecuting, OMX_NOW);
			//omx_normalize_frame_format(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, 300, 300, *sW, *sH);
			//int real_output_width, real_output_height;
			omx_enable_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
			if (omx_create_buffers(iResizerNum, OMX_PORT_1, OMX_PORT_OUT))
			{
				omx_wait_exec_cmd(iResizerNum); 
				omx_get_buffer_size(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, &iBufferSize);				
				cur_buffer = omx_get_active_buffer(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, 0);
				if (cur_buffer == NULL) dbgprintf(1,"Error get buffer2\n");	
				//else memset(cur_buffer->pBuffer, 0, iBufferSize);
				omx_add_cmd_in_list(iResizerNum, OMX_EventFillBuffer, OMX_CommandAny);  
				if (omx_activate_buffers(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, 0))
					omx_wait_exec_cmd(iResizerNum);
			}		
		}
		dbgprintf(8,"W:%i, H:%i, W16:%i, H16:%i\n",iW, iH, iW16, iH16);
		
		unsigned int uiNewSize = iW*iH*4;
		if (iBufferSize >= uiNewSize)
		{
			char *pBuffer = (char*)DBG_MALLOC(uiNewSize);
			int linesize = iW * 4;
			int linesize16 = iW16 * 4;
			int s = 0;
			int i = 0;
			//memset(pBuffer, 0, uiNewSize);
			for(n = 0; n < iH; n++) 
			{
				memcpy(&pBuffer[i], &cur_buffer->pBuffer[s], linesize);			
				i += linesize;
				s += linesize16;
			}
			*vBuffer = pBuffer;
			*iSize = uiNewSize;
			*sW = iW;
			*sH = iH;		
		}
		else
		{
			res = -1;
			*vBuffer = NULL;
			*iSize = 0;
			*sW = 0;
			*sH = 0;
			dbgprintf(2, "omx_image_to_buffer: Wrong buff size, %i %i %i %i\n", iBufferSize, uiNewSize, iW, iH);
		}	
	}
    
	dbgprintf(8,"Now I want everything to SHUT DOWN\n");
  		
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	omx_wait_exec_cmd(iDecoderNum);	
	
	if (resized)
    {
		omx_set_state(iResizerNum, OMX_StateIdle, OMX_NOW);	
		omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_disable_port(iResizerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);   	
		omx_wait_exec_cmd(iDecoderNum);
		omx_wait_exec_cmd(iResizerNum);	
		
		omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);	
		
		omx_disable_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_remove_buffers(iResizerNum, OMX_PORT_1, OMX_PORT_OUT);
		omx_wait_exec_cmd(iResizerNum);		
	}
	else
	{
		omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT);
		omx_wait_exec_cmd(iDecoderNum);
	}
	
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_wait_exec_cmd(iDecoderNum);
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_Release_Component(iDecoderNum);
	
	if (resized)
	{
		omx_flush_port(iResizerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
		omx_flush_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_wait_exec_cmd(iResizerNum);
		omx_set_state(iResizerNum, OMX_StateLoaded, OMX_NOW);	
		omx_Release_Component(iResizerNum);	
	}
			
	tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
   
	DBG_FREE(input_buffer.data); 
	DBG_FREE(LowFilePath);
	dbgprintf(8,"Play done4\n");
  
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxImageStatus--;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	DBG_LOG_OUT();  
	if (res == -1) return 0;
	return 1;
}

int omx_image_to_egl(char *FilePath, GLuint texture_id, EGLDisplay * eglDisplay, EGLContext * eglContext, void ** eglImage, int *sW, int *sH)
{  
	DBG_LOG_IN();
	
	int error = 0;
	if (m_iStarted == 0) {DBG_LOG_OUT();return -2;}
	
	int ret = 0;
	DBG_MUTEX_LOCK(&OMX_mutex);
	if ((omx_resource_priority != 1) || omx_IsFree_Encoder()) ret++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	if (ret == 0) return 100;
	
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);    
    DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxImageStatus++;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	 
	//tx_semaphore_wait(&psem_omx_run);
    if (*eglImage != NULL) 
	{
		eglDestroyImageKHR (*eglDisplay, *eglImage);   
		*eglImage = NULL;
	}
	
	OMX_IMAGE_CODINGTYPE codingtype = OMX_IMAGE_CodingUnused;
	int NameLen = strlen(FilePath);
	char *LowFilePath = (char*)DBG_MALLOC(NameLen+1);
	memset(LowFilePath, 0, NameLen+1);
	memcpy(LowFilePath, FilePath, NameLen);
	LowerText(LowFilePath);
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".jpg") != 0)) codingtype = OMX_IMAGE_CodingJPEG;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-5, ".jpeg") != 0)) codingtype = OMX_IMAGE_CodingJPEG;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-5, ".exif") != 0)) codingtype = OMX_IMAGE_CodingEXIF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".tif") != 0)) codingtype = OMX_IMAGE_CodingTIFF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-5, ".tiff") != 0)) codingtype = OMX_IMAGE_CodingTIFF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".png") != 0)) codingtype = OMX_IMAGE_CodingPNG;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".gif") != 0)) codingtype = OMX_IMAGE_CodingGIF;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".lzw") != 0)) codingtype = OMX_IMAGE_CodingLZW;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".bmp") != 0)) codingtype = OMX_IMAGE_CodingBMP;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".tga") != 0)) codingtype = OMX_IMAGE_CodingTGA;
	if ((codingtype == OMX_IMAGE_CodingUnused) && (SearchStrInData(LowFilePath, NameLen, NameLen-4, ".ppm") != 0)) codingtype = OMX_IMAGE_CodingPPM;
	if (codingtype == OMX_IMAGE_CodingUnused)
	{
		dbgprintf(2,"error open file, unknown type: %s\n", FilePath);
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);  
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxImageStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return 0;
	}
	
	omx_buffer input_buffer;  
	
	if (SearchStrInData(LowFilePath, NameLen, 0, "http://") == 1)
	{
		char cServerName[64];
		memset(cServerName, 0, 64);
		int n;
		for (n = 7; n < NameLen; n++) if (LowFilePath[n] == 47) break;
		if ((n != NameLen) && ((n - 7) < 64)) 
		{
			memcpy(cServerName, &LowFilePath[7], n - 7);
			//printf("%s    %s\n", cServerName, &LowFilePath[n]);
			input_buffer.data_size = DownloadFileNB(cServerName, &LowFilePath[n], &input_buffer.data);
			if (input_buffer.data_size == 0)
			{
				dbgprintf(1,"Error load file %s\n", LowFilePath);
				tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);  
				DBG_MUTEX_LOCK(&OMX_mutex);		
				cThreadOmxImageStatus--;
				DBG_MUTEX_UNLOCK(&OMX_mutex);
				DBG_LOG_OUT();
				return -4;
			}
			//else printf("file:%s, len:%i\n", LowFilePath, input_buffer.data_size);
			//omx_dump_data("/var/log/omx.log",input_buffer.data,input_buffer.data_size); 
		} 
		else 
		{
			dbgprintf(1,"Long server name %s\n", LowFilePath);
			tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
			DBG_MUTEX_LOCK(&OMX_mutex);		
			cThreadOmxImageStatus--;
			DBG_MUTEX_UNLOCK(&OMX_mutex);
			DBG_LOG_OUT();
			return -3;
		}
	}
	else
	{
		if (omx_load_file(FilePath, &input_buffer) == 0)
		{
			dbgprintf(1,"error open file: %s\n", FilePath);
			DBG_FREE(LowFilePath);
			tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
			DBG_MUTEX_LOCK(&OMX_mutex);		
			cThreadOmxImageStatus--;
			DBG_MUTEX_UNLOCK(&OMX_mutex);
			DBG_LOG_OUT();
			return 0;
		}
	}
		
	int iDecoderNum = omx_LoadComponent(OMX_COMP_IMAGE_DECODE, 0);
	if (iDecoderNum < 0)
	{
		DBG_FREE(input_buffer.data); 
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxImageStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_IMAGE_DECODE init\n");   
	int iResizerNum = omx_LoadComponent(OMX_COMP_RESIZE, 0);
	if (iResizerNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		DBG_FREE(input_buffer.data); 
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxImageStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_RESIZE init\n");   
	int iEglRenderNum = omx_LoadComponent(OMX_COMP_EGL_RENDER, 0);
	if (iEglRenderNum < 0)
	{
		omx_Release_Component(iDecoderNum);
		omx_Release_Component(iResizerNum);
		DBG_FREE(input_buffer.data); 
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
		DBG_MUTEX_LOCK(&OMX_mutex);		
		cThreadOmxImageStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		DBG_LOG_OUT();
		return -2;
	}
	dbgprintf(8,"OMX_COMP_EGL_RENDER init\n");  
	
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iResizerNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
 
	omx_set_image_compression_format(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, codingtype, 0);
	omx_set_frame_settings(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT,  160, 64, 0); 
	omx_set_count_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, 2, input_buffer.data_size);

	omx_enable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	if (!omx_create_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN)) {error = 1; goto error_out;}
	omx_wait_exec_cmd(iDecoderNum);  
  
	omx_set_state(iDecoderNum, OMX_StateExecuting, OMX_NOW);
		
	omx_add_cmd_in_list(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);

	omx_load_data_in_buffer(iDecoderNum, OMX_PORT_1, input_buffer.data, input_buffer.data_size, OMX_BUFFERFLAG_EOS, 1, 1000);
	//omx_add_cmd_in_list_spec(iDecoderNum, OMX_EventEmptyBuffer, OMX_CommandAny, 0);
	//omx_activate_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);  
	//omx_wait_exec_spec_cmd(iDecoderNum);
	dbgprintf(8,"data loaded in buffer (%ix%ibytes), wait decode\n",2,input_buffer.data_size);
	if (omx_wait_event_timeout(iDecoderNum, OMX_EventPortSettingsChanged, OMX_CommandAny, 3000) == 0) 
		{ error = 1; dbgprintf(2,"Error decode image\n");}
	
	// {
	omx_set_stride_slice_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, 0,0);
 
	int iW, iH;
	omx_get_frame_resolution(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, &iW, &iH);
	omx_normalize_frame_format(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, iW, iH, *sW, *sH);

	omx_add_cmd_in_list_spec(iResizerNum, OMX_EventPortSettingsChanged, OMX_CommandAny, 0);  
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, iResizerNum, OMX_PORT_1, OMX_TRUE);
  
	omx_wait_exec_spec_cmd(iResizerNum);
	omx_add_cmd_in_list_spec(iResizerNum, OMX_EventBufferFlag, OMX_CommandAny, 0);
    
	omx_set_state(iResizerNum, OMX_StateExecuting, OMX_NOW);
  
	omx_set_tunnel(iResizerNum, OMX_PORT_1, iEglRenderNum, OMX_PORT_1, OMX_TRUE);
        
	omx_add_cmd_in_list(iEglRenderNum, OMX_EventPortSettingsChanged, OMX_CommandAny);
	omx_set_state(iEglRenderNum, OMX_StateExecuting, OMX_LATER);
	omx_wait_exec_cmd(iEglRenderNum);
  
	//int real_output_width, real_output_height;
	omx_get_frame_resolution(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, sW, sH);
  
	omx_create_egl_buffer(iEglRenderNum, *sW, *sH, texture_id, eglDisplay, eglContext);
    *eglImage = m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage;
	
	omx_enable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT,OMX_LATER);
	omx_use_egl_buffer(iEglRenderNum);
	omx_wait_exec_cmd(iEglRenderNum);

	omx_wait_exec_cmd(iResizerNum);
  
	if (error == 0)
    {
		omx_add_cmd_in_list_spec(iEglRenderNum, OMX_EventFillBuffer, OMX_CommandAny, 0);
		if (omx_activate_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, 0))
			omx_wait_exec_spec_cmd(iEglRenderNum);
	}
 // } else dbgo_printf ("Error decode image\n");
    
	dbgprintf(8,"Now I want everything to SHUT DOWN\n");
  
	omx_set_state(iDecoderNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iResizerNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateIdle, OMX_NOW);
	
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iDecoderNum, OMX_PORT_1, OMX_PORT_IN);
	omx_wait_exec_cmd(iDecoderNum);
  
  
	//eglDestroyImageKHR (*e_link->eglDisplay, m_oCompList[iEglRenderNum].out_port[OMX_PORT_1].EglImage);
   
	omx_disable_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_disable_port(iResizerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
   
	omx_disable_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);   
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
  
	omx_wait_exec_cmd(iDecoderNum);
	omx_wait_exec_cmd(iResizerNum);
	omx_wait_exec_cmd(iEglRenderNum);
  
	omx_set_tunnel(iDecoderNum, OMX_PORT_1, -1, 0, OMX_FALSE);
	omx_set_tunnel(iResizerNum, OMX_PORT_1, -1, 0, OMX_FALSE);
  
	omx_disable_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_remove_buffers(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT);
	omx_wait_exec_cmd(iEglRenderNum);
   
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_flush_port(iDecoderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_flush_port(iResizerNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_flush_port(iResizerNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_flush_port(iEglRenderNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_wait_exec_cmd(iDecoderNum);
	omx_wait_exec_cmd(iResizerNum);
	omx_wait_exec_cmd(iEglRenderNum);
  
	omx_set_state(iDecoderNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iResizerNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iEglRenderNum, OMX_StateLoaded, OMX_NOW);

error_out:
	omx_Release_Component(iDecoderNum);
	omx_Release_Component(iResizerNum);
	omx_Release_Component(iEglRenderNum);
   
	tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_VIDEO, TX_ANY);   
   
	DBG_FREE(input_buffer.data); 
	DBG_FREE(LowFilePath);
	dbgprintf(8,"Play done4\n");
  
	DBG_MUTEX_LOCK(&OMX_mutex);		
	cThreadOmxImageStatus--;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	DBG_LOG_OUT();  
	if (error) return 0;
	return 1;
}

int omx_image_encode_in_file(int iWidth, int iHeight, char *pData, unsigned int uiLenData, char *file_name)
{
	DBG_LOG_IN();
	
	FILE *video_file;
	int iFileLen = 0;
	int iWriteLen = 0;
	int res = 1;
	OMX_BUFFERHEADERTYPE *cur_buffer;
	  
	int iImageEncodeNum = omx_LoadComponent(OMX_COMP_IMAGE_ENCODE, 1);
	//printf("OMX_COMP_IMAGE_ENCODE init %ix%i\n", iWidth, iHeight);   
	  
	omx_image_encoder_init(iImageEncodeNum, iWidth, iHeight, OMX_IMAGE_CodingJPEG);  
    omx_set_state(iImageEncodeNum, OMX_StateIdle, OMX_NOW);
	  
	omx_enable_port(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_enable_port(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	  
	if (res && (!omx_create_buffers(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN))) res = 0;  
	if (res && (!omx_create_buffers(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT))) res = 0;
	
	int iBufferSize = 0;
	omx_get_buffer_size(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN, &iBufferSize);
	if (iBufferSize < uiLenData) 
	{
		dbgprintf(2, "omx_image_encode_in_file: low buffer size:%i sizedata:%i\n", iBufferSize, uiLenData);
		res = 0;
	}
	
	if (res)
	{
		omx_set_state(iImageEncodeNum, OMX_StateExecuting, OMX_NOW);
		if ((video_file = fopen(file_name,"wb+")) == NULL)
		{
			dbgprintf(1, "omx_image_encode_in_file: Error create file:%s\n", file_name);
			res = 0;
		}
	}
	
	if (res)
	{
		cur_buffer = omx_get_active_buffer(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN, 200);
		if (!cur_buffer)
		{
			dbgprintf(1, "omx_image_encode_in_file: Error omx_get_active_buffer in\n");
			res = 0;
		}
	}
	if (res)
	{
		memcpy(cur_buffer->pBuffer, pData, uiLenData);
		cur_buffer->nFilledLen = uiLenData;
		if (!omx_activate_buffers(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN, 0))
		{
			res = 0;
		}
		else
		{
			cur_buffer = omx_get_active_buffer(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 200);
			if (!cur_buffer)
			{
				dbgprintf(1, "omx_image_encode_in_file: Error omx_get_active_buffer out\n");
				res = 0;
			}
		}
	}
	
	while((res) && ((cur_buffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) == 0))
	{	
		omx_add_cmd_in_list(iImageEncodeNum, OMX_EventFillBuffer, OMX_CommandAny);
		if (!omx_activate_buffers(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 0))
		{
			res = 0;
			break;
		}
		omx_wait_exec_cmd(iImageEncodeNum);
		cur_buffer = omx_get_active_buffer(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 200);
		if (!cur_buffer)
		{
			dbgprintf(1, "omx_image_encode_in_file: Error omx_get_active_buffer out2\n");
			res = 0;
			break;
		}
		iWriteLen = fwrite(cur_buffer->pBuffer + cur_buffer->nOffset, 1, cur_buffer->nFilledLen, video_file);
		if(iWriteLen != cur_buffer->nFilledLen) 
		{
			dbgprintf(1, "omx_image_encode_in_file: Error write file:%s\n", file_name);
			res = 0;
			break;
		}
		iFileLen += iWriteLen;
		//printf("Write file:%i/%i  %i %i\n", iWriteLen,iFileLen, cur_buffer->nFlags, cur_buffer->nFlags & OMX_BUFFERFLAG_EOS);
		//usleep(1000);
	}
	  
	fclose(video_file);
	  
	omx_flush_port(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	omx_disable_port(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_disable_port(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_remove_buffers(iImageEncodeNum, OMX_PORT_1, OMX_PORT_IN); 
	omx_remove_buffers(iImageEncodeNum, OMX_PORT_1, OMX_PORT_OUT); 

	omx_set_state(iImageEncodeNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iImageEncodeNum, OMX_StateLoaded, OMX_NOW);
	  
	omx_Release_Component(iImageEncodeNum);
	//dbgprintf(4, "omx_image_encode_in_file: done\n");	
	DBG_LOG_OUT();
	return 1;
}

void* thread_omx_video_capture_send(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "video_capture");
	
	//DBG_ON(DBG_MODE_MEMORY_ERROR | DBG_MODE_PRINTF | DBG_MODE_TRACK_FUNC_THREAD);
	omx_egl_link *e_link = pData;
	MODULE_INFO *miModule = e_link->pModule;
	MODULE_INFO rctVirtModule;
	memset(&rctVirtModule, 0, sizeof(MODULE_INFO));
	
	image_sensor_params ispCurrentCameraParams;
	image_sensor_params ispNewCameraParams;
	memset(&ispCurrentCameraParams, 0, sizeof(image_sensor_params));
	omx_get_image_sensor_module_params(&ispCurrentCameraParams, miModule);
	memcpy(&ispNewCameraParams, &ispCurrentCameraParams, sizeof(image_sensor_params));
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	memcpy(&ispCameraImageSettings, &ispCurrentCameraParams, sizeof(image_sensor_params));
	DBG_MUTEX_UNLOCK(&modulelist_mutex);	
	
	char cPreviewIconRendered = 0;
	unsigned int uiPreviewIconFramesToRender = 60;
	unsigned int uiSlowFramesCnt = 1000;	
	unsigned int uiSettings = miModule->Settings[8];	
	unsigned int uiSlowFramesSkip = miModule->Settings[9];
	unsigned int uiMoveFramesSave = miModule->Settings[10] & 0xFFFF;
	unsigned int uiMoveFramesSaveSlow = miModule->Settings[11] & 0xFFFF;
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	
	if (uiMoveFramesSave == 0) uiMoveFramesSave = 1;
	if (uiMoveFramesSaveSlow == 0) uiMoveFramesSaveSlow = 1;
	unsigned char ucMoveLevelSlowDiff = miModule->Settings[12] & 255;
	unsigned int uiMoveSizeDiffSet = miModule->Settings[13];
	unsigned int uiMoveSizeSlowDiffSet = miModule->Settings[14];
	int iSendTypeStartFrame = miModule->Settings[15];
	char cSlowEncoderEnabled = uiSettings & 1;
	char cSlowOnlyDiffRecord = uiSettings & 2;
	char cPreviewEnabled = uiSettings & 4;
	char cRecSlowEnabled = uiSettings & 8;
	if (cRecSlowEnabled == 0) 
	{
		cSlowEncoderEnabled = 0;
		cSlowOnlyDiffRecord = 0;
	}
	char cDiffRecordEnabled = (uiSettings & 16) ? 1 : 0;
	char cNormRecordEnabled = (uiSettings & 32) ? 1 : 0;
	char cMainConstantBitRate = (uiSettings & 256) ? 1 : 0; 
	char cPrevConstantBitRate = (uiSettings & 512) ? 1 : 0; 
	char cSlowConstantBitRate = (uiSettings & 1024) ? 1 : 0; 
	char cGPUResources = (uiSettings >> 12) & 2;
	char cGPUfree = 1;
	int iNeedSend;
	int iNeedRTSP;
	int iHighLightMode = (uiSettings & 16384) ? 1 : 0; 
	int iHighLightBright = (uiSettings >> 16) & 255;
	int iHighLightAmplif = (uiSettings >> 24) & 255;
	
	char cLongNeedControl = miModule->Settings[36] & 7; 
	int iLongScanSet = miModule->Settings[33];
	unsigned char ucLongBrightMoveLevel = miModule->Settings[34] & 255;	
	unsigned char ucLongColorMoveLevel = miModule->Settings[35] & 255;	
	
	char cOmxEncoderStatus = 0;
	char cCurEncoderStatus = 0;
	
	DBG_MUTEX_LOCK(&OMX_mutex);
	cThreadOmxCaptureStatus++;
	omx_resource_priority = cGPUResources;
	cThreadOmxEncoderStatus = cOmxEncoderStatus;
	DBG_MUTEX_UNLOCK(&OMX_mutex);		
	
	unsigned int uiMainAvcProfile = GetOmxProfile(miModule->Settings[19] & 255);
	unsigned int uiPrevAvcProfile = GetOmxProfile((miModule->Settings[19] >> 8) & 255);
	unsigned int uiSlowAvcProfile = GetOmxProfile((miModule->Settings[19] >> 16) & 255);
	unsigned int uiMainAvcLevel = GetOmxLevel(miModule->Settings[20] & 255);
	unsigned int uiPrevAvcLevel = GetOmxLevel((miModule->Settings[20] >> 8) & 255);
	unsigned int uiSlowAvcLevel = GetOmxLevel((miModule->Settings[20] >> 16) & 255);
	int iSensorResizerEnabled = 0;
	if (cPreviewEnabled == 0) miModule->Settings[37] = miModule->Settings[5];		
	if (miModule->Settings[37] != miModule->Settings[5]) iSensorResizerEnabled = 1;
	
	int iPtzStatuses[PTZ_STATUSES_MAX];
	int iPtzPrevStatuses[PTZ_STATUSES_MAX];
	omx_autofocus_params autofocus_pm;
	memset(&autofocus_pm, 0, sizeof(omx_autofocus_params));
	autofocus_pm.PtzEnabled = miModule->Settings[38] & 1;
	autofocus_pm.Enabled = miModule->Settings[38] & 2;
	autofocus_pm.PtzFocusMapEnabled = miModule->Settings[38] & 8;
	autofocus_pm.PtzID = miModule->Settings[39];
	autofocus_pm.SensorWidth = OMX_AUTOFOCUS_WIDTH;
	autofocus_pm.SensorHeight = OMX_AUTOFOCUS_HEIGHT;
	autofocus_pm.GotoHomeAfterStart = (miModule->Settings[42] >> 12) & 1;
	autofocus_pm.GotoHomeAfterTimeout = (miModule->Settings[42] >> 13) & 1;
	autofocus_pm.GotoHomeTime = miModule->Settings[42] & 12;
	autofocus_pm.GotoHomeCounter = 0;
	autofocus_pm.GotoHomeDone = 1;
	
	if (autofocus_pm.PtzEnabled && 
		(!autofocus_pm.Enabled || !autofocus_pm.PtzID ||
		!autofocus_pm.SensorWidth || !autofocus_pm.SensorHeight)) autofocus_pm.Enabled = 0;
	
		
	STREAM_INFO *pCaptStreamNorm = NULL;
	STREAM_INFO *pCaptStreamSlow = NULL;
	STREAM_INFO *pCaptStreamDiff = NULL;
	
	DBG_MUTEX_LOCK(&system_mutex);	
	int n, cn;
	cn = iStreamCnt;
	unsigned char cBcTCP = ucBroadCastTCP;
	DBG_MUTEX_UNLOCK(&system_mutex);	
	
	for (n = 0; n < cn; n++)
	{
		DBG_MUTEX_LOCK(&SyncStream[n]->mutex);
		if (miModule->ID == SyncStream[n]->VidID)
		{
			if (SyncStream[n]->Type == CAPTURE_TYPE_FULL) pCaptStreamNorm = SyncStream[n];
			if (SyncStream[n]->Type == CAPTURE_TYPE_SLOW) pCaptStreamSlow = SyncStream[n];
			if (SyncStream[n]->Type == CAPTURE_TYPE_DIFF) pCaptStreamDiff = SyncStream[n];
		}
		DBG_MUTEX_UNLOCK(&SyncStream[n]->mutex);		
	}
	if (cNormRecordEnabled && (pCaptStreamNorm == NULL)) {dbgprintf(1, "Error find video CAPTURE_TYPE_FULL stream id\n"); cNormRecordEnabled = 0;}
	if (cDiffRecordEnabled && (pCaptStreamDiff == NULL)) {dbgprintf(1, "Error find video CAPTURE_TYPE_DIFF stream id\n"); cDiffRecordEnabled = 0;}
	if (cSlowEncoderEnabled && (pCaptStreamSlow == NULL)) {dbgprintf(1, "Error find video CAPTURE_TYPE_SLOW stream id\n"); cSlowEncoderEnabled = 0;}
	
	//FILE *video_file;
	//int iFileLen = 0;
	//int iWriteLen = 0;
	
	unsigned int iNumFrame = START_VIDEO_FRAME_NUMBER;
	unsigned int iEncFrame = START_VIDEO_FRAME_NUMBER;
	unsigned int iEncFramePrev = START_VIDEO_FRAME_NUMBER;
	unsigned int iEncFrameSlow = START_VIDEO_FRAME_NUMBER;
	
	
	void *pOriginalMainEncoderBuffer = NULL;
	void *pOriginalPrevEncoderBuffer = NULL;
	void *pOriginalSensResizeBuffer = NULL;
	
	//////SENSOR SETTINGS/////
	char cNeedControl = 0;
	if (miModule->ScanSet != 0) cNeedControl = 1;
	
	int ret, ret2, iDiffSize, iMoveSize, iDiffMoveSize, iSlowMoveSize, res, iColorMoveSize;
	unsigned int uiLightSize, uiRectDetected, uiCurBright;
	int iOMX_RectangleCnt = 0;
	RECT_INFO *riOMX_RectangleList = NULL;
	
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_STOP_CAPTURE, TX_ANY);    
	tx_semaphore_add_in_list(&psem_omx_run, OMX_EVENT_BUSY_CAPTURE, TX_ANY);    
     
	  
	OMX_BUFFERHEADERTYPE *pMainEncoderInBuffer;
	OMX_BUFFERHEADERTYPE *pMainEncoderOutBuffer;
	OMX_BUFFERHEADERTYPE *pPrevEncoderInBuffer;
	OMX_BUFFERHEADERTYPE *pPrevEncoderOutBuffer;
	OMX_BUFFERHEADERTYPE *pSlowEncoderInBuffer;
	OMX_BUFFERHEADERTYPE *pSlowEncoderOutBuffer;
	OMX_BUFFERHEADERTYPE *pCameraOutBuffer;
	OMX_BUFFERHEADERTYPE *pPrevResizerOutBuffer;
	OMX_BUFFERHEADERTYPE *pSensResizerInBuffer;
	OMX_BUFFERHEADERTYPE *pSensResizerOutBuffer;
	//OMX_BUFFERHEADERTYPE temp_buff;
	int iResizerNumSens = -1; 
	int iResizerNumPrev = -1;
	int iVideoEncodeNumSlow = -1;
	int iCameraNum = -1;
	int iVideoEncodeNum = -1;
	int iVideoEncodeNumPrev = -1;
	int iCtrlLoad = 0;
	  
	omx_buffer timestamptext;  
	
	if (omx_load_file("Textures/text1626x24.raw", &timestamptext) == 0)
	{
		dbgprintf(1,"error open file: Textures/text1626x24.raw\n");
		
		if (e_link->pSubModule) DBG_FREE(e_link->pSubModule);
		if (e_link->pModule) DBG_FREE(e_link->pModule);
		DBG_FREE(e_link);	
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_CAPTURE, TX_ANY);   
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_CAPTURE, TX_ANY);
		DBG_MUTEX_LOCK(&OMX_mutex);
		cThreadOmxCaptureStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		
		DBG_LOG_OUT();
		return (void*)0;
	}
	
	dbgprintf(8,"OMX_COMP_CAMERA init %i\n", iCameraNum);   
	if (iCtrlLoad >= 0) iCtrlLoad = iCameraNum = omx_LoadComponent(OMX_COMP_CAMERA, 1);
	
	dbgprintf(8,"OMX_COMP_VIDEO_ENCODE Main init %i\n", iVideoEncodeNum);   
	if (iCtrlLoad >= 0) iCtrlLoad = iVideoEncodeNum = omx_LoadComponent(OMX_COMP_VIDEO_ENCODE, 1);
	
	if (cPreviewEnabled)
	{
		dbgprintf(8,"OMX_COMP_VIDEO_ENCODE Preview init %i\n", iVideoEncodeNumSlow);   	
		if (iCtrlLoad >= 0) iCtrlLoad = iVideoEncodeNumPrev = omx_LoadComponent(OMX_COMP_VIDEO_ENCODE, 1);		
	}
	
	if (cSlowEncoderEnabled)
	{
		dbgprintf(8,"OMX_COMP_VIDEO_ENCODE Slow init %i\n", iVideoEncodeNumSlow);   
		if (iCtrlLoad >= 0) iCtrlLoad = iVideoEncodeNumSlow = omx_LoadComponent(OMX_COMP_VIDEO_ENCODE, 1);		
	}
	
	dbgprintf(8,"OMX_COMP_RESIZE Preview init %i\n", iVideoEncodeNum);   
	if (iCtrlLoad >= 0) iCtrlLoad = iResizerNumPrev = omx_LoadComponent(OMX_COMP_RESIZE, 1);
	
	if (iSensorResizerEnabled)
	{
		dbgprintf(8,"OMX_COMP_RESIZE Sensor init %i\n", iVideoEncodeNum);   
		if (iCtrlLoad >= 0) iCtrlLoad = iResizerNumSens = omx_LoadComponent(OMX_COMP_RESIZE, 1); 
	}
	
	if (iCtrlLoad < 0)
	{
		if (iCameraNum >= 0) omx_Release_Component(iCameraNum);
		if (iVideoEncodeNum >= 0) omx_Release_Component(iVideoEncodeNum);
		if ((cPreviewEnabled) && (iVideoEncodeNumPrev >= 0)) omx_Release_Component(iVideoEncodeNumPrev);
		if ((cSlowEncoderEnabled) && (iVideoEncodeNumSlow >= 0)) omx_Release_Component(iVideoEncodeNumSlow);
		if (iResizerNumPrev >= 0) omx_Release_Component(iResizerNumPrev);
		if (iSensorResizerEnabled && (iResizerNumSens >= 0)) omx_Release_Component(iResizerNumSens);
		DBG_FREE(timestamptext.data);
		if (e_link->pSubModule) DBG_FREE(e_link->pSubModule);
		if (e_link->pModule) DBG_FREE(e_link->pModule);
		DBG_FREE(e_link);	
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_CAPTURE, TX_ANY);   
		tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_CAPTURE, TX_ANY);
		DBG_MUTEX_LOCK(&OMX_mutex);
		cThreadOmxCaptureStatus--;
		DBG_MUTEX_UNLOCK(&OMX_mutex);
		
		DBG_LOG_OUT();
		return (void*)-2;
	}	
	dbgprintf(8,"OMX_COMP_RESIZE init %i\n", iResizerNumPrev);  
	
	VideoCodecInfo tVideoInfo;
	VideoCodecInfo tVideoInfoSlow;
	VideoCodecInfo tVideoInfoPreview;
	VideoCodecInfo tVideoInfoSensor;
	
	memset(&tVideoInfo, 0, sizeof(VideoCodecInfo)); 
	tVideoInfo.video_codec = AV_CODEC_ID_H264;
	tVideoInfo.video_bit_rate = miModule->Settings[4]*1000;
	GetResolutionFromModule(miModule->Settings[1], &tVideoInfo.video_width, &tVideoInfo.video_height);
	tVideoInfo.video_width32 = ((tVideoInfo.video_width+31)&~31);
	tVideoInfo.video_height16 = ((tVideoInfo.video_height+15)&~15);
	tVideoInfo.video_frame_rate = miModule->Settings[2];
	tVideoInfo.video_intra_frame = miModule->Settings[3];
	
	if (tVideoInfo.video_frame_rate < 2) 
	{
		dbgprintf(2, "Main frame rate wrong, set 5\n");
		tVideoInfo.video_frame_rate = 5;
	}
	if (tVideoInfo.video_intra_frame < 1) 
	{
		dbgprintf(2, "Main key frame wrong, set 60\n");
		tVideoInfo.video_intra_frame = 60;
	}
	if (tVideoInfo.video_bit_rate < 1) 
	{
		dbgprintf(2, "Main bit rate wrong, set 5000\n");
		tVideoInfo.video_bit_rate = 5000;
	}
	
	tVideoInfo.video_pixel_format = AV_PIX_FMT_YUV420P;
	tVideoInfo.video_frame_size = ((tVideoInfo.video_width * tVideoInfo.video_height16 * 3)/2);	
	tVideoInfo.Filled = 1;
	int iMatrixBufferSize = tVideoInfo.video_width32*tVideoInfo.video_height16;
	memcpy(&tVideoInfoSlow, &tVideoInfo, sizeof(VideoCodecInfo));
	tVideoInfoSlow.video_bit_rate = miModule->Settings[18]*1000;
	tVideoInfoSlow.video_frame_rate = miModule->Settings[23];
	tVideoInfoSlow.video_intra_frame = miModule->Settings[22];
	if (tVideoInfoSlow.video_frame_rate < 2)
	{
		dbgprintf(2, "Slow frame rate wrong, set %i\n", tVideoInfo.video_frame_rate);
		tVideoInfoSlow.video_frame_rate = tVideoInfo.video_frame_rate;
	}
	if (tVideoInfoSlow.video_intra_frame < 1) 
	{
		dbgprintf(2, "Slow key frame wrong, set %i\n", tVideoInfo.video_intra_frame);
		tVideoInfoSlow.video_intra_frame = tVideoInfo.video_intra_frame;
	}
	if (tVideoInfoSlow.video_bit_rate < 1) 
	{
		dbgprintf(2, "Slow bit rate wrong, set %i\n", tVideoInfo.video_bit_rate);
		tVideoInfoSlow.video_bit_rate = tVideoInfo.video_bit_rate;
	}
	
	memset(&tVideoInfoPreview, 0, sizeof(VideoCodecInfo)); 
	tVideoInfoPreview.video_codec = AV_CODEC_ID_H264;
	tVideoInfoPreview.video_bit_rate = miModule->Settings[17]*1000;
	GetResolutionFromModule(miModule->Settings[37], &tVideoInfoPreview.video_width, &tVideoInfoPreview.video_height);
	tVideoInfoPreview.video_width32 = ((tVideoInfoPreview.video_width+31)&~31);
	tVideoInfoPreview.video_height16 = ((tVideoInfoPreview.video_height+15)&~15);	
	tVideoInfoPreview.video_frame_rate = tVideoInfo.video_frame_rate;
	tVideoInfoPreview.video_intra_frame = miModule->Settings[21];
	if (tVideoInfoPreview.video_frame_rate < 2) 
	{
		dbgprintf(2, "Preview frame rate wrong, set %i\n", tVideoInfo.video_frame_rate);
		tVideoInfoPreview.video_frame_rate = tVideoInfo.video_frame_rate;
	}	
	if (tVideoInfoPreview.video_intra_frame < 1) 
	{
		dbgprintf(2, "Preview key frame wrong, set %i\n", tVideoInfo.video_intra_frame);
		tVideoInfoPreview.video_intra_frame = tVideoInfo.video_intra_frame;
	}
	if (tVideoInfoPreview.video_bit_rate < 1000) 
	{
		dbgprintf(2, "Preview bit rate wrong, set %i\n", tVideoInfo.video_bit_rate);
		tVideoInfoPreview.video_bit_rate = tVideoInfo.video_bit_rate;
	}
	tVideoInfoPreview.video_pixel_format = AV_PIX_FMT_YUV420P;
	tVideoInfoPreview.video_frame_size = ((tVideoInfoPreview.video_width * tVideoInfoPreview.video_height16 * 3)/2);	
	tVideoInfoPreview.Filled = 1;	
	int iPreviewMatrixSize = tVideoInfoPreview.video_width32*tVideoInfoPreview.video_height16;
	
	memset(&tVideoInfoSensor, 0, sizeof(VideoCodecInfo)); 
	tVideoInfoSensor.video_codec = AV_CODEC_ID_H264;
	tVideoInfoSensor.video_bit_rate = miModule->Settings[17]*1000;
	GetResolutionFromModule(miModule->Settings[5], &tVideoInfoSensor.video_width, &tVideoInfoSensor.video_height);
	tVideoInfoSensor.video_width32 = ((tVideoInfoSensor.video_width+31)&~31);
	tVideoInfoSensor.video_height16 = ((tVideoInfoSensor.video_height+15)&~15);	
	tVideoInfoSensor.video_frame_rate = tVideoInfo.video_frame_rate;
	tVideoInfoSensor.video_intra_frame = miModule->Settings[21];
	tVideoInfoSensor.video_frame_rate = tVideoInfo.video_frame_rate;
	tVideoInfoSensor.video_intra_frame = tVideoInfo.video_intra_frame;
	tVideoInfoSensor.video_bit_rate = tVideoInfo.video_bit_rate;
	tVideoInfoSensor.video_pixel_format = AV_PIX_FMT_YUV420P;
	tVideoInfoSensor.video_frame_size = ((tVideoInfoSensor.video_width * tVideoInfoSensor.video_height16 * 3)/2);	
	tVideoInfoSensor.Filled = 1;	
	int iSensorMatrixSize = tVideoInfoSensor.video_width32*tVideoInfoSensor.video_height16;
	
	unsigned char ucMoveLevel = miModule->Settings[6] & 255;
	if (ucMoveLevel > 100) ucMoveLevel = 100;
	ucMoveLevel = (unsigned char)(255 * ucMoveLevel / 100);	
	dbgprintf(8,"MOVELEVEL:%i\n",ucMoveLevel);
	unsigned char ucColorMoveLevel = miModule->Settings[16] & 255;	
	dbgprintf(8,"COLORMOVELEVEL:%i\n",ucColorMoveLevel);
	unsigned char ucMoveLevelDiff = miModule->Settings[7] & 255;	
	dbgprintf(8,"MOVELEVELDIFF:%i\n",ucMoveLevelDiff);
	dbgprintf(8,"MOVELEVELSLOWDIFF:%i\n",ucMoveLevelSlowDiff);
	unsigned int uiTempSensorID = miModule->Settings[0];
	unsigned int uiCurrentExposureControl = GetStandartExposure(miModule->Settings[24]); //OMX_ExposureControlSports;
	miModule->Settings[24] = uiCurrentExposureControl;
	//printf("!!!  %s\n", GetNameExposure(uiCurrentExposureControl));
	unsigned int uiCurrentImageFilter = GetStandartImageFilter(miModule->Settings[28]);
	miModule->Settings[28] = uiCurrentImageFilter;
	unsigned int uiCurrentWhiteBalance = GetStandartWhiteBalance(miModule->Settings[29]);
	miModule->Settings[29] = uiCurrentWhiteBalance;
	unsigned int uiCurrentBrightControl = miModule->Settings[25];
	if (uiCurrentBrightControl > 100) uiCurrentBrightControl = 50;
	int iCurrentContrastControl = miModule->Settings[30];
	if ((iCurrentContrastControl < -100) || (iCurrentContrastControl > 100)) iCurrentContrastControl = 0;
	int iCurrentSharpnessControl = miModule->Settings[31];
	if ((iCurrentSharpnessControl < -100) || (iCurrentSharpnessControl > 100)) iCurrentSharpnessControl = 0;
	int iCurrentSaturationControl = miModule->Settings[32];
	if ((iCurrentSaturationControl < -100) || (iCurrentSaturationControl > 100)) iCurrentSaturationControl = 0;
		
	unsigned int uiBrightControlSense = 1;	
	
	unsigned char *ucMainCameraMask = NULL;
	unsigned char *ucMainCameraPrevBuff = NULL;
	unsigned char *ucMainCameraResultBuff = NULL;
	if (iHighLightMode)
	{
		ucMainCameraPrevBuff = (unsigned char*)DBG_MALLOC(tVideoInfo.video_frame_size);
		ucMainCameraMask = (unsigned char*)DBG_MALLOC(tVideoInfo.video_frame_size);
		ucMainCameraResultBuff = (unsigned char*)DBG_MALLOC(tVideoInfo.video_frame_size);
		memset(ucMainCameraPrevBuff, 0, tVideoInfo.video_frame_size);
		memset(ucMainCameraMask, 0, tVideoInfo.video_frame_size);
		memset(ucMainCameraResultBuff, 0, tVideoInfo.video_frame_size);
	}
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	int iCameraModuleNum = ModuleIdToNum(miModule->ID, 1);
	int iTempModuleNum = ModuleIdToNum(miModule->Settings[0], 2);
	int iPtzModuleNum = ModuleIdToNum(autofocus_pm.PtzID, 1);
	miModuleList[iCameraModuleNum].Settings[24] = uiCurrentExposureControl;
	miModuleList[iCameraModuleNum].Settings[28] = uiCurrentImageFilter;
	miModuleList[iCameraModuleNum].Settings[29] = uiCurrentWhiteBalance;
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	int iCurrentTemp = -10000;
	int iSensorPrevMove = 0;
	int iSensorPrevSizeMove = 0;
	int iSensorPrevColorSizeMove = 0;
	int iSensorPrevLight = 0;	
	int iSensorPrevRect = 0;
	char cAreaTempSensor = 0;
	if (iTempModuleNum == -1) cAreaTempSensor = 1;	
	if (iPtzModuleNum == -1) 
	{
		autofocus_pm.PtzEnabled = 0;
		autofocus_pm.Enabled = 0;
	}
	unsigned int uiStepFrameMS = 0;	
	//unsigned int uiResPrev = 0;
	int i, k;
	
	CopyRectList(iRectangleCnt, riRectangleList, &iOMX_RectangleCnt, &riOMX_RectangleList);
	RecalcRectangles(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfo.video_width, tVideoInfo.video_height, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height);
	for (i = 0; i < iOMX_RectangleCnt; i++) 
	{
		riOMX_RectangleList[i].Detected = 0; 
		riOMX_RectangleList[i].DetectSize = 0; 
		riOMX_RectangleList[i].ColorDetected = 0; 
		riOMX_RectangleList[i].ColorDetectSize = 0;
	}
	
	//tVideoInfoSlow.video_frame_rate = 1;
	//tVideoInfoSlow.video_intra_frame = 1; 
	dbgprintf(8,"omx_camera_init %ix%ix%i\n", tVideoInfo.video_width, tVideoInfo.video_height, tVideoInfo.video_frame_rate);  	
	omx_camera_init(iCameraNum, tVideoInfo.video_width, tVideoInfo.video_height, tVideoInfo.video_frame_rate, VIDEO_COLOR_FORMAT, OMX_FALSE);
	omx_camera_apply_params(iCameraNum, &ispCurrentCameraParams, &ispCurrentCameraParams, 1);
	//omx_set_fov_value(iCameraNum, 10, 10);
	//dbgprintf(8,"omx_set_exposure_value\n", tVideoInfo.video_width, tVideoInfo.video_height, tVideoInfo.video_frame_rate);  	
	//omx_set_exposure_value(iCameraNum, OMX_ExposureControlNight, CAM_METERING, CAM_EXPOSURE_COMPENSATION, CAM_SHUTTER_SPEED, CAM_SHUTTER_SPEED_AUTO, CAM_EXPOSURE_ISO_SENSITIVITY, CAM_EXPOSURE_AUTO_SENSITIVITY);

	dbgprintf(8,"omx_video_encoder_init %i\n", iVideoEncodeNum);  	
	omx_video_encoder_init(iVideoEncodeNum, tVideoInfo.video_width, tVideoInfo.video_height, 
					tVideoInfo.video_frame_rate, tVideoInfo.video_bit_rate, VIDEO_CODEC, tVideoInfo.video_intra_frame, OMX_TRUE, cMainConstantBitRate, uiMainAvcProfile, uiMainAvcLevel); 
	if (ispCurrentCameraParams.KeepRatio) omx_set_ratio_value(iVideoEncodeNum, ispCurrentCameraParams.AspectRatioW, ispCurrentCameraParams.AspectRatioH);
	/*if (ispCurrentCameraParams.KeepRatio) 
	{
		printf("%ix%i >> %ix%i\n", tVideoInfo.video_width, tVideoInfo.video_height, ispCurrentCameraParams.AspectRatioW, ispCurrentCameraParams.AspectRatioH);
	}*/
		
	dbgprintf(8,"preview resizer_init %i  %ix%i\n", iVideoEncodeNum, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height);  	
	omx_normalize_frame_format(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, tVideoInfo.video_width, tVideoInfo.video_height, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height);
	omx_set_frame_resolution(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height, OMX_COLOR_FormatYUV420PackedPlanar);

	if (iSensorResizerEnabled)
	{
		dbgprintf(8,"sensor resizer_init %i  %ix%i\n", iResizerNumSens, tVideoInfoSensor.video_width, tVideoInfoSensor.video_height);  	
		omx_normalize_frame_format(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height, tVideoInfoSensor.video_width, tVideoInfoSensor.video_height);
		omx_set_frame_resolution(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height, OMX_COLOR_FormatYUV420PackedPlanar);
		omx_set_frame_resolution(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, tVideoInfoSensor.video_width, tVideoInfoSensor.video_height, OMX_COLOR_FormatYUV420PackedPlanar);
	}
	
	if (cPreviewEnabled) 
	{
		dbgprintf(8,"omx_video_encoder_init Prev %i\n", iVideoEncodeNumPrev);  	
		omx_video_encoder_init(iVideoEncodeNumPrev, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height, 
					tVideoInfoPreview.video_frame_rate, tVideoInfoPreview.video_bit_rate, VIDEO_CODEC, tVideoInfoPreview.video_intra_frame, OMX_TRUE, cPrevConstantBitRate, uiPrevAvcProfile, uiPrevAvcLevel); 
	}
	if (cSlowEncoderEnabled)
	{
		dbgprintf(8,"omx_video_encoder_init Slow %i\n", iVideoEncodeNumSlow);  	
		omx_video_encoder_init(iVideoEncodeNumSlow, tVideoInfoSlow.video_width, tVideoInfoSlow.video_height, 
					tVideoInfoSlow.video_frame_rate, tVideoInfoSlow.video_bit_rate, VIDEO_CODEC, tVideoInfoSlow.video_intra_frame, OMX_TRUE, cSlowConstantBitRate, uiSlowAvcProfile, uiSlowAvcLevel);	
	}		
	
	omx_start_packet StartPack;
	memset(&StartPack, 0, sizeof(omx_start_packet));
	StartPack.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
	StartPack.BufferStartSize = ((tVideoInfo.video_width * tVideoInfo.video_height * 3) / 2) * tVideoInfo.video_intra_frame / 10;
	StartPack.BufferStartFrame = (char*)DBG_MALLOC(StartPack.BufferStartSize);
	StartPack.StartFramesSizes = (unsigned int*)DBG_CALLOC(tVideoInfo.video_intra_frame, sizeof(unsigned int)); 
	StartPack.StartFramesFlags = (unsigned char*)DBG_CALLOC(tVideoInfo.video_intra_frame, 1); 
	StartPack.StartFramesCount = 0;
	StartPack.HaveChangedFrames = 1;
	StartPack.SendType = iSendTypeStartFrame;
	StartPack.PacketType = 0;
	uiStepFrameMS = (1000 / tVideoInfo.video_frame_rate) / 2;
	
	omx_start_packet StartPackSlow;
	memset(&StartPackSlow, 0, sizeof(omx_start_packet));
	if (cSlowEncoderEnabled)
	{
		StartPackSlow.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
		StartPackSlow.BufferStartSize = ((tVideoInfoSlow.video_width * tVideoInfoSlow.video_height * 3) / 2) * tVideoInfoSlow.video_intra_frame / 10;
		StartPackSlow.BufferStartFrame = (char*)DBG_MALLOC(StartPackSlow.BufferStartSize);
		StartPackSlow.StartFramesSizes = (unsigned int*)DBG_CALLOC(tVideoInfoSlow.video_intra_frame, sizeof(unsigned int)); 
		StartPackSlow.StartFramesFlags = (unsigned char*)DBG_CALLOC(tVideoInfoSlow.video_intra_frame, 1); 
		StartPackSlow.StartFramesCount = 0;
		StartPackSlow.HaveChangedFrames = 1;	
		StartPackSlow.SendType = iSendTypeStartFrame;	
		StartPackSlow.PacketType = 2;
	}
	
	omx_start_packet StartPackPrev;
	memset(&StartPackPrev, 0, sizeof(omx_start_packet));
	if (cPreviewEnabled)
	{
		StartPackPrev.BufferCodecInfo = (char*)DBG_MALLOC(CODECINFO_BUFFER_SIZE);
		StartPackPrev.BufferStartSize = ((tVideoInfoPreview.video_width * tVideoInfoPreview.video_height * 3) / 2) * tVideoInfoPreview.video_intra_frame / 10;
		StartPackPrev.BufferStartFrame = (char*)DBG_MALLOC(StartPackPrev.BufferStartSize);
		StartPackPrev.StartFramesSizes = (unsigned int*)DBG_CALLOC(tVideoInfoPreview.video_intra_frame, sizeof(unsigned int)); 
		StartPackPrev.StartFramesFlags = (unsigned char*)DBG_CALLOC(tVideoInfoPreview.video_intra_frame, 1); 
		StartPackPrev.StartFramesCount = 0;
		StartPackPrev.SendType = iSendTypeStartFrame;
		StartPackPrev.PacketType = 1;
	}
	
	omx_set_tunnel(iCameraNum, OMX_PORT_1, iResizerNumPrev, OMX_PORT_1, OMX_FALSE);
	
	omx_set_state(iCameraNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iVideoEncodeNum, OMX_StateIdle, OMX_NOW);
	if (cPreviewEnabled) omx_set_state(iVideoEncodeNumPrev, OMX_StateIdle, OMX_NOW);
	if (cSlowEncoderEnabled) omx_set_state(iVideoEncodeNumSlow, OMX_StateIdle, OMX_NOW);
	omx_set_state(iResizerNumPrev, OMX_StateIdle, OMX_NOW);	
	if (iSensorResizerEnabled) omx_set_state(iResizerNumSens, OMX_StateIdle, OMX_NOW);	
	
	omx_set_count_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 1, VIDEO_CODER_BUFFER_SIZE);	
	if (cPreviewEnabled) omx_set_count_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, 1, VIDEO_CODER_BUFFER_SIZE);	
	if (cSlowEncoderEnabled) omx_set_count_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, 1, VIDEO_CODER_BUFFER_SIZE);	
	
	//omx_enable_port(iCameraNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_enable_port(iCameraNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_enable_port(iCameraNum, OMX_PORT_2, OMX_PORT_OUT, OMX_LATER);
	omx_enable_port(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_enable_port(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	if (cPreviewEnabled) 
	{
		omx_enable_port(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
		omx_enable_port(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	}
	if (cSlowEncoderEnabled)
	{
		omx_enable_port(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
		omx_enable_port(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);	
	}
	
	omx_add_cmd_in_list_spec(iResizerNumPrev, OMX_EventPortSettingsChanged, OMX_CommandAny, 0);  
	omx_enable_port(iResizerNumPrev, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_enable_port(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);	
	omx_wait_exec_spec_cmd(iResizerNumPrev);
	
	if (iSensorResizerEnabled)
	{
		omx_add_cmd_in_list_spec(iResizerNumSens, OMX_EventPortSettingsChanged, OMX_CommandAny, 0);  
		omx_enable_port(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
		omx_enable_port(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);	
		omx_wait_exec_spec_cmd(iResizerNumSens);
		if (!omx_create_buffers(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN)) goto stopcapt;	
		if (!omx_create_buffers(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT)) goto stopcapt;
	}
	
	if (!omx_create_buffers(iCameraNum, OMX_PORT_2, OMX_PORT_OUT)) goto stopcapt;
	if (!omx_create_buffers(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT)) goto stopcapt;	
	if (!omx_create_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN)) goto stopcapt;
	if (!omx_create_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT)) goto stopcapt;
	
	if (cPreviewEnabled) 
	{
		if (!omx_create_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN)) goto stopcapt;
		if (!omx_create_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT)) goto stopcapt;
	}
	if (cSlowEncoderEnabled)
	{
		if (!omx_create_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN)) goto stopcapt;
		if (!omx_create_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT)) goto stopcapt;
	}
	/*if ((video_file = fopen(e_link->data,"wb+")) == NULL)
	{
		dbgprintf(1,("Error create file:%s\n", e_link->data);
		return 0;
	} */ 

	omx_set_state(iResizerNumPrev, OMX_StateExecuting, OMX_NOW);
	if (iSensorResizerEnabled) omx_set_state(iResizerNumSens, OMX_StateExecuting, OMX_NOW);
	omx_set_state(iCameraNum, OMX_StateExecuting, OMX_NOW);
	omx_set_state(iVideoEncodeNum, OMX_StateExecuting, OMX_NOW);
	if (cPreviewEnabled) omx_set_state(iVideoEncodeNumPrev, OMX_StateExecuting, OMX_NOW);
	if (cSlowEncoderEnabled) omx_set_state(iVideoEncodeNumSlow, OMX_StateExecuting, OMX_NOW);
	
	int iBufferSize = 0;
	int iBufferSizePrev = 0;
	int iEncBuffCntPrev = 0;
	int iBufferSizeSlow = 0;
	int iEncBuffCntSlow = 0;
	int iSensorBufferSize = 0;
	int iPreviewBufferSize = 0;
	int iColorSize = 0;
	int iColorBufferSize = 0;
	
	int iDiffValue;
	
	omx_get_buffer_size(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, &iPreviewBufferSize);
	if (iSensorResizerEnabled)
		omx_get_buffer_size(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, &iSensorBufferSize);
		else 
		iSensorBufferSize = iPreviewBufferSize;
	iColorSize = iSensorBufferSize - iSensorMatrixSize;
	iColorBufferSize = iSensorMatrixSize + (iColorSize / 2);
	
	//omx_get_buffer_count(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN, &iEncBuffCnt);
	omx_get_buffer_size(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, &iBufferSize);
	if (cPreviewEnabled) 
	{
		omx_get_buffer_count(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN, &iEncBuffCntPrev);
		omx_get_buffer_size(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, &iBufferSizePrev);
	}
	if (cSlowEncoderEnabled)
	{
		omx_get_buffer_count(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN, &iEncBuffCntSlow);
		omx_get_buffer_size(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, &iBufferSizeSlow);
	}
		
	omx_set_camera_capture_status(iCameraNum, OMX_TRUE, OMX_FALSE);	
	omx_set_exposure_value(iCameraNum, uiCurrentExposureControl);
	omx_set_imagefilter_value(iCameraNum, uiCurrentImageFilter);
	omx_set_whitebalance_value(iCameraNum, uiCurrentWhiteBalance);
	
	DBG_MUTEX_LOCK(&modulelist_mutex);	
	miModuleList[iCameraModuleNum].Status[2] = uiCurrentExposureControl;
	miModuleList[iCameraModuleNum].Status[7] = 0;
	miModuleList[iCameraModuleNum].Status[8] = 0;
	miModuleList[iCameraModuleNum].Status[9] = 0;
	miModuleList[iCameraModuleNum].Status[10] = 0;
	miModuleList[iCameraModuleNum].Status[11] = 0;
	miModuleList[iCameraModuleNum].Status[12] = 0;
	miModuleList[iCameraModuleNum].Status[15] = uiCurrentBrightControl;
	miModuleList[iCameraModuleNum].Status[16] = ispCameraImageSettings.AutoBrightControl; //(uiSettings & 2048) ? 1 : 0;
	miModuleList[iCameraModuleNum].Status[17] = ispCameraImageSettings.DestBrightControl; //miModule->Settings[27];
	miModuleList[iCameraModuleNum].Status[18] = 0;
	miModuleList[iCameraModuleNum].Status[19] = uiCurrentImageFilter;
	miModuleList[iCameraModuleNum].Status[20] = uiCurrentWhiteBalance;
	miModuleList[iCameraModuleNum].Status[21] = iCurrentContrastControl;
	miModuleList[iCameraModuleNum].Status[22] = iCurrentSharpnessControl;
	miModuleList[iCameraModuleNum].Status[23] = iCurrentSaturationControl;
	
	unsigned int uiAutoBrightControl = ispCameraImageSettings.AutoBrightControl;
	unsigned int uiDestBrightControl = ispCameraImageSettings.DestBrightControl;
	unsigned int uiISOControl = ispCameraImageSettings.ISOControl;
	unsigned int uiHardAutoISOControl = ispCameraImageSettings.HardAutoISOControl;
	unsigned int uiSoftAutoISOControl = ispCameraImageSettings.SoftAutoISOControl;
	unsigned int uiDestAutoISOControl = ispCameraImageSettings.DestAutoISOControl; 
	unsigned int uiEVControl = ispCameraImageSettings.EVControl;
	//unsigned int uiHardAutoEVControl = ispCameraImageSettings.HardAutoEVControl;
	unsigned int uiSoftAutoEVControl = ispCameraImageSettings.SoftAutoEVControl;
	unsigned int uiDestAutoEVControl = ispCameraImageSettings.DestAutoEVControl; 
	int uiCurrentEVControl = uiEVControl;
	int uiCurrentISOControl = uiISOControl;	
		
	ispCameraImageSettings.HighLightMode = iHighLightMode;
	ispCameraImageSettings.HighLightBright = iHighLightBright;
	ispCameraImageSettings.HighLightAmplif = iHighLightAmplif;
	unsigned int uiPrevShowLevel = ispCameraImageSettings.PrevShowLevel;
	ispCameraImageSettings.PrevColorLevel = ucColorMoveLevel;
	ispCameraImageSettings.PrevBrigLevel = ucMoveLevel;
	unsigned int uiPrevColorLevel = ispCameraImageSettings.PrevColorLevel;
	unsigned int uiPrevBrigLevel = ispCameraImageSettings.PrevBrigLevel;
	unsigned int uiShowMainRect = ispCameraImageSettings.MainSettings;	
	unsigned int uiPrevSettings = ispCameraImageSettings.PrevSettings;
	unsigned int uiPrevCamBrigShow = ispCameraImageSettings.PrevSettings & 1;
	unsigned int uiPrevCamColorShow = ispCameraImageSettings.PrevSettings & 2;
	unsigned int uiPrevCamGaneEnabled = ispCameraImageSettings.PrevSettings & 4;
	unsigned int uiPrevCamRectShow = ispCameraImageSettings.PrevSettings & 128;
	unsigned int uiPrevBrigSett = (ispCameraImageSettings.PrevSettings >> 3) & 3;
	unsigned int uiPrevColorSett = (ispCameraImageSettings.PrevSettings >> 5) & 3;	
	
	
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	unsigned int uiPrevSettingsTimer = 0;
		
	unsigned char *pucMovePrevBuffer = NULL;
	unsigned char *pucLongPrevDiff12 = NULL;
	unsigned char *pucLongPrevDiff23 = NULL;
	unsigned char *pucLongPrevDiff34 = NULL;
	unsigned char *pucLongPrevDiff24 = NULL;
	unsigned char *pucLongPrevFrame1 = NULL;
	unsigned char *pucLongPrevFrame2 = NULL;
	unsigned char *pucLongPrevFrame3 = NULL;
	unsigned char *pucLongPrevFrame4 = NULL;
	unsigned char *pucLongLimitDiff12 = NULL;
	unsigned char *pucLongLimitDiff23 = NULL;
	unsigned char *pucLongLimitDiff34 = NULL;
	unsigned char *pucLongLimitDiff24 = NULL;
	unsigned char *pucDiffMovePrevBuffer = NULL;
	unsigned char ucFirstLoop = 1;
	unsigned char ucFirstPacket = 1;
	unsigned char ucFirstPacketPrev = 1;
	unsigned char ucFirstPacketSlow = 1;
	if (cNeedControl) pucMovePrevBuffer = (unsigned char*)DBG_MALLOC(iSensorBufferSize);
	if (cLongNeedControl) 
	{		
		pucLongPrevDiff12 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
		pucLongPrevDiff23 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
		pucLongPrevDiff34 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
		pucLongPrevFrame3 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
		pucLongPrevFrame4 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
		if (cLongNeedControl & (2 | 4)) 
		{
			pucLongPrevFrame2 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
			pucLongPrevDiff24 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);			
		}			
		if (cLongNeedControl & 4) 
		{
			pucLongPrevFrame1 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
			
			pucLongLimitDiff12 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
			pucLongLimitDiff23 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
			pucLongLimitDiff34 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);
			pucLongLimitDiff24 = (unsigned char*) DBG_MALLOC(iSensorBufferSize);		
		}
	}
	
	if ((cDiffRecordEnabled) ||(cSlowOnlyDiffRecord)) pucDiffMovePrevBuffer = (unsigned char*) DBG_MALLOC(iSensorMatrixSize);
	
	unsigned int iTimeScanClk = 0;
	unsigned int iLongTimeScanClk = 0;
	unsigned int iLongScanStep = 0;
	unsigned int iTimeScanTemp = tVideoInfo.video_intra_frame;
	unsigned int iTimeScanNewParams = 0;
	unsigned int iTimeoutLastMove = 1;
	unsigned int iTimeoutLastMoveSlow = 1;
	char cShotFile[256];
	int iTimeOutPrev = 1000 / tVideoInfoPreview.video_frame_rate * 2;
	int iTimeOutMain = 1000 / tVideoInfo.video_frame_rate * 2;
	
	void* vControlBufferNorm = NULL;
	void* vControlBufferPrev = NULL;
	void* vControlBufferSlow = NULL;
	char cStop = 0;
	char cCriticalError = 0;
	
	//////////GET ALL BUFFERS>>>>>>>>>>>>>>>>//////////
	do
	{
		pMainEncoderInBuffer = omx_get_active_buffer(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN, 0);
		if (pMainEncoderInBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pMainEncoderInBuffer\n");cStop = 1; break;}
		
		pMainEncoderOutBuffer = omx_get_active_buffer(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 0);
		if (pMainEncoderOutBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pMainEncoderOutBuffer\n");cStop = 1; break;}
		pMainEncoderOutBuffer->nFilledLen = 0;
		vControlBufferNorm = pMainEncoderOutBuffer->pBuffer;
		
		pCameraOutBuffer = omx_get_active_buffer(iCameraNum, OMX_PORT_2, OMX_PORT_OUT, 0);		
		if (pCameraOutBuffer == NULL){dbgprintf(1,"error omx_get_active_buffer pCameraOutBuffer\n");cStop = 1; break;}
		
		pPrevResizerOutBuffer = omx_get_active_buffer(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, 0);		
		if (pPrevResizerOutBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pPrevResizerOutBuffer\n");cStop = 1; break;}
		
		if (iSensorResizerEnabled) 
		{
			pSensResizerInBuffer = omx_get_active_buffer(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN, 0);		
			if (pSensResizerInBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pSensResizerInBuffer\n");cStop = 1; break;}
		
			pSensResizerOutBuffer = omx_get_active_buffer(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, 0);		
			if (pSensResizerOutBuffer == NULL)	{dbgprintf(1,"error omx_get_active_buffer pSensResizerOutBuffer\n");cStop = 1; break;}
		}
		
		omx_add_cmd_in_list(iVideoEncodeNum, OMX_EventFillBuffer, (int)(intptr_t)pMainEncoderOutBuffer->pAppPrivate);
		if (!omx_activate_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 0)) {cStop = 1; break;}	
		
		if (cPreviewEnabled) 
		{
			pPrevEncoderInBuffer = omx_get_active_buffer(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN, 0);
			if (pPrevEncoderInBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pPrevEncoderInBuffer\n");cStop = 1; break;}
		
			pPrevEncoderOutBuffer = omx_get_active_buffer(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, 0);
			if (pPrevEncoderOutBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pPrevEncoderOutBuffer\n");cStop = 1; break;}		
			pPrevEncoderOutBuffer->nFilledLen = 0;
			vControlBufferPrev = pPrevEncoderOutBuffer->pBuffer;
			
			omx_add_cmd_in_list(iVideoEncodeNumPrev, OMX_EventFillBuffer, (int)(intptr_t)pPrevEncoderOutBuffer->pAppPrivate);
			if (!omx_activate_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, 0)) {cStop = 1; break;}	
		}
		
		if (cSlowEncoderEnabled)
		{
			pSlowEncoderInBuffer = omx_get_active_buffer(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN, 0);
			if (pSlowEncoderInBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pSlowEncoderInBuffer\n");cStop = 1; break;}
			
			pSlowEncoderOutBuffer = omx_get_active_buffer(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, 0);
			if (pSlowEncoderOutBuffer == NULL) {dbgprintf(1,"error omx_get_active_buffer pSlowEncoderOutBuffer\n");cStop = 1; break;}
			pSlowEncoderOutBuffer->nFilledLen = 0;
			vControlBufferSlow = pSlowEncoderOutBuffer->pBuffer;
			
			omx_add_cmd_in_list(iVideoEncodeNumSlow, OMX_EventFillBuffer, (int)(intptr_t)pSlowEncoderOutBuffer->pAppPrivate);
			if (!omx_activate_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, 0)) {cStop = 1; break;}	
		}
	} while(1 == 2);
	/////<<<<<<<<<GET ALL BUFFERS//////////
	
	char cPrevTemp;
	//////////EXCHANGE BUFFERS CAMERA AND ENCODER>>>>>>>>>>>>>>>>//////////
	pOriginalMainEncoderBuffer = pMainEncoderInBuffer->pBuffer;
	pMainEncoderInBuffer->pBuffer = pCameraOutBuffer->pBuffer;
	/////<<<<<<<<<EXCHANGE BUFFERS CAMERA AND ENCODER//////////
	
	//////////EXCHANGE BUFFERS PREV RESIZER AND ENCODER>>>>>>>>>>>>>>>>//////////
	if (cPreviewEnabled) 
	{
		pOriginalPrevEncoderBuffer = pPrevEncoderInBuffer->pBuffer;
		pPrevEncoderInBuffer->pBuffer = pPrevResizerOutBuffer->pBuffer;
	}
	/////<<<<<<<<<EXCHANGE BUFFERS PREV RESIZER AND ENCODER//////////
		
	//////////EXCHANGE BUFFERS SENS RESIZER AND ENCODER>>>>>>>>>>>>>>>>//////////
	if (iSensorResizerEnabled)
	{
		pOriginalSensResizeBuffer = pSensResizerInBuffer->pBuffer;
		pSensResizerInBuffer->pBuffer = pPrevResizerOutBuffer->pBuffer;
	}
	else pSensResizerOutBuffer = pPrevResizerOutBuffer;
	/////<<<<<<<<<EXCHANGE BUFFERS SENS RESIZER AND ENCODER//////////
	
	if (autofocus_pm.Enabled)
	{
		autofocus_pm.SensorPosX = (tVideoInfo.video_width / 2) - (autofocus_pm.SensorWidth / 2);
		autofocus_pm.SensorPosY = (tVideoInfo.video_height / 2) - (autofocus_pm.SensorHeight / 2);	
		if (autofocus_pm.SensorPosX == 0) autofocus_pm.SensorPosX = 1;
		if (autofocus_pm.SensorPosY == 0) autofocus_pm.SensorPosY = 1;
		autofocus_pm.CurrCoef = 0.5f;
		if (autofocus_pm.PtzFocusMapEnabled)
		{
			autofocus_pm.PtzFocusMap = (uint16_t*)DBG_MALLOC(2300*sizeof(uint16_t));
			if (!omx_load_file_size("Settings/ptz_focus_map.bin", (char*)autofocus_pm.PtzFocusMap, 2300*sizeof(uint16_t)))
			{
				for (i = 0; i < 2300; i++) autofocus_pm.PtzFocusMap[i] = 0;
				omx_dump_data("Settings/ptz_focus_map.bin", (char*)autofocus_pm.PtzFocusMap, 2300*sizeof(uint16_t)); 
			}
			omx_PrintFocusMapFilled(&autofocus_pm);
		}
	}
	
	dbgprintf(8,"Loop to capture\n");
	
	///LOOP///	
	
	while(cStop == 0) 
	{	

		if (tx_semaphore_exist_in_list(&psem_omx_run, OMX_EVENT_STOP_CAPTURE, TX_ANY) == 0)
		{
			dbgprintf(8,"OMX_EVENT_STOP_CAPTURE\n");	
			cStop = 1;
		}
		//pMainEncoderInBuffer->nFilledLen = 0;
		//omx_wait_exec_cmd(iVideoEncodeNum);
		//dbgprintf(3,"get capture frame\n");
		
		///////////GET CAMERA AND RESIZE FRAME>>>>>>>>>>>////////////
		omx_add_cmd_in_list(iCameraNum, OMX_EventFillBuffer, OMX_CommandAny);
		if (tx_semaphore_exist_in_list(m_oCompList[iResizerNumPrev].psem, OMX_EventFillBuffer, TX_ANY) == 0)
			omx_add_cmd_in_list(iResizerNumPrev, OMX_EventFillBuffer, OMX_CommandAny);
		if (!omx_activate_buffers(iCameraNum, OMX_PORT_2, OMX_PORT_OUT, 0)) break;
		if (!omx_activate_buffers(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, 0)) break;
		
		ret = tx_semaphore_wait_event_timeout(m_oCompList[iCameraNum].psem, OMX_EventFillBuffer, OMX_CommandAny, 1000);
		if (ret == 0) dbgprintf(2,"error get OMX_EventFillBuffer iCameraNum, %i\n", iCameraNum);
		ret = tx_semaphore_wait_event_timeout(m_oCompList[iResizerNumPrev].psem, OMX_EventFillBuffer, OMX_CommandAny, 1000);
		if (ret == 0) dbgprintf(2,"error get OMX_EventFillBuffer iResizerNumPrev, %i\n", iResizerNumPrev);
		///////<<<<<<<<<<GET CAMERA AND RESIZE FRAME////////////

		///////////GET RESIZE SENSOR FRAME>>>>>>>>>>>////////////
		if (iSensorResizerEnabled)
		{
			//printf("222222 %i %i\n", iPreviewBufferSize, iSensorBufferSize, pPrevResizerOutBuffer->nFilledLen, pSensResizerInBuffer->pAppPrivate); 
			pSensResizerInBuffer->nFilledLen = pPrevResizerOutBuffer->nFilledLen;
			pSensResizerInBuffer->nOffset = 0;
			pSensResizerOutBuffer->nFilledLen = 30720;
			//	pSensResizerInBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;
			//	pSensResizerInBuffer->nFlags = OMX_BUFFERFLAG_EOS;
			//if (iNumFrame == START_VIDEO_FRAME_NUMBER) 
				//omx_dump_data("buff0.bin", pSensResizerInBuffer, sizeof(OMX_BUFFERHEADERTYPE));
				//memcpy(&tebpbuff, pSensResizerInBuffer, sizeof(OMX_BUFFERHEADERTYPE));
			//if (iNumFrame != START_VIDEO_FRAME_NUMBER) 
				//memcpy(pSensResizerInBuffer, &tebpbuff, sizeof(OMX_BUFFERHEADERTYPE));
			
			omx_add_cmd_in_list(iResizerNumSens, OMX_EventEmptyBuffer, (int)(intptr_t)pSensResizerInBuffer->pAppPrivate);	
			if (tx_semaphore_exist_in_list(m_oCompList[iResizerNumSens].psem, OMX_EventFillBuffer, TX_ANY) == 0)
				omx_add_cmd_in_list(iResizerNumSens, OMX_EventFillBuffer, OMX_CommandAny);
					
			if (!omx_activate_buffers(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN, 0)) break;
			ret = tx_semaphore_wait_event_timeout(m_oCompList[iResizerNumSens].psem, OMX_EventEmptyBuffer, (int)(intptr_t)pSensResizerInBuffer->pAppPrivate, iTimeOutPrev);
			if (ret == 0) 
			{
				iSensorResizerEnabled = 0;
				dbgprintf(1,"Error resize for sensor frame %i, %i, %i, %i %i\n", iNumFrame, pSensResizerInBuffer->nFilledLen, 
																					pSensResizerOutBuffer->nFilledLen,
																					iSensorBufferSize, iSensorMatrixSize);				
				break;
			}
			else
			{
				if (!omx_activate_buffers(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, 0)) break;
				ret = tx_semaphore_wait_event_timeout(m_oCompList[iResizerNumSens].psem, OMX_EventFillBuffer, OMX_CommandAny, 1000);
				if (ret == 0) 
				{
					iSensorResizerEnabled = 0;
					dbgprintf(2,"error get OMX_EventFillBuffer iResizerNumSens, %i\n", iResizerNumSens);
					break;
				}
			}	
		}		
		//////<<<<<<<<<GET RESIZE SENSOR FRAME>>>>>>>>>>>////////////
		
		if (cSlowEncoderEnabled && (uiSlowFramesCnt >= uiSlowFramesSkip))  
			memcpy(pSlowEncoderInBuffer->pBuffer, pCameraOutBuffer->pBuffer, pCameraOutBuffer->nFilledLen);
		
		iTimeScanNewParams++;
		if (iTimeScanNewParams >= tVideoInfo.video_frame_rate)
		{
			iTimeScanNewParams = 0;
			DBG_MUTEX_LOCK(&modulelist_mutex);
			memcpy(&ispNewCameraParams, &ispCameraImageSettings, sizeof(image_sensor_params));
			if (ispCameraImageSettings.Updated) 
			{
				ispCameraImageSettings.Updated = 0;
				miModuleList[iCameraModuleNum].Status[2] = ispCameraImageSettings.Exposure.Mode;
				miModuleList[iCameraModuleNum].Status[15] = ispCameraImageSettings.Brightness;
				miModuleList[iCameraModuleNum].Status[21] = ispCameraImageSettings.Contrast;
				miModuleList[iCameraModuleNum].Status[22] = ispCameraImageSettings.Sharpness;
				miModuleList[iCameraModuleNum].Status[23] = ispCameraImageSettings.ColorSaturation;
				miModuleList[iCameraModuleNum].Status[19] = ispCameraImageSettings.ImageFilter;
				miModuleList[iCameraModuleNum].Status[20] = ispCameraImageSettings.WhiteBalance.Mode;
				miModuleList[iCameraModuleNum].Status[16] = ispCameraImageSettings.AutoBrightControl;
				miModuleList[iCameraModuleNum].Status[17] = ispCameraImageSettings.DestBrightControl;
				/*uiSoftAutoISOControl = ispCameraImageSettings.SoftAutoISOControl;
				uiDestAutoISOControl = ispCameraImageSettings.DestAutoISOControl;
				uiSoftAutoEVControl = ispCameraImageSettings.SoftAutoEVControl;
				uiDestAutoEVControl = ispCameraImageSettings.DestAutoEVControl;*/
				uiCurrentExposureControl = ispCameraImageSettings.Exposure.Mode;
			}
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			if (ispNewCameraParams.Updated) omx_camera_apply_params(iCameraNum, &ispCurrentCameraParams, &ispNewCameraParams, 0);				
		}
		
		if (uiTempSensorID != 0)
		{
			iTimeScanTemp++;
			if (iTimeScanTemp >= tVideoInfo.video_intra_frame)
			{
				iTimeScanTemp = 0;
				if (cAreaTempSensor == 0)
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);	
					iCurrentTemp = miModuleList[iTempModuleNum].Status[0];
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
				}
				if (cAreaTempSensor == 1)
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);	
					iCurrentTemp = ModuleIdToNum(uiTempSensorID, 2);
					if (iCurrentTemp != -1)	
						iCurrentTemp = miModuleList[iCurrentTemp].Status[0];
						else iCurrentTemp = 10000;
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
				}
			}		
		}
		
		//////////SENSORS>>>>>>>>>>//////////////
		{		
			ret = 0;
			ret2 = 0;
			uiLightSize = 0;
			uiCurBright = 0;
			iDiffSize = 0;
			iMoveSize = 0;
			iColorMoveSize = 0;
			uiRectDetected = 0;
			iDiffMoveSize = 0;
			iSlowMoveSize = 0;
			
			if ((cNeedControl) && ((iTimeScanClk == 0) || uiPrevCamBrigShow || uiPrevCamColorShow))
				for (i = 0; i < iOMX_RectangleCnt; i++) 
				{
					riOMX_RectangleList[i].Detected = 0;
					riOMX_RectangleList[i].ColorDetected = 0;
					riOMX_RectangleList[i].DetectSize = 0;
					riOMX_RectangleList[i].ColorDetectSize = 0;
				}	
			
			if ((cLongNeedControl) && (iLongTimeScanClk == 0))
				for (i = 0; i < iOMX_RectangleCnt; i++) 
				{
					riOMX_RectangleList[i].LongDetected = 0;
					riOMX_RectangleList[i].LongColorDetected = 0;
					riOMX_RectangleList[i].LongDetectSize = 0;
					riOMX_RectangleList[i].LongColorDetectSize = 0;
				}
			
			if ((cLongNeedControl) && (iLongTimeScanClk == 0) && (iLongScanStep > 4))
			{
				////////COLOR LONG SENSOR>>>>>>>>>>>>///////////
				if (cLongNeedControl & 4)
				{
					memcpy(&pucLongLimitDiff12[iSensorMatrixSize], &pucLongLimitDiff23[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);
					memcpy(&pucLongLimitDiff23[iSensorMatrixSize], &pucLongLimitDiff34[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);
					memset(&pucLongLimitDiff34[iSensorMatrixSize], 128, iSensorBufferSize - iSensorMatrixSize);
					memset(&pucLongLimitDiff24[iSensorMatrixSize], 128, iSensorBufferSize - iSensorMatrixSize);				
					
					memcpy(&pucLongPrevFrame1[iSensorMatrixSize], &pucLongPrevFrame2[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);					
				}
				if (cLongNeedControl & (2 | 4))	
					memcpy(&pucLongPrevFrame2[iSensorMatrixSize], &pucLongPrevFrame3[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);					
				memcpy(&pucLongPrevFrame3[iSensorMatrixSize], &pucLongPrevFrame4[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);
				memcpy(&pucLongPrevFrame4[iSensorMatrixSize], &pSensResizerOutBuffer->pBuffer[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);
				
				memcpy(&pucLongPrevDiff12[iSensorMatrixSize], &pucLongPrevDiff23[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);
				
				memcpy(&pucLongPrevDiff23[iSensorMatrixSize], &pucLongPrevDiff34[iSensorMatrixSize], iSensorBufferSize - iSensorMatrixSize);
				for (i = iSensorMatrixSize; i < iSensorBufferSize; i++)
				{						
						if (cLongNeedControl & (2 | 4))
						{
							if (pucLongPrevFrame2[i] > pucLongPrevFrame4[i]) 
							pucLongPrevDiff24[i] = pucLongPrevFrame2[i] - pucLongPrevFrame4[i];
							else
							pucLongPrevDiff24[i] = pucLongPrevFrame4[i] - pucLongPrevFrame2[i];
						}
						
						if (pucLongPrevFrame3[i] > pucLongPrevFrame4[i]) 
							pucLongPrevDiff34[i] = pucLongPrevFrame3[i] - pucLongPrevFrame4[i];
							else
							pucLongPrevDiff34[i] = pucLongPrevFrame4[i] - pucLongPrevFrame3[i];
						
						if (cLongNeedControl & 4)
						{							
							if (pucLongPrevDiff34[i] > ucLongColorMoveLevel) pucLongLimitDiff34[i] = 255;
							if (pucLongPrevDiff24[i] > ucLongColorMoveLevel) pucLongLimitDiff24[i] = 255;
						}
						
						if ((pucLongPrevDiff12[i] < ucLongColorMoveLevel) &&
							(pucLongPrevDiff23[i] > ucLongColorMoveLevel) &&
							(
								((cLongNeedControl & 1) && (pucLongPrevDiff34[i] < ucLongColorMoveLevel))
								||
								((cLongNeedControl & 2) && (pucLongPrevDiff24[i] > ucLongColorMoveLevel))
							))	
						{
							if (iOMX_RectangleCnt)
							{
								if (i < iColorBufferSize)
									MarkRects(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32 / 2, i - iSensorMatrixSize, 3);
									else
									MarkRects(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32 / 2, i - iColorBufferSize, 3);
							}						
						}
						
						if (cLongNeedControl & 4)
						{
							pucLongPrevDiff34[i] = (unsigned char)(128 + (pucLongPrevFrame3[i] - pucLongPrevFrame4[i]));
							pucLongPrevDiff24[i] = (unsigned char)(128 + (pucLongPrevFrame2[i] - pucLongPrevFrame4[i]));
						}
				}							
				////////<<<<<<<<<<<<<<<COLOR LONG SENSOR///////////
			}
			
			if ((cNeedControl) && ((iTimeScanClk == 0) || uiPrevCamBrigShow || uiPrevCamColorShow))
			{
				////////COLOR SENSOR>>>>>>>>>>>>///////////
				for (i = iSensorMatrixSize; i < iSensorBufferSize; i++)
				{
					cPrevTemp = pSensResizerOutBuffer->pBuffer[i];
					
					if (pucMovePrevBuffer[i] > pSensResizerOutBuffer->pBuffer[i]) 
						iDiffValue = pucMovePrevBuffer[i] - pSensResizerOutBuffer->pBuffer[i];
						else
						iDiffValue = pSensResizerOutBuffer->pBuffer[i] - pucMovePrevBuffer[i];
					
					if ((ucFirstLoop == 0) && (iDiffValue > ucColorMoveLevel))
					{
						iColorMoveSize++;
						if (iOMX_RectangleCnt) 
						{
							if (i < iColorBufferSize)
								MarkRects(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32 / 2, i - iSensorMatrixSize, 1);
								else
								MarkRects(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32 / 2, i - iColorBufferSize, 1);
						}												
					}
					
					if (uiPrevCamColorShow)
					{
						if (uiPrevCamGaneEnabled)
						{
							if (iDiffValue > uiPrevColorLevel)
							{
								switch (uiPrevColorSett) 
								{
									case 0:
										if ((iDiffValue * uiPrevShowLevel) > 128) pSensResizerOutBuffer->pBuffer[i] = 255;
											else pSensResizerOutBuffer->pBuffer[i] = 128 + (iDiffValue * uiPrevShowLevel);
										break;
									case 1:
										pSensResizerOutBuffer->pBuffer[i] ^= 255;
										break;
									case 2:
										pSensResizerOutBuffer->pBuffer[i] = 255;
										break;
									case 3:
										pSensResizerOutBuffer->pBuffer[i] = 0;
										break;	
									default:
										break;
								}
							} else pSensResizerOutBuffer->pBuffer[i] = 128;
						}
						else
						{
							if (uiPrevColorSett == 0)
								if ((iDiffValue * uiPrevShowLevel) > 128) pSensResizerOutBuffer->pBuffer[i] = 255;
											else pSensResizerOutBuffer->pBuffer[i] = 128 + (iDiffValue * uiPrevShowLevel);										
							else pSensResizerOutBuffer->pBuffer[i] = 128;
						}						
					}
					if (uiPrevCamBrigShow && (uiPrevCamColorShow == 0)) pSensResizerOutBuffer->pBuffer[i] = 128;
					pucMovePrevBuffer[i] = cPrevTemp;					
				}
				////////<<<<<<<<<<<<<<<COLOR SENSOR///////////
			}
			
			if ((cLongNeedControl) && (iLongTimeScanClk == 0) && (iLongScanStep > 4))
			{
				////////BRIGHT LONG SENSOR>>>>>>>>>>>>///////////
				if (cLongNeedControl & 4)
				{
					memcpy(pucLongLimitDiff12, pucLongLimitDiff23, iSensorMatrixSize);
					memcpy(pucLongLimitDiff23, pucLongLimitDiff34, iSensorMatrixSize);
					memset(pucLongLimitDiff34, 0, iSensorMatrixSize);
					memset(pucLongLimitDiff24, 0, iSensorMatrixSize);				
					
					memcpy(pucLongPrevFrame1, pucLongPrevFrame2, iSensorMatrixSize);					
				}
				
				if (cLongNeedControl & (2 | 4))	
					memcpy(pucLongPrevFrame2, pucLongPrevFrame3, iSensorMatrixSize);					
				memcpy(pucLongPrevFrame3, pucLongPrevFrame4, iSensorMatrixSize);
				memcpy(pucLongPrevFrame4, pSensResizerOutBuffer->pBuffer, iSensorMatrixSize);
				
				memcpy(pucLongPrevDiff12, pucLongPrevDiff23, iSensorMatrixSize);
				memcpy(pucLongPrevDiff23, pucLongPrevDiff34, iSensorMatrixSize);
												
				for (i = 0; i < iSensorMatrixSize; i++)
				{
						if (cLongNeedControl & (2 | 4))
						{
							if (pucLongPrevFrame2[i] > pucLongPrevFrame4[i]) 
							pucLongPrevDiff24[i] = pucLongPrevFrame2[i] - pucLongPrevFrame4[i];
							else
							pucLongPrevDiff24[i] = pucLongPrevFrame4[i] - pucLongPrevFrame2[i];
						}
						if (pucLongPrevFrame3[i] > pucLongPrevFrame4[i]) 
							pucLongPrevDiff34[i] = pucLongPrevFrame3[i] - pucLongPrevFrame4[i];
							else
							pucLongPrevDiff34[i] = pucLongPrevFrame4[i] - pucLongPrevFrame3[i];
						if (cLongNeedControl & 4)
						{
							if (pucLongPrevDiff34[i] > ucLongBrightMoveLevel) pucLongLimitDiff34[i] = 255;
							if (pucLongPrevDiff24[i] > ucLongBrightMoveLevel) pucLongLimitDiff24[i] = 255;
						}
						
						if ((pucLongPrevDiff12[i] < ucLongBrightMoveLevel) &&
							(pucLongPrevDiff23[i] > ucLongBrightMoveLevel) &&
							(
								((cLongNeedControl & 1) && (pucLongPrevDiff34[i] < ucLongBrightMoveLevel))
								||
								((cLongNeedControl & 2) && (pucLongPrevDiff24[i] > ucLongBrightMoveLevel))
							))	
						{
							if (iOMX_RectangleCnt)
								MarkRects(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, i, 2);							
						}			
				}				
				
				////////<<<<<<<<<<<<<<<BRIGHT LONG SENSOR///////////
			}
			
			////////////////BRIGHT SENSOR>>>>>>>>>>>>>////////////////
			for (i = 0; i < iSensorMatrixSize; i++)
			{			
				//////////////SENSOR DIFF RECORD/////////////////		
				if ((cDiffRecordEnabled == 1) || (cSlowOnlyDiffRecord))
				{
					if (pucDiffMovePrevBuffer[i] > pSensResizerOutBuffer->pBuffer[i]) 
					iDiffValue = pucDiffMovePrevBuffer[i] - pSensResizerOutBuffer->pBuffer[i];
					else
					iDiffValue = pSensResizerOutBuffer->pBuffer[i] - pucDiffMovePrevBuffer[i];
				
					if ((cDiffRecordEnabled == 1) && (ret == 0) && (ucFirstLoop == 0))
					{
						if (iDiffValue > ucMoveLevelDiff)
						{
							if (uiMoveSizeDiffSet == 0) 
							{
								StartPack.HaveChangedFrames = 1;
								iTimeoutLastMove = uiMoveFramesSave;
								ret = 2;							
							} else iDiffMoveSize++;
						}
					}
					if ((cSlowOnlyDiffRecord) && (ret2 == 0) && (ucFirstLoop == 0))
					{
						if (iDiffValue > ucMoveLevelSlowDiff)
						{
							if (uiMoveSizeSlowDiffSet == 0) 
							{
								StartPackSlow.HaveChangedFrames = 1;
								iTimeoutLastMoveSlow = uiMoveFramesSaveSlow;								
								ret2 = 2;
							} else iSlowMoveSize++;			
						}
					}
					pucDiffMovePrevBuffer[i] = pSensResizerOutBuffer->pBuffer[i];				
				}
				if (cNeedControl) 
				{
					if ((iTimeScanClk == 0) || uiPrevCamBrigShow || uiPrevCamColorShow)
					{
						cPrevTemp = pSensResizerOutBuffer->pBuffer[i];					
						uiLightSize += pSensResizerOutBuffer->pBuffer[i];
						
						if (pucMovePrevBuffer[i] > pSensResizerOutBuffer->pBuffer[i]) 
							iDiffValue = pucMovePrevBuffer[i] - pSensResizerOutBuffer->pBuffer[i];
							else
							iDiffValue = pSensResizerOutBuffer->pBuffer[i] - pucMovePrevBuffer[i];
				
						if (ucFirstLoop == 0)
						{
							if (iDiffValue > ucMoveLevel)
							{
								iMoveSize++;
								if (iOMX_RectangleCnt) 
								{
									if (uiRectDetected != 1)
									{
										res = GetRectNum(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, i);
										if ((res) && ((res < uiRectDetected) || (uiRectDetected == 0))) uiRectDetected = res;
									}
									MarkRects(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, i, 0);
								}
							}
							if (iDiffValue > iDiffSize) iDiffSize = iDiffValue;
						}						
						/////DRAW BRIG////
						if (uiPrevCamBrigShow)
						{
							if (uiPrevCamGaneEnabled)
							{
								if (iDiffValue > uiPrevBrigLevel)
								{
									switch (uiPrevBrigSett)
									{
										case 0:
											pSensResizerOutBuffer->pBuffer[i] = (iDiffValue * uiPrevShowLevel) & 255;
											break;
										case 1:
											pSensResizerOutBuffer->pBuffer[i] ^= 255;
											break;
										case 2:
											pSensResizerOutBuffer->pBuffer[i] = 255;
											break;
										case 3:
											pSensResizerOutBuffer->pBuffer[i] = 0;
											break;
										default:
											break;
									}
								}
								else if (uiPrevBrigSett == 0) pSensResizerOutBuffer->pBuffer[i] = 0;
							}
							else
							{
								if (uiPrevBrigSett == 0) 
									pSensResizerOutBuffer->pBuffer[i] = (iDiffValue * uiPrevShowLevel) & 255;
									else pSensResizerOutBuffer->pBuffer[i] = 0;
							}						
						}
						//if ((uiPrevCamBrigShow == 0) && uiPrevCamColorShow) pSensResizerOutBuffer->pBuffer[i] = 128;
						pucMovePrevBuffer[i] = cPrevTemp;
					}									
				}
				if (uiAutoBrightControl || uiSoftAutoISOControl || uiSoftAutoEVControl || iHighLightMode) uiCurBright += pSensResizerOutBuffer->pBuffer[i];
			}
			////////////////<<<<<<<<<<<<<BRIGHT SENSOR////////////////
			if ((cDiffRecordEnabled == 1) && (uiMoveSizeDiffSet))
			{
				iDiffMoveSize /= (iSensorMatrixSize/1000);
				if (iDiffMoveSize > uiMoveSizeDiffSet)
				{
					StartPack.HaveChangedFrames = 1;
					iTimeoutLastMove = uiMoveFramesSave;
				}
			}
			if ((cSlowOnlyDiffRecord) && (uiMoveSizeSlowDiffSet))
			{
				iSlowMoveSize /= (iSensorMatrixSize/1000);
				if (iSlowMoveSize > uiMoveSizeSlowDiffSet)
				{
					StartPackSlow.HaveChangedFrames = 1;
					iTimeoutLastMoveSlow = uiMoveFramesSaveSlow;
				}
			}
			if (ucFirstLoop) ucFirstLoop = 0;			
			if (iLongScanStep < 5) iLongScanStep++;
			
			if (uiAutoBrightControl || uiSoftAutoISOControl || uiSoftAutoEVControl  || iHighLightMode)
			{
				uiCurBright /= iSensorMatrixSize;
				uiCurBright = (unsigned char)(100 * uiCurBright / 255);
			}
			if (uiAutoBrightControl)
			{	
				ret = 0;
				//printf("Cur bright %i %i %i %i\n", uiCurBright, uiDestBrightControl, uiBrightControlSense, uiCurrentBrightControl);
				if ((uiCurrentBrightControl < 100) && (uiDestBrightControl > uiCurBright) && ((uiDestBrightControl - uiCurBright) > uiBrightControlSense))
				{
					uiCurrentBrightControl++;
					ret = 1;
				}
				if ((uiCurrentBrightControl > 0) && (uiDestBrightControl < uiCurBright) && ((uiCurBright - uiDestBrightControl) > uiBrightControlSense))
				{
					uiCurrentBrightControl--;
					ret = 1;
				}
				if (ret)
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);	
					miModuleList[iCameraModuleNum].Status[15] = uiCurrentBrightControl;
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					omx_set_bright_value(iCameraNum, uiCurrentBrightControl);
				}
			}
			if (uiSoftAutoISOControl)
			{	
				ret = 0;
				//printf("Cur bright %i %i %i %i\n", uiCurBright, uiDestAutoISOControl, uiBrightControlSense, uiCurrentISOControl);
				if ((uiCurrentISOControl < 800) && (uiDestAutoISOControl > uiCurBright) && ((uiDestAutoISOControl - uiCurBright) > uiBrightControlSense))
				{
					uiCurrentISOControl++;
					ret = 1;
				}
				if ((uiCurrentISOControl > 100) && (uiDestAutoISOControl < uiCurBright) && ((uiCurBright - uiDestAutoISOControl) > uiBrightControlSense))
				{
					uiCurrentISOControl--;
					ret = 1;
				}
				if (ret) 
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);	
					miModuleList[iCameraModuleNum].Status[24] = uiCurrentISOControl;
					DBG_MUTEX_UNLOCK(&modulelist_mutex);					
					omx_set_iso_value(iCameraNum, uiEVControl, uiHardAutoISOControl, uiCurrentISOControl);
					dbg_printf(3, "Camera: Update ISO value: %i\n", uiCurrentISOControl);
				}
			}
			if (uiSoftAutoEVControl)
			{	
				ret = 0;
				//printf("Cur bright %i %i %i %i\n", uiCurBright, uiDestAutoISOControl, uiBrightControlSense, uiCurrentISOControl);
				if ((uiCurrentEVControl < 24) && (uiDestAutoEVControl > uiCurBright) && ((uiDestAutoEVControl - uiCurBright) > uiBrightControlSense))
				{
					uiCurrentEVControl++;
					ret = 1;
				}
				if ((uiCurrentEVControl > -24) && (uiDestAutoEVControl < uiCurBright) && ((uiCurBright - uiDestAutoEVControl) > uiBrightControlSense))
				{
					uiCurrentEVControl--;
					ret = 1;
				}
				if (ret) 
				{
					DBG_MUTEX_LOCK(&modulelist_mutex);	
					miModuleList[iCameraModuleNum].Status[25] = uiCurrentEVControl;
					DBG_MUTEX_UNLOCK(&modulelist_mutex);				
					
					omx_set_iso_value(iCameraNum, uiCurrentEVControl, uiHardAutoISOControl, uiISOControl);
					dbg_printf(5, "Camera: Update EV value: %i\n", uiCurrentEVControl);
				}
			}
			
			if (cNeedControl) 
			{
				if (iTimeScanClk == 0) 
				{
					uiLightSize /= iSensorMatrixSize;
					iMoveSize /= (iSensorMatrixSize/1000);
					iColorMoveSize /= (iColorSize/1000);
					//ret2 &= 252;
					//ret3 &= 252;
					if ((uiLightSize != iSensorPrevLight) || (iDiffSize != iSensorPrevMove) || (iMoveSize != iSensorPrevSizeMove)
						 || (uiRectDetected != iSensorPrevRect) || (iColorMoveSize != iSensorPrevColorSizeMove))
					{
						DBG_MUTEX_LOCK(&modulelist_mutex);	
						miModuleList[iCameraModuleNum].Status[0] = uiLightSize;
						miModuleList[iCameraModuleNum].Status[1] = iDiffSize;
						miModuleList[iCameraModuleNum].Status[3] = iMoveSize;
						miModuleList[iCameraModuleNum].Status[4] = uiRectDetected;
						miModuleList[iCameraModuleNum].Status[13] = iColorMoveSize;
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
						iSensorPrevLight = uiLightSize;
						iSensorPrevMove = iDiffSize;
						iSensorPrevSizeMove = iMoveSize;
						iSensorPrevRect = uiRectDetected;
						iSensorPrevColorSizeMove = iColorMoveSize;
					}
					
					MODULE_EVENT *meEvList = NULL;
					unsigned int uiListCount = 0;
					
					for (i = 0; i < iOMX_RectangleCnt; i++)
					{
						if (riOMX_RectangleList[i].Detected)
						{
							FillModuleEventList(&meEvList, &uiListCount, miModule->ID, 13, i, NULL, 0, 0);
							if (miModule->Global)
							{
								int t = 4096;
								miModule->Status[12] = i;
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)miModule, sizeof(MODULE_INFO_TRANSFER), (char*)&t, sizeof(t));
							}	
							riOMX_RectangleList[i].DetectSize /= (iSensorMatrixSize/1000);
							FillModuleEventList(&meEvList, &uiListCount, riOMX_RectangleList[i].ID, 1, riOMX_RectangleList[i].DetectSize, NULL, 0, 0);	
							if (miModule->Global)
							{
								int t = 1;
								rctVirtModule.ID = riOMX_RectangleList[i].ID;								
								rctVirtModule.Status[0] = riOMX_RectangleList[i].DetectSize;
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&rctVirtModule, sizeof(MODULE_INFO_TRANSFER), (char*)&t, sizeof(t));
							}	
						}
						if (riOMX_RectangleList[i].ColorDetected)
						{
							FillModuleEventList(&meEvList, &uiListCount, miModule->ID, 14, i, NULL, 0, 0);
							if (miModule->Global)
							{
								int t = 4096;
								miModule->Status[14] = i;
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)miModule, sizeof(MODULE_INFO_TRANSFER), (char*)&t, sizeof(t));
							}	
							riOMX_RectangleList[i].ColorDetectSize /= (iColorSize/1000);
							FillModuleEventList(&meEvList, &uiListCount, riOMX_RectangleList[i].ID, 2, riOMX_RectangleList[i].ColorDetectSize, NULL, 0, 0);	
							if (miModule->Global)
							{
								int t = 2;
								rctVirtModule.ID = riOMX_RectangleList[i].ID;								
								rctVirtModule.Status[1] = riOMX_RectangleList[i].ColorDetectSize;
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&rctVirtModule, sizeof(MODULE_INFO_TRANSFER), (char*)&t, sizeof(t));
							}	
						}
					}
					if (uiListCount)
					{
						AddModuleEvents(meEvList, uiListCount);
						DBG_FREE(meEvList);
					}
				} 
				iTimeScanClk++;		
				if (iTimeScanClk >= miModule->ScanSet) iTimeScanClk = 0;
			}
			
			if (cLongNeedControl) 
			{
				if (iLongTimeScanClk == 0) 
				{		
					MODULE_EVENT *meEvList = NULL;
					unsigned int uiListCount = 0;
					for (i = 0; i < iOMX_RectangleCnt; i++)
					{
						if (riOMX_RectangleList[i].LongDetected)
						{
							riOMX_RectangleList[i].LongDetectSize /= (iSensorMatrixSize/1000);
							FillModuleEventList(&meEvList, &uiListCount, riOMX_RectangleList[i].ID, 3, riOMX_RectangleList[i].LongDetectSize, NULL, 0, 0);	
							if (miModule->Global)
							{
								int t = 4;
								rctVirtModule.ID = riOMX_RectangleList[i].ID;								
								rctVirtModule.Status[2] = riOMX_RectangleList[i].LongDetectSize;
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&rctVirtModule, sizeof(MODULE_INFO_TRANSFER), (char*)&t, sizeof(t));
							}	
						}
						if (riOMX_RectangleList[i].LongColorDetected)
						{
							riOMX_RectangleList[i].LongColorDetectSize /= (iColorSize/1000);
							FillModuleEventList(&meEvList, &uiListCount, riOMX_RectangleList[i].ID, 4, riOMX_RectangleList[i].LongColorDetectSize, NULL, 0, 0);	
							if (miModule->Global)
							{
								int t = 8;
								rctVirtModule.ID = riOMX_RectangleList[i].ID;								
								rctVirtModule.Status[3] = riOMX_RectangleList[i].LongColorDetectSize;
								SendBroadCastMessage(cBcTCP, TYPE_MESSAGE_MODULE_STATUS_CHANGED, (char*)&rctVirtModule, sizeof(MODULE_INFO_TRANSFER), (char*)&t, sizeof(t));
							}	
						}
					}
					if (uiListCount)
					{
						AddModuleEvents(meEvList, uiListCount);
						DBG_FREE(meEvList);
					}
				} 
				iLongTimeScanClk++;		
				if (iLongTimeScanClk >= iLongScanSet) iLongTimeScanClk = 0;
			}
		}
		
		DBG_MUTEX_LOCK(&modulelist_mutex);
		
		if (ispCameraImageSettings.ChangedPosition != 0)
		{
			autofocus_pm.GotoHomeDone = 0;
			autofocus_pm.GotoHomeCounter = 0;
			ispCameraImageSettings.ChangedPosition = 0;
		}
		
		if (ispCameraImageSettings.StartAutofocus != 0) 
		{
			if (autofocus_pm.Enabled)
			{
				if (ispCameraImageSettings.StartAutofocus == 2) 
				{
					omx_Stop_Autofocus(&autofocus_pm);
					autofocus_pm.Status = 2;
				}
				else
				{
					DBG_MUTEX_UNLOCK(&modulelist_mutex);
					omx_Start_Autofocus(&autofocus_pm, (ispCameraImageSettings.StartAutofocus == 1) ? 1 : 0, iPtzModuleNum, iPtzStatuses);	
					DBG_MUTEX_LOCK(&modulelist_mutex);				
				}
			}
			ispCameraImageSettings.StartAutofocus = 0;
		}
		
		if (uiPrevSettingsTimer)
		{
			uiPrevSettingsTimer--;
			if (uiPrevSettingsTimer == 0)
			{
				if (ispCameraImageSettings.PrevSettings & 1) ispCameraImageSettings.PrevSettings ^= 1;
				if (ispCameraImageSettings.PrevSettings & 2) ispCameraImageSettings.PrevSettings ^= 2;
				if (ispCameraImageSettings.PrevSettings & 128) ispCameraImageSettings.PrevSettings ^= 128;
			}
		}
		if ((uiPrevSettings != ispCameraImageSettings.PrevSettings) && (uiPrevSettingsTimer == 0)) 
			uiPrevSettingsTimer = 3000;
		uiPrevSettings = ispCameraImageSettings.PrevSettings;
		uiShowMainRect = ispCameraImageSettings.MainSettings;	
		uiPrevShowLevel = ispCameraImageSettings.PrevShowLevel;
		uiPrevCamBrigShow = ispCameraImageSettings.PrevSettings & 1;
		uiPrevCamColorShow = ispCameraImageSettings.PrevSettings & 2;
		uiPrevCamGaneEnabled = ispCameraImageSettings.PrevSettings & 4;
		uiPrevCamRectShow = ispCameraImageSettings.PrevSettings & 128;
		uiPrevBrigSett = (ispCameraImageSettings.PrevSettings >> 3) & 3;
		uiPrevColorSett = (ispCameraImageSettings.PrevSettings >> 5) & 3;
		uiPrevColorLevel = (unsigned char)(ispCameraImageSettings.PrevColorLevel & 255);	
		uiPrevBrigLevel = (unsigned char)(ispCameraImageSettings.PrevBrigLevel & 255);
		uiAutoBrightControl = ispCameraImageSettings.AutoBrightControl;
		uiDestBrightControl = ispCameraImageSettings.DestBrightControl; 
		uiISOControl = ispCameraImageSettings.ISOControl;
		uiHardAutoISOControl = ispCameraImageSettings.HardAutoISOControl;
		uiSoftAutoISOControl = ispCameraImageSettings.SoftAutoISOControl;
		uiDestAutoISOControl = ispCameraImageSettings.DestAutoISOControl; 
		uiEVControl = ispCameraImageSettings.EVControl;
		//uiHardAutoEVControl = ispCameraImageSettings.HardAutoEVControl;
		uiSoftAutoEVControl = ispCameraImageSettings.SoftAutoEVControl;
		uiDestAutoEVControl = ispCameraImageSettings.DestAutoEVControl; 
		
		if (iHighLightMode && !ispCameraImageSettings.HighLightMode) 
		{
			DBG_FREE(ucMainCameraPrevBuff);
			ucMainCameraPrevBuff = NULL;
			DBG_FREE(ucMainCameraMask);
			ucMainCameraMask = NULL;
			DBG_FREE(ucMainCameraResultBuff);
			ucMainCameraResultBuff = NULL;
			iHighLightMode = 0;
		}
		if (!iHighLightMode && ispCameraImageSettings.HighLightMode) 
		{
			ucMainCameraPrevBuff = (unsigned char*)DBG_MALLOC(tVideoInfo.video_frame_size);
			ucMainCameraMask = (unsigned char*)DBG_MALLOC(tVideoInfo.video_frame_size);
			ucMainCameraResultBuff = (unsigned char*)DBG_MALLOC(tVideoInfo.video_frame_size);
			memset(ucMainCameraPrevBuff, 0, tVideoInfo.video_frame_size);
			memset(ucMainCameraMask, 0, tVideoInfo.video_frame_size);
			memset(ucMainCameraResultBuff, 0, tVideoInfo.video_frame_size);
			iHighLightMode = 1;
		}
		iHighLightBright = ispCameraImageSettings.HighLightBright;
		iHighLightAmplif = ispCameraImageSettings.HighLightAmplif;
	
		miModuleList[iCameraModuleNum].Status[18] = uiCurBright;	
		
		ret = miModuleList[iCameraModuleNum].Status[5];
		
		if (cDiffRecordEnabled == 1) 
		{
			miModuleList[iCameraModuleNum].Status[7] = iTimeoutLastMove;
			if (uiMoveSizeDiffSet) 
			{
				if (iTimeoutLastMove == 0) miModuleList[iCameraModuleNum].Status[9] = 0;
				if (iDiffMoveSize > miModuleList[iCameraModuleNum].Status[9]) 
						miModuleList[iCameraModuleNum].Status[9] = iDiffMoveSize;
			}
		}
		if (cSlowOnlyDiffRecord) 
		{
			miModuleList[iCameraModuleNum].Status[8] = iTimeoutLastMoveSlow;
			if (uiMoveSizeSlowDiffSet) 
			{
				if (iTimeoutLastMoveSlow == 0) miModuleList[iCameraModuleNum].Status[10] = 0;
				if (iSlowMoveSize > miModuleList[iCameraModuleNum].Status[10]) 
						miModuleList[iCameraModuleNum].Status[10] = iSlowMoveSize;
			}
		}
		if (miModuleList[iCameraModuleNum].Status[11])
		{
			if (miModuleList[iCameraModuleNum].Status[11] & 1)
			{
				miModuleList[iCameraModuleNum].Status[11] ^= 1;
				StartPack.HaveChangedFrames = 1;
				iTimeoutLastMove = uiMoveFramesSave;
			}
			if (miModuleList[iCameraModuleNum].Status[11] & 2)
			{
				miModuleList[iCameraModuleNum].Status[11] ^= 2;
				StartPackSlow.HaveChangedFrames = 1;
				iTimeoutLastMoveSlow = uiMoveFramesSaveSlow;
			}
		}
		
		if (autofocus_pm.Enabled) 
		{
			memcpy(iPtzPrevStatuses, iPtzStatuses, sizeof(int)*PTZ_STATUSES_MAX);	
			autofocus_pm.PrevZoomStatus = iPtzStatuses[4] == iPtzStatuses[12];
			if (autofocus_pm.CurrentFocusPos == -100000) autofocus_pm.CurrentFocusPos = iPtzStatuses[3];
			memcpy(iPtzStatuses, miModuleList[iPtzModuleNum].Status, sizeof(int)*PTZ_STATUSES_MAX);
			autofocus_pm.ZoomChanged = iPtzStatuses[4] != iPtzPrevStatuses[4];
			autofocus_pm.FocusChanged = iPtzStatuses[3] != iPtzPrevStatuses[3];
		}
		
		DBG_MUTEX_UNLOCK(&modulelist_mutex);
		
		//unsigned int uiCurrContrast = omx_GetSquareContrast(&autofocus_pm, &tVideoInfo, pMainEncoderInBuffer->pBuffer);
		//unsigned int uiPrevontrast = omx_GetContrast(tVideoInfoPreview.video_width32, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height, pPrevResizerOutBuffer->pBuffer);
		//printf("Contr: %i\t%i\n", uiCurrContrast, uiPrevontrast);
		
		if (autofocus_pm.Enabled && (iPtzStatuses[0] != ACTION_PTZ_LOCKED))
		{			
			if (((autofocus_pm.Status == 0) && (iPtzStatuses[4] != iPtzStatuses[12])) 
				|| ((iPtzStatuses[4] == iPtzStatuses[12]) && (!autofocus_pm.PrevZoomStatus || autofocus_pm.ZoomChanged)))				
				omx_Start_Autofocus(&autofocus_pm, 1, iPtzModuleNum, iPtzStatuses);			
			if ((autofocus_pm.Status == 2) && (iPtzStatuses[3] == iPtzStatuses[11]) && (iPtzStatuses[4] == iPtzStatuses[12]))
				omx_Stop_Autofocus(&autofocus_pm);
			if ((autofocus_pm.Status == 0) && (iPtzStatuses[4] == iPtzStatuses[12]))
			{
				autofocus_pm.MinFocusPos = omx_ptz_get_focus_position(iPtzStatuses[13], iPtzStatuses[14], iPtzStatuses[4], 0.0f)-50;
				autofocus_pm.MaxFocusPos = omx_ptz_get_focus_position(iPtzStatuses[13], iPtzStatuses[14], iPtzStatuses[4], 1.0f)+50;
				if ((iPtzStatuses[3] < autofocus_pm.MinFocusPos) || (iPtzStatuses[3] > autofocus_pm.MaxFocusPos))
								omx_Start_Autofocus(&autofocus_pm, 1, iPtzModuleNum, iPtzStatuses);			
			}
			
			if ((autofocus_pm.Status == 0) && (iPtzStatuses[4] != iPtzStatuses[12]) && autofocus_pm.PtzFocusMapEnabled)
			{
				omx_SetFocusFromMap(&autofocus_pm, iPtzModuleNum, iPtzStatuses, 0);
				//dbgprintf(2,"omx_SetFocusFromMap %i %i %i\n", iPtzStatuses[4], iPtzStatuses[12], autofocus_pm.CurrentFocusPos);
			}
			
			if ((autofocus_pm.Status == 0) && (iPtzStatuses[4] == iPtzStatuses[12]) && autofocus_pm.PtzFocusMapEnabled 
				 && (iPtzStatuses[3] == iPtzStatuses[11]) && autofocus_pm.PrevZoomStatus && !autofocus_pm.ZoomChanged &&
				 autofocus_pm.FocusChanged)
			{
				autofocus_pm.CurrentFocusPos = iPtzStatuses[3];
				omx_Save_Autofocus(&autofocus_pm, iPtzStatuses[4]);
			}
			
			if (autofocus_pm.Status == 1)
			{				
				autofocus_pm.FullMinFocusPos = iPtzStatuses[13];
				autofocus_pm.FullMaxFocusPos = iPtzStatuses[14];
				autofocus_pm.MinFocusPos = omx_ptz_get_focus_position(iPtzStatuses[13], iPtzStatuses[14], iPtzStatuses[4], 0.0f)-50;
				autofocus_pm.MaxFocusPos = omx_ptz_get_focus_position(iPtzStatuses[13], iPtzStatuses[14], iPtzStatuses[4], 1.0f)+50;
				if (autofocus_pm.MinFocusPos < iPtzStatuses[13]) autofocus_pm.MinFocusPos = iPtzStatuses[13];
				if (autofocus_pm.MaxFocusPos > iPtzStatuses[14]) autofocus_pm.MaxFocusPos = iPtzStatuses[14];
				
				//Загоняем фокус в расчитанные рамки
				if (autofocus_pm.CurrentFocusPos != -100000)
				{
					if (autofocus_pm.CurrentFocusPos < autofocus_pm.MinFocusPos)
					{
						DBG_MUTEX_LOCK(&modulelist_mutex);
						usb_gpio_set_focus(&miModuleList[iPtzModuleNum], autofocus_pm.MinFocusPos, 1, &autofocus_pm.CurrentFocusPos);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
					}
					if (autofocus_pm.CurrentFocusPos > autofocus_pm.MaxFocusPos)
					{
						DBG_MUTEX_LOCK(&modulelist_mutex);
						usb_gpio_set_focus(&miModuleList[iPtzModuleNum], autofocus_pm.MaxFocusPos, 1, &autofocus_pm.CurrentFocusPos);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
					}
				}
				
				unsigned int uiContrast[4];				
				uiContrast[0] = omx_GetSquareContrast(&autofocus_pm, &tVideoInfo, pMainEncoderInBuffer->pBuffer);
				uiContrast[1] = uiContrast[0];
				if (cPreviewEnabled) uiContrast[1] = omx_GetContrast(tVideoInfoPreview.video_width32, tVideoInfoPreview.video_width, tVideoInfoPreview.video_height, pPrevResizerOutBuffer->pBuffer);
				uiContrast[2] = uiContrast[0];
				uiContrast[3] = uiContrast[1];
				uiContrast[0] >>= 13; 
				uiContrast[0] <<= 3; 
				uiContrast[1] >>= 10;
				uiContrast[2] >>= 13;
				uiContrast[2] <<= 3; 
				uiContrast[3] >>= 10;
				
				for (i = 0; i < 4; i++)
					if (autofocus_pm.LastContrast[i] == 0xFFFFFFFF) autofocus_pm.LastContrast[i] = uiContrast[i];
				
				int uiDiffCntr[4];
				for (i = 0; i < 4; i++)
				{
					uiDiffCntr[i] = uiContrast[i] - autofocus_pm.LastContrast[i];
					autofocus_pm.LastContrast[i] = uiContrast[i];
				}
				
				if (iPtzStatuses[4] == iPtzStatuses[12])
				{					
					if ((autofocus_pm.CurrentFocusPos <= autofocus_pm.MinFocusPos) && (autofocus_pm.Direct != 1))
					{ 
						autofocus_pm.Direct = 1;
						autofocus_pm.FullScanCounter++;
					}
					if ((autofocus_pm.CurrentFocusPos >= autofocus_pm.MaxFocusPos) && (autofocus_pm.Direct != 0))
					{
						autofocus_pm.Direct = 0;
						autofocus_pm.FullScanCounter++;
					}
					//printf("!! %i %i %i %i  %i %i %i %i   %i %i %i\n", uiDiffCntr[0], uiDiffCntr[1], uiDiffCntr[2], uiDiffCntr[3], 
						//						autofocus_pm.ScanCnt[0], autofocus_pm.ScanCnt[1], autofocus_pm.ScanCnt[2], autofocus_pm.ScanCnt[3], 
							//					autofocus_pm.SearchStep, autofocus_pm.Direct, autofocus_pm.FullScanCounter);
					if (autofocus_pm.FullScanCounter >= 2)
					{	
						autofocus_pm.FullScanCounter = 0;
						int iMaxContrastPos = autofocus_pm.MaxContrastPos[0];
						int iMaxContrastVal = autofocus_pm.MaxContrastVal[0];
						for (i = 1; i < 4; i++)
							if (autofocus_pm.MaxContrastVal[i] > iMaxContrastVal)
							{
								iMaxContrastPos = autofocus_pm.MaxContrastPos[i];
								iMaxContrastVal = autofocus_pm.MaxContrastVal[i];
							//	printf("AF Max contrast %i val:%i pos:%i\n", i, iMaxContrastVal, iMaxContrastPos);
							}
						//printf("AF PING PONG done curr:%i new:%i  step %i\n", autofocus_pm.CurrentFocusPos, iMaxContrastPos, autofocus_pm.SearchStep);
						if (iMaxContrastPos != -100000)
						{
							DBG_MUTEX_LOCK(&modulelist_mutex);
							usb_gpio_set_focus(&miModuleList[iPtzModuleNum], iMaxContrastPos, 1, &autofocus_pm.CurrentFocusPos);
							DBG_MUTEX_UNLOCK(&modulelist_mutex);
						}
						if (autofocus_pm.SearchStep > 1) autofocus_pm.SearchStep >>= 1;
								else omx_Stop_Autofocus(&autofocus_pm);
					}
					else
					{
						//Фиксируем значение контраста
						for (i = 1; i < 4; i++)
							if (autofocus_pm.MaxContrastVal[i] < uiContrast[i])
							{
								autofocus_pm.MaxContrastVal[i] = uiContrast[i];
								autofocus_pm.MaxContrastPos[i] = autofocus_pm.CurrentFocusPos;					
							}			
												
						//Ничего не меняется по контрасту
						for (i = 1; i < 4; i++)
							if (uiDiffCntr[i] == 0) autofocus_pm.ScanCnt[i] = 0;
						
						//Обнаружено уменьшение контраста
						for (i = 1; i < 4; i++)
							if (uiDiffCntr[i] < 0) autofocus_pm.ScanCnt[i]++;
												
						//Обнаружено увеличение контраста
						for (i = 1; i < 4; i++)
							if (uiDiffCntr[i] > 0)
							{
								autofocus_pm.ScanCnt[i] = 0;
								autofocus_pm.MaxContrastPos[i] = autofocus_pm.CurrentFocusPos;
							}
							
						//Обнаружено стабильное уменьшение контрастности
						for (i = 1; i < 4; i++)
							if (autofocus_pm.ScanCnt[i] >= 5) 
							{
								//printf("%i sense AF %i\n", i, autofocus_pm.SearchStep);
								autofocus_pm.ScanCnt[0] = 0;
								autofocus_pm.ScanCnt[1] = 0;
								autofocus_pm.ScanCnt[2] = 0;
								autofocus_pm.ScanCnt[3] = 0;
								autofocus_pm.Direct ^= 1;
								if (autofocus_pm.SearchStep > 1)
								{
									autofocus_pm.SearchStep >>= 1;
									if (autofocus_pm.SearchStep <= 3) autofocus_pm.SearchStep = 1;
								}
								else
								{
									if (autofocus_pm.MaxContrastPos[i] != -100000)
									{
										DBG_MUTEX_LOCK(&modulelist_mutex);
										usb_gpio_set_focus(&miModuleList[iPtzModuleNum], autofocus_pm.MaxContrastPos[i], 1, &autofocus_pm.CurrentFocusPos);
										DBG_MUTEX_UNLOCK(&modulelist_mutex);
										omx_Save_Autofocus(&autofocus_pm, iPtzStatuses[4]);
									}
									omx_Stop_Autofocus(&autofocus_pm);									
								}	
								break;
							}
					}
						
					//printf("%i SF1 zoom:%i zdest:%i  stp:%i diff:%i currpos:%i dircnt:%i dir:%i %i\n", get_tick_ms(), iPtzStatuses[4], iPtzStatuses[12], autofocus_pm.SearchStep, uiDiffCntr, iPtzStatuses[3], autofocus_pm.RiseDetected, autofocus_pm.Direct, uiCurrContrast);					
					if (autofocus_pm.Status == 1)
					{
						DBG_MUTEX_LOCK(&modulelist_mutex);
						usb_gpio_set_focus(&miModuleList[iPtzModuleNum], autofocus_pm.Direct ? autofocus_pm.SearchStep : -autofocus_pm.SearchStep, 0, &autofocus_pm.CurrentFocusPos);
						DBG_MUTEX_UNLOCK(&modulelist_mutex);
					}										
				}
				else
				{
					omx_Start_Autofocus(&autofocus_pm, 1, iPtzModuleNum, iPtzStatuses);					
				}					
			}
			if ((autofocus_pm.Status == 1) || (iPtzStatuses[3] != iPtzStatuses[11]) || (iPtzStatuses[4] != iPtzStatuses[12])) 
					omx_SetFocusIndicator(&autofocus_pm, &tVideoInfo, pMainEncoderInBuffer->pBuffer, iPtzStatuses[7], iPtzStatuses[8]);
			//printf("Status %i  %i %i   %i %i\n", autofocus_pm.Status, iPtzStatuses[3], iPtzStatuses[11], iPtzStatuses[4], iPtzStatuses[12]);
		}			
				
		if (uiShowMainRect)
			ShowRectOnBuffer(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfo.video_width32, (char*)pMainEncoderInBuffer->pBuffer, iMatrixBufferSize);
		if (uiPrevCamRectShow)
			ShowRectOnBufferPrev(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoPreview.video_width32, (char*)pPrevResizerOutBuffer->pBuffer, iSensorMatrixSize);
		if ((iHighLightMode == 1) && (iHighLightBright >= uiCurBright))
		{
			if (tVideoInfo.video_frame_size >= pCameraOutBuffer->nFilledLen) 
			{
				memcpy(ucMainCameraMask, pCameraOutBuffer->pBuffer, pCameraOutBuffer->nFilledLen);
				memcpy(ucMainCameraPrevBuff, pCameraOutBuffer->pBuffer, pCameraOutBuffer->nFilledLen);	
				iHighLightMode = 2;
				//printf("iHighLightMode = 2, bright = %i\n", iHighLightBright);
			} //else printf("%i %i\n", tVideoInfo.video_frame_size, pCameraOutBuffer->nFilledLen);
		}
		if ((iHighLightMode == 2) && (iHighLightBright > uiCurBright))
		{
			for (i = 0; i < pCameraOutBuffer->nFilledLen; i++)
			{
				k = ucMainCameraMask[i] + ((ucMainCameraPrevBuff[i] - pCameraOutBuffer->pBuffer[i]) * iHighLightAmplif);
				if (k < 0) k = 0;
				if (k > 255) k = 255;
				ucMainCameraResultBuff[i] = k;
			}
			if (tVideoInfo.video_frame_size >= pCameraOutBuffer->nFilledLen) 
				memcpy(ucMainCameraPrevBuff, pCameraOutBuffer->pBuffer, pCameraOutBuffer->nFilledLen);	
			memcpy(pCameraOutBuffer->pBuffer, ucMainCameraResultBuff, pCameraOutBuffer->nFilledLen);	
		}
		if ((iHighLightMode == 2) && (iHighLightBright < uiCurBright)) 
		{
			iHighLightMode = 1;
			//printf("iHighLightMode = 1, bright = %i\n", iHighLightBright);
		}
		
		//////////<<<<<<<<<<SENSORS//////////////
		
		if (autofocus_pm.PtzEnabled && autofocus_pm.PtzID && autofocus_pm.GotoHomeAfterTimeout && autofocus_pm.GotoHomeTime && !autofocus_pm.GotoHomeDone)
		{
			autofocus_pm.GotoHomeCounter++;
			if ((autofocus_pm.GotoHomeCounter / tVideoInfo.video_frame_rate) >= autofocus_pm.GotoHomeTime)
			{
				onvif_GotoHomePosition(autofocus_pm.PtzID);
				autofocus_pm.GotoHomeDone = 1;
				autofocus_pm.GotoHomeCounter = 0;
			}
		}
		
		if (uiPreviewIconFramesToRender)
		{
			uiPreviewIconFramesToRender--;
			if (!uiPreviewIconFramesToRender) 
			{
				ret |= 1;
				cPreviewIconRendered = 1;
				if (autofocus_pm.PtzEnabled && autofocus_pm.PtzID)
				{
					if (autofocus_pm.GotoHomeAfterStart) 
					{
						onvif_GotoHomePosition(autofocus_pm.PtzID);
						autofocus_pm.GotoHomeDone = 1;
					}
					else
						if (autofocus_pm.Enabled) 
						{
							omx_Start_Autofocus(&autofocus_pm, 1, iPtzModuleNum, iPtzStatuses);
						}
				}
			}
		}
		
		time_t rawtime;
		struct tm timeinfo;	
		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		
		/*char str[64];
		memset(str, 0, 64);		
		sprintf(str, "%02i:%02i:%02i  ch: %i, time: %i fr: %i mv: %i ms: %i s: %i", 
			timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 
			StartPack.HaveChangedFrames, iTimeoutLastMove, iNumFrame, iDiffSize, iMoveSize, pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME);
		text_draw_string((uint8_t *)timestamptext.data, pMainEncoderInBuffer->pBuffer, tVideoInfo.video_width32, tVideoInfo.video_height16, (unsigned char *)str, strlen(str)); */
		text_draw_timestamp((uint8_t *)timestamptext.data, pMainEncoderInBuffer->pBuffer, tVideoInfo.video_width32, tVideoInfo.video_height16, iCurrentTemp, uiCurrentExposureControl, iNumFrame - START_VIDEO_FRAME_NUMBER);
		if (cSlowEncoderEnabled && (uiSlowFramesCnt >= uiSlowFramesSkip))  
			text_draw_timestamp((uint8_t *)timestamptext.data, pSlowEncoderInBuffer->pBuffer, tVideoInfoSlow.video_width32, tVideoInfoSlow.video_height16, iCurrentTemp, uiCurrentExposureControl, iNumFrame - START_VIDEO_FRAME_NUMBER);
				//text_draw_timestamp((uint8_t *)timestamptext.data, pMainEncoderInBuffer->pBuffer, tVideoInfo.video_width32, tVideoInfo.video_height16, iCurrentTemp, uiCurrentExposureControl);
						
		if ((!uiPreviewIconFramesToRender) && (cPreviewIconRendered == 1) && (timeinfo.tm_hour > 11) && (timeinfo.tm_hour < 17))
		{
			ret |= 1;
			cPreviewIconRendered = 2;
		}
			
		if (ret & 1) 
		{
			DBG_MUTEX_LOCK(&system_mutex);	
			memcpy(cShotFile, cCameraSensorFile, 256);			
			DBG_MUTEX_UNLOCK(&system_mutex);
			dbgprintf(4,"Command save preview image %i '%s'\n", ret, cShotFile);
			omx_image_encode_in_file(tVideoInfoPreview.video_width32, tVideoInfoPreview.video_height, (char*)pPrevResizerOutBuffer->pBuffer, pPrevResizerOutBuffer->nFilledLen, cShotFile);
			dbgprintf(4,"Command save preview image DONE\n");
		}
		if (ret & 2) 
		{
			DBG_MUTEX_LOCK(&system_mutex);	
			memcpy(cShotFile, cCameraShotFile, 256);			
			DBG_MUTEX_UNLOCK(&system_mutex);
			dbgprintf(4,"Command save capture image %i '%s'\n", ret, cShotFile);			
			omx_image_encode_in_file(tVideoInfo.video_width32, tVideoInfo.video_height, (char*)pMainEncoderInBuffer->pBuffer, pCameraOutBuffer->nFilledLen, cShotFile);
			dbgprintf(4,"Command save capture image DONE\n");
		}
		if (ret & 4) 
		{
			DBG_MUTEX_LOCK(&system_mutex);	
			memcpy(cShotFile, cCameraRectFile, 256);
			int iCurRect = iCurrentShowRectangle;			
			DBG_MUTEX_UNLOCK(&system_mutex);
			char *cTempBuff = (char*)DBG_MALLOC(pPrevResizerOutBuffer->nFilledLen);
			memcpy(cTempBuff, pPrevResizerOutBuffer->pBuffer, pPrevResizerOutBuffer->nFilledLen);
			ShowCurrRectOnBuffer(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoPreview.video_width32, cTempBuff, iPreviewMatrixSize, iCurRect);
			
			dbgprintf(4,"Command save rect image %i '%s'\n", ret, cShotFile);
			omx_image_encode_in_file(tVideoInfoPreview.video_width32, tVideoInfoPreview.video_height, cTempBuff, pPrevResizerOutBuffer->nFilledLen, cShotFile);
			dbgprintf(4,"Command save rect image DONE\n");
			DBG_FREE(cTempBuff);
		}
		if (ret & 8) 
		{
			CopyRectList(iRectangleCnt, riRectangleList, &iOMX_RectangleCnt, &riOMX_RectangleList);
			RecalcRectangles(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfo.video_width, tVideoInfo.video_height, tVideoInfoSensor.video_width, tVideoInfoSensor.video_height);
			dbgprintf(4,"Recalc rects\n");						
		}
		if (ret & 16) 
		{
			if (cLongNeedControl & 4)
			{
				DBG_MUTEX_LOCK(&system_mutex);	
				memcpy(cShotFile, cCameraRectFile, 256);			
				DBG_MUTEX_UNLOCK(&system_mutex);
				if (strlen(cShotFile) < 240)
				{
					for (i = strlen(cShotFile) - 1; i > 0; i--) 
						if (cShotFile[i] == 46) break;
					if (i == 0) i = strlen(cShotFile);
				
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_f1.jpg");
					dbgprintf(4,"Save smart sensor f1\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevFrame1, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor f1 DONE\n");
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_f2.jpg");
					dbgprintf(4,"Save smart sensor f2\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevFrame2, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor f2 DONE\n");					
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_f3.jpg");
					dbgprintf(4,"Save smart sensor f3\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevFrame3, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor f3 DONE\n");
				
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_f4.jpg");
					dbgprintf(4,"Save smart sensor f4\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevFrame4, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor f4 DONE\n");	
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_d12.jpg");
					dbgprintf(4,"Save smart sensor d12\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevDiff12, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor d12 DONE\n");
										
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_d23.jpg");
					dbgprintf(4,"Save smart sensor d23\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevDiff23, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor d23 DONE\n");
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_d34.jpg");
					dbgprintf(4,"Save smart sensor d34\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevDiff34, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor d34 DONE\n");

					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_d24.jpg");
					dbgprintf(4,"Save smart sensor d24\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongPrevDiff24, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor d24 DONE\n");
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_l12.jpg");
					dbgprintf(4,"Save smart sensor l12\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongLimitDiff12, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor l12 DONE\n");
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_l23.jpg");
					dbgprintf(4,"Save smart sensor l23\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongLimitDiff23, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor l23 DONE\n");
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_l34.jpg");
					dbgprintf(4,"Save smart sensor l34\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongLimitDiff34, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor l34 DONE\n");
					
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_l24.jpg");
					dbgprintf(4,"Save smart sensor l24\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, (char*)pucLongLimitDiff24, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor l24 DONE\n");

					char *cBuff = (char*)DBG_MALLOC(iSensorBufferSize);
					
					memcpy(cBuff, pucLongLimitDiff12, iSensorBufferSize);
					ShowAllRectOnBufferPrev(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, cBuff, iSensorBufferSize);
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_r12.jpg");
					dbgprintf(4,"Save smart sensor r12\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, cBuff, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor r12 DONE\n");
		
					memcpy(cBuff, pucLongLimitDiff23, iSensorBufferSize);
					ShowAllRectOnBufferPrev(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, cBuff, iSensorBufferSize);
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_r23.jpg");
					dbgprintf(4,"Save smart sensor r23\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, cBuff, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor r23 DONE\n");
					
					memcpy(cBuff, pucLongLimitDiff34, iSensorBufferSize);
					ShowAllRectOnBufferPrev(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, cBuff, iSensorBufferSize);
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_r34.jpg");
					dbgprintf(4,"Save smart sensor r34\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, cBuff, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor r34 DONE\n");
					
					memcpy(cBuff, pucLongLimitDiff24, iSensorBufferSize);
					ShowAllRectOnBufferPrev(iOMX_RectangleCnt, riOMX_RectangleList, tVideoInfoSensor.video_width32, cBuff, iSensorBufferSize);
					memset(&cShotFile[i], 0, 256 - i);
					strcat(cShotFile, "_si_r24.jpg");
					dbgprintf(4,"Save smart sensor r24\n");
					omx_image_encode_in_file(tVideoInfoSensor.video_width32, tVideoInfoSensor.video_height, cBuff, iSensorBufferSize, cShotFile);
					dbgprintf(4,"Save smart sensor r24 DONE\n");
					
					DBG_FREE(cBuff);					
				} else dbgprintf(2,"Error save smart camera sensor, big len path %i\n", strlen(cShotFile));
			} else dbgprintf(2,"Save smart camera sensor, only in debug mode\n");
		}
		
		if ((ret & 32) && (autofocus_pm.Enabled))
		{
			omx_Start_Autofocus(&autofocus_pm, 1, -1, iPtzStatuses);	
			dbgprintf(4,"Start Force autofocus\n");						
		}
		
		if (ret)
		{
			DBG_MUTEX_LOCK(&modulelist_mutex);	
			miModuleList[iCameraModuleNum].Status[5] = 0;
			DBG_MUTEX_UNLOCK(&modulelist_mutex);			
		}
		
		pMainEncoderInBuffer->nFilledLen = pCameraOutBuffer->nFilledLen;
		pMainEncoderInBuffer->nOffset = 0;
		if (cSlowEncoderEnabled)
		{
			pSlowEncoderInBuffer->nFilledLen = pCameraOutBuffer->nFilledLen;
			pSlowEncoderInBuffer->nOffset = 0;
		}
		if (cPreviewEnabled) 
		{
			pPrevEncoderInBuffer->nFilledLen = pPrevResizerOutBuffer->nFilledLen;
			pPrevEncoderInBuffer->nOffset = 0;
		}
		//for (n = 0; n != 10000; n++) pMainEncoderInBuffer->pBuffer[n] |= 128;
		
		if (iNumFrame == START_VIDEO_FRAME_NUMBER) 
		{
			pMainEncoderInBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME | OMX_BUFFERFLAG_ENDOFFRAME; 
			if (cPreviewEnabled) pPrevEncoderInBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME | OMX_BUFFERFLAG_ENDOFFRAME; 
			if (cSlowEncoderEnabled) pSlowEncoderInBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME | OMX_BUFFERFLAG_ENDOFFRAME; 
		}
		else 
		{
			pMainEncoderInBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_ENDOFFRAME;
			if (cPreviewEnabled) pPrevEncoderInBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_ENDOFFRAME;
			if (cSlowEncoderEnabled) pSlowEncoderInBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_ENDOFFRAME;			
		}
		if (cStop) 
		{
			pMainEncoderInBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
			if (cPreviewEnabled) pPrevEncoderInBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
			if (cSlowEncoderEnabled) pSlowEncoderInBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
		}
		
		cCurEncoderStatus = 0;
		
		DBG_MUTEX_LOCK(&OMX_mutex);
		cGPUResources = omx_resource_priority;
		DBG_MUTEX_UNLOCK(&OMX_mutex);	
	
		if (cGPUResources != 0) cGPUfree = omx_IsFree_Video() & omx_IsFree_Image(); else cGPUfree = 1;
		if ((cGPUResources == 1) && (cGPUfree == 0)) omx_stop_video(0);
		
		if (cGPUfree)
		{	
			if (cPreviewEnabled)
			{
				iNeedSend = ItsNetNeed(CONNECT_SERVER, TRAFFIC_PREV_VIDEO, FLAG_START_VIDEO_FRAME | FLAG_NEXT_VIDEO_FRAME | FLAG_VIDEO_STREAM | FLAG_VIDEO_CODEC_INFO | FLAG_VIDEO_PARAMS, 0);
				iNeedRTSP = ItsRTSPNeed(0, StartPackPrev.PacketType, 0);		
				if (iNeedSend || iNeedRTSP || (iEncFramePrev == START_VIDEO_FRAME_NUMBER))
				{
					cCurEncoderStatus |= 1;
					omx_add_cmd_in_list(iVideoEncodeNumPrev, OMX_EventEmptyBuffer, (int)(intptr_t)pPrevEncoderInBuffer->pAppPrivate);		
					if (!omx_activate_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN, 0)) break;
					ret = tx_semaphore_wait_event_timeout(m_oCompList[iVideoEncodeNumPrev].psem, OMX_EventEmptyBuffer, (int)(intptr_t)pPrevEncoderInBuffer->pAppPrivate, iTimeOutPrev);
					if (ret == 0) 
					{
						dbgprintf(1,"Error encode preview frame %i, %i, %i\n", iNumFrame, pPrevEncoderInBuffer->nFilledLen, pPrevEncoderOutBuffer->nFilledLen);	
						//cPreviewEnabled--;
						cPreviewEnabled = 0;
					}
					ret = tx_semaphore_wait_event_timeout(m_oCompList[iVideoEncodeNumPrev].psem, OMX_EventFillBuffer, (int)(intptr_t)pPrevEncoderOutBuffer->pAppPrivate, uiStepFrameMS);
					if (ret != 0)
					{	
						while ((tx_semaphore_exist_in_list(m_oCompList[iVideoEncodeNumPrev].psem, OMX_EventFillBuffer, (int)(intptr_t)pPrevEncoderOutBuffer->pAppPrivate) == 0)
							&& (pPrevEncoderOutBuffer->nFilledLen != 0))
						{
							if (vControlBufferPrev == pPrevEncoderOutBuffer->pBuffer)
							{
								//dbgprintf(8,"encoded frame %i, %i\n", iNumFrame, pPrevEncoderOutBuffer->nFilledLen);	
								if (pPrevEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
								{
									if (StartPackPrev.CodecInfoFilled == 1)
									{
										memset(StartPackPrev.BufferCodecInfo, 0, CODECINFO_BUFFER_SIZE);
										StartPackPrev.CodecInfoFilled = 0;
										StartPackPrev.CodecInfoLen = 0;
									}
									if (pPrevEncoderOutBuffer->nFilledLen < (CODECINFO_BUFFER_SIZE - StartPackPrev.CodecInfoLen)) 
									{
										memcpy(StartPackPrev.BufferCodecInfo + StartPackPrev.CodecInfoLen, pPrevEncoderOutBuffer->pBuffer + pPrevEncoderOutBuffer->nOffset, pPrevEncoderOutBuffer->nFilledLen);
										StartPackPrev.CodecInfoLen += pPrevEncoderOutBuffer->nFilledLen;
									}
									if (pPrevEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) 
									{
										StartPackPrev.CodecInfoFilled = 1;
										StartPackPrev.StartFrameFilled = 1;				
									}
								}
								else	
								{
									{	
										if ((pPrevEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME) && (StartPackPrev.CodecInfoFilled == 1) 
											&& ((StartPackPrev.StartFramesCount != 0) || (ucFirstPacketPrev == 1)))
										{
											memset(StartPackPrev.BufferStartFrame, 0, StartPackPrev.BufferStartSize);
											memset(StartPackPrev.StartFramesSizes, 0, tVideoInfoPreview.video_intra_frame*sizeof(unsigned int));
											memset(StartPackPrev.StartFramesFlags, 0, tVideoInfoPreview.video_intra_frame);
											StartPackPrev.StartFrameFilled = 2;
											StartPackPrev.StartFrameLen = 0;
											StartPackPrev.StartFramesCount = 0;
											ucFirstPacketPrev = 0;
										}
										if ((pPrevEncoderOutBuffer->nFilledLen < (StartPackPrev.BufferStartSize - StartPackPrev.StartFrameLen)) && (StartPackPrev.StartFramesCount < tVideoInfoPreview.video_intra_frame))
										{
											ret = (int)(intptr_t)pPrevEncoderOutBuffer->pBuffer;
											memcpy(StartPackPrev.BufferStartFrame + StartPackPrev.StartFrameLen, pPrevEncoderOutBuffer->pBuffer + pPrevEncoderOutBuffer->nOffset, pPrevEncoderOutBuffer->nFilledLen);
											StartPackPrev.StartFrameLen += pPrevEncoderOutBuffer->nFilledLen;							
											StartPackPrev.StartFramesSizes[StartPackPrev.StartFramesCount] += pPrevEncoderOutBuffer->nFilledLen;
											if (pPrevEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
											{
												StartPackPrev.StartFramesFlags[StartPackPrev.StartFramesCount] = 2;
												StartPackPrev.StartFramesCount++;
											}
											else 
											{
												StartPackPrev.StartFramesFlags[StartPackPrev.StartFramesCount] = 1;
											}
										} 
										else 
										{
											if (StartPackPrev.StartFramesCount < tVideoInfoPreview.video_intra_frame)
											{
												dbgprintf(3,"Prev Start Frame buffer small (%i), get more memory\n", StartPackPrev.BufferStartSize);
												StartPackPrev.BufferStartSize += (tVideoInfoPreview.video_width * tVideoInfoPreview.video_height * 3) / 2;
												StartPackPrev.BufferStartFrame = (char*)DBG_REALLOC(StartPackPrev.BufferStartFrame, StartPackPrev.BufferStartSize);
												if (StartPackPrev.BufferStartFrame == NULL) dbgprintf(1,"Main Start Frame buffer get memory error\n");	
											}
											else dbgprintf(1,"Prev Start Frame buffer Overfull\n");	
										}
										e_link->FuncSend(iEncFramePrev, (char*)(pPrevEncoderOutBuffer->pBuffer), iBufferSizePrev, 
																	pPrevEncoderOutBuffer->nFilledLen, pPrevEncoderOutBuffer->nFlags, 
																	(void*)&tVideoInfoPreview, (void*)&StartPackPrev, TRAFFIC_PREV_VIDEO);						
										e_link->FuncRTSP(miModule->ID, (char*)(pPrevEncoderOutBuffer->pBuffer), (unsigned int)iBufferSizePrev, (void*)&StartPackPrev);
									
										if (pPrevEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) 
										{
											iEncFramePrev++;
											if (iEncFramePrev > END_VIDEO_FRAME_NUMBER) iEncFramePrev = START_VIDEO_FRAME_NUMBER + 1;	
										}
									} 
									//while(ret == 0);
								}
								pPrevEncoderOutBuffer->nFilledLen = 0;
								omx_add_cmd_in_list(iVideoEncodeNumPrev, OMX_EventFillBuffer, (int)(intptr_t)pPrevEncoderOutBuffer->pAppPrivate);
								if (!omx_activate_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, 0)) break;
							} 
							else 
							{
								dbgprintf(3, "Error buffer point PREV\n");
								usleep(50000);
							}
						}						
						//if (ret == 0) printf("Skipped\n");
					} else dbgprintf(4,"Not encoded preview frame\n");
				} //else printf("Skip prev encode\n");
			}			
			
			if (cSlowEncoderEnabled && (uiSlowFramesCnt >= uiSlowFramesSkip))
			{
				cCurEncoderStatus |= 2;
				omx_add_cmd_in_list(iVideoEncodeNumSlow, OMX_EventEmptyBuffer, (int)(intptr_t)pSlowEncoderInBuffer->pAppPrivate);		
				if (!omx_activate_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN, 0)) break;
				ret = tx_semaphore_wait_event_timeout(m_oCompList[iVideoEncodeNumSlow].psem, OMX_EventEmptyBuffer, (int)(intptr_t)pSlowEncoderInBuffer->pAppPrivate, iTimeOutMain);
				if (ret == 0) 
				{
					dbgprintf(1,"Error encode preview frame %i, %i, %i\n", iNumFrame, pSlowEncoderInBuffer->nFilledLen, pSlowEncoderOutBuffer->nFilledLen);	
					//cSlowEncoderEnabled--;
					cSlowEncoderEnabled = 0;
				}
				ret = tx_semaphore_wait_event_timeout(m_oCompList[iVideoEncodeNumSlow].psem, OMX_EventFillBuffer, (int)(intptr_t)pSlowEncoderOutBuffer->pAppPrivate, uiStepFrameMS);
				if (ret != 0)
				{	
					while ((tx_semaphore_exist_in_list(m_oCompList[iVideoEncodeNumSlow].psem, OMX_EventFillBuffer, (int)(intptr_t)pSlowEncoderOutBuffer->pAppPrivate) == 0)
						&& (pSlowEncoderOutBuffer->nFilledLen != 0))
					{
						if (vControlBufferSlow == pSlowEncoderOutBuffer->pBuffer)
						{
							if (pSlowEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
							{
								if (StartPackSlow.CodecInfoFilled == 1)
								{
									memset(StartPackSlow.BufferCodecInfo, 0, CODECINFO_BUFFER_SIZE);
									StartPackSlow.CodecInfoFilled = 0;
									StartPackSlow.CodecInfoLen = 0;
								}
								if (pSlowEncoderOutBuffer->nFilledLen < (CODECINFO_BUFFER_SIZE - StartPackSlow.CodecInfoLen)) 
								{
									memcpy(StartPackSlow.BufferCodecInfo + StartPackSlow.CodecInfoLen, pSlowEncoderOutBuffer->pBuffer + pSlowEncoderOutBuffer->nOffset, pSlowEncoderOutBuffer->nFilledLen);
									StartPackSlow.CodecInfoLen += pSlowEncoderOutBuffer->nFilledLen;
								}
								if (pSlowEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) 
								{
									StartPackSlow.CodecInfoFilled = 1;
									StartPackSlow.StartFrameFilled = 1;				
								}
							}
							else	
							{
								{						
									if ((pSlowEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME) && (StartPackSlow.CodecInfoFilled == 1) 
										&& ((StartPackSlow.StartFramesCount != 0) || (ucFirstPacketSlow == 1)))
									{
										memset(StartPackSlow.BufferStartFrame, 0, StartPackSlow.BufferStartSize);
										memset(StartPackSlow.StartFramesSizes, 0, tVideoInfoSlow.video_intra_frame*sizeof(unsigned int));
										memset(StartPackSlow.StartFramesFlags, 0, tVideoInfoSlow.video_intra_frame);
										StartPackSlow.StartFrameFilled = 2;
										StartPackSlow.StartFrameLen = 0;
										StartPackSlow.StartFramesCount = 0;
										ucFirstPacketSlow = 0;
										if (iTimeoutLastMoveSlow == 0) StartPackSlow.HaveChangedFrames = 0;	
									}
									if ((pSlowEncoderOutBuffer->nFilledLen < (StartPackSlow.BufferStartSize - StartPackSlow.StartFrameLen)) && (StartPackSlow.StartFramesCount < tVideoInfoSlow.video_intra_frame))
									{
										memcpy(StartPackSlow.BufferStartFrame + StartPackSlow.StartFrameLen, pSlowEncoderOutBuffer->pBuffer + pSlowEncoderOutBuffer->nOffset, pSlowEncoderOutBuffer->nFilledLen);
										StartPackSlow.StartFrameLen += pSlowEncoderOutBuffer->nFilledLen;							
										StartPackSlow.StartFramesSizes[StartPackSlow.StartFramesCount] += pSlowEncoderOutBuffer->nFilledLen;
										if (pSlowEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
										{
											StartPackSlow.StartFramesFlags[StartPackSlow.StartFramesCount] = 2;
											StartPackSlow.StartFramesCount++;
										}
										else 
										{
											StartPackSlow.StartFramesFlags[StartPackSlow.StartFramesCount] = 1;
										}
									}
									else 
									{
										if (StartPackSlow.StartFramesCount < tVideoInfoSlow.video_intra_frame)
										{
											dbgprintf(3,"Slow Start Frame buffer small (%i), get more memory\n", StartPackSlow.BufferStartSize);
											StartPackSlow.BufferStartSize += (tVideoInfoSlow.video_width * tVideoInfoSlow.video_height * 3) / 2;
											StartPackSlow.BufferStartFrame = (char*)DBG_REALLOC(StartPackSlow.BufferStartFrame, StartPackSlow.BufferStartSize);
											if (StartPackPrev.BufferStartFrame == NULL) dbgprintf(1,"Main Start Frame buffer get memory error\n");
										}
										else dbgprintf(1,"Slow Start Frame buffer Overfull\n");	
									}
									
									if (cSlowOnlyDiffRecord)
										e_link->FuncSaveDiff(iEncFrameSlow, (void*)(pSlowEncoderOutBuffer), iBufferSizeSlow - pSlowEncoderOutBuffer->nOffset, 
																(void*)&tVideoInfoSlow, (void*)&StartPackSlow, (void*)pCaptStreamSlow);
										else
										e_link->FuncSave(iEncFrameSlow, (void*)(pSlowEncoderOutBuffer), iBufferSizeSlow - pSlowEncoderOutBuffer->nOffset, 
																(void*)&tVideoInfoSlow, (void*)&StartPackSlow, (void*)pCaptStreamSlow);
									//printf("Send %i %i %i %i %i\n", iEncFrame, iTimeoutLastMoveSlow, StartPackSlow.HaveChangedFrames, pSlowEncoderOutBuffer->nFilledLen, pSlowEncoderOutBuffer->nFlags);						
									
									//iWriteLen = pSlowEncoderOutBuffer->nFilledLen;
									if (pSlowEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_EOS) cStop = 2;
									if (pSlowEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) 
									{
										iEncFrameSlow++;
										if (iEncFrameSlow > END_VIDEO_FRAME_NUMBER) iEncFrameSlow = START_VIDEO_FRAME_NUMBER + 1;	
									}
									if (iTimeoutLastMoveSlow) iTimeoutLastMoveSlow--;	
								} 
								//while(ret == 0);
							}
							pSlowEncoderOutBuffer->nFilledLen = 0;
							omx_add_cmd_in_list(iVideoEncodeNumSlow, OMX_EventFillBuffer, (int)(intptr_t)pSlowEncoderOutBuffer->pAppPrivate);
							if (!omx_activate_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, 0)) break;
						}
						else 
						{
							dbgprintf(3, "Error buffer point SLOW\n");
							usleep(50000);
						}
					}
					//if (ret == 0) printf("Skipped\n");
				} else dbgprintf(4,"Not encoded Slow captured frame\n");
			}		
			
			if ((cNormRecordEnabled == 0) && (cDiffRecordEnabled == 0) && ((cRecSlowEnabled == 0) || cSlowEncoderEnabled) && (iEncFrame != START_VIDEO_FRAME_NUMBER))
			{
				iNeedSend = ItsNetNeed(CONNECT_SERVER, TRAFFIC_FULL_VIDEO, FLAG_START_VIDEO_FRAME | FLAG_NEXT_VIDEO_FRAME | FLAG_VIDEO_STREAM | FLAG_VIDEO_CODEC_INFO | FLAG_VIDEO_PARAMS, 0);
				iNeedRTSP = ItsRTSPNeed(0, StartPack.PacketType, 0);		
			}
			else
			{
				iNeedSend = 1;
				iNeedRTSP = 1;
			}
			if (cNormRecordEnabled || cDiffRecordEnabled || ((cRecSlowEnabled) && (!(cSlowEncoderEnabled))) || iNeedSend || iNeedRTSP || (iEncFrame == START_VIDEO_FRAME_NUMBER))
			{	
				cCurEncoderStatus |= 4;
				omx_add_cmd_in_list(iVideoEncodeNum, OMX_EventEmptyBuffer, (int)(intptr_t)pMainEncoderInBuffer->pAppPrivate);		
				if (!omx_activate_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN, 0)) break;
				ret = tx_semaphore_wait_event_timeout(m_oCompList[iVideoEncodeNum].psem, OMX_EventEmptyBuffer, (int)(intptr_t)pMainEncoderInBuffer->pAppPrivate, iTimeOutMain);
				if (ret == 0)
				{
					dbgprintf(1,"Error encode main capture frame %i, %i, %i\n", iNumFrame, pMainEncoderInBuffer->nFilledLen, pMainEncoderOutBuffer->nFilledLen);
					cCriticalError = 1;
					break;
				}
				
				ret = tx_semaphore_wait_event_timeout(m_oCompList[iVideoEncodeNum].psem, OMX_EventFillBuffer, (int)(intptr_t)pMainEncoderOutBuffer->pAppPrivate, uiStepFrameMS);
				if (ret != 0)
				{
					while ((tx_semaphore_exist_in_list(m_oCompList[iVideoEncodeNum].psem, OMX_EventFillBuffer, (int)(intptr_t)pMainEncoderOutBuffer->pAppPrivate) == 0)
							&& (pMainEncoderOutBuffer->nFilledLen != 0))
					{
						if (vControlBufferNorm == pMainEncoderOutBuffer->pBuffer)
						{
							if (pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
							{
								if (StartPack.CodecInfoFilled == 1)
								{
									memset(StartPack.BufferCodecInfo, 0, CODECINFO_BUFFER_SIZE);
									StartPack.CodecInfoFilled = 0;
									StartPack.CodecInfoLen = 0;
								}
								if (pMainEncoderOutBuffer->nFilledLen < (CODECINFO_BUFFER_SIZE - StartPack.CodecInfoLen)) 
								{
									memcpy(StartPack.BufferCodecInfo + StartPack.CodecInfoLen, pMainEncoderOutBuffer->pBuffer + pMainEncoderOutBuffer->nOffset, pMainEncoderOutBuffer->nFilledLen);
									StartPack.CodecInfoLen += pMainEncoderOutBuffer->nFilledLen;
								}
								if (pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) 
								{
									StartPack.CodecInfoFilled = 1;
									StartPack.StartFrameFilled = 1;				
								}
							}
							else	
							{
								{						
									if ((pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME) && (StartPack.CodecInfoFilled == 1) 
										&& ((StartPack.StartFramesCount != 0) || (ucFirstPacket == 1)))
									{
										memset(StartPack.BufferStartFrame, 0, StartPack.BufferStartSize);
										memset(StartPack.StartFramesSizes, 0, tVideoInfo.video_intra_frame*sizeof(unsigned int));
										memset(StartPack.StartFramesFlags, 0, tVideoInfo.video_intra_frame);
										StartPack.StartFrameFilled = 2;
										StartPack.StartFrameLen = 0;
										StartPack.StartFramesCount = 0;
										ucFirstPacket = 0;
										if (iTimeoutLastMove == 0) StartPack.HaveChangedFrames = 0;	
									}
									if ((pMainEncoderOutBuffer->nFilledLen < (StartPack.BufferStartSize - StartPack.StartFrameLen)) && (StartPack.StartFramesCount < tVideoInfo.video_intra_frame))
									{
										memcpy(StartPack.BufferStartFrame + StartPack.StartFrameLen, pMainEncoderOutBuffer->pBuffer + pMainEncoderOutBuffer->nOffset, pMainEncoderOutBuffer->nFilledLen);
										StartPack.StartFrameLen += pMainEncoderOutBuffer->nFilledLen;							
										StartPack.StartFramesSizes[StartPack.StartFramesCount] += pMainEncoderOutBuffer->nFilledLen;
										if (pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
										{
											StartPack.StartFramesFlags[StartPack.StartFramesCount] = 2;
											StartPack.StartFramesCount++;
										}
										else 
										{
											StartPack.StartFramesFlags[StartPack.StartFramesCount] = 1;
										}
									}
									else 
									{
										if (StartPack.StartFramesCount < tVideoInfo.video_intra_frame)
										{
											dbgprintf(3,"Main Start Frame buffer small (%i), get more memory\n", StartPack.BufferStartSize);
											StartPack.BufferStartSize += (tVideoInfo.video_width * tVideoInfo.video_height * 3) / 2;
											StartPack.BufferStartFrame = (char*)DBG_REALLOC(StartPack.BufferStartFrame, StartPack.BufferStartSize);
											if (StartPack.BufferStartFrame == NULL) dbgprintf(1,"Main Start Frame buffer get memory error\n");	
										}
										else dbgprintf(1,"Main Start Frame buffer Overfull\n");	
									}
									
									if (cNormRecordEnabled) 
										e_link->FuncSave(0, (void*)(pMainEncoderOutBuffer), iBufferSize - pMainEncoderOutBuffer->nOffset, 
																(void*)&tVideoInfo, (void*)&StartPack, (void*)pCaptStreamNorm);		
									if (cDiffRecordEnabled) 
										e_link->FuncSaveDiff(0, (void*)(pMainEncoderOutBuffer), iBufferSize - pMainEncoderOutBuffer->nOffset, 
																(void*)&tVideoInfo, (void*)&StartPack, (void*)pCaptStreamDiff);
									if ((cRecSlowEnabled) && (!(cSlowEncoderEnabled)))
									{
										e_link->FuncSaveSlow(0, (void*)(pMainEncoderOutBuffer), iBufferSize - pMainEncoderOutBuffer->nOffset, 
																(void*)&tVideoInfoSlow, (void*)&StartPack, (void*)pCaptStreamSlow);
									}
									if (iNeedSend)
										e_link->FuncSend(iEncFrame, (char*)(pMainEncoderOutBuffer->pBuffer), iBufferSize, 
																pMainEncoderOutBuffer->nFilledLen, pMainEncoderOutBuffer->nFlags, (void*)&tVideoInfo, (void*)&StartPack, TRAFFIC_FULL_VIDEO);
									if (iNeedRTSP)
										e_link->FuncRTSP(miModule->ID, (char*)(pMainEncoderOutBuffer->pBuffer), (unsigned int)iBufferSize, (void*)&StartPack);
									
									//iWriteLen = pMainEncoderOutBuffer->nFilledLen;
									
									if (pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_EOS) cStop = 2;
									if (pMainEncoderOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) 
									{
										iEncFrame++;
										if (iEncFrame > END_VIDEO_FRAME_NUMBER) iEncFrame = START_VIDEO_FRAME_NUMBER + 1;	
									}
								} 
							}
							pMainEncoderOutBuffer->nFilledLen = 0;
							omx_add_cmd_in_list(iVideoEncodeNum, OMX_EventFillBuffer, (int)(intptr_t)pMainEncoderOutBuffer->pAppPrivate);
							if (!omx_activate_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, 0)) break;
						}
						else 
						{
							dbgprintf(3, "Error buffer point NORM\n");
							usleep(50000);
						}
					}
				} else dbgprintf(4,"Not encoded Norm captured frame\n");			
			} //else printf("Skip norm encode\n");
		}
		
		iNumFrame++;
		if (iNumFrame > END_VIDEO_FRAME_NUMBER) iNumFrame = START_VIDEO_FRAME_NUMBER + 1;				
			
		if (cSlowEncoderEnabled)
		{
			if (uiSlowFramesCnt >= uiSlowFramesSkip) uiSlowFramesCnt = 0; else uiSlowFramesCnt++;
		}
		if (cCurEncoderStatus != cOmxEncoderStatus)
		{
			cOmxEncoderStatus = cCurEncoderStatus;
			DBG_MUTEX_LOCK(&OMX_mutex);
			cThreadOmxEncoderStatus = cOmxEncoderStatus;
			DBG_MUTEX_UNLOCK(&OMX_mutex);
		}
		//iFileLen += iWriteLen;
		//dbgprintf(8,"Write file:%i/%i\n", iWriteLen,iFileLen);
		if (iTimeoutLastMove) iTimeoutLastMove--;	
		
		//usleep(1000);
	}
	
	dbgprintf(8,"Loop break capture\n");	
	if (pOriginalMainEncoderBuffer) pMainEncoderInBuffer->pBuffer = pOriginalMainEncoderBuffer;
	if (pOriginalPrevEncoderBuffer) pPrevEncoderInBuffer->pBuffer = pOriginalPrevEncoderBuffer;
	if (iSensorResizerEnabled && pOriginalSensResizeBuffer) pSensResizerInBuffer->pBuffer = pOriginalSensResizeBuffer;
	
	omx_set_camera_capture_status(iCameraNum, OMX_FALSE, OMX_FALSE);

	omx_reset_exec_cmd(iCameraNum);
	omx_reset_exec_cmd(iResizerNumPrev);
	if (iSensorResizerEnabled) omx_reset_exec_cmd(iResizerNumSens);
	omx_reset_exec_cmd(iVideoEncodeNum);
	
	omx_flush_port(iCameraNum, OMX_PORT_2, OMX_PORT_OUT, OMX_NOW);
	omx_flush_port(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	if (iSensorResizerEnabled)
	{
		omx_flush_port(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
		omx_flush_port(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	}
	omx_flush_port(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_flush_port(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
	omx_set_state(iCameraNum, OMX_StateIdle, OMX_NOW);
	omx_set_state(iResizerNumPrev, OMX_StateIdle, OMX_NOW);
	if (iSensorResizerEnabled) omx_set_state(iResizerNumSens, OMX_StateIdle, OMX_NOW);
	omx_set_state(iVideoEncodeNum, OMX_StateIdle, OMX_NOW);
	omx_disable_port(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
	omx_remove_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_IN); 	
	omx_disable_port(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_remove_buffers(iVideoEncodeNum, OMX_PORT_1, OMX_PORT_OUT); 	
	omx_disable_port(iCameraNum, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);
	omx_disable_port(iResizerNumPrev, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
	omx_disable_port(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
	omx_remove_buffers(iResizerNumPrev, OMX_PORT_1, OMX_PORT_OUT);  	
	if (iSensorResizerEnabled) 
	{
		omx_disable_port(iResizerNumSens, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
		omx_disable_port(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_remove_buffers(iResizerNumSens, OMX_PORT_1, OMX_PORT_OUT);  	
	}
	omx_disable_port(iCameraNum, OMX_PORT_2, OMX_PORT_OUT, OMX_LATER);
	omx_remove_buffers(iCameraNum, OMX_PORT_2, OMX_PORT_OUT);  
	omx_set_tunnel(iCameraNum, OMX_PORT_1, -1, 0, OMX_FALSE);	
	omx_set_state(iCameraNum, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iResizerNumPrev, OMX_StateLoaded, OMX_NOW);
	if (iSensorResizerEnabled) omx_set_state(iResizerNumSens, OMX_StateLoaded, OMX_NOW);
	omx_set_state(iVideoEncodeNum, OMX_StateLoaded, OMX_NOW);
	omx_Release_Component(iCameraNum);
	omx_Release_Component(iResizerNumPrev);
	if (iSensorResizerEnabled) omx_Release_Component(iResizerNumSens);
	omx_Release_Component(iVideoEncodeNum);
	DBG_FREE(StartPack.BufferCodecInfo);
	DBG_FREE(StartPack.BufferStartFrame);
	DBG_FREE(StartPack.StartFramesFlags);
	DBG_FREE(StartPack.StartFramesSizes);
	
	if (cSlowEncoderEnabled)
	{
		omx_reset_exec_cmd(iVideoEncodeNumSlow);
		omx_flush_port(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
		omx_flush_port(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
		omx_set_state(iVideoEncodeNumSlow, OMX_StateIdle, OMX_NOW);
		omx_disable_port(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
		omx_remove_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_IN); 	
		omx_disable_port(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_remove_buffers(iVideoEncodeNumSlow, OMX_PORT_1, OMX_PORT_OUT); 
		omx_set_state(iVideoEncodeNumSlow, OMX_StateLoaded, OMX_NOW);
		omx_Release_Component(iVideoEncodeNumSlow);	
		DBG_FREE(StartPackSlow.BufferCodecInfo);
		DBG_FREE(StartPackSlow.BufferStartFrame);
		DBG_FREE(StartPackSlow.StartFramesFlags);
		DBG_FREE(StartPackSlow.StartFramesSizes);
	}
	if (cPreviewEnabled)
	{
		omx_reset_exec_cmd(iVideoEncodeNumPrev);
		omx_flush_port(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN, OMX_NOW);
		omx_flush_port(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, OMX_NOW);	
		omx_set_state(iVideoEncodeNumPrev, OMX_StateIdle, OMX_NOW);
		omx_disable_port(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN, OMX_LATER);
		omx_remove_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_IN); 	
		omx_disable_port(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT, OMX_LATER);
		omx_remove_buffers(iVideoEncodeNumPrev, OMX_PORT_1, OMX_PORT_OUT); 
		omx_set_state(iVideoEncodeNumPrev, OMX_StateLoaded, OMX_NOW);
		omx_Release_Component(iVideoEncodeNumPrev);
		DBG_FREE(StartPackPrev.BufferCodecInfo);
		DBG_FREE(StartPackPrev.BufferStartFrame);
		DBG_FREE(StartPackPrev.StartFramesFlags);
		DBG_FREE(StartPackPrev.StartFramesSizes);
	}
	
stopcapt:	

	if ((autofocus_pm.Enabled) && (autofocus_pm.PtzFocusMapEnabled))
	{
		if (autofocus_pm.PtzFocusMapChanged) 
		{
			dbgprintf(3, "Save Settings/ptz_focus_map.bin\n");	
			omx_dump_data("Settings/ptz_focus_map.bin", (char*)autofocus_pm.PtzFocusMap, 2300*sizeof(int16_t)); 
			omx_PrintFocusMapFilled(&autofocus_pm);
		}
		DBG_FREE(autofocus_pm.PtzFocusMap);
	}
	
	if (riOMX_RectangleList) DBG_FREE(riOMX_RectangleList);
	//fclose(video_file);
	
	tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_BUSY_CAPTURE, TX_ANY);   
	tx_semaphore_delete_from_list(&psem_omx_run, OMX_EVENT_STOP_CAPTURE, TX_ANY);   
	
	if (iHighLightMode) 
	{
		DBG_FREE(ucMainCameraPrevBuff);
		ucMainCameraPrevBuff = NULL;
		DBG_FREE(ucMainCameraMask);
		ucMainCameraMask = NULL;
		DBG_FREE(ucMainCameraResultBuff);
		ucMainCameraResultBuff = NULL;		
	}
	if (cNeedControl) DBG_FREE(pucMovePrevBuffer);
	if (cLongNeedControl) 
	{
		DBG_FREE(pucLongPrevDiff12);		
		DBG_FREE(pucLongPrevDiff23); 
		DBG_FREE(pucLongPrevDiff34); 
		DBG_FREE(pucLongPrevFrame3);
		DBG_FREE(pucLongPrevFrame4);
		if (cLongNeedControl & (2 | 4))
		{
			DBG_FREE(pucLongPrevFrame2);
			DBG_FREE(pucLongPrevDiff24);
		}
		if (cLongNeedControl & 4)
		{
			DBG_FREE(pucLongPrevFrame1);
			
			DBG_FREE(pucLongLimitDiff12);
			DBG_FREE(pucLongLimitDiff23);
			DBG_FREE(pucLongLimitDiff34);
			DBG_FREE(pucLongLimitDiff24);	
		}
	}
	if ((cDiffRecordEnabled) || (cSlowOnlyDiffRecord)) DBG_FREE(pucDiffMovePrevBuffer);
	
	if (e_link->pSubModule) DBG_FREE(e_link->pSubModule);
	if (e_link->pModule) DBG_FREE(e_link->pModule);
	DBG_FREE(e_link);
	DBG_FREE(timestamptext.data); 	
	
	DBG_MUTEX_LOCK(&OMX_mutex);
	cThreadOmxCaptureStatus--;
	DBG_MUTEX_UNLOCK(&OMX_mutex);
	
	DBG_MUTEX_LOCK(&system_mutex);
	if (uiErrorCameraRestart && cCriticalError) 
	{
		unsigned int uitWait = uiErrorCameraRestartWait;
		DBG_MUTEX_UNLOCK(&system_mutex);
		if (uitWait) usleep(uitWait * 1000);
		if (uiErrorCameraRestart == 1) System_Restart(NULL, 0); else System_Reboot(NULL, 0);
		uiErrorCameraRestart = 100;
	} else DBG_MUTEX_UNLOCK(&system_mutex);
	
	DBG_LOG_OUT();  
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return (void*)1;
}

int omx_Release()
{
	DBG_LOG_IN();
	
	int n;
	DBG_MUTEX_LOCK(&psem_components.mutex);	
    for (n = 0; n != OMX_COMPONENTS_MAX; n++)
    {
		if (m_oCompList[n].psem != 0)
		{
			if (m_oCompList[n].handle != NULL) OMX_FreeHandle(m_oCompList[n].handle);
			tx_semaphore_delete(m_oCompList[n].psem);
			DBG_FREE(m_oCompList[n].psem);
			memset(&m_oCompList[n], 0, sizeof(omx_component));
		}
    } 
	DBG_MUTEX_UNLOCK(&psem_components.mutex);			
    
	DBG_LOG_OUT();  
	return 1;
}

int omx_Release_Component(int iNum)
{
	DBG_LOG_IN();
	DBG_MUTEX_LOCK(&psem_components.mutex);    
	if (m_oCompList[iNum].handle != NULL) OMX_FreeHandle(m_oCompList[iNum].handle);
	tx_semaphore_delete(m_oCompList[iNum].psem);
	DBG_FREE(m_oCompList[iNum].psem); 
	memset(&m_oCompList[iNum], 0, sizeof(omx_component));
    DBG_MUTEX_UNLOCK(&psem_components.mutex);	
	DBG_LOG_OUT();  
	return 1;
}

int omx_Stop() 
{
	DBG_LOG_IN();
	
	if (m_iStarted != 1) {DBG_LOG_OUT();return 0;}
	if (omx_IsFree() == 0) dbgprintf(2,"Stopping OMX error, OMX is BUSY. WAITING...\n");
	
	tx_semaphore_wait(&psem_omx_run);
	
	tx_semaphore_delete(&psem_omx_sync);
	tx_semaphore_delete(&psem_omx_run);
	
	pthread_attr_destroy(&tattr);
	omx_Release();  
	tx_semaphore_delete(&psem_components);	
	
	pthread_mutex_destroy(&OMX_mutex);
	
	if (OMX_Deinit()!= OMX_ErrorNone) {DBG_LOG_OUT();return 0;}
	m_iStarted = 0;
	
	DBG_LOG_OUT();  
	return 1;
}
