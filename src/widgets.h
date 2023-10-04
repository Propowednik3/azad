#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include "bcm_host.h"

#include <GLES2/gl2.h>
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include <pthread.h>
#include "main.h"

int load_file (char *filename, char ** cBuffer, unsigned int *iSizeBuff);
int get_half_second();

int CreateWidget(WIDGET_INFO* wiWidget);
int RenderWidget(WIDGET_INFO* wiWidget);
int ReleaseWidget(WIDGET_INFO* wiWidget);
char* GetWidgetTypeName(int iType);
char* GetWidgetDirectName(int iDirect);

#endif