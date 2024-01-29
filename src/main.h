#ifndef _MAIN_H_
#define _MAIN_H_

#include "debug.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
//#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <libavcodec/avcodec.h>
#include <GLES2/gl2.h>
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "pthread2threadx.h"
#include "system.h"

#define MODULE_SECSET_OUTPUT	1
#define MODULE_SECSET_INVERSE	2

#define MODULE_SECSET_MONO		1
#define MODULE_SECSET_STEREO	2

#define MISC_INFO_TYPE_MODULE	1

#define MODULE_STATUS_INIT		1

#define MODULE_ENENT_CNT_MAX	1024
#define PTZ_SETTINGS_MAX_CNT	20

#define DIR_NORM				"Norm"
#define DIR_SLOW				"Slow"
#define DIR_DIFF				"Diff"
#define DIR_AUD					"Audio"
#define DIR_STAT				"Statuses"
#define DIR_EVNT				"Events"
#define DIR_ACT					"Actions"
#define CMD_SAVE_COPY_OFFSET	64

#define FILE_TYPE_NA			0
#define FILE_TYPE_IMAGE			1
#define FILE_TYPE_VIDEO			2
#define FILE_TYPE_AUDIO			4
#define FILE_TYPE_MEDIA			8

#define ALARM_TYPE_NULL			0
#define ALARM_TYPE_CLOCK		1
#define ALARM_TYPE_DOOR			2

#define ACTION_FLAG_START		1
#define ACTION_FLAG_VIDEO_FRAME	2
#define ACTION_FLAG_AUDIO_FRAME	4
#define ACTION_FLAG_STOP		8

#define ACTION_TEST_TYPE_EQUALLY	61
#define ACTION_TEST_TYPE_LESS		60
#define ACTION_TEST_TYPE_MORE		62
#define ACTION_TEST_TYPE_AND		38
#define ACTION_TEST_TYPE_NOT		33

#define MAX_SECURITY_KEYS		255
#define MAX_MODULE_SETTINGS		43
#define MAX_MODULE_STATUSES		26
#define MAX_MODULE_INFO_COUNT	255
#define MAX_SYSTEM_INFO_COUNT	255
#define MAX_SYSTEM_INFO_LIVE	60

#define MAX_PATH				256

#define START_VIDEO_FRAME_NUMBER 1000
#define END_VIDEO_FRAME_NUMBER 1999

#define START_AUDIO_FRAME_NUMBER 1000
#define END_AUDIO_FRAME_NUMBER 1999

#define MAX_IRCOMMAND_LEN 			128
#define MAX_FILE_LEN 				256
#define MAX_SYS_MESSAGE_LEN			128
#define MAX_ACCESS_LEVELS			10
#define MAX_MENU_PAGES				27
#define MAX_DISPLAY_CONTENTS		128
#define MAX_SECURITY_SERIAL_LEN		16

#define SHOW_VOLUME_TIME 			7
#define SHOW_TIMER_TIME				7
#define SHOW_PLAYSLIDE_TIME 		15
#define DEFAULT_ACCESS_LEVEL 		0
#define TERMINAL_MENU_SKIP_FRAMES	7

//#define PRIORITY_FS_TIME_LIFE		15

#define SHOW_TYPE_NA			0x00000000
#define SHOW_TYPE_IMAGE			0x00000001
#define SHOW_TYPE_VIDEO			0x00000002
#define SHOW_TYPE_AUDIO			0x00000004
#define SHOW_TYPE_CAMERA		0x00000008
#define SHOW_TYPE_ALARM1		0x00000010
#define SHOW_TYPE_MIC_STREAM	0x00000020
#define SHOW_TYPE_FILE			0x00000040
#define SHOW_TYPE_URL			0x00000080
#define SHOW_TYPE_WEB_STREAM	0x00000100
#define SHOW_TYPE_OFF			0x00000200
#define SHOW_TYPE_NEW			0x00000400
#define SHOW_TYPE_RADIO_STREAM	0x00000800
#define SHOW_TYPE_CLOSE_VIDEO	0x00001000
#define SHOW_TYPE_CLOSE_AUDIO	0x00002000
#define SHOW_TYPE_CAMERA_LIST	0x00004000
#define SHOW_TYPE_CAMERA_FILE	0x00008000
#define SHOW_TYPE_MIC_FILE		0x00010000
#define SHOW_TYPE_ERROR			0x00020000

#define SAMPLE_RATE_ANY			0
#define SAMPLE_RATE_8000		1
#define SAMPLE_RATE_11025		2
#define SAMPLE_RATE_16000		4
#define SAMPLE_RATE_22050		8
#define SAMPLE_RATE_44100		16
#define SAMPLE_RATE_48000		32
#define SAMPLE_RATE_96000		64
#define SAMPLE_RATE_UNKNOWN		128

#define ACCESS_WEB			0x00000001
#define ACCESS_RTSP			0x00000002
#define ACCESS_USERS		0x00000004
#define ACCESS_MENU			0x00000008
#define ACCESS_MODULES		0x00000010
#define ACCESS_ALARMS		0x00000020
#define ACCESS_SOUNDS		0x00000040
#define ACCESS_MAILS		0x00000080
#define ACCESS_STREAMTYPES	0x00000100
#define ACCESS_STREAMS		0x00000200
#define ACCESS_WIDGETS		0x00000400
#define ACCESS_DIRECTORIES	0x00000800
#define ACCESS_EVNTACTIONS	0x00001000
#define ACCESS_MNLACTIONS	0x00002000
#define ACCESS_KEYS			0x00004000
#define ACCESS_IRCODES		0x00008000
#define ACCESS_CAMRECTS		0x00010000
#define ACCESS_CONTROL		0x00020000
#define ACCESS_YOUTUBE		0x00040000
#define ACCESS_MEDIA		0x00080000
#define ACCESS_MANUAL		0x00100000
#define ACCESS_MODSTATUSES	0x00200000
#define ACCESS_SETTINGS		0x00400000
#define ACCESS_RADIOS		0x00800000
#define ACCESS_LOG			0x01000000
#define ACCESS_EXPLORER		0x02000000
#define ACCESS_CAMERA		0x04000000
#define ACCESS_MIC			0x08000000
#define ACCESS_SYSTEM		0x10000000
#define ACCESS_CONNECTS		0x20000000
#define ACCESS_HISTORY		0x40000000
#define ACCESS_ONVIF		0x80000000


#define DEVICE_TYPE_VIDEO_OUT 			0x000001
#define DEVICE_TYPE_AUDIO_OUT 			0x000002
#define DEVICE_TYPE_VIDEO_IN 			0x000004
#define DEVICE_TYPE_AUDIO_IN 			0x000008
#define DEVICE_TYPE_UART 				0x000010
#define DEVICE_TYPE_I2C					0x000020
#define DEVICE_TYPE_GPIO				0x000040
#define DEVICE_TYPE_CLOCK				0x000080
#define DEVICE_TYPE_RADIO				0x000100
#define DEVICE_TYPE_TEMP				0x000200
#define DEVICE_TYPE_PN532				0x000400
#define DEVICE_TYPE_SPI					0x000800
#define DEVICE_TYPE_RS485				0x001000
#define DEVICE_TYPE_SECURITY			0x002000
#define DEVICE_TYPE_RC522				0x004000
#define DEVICE_TYPE_IR_RECEIVER			0x008000
#define DEVICE_TYPE_USB_GPIO			0x010000
#define DEVICE_TYPE_KEYBOARD			0x020000

#define WIDGET_SHOWMODE_DAYTIME			1
#define WIDGET_SHOWMODE_MENU			2
#define WIDGET_SHOWMODE_TIMEOUT			4
#define WIDGET_SHOWMODE_ALLWAYS			8


#define log_print(Buff, ...) debug_fprintf(Buff, ##__VA_ARGS__)

enum POST_ACTION
{
	POST_ACTION_STOP = 1,
	POST_ACTION_OFF,
	POST_ACTION_OFF_STOP,
	POST_ACTION_STOP_IF_WAIT
};

enum CAPTURE_TYPE
{
	CAPTURE_TYPE_FULL,
	CAPTURE_TYPE_SLOW,
	CAPTURE_TYPE_DIFF,
	CAPTURE_TYPE_AUDIO,
	CAPTURE_TYPE_MODULE,
	CAPTURE_TYPE_EVENT,
	CAPTURE_TYPE_ACTION,
	CAPTURE_TYPE_ARCHIVE
};

enum MODULE_TYPE 
{ 
	MODULE_TYPE_UNKNOWN,
	MODULE_TYPE_CAMERA,
	MODULE_TYPE_COUNTER,
	MODULE_TYPE_DISPLAY,
	MODULE_TYPE_GPIO,
	MODULE_TYPE_IR_RECEIVER,
	MODULE_TYPE_MEMORY,
	MODULE_TYPE_MIC,
	MODULE_TYPE_PN532,
	MODULE_TYPE_RADIO,
	MODULE_TYPE_RC522,
	MODULE_TYPE_RTC,
	MODULE_TYPE_RS485,
	MODULE_TYPE_SPEAKER,
	MODULE_TYPE_SYSTEM,
	MODULE_TYPE_TEMP_SENSOR,
	MODULE_TYPE_TIMER,
	MODULE_TYPE_KEYBOARD,
	MODULE_TYPE_USB_GPIO,
	MODULE_TYPE_EXTERNAL,
	MODULE_TYPE_ADS1015,
	MODULE_TYPE_MCP3421,
	MODULE_TYPE_AS5600,
	MODULE_TYPE_HMC5883L,
	MODULE_TYPE_TFP625A,
	MODULE_TYPE_MAX
};

enum MODULE_SUBTYPE 
{ 
	MODULE_SUBTYPE_UNKNOWN,
	MODULE_SUBTYPE_IO,
	MODULE_SUBTYPE_GATE,
	MODULE_SUBTYPE_PTZ,
	MODULE_SUBTYPE_MAX
};

enum RESOLUTION_TYPE
{
	RESOLUTION_TYPE_UNKNOWN,
	RESOLUTION_TYPE_160X120,
	RESOLUTION_TYPE_320X240,
	RESOLUTION_TYPE_480X240,
	RESOLUTION_TYPE_640X480,
	RESOLUTION_TYPE_800X600,
	RESOLUTION_TYPE_1024X768,
	RESOLUTION_TYPE_1280X960,
	RESOLUTION_TYPE_1400X1050,
	RESOLUTION_TYPE_1600X1200,
	RESOLUTION_TYPE_2048X1536,
	RESOLUTION_TYPE_2592X1944,
	RESOLUTION_TYPE_MAX
};

enum UPDATE_LIST_SCANTYPE
{
	UPDATE_LIST_SCANTYPE_FULL,
	UPDATE_LIST_SCANTYPE_SUB,
	UPDATE_LIST_SCANTYPE_CUR,
	UPDATE_LIST_SCANTYPE_NEXT,
	UPDATE_LIST_SCANTYPE_PREV,
	UPDATE_LIST_SCANTYPE_CHANGE
};

enum SYSTEM_CMD 
{
	SYSTEM_CMD_NULL = 0x40000000,
	MENU_PAGE_MAIN,
	MENU_PAGE_CAMERA_LIST,
	MENU_PAGE_RING_LIST,
	MENU_PAGE_CAMERA_OFF_ACT,
	MENU_PAGE_FOTO_ACT,
	MENU_PAGE_MEDIA_ACT,
	MENU_PAGE_RADIO_ACT,
	MENU_PAGE_MEDIA_OFF,
	MENU_PAGE_MODULE_LIST,
	MENU_PAGE_MODULE_ACT,
	MENU_PAGE_MAIN_VOLUME,
	MENU_PAGE_RING_VOLUME,
	MENU_PAGE_MESSAGE_LIST,
	MENU_PAGE_CAMERA_ACT,
	MENU_PAGE_CAMERA_EXP,
	MENU_PAGE_WIDGET_LIST,
	MENU_PAGE_DIRECTORIES,
	MENU_PAGE_FILES,
	MENU_PAGE_STREAMTYPE_LIST,
	MENU_PAGE_SYSTEM_ACT,
	MENU_PAGE_STREAM_LIST,
	MENU_PAGE_ACTION_LIST,
	MENU_PAGE_SYS_ACTION_LIST,
	MENU_PAGE_PLAY_LIST,
	MENU_PAGE_DIR_LIST,
	MENU_PAGE_MAX,
	MODULE_COUNTER_RESET,
	MODULE_COUNTER_SET,
	MODULE_COUNTER_INCREMENT,
	MODULE_COUNTER_DECREMENT,
	MODULE_COUNTER_SET_VALUE,
	MODULE_COUNTER_SET_INC,
	MODULE_COUNTER_SET_DEC,
	MODULE_COUNTER_SET_N_VALUE,
	MODULE_COUNTER_INC_N_VALUE,
	MODULE_COUNTER_DEC_N_VALUE,
	SYSTEM_CMD_CLOCK_ALARM_ON,
	SYSTEM_CMD_CLOCK_ALARM_OFF,
	SYSTEM_CMD_MENU_KEY_ON,
	SYSTEM_CMD_MENU_KEY_OFF,
	SYSTEM_CMD_MENU_KEY_UP,
	SYSTEM_CMD_MENU_KEY_DOWN,
	SYSTEM_CMD_MENU_KEY_LEFT,
	SYSTEM_CMD_MENU_KEY_RIGHT,
	SYSTEM_CMD_MENU_KEY_OK,
	SYSTEM_CMD_MENU_KEY_MENU,
	SYSTEM_CMD_MENU_KEY_BACK,	
	SYSTEM_CMD_SET_MESSAGE,
	SYSTEM_CMD_CLEAR_MESSAGE,
	SYSTEM_CMD_DISPLAY_SET_TEXT,
	SYSTEM_CMD_DISPLAY_CLEAR_TEXT,
	SYSTEM_CMD_EMAIL,
	SYSTEM_CMD_DOOR_ALARM_ON,
	SYSTEM_CMD_SHOW_FULL_NEXT,
	SYSTEM_CMD_SHOW_NEXT,
	SYSTEM_CMD_SHOW_PREV,
	SYSTEM_CMD_SHOW_PLAY,
	SYSTEM_CMD_SHOW_PAUSE,
	SYSTEM_CMD_SHOW_FORWARD,
	SYSTEM_CMD_SHOW_BACKWARD,
	SYSTEM_CMD_SHOW_AGAIN,
	SYSTEM_CMD_SHOW_STOP_AV,
	SYSTEM_CMD_SHOW_STOP_VIDEO,
	SYSTEM_CMD_SHOW_STOP_AUDIO,
	SYSTEM_CMD_SHOW_INFO,
	SYSTEM_CMD_SHOW_EVENT_TEXT,
	SYSTEM_CMD_EXIT,
	SYSTEM_CMD_REBOOT,
	SYSTEM_CMD_SHUTDOWN,
	SYSTEM_CMD_RESTART,
	SYSTEM_CMD_LOST,
	SYSTEM_CMD_FIND,
	SYSTEM_CMD_NEW,
	SYSTEM_CMD_SAVE_REC_NORM,
	SYSTEM_CMD_SAVE_REC_SLOW,
	SYSTEM_CMD_SAVE_REC_DIFF,
	SYSTEM_CMD_RADIO_VOLUME_DOWN,
	SYSTEM_CMD_RADIO_VOLUME_UP,
	SYSTEM_CMD_RADIO_STATION_NEXT,
	SYSTEM_CMD_RADIO_STATION_PREV,
	SYSTEM_CMD_ALARM_VOLUME_DEC,
	SYSTEM_CMD_ALARM_VOLUME_INC,
	SYSTEM_CMD_ALARM_VOLUME_DOWN,
	SYSTEM_CMD_ALARM_VOLUME_UP,
	SYSTEM_CMD_ALARM_VOLUME_SET,
	SYSTEM_CMD_SYS_SOUND_PLAY,
	SYSTEM_CMD_SOUND_VOLUME_DEC,
	SYSTEM_CMD_SOUND_VOLUME_INC,
	SYSTEM_CMD_SOUND_VOLUME_DOWN,
	SYSTEM_CMD_SOUND_VOLUME_UP,
	SYSTEM_CMD_SOUND_VOLUME_MUTE,
	SYSTEM_CMD_SOUND_VOLUME_SET,
	SYSTEM_CMD_MIC_VOLUME_SET,
	SYSTEM_CMD_MIC_VOLUME_DEC,
	SYSTEM_CMD_MIC_VOLUME_INC,
	SYSTEM_CMD_MIC_VOLUME_DOWN,
	SYSTEM_CMD_MIC_VOLUME_UP,
	SYSTEM_CMD_SOFT_VOLUME_SET,
	SYSTEM_CMD_SOFT_VOLUME_STEP,
	SYSTEM_CMD_RADIO_ON,
	SYSTEM_CMD_RADIO_OFF,
	SYSTEM_CMD_ACTION_ON,
	SYSTEM_CMD_ACTION_OFF,
	SYSTEM_CMD_SET_ACCESS_LEVEL,
	SYSTEM_CMD_SKIP,
	SYSTEM_CMD_STREAM_ON,
	SYSTEM_CMD_STREAM_OFF,
	SYSTEM_CMD_STREAM_ON_LAST,
	SYSTEM_CMD_STREAM_ON_NEXT,
	SYSTEM_CMD_STREAM_ON_PREV,
	SYSTEM_CMD_STREAM_TYPE_ON,
	SYSTEM_CMD_STREAM_TYPE_RND_ON,
	SYSTEM_CMD_STREAM_RND_ON,
	SYSTEM_CMD_PLAY_DIR,
	SYSTEM_CMD_PLAY_FILE,
	SYSTEM_CMD_PLAY_DIR_RND_FILE,
	SYSTEM_CMD_PLAY_NEXT_DIR,
	SYSTEM_CMD_PLAY_PREV_DIR,
	SYSTEM_CMD_REDIRECT,
	SYSTEM_CMD_RANDOM_FILE_ON,
	SYSTEM_CMD_RANDOM_FILE_OFF,
	SYSTEM_CMD_RANDOM_DIR_ON,
	SYSTEM_CMD_RANDOM_DIR_OFF,
	SYSTEM_CMD_CLIENT_STOP,
	SYSTEM_CMD_FULL_STOP,
	SYSTEM_CMD_EVENT_TIMER,
	SYSTEM_CMD_EVENT_START,
	SYSTEM_CMD_WIDGET_STATUS_OFF,
	SYSTEM_CMD_WIDGET_STATUS_ALLWAYS,
	SYSTEM_CMD_WIDGET_STATUS_MENU,
	SYSTEM_CMD_WIDGET_STATUS_DAYTIME,
	SYSTEM_CMD_WIDGET_STATUS_TIMEOUT,
	SYSTEM_CMD_WIDGET_TIMEUP,
	SYSTEM_CMD_WIDGET_TIMEDOWN,
	SYSTEM_CMD_WIDGET_UPDATE,
	SYSTEM_CMD_SHOW_MENU,
	SYSTEM_CMD_HIDE_MENU,
	SYSTEM_CMD_PLAY_YOUTUBE_JWZ, 
	SYSTEM_CMD_PLAY_YOUTUBE_DL, 
	SYSTEM_CMD_PLAY_NEW_POS,
	SYSTEM_CMD_MENU_FORWARD,
	SYSTEM_CMD_MENU_BACKWARD,
	SYSTEM_CMD_MENU_NEXT,
	SYSTEM_CMD_MENU_PREV,
	SYSTEM_CMD_SET_SHOW_MODE,
	SYSTEM_CMD_SET_SHOW_STATUS,
	SYSTEM_CMD_SET_ZOOM_MODE,
	SYSTEM_CMD_CAMLIST_CLEAR,
	SYSTEM_CMD_CAMLIST_ADD,
	SYSTEM_CMD_CAMLIST_SHOW,
	SYSTEM_CMD_CAMERA_ERROR,
	SYSTEM_CMD_OPENED_RTSP,
	SYSTEM_CMD_OPENED_CAMERA,
	SYSTEM_CMD_OPENED_MIC,
	SYSTEM_CMD_CLOSED_RTSP,
	SYSTEM_CMD_CLOSED_CAMERA,
	SYSTEM_CMD_CLOSED_MIC,
	SYSTEM_CMD_TIMERS_INCREASE,
	SYSTEM_CMD_TIMERS_DECREASE,
	SYSTEM_CMD_TIMERS_UPDATE,
	SYSTEM_CMD_SAVE_REC_AUD,
	SYSTEM_CMD_SAVE_COPY_REC_AUD,
	SYSTEM_CMD_SAVE_COPY_REC_NORM,
	SYSTEM_CMD_SAVE_COPY_REC_SLOW,
	SYSTEM_CMD_SAVE_COPY_REC_DIFF,
	SYSTEM_CMD_RESET_TIMER,
	SYSTEM_CMD_AUDIO_ERROR,
	SYSTEM_CMD_VIDEO_ERROR,
	SYSTEM_CMD_CLOSE,
	SYSTEM_CMD_PLAY_VIDEO,
	SYSTEM_CMD_PLAY_AUDIO,
	SYSTEM_CMD_STOPED_VIDEO,
	SYSTEM_CMD_STOPED_AUDIO,	
	SYSTEM_CMD_RESET_STATUS_NUM,
	SYSTEM_CMD_SET_STATUS_NUM,
	SYSTEM_CMD_VALUE_STATUS_NUM,
	SYSTEM_CMD_RESET_STATUS_MESSAGE,
	SYSTEM_CMD_SET_STATUS_MESSAGE,
	SYSTEM_CMD_EXEC_EXT_APP_NOW,
	SYSTEM_CMD_EXEC_EXT_APP_EXIT,
	SYSTEM_CMD_SET_SHOW_PATH,	
	SYSTEM_CMD_SET_PLAYLIST_POS,
	SYSTEM_CMD_SET_DIRLIST_POS,
	SYSTEM_CMD_ACTION_TEMP_OFF,
	SYSTEM_CMD_OPENED_FILE,
	SYSTEM_CMD_CLOSED_FILE,
	SYSTEM_CMD_DONE_FILE,
	SYSTEM_CMD_NEW_FILE_POS,
	SYSTEM_CMD_ERROR_FILE,
	SYSTEM_CMD_BUSY_FILE,
	SYSTEM_CMD_FREE_FILE,
	SYSTEM_CMD_EVENT_HOURS,
	SYSTEM_CMD_EVENT_MINUTES,
	SYSTEM_CMD_EVENT_SECONDS,
	EXPOSURE_OFF,
	EXPOSURE_AUTO,
	EXPOSURE_NIGHT,
	EXPOSURE_BACKLIGHT,
	EXPOSURE_SPOTLIGHT,
	EXPOSURE_SPORTS,
	EXPOSURE_SNOW,
	EXPOSURE_BEACH,
	EXPOSURE_LARGEAPERTURE,
	EXPOSURE_SMALLAPERTURE,
	EXPOSURE_VERYLONG,
	EXPOSURE_FIXEDFPS,
	EXPOSURE_NIGHTWITHPREVIEW,
	EXPOSURE_ANTISHAKE,
	EXPOSURE_FIREWORKS,
	FILTER_NONE,
	FILTER_NOISE,
	FILTER_EMBOSS,
	FILTER_NEGATIVE,
	FILTER_SKETCH,
	FILTER_OILPAINT,
	FILTER_HATCH,
	FILTER_GPEN,
	FILTER_ANTIALIAS,
	FILTER_DERING,
	FILTER_SOLARISE,
	FILTER_WATERCOLOR,
	FILTER_PASTEL,
	FILTER_SHARPEN,
	FILTER_FILM,
	FILTER_BLUR,
	FILTER_SATURATION,
	FILTER_DEINTERLACELINEDOUBLE,
	FILTER_DEINTERLACEADVANCED,
	FILTER_COLOURSWAP,
	FILTER_WASHEDOUT,
	FILTER_COLOURPOINT,
	FILTER_POSTERIZE,
	FILTER_COLOURBALANCE,
	FILTER_CARTOON,
	FILTER_ANAGLYPH,
	FILTER_DEINTERLACEFAST,
	WBALANCE_OFF,
	WBALANCE_AUTO,
	WBALANCE_SUNLIGHT,
	WBALANCE_CLOUDY,
	WBALANCE_SHADE,
	WBALANCE_TUNGSTEN,
	WBALANCE_FLUORESCENT,
	WBALANCE_INCANDESCENT,
	WBALANCE_FLASH,
	WBALANCE_HORIZON,
	SYSTEM_CMD_EVENT_MODULE,
	SYSTEM_CMD_APP_UPDATE,
	SYSTEM_CMD_EMPTY,
	SYSTEM_CMD_ACTION_TRANSLATE_SRC,
	SYSTEM_CMD_ACTION_TRANSLATE_TEST1,
	SYSTEM_CMD_ACTION_TRANSLATE_TEST2,
	SYSTEM_CMD_SOUND_VOLUME_TEMP_MUTE,
	SYSTEM_CMD_SOUND_VOLUME_TEMP_UNMUTE,
	SYSTEM_CMD_MAX,	
	
	SYSTEM_KEY_RESERVED	 = 0x41001000,
	SYSTEM_KEY_ESC,
	SYSTEM_KEY__1,
	SYSTEM_KEY__2,
	SYSTEM_KEY__3,
	SYSTEM_KEY__4,
	SYSTEM_KEY__5,
	SYSTEM_KEY__6,
	SYSTEM_KEY__7,
	SYSTEM_KEY__8,
	SYSTEM_KEY__9,
	SYSTEM_KEY__0,
	SYSTEM_KEY_MINUS,
	SYSTEM_KEY_EQUAL,
	SYSTEM_KEY_BACKSPACE,
	SYSTEM_KEY_TAB,
	SYSTEM_KEY__Q,
	SYSTEM_KEY__W,
	SYSTEM_KEY__E,
	SYSTEM_KEY__R,
	SYSTEM_KEY__T,
	SYSTEM_KEY__Y,
	SYSTEM_KEY__U,
	SYSTEM_KEY__I,
	SYSTEM_KEY__O,
	SYSTEM_KEY__P,
	SYSTEM_KEY_LEFTBRACE,
	SYSTEM_KEY_RIGHTBRACE,
	SYSTEM_KEY_ENTER,
	SYSTEM_KEY_LEFTCTRL,
	SYSTEM_KEY__A,
	SYSTEM_KEY__S,
	SYSTEM_KEY__D,
	SYSTEM_KEY__F,
	SYSTEM_KEY__G,
	SYSTEM_KEY__H,
	SYSTEM_KEY__J,
	SYSTEM_KEY__K,
	SYSTEM_KEY__L,
	SYSTEM_KEY_SEMICOLON,
	SYSTEM_KEY_APOSTROPHE,
	SYSTEM_KEY_GRAVE,
	SYSTEM_KEY_LEFTSHIFT,
	SYSTEM_KEY_BACKSLASH,
	SYSTEM_KEY__Z,
	SYSTEM_KEY__X,
	SYSTEM_KEY__C,
	SYSTEM_KEY__V,
	SYSTEM_KEY__B,
	SYSTEM_KEY__N,
	SYSTEM_KEY__M,
	SYSTEM_KEY_COMMA,
	SYSTEM_KEY_DOT,
	SYSTEM_KEY_SLASH,
	SYSTEM_KEY_RIGHTSHIFT,
	SYSTEM_KEY_KPASTERISK,
	SYSTEM_KEY_LEFTALT,
	SYSTEM_KEY_SPACE,
	SYSTEM_KEY_CAPSLOCK,
	SYSTEM_KEY__F1,
	SYSTEM_KEY__F2,
	SYSTEM_KEY__F3,
	SYSTEM_KEY__F4,
	SYSTEM_KEY__F5,
	SYSTEM_KEY__F6,
	SYSTEM_KEY__F7,
	SYSTEM_KEY__F8,
	SYSTEM_KEY__F9,
	SYSTEM_KEY_F10,
	SYSTEM_KEY_NUMLOCK,
	SYSTEM_KEY_SCROLLLOCK,
	SYSTEM_KEY_KP7,
	SYSTEM_KEY_KP8,
	SYSTEM_KEY_KP9,
	SYSTEM_KEY_KPMINUS,
	SYSTEM_KEY_KP4,
	SYSTEM_KEY_KP5,
	SYSTEM_KEY_KP6,
	SYSTEM_KEY_KPPLUS,
	SYSTEM_KEY_KP1,
	SYSTEM_KEY_KP2,
	SYSTEM_KEY_KP3,
	SYSTEM_KEY_KP0,
	SYSTEM_KEY_KPDOT,
	SYSTEM_KEY_84,
	SYSTEM_KEY_ZENKAKUHANKAKU,
	SYSTEM_KEY_102ND,
	SYSTEM_KEY_F11,
	SYSTEM_KEY_F12,
	SYSTEM_KEY_RO,
	SYSTEM_KEY_KATAKANA,
	SYSTEM_KEY_HIRAGANA,
	SYSTEM_KEY_HENKAN,
	SYSTEM_KEY_KATAKANAHIRAGANA,
	SYSTEM_KEY_MUHENKAN,
	SYSTEM_KEY_KPJPCOMMA,
	SYSTEM_KEY_KPENTER,
	SYSTEM_KEY_RIGHTCTRL,
	SYSTEM_KEY_KPSLASH,
	SYSTEM_KEY_SYSRQ,
	SYSTEM_KEY_RIGHTALT,
	SYSTEM_KEY_LINEFEED,
	SYSTEM_KEY_HOME,
	SYSTEM_KEY_UP,
	SYSTEM_KEY_PAGEUP,
	SYSTEM_KEY__LEFT,
	SYSTEM_KEY__RIGHT,
	SYSTEM_KEY_END,
	SYSTEM_KEY_DOWN,
	SYSTEM_KEY_PAGEDOWN,
	SYSTEM_KEY_INSERT,
	SYSTEM_KEY_DELETE,
	SYSTEM_KEY_MACRO,
	SYSTEM_KEY_MUTE,
	SYSTEM_KEY_VOLUMEDOWN,
	SYSTEM_KEY_VOLUMEUP,
	SYSTEM_KEY_POWER,
	SYSTEM_KEY_KPEQUAL,
	SYSTEM_KEY_KPPLUSMINUS,
	SYSTEM_KEY_PAUSE,
	SYSTEM_KEY_SCALE,

	SYSTEM_KEY_KPCOMMA,
	SYSTEM_KEY_HANGEUL,
	SYSTEM_KEY_HANJA,
	SYSTEM_KEY_YEN,
	SYSTEM_KEY_LEFTMETA,
	SYSTEM_KEY_RIGHTMETA,
	SYSTEM_KEY_COMPOSE,

	SYSTEM_KEY_STOP,
	SYSTEM_KEY_AGAIN,
	SYSTEM_KEY_PROPS,
	SYSTEM_KEY_UNDO,
	SYSTEM_KEY_FRONT,
	SYSTEM_KEY_COPY,
	SYSTEM_KEY_OPEN,
	SYSTEM_KEY_PASTE,
	SYSTEM_KEY_FIND,
	SYSTEM_KEY_CUT,
	SYSTEM_KEY_HELP,
	SYSTEM_KEY_MENU,
	SYSTEM_KEY_CALC,
	SYSTEM_KEY_SETUP,
	SYSTEM_KEY_SLEEP,
	SYSTEM_KEY_WAKEUP,
	SYSTEM_KEY_FILE,
	SYSTEM_KEY_SENDFILE,
	SYSTEM_KEY_DELETEFILE,
	SYSTEM_KEY_XFER,
	SYSTEM_KEY_PROG1,
	SYSTEM_KEY_PROG2,
	SYSTEM_KEY_WWW,
	SYSTEM_KEY_MSDOS,
	SYSTEM_KEY_SCREENLOCK,
	SYSTEM_KEY_ROTATE_DISPLAY,
	SYSTEM_KEY_CYCLEWINDOWS,
	SYSTEM_KEY_MAIL,
	SYSTEM_KEY_BOOKMARKS,
	SYSTEM_KEY_COMPUTER,
	SYSTEM_KEY_BACK,
	SYSTEM_KEY_FORWARD,
	SYSTEM_KEY_CLOSECD,
	SYSTEM_KEY_EJECTCD,
	SYSTEM_KEY_EJECTCLOSECD,
	SYSTEM_KEY_NEXTSONG,
	SYSTEM_KEY_PLAYPAUSE,
	SYSTEM_KEY_PREVIOUSSONG,
	SYSTEM_KEY_STOPCD,
	SYSTEM_KEY_RECORD,
	SYSTEM_KEY_REWIND,
	SYSTEM_KEY_PHONE,
	SYSTEM_KEY_ISO,
	SYSTEM_KEY_CONFIG,
	SYSTEM_KEY_HOMEPAGE,
	SYSTEM_KEY_REFRESH,
	SYSTEM_KEY_EXIT,
	SYSTEM_KEY_MOVE,
	SYSTEM_KEY_EDIT,
	SYSTEM_KEY_SCROLLUP,
	SYSTEM_KEY_SCROLLDOWN,
	SYSTEM_KEY_KPLEFTPAREN,
	SYSTEM_KEY_KPRIGHTPAREN,
	SYSTEM_KEY_NEW,
	SYSTEM_KEY_REDO,

	SYSTEM_KEY_F13,
	SYSTEM_KEY_F14,
	SYSTEM_KEY_F15,
	SYSTEM_KEY_F16,
	SYSTEM_KEY_F17,
	SYSTEM_KEY_F18,
	SYSTEM_KEY_F19,
	SYSTEM_KEY_F20,
	SYSTEM_KEY_F21,
	SYSTEM_KEY_F22,
	SYSTEM_KEY_F23,
	SYSTEM_KEY_F24,

	SYSTEM_KEY_PLAYCD		 = 0x400010C8,
	SYSTEM_KEY_PAUSECD,
	SYSTEM_KEY_PROG3,
	SYSTEM_KEY_PROG4,
	SYSTEM_KEY_ALL_APPLICATIONS,
	SYSTEM_KEY_SUSPEND,
	SYSTEM_KEY_CLOSE,
	SYSTEM_KEY_PLAY,
	SYSTEM_KEY_FASTFORWARD,
	SYSTEM_KEY_BASSBOOST,
	SYSTEM_KEY_PRINT,
	SYSTEM_KEY_HP,
	SYSTEM_KEY_CAMERA,
	SYSTEM_KEY_SOUND,
	SYSTEM_KEY_QUESTION,
	SYSTEM_KEY_EMAIL,
	SYSTEM_KEY_CHAT,
	SYSTEM_KEY_SEARCH,
	SYSTEM_KEY_CONNECT,
	SYSTEM_KEY_FINANCE,
	SYSTEM_KEY_SPORT,
	SYSTEM_KEY_SHOP,
	SYSTEM_KEY_ALTERASE,
	SYSTEM_KEY_CANCEL,
	SYSTEM_KEY_BRIGHTNESSDOWN,
	SYSTEM_KEY_BRIGHTNESSUP,
	SYSTEM_KEY_MEDIA,
	SYSTEM_KEY_SWITCHVIDEOMODE,
	SYSTEM_KEY_KBDILLUMTOGGLE,
	SYSTEM_KEY_KBDILLUMDOWN,
	SYSTEM_KEY_KBDILLUMUP,
	SYSTEM_KEY_SEND,
	SYSTEM_KEY_REPLY,
	SYSTEM_KEY_FORWARDMAIL,
	SYSTEM_KEY_SAVE,
	SYSTEM_KEY_DOCUMENTS,
	SYSTEM_KEY_BATTERY,
	SYSTEM_KEY_BLUETOOTH,
	SYSTEM_KEY_WLAN,
	SYSTEM_KEY_UWB,
	SYSTEM_KEY_UNKNOWN,
	SYSTEM_KEY_VIDEO_NEXT,
	SYSTEM_KEY_VIDEO_PREV,
	SYSTEM_KEY_BRIGHTNESS_CYCLE,
	SYSTEM_KEY_BRIGHTNESS_AUTO,
	SYSTEM_KEY_DISPLAY_OFF,
	SYSTEM_KEY_WWAN,
	SYSTEM_KEY_RFKILL,
	SYSTEM_KEY_MICMUTE,
	
	SYSTEM_KEY_MAX
};

enum WIDGET_TYPE
{ 
	WIDGET_TYPE_UNKNOWN,
	WIDGET_TYPE_SENSOR_MIN,
	WIDGET_TYPE_SENSOR_IMAGE,
	WIDGET_TYPE_SENSOR_TACHOMETER,
	WIDGET_TYPE_SENSOR_TACHO_SCALE,
	WIDGET_TYPE_SENSOR_TEMPMETER,
	WIDGET_TYPE_SENSOR_WHITETACHO,
	WIDGET_TYPE_SENSOR_BLACKTACHO,
	WIDGET_TYPE_SENSOR_BLACKREGULATOR,
	WIDGET_TYPE_SENSOR_CIRCLETACHO,
	WIDGET_TYPE_SENSOR_DARKMETER,
	WIDGET_TYPE_SENSOR_DARKTERMOMETER,
	WIDGET_TYPE_SENSOR_GREENTACHO,
	WIDGET_TYPE_SENSOR_OFFTACHO,
	WIDGET_TYPE_SENSOR_SILVERREGULATOR,
	WIDGET_TYPE_SENSOR_WHITETERMOMETER,
	WIDGET_TYPE_SENSOR_MAX,
	WIDGET_TYPE_IMAGE,
	WIDGET_TYPE_WEATHER,
	WIDGET_TYPE_CLOCK_MIN,
	WIDGET_TYPE_CLOCK_EL,
	WIDGET_TYPE_CLOCK_MECH,
	WIDGET_TYPE_CLOCK_WHITE,
	WIDGET_TYPE_CLOCK_BROWN,
	WIDGET_TYPE_CLOCK_QUARTZ,
	WIDGET_TYPE_CLOCK_SKYBLUE,
	WIDGET_TYPE_CLOCK_ARROW,
	WIDGET_TYPE_CLOCK_MAX,	
	WIDGET_TYPE_MAX
};

enum WIDGET_DIRECT
{ 
	WIDGET_DIRECT_UNKNOWN,
	WIDGET_DIRECT_BORDER_CW,
	WIDGET_DIRECT_BORDER_CCW,
	WIDGET_DIRECT_BORDER_PINGPONG,
	WIDGET_DIRECT_STATIC,
	WIDGET_DIRECT_MAX
};

enum DIRECTORY_TYPE
{ 
	DIRECTORY_TYPE_MEDIA,
	DIRECTORY_TYPE_NORM,
	DIRECTORY_TYPE_SLOW,
	DIRECTORY_TYPE_DIFF,
	DIRECTORY_TYPE_AUDIO,
	DIRECTORY_TYPE_STATUSES,
	DIRECTORY_TYPE_EVENTS,
	DIRECTORY_TYPE_ACTIONS,
	DIRECTORY_TYPE_OTHER,
	DIRECTORY_TYPE_MAX
};

typedef struct
{
	char					Enabled;
	unsigned char			Days;
	unsigned int			Time;
	unsigned int			Skip;
	unsigned int			Limit;
	char					Path[MAX_FILE_LEN];
	unsigned int			BeginVolume;
	unsigned int			TimeSetVolume;
	unsigned int			TimerAlarmPause;
} ALARM_CLOCK_TYPE;

typedef struct
{
	unsigned int			ID;
	int						Status;
	GLuint 					Texture;
	void					*Image;
	GLfloat					PosW;
	GLfloat					PosH;
} CAMERA_LIST_INFO;

typedef struct
{
	char					Name[MAX_FILE_LEN];
	char					ShowName[MAX_FILE_LEN];
	int						Flag;
	unsigned int			Sort;
} LIST_FILES_TYPE;

typedef struct
{
	unsigned int			ID;
	char					Enabled;
	unsigned int			Group;
	char					Detected;
	unsigned int			DetectSize;
	char					ColorDetected;
	unsigned int			ColorDetectSize;
	char					LongDetected;
	unsigned int			LongDetectSize;
	char					LongColorDetected;
	unsigned int			LongColorDetectSize;
	unsigned int			X1;
	unsigned int			Y1;
	unsigned int			X2;
	unsigned int			Y2;
	unsigned int			PX1;
	unsigned int			PY1;
	unsigned int			PX2;
	unsigned int			PY2;
	unsigned int			BX1;
	unsigned int			BY1;
	unsigned int			BX2;
	unsigned int			BY2;
	char					Name[64];
} RECT_INFO;

typedef struct
{
	char			Name[64];
	double			Frequency;
} RADIO_INFO;

typedef struct
{
	unsigned int			Group;
	char					Address[64];
	char					Address2[64];
	char					MainText[64];
	char					BodyText[64];
	char					FilePath[64];
	char					FilePath2[64];
	char					*ExtraText;
	unsigned int			ExtraTextSize;
} MAIL_INFO;

typedef struct 
{
    int tm_sec;    /* Seconds (0-60) */
    int tm_min;    /* Minutes (0-59) */
    int tm_hour;   /* Hours (0-23) */
    int tm_mday;   /* Day of the month (1-31) */
    int tm_mon;    /* Month (0-11) */
    int tm_year;   /* Year - 1900 */
    int tm_wday;   /* Day of the week (0-6, Sunday = 0) */
    int tm_yday;   /* Day in the year (0-365, 1 Jan = 0) */
    int tm_isdst;  /* Daylight saving time */
} TIME_INFO;

typedef struct
{
	GLfloat		X;
	GLfloat		Y;
} GLpoint2D;

typedef struct
{
	GLpoint2D	Point[4];
} GLmodel2D;

typedef struct
{
	GLpoint2D	Point[4];
} GLtexturemap;

typedef struct
{
   unsigned int			ID;
   unsigned int			Data;
} SYS_MESSAGE_INFO;

typedef struct
{
   unsigned int			ID;
   char					Zero;
   char					Name[64];
   struct sockaddr_in 	Address;
   unsigned int			Type;
   char 				Active;
   char					Local;
   char					Version[64];
   int					IntVersion[4];
} SYSTEM_INFO;

typedef struct
{
   char				Name[64];
   char				Name2[MAX_FILE_LEN];
   char				Show;
   unsigned int		Access;
   int				PrevPage;   
   char				PrevPageNoHistory;
   int				NextPage;
   unsigned int		ViewLevel;
   int				MiscData;
   int 				(*ActionFunc)(void*, int);
} MENU_OPTION;

typedef struct
{
   char			Name[64];
   char			Path[MAX_FILE_LEN];
   int 			SelectedOption;
   int 			CountOptions;
   char			ShowInfo;   
   int 			(*OpenFunc)(void*, int);   
   int 			(*RefreshFunc)(void*, int);   
   MENU_OPTION 	*Options;
} MENU_PAGE;

typedef struct
{
	unsigned int ID;
	unsigned int Freq;
	unsigned int Channels;
	unsigned int CodecNum;
	unsigned int BitRate;
} MIC_LIST;

typedef struct  
{
	char* data;
	char* data2;
	void* void_data;
	void* void_data2;
	void* void_data3;
	void* void_data4;
	void* void_data5;
	unsigned int data_size; 
	int clock;
	int dsize;
	int frsize;
	unsigned int flag;
	unsigned int uidata[20];
	int64_t pts;
} misc_buffer;

typedef struct  
{
	AVPacket		*AVBuffer;	
	char 			*DataBuffer;
	unsigned int 	*DataSize; 
	//unsigned int 	*TimeStamp;
	//unsigned int 	BufferMaxSize; 
	unsigned int 	BufferSize; 
	unsigned int 	FullDataSize;
	unsigned int 	DataCnt;
	unsigned int 	DataMaxCnt; 
	//unsigned int 	Status;
	int 			PlayBlockNum;
	int 			LoadBlockNum;
	//unsigned int 	Flag; 
} array_buffer;

typedef struct  
{
	unsigned int 	ID;
	unsigned int 	Len;
	uint16_t 		Code[MAX_IRCOMMAND_LEN];
} IR_COMMAND_TYPE;

typedef struct
{
	unsigned int 	ID;
	void*			Data;
	unsigned int 	Len;
	char			Path[256];
} SYS_SOUND_TYPE;

typedef struct 
{
	pthread_mutex_t mutex;
	unsigned int Num;
	TX_EVENTER run;
	TX_EVENTER pevntS;
	TX_EVENTER pevntV;
	TX_EVENTER pevntA;
	unsigned int Type;
	unsigned int VidID;
	unsigned int AudID;
	pthread_mutex_t CopyMutex;
	TX_EVENTER CopyEvent;
	unsigned int CopyStatus;
	unsigned int CopyCurOrder;
	unsigned int CopyNextOrder;
	char *CurrentPath;
} STREAM_INFO;

typedef struct  
{
	unsigned int SizeLimit; 
	unsigned int TimeSize; 
	unsigned char OrderLmt;
	unsigned int BackUpEnabled;
	char *FilePath;
	char *BackUpFile;
	char *ErrorFile;
	file_struct *FileData; 
	FILE *filehandle;
	STREAM_INFO *Stream;
} capture_settings;

typedef struct
{ 
   MENU_OPTION 	*Options;
} TEST1;

typedef struct
{ 
   int 			(*RefreshFunc)(int); 
} TEST2;

typedef struct
{
   char 	Red;
   char 	Green;
   char 	Blue;
} RGB_T;

typedef struct
{
   char 	Red;
   char 	Green;
   char 	Blue;
   char		Alpha;
} RGBA_T;	

typedef struct
{
   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   
   uint32_t screen_width;
   uint32_t screen_height;
   uint32_t menu_width;
   uint32_t menu_height;
   unsigned int menu_terminal_id;
   int menu_terminal_enabled;
   struct sockaddr_in menu_terminal_addr;
   uint32_t menu_render_count;   
   uint32_t menu_skip_timer;   
   int32_t screen_width_center;
   int32_t screen_height_center;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
   GLuint tex[6];
   GLfloat fScreenVertCoords[3*4];
   GLfloat fScreenTextCoords[2*4];
   GLfloat fAlphaVertCoords[3*4*256];
   GLfloat fAlphaTextCoords[2*4*256];
   GLfloat fGraphVertCoords[3*4];
   GLfloat fGraphTextCoords[2*4];
   char*	tex_screen;
   RGBA_T*	tex_main; 
   uint32_t tex_main_size;
   RGBA_T*	tex_alpha;   
   RGBA_T*	tex_graph;   
   char*	tex_buf2;
   char		tex_buf3[4];
} GL_STATE_T;

typedef struct Sensor_Params
{
	unsigned char Enabled;	
	unsigned char Tested;
	unsigned char Initialized;
	unsigned char PwrInit;
	unsigned char ID;
	unsigned char Number;
	unsigned char Port;
	unsigned char CurrentType;
	unsigned char Pull;
	unsigned char Mode;
	unsigned char Inverted;
	unsigned char ImpulseLen;
	unsigned char OpenDrain;
	unsigned char PWM;
	unsigned char PWMValue;	
	unsigned char UartPortNum;
	unsigned char Analog;
	unsigned char IORControlUse;
	unsigned char IORControlPort;
	unsigned char IOWControlUse;
	unsigned char IOWControlPort;	
	unsigned char IOModeControl;	
	unsigned char Started;
	unsigned int  Accuracy;
	unsigned int  SupportTypes;
	unsigned int  DefaultValue;
	unsigned int  Interval;
	unsigned int  LastTick;
	unsigned int  ImpulseTick;
	unsigned int  PortSpeedCode;
	unsigned int  ResultType;
	unsigned int  TimeSkip;
	unsigned int  Address;
	unsigned int  Gain;
} Sensor_Params;

typedef struct
{
	unsigned int	Enabled;
	unsigned int	ParentID;
	unsigned int	ID;
	unsigned int	NewID;
	unsigned char	Zero;	//show id as txt
	unsigned char	Type;
	unsigned char	SubType;
	char			Name[64];
	unsigned int	ViewLevel;
	unsigned int	AccessLevel;
	unsigned int	Global;
	unsigned int	ScanSet;	
	unsigned int	SaveChanges;
	unsigned int	GenEvents;
	unsigned int	Settings[MAX_MODULE_SETTINGS];
	unsigned int	SubModule;
	int				Status[MAX_MODULE_STATUSES];
	unsigned int	ParamsCount;
	unsigned int	SettingsCount;
	unsigned int	SettingSelected;
	unsigned int	Sort;
	struct sockaddr_in  Address;
	char			Local;
	int				InitParams[1];
	unsigned int	Version[4];
	Sensor_Params	*ParamList;
	Sensor_Params	*SettingList;
	int				*IO_flag;
	pthread_mutex_t *IO_mutex;
	int				*IO_data;	
	int				*IO_size_data;
} MODULE_INFO;


typedef struct
{
	unsigned int	Enabled;
	unsigned int	ParentID;
	unsigned int	ID;
	unsigned int	NewID;
	unsigned char	Zero;	//show id as txt
	unsigned char	Type;
	unsigned char	SubType;
	char			Name[64];
	unsigned int	ViewLevel;
	unsigned int	AccessLevel;
	unsigned int	Global;
	unsigned int	ScanSet;	
	unsigned int	SaveChanges;
	unsigned int	GenEvents;
	unsigned int	Settings[MAX_MODULE_SETTINGS];
	unsigned int	SubModule;
	int				Status[MAX_MODULE_STATUSES];
	unsigned int	ParamsCount;
	unsigned int	SettingsCount;
	unsigned int	SettingSelected;
	unsigned int	Sort;
	struct sockaddr_in  Address;
	char			Local;
	int				InitParams[1];
	unsigned int	Version[4];
} MODULE_INFO_TRANSFER;

typedef struct
{
	unsigned int	WidgetID;
	char			Enabled;	
	unsigned int	ShowMode;
	unsigned char	Type;
	char			Name[64];
	unsigned int	ModuleID;
	unsigned int	ModuleID2;
	char			Path[256];
	double			Scale;
	unsigned int	Direct;
	double			DirectX;
	double			DirectY;
	double			Angle;
	double			OffsetX;
	double			OffsetY;
	double			Speed;
	unsigned int	Settings[2];
	unsigned int	Timer;
	unsigned int	ShowTimer;
	unsigned int	ShowDirect;
	unsigned int	ShowRepeat;
	unsigned int	ShowTime;
	int				Status;
	unsigned int	Refresh;
	unsigned char	WeekDays;
	unsigned int	NotBeforeTime;
	unsigned int	NotAfterTime;
	unsigned int	NotBeforeTime2;
	unsigned int	NotAfterTime2;
	unsigned int	NotBeforeTime3;
	unsigned int	NotAfterTime3;
	char			InTimeLimit;
	char			SensorValueStr[64];
	double 			SensorValue;
	int 			SensorValue2;
	double 			SensorPrevValue;
	int				OffsetValue;
	//void*			vData;
	double			PosX;
	double			PosY;
	double			SizeX;
	double			SizeY;
	double			MaxSizeX;
	double			MaxSizeY;
	double			Width;
	double			Height;
	double			DefaultScale;
	double			IconScale;	
	double			IconDefaultWidth;
	double			IconDefaultHeight;
	double			IconSourceWidth;
	double			IconSourceHeight;	
	double			IconRenderWidth;
	double			IconRenderHeight;
	GLmodel2D		IconModel;
	GLtexturemap	IconTextureMap;	
	unsigned int	IconTexture;
	int				AnimateMode;
	double			ShowValueFrom;
	double			ShowValueTo;
	int				ShowValue2From;
	int				ShowValue2To;
	//double			ShowValueStr;
	int				ShowValue2Str;
	int				ShowWithCamera;
	unsigned int	SelfPath;
	unsigned int	SourceCell;
	unsigned int	SourceCell2;
	char			Color[10];
	RGB_T			RGBColor;
	char			ValueTypeName[10];
	double			Coefficient;			
} WIDGET_INFO;

typedef struct
{
	unsigned int	Access;
	unsigned int	Type;
	char			Name[64];
	char			URL[256];	
	char			OverTCP;
} STREAM_SETT;

typedef struct
{
	char			Name[64];
} STREAM_TYPE;

typedef struct
{
	unsigned int	ActionID;
	int				Enabled;
	unsigned int	SrcID;
	unsigned int	SrcSubNumber;
	int				SrcStatus;
	unsigned int	DestID;
	unsigned int	DestSubNumber;
	int				DestStatus;
	char			TestType;
	char			Test2Type;
	unsigned int	TestModuleID;
	unsigned int	TestModuleSubNumber;
	int				TestModuleStatus;
	char			Test3Type;
	unsigned int	Test3ModuleID;
	unsigned int	Test3ModuleSubNumber;
	int				Test3ModuleStatus;
	unsigned int	AfterAccept;
	char			Name[128];
	char			GroupName[128];
	unsigned char	WeekDays;
	unsigned int	NotBeforeTime;
	unsigned int	NotAfterTime;
	unsigned int	TimerWaitBefore;
	unsigned int	TimerWaitBeforeStatus;
	unsigned int	TimerWaitBeforeDone;
	unsigned int	TimerOff;
	unsigned int	TimerOffStatus;
	unsigned int	RestartTimer;
	unsigned int	CounterExec;
} ACTION_INFO;

typedef struct
{
	char			Name[128];
} GROUP_TYPE;

typedef struct
{
	unsigned int	Access;
	unsigned int	ID;
	unsigned int	SubNumber;
	unsigned int	Status;
	char			Name[128];
} ACTION_MANUAL_INFO;

typedef struct
{
	int				Enabled;	
	char			Login[64];
	char			Password[64];
	unsigned int	Access;	
	unsigned int	Level;	
} USER_INFO;

typedef struct
{
	int				Type;
	int				Size;
	float			PosX;
	float			PosY;
	float			ScaleX;
	float			ScaleY;
	char			Text[64];
} DISPLAY_CONTENT_INFO;

typedef struct
{
	char				New;
	unsigned int		ID;
	unsigned int		SubNumber;
	int					Status;
	char				Name[64];
	char				Date[64];
	char				ExecNow;
} MODULE_EVENT;

typedef struct
{
	unsigned int		Access;
	unsigned int		RemoteAccess;
	char				Name[64];
	char				Path[MAX_FILE_LEN];
	int					Sort;
	unsigned int		Type;
	unsigned int 		MinSpace;
	char				BeginPath[MAX_FILE_LEN];
	unsigned int		MaxFileSize;
	unsigned int		MaxFileTime;
	unsigned int		BackUpMode;	
	char				BackUpPath[MAX_FILE_LEN];
	unsigned int		BackUpMaxOrderLen;
	unsigned int		BackUpUseCopyService;
	char				BackUpIPCopyService[32];
	unsigned int		BackUpMaxWaitCancel;
	unsigned int		BackUpMaxWaitMess;
	unsigned int		ArchiveMode;
	unsigned int		ArchiveSelectType;
	char				ArchivePath[MAX_FILE_LEN];
	unsigned int		ArchiveTimeFrom;
	unsigned int		ArchiveTimeTo;
	unsigned int		ArchiveMaxOrderLen;
	unsigned int		ArchiveUseCopyService;
	char				ArchiveIPCopyService[32];
	unsigned int		ArchiveMaxWaitCancel;
	unsigned int		ArchiveMaxWaitMess;
	char				BackUpStorage;
	char				ArchiveStorage;
	
} DIRECTORY_INFO;

typedef struct AudioCodecInfo 
{
	int CodecInfoFilled;
	int audio_sample_rate; // e.g. 22050
	int audio_bit_rate;    // e.g. 128000
	int audio_channels;    // e.g. 1
	int audio_profile;     // e.g. FF_PROFILE_AAC_LOW
	int audio_codec; 		// e.g. AV_CODEC_ID_AAC
	int audio_frame_size; 
	int audio_raw_buffer_size;		
	int audio_frame_rate;
	int FrameNum;
	unsigned int nb_samples;
	char* empty_frame;
	int size_empty_frame;
} AudioCodecInfo;

typedef struct VideoCodecInfo 
{
	int Filled;
	int video_codec;
	int video_bit_rate;
	int video_width;
	int video_height;
	int video_width32;
	int video_height16;
	int video_frame_size;
	int video_frame_rate;
	int video_intra_frame; 
	int video_pixel_format;
	int FrameNum;
} VideoCodecInfo;

typedef struct
{
	unsigned int	ParentID;
	unsigned int	ID;
	unsigned char	Enabled;
	unsigned int	Type;
	unsigned char	Serial[MAX_SECURITY_SERIAL_LEN];
	unsigned int	SerialLength;
	char			Name[64];
	unsigned char	Sector;
	unsigned char	VerifyKeyA;
	unsigned char	KeyA[6];
	unsigned char	VerifyKeyB;
	unsigned char	KeyB[6];
	unsigned char	Local;	
	//unsigned char	EventEnabled;	
} SECURITY_KEY_INFO;

typedef struct
{
	char			Used;
	char			Name[64];
	float			PanTiltX;
	float			PanTiltY;
	float			Zoom;
	float			Focus;
} PTZ_SET_INFO;

typedef struct func_link 
{
	char *Name;	
	char *data;	
	AudioCodecInfo codec_info; 
	char errorcode;
	//int (*FuncRecvPts)(char*, int, unsigned int*, int*, int*, void*, char, int, unsigned int*);
	int (*FuncRecv)(void*, unsigned int*, int*, void*, char, unsigned int, unsigned int);
	int (*FuncSend)(unsigned int, unsigned int, char*, int, void*, int);
	int (*FuncSave)(unsigned int, char*, int, void*, void*);
	int (*FuncRTSP)(unsigned int, char*, unsigned int);
	void *Handle;
	void *pModule;
	void *pSubModule;
	struct sockaddr_in RecvAddress;
	unsigned int ConnectNum;
	unsigned int ConnectID;
	unsigned int DeviceNum;
	misc_buffer mbuffer;
	struct sockaddr_in Address;	
} func_link;

typedef struct
{
	unsigned int	ID;
	unsigned char	Zero;
} ID_TXT;

typedef struct
{
	int				ID[2];
	unsigned int	Count;
	unsigned int	MinFree;
	char			**Paths;
} FS_GROUP;

void to_freeze();
int FillConfigPath(char *cPath, unsigned int uiLen, char *cFile, char cCreate);
int load_file_in_buff(char *filename, misc_buffer * buffer);
char *GetModuleStatusName(unsigned int uiType, unsigned int uiStatusNum, char*OutBuff, unsigned int OutSize, char cLang);
char *GetModuleStatusValue(unsigned int uiType, unsigned int uiStatusNum, int iStatus, char*OutBuff, unsigned int OutSize);
char *GetModuleStatusValueType(unsigned int uiType, unsigned int uiStatusNum, char*OutBuff, unsigned int OutSize);
char GetModuleStatusEn(unsigned int uiType, unsigned int uiStatusNum);
char GetModuleActionEn(unsigned int uiType, unsigned int uiStatusNum);
char *GetModuleActionName(unsigned int uiType, unsigned int uiStatusNum, char*OutBuff, unsigned int OutSize, char cLang);
int RequestModulesSecurity(char cType);
//int ClearMessageList(int iNum);
int SearchStrInData(char *cData, int iDataLen, int iPos, char *cStr);
int ClearSecurityKey(unsigned int uiID);
int ClearModuleList(unsigned int uiID);
int ClearSystemList(unsigned int uiID);
void UpdateModuleStatus(MODULE_INFO *pModule, char cAll);
void SortModules(MODULE_INFO *pModuleList, int iModCnt);
unsigned int GetLocalSysID();
SYSTEM_INFO* GetLocalSysInfo();
SYSTEM_INFO* GetSysInfo(unsigned int uiID);
MODULE_INFO* ModuleIdToPoint(int iID, char cLocal);
int ModuleIdToNum(unsigned int iID, char cLocal);
int ModuleTypeToNum(int iType, char cLocal);
MODULE_INFO* ModuleTypeToPoint(int iType, char cLocal);
int ModuleSortToNum(unsigned int iSort);
int ModuleAction(unsigned int iModuleID, int iSubModuleNum, unsigned int iActionCode, char *cActionName);
int SetModuleStatus(unsigned int iID, unsigned int iSubData, unsigned int iStatus);
int ReqModuleStatus(unsigned int iID);
int GetModuleStatus(unsigned int iID, unsigned int uiSubNumber);
unsigned int GetModuleSettings(char *Buff, int iLen, char iInt);
char* GetActionCodeName(int iCode, char* cBuff, int iBufflen, int iMode);
int PlaySound(unsigned int iNum);
int Action_PlaySound(unsigned int iNum, int iRepeats, int iVolumePercent);
int StreamOn(int iNum);
int CopyFile(char *cFrom, char *cTo, char *cToError, char cWait, char cRemoveAfter, STREAM_INFO *pStream, unsigned char ucOrderLmt, char* cAddrOrderer, 
			unsigned int uiMaxWaitTime, unsigned int uiMessWaitTime, unsigned int uiDestType);
char debug_fprintf(char *Buff, ...);
void AnimateWidget(GL_STATE_T *state, WIDGET_INFO *wiWidget);
void RenderImage(GL_STATE_T *pstate, WIDGET_INFO *wiWidget);
int UpdateWidgets(GL_STATE_T *pstate, int iRecalcForce);
int Menu_ResetShower(void *pData, int iNum);
int PlayYouTube(char * cUrl, char * cFullUrl);
void add_sys_cmd_in_list(unsigned int uiID, unsigned int uiVal);
int utf8_to_cp866(char *cSourceStr, unsigned int uiSourceLen, char *cDestStr, unsigned int *uiDestLen);
int PlayURL(char* cUrl, int iNum);
int GetFileType(char *FileName);
int load_limit_file_in_buff(char *filename, misc_buffer * buffer, unsigned int uiLen);
unsigned int GenRandomInt(unsigned int cmask);
int WEB_get_module_info(char* cStr, int iLen);
int Menu_ClearMessageList(void *pData, int iNum);
int SearchDataInBuffer(char *cBuffer, unsigned int iBufferLen, unsigned int iPos, char *cData, unsigned int uiDataLen);
int SearchStrInDataCaseIgn(char *Data, int DataLen, int Pos, char *Str);
int CompareStrCaseIgn(char *Data, char *Str);
int GetParamSetting(unsigned int uiNum, char cParamKey, char *cBuffIn, unsigned int uiBuffInSize, char *cBuffOut, unsigned int uiBuffOutSize);
int SaveAlarms();
int SaveModules();
int SaveSounds();
int SaveMails();
int SaveStreamTypes();
int SaveStreams();
int SaveWidgets();
int SaveDirectories();
int SaveCamRectangles();
int SaveMnlActions();
int SaveEvntActions();
int SaveKeys();
int SaveIrCodes();
int SaveUsers();
int SaveRadios();
int SaveSettings();
int GetFirstMicDevNum();
char* GetModuleTypeName(int iCode);
char* GetModuleSubTypeName(int iCode);
unsigned int GetModuleType(char *Buff, int iLen);
void GetDateTimeStr(char *buff, int iLen);
int TestSettings(int iMode);
int TestModules(int iMode);
int TestAlarms(int iMode);
int TestSounds(int iMode);
int TestMails(int iMode);
int TestStreamTypes(int iMode);
int TestStreams(int iMode);
int TestWidgets(int iMode);
int TestCamRectangles(int iMode);
int TestIrCodes(int iMode);
int TestKeys(int iMode);
int TestEvntActions(int iMode);
int TestMnlActions(int iMode);
int TestDirectories(int iMode);
int TestUsers(int iMode);
int TestRadios(int iMode);
int PlayYouTubeFromURL(char *cURL);
int PlayYouTubeFromURL2(char *cURL, int iType);
int UpdateListFiles(char *cPath, int iMode);
void ShowNewMessage(char *cText);
void SetChangeShowNow(char val);
void AddMessageInList(char *cMessage, int iMessageLen, unsigned int cAddr);
void AddModuleEventInList(unsigned int uiID, int iNumSens, int iStatus, char* cName, unsigned int uiNameLen, char cExecNow);
void AddModuleEvents(MODULE_EVENT *meList, unsigned int uiCount);
void FillModuleEventList(MODULE_EVENT **meList, unsigned int *uiCount, unsigned int uiID, int iNumSens, int iStatus, char* cName, unsigned int uiNameLen, char cExecNow);
void AddSkipIrCodeInList(uint16_t *pCode, unsigned int uiLen);
void AddAlienKeyInList(unsigned char *Serial, unsigned int Len);
void AddSkipEventInList(unsigned int uiID, unsigned int SubNumber, unsigned int Status);
void ClearSkipEventList();
void ClearAlienKeyList();
void ClearSkipIrCodeList();
char* GetCurrDateTimeStr(char* cBuff, int iLen);
int System_Update();
int System_Shutdown(void *pData, int iNum);
int System_Reboot(void *pData, int iNum);
int System_ResetDefault(void *pData, int iNum);
int System_Close(void *pData, int iNum);
int System_Exit(void *pData, int iNum);
int System_Restart(void *pData, int iNum);
int Exec_In_Buff(char *cPath, char *cBuff, unsigned int uiLen);
int SetModuleDefaultSettings(MODULE_INFO* miModule);
unsigned int get_mem_cpu_mb();
unsigned int get_mem_gpu_mb();
unsigned int get_used_mem_gpu_mb();
void DisplayContentListClear();
void DisplayContentListAdd(DISPLAY_CONTENT_INFO *pDyspContNew);
void DisplayContentListSwitch();
void SortFiles(LIST_FILES_TYPE *pFileList, int iFileCnt);
int UpdateSecurityKey(int iAction, MODULE_INFO *pModule, SECURITY_KEY_INFO *skiUpdateInfo, uint8_t uiSensnum);
int SetPtzSettingInList(char *cName, float fX, float fY, float fZoom, float fFocus);
int AddPtzSettingInList(char *cName, float fX, float fY, float fZoom, float fFocus);
int DelPtzSettingInList(char *cName);
void ClearPtzSettingsList();
int MusicVolumeSet(int iNum);
DIRECTORY_INFO * GetDirectoryInfoPoint(DIRECTORY_INFO*sourceList, unsigned int listSize, unsigned int type, char storage);
int a_stime(time_t *t);

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
pthread_mutex_t ircode_mutex;
pthread_mutex_t evntaction_mutex;
pthread_mutex_t mnlaction_mutex;
pthread_mutex_t alienkey_mutex;
pthread_mutex_t ptz_mutex;
pthread_mutex_t skipevent_mutex;
pthread_mutex_t skipircode_mutex;
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

int iSkipIrCodeMaxCnt;
int iSkipIrCodeListCnt;
IR_COMMAND_TYPE *cSkipIrCodeList;

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

unsigned int iIRCommandCnt;
IR_COMMAND_TYPE *mIRCommandList;

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
char cIrCodeFile[256];
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

#endif