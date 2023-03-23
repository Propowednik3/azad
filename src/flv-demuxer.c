#include "flv-demuxer.h"
#include "flv-muxer.h"
#include "main.h"
#include "omx_client.h"

/*
unsigned int get_bits(char *cSPS, int len, int count, int *pnum, int *pbit, char cDirect)
{
	int num = *pnum; 
	int bit = *pbit;
	unsigned int ret = 0;
	int i, val;
	if (cDirect) val = 1 << (count - 1); else val = 1;
	for (i = 0; i <= count; i++) 
	{
		ret |= (cSPS[num] & bit) ? val : 0;
		if (cDirect) val >>= 1; else ret <<= 1;
		bit++;
		if (bit >= 8)
		{
			bit = 0;
			num++;
			if (num >= len) break;
		}
	}
	*pnum = num;
	*pbit = bit;
	return ret;
}
*/
/*int flv_dm_decode_sps(flv_demux_struct *flv_handle)
{
    int profile_idc, level_idc;
    unsigned int sps_id;
	unsigned int uiSpsLen = flv_handle->SPS_Lenght;
	char *cSPS = flv_handle->SPS_Data;
	int SpsCurNum = 0;
	int SpsCurBit = 0;
	
    profile_idc= get_bits(cSPS, uiSpsLen, 8, &SpsCurNum, &SpsCurBit, 1);
	get_bits(cSPS, uiSpsLen, 1, &SpsCurNum, &SpsCurBit, 1);   //constraint_set0_flag
    get_bits(cSPS, uiSpsLen, 1, &SpsCurNum, &SpsCurBit, 1);   //constraint_set1_flag
    get_bits(cSPS, uiSpsLen, 1, &SpsCurNum, &SpsCurBit, 1);   //constraint_set2_flag
    get_bits(cSPS, uiSpsLen, 1, &SpsCurNum, &SpsCurBit, 1);   //constraint_set3_flag
    get_bits(cSPS, uiSpsLen, 4, &SpsCurNum, &SpsCurBit, 1);// reserved
	level_idc= get_bits(cSPS, uiSpsLen, 8, &SpsCurNum, &SpsCurBit, 1);
	
	printf("profile_idc %i  level_idc %i\n", profile_idc, level_idc);*/
  /*  sps_id= get_ue_golomb_31(&s->gb);
 
    if(sps_id >= MAX_SPS_COUNT) {
        av_log(h->s.avctx, AV_LOG_ERROR, "sps_id (%d) out of range\n", sps_id);
         return -1;
    }
    sps= av_mallocz(sizeof(SPS));
    if(sps == NULL)
         return -1;
 
    sps->profile_idc= profile_idc;
    sps->level_idc= level_idc;
 
    memset(sps->scaling_matrix4, 16, sizeof(sps->scaling_matrix4));
    memset(sps->scaling_matrix8, 16, sizeof(sps->scaling_matrix8));
    sps->scaling_matrix_present = 0;
*/
/*    if(profile_idc >= 100)
	{ //high profile
         sps->chroma_format_idc= get_ue_golomb_31(&s->gb);
         if (sps->chroma_format_idc > 3U) 
		 {
             av_log(h->s.avctx, AV_LOG_ERROR, "chroma_format_idc %d is illegal\n", sps->chroma_format_idc);
             goto fail;
         } else if(sps->chroma_format_idc == 3) {
             sps->residual_color_transform_flag = get_bits1(&s->gb);
         }
         sps->bit_depth_luma   = get_ue_golomb(&s->gb) + 8;
         sps->bit_depth_chroma = get_ue_golomb(&s->gb) + 8;
         sps->transform_bypass = get_bits1(&s->gb);
         decode_scaling_matrices(h, sps, NULL, 1, sps->scaling_matrix4, sps->scaling_matrix8);
    }
	else
	{
        sps->chroma_format_idc= 1;
        sps->bit_depth_luma   = 8;
        sps->bit_depth_chroma = 8;
    }
 
     sps->log2_max_frame_num= get_ue_golomb(&s->gb) + 4;
     sps->poc_type= get_ue_golomb_31(&s->gb);
 
     if(sps->poc_type == 0){ //FIXME #define
         sps->log2_max_poc_lsb= get_ue_golomb(&s->gb) + 4;
     } else if(sps->poc_type == 1){//FIXME #define
         sps->delta_pic_order_always_zero_flag= get_bits1(&s->gb);
         sps->offset_for_non_ref_pic= get_se_golomb(&s->gb);
         sps->offset_for_top_to_bottom_field= get_se_golomb(&s->gb);
         sps->poc_cycle_length                = get_ue_golomb(&s->gb);
00326 
00327         if((unsigned)sps->poc_cycle_length >= FF_ARRAY_ELEMS(sps->offset_for_ref_frame)){
00328             av_log(h->s.avctx, AV_LOG_ERROR, "poc_cycle_length overflow %u\n", sps->poc_cycle_length);
00329             goto fail;
00330         }
00331 
00332         for(i=0; i<sps->poc_cycle_length; i++)
00333             sps->offset_for_ref_frame[i]= get_se_golomb(&s->gb);
00334     }else if(sps->poc_type != 2){
00335         av_log(h->s.avctx, AV_LOG_ERROR, "illegal POC type %d\n", sps->poc_type);
00336         goto fail;
00337     }
00338 
00339     sps->ref_frame_count= get_ue_golomb_31(&s->gb);
00340     if(sps->ref_frame_count > MAX_PICTURE_COUNT-2 || sps->ref_frame_count >= 32U){
00341         av_log(h->s.avctx, AV_LOG_ERROR, "too many reference frames\n");
00342         goto fail;
00343     }
00344     sps->gaps_in_frame_num_allowed_flag= get_bits1(&s->gb);
00345     sps->mb_width = get_ue_golomb(&s->gb) + 1;
00346     sps->mb_height= get_ue_golomb(&s->gb) + 1;
00347     if((unsigned)sps->mb_width >= INT_MAX/16 || (unsigned)sps->mb_height >= INT_MAX/16 ||
00348        avcodec_check_dimensions(NULL, 16*sps->mb_width, 16*sps->mb_height)){
00349         av_log(h->s.avctx, AV_LOG_ERROR, "mb_width/height overflow\n");
00350         goto fail;
00351     }
00352 
00353     sps->frame_mbs_only_flag= get_bits1(&s->gb);
00354     if(!sps->frame_mbs_only_flag)
00355         sps->mb_aff= get_bits1(&s->gb);
00356     else
00357         sps->mb_aff= 0;
00358 
00359     sps->direct_8x8_inference_flag= get_bits1(&s->gb);
00360     if(!sps->frame_mbs_only_flag && !sps->direct_8x8_inference_flag){
00361         av_log(h->s.avctx, AV_LOG_ERROR, "This stream was generated by a broken encoder, invalid 8x8 inference\n");
00362         goto fail;
00363     }
00364 
00365 #ifndef ALLOW_INTERLACE
00366     if(sps->mb_aff)
00367         av_log(h->s.avctx, AV_LOG_ERROR, "MBAFF support not included; enable it at compile-time.\n");
00368 #endif
00369     sps->crop= get_bits1(&s->gb);
00370     if(sps->crop){
00371         sps->crop_left  = get_ue_golomb(&s->gb);
00372         sps->crop_right = get_ue_golomb(&s->gb);
00373         sps->crop_top   = get_ue_golomb(&s->gb);
00374         sps->crop_bottom= get_ue_golomb(&s->gb);
00375         if(sps->crop_left || sps->crop_top){
00376             av_log(h->s.avctx, AV_LOG_ERROR, "insane cropping not completely supported, this could look slightly wrong ...\n");
00377         }
00378         if(sps->crop_right >= 8 || sps->crop_bottom >= (8>> !sps->frame_mbs_only_flag)){
00379             av_log(h->s.avctx, AV_LOG_ERROR, "brainfart cropping not supported, this could look slightly wrong ...\n");
00380         }
00381     }else{
00382         sps->crop_left  =
00383         sps->crop_right =
00384         sps->crop_top   =
00385         sps->crop_bottom= 0;
00386     }
00387 
00388     sps->vui_parameters_present_flag= get_bits1(&s->gb);
00389     if( sps->vui_parameters_present_flag )
00390         if (decode_vui_parameters(h, sps) < 0)
00391             goto fail;
00392 
00393     if(s->avctx->debug&FF_DEBUG_PICT_INFO){
00394         av_log(h->s.avctx, AV_LOG_DEBUG, "sps:%u profile:%d/%d poc:%d ref:%d %dx%d %s %s crop:%d/%d/%d/%d %s %s %d/%d\n",
00395                sps_id, sps->profile_idc, sps->level_idc,
00396                sps->poc_type,
00397                sps->ref_frame_count,
00398                sps->mb_width, sps->mb_height,
9                sps->frame_mbs_only_flag ? "FRM" : (sps->mb_aff ? "MB-AFF" : "PIC-AFF"),
                sps->direct_8x8_inference_flag ? "8B8" : "",
                sps->crop_left, sps->crop_right,
                sps->crop_top, sps->crop_bottom,
                sps->vui_parameters_present_flag ? "VUI" : "",
                ((const char*[]){"Gray","420","422","444"})[sps->chroma_format_idc],
                sps->timing_info_present_flag ? sps->num_units_in_tick : 0,
                sps->timing_info_present_flag ? sps->time_scale : 0
            );
    }
    av_free(h->sps_buffers[sps_id]);
    h->sps_buffers[sps_id]= sps;
    h->sps = *sps;
    return 0;
}*/

int flv_dm_open(flv_demux_struct *flv_handle, omx_start_packet *StartPack, AudioCodecInfo *tCodecInfo, unsigned int uiReqStreams)
{
	DBG_LOG_IN();
	int res = 0;  
	
	if (flv_handle->Status)
	{
		dbgprintf(2, "File opened, get close him\n");
		flv_dm_close(flv_handle, StartPack);
	}
	
	flv_handle->Status = 0;
	flv_handle->Reworked = 1;
	flv_handle->Frame_Sended = 0;
	flv_handle->CurrentByte = 13;
	flv_handle->Crashed = 0;
	
    flv_handle->filehandle = fopen(flv_handle->Path, "rb");
	if (!flv_handle->filehandle) dbgprintf(2, "Error open file (%i)%s\n",errno, strerror(errno));
	while (flv_handle->filehandle) 
	{
		if (fseek(flv_handle->filehandle, 0L, SEEK_END) != 0) break;
		
		flv_handle->FileSize = ftell(flv_handle->filehandle);
		if (flv_handle->FileSize < (13 + sizeof(flv_tag_t)))
		{
			dbgprintf(2, "Very short file stream\n");
			break;
		}
		if (fseek(flv_handle->filehandle, 0, SEEK_SET) != 0) break;
	
		char cBuff[13];
		int ret = fread(cBuff, 1, 13, flv_handle->filehandle);
		if ((ret == 13) && (cBuff[0] == 70) && (cBuff[1] == 76) && (cBuff[2] == 86) && (cBuff[3] == 1) && (cBuff[8] == 9))
		{
			flv_handle->AudioStream = (cBuff[4] & 4) >> 2;
			flv_handle->VideoStream = cBuff[4] & 1;		
			if (flv_handle->AudioStream || flv_handle->VideoStream) 
			{
				flv_handle->Status = 1;
				if (flv_handle->AudioStream && (uiReqStreams & 2)) flv_handle->AudioEnabled = 1; 
				if (flv_handle->VideoStream && (uiReqStreams & 1)) flv_handle->VideoEnabled = 1;
							
				if (!flv_dm_read_info(flv_handle, StartPack, tCodecInfo)) 
				{
					flv_handle->Crashed = 1;
					dbgprintf(3, "Error read info from FLV file\n");
				}
								
				flv_dm_fill_map(flv_handle);
				
				flv_handle->Frame = (char*)DBG_MALLOC(flv_handle->Frame_MaxSize);
				
				if (flv_handle->VideoEnabled) 
					flv_handle->FrameWeight = (float)flv_handle->VideoFrames/1000; 
					else
					flv_handle->FrameWeight = (float)flv_handle->AudioFrames/1000; 
				//printf("FLV info: VF:%i, AF:%i, FW:%f\n", flv_handle->VideoFrames, flv_handle->AudioFrames, flv_handle->FrameWeight);	
				res = 1;
				flv_dm_set_pos(flv_handle, 0);
			}
			else 
			{
				dbgprintf(2, "File not have audio or video streams\n");
				break;
			}
		}
		else 
		{
			dbgprintf(2, "File not FLV\n");
			break;
		}
		break;
	}
	if ((flv_handle->filehandle) && (flv_handle->Status == 0))
	{
		fclose(flv_handle->filehandle);				
	}
	DBG_LOG_OUT();
	return res;
}

int flv_dm_close(flv_demux_struct *flv_handle, omx_start_packet *StartPack)
{
	DBG_LOG_IN();
	if (flv_handle->Status != 0)
	{
		if (flv_handle->SPS_Lenght) DBG_FREE(flv_handle->SPS_Data);
		if (flv_handle->PPS_Lenght) DBG_FREE(flv_handle->PPS_Data);
		if (flv_handle->Frame) DBG_FREE(flv_handle->Frame);
		if (flv_handle->ScrollMap) DBG_FREE(flv_handle->ScrollMap);
		if (StartPack->StartFrameLen) 
		{
			DBG_FREE(StartPack->BufferStartFrame);
			StartPack->StartFrameLen = 0;
		}
		if (StartPack->StartFramesCount) 
		{
			StartPack->StartFramesCount = 0;
			DBG_FREE(StartPack->StartFramesSizes);
			DBG_FREE(StartPack->StartFramesFlags);
		}
		if (StartPack->CodecInfoLen) 
		{
			StartPack->CodecInfoLen = 0;
			DBG_FREE(StartPack->BufferCodecInfo);
		}
		
		fclose(flv_handle->filehandle);
		memset(flv_handle, 0, sizeof(flv_demux_struct));
		DBG_LOG_OUT();
		return 1;
	}
	DBG_LOG_OUT();
	return 0;
}

unsigned int flv_dm_read_info(flv_demux_struct *flv_handle, omx_start_packet *StartPack, AudioCodecInfo *tCodecInfo)
{
	flv_tag_t f_tag;
	
	//printf("File size %i\n", flv_handle->FileSize);
	
	unsigned int pack_len, sps_len, pps_len, ret;
	unsigned int uiCurrPos = 13;
	char cFirstVideoLoaded = 0;
	char cFirstAudioLoaded = 0;
	char cBuff[6];
	
	if (fseek(flv_handle->filehandle, uiCurrPos, SEEK_SET) == 0)
	{
		do
		{
			uiCurrPos += sizeof(flv_tag_t);
			if (uiCurrPos >= flv_handle->FileSize) break;
			if (fread(&f_tag, 1, sizeof(flv_tag_t), flv_handle->filehandle) != sizeof(flv_tag_t)) return 0;
			
			pack_len = f_tag.data_size[0] << 16;
			pack_len |= f_tag.data_size[1] << 8;
			pack_len |= f_tag.data_size[2];
						
			if ((f_tag.type != FLV_TAG_TYPE_META) && (f_tag.type != FLV_TAG_TYPE_AUDIO) && (f_tag.type != FLV_TAG_TYPE_VIDEO)) return 0;
			if (f_tag.type == FLV_TAG_TYPE_META) 
			{
				flv_handle->MetaFrames++;
				flv_handle->TotalFrames++;
			}
			if (f_tag.type == FLV_TAG_TYPE_AUDIO)
			{	
				flv_handle->AudioFrames++;
				flv_handle->TotalFrames++;
				if (cFirstAudioLoaded == 0)
				{
					//printf("Frame %i size %i\n", f_tag.type, pack_len);				
					if (pack_len >= 2)
					{
						if (fread(cBuff, 1, 2, flv_handle->filehandle) != 2) return 0;
						cFirstAudioLoaded = 1;					
						
						switch((cBuff[0] >> 2) & 3)
						{
							case 1:
								tCodecInfo->audio_sample_rate = 11025;
								break;
							case 2:
								tCodecInfo->audio_sample_rate = 22050;
								break;
							case 3:
								tCodecInfo->audio_sample_rate = 44100;
								break;
							default:
								tCodecInfo->audio_sample_rate = 44100;
								break;							
						}
						if (cBuff[0] & 1) tCodecInfo->audio_channels = 2; else tCodecInfo->audio_channels = 1;
						tCodecInfo->audio_codec = AV_CODEC_ID_AAC;
						tCodecInfo->audio_profile = FF_PROFILE_UNKNOWN;
						ret = (cBuff[0] >> 4) & 15;
						if ((ret == 3) && (cBuff[1] == 0)) tCodecInfo->audio_codec = AV_CODEC_ID_PCM_S16LE;
						if ((ret == 1) && (cBuff[1] == 0)) tCodecInfo->audio_codec = AV_CODEC_ID_ADPCM_SWF;
						if ((ret == 10) && (cBuff[1] == 1)) tCodecInfo->audio_codec = AV_CODEC_ID_AAC;
						if ((ret == 2) && (cBuff[1] == 0)) tCodecInfo->audio_codec = AV_CODEC_ID_MP3;
						if ((ret == 11) && (cBuff[1] == 0)) tCodecInfo->audio_codec = AV_CODEC_ID_SPEEX;
						if ((ret == 6) && (cBuff[1] == 0)) tCodecInfo->audio_codec = AV_CODEC_ID_NELLYMOSER;	
						tCodecInfo->CodecInfoFilled = 1;
						//printf("Audio %i %i %i\n", ret, tCodecInfo->audio_channels, tCodecInfo->audio_sample_rate);
					}
				}
				if (tCodecInfo->audio_raw_buffer_size < (pack_len - 2)) tCodecInfo->audio_raw_buffer_size = pack_len - 2;
				
				if (tCodecInfo->audio_raw_buffer_size > flv_handle->Frame_MaxSize)
					flv_handle->Frame_MaxSize = tCodecInfo->audio_raw_buffer_size;
			}
			if (f_tag.type == FLV_TAG_TYPE_VIDEO)
			{
				if (pack_len > flv_handle->Frame_MaxSize) flv_handle->Frame_MaxSize = pack_len;
				
				if (cFirstVideoLoaded == 2)
				{
					if (pack_len)
					{
						if (fread(cBuff, 1, 1, flv_handle->filehandle) != 1) return 0;
						if (cBuff[0] == 0x17) flv_handle->VideoKeyFrames++;
					}
				}
				
				if (cFirstVideoLoaded == 1)
				{					
					if (pack_len >= 5)
					{
						if (fread(cBuff, 1, 5, flv_handle->filehandle) != 5) return 0;
						if ((cBuff[0] == 0x17) && (cBuff[1] == 1) && (cBuff[2] == 0) && (cBuff[3] == 0) && (cBuff[4] == 0))
						{
							//printf("FIRST FRAME %i\n", uiCurrPos - sizeof(flv_tag_t));
							flv_handle->VideoKeyFrames++;
							if (fread(cBuff, 1, 4, flv_handle->filehandle) != 4) return 0;
							sps_len = cBuff[0] << 24;
							sps_len |= cBuff[1] << 16;
							sps_len |= cBuff[2] << 8;
							sps_len |= cBuff[3];
							if (flv_handle->SPS_Lenght == sps_len)
							{
								if (fseek(flv_handle->filehandle, sps_len, SEEK_CUR) != 0) return 0;
								if (fread(cBuff, 1, 4, flv_handle->filehandle) != 4) return 0;
								pps_len = cBuff[0] << 24;
								pps_len |= cBuff[1] << 16;
								pps_len |= cBuff[2] << 8;
								pps_len |= cBuff[3];
								if (flv_handle->PPS_Lenght == pps_len)
								{
									if (fseek(flv_handle->filehandle, pps_len, SEEK_CUR) != 0) return 0;
									if (fread(cBuff, 1, 4, flv_handle->filehandle) != 4) return 0;
									StartPack->BufferStartSize = cBuff[0] << 24;
									StartPack->BufferStartSize |= cBuff[1] << 16;
									StartPack->BufferStartSize |= cBuff[2] << 8;
									StartPack->BufferStartSize |= cBuff[3];
									StartPack->BufferStartSize += 4;
									if (pack_len > StartPack->BufferStartSize)
									{
										StartPack->BufferStartFrame = (char*)DBG_MALLOC(StartPack->BufferStartSize);
										StartPack->StartFrameLen = StartPack->BufferStartSize;
										StartPack->StartFramesCount = 1;
										StartPack->StartFramesSizes = (unsigned int*)DBG_MALLOC(StartPack->StartFramesCount * sizeof(unsigned int));
										StartPack->StartFramesFlags = (unsigned char*)DBG_MALLOC(StartPack->StartFramesCount * sizeof(unsigned char));
										StartPack->StartFramesSizes[0] = StartPack->StartFrameLen;
										StartPack->StartFramesFlags[0] = 2;
										StartPack->StartFrameFilled = 2;
										StartPack->BufferStartFrame[0] = 0;
										StartPack->BufferStartFrame[1] = 0;
										StartPack->BufferStartFrame[2] = 0;
										StartPack->BufferStartFrame[3] = 1;
										if (fread(&StartPack->BufferStartFrame[4], 1, StartPack->BufferStartSize - 4, flv_handle->filehandle) != (StartPack->BufferStartSize - 4)) return 0;																				
									} else dbgprintf(3, "Wrong flv format, frame_len wrong\n");
								} else dbgprintf(3, "Wrong flv format, pps_len wrong (%i != %i)\n", flv_handle->PPS_Lenght, pps_len);
							} else dbgprintf(3, "Wrong flv format, sps_len wrong (%i != %i)\n", flv_handle->SPS_Lenght, sps_len);
						} else dbgprintf(3, "Wrong flv format, header wrong\n");
					}
					flv_handle->CurrentByte = uiCurrPos + pack_len + 4;
					cFirstVideoLoaded = 2;
				}
				if (cFirstVideoLoaded == 0)
				{
					//printf("Frame %i size %i\n", f_tag.type, pack_len);				
					if (pack_len >= 6)
					{
						if (fread(cBuff, 1, 6, flv_handle->filehandle) != 6) return 0;
						if ((cBuff[0] == 0x17) && (cBuff[1] == 0) && (cBuff[2] == 0) && (cBuff[3] == 0) && (cBuff[4] == 0) && (cBuff[5] == 1))
						{
							if (fread(cBuff, 1, 5, flv_handle->filehandle) != 5) return 0;
							if (fread(cBuff, 1, 2, flv_handle->filehandle) != 2) return 0;
							sps_len = cBuff[0] << 8;
							sps_len |= cBuff[1];
							//printf("sps len %i\n", sps_len);								
							
							if (((pack_len - 13) >= sps_len) && (sps_len < 1000000))
							{
								flv_handle->SPS_Lenght = sps_len;
								flv_handle->SPS_Data = (char*)DBG_MALLOC(sps_len);
								if (fread(flv_handle->SPS_Data, 1, sps_len, flv_handle->filehandle) != sps_len) return 0;	
								if (fread(cBuff, 1, 3, flv_handle->filehandle) != 3) return 0;
								pps_len = cBuff[1] << 8;
								pps_len |= cBuff[2];
								//printf("pps len %i\n", pps_len);									
								if (((pack_len - 16 - sps_len) >= pps_len) && (pps_len < 1000000))
								{
									flv_handle->PPS_Lenght = pps_len;
									flv_handle->PPS_Data = (char*)DBG_MALLOC(pps_len);
									if (fread(flv_handle->PPS_Data, 1, pps_len, flv_handle->filehandle) != pps_len) return 0;									
								} else dbgprintf(2, "flv_dm_read_info: Wrong pps len %i\n", pps_len);
							} else dbgprintf(2, "flv_dm_read_info: Wrong sps len %i\n", sps_len);
							if (flv_handle->SPS_Lenght && flv_handle->PPS_Lenght)
							{
								StartPack->CodecInfoLen = flv_handle->SPS_Lenght + flv_handle->PPS_Lenght + 8;
								StartPack->BufferCodecInfo = (char*)DBG_MALLOC(StartPack->CodecInfoLen);
								StartPack->BufferCodecInfo[0] = 0;
								StartPack->BufferCodecInfo[1] = 0;
								StartPack->BufferCodecInfo[2] = 0;
								StartPack->BufferCodecInfo[3] = 1;
								memcpy(&StartPack->BufferCodecInfo[4], flv_handle->SPS_Data, flv_handle->SPS_Lenght);
								StartPack->BufferCodecInfo[flv_handle->SPS_Lenght + 4] = 0;
								StartPack->BufferCodecInfo[flv_handle->SPS_Lenght + 5] = 0;
								StartPack->BufferCodecInfo[flv_handle->SPS_Lenght + 6] = 0;
								StartPack->BufferCodecInfo[flv_handle->SPS_Lenght + 7] = 1;
								memcpy(&StartPack->BufferCodecInfo[flv_handle->SPS_Lenght + 8], flv_handle->PPS_Data, flv_handle->PPS_Lenght);
								StartPack->CodecInfoFilled = 1;
							}
						}
						flv_handle->CurrentByte = uiCurrPos + pack_len + 4;
						cFirstVideoLoaded = 1;
					}					
				}
				flv_handle->VideoFrames++;
				flv_handle->TotalFrames++;				
			}
			uiCurrPos += pack_len + 4;
			if ((uiCurrPos > flv_handle->FileSize) || (fseek(flv_handle->filehandle, uiCurrPos, SEEK_SET) != 0))
			{
				flv_handle->TotalFrames--;
				return 0;
			}
		} while(1);
		return 1;
	}
	return 0;
}

int flv_dm_read_frame(flv_demux_struct *flv_handle, omx_start_packet *StartPack)
{
	if (flv_handle->Status == 0) return -1;
	//if (flv_handle->Frame_Length) DBG_FREE(flv_handle->Frame);
	flv_handle->Frame_Lenght = 0;
	flv_handle->Reworked = 0;
	flv_handle->Frame_Sended = 0;
	flv_tag_t f_tag;	
	unsigned int pack_len;
	int ret = 0;
	//unsigned int uiCurrPos = flv_handle->CurrentByte;
	char cBuff[6];
	
	if (fseek(flv_handle->filehandle, flv_handle->CurrentByte, SEEK_SET) != 0) return -10;
	
	if ((flv_handle->CurrentFrame == 0) && (flv_handle->VideoEnabled))
	{
		flv_handle->CurrentByte += sizeof(flv_tag_t);
		if (flv_handle->CurrentByte > flv_handle->FileSize) return 0;
		if (fread(&f_tag, 1, sizeof(flv_tag_t), flv_handle->filehandle) != sizeof(flv_tag_t)) return -2;
			
		pack_len = f_tag.data_size[0] << 16;
		pack_len |= f_tag.data_size[1] << 8;
		pack_len |= f_tag.data_size[2];
			
		if (pack_len)
		{
			flv_handle->Frame_Type = FLV_TAG_TYPE_VIDEO;
			flv_handle->CurrentFrame++;
			flv_handle->CurrentScrollFrame++;
			flv_handle->Frame_Lenght = StartPack->StartFrameLen;
			if (flv_handle->Frame_Lenght > flv_handle->Frame_MaxSize)
			{
				flv_handle->Frame_MaxSize = flv_handle->Frame_Lenght;
				flv_handle->Frame = (char*)DBG_REALLOC(flv_handle->Frame, flv_handle->Frame_MaxSize);
			}
			memcpy(flv_handle->Frame, StartPack->BufferStartFrame, flv_handle->Frame_Lenght);
			//printf("Readed0 %i %i\n", flv_handle->CurrentFrame - 1, flv_handle->Frame_Lenght);
			flv_handle->CurrentByte += pack_len + 4;
			if (flv_handle->CurrentByte > flv_handle->FileSize) return -6;
			return 1;
		}
		return -9;		
	}
	else
	{
			//printf("Reading0 from %i of %i\n", flv_handle->CurrentByte, flv_handle->FileSize);
			flv_handle->CurrentByte += sizeof(flv_tag_t);
			if (flv_handle->CurrentByte > flv_handle->FileSize) return 0;
			if (fread(&f_tag, 1, sizeof(flv_tag_t), flv_handle->filehandle) != sizeof(flv_tag_t)) return -2;
			
			pack_len = f_tag.data_size[0] << 16;
			pack_len |= f_tag.data_size[1] << 8;
			pack_len |= f_tag.data_size[2];
			
			flv_handle->CurrentFrame++;
						
			if (f_tag.type == FLV_TAG_TYPE_AUDIO)
			{	
				flv_handle->Frame_Type = FLV_TAG_TYPE_AUDIO;
				if (!flv_handle->VideoEnabled) flv_handle->CurrentScrollFrame++;

				if (fread(cBuff, 1, 2, flv_handle->filehandle) != 2) return -9;
				
				flv_handle->Frame_Lenght = pack_len - 2;
				if (flv_handle->Frame_Lenght > flv_handle->Frame_MaxSize)
				{
					dbgprintf(2, "Add mem for file stream %i\n", flv_handle->Frame_Lenght);
					flv_handle->Frame_MaxSize = flv_handle->Frame_Lenght;
					flv_handle->Frame = (char*)DBG_REALLOC(flv_handle->Frame, flv_handle->Frame_MaxSize);
				}
				if (fread(flv_handle->Frame, 1, flv_handle->Frame_Lenght, flv_handle->filehandle) != flv_handle->Frame_Lenght) return -10;									
				ret = 1;
			}
			//printf("Reading1 type:%i len:%i\n", f_tag.type, pack_len);
			if ((f_tag.type == FLV_TAG_TYPE_VIDEO) && (flv_handle->VideoEnabled))
			{
				if (pack_len >= 5)
				{
					flv_handle->Frame_Type = FLV_TAG_TYPE_VIDEO;
					flv_handle->CurrentScrollFrame++;
									
					if (fread(cBuff, 1, 5, flv_handle->filehandle) != 5) return -3;
					if (((cBuff[0] == 0x17) || (cBuff[0] == 0x27)) && (cBuff[1] == 1) && (cBuff[2] == 0) && (cBuff[3] == 0) && (cBuff[4] == 0))
					{
						if (cBuff[0] == 0x17) flv_handle->Frame_Key = 1; else flv_handle->Frame_Key = 0;
						if (fread(cBuff, 1, 4, flv_handle->filehandle) != 4) return -4;
						flv_handle->Frame_Lenght = cBuff[0] << 24;
						flv_handle->Frame_Lenght |= cBuff[1] << 16;
						flv_handle->Frame_Lenght |= cBuff[2] << 8;
						flv_handle->Frame_Lenght |= cBuff[3];
						if (pack_len > flv_handle->Frame_Lenght)
						{
							flv_handle->Frame_Lenght += 4;						
							if (flv_handle->Frame_Lenght > flv_handle->Frame_MaxSize)
							{
								flv_handle->Frame_MaxSize = flv_handle->Frame_Lenght;
								flv_handle->Frame = (char*)DBG_REALLOC(flv_handle->Frame, flv_handle->Frame_MaxSize);
							}
							flv_handle->Frame[0] = 0;
							flv_handle->Frame[1] = 0;
							flv_handle->Frame[2] = 0;
							flv_handle->Frame[3] = 1;
							if (fread(&flv_handle->Frame[4], 1, (flv_handle->Frame_Lenght - 4), flv_handle->filehandle) != (flv_handle->Frame_Lenght - 4)) return -5;
							//printf("Readed1 %i %i\n", flv_handle->CurrentFrame - 1, flv_handle->Frame_Lenght);	
							ret = 1;
						} else dbgprintf(3, "Wrong flv format, frame_len wrong\n");
					} else dbgprintf(3, "Wrong flv format, header wrong\n");
				}
			}
			
			flv_handle->CurrentByte += pack_len + 4;
			if (flv_handle->CurrentByte > flv_handle->FileSize) return -6;
		return ret;
	}
	return -8;
}

int flv_dm_fill_map(flv_demux_struct *flv_handle)
{
	flv_tag_t f_tag;
	
	unsigned int pack_len;
	unsigned int uiCurrPos = 13;
	
	char cBuff[5];
	memset(cBuff, 0, 5);
	unsigned int uiFrameNum = 0;
	unsigned int uiFrameGlobalNum = 0;
	unsigned int uiMapLenght = 0;
	if (flv_handle->VideoEnabled) 
		uiMapLenght = flv_handle->VideoKeyFrames;
		else
		uiMapLenght = flv_handle->AudioFrames;	
	map_struct *msFullScrollMap = (map_struct*)DBG_MALLOC(sizeof(map_struct) * uiMapLenght);
	memset(msFullScrollMap, 0, sizeof(map_struct) * uiMapLenght);
	
	if (fseek(flv_handle->filehandle, uiCurrPos, SEEK_SET) == 0)
	{
		do
		{
			uiCurrPos += sizeof(flv_tag_t);
			if (uiCurrPos >= flv_handle->FileSize) break;
			if (fread(&f_tag, 1, sizeof(flv_tag_t), flv_handle->filehandle) != sizeof(flv_tag_t)) break;
			
			pack_len = f_tag.data_size[0] << 16;
			pack_len |= f_tag.data_size[1] << 8;
			pack_len |= f_tag.data_size[2];
						
			if ((f_tag.type != FLV_TAG_TYPE_META) && (f_tag.type != FLV_TAG_TYPE_AUDIO) && (f_tag.type != FLV_TAG_TYPE_VIDEO)) break;
			if (pack_len)
			{				
				if ((flv_handle->VideoEnabled) && (f_tag.type == FLV_TAG_TYPE_VIDEO))
				{
					cBuff[0] = 0;
					cBuff[1] = 0;
					if (fread(cBuff, 1, 2, flv_handle->filehandle) != 2) break;
				}
				if (((flv_handle->VideoEnabled) && (f_tag.type == FLV_TAG_TYPE_VIDEO) && (cBuff[0] == 0x17) && (cBuff[1] == 1))
					|| 
					((!flv_handle->VideoEnabled) && (f_tag.type == FLV_TAG_TYPE_AUDIO)))
				{	
					if (uiFrameNum >= uiMapLenght)
					{
						dbgprintf(2, "Error calc Scroll map %i %i\n", uiFrameNum, uiMapLenght);
						break;
					}
					msFullScrollMap[uiFrameNum].FramePos = uiCurrPos - sizeof(flv_tag_t);
					msFullScrollMap[uiFrameNum].FrameNum = uiFrameGlobalNum;
					//printf("Map %i %i\n", uiFrameGlobalNum, msFullScrollMap[uiFrameNum].FramePos);
					uiFrameNum++;					
				}
				if (((flv_handle->VideoEnabled) && (f_tag.type == FLV_TAG_TYPE_VIDEO) && (cBuff[1] == 1))
					|| 
					((!flv_handle->VideoEnabled) && (f_tag.type == FLV_TAG_TYPE_AUDIO)))
					{
						uiFrameGlobalNum++;
					}				
			}
			
			uiCurrPos += pack_len + 4;
			if (uiCurrPos > flv_handle->FileSize) break;
			
			if (fseek(flv_handle->filehandle, uiCurrPos, SEEK_SET) != 0) break;
		} while(1);
		
		flv_handle->ScrollMap = (map_struct*)DBG_MALLOC(sizeof(map_struct) * 1000);
		memset(flv_handle->ScrollMap, 0, sizeof(map_struct) * 1000);
		int i;
		float uiStep = (float)(uiMapLenght) / 1000;
		float fCounter = 0;
		for (i = 0; i < 1000; i++)
		{
			flv_handle->ScrollMap[i].FramePos = msFullScrollMap[(unsigned int)fCounter].FramePos;
			flv_handle->ScrollMap[i].FrameNum = msFullScrollMap[(unsigned int)fCounter].FrameNum;
			//printf("Map %i %i %i\n", i, flv_handle->ScrollMap[i].FramePos, flv_handle->ScrollMap[i].FrameNum);
			fCounter += uiStep;
		}
		DBG_FREE(msFullScrollMap);
		return 1;
	}
	return 0;
}

int flv_dm_set_pos(flv_demux_struct *flv_handle, unsigned int uiPos)
{
	if (flv_handle->Status == 0)
	{
		dbgprintf(2, "Wrong Status file, new pos for camera file\n");
		return 0;
	}
	if ((uiPos < 0) || (uiPos >= 1000))
	{
		dbgprintf(2, "Wrong new pos for camera file\n");
		return 0;
	}
	//printf("flv_dm_set_pos %i %i\n", (int)flv_handle->ScrollMap, uiPos);
	flv_handle->CurrentByte = flv_handle->ScrollMap[uiPos].FramePos;
	flv_handle->CurrentFrame = flv_handle->ScrollMap[uiPos].FrameNum;
	flv_handle->CurrentScrollFrame = flv_handle->ScrollMap[uiPos].FrameNum;
	return 1;
}

