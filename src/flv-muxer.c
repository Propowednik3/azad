#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "flv-muxer.h"
#include "debug.h"
#include "system.h"


int SearchData(char *FindData, int FindDataLen, char *InData, int InDataLen, int Pos)
{
	int Flag1 = 0;
	int n;
	if (FindDataLen > InDataLen) return -1;
	for (n = Pos; n < InDataLen; n++)
	{
		if (FindData[Flag1] == InData[n]) Flag1++; 
		else
		{
			Flag1 = 0;
			if (FindData[Flag1] == InData[n]) Flag1++;
		}
		if (Flag1 == FindDataLen)
		{
			return (n + 1) - FindDataLen;
		}
	}
	return -1;
}

void* thread_flv_fast_writer(void* pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "flv_writer");
	
	flv_fast_struct* flv_fast = (flv_fast_struct*)pData;
	char iloop = 1;
	unsigned int ret = 0;
	unsigned int uizero = 0;
	unsigned int *idatasize = &uizero;
	unsigned int diff = 0;	
	char cGetWait = 0;
	char *buffer = NULL;
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	tx_eventer_add_event(&flv_fast->evnt, 210);	
			
	while(iloop)
	{
		DBG_MUTEX_LOCK(&flv_fast->buffmutex);
		if (flv_fast->flagstop > 0)
		{			
			if (flv_fast->freebuffer == 0) 
			{
				idatasize =  &flv_fast->datasize2;
				buffer = flv_fast->buffer2;
				if ((flv_fast->datasize2 == 0) && (flv_fast->datasize1 != 0) && ((flv_fast->datasize1 >= flv_fast->minwritesize) || (flv_fast->flagstop >= 2)))
				{
					flv_fast->freebuffer = 1;
					idatasize = &flv_fast->datasize1;
					buffer = flv_fast->buffer1;			
				}
			}
			else
			{
				idatasize = &flv_fast->datasize1;
				buffer = flv_fast->buffer1;
				if ((flv_fast->datasize1 == 0) && (flv_fast->datasize2 != 0) && ((flv_fast->datasize2 >= flv_fast->minwritesize) || (flv_fast->flagstop >= 2)))
				{
					flv_fast->freebuffer = 0;
					idatasize = &flv_fast->datasize2;
					buffer = flv_fast->buffer2;
				}
			}
			if ((*idatasize == 0) && (flv_fast->flagstop == 2))	
			{
				flv_fast->flagstop = 0;
				idatasize = &uizero;
				tx_eventer_send_event(&flv_fast->evnt_close, 220);
			}
		}
		DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);
		//*printf("idatasize %i\n",*idatasize);			
		if (*idatasize != 0)
		{
			diff = (unsigned int)get_ms(&previous_ms);
			ret = fwrite(buffer, 1, *idatasize, flv_fast->filehandle);
			diff = (unsigned int)get_ms(&previous_ms) - diff;
			if (diff > 1000) 
			{
				DBG_MUTEX_LOCK(&flv_fast->buffmutex);
				dbgprintf(3,"LONG WRITE FILE: time:%d len:%i data1:%i data2:%i\n",diff, ret, flv_fast->datasize1, flv_fast->datasize2);			
				DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);
			}
			if (ret != *idatasize)
			{
				dbgprintf(3,"ERROR WRITE FILE with errno =  %d %s \n", errno, strerror(errno));
				/**idatasize -= ret;
				memmove(buffer, &buffer[ret], *idatasize);	*/
				DBG_MUTEX_LOCK(&flv_fast->buffmutex);
				if (flv_fast->flagstop != 2) 
				{
					flv_fast->flagstop = 4;
					flv_fast->freebuffer = 0;
					flv_fast->datasize1 = 0;
					flv_fast->datasize2 = 0;
				}
				DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);
			} 
			else *idatasize = 0; 
		}
		else
		{
			DBG_MUTEX_LOCK(&flv_fast->buffmutex);
			if (flv_fast->flagstop < 2) cGetWait = 1;
			DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);		
		}
		
		DBG_MUTEX_LOCK(&flv_fast->buffmutex);				
		if ((flv_fast->datasize1 == 0) && (flv_fast->datasize2 == 0) && (flv_fast->flagstop == 3)) {iloop = 0; cGetWait = 0;}
		DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);			
		if (cGetWait) 
		{
			tx_eventer_recv_event(&flv_fast->evnt, 210, TX_WAIT_FOREVER);
			cGetWait = 0;
		}
	}
	tx_eventer_send_event(&flv_fast->evnt_close, 230);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return NULL;
}

flv_fast_struct* flv_fast_writer_open(unsigned int uiBufferSize, unsigned int uiMinSizeWrite)
{
	DBG_LOG_IN();
	
	flv_fast_struct* flv_fast = (flv_fast_struct*)DBG_MALLOC(sizeof(flv_fast_struct));
	memset(flv_fast, 0, sizeof(flv_fast_struct));
	
	tx_eventer_create(&flv_fast->evnt, 1);
	tx_eventer_create(&flv_fast->evnt_close, 0);
	pthread_mutex_init(&flv_fast->buffmutex, NULL);
	flv_fast->buffer1 = (char*)DBG_MALLOC(uiBufferSize);
	flv_fast->buffer2 = (char*)DBG_MALLOC(uiBufferSize);
	flv_fast->buffersize = uiBufferSize;
	flv_fast->freebuffer = 0;
	flv_fast->datasize1 = 0;
	flv_fast->datasize2 = 0;
	flv_fast->flagstop = 0;
	flv_fast->minwritesize = uiMinSizeWrite;
	pthread_attr_init(&flv_fast->tattrFileIO);   
	pthread_attr_setdetachstate(&flv_fast->tattrFileIO, PTHREAD_CREATE_DETACHED);
	pthread_create(&flv_fast->threadFileIO, &flv_fast->tattrFileIO, thread_flv_fast_writer, (void*)flv_fast);	

	DBG_LOG_OUT();	
	return flv_fast;
}

void flv_fast_writer_close(flv_fast_struct* flv_fast)
{
	DBG_LOG_IN();
	
	DBG_MUTEX_LOCK(&flv_fast->buffmutex);
	flv_fast->flagstop = 3; // stop
	flv_fast->minwritesize = 0;
	DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);	
	tx_eventer_add_event(&flv_fast->evnt_close, 230);				
	flv_send_signal_write(flv_fast);
	tx_eventer_recv_event(&flv_fast->evnt_close, 230, TX_WAIT_FOREVER);
	pthread_attr_destroy(&flv_fast->tattrFileIO);
	pthread_mutex_destroy(&flv_fast->buffmutex);
	DBG_FREE(flv_fast->buffer1);
	DBG_FREE(flv_fast->buffer2);
	tx_eventer_delete(&flv_fast->evnt);
	tx_eventer_delete(&flv_fast->evnt_close);
	
	DBG_LOG_OUT();
	DBG_FREE(flv_fast);	
}

void flv_send_signal_write(flv_fast_struct* flv_fast)
{
	tx_eventer_send_event(&flv_fast->evnt, 210);
}

unsigned int flv_write(uint8_t *data, unsigned int datalen, FILE* f, flv_fast_struct* flv_fast)
{
	DBG_LOG_IN();
	
	int stopped = 0;
	int ret = 0;
	unsigned int uiWaited;
	
	if (flv_fast == NULL) 
	{
		ret = fwrite(data, 1, datalen, f);
		DBG_LOG_OUT();
		return ret;
	}
	uiWaited = 0;
	do
	{
		if (ret)
		{
			if (uiWaited == 0) dbgprintf(2,"buffer overfull (%iMb): waiting ...\n", flv_fast->buffersize / 1048576);
			usleep(100000);			
			ret = 0;
			uiWaited++;
		}
		DBG_MUTEX_LOCK(&flv_fast->buffmutex);
		if (flv_fast->flagstop == 4)
		{
			stopped = 1;
			DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);
			break;
		}
		if (flv_fast->freebuffer == 0)
		{
			if ((flv_fast->datasize1 + datalen) > flv_fast->buffersize) ret = 1;
			else
			{
				memcpy(&flv_fast->buffer1[flv_fast->datasize1], data, datalen);
				flv_fast->datasize1 += datalen;
			}
		}
		else
		{
			if ((flv_fast->datasize2 + datalen) > flv_fast->buffersize) ret = 1;
			else
			{
				memcpy(&flv_fast->buffer2[flv_fast->datasize2], data, datalen);
				flv_fast->datasize2 += datalen;
			}
		}
		DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);	
	} while(ret);
	
	if (stopped)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	if (uiWaited > 1) dbgprintf(2,"buffer overfull: waited %ims\n", uiWaited * 100);
	flv_send_signal_write(flv_fast);
		
	DBG_LOG_OUT();
	return datalen;
}

int flv_file_open(FILE **g_file_handle, char *filename, flv_fast_struct* flv_fast) 
{
	DBG_LOG_IN();
	
    if (NULL == filename) 
	{
		DBG_LOG_OUT();
		return 0;
	}
    *g_file_handle = fopen(filename, "wb");
	if (*g_file_handle) 
	{
		//if (setvbuf(*g_file_handle, NULL, _IOFBF, 10485760) != 0) perror("Error setvbuf"); else perror("Ok setvbuf");
		if (flv_fast) 
		{
			DBG_MUTEX_LOCK(&flv_fast->buffmutex);
			flv_fast->filehandle = *g_file_handle;
			flv_fast->flagstop = 1;
			DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);
			flv_send_signal_write(flv_fast);	
		}
		DBG_LOG_OUT();
		return 1;
	}
	DBG_LOG_OUT();
	return 0;
}

int flv_file_close(FILE *g_file_handle, flv_fast_struct* flv_fast) 
{
	DBG_LOG_IN();
	
	if (NULL == g_file_handle) 
	{
		DBG_LOG_OUT();
		return 0;
	}
	if (flv_fast) 
	{
		tx_eventer_add_event(&flv_fast->evnt_close, 220);	
		DBG_MUTEX_LOCK(&flv_fast->buffmutex);
		flv_fast->flagstop = 2;
		DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);	
		flv_send_signal_write(flv_fast);	
		tx_eventer_recv_event(&flv_fast->evnt_close, 220, TX_WAIT_FOREVER);
	}
	fclose(g_file_handle);
	//printf("closed\n");
	if (flv_fast) 
	{
		DBG_MUTEX_LOCK(&flv_fast->buffmutex);
		flv_fast->filehandle = NULL;
		DBG_MUTEX_UNLOCK(&flv_fast->buffmutex);
	}
	DBG_LOG_OUT();
	return 1;
}

int flv_write_header(FILE *g_file_handle, bool is_have_audio, bool is_have_video, flv_fast_struct* flv_fast, char cAudioCodecNum) 
{
	DBG_LOG_IN();
	
	int res;
	if (is_have_video || (cAudioCodecNum < 6))
	{
		char flv_file_header[] = "FLV\x1\x5\0\0\0\x9\0\0\0\0"; // have audio and have video

		if (is_have_audio && is_have_video) 
		{
			flv_file_header[4] = 0x05;
		} 
		if (is_have_audio && !is_have_video) 
		{
			flv_file_header[4] = 0x04;
		} 
		if (!is_have_audio && is_have_video) 
		{
			flv_file_header[4] = 0x01;
		} 
		if (!is_have_audio && !is_have_video)  
		{
			flv_file_header[4] = 0x00;
		}

		//fwrite(flv_file_header, 1, 13, g_file_handle);
		res = flv_write((uint8_t*)flv_file_header, 13, g_file_handle, flv_fast);
		if (res != 13)
		{
			DBG_LOG_OUT();
			return 0;
		}
	}
	
	DBG_LOG_OUT();
    return 1;
}

int flv_write_audio_frame(FILE *g_file_handle, AudioCodecInfo *CodecInfo, uint8_t *framedata, uint32_t framedata_len, uint32_t timestamp, flv_fast_struct* flv_fast) 
{
	DBG_LOG_IN();
	
	int res;
    char cFlv = 1;
	uint8_t buff[5];
	flv_tag_t flvtag;	
    uint8_t data_size[4] = {0};
	uint32_t pack_len = framedata_len + 2;
	
	switch(CodecInfo->audio_codec)
	{
		case AV_CODEC_ID_PCM_S16LE: 
			buff[0] = 3 << 4; 
			buff[1] = 0;
			break;		 
		case AV_CODEC_ID_ADPCM_SWF: 
			buff[0] = 1 << 4; 
			buff[1] = 0;
			break;		 
		case AV_CODEC_ID_AAC: 
			buff[0] = 10 << 4; //AAC CODEC
			buff[1] = 1; //AAC RAW
			break;
		case AV_CODEC_ID_MP3: 
			buff[0] = 2 << 4; 
			buff[1] = 0;
			break;
		case AV_CODEC_ID_SPEEX: 
			buff[0] = 11 << 4; 
			buff[1] = 0;
			break;
		case AV_CODEC_ID_NELLYMOSER: 
			buff[0] = 6 << 4; 
			buff[1] = 0;
			break;
		default: 
			cFlv = 0;
			break;
	}
	
	if (cFlv)
	{
		memset(&flvtag, 0, sizeof(flvtag));	
		//VIDEO TAG
		flvtag.type = FLV_TAG_TYPE_AUDIO;    
		flvtag.data_size[0] = (uint8_t) ((pack_len >> 16) & 0xff);
		flvtag.data_size[1] = (uint8_t) ((pack_len >> 8) & 0xff);
		flvtag.data_size[2] = (uint8_t) ((pack_len) & 0xff);
		flvtag.timestamp_ex = (uint8_t) ((timestamp >> 24) & 0xff);
		flvtag.timestamp[0] = (uint8_t) ((timestamp >> 16) & 0xff);
		flvtag.timestamp[1] = (uint8_t) ((timestamp >> 8) & 0xff);
		flvtag.timestamp[2] = (uint8_t) ((timestamp) & 0xff);
		//fwrite(&flvtag, 1, sizeof(flvtag), g_file_handle);	
		res = flv_write((uint8_t*)&flvtag, sizeof(flvtag), g_file_handle, flv_fast);
		if (res != sizeof(flvtag))
		{
			DBG_LOG_OUT();
			return 0;
		}
		
		if (CodecInfo->audio_sample_rate == 44100) buff[0] |= 3 << 2;
		if (CodecInfo->audio_sample_rate == 22050) buff[0] |= 2 << 2;
		if (CodecInfo->audio_sample_rate == 11025) buff[0] |= 1 << 2;
		buff[0] |= 2; // 16 bit
		if (CodecInfo->audio_channels == 2) buff[0] |= 1;  //MONO - STEREO
		
		//fwrite(buff, 1, 2, g_file_handle);	
		res = flv_write(buff, 2, g_file_handle, flv_fast);
		if (res != 2)
		{
			DBG_LOG_OUT();
			return 0;
		}
	}
	
	//MAIN FRAME DATA
	//fwrite(framedata, 1, framedata_len, g_file_handle);
	res = flv_write(framedata, framedata_len, g_file_handle, flv_fast);
	if (res != framedata_len)
	{
		DBG_LOG_OUT();
		return 0;
	}
		
    if (cFlv)
	{
		pack_len = sizeof(flvtag) + framedata_len + 2;
		data_size[0] = (uint8_t) ((pack_len >> 24) & 0xff);
		data_size[1] = (uint8_t) ((pack_len >> 16) & 0xff);
		data_size[2] = (uint8_t) ((pack_len >> 8) & 0xff);
		data_size[3] = (uint8_t) ((pack_len) & 0xff);
		//memset(data_size,0,4);
		//fwrite(data_size, 1, 4, g_file_handle);
		res = flv_write(data_size, 4, g_file_handle, flv_fast);
		if (res != 4)
		{
			DBG_LOG_OUT();
			return 0;
		}

	}
	
	DBG_LOG_OUT();
    return 1;
}

int flv_write_video_tag(FILE *g_file_handle, uint8_t *tag, uint8_t *buf, uint32_t buf_len, uint32_t timestamp, flv_fast_struct* flv_fast) 
{
	DBG_LOG_IN();
	
    int res;
	uint8_t prev_size[4] = {0};
	
    flv_tag_t flvtag;
	uint32_t uiSize =  buf_len + (uint32_t) sizeof(flvtag);

    memset(&flvtag, 0, sizeof(flvtag));

    flvtag.type = FLV_TAG_TYPE_VIDEO;
    
	flvtag.data_size[0] = (uint8_t) ((buf_len >> 16) & 0xff);
    flvtag.data_size[1] = (uint8_t) ((buf_len >> 8) & 0xff);
    flvtag.data_size[2] = (uint8_t) ((buf_len) & 0xff);

    flvtag.timestamp_ex = (uint8_t) ((timestamp >> 24) & 0xff);
    flvtag.timestamp[0] = (uint8_t) ((timestamp >> 16) & 0xff);
    flvtag.timestamp[1] = (uint8_t) ((timestamp >> 8) & 0xff);
    flvtag.timestamp[2] = (uint8_t) ((timestamp) & 0xff);

    //fwrite(&flvtag, 1, sizeof(flvtag), g_file_handle);    
	res = flv_write((uint8_t*)&flvtag, sizeof(flvtag), g_file_handle, flv_fast);
	if (res != sizeof(flvtag))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	if (tag) 
	{
		//fwrite(tag, 1, 5, g_file_handle);
		res = flv_write(tag, 5, g_file_handle, flv_fast);
		if (res != 5)
		{
			DBG_LOG_OUT();
			return 0;
		}
	}
	
	prev_size[0] = (uint8_t) ((buf_len >> 24) & 0xff);
    prev_size[1] = (uint8_t) ((buf_len >> 16) & 0xff);
    prev_size[2] = (uint8_t) ((buf_len >> 8) & 0xff);
    prev_size[3] = (uint8_t) ((buf_len) & 0xff);
	//fwrite(prev_size, 1, 4, g_file_handle);
	res = flv_write(prev_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
		
    //fwrite(buf + 4, 1, buf_len - 9, g_file_handle);
	res = flv_write(buf + 4, buf_len - 9, g_file_handle, flv_fast);
	if (res != (buf_len - 9))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
    prev_size[0] = (uint8_t) ((uiSize >> 24) & 0xff);
    prev_size[1] = (uint8_t) ((uiSize >> 16) & 0xff);
    prev_size[2] = (uint8_t) ((uiSize >> 8) & 0xff);
    prev_size[3] = (uint8_t) ((uiSize) & 0xff);
	//fwrite(prev_size, 1, 4, g_file_handle);
	res = flv_write(prev_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	DBG_LOG_OUT();
    return 1;
}

int flv_write_avc_frame(FILE *g_file_handle, uint8_t *framedata, uint32_t framedata_len, uint32_t timestamp, int is_keyframe, flv_fast_struct* flv_fast) 
{
	DBG_LOG_IN();
	
	int res;
    uint8_t buff[5];
	flv_tag_t flvtag;	
    uint8_t data_size[4] = {0};
	uint32_t pack_len = framedata_len + 5;
	
	memset(&flvtag, 0, sizeof(flvtag));	
	//VIDEO TAG
	flvtag.type = FLV_TAG_TYPE_VIDEO;    
	flvtag.data_size[0] = (uint8_t) ((pack_len >> 16) & 0xff);
    flvtag.data_size[1] = (uint8_t) ((pack_len >> 8) & 0xff);
    flvtag.data_size[2] = (uint8_t) ((pack_len) & 0xff);
    flvtag.timestamp_ex = (uint8_t) ((timestamp >> 24) & 0xff);
    flvtag.timestamp[0] = (uint8_t) ((timestamp >> 16) & 0xff);
    flvtag.timestamp[1] = (uint8_t) ((timestamp >> 8) & 0xff);
    flvtag.timestamp[2] = (uint8_t) ((timestamp) & 0xff);
    //fwrite(&flvtag, 1, sizeof(flvtag), g_file_handle);
	res = flv_write((uint8_t*)&flvtag, sizeof(flvtag), g_file_handle, flv_fast);
	if (res != sizeof(flvtag))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	//AVC TAG
	if (is_keyframe) buff[0] = 0x17; else buff[0] = 0x27;
	buff[1] = 1; //AVC NALU
	buff[2] = 0;
	buff[3] = 0;
	buff[4] = 0;
	
	//fwrite(buff, 1, 5, g_file_handle);
	res = flv_write(buff, 5, g_file_handle, flv_fast);
	if (res != 5)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	//MAIN FRAME DATA
	data_size[0] = (uint8_t) (((framedata_len - 4) >> 24) & 0xff);
    data_size[1] = (uint8_t) (((framedata_len - 4) >> 16) & 0xff);
    data_size[2] = (uint8_t) (((framedata_len - 4) >> 8) & 0xff);
    data_size[3] = (uint8_t) (((framedata_len - 4)) & 0xff);
	//fwrite(data_size, 1, 4, g_file_handle);
	res = flv_write(data_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
    //fwrite(framedata + 4, 1, framedata_len - 4, g_file_handle);
	res = flv_write(framedata + 4, framedata_len - 4, g_file_handle, flv_fast);
	if (res != (framedata_len - 4))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
    pack_len = sizeof(flvtag) + framedata_len + 5;
	data_size[0] = (uint8_t) ((pack_len >> 24) & 0xff);
    data_size[1] = (uint8_t) ((pack_len >> 16) & 0xff);
    data_size[2] = (uint8_t) ((pack_len >> 8) & 0xff);
    data_size[3] = (uint8_t) ((pack_len) & 0xff);
	//memset(data_size,0,4);
	//fwrite(data_size, 1, 4, g_file_handle);
	res = flv_write(data_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	DBG_LOG_OUT();
    return 1;
}

int flv_write_avc_spspps(FILE *g_file_handle, uint8_t *codecinfo, uint32_t codecinfo_len, uint32_t timestamp, flv_fast_struct* flv_fast)
{
	DBG_LOG_IN();
	
	int res;
    uint8_t buff[5];
	flv_tag_t flvtag;	
    uint8_t data_size[4] = {0};
	uint32_t pack_len = (codecinfo_len + 16) - 8;
	
	memset(&flvtag, 0, sizeof(flvtag));	
	//VIDEO TAG
	flvtag.type = FLV_TAG_TYPE_VIDEO;    
	flvtag.data_size[0] = (uint8_t) ((pack_len >> 16) & 0xff);
    flvtag.data_size[1] = (uint8_t) ((pack_len >> 8) & 0xff);
    flvtag.data_size[2] = (uint8_t) ((pack_len) & 0xff);
    flvtag.timestamp_ex = (uint8_t) ((timestamp >> 24) & 0xff);
    flvtag.timestamp[0] = (uint8_t) ((timestamp >> 16) & 0xff);
    flvtag.timestamp[1] = (uint8_t) ((timestamp >> 8) & 0xff);
    flvtag.timestamp[2] = (uint8_t) ((timestamp) & 0xff);
    //fwrite(&flvtag, 1, sizeof(flvtag), g_file_handle);
	res = flv_write((uint8_t*)&flvtag, sizeof(flvtag), g_file_handle, flv_fast);
	if (res != sizeof(flvtag))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	//SPS PPS DATA
	uint8_t *avc_seq_buf = (uint8_t*)DBG_MALLOC(codecinfo_len + 16);
	uint32_t sps_len = 0;
	uint32_t pps_len = 0;	
	buff[0] = 0; 
	buff[1] = 0; 
	buff[2] = 0; 
	buff[3] = 1; 
	sps_len = SearchData((char*)buff, 4, (char*)codecinfo, codecinfo_len, 1);
	pps_len = codecinfo_len - sps_len;	
    
	avc_seq_buf[0] = (1 << 4) | 7; //frametype "1 == keyframe" codecid "7 == AVC"
	avc_seq_buf[1] = 0; // AVCPacketType: 0x00 - AVC sequence header
	avc_seq_buf[2] = 0; // composition time
	avc_seq_buf[3] = 0; // composition time
	avc_seq_buf[4] = 0; // composition time
    // generate AVCC with sps and pps, AVCDecoderConfigurationRecord
    avc_seq_buf[5] = 1; // configurationVersion
    avc_seq_buf[6] = codecinfo[5]; // AVCProfileIndication
    avc_seq_buf[7] = codecinfo[6]; // profile_compatibility
    avc_seq_buf[8] = codecinfo[7]; // AVCLevelIndication
    // 6 bits reserved (111111) + 2 bits nal size length - 1
    // (Reserved << 2) | Nal_Size_length = (0x3F << 2) | 0x03 = 0xFF
    avc_seq_buf[9] = 0xff;	// 3 bits reserved (111) + 5 bits number of sps (00001)    
							// (Reserved << 5) | Number_of_SPS = (0x07 << 5) | 0x01 = 0xe1
    avc_seq_buf[10] = 0xe1;
    // sps
	sps_len -= 4;
    avc_seq_buf[11] = ((uint16_t)(sps_len) >> 8) & 0xff;
    avc_seq_buf[12] = (uint16_t)(sps_len) & 0xff;
    memcpy(&avc_seq_buf[13], &codecinfo[4], sps_len);    
    // pps
	pps_len -= 4;
    avc_seq_buf[13 + sps_len] = 1; // number of pps
    avc_seq_buf[14 + sps_len] = ((uint16_t)(pps_len) >> 8) & 0xff;
    avc_seq_buf[15 + sps_len] = (uint16_t)(pps_len) & 0xff;
    memcpy(&avc_seq_buf[16 + sps_len], &codecinfo[sps_len + 8], pps_len);    
	
	//fwrite(avc_seq_buf, 1, pack_len, g_file_handle);
	res = flv_write(avc_seq_buf, pack_len, g_file_handle, flv_fast);
	if (res != pack_len)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
    pack_len = (sizeof(flvtag) + codecinfo_len + 16) - 8;
	data_size[0] = (uint8_t) ((pack_len >> 24) & 0xff);
    data_size[1] = (uint8_t) ((pack_len >> 16) & 0xff);
    data_size[2] = (uint8_t) ((pack_len >> 8) & 0xff);
    data_size[3] = (uint8_t) ((pack_len) & 0xff);
	//fwrite(data_size, 1, 4, g_file_handle);
	res = flv_write(data_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	DBG_FREE(avc_seq_buf);
	
	DBG_LOG_OUT();
    return 1;
}

int flv_write_avc_first_frame(FILE *g_file_handle, uint8_t *codecinfo, uint32_t codecinfo_len, uint8_t *framedata, uint32_t framedata_len, uint32_t timestamp, flv_fast_struct* flv_fast) 
{
	DBG_LOG_IN();
	
	int res;
	uint8_t buff[5];
	uint32_t sps_len = 0;
	uint32_t pps_len = 0;
	uint8_t data_size[4] = {0};
	flv_tag_t flvtag;	
    uint32_t pack_len = 5 + codecinfo_len + framedata_len;
	uint8_t *spspps;
	
	memset(&flvtag, 0, sizeof(flvtag));	
	//VIDEO TAG
	flvtag.type = FLV_TAG_TYPE_VIDEO;    
	flvtag.data_size[0] = (uint8_t) ((pack_len >> 16) & 0xff);
    flvtag.data_size[1] = (uint8_t) ((pack_len >> 8) & 0xff);
    flvtag.data_size[2] = (uint8_t) ((pack_len) & 0xff);
    flvtag.timestamp_ex = (uint8_t) ((timestamp >> 24) & 0xff);
    flvtag.timestamp[0] = (uint8_t) ((timestamp >> 16) & 0xff);
    flvtag.timestamp[1] = (uint8_t) ((timestamp >> 8) & 0xff);
    flvtag.timestamp[2] = (uint8_t) ((timestamp) & 0xff);
    //fwrite(&flvtag, 1, sizeof(flvtag), g_file_handle);
	res = flv_write((uint8_t*)&flvtag, sizeof(flvtag), g_file_handle, flv_fast);
	if (res != sizeof(flvtag))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	//AVC TAG	
	buff[0] = 0x17; // 1 << 4 (keyframe) | 7 (AVC )
	buff[1] = 1; //AVC NALU
	buff[2] = 0; // composition time	
	buff[3] = 0; // composition time	
	buff[4] = 0; // composition time		
	//fwrite(buff, 1, 5, g_file_handle);
	res = flv_write(buff, 5, g_file_handle, flv_fast);
	if (res != 5)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	//SPS PPS DATA
	spspps = (uint8_t*)DBG_MALLOC(codecinfo_len);
	memcpy(spspps, codecinfo, codecinfo_len);	
	buff[0] = 0; 
	buff[1] = 0; 
	buff[2] = 0; 
	buff[3] = 1; 
	sps_len = SearchData((char*)buff, 4, (char*)spspps, codecinfo_len, 1);
	pps_len = codecinfo_len - sps_len;	
	spspps[0] = (uint8_t) (((sps_len - 4) >> 24) & 0xff);
    spspps[1] = (uint8_t) (((sps_len - 4) >> 16) & 0xff);
    spspps[2] = (uint8_t) (((sps_len - 4) >> 8) & 0xff);
    spspps[3] = (uint8_t) (((sps_len - 4)) & 0xff);
	spspps[sps_len] = (uint8_t) (((pps_len - 4) >> 24) & 0xff);
    spspps[sps_len+1] = (uint8_t) (((pps_len - 4) >> 16) & 0xff);
    spspps[sps_len+2] = (uint8_t) (((pps_len - 4) >> 8) & 0xff);
    spspps[sps_len+3] = (uint8_t) (((pps_len - 4)) & 0xff);  
	//fwrite(spspps, 1, codecinfo_len, g_file_handle);
	res = flv_write(spspps, codecinfo_len, g_file_handle, flv_fast);
	if (res != codecinfo_len)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	//MAIN FRAME DATA
	data_size[0] = (uint8_t) (((framedata_len - 4) >> 24) & 0xff);
    data_size[1] = (uint8_t) (((framedata_len - 4) >> 16) & 0xff);
    data_size[2] = (uint8_t) (((framedata_len - 4) >> 8) & 0xff);
    data_size[3] = (uint8_t) (((framedata_len - 4)) & 0xff);
	//fwrite(data_size, 1, 4, g_file_handle);
    res = flv_write(data_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
	//fwrite(framedata + 4, 1, framedata_len - 4, g_file_handle);
	res = flv_write(framedata + 4, framedata_len - 4, g_file_handle, flv_fast);
	if (res != (framedata_len - 4))
	{
		DBG_LOG_OUT();
		return 0;
	}
	
	pack_len = sizeof(flvtag) + 5 + codecinfo_len + framedata_len;
    data_size[0] = (uint8_t) ((pack_len >> 24) & 0xff);
    data_size[1] = (uint8_t) ((pack_len >> 16) & 0xff);
    data_size[2] = (uint8_t) ((pack_len >> 8) & 0xff);
    data_size[3] = (uint8_t) ((pack_len) & 0xff);
	//memset(data_size,0,4);
	//fwrite(data_size, 1, 4, g_file_handle);
    res = flv_write(data_size, 4, g_file_handle, flv_fast);
	if (res != 4)
	{
		DBG_LOG_OUT();
		return 0;
	}
	
    DBG_FREE(spspps);
	
	DBG_LOG_OUT();	
    return 1;
}






