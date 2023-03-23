
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "bcm_host.h"

#include "GLES/gl.h"
#include "GLES/glext.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include <pthread.h>

#include "widgets.h"
#include "omx_client.h"
#include "debug.h"
#include "main.h"
#include "text_func.h"
#include "system.h"
 

GLuint 			tElClockTexture;
int				iElClockInit = 0;
GLuint 			tMechClockTexture;
int				iMechClockInit = 0;
GLuint 			tWhiteClockTexture;
int				iWhiteClockInit = 0;
GLuint 			tBrownClockTexture;
int				iBrownClockInit = 0;
GLuint 			tQuartzClockTexture;
int				iQuartzClockInit = 0;
GLuint 			tSkyBlueClockTexture;
int				iSkyBlueClockInit = 0;
GLuint 			tArrowClockTexture;
int				iArrowClockInit = 0;
GLuint			tTachoMeterTexture;
int				iTachoMeterInit = 0;
GLuint			tIndicatorTexture;
int				iIndicatorInit = 0;
GLuint			tTempMeterTexture;
int				iTempMeterInit = 0;
GLuint			tWhiteTachoTexture;
int				iWhiteTachoInit = 0;
GLuint			tBlackTachoTexture;
int				iBlackTachoInit = 0;
GLuint			tCircleTachoTexture;
int				iCircleTachoInit = 0;
GLuint			tGreenTachoTexture;
int				iGreenTachoInit = 0;
GLuint			tOffTachoTexture;
int				iOffTachoInit = 0;
GLuint			tDarkMeterTexture;
int				iDarkMeterInit = 0;
GLuint			tBlackRegulatorTexture;
int				iBlackRegulatorInit = 0;
GLuint			tSilverRegulatorTexture;
int				iSilverRegulatorInit = 0;
GLuint			tDarkTermometerTexture;
int				iDarkTermometerInit = 0;
GLuint			tWhiteTermometerTexture;
int				iWhiteTermometerInit = 0;

GLmodel2D		mdElClockModel[16];
GLtexturemap	tmElClockMap[16];
GLmodel2D		mdMechClockModel[11];
GLtexturemap	tmMechClockMap[11];
GLmodel2D		mdWhiteClockModel[4];
GLtexturemap	tmWhiteClockMap[4];
GLmodel2D		mdBrownClockModel[4];
GLtexturemap	tmBrownClockMap[4];
GLmodel2D		mdQuartzClockModel[4];
GLtexturemap	tmQuartzClockMap[4];
GLmodel2D		mdSkyBlueClockModel[4];
GLtexturemap	tmSkyBlueClockMap[4];
GLmodel2D		mdArrowClockModel[3];
GLtexturemap	tmArrowClockMap[3];
GLmodel2D		mdTempMeterModel[2];
GLtexturemap	tmTempMeterMap[2];
GLmodel2D		mdTachoMeterModel[2];
GLtexturemap	tmTachoMeterMap[2];
GLmodel2D		mdIndicatorModel[2];
GLtexturemap	tmIndicatorMap[2];
GLmodel2D		mdWhiteTachoModel[2];
GLtexturemap	tmWhiteTachoMap[2];
GLmodel2D		mdBlackTachoModel[2];
GLtexturemap	tmBlackTachoMap[2];
GLmodel2D		mdCircleTachoModel[2];
GLtexturemap	tmCircleTachoMap[2];
GLmodel2D		mdGreenTachoModel[2];
GLtexturemap	tmGreenTachoMap[2];
GLmodel2D		mdOffTachoModel[2];
GLtexturemap	tmOffTachoMap[2];
GLmodel2D		mdDarkMeterModel[2];
GLtexturemap	tmDarkMeterMap[2];
GLmodel2D		mdBlackRegulatorModel[2];
GLtexturemap	tmBlackRegulatorMap[2];
GLmodel2D		mdSilverRegulatorModel[2];
GLtexturemap	tmSilverRegulatorMap[2];
GLmodel2D		mdSilverRegulatorModel[2];
GLtexturemap	tmSilverRegulatorMap[2];
GLmodel2D		mdDarkTermometerModel[3];
GLtexturemap	tmDarkTermometerMap[3];
GLmodel2D		mdWhiteTermometerModel[3];
GLtexturemap	tmWhiteTermometerMap[3];

void CreateWidgetModel(WIDGET_INFO *wiWidget, int iMirror);
int CreateWidgetSensorText(WIDGET_INFO* wiWidget);
int RenderWidgetSensorText(WIDGET_INFO* wiWidget);
int ReleaseWidgetSensorText(WIDGET_INFO* wiWidget);
int CreateWidgetSensorTachoMeter(WIDGET_INFO* wiWidget);
int RenderWidgetSensorTachoMeter(WIDGET_INFO* wiWidget);
int ReleaseWidgetSensorTachoMeter(WIDGET_INFO* wiWidget);
int CreateWidgetSensorIndicator(WIDGET_INFO* wiWidget);
int RenderWidgetSensorIndicator(WIDGET_INFO* wiWidget);
int ReleaseWidgetSensorIndicator(WIDGET_INFO* wiWidget);

int load_file (char *filename, char ** cBuffer, unsigned int *iSizeBuff) 
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

int get_half_second()
{
	struct timespec nanotime;
	int ret = 0;
	long int now_nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	now_nanotime = nanotime.tv_nsec/50000000;
	if (now_nanotime < 5) ret = 1;
	return ret;
}

void CreateIconModel(WIDGET_INFO *wiWidget)
{
	DBG_LOG_IN();
	
	wiWidget->IconModel.Point[0].X = 0;
	wiWidget->IconModel.Point[0].Y = 0;
	wiWidget->IconModel.Point[1].X = wiWidget->IconSourceWidth;
	wiWidget->IconModel.Point[1].Y = 0;
	wiWidget->IconModel.Point[2].X = 0;
	wiWidget->IconModel.Point[2].Y = wiWidget->IconSourceHeight;
	wiWidget->IconModel.Point[3].X = wiWidget->IconSourceWidth;
	wiWidget->IconModel.Point[3].Y = wiWidget->IconSourceHeight;
		
	wiWidget->IconTextureMap.Point[0].X = 0.0f;
	wiWidget->IconTextureMap.Point[0].Y = 1.0f;
	wiWidget->IconTextureMap.Point[1].X = 1.0f;
	wiWidget->IconTextureMap.Point[1].Y = 1.0f;
	wiWidget->IconTextureMap.Point[2].X = 0.0f;
	wiWidget->IconTextureMap.Point[2].Y = 0.0f;
	wiWidget->IconTextureMap.Point[3].X = 1.0f;
	wiWidget->IconTextureMap.Point[3].Y = 0.0f;
	
	DBG_LOG_OUT();
}

int CreateWidgetElClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 1000;
	GLfloat iWidgetHeight = 332;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iElClockInit)
	{
			
		GLfloat iWidth = 1000;
		GLfloat iHeight = 1000;
		
		int i;
		GLfloat PosX1 = 0.0f;
		GLfloat PosY1 = 0.331f;
		GLfloat PosX2 = 0.199f;
		GLfloat PosY2 = 0.0f;
		
		for (i = 0; i < 16; i++) 
		{
			mdElClockModel[i].Point[0].X = 0;
			mdElClockModel[i].Point[0].Y = 0;
			mdElClockModel[i].Point[1].X = 199;
			mdElClockModel[i].Point[1].Y = 0;
			mdElClockModel[i].Point[2].X = 0;
			mdElClockModel[i].Point[2].Y = 331;
			mdElClockModel[i].Point[3].X = 199;
			mdElClockModel[i].Point[3].Y = 331;
			
			tmElClockMap[i].Point[0].X = PosX1;
			tmElClockMap[i].Point[0].Y = PosY1;
			tmElClockMap[i].Point[1].X = PosX2;
			tmElClockMap[i].Point[1].Y = PosY1;
			tmElClockMap[i].Point[2].X = PosX1;
			tmElClockMap[i].Point[2].Y = PosY2;
			tmElClockMap[i].Point[3].X = PosX2;
			tmElClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += 0.2f;
			PosX2 += 0.2f;
			if ((i == 4) || (i == 9))
			{
				PosY1 += 0.332f;
				PosY2 += 0.332f;
				PosX1 = 0.0f;
				PosX2 = 0.199f;
			}
		}		
				
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		//if (cTexture1!=NULL) DBG_FREE(cTexture1);	
		/*if (load_file("Textures/elclocktex.raw", &cBuffer, &iSize) == 0) 
		{
			dbgprintf(1,"error open file: Textures/elclocktex.raw\n");
			return 0;
		}*/
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/ElClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'ElClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture ElClock.png for electronic clock %i != 1000   %i != 1000\n", sW, sH);
			DBG_FREE(cBuffer);
			return 0;
		}
		//int n;
		//int i = 0;
		//unsigned int uiColor = Hex2Int(wiWidget->Color);
		//RGB_T *rgbColor = (RGB_T*)&uiColor;
		
		RGBA_T *cTexture1 = (RGBA_T*)cBuffer;
		unsigned int uiMatrixSize = sW * sH;
		int n;
		for (n = 0; n != uiMatrixSize; n++)
		{
			cTexture1[n].Red = wiWidget->RGBColor.Red;
			cTexture1[n].Green = wiWidget->RGBColor.Green;
			cTexture1[n].Blue = wiWidget->RGBColor.Blue;
		}
		
		glGenTextures(1, &tElClockTexture);
		glBindTexture(GL_TEXTURE_2D, tElClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iElClockInit = 1;
    }
    	
	return 1;
}

int RenderWidgetElClock(WIDGET_INFO* wiWidget)
{
	if (!iElClockInit) return 0;
	
	GLfloat PosX = 0;
	GLfloat PosY = 0;
	int pos1, pos2;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
		
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdElClockModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmElClockMap); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    glBindTexture(GL_TEXTURE_2D, tElClockTexture);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	//Render Hours
	pos1 = (int)(timeinfo.tm_hour/10);
	pos2 = timeinfo.tm_hour-(pos1*10);    
    pos1*=4;
	pos2*=4;
	glTranslatef(PosX, PosY, 0.0f);
	PosX+=200;
    glDrawArrays(GL_TRIANGLE_STRIP, pos1, 4);
	glTranslatef(PosX, PosY, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, pos2, 4);  
    glTranslatef(PosX, PosY, 0.0f);
    if (get_half_second() == 1) glDrawArrays( GL_TRIANGLE_STRIP, 44, 4);
    
    //Render Mins
	pos1 = (int)(timeinfo.tm_min/10);
	pos2 = timeinfo.tm_min-(pos1*10);    
    pos1*=4;
	pos2*=4;
	glTranslatef(PosX, PosY, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, pos1, 4);
	glTranslatef(PosX, PosY, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, pos2, 4);  
    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetElClock(WIDGET_INFO* wiWidget)
{
	if (iElClockInit) glDeleteTextures(1, &tElClockTexture); else return 0;
	return 1;
}

int CreateWidgetMechClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 1133;
	GLfloat iWidgetHeight = 595;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
	
	if (iMechClockInit == 0)
	{
		GLfloat iWidth = 1152;
		GLfloat iHeight = 1344;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdMechClockModel[0].Point[0].X = 0;
		mdMechClockModel[0].Point[0].Y = 0;
		mdMechClockModel[0].Point[1].X = iWidgetWidth;
		mdMechClockModel[0].Point[1].Y = 0;
		mdMechClockModel[0].Point[2].X = 0;
		mdMechClockModel[0].Point[2].Y = iWidgetHeight;
		mdMechClockModel[0].Point[3].X = iWidgetWidth;
		mdMechClockModel[0].Point[3].Y = iWidgetHeight;
		
		tmMechClockMap[0].Point[0].X = 0.0f;
		tmMechClockMap[0].Point[0].Y = fHeightBase;
		tmMechClockMap[0].Point[1].X = fWidthBase;
		tmMechClockMap[0].Point[1].Y = fHeightBase;
		tmMechClockMap[0].Point[2].X = 0.0f;
		tmMechClockMap[0].Point[2].Y = 0.0f;
		tmMechClockMap[0].Point[3].X = fWidthBase;
		tmMechClockMap[0].Point[3].Y = 0.0f;
		
		GLfloat fWidthStep = 215/iWidth;
		GLfloat fHeightStep = 371/iHeight;
		GLfloat PosX1 = 0.0f;
		GLfloat PosY1 = fHeightBase + fHeightStep;
		GLfloat PosX2 = fWidthStep;
		GLfloat PosY2 = fHeightBase;
		
		
		for (i = 1; i < 11; i++) 
		{
			mdMechClockModel[i].Point[0].X = 0;
			mdMechClockModel[i].Point[0].Y = 0;
			mdMechClockModel[i].Point[1].X = 214;
			mdMechClockModel[i].Point[1].Y = 0;
			mdMechClockModel[i].Point[2].X = 0;
			mdMechClockModel[i].Point[2].Y = 370;
			mdMechClockModel[i].Point[3].X = 214;
			mdMechClockModel[i].Point[3].Y = 370;
			
			tmMechClockMap[i].Point[0].X = PosX1;
			tmMechClockMap[i].Point[0].Y = PosY1;
			tmMechClockMap[i].Point[1].X = PosX2;
			tmMechClockMap[i].Point[1].Y = PosY1;
			tmMechClockMap[i].Point[2].X = PosX1;
			tmMechClockMap[i].Point[2].Y = PosY2;
			tmMechClockMap[i].Point[3].X = PosX2;
			tmMechClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
			if (i == 5)
			{
				PosY1 += fHeightStep;
				PosY2 += fHeightStep;
				PosX1 = 0.0f;
				PosX2 = fWidthStep;
			}
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/MechClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'MechClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture MechClock.png for mechanic clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
				
		glGenTextures(1, &tMechClockTexture);
		glBindTexture(GL_TEXTURE_2D, tMechClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iMechClockInit = 1;
    }
    	
	return 1;
}

int RenderWidgetMechClock(WIDGET_INFO* wiWidget)
{
	if (!iMechClockInit) return 0;
	
	GLfloat PosX = 0;
	GLfloat PosY = 0;
	int pos1, pos2;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdMechClockModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmMechClockMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tMechClockTexture);
	
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	//Render Hours
	pos1 = (int)(timeinfo.tm_hour/10);
	pos2 = timeinfo.tm_hour-(pos1*10);    
    pos1*=4;
	pos2*=4;
	
	
	glTranslatef(65, 108, 0.0f);
	if (pos1) glDrawArrays(GL_TRIANGLE_STRIP, pos1+4, 4);
	
	PosX += 230;    
    glTranslatef(PosX, PosY, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, pos2+4, 4);  
    
	glTranslatef(331, 0, 0.0f);
    
    //Render Mins
	pos1 = (int)(timeinfo.tm_min/10);
	pos2 = timeinfo.tm_min-(pos1*10);    
    pos1*=4;
	pos2*=4;
	glDrawArrays(GL_TRIANGLE_STRIP, pos1+4, 4);
	glTranslatef(PosX, PosY, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, pos2+4, 4);  
    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetMechClock(WIDGET_INFO* wiWidget)
{
	if (iMechClockInit) glDeleteTextures(1, &tMechClockTexture); else return 0;
	return 1;
}

int CreateWidgetWhiteClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 1200;
	GLfloat iWidgetHeight = 1200;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iWhiteClockInit)
	{
			
		GLfloat iWidth = 1408;
		GLfloat iHeight = 1216;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdWhiteClockModel[0].Point[0].X = 0;
		mdWhiteClockModel[0].Point[0].Y = 0;
		mdWhiteClockModel[0].Point[1].X = iWidgetWidth;
		mdWhiteClockModel[0].Point[1].Y = 0;
		mdWhiteClockModel[0].Point[2].X = 0;
		mdWhiteClockModel[0].Point[2].Y = iWidgetHeight;
		mdWhiteClockModel[0].Point[3].X = iWidgetWidth;
		mdWhiteClockModel[0].Point[3].Y = iWidgetHeight;
		
		tmWhiteClockMap[0].Point[0].X = 0.0f;
		tmWhiteClockMap[0].Point[0].Y = fHeightBase;
		tmWhiteClockMap[0].Point[1].X = fWidthBase;
		tmWhiteClockMap[0].Point[1].Y = fHeightBase;
		tmWhiteClockMap[0].Point[2].X = 0.0f;
		tmWhiteClockMap[0].Point[2].Y = 0.0f;
		tmWhiteClockMap[0].Point[3].X = fWidthBase;
		tmWhiteClockMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 1217/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 401/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 4; i++) 
		{
			mdWhiteClockModel[i].Point[0].X = -31;
			mdWhiteClockModel[i].Point[0].Y = -97;
			mdWhiteClockModel[i].Point[1].X = 32;
			mdWhiteClockModel[i].Point[1].Y = -97;
			mdWhiteClockModel[i].Point[2].X = -31;
			mdWhiteClockModel[i].Point[2].Y = 304;
			mdWhiteClockModel[i].Point[3].X = 32;
			mdWhiteClockModel[i].Point[3].Y = 304;
			
			tmWhiteClockMap[i].Point[0].X = PosX1;
			tmWhiteClockMap[i].Point[0].Y = PosY1;
			tmWhiteClockMap[i].Point[1].X = PosX2;
			tmWhiteClockMap[i].Point[1].Y = PosY1;
			tmWhiteClockMap[i].Point[2].X = PosX1;
			tmWhiteClockMap[i].Point[2].Y = PosY2;
			tmWhiteClockMap[i].Point[3].X = PosX2;
			tmWhiteClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/WhiteClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'WhiteClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture WhiteClock.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
				
		glGenTextures(1, &tWhiteClockTexture);
		glBindTexture(GL_TEXTURE_2D, tWhiteClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iWhiteClockInit = 1;
    }
    	
	return 1;
}

int RenderWidgetWhiteClock(WIDGET_INFO* wiWidget)
{
	if (!iWhiteClockInit) return 0;

	GLfloat fAngle;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdWhiteClockModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmWhiteClockMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tWhiteClockTexture);
	
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glTranslatef(600, 600, 0.0f);
	
	//Render Hours
	if (timeinfo.tm_hour >= 12) timeinfo.tm_hour -= 12;		
	fAngle = (GLfloat)(timeinfo.tm_hour) * 360/12;
	fAngle += (GLfloat)(timeinfo.tm_min) * 360/720;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);
    
    //Render Mins
	fAngle = (GLfloat)(timeinfo.tm_min) * 6;
	fAngle += (GLfloat)(timeinfo.tm_sec) * 360/3600;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);	
	
	//Render Secs
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	int now_mstime = nanotime.tv_nsec/10000000;
	
	fAngle = (GLfloat)(timeinfo.tm_sec) * 6;
	fAngle += (GLfloat)(now_mstime) * 360/6000;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetWhiteClock(WIDGET_INFO* wiWidget)
{
	if (iWhiteClockInit) glDeleteTextures(1, &tWhiteClockTexture); else return 0;
	return 1;
}

int CreateWidgetBrownClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 1200;
	GLfloat iWidgetHeight = 1200;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
	
	if (iBrownClockInit == 0)
	{
		GLfloat iWidth = 1408;
		GLfloat iHeight = 1216;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdBrownClockModel[0].Point[0].X = 0;
		mdBrownClockModel[0].Point[0].Y = 0;
		mdBrownClockModel[0].Point[1].X = iWidgetWidth;
		mdBrownClockModel[0].Point[1].Y = 0;
		mdBrownClockModel[0].Point[2].X = 0;
		mdBrownClockModel[0].Point[2].Y = iWidgetHeight;
		mdBrownClockModel[0].Point[3].X = iWidgetWidth;
		mdBrownClockModel[0].Point[3].Y = iWidgetHeight;
		
		tmBrownClockMap[0].Point[0].X = 0.0f;
		tmBrownClockMap[0].Point[0].Y = fHeightBase;
		tmBrownClockMap[0].Point[1].X = fWidthBase;
		tmBrownClockMap[0].Point[1].Y = fHeightBase;
		tmBrownClockMap[0].Point[2].X = 0.0f;
		tmBrownClockMap[0].Point[2].Y = 0.0f;
		tmBrownClockMap[0].Point[3].X = fWidthBase;
		tmBrownClockMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 1216/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 430/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 4; i++) 
		{
			mdBrownClockModel[i].Point[0].X = -31;
			mdBrownClockModel[i].Point[0].Y = -29;
			mdBrownClockModel[i].Point[1].X = 32;
			mdBrownClockModel[i].Point[1].Y = -29;
			mdBrownClockModel[i].Point[2].X = -31;
			mdBrownClockModel[i].Point[2].Y = 401;
			mdBrownClockModel[i].Point[3].X = 32;
			mdBrownClockModel[i].Point[3].Y = 401;
			
			tmBrownClockMap[i].Point[0].X = PosX1;
			tmBrownClockMap[i].Point[0].Y = PosY1;
			tmBrownClockMap[i].Point[1].X = PosX2;
			tmBrownClockMap[i].Point[1].Y = PosY1;
			tmBrownClockMap[i].Point[2].X = PosX1;
			tmBrownClockMap[i].Point[2].Y = PosY2;
			tmBrownClockMap[i].Point[3].X = PosX2;
			tmBrownClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/BrownClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'BrownClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture BrownClock.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
				
		glGenTextures(1, &tBrownClockTexture);
		glBindTexture(GL_TEXTURE_2D, tBrownClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iBrownClockInit = 1;
    }
    	
	return 1;
}

int RenderWidgetBrownClock(WIDGET_INFO* wiWidget)
{
	if (!iBrownClockInit) return 0;

	GLfloat fAngle;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdBrownClockModel); //point to vert array
	glEnableClientState(GL_VERTEX_ARRAY); //enable vert array
	glTexCoordPointer(2, GL_FLOAT, 0, tmBrownClockMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tBrownClockTexture);
	
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
		
	glTranslatef(610, 595, 0.0f);
	
	//Render Hours
	if (timeinfo.tm_hour >= 12) timeinfo.tm_hour -= 12;		
	fAngle = (GLfloat)(timeinfo.tm_hour) * 360/12;
	fAngle += (GLfloat)(timeinfo.tm_min) * 360/720;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);
    
    //Render Mins
	fAngle = (GLfloat)(timeinfo.tm_min) * 6;
	fAngle += (GLfloat)(timeinfo.tm_sec) * 360/3600;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);	
	
	//Render Secs
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	int now_mstime = nanotime.tv_nsec/10000000;
	
	fAngle = (GLfloat)(timeinfo.tm_sec) * 6;
	fAngle += (GLfloat)(now_mstime) * 360/6000;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetBrownClock(WIDGET_INFO* wiWidget)
{
	if (iBrownClockInit) glDeleteTextures(1, &tBrownClockTexture); else return 0;
	return 1;
}

int CreateWidgetQuartzClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 754;
	GLfloat iWidgetHeight = 754;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iQuartzClockInit)
	{			
		GLfloat iWidth = 960;
		GLfloat iHeight = 768;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdQuartzClockModel[0].Point[0].X = 0;
		mdQuartzClockModel[0].Point[0].Y = 0;
		mdQuartzClockModel[0].Point[1].X = iWidgetWidth;
		mdQuartzClockModel[0].Point[1].Y = 0;
		mdQuartzClockModel[0].Point[2].X = 0;
		mdQuartzClockModel[0].Point[2].Y = iWidgetHeight;
		mdQuartzClockModel[0].Point[3].X = iWidgetWidth;
		mdQuartzClockModel[0].Point[3].Y = iWidgetHeight;
		
		tmQuartzClockMap[0].Point[0].X = 0.0f;
		tmQuartzClockMap[0].Point[0].Y = fHeightBase;
		tmQuartzClockMap[0].Point[1].X = fWidthBase;
		tmQuartzClockMap[0].Point[1].Y = fHeightBase;
		tmQuartzClockMap[0].Point[2].X = 0.0f;
		tmQuartzClockMap[0].Point[2].Y = 0.0f;
		tmQuartzClockMap[0].Point[3].X = fWidthBase;
		tmQuartzClockMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 768/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 412/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 4; i++) 
		{
			mdQuartzClockModel[i].Point[0].X = -31;
			mdQuartzClockModel[i].Point[0].Y = -111;
			mdQuartzClockModel[i].Point[1].X = 32;
			mdQuartzClockModel[i].Point[1].Y = -111;
			mdQuartzClockModel[i].Point[2].X = -31;
			mdQuartzClockModel[i].Point[2].Y = 300;
			mdQuartzClockModel[i].Point[3].X = 32;
			mdQuartzClockModel[i].Point[3].Y = 300;
			
			tmQuartzClockMap[i].Point[0].X = PosX1;
			tmQuartzClockMap[i].Point[0].Y = PosY1;
			tmQuartzClockMap[i].Point[1].X = PosX2;
			tmQuartzClockMap[i].Point[1].Y = PosY1;
			tmQuartzClockMap[i].Point[2].X = PosX1;
			tmQuartzClockMap[i].Point[2].Y = PosY2;
			tmQuartzClockMap[i].Point[3].X = PosX2;
			tmQuartzClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/QuartzClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'QuartzClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture QuartzClock.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
				
		glGenTextures(1, &tQuartzClockTexture);
		glBindTexture(GL_TEXTURE_2D, tQuartzClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iQuartzClockInit = 1;
    }
        	
	return 1;
}

int RenderWidgetQuartzClock(WIDGET_INFO* wiWidget)
{
	if (!iQuartzClockInit) return 0;

	GLfloat fAngle;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdQuartzClockModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmQuartzClockMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tQuartzClockTexture);
	
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
	glTranslatef(380, 376, 0.0f);
	
	//Render Hours
	if (timeinfo.tm_hour >= 12) timeinfo.tm_hour -= 12;		
	fAngle = (GLfloat)(timeinfo.tm_hour) * 360/12;
	fAngle += (GLfloat)(timeinfo.tm_min) * 360/720;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);
    
    //Render Mins
	fAngle = (GLfloat)(timeinfo.tm_min) * 6;
	fAngle += (GLfloat)(timeinfo.tm_sec) * 360/3600;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);	
	
	//Render Secs
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	int now_mstime = nanotime.tv_nsec/10000000;
	
	fAngle = (GLfloat)(timeinfo.tm_sec) * 6;
	fAngle += (GLfloat)(now_mstime) * 360/6000;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetQuartzClock(WIDGET_INFO* wiWidget)
{
	if (iQuartzClockInit) glDeleteTextures(1, &tQuartzClockTexture); else return 0;
	return 1;
}

int CreateWidgetSkyBlueClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 1035;
	GLfloat iWidgetHeight = 1035;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iSkyBlueClockInit)
	{			
		GLfloat iWidth = 1280;
		GLfloat iHeight = 1088;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdSkyBlueClockModel[0].Point[0].X = 0;
		mdSkyBlueClockModel[0].Point[0].Y = 0;
		mdSkyBlueClockModel[0].Point[1].X = iWidgetWidth;
		mdSkyBlueClockModel[0].Point[1].Y = 0;
		mdSkyBlueClockModel[0].Point[2].X = 0;
		mdSkyBlueClockModel[0].Point[2].Y = iWidgetHeight;
		mdSkyBlueClockModel[0].Point[3].X = iWidgetWidth;
		mdSkyBlueClockModel[0].Point[3].Y = iWidgetHeight;
		
		tmSkyBlueClockMap[0].Point[0].X = 0.0f;
		tmSkyBlueClockMap[0].Point[0].Y = fHeightBase;
		tmSkyBlueClockMap[0].Point[1].X = fWidthBase;
		tmSkyBlueClockMap[0].Point[1].Y = fHeightBase;
		tmSkyBlueClockMap[0].Point[2].X = 0.0f;
		tmSkyBlueClockMap[0].Point[2].Y = 0.0f;
		tmSkyBlueClockMap[0].Point[3].X = fWidthBase;
		tmSkyBlueClockMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 1088/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 530/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 4; i++) 
		{
			mdSkyBlueClockModel[i].Point[0].X = -31;
			mdSkyBlueClockModel[i].Point[0].Y = -143;
			mdSkyBlueClockModel[i].Point[1].X = 32;
			mdSkyBlueClockModel[i].Point[1].Y = -143;
			mdSkyBlueClockModel[i].Point[2].X = -31;
			mdSkyBlueClockModel[i].Point[2].Y = 387;
			mdSkyBlueClockModel[i].Point[3].X = 32;
			mdSkyBlueClockModel[i].Point[3].Y = 387;
			
			tmSkyBlueClockMap[i].Point[0].X = PosX1;
			tmSkyBlueClockMap[i].Point[0].Y = PosY1;
			tmSkyBlueClockMap[i].Point[1].X = PosX2;
			tmSkyBlueClockMap[i].Point[1].Y = PosY1;
			tmSkyBlueClockMap[i].Point[2].X = PosX1;
			tmSkyBlueClockMap[i].Point[2].Y = PosY2;
			tmSkyBlueClockMap[i].Point[3].X = PosX2;
			tmSkyBlueClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/SkyBlueClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'SkyBlueClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture SkyBlueClock.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
				
		glGenTextures(1, &tSkyBlueClockTexture);
		glBindTexture(GL_TEXTURE_2D, tSkyBlueClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iSkyBlueClockInit = 1;
    }
    	
	return 1;
}

int RenderWidgetSkyBlueClock(WIDGET_INFO* wiWidget)
{
	if (!iSkyBlueClockInit) return 0;

	GLfloat fAngle;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdSkyBlueClockModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmSkyBlueClockMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tSkyBlueClockTexture);
	
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
	glTranslatef(518, 510, 0.0f);
	
	//Render Hours
	if (timeinfo.tm_hour >= 12) timeinfo.tm_hour -= 12;		
	fAngle = (GLfloat)(timeinfo.tm_hour) * 360/12;
	fAngle += (GLfloat)(timeinfo.tm_min) * 360/720;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);
    
    //Render Mins
	fAngle = (GLfloat)(timeinfo.tm_min) * 6;
	fAngle += (GLfloat)(timeinfo.tm_sec) * 360/3600;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);	
	
	//Render Secs
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	int now_mstime = nanotime.tv_nsec/10000000;
	
	fAngle = (GLfloat)(timeinfo.tm_sec) * 6;
	fAngle += (GLfloat)(now_mstime) * 360/6000;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetSkyBlueClock(WIDGET_INFO* wiWidget)
{
	if (iSkyBlueClockInit) glDeleteTextures(1, &tSkyBlueClockTexture); else return 0;
	return 1;
}

int CreateWidgetArrowClock(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 1088;
	GLfloat iWidgetHeight = 1088;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
			
	if (iArrowClockInit == 0)
	{			
		GLfloat iWidth = 1216;
		GLfloat iHeight = 1152;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdArrowClockModel[0].Point[0].X = 0;
		mdArrowClockModel[0].Point[0].Y = 0;
		mdArrowClockModel[0].Point[1].X = iWidgetWidth;
		mdArrowClockModel[0].Point[1].Y = 0;
		mdArrowClockModel[0].Point[2].X = 0;
		mdArrowClockModel[0].Point[2].Y = iWidgetHeight;
		mdArrowClockModel[0].Point[3].X = iWidgetWidth;
		mdArrowClockModel[0].Point[3].Y = iWidgetHeight;
		
		tmArrowClockMap[0].Point[0].X = 0.0f;
		tmArrowClockMap[0].Point[0].Y = fHeightBase;
		tmArrowClockMap[0].Point[1].X = fWidthBase;
		tmArrowClockMap[0].Point[1].Y = fHeightBase;
		tmArrowClockMap[0].Point[2].X = 0.0f;
		tmArrowClockMap[0].Point[2].Y = 0.0f;
		tmArrowClockMap[0].Point[3].X = fWidthBase;
		tmArrowClockMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = iWidgetWidth/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 490/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 3; i++) 
		{
			mdArrowClockModel[i].Point[0].X = -31;
			mdArrowClockModel[i].Point[0].Y = -89;
			mdArrowClockModel[i].Point[1].X = 32;
			mdArrowClockModel[i].Point[1].Y = -89;
			mdArrowClockModel[i].Point[2].X = -31;
			mdArrowClockModel[i].Point[2].Y = 400;
			mdArrowClockModel[i].Point[3].X = 32;
			mdArrowClockModel[i].Point[3].Y = 400;
			
			tmArrowClockMap[i].Point[0].X = PosX1;
			tmArrowClockMap[i].Point[0].Y = PosY1;
			tmArrowClockMap[i].Point[1].X = PosX2;
			tmArrowClockMap[i].Point[1].Y = PosY1;
			tmArrowClockMap[i].Point[2].X = PosX1;
			tmArrowClockMap[i].Point[2].Y = PosY2;
			tmArrowClockMap[i].Point[3].X = PosX2;
			tmArrowClockMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/ArrowClock.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'ArrowClock.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture ArrowClock.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
		
		RGBA_T *cTexture1 = (RGBA_T*)cBuffer;
		unsigned int uiMatrixSize = sW * sH;
		int n;
		for (n = 0; n != uiMatrixSize; n++)
		{
			cTexture1[n].Red = wiWidget->RGBColor.Red;
			cTexture1[n].Green = wiWidget->RGBColor.Green;
			cTexture1[n].Blue = wiWidget->RGBColor.Blue;
		}
				
		glGenTextures(1, &tArrowClockTexture);
		glBindTexture(GL_TEXTURE_2D, tArrowClockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
		iArrowClockInit = 1;
    }
    	
	return 1;
}

int RenderWidgetArrowClock(WIDGET_INFO* wiWidget)
{
	if (!iArrowClockInit) return 0;

	GLfloat fAngle;
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	if (fScale != 1.0f) glScalef(fScale,fScale,0.0f);
	glVertexPointer(2, GL_FLOAT, 0, mdArrowClockModel); //point to vert array
	glEnableClientState(GL_VERTEX_ARRAY); //enable vert array
	glTexCoordPointer(2, GL_FLOAT, 0, tmArrowClockMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tArrowClockTexture);
	
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
	glTranslatef(545, 556, 0.0f);
	
	//Render Hours
	if (timeinfo.tm_hour >= 12) timeinfo.tm_hour -= 12;		
	fAngle = (GLfloat)(timeinfo.tm_hour) * 360/12;
	fAngle += (GLfloat)(timeinfo.tm_min) * 360/720;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);
    
    //Render Mins
	fAngle = (GLfloat)(timeinfo.tm_min) * 6;
	fAngle += (GLfloat)(timeinfo.tm_sec) * 360/3600;
	glRotatef(-fAngle, 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	glRotatef(fAngle, 0.0f, 0.0f, 1.0f);	
	
	    
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetArrowClock(WIDGET_INFO* wiWidget)
{
	if (iArrowClockInit) glDeleteTextures(1, &tArrowClockTexture); else return 0;
	return 1;
}

int CreateWidgetImage(WIDGET_INFO *wiWidget)
{
	int sW = 2048;
	int sH = 2048;	
	
	unsigned int uiSize;
	void *pBuff = NULL;
	if ((!wiWidget->Path[0]) || (omx_image_to_buffer(wiWidget->Path, &pBuff, &uiSize, &sW, &sH) != 1))
	{
		dbgprintf(2, "Error omx_image_to_buffer for widget %i '%s'\n", wiWidget->WidgetID, wiWidget->Path);
		return 0;
	}
	
	glGenTextures(1,&wiWidget->IconTexture);
	glBindTexture (GL_TEXTURE_2D, wiWidget->IconTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//GL_LINEAR
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pBuff);	
	DBG_FREE(pBuff);
	
	if (sW > sH)
		wiWidget->IconScale = wiWidget->IconDefaultWidth / sW;
		else
		wiWidget->IconScale = wiWidget->IconDefaultHeight / sH;	
	wiWidget->IconSourceWidth = sW;
	wiWidget->IconSourceHeight = sH;
	wiWidget->IconRenderWidth = sW * wiWidget->IconScale;
	wiWidget->IconRenderHeight = sH * wiWidget->IconScale;
	
	if (wiWidget->SizeX == 0) 
	{
		wiWidget->Width = wiWidget->IconRenderWidth;
		wiWidget->SizeX = wiWidget->IconRenderWidth;
	}
	if (wiWidget->SizeY == 0) 
	{
		wiWidget->Height = wiWidget->IconRenderHeight;
		wiWidget->SizeY = wiWidget->IconRenderHeight;
	}
	
	CreateIconModel(wiWidget);
	
	return 1;	
}

int RenderWidgetImage(WIDGET_INFO *wiWidget, char cSetBlend)
{
	if (cSetBlend) 
	{
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
	}
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
		
	glLoadIdentity();
	
	glVertexPointer(2, GL_FLOAT, 0, &wiWidget->IconModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, &wiWidget->IconTextureMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);        
	glBindTexture(GL_TEXTURE_2D, wiWidget->IconTexture);

	glScalef(fScale * wiWidget->IconScale, fScale * wiWidget->IconScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale/wiWidget->IconScale, wiWidget->PosY/fScale/wiWidget->IconScale, 0.0f);							
			
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (cSetBlend) 
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}
	return 1;
}

int ReleaseWidgetImage(WIDGET_INFO* wiWidget)
{
	glDeleteTextures(1, &wiWidget->IconTexture);	
	return 1;
}

int CreateWidgetSensorImage(WIDGET_INFO* wiWidget)
{
	int result = 1;
	if (wiWidget->Path[0]) 
		result = CreateWidgetImage(wiWidget);
		else wiWidget->Height = wiWidget->IconDefaultHeight;
	return result;
}

int RenderWidgetSensorImage(WIDGET_INFO *wiWidget)
{
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	//GLfloat fHeight = wiWidget->IconDefaultHeight;
	//GLfloat fWidth = 0;
	if (wiWidget->IconSourceWidth) 
	{
		RenderWidgetImage(wiWidget, 1);
		//fWidth = wiWidget->IconRenderWidth;
		//fHeight = wiWidget->IconRenderHeight;
	}
	
	if (wiWidget->Name[0] != 0) RenderGLText(18*fScale, wiWidget->PosX, 
													wiWidget->PosY + wiWidget->Height*fScale, 
													"%s", wiWidget->Name);
	RenderGLText(wiWidget->Height*1.5*fScale, wiWidget->PosX + wiWidget->Width * fScale, wiWidget->PosY - (wiWidget->Height*1.5*fScale*0.22), wiWidget->SensorValueStr);		
		
	return 1;
}

int ReleaseWidgetSensorImage(WIDGET_INFO* wiWidget)
{
	if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	return 1;
}

int CreateWidgetSensorTachoMeter(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 512;
	GLfloat iWidgetHeight = 608;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iTachoMeterInit)
	{		
		GLfloat iWidth = 768;
		GLfloat iHeight = 608;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdTachoMeterModel[0].Point[0].X = 0;
		mdTachoMeterModel[0].Point[0].Y = 0;
		mdTachoMeterModel[0].Point[1].X = iWidgetWidth;
		mdTachoMeterModel[0].Point[1].Y = 0;
		mdTachoMeterModel[0].Point[2].X = 0;
		mdTachoMeterModel[0].Point[2].Y = iWidgetHeight;
		mdTachoMeterModel[0].Point[3].X = iWidgetWidth;
		mdTachoMeterModel[0].Point[3].Y = iWidgetHeight;
		
		tmTachoMeterMap[0].Point[0].X = 0.0f;
		tmTachoMeterMap[0].Point[0].Y = fHeightBase;
		tmTachoMeterMap[0].Point[1].X = fWidthBase;
		tmTachoMeterMap[0].Point[1].Y = fHeightBase;
		tmTachoMeterMap[0].Point[2].X = 0.0f;
		tmTachoMeterMap[0].Point[2].Y = 0.0f;
		tmTachoMeterMap[0].Point[3].X = fWidthBase;
		tmTachoMeterMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 512/iWidth;
		GLfloat fWidthStep = 256/iWidth;
		GLfloat fHeightStep = 512/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdTachoMeterModel[i].Point[0].X = -127;
			mdTachoMeterModel[i].Point[0].Y = -197;
			mdTachoMeterModel[i].Point[1].X = 128;
			mdTachoMeterModel[i].Point[1].Y = -197;
			mdTachoMeterModel[i].Point[2].X = -127;
			mdTachoMeterModel[i].Point[2].Y = 315;
			mdTachoMeterModel[i].Point[3].X = 128;
			mdTachoMeterModel[i].Point[3].Y = 315;
			
			tmTachoMeterMap[i].Point[0].X = PosX1;
			tmTachoMeterMap[i].Point[0].Y = PosY1;
			tmTachoMeterMap[i].Point[1].X = PosX2;
			tmTachoMeterMap[i].Point[1].Y = PosY1;
			tmTachoMeterMap[i].Point[2].X = PosX1;
			tmTachoMeterMap[i].Point[2].Y = PosY2;
			tmTachoMeterMap[i].Point[3].X = PosX2;
			tmTachoMeterMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/TachoMeter.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'TachoMeter.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture TachoMeter.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tTachoMeterTexture);
		glBindTexture(GL_TEXTURE_2D, tTachoMeterTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iTachoMeterInit = 1;
	}
	
	if (iTachoMeterInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorTachoMeter(WIDGET_INFO* wiWidget)
{	
	if (!iTachoMeterInit) return 0;	
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
	}
	
	glVertexPointer(2, GL_FLOAT, 0, mdTachoMeterModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmTachoMeterMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tTachoMeterTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glTranslatef(341, 263, 0.0f);	
	
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -10) val = -10;
	if (val > 115) val = 115;
	val *= 1.67f;	
	
	glRotatef(139 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}


int ReleaseWidgetSensorTachoMeter(WIDGET_INFO* wiWidget)
{
	if (iTachoMeterInit)
	{
		glDeleteTextures(1, &tTachoMeterTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}


int CreateWidgetSensorIndicator(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 512;
	GLfloat iWidgetHeight = 608;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iIndicatorInit)
	{		
		GLfloat iWidth = 768;
		GLfloat iHeight = 608;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdIndicatorModel[0].Point[0].X = 0;
		mdIndicatorModel[0].Point[0].Y = 0;
		mdIndicatorModel[0].Point[1].X = iWidgetWidth;
		mdIndicatorModel[0].Point[1].Y = 0;
		mdIndicatorModel[0].Point[2].X = 0;
		mdIndicatorModel[0].Point[2].Y = iWidgetHeight;
		mdIndicatorModel[0].Point[3].X = iWidgetWidth;
		mdIndicatorModel[0].Point[3].Y = iWidgetHeight;
		
		tmIndicatorMap[0].Point[0].X = 0.0f;
		tmIndicatorMap[0].Point[0].Y = fHeightBase;
		tmIndicatorMap[0].Point[1].X = fWidthBase;
		tmIndicatorMap[0].Point[1].Y = fHeightBase;
		tmIndicatorMap[0].Point[2].X = 0.0f;
		tmIndicatorMap[0].Point[2].Y = 0.0f;
		tmIndicatorMap[0].Point[3].X = fWidthBase;
		tmIndicatorMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 512/iWidth;
		GLfloat fWidthStep = 256/iWidth;
		GLfloat fHeightStep = 512/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdIndicatorModel[i].Point[0].X = -127;
			mdIndicatorModel[i].Point[0].Y = -197;
			mdIndicatorModel[i].Point[1].X = 128;
			mdIndicatorModel[i].Point[1].Y = -197;
			mdIndicatorModel[i].Point[2].X = -127;
			mdIndicatorModel[i].Point[2].Y = 315;
			mdIndicatorModel[i].Point[3].X = 128;
			mdIndicatorModel[i].Point[3].Y = 315;
			
			tmIndicatorMap[i].Point[0].X = PosX1;
			tmIndicatorMap[i].Point[0].Y = PosY1;
			tmIndicatorMap[i].Point[1].X = PosX2;
			tmIndicatorMap[i].Point[1].Y = PosY1;
			tmIndicatorMap[i].Point[2].X = PosX1;
			tmIndicatorMap[i].Point[2].Y = PosY2;
			tmIndicatorMap[i].Point[3].X = PosX2;
			tmIndicatorMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/Indicator.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'Indicator.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture Indicator.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tIndicatorTexture);
		glBindTexture(GL_TEXTURE_2D, tIndicatorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iIndicatorInit = 1;
	}
	
	if (iIndicatorInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorIndicator(WIDGET_INFO* wiWidget)
{	
	if (!iIndicatorInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
	}
	
	glVertexPointer(2, GL_FLOAT, 0, mdIndicatorModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmIndicatorMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tIndicatorTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glTranslatef(341, 263, 0.0f);	
	
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -10) val = -10;
	if (val > 115) val = 115;
	val *= 1.67f;	
	
	glRotatef(139 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}


int ReleaseWidgetSensorIndicator(WIDGET_INFO* wiWidget)
{
	if (iIndicatorInit)
	{
		glDeleteTextures(1, &tIndicatorTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorTempMeter(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 616;
	GLfloat iWidgetHeight = 616;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
	
	if (!iTempMeterInit)
	{			
		GLfloat iWidth = 704;
		GLfloat iHeight = 640;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdTempMeterModel[0].Point[0].X = 0;
		mdTempMeterModel[0].Point[0].Y = 0;
		mdTempMeterModel[0].Point[1].X = iWidgetWidth;
		mdTempMeterModel[0].Point[1].Y = 0;
		mdTempMeterModel[0].Point[2].X = 0;
		mdTempMeterModel[0].Point[2].Y = iWidgetHeight;
		mdTempMeterModel[0].Point[3].X = iWidgetWidth;
		mdTempMeterModel[0].Point[3].Y = iWidgetHeight;
		
		tmTempMeterMap[0].Point[0].X = 0.0f;
		tmTempMeterMap[0].Point[0].Y = fHeightBase;
		tmTempMeterMap[0].Point[1].X = fWidthBase;
		tmTempMeterMap[0].Point[1].Y = fHeightBase;
		tmTempMeterMap[0].Point[2].X = 0.0f;
		tmTempMeterMap[0].Point[2].Y = 0.0f;
		tmTempMeterMap[0].Point[3].X = fWidthBase;
		tmTempMeterMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 640/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 285/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 2; i < 3; i++) 
		{
			mdTempMeterModel[i].Point[0].X = -31;
			mdTempMeterModel[i].Point[0].Y = -72;
			mdTempMeterModel[i].Point[1].X = 32;
			mdTempMeterModel[i].Point[1].Y = -72;
			mdTempMeterModel[i].Point[2].X = -31;
			mdTempMeterModel[i].Point[2].Y = 212;
			mdTempMeterModel[i].Point[3].X = 32;
			mdTempMeterModel[i].Point[3].Y = 212;
			
			tmTempMeterMap[i].Point[0].X = PosX1;
			tmTempMeterMap[i].Point[0].Y = PosY1;
			tmTempMeterMap[i].Point[1].X = PosX2;
			tmTempMeterMap[i].Point[1].Y = PosY1;
			tmTempMeterMap[i].Point[2].X = PosX1;
			tmTempMeterMap[i].Point[2].Y = PosY2;
			tmTempMeterMap[i].Point[3].X = PosX2;
			tmTempMeterMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/TempMeter.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'TempMeter.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture TempMeter.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tTempMeterTexture);
		glBindTexture(GL_TEXTURE_2D, tTempMeterTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
				
		iTempMeterInit = 1;
    }
	
	if (iTempMeterInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}    	
	return 0;
}

int RenderWidgetSensorTempMeter(WIDGET_INFO* wiWidget)
{
	if (!iTempMeterInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLoadIdentity();
	glVertexPointer(2, GL_FLOAT, 0, mdTempMeterModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmTempMeterMap);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tTempMeterTexture);
	
	glScalef(fScale,fScale,0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdTempMeterModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmTempMeterMap);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tTempMeterTexture);
		glScalef(fScale,fScale,0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(309, 308, 0.0f);
	
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -40) val = -40;
	if (val > 40) val = 40;
	val *= 3;	
	glRotatef(-val , 0.0f, 0.0f, 1.0f);	
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int ReleaseWidgetSensorTempMeter(WIDGET_INFO* wiWidget)
{
	if (iTempMeterInit) 
	{
		glDeleteTextures(1, &tTempMeterTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorWhiteTacho(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 468;
	GLfloat iWidgetHeight = 468;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iWhiteTachoInit)
	{		
		GLfloat iWidth = 576;
		GLfloat iHeight = 512;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdWhiteTachoModel[0].Point[0].X = 0;
		mdWhiteTachoModel[0].Point[0].Y = 0;
		mdWhiteTachoModel[0].Point[1].X = iWidgetWidth;
		mdWhiteTachoModel[0].Point[1].Y = 0;
		mdWhiteTachoModel[0].Point[2].X = 0;
		mdWhiteTachoModel[0].Point[2].Y = iWidgetHeight;
		mdWhiteTachoModel[0].Point[3].X = iWidgetWidth;
		mdWhiteTachoModel[0].Point[3].Y = iWidgetHeight;
		
		tmWhiteTachoMap[0].Point[0].X = 0.0f;
		tmWhiteTachoMap[0].Point[0].Y = fHeightBase;
		tmWhiteTachoMap[0].Point[1].X = fWidthBase;
		tmWhiteTachoMap[0].Point[1].Y = fHeightBase;
		tmWhiteTachoMap[0].Point[2].X = 0.0f;
		tmWhiteTachoMap[0].Point[2].Y = 0.0f;
		tmWhiteTachoMap[0].Point[3].X = fWidthBase;
		tmWhiteTachoMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 480/iWidth;
		GLfloat fWidthStep = 96/iWidth;
		GLfloat fHeightStep = 230/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdWhiteTachoModel[i].Point[0].X = -47;
			mdWhiteTachoModel[i].Point[0].Y = -43;
			mdWhiteTachoModel[i].Point[1].X = 48;
			mdWhiteTachoModel[i].Point[1].Y = -43;
			mdWhiteTachoModel[i].Point[2].X = -47;
			mdWhiteTachoModel[i].Point[2].Y = 187;
			mdWhiteTachoModel[i].Point[3].X = 48;
			mdWhiteTachoModel[i].Point[3].Y = 187;
			
			tmWhiteTachoMap[i].Point[0].X = PosX1;
			tmWhiteTachoMap[i].Point[0].Y = PosY1;
			tmWhiteTachoMap[i].Point[1].X = PosX2;
			tmWhiteTachoMap[i].Point[1].Y = PosY1;
			tmWhiteTachoMap[i].Point[2].X = PosX1;
			tmWhiteTachoMap[i].Point[2].Y = PosY2;
			tmWhiteTachoMap[i].Point[3].X = PosX2;
			tmWhiteTachoMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/WhiteTacho.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'WhiteTacho.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture WhiteTacho.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tWhiteTachoTexture);
		glBindTexture(GL_TEXTURE_2D, tWhiteTachoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iWhiteTachoInit = 1;
	}
	
	if (iWhiteTachoInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorWhiteTacho(WIDGET_INFO* wiWidget)
{	
	if (!iWhiteTachoInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdWhiteTachoModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmWhiteTachoMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tWhiteTachoTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdWhiteTachoModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmWhiteTachoMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tWhiteTachoTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(237, 232, 0.0f);	
	
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -10) val = -10;
	if (val > 115) val = 115;
	val *= 2.38f;	
	
	glRotatef(119 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}


int ReleaseWidgetSensorWhiteTacho(WIDGET_INFO* wiWidget)
{
	if (iWhiteTachoInit)
	{
		glDeleteTextures(1, &tWhiteTachoTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorBlackTacho(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 468;
	GLfloat iWidgetHeight = 468;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 576;
		GLfloat iHeight = 512;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdBlackTachoModel[0].Point[0].X = 0;
		mdBlackTachoModel[0].Point[0].Y = 0;
		mdBlackTachoModel[0].Point[1].X = iWidgetWidth;
		mdBlackTachoModel[0].Point[1].Y = 0;
		mdBlackTachoModel[0].Point[2].X = 0;
		mdBlackTachoModel[0].Point[2].Y = iWidgetHeight;
		mdBlackTachoModel[0].Point[3].X = iWidgetWidth;
		mdBlackTachoModel[0].Point[3].Y = iWidgetHeight;
		
		tmBlackTachoMap[0].Point[0].X = 0.0f;
		tmBlackTachoMap[0].Point[0].Y = fHeightBase;
		tmBlackTachoMap[0].Point[1].X = fWidthBase;
		tmBlackTachoMap[0].Point[1].Y = fHeightBase;
		tmBlackTachoMap[0].Point[2].X = 0.0f;
		tmBlackTachoMap[0].Point[2].Y = 0.0f;
		tmBlackTachoMap[0].Point[3].X = fWidthBase;
		tmBlackTachoMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 480/iWidth;
		GLfloat fWidthStep = 96/iWidth;
		GLfloat fHeightStep = 230/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdBlackTachoModel[i].Point[0].X = -47;
			mdBlackTachoModel[i].Point[0].Y = -43;
			mdBlackTachoModel[i].Point[1].X = 48;
			mdBlackTachoModel[i].Point[1].Y = -43;
			mdBlackTachoModel[i].Point[2].X = -47;
			mdBlackTachoModel[i].Point[2].Y = 187;
			mdBlackTachoModel[i].Point[3].X = 48;
			mdBlackTachoModel[i].Point[3].Y = 187;
			
			tmBlackTachoMap[i].Point[0].X = PosX1;
			tmBlackTachoMap[i].Point[0].Y = PosY1;
			tmBlackTachoMap[i].Point[1].X = PosX2;
			tmBlackTachoMap[i].Point[1].Y = PosY1;
			tmBlackTachoMap[i].Point[2].X = PosX1;
			tmBlackTachoMap[i].Point[2].Y = PosY2;
			tmBlackTachoMap[i].Point[3].X = PosX2;
			tmBlackTachoMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/BlackTacho.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'BlackTacho.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture BlackTacho.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tBlackTachoTexture);
		glBindTexture(GL_TEXTURE_2D, tBlackTachoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iBlackTachoInit = 1;
	}
	
	if (iBlackTachoInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorBlackTacho(WIDGET_INFO* wiWidget)
{	
	if (!iBlackTachoInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdBlackTachoModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmBlackTachoMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tBlackTachoTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdBlackTachoModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmBlackTachoMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tBlackTachoTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(237, 232, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -10) val = -10;
	if (val > 115) val = 115;
	val *= 2.38f;	
	
	glRotatef(119 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}


int ReleaseWidgetSensorBlackTacho(WIDGET_INFO* wiWidget)
{
	if (iBlackTachoInit)
	{
		glDeleteTextures(1, &tBlackTachoTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorCircleTacho(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 648;
	GLfloat iWidgetHeight = 648;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 768;
		GLfloat iHeight = 704;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdCircleTachoModel[0].Point[0].X = 0;
		mdCircleTachoModel[0].Point[0].Y = 0;
		mdCircleTachoModel[0].Point[1].X = iWidgetWidth;
		mdCircleTachoModel[0].Point[1].Y = 0;
		mdCircleTachoModel[0].Point[2].X = 0;
		mdCircleTachoModel[0].Point[2].Y = iWidgetHeight;
		mdCircleTachoModel[0].Point[3].X = iWidgetWidth;
		mdCircleTachoModel[0].Point[3].Y = iWidgetHeight;
		
		tmCircleTachoMap[0].Point[0].X = 0.0f;
		tmCircleTachoMap[0].Point[0].Y = fHeightBase;
		tmCircleTachoMap[0].Point[1].X = fWidthBase;
		tmCircleTachoMap[0].Point[1].Y = fHeightBase;
		tmCircleTachoMap[0].Point[2].X = 0.0f;
		tmCircleTachoMap[0].Point[2].Y = 0.0f;
		tmCircleTachoMap[0].Point[3].X = fWidthBase;
		tmCircleTachoMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 704/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 237/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdCircleTachoModel[i].Point[0].X = -31;
			mdCircleTachoModel[i].Point[0].Y = 0;
			mdCircleTachoModel[i].Point[1].X = 32;
			mdCircleTachoModel[i].Point[1].Y = 0;
			mdCircleTachoModel[i].Point[2].X = -31;
			mdCircleTachoModel[i].Point[2].Y = 237;
			mdCircleTachoModel[i].Point[3].X = 32;
			mdCircleTachoModel[i].Point[3].Y = 237;
			
			tmCircleTachoMap[i].Point[0].X = PosX1;
			tmCircleTachoMap[i].Point[0].Y = PosY1;
			tmCircleTachoMap[i].Point[1].X = PosX2;
			tmCircleTachoMap[i].Point[1].Y = PosY1;
			tmCircleTachoMap[i].Point[2].X = PosX1;
			tmCircleTachoMap[i].Point[2].Y = PosY2;
			tmCircleTachoMap[i].Point[3].X = PosX2;
			tmCircleTachoMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/CircleTacho.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'CircleTacho.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture CircleTacho.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tCircleTachoTexture);
		glBindTexture(GL_TEXTURE_2D, tCircleTachoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iCircleTachoInit = 1;
	}
	
	if (iCircleTachoInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorCircleTacho(WIDGET_INFO* wiWidget)
{	
	if (!iCircleTachoInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdCircleTachoModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmCircleTachoMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tCircleTachoTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdCircleTachoModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmCircleTachoMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tCircleTachoTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(324, 324, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -10) val = -10;
	if (val > 115) val = 115;
	val *= 2.4f;	
	glRotatef(90 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorCircleTacho(WIDGET_INFO* wiWidget)
{
	if (iCircleTachoInit)
	{
		glDeleteTextures(1, &tCircleTachoTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorGreenTacho(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 638;
	GLfloat iWidgetHeight = 638;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 704;
		GLfloat iHeight = 640;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdGreenTachoModel[0].Point[0].X = 0;
		mdGreenTachoModel[0].Point[0].Y = 0;
		mdGreenTachoModel[0].Point[1].X = iWidgetWidth;
		mdGreenTachoModel[0].Point[1].Y = 0;
		mdGreenTachoModel[0].Point[2].X = 0;
		mdGreenTachoModel[0].Point[2].Y = iWidgetHeight;
		mdGreenTachoModel[0].Point[3].X = iWidgetWidth;
		mdGreenTachoModel[0].Point[3].Y = iWidgetHeight;
		
		tmGreenTachoMap[0].Point[0].X = 0.0f;
		tmGreenTachoMap[0].Point[0].Y = fHeightBase;
		tmGreenTachoMap[0].Point[1].X = fWidthBase;
		tmGreenTachoMap[0].Point[1].Y = fHeightBase;
		tmGreenTachoMap[0].Point[2].X = 0.0f;
		tmGreenTachoMap[0].Point[2].Y = 0.0f;
		tmGreenTachoMap[0].Point[3].X = fWidthBase;
		tmGreenTachoMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 640/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 270/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdGreenTachoModel[i].Point[0].X = -31;
			mdGreenTachoModel[i].Point[0].Y = -12;
			mdGreenTachoModel[i].Point[1].X = 32;
			mdGreenTachoModel[i].Point[1].Y = -12;
			mdGreenTachoModel[i].Point[2].X = -31;
			mdGreenTachoModel[i].Point[2].Y = 258;
			mdGreenTachoModel[i].Point[3].X = 32;
			mdGreenTachoModel[i].Point[3].Y = 258;
			
			tmGreenTachoMap[i].Point[0].X = PosX1;
			tmGreenTachoMap[i].Point[0].Y = PosY1;
			tmGreenTachoMap[i].Point[1].X = PosX2;
			tmGreenTachoMap[i].Point[1].Y = PosY1;
			tmGreenTachoMap[i].Point[2].X = PosX1;
			tmGreenTachoMap[i].Point[2].Y = PosY2;
			tmGreenTachoMap[i].Point[3].X = PosX2;
			tmGreenTachoMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/GreenTacho.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'GreenTacho.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture GreenTacho.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tGreenTachoTexture);
		glBindTexture(GL_TEXTURE_2D, tGreenTachoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iGreenTachoInit = 1;
	}
	
	if (iGreenTachoInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorGreenTacho(WIDGET_INFO* wiWidget)
{	
	if (!iGreenTachoInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdGreenTachoModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmGreenTachoMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tGreenTachoTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdGreenTachoModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmGreenTachoMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tGreenTachoTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(318, 318, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -10) val = -10;
	if (val > 115) val = 115;
	val *= 2.89f;
	
	glRotatef(145 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorGreenTacho(WIDGET_INFO* wiWidget)
{
	if (iGreenTachoInit)
	{
		glDeleteTextures(1, &tGreenTachoTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorOffTacho(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 601;
	GLfloat iWidgetHeight = 601;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 704;
		GLfloat iHeight = 640;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdOffTachoModel[0].Point[0].X = 0;
		mdOffTachoModel[0].Point[0].Y = 0;
		mdOffTachoModel[0].Point[1].X = iWidgetWidth;
		mdOffTachoModel[0].Point[1].Y = 0;
		mdOffTachoModel[0].Point[2].X = 0;
		mdOffTachoModel[0].Point[2].Y = iWidgetHeight;
		mdOffTachoModel[0].Point[3].X = iWidgetWidth;
		mdOffTachoModel[0].Point[3].Y = iWidgetHeight;
		
		tmOffTachoMap[0].Point[0].X = 0.0f;
		tmOffTachoMap[0].Point[0].Y = fHeightBase;
		tmOffTachoMap[0].Point[1].X = fWidthBase;
		tmOffTachoMap[0].Point[1].Y = fHeightBase;
		tmOffTachoMap[0].Point[2].X = 0.0f;
		tmOffTachoMap[0].Point[2].Y = 0.0f;
		tmOffTachoMap[0].Point[3].X = fWidthBase;
		tmOffTachoMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 640/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 316/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdOffTachoModel[i].Point[0].X = -31;
			mdOffTachoModel[i].Point[0].Y = -82;
			mdOffTachoModel[i].Point[1].X = 32;
			mdOffTachoModel[i].Point[1].Y = -82;
			mdOffTachoModel[i].Point[2].X = -31;
			mdOffTachoModel[i].Point[2].Y = 234;
			mdOffTachoModel[i].Point[3].X = 32;
			mdOffTachoModel[i].Point[3].Y = 234;
			
			tmOffTachoMap[i].Point[0].X = PosX1;
			tmOffTachoMap[i].Point[0].Y = PosY1;
			tmOffTachoMap[i].Point[1].X = PosX2;
			tmOffTachoMap[i].Point[1].Y = PosY1;
			tmOffTachoMap[i].Point[2].X = PosX1;
			tmOffTachoMap[i].Point[2].Y = PosY2;
			tmOffTachoMap[i].Point[3].X = PosX2;
			tmOffTachoMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/OffTacho.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'OffTacho.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture OffTacho.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tOffTachoTexture);
		glBindTexture(GL_TEXTURE_2D, tOffTachoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iOffTachoInit = 1;
	}
	
	if (iOffTachoInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorOffTacho(WIDGET_INFO* wiWidget)
{	
	if (!iOffTachoInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdOffTachoModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmOffTachoMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tOffTachoTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdOffTachoModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmOffTachoMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tOffTachoTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(301, 301, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val <= 0) val = -15;
	if (val > 115) val = 115;
	val *= 2.4f;
	
	glRotatef(90 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorOffTacho(WIDGET_INFO* wiWidget)
{
	if (iOffTachoInit)
	{
		glDeleteTextures(1, &tOffTachoTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorDarkMeter(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 542;
	GLfloat iWidgetHeight = 297;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 640;
		GLfloat iHeight = 320;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdDarkMeterModel[0].Point[0].X = 0;
		mdDarkMeterModel[0].Point[0].Y = 0;
		mdDarkMeterModel[0].Point[1].X = iWidgetWidth;
		mdDarkMeterModel[0].Point[1].Y = 0;
		mdDarkMeterModel[0].Point[2].X = 0;
		mdDarkMeterModel[0].Point[2].Y = iWidgetHeight;
		mdDarkMeterModel[0].Point[3].X = iWidgetWidth;
		mdDarkMeterModel[0].Point[3].Y = iWidgetHeight;
		
		tmDarkMeterMap[0].Point[0].X = 0.0f;
		tmDarkMeterMap[0].Point[0].Y = fHeightBase;
		tmDarkMeterMap[0].Point[1].X = fWidthBase;
		tmDarkMeterMap[0].Point[1].Y = fHeightBase;
		tmDarkMeterMap[0].Point[2].X = 0.0f;
		tmDarkMeterMap[0].Point[2].Y = 0.0f;
		tmDarkMeterMap[0].Point[3].X = fWidthBase;
		tmDarkMeterMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 576/iWidth;
		GLfloat fWidthStep = 64/iWidth;
		GLfloat fHeightStep = 260/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdDarkMeterModel[i].Point[0].X = -31;
			mdDarkMeterModel[i].Point[0].Y = -50;
			mdDarkMeterModel[i].Point[1].X = 32;
			mdDarkMeterModel[i].Point[1].Y = -50;
			mdDarkMeterModel[i].Point[2].X = -31;
			mdDarkMeterModel[i].Point[2].Y = 210;
			mdDarkMeterModel[i].Point[3].X = 32;
			mdDarkMeterModel[i].Point[3].Y = 210;
			
			tmDarkMeterMap[i].Point[0].X = PosX1;
			tmDarkMeterMap[i].Point[0].Y = PosY1;
			tmDarkMeterMap[i].Point[1].X = PosX2;
			tmDarkMeterMap[i].Point[1].Y = PosY1;
			tmDarkMeterMap[i].Point[2].X = PosX1;
			tmDarkMeterMap[i].Point[2].Y = PosY2;
			tmDarkMeterMap[i].Point[3].X = PosX2;
			tmDarkMeterMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/DarkMeter.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'DarkMeter.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture DarkMeter.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tDarkMeterTexture);
		glBindTexture(GL_TEXTURE_2D, tDarkMeterTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iDarkMeterInit = 1;
	}
	
	if (iDarkMeterInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorDarkMeter(WIDGET_INFO* wiWidget)
{	
	if (!iDarkMeterInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdDarkMeterModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmDarkMeterMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tDarkMeterTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdDarkMeterModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmDarkMeterMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tDarkMeterTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(265, 29, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val <= 0) val = -15;
	if (val > 115) val = 115;
	val *= 1.8f;
	
	glRotatef(90 - val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorDarkMeter(WIDGET_INFO* wiWidget)
{
	if (iDarkMeterInit)
	{
		glDeleteTextures(1, &tDarkMeterTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorBlackRegulator(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 808;
	GLfloat iWidgetHeight = 808;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 1408;
		GLfloat iHeight = 832;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdBlackRegulatorModel[0].Point[0].X = 0;
		mdBlackRegulatorModel[0].Point[0].Y = 0;
		mdBlackRegulatorModel[0].Point[1].X = iWidgetWidth;
		mdBlackRegulatorModel[0].Point[1].Y = 0;
		mdBlackRegulatorModel[0].Point[2].X = 0;
		mdBlackRegulatorModel[0].Point[2].Y = iWidgetHeight;
		mdBlackRegulatorModel[0].Point[3].X = iWidgetWidth;
		mdBlackRegulatorModel[0].Point[3].Y = iWidgetHeight;
		
		tmBlackRegulatorMap[0].Point[0].X = 0.0f;
		tmBlackRegulatorMap[0].Point[0].Y = fHeightBase;
		tmBlackRegulatorMap[0].Point[1].X = fWidthBase;
		tmBlackRegulatorMap[0].Point[1].Y = fHeightBase;
		tmBlackRegulatorMap[0].Point[2].X = 0.0f;
		tmBlackRegulatorMap[0].Point[2].Y = 0.0f;
		tmBlackRegulatorMap[0].Point[3].X = fWidthBase;
		tmBlackRegulatorMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 828/iWidth;
		GLfloat fWidthStep = 580/iWidth;
		GLfloat fHeightStep = 580/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdBlackRegulatorModel[i].Point[0].X = -264;
			mdBlackRegulatorModel[i].Point[0].Y = -264;
			mdBlackRegulatorModel[i].Point[1].X = 265;
			mdBlackRegulatorModel[i].Point[1].Y = -264;
			mdBlackRegulatorModel[i].Point[2].X = -264;
			mdBlackRegulatorModel[i].Point[2].Y = 265;
			mdBlackRegulatorModel[i].Point[3].X = 265;
			mdBlackRegulatorModel[i].Point[3].Y = 265;
			
			tmBlackRegulatorMap[i].Point[0].X = PosX1;
			tmBlackRegulatorMap[i].Point[0].Y = PosY1;
			tmBlackRegulatorMap[i].Point[1].X = PosX2;
			tmBlackRegulatorMap[i].Point[1].Y = PosY1;
			tmBlackRegulatorMap[i].Point[2].X = PosX1;
			tmBlackRegulatorMap[i].Point[2].Y = PosY2;
			tmBlackRegulatorMap[i].Point[3].X = PosX2;
			tmBlackRegulatorMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/BlackRegulator.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'BlackRegulator.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture BlackRegulator.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tBlackRegulatorTexture);
		glBindTexture(GL_TEXTURE_2D, tBlackRegulatorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iBlackRegulatorInit = 1;
	}
	
	if (iBlackRegulatorInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorBlackRegulator(WIDGET_INFO* wiWidget)
{	
	if (!iBlackRegulatorInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdBlackRegulatorModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmBlackRegulatorMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tBlackRegulatorTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdBlackRegulatorModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmBlackRegulatorMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tBlackRegulatorTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(404, 404, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -115) val = -115;
	if (val > 115) val = 115;
	val *= -0.97f;
	
	glRotatef(val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorBlackRegulator(WIDGET_INFO* wiWidget)
{
	if (iBlackRegulatorInit)
	{
		glDeleteTextures(1, &tBlackRegulatorTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorSilverRegulator(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 947;
	GLfloat iWidgetHeight = 947;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 1664;
		GLfloat iHeight = 960;
		GLfloat fWidthBase = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		
		int i;
		
		mdSilverRegulatorModel[0].Point[0].X = 0;
		mdSilverRegulatorModel[0].Point[0].Y = 0;
		mdSilverRegulatorModel[0].Point[1].X = iWidgetWidth;
		mdSilverRegulatorModel[0].Point[1].Y = 0;
		mdSilverRegulatorModel[0].Point[2].X = 0;
		mdSilverRegulatorModel[0].Point[2].Y = iWidgetHeight;
		mdSilverRegulatorModel[0].Point[3].X = iWidgetWidth;
		mdSilverRegulatorModel[0].Point[3].Y = iWidgetHeight;
		
		tmSilverRegulatorMap[0].Point[0].X = 0.0f;
		tmSilverRegulatorMap[0].Point[0].Y = fHeightBase;
		tmSilverRegulatorMap[0].Point[1].X = fWidthBase;
		tmSilverRegulatorMap[0].Point[1].Y = fHeightBase;
		tmSilverRegulatorMap[0].Point[2].X = 0.0f;
		tmSilverRegulatorMap[0].Point[2].Y = 0.0f;
		tmSilverRegulatorMap[0].Point[3].X = fWidthBase;
		tmSilverRegulatorMap[0].Point[3].Y = 0.0f;
		
		fWidthBase = 990/iWidth;
		GLfloat fWidthStep = 674/iWidth;
		GLfloat fHeightStep = 674/iHeight;
		GLfloat PosX1 = fWidthBase;
		GLfloat PosY1 = fHeightStep;
		GLfloat PosX2 = fWidthBase + fWidthStep;
		GLfloat PosY2 = 0;
		
		
		for (i = 1; i < 2; i++) 
		{
			mdSilverRegulatorModel[i].Point[0].X = -336;
			mdSilverRegulatorModel[i].Point[0].Y = -336;
			mdSilverRegulatorModel[i].Point[1].X = 337;
			mdSilverRegulatorModel[i].Point[1].Y = -336;
			mdSilverRegulatorModel[i].Point[2].X = -336;
			mdSilverRegulatorModel[i].Point[2].Y = 337;
			mdSilverRegulatorModel[i].Point[3].X = 337;
			mdSilverRegulatorModel[i].Point[3].Y = 337;
			
			tmSilverRegulatorMap[i].Point[0].X = PosX1;
			tmSilverRegulatorMap[i].Point[0].Y = PosY1;
			tmSilverRegulatorMap[i].Point[1].X = PosX2;
			tmSilverRegulatorMap[i].Point[1].Y = PosY1;
			tmSilverRegulatorMap[i].Point[2].X = PosX1;
			tmSilverRegulatorMap[i].Point[2].Y = PosY2;
			tmSilverRegulatorMap[i].Point[3].X = PosX2;
			tmSilverRegulatorMap[i].Point[3].Y = PosY2;
			
			PosX1 += fWidthStep;
			PosX2 += fWidthStep;
		}
		
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/SilverRegulator.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'SilverRegulator.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture SilverRegulator.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tSilverRegulatorTexture);
		glBindTexture(GL_TEXTURE_2D, tSilverRegulatorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iSilverRegulatorInit = 1;
	}
	
	if (iSilverRegulatorInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorSilverRegulator(WIDGET_INFO* wiWidget)
{	
	if (!iSilverRegulatorInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdSilverRegulatorModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmSilverRegulatorMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tSilverRegulatorTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdSilverRegulatorModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmSilverRegulatorMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tSilverRegulatorTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(473, 473, 0.0f);	
		
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;

	if (val < -115) val = -115;
	if (val > 115) val = 115;
	val *= -0.97f;
	
	glRotatef(val , 0.0f, 0.0f, 1.0f);	
	
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorSilverRegulator(WIDGET_INFO* wiWidget)
{
	if (iSilverRegulatorInit)
	{
		glDeleteTextures(1, &tSilverRegulatorTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorDarkTermometer(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 216;
	GLfloat iWidgetHeight = 719;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 512;
		GLfloat iHeight = 768;
		GLfloat fWidthBase1 = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		GLfloat fWidthBase2 = 220/iWidth;
		GLfloat fWidthBase3 = 463/iWidth;
		GLfloat fHeightBase2 = 1/iHeight;
		GLfloat fWidthBase4 = 32/iWidth;
		
		mdDarkTermometerModel[0].Point[0].X = 0;
		mdDarkTermometerModel[0].Point[0].Y = 0;
		mdDarkTermometerModel[0].Point[1].X = iWidgetWidth;
		mdDarkTermometerModel[0].Point[1].Y = 0;
		mdDarkTermometerModel[0].Point[2].X = 0;
		mdDarkTermometerModel[0].Point[2].Y = iWidgetHeight;
		mdDarkTermometerModel[0].Point[3].X = iWidgetWidth;
		mdDarkTermometerModel[0].Point[3].Y = iWidgetHeight;
		memcpy(&mdDarkTermometerModel[1], &mdDarkTermometerModel[0], sizeof(GLmodel2D));
		
		tmDarkTermometerMap[0].Point[0].X = 0.0f;
		tmDarkTermometerMap[0].Point[0].Y = fHeightBase;
		tmDarkTermometerMap[0].Point[1].X = fWidthBase1;
		tmDarkTermometerMap[0].Point[1].Y = fHeightBase;
		tmDarkTermometerMap[0].Point[2].X = 0.0f;
		tmDarkTermometerMap[0].Point[2].Y = 0.0f;
		tmDarkTermometerMap[0].Point[3].X = fWidthBase1;
		tmDarkTermometerMap[0].Point[3].Y = 0.0f;
		
		tmDarkTermometerMap[1].Point[0].X = fWidthBase2;
		tmDarkTermometerMap[1].Point[0].Y = fHeightBase;
		tmDarkTermometerMap[1].Point[1].X = fWidthBase2 + fWidthBase1;
		tmDarkTermometerMap[1].Point[1].Y = fHeightBase;
		tmDarkTermometerMap[1].Point[2].X = fWidthBase2;
		tmDarkTermometerMap[1].Point[2].Y = 0.0f;
		tmDarkTermometerMap[1].Point[3].X = fWidthBase2 + fWidthBase1;
		tmDarkTermometerMap[1].Point[3].Y = 0.0f;
				
		mdDarkTermometerModel[2].Point[0].X = 0;
		mdDarkTermometerModel[2].Point[0].Y = -iWidgetHeight;
		mdDarkTermometerModel[2].Point[1].X = 32;
		mdDarkTermometerModel[2].Point[1].Y = -iWidgetHeight;
		mdDarkTermometerModel[2].Point[2].X = 0;
		mdDarkTermometerModel[2].Point[2].Y = 0.0f;
		mdDarkTermometerModel[2].Point[3].X = 32;
		mdDarkTermometerModel[2].Point[3].Y = 0.0f;
			
		tmDarkTermometerMap[2].Point[0].X = fWidthBase3;
		tmDarkTermometerMap[2].Point[0].Y = 0.9f;
		tmDarkTermometerMap[2].Point[1].X = fWidthBase3 + fWidthBase4;
		tmDarkTermometerMap[2].Point[1].Y = 0.9f;
		tmDarkTermometerMap[2].Point[2].X = fWidthBase3;
		tmDarkTermometerMap[2].Point[2].Y = fHeightBase2;
		tmDarkTermometerMap[2].Point[3].X = fWidthBase3 + fWidthBase4;
		tmDarkTermometerMap[2].Point[3].Y = fHeightBase2;
			
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/DarkTermometer.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'DarkTermometer.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture DarkTermometer.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tDarkTermometerTexture);
		glBindTexture(GL_TEXTURE_2D, tDarkTermometerTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iDarkTermometerInit = 1;
	}
	
	if (iDarkTermometerInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorDarkTermometer(WIDGET_INFO* wiWidget)
{	
	if (!iDarkTermometerInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdDarkTermometerModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmDarkTermometerMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tDarkTermometerTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	
	GLfloat fStep = (0.706f - 0.491f)/25;	
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;
	if (val < -28) val = -28;
	if (val > 60) val = 60;
	val *= -fStep;
	
	if (val < 0.0f) glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); else glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdDarkTermometerModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmDarkTermometerMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tDarkTermometerTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(90, 665, 0.0f);
	glScalef(1.0f, 0.491f + val, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorDarkTermometer(WIDGET_INFO* wiWidget)
{
	if (iDarkTermometerInit)
	{
		glDeleteTextures(1, &tDarkTermometerTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidgetSensorWhiteTermometer(WIDGET_INFO* wiWidget)
{
	GLfloat iWidgetWidth = 296;
	GLfloat iWidgetHeight = 936;
	wiWidget->Width = iWidgetWidth;
	wiWidget->Height = iWidgetHeight;
	wiWidget->SizeX = iWidgetWidth;
	wiWidget->SizeY = iWidgetHeight;
	if (iWidgetWidth > iWidgetHeight)
		wiWidget->DefaultScale = wiWidget->IconDefaultWidth / iWidgetWidth;
		else
		wiWidget->DefaultScale = wiWidget->IconDefaultHeight / iWidgetHeight;	
		
	if (!iBlackTachoInit)
	{		
		GLfloat iWidth = 704;
		GLfloat iHeight = 960;
		GLfloat fWidthBase1 = iWidgetWidth/iWidth;
		GLfloat fHeightBase = iWidgetHeight/iHeight;
		GLfloat fWidthBase2 = 300/iWidth;
		GLfloat fWidthBase3 = 666/iWidth;
		GLfloat fHeightBase2 = 1/iHeight;
		GLfloat fWidthBase4 = 12/iWidth;
		
		mdWhiteTermometerModel[0].Point[0].X = 0;
		mdWhiteTermometerModel[0].Point[0].Y = 0;
		mdWhiteTermometerModel[0].Point[1].X = iWidgetWidth;
		mdWhiteTermometerModel[0].Point[1].Y = 0;
		mdWhiteTermometerModel[0].Point[2].X = 0;
		mdWhiteTermometerModel[0].Point[2].Y = iWidgetHeight;
		mdWhiteTermometerModel[0].Point[3].X = iWidgetWidth;
		mdWhiteTermometerModel[0].Point[3].Y = iWidgetHeight;
		memcpy(&mdWhiteTermometerModel[1], &mdWhiteTermometerModel[0], sizeof(GLmodel2D));
		
		tmWhiteTermometerMap[0].Point[0].X = 0.0f;
		tmWhiteTermometerMap[0].Point[0].Y = fHeightBase;
		tmWhiteTermometerMap[0].Point[1].X = fWidthBase1;
		tmWhiteTermometerMap[0].Point[1].Y = fHeightBase;
		tmWhiteTermometerMap[0].Point[2].X = 0.0f;
		tmWhiteTermometerMap[0].Point[2].Y = 0.0f;
		tmWhiteTermometerMap[0].Point[3].X = fWidthBase1;
		tmWhiteTermometerMap[0].Point[3].Y = 0.0f;
		
		tmWhiteTermometerMap[1].Point[0].X = fWidthBase2;
		tmWhiteTermometerMap[1].Point[0].Y = fHeightBase;
		tmWhiteTermometerMap[1].Point[1].X = fWidthBase2 + fWidthBase1;
		tmWhiteTermometerMap[1].Point[1].Y = fHeightBase;
		tmWhiteTermometerMap[1].Point[2].X = fWidthBase2;
		tmWhiteTermometerMap[1].Point[2].Y = 0.0f;
		tmWhiteTermometerMap[1].Point[3].X = fWidthBase2 + fWidthBase1;
		tmWhiteTermometerMap[1].Point[3].Y = 0.0f;
				
		mdWhiteTermometerModel[2].Point[0].X = 0;
		mdWhiteTermometerModel[2].Point[0].Y = -iWidgetHeight;
		mdWhiteTermometerModel[2].Point[1].X = 13;
		mdWhiteTermometerModel[2].Point[1].Y = -iWidgetHeight;
		mdWhiteTermometerModel[2].Point[2].X = 0;
		mdWhiteTermometerModel[2].Point[2].Y = 0.0f;
		mdWhiteTermometerModel[2].Point[3].X = 13;
		mdWhiteTermometerModel[2].Point[3].Y = 0.0f;
			
		tmWhiteTermometerMap[2].Point[0].X = fWidthBase3;
		tmWhiteTermometerMap[2].Point[0].Y = 0.9f;
		tmWhiteTermometerMap[2].Point[1].X = fWidthBase3 + fWidthBase4;
		tmWhiteTermometerMap[2].Point[1].Y = 0.9f;
		tmWhiteTermometerMap[2].Point[2].X = fWidthBase3;
		tmWhiteTermometerMap[2].Point[2].Y = fHeightBase2;
		tmWhiteTermometerMap[2].Point[3].X = fWidthBase3 + fWidthBase4;
		tmWhiteTermometerMap[2].Point[3].Y = fHeightBase2;
			
		char *cBuffer = NULL;
		unsigned int iSize = 0;
		
		int sW = 2048;
		int sH = 2048;	
	
		if (omx_image_to_buffer("Textures/WhiteTermometer.png", (void**)&cBuffer, &iSize, &sW, &sH) != 1)
		{
			dbgprintf(2, "Error omx_image_to_buffer 'WhiteTermometer.png'\n");
			return 0;
		}	
		if ((sW != iWidth) || (sH != iHeight))
		{
			dbgprintf(1, "Wrong size texture WhiteTermometer.png for white clock %i != %i   %i != %i\n", sW, iWidth, sH, iHeight);
			DBG_FREE(cBuffer);
			return 0;
		}
						
		glGenTextures(1, &tWhiteTermometerTexture);
		glBindTexture(GL_TEXTURE_2D, tWhiteTermometerTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sW, sH, 0, GL_RGBA, GL_UNSIGNED_BYTE, cBuffer);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cBuffer);
			
		iWhiteTermometerInit = 1;
	}
	
	if (iWhiteTermometerInit)
	{
		if (wiWidget->Path[0]) return CreateWidgetImage(wiWidget);
		return 1;
	}
	return 0;
}

int RenderWidgetSensorWhiteTermometer(WIDGET_INFO* wiWidget)
{	
	if (!iWhiteTermometerInit) return 0;
	
	GLfloat fScale = wiWidget->Scale * wiWidget->DefaultScale;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glVertexPointer(2, GL_FLOAT, 0, mdWhiteTermometerModel);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tmWhiteTermometerMap); 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, tWhiteTermometerTexture);
	
	glScalef(fScale, fScale, 0.0f);
	glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);
	
	GLfloat fStep = (0.617f - 0.426f)/25;	
	GLfloat val = wiWidget->SensorPrevValue;
	GLfloat diff = (GLfloat)(wiWidget->SensorValue - wiWidget->SensorPrevValue) / 20;
	wiWidget->SensorPrevValue += diff;
	val = wiWidget->SensorPrevValue;
	
	if (val < -53) val = -53;
	if (val > 53) val = 53;
	
	if (val > 0.0f) glDrawArrays(GL_TRIANGLE_STRIP, 4, 4); else glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	val *= -fStep;
	
	if (wiWidget->IconSourceWidth)
	{
		RenderWidgetImage(wiWidget, 0);
		glLoadIdentity();
		
		glVertexPointer(2, GL_FLOAT, 0, mdWhiteTermometerModel);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tmWhiteTermometerMap); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, tWhiteTermometerTexture);
	
		glScalef(fScale, fScale, 0.0f);
		glTranslatef(wiWidget->PosX/fScale, wiWidget->PosY/fScale, 0.0f);	
	}
	
	glTranslatef(142, 875, 0.0f);
	glScalef(1.0f, 0.426f + val, 0.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);	
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	if (wiWidget->Name[0] != 0) RenderGLText(16, wiWidget->PosX, 
												wiWidget->PosY + (wiWidget->SizeY - 42)*fScale, 
												"%s", wiWidget->Name);
	return 1;
}

int ReleaseWidgetSensorWhiteTermometer(WIDGET_INFO* wiWidget)
{
	if (iWhiteTermometerInit)
	{
		glDeleteTextures(1, &tWhiteTermometerTexture);
		if (wiWidget->IconSourceWidth) glDeleteTextures(1, &wiWidget->IconTexture);
	}
	return 1;
}

int CreateWidget(WIDGET_INFO* wiWidget)
{
	memset(wiWidget->SensorValueStr, 0, 64);
	wiWidget->IconDefaultWidth = 150;
	wiWidget->IconDefaultHeight = 150;
	wiWidget->IconSourceWidth = 0;
	wiWidget->IconSourceHeight = 0;
	wiWidget->IconRenderWidth = 0;
	wiWidget->IconRenderHeight = 0;
	wiWidget->SizeX = 0;
	wiWidget->SizeY = 0;
	wiWidget->IconScale = 1.0f;
	wiWidget->DefaultScale = 1.0f;
	
		
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_EL) return CreateWidgetElClock(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_MECH) return CreateWidgetMechClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_WHITE) return CreateWidgetWhiteClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_BROWN) return CreateWidgetBrownClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_QUARTZ) return CreateWidgetQuartzClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_SKYBLUE) return CreateWidgetSkyBlueClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_ARROW) return CreateWidgetArrowClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_IMAGE) return CreateWidgetSensorImage(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TACHOMETER) return CreateWidgetSensorTachoMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TACHO_SCALE) return CreateWidgetSensorIndicator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TEMPMETER) return CreateWidgetSensorTempMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_IMAGE) return CreateWidgetImage(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_WHITETACHO) return CreateWidgetSensorWhiteTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_BLACKTACHO) return CreateWidgetSensorBlackTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_CIRCLETACHO) return CreateWidgetSensorCircleTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_GREENTACHO) return CreateWidgetSensorGreenTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_OFFTACHO) return CreateWidgetSensorOffTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_DARKMETER) return CreateWidgetSensorDarkMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_BLACKREGULATOR) return CreateWidgetSensorBlackRegulator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_SILVERREGULATOR) return CreateWidgetSensorSilverRegulator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_DARKTERMOMETER) return CreateWidgetSensorDarkTermometer(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_WHITETERMOMETER) return CreateWidgetSensorWhiteTermometer(wiWidget);
	return 0;
}

int RenderWidget(WIDGET_INFO* wiWidget)
{
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_EL) return RenderWidgetElClock(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_MECH) return RenderWidgetMechClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_WHITE) return RenderWidgetWhiteClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_BROWN) return RenderWidgetBrownClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_QUARTZ) return RenderWidgetQuartzClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_SKYBLUE) return RenderWidgetSkyBlueClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_ARROW) return RenderWidgetArrowClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_IMAGE) return RenderWidgetSensorImage(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TACHOMETER) return RenderWidgetSensorTachoMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TACHO_SCALE) return RenderWidgetSensorIndicator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TEMPMETER) return RenderWidgetSensorTempMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_IMAGE) return RenderWidgetImage(wiWidget, 1);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_WHITETACHO) return RenderWidgetSensorWhiteTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_BLACKTACHO) return RenderWidgetSensorBlackTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_CIRCLETACHO) return RenderWidgetSensorCircleTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_GREENTACHO) return RenderWidgetSensorGreenTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_OFFTACHO) return RenderWidgetSensorOffTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_DARKMETER) return RenderWidgetSensorDarkMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_BLACKREGULATOR) return RenderWidgetSensorBlackRegulator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_SILVERREGULATOR) return RenderWidgetSensorSilverRegulator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_DARKTERMOMETER) return RenderWidgetSensorDarkTermometer(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_WHITETERMOMETER) return RenderWidgetSensorWhiteTermometer(wiWidget);
	return 0;
}

int ReleaseWidget(WIDGET_INFO* wiWidget)
{
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_EL) return ReleaseWidgetElClock(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_MECH) return ReleaseWidgetMechClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_WHITE) return ReleaseWidgetWhiteClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_BROWN) return ReleaseWidgetBrownClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_QUARTZ) return ReleaseWidgetQuartzClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_SKYBLUE) return ReleaseWidgetSkyBlueClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_CLOCK_ARROW) return ReleaseWidgetArrowClock(wiWidget);	
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_IMAGE) return ReleaseWidgetSensorImage(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TACHOMETER) return ReleaseWidgetSensorTachoMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TACHO_SCALE) return ReleaseWidgetSensorIndicator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_TEMPMETER) return ReleaseWidgetSensorTempMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_IMAGE) return ReleaseWidgetImage(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_WHITETACHO) return ReleaseWidgetSensorWhiteTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_BLACKTACHO) return ReleaseWidgetSensorBlackTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_CIRCLETACHO) return ReleaseWidgetSensorCircleTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_GREENTACHO) return ReleaseWidgetSensorGreenTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_OFFTACHO) return ReleaseWidgetSensorOffTacho(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_DARKMETER) return ReleaseWidgetSensorDarkMeter(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_BLACKREGULATOR) return ReleaseWidgetSensorBlackRegulator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_SILVERREGULATOR) return ReleaseWidgetSensorSilverRegulator(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_DARKTERMOMETER) return ReleaseWidgetSensorDarkTermometer(wiWidget);
	if (wiWidget->Type == WIDGET_TYPE_SENSOR_WHITETERMOMETER) return ReleaseWidgetSensorWhiteTermometer(wiWidget);
	return 0;
}

char* GetWidgetTypeName(int iType)
{
	switch(iType)
	{
		case WIDGET_TYPE_SENSOR_IMAGE: return "Картинка и статус";
		case WIDGET_TYPE_SENSOR_TACHOMETER: return "Тахометр и шкала";
		case WIDGET_TYPE_SENSOR_TACHO_SCALE: return "Тахометр";
		case WIDGET_TYPE_SENSOR_TEMPMETER: return "Круглый термометр";
		case WIDGET_TYPE_IMAGE: return "Картинка";
		case WIDGET_TYPE_WEATHER: return "Погода";
		case WIDGET_TYPE_CLOCK_EL: return "Электронные часы";
		case WIDGET_TYPE_CLOCK_MECH: return "Механические часы";
		case WIDGET_TYPE_CLOCK_WHITE: return "Белые часы";
		case WIDGET_TYPE_CLOCK_BROWN: return "Коричневые часы";
		case WIDGET_TYPE_CLOCK_QUARTZ: return "QUARTZ часы";
		case WIDGET_TYPE_CLOCK_SKYBLUE: return "Голубые часы";
		case WIDGET_TYPE_CLOCK_ARROW: return "Стрелочные часы";
		case WIDGET_TYPE_SENSOR_WHITETACHO: return "Белый тахометр";
		case WIDGET_TYPE_SENSOR_BLACKTACHO: return "Черный тахометр";
		case WIDGET_TYPE_SENSOR_CIRCLETACHO: return "Круглый тахометр";
		case WIDGET_TYPE_SENSOR_GREENTACHO: return "Зеленый тахометр";
		case WIDGET_TYPE_SENSOR_OFFTACHO: return "Тахометр с выключением";
		case WIDGET_TYPE_SENSOR_DARKMETER: return "Полукруглый измеритель";
		case WIDGET_TYPE_SENSOR_BLACKREGULATOR: return "Черный регулятор";
		case WIDGET_TYPE_SENSOR_SILVERREGULATOR: return "Серебряный регулятор";
		case WIDGET_TYPE_SENSOR_DARKTERMOMETER: return "Темный термометер";
		case WIDGET_TYPE_SENSOR_WHITETERMOMETER: return "Белый термометер";
		default: return "Неизвестный";
	}
	return NULL;
}

char* GetWidgetDirectName(int iDirect)
{
	switch(iDirect)
	{
		case WIDGET_DIRECT_BORDER_CW: return "По кругу, по часовой";
		case WIDGET_DIRECT_BORDER_CCW: return "По кругу, против часовой";
		case WIDGET_DIRECT_BORDER_PINGPONG : return "Пинг понг";
		case WIDGET_DIRECT_STATIC : return "Статичный";
		default: return "Неизвестный";
	}
	return NULL;
}

