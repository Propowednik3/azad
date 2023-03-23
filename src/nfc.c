#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
//#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "bcm_host.h"
#include "gpio.h" 
#include "nfc.h" 
#include "debug.h"

unsigned char read_sta(void);
unsigned char write_cmd_check_ack(unsigned char *cmd, unsigned char len);
unsigned char wait_ready(unsigned char ms);
unsigned char read_ack(void);

unsigned char hextab[17]="0123456789ABCDEF";
//unsigned char PN532_ACK[6]={0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
//unsigned char nfc_version[6]={0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};
unsigned char _uid[7];  // ISO14443A uid
unsigned char _uidLen;  // uid len
unsigned char _key[6];  // Mifare Classic key
unsigned char inListedTag; // Tg number of inlisted tag.

/** data buffer */
//unsigned char nfc_buf[NFC_CMD_BUF_LEN];
unsigned char pn532_packetbuffer[NFC_CMD_BUF_LEN];
unsigned char nfc_buf[NFC_CMD_BUF_LEN];
unsigned char command = 0;

void PN532_wakeup(int iPort)
{
	unsigned char send_buff[32];
	memset(send_buff, 0, 32);
	send_buff[0] = PN532_WAKEUP;
	send_buff[1] = PN532_WAKEUP;	
	uart_write_port(iPort, (char*)send_buff, 16, 0);	
	while (uart_read_port(iPort, (char*)send_buff, 1, 0) > 0);
}

int8_t PN532_writeCommand(int iPort, const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen)
{
	while (uart_read_port(iPort, (char*)nfc_buf, 1, 0) > 0);
	command = header[0];
	unsigned char cnt = 0;
	nfc_buf[cnt++] = PN532_PREAMBLE;
    nfc_buf[cnt++] = PN532_STARTCODE1;
    nfc_buf[cnt++] = PN532_STARTCODE2;
    
    unsigned char length = hlen + blen + 1;   // length of data field: TFI + DATA
    nfc_buf[cnt++] = length;
    nfc_buf[cnt++] = ~length + 1;                 // checksum of length
    
    nfc_buf[cnt++] = PN532_HOSTTOPN532;
    unsigned char sum = PN532_HOSTTOPN532;    // sum of TFI + DATA
    
    //DMSG("write: ");
    unsigned char i;
    for (i = 0; i < hlen; i++) 
	{
        nfc_buf[cnt++] = header[i];
		sum += header[i];            
        //DMSG_HEX(header[i]);
    }
    //DMSG("\n");
	for (i = 0; i < blen; i++) 
	{
		nfc_buf[cnt++] = body[i];
        sum += body[i];            
        //DMSG_HEX(body[i]);
    }
	//DMSG("\n");
  
    unsigned char checksum = ~sum + 1;            // checksum of TFI + DATA
    nfc_buf[cnt++] = checksum;
	nfc_buf[cnt++] = PN532_POSTAMBLE;
	//PN532_PrintHex(nfc_buf, cnt);
	//DMSG("\n");
	uart_write_port(iPort, (char*)nfc_buf, cnt, 0);
    //DMSG("\n");

    return PN532_readAckFrame(iPort);
}

unsigned char PN532_readAckFrame(int iPort)
{
	const uint8_t PN532_ACK[] = {0, 0, 0xFF, 0, 0xFF, 0};
    unsigned char ackBuf[sizeof(PN532_ACK)];
    int ret;
    //DMSG("\nAck: "); 
	memset(ackBuf, 0, sizeof(PN532_ACK));
    ret = uart_read_port(iPort, (char*)ackBuf, sizeof(PN532_ACK), PN532_ACK_WAIT_TIME);
	if (ret < sizeof(PN532_ACK))
	{
        DMSG("Timeout Ack\n");
        return PN532_TIMEOUT;
    }
    if (memcmp(ackBuf, PN532_ACK, sizeof(PN532_ACK)))
	{
        DMSG("Invalid\n");
		PN532_PrintHex(ackBuf,sizeof(PN532_ACK));
        return PN532_INVALID_ACK;
    }// else DMSG("Ok\n");
    return 0;
}

int PN532_readResponse(int iPort, unsigned char *buf, unsigned char len, unsigned int timeout)
{
    unsigned char tmp[3];
    
    //DMSG("\nRead:  \n");
    
    /** Frame Preamble and Start Code */
    if (uart_read_port(iPort, (char*)tmp, 3, timeout) < 3) return PN532_TIMEOUT;
	
    if ((0 != tmp[0]) || (0 != tmp[1]) || (0xFF != tmp[2]))
	{
        DMSG("Preamble error");
        return PN532_INVALID_FRAME;
    }
    
    /** receive length and check */
    unsigned char length[2];
    if (uart_read_port(iPort, (char*)length, 2, timeout) < 2) return PN532_TIMEOUT;
    if (0 != (unsigned char)(length[0] + length[1]))
	{
        DMSG("Length error");
        return PN532_INVALID_FRAME;
    }
    length[0] -= 2;
    if (length[0] > len) return PN532_NO_SPACE;
    
    /** receive command byte */
    unsigned char cmd = command + 1;               // response command
    if (uart_read_port(iPort, (char*)tmp, 2, timeout) < 2) return PN532_TIMEOUT;
    if ((PN532_PN532TOHOST != tmp[0]) || (cmd != tmp[1]))
	{
        DMSG("Command error");
        return PN532_INVALID_FRAME;
    }
    
    if (uart_read_port(iPort, (char*)buf, length[0], timeout) != length[0]) return PN532_TIMEOUT;
    unsigned char sum = PN532_PN532TOHOST + cmd;
	unsigned char i;
    for (i=0; i<length[0]; i++) sum += buf[i];
    
    /** checksum and postamble */
    if (uart_read_port(iPort, (char*)tmp, 2, timeout) < 2) return PN532_TIMEOUT;
    if (0 != (unsigned char)(sum + tmp[0]) || 0 != tmp[1])
	{
        DMSG("Checksum error");
        return PN532_INVALID_FRAME;
    }    
    return length[0];
}

unsigned long PN532_getFirmwareVersion(int iPort)
{
    unsigned long response;

    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
	
    if (PN532_writeCommand(iPort, pn532_packetbuffer, 1, NULL, 0)) return 0;

    // read data packet
    int status = PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);
    if (0 > status) return 0;

    response = pn532_packetbuffer[0];
    response <<= 8;
    response |= pn532_packetbuffer[1];
    response <<= 8;
    response |= pn532_packetbuffer[2];
    response <<= 8;
    response |= pn532_packetbuffer[3];

    return response;
}

void PN532_PrintHexChar(unsigned char data)
{
    dbgprintf(4,"%c", hextab[(data>>4)&0x0F]);
    dbgprintf(4,"%c", hextab[data&0x0F]);
    dbgprintf(4," ");
}

void PN532_PrintHex(unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for(i=0; i<len; i++)
    {
        dbgprintf(4,"%c", hextab[(buf[i]>>4)&0x0F]);
		dbgprintf(4,"%c", hextab[buf[i]&0x0F]);
		dbgprintf(4," ");
    }
}

char PN532_SAMConfig(int iPort)
{
    pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!

    DMSG("SAMConfig\n");

    if (PN532_writeCommand(iPort, pn532_packetbuffer, 4, NULL, 0)) return 0;

    return (0 < PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), PN532_ACK_WAIT_TIME));
}

uint8_t PN532_mifareclassic_AuthenticateBlock(int iPort, uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData)
{
    uint8_t i;

    // Hang on to the key and uid data
    memcpy (_key, keyData, 6);
    memcpy (_uid, uid, uidLen);
    _uidLen = uidLen;

    // Prepare the authentication command //
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;   /* Data Exchange Header */
    pn532_packetbuffer[1] = 1;                              /* Max card numbers */
    pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
    pn532_packetbuffer[3] = blockNumber;                    /* Block Number (1K = 0..63, 4K = 0..255 */
    memcpy (pn532_packetbuffer + 4, _key, 6);
    for (i = 0; i < _uidLen; i++) {
        pn532_packetbuffer[10 + i] = _uid[i];              /* 4 bytes card ID */
    }

    if (PN532_writeCommand(iPort, pn532_packetbuffer, 10 + _uidLen, NULL, 0))
        return 0;

    // Read the response packet
    PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), PN532_ACK_WAIT_TIME);

    // Check if the response is valid and we are authenticated???
    // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
    // Mifare auth error is technically byte 7: 0x14 but anything other and 0x00 is not good
    if (pn532_packetbuffer[0] != 0x00) 
	{
        DMSG("Authentification failed\n");
        return 0;
    }
    return 1;
}

int PN532_readPassiveTargetID(int iPort, uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout)
{
	PN532_wakeup(iPort);
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;

    if (PN532_writeCommand(iPort, pn532_packetbuffer, 3, NULL, 0)) return -1;  // command failed

    // read data packet
    if (PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), timeout) < 0) return -2;

    // check some basic stuff
    /* ISO14443A card response should be in the following format:

      byte            Description
      -------------   ------------------------------------------
      b0              Tags Found
      b1              Tag Number (only one used in this example)
      b2..3           SENS_RES
      b4              SEL_RES
      b5              NFCID Length
      b6..NFCIDLen    NFCID
    */

    if (pn532_packetbuffer[0] != 1) return -3;

    uint16_t sens_res = pn532_packetbuffer[2];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[3];

    DMSG("ATQA: 0x");  DMSG_HEX(sens_res);
    DMSG("SAK: 0x");  DMSG_HEX(pn532_packetbuffer[4]);
    DMSG("\n");

    /* Card appears to be Mifare Classic */
    *uidLength = pn532_packetbuffer[5];
	uint8_t i;
    for (i = 0; i < pn532_packetbuffer[5]; i++) 
	{
        uid[i] = pn532_packetbuffer[6 + i];
    }
    return 1;
}

uint8_t PN532_mifareclassic_ReadDataBlock(int iPort, uint8_t blockNumber, uint8_t *data)
{
    DMSG("Trying to read 16 bytes from block ");
    DMSG_INT(blockNumber);

    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                      /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ;        /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = blockNumber;            /* Block Number (0..63 for 1K, 0..255 for 4K) */

    /* Send the command */
    if (PN532_writeCommand(iPort, pn532_packetbuffer, 4, NULL, 0)) return 0;

    /* Read the response packet */
    PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), PN532_ACK_WAIT_TIME);

    /* If byte 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[0] != 0x00) return 0;

    /* Copy the 16 data bytes to the output buffer        */
    /* Block content starts at byte 9 of a valid response */
    memcpy (data, pn532_packetbuffer + 1, 16);
    return 1;
}

uint8_t PN532_mifareclassic_WriteDataBlock(int iPort, uint8_t blockNumber, uint8_t *data)
{
    /* Prepare the first command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                      /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_WRITE;       /* Mifare Write command = 0xA0 */
    pn532_packetbuffer[3] = blockNumber;            /* Block Number (0..63 for 1K, 0..255 for 4K) */
    memcpy (pn532_packetbuffer + 4, data, 16);        /* Data Payload */

    /* Send the command */
    if (PN532_writeCommand(iPort, pn532_packetbuffer, 20, NULL, 0)) 
	{
        return 0;
    }

    /* Read the response packet */
    return (0 < PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), PN532_ACK_WAIT_TIME));
}

char PN532_readMifare(int iPort)
{
	PN532_SAMConfig(iPort);
	uint8_t success;
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)    
	// Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
	// 'uid' will be populated with the UID, and uidLength will indicate
	// if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
	success = PN532_readPassiveTargetID(iPort, PN532_MIFARE_ISO14443A, uid, &uidLength, 65000);  
	if (success) 
	{
		// Display some basic information about the card
		dbgprintf(4,"Found an ISO14443A card\n");
		dbgprintf(4,"  UID Length: %i bytes\n",uidLength);
		dbgprintf(4,"  UID Value: ");
		PN532_PrintHex(uid, uidLength);
		dbgprintf(4,"\n");    
		if (uidLength == 4)
		{
			// We probably have a Mifare Classic card ... 
			dbgprintf(4,"Seems to be a Mifare Classic card (4 byte UID)\n");	  
			// Now we need to try to authenticate it for read/write access
			// Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
			dbgprintf(4,"Trying to authenticate block 4 with default KEYA value\n");
			uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };	  
			// Start with block 4 (the first block of sector 1) since sector 0
			// contains the manufacturer data and it's probably better just
			// to leave it alone unless you know what you're doing
			success = PN532_mifareclassic_AuthenticateBlock(iPort, uid, uidLength, 4, 0, keya);
	  
			if (success)
			{
				dbgprintf(4,"Sector 1 (Blocks 4..7) has been authenticated\n");
				uint8_t data[16];
		
				// If you want to write something to block 4 to test with, uncomment
				// the following line and this text should be read back in a minute
				// data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
				// success = nfc.mifareclassic_WriteDataBlock (4, data);
				// Try to read the contents of block 4
				success = PN532_mifareclassic_ReadDataBlock(iPort, 4, data);
		
				if (success)
				{
					// Data seems to have been read ... spit it out
					dbgprintf(4,"Reading Block 4:\n");
					PN532_PrintHex(data,16);
					dbgprintf(4,"\n");		  
					// Wait a bit before reading the card again
					usleep(1000000);
				}
				else
				{
					dbgprintf(4,"Ooops ... unable to read the requested block.  Try another key?\n");
				}
			}
			else
			{
				dbgprintf(4,"Ooops ... authentication failed: Try another key?\n");
			}
		}
    
		if (uidLength == 7)
		{
			// We probably have a Mifare Ultralight card ...
			dbgprintf(4,"Seems to be a Mifare Ultralight tag (7 byte UID)\n");	  
			// Try to read the first general-purpose user page (#4)
			dbgprintf(4,"Reading page 4\n");
			uint8_t data[32];
			success = PN532_mifareultralight_ReadPage(iPort, 4, data);
			if (success)
			{
				// Data seems to have been read ... spit it out
				PN532_PrintHex(data, 4);
				dbgprintf(4,"\n");		
				// Wait a bit before reading the card again
				usleep(1000000);
			}
			else
			{
				dbgprintf(4,"Ooops ... unable to read the requested page!?\n");
			}
		}
	}
	return 1;
}

uint8_t PN532_mifareultralight_ReadPage(int iPort, uint8_t page, uint8_t *buffer)
{
    if (page >= 64) 
	{
        DMSG("Page value out of range\n");
        return 0;
    }

    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                   /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ;     /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = page;                /* Page Number (0..63 in most cases) */

    /* Send the command */
    if (PN532_writeCommand(iPort, pn532_packetbuffer, 4, NULL, 0)) return 0;
    /* Read the response packet */
    PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), PN532_ACK_WAIT_TIME);

    /* If byte 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[0] == 0x00) 
	{
        /* Copy the 4 data bytes to the output buffer         */
        /* Block content starts at byte 9 of a valid response */
        /* Note that the command actually reads 16 bytes or 4  */
        /* pages at a time ... we simply discard the last 12  */
        /* bytes                                              */
        memcpy (buffer, pn532_packetbuffer + 1, 4);
    } 
	else 
	{
        return 0;
    }
    // Return OK signal
    return 1;
}

char PN532_inListPassiveTarget(int iPort)
{
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;
    pn532_packetbuffer[2] = 0;

    DMSG("inList passive target\n");

    if (PN532_writeCommand(iPort, pn532_packetbuffer, 3, NULL, 0)) 
	{
        return 0;
    }
	
	int16_t status = PN532_readResponse(iPort, pn532_packetbuffer, sizeof(pn532_packetbuffer), 30000);
    if (status < 0) 
	{
        return 0;
    }

    if (pn532_packetbuffer[0] != 1) return 0;

    inListedTag = pn532_packetbuffer[1];

    return 1;
}

int PN532_deinit(int iPort)
{
	if (iPort) close(iPort);
	return 1;
}

int PN532_init(int iPortNum)
{
	int iPort = uart_init_port(iPortNum, 115200);
	if (iPort <= 0) return 0;
	int res = 0;
	
	PN532_wakeup(iPort);
	uint32_t ret = PN532_getFirmwareVersion(iPort);
	//printf("PN532_getFirmwareVersion %i\n", ret);
	if (ret)
	{
		PN532_SAMConfig(iPort);
		res = 1;
	} 
	else close(iPort);
	//printf("PN532_SAMConfig %i\n", res);
	return res ? iPort : 0;
}
/*
void * Accesser()
{
	dbgprintf(4,"Start ACCESS\n");
	PN532_wakeup();
	//unsigned int ver = PN532_getFirmwareVersion();
	//printf("NFC FW version:%i.%i.%i.%i\n", ver >> 24, (ver >> 16) & 255, (ver >> 8) & 255, ver &255);
	char ret = PN532_inListPassiveTarget();
	if (ret) dbgprintf(4,"Found something!\n");

	//PN532_readMifare();
	dbgprintf(4,"Stop ACCESS\n");
	return 0;
	
}*/
