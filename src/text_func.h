
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


int load_text_file (char *filename, char ** cBuffer, unsigned int *iSizeBuff) ;
int InitGLText(unsigned int uiColor, GLfloat gA, char cAccelerator);
int RenderGLText(int iTextSize, GLfloat gX, GLfloat gY, char *cText, ...);
int RenderGLTextScale(int iTextSize, GLfloat gX, GLfloat gY, GLfloat gScX, GLfloat gScY, char *cText, ...);
int SetObject(int iPoliSizeX, int iPoliSizeY);
int DeInitGLText();
unsigned int GetCRC(char *buffer, unsigned int iLength);
