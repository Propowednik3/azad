#ifndef _WEATHER_H_
#define _WEATHER_H_

typedef struct
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
	int Loaded;
} WeatherRise;

int DownloadFileNB(char *cServer, char *cPath, char **cBuffer);
int WSearchStrInData(char *Data, int DataLen, int Pos, char *Str);
int SearchStrInDataToStr(char *Data, int DataLen, int Pos, int *NewPos, char *Str, char *BeforeStr);
int GetValueFromXml(char *cXmlData, int iXmlSize, char *Table, char *Field, char *Value, char *Res, int ResSize, int iStartPos);
void DumpData(char *Buff, int Len, char *Lable);
float Str2FloatLimit(char *cString, unsigned int cLen);
float Str2Float(char *cString);
int ConvertXml(char *cXmlData, int iXmlSize, WeatherStruct**wConvertData, WeatherRise *RiseData);
int ConvertDayXml(char *cXmlData, int iXmlSize, WeatherDayStruct**wConvertData);
int Str2Int(char *cString);
int Str2IntLimit(char *cString, unsigned int cLen);
int Char2Hex(char *cOutString, unsigned char *cInString, unsigned int cLen, char cDirect);

#endif