#include <stdint.h>
#include "stdio.h" 
#include "stdlib.h" 
#include "string.h" 
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <time.h>
#include <sys/time.h>
#include "nal_to_rtp.h"

#define FUA_FRAGMENT_START  0x80
#define FUA_FRAGMENT_MIDDLE 0x00
#define FUA_FRAGMENT_END    0x40

static sRTP_PKT_NODE * get_single_aac_pkt_chain(RTP_CBLK  *cblk, unsigned char *aac_frame, unsigned int aac_frame_len);
static sRTP_PKT_NODE * get_single_ac3_pkt_chain(RTP_CBLK  *cblk, unsigned char *aac_frame, unsigned int aac_frame_len);

// Function definition to allocate a node. 
static sRTP_PKT_NODE  *node_malloc(
    void
    )
{
    sRTP_PKT_NODE  *node; 


    node = malloc(sizeof(sRTP_PKT_NODE)); 

    node->next          = NULL; 
    node->rtp_pkt_len   = 0; 

    return node;
}

int64_t get_mks(int64_t *previous_mks) 
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	if ((*previous_mks) == 0)
	{
		*previous_mks = (ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000;
		return 0;
	}
	return ((ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec) / 1000) - (*previous_mks);
}

// Set RTP header. 
static void rtp_header_set(RTP_CBLK  *cblk, sRTP_HEADER *hdr, unsigned char m_bit_set)
{
	if (m_bit_set)
	{
		if (cblk->timestampstep)
			cblk->timestamp += cblk->timestampstep;
			else
			{
				cblk->timestamp += (((unsigned int)get_mks(&cblk->previous_mks)) / 11) - 2000;
				cblk->previous_mks = 0;
				get_mks(&cblk->previous_mks);
			}
		//printf("timestamp %i %i\n", cblk->typedata, cblk->timestamp);
	}
	//printf("Type: %i Stream: %i timestamp: %i m_bit_set: %i\n", cblk->typedata, cblk->streamnum, cblk->timestamp, m_bit_set);
		
	
	hdr->version_p_x_cc     = 0x80; 
    hdr->m_pt               = m_bit_set << 7 | cblk->streamnum; 
    hdr->sequence_number    = ntohs(cblk->sequence_number);
	hdr->timestamp          = ntohl(cblk->timestamp);
	
	hdr->ssrc               = ntohs(cblk->streamnum); 
	hdr->csrc               = ntohs(cblk->streamnum + 1); 
	cblk->sequence_number++;
}	


// Set the FU-A unit header. 
static void h264_fua_header_set(
    unsigned char  *fua_header, 
    unsigned char   nri, 
    unsigned char   pos, 
    unsigned char   nal_type
    )
{
    // Construct fragment unit indicator. 

    // Set NRI. 
    fua_header[0] = nri << 5; 

    // Set NAL type FU-A. 
    fua_header[0] |= 28; 

    // Construct Fragment unit header. 

    fua_header[1] = pos; 
    fua_header[1] |= nal_type; 
}


// Get single packet slice. 
static sRTP_PKT_NODE * get_single_pkt_chain(RTP_CBLK *cblk, unsigned char *h264_frame, unsigned int h264_frame_len)
{
    sRTP_PKT_NODE   *node; 


    // Get node. 
    node = node_malloc(); 

    // Set header.
    rtp_header_set(cblk, &node->rtp_pkt.header, 1);

    // Set payload. 
    memcpy(&node->rtp_pkt.payload.bytes[0], h264_frame, h264_frame_len); 

    // Set pkt length. 
    node->rtp_pkt_len = sizeof(sRTP_HEADER) + h264_frame_len; 

    return node; 
}

static sRTP_PKT_NODE * get_single_aac_pkt_chain(RTP_CBLK  *cblk, unsigned char *aac_frame, unsigned int aac_frame_len)
{
    sRTP_PKT_NODE   *node; 


    // Get node. 
    node = node_malloc(); 

    // Set header.
    rtp_header_set(cblk, &node->rtp_pkt.header, 1);

	unsigned short aac_len = (unsigned short)aac_frame_len << 3;
	unsigned char* plen = (unsigned char*)&aac_len;
    // Set payload. 
	node->rtp_pkt.payload.bytes[0] = 0;
    node->rtp_pkt.payload.bytes[1] = 16;
	node->rtp_pkt.payload.bytes[2] = plen[1];
    node->rtp_pkt.payload.bytes[3] = plen[0];
	//printf("%i, %i %i--%i---%i %i\n", plen[0],  plen[1], (plen[0]*256+plen[1])>>3, (plen[1]*256+plen[0])>>3, aac_frame_len, aac_len);
    memcpy(&node->rtp_pkt.payload.bytes[4], aac_frame, aac_frame_len); 

    // Set pkt length. 
    node->rtp_pkt_len = sizeof(sRTP_HEADER) + aac_frame_len + 4; 

    return node; 
}

static sRTP_PKT_NODE * get_single_ac3_pkt_chain(RTP_CBLK  *cblk, unsigned char *ac3_frame, unsigned int ac3_frame_len)
{
    sRTP_PKT_NODE   *node; 


    // Get node. 
    node = node_malloc(); 

    // Set header.
    rtp_header_set(cblk, &node->rtp_pkt.header, 1);

	// Set payload. 
	node->rtp_pkt.payload.bytes[0] = 1;
    node->rtp_pkt.payload.bytes[1] = 0;
	//printf("%i, %i %i--%i---%i %i\n", plen[0],  plen[1], (plen[0]*256+plen[1])>>3, (plen[1]*256+plen[0])>>3, aac_frame_len, aac_len);
    memcpy(&node->rtp_pkt.payload.bytes[2], ac3_frame, ac3_frame_len); 

    // Set pkt length. 
    node->rtp_pkt_len = sizeof(sRTP_HEADER) + ac3_frame_len + 2; 

    return node; 
}


// Get multi packet slice. 
static sRTP_PKT_NODE * get_multi_pkt_chain(RTP_CBLK  *cblk, unsigned char *h264_frame, unsigned int h264_frame_len)
{
    unsigned int    bytes_remaining; 
    unsigned char   nri; 
    unsigned char   nal_type; 
    unsigned char   marker_bit_set; 
    unsigned int    curr_index; 
    unsigned int    pkt_size_left; 
    unsigned char   pos; 
    unsigned int    bytes_to_copy; 
    sRTP_PKT_NODE  *node; 
    sRTP_PKT_NODE  *head; 


    // Extract NRI. 
    nri         = (h264_frame[0] & 0x60) >> 5; 

    // Extract NAL type. 
    nal_type    = (h264_frame[0] & 0x1f); 

    // Header is parsed. 
    bytes_remaining = h264_frame_len - 1; 

    // Header is parsed. 
    curr_index = 1; 

    do
    {
        // Bytes left in pkt. 
        pkt_size_left = RTP_PAYLOAD_SIZE; 

        // Determine the fragment position 
        marker_bit_set = 0; 
        if(bytes_remaining == (h264_frame_len - 1))
        {
            // First iteration. 
            pos = FUA_FRAGMENT_START; 
        }
        else if (bytes_remaining < RTP_PAYLOAD_SIZE)
        {
            // Last iteratoin. 
            marker_bit_set = 1; 
            pos = FUA_FRAGMENT_END; 
        }
        else
        {
            pos = FUA_FRAGMENT_MIDDLE; 
        }

        // Locate node. 
        if(pos == FUA_FRAGMENT_START)
        {
            // First node. 
            node = node_malloc(); 

            // Cache head. 
            head = node; 
        }
        else
        {
            // Allocate next node. 
            node->next = node_malloc(); 

            // Advance. 
            node = node->next; 
        }

        // Set RTP header. 
        rtp_header_set(cblk, &node->rtp_pkt.header, marker_bit_set);

        // Set FUA unit header.
        h264_fua_header_set(&node->rtp_pkt.payload.bytes[0], 
                            nri, 
                            pos, 
                            nal_type); 

        // FUA header is 2 bytes. 
        pkt_size_left -= 2; 

        // Copy as many bytes as we can. 
        bytes_to_copy = (pkt_size_left < bytes_remaining) ? 
            pkt_size_left : bytes_remaining; 

        // Copy payload. 
        memcpy(&node->rtp_pkt.payload.bytes[2], 
                &h264_frame[curr_index], 
                bytes_to_copy); 

        node->rtp_pkt_len = sizeof(sRTP_HEADER) + 2 + bytes_to_copy; 

        // Advance h264 frame index. 
        curr_index += bytes_to_copy; 

        // Decrement bytes to copy. 
        bytes_remaining -= bytes_to_copy; 

    } while (!marker_bit_set); 

    return head; 
}


void * sx_nal_to_rtp_util_create(unsigned char ucTypeData, unsigned char ucStreamNum, unsigned char ucFrameRate)
{
    RTP_CBLK *cblk = malloc(sizeof(RTP_CBLK));

    memset(cblk, 0, sizeof(RTP_CBLK));
	
	cblk->typedata = ucTypeData;
	cblk->previous_mks = 0;
	get_mks(&cblk->previous_mks);
	
	cblk->streamnum = ucStreamNum;
	if (ucTypeData == 0) 
	{
		cblk->sequence_number = 1;
		cblk->timestamp = 0;
		//cblk->timestampfirststep = 0;
		cblk->timestampstep = ((1000000 / ucFrameRate)-2000) / 11;
		//cblk->timestampstep = (((1000000 / ucFrameRate)) / 11) - 6500;
		//cblk->timestampstep = 0;
	}
	else 
	{
		cblk->sequence_number = 1;
		cblk->timestamp = 0;
		//cblk->timestampfirststep = 1024;
		if (ucTypeData == 1) cblk->timestampstep = 1024;
		else 
			if (ucTypeData == 2) cblk->timestampstep = 1536;
				else cblk->timestampstep = 1024;
	}
	//cblk->timestamp = (unsigned int)get_mks(&cblk->previous_mks);
	
    return (void*)cblk;
}

void sx_nal_to_rtp_util_destroy(void   *ptr)
{
    free(ptr);
}


// Get chain. 
sRTP_PKT_NODE * sx_nal_to_rtp_util_get(RTP_CBLK  *cblk, unsigned char *h264_frame, unsigned int h264_frame_len)
{
   // RTP_CBLK  *cblk = arg;
	//cblk->timestamp = get_mks(&cblk->previous_mks) / 11;	
	//printf("Video %i mks\n", cblk->timestamp);
	
    if(h264_frame_len <= RTP_PAYLOAD_SIZE)
    {
        // Single packet. 
        return get_single_pkt_chain(cblk, h264_frame, h264_frame_len);
    }

    // Multi packet chain. 
    return get_multi_pkt_chain(cblk, h264_frame, h264_frame_len);
}

sRTP_PKT_NODE * sx_aac_to_rtp_util_get(RTP_CBLK  *cblk, unsigned char *aac_frame, unsigned int aac_frame_len)
{
	//cblk->timestamp = get_mks(&cblk->previous_mks)/31.25*2;	
	//printf("Audio %i mks\n", (unsigned int)get_mks(&cblk->previous_mks));
	
    if(aac_frame_len <= RTP_PAYLOAD_SIZE)
    {
        // Single packet. 
        return get_single_aac_pkt_chain(cblk, aac_frame, aac_frame_len);
    }

    // Multi packet chain. 
    return NULL; //get_multi_pkt_chain(cblk, aac_frame, aac_frame_len);
}

sRTP_PKT_NODE * sx_ac3_to_rtp_util_get(RTP_CBLK  *cblk, unsigned char *ac3_frame, unsigned int ac3_frame_len)
{
	//cblk->timestamp = get_mks(&cblk->previous_mks)/31.25*2;	
	//printf("Audio %i mks\n", (unsigned int)get_mks(&cblk->previous_mks));
	
    if(ac3_frame_len <= RTP_PAYLOAD_SIZE)
    {
        // Single packet. 
        return get_single_ac3_pkt_chain(cblk, ac3_frame, ac3_frame_len);
    }

    // Multi packet chain. 
    return NULL; //get_multi_pkt_chain(cblk, ac3_frame, ac3_frame_len);
}
 

 void sx_nal_to_rtp_util_free(
     sRTP_PKT_NODE * head 
    )
 {
     sRTP_PKT_NODE * node; 
     sRTP_PKT_NODE * temp; 


     node = head; 
     do
     {
         temp = node; 
         node = node->next; 

         free(temp); 

     } while(node != NULL); 
 }
