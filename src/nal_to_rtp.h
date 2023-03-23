#if !defined(__H264_TO_RTP__)
#define __H264_TO_RTP__

#if defined(cplusplus)
extern "C"
{
#endif 

#define RTP_PAYLOAD_SIZE    1436

// RTP header definition. 
typedef struct
{
    unsigned char   version_p_x_cc; 
    unsigned char   m_pt; 
    unsigned short  sequence_number; 
    unsigned int    timestamp; 
    unsigned short  ssrc; 
    unsigned short  csrc; 
} sRTP_HEADER; 


// RTP payload definition. 
typedef struct
{
    unsigned char   bytes[RTP_PAYLOAD_SIZE]; 

} sRTP_PAYLOAD; 


// RTP packet definition. 
typedef struct
{
    sRTP_HEADER     header; 
    sRTP_PAYLOAD    payload; 

} sRTP_PKT; 

// RTP packet node definition. 
typedef struct sRTP_PKT_NODE
{
    struct sRTP_PKT_NODE   *next;           ///< Next packet
    sRTP_PKT                rtp_pkt;        ///< Packet
    unsigned int            rtp_pkt_len;    ///< Packet length

} sRTP_PKT_NODE;

// Module control block. 
typedef struct
{
    unsigned int    timestamp; 
	//unsigned int    timestampfirst;
	//unsigned int    first_seq_number;
    unsigned int    sequence_number;
	//unsigned int    timestampfirststep;
	unsigned int    timestampstep;
	unsigned char   streamnum;
	unsigned char   typedata;
	int64_t			previous_mks;
} RTP_CBLK; 

extern void * sx_nal_to_rtp_util_create(unsigned char ucTypeData, unsigned char ucStreamNum, unsigned char ucFrameRate);
extern void sx_nal_to_rtp_util_destroy(void *arg);
extern sRTP_PKT_NODE * sx_nal_to_rtp_util_get(RTP_CBLK  *cblk, unsigned char  *h264_frame, unsigned int h264_frame_len); 
extern sRTP_PKT_NODE * sx_aac_to_rtp_util_get(RTP_CBLK *cblk, unsigned char *aac_frame, unsigned int aac_frame_len);
extern sRTP_PKT_NODE * sx_ac3_to_rtp_util_get(RTP_CBLK *cblk, unsigned char *ac3_frame, unsigned int ac3_frame_len);
extern void sx_nal_to_rtp_util_free(sRTP_PKT_NODE * head);
 
#if defined(cplusplus)
}
#endif 

#endif // #if !defined(__H264_TO_RTP__)

