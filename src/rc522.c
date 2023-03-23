
#include "rc522.h"

//int _sad, _reset;

/**************************************************************************/
/*!
  @brief   Writes value to a register.
  @param   addr  The address a register.
  @param   val   The value to write to a register.
 */
/**************************************************************************/
int MFRC522_writeToRegister(int iPort, uint8_t addr, uint8_t val) 
{
  //Address format: 0XXXXXX0
  uint8_t ucBuffer[2];
  char ret;
  ucBuffer[0] = addr & 0x3F;
  ucBuffer[1] = val;
  ret = uart_write_port(iPort, (char*)ucBuffer, 2, 0);
  //if (ret != 2) printf("MFRC522_writeToRegister: error write address and value send len:%i\n", ret);
  if (ret != 2) return -1;
  ret = uart_read_port(iPort, (char*)&ucBuffer[1], 1, 100);
  //if (ret != 1) printf("MFRC522_writeToRegister: error read echo uart %i\n", ret); 
  if (ret != 1) return -1;
  //else if (ucBuffer[0] != ucBuffer[1]) printf("MFRC522_writeToRegister: wrong echo write %i != %i\n",ucBuffer[0], ucBuffer[1]);  
  //printf("write addr: %i, val:%i\n", addr, val);	
  return 1;
}

/**************************************************************************/
/*!
  @brief   Reads the value at a register.
  @param   addr  The address a register.
  @returns The uint8_t at the register.
 */
/**************************************************************************/
int MFRC522_readFromRegister(int iPort, uint8_t addr) 
{
	char ret;
	uint8_t val = (addr & 0x3F) | 0x80;
	ret = uart_write_port(iPort, (char*)&val, 1, 0);
	//if (ret != 1) printf("MFRC522_readFromRegister: error write address send len:%i\n", ret);
	if (ret != 1) return -1;
	ret = uart_read_port(iPort, (char*)&val, 1, 100);
	//if (ret != 1) printf("MFRC522_readFromRegister: error read value recv len:%i\n", ret);
	if (ret != 1) return -1;
	//printf("read addr: %i, cnt:%i, val:%i\n", addr, ret, val);
	return val;
}

/**************************************************************************/
/*!
  @brief   Adds a bitmask to a register.
  @param   addr   The address a register.
  @param   mask  The mask to update the register with.
 */
/**************************************************************************/
void MFRC522_setBitMask(int iPort, uint8_t addr, uint8_t mask) 
{
  uint8_t current;
  current = MFRC522_readFromRegister(iPort, addr);
  MFRC522_writeToRegister(iPort, addr, current | mask);
}

/**************************************************************************/
/*!
  @brief   Removes a bitmask from the register.
  @param   reg   The address a register.
  @param   mask  The mask to update the register with.
 */
/**************************************************************************/
void MFRC522_clearBitMask(int iPort, uint8_t addr, uint8_t mask) 
{
  uint8_t current;
  current = MFRC522_readFromRegister(iPort, addr);
  MFRC522_writeToRegister(iPort, addr, current & (~mask));
}

/**************************************************************************/
/*!
  @brief   Does the setup for the MFRC522.
 */
/**************************************************************************/
void MFRC522_begin(int iPort) 
{
  MFRC522_reset(iPort);

  //Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
  MFRC522_writeToRegister(iPort, TModeReg, 0x8D);       // Tauto=1; f(Timer) = 6.78MHz/TPreScaler
  MFRC522_writeToRegister(iPort, TPrescalerReg, 0x3E);  // TModeReg[3..0] + TPrescalerReg
  MFRC522_writeToRegister(iPort, TReloadRegL, 30);
  MFRC522_writeToRegister(iPort, TReloadRegH, 0);

  MFRC522_writeToRegister(iPort, TxAutoReg, 0x40);      // 100%ASK
  MFRC522_writeToRegister(iPort, ModeReg, 0x3D);        // CRC initial value 0x6363

  MFRC522_setBitMask(iPort, TxControlReg, 0x03);        // Turn antenna on.
}

/**************************************************************************/
/*!
  @brief   Sends a SOFTRESET command to the MFRC522 chip.
 */
/**************************************************************************/
void MFRC522_reset(int iPort) 
{
	MFRC522_writeToRegister(iPort, BitFramingReg, 0x07);  // TxLastBists = BitFramingReg[2..0]
	MFRC522_writeToRegister(iPort, CommandReg, MFRC522_SOFTRESET);
}

void MFRC522_cancel(int iPort) 
{
  MFRC522_writeToRegister(iPort, CommandReg, MFRC522_IDLE);
}

void MFRC522_wakeup(int iPort) 
{
	uint8_t val = 55;
	uart_write_port(iPort, (char*)&val, 1, 0);
	dbgprintf(3,"addr 0: %i \n",MFRC522_readFromRegister(iPort, Reserved00));
}

void MFRC522_baudrate(int iPort, int iSpeed) 
{
	uint8_t val = 0xEB;
	switch (iSpeed)
	{
		case 7200:
			val = 0xFA;
			break;
		case 9600:
			val = 0xEB;
			break;
		case 14400:
			val = 0xDA;
			break;
		case 19200:
			val = 0xCB;
			break;
		case 38400:
			val = 0xAB;
			break;
		case 57600:
			val = 0x9A;
			break;
		case 115200:
			val = 0x7A;
			break;
		case 128000:
			val = 0x74;
			break;
		case 230400:
			val = 0x5A;
			break;
		case 460800:
			val = 0x3A;
			break;
		case 921600:
			val = 0x1C;
			break;
		case 1228800:
			val = 0x15;
			break;
		default:
			val = 0xEB;
			break;
	}
	dbgprintf(3,"%i\n", val);
	MFRC522_writeToRegister(iPort, SerialSpeedReg, val);
}

/**************************************************************************/
/*!
  @brief   Checks the firmware version of the chip.
  @returns The firmware version of the MFRC522 chip.
 */
/**************************************************************************/
uint8_t MFRC522_getFirmwareVersion(int iPort) 
{
  uint8_t response;
  response = MFRC522_readFromRegister(iPort, VersionReg);
  return response;
}

/**************************************************************************/
/*!
  @brief   Runs the digital self test.
  @returns True if the self test passes, false otherwise.
 */
/**************************************************************************/
char MFRC522_digitalSelfTestPass(int iPort) 
{
  int i;
  uint8_t n;

  uint8_t selfTestResultV1[] = {0x00, 0xC6, 0x37, 0xD5, 0x32, 0xB7, 0x57, 0x5C,
                          0xC2, 0xD8, 0x7C, 0x4D, 0xD9, 0x70, 0xC7, 0x73,
                          0x10, 0xE6, 0xD2, 0xAA, 0x5E, 0xA1, 0x3E, 0x5A,
                          0x14, 0xAF, 0x30, 0x61, 0xC9, 0x70, 0xDB, 0x2E,
                          0x64, 0x22, 0x72, 0xB5, 0xBD, 0x65, 0xF4, 0xEC,
                          0x22, 0xBC, 0xD3, 0x72, 0x35, 0xCD, 0xAA, 0x41,
                          0x1F, 0xA7, 0xF3, 0x53, 0x14, 0xDE, 0x7E, 0x02,
                          0xD9, 0x0F, 0xB5, 0x5E, 0x25, 0x1D, 0x29, 0x79};
  uint8_t selfTestResultV2[] = {0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95,
                          0xD0, 0xE3, 0x0D, 0x3D, 0x27, 0x89, 0x5C, 0xDE,
                          0x9D, 0x3B, 0xA7, 0x00, 0x21, 0x5B, 0x89, 0x82,
                          0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5, 0x00, 0x49,
                          0x7C, 0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81,
                          0x5D, 0x48, 0x76, 0xD5, 0x71, 0x61, 0x21, 0xA9,
                          0x86, 0x96, 0x83, 0x38, 0xCF, 0x9D, 0x5B, 0x6D,
                          0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F};
  uint8_t *selfTestResult;
  switch(MFRC522_getFirmwareVersion(iPort)) 
  {
    case 0x91 :
		selfTestResult = selfTestResultV1;
		break;
    case 0x92 :
		selfTestResult = selfTestResultV2;
		break;
    default:
		return 0;		
  }

  MFRC522_reset(iPort);
  MFRC522_writeToRegister(iPort, FIFODataReg, 0x00);
  MFRC522_writeToRegister(iPort, CommandReg, MFRC522_MEM);
  MFRC522_writeToRegister(iPort, AutoTestReg, 0x09);
  MFRC522_writeToRegister(iPort, FIFODataReg, 0x00);
  MFRC522_writeToRegister(iPort, CommandReg, MFRC522_CALCCRC);

  // Wait for the self test to complete.
  i = 0xFF;
  do 
  {
	n = MFRC522_readFromRegister(iPort, DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04));

  for (i=0; i < 64; i++) 
  {
	if (MFRC522_readFromRegister(iPort, FIFODataReg) != selfTestResult[i]) 
	{		
      return 0;
    }
  }
  return 1;
}

/**************************************************************************/
/*!
  @brief   Sends a command to a tag.
  @param   cmd     The command to the MFRC522 to send a command to the tag.
  @param   data    The data that is needed to complete the command.
  @param   dlen    The length of the data.
  @param   result  The result returned by the tag.
  @param   rlen    The number of valid bits in the resulting value.
  @returns Returns the status of the calculation.
           MI_ERR        if something went wrong,
           MI_NOTAGERR   if there was no tag to send the command to.
           MI_OK         if everything went OK.
 */
/**************************************************************************/
int MFRC522_commandTag(int iPort, uint8_t cmd, uint8_t *data, int dlen, uint8_t *result, int *rlen) 
{
  int status = MI_ERR;
  uint8_t irqEn = 0x00;
  uint8_t waitIRq = 0x00;
  uint8_t lastBits, n;
  int i;

	switch (cmd) 
	{
	  case MFRC522_AUTHENT:
		irqEn = 0x12;
		waitIRq = 0x10;
		break;
	  case MFRC522_TRANSCEIVE:
		irqEn = 0x77;
		waitIRq = 0x30;
		break;
	  default:
		break;
	}

  MFRC522_writeToRegister(iPort, CommIEnReg, irqEn|0x80);    // interrupt request
  MFRC522_clearBitMask(iPort, CommIrqReg, 0x80);             // Clear all interrupt requests bits.
  MFRC522_setBitMask(iPort, FIFOLevelReg, 0x80);             // FlushBuffer=1, FIFO initialization.

  MFRC522_writeToRegister(iPort, CommandReg, MFRC522_IDLE);  // No action, cancel the current command.

  // Write to FIFO
  for (i=0; i < dlen; i++) 
  {
    MFRC522_writeToRegister(iPort, FIFODataReg, data[i]);
  }

  // Execute the command.
  MFRC522_writeToRegister(iPort, CommandReg, cmd);
  if (cmd == MFRC522_TRANSCEIVE) 
  {
    MFRC522_setBitMask(iPort, BitFramingReg, 0x80);  // StartSend=1, transmission of data starts
  }

  // Waiting for the command to complete so we can receive data.
  i = 25; // Max wait time is 25ms.
  do 
  {
    usleep(1000);
    // CommIRqReg[7..0]
    // Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
    n = MFRC522_readFromRegister(iPort, CommIrqReg);
    i--;
  } while ((i!=0) && !(n&0x01) && !(n&waitIRq));

  MFRC522_clearBitMask(iPort, BitFramingReg, 0x80);  // StartSend=0

  if (i != 0) 
  { // Request did not time out.
    i = MFRC522_readFromRegister(iPort, ErrorReg);
    if (i>=0)
	{
		if(!(i & 0x1D))
		{  // BufferOvfl Collerr CRCErr ProtocolErr
		  status = MI_OK;
		  if (n & irqEn & 0x01) 
		  {
			status = MI_NOTAGERR;
		  }

		  if (cmd == MFRC522_TRANSCEIVE) 
		  {
			n = MFRC522_readFromRegister(iPort, FIFOLevelReg);
			lastBits = MFRC522_readFromRegister(iPort, ControlReg) & 0x07;
			if (lastBits) 
			{
			  *rlen = (n-1)*8 + lastBits;
			} 
			else 
			{
			  *rlen = n*8;
			}

			if (n == 0) 
			{
			  n = 1;
			}

			if (n > MAX_LEN) 
			{
			  n = MAX_LEN;
			}

			// Reading the recieved data from FIFO.
			for (i=0; i<n; i++) 
			{
			  result[i] = MFRC522_readFromRegister(iPort, FIFODataReg);
			}
		  }
		} 
		else 
		{
		  status = MI_ERR;
		}
	}
	else
	{
		status = MI_TIMEOUT;
	}
  }
  return status;
}

/**************************************************************************/
/*!
  @brief   Checks to see if there is a tag in the vicinity.
  @param   mode  The mode we are requsting in.
  @param   type  If we find a tag, this will be the type of that tag.
                 0x4400 = Mifare_UltraLight
                 0x0400 = Mifare_One(S50)
                 0x0200 = Mifare_One(S70)
                 0x0800 = Mifare_Pro(X)
                 0x4403 = Mifare_DESFire
  @returns Returns the status of the request.
           MI_ERR        if something went wrong,
           MI_NOTAGERR   if there was no tag to send the command to.
           MI_OK         if everything went OK.
 */
/**************************************************************************/
int MFRC522_requestTag(int iPort, uint8_t mode, uint8_t *data) 
{
  int status, len;
  MFRC522_writeToRegister(iPort, BitFramingReg, 0x07);  // TxLastBists = BitFramingReg[2..0]

  data[0] = mode;
  status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, data, 1, data, &len);

	if ((status == MI_OK) && (len != 0x10)) 
	{
		status = MI_ERR;
	}

  return status;
}

/**************************************************************************/
/*!
  @brief   Handles collisions that might occur if there are multiple
           tags available.
  @param   serial  The serial nb of the tag.
  @returns Returns the status of the collision detection.
           MI_ERR        if something went wrong,
           MI_NOTAGERR   if there was no tag to send the command to.
           MI_OK         if everything went OK.
 */
/**************************************************************************/
int MFRC522_antiCollision(int iPort, uint8_t *serial) 
{
  int status, i, len;
  uint8_t check = 0x00;

  MFRC522_writeToRegister(iPort, BitFramingReg, 0x00);  // TxLastBits = BitFramingReg[2..0]

  serial[0] = MF1_ANTICOLL;
  serial[1] = 0x20;
  status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, serial, 2, serial, &len);
  len = len / 8; // len is in bits, and we want each byte.
  if (status == MI_OK) 
  {
    // The checksum of the tag is the ^ of all the values.
    for (i = 0; i < len-1; i++) 
	{
      check ^= serial[i];
    }
    // The checksum should be the same as the one provided from the
    // tag (serial[4]).
    if (check != serial[i]) 
	{
      status = MI_ERR;
    }
  }

  return status;
}

/**************************************************************************/
/*!
  @brief   Calculates the CRC value for some data that should be sent to
           a tag.
  @param   data    The data to calculate the value for.
  @param   len     The length of the data.
  @param   result  The result of the CRC calculation.
 */
/**************************************************************************/
void MFRC522_calculateCRC(int iPort, uint8_t *data, int len, uint8_t *result) 
{
  int i;
  uint8_t n;

  MFRC522_clearBitMask(iPort, DivIrqReg, 0x04);   // CRCIrq = 0
  MFRC522_setBitMask(iPort, FIFOLevelReg, 0x80);  // Clear the FIFO pointer

  //Writing data to the FIFO.
  for (i = 0; i < len; i++) 
  {
    MFRC522_writeToRegister(iPort, FIFODataReg, data[i]);
  }
  MFRC522_writeToRegister(iPort, CommandReg, MFRC522_CALCCRC);

  // Wait for the CRC calculation to complete.
  i = 0xFF;
  do 
  {
    n = MFRC522_readFromRegister(iPort, DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04));  //CRCIrq = 1

  // Read the result from the CRC calculation.
  result[0] = MFRC522_readFromRegister(iPort, CRCResultRegL);
  result[1] = MFRC522_readFromRegister(iPort, CRCResultRegM);
}

/**************************************************************************/
/*!
  @brief   Selects a tag for processing.
  @param   serial  The serial number of the tag that is to be selected.
  @returns The SAK response from the tag.
 */
/**************************************************************************/
uint8_t MFRC522_selectTag(int iPort, uint8_t *serial) 
{
  int i, status, len;
  uint8_t sak;
  uint8_t buffer[9];

  buffer[0] = MF1_SELECTTAG;
  buffer[1] = 0x70;
  for (i = 0; i < 5; i++) 
  {
    buffer[i+2] = serial[i];
  }
  MFRC522_calculateCRC(iPort, buffer, 7, &buffer[7]);

  status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, buffer, 9, buffer, &len);

  if ((status == MI_OK) && (len == 0x18)) 
  {
    sak = buffer[0];
  }
  else 
  {
    sak = 0;
  }

  return sak;
}

/**************************************************************************/
/*!
  @brief   Handles the authentication between the tag and the reader.
  @param   mode    What authentication key to use.
  @param   block   The block that we want to read.
  @param   key     The authentication key.
  @param   serial  The serial of the tag.
  @returns Returns the status of the collision detection.
           MI_ERR        if something went wrong,
           MI_OK         if everything went OK.
 */
/**************************************************************************/
int MFRC522_authenticate(int iPort, uint8_t mode, uint8_t block, uint8_t *key, uint8_t *serial) 
{
  int i, status, len;
  uint8_t buffer[12];

  //Verify the command block address + sector + password + tag serial number
  buffer[0] = mode;          // 0th uint8_t is the mode
  buffer[1] = block;         // 1st uint8_t is the block to address.
  for (i = 0; i < 6; i++) 
  {  // 2nd to 7th uint8_t is the authentication key.
    buffer[i+2] = key[i];
  }
  for (i = 0; i < 4; i++) 
  {  // 8th to 11th uint8_t is the serial of the tag.
    buffer[i+8] = serial[i];
  }

  status = MFRC522_commandTag(iPort, MFRC522_AUTHENT, buffer, 12, buffer, &len);
  if ((status != MI_OK) || (!(MFRC522_readFromRegister(iPort, Status2Reg) & 0x08))) 
  {
    status = MI_ERR;
  }

  return status;
}

/**************************************************************************/
/*!
  @brief   Tries to read from the current (authenticated) tag.
  @param   block   The block that we want to read.
  @param   result  The resulting value returned from the tag.
  @returns Returns the status of the collision detection.
           MI_ERR        if something went wrong,
           MI_OK         if everything went OK.
 */
/**************************************************************************/
int MFRC522_readFromTag(int iPort, uint8_t block, uint8_t *result) 
{
  int status, len;

  result[0] = MF1_READ;
  result[1] = block;
  MFRC522_calculateCRC(iPort, result, 2, &result[2]);
  status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, result, 4, result, &len);

  if ((status != MI_OK) || (len != 0x90)) 
  {
    status = MI_ERR;
  }

  return status;
}

/**************************************************************************/
/*!
  @brief   Tries to write to a block on the current tag.
  @param   block  The block that we want to write to.
  @param   data   The data that we shoudl write to the block.
  @returns Returns the status of the collision detection.
           MI_ERR        if something went wrong,
           MI_OK         if everything went OK.
 */
/**************************************************************************/
int MFRC522_writeToTag(int iPort, uint8_t block, uint8_t *data) 
{
  int status, i, len;
  uint8_t buffer[18];

  buffer[0] = MF1_WRITE;
  buffer[1] = block;
  MFRC522_calculateCRC(iPort, buffer, 2, &buffer[2]);
  status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, buffer, 4, buffer, &len);

  if ((status != MI_OK) || (len != 4) || ((buffer[0] & 0x0F) != 0x0A)) 
  {
    status = MI_ERR;
  }

  if (status == MI_OK) 
  {
    for (i = 0; i < 16; i++) 
	{
      buffer[i] = data[i];
    }
    MFRC522_calculateCRC(iPort, buffer, 16, &buffer[16]);
    status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, buffer, 18, buffer, &len);

    if ((status != MI_OK) || (len != 4) || ((buffer[0] & 0x0F) != 0x0A)) 
	{
      status = MI_ERR;
    }
  }

  return status;
}

/**************************************************************************/
/*!
  @brief   Sends a halt command to the current tag.
  @returns Returns the result of the halt.
           MI_ERR        If the command didn't complete properly.
           MI_OK         If the command completed.
 */
/**************************************************************************/
int MFRC522_haltTag(int iPort) 
{
  int status, len;
  uint8_t buffer[4];

  buffer[0] = MF1_HALT;
  buffer[1] = 0;
  MFRC522_calculateCRC(iPort, buffer, 2, &buffer[2]);
  status = MFRC522_commandTag(iPort, MFRC522_TRANSCEIVE, buffer, 4, buffer, &len);
  MFRC522_clearBitMask(iPort, Status2Reg, 0x08);  // turn off encryption
  return status;
}

int MFRC522_deinit(int iPort)
{
	if (iPort) close(iPort);
	return 1;
}

int MFRC522_init(int iPortNum)
{
	int iPort = uart_init_port(iPortNum, 9600);
	if (iPort <= 0) return 0;
	
	int res = 0;
	int status = MFRC522_getFirmwareVersion(iPort);
	if ((status == 145) || (status == 146))
	{
		if (MFRC522_digitalSelfTestPass(iPort))
		{
			res = 1; 
			MFRC522_begin(iPort);						
		}
	}
	if (!res) 
	{
		MFRC522_reset(iPort);
		close(iPort);
	}	
	return res ? iPort : 0;
}	
