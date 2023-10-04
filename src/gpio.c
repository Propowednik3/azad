
#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "bcm_host.h"
#include <locale.h>
#include <libudev.h>
#include "main.h"
 
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#include "gpio.h" 
#include "pthread2threadx.h"
#include "network.h"
#include "text_func.h"
#include "onvif.h" 

#define GPIO_WORKS_MAX 		20
// ----- Register Definitions ----- 
// this chip only supports FM mode
#define FREQ_STEPS 10

#define USBIO_DEFAULT_SPEED_USB 115200
#define USBIO_DEFAULT_SPEED_UART 9600

#define RADIO_REG_CHIPID  0x00

#define RADIO_REG_CTRL    0x02
#define RADIO_REG_CTRL_OUTPUT 0x8000
#define RADIO_REG_CTRL_UNMUTE 0x4000
#define RADIO_REG_CTRL_MONO   0x2000
#define RADIO_REG_CTRL_BASS   0x1000
#define RADIO_REG_CTRL_SEEKUP 0x0200
#define RADIO_REG_CTRL_SEEK   0x0100
#define RADIO_REG_CTRL_RDS    0x0008
#define RADIO_REG_CTRL_NEW    0x0004
#define RADIO_REG_CTRL_RESET  0x0002
#define RADIO_REG_CTRL_ENABLE 0x0001

#define RADIO_REG_CHAN    0x03
#define RADIO_REG_CHAN_SPACE     0x0003
#define RADIO_REG_CHAN_SPACE_100 0x0000
#define RADIO_REG_CHAN_BAND      0x000C
#define RADIO_REG_CHAN_BAND_FM      0x0000
#define RADIO_REG_CHAN_BAND_JAPAN   0x0004
#define RADIO_REG_CHAN_BAND_FMWORLD 0x0008
#define RADIO_REG_CHAN_BAND_EFM 	0x000B
#define RADIO_REG_CHAN_TUNE   0x0010
//      RADIO_REG_CHAN_TEST   0x0020
#define RADIO_REG_CHAN_NR     0x7FC0

#define RADIO_REG_R4    0x04
#define RADIO_REG_R4_EM50   0x0800
//      RADIO_REG_R4_RES   0x0400
#define RADIO_REG_R4_SOFTMUTE   0x0200
#define RADIO_REG_R4_AFC   0x0100


#define RADIO_REG_VOL     0x05
#define RADIO_REG_VOL_VOL   0x000F


#define RADIO_REG_RA      0x0A
#define RADIO_REG_RA_RDS       0x8000
#define RADIO_REG_RA_RDSBLOCK  0x0800
#define RADIO_REG_RA_STEREO    0x0400
#define RADIO_REG_RA_NR        0x03FF

#define RADIO_REG_RB          0x0B
#define RADIO_REG_RB_FMTRUE   0x0100
#define RADIO_REG_RB_FMREADY  0x0080


#define RADIO_REG_RDSA   0x0C
#define RADIO_REG_RDSB   0x0D
#define RADIO_REG_RDSC   0x0E
#define RADIO_REG_RDSD   0x0F

// I2C-Address RDA Chip for sequential  Access
#define I2C_SEQ  0x10

// I2C-Address RDA Chip for Index  Access
#define I2C_INDX  0x11

#define SEP_ADDR_ANY	0xFF
#define SEP_DEV_ANY		0xFF
#define SEP_ADDR_MY		1

enum  USBIO_CMD
{
	  SEP_CMD_NULL,
	  SEP_CMD_BAD,
	  SEP_CMD_OK,
	  SEP_CMD_ECHO,
	  SEP_CMD_INIT,
	  SEP_CMD_TEST_REQUEST,
	  SEP_CMD_TEST_RESPONSE,
	  SEP_CMD_STOP,
	  SEP_CMD_GET_TYPE,
	  SEP_CMD_TYPE,
	  SEP_CMD_GET_STATUSES,
	  SEP_CMD_STATUSES,
	  SEP_CMD_ACCEPT_STATUSES,
	  SEP_CMD_SET_STATUS,
	  SEP_CMD_GET_PARAMS,
	  SEP_CMD_PARAMS,
	  SEP_CMD_GET_SENSORS_COUNT,
	  SEP_CMD_SENSORS_COUNT,
	  SEP_CMD_START,
	  SEP_CMD_DEINIT,
	  SEP_CMD_GET_IR_DATA,
	  SEP_CMD_IR_DATA,
	  SEP_CMD_NO_DATA,
	  SEP_CMD_GET_CARD_SERIAL,
	  SEP_CMD_CARD_SERIAL,
	  SEP_CMD_GET_AUTHENTICATE_CARD,
	  SEP_CMD_AUTHENTICATE_CARD,
	  SEP_CMD_STOPED,
	  SEP_CMD_STARTED,
	  SEP_CMD_WRITE_DATA_BLOCK,
	  SEP_CMD_WRITE_DATA_RESULT,
	  SEP_CMD_GET_VERSION,
	  SEP_CMD_VERSION,
	  SEP_CMD_GET_SETTINGS_DATA,
	  SEP_CMD_SET_SETTINGS_DATA,
	  SEP_CMD_SETTINGS_DATA,
	  SEP_CMD_CHANGE_SPEED,
	  SEP_CMD_TEXT_MESSAGE,
	  SEP_CMD_GET_DEGREE_ANGLE,
	  SEP_CMD_DEGREE_ANGLE,
	  SEP_CMD_GET_RAW_ANGLE,
	  SEP_CMD_RAW_ANGLE,
	  SEP_CMD_SET_ADDRESS,
	  SEP_CMD_GET_C_TEMPERATURE,
	  SEP_CMD_C_TEMPERATURE,
	  SEP_CMD_GET_RAW_TEMPERATURE,
	  SEP_CMD_RAW_TEMPERATURE,
	  SEP_CMD_GET_NUM_REVOLUTIONS,
	  SEP_CMD_NUM_REVOLUTIONS,
	  SEP_CMD_GET_RAW_SPEED,
	  SEP_CMD_RAW_SPEED,
	  SEP_CMD_GET_DOUBLE_SPEED,
	  SEP_CMD_DOUBLE_SPEED,
	  SEP_CMD_GET_ANGLE_RANGE,
	  SEP_CMD_ANGLE_RANGE,
	  SEP_CMD_SET_TWO_STATUSES,
	  SEP_CMD_SET_STATUS_EXT
};

int spi_filestream = -1;
int i2c_filestream = -1;
int uart0_filestream = -1;

char cThreadGpioStatus = 0;

TX_EVENTER *pevntGPIOWork;
pthread_mutex_t GPIOWork_mutex;

pthread_mutex_t bcm_mutex;
pthread_mutex_t i2c_mutex;
pthread_mutex_t spi_mutex;
pthread_mutex_t uart_mutex;

static pthread_t threadGPIOWork;
pthread_attr_t tattrGPIOWork;

GPIO_LIST tGPIO_List[GPIO_WORKS_MAX];
unsigned char	ucGPIO_List_Cnt;

RADIO_FREQ _freqLow;    ///< Lowest frequency of the current selected band.
RADIO_FREQ _freqHigh;   ///< Highest frequency of the current selected band.
RADIO_FREQ _freqSteps;  ///< Resulution of the tuner.
//receiveRDSFunction _sendRDS; ///< Registered RDS Function that is called on new available data.

void* GPIOWorker(void *pData);
void StartGPIOWork(void);
void StopGPIOWork(void);
void StopIRWork(void);

char i2c_set_address(int filestream, unsigned char iAddress);
unsigned char i2c_read_data(int filestream, unsigned char iAddress, char * cToBuffer, int iLenData);
char i2c_read_radio5807(unsigned char iAddress);
unsigned char i2c_write_data(int filestream, unsigned char iAddress, char * cToBuffer, int iLenData);

void i2c_scan();
unsigned char gpio_get_pin_code(unsigned char pinnum);
char uart_put(char *cdata, int ilen, MODULE_INFO *miModule);
int uart_get(char *cdata, int ilen);

int usb_gpio_reinit(MODULE_INFO *miModule);
int usb_gpio_stop_with_test(MODULE_INFO *miModule);
int usb_gpio_start(MODULE_INFO *miModule);
int usb_gpio_request_statuses(MODULE_INFO *miModule);
int usb_gpio_exchange_data(int stream, unsigned int uiCmd, unsigned char* cSendData, unsigned int uiSendSize, unsigned char* cRecvData, unsigned int *uiRecvLen, unsigned int uiMaxLen, unsigned int cTimeWaitResult);
int usb_gpio_load_settings(MODULE_INFO *miModule, uint32_t* cSettings, unsigned int uiMaxLen, unsigned int *uiSettingsLen);
char* usb_gpio_get_serialname(int iType);

void gpio_switch_on(int iPin);
void gpio_switch_off(int iPin);
char gpio_get_status(int iPin);

int uart_open(char *cPort, int iSpeed);
int uart_clear_port(int iPort);
int get_uart_speed_code(int iSpeed);
int get_uart_speed_to_code(int iSpeed);
int get_i2c_speed_to_code(int iSpeed);

void RDA5807M_setBassBoost(char switchOn);
void RDA5807M_setMono(char switchOn);
void RDA5807M_setSoftMute(char switchOn);
void RDA5807M_setBand(char newBand);
char RDA5807M_getStereo(char cReadNow);
void RDA5807M_readRegisters();
void RDA5807M_saveRegisters();
void RDA5807M_saveRegister(uint8_t regNr);

int usb_gpio_fill_settings(MODULE_INFO *miModule);

unsigned int io_GetCRC(char *buffer, unsigned int iLength)
{	
	unsigned int		CRC1 = 0;
	unsigned int		CRC2 = 0;
	unsigned int		Len;
	unsigned int		Arr[256];
	
	if (!iLength) return 0xF1F2F3F4;
	
	memset(Arr, 0, sizeof(Arr));
	//for (Len = 0; Len < iLength; Len++)
		//Arr[buffer[Len]] = Len;
	for (Len = 0; Len < iLength; Len++)
	{
		Arr[(unsigned char)buffer[Len]]++;
		CRC1 += buffer[Len];
	}

	iLength = 256;
	for (Len = 0; Len < iLength; Len++)
	{
		CRC2 ^= Arr[Len];
	}
	
	return (CRC1 ^ CRC2);
}

char spi_init(unsigned long speed)
{		
	spi_filestream = open("/dev/spidev0.0", O_RDWR);		//Open in non blocking read/write mode
	if (spi_filestream == -1)
	{
		dbgprintf(1,"Error - Unable to open SPI.  Ensure it is not in use by another application\n");
		return 0;
	}
	int ret;
	unsigned char mode = 0;
	unsigned char bits = 8;

	ret = ioctl(spi_filestream, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) perror("can't set spi mode");

	ret = ioctl(spi_filestream, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) {perror("can't get spi mode"); return 0;}
	ret = ioctl(spi_filestream, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {perror("can't set bits per word"); return 0;}
	ret = ioctl(spi_filestream, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {perror("can't get bits per word"); return 0;}
	ret = ioctl(spi_filestream, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {perror("can't set max speed hz"); return 0;}
	ret = ioctl(spi_filestream, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {perror("can't get max speed hz"); return 0;}
	
	pthread_mutex_init(&spi_mutex, NULL);   
	
	dbgprintf(3,"spi mode: %d\n", mode);
	dbgprintf(3,"bits per word: %d\n", bits);
	dbgprintf(3,"max speed: %d Hz (%d KHz)\n", (int)speed, (int)speed/1000);
	return 1;
}	

void i2c_init()
{
	i2c_close(i2c_filestream);
	cRadioRegisters = NULL;
	i2c_filestream = 0;
	pthread_mutex_init(&i2c_mutex, NULL);  		
}

void i2c_deinit()
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	if (cRadioRegisters) DBG_FREE(cRadioRegisters);
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	pthread_mutex_destroy(&i2c_mutex);	
}

int i2c_open(unsigned int baudrate)
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	if (i2c_filestream <= 0) i2c_filestream = open("/dev/i2c-1", O_RDWR);
	if (i2c_filestream <= 0) i2c_filestream = open("/dev/i2c-0", O_RDWR);
	if (i2c_filestream <= 0) dbgprintf(2,"Failed to open i2c port\n");
		else if (baudrate != 0) bcm2835_i2c_set_baudrate(baudrate);
	if (i2c_filestream <= 0) i2c_filestream = 0;
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return i2c_filestream;
}

unsigned char gpio_get_pin_code(unsigned char pinnum)
{
	unsigned char ret = 0;
	switch (pinnum)
	{
		case 3: ret = RPI_V2_GPIO_P1_03; break;
		case 5: ret = RPI_V2_GPIO_P1_05; break;
		case 7: ret = RPI_V2_GPIO_P1_07; break;
		case 8: ret = RPI_V2_GPIO_P1_08; break;
		case 10: ret = RPI_V2_GPIO_P1_10; break;
		case 11: ret = RPI_V2_GPIO_P1_11; break;
		case 12: ret = RPI_V2_GPIO_P1_12; break;
		case 13: ret = RPI_V2_GPIO_P1_13; break;
		case 15: ret = RPI_V2_GPIO_P1_15; break;
		case 16: ret = RPI_V2_GPIO_P1_16; break;
		case 18: ret = RPI_V2_GPIO_P1_18; break;
		case 19: ret = RPI_V2_GPIO_P1_19; break;
		case 21: ret = RPI_V2_GPIO_P1_21; break;
		case 22: ret = RPI_V2_GPIO_P1_22; break;
		case 23: ret = RPI_V2_GPIO_P1_23; break;
		case 24: ret = RPI_V2_GPIO_P1_24; break;
		case 26: ret = RPI_V2_GPIO_P1_26; break;	
		case 29: ret = RPI_V2_GPIO_P1_29; break;
		case 31: ret = RPI_V2_GPIO_P1_31; break;
		case 32: ret = RPI_V2_GPIO_P1_32; break;
		case 33: ret = RPI_V2_GPIO_P1_33; break;
		case 35: ret = RPI_V2_GPIO_P1_35; break;
		case 36: ret = RPI_V2_GPIO_P1_36; break;
		case 37: ret = RPI_V2_GPIO_P1_37; break;
		case 38: ret = RPI_V2_GPIO_P1_38; break;
		case 40: ret = RPI_V2_GPIO_P1_40; break;
	}
	return ret;
}

char i2c_set_address(int filestream, unsigned char iAddress)
{
	if (filestream <= 0) return 0;
	
	if (iAddress > 127)	return 0;
	
	if (ioctl(filestream, I2C_SLAVE, iAddress) < 0) 
	{					// Set the port options and set the address of the device we wish to speak to
		dbgprintf(2,"Unable to get bus access to talk to slave\n");
		return 0;
	}
	return 1;
}

char i2c_echo(int filestream, unsigned char iAddress)
{
	char i = 0;
	if (filestream == 0) filestream = i2c_filestream;	
	if ((filestream > 0) && (iAddress <= 127))
	{
		if (i2c_set_address(filestream, iAddress))
		{
			char buf = 33;
			if ((write(filestream, &buf, 1)) == 1) i |= 2;
			if ((read(filestream, &buf, 1)) == 1) i |= 1;
						
		}
	}
		
	return i;
}

unsigned char i2c_read_data(int filestream, unsigned char iAddress, char * cToBuffer, int iLenData)
{
	DBG_MUTEX_LOCK(&i2c_mutex);	
	if (filestream <= 0) 
	{
		DBG_MUTEX_UNLOCK(&i2c_mutex);
		return 0;
	}
	//if (iAddress > 127) return 0;
	if (iAddress < 128) i2c_set_address(filestream, iAddress);
	memset(cToBuffer, 0, iLenData);
	unsigned char ilen = read(filestream, cToBuffer, iLenData);
	if (ilen != iLenData) dbgprintf(1,"Unable to read from i2c slave\n");
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return ilen;
}

unsigned char i2c_read_register(int filestream, unsigned char iAddress, char cRegister, char* cResult, unsigned int uiLen, unsigned int uiTimeWait)
{
	DBG_MUTEX_LOCK(&i2c_mutex);	
	if (filestream <= 0) 
	{
		DBG_MUTEX_UNLOCK(&i2c_mutex);
		return 0;
	}
	unsigned char res = 0;
	if (iAddress < 128) i2c_set_address(filestream, iAddress);
	memset(cResult, 0, uiLen);
	unsigned char ilen;
	ilen = write(filestream, &cRegister, 1);
	if (ilen == 1)
	{
		if (uiTimeWait) usleep(uiTimeWait*1000);
		ilen = read(filestream, cResult, uiLen);
		if (ilen != uiLen) dbgprintf(1,"Unable to read from i2c slave register\n");
			else res = 1;
	}
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return res;
}

unsigned char i2c_write_data(int filestream, unsigned char iAddress, char * cToBuffer, int iLenData)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	if (filestream <= 0) 
	{
		DBG_MUTEX_UNLOCK(&i2c_mutex);
		return 0;
	}
	if (iAddress < 128) i2c_set_address(filestream, iAddress);
	unsigned char iLen = write(filestream, cToBuffer, iLenData);
	if (iLen != iLenData) dbgprintf(1,"Error writing to i2c slave\n");
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return iLen;
}
/*
unsigned short crc16(unsigned char *ptr, unsigned char len)
{
	unsigned short crc =0xFFFF;
	unsigned char i;
	while(len--)
	{
		crc ^=*ptr++;
		for(i=0;i<8;i++)
		{
			if(crc & 0x01)
			{
				crc>>=1;
				crc^=0xA001;
			}
			else
			{
				crc>>=1;
			}
		}
	}
	return crc;
}*/

char AM2320_test_packet(unsigned char* buff, unsigned char isize)
{
	if (buff[0] != 3) {dbgprintf(4,"wrong func %i\n", buff[0]);return 0;}
	if (buff[1] != (isize - 4)) {dbgprintf(4,"wrong size %i\n", buff[1]);return 0;}
	return 1;
	/*unsigned short *crc = (unsigned short *)&buff[isize-2];
	if (crc16(buff, isize) != *crc) {dbgprintf(1,"wrong crc %i, %i, %i, %i\n", crc16(buff, isize) >> 8, crc16(buff, isize) & 255, buff[isize-2], buff[isize-1]);return 0;}
	return 1;*/
}

char LM75_read(int filestream, unsigned int uiAddr, int *iTemp)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	int res = 1;
		
	if (filestream > 0)
	{	
		*iTemp = 0;
		unsigned char buff[8];
		int ret;
		i2c_set_address(filestream, uiAddr);
		///wake up
		write(filestream, buff, 0);
		usleep(1000);
		
		buff[0] = 0;
		ret = write(filestream, buff, 1);
		if (ret != 1) 
		{
			dbgprintf(1,"wrong write size %i\n", ret);
			res = 0;
		}
		usleep(1600);
		
		ret = read(filestream, buff, 2);
		if (ret != 2) 
		{
			dbgprintf(1,"wrong write size %i\n", ret);
			res = 0;
		}
		
		ret = (buff[0] & 127) << 3;
		ret |= buff[1] >> 5;
		if (buff[0] & 128) 
		{
			ret |= 0xFFFFFC00;
		}
		*iTemp = (int)((double)ret*1.25);
	}
		
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return res;
} 

char ADS1015_init(int filestream, unsigned int uiAddr, int iGain)
{	
	char res = 1;
	DBG_MUTEX_LOCK(&i2c_mutex);	
	if (filestream <= 0) 
	{
		DBG_MUTEX_UNLOCK(&i2c_mutex);
		return 0;
	}
	
	char cParam[3];	
	cParam[0] = 0x01; //Register addr
	cParam[1] = 0b10000000 | ((iGain & 7) << 1);
	cParam[2] = 0b10000011;
	
	if (uiAddr < 128) i2c_set_address(filestream, uiAddr);
	unsigned char ilen;
	ilen = write(filestream, cParam, 3);
	if (ilen != 3) 
	{
		dbgprintf(1,"Unable to write to i2c slave ADS1015\n");
		res = 0;
	}
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return res;
}

char ADS1015_read(int filestream, unsigned int uiAddr, int *iValue)
{	
	int res = 1;
	char rData[2];	
	*iValue = 0;
	if (!i2c_read_register(filestream, uiAddr, 0x00, rData, 2, 1)) res = 0;
	int val = rData[0] << 8;
	val |= rData[1];
	if (rData[0] & 128) val -= 0xFFFF;
	*iValue = val;
	return res;
} 

char MCP3421_init(int filestream, unsigned int uiAddr, int iMode, int iGain)
{	
	char res = 1;
	DBG_MUTEX_LOCK(&i2c_mutex);	
	if (filestream <= 0) 
	{
		DBG_MUTEX_UNLOCK(&i2c_mutex);
		return 0;
	}
	unsigned char cParam = 0b10010000 | ((iMode & 3) << 2) | (iGain & 3);
	
	if (uiAddr < 128) i2c_set_address(filestream, uiAddr);
	unsigned char ilen;
	ilen = write(filestream, &cParam, 1);
	if (ilen != 1) 
	{
		dbgprintf(1,"Unable to write to i2c slave MCP3421\n");
		res = 0;
	}
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return res;
}

char MCP3421_read(int filestream, unsigned int uiAddr, unsigned int uiMode, int *iValue)
{	
	char res = 1;
	unsigned int uiWantLen = 2;
	if (uiMode == 3) uiWantLen = 3;
	
	DBG_MUTEX_LOCK(&i2c_mutex);	
	if (filestream <= 0) 
	{
		DBG_MUTEX_UNLOCK(&i2c_mutex);
		return 0;
	}
	if (uiAddr < 128) i2c_set_address(filestream, uiAddr);
	
	int result = 0;
	char pData[3];
	memset(pData, 0, 3);
	unsigned char ilen;
	if (uiMode == 3) 
		ilen = read(filestream, pData, 3);
		else
		ilen = read(filestream, &pData[1], 2);
	if (ilen != uiWantLen) 
	{
		dbgprintf(1,"Unable to read from i2c slave MCP3421\n");
		res = 0;
	}
	else
	{
		result = pData[0] << 16;
		result |= pData[1] << 8;
		result |= pData[2];
		
		if ((uiMode < 3) && (pData[1] & 128)) result -= 0xFFFF;
		if ((uiMode == 3) && (pData[0] & 128)) result -= 0xFFFFFF;
	}
	
	*iValue = result;
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return res;
}

char AS5600_read(int filestream, unsigned int uiAddr, int *iAngleRaw, int *iAngle)
{	
	int res = 1;
	char *point;	
	point = (char*)iAngleRaw;
	*point = 0;
	if (!i2c_read_register(filestream, uiAddr, 0x0C, &point[1], 1, 1)) res = 0; 
	if (!i2c_read_register(filestream, uiAddr, 0x0D, &point[0], 1, 1)) res = 0;	
	point = (char*)iAngle;
	*point = 0;
	if (!i2c_read_register(filestream, uiAddr, 0x0E, &point[1], 1, 1)) res = 0; 
	if (!i2c_read_register(filestream, uiAddr, 0x0F, &point[0], 1, 1)) res = 0;		
	return res;
}

char HMC5883L_init(int filestream)
{	
	int res = 1;
	char buff[10];	
	memset(buff, 0, 10);
	buff[0] = 0;			//RegisterAddress	
	buff[1] |= 0b11 << 5;	//Samples 00=1  01=2  10=4  11=8
	buff[1] |= 0b101 << 2;	//DataRate 000-110  => 0.75Hz-75Hz
	buff[1] |= 0b00;		//Type
	if (i2c_write_data(filestream, I2C_ADDRESS_HMC5883L, buff, 2) != 2) res = 0;
	
	memset(buff, 0, 10);
	buff[0] = 1;			//RegisterAddress	
	buff[1] |= 0b001 << 5;	//Sensitive 000-111  => 1370-230
	if (i2c_write_data(filestream, I2C_ADDRESS_HMC5883L, buff, 2) != 2) res = 0;
	
	memset(buff, 0, 10);
	buff[0] = 2;			//RegisterAddress	
	buff[1] |= 0b0 << 7;	//Speed 0=Normal  1=3.4MHz
	buff[1] |= 0b00;		//Mode 00=Continuous 01=Single 10=StandBy 11=StandBy
	if (i2c_write_data(filestream, I2C_ADDRESS_HMC5883L, buff, 2) != 2) res = 0;
	
	usleep(10000);
	printf("HMC5883L_init %i %i\n", res, buff[1]);
	return res;
}

char HMC5883L_read(int filestream, int *iX, int *iY, int *iZ)
{	
	int res = 1;
	char buff[10];		
	
	memset(buff, 0, 10);
	if (!i2c_read_register(filestream, I2C_ADDRESS_HMC5883L, 0x02, buff, 7, 1)) res = 0; 
	else
	{
		if ((buff[0] & 0b11) != 0) res = 0;
		else
		{
			*iX = buff[2] | (buff[1] << 8);
			*iY = buff[4] | (buff[3] << 8);
			*iZ = buff[6] | (buff[5] << 8);
			if(*iX > 32767)	*iX -= 65536;
			if(*iY > 32767)	*iY -= 65536;
			if(*iZ > 32767)	*iZ -= 65536;
			//if (buff[6] & 2) res = 2;
		}
	}
	return res;
}

char AM2320_set_baudrate(int filestream, unsigned int baudrate)
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	if (filestream > 0)
	{
		unsigned char buff[8];
		i2c_set_address(filestream, I2C_ADDRESS_AM2320);
		write(filestream, buff, 0);
	}
	DBG_MUTEX_UNLOCK(&i2c_mutex);	
	return 0;
}

char AM2320_read(int filestream, int *iTemp, int *iHumid)
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	int res = 1;
	
	if (filestream > 0)
	{
		do
		{
			*iTemp = 0;
			*iHumid = 0;
			unsigned char buff[8];
			int ret;
			
			i2c_set_address(filestream, I2C_ADDRESS_AM2320);
			///wake up
			write(filestream, buff, 0);
			usleep(1000);
			
			buff[0] = 3;
			buff[1] = 0;
			buff[2] = 2;
			ret = write(filestream, buff, 3);
			if (ret != 3)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (1) error write: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (1) wrong write size %i\n", ret);
				res = 0;
				break;
			}
			usleep(1600);
			ret = read(filestream, buff, 6);
			if (ret != 6)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (1) error read: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (1) wrong read size %i\n", ret);
				res = 0;
				break;
			}
			if (AM2320_test_packet(buff, 6) == 0) {dbgprintf(4,"I2C BAD TEST PACKET1\n"); res = 0;}
			*iHumid = buff[2] * 256 + buff[3];
				
			buff[0] = 3;
			buff[1] = 2;
			buff[2] = 2;
			ret = write(filestream, buff, 3);
			if (ret != 3)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (2) error write: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (2) wrong write size %i\n", ret);
				res = 0;
				break;
			}
			usleep(1600);
			ret = read(filestream, buff, 6);
			if (ret != 6)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (2) error read: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (2) wrong read size %i\n", ret);
				res = 0;
				break;
			}
			if (AM2320_test_packet(buff, 6) == 0) {dbgprintf(4,"I2C BAD TEST PACKET2\n"); res = 0;}
			*iTemp = (buff[2] & 127) * 256 + buff[3];
			if (buff[2] & 128) *iTemp *= -1;
			
			/*DBG_MUTEX_UNLOCK(&i2c_mutex);
			usleep(300000);
			DBG_MUTEX_LOCK(&i2c_mutex);
			
			///wake up
			write(filestream, buff, 0);
			usleep(1000);
			
			buff[0] = 3;
			buff[1] = 0;
			buff[2] = 2;
			ret = write(filestream, buff, 3);
			if (ret != 3)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (3) error write: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (3) wrong write size %i\n", ret);
				res = 0;
				break;
			}
			usleep(1600);
			ret = read(filestream, buff, 6);
			if (ret != 6)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (3) error read: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (3) wrong read size %i\n", ret);
				res = 0;
				break;
			}
			if (AM2320_test_packet(buff, 6) == 0) {dbgprintf(4,"I2C BAD TEST PACKET3\n"); res = 0;}
			*iHumid = buff[2] * 256 + buff[3];
			
			buff[0] = 3;
			buff[1] = 2;
			buff[2] = 2;
			ret = write(filestream, buff, 3);
			if (ret != 3)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (4) error write: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (4) wrong write size %i\n", ret);
				res = 0;
				break;
			}
			usleep(1600);
			ret = read(filestream, buff, 6);
			if (ret != 6)
			{
				if (ret < 0)
					dbgprintf(4,"I2C AM2320 (4) error read: %s (%i)\n", strerror(errno), errno);
					else dbgprintf(4,"I2C AM2320 (4) wrong read size %i\n", ret);
				res = 0;
				break;
			}
			if (AM2320_test_packet(buff, 6) == 0) {dbgprintf(4,"I2C BAD TEST PACKET4\n"); res = 0;}
			*iTemp = (buff[2] & 127) * 256 + buff[3];
			if (buff[2] & 128) *iTemp *= -1;*/
		} while(0);
	}

	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return res;
} 

char RDA5807M_init()
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	
	if (i2c_filestream > 0)
	{
		cRadioStatus = 0;
		if (cRadioRegisters == NULL) cRadioRegisters = (uint16_t*)DBG_MALLOC(32);
		memset(cRadioRegisters, 0, 32);
		i2c_set_address(i2c_filestream, I2C_INDX);
		read(i2c_filestream, NULL, 0);
		 
		// initialize all registers
		cRadioRegisters[RADIO_REG_CHIPID] = 0x5804;  // 00 id
		cRadioRegisters[1] = 0x0000;  // 01 not used
		cRadioRegisters[RADIO_REG_CTRL] = (RADIO_REG_CTRL_OUTPUT | RADIO_REG_CTRL_RESET | RADIO_REG_CTRL_ENABLE);
		
		RDA5807M_setBand(RADIO_BAND_US_EU);
		cRadioRegisters[RADIO_REG_R4] = RADIO_REG_R4_EM50 | RADIO_REG_R4_SOFTMUTE;//  0x1800;  // 04 DE ? SOFTMUTE
		cRadioRegisters[RADIO_REG_VOL] = 0x9481; //0x9081; // 0x81D1;  // 0x82D1 / INT_MODE, SEEKTH=0110,????, Volume=1
		cRadioRegisters[6] = 0x0000;
		cRadioRegisters[7] = 0x4202;
		cRadioRegisters[8] = 0x0000;
		cRadioRegisters[9] = 0x0000;

		// reset the chip
		RDA5807M_saveRegisters();

		cRadioRegisters[RADIO_REG_CTRL] = RADIO_REG_CTRL_ENABLE;
		RDA5807M_saveRegister(RADIO_REG_CTRL);
		
		RDA5807M_setMute(1);
	}

	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return 1;
}

// switch the power off
void RDA5807M_term()
{
	DBG_MUTEX_LOCK(&i2c_mutex);
  
	RDA5807M_setVolume(0);
	cRadioRegisters[RADIO_REG_CTRL] = 0x0000;   // all bits off
	RDA5807M_saveRegisters();
	DBG_MUTEX_UNLOCK(&i2c_mutex);	
} // term


// ----- Volume control -----

void RDA5807M_setVolume(uint8_t newVolume)
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	//RDA5807M_setVolume(newVolume);
	newVolume &= RADIO_REG_VOL_VOL;
	cRadioRegisters[RADIO_REG_VOL] &= (~RADIO_REG_VOL_VOL);
	cRadioRegisters[RADIO_REG_VOL] |= newVolume;
	RDA5807M_saveRegister(RADIO_REG_VOL);
	DBG_MUTEX_UNLOCK(&i2c_mutex); 
} // setVolume()

void RDA5807M_setBassBoost(char switchOn)
{
  RDA5807M_setBassBoost(switchOn);
  uint16_t regCtrl = cRadioRegisters[RADIO_REG_CTRL];
  if (switchOn) regCtrl |= RADIO_REG_CTRL_BASS;
	else regCtrl &= (~RADIO_REG_CTRL_BASS);
  cRadioRegisters[RADIO_REG_CTRL] = regCtrl;
  RDA5807M_saveRegister(RADIO_REG_CTRL);
} // setBassBoost()

// Mono / Stereo
void RDA5807M_setMono(char switchOn)
{
  RDA5807M_setMono(switchOn);

  cRadioRegisters[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_SEEK);
  if (switchOn) cRadioRegisters[RADIO_REG_CTRL] |= RADIO_REG_CTRL_MONO;
  else cRadioRegisters[RADIO_REG_CTRL] &= ~RADIO_REG_CTRL_MONO;
  RDA5807M_saveRegister(RADIO_REG_CTRL);
} // setMono

// Switch mute mode.
void RDA5807M_setMute(char switchOn)
{
	DBG_MUTEX_LOCK(&i2c_mutex);
  
	if (switchOn) 
	{
		cRadioRegisters[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_UNMUTE);
		cRadioStatus = 1;
	}
	else
	{
		cRadioRegisters[RADIO_REG_CTRL] |= RADIO_REG_CTRL_UNMUTE;
		cRadioStatus = 0;
	}
	RDA5807M_saveRegister(RADIO_REG_CTRL);
	
	DBG_MUTEX_UNLOCK(&i2c_mutex);  
} // setMute()

// Switch softmute mode.
void RDA5807M_setSoftMute(char switchOn)
{
	if (switchOn) 
		cRadioRegisters[RADIO_REG_R4] |= (RADIO_REG_R4_SOFTMUTE);
	else 
		cRadioRegisters[RADIO_REG_R4] &= (~RADIO_REG_R4_SOFTMUTE);
	RDA5807M_saveRegister(RADIO_REG_R4);
} // setSoftMute()

// ----- Band and frequency control methods -----

// tune to new band.
void RDA5807M_setBand(char newBand) 
{
	uint16_t r = (newBand & 3) << 2;
	if (newBand == RADIO_BAND_US_EU)
	{		
		_freqLow = 8700;
		_freqHigh = 10800;
		_freqSteps = 10;
	}
	if (newBand == RADIO_BAND_JA)
	{
		_freqLow = 7600;
		_freqHigh = 9100;
		_freqSteps = 10;
	}
	if (newBand == RADIO_BAND_WORLD)
	{
		_freqLow = 7600;
		_freqHigh = 10800;
		_freqSteps = 10;
	}
	if (newBand == RADIO_BAND_EEU)
	{
		_freqLow = 6500;
		_freqHigh = 7600;
		_freqSteps = 10;
	}	
	cRadioRegisters[RADIO_REG_CHAN] = (r | RADIO_REG_CHAN_SPACE_100);
} // setBand()

// retrieve the real frequency from the chip after automatic tuning.
int RDA5807M_getFrequency(char cReadNow) 
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	int ret = 0;
	if (cRadioRegisters != NULL)
	{
		if (cReadNow) RDA5807M_readRegisters();
		uint16_t ch = cRadioRegisters[RADIO_REG_RA] & RADIO_REG_RA_NR;  
		ret = _freqLow + ((int)ch * 10);  // assume 100 kHz spacing
	}
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return ret;
}  // getFrequency

char RDA5807M_getStereo(char cReadNow)
{
  	if (cReadNow) RDA5807M_readRegisters();
	if (cRadioRegisters[RADIO_REG_RA] & RADIO_REG_RA_STEREO) return 1;  
	return 0;  
}

void RDA5807M_setFrequency(RADIO_FREQ newF) 
{
	DBG_MUTEX_LOCK(&i2c_mutex);
  
	  uint16_t newChannel;
	  uint16_t regChannel = cRadioRegisters[RADIO_REG_CHAN] & (RADIO_REG_CHAN_SPACE | RADIO_REG_CHAN_BAND);

	  if (newF < _freqLow) newF = _freqLow;
	  if (newF > _freqHigh) newF = _freqHigh;
	  newChannel = (newF - _freqLow) / 10;

	  regChannel += RADIO_REG_CHAN_TUNE; // enable tuning
	  regChannel |= newChannel << 6;
	  
	  // enable output and unmute
	  cRadioRegisters[RADIO_REG_CTRL] |= RADIO_REG_CTRL_OUTPUT | RADIO_REG_CTRL_UNMUTE | RADIO_REG_CTRL_ENABLE; //  | RADIO_REG_CTRL_NEW | RADIO_REG_CTRL_RDS
	  RDA5807M_saveRegister(RADIO_REG_CTRL);

	  cRadioRegisters[RADIO_REG_CHAN] = regChannel;
	  RDA5807M_saveRegister(RADIO_REG_CHAN);

	  // adjust Volume
	 // RDA5807M_saveRegister(RADIO_REG_VOL);
	DBG_MUTEX_UNLOCK(&i2c_mutex);  
} // setFrequency()

void RDA5807M_seekStop() 
{
	DBG_MUTEX_LOCK(&i2c_mutex); 
	cRadioRegisters[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_SEEK);
    RDA5807M_saveRegister(RADIO_REG_CTRL);
	DBG_MUTEX_UNLOCK(&i2c_mutex); 
}

// start seek mode upwards
void RDA5807M_seekUp(char toNextSender) 
{
	DBG_MUTEX_LOCK(&i2c_mutex);  
	// start seek mode
  cRadioRegisters[RADIO_REG_CTRL] |= RADIO_REG_CTRL_SEEKUP;
  cRadioRegisters[RADIO_REG_CTRL] |= RADIO_REG_CTRL_SEEK;
  RDA5807M_saveRegister(RADIO_REG_CTRL);

  if (! toNextSender) 
  {
    // stop scanning right now
    cRadioRegisters[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_SEEK);
    RDA5807M_saveRegister(RADIO_REG_CTRL);
  } // if
  DBG_MUTEX_UNLOCK(&i2c_mutex); 
} // seekUp()

// start seek mode downwards
void RDA5807M_seekDown(char toNextSender) 
{
	DBG_MUTEX_LOCK(&i2c_mutex); 
  cRadioRegisters[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_SEEKUP);
  cRadioRegisters[RADIO_REG_CTRL] |= RADIO_REG_CTRL_SEEK;
  RDA5807M_saveRegister(RADIO_REG_CTRL);

  if (! toNextSender) 
  {
    // stop scanning right now
    cRadioRegisters[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_SEEK);
    RDA5807M_saveRegister(RADIO_REG_CTRL);
  } // if
  DBG_MUTEX_UNLOCK(&i2c_mutex); 
} // seekDown()

// Load all status registers from to the chip
// registers 0A through 0F
// using the sequential read access mode.
void RDA5807M_readRegisters()
{
	if (i2c_filestream <= 0) return;
	i2c_set_address(i2c_filestream, I2C_SEQ);
	read(i2c_filestream, &cRadioRegisters[RADIO_REG_RA], 12);
	char *buff = (char*)&cRadioRegisters[RADIO_REG_RA];
	char bb = buff[0];
	buff[0] = buff[1];
	buff[1] = bb;
	//cRadioRegisters[RADIO_REG_RA] = (cRadioRegisters[RADIO_REG_RA] >> 8) & ((cRadioRegisters[RADIO_REG_RA] & 0xFF) << 8);
} 

// Save writable registers back to the chip
// The registers 02 through 06, containing the configuration
// using the sequential write access mode.
void RDA5807M_saveRegisters()
{
	if (i2c_filestream <= 0) return;
	i2c_set_address(i2c_filestream, I2C_SEQ);
	write(i2c_filestream, cRadioRegisters, 14); 
} // _saveRegisters

// Save one register back to the chip
void RDA5807M_saveRegister(uint8_t regNr)
{
	if (i2c_filestream <= 0) return;
	char buff[3];
	i2c_set_address(i2c_filestream, I2C_INDX);
	buff[0] = regNr;
	buff[1] = cRadioRegisters[regNr] >> 8; 
	buff[2] = cRadioRegisters[regNr] & 0xFF;
	write(i2c_filestream, buff, 3);
} // _saveRegister

char i2c_read_to_buff_timedate3231(unsigned char iAddress, char *cOutBuffer, int iLen)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	
	if ((i2c_filestream > 0) && (iAddress <= 127))
	{
		i2c_set_address(i2c_filestream, iAddress);
		char cBuffer[32];
		memset(cBuffer, 0, 32);	
		if ((write(i2c_filestream, cBuffer, 1)) != 1) dbgprintf(1,"Unable to write from i2c slave\n"); 
		else
		if ((read(i2c_filestream, cBuffer, 19)) != 19) dbgprintf(1,"Unable to read from i2c slave\n");
		else
		{		
			struct tm timeinfo = {0};
			
			timeinfo.tm_sec = ((cBuffer[0] >> 4) * 10) + (cBuffer[0] & 15);
			timeinfo.tm_min = ((cBuffer[1] >> 4) * 10) + (cBuffer[1] & 15);
			timeinfo.tm_hour = (((cBuffer[2] >> 4) & 3) * 10) + (cBuffer[2] & 15);
			if ((cBuffer[2] & 96) == 96) timeinfo.tm_hour += 12;
			//timeinfo.tm_wday = cBuffer[3] - 1;
			timeinfo.tm_mday = (((cBuffer[4] >> 4) * 10) + (cBuffer[4] & 15));
			timeinfo.tm_mon = ((((cBuffer[5] >> 4) & 1) * 10) + (cBuffer[5] & 15))-1;
			timeinfo.tm_year = ((cBuffer[5] >> 7) * 100) + ((cBuffer[6] >> 4) * 10) + (cBuffer[6] & 15);
			if (timeinfo.tm_year < 100) 
			{
				dbgprintf(1,"!!!!!!!!!!!!Warning Wrong RTC time!!!!!!!!!!!!!!!\n");
				dbgprintf(1,"Time :%i.%i.%i %i:%i:%i -- day=%i\n",timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,timeinfo.tm_wday+1);
			}
			if (timeinfo.tm_year < 70) timeinfo.tm_year = 70;
			memset(cOutBuffer, 0, iLen);
			strftime(cOutBuffer, iLen, "%Y-%m-%d %H:%M:%S", &timeinfo);						
		}
	}
	
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return 1;
}

char i2c_read_timedate3231(unsigned char iAddress)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	
	if ((i2c_filestream > 0) && (iAddress <= 127))
	{
		i2c_set_address(i2c_filestream, iAddress);
		char cBuffer[32];
		memset(cBuffer, 0, 32);	
		if ((write(i2c_filestream, cBuffer, 1)) != 1) dbgprintf(1,"Unable to write from i2c slave\n"); 
		else
		if ((read(i2c_filestream, cBuffer, 19)) != 19) dbgprintf(1,"Unable to read from i2c slave\n");
		else
		{		
			struct tm timeinfo = {0};
			
			timeinfo.tm_sec = ((cBuffer[0] >> 4) * 10) + (cBuffer[0] & 15);
			timeinfo.tm_min = ((cBuffer[1] >> 4) * 10) + (cBuffer[1] & 15);
			timeinfo.tm_hour = (((cBuffer[2] >> 4) & 3) * 10) + (cBuffer[2] & 15);
			if ((cBuffer[2] & 96) == 96) timeinfo.tm_hour += 12;
			//timeinfo.tm_wday = cBuffer[3] - 1;
			timeinfo.tm_mday = (((cBuffer[4] >> 4) * 10) + (cBuffer[4] & 15));
			timeinfo.tm_mon = ((((cBuffer[5] >> 4) & 1) * 10) + (cBuffer[5] & 15))-1;
			timeinfo.tm_year = ((cBuffer[5] >> 7) * 100) + ((cBuffer[6] >> 4) * 10) + (cBuffer[6] & 15);
			if (timeinfo.tm_year < 100) 
			{
				dbgprintf(1,"!!!!!!!!!!!!Warning Wrong RTC time!!!!!!!!!!!!!!!\n");
				dbgprintf(1,"Time :%i.%i.%i %i:%i:%i -- day=%i\n",timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,timeinfo.tm_wday+1);
			}
			if (timeinfo.tm_year < 70) timeinfo.tm_year = 70;	
			
			time_t tofday = mktime(&timeinfo);
			a_stime(&tofday);		
			
			char timebuff[64];
			strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);		
			dbgprintf(4, "Updated time from RTC: %s\n", timebuff);		
		}
	}
	
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return 1;
}

char i2c_read_spec_timedate3231(unsigned char iAddress, struct tm *timeinfo)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	
	if ((i2c_filestream > 0) && (iAddress <= 127))
	{
		i2c_set_address(i2c_filestream, iAddress);
		char cBuffer[32];
		memset(cBuffer, 0, 32);	
		if ((write(i2c_filestream, cBuffer, 1)) != 1) dbgprintf(1,"Unable to write from i2c slave\n"); 
		else
		if ((read(i2c_filestream, cBuffer, 19)) != 19) dbgprintf(1,"Unable to read from i2c slave\n");
		else
		{		
			memset(timeinfo, 0, sizeof(struct tm));
			
			timeinfo->tm_sec = ((cBuffer[0] >> 4) * 10) + (cBuffer[0] & 15);
			timeinfo->tm_min = ((cBuffer[1] >> 4) * 10) + (cBuffer[1] & 15);
			timeinfo->tm_hour = (((cBuffer[2] >> 4) & 3) * 10) + (cBuffer[2] & 15);
			if ((cBuffer[2] & 96) == 96) timeinfo->tm_hour += 12;
			//timeinfo->tm_wday = cBuffer[3] - 1;
			timeinfo->tm_mday = (((cBuffer[4] >> 4) * 10) + (cBuffer[4] & 15));
			timeinfo->tm_mon = ((((cBuffer[5] >> 4) & 1) * 10) + (cBuffer[5] & 15))-1;
			timeinfo->tm_year = ((cBuffer[5] >> 7) * 100) + ((cBuffer[6] >> 4) * 10) + (cBuffer[6] & 15);
			if (timeinfo->tm_year < 100) 
			{
				dbgprintf(1,"!!!!!!!!!!!!Warning Wrong RTC time!!!!!!!!!!!!!!!\n");
				dbgprintf(1,"Time :%i.%i.%i %i:%i:%i -- day=%i\n",
					timeinfo->tm_mday,timeinfo->tm_mon+1,timeinfo->tm_year+1900,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_wday+1);
			}
			if (timeinfo->tm_year < 70) timeinfo->tm_year = 70;
		}
	}
	
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return 1;
}

char i2c_write_timedate3231(unsigned char iAddress)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	int ret = 1;
	if ((i2c_filestream > 0) && (iAddress <= 127))
	{
		i2c_set_address(i2c_filestream, iAddress);
		char cBuffer[32];
		memset(cBuffer, 0, 32);
		time_t rawtime;
		struct tm stTime;
		time(&rawtime);
		localtime_r(&rawtime,&stTime);	
		int iYear = stTime.tm_year;
		
		cBuffer[0] = 0;
		cBuffer[1] |= stTime.tm_sec - (((char)(stTime.tm_sec/10))*10);
		cBuffer[1] |= ((char)(stTime.tm_sec/10) << 4);
		cBuffer[2] |= stTime.tm_min - (((char)(stTime.tm_min/10))*10);
		cBuffer[2] |= ((char)(stTime.tm_min/10) << 4);
		cBuffer[3] |= stTime.tm_hour - (((char)(stTime.tm_hour/10))*10);
		cBuffer[3] |= ((char)(stTime.tm_hour/10) << 4);
		cBuffer[4] |= stTime.tm_wday + 1;
		cBuffer[5] |= stTime.tm_mday - (((char)(stTime.tm_mday/10))*10);
		cBuffer[5] |= ((char)(stTime.tm_mday/10) << 4);	
		cBuffer[6] |= (stTime.tm_mon+1) - (((char)((stTime.tm_mon+1)/10))*10);
		cBuffer[6] |= ((char)((stTime.tm_mon+1)/10) << 4);	
		if (stTime.tm_year > 99) {cBuffer[6] |= 128; stTime.tm_year -= 100;}
		cBuffer[7] |= stTime.tm_year - (((char)(stTime.tm_year/10))*10);
		cBuffer[7] |= ((char)(stTime.tm_year/10) << 4);		
		
		//dbgprintf(1,"Time :%i.%i.%i %i:%i:%i -- day=%i\n",stTime.tm_mday,stTime.tm_mon+1,stTime.tm_year+1900,stTime.tm_hour,stTime.tm_min,stTime.tm_sec,stTime.tm_wday+1);
			
		if ((write(i2c_filestream, cBuffer, 8)) != 8) 
		{
			dbgprintf(1,"Error writing2 to i2c slave\n"); 
			ret = 0;
		} 
		else 
		{
			stTime.tm_year = iYear;
			char timebuff[64];
			strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &stTime);		
			dbgprintf(4, "RTC Updated to %s\n", timebuff);		
		}
	}
	
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return ret;
}

char i2c_write_spec_timedate3231(unsigned char iAddress, struct tm *stTime)
{	
	DBG_MUTEX_LOCK(&i2c_mutex);
	int ret = 1;
	if ((i2c_filestream > 0) && (iAddress <= 127))
	{
		i2c_set_address(i2c_filestream, iAddress);
		char cBuffer[32];
		memset(cBuffer, 0, 32);
		int iYear = stTime->tm_year;
		
		cBuffer[0] = 0;
		cBuffer[1] |= stTime->tm_sec - (((char)(stTime->tm_sec/10))*10);
		cBuffer[1] |= ((char)(stTime->tm_sec/10) << 4);
		cBuffer[2] |= stTime->tm_min - (((char)(stTime->tm_min/10))*10);
		cBuffer[2] |= ((char)(stTime->tm_min/10) << 4);
		cBuffer[3] |= stTime->tm_hour - (((char)(stTime->tm_hour/10))*10);
		cBuffer[3] |= ((char)(stTime->tm_hour/10) << 4);
		cBuffer[4] |= stTime->tm_wday + 1;
		cBuffer[5] |= stTime->tm_mday - (((char)(stTime->tm_mday/10))*10);
		cBuffer[5] |= ((char)(stTime->tm_mday/10) << 4);	
		cBuffer[6] |= (stTime->tm_mon+1) - (((char)((stTime->tm_mon+1)/10))*10);
		cBuffer[6] |= ((char)((stTime->tm_mon+1)/10) << 4);	
		if (stTime->tm_year > 99) {cBuffer[6] |= 128; stTime->tm_year -= 100;}
		cBuffer[7] |= stTime->tm_year - (((char)(stTime->tm_year/10))*10);
		cBuffer[7] |= ((char)(stTime->tm_year/10) << 4);		
		
		//dbgprintf(1,"Time :%i.%i.%i %i:%i:%i -- day=%i\n",stTime.tm_mday,stTime.tm_mon+1,stTime.tm_year+1900,stTime.tm_hour,stTime.tm_min,stTime.tm_sec,stTime.tm_wday+1);
			
		if ((write(i2c_filestream, cBuffer, 8)) != 8) 
		{
			dbgprintf(1,"Error writing2 to i2c slave\n"); 
			ret = 0;
		} 
		else 
		{
			stTime->tm_year = iYear;
			char timebuff[64];
			strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", stTime);		
			dbgprintf(4, "RTC Updated to %s\n", timebuff);		
		}
	}
	
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	return ret;
}

void i2c_scan()
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	
	if (i2c_filestream > 0)
	{
		printf("Scan I2C:");
		int ret, i;
		for(ret = 0; ret < 128;ret++)
		{
			printf("%i(",ret);
			i = i2c_echo(i2c_filestream, ret);
			if (i != 0) 
			{					
				if (i & 1) printf("r");
				//if (i = 3) dbgprintf(1,"/",ret);
				if (i & 2) printf("w");					
			}
			printf(")\n");
			usleep(100000);
		}
		printf("\n");
	} else printf("I2C not inited\n");
	DBG_MUTEX_UNLOCK(&i2c_mutex);
}

char i2c_close(int filestream)
{
	DBG_MUTEX_LOCK(&i2c_mutex);
	if (filestream > 0) close(filestream);
	DBG_MUTEX_UNLOCK(&i2c_mutex);
	return 0;
}

void StartGPIOWork(void)
{
	tx_eventer_send_event(pevntGPIOWork, EVENT_START);	
}

void StopGPIOWork(void)
{
	tx_eventer_send_event(pevntGPIOWork, EVENT_STOP);	
}

char* usb_gpio_print_cmd_name(unsigned int uiCmd)
{
	switch(uiCmd)
	{
		case SEP_CMD_NULL: return "SEP_CMD_NULL";
		case SEP_CMD_BAD: return "SEP_CMD_BAD";
		case SEP_CMD_OK: return "SEP_CMD_OK";
		case SEP_CMD_ECHO: return "SEP_CMD_ECHO";
		case SEP_CMD_INIT: return "SEP_CMD_INIT";
		case SEP_CMD_TEST_REQUEST: return "SEP_CMD_TEST_REQUEST";
		case SEP_CMD_TEST_RESPONSE: return "SEP_CMD_TEST_RESPONSE";
		case SEP_CMD_STOP: return "SEP_CMD_STOP";
		case SEP_CMD_GET_TYPE: return "SEP_CMD_GET_TYPE";
		case SEP_CMD_TYPE: return "SEP_CMD_TYPE";
		case SEP_CMD_GET_STATUSES: return "SEP_CMD_GET_STATUSES";
		case SEP_CMD_STATUSES: return "SEP_CMD_STATUSES";
		case SEP_CMD_ACCEPT_STATUSES: return "SEP_CMD_ACCEPT_STATUSES";
		case SEP_CMD_SET_STATUS: return "SEP_CMD_SET_STATUS";
		case SEP_CMD_GET_PARAMS: return "SEP_CMD_GET_PARAMS";
		case SEP_CMD_PARAMS: return "SEP_CMD_PARAMS";
		case SEP_CMD_GET_SENSORS_COUNT: return "SEP_CMD_GET_SENSORS_COUNT";
		case SEP_CMD_SENSORS_COUNT: return "SEP_CMD_SENSORS_COUNT";
		case SEP_CMD_START: return "SEP_CMD_START";
		case SEP_CMD_DEINIT: return "SEP_CMD_DEINIT";
		case SEP_CMD_GET_IR_DATA: return "SEP_CMD_GET_IR_DATA";
		case SEP_CMD_IR_DATA: return "SEP_CMD_IR_DATA";
		case SEP_CMD_NO_DATA: return "SEP_CMD_NO_DATA";
		case SEP_CMD_GET_CARD_SERIAL: return "SEP_CMD_GET_CARD_SERIAL";
		case SEP_CMD_CARD_SERIAL: return "SEP_CMD_CARD_SERIAL";
		case SEP_CMD_GET_AUTHENTICATE_CARD: return "SEP_CMD_GET_AUTHENTICATE_CARD";	
		case SEP_CMD_AUTHENTICATE_CARD: return "SEP_CMD_AUTHENTICATE_CARD";	
		case SEP_CMD_STOPED: return "SEP_CMD_STOPED";	
		case SEP_CMD_STARTED: return "SEP_CMD_STARTED";
		case SEP_CMD_WRITE_DATA_BLOCK: return "SEP_CMD_WRITE_DATA_BLOCK";
		case SEP_CMD_WRITE_DATA_RESULT: return "SEP_CMD_WRITE_DATA_RESULT";
		case SEP_CMD_GET_VERSION: return "SEP_CMD_GET_VERSION";
		case SEP_CMD_VERSION: return "SEP_CMD_VERSION";
		case SEP_CMD_GET_SETTINGS_DATA: return "SEP_CMD_GET_SETTINGS_DATA";
		case SEP_CMD_SET_SETTINGS_DATA: return "SEP_CMD_SET_SETTINGS_DATA";
		case SEP_CMD_SETTINGS_DATA: return "SEP_CMD_SETTINGS_DATA";		
		case SEP_CMD_CHANGE_SPEED: return "SEP_CMD_CHANGE_SPEED";
		case SEP_CMD_TEXT_MESSAGE: return "SEP_CMD_TEXT_MESSAGE";		
		default: return "UNKNOWN";
	}
	return "UNKNOWN";
}

char* usb_gpio_get_mode_name(unsigned int uiCode)
{
	switch(uiCode)
	{
		case 0: return "Постоянный";
		case 1: return "Импульс";
		case 2: return "Периодический";
		default: return "Неизвестный";
	}
	return "Неизвестный";
}

char* usb_gpio_get_pull_name(unsigned int uiCode)
{
	switch(uiCode)
	{
		case 0: return "Нет";
		case 1: return "+";
		case 2: return "-";
		default: return "Неизвестный";
	}
	return "Неизвестный";
}

char* Get_ADS1015_RangeName(char uiCode)
{
	switch(uiCode)
	{
		case 0: return "+6.144 V";
		case 1: return "+4.096 V";
		case 2: return "+2.048 V";
		case 3: return "+1.024 V";
		case 4: return "+0.512 V";
		case 5: return "+0.256 V";
		case 6: return "+0.256 V";
		case 7: return "+0.256 V";
		default: return "Неизвестный";
	}
	return "Неизвестный";
}

char* Get_MCP3421_AccuracyName(char uiCode)
{
	switch(uiCode)
	{
		case 0: return "12bit";
		case 1: return "14bit";
		case 2: return "16bit";
		case 3: return "18bit";
		default: return "Неизвестный";
	}
	return "Неизвестный";
}

char* Get_MCP3421_GainName(char uiCode)
{
	switch(uiCode)
	{
		case 0: return "x1";
		case 1: return "x2";
		case 2: return "x4";
		case 3: return "x8";
		default: return "Неизвестный";
	}
	return "Неизвестный";
}

char* usb_gpio_get_type_name(unsigned int uiCode)
{
	switch(uiCode)
	{
		case USBIO_PIN_SETT_TYPE_DISABLED: return "Отключено";
		case USBIO_PIN_SETT_TYPE_BUSY: return "Занят";
		case USBIO_PIN_SETT_TYPE_UNSUPPORTED: return "Не поддерживается";
		case USBIO_PIN_SETT_TYPE_WRONG_PARAMS: return "Ошибочные настройки";
		case USBIO_PIN_SETT_TYPE_INPUT: return "Вход";
		case USBIO_PIN_SETT_TYPE_OUTPUT: return "Выход";
		case USBIO_PIN_SETT_TYPE_IR_RECIEVER: return "ИК приемник";
		case USBIO_PIN_SETT_TYPE_I2C_LM75: return "Датчик температуры LM75";
		case USBIO_PIN_SETT_TYPE_I2C_AM2320: return "Датчик температуры AM2320";
		case USBIO_PIN_SETT_TYPE_RS485: return "RS485";
		case USBIO_PIN_SETT_TYPE_RS485_RC522: return "Считыватель карт RC522";
		case USBIO_PIN_SETT_TYPE_RS485_PN532: return "Считыватель карт PN532";		
		case USBIO_PIN_SETT_TYPE_I2C_MISC: return "Датчик универсальный (i2c)";
		case USBIO_PIN_SETT_TYPE_RS485_MISC: return "Датчик универсальный (rs485)";
		case USBIO_PIN_SETT_TYPE_I2C_ADS1015: return "Датчик ADC ADS1015";
		case USBIO_PIN_SETT_TYPE_I2C_MCP3421: return "Датчик ADC MCP3421";
		case USBIO_PIN_SETT_TYPE_I2C_AS5600: return "Датчик угла AS5600";
		case USBIO_PIN_SETT_TYPE_I2C_HMC5883L: return "Датчик положения HMC5883L";
		default: return "Неизвестный";
	}
	return "Неизвестный";
}

int usb_gpio_recv_cmd(int stream, uint8_t iProtVersion, uint8_t *iDevSrcAddress, uint8_t iDevDestAddress, uint8_t *iDevType, unsigned int *uiCmd, unsigned char* cData, unsigned int *uiLen, unsigned int cTimeWait, char cDontWaitIfEmpty)
{
	*uiCmd = SEP_CMD_NULL;
	int reslt = -5;
	int ret;
	int64_t full_previous_ms = 0;
	get_ms(&full_previous_ms);
	int64_t previous_ms = 0;
	get_ms(&previous_ms);
	
	unsigned int uiMaxLen = *uiLen;
	*uiLen = 0;
	uint16_t tempCmd = 0;
	uint8_t iWantSrcAddress = *iDevSrcAddress;
	uint8_t iWantDevice = *iDevType;
	uint8_t iRecvVersionProtocol = 0;
	uint8_t iRecvDeviceSrcAddress = 0;
	uint8_t iRecvDeviceDestAddress = 0;
	uint8_t iRecvDeviceType = 0;
	
	unsigned char cBuffer[21];
	unsigned char cHead[4];
	memset(cHead, 0, 3);
	cHead[0] = 170;
	cHead[1] = 170;
	cHead[2] = 170;
	cHead[3] = 85;
	memset(cBuffer, 0, 21);
	int iPosition;
	unsigned int uiTraffRecv = 0;
	unsigned int recved = 0;
	unsigned int uiRecvLen = 14;
	unsigned int uiRecvDataLen = 0;
	unsigned char* ucDestBuff = cBuffer;
	char cMode = 0;
	unsigned int cTimeWaitResult = cTimeWait;
	if (cDontWaitIfEmpty) cTimeWaitResult = 0;
	//printf("USB start %i\n", uiMaxLen);
	char loop = 0;
	do
	{
		loop = 0;
		do
		{
			ret = read(stream, &ucDestBuff[recved], uiRecvLen - recved);
			if ((ret < 0) && (errno != 11))
			{
				//printf("read:%i %i %s\n", ret, errno, strerror(errno));
				reslt = -1;
				break;
			}
			if (ret <= 0) 
			{
				if (!cTimeWaitResult)
				{
					//printf("ret %i, %s\n", strerror(errno));
					reslt = 0;
					break;
				}
				if ((unsigned int)(get_ms(&previous_ms)) >= cTimeWaitResult) 
				{
					reslt = -4;
					break;
				}
				usleep(1000);
				//printf("Waiting %i\n", (unsigned int)(get_ms(&previous_ms)));
			}
			
			if (ret > 0) 
			{
				//printf("USB read %i mode: %i waitlen: %i\n", ret, cMode, uiRecvLen);			
				uiTraffRecv += ret;
				recved += ret;
				previous_ms = 0;
				get_ms(&previous_ms);
				if (cDontWaitIfEmpty) 
				{
					cTimeWaitResult = cTimeWait;
					cDontWaitIfEmpty = 0;
				}
				if (cTimeWaitResult < 200) cTimeWaitResult = 200;
			}
			if ((cMode == 0) && (recved >= 14))
			{
				iPosition = SearchDataInBuffer((char*)ucDestBuff, recved, 0, (char*)cHead, 4) - 1;
				if (iPosition > 0) 
				{
					dbgprintf(2, "USB IO, wrong data (skipped) Cnt:%i\n", iPosition);
					recved -= iPosition;
					memmove(cBuffer, &cBuffer[iPosition], recved);
					iPosition = 0;
					if (cTimeWaitResult < 200) cTimeWaitResult = 200;
				}
				
				if (iPosition < 0) 
				{
					dbgprintf(2, "USB IO, wrong data (no header) Cnt:%i\n", recved);
					recved = 0;			
				}
				if ((iPosition == 0) && (recved >= 14))
				{					
					if (cTimeWaitResult < 400) cTimeWaitResult = 400;		
					cMode = 1;
					
					iRecvVersionProtocol = cBuffer[4];
					iRecvDeviceSrcAddress = cBuffer[5];
					iRecvDeviceDestAddress = cBuffer[6];
					iRecvDeviceType = cBuffer[7];
					memcpy(&tempCmd, &cBuffer[8], sizeof(uint16_t));
					memcpy(&uiRecvLen, &cBuffer[10], sizeof(unsigned int));
										
					if (uiRecvLen > uiMaxLen)
					{
						reslt = -2;
						dbgprintf(2, "USB IO, Wrong size packet recv:%i max:%i cmd:%s\n", uiRecvLen, uiMaxLen, usb_gpio_print_cmd_name(tempCmd));
						break;
					}
					ucDestBuff = cData;
					if ((uiRecvLen == 0) || (cData == NULL))
					{
						cMode = 2;
						ucDestBuff = cBuffer;
						uiRecvLen = 4;
						uiRecvDataLen = 0;
					}
					recved -= 14;
					//printf("USB header mode: %i packLen:%i cmd:%i(%s) recved:%i\n", cMode, uiRecvLen, tempCmd, usb_gpio_print_cmd_name(tempCmd), recved);					
				}	
			}
			if ((cMode == 1) && (recved >= uiRecvLen)) 
			{
				//printf("USB readed %i %i\n", recved, uiRecvLen);
				cMode = 2;
				ucDestBuff = cBuffer;
				recved = 0;
				uiRecvDataLen = uiRecvLen;
				uiRecvLen = 4;
			}
			if ((cMode == 2) && (recved >= uiRecvLen))
			{			
				if (recved < uiMaxLen) reslt = 2; else reslt = 1;
				break;
			}
		} while(recved < uiRecvLen);
		//if (reslt != 0) printf("Ext loop %i %i %i %i %i %i\n", reslt, uiRecvLen, cMode, ret, (unsigned int)(get_ms(&previous_ms)), cBuffer[0]);
		//if (recved) printf("recv_cmd(0): recved %i %i %i %i %i\r\n", cMode, recved, uiRecvLen, (unsigned int)(HAL_GetTick() - previous_ms) , cTimeWaitResult);

		if ((cMode == 2) && (reslt > 0))
		{
			uint32_t uiCRC = io_GetCRC((char*)cData, uiRecvDataLen);
			uint32_t uiRecvCRC;
			memcpy(&uiRecvCRC, cBuffer, sizeof(uint32_t));
			
			if (uiCRC != uiRecvCRC)
			{
				dbgprintf(2, "Wrong CRC USB GPIO stream CRC(%i!=%i) Cmd:%s %i %i\n", uiCRC, uiRecvCRC, usb_gpio_print_cmd_name(tempCmd), uiRecvDataLen, reslt);
				reslt = -3;			
			}
			else
			{
				/*printf(">> %i %i %i %i\n",
				(iProtVersion == iRecvVersionProtocol)
				,((iWantSrcAddress == SEP_ADDR_ANY) || (iRecvDeviceSrcAddress == iWantSrcAddress) || (iRecvDeviceSrcAddress == SEP_ADDR_ANY))
				,((iDevDestAddress == SEP_ADDR_ANY) || (iRecvDeviceDestAddress == iDevDestAddress) || (iRecvDeviceDestAddress == SEP_ADDR_ANY))
				,((iWantDevice == SEP_DEV_ANY) || (iRecvDeviceType == iWantDevice) || (iRecvDeviceType == SEP_DEV_ANY))
				);*/
				if ((iProtVersion == iRecvVersionProtocol)
					&& ((iWantSrcAddress == SEP_ADDR_ANY) || (iRecvDeviceSrcAddress == iWantSrcAddress) || (iRecvDeviceSrcAddress == SEP_ADDR_ANY))
					&& ((iDevDestAddress == SEP_ADDR_ANY) || (iRecvDeviceDestAddress == iDevDestAddress) || (iRecvDeviceDestAddress == SEP_ADDR_ANY))
					&& ((iWantDevice == SEP_DEV_ANY) || (iRecvDeviceType == iWantDevice) || (iRecvDeviceType == SEP_DEV_ANY)))
				{
					*uiCmd = tempCmd;
					*iDevSrcAddress = iRecvDeviceSrcAddress;
					*iDevType = iRecvDeviceType;
					if (cData && (uiMaxLen >= uiRecvDataLen))
					{
						*uiLen = uiRecvDataLen;
					}
				}
				else
				{
					reslt = 0;
					loop = 1;
					
					memset(cBuffer, 0, 20);
					recved = 0;
					uiRecvLen = 14;
					uiRecvDataLen = 0;
					ucDestBuff = cBuffer;
					cTimeWaitResult = cTimeWait;
					cMode = 0;
					/*printf("Skipped alien packet %i %i %i %i\n", 
						(iProtVersion == iRecvVersionProtocol),
						((iWantSrcAddress == SEP_ADDR_ANY) || (iRecvDeviceSrcAddress == iWantSrcAddress) || (iRecvDeviceSrcAddress == SEP_ADDR_ANY)),
						((iDevDestAddress == SEP_ADDR_ANY) || (iRecvDeviceDestAddress == iDevDestAddress) || (iRecvDeviceDestAddress == SEP_ADDR_ANY)),
						((iWantDevice == SEP_DEV_ANY) || (iRecvDeviceType == iWantDevice) || (iRecvDeviceType == SEP_DEV_ANY)));*/
				}
			}
		}
		if (loop && ((unsigned int)(get_ms(&full_previous_ms)) >= cTimeWait)) break;
	} while(loop);
	//if (reslt > 0) printf("USB done %i %i\n", reslt, uiRecvDataLen);
	if (uiTraffRecv && (reslt == 0)) dbgprintf(2, "USB IO, Not full data detected %i %i %s\n", uiTraffRecv, cTimeWaitResult, usb_gpio_print_cmd_name(tempCmd));
	//if (reslt > 0) 
		//printf("Recv CMD (%i): %i %i %s, %i %i\n", GetTickCount(), reslt, (unsigned int)(get_ms(&previous_ms)) , usb_gpio_print_cmd_name(*uiCmd), *uiLen, ((*uiLen) == 4) ? *((unsigned int*)cData): 0);
	
	return reslt;
}

/*static void get_vendor_product_with_fallback(char *vendor, int vendor_len,    char *product, int product_len,
					     libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	char sysfs_name[PATH_MAX];
	bool have_vendor, have_product;

	libusb_get_device_descriptor(dev, &desc);

	have_vendor = !!get_vendor_string(vendor, vendor_len, desc.idVendor);
	have_product = !!get_product_string(product, product_len,
			desc.idVendor, desc.idProduct);

	if (have_vendor && have_product)
		return;

	if (get_sysfs_name(sysfs_name, sizeof(sysfs_name), dev) >= 0) {
		if (!have_vendor)
			read_sysfs_prop(vendor, vendor_len, sysfs_name,
					"manufacturer");
		if (!have_product)
			read_sysfs_prop(product, product_len, sysfs_name,
					"product");
	}
}*/
/*
void print_usb_serial()
{
	setlocale(LC_CTYPE, "");
	libusb_device **list;
	struct libusb_device_descriptor desc;
	char vendor[128], product[128];
	
	ssize_t num_devs, i;

	libusb_context *ctx;
		
	int err = libusb_init(&ctx);
	if (err)
	{
		printf("unable to initialize libusb: %i\n", err);
		return;
	}
	
	num_devs = libusb_get_device_list(ctx, &list);
	if (num_devs >= 0)
	{
		for (i = 0; i < num_devs; ++i) 
		{
			libusb_device *dev = list[i];
			uint8_t bnum = libusb_get_bus_number(dev);
			uint8_t dnum = libusb_get_device_address(dev);
			uint8_t pnum = libusb_get_port_number(dev);

			libusb_get_device_descriptor(dev, &desc);
			
			printf("\tPath:%s\n", dev->device->devpath);
			printf("\tPort:%i.", pnum);
			parent_dev = dev;
			do
			{
				parent_dev = libusb_get_parent(parent_dev);
				if (parent_dev) printf("%i.", libusb_get_port_number(parent_dev));
			} while(parent_dev);
			printf("\n");
		}
	}

	libusb_free_device_list(list, 0);
	
	libusb_exit(ctx);
}*/

unsigned int uart_path_to_num(char *path)
{
	unsigned int result = 0;
	int length = strlen(path);
	int val = 0;
	
	int i;
	for (i = 0; i < length; i++)
	{		
		if (path[i] != 46)
		{
			if ((path[i] > 47) && (path[i] < 58))
			{
				val *= 10;
				val += (path[i] - 48);
			}
		}
		if ((path[i] == 46) || (i == (length-1)))
		{
			result <<= 4;
			result |= val & 0b1111;
			val = 0;
		}
	}	
	return result;
}

void get_uart_ports(uart_info **tty_list, uint32_t *tty_length)
{
	*tty_list = NULL;
	*tty_length = 0;
	
	uint32_t uart_length = 1;
	uart_info *uart_list = DBG_MALLOC(sizeof(uart_info));
	memset(uart_list, 0, sizeof(uart_info));
	uart_list[0].Port = 0;
	strcpy(uart_list[0].Path, "/dev/ttyAMA0");
	strcpy(uart_list[0].Name, "Local");
	strcpy(uart_list[0].Chain, "0");
	
	struct udev *udev;
	struct udev_device *dev, *pdev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	// create udev object 
	udev = udev_new();
	if (!udev) 
	{
		printf("Cannot create udev context.\n");
		return;
	}

	// create enumerate object 
	enumerate = udev_enumerate_new(udev);
	if (!enumerate) 
	{
		printf("Cannot create enumerate context.\n");
		return;
	}

	udev_enumerate_add_match_subsystem(enumerate, "tty");
	udev_enumerate_scan_devices(enumerate);

	// fillup device list 
	devices = udev_enumerate_get_list_entry(enumerate);
	if (!devices) 
	{
		printf("Failed to get device list.\n");
		return;
	}

	udev_list_entry_foreach(dev_list_entry, devices) 
	{
		const char *path, *chain, *device, *product;

		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		//printf("I: PATH=%s\n", path);
		pdev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
		device = udev_device_get_devnode(dev);	
		
		while(pdev && device)
		{
			chain = udev_device_get_sysattr_value(pdev, "devpath");
			product = udev_device_get_sysattr_value(pdev, "product");
			if (chain) 
			{
				uart_length++;
				uart_list = DBG_REALLOC(uart_list, sizeof(uart_info)*uart_length);
				memset(&uart_list[uart_length - 1], 0, sizeof(uart_info));
				uart_list[uart_length - 1].Port = uart_path_to_num((char*)chain);
				strcpy(uart_list[uart_length - 1].Path, device);
				strcpy(uart_list[uart_length - 1].Name, product);
				strcpy(uart_list[uart_length - 1].Chain, chain);
				/*printf("%s %s %s %i %i %i\n", uart_list[uart_length - 1].Path, 
										uart_list[uart_length - 1].Name, 
										uart_list[uart_length - 1].Chain, 
										uart_list[uart_length - 1].Port,
										uart_path_to_num("1.2.3"),
										uart_path_to_num("1.14.7.9"));	*/			
				break;
			}
			pdev = udev_device_get_parent(pdev);
		}
		/* free dev */
		udev_device_unref(dev);
	}
	// free enumerate 
	udev_enumerate_unref(enumerate);
	// free udev 
	udev_unref(udev);
	*tty_list = uart_list;
	*tty_length = uart_length;
}

int get_uart_port_path(unsigned int port, char *path, unsigned int max_len)
{	
	int result = 0;
	memset(path, 0, max_len);
	
	uart_info *tty_list;
	uint32_t tty_length;
	get_uart_ports(&tty_list, &tty_length);
	int i;
	for (i = 0; i < tty_length; i++)
		if (tty_list[i].Port == port)
		{
			result = 1;
			if (strlen(tty_list[i].Path) < max_len) 
				strcpy(path, tty_list[i].Path);
				else
				memcpy(path, tty_list[i].Path, max_len - 1);
			break;
		}
	if (tty_length) DBG_FREE(tty_list);
	return result;
}

char* get_uart_chain_name(unsigned int uiPortNum, char *chain, unsigned int chainlen)
{
	char buff[32];
	memset(chain, 0, chainlen);
	int i;
	int first = 1;
	unsigned int val;
	for (i = 0; i < 8; i++)
	{
		val = (uiPortNum & 0xF0000000) >> 28;
		uiPortNum <<= 4;
		if (val || !first)
		{
			if (!first) strcat(chain, "."); else first = 0;
			memset(buff, 0, 32);
			snprintf(buff, 32, "%i", val);
			strcat(chain, buff);
		}
	}
	return chain;
}

/*
void uart_printf_array(uint8_t *Buff, uint8_t iLen)
{
	printf("Arr: [%i] : ", iLen);
	int i;
	for (i = 0; i < iLen; i++) printf("%i ", Buff[i]);
	printf("\n");
}*/

int usb_gpio_recv_command(int stream, unsigned int *uiCmd, unsigned char* cData, unsigned int *uiLen, unsigned int cTimeWait, char cDontWaitIfEmpty)
{
	uint8_t iDevSrcAddress = SEP_ADDR_ANY;
	uint8_t iDevType = SEP_DEV_ANY;
	return usb_gpio_recv_cmd(stream, 1, &iDevSrcAddress, SEP_ADDR_MY, &iDevType, uiCmd, cData, uiLen, cTimeWait, cDontWaitIfEmpty);
}

int usb_gpio_send_cmd(int stream, uint8_t iVersion, uint8_t iDevSrcAddress, uint8_t iDevDestAddress, uint8_t iDevType, uint16_t uiCmd, unsigned char* cData, unsigned int uiLen, unsigned int cTimeWaitResult, unsigned int cResultOk)
{
	//if (uiPackLen > 256) return -1;
	unsigned char cBuffer[21];
	memset(cBuffer, 0, 21);
	cBuffer[0] = 170;
	cBuffer[1] = 170;
	cBuffer[2] = 170;
	cBuffer[3] = 85;
	cBuffer[4] = iVersion;
	cBuffer[5] = iDevSrcAddress;
	cBuffer[6] = iDevDestAddress;
	cBuffer[7] = iDevType;
	memcpy(&cBuffer[8], &uiCmd, sizeof(uint16_t));
	memcpy(&cBuffer[10], &uiLen, sizeof(uint32_t));
	int count = write_t(stream, (char*)cBuffer, 14, 1000);
	if (count != 14) return -2;
	//printf("sended1 %i\r\n", count);
	if (cData && uiLen)
	{
		count = write_t(stream, (char*)cData, uiLen, 1000);
		if (count != uiLen) return -2;
		//printf("sended2 %i\r\n", count);
	}

	uint32_t uiCRC = io_GetCRC((char*)cData, uiLen);
	count = write_t(stream, (char*)&uiCRC, 4, 1000);
	if (count != 4) return -2;
	//printf("sended3 %i\r\n", count);
	if (cTimeWaitResult)
	{
		uint32_t uiRecvCmd = 0;
		unsigned char recv_buff[64];
		uiLen = 64;
		uint8_t uiAddr = iDevDestAddress;
		uint8_t uiDev = iDevType;
		
		int i = usb_gpio_recv_cmd(stream, 1, &uiAddr, iDevSrcAddress, &uiDev, &uiRecvCmd, recv_buff, &uiLen, cTimeWaitResult, 0);
		//printf("CMD %s>>>%s  %i\n", usb_gpio_print_cmd_name(uiCmd), usb_gpio_print_cmd_name(uiRecvCmd), i);
		if (i == 0) return -3;
		if (i == -4)
		{
			dbgprintf(3, "USBIO Timeout (%ims) response for %s\n", cTimeWaitResult, usb_gpio_print_cmd_name(cResultOk));
			return 0;
		}
		if (uiRecvCmd != cResultOk) 
		{
			dbgprintf(3, "USBIO Wrong response detected %s != %s for %s\n", usb_gpio_print_cmd_name(uiRecvCmd), usb_gpio_print_cmd_name(cResultOk), usb_gpio_print_cmd_name(uiCmd));
			dbgprintf(3, "USBIO Wrong response %i len %i\n", i, uiLen);
			if (uiLen > 2) dbgprintf(3, "USBIO response %.3s %i,%i,%i\n", recv_buff, recv_buff[0], recv_buff[1], recv_buff[2]);
			uart_clear_port(stream);
			return 0;
		}
	}
	return 1;
}

int usb_gpio_send_command(int stream, uint16_t uiCmd, unsigned char* cData, unsigned int uiLen, unsigned int cTimeWaitResult, unsigned int cResultOk)
{
	int result = usb_gpio_send_cmd(stream, 1, SEP_ADDR_MY, SEP_ADDR_ANY, SEP_DEV_ANY, uiCmd, cData, uiLen, cTimeWaitResult, cResultOk);
	//printf("%i Send cmd: %s (%i) res: %i\n", GetTickCount(), usb_gpio_print_cmd_name(uiCmd), uiCmd, result);
	return result;
}
/*{
	printf("Send CMD: %s, %i\n", usb_gpio_print_cmd_name(uiCmd), uiLen);
	unsigned int uiPackLen = 13 + uiLen;
	if (uiPackLen > 256) return -1;
	unsigned char cBuffer[256];
	memset(cBuffer, 0, 256);
	uiPackLen = 9 + uiLen;
	cBuffer[0] = 0;
	cBuffer[1] = 0;
	cBuffer[2] = 0;
	cBuffer[3] = 1;
	cBuffer[4] = uiCmd;
	cBuffer[5] = (unsigned char)((uiLen >> 24) & 255);
	cBuffer[6] = (unsigned char)((uiLen >> 16) & 255);
	cBuffer[7] = (unsigned char)((uiLen >> 8) & 255);
	cBuffer[8] = (unsigned char)(uiLen & 255);
	if (cData && uiLen) memcpy(&cBuffer[9], cData, uiLen);
	unsigned int uiCRC = GetCRC((char*)cBuffer, uiPackLen);
	cBuffer[uiPackLen] = (unsigned char)((uiCRC >> 24) & 255);
	uiPackLen++;
	cBuffer[uiPackLen] = (unsigned char)((uiCRC >> 16) & 255);
	uiPackLen++;
	cBuffer[uiPackLen] = (unsigned char)((uiCRC >> 8) & 255);
	uiPackLen++;
	cBuffer[uiPackLen] = (unsigned char)(uiCRC & 255);
	uiPackLen++;
	
	int count = write(stream, cBuffer, uiPackLen);
	if (count < 0) return -2;
	if (cTimeWaitResult) 
	{
		unsigned int uiCmd = 0;
		uiLen = 256;
		int i = usb_gpio_recv_command(stream, &uiCmd, cBuffer, &uiLen, cTimeWaitResult);
		if (i == 0) return -3;
		if (uiCmd != SEP_CMD_OK) return 0;
	}
	return 1;
}*/

int usb_gpio_request_data(int stream, unsigned int uiCmd, unsigned char* cData, unsigned int uiMaxLen, unsigned int *uiLen, unsigned int cTimeWaitResult)
{	
	//printf("Request Data (%i): %s (%i)\n", GetTickCount(), usb_gpio_print_cmd_name(uiCmd), uiCmd);
	if (uiLen) *uiLen = 0;
	if (usb_gpio_send_command(stream, uiCmd, (unsigned char*)&uiMaxLen, sizeof(uiMaxLen), 0, SEP_CMD_NULL) > 0)
	{
		unsigned int uiRetCmd;
		unsigned int uiRecvLen = uiMaxLen;
		if (usb_gpio_recv_command(stream, &uiRetCmd, cData, &uiRecvLen, cTimeWaitResult, 0) > 0)
		{
			if (uiLen) *uiLen = uiRecvLen;
			return uiRetCmd;
		} else return SEP_CMD_NULL;
	}
	return SEP_CMD_NULL;
}

int usb_gpio_exchange_data(int stream, unsigned int uiCmd, unsigned char* cSendData, unsigned int uiSendSize, unsigned char* cRecvData, unsigned int *uiRecvLen, unsigned int uiMaxLen, unsigned int cTimeWaitResult)
{	
	//printf("Request Data (%i): %s (%i)\n", GetTickCount(), usb_gpio_print_cmd_name(uiCmd), uiCmd);
	if (uiRecvLen) *uiRecvLen = 0;
	if (usb_gpio_send_command(stream, uiCmd, (unsigned char*)cSendData, uiSendSize, 0, SEP_CMD_NULL) > 0)
	{
		unsigned int uiRetCmd;
		*uiRecvLen = uiMaxLen;
		if (usb_gpio_recv_command(stream, &uiRetCmd, cRecvData, uiRecvLen, cTimeWaitResult, 0) > 0)
			return uiRetCmd;
	}
	return SEP_CMD_NULL;
}

int usb_gpio_test_connect(int stream)
{	
	int result = 0;
	uint8_t buffer[128];
	int i;
	for (i = 0; i < 128; i += 1) buffer[i] = i*2;
	if (usb_gpio_send_command(stream, SEP_CMD_TEST_REQUEST, (unsigned char*)buffer, 128, 0, SEP_CMD_NULL) > 0)
	{
		unsigned int uiRetCmd;
		unsigned int uiRecvLen = 128;
		memset(buffer , 0, 128);
		if (usb_gpio_recv_command(stream, &uiRetCmd, buffer, &uiRecvLen, 1000, 0) > 0)
		{
			//printf("Recv Data (%i): %s (%i)\n", GetTickCount(), usb_gpio_print_cmd_name(uiRetCmd), uiRetCmd);
			if (uiRetCmd == SEP_CMD_TEST_RESPONSE)
			{
				for (i = 0; i < 128; i++) if (buffer[i] != (128-i)) break;
				if (i == 128) result = 1;
			}
		}
	}
	return result;
}

void usb_gpio_clean_transfer_data(MODULE_INFO *miModule)
{
	char cBuffer[32];
	int res;
	int recved = 0;
	do
	{
		res = read(miModule->InitParams[0], cBuffer, 32);
		if (res <= 0) break;
		recved += res;
	} while(1);
	if (recved) dbgprintf(3,"Cleaned usbio port: %i bytes\n", recved);
}

int usb_gpio_change_speed(int stream, unsigned int uiSpeed)
{
	if (usb_gpio_send_command(stream, SEP_CMD_CHANGE_SPEED, (unsigned char*)&uiSpeed, sizeof(unsigned int), 1000, SEP_CMD_OK) <= 0)
	{
		dbgprintf(2, "Change speed USB GPIO, Error\n");
		return 0;
	}
	return 1;
}

int usb_gpio_activate(MODULE_INFO *miModule)
{
	
/*	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_TEST, NULL, 0, 1000, SEP_CMD_OK) <= 0)
	{
		dbgprintf(2, "Tested USB GPIO connect, Error\n");
		return 0;
	}*/
	
	unsigned int uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_VERSION, (unsigned char*)&miModule->Version, sizeof(unsigned int)*4, NULL, 1000);
	if (uiRetCmd != SEP_CMD_VERSION)
	{
		dbgprintf(2, "USB GPIO get version, Error %i %s\n", uiRetCmd, usb_gpio_print_cmd_name(uiRetCmd));
		uart_clear_port(miModule->InitParams[0]);
		return 0;
	}
	
	uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_TYPE, (unsigned char*)&miModule->SubType, 1, NULL, 1000);
	if (uiRetCmd != SEP_CMD_TYPE)
	{
		dbgprintf(2, "USB GPIO get n, Error %i %s\n", uiRetCmd, usb_gpio_print_cmd_name(uiRetCmd));
		uart_clear_port(miModule->InitParams[0]);
		return 0;
	}
	
	/*if (uiNameLen)
	{
		if (SearchDataInBuffer(miModule->Name, 64, 0, cName, uiNameLen) != 1)
		{
			char cOldName[64];
			memcpy(cOldName, miModule->Name, 64);
			memset(miModule->Name, 0, 64);
			memcpy(miModule->Name, cName, uiNameLen);
			miModule->Name[uiNameLen] = 32;
			uiNameLen++;
			int iAddLen = 63 - uiNameLen;
			if (iAddLen > 0) memcpy(&miModule->Name[uiNameLen], cOldName, iAddLen);
		}
	}*/
	
	unsigned int uiSensors_Count = 0;
	uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_SENSORS_COUNT, (unsigned char*)&uiSensors_Count, sizeof(unsigned int), NULL, 1000);
	if (uiRetCmd != SEP_CMD_SENSORS_COUNT)
	{
		dbgprintf(2, "USB GPIO get sensors count, Error\n");
		uart_clear_port(miModule->InitParams[0]);
		return 0;
	}
	
	//unsigned int uiSensorsLocate = miModule->Settings[0] & USBIO_MAIN_SETT_SAVE_IN_MODULE;	
	//if (uiSensorsLocate) 
	{
		unsigned int uiSettingsLen = 0;
		unsigned int uiSettings[MAX_MODULE_SETTINGS];
		
		if (!usb_gpio_load_settings(miModule, uiSettings, MAX_MODULE_SETTINGS, &uiSettingsLen))
		{
			dbgprintf(2, "USB GPIO load settings, Error\n");
			return 0;
		}
		uiSettingsLen >>= 2;
		if ((uiSettingsLen < 2) || (uiSettingsLen > MAX_MODULE_SETTINGS))
			dbgprintf(2, "USB GPIO load settings, wrong length %i != %i\n", uiSettingsLen, MAX_MODULE_SETTINGS);
		if (uiSettings[0] & USBIO_MAIN_SETT_SAVE_IN_MODULE)
		{
			//uiSettings[0] &= 0x0000FF00 | USBIO_MAIN_SETT_PARAMS_MASK;
			//uiSettings[0] |= miModule->Settings[0] & (0xFFFF0000 | USBIO_MAIN_SETT_PORT_TYPE_MASK);					
			uiSettings[0] &= 0x0000FF00 | USBIO_MAIN_SETT_SAVE_IN_MODULE;
			uiSettings[0] |= miModule->Settings[0] & 0xFFFF00FF;					
			uiSettings[1] =  miModule->Settings[1];
			memcpy(miModule->Settings, uiSettings, sizeof(unsigned int) * MAX_MODULE_SETTINGS);			
		}
	}			
	
	usb_gpio_fill_settings(miModule);
	
	unsigned int uiSensorsSettCount = (miModule->Settings[0] & 0x0000FF00) >> 8;
	
	if (uiSensorsSettCount > uiSensors_Count) dbgprintf(2, "USB IO Settings big size, Sett: %i, Support: %i\n", uiSensorsSettCount, uiSensors_Count);
	
	if (uiSensorsSettCount > MAX_MODULE_STATUSES)
	{
		dbgprintf(2, "USB GPIO sensors count size %i, MAX:%i\n", uiSensorsSettCount, MAX_MODULE_STATUSES);
		dbgprintf(2, "USB GPIO sensors count size set %i\n", MAX_MODULE_STATUSES);
		uiSensorsSettCount = MAX_MODULE_STATUSES;
	}
	
	if (((uiSensorsSettCount*2) + 2) > MAX_MODULE_SETTINGS)
	{
		dbgprintf(2, "USB GPIO: Big size sensors:%i MAX:%i\n", uiSensorsSettCount, MAX_MODULE_SETTINGS);
		dbgprintf(2, "USB GPIO: size sensors set %i\n", (MAX_MODULE_SETTINGS/2) - 2);
		uiSensorsSettCount = (MAX_MODULE_SETTINGS/2) - 2;
	}
	
	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_INIT, (unsigned char*)miModule->Settings, ((uiSensorsSettCount*2) + 2) * sizeof(unsigned int), 1000, SEP_CMD_OK) <= 0)
	{
		dbgprintf(2, "USB GPIO init, Error\n");
		return 0;
	}
	
	miModule->ParamsCount = 0;
	miModule->ParamList = NULL;
	unsigned int uiRecvSize = 0;
	if (uiSensors_Count)
	{
		Sensor_Params *spData = (Sensor_Params*)DBG_MALLOC(uiSensors_Count * sizeof(Sensor_Params));
		uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_PARAMS, (unsigned char*)spData, sizeof(Sensor_Params)*uiSensors_Count, &uiRecvSize, 1000);
		if (uiRetCmd != SEP_CMD_PARAMS)
		{
			dbgprintf(2, "USB GPIO get sensors(%i) recv param, Error %i %s\n", uiSensors_Count, uiRetCmd, usb_gpio_print_cmd_name(uiRetCmd));
			uart_clear_port(miModule->InitParams[0]);
			DBG_FREE(spData);		
			return 0;
		}
		if (uiRecvSize != (sizeof(Sensor_Params)*uiSensors_Count))
		{
			dbgprintf(2, "USB GPIO get sensors(%i) size param, Error %i %s (%i!=%i)\n", uiSensors_Count, uiRetCmd, usb_gpio_print_cmd_name(uiRetCmd), uiRecvSize, sizeof(Sensor_Params)*uiSensors_Count);
			DBG_FREE(spData);		
			return 0;
		}
		if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_START, NULL, 0, 1000, SEP_CMD_STARTED) <= 0)
		{
			dbgprintf(2, "USB GPIO start, Error\n");
			return 0;
		}
		miModule->ParamsCount = uiSensors_Count;
		miModule->ParamList = spData;
	}
	
	miModule->Settings[0] = (miModule->Settings[0] & 0xFFFF00FF) | (uiSensorsSettCount << 8);		
	
	dbgprintf(3, "USB GPIO init (sensors:%i), OK\n", uiSensorsSettCount);
	return 1;
}

int usb_gpio_stop_deinit(MODULE_INFO *miModule)
{
	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_STOP, NULL, 0, 1000, SEP_CMD_STOPED) > 0)
		dbgprintf(4, "USB GPIO stopped, OK\n");
			else dbgprintf(2, "USB GPIO stoping, Error\n");
	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_DEINIT, NULL, 0, 1000, SEP_CMD_OK) > 0)
			dbgprintf(4, "USB GPIO Deinited, OK\n");
			else dbgprintf(2, "USB GPIO Deiniting, Error\n");
		
	return 1;
}

int usb_gpio_deactivate(MODULE_INFO *miModule)
{
	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_STOP, NULL, 0, 1000, SEP_CMD_STOPED) > 0)
		dbgprintf(4, "USB GPIO stopped, OK\n");
			else dbgprintf(2, "USB GPIO stoping, Error\n");
	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_DEINIT, NULL, 0, 1000, SEP_CMD_OK) > 0)
			dbgprintf(4, "USB GPIO Deinited, OK\n");
			else dbgprintf(2, "USB GPIO Deiniting, Error\n");
			
	if (miModule->ParamsCount) DBG_FREE(miModule->ParamList);
	if (miModule->SettingsCount) DBG_FREE(miModule->SettingList);
	miModule->ParamsCount = 0;
	miModule->ParamList = NULL;
	miModule->SettingsCount = 0;
	miModule->SettingSelected = 0;	
	miModule->SettingList = NULL;
	
	return 1;
}

int usb_gpio_fill_settings(MODULE_INFO *miModule)
{
	miModule->SettingsCount = (miModule->Settings[0] >> 8) & 0xFF;
	if (miModule->SettingsCount >= ((MAX_MODULE_SETTINGS / 2) - 2)) miModule->SettingsCount = ((MAX_MODULE_SETTINGS / 2) - 2);
	if (miModule->SettingsCount >= MAX_MODULE_STATUSES) miModule->SettingsCount = MAX_MODULE_STATUSES;	
	
	miModule->SettingList = (Sensor_Params*)DBG_MALLOC(sizeof(Sensor_Params) * miModule->SettingsCount);
	memset(miModule->SettingList, 0, sizeof(Sensor_Params) * miModule->SettingsCount);
	
	if (miModule->ScanSet)
	{	
		if (!(miModule->Settings[0] & USBIO_MAIN_SETT_MAINSCAN)) miModule->Settings[0] |= USBIO_MAIN_SETT_MAINSCAN;
	}
	else
	{
		if (miModule->Settings[0] & USBIO_MAIN_SETT_MAINSCAN) miModule->Settings[0] ^= USBIO_MAIN_SETT_MAINSCAN;
	}
	
	usb_gpio_convert_settings(miModule->SettingList, miModule->SettingsCount, &miModule->Settings[2], 0);
	return 1;
}

int usb_gpio_convert_settings(Sensor_Params *pList, unsigned int uiCount, unsigned int *pSettings, char cDirect)
{
	int i,  k;
	unsigned int temp;
								// 	-2	//	| 00000000	|	00000000 |	00000000	| 	000		0			0		000	|
								//	-2	//			UartPath(16)	SensorsUse(8)		MainScan(1)	AutoScan(1)	NotUse(3)
								//	-1	//	| 00000000	|	00000000 |	00000000	| 	00000000	|
								//	-1	//					Speed(32)

								//	0	//	|	00000000	|00000000|00000000|   	 0	 		 0  		000000  |
								//	0	//		Type(8)			Period(16)		PwrOnInit(1)   Enabl(1)    PortNum(6)
					//INPUT		//	1	//  |   									0000			0      	0 		00	|
								//  1	//	    								Accuracy(4)		 Analog(1) Inv(1)  Pull(2)
					//OUTPUT	//	1	//  |   	|00000000|	00000000|	000			0	   0    	0 		00	|
								//  1	//	    	ImpulseLen	DefVal(8)  Mode(3)	 PWM(1) OpnDrn(1) Inv(1)  Pull(2)
					//LM75		//	1	//  |   																000	|
								//  1	//	    															Address(3)
					//AM2320	//	1	//  |
								//  1	//
					//RS485		//	1	//  |								0000000|		0		0			000000|
								//  1	//									IOPortNum(7) 		IoControl(1)  	SpeedCode(6)
					//IR RECV	//	1	//  |																	00000000|
								//  1	//																		TimeSkip(8)
	
	k = 0;
	for(i = 0; i < uiCount; i++)
	{
		if (cDirect)
		{
			pSettings[k] = (pList[i].ID & 0b111111) |
							((pList[i].Enabled & 1) << 6) |
							((pList[i].PwrInit & 1) << 7) |
							((pList[i].Interval & 0xFFFF) << 8) |
							((pList[i].CurrentType & 0xFF) << 24);
			pSettings[k+1] = 0;
		}
		else
		{
			memset(&pList[i] , 0, sizeof(Sensor_Params));
			temp = pSettings[k];
			pList[i].ID = temp & 0b111111;		
			temp >>= 6;
			pList[i].Enabled = temp & 1;
			temp >>= 1;
			pList[i].PwrInit = temp & 1;
			temp >>= 1;
			pList[i].Interval = temp & 0xFFFF;
			temp >>= 16;
			pList[i].CurrentType = temp & 0xFF;
		}
		k++;
		temp = pSettings[k];
		switch(pList[i].CurrentType)
		{
			case USBIO_PIN_SETT_TYPE_INPUT:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Pull & 0b11) |
									((pList[i].Inverted & 1) << 2) |
									((pList[i].Analog & 1) << 3) |
									((pList[i].Accuracy & 0b1111) << 4);
				}
				else
				{
					pList[i].Pull = temp & 3;
					if (pList[i].Pull == 3) pList[i].Pull = 0;
					temp >>= 2;
					pList[i].Inverted = temp & 1;
					temp >>= 1;
					pList[i].Analog = temp & 1;
					temp >>= 1;
					pList[i].Accuracy = temp & 0b1111;
					if (pList[i].Accuracy > 12) pList[i].Accuracy = 0;
				}
				break;
			case USBIO_PIN_SETT_TYPE_OUTPUT:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Pull & 0b11) |
									((pList[i].Inverted & 1) << 2) |
									((pList[i].OpenDrain & 1) << 3) |
									((pList[i].PWM & 1) << 4) |
									((pList[i].Mode & 0b111) << 5) |
									((pList[i].DefaultValue & 0xFF) << 8) |
									((pList[i].ImpulseLen & 0xFF) << 16)|
									((pList[i].PortSpeedCode & 0xFF) << 24);
				}
				else
				{
					pList[i].Pull = temp & 0b11;
					if (pList[i].Pull == 3) pList[i].Pull = 0;
					temp >>= 2;
					pList[i].Inverted = temp & 1;
					temp >>= 1;
					pList[i].OpenDrain = temp & 1;
					temp >>= 1;
					pList[i].PWM = temp & 1;
					temp >>= 1;
					pList[i].Mode = temp & 0b111;
					temp >>= 3;
					pList[i].DefaultValue = temp & 0xFF;
					temp >>= 8;
					pList[i].ImpulseLen = temp & 0xFF;
					temp >>= 8;
					pList[i].PortSpeedCode = temp & 0xFF;
				}
				break;
			case USBIO_PIN_SETT_TYPE_I2C_LM75:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Address & 0b111);
				}
				else
				{
					pList[i].Address = temp & 0b111;
				}
				break;
			case USBIO_PIN_SETT_TYPE_I2C_ADS1015:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Address & 0b11) |
									((pList[i].Gain & 0b111) << 2);
				}
				else
				{
					pList[i].Address = temp & 0b11;
					temp >>= 2;
					pList[i].Gain = temp & 0b111;
				}
				break;
			case USBIO_PIN_SETT_TYPE_I2C_MCP3421:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Address & 0b111) |
									((pList[i].Accuracy & 0b11) << 3) |
									((pList[i].Gain & 0b11) << 5);
				}
				else
				{
					pList[i].Address = temp & 0b111;
					temp >>= 3;
					pList[i].Accuracy = temp & 0b11;
					temp >>= 2;
					pList[i].Gain = temp & 0b11;
				}
				break;
			case USBIO_PIN_SETT_TYPE_I2C_AS5600:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Address & 0b111) |
									((pList[i].ResultType & 0b1) << 3);
				}
				else
				{
					pList[i].Address = temp & 0b111;
					temp >>= 3;
					pList[i].ResultType = temp & 0b1;
				}
				break;
			case USBIO_PIN_SETT_TYPE_I2C_HMC5883L:
				/*if (cDirect)
				{
					pSettings[k] = (pList[i].Address & 0b111) |
									((pList[i].ResultType & 0b1) << 3);
				}
				else
				{
					pList[i].Address = temp & 0b111;
					temp >>= 3;
					pList[i].ResultType = temp & 0b1;
				}*/
				break;
			case USBIO_PIN_SETT_TYPE_IR_RECIEVER:
				if (cDirect)
				{
					pSettings[k] = pList[i].TimeSkip & 0xFF;
				}
				else
				{
					pList[i].TimeSkip = temp & 0xFF;
				}
				break;
			case USBIO_PIN_SETT_TYPE_RS485:
				if (cDirect)
				{
					pSettings[k] = ((pList[i].PortSpeedCode) & 0b111111) |
									((pList[i].IOWControlUse & 1) << 6) |
									((pList[i].IORControlUse & 1) << 7) |
									((pList[i].IOWControlPort & 0b1111111) << 8) |
									((pList[i].IORControlPort & 0b1111111) << 15)|
									((pList[i].IOModeControl & 1) << 24)|
									((pList[i].ImpulseLen & 0b01111111) << 25);
				}
				else
				{
					pList[i].PortSpeedCode = temp & 0b111111;
					temp >>= 6;
					pList[i].IOWControlUse = temp & 1;
					temp >>= 1;
					pList[i].IORControlUse = temp & 1;
					temp >>= 1;
					pList[i].IOWControlPort = temp & 0b1111111;
					temp >>= 7;
					pList[i].IORControlPort = temp & 0b1111111;
					temp >>= 9;
					pList[i].IOModeControl = temp & 1;
					temp >>= 1;
					pList[i].ImpulseLen = temp & 0b01111111;
				}
				break;
			case USBIO_PIN_SETT_TYPE_RS485_PN532:
			case USBIO_PIN_SETT_TYPE_RS485_RC522:
				if (cDirect)
				{					
					pSettings[k] = 	((pList[i].IOWControlUse & 1) << 6) |
									((pList[i].IORControlUse & 1) << 7) |
									((pList[i].IOWControlPort & 0b1111111) << 8) |
									((pList[i].IORControlPort & 0b1111111) << 15)|
									((pList[i].TimeSkip & 0b11) << 22)|
									((pList[i].IOModeControl & 1) << 24)|
									((pList[i].ImpulseLen & 0xFF) << 25);
				}
				else
				{
					temp >>= 6;
					pList[i].IOWControlUse = temp & 1;
					temp >>= 1;
					pList[i].IORControlUse = temp & 1;
					temp >>= 1;
					pList[i].IOWControlPort = temp & 0b1111111;
					temp >>= 7;
					pList[i].IORControlPort = temp & 0b1111111;
					temp >>= 7;					
					pList[i].TimeSkip = temp & 0b11;
					temp >>= 2;
					pList[i].IOModeControl = temp & 1;
					temp >>= 1;
					pList[i].ImpulseLen = temp & 0b01111111;
				}
				break;
			case USBIO_PIN_SETT_TYPE_I2C_MISC:
				if (cDirect)
				{					
					pSettings[k] = 	(pList[i].Address & 0b1111111) |
									((pList[i].PortSpeedCode & 0b111111) << 7) |
									((pList[i].ResultType & 0b11111) << 13);
				}
				else
				{
					pList[i].Address = temp & 0b1111111;
					temp >>= 7;
					pList[i].PortSpeedCode = temp & 0b111111;
					temp >>= 6;
					pList[i].ResultType = temp & 0b11111;
				}
				break;
			case USBIO_PIN_SETT_TYPE_RS485_MISC:
				if (cDirect)
				{
					pSettings[k] = (pList[i].Address & 0b1111111) |
									((pList[i].PortSpeedCode & 0b111111) << 7) |
									((pList[i].IOWControlUse & 1) << 13) |
									((pList[i].IOWControlPort & 0b1111111) << 14) |
									((pList[i].IOModeControl & 1) << 21) |
									((pList[i].ImpulseLen & 0b011111) << 22) |
									((pList[i].ResultType & 0b011111) << 27);
				}
				else
				{
					pList[i].Address = temp & 0b1111111;
					temp >>= 7;
					pList[i].PortSpeedCode = temp & 0b111111;
					temp >>= 6;
					pList[i].IOWControlUse = temp & 1;
					temp >>= 1;
					pList[i].IOWControlPort = temp & 0b1111111;
					temp >>= 7;
					pList[i].IOModeControl = temp & 1;
					temp >>= 1;
					pList[i].ImpulseLen = temp & 0b011111;
					temp >>= 5;
					pList[i].ResultType = temp & 0b011111;
				}
				break;			
			case USBIO_PIN_SETT_TYPE_I2C_AM2320:
			default:
				if (cDirect) 
				{
					pSettings[k] = pList[i].ResultType & 1;
				}
				else
				{
					pList[i].ResultType = temp & 1;
				}
				break;			
		}
		k++;
	}
	return 1;
}

void usb_gpio_lock(MODULE_INFO *miModule)
{
	DBG_MUTEX_LOCK(miModule->IO_mutex);
	while(1)
	{
		if (miModule->IO_flag[0] == 0) 
		{
			miModule->IO_flag[0] = 1;
			break;
		}
		else
		{
			DBG_MUTEX_UNLOCK(miModule->IO_mutex);
			usleep(1000);
			DBG_MUTEX_LOCK(miModule->IO_mutex);
		}
	}
	DBG_MUTEX_UNLOCK(miModule->IO_mutex);
}

void usb_gpio_unlock(MODULE_INFO *miModule)
{
	DBG_MUTEX_LOCK(miModule->IO_mutex);
	miModule->IO_flag[0] = 0;
	DBG_MUTEX_UNLOCK(miModule->IO_mutex);
}

int usb_gpio_init(MODULE_INFO *miModule)
{
	int iPortNum = (miModule->Settings[0] >> 16) & USBIO_MAIN_SETT_PORT_NUM_MASK;
	int iPortSpeed = miModule->Settings[1];
	int iDefaultSpeed;
	if (iPortNum == 0) 
		{
			iDefaultSpeed = USBIO_DEFAULT_SPEED_UART;
			if (iPortSpeed == 0) iPortSpeed = USBIO_DEFAULT_SPEED_UART;
		}
		else
		{
			if (iPortSpeed == 0) iDefaultSpeed = USBIO_DEFAULT_SPEED_USB; else iDefaultSpeed = iPortSpeed;
		}
	
	char cPortName[32];
	if (!get_uart_port_path(iPortNum, cPortName, 32)) 
	{
		dbgprintf(2, "UART port %i: Not available\n", iPortNum);		
		return 0;
	}
	
	miModule->InitParams[0] = uart_open(cPortName, iDefaultSpeed);
	
	if (miModule->InitParams[0] <= 0) 
	{
		dbgprintf(2, "Error connect to USBIO port: %s, module: %.4s, speed:%i\n", cPortName, (char*)&miModule->ID, iDefaultSpeed);
		return 0;
	}
	
	uart_clear_port(miModule->InitParams[0]);
	//usb_gpio_fill_settings(miModule);
	usb_gpio_clean_transfer_data(miModule);
	
	if (!usb_gpio_test_connect(miModule->InitParams[0]))
	{
		close(miModule->InitParams[0]);
		
		if ((iPortNum == 0) && (iPortSpeed != iDefaultSpeed))
		{
			miModule->InitParams[0] = uart_open(cPortName, iPortSpeed);
			if (miModule->InitParams[0] <= 0) 
			{
				dbgprintf(2, "Error connect to USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
				return 0;
			}
			usb_gpio_clean_transfer_data(miModule);
			if (!usb_gpio_test_connect(miModule->InitParams[0]))
			{
				close(miModule->InitParams[0]);
				dbgprintf(2, "Error test2 connect to USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
				return 0;
			} else dbgprintf(3, "Speed connect not need switch, USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
		}
		else
		{
			dbgprintf(2, "Error test1 connect to USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
			return 0;
		}
	}
	else
	{		
		if ((iPortNum == 0) && (iPortSpeed != iDefaultSpeed))
		{
			if (!usb_gpio_change_speed(miModule->InitParams[0], iPortSpeed))
			{
				//close(miModule->InitParams[0]);
				dbgprintf(2, "Error change speed connect to USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
				dbgprintf(2, "Use defaul speed connect to USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iDefaultSpeed);
				//return 0;
			} 
			else 
			{
				close(miModule->InitParams[0]);
				miModule->InitParams[0] = uart_open(cPortName, iPortSpeed);	
				if (miModule->InitParams[0] <= 0) 
				{
					dbgprintf(2, "Error change speed UART for USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
					return 0;
				}
				usleep(400000);
			}
			usb_gpio_clean_transfer_data(miModule);
			if (!usb_gpio_test_connect(miModule->InitParams[0]))
			{
				close(miModule->InitParams[0]);
				dbgprintf(2, "Error test2 connect to USBIO module: %.4s, speed:%i\n", (char*)&miModule->ID, iPortSpeed);
				return 0;
			}
		}
	}
	
	usb_gpio_stop_deinit(miModule);
	usb_gpio_clean_transfer_data(miModule);
	int res = usb_gpio_activate(miModule);
	if (res) 
	{
		miModule->IO_mutex = DBG_MALLOC(sizeof(pthread_mutex_t));
		miModule->IO_flag = DBG_MALLOC(sizeof(int));		
		miModule->IO_data = DBG_MALLOC(sizeof(int)*MAX_MODULE_STATUSES);
		miModule->IO_size_data = DBG_MALLOC(sizeof(int));
		
		miModule->IO_size_data[0] = 0;
		miModule->IO_flag[0] = 0;
		pthread_mutex_init(miModule->IO_mutex, NULL); 
	}
	return res;
}

int usb_gpio_reinit(MODULE_INFO *miModule)
{
	if (miModule->InitParams[0] <= 0) return 0;
	usb_gpio_clean_transfer_data(miModule);
	
	if (!usb_gpio_deactivate(miModule)) return 0;
	return usb_gpio_activate(miModule);
}

int usb_gpio_close(MODULE_INFO *miModule)
{
	if (miModule->InitParams[0] <= 0) return 0;
	usb_gpio_clean_transfer_data(miModule);
	usb_gpio_deactivate(miModule);
	usb_gpio_change_speed(miModule->InitParams[0], USBIO_DEFAULT_SPEED_UART);
	
	close(miModule->InitParams[0]);
	miModule->InitParams[0] = 0;
	
	pthread_mutex_destroy(miModule->IO_mutex);
	DBG_FREE(miModule->IO_mutex);
	DBG_FREE(miModule->IO_flag);
	DBG_FREE(miModule->IO_size_data);
	DBG_FREE(miModule->IO_data);
	
	return 1;
}

int usb_gpio_stop_with_test(MODULE_INFO *miModule)
{
	usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_STOP, NULL, 0, 0, SEP_CMD_NULL);
	unsigned int uiCmd = SEP_CMD_NULL;
	unsigned int uiRecvLen = 256;
	unsigned char uiRecvData[256];
	int res;
	do
	{
		res = usb_gpio_recv_command(miModule->InitParams[0], &uiCmd, uiRecvData, &uiRecvLen, 1000, 0);
		if (res == 0) break;
		if (uiCmd == SEP_CMD_STATUSES)
		{		
			if ((uiRecvLen >= 4) && (uiRecvLen <= (MAX_MODULE_STATUSES * sizeof(int))))
			{
				miModule->IO_size_data[0] = uiRecvLen;
				memcpy(miModule->IO_data, uiRecvData, uiRecvLen);
				//printf("find %i %i\n", miModule->IO_data[12], uiRecvLen);				
			}
		}
	} while ((uiCmd != SEP_CMD_STOPED) && (uiCmd != SEP_CMD_NULL));
	
	return 0;
}

int usb_gpio_start(MODULE_INFO *miModule)
{
	return usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_START, NULL, 0, 1000, SEP_CMD_STARTED);
}

int usb_gpio_request_statuses(MODULE_INFO *miModule)
{	
	if (usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_GET_STATUSES, NULL, 0, 0, SEP_CMD_NULL) > 0)
		return 1;
	return 0;
}

int usb_gpio_authenticate_card(MODULE_INFO *miModule, unsigned int sensenum, uint8_t iBlock, void *pKeyInfo, unsigned int uiLength)
{	
	int iPacketLen = uiLength + 2;
	uint8_t *sbuff = (uint8_t*)DBG_MALLOC(iPacketLen);
	sbuff[0] = sensenum;
	sbuff[1] = iBlock;
	memcpy(&sbuff[2], pKeyInfo, uiLength);
	
	usb_gpio_lock(miModule);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN)
		usb_gpio_stop_with_test(miModule);
	
	unsigned int uiResSize = 0;
	int pAuthTrsult = 0;
	unsigned int uiRetCmd = usb_gpio_exchange_data(miModule->InitParams[0], SEP_CMD_GET_AUTHENTICATE_CARD, sbuff, iPacketLen, (unsigned char*)&pAuthTrsult, &uiResSize, sizeof(pAuthTrsult), 1000);
	if (sizeof(pAuthTrsult) != uiResSize) dbgprintf(2, "Wrong result size SMART SEP_CMD_GET_AUTHENTICATE_CARD %i != %i   %s\n", uiResSize, sizeof(pAuthTrsult), usb_gpio_print_cmd_name(uiRetCmd));
	if (uiRetCmd == SEP_CMD_AUTHENTICATE_CARD) return pAuthTrsult;
		else 
		{
			dbgprintf(2, "Wrong result SMART CARD AUTH %s %i\n", usb_gpio_print_cmd_name(uiRetCmd), uiResSize);
			uart_clear_port(miModule->InitParams[0]);
		}
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
	usb_gpio_unlock(miModule);									
		
	return 0;	
}

int usb_gpio_write_data_block(MODULE_INFO *miModule, unsigned int sensenum, int iBlock, int iAction, void *pKeyInfo, uint32_t uiLength, uint8_t* pData, uint32_t uiDataLen)
{
	int iPacketLen = uiLength + 11 + uiDataLen;
	uint8_t *sbuff = (uint8_t*)DBG_MALLOC(iPacketLen);
	sbuff[0] = (uint8_t)sensenum;
	sbuff[1] = (uint8_t)iBlock;
	sbuff[2] = (uint8_t)iAction;
	memcpy(&sbuff[3], &uiLength, sizeof(uint32_t));
	memcpy(&sbuff[7], &uiDataLen, sizeof(uint32_t));
	memcpy(&sbuff[11], pKeyInfo, uiLength);
	memcpy(&sbuff[11+uiLength], pData, uiDataLen);
	
	usb_gpio_lock(miModule);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN)
			usb_gpio_stop_with_test(miModule);
	
	unsigned int uiResSize = 0;
	int pResult = 0;
	unsigned int uiRetCmd = usb_gpio_exchange_data(miModule->InitParams[0], SEP_CMD_WRITE_DATA_BLOCK, sbuff, iPacketLen, (unsigned char*)&pResult, &uiResSize, sizeof(pResult), 1000);
	if (sizeof(pResult) != uiResSize) dbgprintf(2, "Wrong result size SMART SEP_CMD_WRITE_DATA_BLOCK %i != %i\n", uiResSize, sizeof(pResult));
	if (uiRetCmd == SEP_CMD_WRITE_DATA_RESULT) return pResult;
		else 
		{
			dbgprintf(2, "Wrong result SEP_CMD_WRITE_DATA_BLOCK %s %i\n", usb_gpio_print_cmd_name(uiRetCmd), uiResSize);
			uart_clear_port(miModule->InitParams[0]);
		}
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
	usb_gpio_unlock(miModule);									
		
	return 0;	
}

int usb_gpio_get_card_serial(MODULE_INFO *miModule, uint8_t* pSensorNum, uint8_t* pSerialOut, unsigned int uiMaxLen, unsigned int *uiOutLen)
{
	if (!miModule->SettingsCount) return 0;
	int i, iResult = 0;
	unsigned int uiID, type, uiRetCmd;
	for (i = 0; i < miModule->SettingsCount; i++)	
	{
		uiID = miModule->SettingList[i].ID;
		type = miModule->ParamList[uiID].CurrentType;
		if (miModule->ParamList[uiID].Enabled &&
			((type == USBIO_PIN_SETT_TYPE_RS485_RC522) ||
			(type == USBIO_PIN_SETT_TYPE_RS485_PN532)))
		{
			iResult = 1; 
			break;
		}
	}
	if (iResult == 0) return 0;
	
	usb_gpio_lock(miModule);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN)
		usb_gpio_stop_with_test(miModule);
	
	unsigned int uiResSize = 0;
	uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_CARD_SERIAL, pSerialOut, uiMaxLen, &uiResSize, 1000);
	if ((uiRetCmd == SEP_CMD_CARD_SERIAL) && (uiResSize > 1))
	{
		*pSensorNum = pSerialOut[0];
		memmove(&pSerialOut[0], &pSerialOut[1], uiResSize - 1);
		*uiOutLen = uiResSize - 1;
	}	
	else dbgprintf(2, "Wrong result SMART CARD SERIAL %s %i\n", usb_gpio_print_cmd_name(uiRetCmd), uiResSize);
	
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
	usb_gpio_unlock(miModule);									
		
	if (uiResSize) return 1;
	return 0;
}

int usb_gpio_get_ir_data(MODULE_INFO *miModule, uint16_t* cOutData, unsigned int uiMaxLen, unsigned int *uiOutLen)
{
	if (!miModule->SettingsCount) return 0;
	int i, iResult = 0;
	unsigned int uiID, type, uiRetCmd;
	for (i = 0; i < miModule->SettingsCount; i++)	
	{
		uiID = miModule->SettingList[i].ID;
		type = miModule->ParamList[uiID].CurrentType;
		if (miModule->ParamList[uiID].Enabled &&
			(type == USBIO_PIN_SETT_TYPE_IR_RECIEVER))
		{
			iResult = 1; 
			break;
		}
	}
	if (iResult == 0) return 0;
	
	usb_gpio_lock(miModule);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN)
		usb_gpio_stop_with_test(miModule);
									
	unsigned int uiResSize = 0;
	unsigned char *cResData = (unsigned char*)DBG_MALLOC(uiMaxLen);
	uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_IR_DATA, cResData, uiMaxLen, &uiResSize, 1000);
	if (uiRetCmd == SEP_CMD_IR_DATA) 
	{
		for (i = 0; i < uiResSize; i++) cOutData[i] = cResData[i];
		*uiOutLen = uiResSize;
	} else dbgprintf(2, "Wrong result IR DATA %s\n", usb_gpio_print_cmd_name(uiRetCmd));
	DBG_FREE(cResData);
	
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
	usb_gpio_unlock(miModule);									
	
	if (uiResSize) return 1;
	return 0;
}

int usb_gpio_load_settings(MODULE_INFO *miModule, uint32_t* cSettings, unsigned int uiMaxLen, unsigned int *uiSettingsLen)
{		
	unsigned int uiRetCmd;	
	unsigned int uiResSize = 0;
	uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_SETTINGS_DATA, (unsigned char *)cSettings, uiMaxLen*sizeof(unsigned int), &uiResSize, 1000);
	if (uiRetCmd == SEP_CMD_SETTINGS_DATA) 
	{
		*uiSettingsLen = uiResSize;
	} 
	else 
	{
		dbgprintf(2, "Wrong result SETTINGS DATA %s\n", usb_gpio_print_cmd_name(uiRetCmd));
		uart_clear_port(miModule->InitParams[0]);
	}
	
	if (uiResSize) return 1;
	return 0;
}

int usb_gpio_save_settings(MODULE_INFO *miModule, uint32_t* cSettings, unsigned int uiSettingsLen)
{
	usb_gpio_lock(miModule);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_stop_with_test(miModule);
	usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_SET_SETTINGS_DATA, (unsigned char *)cSettings, uiSettingsLen * sizeof(unsigned int), 1000, SEP_CMD_OK);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
	usb_gpio_unlock(miModule);
	return 1;
}

int usb_gpio_set_focus(MODULE_INFO *miModule, int iNewPos, char cAbsolute, int *iCurrPos)
{
	int result = 0;
	usb_gpio_lock(miModule);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_stop_with_test(miModule);
	
	unsigned char cBuff[5];
	cBuff[0] = cAbsolute ? PTZ_TARGET_ABSLT_RAW_FOCUS_FAST : PTZ_TARGET_STEP_RAW_FOCUS_FAST;
	memcpy(&cBuff[1], &iNewPos, sizeof(int));
	
	int iLenPos = 0;
	
	int res = usb_gpio_exchange_data(miModule->InitParams[0], SEP_CMD_SET_STATUS_EXT, cBuff, 5, 
										(unsigned char*)iCurrPos, (unsigned int*)&iLenPos, 4, 3000);
	if (res == SEP_CMD_OK)
	{
		if (iLenPos == sizeof(int))
		{
			result = 1;
			/*unsigned int uiRetCmd;
			unsigned int uiRecvLen = 128;
			unsigned char buffer[128];
			memset(buffer , 0, 128);
			if (usb_gpio_recv_command(miModule->InitParams[0], &uiRetCmd, buffer, &uiRecvLen, 1000, 0) > 0)
			{
				if (uiRetCmd != SEP_CMD_OK) dbgprintf(2, "Error response type SEP_CMD_SET_STATUS_EXT: %i\n", res);
			} else dbgprintf(2, "Error recv SEP_CMD_OK\n");*/
		}
		else dbgprintf(2, "Error transfer len SEP_CMD_SET_STATUS_EXT:%i != %i\n", iLenPos, sizeof(int));
	} else dbgprintf(2, "Error response type SEP_CMD_SET_STATUS_EXT: %i\n", res);
	if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
	usb_gpio_unlock(miModule);
	return result;
}

int usb_gpio_get_status_module(MODULE_INFO *miModule, char cFull)
{	
	if (miModule->InitParams[0] <= 0) return 0;	
	
	int iStatuses[MAX_MODULE_STATUSES];
	int i, type;
	int iResult = 0;
	unsigned int uiRetCmd = 0;
	unsigned int uiID;
	int iAutoScan = miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN;
	if (!miModule->ScanSet) iAutoScan = 0;
	if (!miModule->SettingsCount) return 0;
	
	for (i = 0; i < miModule->SettingsCount; i++)	
	{
		uiID = miModule->SettingList[i].ID;
		type = miModule->ParamList[uiID].CurrentType;
		if (miModule->ParamList[uiID].Enabled &&
			((type == USBIO_PIN_SETT_TYPE_INPUT) ||
			(type == USBIO_PIN_SETT_TYPE_OUTPUT) ||
			(type == USBIO_PIN_SETT_TYPE_IR_RECIEVER) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_AM2320) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_LM75) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_MISC) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_ADS1015) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_MCP3421) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_AS5600) ||
			(type == USBIO_PIN_SETT_TYPE_I2C_HMC5883L) ||
			(type == USBIO_PIN_SETT_TYPE_RS485) ||
			(type == USBIO_PIN_SETT_TYPE_RS485_PN532) ||
			(type == USBIO_PIN_SETT_TYPE_RS485_RC522) ||
			(type == USBIO_PIN_SETT_TYPE_RS485_MISC))) 
		{
			iResult = 1; 
			break;
		}
	}
	
	if (iResult == 0) return 0;	
		
	//if (miModule->SettingsCount > MAX_MODULE_STATUSES) 
	//	i = MAX_MODULE_STATUSES * sizeof(int);
		//else i = miModule->SettingsCount * sizeof(int);
	usb_gpio_lock(miModule);
	
	do
	{
		i = MAX_MODULE_STATUSES * sizeof(int);
		if (iAutoScan) //AutoScan
			usb_gpio_recv_command(miModule->InitParams[0], &uiRetCmd, (unsigned char*)iStatuses, (unsigned int*)&i, 1000, 1);
			else
			{
				if (uiRetCmd != SEP_CMD_TEXT_MESSAGE)
					uiRetCmd = usb_gpio_request_data(miModule->InitParams[0], SEP_CMD_GET_STATUSES, (unsigned char*)iStatuses, i, (unsigned int*)&i, 1000);
				else
					usb_gpio_recv_command(miModule->InitParams[0], &uiRetCmd, (unsigned char*)iStatuses, (unsigned int*)&i, 1000, 0);
			}
			
		if (uiRetCmd == SEP_CMD_TEXT_MESSAGE)
		{
			if (i <= 64)
			{
				char *txt = (char*)iStatuses;
				int iMessLevel = 2;
				if (txt[0] < 32)
				{
					iMessLevel = txt[0];
					txt = (char*)&txt[1];
				}
				txt[64] = 0;
				dbgprintf(iMessLevel, "USBIO %.4s:%s\n", (char*)&miModule->ID, txt);
			} else dbgprintf(2, "Wrong size text messages from %.4s\n", (char*)&miModule->ID);
		}
	} while (uiRetCmd == SEP_CMD_TEXT_MESSAGE);
	
	//if (uiRetCmd != SEP_CMD_NULL) printf("recv %s\n", usb_gpio_print_cmd_name(uiRetCmd));

	//if (uiRetCmd != SEP_CMD_BAD) printf("get stat %i %s\n", iAutoScan, usb_gpio_print_cmd_name(uiRetCmd)); else printf(".\n");
	if (uiRetCmd == SEP_CMD_STATUSES)
	{
		if (i > (MAX_MODULE_STATUSES * sizeof(int)))
		{
			dbgprintf(2, "Wrong size statuses from usb gpio %i!=%i (cnt:%i)\n", MAX_MODULE_STATUSES * sizeof(int), i, miModule->SettingsCount);
			i = MAX_MODULE_STATUSES * sizeof(int);
		}
		memcpy(miModule->Status, iStatuses, i);		
		if (iAutoScan) usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_ACCEPT_STATUSES, NULL, 0, 0, SEP_CMD_NULL);
	} 
	
	if (cFull && (uiRetCmd == SEP_CMD_NULL) && miModule->IO_size_data[0])
	{
		i = miModule->IO_size_data[0];
		if (i > (MAX_MODULE_STATUSES * sizeof(int)))
		{
			dbgprintf(2, "Wrong size statuses from usb gpio %i!=%i (cnt:%i)\n", MAX_MODULE_STATUSES * sizeof(int), i, miModule->SettingsCount);
			i = MAX_MODULE_STATUSES * sizeof(int);
		}
		memcpy(miModule->Status, miModule->IO_data, i);
		miModule->IO_size_data[0] = 0;
		if (iAutoScan) usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_ACCEPT_STATUSES, NULL, 0, 0, SEP_CMD_NULL);
		uiRetCmd = SEP_CMD_STATUSES;
	}
	
	if (!iAutoScan && ((uiRetCmd == SEP_CMD_NULL) || (uiRetCmd == SEP_CMD_BAD)))
	{
		dbgprintf(2, "Error request statuses usb gpio %s\n", usb_gpio_print_cmd_name(uiRetCmd));
		uart_clear_port(miModule->InitParams[0]);
		//miModule->Enabled ^= 1;		
	}	
	
	usb_gpio_unlock(miModule);
	if ((uiRetCmd == SEP_CMD_NULL) || (uiRetCmd == SEP_CMD_BAD)) return 0;
	//printf("CS %i %i %i %i\n", uiRetCmd, miModule->Status[0], miModule->Status[1], miModule->Status[2]);
	return 1;
}

int usb_gpio_set_status_module(MODULE_INFO *miModule, int iSubModule, int iValue)
{
	//iSubModule--;
	if (miModule->SubType == MODULE_SUBTYPE_IO)
	{
		if ((iSubModule < 0) || (iSubModule >= miModule->SettingsCount)) 
		{
			dbgprintf(2, "USBIO set status error, wrong params %.4s %i, %i\n", (char*)&miModule->ID, iSubModule, miModule->ParamsCount);
			//return 0;
		}
		else
		{	
			int iID = miModule->SettingList[iSubModule].ID;
		
			if (!miModule->ParamList[iID].Enabled || (miModule->ParamList[iID].CurrentType == 0))
			{
				dbgprintf(2, "USBIO set status error, sensor(%i[%i]) disabled: %.4s, %i\n", iSubModule, iID, (char*)&miModule->ID, miModule->ParamsCount);
				//return 0;
			}
		}
	}
	
	int result = 1;
		
	if ((iSubModule == 0) && (miModule->SubType == MODULE_SUBTYPE_PTZ))
	{
		onvif_GotoPresetPosition(miModule->ID, iValue);
	}
	else
	{
		unsigned char cBuff[5];
		cBuff[0] = (unsigned char)iSubModule & 255;
		memcpy(&cBuff[1], &iValue, sizeof(int));
		
		usb_gpio_lock(miModule);
						
		if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_stop_with_test(miModule);
		if (!usb_gpio_send_command(miModule->InitParams[0], SEP_CMD_SET_STATUS, cBuff, 5, 1000, SEP_CMD_OK))
		{
			dbgprintf(2, "USBIO set status error, cmd not accepted\n");
			result = 0;
		}
		if (miModule->Settings[0] & USBIO_MAIN_SETT_AUTOSCAN) usb_gpio_start(miModule);
		
		usb_gpio_unlock(miModule);
	}
	
	return result;
}

int external_init(MODULE_INFO *miModule)
{
	//i2c_scan();
	unsigned char buff[8];
	int ret;
	int result = 0;
	miModule->Settings[5] = 0;
	miModule->InitParams[0] = 0;
	
	if ((miModule->Settings[0] < 1) || (miModule->Settings[0] > 2)) return 0;
	if ((miModule->Settings[0] == 1) && (miModule->InitParams[0] == 0)) miModule->InitParams[0] = i2c_open(miModule->Settings[2]);
	if ((miModule->Settings[0] == 2) && (miModule->InitParams[0] == 0)) miModule->InitParams[0] = uart_init_port(miModule->Settings[3], miModule->Settings[2]);
	
	if (!miModule->InitParams[0])
	{
		dbgprintf(2,"External module %.4s init error, did not inited\n", (char*)&miModule->ID);
		return 0;
	}
	
	if (miModule->Settings[0] == 1) 
	{
		
		DBG_MUTEX_LOCK(&i2c_mutex);
		i2c_set_address(miModule->InitParams[0], miModule->Settings[1]);
	}
	if (miModule->Settings[0] == 2) uart_clear_port(miModule->InitParams[0]);
	
	do
	{
		buff[0] = 255;
		buff[1] = miModule->Settings[1];
		buff[2] = EXTERNAL_COMMAND_INIT;
		if (write(miModule->InitParams[0], buff, 8) != 8) 
		{
			dbgprintf(2,"Error write to module %.4s\n", (char*)&miModule->ID);
			break;
		}
		ret = read_t(miModule->InitParams[0], (char*)buff, 3, 100);
		if (ret != 3)
		{
			if (ret < 0) dbgprintf(2,"Module %.4s (1) error read: %s (%i)\n", (char*)&miModule->ID, strerror(errno), errno);
				else
				dbgprintf(2,"external_init %.4s wrong ret len read: %i/3\n", (char*)&miModule->ID, ret);
			break;
		}
		if ((buff[0] != 255) || (buff[1] != 0) || (buff[2] != EXTERNAL_COMMAND_OK))
		{
			dbgprintf(2,"External module init %.4s error (%i %i %i)\n", (char*)&miModule->ID, buff[0], buff[1], buff[2]);
			break;
		}
		
		buff[0] = 255;
		buff[1] = miModule->Settings[1];
		buff[2] = EXTERNAL_COMMAND_GET_COUNT;
		
		if (write(miModule->InitParams[0], buff, 8) != 8)
		{
			dbgprintf(2,"Error write to module %.4s\n", (char*)&miModule->ID);
			break;
		}	
		ret = read_t(miModule->InitParams[0], (char*)buff, 4, 100);
		if (ret != 4)
		{
			if (ret < 0) 
				dbgprintf(2,"external_init %.4s error read: %s (%i)\n", (char*)&miModule->ID, strerror(errno), errno);
				else
				dbgprintf(2,"external_init %.4s wrong ret len read: %i/4\n", (char*)&miModule->ID, ret);
			break;
		}			
		if ((buff[0] != 255) || (buff[1] != 0) || (buff[2] != EXTERNAL_COMMAND_COUNT))
		{
			dbgprintf(2,"External module init (cnt) %.4s error\n", (char*)&miModule->ID);
			break;
		}
		if (buff[3] > MAX_MODULE_STATUSES) 
		{
			dbgprintf(2,"External %.4s, submodules count %i>%i error\n", (char*)&miModule->ID, buff, MAX_MODULE_STATUSES);
			break;
		}
		result = 1;
	} while(0);

	if (miModule->Settings[0] == 1) DBG_MUTEX_UNLOCK(&i2c_mutex);
	if (result) 
	{
		miModule->Settings[5] = buff[3];
		miModule->IO_mutex = DBG_MALLOC(sizeof(pthread_mutex_t));
		miModule->IO_flag = DBG_MALLOC(sizeof(int));
		miModule->IO_data = DBG_MALLOC(sizeof(int)*MAX_MODULE_STATUSES);
		miModule->IO_size_data = DBG_MALLOC(sizeof(int));
		
		miModule->IO_size_data[0] = 0;
		miModule->IO_flag[0] = 0;
		pthread_mutex_init(miModule->IO_mutex, NULL);
	}
	
	return result;
}

int external_close(MODULE_INFO *miModule)
{
	unsigned char buff[8];
	int ret;
	int result = 0;
	miModule->Settings[5] = 0;
	
	if (!miModule->InitParams[0])
	{
		dbgprintf(2,"External module %.4s close error, did not inited\n", (char*)&miModule->ID);
		return 0;
	}
	
	if (miModule->Settings[0] == 1) 
	{
		DBG_MUTEX_LOCK(&i2c_mutex);
		i2c_set_address(miModule->InitParams[0], miModule->Settings[1]);
	}
	
	do
	{
		buff[0] = 255;
		buff[1] = miModule->Settings[1];
		buff[2] = EXTERNAL_COMMAND_DEINIT;
		if (write(miModule->InitParams[0], buff, 8) != 8)
		{
			dbgprintf(2,"Error write to module %.4s\n", (char*)&miModule->ID);
			break;
		}
		ret = read_t(miModule->InitParams[0], (char*)buff, 3, 100);
		if (ret != 3)
		{
			if (ret < 0) dbgprintf(2,"Module %.4s (1) error read: %s (%i)\n", (char*)&miModule->ID, strerror(errno), errno);
				else
				dbgprintf(2,"external_close %.4s wrong ret len read: %i/3\n", (char*)&miModule->ID, ret);
			break;
		}
		if ((buff[0] != 255) || (buff[1] != 0) || (buff[2] != EXTERNAL_COMMAND_OK))		
		{
			dbgprintf(2,"External module deinit %.4s error\n", (char*)&miModule->ID);		
			break;
		}
		result = 1;
	} while(0);
	if (miModule->Settings[0] == 1) DBG_MUTEX_UNLOCK(&i2c_mutex);
	i2c_close(miModule->InitParams[0]);
	miModule->InitParams[0] = 0;
	
	pthread_mutex_destroy(miModule->IO_mutex);
	DBG_FREE(miModule->IO_mutex);
	DBG_FREE(miModule->IO_flag);
	DBG_FREE(miModule->IO_data);
	DBG_FREE(miModule->IO_size_data);
	
	return result;
}

int external_get_statuses_module(MODULE_INFO *miModule)
{
	unsigned char buff[MAX_MODULE_STATUSES*sizeof(int)+8];
	int ret, len;
	int result = 0;
	
	if ((miModule->Settings[5] < 1) || (miModule->Settings[5] > MAX_MODULE_STATUSES)) return 0;
	len = miModule->Settings[5] * sizeof(int) + 3;
	memset(miModule->Status, 0, len);
	
	if (!miModule->InitParams[0])
	{
		dbgprintf(2,"External module %.4s get status error, did not inited\n", (char*)&miModule->ID);
		return 0;
	}
	
	usb_gpio_lock(miModule);
	
	if (miModule->Settings[0] == 1) 
	{
		DBG_MUTEX_LOCK(&i2c_mutex);
		i2c_set_address(miModule->InitParams[0], miModule->Settings[1]);
	}
	
	do
	{
		buff[0] = 255;
		buff[1] = miModule->Settings[1];
		buff[2] = EXTERNAL_COMMAND_GET_STATUSES;
		if (write(miModule->InitParams[0], buff, 8) != 8)
		{
			dbgprintf(2,"Error write to module %.4s\n", (char*)&miModule->ID);
			break;
		}
		ret = read_t(miModule->InitParams[0], (char*)buff, len, 100);
		if (ret != len)
		{
			if (ret < 0) dbgprintf(2,"external_get %.4s error read: %s (%i)\n", (char*)&miModule->ID, strerror(errno), errno);
				else
				dbgprintf(2,"external_get_statuses %.4s wrong ret len read: %i/%i\n", (char*)&miModule->ID, ret, len);
			break;
		}
		if ((buff[0] != 255) || (buff[1] != 0) || (buff[2] != EXTERNAL_COMMAND_STATUSES))		
		{
			dbgprintf(2,"External module deinit %.4s error\n", (char*)&miModule->ID);		
			break;
		}
		memcpy(miModule->Status, &buff[3], len - 3);
		result = 1;
	}
	while(0);

	if (miModule->Settings[0] == 1)	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	usb_gpio_unlock(miModule);
	//printf("S %i\n", miModule->Status[2]);
	
	return result;
}

int external_set_status_module(MODULE_INFO *miModule, int iSubModule, int iValue)
{
	if (!miModule->InitParams[0]) 
	{
		dbgprintf(2,"External module %.4s set status error, did not inited\n", (char*)&miModule->ID);
		return 0;
	}
	
	if ((iSubModule < 0) || (iSubModule >= miModule->Settings[5]))
	{
		dbgprintf(2,"External module %.4s set status error, submodule num out of range (1<%i<%i)\n", (char*)&miModule->ID, iSubModule+1, miModule->Settings[5]+1);
		return 0;
	}
	
	usb_gpio_lock(miModule);
	
	if (miModule->Settings[0] == 1) 
	{
		DBG_MUTEX_LOCK(&i2c_mutex);
		i2c_set_address(miModule->InitParams[0], miModule->Settings[1]);
	}
	
	char buff[8];
	int ret;
	int result = 0;	
	
	do
	{
		buff[0] = 255;
		buff[1] = miModule->Settings[1];
		buff[2] = EXTERNAL_COMMAND_SET_STATUS;
		buff[3] = iSubModule & 0xFF;
		
		memcpy(&buff[4], &iValue, 4);
		if (write(miModule->InitParams[0], buff, 8) != 8)
		{
			dbgprintf(2,"Error write to module %.4s\n", (char*)&miModule->ID);
			break;
		}
		ret = read_t(miModule->InitParams[0], (char*)buff, 3, 100);
		if (ret != 3)
		{
			if (ret < 0) dbgprintf(2,"external_set %.4s error read: %s (%i)\n", (char*)&miModule->ID, strerror(errno), errno);
				else
				dbgprintf(2,"external_set_status %.4s wrong ret len read: %i/3\n", (char*)&miModule->ID, ret);
			break;
		}
		if ((buff[0] != 255) || (buff[1] != 0) || (buff[2] != EXTERNAL_COMMAND_OK))		
		{
			dbgprintf(2,"External module set status %.4s error\n", (char*)&miModule->ID);
			break;
		}
		result = 1;
	} while(0);
	
	if (miModule->Settings[0] == 1)	DBG_MUTEX_UNLOCK(&i2c_mutex);
	
	usb_gpio_unlock(miModule);
	
	return result;
}

int get_uart_speed_code(int iSpeed)
{
	int iSpeedCode = 0;
	switch(iSpeed)
	{
		case 0: iSpeedCode = B0; break;
		case 50: iSpeedCode = B50; break;
		case 75: iSpeedCode = B75; break;
		case 110: iSpeedCode = B110; break;
		case 134: iSpeedCode = B134; break;
		case 150: iSpeedCode = B150; break;
		case 200: iSpeedCode = B200; break;
		case 300: iSpeedCode = B300; break;
		case 600: iSpeedCode = B600; break;
		case 1200: iSpeedCode = B1200; break;
		case 1800: iSpeedCode = B1800; break;
		case 2400: iSpeedCode = B2400; break;
		case 4800: iSpeedCode = B4800; break;
		case 9600: iSpeedCode = B9600; break;
		case 19200: iSpeedCode = B19200; break;
		case 38400: iSpeedCode = B38400; break;
		case 57600: iSpeedCode = B57600; break;
		case 115200: iSpeedCode = B115200; break;
		case 230400: iSpeedCode = B230400; break;
		case 460800: iSpeedCode = B460800; break;
		case 500000: iSpeedCode = B500000; break;
		case 576000: iSpeedCode = B576000; break;
		case 921600: iSpeedCode = B921600; break;
		case 1000000: iSpeedCode = B1000000; break;
		case 1152000: iSpeedCode = B1152000; break;
		case 1500000: iSpeedCode = B1500000; break;
		case 2000000: iSpeedCode = B2000000; break;
		case 2500000: iSpeedCode = B2500000; break;
		case 3000000: iSpeedCode = B3000000; break;
		case 3500000: iSpeedCode = B3500000; break;
		case 4000000: iSpeedCode = B4000000; break;
		default : iSpeedCode = 0; break;		
	}
	return iSpeedCode;
}

int get_uart_code_to_speed(int iCode)
{
	int iSpeedCode = 0;
	switch(iCode)
	{
		case 0: iSpeedCode = 0; break;
		case 1: iSpeedCode = 50; break;
		case 2: iSpeedCode = 75; break;
		case 3: iSpeedCode = 110; break;
		case 4: iSpeedCode = 134; break;
		case 5: iSpeedCode = 150; break;
		case 6: iSpeedCode = 200; break;
		case 7: iSpeedCode = 300; break;
		case 8: iSpeedCode = 600; break;
		case 9: iSpeedCode = 1200; break;
		case 10: iSpeedCode = 1800; break;
		case 11: iSpeedCode = 2400; break;
		case 12: iSpeedCode = 4800; break;
		case 13: iSpeedCode = 9600; break;
		case 14: iSpeedCode = 19200; break;
		case 15: iSpeedCode = 38400; break;
		case 16: iSpeedCode = 57600; break;
		case 17: iSpeedCode = 115200; break;
		case 18: iSpeedCode = 230400; break;
		case 19: iSpeedCode = 460800; break;
		case 20: iSpeedCode = 500000; break;
		case 21: iSpeedCode = 576000; break;
		case 22: iSpeedCode = 921600; break;
		case 23: iSpeedCode = 1000000; break;
		case 24: iSpeedCode = 1152000; break;
		case 25: iSpeedCode = 1500000; break;
		case 26: iSpeedCode = 2000000; break;
		case 27: iSpeedCode = 2500000; break;
		case 28: iSpeedCode = 3000000; break;
		case 29: iSpeedCode = 3500000; break;
		case 30: iSpeedCode = 4000000; break;
		default : iSpeedCode = 0; break;		
	}
	return iSpeedCode;
}

int get_i2c_code_to_speed(int iCode)
{
	int iSpeedCode = 0;
	switch(iCode)
	{
		case 0: iSpeedCode = 0; break;
		case 1: iSpeedCode = 100000; break;
		case 2: iSpeedCode = 400000; break;
		default : iSpeedCode = 0; break;		
	}
	return iSpeedCode;
}

int get_i2c_speed_to_code(int iSpeed)
{
	int iSpeedCode = 0;
	switch(iSpeed)
	{
		case 0: iSpeedCode = 1; break;
		case 100000: iSpeedCode = 1; break;
		case 400000: iSpeedCode = 2; break;
		default : iSpeedCode = 1; break;		
	}
	return iSpeedCode;
}

int get_uart_speed_to_code(int iSpeed)
{
	int iSpeedCode = 0;
	switch(iSpeed)
	{
		case 0: iSpeedCode = 13; break;
		case 50: iSpeedCode = 1; break;
		case 75: iSpeedCode = 2; break;
		case 110: iSpeedCode = 3; break;
		case 134: iSpeedCode = 4; break;
		case 150: iSpeedCode = 5; break;
		case 200: iSpeedCode = 6; break;
		case 300: iSpeedCode = 7; break;
		case 600: iSpeedCode = 8; break;
		case 1200: iSpeedCode = 9; break;
		case 1800: iSpeedCode = 10; break;
		case 2400: iSpeedCode = 11; break;
		case 4800: iSpeedCode = 12; break;
		case 9600: iSpeedCode = 13; break;
		case 19200: iSpeedCode = 14; break;
		case 38400: iSpeedCode = 15; break;
		case 57600: iSpeedCode = 16; break;
		case 115200: iSpeedCode = 17; break;
		case 230400: iSpeedCode = 18; break;
		case 460800: iSpeedCode = 19; break;
		case 500000: iSpeedCode = 20; break;
		case 576000: iSpeedCode = 21; break;
		case 921600: iSpeedCode = 22; break;
		case 1000000: iSpeedCode = 23; break;
		case 1152000: iSpeedCode = 24; break;
		case 1500000: iSpeedCode = 25; break;
		case 2000000: iSpeedCode = 26; break;
		case 2500000: iSpeedCode = 27; break;
		case 3000000: iSpeedCode = 28; break;
		case 3500000: iSpeedCode = 29; break;
		case 4000000: iSpeedCode = 30; break;
		default : iSpeedCode = 13; break;		
	}
	return iSpeedCode;
}

int uart_open(char *cPort, int iSpeed)
{	
	int reslt = 0;
	int iSpeedCode = get_uart_speed_code(iSpeed);
	if (iSpeedCode == 0) 
	{		
		dbgprintf(2,"Error - Wrong speed: %i set 9600\n", iSpeed);
		iSpeed = 9600;
	}
	reslt = open(cPort, O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	//reslt = open(cPort, O_RDWR | O_NOCTTY | O_NONBLOCK);		//Open in non blocking read/write mode	
	if (reslt == -1)
	{
		dbgprintf(1,"Error - Unable to open UART.  Ensure it is not in use by another application\n");
		return -1;
	}
	
	dbgprintf(4,"uart_open %s %i %i\n", cPort, iSpeed, reslt);
	
	struct termios options;
	tcgetattr(reslt, &options);
	//options.c_cflag = iSpeedCode | CS8 | CLOCAL | CREAD | PARODD | PARENB ;		//<Set baud rate
	options.c_cflag = iSpeedCode | CS8 | CREAD;		//<Set baud rate
	//options.c_iflag = IGNPAR;
	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
	//options.c_cc[VMIN] = 0;
    //options.c_cc[VTIME] = 0;
 
	tcflush(reslt, TCIFLUSH);
	tcsetattr(reslt, TCSANOW, &options);	
	
	return reslt;
}

int uart_init_port(int iPortNum, int iPortSpeed)
{
	char cPortName[32];
	if (!get_uart_port_path(iPortNum, cPortName, 32)) 
	{
		dbgprintf(2, "UART port %i: Not available\n", iPortNum);		
		return 0;
	}
	
	int iPort = uart_open(cPortName, iPortSpeed);
	
	if (iPort <= 0) 
	{
		dbgprintf(2, "Error connect to UART port: %s, speed:%i\n", cPortName, iPortSpeed);
		return 0;
	}
	return iPort;
}
	

char uart_init()
{		
	//uart0_filestream = uart_open("/dev/ttyAMA0", iSpeed);
	//if (uart0_filestream == 0) return 0;
		
	pthread_mutex_init(&uart_mutex, NULL);   
	
	return 1;
}

char gpio_init(MODULE_INFO *miModule, int iModCnt)
{		
	int n;
	for (n = 0; n != iModCnt; n++)
	{
		if ((miModule[n].Enabled & 1) && (miModule[n].Type == MODULE_TYPE_GPIO))
		{
			//dbgprintf(1,"init %s, pin: %i = %i\n",&miModule[n].ID,miModule[n].Settings[3],gpio_get_pin_code(miModule[n].Settings[3]));
			miModule[n].Settings[3] = gpio_get_pin_code(miModule[n].Settings[1]);
			if (miModule[n].Settings[0] & MODULE_SECSET_OUTPUT)
			{
				bcm2835_gpio_fsel(miModule[n].Settings[3], BCM2835_GPIO_FSEL_OUTP);
				if (miModule[n].Settings[0] & MODULE_SECSET_INVERSE) 
					bcm2835_gpio_write(miModule[n].Settings[3], HIGH);
					else 
					bcm2835_gpio_write(miModule[n].Settings[3], LOW);
			}
			else 
			{
				bcm2835_gpio_fsel(miModule[n].Settings[3], BCM2835_GPIO_FSEL_INPT);
				if (miModule[n].Settings[0] & MODULE_SECSET_INVERSE) 
					bcm2835_gpio_set_pud(miModule[n].Settings[3], BCM2835_GPIO_PUD_UP);
					else
					bcm2835_gpio_set_pud(miModule[n].Settings[3], BCM2835_GPIO_PUD_DOWN);
			}			
		}
	}
	//bcm2835_gpio_fsel(PIN_UART, BCM2835_GPIO_FSEL_OUTP);
	//bcm2835_gpio_write(PIN_UART, LOW);		
	//bcm2835_gpio_fsel(PIN_SOUND, BCM2835_GPIO_FSEL_OUTP);
	//bcm2835_gpio_write(PIN_SOUND, LOW);
	/*bcm2835_gpio_fsel(PIN_SCREEN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(PIN_SCREEN, LOW);
	bcm2835_delay(10000);*/
	//bcm2835_gpio_write(PIN_SOUND, HIGH);
	
	//bcm2835_gpio_fsel(PIN_SCREEN, BCM2835_GPIO_FSEL_INPT);
	//bcm2835_gpio_fsel(PIN_MENU, BCM2835_GPIO_FSEL_INPT);
	//bcm2835_gpio_set_pud(PIN_SCREEN, BCM2835_GPIO_PUD_DOWN);	//set low level	
	
	//while (bcm2835_gpio_lev(PIN_MENU) == 1) ;//bcm2835_delay(500);
	ucGPIO_List_Cnt = 0;
	cThreadGpioStatus = 0;
	memset(tGPIO_List, 0, sizeof(tGPIO_List));
	pevntGPIOWork = DBG_MALLOC(sizeof(TX_EVENTER));
	tx_eventer_create(pevntGPIOWork, 1);
	pthread_mutex_init(&GPIOWork_mutex, NULL);   
	pthread_mutex_init(&bcm_mutex, NULL); 
	pthread_attr_init(&tattrGPIOWork);      
	pthread_attr_setdetachstate(&tattrGPIOWork, PTHREAD_CREATE_DETACHED);	
	pthread_create(&threadGPIOWork, &tattrGPIOWork, GPIOWorker, NULL);
		
    return 1;
}

char gpio_close(void)
{
	StopGPIOWork();
	
	int ret;
	do
	{
		DBG_MUTEX_LOCK(&GPIOWork_mutex);
		ret = cThreadGpioStatus;
		DBG_MUTEX_UNLOCK(&GPIOWork_mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);
	
	pthread_attr_destroy(&tattrGPIOWork);	
	tx_eventer_delete(pevntGPIOWork);
	DBG_FREE(pevntGPIOWork);
	pthread_mutex_destroy(&GPIOWork_mutex);
	pthread_mutex_destroy(&bcm_mutex);
	
	return 1;
}

int write_t(int stream, char *cdata, int ilen, unsigned int itime)
{
	int count = 0;
	int ret;
	if (stream > 0)
	{			
		do
		{
			ret = write(stream, cdata + count, ilen - count);
			if (ret > 0) count += ret;		
			if ((itime != 0) && (count != ilen)) {usleep(1000);itime--;}				
		} while ((itime != 0) && (count < ilen));	
	} else count = -1;
	return count;
}

/*void uart_printf_array(uint8_t *Buff, uint8_t iLen)
{
	printf("Arr: [%i] : ", iLen);
	int i;
	for (i = 0; i < iLen; i++) printf("%i ", Buff[i]);
	printf("\r\n");
}*/

int uart_write(char *cdata, int ilen, unsigned int itime)
{
	return uart_write_port(uart0_filestream, cdata, ilen, itime);
}

int uart_write_port(int iPort, char *cdata, int ilen, unsigned int itime)
{
	DBG_MUTEX_LOCK(&uart_mutex);
	//uart_printf_array(cdata, ilen);
	int count = 0;
	int ret;
	if (iPort > 0)
	{			
		do
		{
			ret = write(iPort, cdata + count, ilen - count);
			if (ret > 0) count += ret;		
			if ((itime != 0) && (count != ilen)) {usleep(1000);itime--;}				
		} while ((itime != 0) && (count != ilen));	
	} else count = -1;
	//printf("uart_write %i\n",count);
	DBG_MUTEX_UNLOCK(&uart_mutex);
	
	return count;
}

int uart_read(char *cdata, int ilen, unsigned int itime)
{
	return uart_read_port(uart0_filestream, cdata, ilen, itime);
}

int uart_read_port(int iPort, char *cdata, int ilen, unsigned int itime)
{
	DBG_MUTEX_LOCK(&uart_mutex);
	
	int count = 0;
	int ret;
	if (iPort > 0)
	{			
		do
		{
			ret = read(iPort, cdata + count, ilen - count);
			//if (ret > 0) dbgprintf(1,"Readed:%i\n",ret);
			if (ret > 0) count += ret;	
			if ((itime != 0) && (count != ilen)) {usleep(1000);itime--;}			
		} while ((itime != 0) && (count != ilen));	
	} else count = -1;
	//printf("uart_read %i\n",count);
	
	DBG_MUTEX_UNLOCK(&uart_mutex);
	
	return count;
}

int read_t(int iStream, char *cdata, int ilen, unsigned int itime)
{
	int count = 0;
	int ret;
	if (iStream > 0)
	{			
		do
		{
			ret = read(iStream, cdata + count, ilen - count);
			//if (ret > 0) dbgprintf(1,"Readed:%i\n",ret);
			if (ret > 0) count += ret;	
			if ((itime != 0) && (count != ilen)) {usleep(1000);itime--;}			
		} while ((itime != 0) && (count != ilen));	
	} else count = -1;
	//printf("uart_read %i\n",count);	
	return count;
}

int uart_clear_port(int iPort)
{
	int ret = 0;
	if (iPort > 0)
	{			
		char data;
		while (read(iPort, &data, 1) > 0) ret++;
	}
	if (ret) dbgprintf(3,"Cleared UART port: %i bytes\n", ret);
	return ret;
}

char uart_put(char *cdata, int ilen, MODULE_INFO *miModule)
{
	int ret = 1;
	DBG_MUTEX_LOCK(&uart_mutex);	
	//----- TX BYTES -----
	if (uart0_filestream > 0)
	{			
		gpio_switch_on_module(miModule);
		usleep(10000);
		int count = write(uart0_filestream, cdata, ilen);		//Filestream, bytes to write, number of bytes to write
		usleep(10000);
		gpio_switch_off_module(miModule);
		if (count < 0)
		{
			ret = 0;
		}
		
	} else ret = -1;
	
	DBG_MUTEX_UNLOCK(&uart_mutex);
		
	return ret;
}

char put_char(char cdata, MODULE_INFO *miModule)
{
	int ret = 1;
	DBG_MUTEX_LOCK(&uart_mutex);
	//----- TX BYTES -----	
	if (uart0_filestream > 0)
	{			
		if (miModule) 
		{
			gpio_switch_on_module(miModule);
			usleep(10000);
		}
		int count = write(uart0_filestream, &cdata, 1);		//Filestream, bytes to write, number of bytes to write
		if (miModule) 
		{
			usleep(10000);
			gpio_switch_off_module(miModule);
		}
		if (count < 0)
		{
			ret = 0;
		}		
	} else ret = -1;
	
	DBG_MUTEX_UNLOCK(&uart_mutex);
	
	return ret;
}

int uart_get(char *cdata, int ilen)
{
	if (uart0_filestream <= 0) return 0;
	//----- CHECK FOR ANY RX BYTES -----
	memset(cdata, 0, ilen);	
	if (uart0_filestream > 0)
	{
		int rx_length = read(uart0_filestream, (void*)cdata, ilen);		//Filestream, buffer to store in, number of bytes to read (max)
		if (rx_length < 0)
		{
			return rx_length*10000; //An error occured (will occur if there are no bytes)
		}
		if (rx_length == 0)
		{
			return 0; //No data waiting
		}
		return rx_length;		
	}
	return -1;
}

int get_char()
{
	int ret = -3;
	DBG_MUTEX_LOCK(&uart_mutex);	
	//----- CHECK FOR ANY RX BYTES -----
	char cdata;
	if (uart0_filestream > 0)
	{
		int rx_length = read(uart0_filestream, (void*)&cdata, 1);		//Filestream, buffer to store in, number of bytes to read (max)
		if (rx_length > 0) ret = (char)(cdata);
			else ret = -2; //No data
	} else ret = -1;
	
	DBG_MUTEX_UNLOCK(&uart_mutex);
	return ret;
}

char uart_close_port(int iPort)
{	
	if (iPort <= 0) return 0;
	close(iPort);

	return 0;
}

char uart_close(void)
{	
	uart0_filestream = 0;
	pthread_mutex_destroy(&uart_mutex);

	return 0;
}

char spi_close(void)
{
	if (spi_filestream <= 0) return 0;
	//----- CLOSE THE SPI -----
	close(spi_filestream);
	spi_filestream = 0;
	pthread_mutex_destroy(&spi_mutex);
	return 0;
}

void gpio_switch_on(int iPin)
{
	DBG_MUTEX_LOCK(&bcm_mutex);
	bcm2835_gpio_write(iPin, HIGH);
	DBG_MUTEX_UNLOCK(&bcm_mutex);	
}

void gpio_switch_off(int iPin)
{
	DBG_MUTEX_LOCK(&bcm_mutex);
	bcm2835_gpio_write(iPin, LOW);
	DBG_MUTEX_UNLOCK(&bcm_mutex);
}

char gpio_get_status(int iPin)
{
	char ret;
	
	DBG_MUTEX_LOCK(&bcm_mutex);
	ret = bcm2835_gpio_lev(iPin);
	DBG_MUTEX_UNLOCK(&bcm_mutex);
	
	return ret;
}

char gpio_get_status_module(MODULE_INFO *miModule)
{
	char ret = -1;
	
	DBG_MUTEX_LOCK(&bcm_mutex);
	if (miModule->Type == MODULE_TYPE_GPIO)
		ret = bcm2835_gpio_lev(miModule->Settings[3]) ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE);
	DBG_MUTEX_UNLOCK(&bcm_mutex);	
	
	return ret;
}

int get_status_module_temp(MODULE_INFO *miModule)
{
	int ret = 0;
	
	DBG_MUTEX_LOCK(&bcm_mutex);
	if (miModule->Type == MODULE_TYPE_TEMP_SENSOR)
		ret = bcm2835_gpio_lev(miModule->Settings[3]) ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE);
	DBG_MUTEX_UNLOCK(&bcm_mutex);	
	
	return ret;
}


void gpio_switch_off_module(MODULE_INFO *miModule)
{
	if ((miModule->Type == MODULE_TYPE_GPIO) && (miModule->Settings[0] && MODULE_SECSET_OUTPUT))
	{
		DBG_MUTEX_LOCK(&bcm_mutex);	
		bcm2835_gpio_write(miModule->Settings[3], 0 ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE));		
		DBG_MUTEX_UNLOCK(&bcm_mutex);	
		/*if (miModule->Settings[2] != 0)
		{
			int n;
			int iRet = 0;
			DBG_MUTEX_LOCK(&GPIOWork_mutex);						
			for (n = 0; n != GPIO_WORKS_MAX; n++)
			{
				if (tGPIO_List[n].Number == 0)
				{
					iRet = 1;
					tGPIO_List[n].Number = miModule->Settings[3];
					tGPIO_List[n].Delay = miModule->Settings[2] / 10;
					tGPIO_List[n].Set = 1 ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE);		
					ucGPIO_List_Cnt++;
					StartGPIOWork();
					break;
				}
			}
			DBG_MUTEX_UNLOCK(&GPIOWork_mutex);
			if (iRet == 0)
			{
				usleep(miModule->Settings[2]*1000);
				DBG_MUTEX_LOCK(&bcm_mutex);	
				bcm2835_gpio_write(miModule->Settings[3], 1 ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE));		
				DBG_MUTEX_UNLOCK(&bcm_mutex);	
			}	
		}*/
	}	
}

void gpio_switch_on_module(MODULE_INFO *miModule)
{
	int iRet, n;
	if ((miModule->Type == MODULE_TYPE_GPIO) && (miModule->Settings[0] && MODULE_SECSET_OUTPUT))
	{
		DBG_MUTEX_LOCK(&bcm_mutex);		
		bcm2835_gpio_write(miModule->Settings[3], 1 ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE));
		DBG_MUTEX_UNLOCK(&bcm_mutex);		
		if (miModule->Settings[2] != 0)
		{
			iRet = 0;
			DBG_MUTEX_LOCK(&GPIOWork_mutex);						
			for (n = 0; n != GPIO_WORKS_MAX; n++)
			{
				if (tGPIO_List[n].Number == 0)
				{
					iRet = 1;
					tGPIO_List[n].Number = miModule->Settings[3];
					tGPIO_List[n].Delay = miModule->Settings[2] / 10;
					tGPIO_List[n].Set = 0 ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE);	
					ucGPIO_List_Cnt++;
					StartGPIOWork();
					break;
				}
			}
			DBG_MUTEX_UNLOCK(&GPIOWork_mutex);
			if (iRet == 0)
			{
				usleep(miModule->Settings[2]*1000);
				DBG_MUTEX_LOCK(&bcm_mutex);
				bcm2835_gpio_write(miModule->Settings[3], 0 ^ ((miModule->Settings[0] & MODULE_SECSET_INVERSE) == MODULE_SECSET_INVERSE));		
				DBG_MUTEX_UNLOCK(&bcm_mutex);
			}
		}			
	}	
}

void* GPIOWorker(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	int iLoop = 1;
	int iLoop2, iCnt;
	int n, ret;
	unsigned int uiEvent = 0;
	
	pthread_setname_np(pthread_self(), "GPIO_Worker");
	
	DBG_MUTEX_LOCK(&GPIOWork_mutex);
	cThreadGpioStatus = 1;
	DBG_MUTEX_UNLOCK(&GPIOWork_mutex);
	
	tx_eventer_add_event(pevntGPIOWork, EVENT_START);
	tx_eventer_add_event(pevntGPIOWork, EVENT_STOP);		
	do
	{
		uiEvent = 0;
		ret = tx_eventer_recv(pevntGPIOWork, &uiEvent, 30000, 0);
		if (ret != 0)
		{		
			if (uiEvent == EVENT_STOP) iLoop = 0;
			if (uiEvent == EVENT_START) 
			{
				do
				{
					DBG_MUTEX_LOCK(&GPIOWork_mutex);
					iCnt = ucGPIO_List_Cnt;
					iLoop2 = 0;			
					for (n = 0; n != GPIO_WORKS_MAX; n++)
					{
						if (tGPIO_List[n].Number != 0)
						{
							iLoop2++;
							if (tGPIO_List[n].Delay > 0) tGPIO_List[n].Delay--;
							if (tGPIO_List[n].Delay == 0) 
							{
								DBG_MUTEX_LOCK(&bcm_mutex);	
								bcm2835_gpio_write(tGPIO_List[n].Number, tGPIO_List[n].Set);	
								DBG_MUTEX_UNLOCK(&bcm_mutex);	
								tGPIO_List[n].Number = 0;
								iLoop2--;
							}
							iCnt--;
							if (iCnt == 0) break;
						}
					}
					DBG_MUTEX_UNLOCK(&GPIOWork_mutex);
					if (iLoop2) usleep(10000);
				} while(iLoop2);
			}
		}
		//if (ret == 0) dbgprintf(1,"Timeout GPIOWorker\n"); else dbgprintf(1,"Signal GPIOWorker\n");
	} while (iLoop);
	
	DBG_MUTEX_LOCK(&GPIOWork_mutex);
	cThreadGpioStatus = 0;
	DBG_MUTEX_UNLOCK(&GPIOWork_mutex);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return 0;
}
