
#ifndef FLV_DEMUXER_H_
#define FLV_DEMUXER_H_

#include "main.h"
#include "omx_client.h"

typedef struct
{	
	unsigned int 	FrameNum;
	unsigned int	FramePos;
} map_struct;

typedef struct
{	
	int 			Status;
	unsigned int 	FileSize;
	char 			Path[MAX_PATH];
	char 			ShowPath[MAX_PATH];
	char 			Name[MAX_FILE_LEN];
	char 			Crashed;
	unsigned int 	VideoStream;
	unsigned int 	AudioStream;
	unsigned int 	VideoEnabled;
	unsigned int 	AudioEnabled;
	unsigned int 	VideoKeyFrames;
	unsigned int 	VideoFrames;
	unsigned int 	AudioFrames;
	unsigned int 	TotalFrames;	
	unsigned int 	MetaFrames;
	FILE			*filehandle;
	char			*SPS_Data;
	unsigned int	SPS_Lenght;
	char			*PPS_Data;
	unsigned int	PPS_Lenght;
	char			*Frame;
	unsigned int	Frame_Lenght;
	unsigned int	Frame_MaxSize;
	unsigned int	Frame_Sended;
	unsigned int	Frame_Type;
	unsigned int	Frame_Key;	
	unsigned char 	Reworked;
	unsigned int	CurrentByte;
	unsigned int 	CurrentFrame;
	unsigned int 	CurrentScrollFrame;
	float		 	FrameWeight;
	map_struct		*ScrollMap;
} flv_demux_struct;

int flv_dm_open(flv_demux_struct *flv_handle, omx_start_packet *StartPack, AudioCodecInfo *tCodecInfo, unsigned int uiReqStreams);
int flv_dm_close(flv_demux_struct *flv_handle, omx_start_packet *StartPack);
unsigned int flv_dm_read_info(flv_demux_struct *flv_handle, omx_start_packet *StartPack, AudioCodecInfo *tCodecInfo);
int flv_dm_read_frame(flv_demux_struct *flv_handle, omx_start_packet *StartPack);
int flv_dm_fill_map(flv_demux_struct *flv_handle);
int flv_dm_set_pos(flv_demux_struct *flv_handle, unsigned int uiPos);

#endif // FLV_DEMUXER_H_
