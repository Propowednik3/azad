/* usbreset -- send a USB port reset to a USB device
 *
 * Compile using: gcc -o updater updater.c
 *
 *
 * */

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "iconv.h"
#include <sys/vfs.h>
#include <linux/mempolicy.h>
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "linux/input.h"
#include <errno.h>


char Its_Directory(char* cPath)
{
	char ret = -1;
	DIR *dir = opendir(cPath);
	if (dir != NULL)
	{
		ret = 1;
		closedir(dir);
    }
	else 
	{
		if (errno == ENOTDIR) ret = 0;
	}
	return ret;
}

int ClearDir(char *cPath)
{
	DIR *dir;
	struct dirent *dp;	
	char cFileDir[512];
	int iCnt = 0;
	
	dir = opendir(cPath);
	if (dir != NULL)
	{
		while((dp=readdir(dir)) != NULL)
		{
			memset(cFileDir, 0, 512);
			strcpy(cFileDir, cPath);
			strcat(cFileDir, "/");
			strcat(cFileDir, dp->d_name);
			
			if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0) 
				&& !Its_Directory(cFileDir))
			{
				if (remove(cFileDir) != 0) 
					printf("Error delete '%s'\n", cFileDir); 
					else 
					{
						printf("Deleted '%s'\n", cFileDir);
						iCnt++;
					}
			}
		}
		closedir(dir);
	}
	return iCnt;
}

void system_exec(char* cmd)
{
	printf("CMD: '%s'\n", cmd);
	system(cmd);
}

int main(int argc, char **argv)
{
	if (argc < 5) 
	{
		printf("Updater: wrong params %i\n", argc);
		return 0;
	}
	char cExeStr[256];
	char cLogStr[256];
	char cSourcePath[256];
	char *cReboot = argv[1];
	char *cNewSourcePath = argv[2];
	char *cNewSourceLogin = argv[3];
	char *cNewSourcePass = argv[4];
	char *cNewSourceFile = argv[5];
	
	
	usleep(5000);
	ClearDir("src");
	int iLastPos = strlen(cNewSourcePath) - 1;
	if (cNewSourcePath[iLastPos] == 47) cNewSourcePath[iLastPos] = 0;
		
		if ((cNewSourcePath[0] == 47) && (cNewSourcePath[1] == 47))
		{
			char cAppPath[256];			
			memset(cAppPath, 0, 256);
			getcwd(cAppPath, 255);
			memcpy(cSourcePath, cAppPath, 256);
			strcat(cSourcePath, "/new_src");
			mkdir(cSourcePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			
			memset(cExeStr, 0, 256);
			snprintf(cExeStr, 255, "mount -t cifs %s %s -o user=%s,pass=%s >> src/update.log", cNewSourcePath, cSourcePath, cNewSourceLogin, cNewSourcePass);
			memset(cLogStr, 0, 256);
			snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
			system_exec(cLogStr);
			system_exec(cExeStr);
		} else memcpy(cSourcePath, cNewSourcePath, 256);
		
		memset(cExeStr, 0, 256);
		snprintf(cExeStr, 255, "cp %s/*.c src >> src/update.log", cSourcePath);
		memset(cLogStr, 0, 256);
		snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
		system_exec(cLogStr);
		system_exec(cExeStr);
		
		memset(cExeStr, 0, 256);
		snprintf(cExeStr, 255, "cp %s/*.h src >> src/update.log", cSourcePath);
		memset(cLogStr, 0, 256);
		snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
		system_exec(cLogStr);
		system_exec(cExeStr);
		
		memset(cExeStr, 0, 256);
		snprintf(cExeStr, 255, "cp %s/make* src >> src/update.log", cSourcePath);
		memset(cLogStr, 0, 256);
		snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
		system_exec(cLogStr);
		system_exec(cExeStr);
		
		memset(cExeStr, 0, 256);
		snprintf(cExeStr, 255, "cp %s/Make* src >> src/update.log", cSourcePath);
		memset(cLogStr, 0, 256);
		snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
		system_exec(cLogStr);
		system_exec(cExeStr);
		 
		memset(cExeStr, 0, 256);
		snprintf(cExeStr, 255, "cp %s/build-number.txt src >> src/update.log", cSourcePath);
		memset(cLogStr, 0, 256);
		snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
		system_exec(cLogStr);
		system_exec(cExeStr);
		
		chdir("src");
		
		memset(cExeStr, 0, 256);
		if (cNewSourceFile != NULL)
			snprintf(cExeStr, 255, "make -B -f %s >> update.log", cNewSourceFile);
			else strcpy(cExeStr, "make -B >> update.log");
		memset(cLogStr, 0, 256);
		snprintf(cLogStr, 255, "echo '%s' >> update.log", cExeStr);
		system_exec(cLogStr);				
		system_exec(cExeStr);
		chdir("..");
		//system_exec("cp src/azad", "azad");
		
		if ((cNewSourcePath[0] == 47) && (cNewSourcePath[1] == 47))
		{
			memset(cExeStr, 0, 256);
			snprintf(cExeStr, 255, "umount %s >> src/update.log", cSourcePath);
			memset(cLogStr, 0, 256);
			snprintf(cLogStr, 255, "echo '%s' >> src/update.log", cExeStr);
			system_exec(cLogStr);
			system_exec(cExeStr);			
			remove(cSourcePath);
		}
		if (strcmp(cReboot, "0") == 0) system_exec("./azad &"); else system_exec("reboot");	
	
	return 0;
}