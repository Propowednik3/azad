#ifndef _TFP625A_H_
#define _TFP625A_H_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include "gpio.h"

typedef struct FINGER_INFO
{
	char			Filled;
	unsigned int	ID;
	char			Info[32];
} FINGER_INFO;

#define FINGER_INFO_SIZE 1024

int TFP625A_init(MODULE_INFO *miModule);
int TFP625A_close(MODULE_INFO *miModule);
int TFP625A_get_statuses_module(MODULE_INFO *miModule);
int TFP625A_AutoIdentify(MODULE_INFO *miModule);
int TFP625A_AutoEnroll(MODULE_INFO *miModule, int iTempNum, int iTempID, char *sTempInfo);
int TFP625A_DeleteTemplate(MODULE_INFO *miModule, int iTemplateID);
int TFP625A_CleanDatabase(MODULE_INFO *miModule);
int TFP625A_ChangeInfo(MODULE_INFO *miModule, int iTempNum, int iTempID, char *sTempInfo);
int TFP625A_UpdateInfo(MODULE_INFO *miModule);

#endif