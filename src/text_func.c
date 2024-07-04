#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "bcm_host.h"

#include "GLES/gl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include <pthread.h>

#include "text_func.h"
#include "debug.h"
#include "omx_client.h"

#define			MAX_SIZE_TEXT_LIST	256

typedef struct
{
	int				Lenght;    
	int				Size;
	char 			Text[256];
	unsigned int 	CRC;
	int64_t 		Timer;
	unsigned int	UseCnt;
	GLuint			Texture;
	GLfloat			Scale;
	GLfloat			VertPoints[8];
	GLfloat			TextPoints[8];
} TEXT_LIST;

int RenderGLTextSlow(int iTextSize, GLfloat gX, GLfloat gY, char *cText);

int 			iTextInit = 0;
char 			cAccelerateRender = 1;
RGB_T			rgbTextColor;

GLuint 			text_tex[10];
static GLfloat gPoints[160*4*2];
static GLfloat gTexturePoints[160*4*2];
TEXT_LIST 		**ppTextList;
unsigned int	uiTextListCnt = 0;
//GLuint 			FramebufferName = 0;
		

int load_text_file (char *filename, char ** cBuffer, unsigned int *iSizeBuff) 
{
  int32_t pos;
  FILE *fp;
  
  *iSizeBuff = 0;
  *cBuffer = NULL;
  fp = fopen (filename, "rb");
  if (!fp) return 0;
  if (fseek (fp, 0L, SEEK_END) < 0) 
  {
    fclose (fp);
    return 0;
  };
  pos = ftell (fp);
  if (pos == LONG_MAX) 
  {
    fclose (fp);
    return 0;
  };
  *iSizeBuff = pos;
  fseek (fp, 0L, SEEK_SET);
  *cBuffer = DBG_MALLOC(*iSizeBuff);
  fread (*cBuffer, 1, *iSizeBuff, fp);
  fclose (fp);
  return 1;
}

int SetObject(int iPoliSizeX, int iPoliSizeY)
{
	int n, m;
	GLfloat gX, gY, gX1, gY1, gX2, gY2, gXd, gYd;
	gXd = 1.0f / (iPoliSizeX * 16);
	gYd = 1.0f / (iPoliSizeY * 10);
	//gX = gXd * (iPoliSizeX - 1);
	//gY = gYd * (iPoliSizeY - 1);
	gX = gXd * (iPoliSizeX - 1);
	gY = gYd * (iPoliSizeY - 1);
	gX1 = 0.0f;
	gY1 = 0.0f;
	gX2 = 0.0f;
	gY2 = 0.0f;
	memset(gPoints, 0, sizeof(GLfloat)*160*4*2);
	m = 0;
	for(n = 0; n != 160;n++)
	{
		gPoints[m + 2] = 35;
		gPoints[m + 5] = 64;
		gPoints[m + 6] = 35;
		gPoints[m + 7] = 64;
		m+=8;
	}
	memset(gTexturePoints, 0, sizeof(GLfloat)*160*4*2);
	m = 0;
	for(n = 0; n < 160;n++)
	{
		gX2 = gX1 + gX;
		//if (gX1 == 0) gX2 -= gXd;
		gY2 = gY1 + gY;
		gTexturePoints[m + 0] = gX1;	gTexturePoints[m + 1] = gY2;
		gTexturePoints[m + 2] = gX2;	gTexturePoints[m + 3] = gY2;
		gTexturePoints[m + 4] = gX1;	gTexturePoints[m + 5] = gY1;
		gTexturePoints[m + 6] = gX2;	gTexturePoints[m + 7] = gY1;
		gX1 = gX2 + gXd;
		if (gX1 >= 1.0f)
		{
			gY1 = gY2 + gYd;
			gX1 = 0.0f;
			if (gY1 >= 1.0f) break;
		}
		m+=8;
	}
		
	return 1;
}

unsigned int GetCRC(char *buffer, unsigned int iLength)
{	
	return 0xFF00F00F;
	unsigned int		CRC1 = 0;
	unsigned int		CRC2 = 0;
	unsigned int		Len;
	unsigned int		Arr[256];
	
	
	memset(Arr, 0, sizeof(Arr));
	for (Len = 0; Len < iLength; Len++)
		Arr[Len] = Len;
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

void DestroyTextTexture(TEXT_LIST* tList)
{
	glDeleteTextures(1, &tList->Texture);
}

void CreateTextTexture(int iTextSize, char *cText, unsigned int uiTextLen, TEXT_LIST* tList) 
{
	GLfloat gScaleX = ((GLfloat)(iTextSize))/60;
	tList->Scale = gScaleX;
	tList->Scale = 1.0f;
	
	//Big size limit >>>>
	/*if (iTextSize <= 60) tList->Scale = 1.0f;
	else 
	{
		iTextSize = 60;
		gScaleX = 1.0f;
	}
	// <<<<<Big size limit	*/
	
	//printf("%f %i\n", gScaleX, iTextSize);
	GLfloat gW = gScaleX * 36 * uiTextLen;
	GLfloat gH = gScaleX * 65;
	GLfloat	gW16 = gW; //((int)gW+15)&~15;
	GLfloat	gH16 = gH; //((int)gH+15)&~15;
	unsigned int uiTextureSize = gW16 * gH16;
	
	memset(tList->VertPoints, 0, 8*sizeof(GLfloat));
	tList->VertPoints[2] = gW - 1;
	tList->VertPoints[5] = gH - 1;
	tList->VertPoints[6] = gW - 1;
	tList->VertPoints[7] = gH - 1;
	
	memset(tList->TextPoints, 0, 8*sizeof(GLfloat));
	tList->TextPoints[2] = 1.0f;
	tList->TextPoints[5] = 1.0f;
	tList->TextPoints[6] = 1.0f;
	tList->TextPoints[7] = 1.0f;
	
	RGBA_T *bb = (RGBA_T*)DBG_MALLOC(uiTextureSize * sizeof(RGBA_T));
	memset(bb, 255, uiTextureSize * sizeof(RGBA_T));		
	glGenTextures(1, &tList->Texture);
	glBindTexture(GL_TEXTURE_2D, tList->Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gW16, gH16, 0, GL_RGBA, GL_UNSIGNED_BYTE, bb);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);		
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
	DBG_FREE(bb);
		
	GLuint 			FramebufferName = 0;	
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, gW16, gH16);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tList->Texture, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
		
    // FBO attachment is complete?
	GLenum res = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (res == GL_FRAMEBUFFER_COMPLETE)
    {
        glClearColor(0.0f,0.0f,0.0f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
		RenderGLTextSlow(iTextSize, 0, 0, cText);

	} //else printf("NO glBindFrame %#010x\n", res);	
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteRenderbuffers(1, &FramebufferName);
	glDeleteFramebuffers(1, &FramebufferName);
}

int DeInitGLText()
{
	if (iTextInit != 1) return 0;
	
	glDeleteTextures(3, text_tex);
	
	/*if (cAccelerateRender)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &FramebufferName);
	}*/
	
	if (cAccelerateRender)
	{
		int i;
		for (i = 0; i < uiTextListCnt; i++)
		{
			if (ppTextList[i]->Lenght) DestroyTextTexture(ppTextList[i]);
			DBG_FREE(ppTextList[i]);
		}
		if (uiTextListCnt) DBG_FREE(ppTextList);
	}
	iTextInit = 0;
	
	return 1;
}

int InitGLText(unsigned int uiColor, GLfloat gA, char cAccelerator)
{
	if (iTextInit == 1) return 0;
	
	cAccelerateRender = cAccelerator;
	char *cBuffer = NULL;
	ppTextList = NULL;
	uiTextListCnt = 0;
	
	rgbTextColor.Red = uiColor & 255;
	rgbTextColor.Green = (uiColor >> 8) & 255;
	rgbTextColor.Blue = (uiColor >> 16) & 255;
	
	int iSmallWidht = 192;
	int iSmallHigth = 256;	
	int iMediumWidht = 576;
	int iMediumHigth = 640;
	int iLargeWidht = 1792;
	int iLargeHigth = 2048;
	

	unsigned int iSize = 0;
	int sW = 2048;
	int sH = 2048;	
	
	if (omx_image_to_buffer("Textures/TextSmall.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
	{
		dbgprintf(2, "Error omx_image_to_buffer 'TextSmall.png'\n");
		return 0;
	}	
	if ((sW != iSmallWidht) || (sH != iSmallHigth))
	{
		dbgprintf(1, "Wrong size texture TextSmall.png for text %i != %i   %i != %i\n", sW, iSmallWidht, sH, iSmallHigth);
		DBG_FREE(cBuffer);
		return 0;
	}
	//if (!load_text_file("Textures/text192x230.raw", &cBuffer, &iSize)) 
	//	dbgprintf(1, "Error load texture Textures/text192x230.raw\n");
	int n;
//	int i = 0;
//	RGB_T *rgbColor = (RGB_T*)&uiColor;
	
	RGBA_T *cTexture1[3];
	unsigned int uiMatrixSize = sW * sH;
	cTexture1[0] = (RGBA_T *)cBuffer;
	for (n = 0; n != uiMatrixSize; n++)
	{
		cTexture1[0][n].Red = rgbTextColor.Red;
		cTexture1[0][n].Green = rgbTextColor.Green;
		cTexture1[0][n].Blue = rgbTextColor.Blue;
		cTexture1[0][n].Alpha *= gA;
	}
	//omx_dump_data("ascii.raw", cBuffer, iSize);	
	cBuffer = NULL;
	iSize = 0;
	/*if (!load_text_file("Textures/text576x650.raw", &cBuffer, &iSize)) 
		dbgprintf(1, "Error load texture Textures/text576x650.raw\n");*/
	if (omx_image_to_buffer("Textures/TextMedium.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
	{
		dbgprintf(2, "Error omx_image_to_buffer 'TextMedium.png'\n");
		return 0;
	}	
	if ((sW != iMediumWidht) || (sH != iMediumHigth))
	{
		dbgprintf(1, "Wrong size texture TextMedium.png for text %i != %i   %i != %i\n", sW, iMediumWidht, sH, iMediumHigth);
		DBG_FREE(cBuffer);
		glDeleteTextures(1, text_tex);
		return 0;
	}
	
	cTexture1[1] = (RGBA_T *)cBuffer;
	uiMatrixSize = sW * sH;
	for (n = 0; n < uiMatrixSize; n++)
	{
		cTexture1[1][n].Red = rgbTextColor.Red;
		cTexture1[1][n].Green = rgbTextColor.Green;
		cTexture1[1][n].Blue = rgbTextColor.Blue;
		cTexture1[1][n].Alpha *= gA;
	}
	//omx_dump_data("ascii2.raw", cBuffer, iSize);	
	
	cBuffer = NULL;
	iSize = 0;
	/*if (!load_text_file("Textures/text1815x2048.raw", &cBuffer, &iSize)) 
		dbgprintf(1, "Error load texture Textures/text1815x2048.raw\n");*/
	if (omx_image_to_buffer("Textures/TextLarge.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
	{
		dbgprintf(2, "Error omx_image_to_buffer 'TextLarge.png'\n");
		return 0;
	}	
	if ((sW != iLargeWidht) || (sH != iLargeHigth))
	{
		dbgprintf(1, "Wrong size texture TextLarge.png for text %i != %i   %i != 2048\n", sW, iLargeWidht, sH, iLargeHigth);
		DBG_FREE(cBuffer);
		glDeleteTextures(2, text_tex);
		return 0;
	}
	
	cTexture1[2] = (RGBA_T *)cBuffer;
	uiMatrixSize = sW * sH;
	for (n = 0; n != uiMatrixSize; n++)
	{
		cTexture1[2][n].Red = rgbTextColor.Red;
		cTexture1[2][n].Green = rgbTextColor.Green;
		cTexture1[2][n].Blue = rgbTextColor.Blue;
		cTexture1[2][n].Alpha *= gA;
	}
	//omx_dump_data("ascii3.raw", cBuffer, iSize);	
	
	SetObject(36, 60);
	
	glGenTextures(3, &text_tex[0]);
	glBindTexture(GL_TEXTURE_2D, text_tex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iSmallWidht, iSmallHigth, 0,
								   GL_RGBA, GL_UNSIGNED_BYTE, cTexture1[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR); //GL_LINEAR
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR); //GL_NEAREST
    
    glBindTexture(GL_TEXTURE_2D, text_tex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iMediumWidht, iMediumHigth, 0,
								   GL_RGBA, GL_UNSIGNED_BYTE, cTexture1[1]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR); 
	
	glBindTexture(GL_TEXTURE_2D, text_tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iLargeWidht, iLargeHigth, 0,
								   GL_RGBA, GL_UNSIGNED_BYTE, cTexture1[2]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR); 
   
    DBG_FREE(cTexture1[0]);
	DBG_FREE(cTexture1[1]);
	DBG_FREE(cTexture1[2]);
	
	/*if (cAccelerateRender)
	{
		glGenFramebuffers(1, &FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}*/
    
	iTextInit = 1;
//   glEnable(GL_TEXTURE_2D);
	return 1;
}

int RenderGLTextFast(int iTextSize, GLfloat gX, GLfloat gY, char *cText, int iLen)
{				
	//printf("RenderGLTextFast %s\n", cText);
	int i;
	int iExist = 0;
	unsigned int uiMinUseCnt = 0xFFFFFFFF;
	int uiMinUseCell = -1;
	int uiFreeCell = -1;
	TEXT_LIST *txtList;
	unsigned int uiCRC = GetCRC(cText, iLen);
	
	/*for (i = 0; i < uiTextListCnt; i++)
	{
		if ((ppTextList[i]->Lenght) && ((unsigned int)(get_ms(&ppTextList[i]->Timer) > 2000)))
		{
			ppTextList[i]->Lenght = 0;
			DestroyTextCanvas(&ppTextList[i]);
		}
	}*/
	
	for (i = 0; i < uiTextListCnt; i++)
	{
		if (uiFreeCell == -1)
		{
			if (ppTextList[i]->UseCnt < uiMinUseCnt) 
			{
				uiMinUseCnt = ppTextList[i]->UseCnt;
				uiMinUseCell = i;
			}
			if (ppTextList[i]->Lenght == 0) uiFreeCell = i;
		}
		
		if ((ppTextList[i]->Lenght == iLen) && 
			(ppTextList[i]->CRC == uiCRC) && 
			(ppTextList[i]->Size == iTextSize) && 
			(strcmp(ppTextList[i]->Text, cText) == 0)) 
			{
				ppTextList[i]->Timer = 0;
				get_ms(&ppTextList[i]->Timer);
				iExist = 1;			
				if (uiFreeCell != -1)
				{
					txtList = ppTextList[uiFreeCell];
					ppTextList[uiFreeCell] = ppTextList[i];
					ppTextList[i] = txtList;
					i = uiFreeCell;
				}
				if ((uiMinUseCell != -1) && (i > uiMinUseCell) && (ppTextList[i]->UseCnt > ppTextList[uiMinUseCell]->UseCnt))
				{
					txtList = ppTextList[uiMinUseCell];
					ppTextList[uiMinUseCell] = ppTextList[i];
					ppTextList[i] = txtList;
					i = uiMinUseCell;
				}
				//printf("Finded in %i\n", i);
				break;
			}
	}
	if (iExist == 0)
	{
		for (i = 0; i < uiTextListCnt; i++)
		{
			if (ppTextList[i]->Lenght == 0) 
			{
				iExist = 1;
				break;
			}
			if ((ppTextList[i]->Lenght) && ((unsigned int)(get_ms(&ppTextList[i]->Timer) > 1500)))
			{
				DestroyTextTexture(ppTextList[i]);
				memset(ppTextList[i], 0, sizeof(TEXT_LIST));
				iExist = 1;
				break;
			}			
		}
		
		if (i == uiTextListCnt)
		{
			if (uiTextListCnt < MAX_SIZE_TEXT_LIST)
			{
				ppTextList = (TEXT_LIST**)DBG_REALLOC(ppTextList, sizeof(TEXT_LIST*)*(uiTextListCnt + 1));
				ppTextList[uiTextListCnt] = (TEXT_LIST*)DBG_MALLOC(sizeof(TEXT_LIST));
				memset(ppTextList[uiTextListCnt], 0, sizeof(TEXT_LIST));
				uiTextListCnt++;
				iExist = 1;
			}
		}
		if (iExist)
		{
			ppTextList[i]->Lenght = iLen;
			ppTextList[i]->CRC = uiCRC;
			ppTextList[i]->Size = iTextSize;
			memcpy(ppTextList[i]->Text, cText, iLen);
			ppTextList[i]->Text[iLen] = 0;
			ppTextList[i]->Timer = 0;
			get_ms(&ppTextList[i]->Timer);
				
			CreateTextTexture(iTextSize, cText, iLen, ppTextList[i]);
		}
	}
	if (iExist)
	{
		//glColorMask(rgbTextColor.Red, rgbTextColor.Green, rgbTextColor.Blue,0.0f);	
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		
		glLoadIdentity();
		if (ppTextList[i]->Scale != 1.0f) glScalef(ppTextList[i]->Scale,ppTextList[i]->Scale,0.0f);
		glVertexPointer(2, GL_FLOAT, 0, ppTextList[i]->VertPoints);
		glEnableClientState(GL_VERTEX_ARRAY);	
		glTexCoordPointer(2, GL_FLOAT, 0, ppTextList[i]->TextPoints); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
		glBindTexture(GL_TEXTURE_2D, ppTextList[i]->Texture);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
		//glBlendFunc(GL_ONE, GL_DST_ALPHA);
    
		//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		glTranslatef(gX/ppTextList[i]->Scale, gY/ppTextList[i]->Scale, 0.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	
						
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		//glColorMask(1.0f, 1.0f, 1.0f, 0.0f);
	
		return 1;
	}
	else
	{
		RenderGLTextSlow(iTextSize, gX, gY, cText);
		return 0;
	}
	return 0;
}

int RenderGLTextSlow(int iTextSize, GLfloat gX, GLfloat gY, char *cText)
{	
	GLfloat PosX = gX;
	GLfloat PosY = gY;
	GLfloat PosX2 = 0;
	GLfloat PosY2 = 0;
	GLfloat gScaleX = 0;
	GLuint 	cur_tex = 0;
	 
	int cc = 0;
	int pos1 = 0;
	gScaleX = ((GLfloat)(iTextSize))/60;
	if (iTextSize < 16) cur_tex = text_tex[0];
	else 
		if (iTextSize < 48) cur_tex = text_tex[1];
		else cur_tex = text_tex[2];
		
	//glColorMask(rgbTextColor.Red, rgbTextColor.Green, rgbTextColor.Blue,0.0f);	
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	if (iTextSize != 60) glScalef(gScaleX,gScaleX,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, gPoints); //point to vert array
	glEnableClientState(GL_VERTEX_ARRAY); //enable vert array	
	glTexCoordPointer(2, GL_FLOAT, 0, gTexturePoints); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    glBindTexture(GL_TEXTURE_2D, cur_tex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	while((cText[cc] != 0) && (cc < 255))
	{	
		if (cText[cc] == 96) cText[cc] = 127;
		if (cText[cc] > 175) cText[cc] = cText[cc] - 48;
		if (cText[cc] == 9) PosX+=36*gScaleX*4;
		if (cText[cc] == 10) {PosY-=65*gScaleX;PosX=gX;}	
		if (cText[cc] == 13) {PosY-=65*gScaleX;PosX=gX;}	
		if (cText[cc] >= 32)
		{
			if ((cText[cc] > 32) && (cText[cc] < 192))
			{
				pos1 = (cText[cc] - 32) * 4;
				PosX2 = PosX / gScaleX;
				PosY2 = PosY / gScaleX;
				glTranslatef(PosX2, PosY2, 0.0f);
				glDrawArrays( GL_TRIANGLE_STRIP, pos1, 4);
				glTranslatef(-PosX2, -PosY2, 0.0f);				
			}
			PosX+=36*gScaleX;
		}	
		cc++;	
	}
    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	//glColorMask(1.0f, 1.0f, 1.0f, 0.0f);
	
	return 1;
}

int RenderGLText(int iTextSize, GLfloat gX, GLfloat gY, char *cText, ...)
{	
	if (iTextInit != 1) return 0;
	
	char textbuffer[256];
	memset(textbuffer, 0 , 256);
	va_list valist;
	va_start(valist, cText);		
	vsprintf(textbuffer, cText, valist);		
	va_end(valist);
	
	int iLen = strlen(textbuffer);
	//for (iLen == 0; iLen < 256; iLen++) if (textbuffer[iLen] == 0) break; else iLen++;
	
	if (iLen > 255) iLen = 255;
	//printf("# '%s' %i\n", textbuffer, iLen);
	if (cAccelerateRender && (iLen > 1)) 
		return RenderGLTextFast(iTextSize, gX, gY, textbuffer, iLen);
		else
		return RenderGLTextSlow(iTextSize, gX, gY, textbuffer);			
}

/*
int RenderGLTextScale(int iTextSize, GLfloat gX, GLfloat gY, GLfloat gScX, GLfloat gScY, char *cText, ...)
{	
	if (iTextInit != 1) return 0;
		
	GLfloat PosX = gX;
	GLfloat PosY = gY;
	GLfloat PosX2 = 0;
	GLfloat PosY2 = 0;
	GLfloat gScaleX = 0;
	GLfloat gScaleY = 0;
	GLuint 	cur_tex = 0;
	char textbuffer[256];
	memset(textbuffer, 0 , 256);
	va_list valist;
	va_start(valist, cText);		
	vsprintf(textbuffer, cText, valist);		
	va_end(valist);
	//strcpy(textbuffer, cText); 
	int cc = 0;
	int pos1 = 0;
	gScaleX = ((GLfloat)(iTextSize))/60;
	gScaleY = gScaleX;
	gScaleX *= gScX;
	gScaleY *= gScY;
	
	if (iTextSize > 15) 
		cur_tex = text_tex[1]; 
		else cur_tex = text_tex[0];
		
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	if (iTextSize != 60) glScalef(gScaleX,gScaleY,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, gPoints); //point to vert array
	glEnableClientState(GL_VERTEX_ARRAY); //enable vert array	
	glTexCoordPointer(2, GL_FLOAT, 0, gTexturePoints); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    glBindTexture(GL_TEXTURE_2D, cur_tex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	while((textbuffer[cc] != 0) && (cc < 255))
	{	
		if (textbuffer[cc] == 96) textbuffer[cc] = 127;
		if (textbuffer[cc] > 175) textbuffer[cc] = textbuffer[cc] - 48;
		if (textbuffer[cc] == 9) PosX+=36*gScaleX*4;
		if (textbuffer[cc] == 10) {PosY-=65*gScaleY;PosX=gX;}	
		if (textbuffer[cc] == 13) {PosY-=65*gScaleY;PosX=gX;}	
		if (textbuffer[cc] >= 32)
		{
			if ((textbuffer[cc] > 32) && (textbuffer[cc] < 192))
			{
				pos1 = (textbuffer[cc] - 32) * 4;
				PosX2 = PosX / gScaleX;
				PosY2 = PosY / gScaleY;
				glTranslatef(PosX2, PosY2, 0.0f);
				glDrawArrays( GL_TRIANGLE_STRIP, pos1, 4);
				glTranslatef(-PosX2, -PosY2, 0.0f);				
			}
			PosX+=36*gScaleX;
		}	
		cc++;	
	}
    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	return 1;
}*/

