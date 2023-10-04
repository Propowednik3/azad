#ifndef _NFC_H_
#define _NFC_H_

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_HOSTTOPN532                   (0xD4)

// PN532 Commands
#define PN532_COMMAND_DIAGNOSE              (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_GETGENERALSTATUS      (0x04)
#define PN532_COMMAND_READREGISTER          (0x06)
#define PN532_COMMAND_WRITEREGISTER         (0x08)
#define PN532_COMMAND_READGPIO              (0x0C)
#define PN532_COMMAND_WRITEGPIO             (0x0E)
#define PN532_COMMAND_SETSERIALBAUDRATE     (0x10)
#define PN532_COMMAND_SETPARAMETERS         (0x12)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_POWERDOWN             (0x16)
#define PN532_COMMAND_RFCONFIGURATION       (0x32)
#define PN532_COMMAND_RFREGULATIONTEST      (0x58)
#define PN532_COMMAND_INJUMPFORDEP          (0x56)
#define PN532_COMMAND_INJUMPFORPSL          (0x46)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INATR                 (0x50)
#define PN532_COMMAND_INPSL                 (0x4E)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define PN532_COMMAND_INCOMMUNICATETHRU     (0x42)
#define PN532_COMMAND_INDESELECT            (0x44)
#define PN532_COMMAND_INRELEASE             (0x52)
#define PN532_COMMAND_INSELECT              (0x54)
#define PN532_COMMAND_INAUTOPOLL            (0x60)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)
#define PN532_COMMAND_TGSETGENERALBYTES     (0x92)
#define PN532_COMMAND_TGGETDATA             (0x86)
#define PN532_COMMAND_TGSETDATA             (0x8E)
#define PN532_COMMAND_TGSETMETADATA         (0x94)
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define PN532_COMMAND_TGGETTARGETSTATUS     (0x8A)

#define PN532_RESPONSE_INDATAEXCHANGE       (0x41)
#define PN532_RESPONSE_INLISTPASSIVETARGET  (0x4B)



#define PN532_MIFARE_ISO14443A              (0x00)

#define PN532_WAKEUP                        (0x55)

#define PN532_SPI_STATREAD                  (0x02)
#define PN532_SPI_DATAWRITE                 (0x01)
#define PN532_SPI_DATAREAD                  (0x03)
#define PN532_SPI_READY                     (0x01)

#define PN532_I2C_ADDRESS                   (0x48 >> 1)
#define PN532_I2C_ADDRESS_WRITE				(0x48 >> 1)
#define PN532_I2C_ADDRESS_READ				(0x49 >> 1)
#define PN532_I2C_READBIT                   (0x01)
#define PN532_I2C_BUSY                      (0x00)
#define PN532_I2C_READY                     (0x01)
#define PN532_I2C_READYTIMEOUT              (20)

#define PN532_MIFARE_ISO14443A              (0x00)

// Mifare Commands
#define MIFARE_CMD_AUTH_A                   (0x60)
#define MIFARE_CMD_AUTH_B                   (0x61)
#define MIFARE_CMD_READ                     (0x30)
#define MIFARE_CMD_WRITE                    (0xA0)
#define MIFARE_CMD_TRANSFER                 (0xB0)
#define MIFARE_CMD_DECREMENT                (0xC0)
#define MIFARE_CMD_INCREMENT                (0xC1)
#define MIFARE_CMD_RESTORE                  (0xC2)

// Prefixes for NDEF Records (to identify record type)
#define NDEF_URIPREFIX_NONE                 (0x00)
#define NDEF_URIPREFIX_HTTP_WWWDOT          (0x01)
#define NDEF_URIPREFIX_HTTPS_WWWDOT         (0x02)
#define NDEF_URIPREFIX_HTTP                 (0x03)
#define NDEF_URIPREFIX_HTTPS                (0x04)
#define NDEF_URIPREFIX_TEL                  (0x05)
#define NDEF_URIPREFIX_MAILTO               (0x06)
#define NDEF_URIPREFIX_FTP_ANONAT           (0x07)
#define NDEF_URIPREFIX_FTP_FTPDOT           (0x08)
#define NDEF_URIPREFIX_FTPS                 (0x09)
#define NDEF_URIPREFIX_SFTP                 (0x0A)
#define NDEF_URIPREFIX_SMB                  (0x0B)
#define NDEF_URIPREFIX_NFS                  (0x0C)
#define NDEF_URIPREFIX_FTP                  (0x0D)
#define NDEF_URIPREFIX_DAV                  (0x0E)
#define NDEF_URIPREFIX_NEWS                 (0x0F)
#define NDEF_URIPREFIX_TELNET               (0x10)
#define NDEF_URIPREFIX_IMAP                 (0x11)
#define NDEF_URIPREFIX_RTSP                 (0x12)
#define NDEF_URIPREFIX_URN                  (0x13)
#define NDEF_URIPREFIX_POP                  (0x14)
#define NDEF_URIPREFIX_SIP                  (0x15)
#define NDEF_URIPREFIX_SIPS                 (0x16)
#define NDEF_URIPREFIX_TFTP                 (0x17)
#define NDEF_URIPREFIX_BTSPP                (0x18)
#define NDEF_URIPREFIX_BTL2CAP              (0x19)
#define NDEF_URIPREFIX_BTGOEP               (0x1A)
#define NDEF_URIPREFIX_TCPOBEX              (0x1B)
#define NDEF_URIPREFIX_IRDAOBEX             (0x1C)
#define NDEF_URIPREFIX_FILE                 (0x1D)
#define NDEF_URIPREFIX_URN_EPC_ID           (0x1E)
#define NDEF_URIPREFIX_URN_EPC_TAG          (0x1F)
#define NDEF_URIPREFIX_URN_EPC_PAT          (0x20)
#define NDEF_URIPREFIX_URN_EPC_RAW          (0x21)
#define NDEF_URIPREFIX_URN_EPC              (0x22)
#define NDEF_URIPREFIX_URN_NFC              (0x23)

#define PN532_GPIO_VALIDATIONBIT            (0x80)
#define PN532_GPIO_P30                      (0)
#define PN532_GPIO_P31                      (1)
#define PN532_GPIO_P32                      (2)
#define PN532_GPIO_P33                      (3)
#define PN532_GPIO_P34                      (4)
#define PN532_GPIO_P35                      (5)

#define PN532_SAM_NORMAL_MODE               (0x01)
#define PN532_SAM_VIRTUAL_CARD              (0x02)
#define PN532_SAM_WIRED_CARD                (0x03)
#define PN532_SAM_DUAL_CARD                 (0x04)

#define PN532_BRTY_ISO14443A                0x00
#define PN532_BRTY_ISO14443B                0x03
#define PN532_BRTY_212KBPS                  0x01
#define PN532_BRTY_424KBPS                  0x02
#define PN532_BRTY_JEWEL                    0x04

//#define PN532DEBUG	1
//#define PN532_P2P_DEBUG
#define NFC_WAIT_TIME                       30
#define NFC_CMD_BUF_LEN                     64
#define NFC_FRAME_ID_INDEX                  6

#define PN532_PREAMBLE                (0x00)
#define PN532_STARTCODE1              (0x00)
#define PN532_STARTCODE2              (0xFF)
#define PN532_POSTAMBLE               (0x00)

#define PN532_HOSTTOPN532             (0xD4)
#define PN532_PN532TOHOST             (0xD5)

#define PN532_ACK_WAIT_TIME           (10)  // ms, timeout of waiting for ACK

#define PN532_INVALID_ACK             (-1)
#define PN532_TIMEOUT                 (-2)
#define PN532_INVALID_FRAME           (-3)
#define PN532_NO_SPACE                (-4)

#define REVERSE_BITS_ORDER(b)         b = (b & 0xF0) >> 4 | (b & 0x0F) << 4; \
                                      b = (b & 0xCC) >> 2 | (b & 0x33) << 2; \
                                      b = (b & 0xAA) >> 1 | (b & 0x55) << 1

#ifdef PN532DEBUG
#define DMSG(args...)       printf(args)
#define DMSG_STR(str)       printf(str); printf("\n")
#define DMSG_HEX(num)       printf(" %X", num)
#define DMSG_INT(num)       printf(" %i",num)
#else
#define DMSG(args...)
#define DMSG_STR(str)
#define DMSG_HEX(num)
#define DMSG_INT(num)
#endif

typedef enum{
    NFC_STA_TAG,
    NFC_STA_GETDATA,
    NFC_STA_SETDATA,
} poll_sta_type;
	
   // void begin(void);
	unsigned char 	PN532_readAckFrame(int iPort);
	int8_t 			PN532_writeCommand(int iPort, const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen);
	unsigned long 	PN532_getFirmwareVersion(int iPort);
	char 			PN532_SAMConfig(int iPort);
	int 			PN532_readResponse(int iPort, unsigned char *buf, unsigned char len, unsigned int timeout);
	uint8_t 		PN532_mifareultralight_ReadPage(int iPort, uint8_t page, uint8_t *buffer);
	uint8_t 		PN532_mifareclassic_ReadDataBlock(int iPort, uint8_t blockNumber, uint8_t *data);
	uint8_t 		PN532_mifareclassic_WriteDataBlock(int iPort, uint8_t blockNumber, uint8_t *data);
	int 			PN532_readPassiveTargetID(int iPort, uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout);
	uint8_t 		PN532_mifareclassic_AuthenticateBlock(int iPort, uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData);
	char 			PN532_inListPassiveTarget(int iPort);
	void 			PN532_wakeup(int iPort);
	void 			PN532_PrintHex(unsigned char *buf, unsigned int len);
	int 			PN532_init(int iPortNum);
	int 			PN532_deinit(int iPort);
    //void * Accesser();
    //unsigned char SAMConfiguration(unsigned char mode=PN532_SAM_NORMAL_MODE, unsigned char timeout=20, unsigned char irq=0);

    //unsigned char InListPassiveTarget(unsigned char *buf, unsigned char brty, unsigned char maxtg, unsigned char *idata);
    //unsigned char MifareAuthentication(unsigned char type, unsigned char block, unsigned char *uuid, unsigned char uuid_len, unsigned char *key);
    //unsigned char MifareReadBlock(unsigned char block, unsigned char *buf);
    /*unsigned char MifareWriteBlock(unsigned char block, unsigned char *buf);
	unsigned char InDataExchange(unsigned char mode, unsigned char tg, unsigned char *buf=NULL, unsigned char len=0);
    
    unsigned char P2PInitiatorInit();
    unsigned char P2PTargetInit();
    unsigned char P2PInitiatorTxRx(unsigned char *t_buf, unsigned char t_len, unsigned char *r_buf, unsigned char *r_len);
    unsigned char P2PTargetTxRx(unsigned char *t_buf, unsigned char t_len, unsigned char *r_buf, unsigned char *r_len);

    unsigned char TgInitAsTarget();
    unsigned char TargetPolling();
    unsigned char SetParameters(unsigned char para);
*/
    void puthex(unsigned char buf);
	void puthex_arr(unsigned char *buf, unsigned int len);

#endif