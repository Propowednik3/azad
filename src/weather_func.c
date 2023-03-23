
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include <pthread.h>

#include "weather_func.h"
#include "text_func.h"
//#include "weather.h"
#include "debug.h"
#include "system.h"
#include "omx_client.h"

#define WEATHER_CLEAR_DAY	0
#define WEATHER_CLOUDS1		1*4
#define WEATHER_CLOUDS2		2*4
#define WEATHER_CLOUDS3		3*4
#define WEATHER_CLOUDS4		4*4
#define WEATHER_CLEAR_NIGHT	5*4
#define WEATHER_SNOW1		6*4
#define WEATHER_SNOW2		7*4
#define WEATHER_SNOW3		8*4
#define WEATHER_SNOW4		9*4
#define WEATHER_NA 			10*4
#define WEATHER_RAIN1		11*4
#define WEATHER_RAIN2		12*4
#define WEATHER_RAIN3		13*4
#define WEATHER_RAIN4		14*4
#define WEATHER_HAIL		15*4
#define WEATHER_WIND1		16*4
#define WEATHER_WIND2		17*4
#define WEATHER_WIND3		18*4
#define WEATHER_WIND4		19*4
#define WEATHER_MIST_DAY	20*4
#define WEATHER_MIST_NIGHT	21*4
#define WEATHER_FOG_DAY		22*4
#define WEATHER_FOG_NIGHT	23*4
#define WEATHER_SLEET		24*4
#define WEATHER_HOT			25*4
#define WEATHER_FROST		26*4
#define WEATHER_BORDER		27*4
#define WEATHER_FRAME		28*4

int 			iWeatherInit = 0;
unsigned int 	iScrSizeX=0;
unsigned int 	iScrSizeY=0;
unsigned int 	iPosX[7];
unsigned int 	iPosY[7];
int 			iDirection[7];
int 			iDirection2[7];
char 			*cTextureWeather[16];
GLuint 			weather_tex[16];
GLfloat 		gWScaleX;
static GLfloat gWeatherPoints[30*4*2];
static GLfloat gWeatherTexturePoints[30*4*2];
static GLfloat gWeatherScrPoints[2*4*2];
static GLfloat gWeatherScrTexturePoints[4*2];
WeatherStruct *WeaterData = NULL;
int HourDataCount 	= 0;
int DayDataCount 	= 0;

WeatherDayStruct *WeaterDayData = NULL;
WeatherRise 	WeaterRiseData;

void GetStr(char *textbuffer, int textbufferLen, char *cText, ...)
{
	memset(textbuffer, 0 , textbufferLen);
	va_list valist;
	va_start(valist, cText);		
	vsprintf(textbuffer, cText, valist);		
	va_end(valist);	
}	

int GetHourFromTime(int iTime)
{
	char buff[32];
	int i;
	GetStr(buff, 32, "%i", iTime);
	i = strlen(buff);
	if (i == 5)
	{
		buff[4] = 0;
		buff[5] = 0;
		return Str2Int(buff+1);
	}
	if (i == 6)
	{
		buff[6] = 0;
		buff[5] = 0;
		return Str2Int(buff+2);
	}
	return 0;
}

void GetDayNumName(char *textbuffer, int textbufferLen, int iNum)
{
	memset(textbuffer, 0 , textbufferLen);
	switch (iNum)
	{
		case 0:
			strcpy(textbuffer, "ВСК");
			break;
		case 1:
			strcpy(textbuffer, "ПОН");
			break;
		case 2:
			strcpy(textbuffer, "ВТР");
			break;
		case 3:
			strcpy(textbuffer, "СРД");
			break;
		case 4:
			strcpy(textbuffer, "ЧТВ");
			break;
		case 5:
			strcpy(textbuffer, "ПТН");
			break;
		case 6:
			strcpy(textbuffer, "СБТ");
			break;
		default:
			break;
	}
}

int GetCharPos(char *textbuffer, int textbufferLen, char cChar, char cDirect)
{
	int ret = -1;
	int n;
	if (cDirect == 0)
	{
		for (n = 0; n < textbufferLen; n++)
			if (textbuffer[n] == cChar) return n;
	}
	else
	{
		for (n = (textbufferLen - 1); n >= 0; n--)
			if (textbuffer[n] == cChar) return n;
	}
	return ret;
}

void UpperText(char *cText)
{
	int n;
	int m = strlen(cText);
	for (n = 0; n != m; n++) if ((cText[n] > 96) && (cText[n] < 123)) cText[n] = cText[n] - 32;
}

void UpperTextLimit(char *cText, int iLen)
{
	int n;
	int m = iLen;
	for (n = 0; n != m; n++) if ((cText[n] > 96) && (cText[n] < 123)) cText[n] = cText[n] - 32;
}

void LowerText(char *cText)
{
	int n;
	int m = strlen(cText);
	for (n = 0; n != m; n++) if ((cText[n] > 64) && (cText[n] < 91)) cText[n] = cText[n] + 32;
}

void LowerTextLimit(char *cText, int iLen)
{
	int n;
	int m = iLen;
	for (n = 0; n != m; n++) if ((cText[n] > 64) && (cText[n] < 91)) cText[n] = cText[n] + 32;
}

int load_weather_file (char *filename, char ** cBuffer, unsigned int *iSizeBuff) 
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

int SearchStr(char *Data, int DataLen, int Pos, char *Str, int *FindedPos)
{
	int StrLen = strlen(Str);
	int Flag1 = 0;
	int n;
	for (n = Pos; n != DataLen; n++)
	{
		if (Str[Flag1] == Data[n]) Flag1++; 
		else
		{
			Flag1 = 0;
			if (Str[Flag1] == Data[n]) Flag1++;
		}
		if (Flag1 == StrLen)
		{
			if (FindedPos != NULL) *FindedPos = (n - StrLen) + 1;
			return 1;
		}
	}
	if (FindedPos != NULL) *FindedPos = 0;
	return 0;
}

int SetObjectWeather(int iPoliSizeX, int iBlocksX, int iPoliSizeY, int iBlocksY)
{
	int n, m;
	GLfloat gX, gY, gX1, gY1, gX2, gY2, gXd, gYd;
	gXd = 1.0f / (iPoliSizeX * iBlocksX);
	gYd = 1.0f / (iPoliSizeY * iBlocksY);
	//gX = gXd * (iPoliSizeX - 1);
	//gY = gYd * (iPoliSizeY - 1);
	gX = gXd * (iPoliSizeX - 1);
	gY = gYd * (iPoliSizeY - 1);
	gX1 = 0.0f;
	gY1 = 0.0f;
	gX2 = 0.0f;
	gY2 = 0.0f;
	memset(gWeatherPoints, 0, sizeof(GLfloat)*30*4*2);
	m = 0;
	for(n = 0; n != 30;n++)
	{
		gWeatherPoints[m + 2] = iPoliSizeX - 1;
		gWeatherPoints[m + 5] = iPoliSizeY - 1;
		gWeatherPoints[m + 6] = iPoliSizeX - 1;
		gWeatherPoints[m + 7] = iPoliSizeY - 1;
		m+=8;
	}
	memset(gWeatherTexturePoints, 0, sizeof(GLfloat)*30*4*2);
	m = 0;
	for(n = 0; n != 30;n++)
	{
		gX2 = gX1 + gX;
		//if (gX1 == 0) gX2 -= gXd;
		gY2 = gY1 + gY;
		gWeatherTexturePoints[m + 0] = gX1;	gWeatherTexturePoints[m + 1] = gY2;
		gWeatherTexturePoints[m + 2] = gX2;	gWeatherTexturePoints[m + 3] = gY2;
		gWeatherTexturePoints[m + 4] = gX1;	gWeatherTexturePoints[m + 5] = gY1;
		gWeatherTexturePoints[m + 6] = gX2;	gWeatherTexturePoints[m + 7] = gY1;
		gX1 = gX2 + gXd;
		if (gX1 >= 1.0f)
		{
			gY1 = gY2 + gYd;
			gX1 = 0.0f;
			if (gY1 >= 1.0f) break;
		}
		m+=8;
	}
	gWeatherPoints[27*8+2] = 5;
	gWeatherPoints[27*8+5] = 200;
	gWeatherPoints[27*8+6] = 5;
	gWeatherPoints[27*8+7] = 200;
	//gWeatherTexturePoints[27*8+0] = 1.0f - (gXd*360);	//gWeatherTexturePoints[28*8+1] = 0.0f;
	gWeatherTexturePoints[27*8+2] = gWeatherTexturePoints[27*8+0] + (gXd*5);	//gWeatherTexturePoints[28*8+3] = 0.0f;
	//gWeatherTexturePoints[27*8+4] = 1.0f - (gXd*360);	//gWeatherTexturePoints[28*8+5] = 1.0f;
	gWeatherTexturePoints[27*8+6] = gWeatherTexturePoints[27*8+0] + (gXd*5);	//gWeatherTexturePoints[28*8+7] = 1.0f;
	gWeatherPoints[28*8+2] = 360;
	gWeatherPoints[28*8+5] = 200;
	gWeatherPoints[28*8+6] = 360;
	gWeatherPoints[28*8+7] = 200;
	gWeatherTexturePoints[28*8+0] = 1.0f - (gXd*360);	//gWeatherTexturePoints[28*8+1] = 0.0f;
	gWeatherTexturePoints[28*8+2] = 1.0f;	//gWeatherTexturePoints[28*8+3] = 0.0f;
	gWeatherTexturePoints[28*8+4] = 1.0f - (gXd*360);	//gWeatherTexturePoints[28*8+5] = 1.0f;
	gWeatherTexturePoints[28*8+6] = 1.0f;	//gWeatherTexturePoints[28*8+7] = 1.0f;
	return 1;
}

int DeInitGLWeather()
{
	if (iWeatherInit != 1) return 0;
	int i;
	for (i = 0; i < 8; i++) glDeleteTextures(1, &weather_tex[i]);
	return 1;
}

int InitGLWeather(unsigned int uScrSizeX, unsigned int uScrSizeY, GLfloat gScaleX)
{
	if (iWeatherInit != 0) return 0;
	
	int sW = 2048;
	int sH = 2048;	
	unsigned int uiSize;
	/*if (!load_weather_file("Textures/weather1200x1200.raw", &cTextureWeather[0], &iSize))
			dbgprintf(1, "Error load texture: Textures/weather1200x1200.raw");*/
	if (omx_image_to_buffer("Textures/Weather.png", (void**)&cTextureWeather[0], &uiSize, &sW, &sH) != 1)
	{
		dbgprintf(2, "Error omx_image_to_buffer 'Weather.png'\n");
		return 0;
	}
	
	if ((sW != 1200) || (sH != 1200))
	{
		dbgprintf(1, "Wrong size texture for widget weather\n");
		DBG_FREE(cTextureWeather[0]);
		return 0;
	}
	
	iScrSizeX = uScrSizeX;
	iScrSizeY = uScrSizeY;
	gWScaleX  = gScaleX;
	iPosX[0] = 0;
	iPosY[0] = 0;
	iPosX[1] = 360 * gScaleX;
	iPosY[1] = 180;
	iPosX[2] = iPosX[1] + ((240-24) * gScaleX);
	iPosY[2] = iPosY[1] + ((180-18) * gScaleX);
	iPosX[3] = iPosX[2] + ((240-48) * gScaleX);
	iPosY[3] = iPosY[2] + ((180-36) * gScaleX);
	iPosX[4] = iPosX[3] + ((240-72) * gScaleX);
	iPosY[4] = iPosY[3] + ((180-54) * gScaleX);
	iPosX[5] = iPosX[4] + ((240-96) * gScaleX);
	iPosY[5] = iPosY[4] + ((180-72) * gScaleX);
	iPosX[6] = iPosX[5] + ((240-120) * gScaleX);
	iPosY[6] = iPosY[5] + ((180-90) * gScaleX);
	iDirection[0] = 3;
	iDirection[1] = 3;
	iDirection[2] = 3;
	iDirection[3] = 3;
	iDirection[4] = 3;
	iDirection[5] = 3;
	iDirection[6] = 3;
	iDirection2[0] = 3;
	iDirection2[1] = 3;
	iDirection2[2] = 3;
	iDirection2[3] = 3;
	iDirection2[4] = 3;
	iDirection2[5] = 3;
	iDirection2[6] = 3;
	
	SetObjectWeather(240, 5, 200, 6);
	
	gWeatherScrPoints[0] = 0;
	gWeatherScrPoints[1] = 0;
	gWeatherScrPoints[2] = 359;
	gWeatherScrPoints[3] = 0;
	gWeatherScrPoints[4] = 0;
	gWeatherScrPoints[5] = 199;
	gWeatherScrPoints[6] = 359;
	gWeatherScrPoints[7] = 199;
	
	gWeatherScrPoints[8] = 0;
	gWeatherScrPoints[9] = 0;
	gWeatherScrPoints[10] = 239;
	gWeatherScrPoints[11] = 0;
	gWeatherScrPoints[12] = 0;
	gWeatherScrPoints[13] = 199;
	gWeatherScrPoints[14] = 239;
	gWeatherScrPoints[15] = 199;
	
	gWeatherScrTexturePoints[0] = 0.0f;	gWeatherScrTexturePoints[1] = 0.0f;
	gWeatherScrTexturePoints[2] = 1.0f;	gWeatherScrTexturePoints[3] = 0.0f;
	gWeatherScrTexturePoints[4] = 0.0f;	gWeatherScrTexturePoints[5] = 1.0f;
	gWeatherScrTexturePoints[6] = 1.0f;	gWeatherScrTexturePoints[7] = 1.0f;
	
	glGenTextures(8, &weather_tex[0]);
	glBindTexture(GL_TEXTURE_2D, weather_tex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1200, 1200, 0,
								   GL_RGBA, GL_UNSIGNED_BYTE, cTextureWeather[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);     
    DBG_FREE(cTextureWeather[0]);
	
    cTextureWeather[1] = DBG_MALLOC(360*200*4);
	memset(cTextureWeather[1], 0, 360*200*4);
	glBindTexture(GL_TEXTURE_2D, weather_tex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 360, 200, 0,
								GL_RGBA, GL_UNSIGNED_BYTE, cTextureWeather[1]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);		
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR); 
	DBG_FREE(cTextureWeather[1]);
		
	int i;
	for (i = 2; i < 8; i++)
	{
		cTextureWeather[i] = DBG_MALLOC(240*200*4);
		memset(cTextureWeather[i], 0, 240*200*4);
		glBindTexture(GL_TEXTURE_2D, weather_tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 240, 200, 0,
									GL_RGBA, GL_UNSIGNED_BYTE, cTextureWeather[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
		DBG_FREE(cTextureWeather[i]);	
	}
	
    iWeatherInit = 1;
//   glEnable(GL_TEXTURE_2D);
	return 1;
}

int GetWeatherSize(double *SizeX, double *SizeY, unsigned int iDay)
{
	if (iDay == 1) {*SizeX = 360; *SizeY = 200;}
	if ((iDay > 1) && (iDay < 8)) {*SizeX = 240; *SizeY = 200;}
	if (iDay > 7) return 0;
	return 1;
}

int RenderIconHour(GLfloat gWPosX, GLfloat gWPosY, GLfloat gWScaleX, WeatherStruct *wWeatherHour, int iDay, int iNight, int iBlend1, int iBlend2)
{
	UpperText(wWeatherHour->SymbolName);
	UpperText(wWeatherHour->PrecipitationType);
	
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glLoadIdentity();
	glVertexPointer(2, GL_FLOAT, 0, gWeatherPoints);
	glEnableClientState(GL_VERTEX_ARRAY);	
	glTexCoordPointer(2, GL_FLOAT, 0, gWeatherTexturePoints); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    glBindTexture(GL_TEXTURE_2D, weather_tex[0]);
    glBlendFunc(iBlend1, iBlend2);
    //glTranslatef(1, 0, 0.0f);
	//glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FRAME, 4);
	//glLoadIdentity();
	
    //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    if (gWScaleX != 1.0f) glScalef(gWScaleX,gWScaleX,0.0f);
	glTranslatef(gWPosX/gWScaleX, gWPosY/gWScaleX, 0.0f);
	
	glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FRAME, 4);
	
	int iRain 		= 0;
	int iSnow		= 0;
	int iMist 		= 0;
	int iFog		= 0;
	int iThunder	= 0;
	int iHail		= 0;
	int iSleet		= 0;
	
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "SNOW", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "SNOW", NULL) != 0)) iSnow = 1;
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "RAIN", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "RAIN", NULL) != 0)) iRain = 1;
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "MIST", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "MIST", NULL) != 0)) iMist = 1;
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "FOG", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "FOG", NULL) != 0)) iFog = 1;
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "THUNDER", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "THUNDER", NULL) != 0)) iThunder = 1;
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "HAIL", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "HAIL", NULL) != 0)) iHail = 1;
	if ((SearchStr(wWeatherHour->SymbolName, strlen(wWeatherHour->SymbolName), 0, "SLEET", NULL) != 0)
			|| (SearchStr(wWeatherHour->PrecipitationType, strlen(wWeatherHour->PrecipitationType), 0, "SLEET", NULL) != 0)) iSleet = 1;
	//printf("%s,%s,%i,%i,%i,%i,%i,%i,%i,%f\n",wWeatherHour->SymbolName,wWeatherHour->PrecipitationType,iSnow,iRain,iMist,iFog,iThunder,iHail,iSleet,wWeatherHour->PrecipitationValue);
	
	if ((iSnow == 1) && (iRain == 1)) {iSleet = 1; iSnow = 0; iRain = 0;}
	if (wWeatherHour->Clouds < 85)
	{
		if (iMist == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_MIST_DAY, 4);
			else if (iFog == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FOG_DAY, 4);
			else if (iDay == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLEAR_DAY, 4);
		if (iMist == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_MIST_NIGHT, 4);
			else if (iFog == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FOG_NIGHT, 4);
			else if (iNight == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLEAR_NIGHT, 4);
	}
	if ((iSnow == 0) && (iRain == 0) && (iSleet == 0) && (iHail == 0) && (iThunder == 0))
	{
		if ((wWeatherHour->Clouds >= 13) && (wWeatherHour->Clouds < 38)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS1, 4);
		if ((wWeatherHour->Clouds >= 38) && (wWeatherHour->Clouds < 63)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS2, 4);
		if ((wWeatherHour->Clouds >= 63) && (wWeatherHour->Clouds < 88)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS3, 4);
		if (wWeatherHour->Clouds >= 88) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS4, 4);
	}
	if (iSnow == 1)
	{
		if ((wWeatherHour->PrecipitationValue >= 0.025f) && (wWeatherHour->PrecipitationValue < 0.5f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW1, 4);
		if ((wWeatherHour->PrecipitationValue >= 0.5f) && (wWeatherHour->PrecipitationValue < 1.75f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW2, 4);
		if ((wWeatherHour->PrecipitationValue >= 1.75f) && (wWeatherHour->PrecipitationValue < 5.0f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW3, 4);
		if (wWeatherHour->PrecipitationValue >= 5.0f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW4, 4);
	}
	if (iRain == 1)
	{
		if ((wWeatherHour->PrecipitationValue >= 0.025f) && (wWeatherHour->PrecipitationValue < 0.75f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN1, 4);
		if ((wWeatherHour->PrecipitationValue >= 0.75f) && (wWeatherHour->PrecipitationValue < 3.75f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN2, 4);
		if ((wWeatherHour->PrecipitationValue >= 3.75f) && (wWeatherHour->PrecipitationValue < 12.5f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN3, 4);
		if (wWeatherHour->PrecipitationValue >= 12.5f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN4, 4);
	}
	if (iThunder == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN4, 4);
	if (iSleet == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SLEET, 4);
	if (iHail == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_HAIL, 4);
	
	if ((wWeatherHour->WindSpeed >= 1.5f) && (wWeatherHour->PrecipitationValue < 5.6f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND1, 4);
	if ((wWeatherHour->WindSpeed >= 5.6f) && (wWeatherHour->PrecipitationValue < 9.7f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND2, 4);
	if ((wWeatherHour->WindSpeed >= 9.7f) && (wWeatherHour->PrecipitationValue < 13.8f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND3, 4);
	if (wWeatherHour->WindSpeed >= 13.8f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND4, 4);
	
	if (wWeatherHour->Temperature >= 35.0f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_HOT, 4);
	if (wWeatherHour->Temperature <= -25.0f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FROST, 4);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int RenderIconDay(GLfloat gWPosX, GLfloat gWPosY, GLfloat gWScaleX, WeatherDayStruct *wWeatherDay, int iDay, int iNight, int iBlend1, int iBlend2)
{
	UpperText(wWeatherDay->SymbolName);
	UpperText(wWeatherDay->PrecipitationType);
	//glColorMask(0,0,0,1);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);	
	glLoadIdentity();
	glVertexPointer(2, GL_FLOAT, 0, gWeatherPoints);
	glEnableClientState(GL_VERTEX_ARRAY);	
	glTexCoordPointer(2, GL_FLOAT, 0, gWeatherTexturePoints); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    glBindTexture(GL_TEXTURE_2D, weather_tex[0]);
    glBlendFunc(iBlend1, iBlend2);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    if (gWScaleX != 1.0f) glScalef(gWScaleX,gWScaleX,0.0f);
	glTranslatef(gWPosX/gWScaleX, gWPosY/gWScaleX, 0.0f);
	
	int iRain 		= 0;
	int iSnow		= 0;
	int iMist 		= 0;
	int iFog		= 0;
	int iThunder	= 0;
	int iHail		= 0;
	int iSleet		= 0;
	
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "SNOW", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "SNOW", NULL) != 0)) iSnow = 1;
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "RAIN", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "RAIN", NULL) != 0)) iRain = 1;
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "MIST", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "MIST", NULL) != 0)) iMist = 1;
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "FOG", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "FOG", NULL) != 0)) iFog = 1;
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "THUNDER", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "THUNDER", NULL) != 0)) iThunder = 1;
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "HAIL", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "HAIL", NULL) != 0)) iHail = 1;
	if ((SearchStr(wWeatherDay->SymbolName, strlen(wWeatherDay->SymbolName), 0, "SLEET", NULL) != 0)
			|| (SearchStr(wWeatherDay->PrecipitationType, strlen(wWeatherDay->PrecipitationType), 0, "SLEET", NULL) != 0)) iSleet = 1;
	//printf("%s,%s,%i,%i,%i,%i,%i,%i,%i,%f\n",wWeatherHour->SymbolName,wWeatherHour->PrecipitationType,iSnow,iRain,iMist,iFog,iThunder,iHail,iSleet,wWeatherHour->PrecipitationValue);
	if ((iSnow == 1) && (iRain == 1)) {iSleet = 1; iSnow = 0; iRain = 0;}
	if (wWeatherDay->Clouds < 85)
	{
		if (iMist == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_MIST_DAY, 4);
			else if (iFog == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FOG_DAY, 4);
			else if (iDay == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLEAR_DAY, 4);
		if (iMist == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_MIST_NIGHT, 4);
			else if (iFog == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FOG_NIGHT, 4);
			else if (iNight == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLEAR_NIGHT, 4);
	}
	if (wWeatherDay->PrecipitationValue == 0.0f)
	{
		iSnow = 0;
		iRain = 0;
		iSleet = 0;
	}
	if ((iSnow == 0) && (iRain == 0) && (iSleet == 0) && (iHail == 0) && (iThunder == 0))
	{
		if ((wWeatherDay->Clouds >= 13) && (wWeatherDay->Clouds < 38)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS1, 4);
		if ((wWeatherDay->Clouds >= 38) && (wWeatherDay->Clouds < 63)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS2, 4);
		if ((wWeatherDay->Clouds >= 63) && (wWeatherDay->Clouds < 88)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS3, 4);
		if (wWeatherDay->Clouds >= 88) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_CLOUDS4, 4);
	}
	if (iSnow == 1)
	{
		if ((wWeatherDay->PrecipitationValue >= 0.025f) && (wWeatherDay->PrecipitationValue < 0.5f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW1, 4);
		if ((wWeatherDay->PrecipitationValue >= 0.5f) && (wWeatherDay->PrecipitationValue < 1.75f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW2, 4);
		if ((wWeatherDay->PrecipitationValue >= 1.75f) && (wWeatherDay->PrecipitationValue < 5.0f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW3, 4);
		if (wWeatherDay->PrecipitationValue >= 5.0f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SNOW4, 4);
	}
	if (iRain == 1)
	{
		if ((wWeatherDay->PrecipitationValue >= 0.025f) && (wWeatherDay->PrecipitationValue < 0.75f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN1, 4);
		if ((wWeatherDay->PrecipitationValue >= 0.75f) && (wWeatherDay->PrecipitationValue < 3.75f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN2, 4);
		if ((wWeatherDay->PrecipitationValue >= 3.75f) && (wWeatherDay->PrecipitationValue < 12.5f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN3, 4);
		if (wWeatherDay->PrecipitationValue >= 12.5f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN4, 4);
	}
	if (iThunder == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_RAIN4, 4);
	if (iSleet == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_SLEET, 4);
	if (iHail == 1) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_HAIL, 4);
	
	if ((wWeatherDay->WindSpeed >= 1.5f) && (wWeatherDay->PrecipitationValue < 5.6f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND1, 4);
	if ((wWeatherDay->WindSpeed >= 5.6f) && (wWeatherDay->PrecipitationValue < 9.7f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND2, 4);
	if ((wWeatherDay->WindSpeed >= 9.7f) && (wWeatherDay->PrecipitationValue < 13.8f)) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND3, 4);
	if (wWeatherDay->WindSpeed >= 13.8f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_WIND4, 4);
	
	//if (wWeatherDay->Temperature >= 35.0f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_HOT, 4);
	//if (wWeatherDay->Temperature <= -25.0f) glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FROST, 4);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

void GetWindName(char *cNameBuff, int ibuffsize, GLfloat gWindSpeed)
{
	memset(cNameBuff, 0, ibuffsize);
	if (gWindSpeed < 0.3f) strcpy(cNameBuff, "Штиль");
	if ((gWindSpeed >= 0.3f) && (gWindSpeed < 1.6f)) strcpy(cNameBuff, "Тихий");
	if ((gWindSpeed >= 1.6f) && (gWindSpeed < 3.4f)) strcpy(cNameBuff, "Легкий");
	if ((gWindSpeed >= 3.4f) && (gWindSpeed < 5.5f)) strcpy(cNameBuff, "Слабый");
	if ((gWindSpeed >= 5.5f) && (gWindSpeed < 8.0f)) strcpy(cNameBuff, "Умеренный");
	if ((gWindSpeed >= 8.0f) && (gWindSpeed < 10.8f)) strcpy(cNameBuff, "Свежий");
	if ((gWindSpeed >= 10.8f) && (gWindSpeed < 13.9f)) strcpy(cNameBuff, "Сильный");
	if ((gWindSpeed >= 13.9f) && (gWindSpeed < 17.2f)) strcpy(cNameBuff, "Крепкий");
	if ((gWindSpeed >= 17.2f) && (gWindSpeed < 20.8f)) strcpy(cNameBuff, "Очень крепкий");
	if ((gWindSpeed >= 20.8f) && (gWindSpeed < 24.5f)) strcpy(cNameBuff, "Шторм");
	if ((gWindSpeed >= 24.5f) && (gWindSpeed < 28.5f)) strcpy(cNameBuff, "Сильный Шторм");
	if ((gWindSpeed >= 28.5f) && (gWindSpeed < 32.6f)) strcpy(cNameBuff, "Жестокий Шторм");
	if (gWindSpeed >= 32.6f) strcpy(cNameBuff, "Ураган");
}

void GetWeaterRiseData(WeatherRise *wr)
{
	wr->FromDate = WeaterRiseData.FromDate;
	wr->FromTime = WeaterRiseData.FromTime;
	wr->ToDate = WeaterRiseData.ToDate;
	wr->ToTime = WeaterRiseData.ToTime;
	wr->Loaded = WeaterRiseData.Loaded;
}

int PreRenderGLWeather(int iTimeCorrect, int DayCountMax)
{	
	if (iWeatherInit != 1) return 0;
	char *cBuffer;
	int iSize,n,i, ret, res;
	res = 1;
	WeatherStruct *WeaterBuff;
	WeatherDayStruct *WeaterDayBuff;
//	int ret = DownloadFile("api.openweathermap.org", "/data/2.5/forecast/daily?q=Samara&mode=xml&units=metric&cnt=7", &cBuffer);
//	int ret = DownloadFile("127.0.0.1", "/data/2.5/forecast/cc?q=Samara&mode=xml&units=metric", &cBuffer);
	ret = DownloadFileNB("api.openweathermap.org", "/data/2.5/forecast?q=Samara&mode=xml&units=metric&APPID=5c10dadc99eed2c15856c1bdf90188e8", &cBuffer);
	if (ret > 0)
	{
		iSize = ret;
		//DumpData(cBuffer, iSize, "xxxx");
		ret = ConvertXml(cBuffer, iSize, &WeaterBuff, &WeaterRiseData);
		if (ret != 0)
		{
			if (WeaterData != NULL) DBG_FREE(WeaterData);
			WeaterData = WeaterBuff;
			HourDataCount = ret;
			WeaterRiseData.Loaded = 1;
			WeaterRiseData.FromTime += iTimeCorrect;
			if (WeaterRiseData.FromTime > 235959)
			{
					WeaterRiseData.FromTime -= 240000;
					WeaterRiseData.FromDate++;
			}
			if (WeaterRiseData.FromTime < 0)
			{
					WeaterRiseData.FromTime += 240000;
					WeaterRiseData.FromDate--;
			}
			WeaterRiseData.ToTime += iTimeCorrect;
			if (WeaterRiseData.ToTime > 235959)
			{
					WeaterRiseData.ToTime -= 240000;
					WeaterRiseData.ToDate++;
			}
			if (WeaterRiseData.ToTime < 0)
			{
					WeaterRiseData.ToTime += 240000;
					WeaterRiseData.ToDate--;
			}
			//printf("%i:%i,%i:%i\n",WeaterRiseData.FromDate,WeaterRiseData.FromTime,WeaterRiseData.ToDate,WeaterRiseData.ToTime);
			for (ret = 0; ret != HourDataCount; ret++)
			{
				WeaterData[ret].FromTime += iTimeCorrect;
				if (WeaterData[ret].FromTime > 235959)
				{
					WeaterData[ret].FromTime -= 240000;
					WeaterData[ret].FromDate++;
				}
				if (WeaterData[ret].FromTime < 0)
				{
					WeaterData[ret].FromTime += 240000;
					WeaterData[ret].FromDate--;
				}
				WeaterData[ret].ToTime += iTimeCorrect;
				if (WeaterData[ret].ToTime > 235959)
				{
					WeaterData[ret].ToTime -= 240000;
					WeaterData[ret].ToDate++;
				}
				if (WeaterData[ret].ToTime < 0)
				{
					WeaterData[ret].ToTime += 240000;
					WeaterData[ret].ToDate--;
				}
			}			
		} 
		else 
		{
			dbgprintf(2, "Error parse current weather\n");
			WeaterRiseData.Loaded = 0;
		}
		DBG_FREE(cBuffer);
	} 
	else 
	{
		dbgprintf(2, "Error load weather 1\n");
		return -1;
	}
	ret = DownloadFileNB("api.openweathermap.org", "/data/2.5/forecast/daily?q=Samara&mode=xml&units=metric&cnt=7&APPID=5c10dadc99eed2c15856c1bdf90188e8", &cBuffer);
	if (ret > 0)
	{
		iSize = ret;
		//DumpData(cBuffer, iSize, "xxxx");
		ret = ConvertDayXml(cBuffer, iSize, &WeaterDayBuff);
		if (ret != 0)
		{
			if (WeaterDayData != NULL) DBG_FREE(WeaterDayData);
			WeaterDayData = WeaterDayBuff;
			DayDataCount = ret;	
			if (DayDataCount > DayCountMax) DayDataCount = DayCountMax;
		}
		else
		{
			dbgprintf(2, "Error parse next days weather\n");			
		}			
		DBG_FREE(cBuffer);
	} 
	else 
	{
		dbgprintf(2, "Error load weather 2\n");
		return -2;
	}
	
	int iDay = 0;
	int iNight = 0;
	int iDayNum = 0;
	time_t rawtime;
	struct tm timeinfo;
	char txtBuff[32], txtBuff2[256], txtBuff3[32];
	time(&rawtime);
	localtime_r(&rawtime,&timeinfo);
	int NowDate, NowTime;
	NowDate = (timeinfo.tm_year+1900) * 10000;
	NowDate += (timeinfo.tm_mon+1)*100;
	NowDate += timeinfo.tm_mday;
	NowTime = timeinfo.tm_hour * 10000;
	NowTime += (timeinfo.tm_min)*100;
	NowTime += timeinfo.tm_sec;
	iDayNum = timeinfo.tm_wday;
	if ((NowTime > WeaterRiseData.FromTime) && (NowTime < WeaterRiseData.ToTime)) iDay = 1;
	if (NowTime < WeaterRiseData.FromTime) iNight = 1;
	
	i = 0;
	for (n = 0; n != HourDataCount; n++)
	{
		if (WeaterData[n].ToTime == 0) WeaterData[n].ToTime = 235959;
		if ((WeaterData[n].FromDate ==  NowDate) && (WeaterData[n].ToTime >= NowTime) && (WeaterData[n].FromTime <= NowTime))
		{
			glColorMask(1,1,1,1);
			glClearColor(0.0f,0.0f,0.0f,0.0f); 
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
			glColorMask(0,0,0,1);
			RenderIconHour(0, 0, 1.0f, &WeaterData[n], iDay, iNight, GL_SRC_ALPHA, GL_DST_ALPHA);
			glColorMask(1,1,1,0);
			RenderIconHour(0, 0, 1.0f, &WeaterData[n], iDay, iNight, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColorMask(1,1,1,1);
			
			memset(txtBuff3, 0, 32);
			GetStr(txtBuff2, 256, "%i", NowDate);
			memcpy(txtBuff3, txtBuff2+6,2);
			strcat(txtBuff3, ".");
			memcpy(txtBuff3+3, txtBuff2+4,2);
			strcat(txtBuff3, ".");
			memcpy(txtBuff3+6, txtBuff2,4);
			GetDayNumName(txtBuff2, 256, iDayNum);
			
			RenderGLText(25, 70, 173, "%s  %s", txtBuff2, txtBuff3);
			
			RenderGLText(50, 210, 120, "%.0f`C", WeaterData[n].Temperature);
			GetWindName(txtBuff, sizeof(txtBuff), WeaterData[n].WindSpeed);
			RenderGLText(19, 195, 100, "Влажность: %.0f\%\nТемпература:\n  %.1f`: %.1f`\nВетер: %.1f м/с\n%s", 
				WeaterData[n].Humidity, WeaterData[n].TemperatureMin ,WeaterData[n].TemperatureMax, WeaterData[n].WindSpeed, txtBuff);			
			glBindTexture(GL_TEXTURE_2D, weather_tex[1]);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0,360,200);	
			i = 1;			
			break;
		}
	}
	if (i != 1)
	{
		glColorMask(1,1,1,1);	
		glClearColor(0.0f,0.0f,0.0f,0.0f); 
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    				
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);	
		glLoadIdentity();
		glVertexPointer(2, GL_FLOAT, 0, gWeatherPoints);
		glEnableClientState(GL_VERTEX_ARRAY);	
		glTexCoordPointer(2, GL_FLOAT, 0, gWeatherTexturePoints); 
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
		glBindTexture(GL_TEXTURE_2D, weather_tex[0]);
		glColorMask(0,0,0,1);			
		glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
		glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FRAME, 4);
		glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_NA, 4);
		glColorMask(1,1,1,0);			
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_FRAME, 4);
		glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_NA, 4);
		glColorMask(1,1,1,1);						
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, weather_tex[1]);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0,360,200); 
		dbgprintf(3, "Not finded day weather Total: %i Need date: %i\n", HourDataCount, NowDate);
		for (n = 0; n != HourDataCount; n++) dbgprintf(3, "Exist: %i\n", WeaterData[n].FromDate);
		res = 0;
	}
	i = 0;
	NowDate++;
	iDayNum++;
	if (iDayNum == 7) iDayNum = 0;					
	for (n = 1; n < DayDataCount; n++)
	{
		if (WeaterDayData[n].Date == NowDate)
		{			
			glColorMask(1,1,1,1);	
			glClearColor(0.0f,0.0f,0.0f,0.0f); 
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    				
			glColorMask(0,0,0,1);
			RenderIconDay(0, 0.0f, 1.0f, &WeaterDayData[n], iDay, iNight, GL_SRC_ALPHA, GL_DST_ALPHA);
			glColorMask(1,1,1,0);
			RenderIconDay(0, 0.0f, 1.0f, &WeaterDayData[n], iDay, iNight, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColorMask(1,1,1,1);
			
			memset(txtBuff3, 0, 32);
			GetStr(txtBuff2, 256, "%i", NowDate);
			memcpy(txtBuff3, txtBuff2+6,2);
			strcat(txtBuff3, ".");
			memcpy(txtBuff3+3, txtBuff2+4,2);
			strcat(txtBuff3, ".");
			memcpy(txtBuff3+6, txtBuff2,4);
			GetDayNumName(txtBuff2, 256, iDayNum);
			
			RenderGLText(25, 20, 152, "%s %s", txtBuff2, txtBuff3);
			
			RenderGLText(50, 150, 110, "%.0f`", WeaterDayData[n].TemperatureDay);
			RenderGLText(35, 155, 90, "%.0f`", WeaterDayData[n].TemperatureNight);
			glBindTexture(GL_TEXTURE_2D, weather_tex[i+2]);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0,240,200);	
			i++;
			NowDate++;	
			iDayNum++;
			if (iDayNum == 7) iDayNum = 0;							
			if (i == 6) break;		
		}
		/*else 
		{
			glColorMask(1,1,1,1);	
			glClearColor(0.0f,0.0f,0.0f,0.0f); 
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    				
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_BLEND);	
			glLoadIdentity();
			glVertexPointer(2, GL_FLOAT, 0, gWeatherPoints);
			glEnableClientState(GL_VERTEX_ARRAY);	
			glTexCoordPointer(2, GL_FLOAT, 0, gWeatherTexturePoints); 
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
			glBindTexture(GL_TEXTURE_2D, weather_tex[0]);
			glColorMask(0,0,0,1);			
			glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
			glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_NA, 4);
			glColorMask(1,1,1,0);			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_NA, 4);
			glColorMask(1,1,1,1);						
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
			glBindTexture(GL_TEXTURE_2D, weather_tex[n+1]);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0,240,200);		
		}*/
	}
	if (i != 6)
	for (n = i; n != 7; n++)
	{
			glColorMask(1,1,1,1);	
			glClearColor(0.0f,0.0f,0.0f,0.0f); 
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    				
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_BLEND);	
			glLoadIdentity();
			glVertexPointer(2, GL_FLOAT, 0, gWeatherPoints);
			glEnableClientState(GL_VERTEX_ARRAY);	
			glTexCoordPointer(2, GL_FLOAT, 0, gWeatherTexturePoints); 
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
			glBindTexture(GL_TEXTURE_2D, weather_tex[0]);
			glColorMask(0,0,0,1);			
			glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
			glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_NA, 4);
			glColorMask(1,1,1,0);			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDrawArrays( GL_TRIANGLE_STRIP, WEATHER_NA, 4);
			glColorMask(1,1,1,1);						
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
			glBindTexture(GL_TEXTURE_2D, weather_tex[n+2]);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0,0,0,0,0,240,200);	
			res = 0;
			dbgprintf(3, "Error weather\n");
	}
	glColorMask(1,1,1,0);
	return res;
}

void CorrectCoords(int num, int *posX, int *posY, GLfloat scaleXY)
{	
	int n = 360;
	if (num != 0) n = 240;	
	if ((iDirection[num] == 0) || (iDirection2[num] == 0))
	{
		if ((iPosY[num] + 210) == iScrSizeY) iDirection[num] = 1;
		if ((iPosY[num] + (200*scaleXY)) < iScrSizeY) iPosY[num]++; else iDirection2[num] = 1;
	}
	if ((iDirection[num] == 1) || (iDirection2[num] == 1))
	{
		if ((iPosX[num] + 370) == iScrSizeX) iDirection[num] = 2;
		if ((iPosX[num] + (n*scaleXY)) < iScrSizeX) iPosX[num]++; else iDirection2[num] = 2;
	}
	if ((iDirection[num] == 2) || (iDirection2[num] == 2)) 
	{
		if (iPosY[num] == 10) iDirection[num] = 2;
		if (iPosY[num] > 0) iPosY[num]--; else iDirection2[num] = 3;
	}
	if ((iDirection[num] == 3)  || (iDirection2[num] == 3))
	{
		if (iPosX[num] == 10) iDirection[num] = 0;
		if (iPosX[num] > 0) iPosX[num]--; else 
		{
			iDirection2[num] = 0;
			iPosY[num]++;
		}
	}
	*posX = iPosX[num];
	*posY = iPosY[num];
}

int RenderGLWeather(double gScaleXY, double posX, double posY, unsigned int iDay)
{	
	if ((iWeatherInit != 1) || (iDay < 1) || (iDay > 7)) return 0;
	
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
		
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	glTexCoordPointer(2, GL_FLOAT, 0, gWeatherScrTexturePoints); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    
	if (iDay > 1)
	{
		glVertexPointer(2, GL_FLOAT, 0, &gWeatherScrPoints[8]); //point to vert array
		glEnableClientState(GL_VERTEX_ARRAY); //enable vert array	
	
		glLoadIdentity();
		glTranslatef(posX, posY, 0.0f);	
		if (gScaleXY != 1.0f) glScalef(gScaleXY,gScaleXY,0.0f);
		glBindTexture(GL_TEXTURE_2D, weather_tex[iDay]);
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);		
	}
	if (iDay == 1)
	{
		glVertexPointer(2, GL_FLOAT, 0, gWeatherScrPoints); //point to vert array
		glEnableClientState(GL_VERTEX_ARRAY); //enable vert array	
		glLoadIdentity();
		glTranslatef(posX, posY, 0.0f);
		if (gScaleXY != 1.0f) glScalef(gScaleXY,gScaleXY,0.0f);	
		glBindTexture(GL_TEXTURE_2D, weather_tex[iDay]);
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
	}
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

int RenderGLStaticWeather(int iMode)
{	
	if (iWeatherInit != 1) return 0;
	
	GLfloat gScaleXY = 0.3f;
	int posX = 0, posY = 0;
		
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexCoordPointer(2, GL_FLOAT, 0, gWeatherScrTexturePoints); 
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
    
	glVertexPointer(2, GL_FLOAT, 0, &gWeatherScrPoints[8]); //point to vert array
	glEnableClientState(GL_VERTEX_ARRAY); //enable vert array	
	
	int n, i;
	i = 240; 
	for (n = 7; n != 0; n--)
	{
		gScaleXY += 0.1f;
		if (n == 1) 
		{
			glVertexPointer(2, GL_FLOAT, 0, gWeatherScrPoints);
			i = 360;
		} 
		switch (iMode)
		{
			case 1:
				posX = iPosX[n-1];
				posY = (iScrSizeY-1)/gWScaleX - (200*gScaleXY);				
				break;
			case 2:
				posX = iScrSizeX - iPosX[n-1] - (i*gScaleXY);
				posY = (iScrSizeY-1)/gWScaleX - (200*gScaleXY);
				break;
			case 3:				
				posX = (iScrSizeX-1)/gWScaleX - (i*gScaleXY); 
				posY = (iScrSizeY-1) - iPosY[n-1] - (200*gScaleXY);	
				break;
			case 4:				
				posX = (iScrSizeX-1)/gWScaleX - (i*gScaleXY); 
				posY = iPosY[n-1];				
				break;
			case 5:
				posY = 0;
				posX = (iScrSizeX-1) - iPosX[n-1] - (i*gScaleXY);
				break;
			case 6:
				posY = 0;
				posX = iPosX[n-1];
				break;
			case 7:				
				posX = 0;
				posY = iPosY[n-1];			
				break;
			case 8:				
				posX = 0; 
				posY = (iScrSizeY-1) - iPosY[n-1] - (200*gScaleXY);	
				break;
			
			default:
				break;
		}
		glLoadIdentity();
		glTranslatef(posX, posY, 0.0f);	
		glScalef(gWScaleX*gScaleXY,gWScaleX*gScaleXY,0.0f);
		glBindTexture(GL_TEXTURE_2D, weather_tex[n]);
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);
		//if (n == 3) RenderGLText(19, 195, 100, "%i",posY);
	}
	/*glVertexPointer(2, GL_FLOAT, 0, gWeatherScrPoints); //point to vert array
	glEnableClientState(GL_VERTEX_ARRAY); //enable vert array	
	glBindTexture(GL_TEXTURE_2D, weather_tex[1]);
    CorrectCoords(0, &posX, &posY, gWScaleX);
	glLoadIdentity();
	glTranslatef(posX, posY, 0.0f);
	if (gWScaleX != 1.0f) glScalef(gWScaleX,gWScaleX,0.0f);	
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);*/
	
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	
	return 1;
}

void FreeGLText();
