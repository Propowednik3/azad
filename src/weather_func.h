#ifndef _WEATHER_FUNC_H_
#define _WEATHER_FUNC_H_

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

#include "weather.h"

/*typedef struct
{
	int FromDate;
	int FromTime;
	int ToDate;
	int ToTime;
	int Symbol;
	char SymbolName[32];	
	float PrecipitationValue;
	char PrecipitationType[32];
	float WindDirection;
	float WindSpeed;
	float Temperature;
	float TemperatureMin;
	float TemperatureMax;
	float Pressure;
	float Humidity;
	float Clouds;
} WeatherStruct;

typedef struct
{
	int Date;
	int Symbol;
	char SymbolName[32];	
	float PrecipitationValue;
	char PrecipitationType[32];
	float WindDirection;
	float WindSpeed;
	float TemperatureDay;
	float TemperatureNight;
	float TemperatureEve;
	float TemperatureMorn;
	float TemperatureMin;
	float TemperatureMax;
	float Pressure;
	float Humidity;
	float Clouds;
} WeatherDayStruct;

typedef struct
{
	int FromDate;
	int FromTime;
	int ToDate;
	int ToTime;
} WeatherRise;

*/
void GetWeaterRiseData(WeatherRise *wr); 
void CorrectCoords(int num, int *posX, int *posY, GLfloat scaleXY);
int load_weather_file (char *filename, char ** cBuffer, unsigned int *iSizeBuff);
int SearchStr(char *Data, int DataLen, int Pos, char *Str, int *FindedPos);
int InitGLWeather(unsigned int uScrSizeX, unsigned int uScrSizeY, GLfloat gScaleX);
int RenderIconDay(GLfloat gWPosX, GLfloat gWPosY, GLfloat gWScaleX, WeatherDayStruct *wWeatherDay, int iDay, int iNight, int iBlend1, int iBlend2);
int RenderIconHour(GLfloat gWPosX, GLfloat gWPosY, GLfloat gWScaleX, WeatherStruct *wWeatherHour, int iDay, int iNight, int iBlend1, int iBlend2);
int PreRenderGLWeather(int iTimeCorrect, int DayCountMax);
int RenderGLWeather(double gScaleXY, double posX, double posY, unsigned int iDay);
int RenderGLStaticWeather(int iMode);
int SetObjectWeather(int iPoliSizeX, int iBlocksX, int iPoliSizeY, int iBlocksY);
int DeInitGLWeather();
void UpperText(char *cText);
void UpperTextLimit(char *cText, int iLen);
void LowerText(char *cText);
void LowerTextLimit(char *cText, int iLen);
void GetStr(char *textbuffer, int textbufferLen, char *cText, ...);
void GetDayNumName(char *textbuffer, int textbufferLen, int iNum);
int GetWeatherSize(double *SizeX, double *SizeY, unsigned int iDay);
int GetCharPos(char *textbuffer, int textbufferLen, char cChar, char cDirect);

#endif