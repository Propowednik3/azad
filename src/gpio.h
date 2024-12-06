#ifndef _GPIO_H_
#define _GPIO_H_

#include "bcm2835.h"
#include "main.h"

//#define PIN_SOUND RPI_GPIO_P1_05
//#define PIN_SCREEN RPI_GPIO_P1_03
//#define PIN_SCREEN 	RPI_V2_GPIO_P1_05
//#define ID_PIN_MENU 	0x554E454D	//MENU
//#define ID_PIN_SPEAKER 	0x524B5053	//SPKR
//#define ID_PIN_UART 	0x54524155	//UART
//#define ID_ACCESS 	0x53454341	//ACES
//#define ID_ALL 		0x004C4C41	//ALL

#define I2C_ADDRESS_CLOCK		104
#define I2C_ADDRESS_RADIO		16
#define I2C_ADDRESS_AM2320		92
#define I2C_ADDRESS_LM75		72
#define I2C_ADDRESS_ADS1015		72
#define I2C_ADDRESS_MCP3421		104
#define I2C_ADDRESS_AS5600		54
#define I2C_ADDRESS_HMC5883L	30

#define USBIO_PIN_SETT_TYPE_MASK		255
#define USBIO_MAIN_SETT_PORT_NUM_MASK	0xFFFF
#define USBIO_MAIN_SETT_AUTOSCAN 		0b00001000
#define USBIO_MAIN_SETT_MAINSCAN		0b00010000
#define USBIO_MAIN_SETT_SAVE_IN_MODULE	0b00100000
#define USBIO_MAIN_SETT_PARAMS_MASK		0b11111000

enum  USBIO_PIN_SETT_TYPE
{
	USBIO_PIN_SETT_TYPE_DISABLED,
	USBIO_PIN_SETT_TYPE_BUSY,
	USBIO_PIN_SETT_TYPE_UNSUPPORTED,
	USBIO_PIN_SETT_TYPE_WRONG_PARAMS,	
	USBIO_PIN_SETT_TYPE_INPUT,
	USBIO_PIN_SETT_TYPE_OUTPUT,
	USBIO_PIN_SETT_TYPE_IR_RECIEVER,
	USBIO_PIN_SETT_TYPE_I2C_LM75,
	USBIO_PIN_SETT_TYPE_I2C_AM2320,
	USBIO_PIN_SETT_TYPE_RS485,
	USBIO_PIN_SETT_TYPE_RS485_PN532,
	USBIO_PIN_SETT_TYPE_RS485_RC522,
	USBIO_PIN_SETT_TYPE_I2C_MISC,
	USBIO_PIN_SETT_TYPE_RS485_MISC,	
	USBIO_PIN_SETT_TYPE_I2C_ADS1015,
	USBIO_PIN_SETT_TYPE_I2C_MCP3421,
	USBIO_PIN_SETT_TYPE_I2C_AS5600,
	USBIO_PIN_SETT_TYPE_I2C_HMC5883L,
	USBIO_PIN_SETT_TYPE_MAX
};


#define USBIO_PIN_PARAM_TYPE_DIGITAL_IN 		0x00000001
#define USBIO_PIN_PARAM_TYPE_DIGITAL_OUT_OD		0x00000002
#define USBIO_PIN_PARAM_TYPE_DIGITAL_OUT_PP		0x00000004
#define USBIO_PIN_PARAM_TYPE_ANALOG_IN			0x00000008
#define USBIO_PIN_PARAM_TYPE_PWM_OUT_OD			0x00000010
#define USBIO_PIN_PARAM_TYPE_PWM_OUT_PP			0x00000020
#define USBIO_PIN_PARAM_TYPE_IR_RECIEVER		0x00000040
#define USBIO_PIN_PARAM_TYPE_20MA				0x00000080
#define USBIO_PIN_PARAM_TYPE_3MA				0x00000100
#define USBIO_PIN_PARAM_TYPE_TOLERANT_5V		0x00000200
#define USBIO_PIN_PARAM_TYPE_UART				0x00000400
//#define USBIO_PIN_PARAM_TYPE_I2C_LM75			0x00000800
//#define USBIO_PIN_PARAM_TYPE_I2C_AM2320			0x00001000
#define USBIO_PIN_PARAM_TYPE_RS485				0x00002000
#define USBIO_PIN_PARAM_TYPE_UART_TX			0x00004000
#define USBIO_PIN_PARAM_TYPE_UART_RX			0x00008000
#define USBIO_PIN_PARAM_TYPE_I2C_SCL			0x00010000
#define USBIO_PIN_PARAM_TYPE_I2C_SDA			0x00020000
#define USBIO_PIN_PARAM_TYPE_I2C_MISC			0x00040000


typedef struct
{
   unsigned int	Port;    
   char			Path[32];
   char 		Name[32];
   char 		Chain[32];
} uart_info;

typedef struct
{
   int		Number;    
   unsigned int		Delay;
   char 	Set;
} GPIO_LIST;

enum  USBIO_GPIO_MODE
{
  USBIO_CMD_MODE_STATIC,
  USBIO_CMD_MODE_IMPULSE,
  USBIO_CMD_MODE_PERIOD,
};

enum  EXTERNAL_COMMAND
{	
  EXTERNAL_COMMAND_INIT = 1,
  EXTERNAL_COMMAND_DEINIT,
  EXTERNAL_COMMAND_GET_COUNT,
  EXTERNAL_COMMAND_GET_STATUSES,
  EXTERNAL_COMMAND_SET_STATUS,  
  EXTERNAL_COMMAND_STATUS,
  EXTERNAL_COMMAND_OK,
  EXTERNAL_COMMAND_BAD,
  EXTERNAL_COMMAND_GET_STATUS,
  EXTERNAL_COMMAND_COUNT,
  EXTERNAL_COMMAND_STATUSES
};

/// Band datatype.
/// The BANDs a receiver probably can implement.
enum  
{
  RADIO_BAND_NONE    = 0, ///< No band selected.
  RADIO_BAND_FM      = 1, ///< FM band 87.5 – 108 MHz (USA, Europe) selected.
  RADIO_BAND_FMWORLD = 2, ///< FM band 76 – 108 MHz (Japan, Worldwide) selected.
  RADIO_BAND_AM      = 3, ///< AM band selected.
  RADIO_BAND_KW      = 4, ///< KW band selected.
  RADIO_BAND_MAX     = 4  ///< Maximal band enumeration value.
};

enum  
{
  RADIO_BAND_US_EU    	= 0, ///< 87–108 MHz (US/Europe) 
  RADIO_BAND_JA      	= 1, ///< 76–91 MHz (Japan) 
  RADIO_BAND_WORLD 		= 2, ///< 76–108 MHz (world wide) 
  RADIO_BAND_EEU      	= 3, ///< 65 –76 MHz （East Europe）  or 50-65MHz 
};

/// Frequency data type.
/// Only 16 bits are used for any frequency value (not the real one)
typedef uint16_t RADIO_FREQ;

/// A structure that contains information about the radio features from the chip.
/*typedef struct RADIO_INFO {
  char active;   ///< receiving is active.
  uint8_t rssi;  ///< Radio Station Strength Information.
  char rds;      ///< RDS information is available.
  char tuned;    ///< A stable frequency is tuned.
  char mono;     ///< Mono mode is on.
  char stereo;   ///< Stereo audio is available
} _RADIO_INFO;*/


/// a structure that contains information about the audio features
typedef struct AUDIO_INFO {
  uint8_t volume;
  char mute;
  char softmute;
  char bassBoost;
} _AUDIO_INFO;

extern uint16_t *cRadioRegisters;
extern char cRadioStatus;

char uart_init();
int uart_init_port(int iPortNum, int iPortSpeed);
int uart_write(char *cdata, int ilen, unsigned int itime);
int uart_read(char *cdata, int ilen, unsigned int itime);
int uart_write_port(int iPort, char *cdata, int ilen, unsigned int itime);
int uart_read_port(int iPort, char *cdata, int ilen, unsigned int itime);
char put_char(unsigned char cdata, MODULE_INFO *miModule);
int put_chars(uint8_t *pdata, uint16_t data_len, MODULE_INFO *miModule);
int get_char();
char uart_close(void);
char uart_close_port(int iPort);
int get_uart_speed(int iCode);
void get_uart_ports(uart_info **tty_list, uint32_t *tty_length);
char* get_uart_chain_name(unsigned int uiPortNum, char *chain, unsigned int chainlen);
int uart_clear_port(int iPort);
int write_t(int stream, char *cdata, int ilen, unsigned int itime);
int read_t(int iStream, char *cdata, int ilen, unsigned int itime);

int usb_gpio_init(MODULE_INFO *miModule);
int usb_gpio_close(MODULE_INFO *miModule);
int usb_gpio_get_status_module(MODULE_INFO *miModule, char cFull);
int usb_gpio_authenticate_card(MODULE_INFO *miModule, unsigned int sensenum, uint8_t iBlock, void *pKeyInfo, unsigned int uiLength);
int usb_gpio_set_status_module(MODULE_INFO *miModule, int iSubModule, int iValue);
int usb_gpio_write_data_block(MODULE_INFO *miModule, unsigned int sensenum, int iBlock, int iAction, void *pKeyInfo, uint32_t uiLength, uint8_t* pData, uint32_t uiDataLen);
int usb_gpio_get_ir_data(MODULE_INFO *miModule, uint16_t* cOutData, unsigned int uiMaxLen, unsigned int *uiOutLen);
int usb_gpio_convert_settings(Sensor_Params *pList, unsigned int uiCount, unsigned int *pSettings, char cDirect);
int usb_gpio_get_card_serial(MODULE_INFO *miModule, uint8_t* pSensorNum, uint8_t* pSerialOut, unsigned int uiMaxLen, unsigned int *uiOutLen);
int usb_gpio_save_settings(MODULE_INFO *miModule, uint32_t* cSettings, unsigned int uiSettingsLen);
int usb_gpio_set_focus(MODULE_INFO *miModule, int iNewPos, char cAbsolute, int *iCurrPos);
char* usb_gpio_get_pull_name(unsigned int uiCode);
char* usb_gpio_get_type_name(unsigned int uiCode);
char* usb_gpio_get_mode_name(unsigned int uiCode);
int get_uart_code_to_speed(int iCode);

void i2c_init();
void i2c_scan();
void i2c_deinit();
int i2c_open(unsigned int baudrate);
char i2c_close(int filestream);
char i2c_echo(int filestream, unsigned char iAddress);
char i2c_read_to_buff_timedate3231(unsigned char iAddress, char *cBuffer, int iLen);
char i2c_read_timedate3231(unsigned char iAddress);
char i2c_read_spec_timedate3231(unsigned char iAddress, struct tm *timeinfo);
char i2c_write_timedate3231(unsigned char iAddress);
char i2c_write_spec_timedate3231(unsigned char iAddress, struct tm *stTime);
char AM2320_read(int filestream, int *iTemp, int *iHumid);
char LM75_read(int filestream, unsigned int uiAddr, int *iTemp);
char ADS1015_init(int filestream, unsigned int uiAddr, int iGain);
char ADS1015_read(int filestream, unsigned int uiAddr, int *iValue);
char MCP3421_init(int filestream, unsigned int uiAddr, int iMode, int iGain);
char MCP3421_read(int filestream, unsigned int uiAddr, unsigned int uiMode, int *iValue);
char AS5600_read(int filestream, unsigned int uiAddr, int *iAngleRaw, int *iAngle);
char HMC5883L_init(int filestream);
char HMC5883L_read(int filestream, int *iX, int *iY, int *iZ);
char RDA5807M_init();
void RDA5807M_term();
void RDA5807M_setVolume(uint8_t newVolume);
void RDA5807M_setMute(char switchOn);
int RDA5807M_getFrequency(char cReadNow);
void RDA5807M_setFrequency(RADIO_FREQ newF);
void RDA5807M_seekStop();
void RDA5807M_seekUp(char toNextSender);
void RDA5807M_seekDown(char toNextSender);

char* Get_ADS1015_RangeName(char uiCode);
char* Get_MCP3421_AccuracyName(char uiCode);
char* Get_MCP3421_GainName(char uiCode);

int external_close(MODULE_INFO *miModule);
int external_get_statuses_module(MODULE_INFO *miModule);
int external_set_status_module(MODULE_INFO *miModule, int iSubModule, int iValue);
int external_init(MODULE_INFO *miModule);

int get_i2c_code_to_speed(int iCode);

char gpio_init(MODULE_INFO *miModule, int iModuleCnt);
void gpio_switch_on_module(MODULE_INFO *miModule);
void gpio_switch_off_module(MODULE_INFO *miModule);
char gpio_get_status_module(MODULE_INFO *miModule);
char gpio_close(void);

char spi_init(unsigned long speed);
char spi_close(void);


#endif