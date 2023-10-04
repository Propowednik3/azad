#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include "ir_control.h"
#include "pthread2threadx.h"
#include "network.h"
#include "debug.h"
#include "system.h"
  
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64
#define MAX_RECV_BUF 64
#define MAX_IR_BUF 256


pthread_mutex_t IRWork_mutex;
static pthread_t threadIRWork;
pthread_attr_t tattrIRWork;
char cThreadIRStatus = 0;
uint16_t ucIRData[MAX_IR_BUF];
unsigned int uiIRDataCnt = 0;


void* IRWorker(void *pData);

/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
 
	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[MAX_BUF];
 
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}
 
	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);
 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd;
	char buf[MAX_BUF];
 
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}
 
	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd;
	char buf[MAX_BUF];
	char ch;

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}
 
	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}
 
	close(fd);
	return 0;
}


/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}
 
	write(fd, edge, strlen(edge) + 1); 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_fd_open
 ****************************************************************/

int gpio_fd_open(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

/****************************************************************
 * gpio_fd_close
 ****************************************************************/

int gpio_fd_close(int fd)
{
	return close(fd);
}

int GetPinCode(char cPinNum)
{
	int ret = -1;
	switch(cPinNum)
	{
		case 3: ret = 2; break;
		case 5: ret = 3; break;
		case 7: ret = 4; break;
		case 8: ret = 14; break;
		case 10: ret = 15; break;
		case 11: ret = 17; break;
		case 12: ret = 18; break;
		case 13: ret = 27; break;
		case 15: ret = 22; break;
		case 18: ret = 24; break;
		case 19: ret = 10; break;
		case 21: ret = 9; break;
		case 22: ret = 25; break;
		case 23: ret = 11; break;
		case 24: ret = 8; break;
		case 26: ret = 7; break;
		case 29: ret = 5; break;
		case 30: ret = 6; break;
		case 32: ret = 12; break;
		case 33: ret = 13; break;
		case 35: ret = 19; break;
		case 36: ret = 16; break;
		case 37: ret = 26; break;
		case 38: ret = 20; break;
		case 40: ret = 21; break;
		default: ret = -1; break;
	}
	return ret;
}

char ir_init(unsigned int uiPinNum)
{	
	cThreadIRStatus = 0;
	int iPinCode = (int)GetPinCode(uiPinNum);
	
	if (iPinCode == -1)
	{
		dbgprintf(2, "Wrong pin number for IR reciever %i\n", uiPinNum);
		return 0;
	}
	pthread_mutex_init(&IRWork_mutex, NULL);
	pthread_attr_init(&tattrIRWork);      
	pthread_attr_setdetachstate(&tattrIRWork, PTHREAD_CREATE_DETACHED);	
	pthread_create(&threadIRWork, &tattrIRWork, IRWorker, (void*)iPinCode);
	
	return 1;
}

char ir_close(void)
{	
	int ret;
	
	DBG_MUTEX_LOCK(&IRWork_mutex);
	ret = cThreadIRStatus;
	if (ret == 1) cThreadIRStatus = 2;
	DBG_MUTEX_UNLOCK(&IRWork_mutex);
	while(ret != 0)
	{
		DBG_MUTEX_LOCK(&IRWork_mutex);
		ret = cThreadIRStatus;
		DBG_MUTEX_UNLOCK(&IRWork_mutex);
		if (ret != 0) usleep(50000);
	} 
	
	pthread_attr_destroy(&tattrIRWork);
	pthread_mutex_destroy(&IRWork_mutex);
	
	return 1;	
}

int ir_read(uint16_t *cdata, unsigned int ilen)
{
	int count = 0;
	if (ilen == 0) return 0;
	
	DBG_MUTEX_LOCK(&IRWork_mutex);
	if (uiIRDataCnt)
	{
		if (ucIRData[0] & 1024)
		{
			int i;
			for (i = 0; i < uiIRDataCnt; i++) 
			{
				if ((i != 0) && (ucIRData[i] & 1024)) break;
				if (count == ilen) 
				{
					count = -1;
					break;
				}
				cdata[count] = ucIRData[i] & 255;
				count++;				
			}
			if (count > 0) 
			{
				uiIRDataCnt -= count;
				memmove(ucIRData, &ucIRData[count], sizeof(uint16_t) * uiIRDataCnt);
			}
		} else dbgprintf(2, "Error ir order in buffer\n");
	}
	DBG_MUTEX_UNLOCK(&IRWork_mutex);
	//if (count != 0) printf("read %i\n", count);
	return count;
}

/*
void* IRWorker2(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	struct pollfd fdset[2];
	int nfds = 2;
	int gpio_fd, rc;
	char buf[MAX_BUF];
	int len;
	unsigned int gpio_pin = (unsigned int)pData;
	
	DBG_MUTEX_LOCK(&IRWork_mutex);
	cThreadIRStatus = 1;
	DBG_MUTEX_UNLOCK(&IRWork_mutex);

	
	gpio_export(gpio_pin);
	gpio_set_dir(gpio_pin, 0);
	gpio_set_edge(gpio_pin, "both");
	gpio_fd = gpio_fd_open(gpio_pin);
	
	int64_t previous_us = 0;
	get_us(&previous_us);
	unsigned int us, i;
	unsigned char ucRecvBuffer[MAX_RECV_BUF];
	unsigned int uiRecvCnt = 0;
	unsigned char cRecvFlag = 128;
	
	memset(ucIRData, 0, MAX_IR_BUF * sizeof(uint16_t));
	uiIRDataCnt = 0;
	char cStatus = 0;
	
	while (1) 
	{		
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;      
		fdset[1].fd = gpio_fd;
		fdset[1].events = POLLPRI;

		previous_us = 0;
		get_us(&previous_us);
		
		if (cStatus) rc = poll(fdset, nfds, 10); else rc = poll(fdset, nfds, 300);
		if (rc < 0) 
		{
			dbgprintf(1, "\npoll() failed!\n");
			break;
		}
      
		if (rc == 0) //TIMEOUT
		{
			if (uiRecvCnt)
			{
				//printf("Done\n");
				cStatus = 0;
				uiRecvCnt++;
				DBG_MUTEX_LOCK(&IRWork_mutex);
				if ((uiIRDataCnt + uiRecvCnt) < MAX_IR_BUF)
				{
					ucIRData[uiIRDataCnt] = 1024 | ucRecvBuffer[0];
					for (i = 1; i < uiRecvCnt; i++) 
					{
						ucIRData[uiIRDataCnt + i] = ucRecvBuffer[i];
						printf("%i,", ucRecvBuffer[i]);
					}
					printf("\n");
					uiIRDataCnt += uiRecvCnt;
				} else dbgprintf(2, "Overfull IR buffer %i\n", MAX_IR_BUF);
				DBG_MUTEX_UNLOCK(&IRWork_mutex);
				memset(ucRecvBuffer, 0, MAX_RECV_BUF);
				uiRecvCnt = 0;
			}
			else
			{
				DBG_MUTEX_LOCK(&IRWork_mutex);
				rc = cThreadIRStatus;
				DBG_MUTEX_UNLOCK(&IRWork_mutex);
				if (rc == 2) break;
			}
		}
        else
		{			
			if (fdset[1].revents & POLLPRI) 
			{
				lseek(fdset[1].fd, 0, SEEK_SET);
				len = read(fdset[1].fd, buf, MAX_BUF);
				if (len) // && buf[0] == 48)
				{
					us = (unsigned int)get_us(&previous_us);
					//printf(">> %i\n", us);
					if (us > 5000)
					{
						//printf("New\n");
						cStatus = 1;
						memset(ucRecvBuffer, 0, MAX_RECV_BUF);
						uiRecvCnt = 0;
						cRecvFlag = 128;
					}
					if (cRecvFlag & 128)
					{
						uiRecvCnt++;
						cRecvFlag = 1;
					} else cRecvFlag <<= 1;
					
					if (uiRecvCnt < MAX_RECV_BUF)
					{
						if (us > 600) ucRecvBuffer[uiRecvCnt] |= cRecvFlag;
					}
				}
			}

			if (fdset[0].revents & POLLIN) 
			{
				read(fdset[0].fd, buf, 1);
				//dbgprintf(2, "IR poll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
			}		
			//fflush(stdout);	
		}			
	}

	gpio_fd_close(gpio_fd);	
	
		
	DBG_MUTEX_LOCK(&IRWork_mutex);
	cThreadIRStatus = 0;
	DBG_MUTEX_UNLOCK(&IRWork_mutex);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return 0;
}*/

void* IRWorker(void *pData)
{
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	DBG_LOG_IN();
	
	struct pollfd fdset;
	int gpio_fd, rc;
	char buf[MAX_BUF];
	int len;
	unsigned int gpio_pin = (unsigned int)pData;
	
	char threadName[16];
	memset(threadName, 0, 16);
	snprintf(threadName, 15, "IR_recv_%i", gpio_pin);
	pthread_setname_np(pthread_self(), threadName);
		
	DBG_MUTEX_LOCK(&IRWork_mutex);
	cThreadIRStatus = 1;
	DBG_MUTEX_UNLOCK(&IRWork_mutex);

	
	gpio_export(gpio_pin);
	gpio_set_dir(gpio_pin, 0);
	gpio_set_edge(gpio_pin, "both");
	gpio_fd = gpio_fd_open(gpio_pin);
	
	int64_t previous_us = 0;
	get_us(&previous_us);
	unsigned int us, i;
	unsigned char ucRecvBuffer[MAX_RECV_BUF];
	unsigned int uiRecvCnt = 0;
	unsigned char cRecvFlag = 128;
	
	memset(ucIRData, 0, MAX_IR_BUF * sizeof(uint16_t));
	uiIRDataCnt = 0;
	char cStatus = 0;
	
	while (1) 
	{		
		memset(&fdset, 0, sizeof(fdset));

		fdset.fd = gpio_fd;
		fdset.events = POLLPRI | POLLERR;
		fdset.revents = 0;

		previous_us = 0;
		get_us(&previous_us);
		
		if (cStatus) rc = poll(&fdset, 1, 10); else rc = poll(&fdset, 1, 300);
		if (rc < 0) 
		{
			dbgprintf(1, "\npoll() failed!\n");
			break;
		}
		
		if (rc == 0) //TIMEOUT
		{
			if (uiRecvCnt)
			{
				cStatus = 0;
				uiRecvCnt++;
				DBG_MUTEX_LOCK(&IRWork_mutex);
				if ((uiIRDataCnt + uiRecvCnt) < MAX_IR_BUF)
				{
					ucIRData[uiIRDataCnt] = 1024 | ucRecvBuffer[0];
					for (i = 1; i < uiRecvCnt; i++) 
					{
						ucIRData[uiIRDataCnt + i] = ucRecvBuffer[i];
						//printf("\t%i", ucRecvBuffer[i]);
					}
					//printf("\n");
					uiIRDataCnt += uiRecvCnt;
				} else dbgprintf(2, "Overfull IR buffer %i\n", MAX_IR_BUF);
				DBG_MUTEX_UNLOCK(&IRWork_mutex);
				memset(ucRecvBuffer, 0, MAX_RECV_BUF);
				uiRecvCnt = 0;
			//	printf("\tDone\n");				
			}
			else
			{
				DBG_MUTEX_LOCK(&IRWork_mutex);
				rc = cThreadIRStatus;
				DBG_MUTEX_UNLOCK(&IRWork_mutex);
				if (rc == 2) break;
			}
		}
        else
		{			
			if (fdset.revents & POLLPRI) 
			{
				lseek(fdset.fd, 0, SEEK_SET);
				len = read(fdset.fd, buf, MAX_BUF);
				if (len) // && buf[0] == 48)
				{
					us = (unsigned int)get_us(&previous_us);
					//printf("%i", (us > 600) ? 1 : 0);	
					if (us > 5000)
					{
						//printf("New\n");
						cStatus = 1;
						memset(ucRecvBuffer, 0, MAX_RECV_BUF);
						uiRecvCnt = 0;
						cRecvFlag = 1;
					}					
					if (uiRecvCnt < MAX_RECV_BUF)
					{
						if (us > 600) ucRecvBuffer[uiRecvCnt] |= cRecvFlag;
					}
					if (cRecvFlag & 128)
					{
						//printf(",%i\t", ucRecvBuffer[uiRecvCnt]);
						uiRecvCnt++;
						cRecvFlag = 1;
					} else cRecvFlag <<= 1;					
					//printf("%i\t>%i<\t", (us > 600) ? 1 : 0, us);
				}
			}
		}
	}

	gpio_fd_close(gpio_fd);	
	
		
	DBG_MUTEX_LOCK(&IRWork_mutex);
	cThreadIRStatus = 0;
	DBG_MUTEX_UNLOCK(&IRWork_mutex);
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());	
	return 0;
}

