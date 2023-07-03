
#ifndef FLV_MUXER_H_
#define FLV_MUXER_H_

#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "audio.h"

//#define EVENT_STOP			1
//#define EVENT_WRITE			2
//#define EVENT_WRITE_DONE	3

enum {
    FLV_TAG_TYPE_AUDIO = 8,
    FLV_TAG_TYPE_VIDEO = 9,
    FLV_TAG_TYPE_META = 18,
	FLV_TAG_TYPE_SPS_PPS = 200
};

typedef struct
{
	TX_EVENTER 	evnt;
    TX_EVENTER 	evnt_close;
    pthread_t threadFileIO;
	pthread_attr_t tattrFileIO;
	char *buffer1;
	char *buffer2;
	pthread_mutex_t buffmutex;
	char freebuffer;
	char flagstop;
	unsigned int datasize1;
	unsigned int datasize2;
	unsigned int buffersize;
	unsigned int minwritesize;
	FILE *filehandle;
} flv_fast_struct;

struct flv_tag {
    uint8_t type;
    uint8_t data_size[3];
    uint8_t timestamp[3];
    uint8_t timestamp_ex;
    uint8_t streamid[3];
} __attribute__((__packed__));

typedef struct flv_tag flv_tag_t;

int SearchData(char *FindData, int FindDataLen, char *InData, int InDataLen, int Pos);
int flv_file_open(FILE **g_file_handle, char *filename, flv_fast_struct* flv_fast);
int flv_file_close(FILE *g_file_handle, flv_fast_struct* flv_fast);
int flv_write_header(FILE *g_file_handle, bool is_have_audio, bool is_have_video, flv_fast_struct* flv_fast, char cAudioCodecNum);
int flv_write_avc_spspps(FILE *g_file_handle, uint8_t *codecinfo, uint32_t codecinfo_len, uint32_t timestamp, flv_fast_struct* flv_fast);
int flv_write_avc_first_frame(FILE *g_file_handle, uint8_t *codecinfo, uint32_t codecinfo_len, uint8_t *framedata, uint32_t framedata_len, uint32_t timestamp, flv_fast_struct* flv_fast);
int flv_write_avc_frame(FILE *g_file_handle, uint8_t *framedata, uint32_t framedata_len, uint32_t timestamp, int is_keyframe, flv_fast_struct* flv_fast);
int flv_write_audio_frame(FILE *g_file_handle, AudioCodecInfo *CodecInfo, uint8_t *framedata, uint32_t framedata_len, uint32_t timestamp, flv_fast_struct* flv_fast);

void flv_write_aac_sequence_header_tag(FILE *g_file_handle, int sample_rate, int channel);
void flv_write_aac_data_tag(FILE *g_file_handle, uint8_t *data, uint32_t data_len, uint32_t timestamp);
void flv_write_audio_data_tag(FILE *g_file_handle, uint8_t *data, uint32_t data_len, uint32_t timestamp);

void flv_fast_writer_close(flv_fast_struct* flv_fast);
void flv_send_signal_write(flv_fast_struct* flv_fast);
flv_fast_struct* flv_fast_writer_open(unsigned int uiBufferSize, unsigned int uiMinSizeWrite);
unsigned int flv_write(uint8_t *data, unsigned int datalen, FILE* f, flv_fast_struct* flv_fast);

#endif // FLV_MUXER_H_
