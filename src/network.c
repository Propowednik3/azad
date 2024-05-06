#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#include <stdio.h>
#include <curl/curl.h>

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h> 
#include <time.h>
//#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "bcm_host.h"
#include "iconv.h"
#include <sys/vfs.h>
#include <linux/mempolicy.h>
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "linux/input.h"

#include "network.h"
//#include "pthread2threadx.h"
//#include "pthread2threadx.h"
#include "omx_client.h"
#include "gpio.h" 
#include "rtsp.h"
#include "debug.h"
#include "streamer.h"
#include "flv-demuxer.h"
#include "writer.h"
#include "system.h"

#define SOCKET_BUFFER_SIZE	65536


static pthread_t threadAcceptServer;
static pthread_t threadTrafWork;
pthread_attr_t tattrAcceptServer;
pthread_attr_t tattrTrafWork;
unsigned int 		Connects_ID_Cnt;
char cThreadAcceptStatus;
int Accept_Pair[2];

void ResetConnectInfo(CONNECT_INFO *conn);
void* Accept_Server(void *pData);
void* TrafWorker(void *pData);
char* getnametraffic(int TypeTraffic);
char* getnamemessage(int TypeMessage);
struct sockaddr_in aUDPTargetAddress;

int PrintIf()
{
	struct ifaddrs *interfaceArray = NULL, *tempIfAddr = NULL;
	void *tempAddrPtr = NULL;
	int rc = 0;
	char addressOutputBuffer[INET6_ADDRSTRLEN];
  
	rc = getifaddrs(&interfaceArray);  /* retrieve the current interfaces */
	if (rc == 0)
	{    
		for(tempIfAddr = interfaceArray; tempIfAddr != NULL; tempIfAddr = tempIfAddr->ifa_next)
		{
			if (tempIfAddr->ifa_addr == NULL)
                   continue;
			if(tempIfAddr->ifa_addr->sa_family == AF_INET)
				tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_addr)->sin_addr;
			else
				tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_addr)->sin6_addr;
      
			printf("Internet Address:  %s \n",
				inet_ntop(tempIfAddr->ifa_addr->sa_family,
                       tempAddrPtr,
                       addressOutputBuffer,
                       sizeof(addressOutputBuffer)));

			printf("LineDescription :  %s \n", tempIfAddr->ifa_name);
			if(tempIfAddr->ifa_netmask != NULL)
			{
				if(tempIfAddr->ifa_netmask->sa_family == AF_INET)
					tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_netmask)->sin_addr;
				else
					tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_netmask)->sin6_addr;

				printf("Netmask         :  %s \n",
					inet_ntop(tempIfAddr->ifa_netmask->sa_family,
                         tempAddrPtr,
                         addressOutputBuffer,
                         sizeof(addressOutputBuffer)));
			}
			if(tempIfAddr->ifa_ifu.ifu_broadaddr != NULL)
			{
				/* If the ifa_flags field indicates that this is a P2P interface */
				if(tempIfAddr->ifa_flags & IFF_POINTOPOINT)
				{
					printf("Destination Addr:  ");
					if(tempIfAddr->ifa_ifu.ifu_dstaddr->sa_family == AF_INET)
						tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_ifu.ifu_dstaddr)->sin_addr;
					else
						tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_ifu.ifu_dstaddr)->sin6_addr;
				}
				else
				{
					printf("Broadcast Addr  :  ");
					if(tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family == AF_INET)
						tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_ifu.ifu_broadaddr)->sin_addr;
					else
						tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_ifu.ifu_broadaddr)->sin6_addr;
				}          
				printf("%s \n",
					inet_ntop(tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family,
                         tempAddrPtr,
                         addressOutputBuffer,
                         sizeof(addressOutputBuffer)));
			}
			if(tempIfAddr->ifa_flags & IFF_RUNNING) printf("RUNNING\n");
			if(tempIfAddr->ifa_flags & IFF_UP) printf("UP\n");
			printf("\n");
		}
		freeifaddrs(interfaceArray);             /* free the dynamic memory */
		interfaceArray = NULL;                   /* prevent use after free  */
	}
	else
	{
		printf("getifaddrs() failed with errno =  %d %s \n",
            errno, strerror(errno));     
	}
	return rc;
}

int GetBroadcastAddr(struct sockaddr_in *Address)
{
	struct ifaddrs *interfaceArray = NULL, *tempIfAddr = NULL;
	int rc = 0;
	int ret = 0;
	Address->sin_addr.s_addr = inet_addr("127.0.0.1");
	Address->sin_family = AF_INET;
	Address->sin_port = 0;
	
	rc = getifaddrs(&interfaceArray);  /* retrieve the current interfaces */
	if (rc == 0)
	{    
		for(tempIfAddr = interfaceArray; tempIfAddr != NULL; tempIfAddr = tempIfAddr->ifa_next)
		{
			if  ((tempIfAddr->ifa_addr == NULL) ||
				(strcmp("lo", tempIfAddr->ifa_name) == 0) ||
				(!(tempIfAddr->ifa_flags & (IFF_RUNNING))) || 
				(tempIfAddr->ifa_addr->sa_family != AF_INET) ||
				(tempIfAddr->ifa_ifu.ifu_broadaddr == NULL) ||
				(tempIfAddr->ifa_flags & IFF_POINTOPOINT) ||
				(tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family != AF_INET)) continue;
		    Address->sin_addr = ((struct sockaddr_in *)tempIfAddr->ifa_ifu.ifu_broadaddr)->sin_addr;
			ret = 1;
			dbgprintf(4, "Get Broadcast address: %s\n",  inet_ntoa(Address->sin_addr));
			break;
        }
		if (tempIfAddr == NULL) dbgprintf(4, "Get Broadcast address not succes\n");
		freeifaddrs(interfaceArray);             /* free the dynamic memory */
		interfaceArray = NULL;                   /* prevent use after free  */			
    }
	else
	{
		ret = -1;
		dbgprintf(2,"getifaddrs() failed with errno =  %d %s \n",
            errno, strerror(errno));		
	}	
	return ret;
}

int GetLocalNetIfFromDhcpcd(char *ipaddress, char *routers, char *mask, char *domain_name_servers, unsigned int ui_max_len)
{
	FILE *f;
	if ((f = fopen("/etc/dhcpcd.conf","r")) == NULL)
	{
		dbgprintf(1,"Error load dhcpcd.conf\n");
		return 0;
	}
	
	char Buff1[1024];
	int iInBlock = 0;
	int ipos, ilen, n, ifrom, idestlen, itype;
	
	memset(Buff1, 0, 1024);
	while (fgets(Buff1, 1023, f))
	{
		ilen = strlen(Buff1);
		if (iInBlock)
		{
			ipos = SearchStrInDataCaseIgn(Buff1, ilen, 0, "static ");
			if (ipos == 1)
			{
				itype = 0;
				ipos = 0;
				
				if (!ipos)
				{
					ipos = SearchStrInDataCaseIgn(Buff1, ilen, ipos - 1, "ip_address=");
					if (ipos) 
					{
						ipos += strlen("ip_address=") - 1;
						itype = 1;
					}				
				}
				if (!ipos) 
				{
					ipos = SearchStrInDataCaseIgn(Buff1, ilen, ipos - 1, "routers=");
					if (ipos) 
					{
						ipos += strlen("routers=") - 1;
						itype = 2;
					}
				}
				if (!ipos) 
				{
					ipos = SearchStrInDataCaseIgn(Buff1, ilen, ipos - 1, "domain_name_servers=");
					if (ipos) 
					{
						ipos += strlen("domain_name_servers=") - 1;
						itype = 3;
					}
				}
				if (ipos) 
				{
					for (; ipos < ilen; ipos++) if (Buff1[ipos] >= 33) break;
					if (ipos >= ilen) break;
					ifrom = ipos;
					for (; ipos < ilen; ipos++) if ((Buff1[ipos] == 32) || (Buff1[ipos] == 10) || (Buff1[ipos] == 13)) break;
					idestlen = ipos - ifrom;
					if (idestlen >= ui_max_len) break;
					if (itype == 1) 
					{
						for (n = ifrom; n < ipos; n++) 
							if (Buff1[n] == 47) 
							{
								if (ipaddress) memset(ipaddress, 0, ui_max_len);
								if (mask) memset(mask, 0, ui_max_len);
								if (ipaddress) memcpy(ipaddress, &Buff1[ifrom], n - ifrom);
								if (mask) memcpy(mask, &Buff1[n + 1], idestlen - (n - ifrom) - 1);
								break;
							}													
					}
					if ((itype == 2) && routers) 
					{
						memset(routers, 0, ui_max_len);
						memcpy(routers, &Buff1[ifrom], idestlen);
					}
					if ((itype == 3) && domain_name_servers) 
					{
						memset(domain_name_servers, 0, ui_max_len);
						memcpy(domain_name_servers, &Buff1[ifrom], idestlen);
					}
				}
			}			
		}
		
		ipos = SearchStrInDataCaseIgn(Buff1, ilen, 0, "interface ");
		if (ipos == 1)
		{
			ipos = SearchStrInDataCaseIgn(Buff1, ilen, ipos - 1, "eth0");
			if (ipos) iInBlock = 1; else iInBlock = 0;
		}					
	}
	fclose(f);
		
	return 1;	
}
	
int GetLocalNetIf(eth_config *ifr_out)
{
	memset(ifr_out, 0, sizeof(eth_config));
    struct ifreq ifr;
	struct ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) { /* handle error*/ };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */ }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) 
	{
		strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) 
		{
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) 
			{ // don't count loopback
				ifr_out->if_flags  = ifr.ifr_flags;            
				//if (ioctl(sock, SIOCGIFNAME, &ifr) == 0) 
				memcpy(ifr_out->if_name, ifr.ifr_name, IFNAMSIZ);
				if (ioctl(sock, SIOCGIFINDEX, &ifr) == 0) ifr_out->if_ifindex = ifr.ifr_ifindex;
				if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) memcpy(&ifr_out->if_addr, &ifr.ifr_addr, sizeof(struct sockaddr));
				if (ioctl(sock, SIOCGIFDSTADDR, &ifr) == 0)	memcpy(&ifr_out->if_dstaddr, &ifr.ifr_dstaddr, sizeof(struct sockaddr));
				if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == 0)	memcpy(&ifr_out->if_broadaddr, &ifr.ifr_broadaddr, sizeof(struct sockaddr));
				if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0)	memcpy(&ifr_out->if_netmask, &ifr.ifr_netmask, sizeof(struct sockaddr));				
				if (ioctl(sock, SIOCGIFMETRIC, &ifr) == 0)	ifr_out->if_metric = ifr.ifr_metric;				
				if (ioctl(sock, SIOCGIFMTU, &ifr) == 0) ifr_out->if_mtu = ifr.ifr_mtu;				
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) memcpy(&ifr_out->if_hwaddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));				
				if (ioctl(sock, SIOCGIFMAP, &ifr) == 0)	memcpy(&ifr_out->if_map, &ifr.ifr_map, sizeof(struct ifmap));
				//if (ioctl(sock, SIOCGIFPFLAGS, &ifr) == 0) success++;
				//if (ioctl(sock, SIOCSIFHWBROADCAST, &ifr) == 0) success++;
				//if (ioctl(sock, SIOCGIFTXQLEN, &ifr) == 0) success++;
				break;
            }
        }
    }  
					
	return success;
}

int GetLocalAddr(struct sockaddr_in *Address)
{
	struct ifaddrs *interfaceArray = NULL, *tempIfAddr = NULL;
	int rc = 0;
	int ret = 0;
	Address->sin_addr.s_addr = 0; //inet_addr("127.0.0.1");
	Address->sin_family = AF_INET;
	Address->sin_port = 0;
	
	rc = getifaddrs(&interfaceArray);  /* retrieve the current interfaces */
	if (rc == 0)
	{    
		for(tempIfAddr = interfaceArray; tempIfAddr != NULL; tempIfAddr = tempIfAddr->ifa_next)
		{
			if  ((tempIfAddr->ifa_addr == NULL) ||
				(strcmp("lo", tempIfAddr->ifa_name) == 0) ||
				(!(tempIfAddr->ifa_flags & (IFF_RUNNING))) || 
				(tempIfAddr->ifa_addr == NULL) ||
				(tempIfAddr->ifa_addr->sa_family != AF_INET) ||
				(tempIfAddr->ifa_flags & IFF_POINTOPOINT)) continue;
		    Address->sin_addr = ((struct sockaddr_in *)tempIfAddr->ifa_addr)->sin_addr;
			ret = 1;
			dbgprintf(4, "Get Local address: %s\n",  inet_ntoa(Address->sin_addr));
			break;
        }
		freeifaddrs(interfaceArray);             /* free the dynamic memory */
		interfaceArray = NULL;                   /* prevent use after free  */			
    }
	else
	{
		ret = -1;
		dbgprintf(2,"getifaddrs() failed with errno =  %d %s \n", errno, strerror(errno));		
	}	
	return ret;
}

void LoadPrintDump(char *cPath)
{
	DBG_LOG_IN();
	
	FILE *f;
	if ((f = fopen(cPath,"rb")) == NULL)
	{
		dbgprintf(1,"Error open log file:%s\n", cPath);
		return;
	}
	char *Buff = DBG_MALLOC(65535);
	TRANSFER_DATA* trInHeaderData;
	int iReadLen = fread(Buff, 1, 65535, f);
	dbgprintf(3,"Dump file %s, readed %i :\n", cPath, iReadLen);
	int i, ret;
	for (i = 0; i < iReadLen; i++)
	{
		ret = SearchKeyInData(Buff, iReadLen, i);		
		if ((ret != 0) && ((iReadLen - i) >= sizeof(TRANSFER_DATA)))
		{
			i = ret - 1;
			trInHeaderData = (TRANSFER_DATA*)&Buff[i];
			dbgprintf(3,"Packet: position=%i, size=%i, Type=%i(%s)\n", i, trInHeaderData->SizeMessage,trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));
			if (trInHeaderData->TypeMessage == TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA)
			{
				VIDEO_FRAME_INFO* vVFI = (VIDEO_FRAME_INFO*)&Buff[sizeof(TRANSFER_DATA)+i];	
				dbgprintf(3,"TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA Number=%i, LenData=%i, Flag=%i\n",vVFI->Number,vVFI->LenData,vVFI->Flag);							
			}
			if (trInHeaderData->TypeMessage == TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA)
			{
				VIDEO_SUBFRAME_INFO* vVSFI = (VIDEO_SUBFRAME_INFO*)&Buff[sizeof(TRANSFER_DATA)+i];	
				dbgprintf(3,"TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA Number:%i,LenData:%i,SubNumber:%i\n",vVSFI->Number,vVSFI->LenData,vVSFI->SubNumber);	
			}
			i+=sizeof(TRANSFER_DATA); 
		}
		else break;
	}
	dbgprintf(3,"Dump file done\n");
	fclose(f);
	DBG_FREE(Buff);
	
	DBG_LOG_OUT();	
}

void PrintNanos(char *cLabel)
{
	struct timespec nanotime;
	clock_gettime(CLOCK_REALTIME, &nanotime);
	dbgprintf(3,"nano:%u  %s\n",(unsigned int)nanotime.tv_nsec, cLabel);	
}

int InitLAN()
{
	DBG_LOG_IN();
	
	pthread_attr_init(&tattrAcceptServer);      
	pthread_attr_setdetachstate(&tattrAcceptServer, PTHREAD_CREATE_DETACHED);
	pthread_attr_init(&tattrTrafWork);   
	pthread_attr_setdetachstate(&tattrTrafWork, PTHREAD_CREATE_DETACHED);	
	
	uiRecvDataCnt = 0;
	uiSendDataCnt = 0;
	Connects_ID_Cnt = 999;
	Accept_Pair[0] = 0;
	Accept_Pair[1] = 0;	
	
	pthread_mutex_init(&Network_Mutex, NULL);
	
	aUDPTargetAddress.sin_family 	= AF_INET;
	aUDPTargetAddress.sin_port 		= htons(UDP_PORT);
	if (cUdpTargetAddress[0]) 
		aUDPTargetAddress.sin_addr.s_addr = inet_addr(cUdpTargetAddress);
		else 
		aUDPTargetAddress.sin_addr.s_addr = 0;
	
	int ret = Init_Servers();
	DBG_LOG_OUT();	
	return ret;
}

void DelLAN()
{
	DBG_LOG_IN();
	int n, ret;	
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	if (Accept_Pair[0] != 0)
	{
		char cType = SIGNAL_CLOSE;
		int rv = write(Accept_Pair[0], &cType, 1);
		if (rv < 1) dbgprintf(4, "DelLAN: write socketpair signal %i (errno:%i, %s)\n", cType, errno, strerror(errno));	
	}
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	do
	{
		DBG_MUTEX_LOCK(&Network_Mutex);
		ret = cThreadAcceptStatus;
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		if (ret != 0) usleep(50000);
	} while(ret != 0);
	DBG_MUTEX_LOCK(&Network_Mutex);
	for (n = 0; n < Connects_Count; n++)
	{
		ret = 0;
		DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
		if (Connects_Info[n].Status == CONNECT_STATUS_ONLINE)
		{
			ret = 1;	
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
		if (ret) SendSignalType(n, SIGNAL_CLOSE);			
	}
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	while(1)
	{
		ret = 0;			
		DBG_MUTEX_LOCK(&Network_Mutex);
		for (n = 0; n < Connects_Count; n++)
		{
			DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
			if (Connects_Info[n].Status == CONNECT_STATUS_ONLINE) ret++;	
			DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);	
		}
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		if (ret) usleep(50000); else break;
	}
	dbgprintf(4, "Closed All Connections\n");	
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	for (n = 0; n < Connects_Count; n++)
	{
		pthread_mutex_destroy(&Connects_Info[n].Socket_Mutex);
		pthread_mutex_destroy(&Connects_Info[n].Pair_Mutex);
		tx_eventer_delete(Connects_Info[n].pevntIP);
		DBG_FREE(Connects_Info[n].pevntIP);	
	}
	Connects_Max_Active = 0;
	Connects_Count = 0;
	DBG_FREE(Connects_Info);
	Connects_Info = NULL;
	DBG_MUTEX_UNLOCK(&Network_Mutex);	
	dbgprintf(4, "Release All Connections\n");	
	
	pthread_attr_destroy(&tattrTrafWork);
	pthread_attr_destroy(&tattrAcceptServer);	
	
	DBG_LOG_OUT();	
}

unsigned int CalcCRC(char *buffer, unsigned int iLength)
{	
	return 0;
	DBG_LOG_IN();
	
	unsigned int		CRC = 0;
	unsigned int		CRC2 = 0;
	unsigned int		Len;
	unsigned int		Arr[256];
	
	
	memset(Arr, 0, sizeof(Arr));
	for (Len = 0;Len != iLength; Len++)
	{		
		Arr[(unsigned char)buffer[Len]]++;
		CRC += (unsigned char)buffer[Len];
	}
	
	iLength = 256;
	for (Len = 0;Len != iLength; Len++)
	{
		CRC2 += Arr[Len];
	}
	
	DBG_LOG_OUT();	
	return (CRC ^ CRC2);
}

unsigned int CalcCRC2(unsigned char *buffer, unsigned int iLength)
{
	DBG_LOG_IN();
	
	unsigned int		CRC = 0;
	unsigned char		*pCRC = (unsigned char*)&CRC;
	unsigned int		n, i;
	
	
	i = 0;
	for (n = 0; n != iLength; n++)
	{		
		pCRC[i] ^= buffer[n];
		i++;
		if (i == 4) i = 0;
	}
	
	DBG_LOG_OUT();	
	return CRC;
}

void CloseAllConnects(char cServer, unsigned int iFlag, char cFull)
{
	DBG_LOG_IN();
	int iFirst = 1;	
	if (cFull) iFirst = 0;
	int s, r;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iLast = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	for (s = iFirst; s < iLast; s++)
	{
		r = 0;
		DBG_MUTEX_LOCK(&Connects_Info[s].Socket_Mutex);
		if (
			(Connects_Info[s].Status == CONNECT_STATUS_ONLINE) 
			&& 
			(Connects_Info[s].Type == cServer)
			&& 
			((Connects_Info[s].NeedData & iFlag) || (iFlag == 0))
			&&
			(Connects_Info[s].TraffType != TRAFFIC_TRANSFER_FILE))
		{
			r = 1;	
			//printf("Close %i\n", s);
		} 
		DBG_MUTEX_UNLOCK(&Connects_Info[s].Socket_Mutex);
		if (r) SendSignalType(s, SIGNAL_CLOSE);
	}
	
	DBG_LOG_OUT();	
}

int CloseConnectID(unsigned int uiID)
{
	DBG_LOG_IN();
	
	int s;
	int ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iLast = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	for (s = 0; s < iLast; s++)
	{
		ret = 0;
		DBG_MUTEX_LOCK(&Connects_Info[s].Socket_Mutex);
		if (
			(Connects_Info[s].Status == CONNECT_STATUS_ONLINE) 
			&& 
			(Connects_Info[s].Type == CONNECT_CLIENT)
			&& 
			(Connects_Info[s].ID == uiID)
			)
		{
			ret = 1;			
		} 
		DBG_MUTEX_UNLOCK(&Connects_Info[s].Socket_Mutex);	
		if (ret) SendSignalType(s, SIGNAL_CLOSE);
	}
	
	DBG_LOG_OUT();	
	return ret;
}

int TCP_Client(struct sockaddr_in *m_sin, int cTypeConnect, unsigned int iFlag, char cJoinToExist, unsigned int *uiID)
{
	DBG_LOG_IN();
	int ret, n, i;
	int iConnCnt;
	*uiID = 0;
	unsigned int uiSendMessages[5];
	char cMessagesCnt = 0;
	switch(cTypeConnect)
	{
		case TRAFFIC_SERVICE:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_SERVICE;
			cMessagesCnt = 1;
			break;
		case TRAFFIC_AUDIO:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_AUDIO;
			uiSendMessages[1] = TYPE_MESSAGE_REQUEST_AUDIO_CODEC_INFO;
			uiSendMessages[2] = TYPE_MESSAGE_REQUEST_AUDIO_STREAM;
			cMessagesCnt = 3;
			break;
		case TRAFFIC_FULL_VIDEO:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_FULL_VIDEO;
			uiSendMessages[1] = TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO;
			uiSendMessages[2] = TYPE_MESSAGE_REQUEST_VIDEO_PARAMS;
			uiSendMessages[3] = TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME;
			uiSendMessages[4] = TYPE_MESSAGE_REQUEST_VIDEO_STREAM;
			cMessagesCnt = 5;
			break;
		case TRAFFIC_PREV_VIDEO:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_PREV_VIDEO;
			uiSendMessages[1] = TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO;
			uiSendMessages[2] = TYPE_MESSAGE_REQUEST_VIDEO_PARAMS;
			uiSendMessages[3] = TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME;
			uiSendMessages[4] = TYPE_MESSAGE_REQUEST_VIDEO_STREAM;
			cMessagesCnt = 5;
			break;
		case TRAFFIC_REMOTE_FILE:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_FILE;
			cMessagesCnt = 1;
			break;
		case TRAFFIC_REMOTE_VIDEO:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_VIDEO;
			cMessagesCnt = 1;
			break;
		case TRAFFIC_REMOTE_AUDIO:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_AUDIO;			
			cMessagesCnt = 1;
			if (iFlag & FLAG_AUDIO_STREAM)
			{
				uiSendMessages[1] = TYPE_MESSAGE_REQUEST_AUDIO_STREAM;			
				cMessagesCnt = 2;			
			}
			break;
		case TRAFFIC_TRANSFER_FILE:
			uiSendMessages[0] = TYPE_MESSAGE_TRAFFIC_TYPE_TRANSFER_FILE;
			cMessagesCnt = 1;
			break;
		default:
			cMessagesCnt = 0;
			break;
	}
		
	if (cJoinToExist) 
	{
		ret = 0;
		DBG_MUTEX_LOCK(&Network_Mutex);
		iConnCnt = Connects_Max_Active;
		DBG_MUTEX_UNLOCK(&Network_Mutex);	
		for (n = 1; n < iConnCnt; n++)
		{
			DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);			
			if ((Connects_Info[n].Status == CONNECT_STATUS_ONLINE) 
				&& (Connects_Info[n].Type == CONNECT_CLIENT) 
				&& (Connects_Info[n].Addr.sin_addr.s_addr == m_sin->sin_addr.s_addr) 
				&& (Connects_Info[n].Addr.sin_port == m_sin->sin_port)
				&& (Connects_Info[n].Addr.sin_family == m_sin->sin_family))
				{
					ret = 1; //Connects_Info[n].ClientPort;
					DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);					
					break;	
				}
			DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);			
		}	
		
		if (ret) 
		{
			dbgprintf(4,"exist connect using\n");
			DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
			*uiID = Connects_Info[n].ID;
			Connects_Info[n].NeedData |= iFlag;				
			for (i = 0; i != cMessagesCnt; i++)	SendMessage(n, uiSendMessages[i], NULL, 0, NULL, 0, &Connects_Info[n].Addr);		
			DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
			DBG_LOG_OUT();	
			return n;
		}
			
	}
	
	int sock;
	struct sockaddr_in m_sout;
	
	memset(&m_sout, 0, sizeof(m_sout));	
	m_sout.sin_family = AF_INET;
	m_sout.sin_port = m_sin->sin_port;
	m_sout.sin_addr = m_sin->sin_addr;	
	//DBG_FREE(m_sin);
	
	fd_set rfds, wfds;
	struct timeval tv;
		
	int max_fd = -1;
	socklen_t err_len;
	int error;
	int flags;
	int printed_error = 0;
	
	//printf("connect\n");
	int iCntConnect = 4;
	do
	{
		iCntConnect--;
		if (iCntConnect == 0) { DBG_LOG_OUT(); return 0;}
		//printf("connecting\n");
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
		if (sock < 0 ) 
		{
			dbgprintf(1,"CLNT error create socket: IP_Client\n");
			DBG_LOG_OUT();	
			return 0;
		}
		flags = fcntl(sock, F_GETFL, 0);
		if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			dbgprintf(1,"CLNT error set nonblock: IP_Client\n");
			close(sock);
			DBG_LOG_OUT();	
			return 0;
		}
		n = SOCKET_BUFFER_SIZE;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) 
			dbgprintf(2,"CLNT error setsockopt (SO_RCVBUF)(%i) %s\n", errno, strerror(errno));
			
		n = SOCKET_BUFFER_SIZE;
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)) == -1) 
			dbgprintf(2,"CLNT error setsockopt (SO_SNDBUF)(%i) %s\n", errno, strerror(errno));
			
		max_fd = -1;
		if (sock > max_fd) max_fd = sock;	
		dbgprintf(5, "connecting to %s:%i\n", inet_ntoa(m_sout.sin_addr), ntohs(m_sout.sin_port));
		ret = connect(sock, (struct sockaddr*)&m_sout, sizeof(m_sout));
		if (ret != 0) 
		{
			if (errno == EINPROGRESS) 
			{
				//printf("connecting\n");
				FD_ZERO(&wfds);
				FD_ZERO(&rfds);
				FD_SET(sock, &wfds);
				FD_SET(sock, &rfds);
				tv.tv_sec = 1;
				tv.tv_usec = 0;	
				ret = select(max_fd + 1, &rfds, &wfds, NULL, &tv);
				if (ret == 0)
				{
					dbgprintf(4,"timeout connect\n");
					shutdown(sock, SHUT_RDWR);
					close(sock);
					DBG_LOG_OUT();	
					return 0;			
				}
				if (ret > 0)
				{
					if (FD_ISSET(sock, &wfds) || FD_ISSET(sock, &rfds))
					{
						err_len = sizeof(error);
						error = 0;
						if ((getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &err_len) < 0) || (error != 0))
						{
							errno = error;
							if (!printed_error) 
							{
								dbgprintf(1,"error connect2 %i(%s), %i\n", cTypeConnect, getnametraffic(cTypeConnect), iFlag);
								printed_error = 1;
							}
							shutdown(sock, SHUT_RDWR);
							close(sock);
							//return 0;
						} 
						else iCntConnect = 0;
					}// else dbgprintf(3,"wait\n");
				}
				else dbgprintf(1,"error select\n");
			}
			else		
			{
				dbgprintf(1,"CLNT error connect, get out %i(%s), %i\n", cTypeConnect, getnametraffic(cTypeConnect), iFlag);
				close(sock);
				DBG_LOG_OUT();	
				return 0;
			}				
		} else iCntConnect = 0;
		//if (iCntConnect != 0) usleep(100000);
	} while (iCntConnect);
	
	ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);	
	for (n = 1; n < Connects_Count; n++)
	{
		DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
		if (Connects_Info[n].Status == CONNECT_STATUS_OFFLINE) 
		{
			ret = 1;
			DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);		
			break;
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);		
	}
	if ((ret == 0) && (Connects_Count >= MAX_CONNECTIONS))
	{
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		dbgprintf(1,"LIMIT connections\n"); 
		shutdown(sock, SHUT_RDWR);
		close(sock);
		DBG_LOG_OUT();	
		return 0;
	}	
	if (ret == 0)
	{
		n = Connects_Count;
		Connects_Count++;
		memset(&Connects_Info[n], 0, sizeof(CONNECT_INFO));
		pthread_mutex_init(&Connects_Info[n].Socket_Mutex, NULL);
		pthread_mutex_init(&Connects_Info[n].Pair_Mutex, NULL);
		Connects_Info[n].pevntIP = DBG_MALLOC(sizeof(TX_EVENTER));
		tx_eventer_create(Connects_Info[n].pevntIP, 0);	
	}
		
	DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
	ResetConnectInfo(&Connects_Info[n]);
	int spair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, spair) < 0)
	{
		dbgprintf(1,"error create socketpair\n");
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		DBG_LOG_OUT();	
		return 0;
	}
	flags = fcntl(spair[0], F_GETFL, 0);
	if (fcntl(spair[0], F_SETFL, flags | O_NONBLOCK) < 0)
	{
		dbgprintf(1,"error set socketpair nonblock\n");
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		close(spair[0]);
		close(spair[1]);
		DBG_LOG_OUT();	
		return 0;
	}
	flags = fcntl(spair[1], F_GETFL, 0);
	if (fcntl(spair[1], F_SETFL, flags | O_NONBLOCK) < 0)
	{
		dbgprintf(1,"error set socketpair nonblock\n");
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		close(spair[0]);
		close(spair[1]);
		DBG_LOG_OUT();	
		return 0;
	}
	
	Connects_Info[n].Status = CONNECT_STATUS_ONLINE;
	Connects_Info[n].Socket = sock;
	Connects_Info[n].Type = CONNECT_CLIENT;
	Connects_Info[n].TraffType = cTypeConnect;
	memcpy(&Connects_Info[n].Addr, &m_sout, sizeof(m_sout));
	//printf("connected %i, to addr:%s\n", n, inet_ntoa(Connects_Info[n].Addr.sin_addr));
	Connects_Info[n].NeedData = iFlag;
			
	i = sizeof(m_sout);
	ret = getsockname(sock, (struct sockaddr*)&m_sout, (socklen_t*)&i);
	if (ret < 0) dbgprintf(1,"Error getsockname\n"); 
	Connects_Info[n].ClientPort = m_sout.sin_port;
	Connects_ID_Cnt++;
	Connects_Info[n].ID = Connects_ID_Cnt;
	*uiID = Connects_ID_Cnt;
		
	switch(Connects_Info[n].TraffType)
	{
		case TRAFFIC_SERVICE:
			Connects_Info[n].InBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_AUDIO:
			Connects_Info[n].InBufferSize = AUDIO_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_FULL_VIDEO:
			Connects_Info[n].InBufferSize = FVIDEO_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_PREV_VIDEO:
			Connects_Info[n].InBufferSize = PVIDEO_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_REMOTE_FILE:
			Connects_Info[n].InBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_REMOTE_VIDEO:
			Connects_Info[n].InBufferSize = FVIDEO_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_REMOTE_AUDIO:
			Connects_Info[n].InBufferSize = AUDIO_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		case TRAFFIC_TRANSFER_FILE:
			Connects_Info[n].InBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			break;
		default:
			Connects_Info[n].InBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
			dbgprintf(2, "Unknown Traffic time %i\n", Connects_Info[n].TraffType);
			break;
	}
	
	Connects_Info[n].InBuffer = (char*)DBG_MALLOC(Connects_Info[n].InBufferSize);
	Connects_Info[n].InDataSize = 0;
	
	Connects_Info[n].OutBuffer = (char*)DBG_MALLOC(Connects_Info[n].OutBufferSize);
	Connects_Info[n].OutDataSize = 0;
	
	GetCurrDateTimeStr(Connects_Info[n].DateConnect, 64);
	
	DBG_MUTEX_LOCK(&Connects_Info[n].Pair_Mutex);
	Connects_Info[n].socketpair[0] = spair[0];
	Connects_Info[n].socketpair[1] = spair[1];
	DBG_MUTEX_UNLOCK(&Connects_Info[n].Pair_Mutex);
	
	DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
	
	if (Connects_Max_Active < (n + 1)) Connects_Max_Active = n + 1;
	DBG_MUTEX_UNLOCK(&Network_Mutex);		
	
	DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
	for (i = 0; i != cMessagesCnt; i++)	SendMessage(n, uiSendMessages[i], NULL, 0, NULL, 0, &Connects_Info[n].Addr);	
	DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
	
	dbgprintf(5, "connected: %i\n", n);
	
	pthread_create(&threadTrafWork, &tattrTrafWork, TrafWorker, (void*)n);
		
	DBG_LOG_OUT();	
	return n;
}

int SearchKeyInData(char *cData, int iDataLen, int iPos)
{
	DBG_LOG_IN();
	
	unsigned int iKey[3];
	char *cKey = (char*)&iKey;
	int iKeyLen = 12;
	int iFlag = 0;
	int n;
	int ret = 0;
	iKey[0] = TRANSFER_KEY1;
	iKey[1] = TRANSFER_KEY2;
	iKey[2] = TRANSFER_KEY3;
	for (n = iPos; n != iDataLen; n++)
	{
		if (cKey[iFlag] == cData[n]) iFlag++; 
		else
		{
			iFlag = 0;
			if (cKey[iFlag] == cData[n]) iFlag++;
		}
		if (iFlag == iKeyLen) 
		{
			ret = (n - iKeyLen + 2);
			break;
		}
	}
	
	DBG_LOG_OUT();	
	return ret;
}

void ResetConnectInfo(CONNECT_INFO *conn)
{
	conn->Socket = 0;
	conn->socketpair[0] = 0;
	conn->socketpair[1] = 0;
	memset(&conn->Addr, 0, sizeof(struct sockaddr_in));
	conn->Status = CONNECT_STATUS_OFFLINE;
	conn->Type = 0;
	conn->TraffType = TRAFFIC_UNKNOWN;
	conn->NeedData = 0;
	conn->Timer = 0;
	conn->InBuffer = NULL;
	conn->InDataSize = 0;
	conn->OutBuffer = NULL;
	conn->OutDataSize = 0;
	conn->ClientPort = 0;
	conn->DataNum = 0;
	conn->DataPos = 0;
	
	//conn->pevntIP = NULL;
	//conn->Socket_Mutex = 0;
}

char TestIncomeConnects()
{	
	int s;
	char ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
		
	for (s = 1; s < iConnMax; s++)
	{
		DBG_MUTEX_LOCK(&Connects_Info[s].Socket_Mutex);
		if ((Connects_Info[s].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[s].Type == CONNECT_SERVER))
		{
			if (Connects_Info[s].TraffType == TRAFFIC_AUDIO) ret |= 1;
			if ((Connects_Info[s].TraffType == TRAFFIC_FULL_VIDEO) ||
				(Connects_Info[s].TraffType == TRAFFIC_PREV_VIDEO)) ret |= 2;
		}		
		DBG_MUTEX_UNLOCK(&Connects_Info[s].Socket_Mutex);			
	}
	return ret;
}

char TestOutcomeConnects()
{	
	int s;
	char ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
		
	for (s = 1; s < iConnMax; s++)
	{
		DBG_MUTEX_LOCK(&Connects_Info[s].Socket_Mutex);
		if ((Connects_Info[s].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[s].Type == CONNECT_CLIENT))
		{
			if (Connects_Info[s].TraffType == TRAFFIC_AUDIO) ret |= 1;
			if ((Connects_Info[s].TraffType == TRAFFIC_FULL_VIDEO) ||
				(Connects_Info[s].TraffType == TRAFFIC_PREV_VIDEO)) ret |= 2;
		}		
		DBG_MUTEX_UNLOCK(&Connects_Info[s].Socket_Mutex);			
	}
	return ret;
}

void ReCalcMaxActConnects()
{
	int n;
	DBG_MUTEX_LOCK(&Network_Mutex);
	Connects_Max_Active = 0;
	for (n = 0; n < Connects_Count; n++)
	{
		DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
		if (Connects_Info[n].Status == CONNECT_STATUS_ONLINE)
			Connects_Max_Active = n;
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
	}
	Connects_Max_Active++;
	DBG_MUTEX_UNLOCK(&Network_Mutex);	
}

void CloseConnect(int iNum, int self)
{
	DBG_LOG_IN();
	dbgprintf(5,"CloseConnect %i, %i\n", iNum, self);
	
	DBG_MUTEX_LOCK(&Connects_Info[iNum].Socket_Mutex);	
	Connects_Info[iNum].Status = CONNECT_STATUS_OFFLINE;
	tx_eventer_send_data(Connects_Info[iNum].pevntIP, 0, NULL, 0, 0, 0);
	if (self) shutdown(Connects_Info[iNum].Socket, SHUT_RDWR);
	close(Connects_Info[iNum].Socket);
	
	DBG_MUTEX_LOCK(&Connects_Info[iNum].Pair_Mutex);
	close(Connects_Info[iNum].socketpair[0]);
	close(Connects_Info[iNum].socketpair[1]);
	Connects_Info[iNum].socketpair[0] = 0;
	Connects_Info[iNum].socketpair[1] = 0;
	DBG_MUTEX_UNLOCK(&Connects_Info[iNum].Pair_Mutex);
	
	if (Connects_Info[iNum].InBuffer != NULL) {DBG_FREE(Connects_Info[iNum].InBuffer);Connects_Info[iNum].InDataSize = 0;}
	if (Connects_Info[iNum].OutBuffer != NULL) {DBG_FREE(Connects_Info[iNum].OutBuffer);Connects_Info[iNum].OutDataSize = 0;}
	
	ResetConnectInfo(&Connects_Info[iNum]);	
	
	DBG_MUTEX_UNLOCK(&Connects_Info[iNum].Socket_Mutex);
	
	DBG_LOG_OUT();	
}

void printmessages(char *labl, char *buff, int ilen)
{
	DBG_LOG_IN();
	
	int ret = 0;
	TRANSFER_DATA *trInHeaderData;
	VIDEO_FRAME_INFO *vVFI;
	VIDEO_SUBFRAME_INFO *vVSFI;
	do
	{
		ret = SearchKeyInData(buff, ilen, ret);
		if (ret != 0)
		{
			trInHeaderData = (TRANSFER_DATA*)&buff[ret-1];
			switch (trInHeaderData->TypeMessage)
					{						
						case TYPE_MESSAGE_EMPTY:
							dbgprintf(3,"(%s)TYPE_MESSAGE_EMPTY\n", labl);	
							break;
						case TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO:
							dbgprintf(3,"(%s)TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO\n", labl);
							break;
						case TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA:
							dbgprintf(3,"(%s)TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA()\n", labl);
							break;
						case TYPE_MESSAGE_REQUEST_VIDEO_STREAM:
							dbgprintf(3,"(%s)TYPE_MESSAGE_START_VIDEO_STREAM\n", labl);
							break;	
						case TYPE_MESSAGE_STOP_VIDEO_STREAM:
							dbgprintf(3,"(%s)TYPE_MESSAGE_START_VIDEO_STREAM\n", labl);
							break;	
						case TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME:
							dbgprintf(3,"(%s)TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME\n", labl);
							break;
						case TYPE_MESSAGE_START_VIDEO_FRAME_DATA:
							dbgprintf(3,"(%s)TYPE_MESSAGE_START_VIDEO_FRAME_DATA\n", labl);
							break;
						case TYPE_MESSAGE_CLOSE:
							dbgprintf(3,"(%s)TYPE_MESSAGE_CLOSE\n", labl);	
							break;
						case TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME:
							dbgprintf(3,"(%s)TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME\n", labl);						
							break;
						case TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA:
							//printf("(>>>>>>>>>) 3. TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA(%i)\n",Connects_Info[0].NeedData);							
							vVFI = (VIDEO_FRAME_INFO*)&buff[ret - 1 + sizeof(TRANSFER_DATA)];	
							dbgprintf(3,"(%s)TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA(%i)\n", labl,vVFI->Number);
							break;						
						case TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA:
							//printf("(>>>>>>>>>) 3. TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA\n");	
							vVSFI = (VIDEO_SUBFRAME_INFO*)&buff[ret-1 + sizeof(TRANSFER_DATA)];	
							dbgprintf(3,"(%s)TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA(%i)(%i)\n", labl,vVSFI->Number,vVSFI->SubNumber);
							break;
						case TYPE_MESSAGE_ALARM_SET:
							dbgprintf(3,"(%s)TYPE_MESSAGE_ALARM_SET\n", labl);
							break;							
						case TYPE_MESSAGE_ALARM_RESET:
							dbgprintf(3,"(%s)TYPE_MESSAGE_ALARM_RESET\n", labl);
							break;							
						default:
							break;				
					}
		}
	} while(ret != 0);
	
	DBG_LOG_OUT();	
}

char* getnametraffic(int TypeTraffic)
{
	switch (TypeTraffic)
	{						
		case TRAFFIC_SERVICE: return "TRAFFIC_SERVICE";
		case TRAFFIC_AUDIO: return "TRAFFIC_AUDIO";	
		case TRAFFIC_FULL_VIDEO: return "TRAFFIC_FULL_VIDEO";
		case TRAFFIC_PREV_VIDEO: return "TRAFFIC_PREV_VIDEO";	
		case TRAFFIC_REMOTE_FILE: return "TRAFFIC_REMOTE_FILE";
		case TRAFFIC_REMOTE_VIDEO: return "TRAFFIC_REMOTE_VIDEO";
		case TRAFFIC_REMOTE_AUDIO: return "TRAFFIC_REMOTE_AUDIO";
		case TRAFFIC_TRANSFER_FILE: return "TRAFFIC_TRANSFER_FILE";		
		default: return "TRAFFIC_UNKNOWN";		
	}		
	return "";
}

char* getnamemessage(int TypeMessage)
{
	switch (TypeMessage)
	{						
		case TYPE_MESSAGE_SKIP: return "SKIP";
		case TYPE_MESSAGE_EMPTY: return "EMPTY";	
		case TYPE_MESSAGE_CLOSE: return "CLOSE";	
		case TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO: return "REQUEST_VIDEO_CODEC_INFO";
		case TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA: return "VIDEO_CODEC_INFO_DATA";
		case TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME: return "REQUEST_START_VIDEO_FRAME";
		case TYPE_MESSAGE_START_VIDEO_FRAME_DATA: return "START_VIDEO_FRAME_DATA";
		case TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME: return "REQUEST_NEXT_VIDEO_FRAME";					
		case TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA: return "NEXT_VIDEO_FRAME_DATA";
		case TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA: return "SUB_VIDEO_FRAME_DATA";
		case TYPE_MESSAGE_ALARM_SET: return "ALARM_SET";
		case TYPE_MESSAGE_ALARM_RESET: return "ALARM_RESET";
		case TYPE_MESSAGE_REQUEST_VIDEO_STREAM: return "START_VIDEO_STREAM";
		case TYPE_MESSAGE_STOP_VIDEO_STREAM: return "START_VIDEO_STREAM";
		case TYPE_MESSAGE_REQUEST_DATETIME: return "REQUEST_DATETIME";
		case TYPE_MESSAGE_DATETIME: return "DATETIME";
		case TYPE_MESSAGE_REQUEST_MODULE_LIST: return "REQUEST_MODULE_LIST";
		case TYPE_MESSAGE_MODULE_LIST: return "MODULE_LIST";
		case TYPE_MESSAGE_MODULE_SET: return "MODULE_SET";
		case TYPE_MESSAGE_REQUEST_AUDIO_STREAM: return "REQUEST_AUDIO_STREAM";
		case TYPE_MESSAGE_STOP_AUDIO_STREAM: return "STOP_AUDIO_STREAM";
		case TYPE_MESSAGE_REQUEST_AUDIO_CODEC_INFO: return "REQUEST_AUDIO_CODEC_INFO";
		case TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA: return "AUDIO_CODEC_INFO_DATA";
		case TYPE_MESSAGE_REQUEST_NEXT_AUDIO_FRAME: return "REQUEST_NEXT_AUDIO_FRAME";
		case TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA: return "NEXT_AUDIO_FRAME_DATA";
		case TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA: return "SUB_AUDIO_FRAME_DATA";
		case TYPE_MESSAGE_MODULE_STATUS: return "MODULE_STATUS";
		case TYPE_MESSAGE_DEVICE_STARTED: return "DEVICE_STARTED";
		case TYPE_MESSAGE_REQUEST_SECURITYLIST: return "REQUEST_SECURITYLIST";
		case TYPE_MESSAGE_SECURITYLIST: return "SECURITYLIST";
		case TYPE_MESSAGE_REQUEST_VIDEO_PARAMS: return "REQUEST_VIDEO_PARAMS";
		case TYPE_MESSAGE_VIDEO_PARAMS: return "VIDEO_PARAMS";
		case TYPE_MESSAGE_TEXT: return "TEXT";
		case TYPE_MESSAGE_REQUEST_MODULE_STATUS: return "REQUEST_MODULE_STATUS";
		case TYPE_MESSAGE_MODULE_STATUS_CHANGED: return "MODULE_STATUS_CHANGED";
		case TYPE_MESSAGE_REQUEST_SYSTEM_INFO: return "REQUEST_SYSTEM_INFO";
		case TYPE_MESSAGE_SYSTEM_SIGNAL: return "SYSTEM_SIGNAL";	
		case TYPE_MESSAGE_SYSTEM_INFO: return "SYSTEM_INFO";	
		case TYPE_MESSAGE_DEVICE_STOPED: return "DEVICE_STOPED";
		case TYPE_MESSAGE_TRAFFIC_TYPE_AUDIO: return "TRAFFIC_TYPE_AUDIO";
		case TYPE_MESSAGE_TRAFFIC_TYPE_FULL_VIDEO: return "TRAFFIC_TYPE_FULL_VIDEO";
		case TYPE_MESSAGE_TRAFFIC_TYPE_PREV_VIDEO: return "TRAFFIC_TYPE_PREV_VIDEO";
		case TYPE_MESSAGE_TRAFFIC_TYPE_SERVICE: return "TRAFFIC_TYPE_SERVICE";
		case TYPE_MESSAGE_FORCE_DATETIME: return "FORCE_DATETIME";
		case TYPE_MESSAGE_MODULE_CHANGED: return "MODULE_CHANGED";
		case TYPE_MESSAGE_DISPLAY_CONTENT: return "DISPLAY_CONTENT";		
		case TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_FILE: return "TRAFFIC_TYPE_REMOTE_FILE";
		case TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_VIDEO: return "TRAFFIC_TYPE_REMOTE_VIDEO";
		case TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_AUDIO: return "TRAFFIC_TYPE_REMOTE_AUDIO";
		case TYPE_MESSAGE_REQUEST_FILELIST: return "REMOTE_FILELIST";		
		case TYPE_MESSAGE_FILELIST: return "FILELIST";
		case TYPE_MESSAGE_FILEPATH: return "FILEPATH";		
		case TYPE_MESSAGE_NEXT_FILELIST: return "NEXT_FILELIST";
		case TYPE_MESSAGE_PREV_FILELIST: return "PREV_FILELIST";
		case TYPE_MESSAGE_GET_PLAY_FILE: return "GET_PLAY_FILE";
		case TYPE_MESSAGE_OPENED_FILE: return "OPENED_FILE";
		case TYPE_MESSAGE_CLOSED_FILE: return "CLOSED_FILE";
		case TYPE_MESSAGE_ERROR_FILE: return "ERROR_FILE";		
		case TYPE_MESSAGE_CHANGE_PLAY_FILE_POS: return "CHANGE_PLAY_FILE_POS";
		case TYPE_MESSAGE_DONE_FILE: return "DONE_FILE";
		case TYPE_MESSAGE_GET_CLOSE_FILE: return "GET_CLOSE_FILE";
		case TYPE_MESSAGE_NEW_FILE_POS: return "NEW_FILE_POS";
		case TYPE_MESSAGE_BUSY_FILE: return "BUSY_FILE";		
		case TYPE_MESSAGE_FREE_FILE: return "FREE_FILE";		
		case TYPE_MESSAGE_FILELIST_LOW_GRID: return "FILELIST_LOW_GRID";
		case TYPE_MESSAGE_FILELIST_HIGH_GRID: return "FILELIST_HIGH_GRID";
		case TYPE_MESSAGE_COPY_FILE_LOW_GRID: return "COPY_FILE_LOW_GRID";
		case TYPE_MESSAGE_COPY_FILE_GET_ORDER: return "COPY_FILE_GET_ORDER";
		case TYPE_MESSAGE_COPY_FILE_RELEASE: return "COPY_FILE_RELEASE";
		case TYPE_MESSAGE_COPY_FILE_DATA: return "COPY_FILE_DATA";
		case TYPE_MESSAGE_COPY_FILE_DONE: return "COPY_FILE_DONE";
		case TYPE_MESSAGE_COPY_FILE_BUSY: return "COPY_FILE_BUSY";
		case TYPE_MESSAGE_COPY_FILE_REJECT: return "COPY_FILE_REJECT";
		case TYPE_MESSAGE_COPY_FILE_HIGH_GRID: return "COPY_FILE_HIGH_GRID";
		default: return "UNKNOWN";
	}		
	return "";
}

void GetConnBufferStatus(unsigned int *pRecv, unsigned int *pSend)
{	
	unsigned int uiRecv = 0;
	unsigned int uiSend = 0;
	int s;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
		
	for (s = 0; s < iConnMax; s++)
	{
		DBG_MUTEX_LOCK(&Connects_Info[s].Socket_Mutex);
		if (Connects_Info[s].Status == CONNECT_STATUS_ONLINE)			
		{
			uiRecv += Connects_Info[s].InDataSize;			
			uiSend += Connects_Info[s].OutDataSize;			
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[s].Socket_Mutex);			
	}
		
	*pRecv = uiRecv;
	*pSend = uiSend;
}

int Init_Servers(void)
{	
	DBG_LOG_IN();	
		
	struct sockaddr_in my_addr;
	struct sockaddr_in my_addr_udp;
	      
    int sock, sock_udp;
    int flags, n;
	int broadcast = 0;
	int iLoop = 1;
	
    if ((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
		dbgprintf(1,"Error create tcp socket(%i) %s\n", errno, strerror(errno));
		DBG_LOG_OUT();
        return 0;
    }
	
	n = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int)) < 0)
		dbgprintf(2, "WEB setsockopt(SO_REUSEADDR) failed\n");

	if ((sock_udp=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)  
    {
		dbgprintf(1,"Error create udp socket(%i) %s\n", errno, strerror(errno));
		DBG_LOG_OUT();
        return 0;
    }  

	n = 1;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int)) < 0)
		dbgprintf(2, "WEB setsockopt(SO_REUSEADDR) failed\n");
	
    memset((char *) &my_addr, 0, sizeof(my_addr));     
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(TCP_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset((char *) &my_addr_udp, 0, sizeof(my_addr_udp));     
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_port = htons(UDP_PORT);
    my_addr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if(bind(sock , (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
    {
        dbgprintf(1,"Error bind tcp socket(%i) %s\n", errno, strerror(errno));
		close(sock);
		close(sock_udp);
        DBG_LOG_OUT();
        return 0;
    }
	
	if(bind(sock_udp , (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp) ) == -1)
    {
        dbgprintf(1,"Error bind udp socket(%i) %s\n", errno, strerror(errno));
        close(sock);
		close(sock_udp);
        DBG_LOG_OUT();
        return 0;
    }  
	
	if (listen(sock, MAX_CONNECTIONS) == -1) 
	{
        dbgprintf(1,"Error listen tcp socket(%i) %s\n", errno, strerror(errno));
        close(sock);
		close(sock_udp);
		DBG_LOG_OUT();
        return 0;
    }
    
	flags = fcntl(sock, F_GETFL, 0);
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(1,"error set tcp nonblock(%i) %s\n", errno, strerror(errno));
		close(sock);
		close(sock_udp);
		DBG_LOG_OUT();
        return 0;
	}
	
	flags = fcntl(sock_udp, F_GETFL, 0);
	if (fcntl(sock_udp, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		dbgprintf(1,"error set udp nonblock(%i) %s\n", errno, strerror(errno));
		close(sock);
		close(sock_udp);
		DBG_LOG_OUT();
        return 0;
	}
	
	if (setsockopt(sock_udp, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) 
	{
        dbgprintf(1,"setsockopt (SO_BROADCAST)(%i) %s\n", errno, strerror(errno));
        close(sock);
		close(sock_udp);
		DBG_LOG_OUT();
        return 0;
    }	
	n = SOCKET_BUFFER_SIZE;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) 
	    dbgprintf(2,"UDP setsockopt (SO_RCVBUF)(%i) %s\n", errno, strerror(errno));
    		
	n = SOCKET_BUFFER_SIZE;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)) == -1) 
	    dbgprintf(2,"UDP setsockopt (SO_SNDBUF)(%i) %s\n", errno, strerror(errno));
    
	int spair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, spair) < 0) 
	{
        dbgprintf(1,"error create socketpair(%i) %s\n", errno, strerror(errno));
        close(sock);
		close(sock_udp);
		DBG_LOG_OUT();
        return 0;
    }
	flags = fcntl(spair[0], F_GETFL, 0);
	if (fcntl(spair[0], F_SETFL, flags | O_NONBLOCK) < 0)
	{
		dbgprintf(1,"error set socketpair nonblock(%i) %s\n", errno, strerror(errno));
		close(sock);
		close(sock_udp);
		close(spair[0]);
		close(spair[1]);
		DBG_LOG_OUT();
        return 0;
	}	
	flags = fcntl(spair[1], F_GETFL, 0);
	if (fcntl(spair[1], F_SETFL, flags | O_NONBLOCK) < 0)
	{
		dbgprintf(1,"error set socketpair nonblock(%i) %s\n", errno, strerror(errno));
		close(sock);
		close(sock_udp);
		close(spair[0]);
		close(spair[1]);
		DBG_LOG_OUT();
        return 0;
	}	
	
	Connects_Max_Active = 1;	
	Connects_Count = 1;
	Connects_Info = (CONNECT_INFO*)DBG_MALLOC(sizeof(CONNECT_INFO)*MAX_CONNECTIONS);
	memset(Connects_Info, 0, sizeof(Connects_Info)*MAX_CONNECTIONS);
	pthread_mutex_init(&Connects_Info[0].Socket_Mutex, NULL);
	pthread_mutex_init(&Connects_Info[0].Pair_Mutex, NULL);
	Connects_Info[0].pevntIP = DBG_MALLOC(sizeof(TX_EVENTER));
	tx_eventer_create(Connects_Info[0].pevntIP, 0);	
	Connects_Info[0].Socket = sock_udp;
	Connects_Info[0].Type = CONNECT_SERVER;
	Connects_Info[0].TraffType = TRAFFIC_SERVICE;
	memcpy(&Connects_Info[0].Addr, &my_addr_udp, sizeof(my_addr_udp));
	Connects_Info[0].Status = CONNECT_STATUS_ONLINE;
	Connects_Info[0].InBufferSize = UDP_TRAFFIC_BUFFER_SIZE;
	Connects_Info[0].OutBufferSize = UDP_TRAFFIC_BUFFER_SIZE;
	Connects_Info[0].InBuffer = (char*)DBG_MALLOC(Connects_Info[0].InBufferSize);
	Connects_Info[0].OutBuffer = (char*)DBG_MALLOC(Connects_Info[0].OutBufferSize);
	Connects_Info[0].InDataSize = 0;
	Connects_Info[0].OutDataSize = 0;	
	Connects_Info[0].socketpair[0] = spair[0];
	Connects_Info[0].socketpair[1] = spair[1];
	GetCurrDateTimeStr(Connects_Info[0].DateConnect, 64);
	
	pthread_create(&threadTrafWork, &tattrTrafWork, TrafWorker, (void*)0);
	
	pthread_create(&threadAcceptServer, &tattrAcceptServer, Accept_Server, (void*)sock);
		
	DBG_LOG_OUT();	
	return iLoop;
}


void* Accept_Server(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());			
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "conn_acceptor");
	
	struct sockaddr_in new_addr;
	
	int sock, new_sock, addr_len;
    int iLoop = 1;
	int flags;
	int sockpair[2];
	sock = (int)pData;
	
	fd_set rfds;
	struct timeval tv;
	int i, n, ret, k;
	
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, Accept_Pair) < 0)
	{	
		dbgprintf(1,"error create socketpair\n");
		Accept_Pair[0] = 0;
		Accept_Pair[1] = 0;
		iLoop = 0;
	}
	else
	{
		flags = fcntl(Accept_Pair[0], F_GETFL, 0);
		if (fcntl(Accept_Pair[0], F_SETFL, flags | O_NONBLOCK) < 0)
		{
			dbgprintf(1,"error set socketpair nonblock\n");
			iLoop = 0;
		}
		else
		{
			flags = fcntl(Accept_Pair[1], F_GETFL, 0);
			if (fcntl(Accept_Pair[1], F_SETFL, flags | O_NONBLOCK) < 0)
			{
				dbgprintf(1,"error set socketpair nonblock\n");
				iLoop = 0;
			}
		}
	}
					
	
	char ReadSignalReady = 1;
	char SignalBuff;
	int max_fd;
	i = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	cThreadAcceptStatus = 1;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	while(iLoop)
    {
		FD_ZERO(&rfds);		
		FD_SET(sock, &rfds);
		max_fd = sock;
		if (ReadSignalReady == 0)
		{
			FD_SET(Accept_Pair[1], &rfds);
			if (Accept_Pair[1] > max_fd) max_fd = Accept_Pair[1];
			tv.tv_sec = 1;
		} else tv.tv_sec = 0;
		
		tv.tv_usec = 0;		
		
		ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		
		if ((ReadSignalReady == 0) && (FD_ISSET(Accept_Pair[1], &rfds))) ReadSignalReady = 1;
		if (ReadSignalReady == 1)
		{	
			ret = read(Accept_Pair[1], &SignalBuff, 1);
			if ((ret > 0) && (SignalBuff == SIGNAL_CLOSE))
			{
				iLoop = 0;
				//printf("SIGNAL_CLOSE %i\n", conn_num);
				dbgprintf(4, "Close Accept signal\n");	
				break;
			}
			else
			{
				if (errno == EAGAIN) ReadSignalReady = 0;
				else 
				{
					dbgprintf(2, "Close accept socketpair (errno:%i, %s)\n",errno, strerror(errno));
					break;
				}
			}
		}
		if (ret != 0)
		{
			if (FD_ISSET(sock, &rfds))
			{
				dbgprintf(5,"Accepting new connect\n");	
				addr_len = sizeof(new_addr);
				new_sock = accept(sock, (struct sockaddr*)&new_addr, (socklen_t *)&addr_len);
				if (new_sock < 0) 
				{
					if (errno == EAGAIN)
					{
						dbgprintf(4,"Error accept tcp socket:(%i) %s\n", errno, strerror(errno));					
					}
					else
					{
						dbgprintf(2,"Error accept tcp socket:(%i) %s\n", errno, strerror(errno));
						iLoop = 0;
					}
				}
				else
				{
					dbgprintf(5,"Accepted new connect\n");			
					DBG_MUTEX_LOCK(&Network_Mutex);
					for (n = 1; n < Connects_Count; n++)
					{
						DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
						if (Connects_Info[n].Status == CONNECT_STATUS_OFFLINE) 
						{
							DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
							break;
						}
						DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);					
					}
					if ((n == Connects_Count) && (Connects_Count >= MAX_CONNECTIONS))
					{
						dbgprintf(1,"Overfull socket buffer\n");										
					}
					else
					{
						if (n == Connects_Count)
						{
							Connects_Count++;
							memset(&Connects_Info[n], 0, sizeof(CONNECT_INFO));
							pthread_mutex_init(&Connects_Info[n].Socket_Mutex, NULL);
							pthread_mutex_init(&Connects_Info[n].Pair_Mutex, NULL);
							Connects_Info[n].pevntIP = DBG_MALLOC(sizeof(TX_EVENTER));
							tx_eventer_create(Connects_Info[n].pevntIP, 0);												
						}
						flags = fcntl(new_sock, F_GETFL, 0);
						i = 0;
						if (fcntl(new_sock, F_SETFL, flags | O_NONBLOCK) < 0) 
									{ i = 1; dbgprintf(1,"error set tcp nonblock\n");}
								
						k = SOCKET_BUFFER_SIZE;
						if (setsockopt(new_sock, SOL_SOCKET, SO_RCVBUF, &k, sizeof(k)) == -1) 
							dbgprintf(2,"SERV error setsockopt (SO_RCVBUF)(%i) %s\n", errno, strerror(errno));
						
						k = SOCKET_BUFFER_SIZE;
						if (setsockopt(new_sock, SOL_SOCKET, SO_SNDBUF, &k, sizeof(k)) == -1) 
							dbgprintf(2,"SERV error setsockopt (SO_SNDBUF)(%i) %s\n", errno, strerror(errno));
						
						if ((i == 0) && (socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair) < 0))
									{ i = 2; dbgprintf(1,"error create socketpair\n");}
						if (i == 0) flags = fcntl(sockpair[0], F_GETFL, 0);
						if ((i == 0) && (fcntl(sockpair[0], F_SETFL, flags | O_NONBLOCK) < 0))
									{ i = 3; dbgprintf(1,"error set socketpair nonblock\n");}
						if (i == 0) flags = fcntl(sockpair[1], F_GETFL, 0);
						if ((i == 0) && (fcntl(sockpair[1], F_SETFL, flags | O_NONBLOCK) < 0))
									{ i = 4; dbgprintf(1,"error set socketpair nonblock\n");}
						if (i == 0)
						{						
							DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
							ResetConnectInfo(&Connects_Info[n]);
							
							DBG_MUTEX_LOCK(&Connects_Info[n].Pair_Mutex);
							Connects_Info[n].socketpair[0] = sockpair[0];
							Connects_Info[n].socketpair[1] = sockpair[1];
							DBG_MUTEX_UNLOCK(&Connects_Info[n].Pair_Mutex);
							
							Connects_Info[n].Socket = new_sock;
							memcpy(&Connects_Info[n].Addr, &new_addr, addr_len);
							Connects_Info[n].Type = CONNECT_SERVER;
							Connects_Info[n].Status = CONNECT_STATUS_ONLINE;
							Connects_Info[n].ClientPort = new_addr.sin_port;
							
							Connects_Info[n].InBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
							Connects_Info[n].InBuffer = (char*)DBG_MALLOC(Connects_Info[n].InBufferSize);
							Connects_Info[n].InDataSize = 0;
							
							Connects_Info[n].OutBufferSize = SERVICE_TRAFFIC_BUFFER_SIZE;
							Connects_Info[n].OutBuffer = (char*)DBG_MALLOC(Connects_Info[n].OutBufferSize);
							Connects_Info[n].OutDataSize = 0;
							
							GetCurrDateTimeStr(Connects_Info[n].DateConnect, 64);
													
							DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
							
							if (Connects_Max_Active < (n + 1)) Connects_Max_Active = n + 1;
							
							pthread_create(&threadTrafWork, &tattrTrafWork, TrafWorker, (void*)n);	
						}
						else
						{
							close(new_sock);
							if (i > 1)
							{
								close(sockpair[0]);
								close(sockpair[1]);
							}
							DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);
							ResetConnectInfo(&Connects_Info[n]);
							DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);	
						}
					}
					DBG_MUTEX_UNLOCK(&Network_Mutex);
				}
			}	
		}
	} 
	
    close(sock);
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	if (Accept_Pair[0] != 0) close(Accept_Pair[0]);
	if (Accept_Pair[1] != 0) close(Accept_Pair[1]);
	Accept_Pair[0] = 0;
	Accept_Pair[1] = 0;
	cThreadAcceptStatus = 0;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	DBG_LOG_OUT();
	dbgprintf(4, "Closed AcceptThread\n");	
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return 0;	
}

void* TrafWorker(void *pData)
{	
	dbgprintf(5, "Create new Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());			
	DBG_LOG_IN();
	
	pthread_setname_np(pthread_self(), "traff_reworker");
	
	unsigned int conn_num = (unsigned int)pData;
    
	struct sockaddr_in my_addr;
	struct sockaddr_in addr;
	struct sockaddr_in siBroadAddress;
	memset(&siBroadAddress, 0, sizeof(struct sockaddr_in));
	int iBroadcast = 1;
	int64_t iBrTm = 0;
	if (conn_num == 0) 
	{
		iBroadcast = GetBroadcastAddr(&siBroadAddress);
		if (iBroadcast == 0) get_ms(&iBrTm);
	}
	int broadcast = 0;
	
	TRANSFER_DATA *trInHeaderData;
	TRANSFER_DATA *trOutHeaderData;
	     
    int addr_len;
	unsigned int uiRecvCnt = 0;
	unsigned int uiRecvCnt2 = 0;
	unsigned int uiSendCnt = 0;
	
	DBG_MUTEX_LOCK(&Connects_Info[conn_num].Socket_Mutex);
	int sock = Connects_Info[conn_num].Socket;
	Connects_Info[conn_num].RecvedBytes = 0;
	Connects_Info[conn_num].SendedBytes = 0;
	DBG_MUTEX_LOCK(&Connects_Info[conn_num].Pair_Mutex);
	int sockpair = Connects_Info[conn_num].socketpair[1];
	DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Pair_Mutex);
	unsigned int InDataSize = Connects_Info[conn_num].InDataSize;
	unsigned int InBufferSize = Connects_Info[conn_num].InBufferSize;
	memcpy(&my_addr, &Connects_Info[conn_num].Addr, sizeof(my_addr));
	DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);
	
	addr_len = sizeof(my_addr);
	int max_fd = -1;
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	int ret, tof, i;
	
	i = 0;
	char ReadSignalReady = 1;	
	char SignalBuff;
	char ReadReady = 1;	
	char WriteReady = 1;
	unsigned int uiLastError = 0;
	int iTimer = 0;
	int iError = 0;
	while(1)
    {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		max_fd = -1;
		i = 0;	
		ret = 0;
		if (ReadSignalReady == 0)
		{
			FD_SET(sockpair, &rfds);
			if (sockpair > max_fd) max_fd = sockpair;				
		} else i = 3;
		if (InDataSize < InBufferSize)
		{
			ret++;
			if (ReadReady == 0)
			{
				i++;
				FD_SET(sock, &rfds);
				if (sock > max_fd) max_fd = sock;
			}	
		}
		DBG_MUTEX_LOCK(&Connects_Info[conn_num].Socket_Mutex);
		Connects_Info[conn_num].Timer = iTimer;
		Connects_Info[conn_num].RecvedBytes = uiRecvCnt2;		
		if (Connects_Info[conn_num].OutDataSize != 0)
		{
			ret++;
			if (WriteReady == 0)
			{
				i++;
				FD_SET(sock, &wfds);
				if (sock > max_fd) max_fd = sock;
				i++;
			}
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);	
		
		if ((iBroadcast == 0) && (conn_num == 0)) 
		{
			if ((unsigned int)get_ms(&iBrTm) > 180000) 
			{
				iBroadcast = GetBroadcastAddr(&siBroadAddress);			
				if (iBroadcast == 0)
				{
					iBrTm = 0;
					get_ms(&iBrTm);
				}
			}
		}
		
		if (ret == i) tv.tv_sec = 1; else tv.tv_sec = 0;
		tv.tv_usec = 100;		
		
		tof = select(max_fd + 1, &rfds, &wfds, NULL, &tv);
		//if ((ret == i) && (conn_num != 0) && (tof == 0)) printf("select2 %i, %i, %i, %i\n",conn_num, ret,i, Connects_Info[conn_num].Type);
		//if (ret == 0) dbgprintf(5, "Timeout Recv data %i\n", i);// else dbgprintf(5, "Select Recv data %i\n", i);   
		//if (tof == 0) tof = 4; else 
		if (i > 2) tof = 4; else tof = 0;
		
		if ((ReadSignalReady == 0) && (FD_ISSET(sockpair, &rfds))) ReadSignalReady = 1;
		if ((ReadReady == 0) && (FD_ISSET(sock, &rfds))) ReadReady = 1;
		if ((WriteReady == 0) && (FD_ISSET(sock, &wfds))) WriteReady = 1;
		if (ReadSignalReady == 1)
		{	
			do
			{
				ret = read(sockpair, &SignalBuff, 1);
				if (ret > 0) 
				{
					if (SignalBuff == SIGNAL_CLOSE)
					{
						iError = 2;
						//printf("SIGNAL_CLOSE %i\n", conn_num);
						dbgprintf(4, "Close connect signal %i\n",conn_num);	
						break;
					}
					if (SignalBuff == SIGNAL_WORK) 
					{
						//printf("SIGNAL_WORK %i\n", conn_num);
						iTimer = 0;
						tof |= 2;
					}	
					if (SignalBuff == SIGNAL_SEND)
					{
						iTimer = 0;
						tof |= 4;
					}
					if ((SignalBuff != SIGNAL_WORK) && (SignalBuff != SIGNAL_CLOSE) && (SignalBuff != SIGNAL_SEND)) 
						dbgprintf(2, "TrafWork: unknown signal %i\n", SignalBuff);
				}
				else
				{
					if (errno == EAGAIN) 
					{
						ReadSignalReady = 0;
						//printf("ReadSignalReady EAGAIN %i\n", conn_num);
						break;
					}
					else 
					{
						iError = 2;
						dbgprintf(2, "Close socketpair %i (errno:%i, %s)\n",conn_num, errno, strerror(errno));	
						break;
					}
				}
			} while(1);
			if (iError == 2) break;
		}
	
		uiRecvCnt = 0;	
		uiSendCnt = 0;
		
		if (InDataSize >= InBufferSize) tof |= 2;
		if ((ReadReady == 1) && (InDataSize < InBufferSize))
		{	
			tof |= 1;
			iTimer = 0;
			if (conn_num != 0) 
			{	
				ret = read(sock, &Connects_Info[conn_num].InBuffer[InDataSize], InBufferSize - InDataSize);
				if (ret > 0) 
				{
					InDataSize += ret;
					uiRecvCnt += ret;
					uiRecvCnt2 += ret;
					//printf("to inside %i >>> %i (total:%i)\n", conn_num, ret, InDataSize);
					tof |= 2;
				}
				if (ret <= 0) 
				{
					if (errno == EAGAIN) ReadReady = 0;
					if ((ret == 0) || (errno != EAGAIN))
					{
						if (ret != 0) dbgprintf(4, "Close connect from Recv thread TCP %i (errno:%i, %s)\n",conn_num, errno, strerror(errno));
							else dbgprintf(4, "Close connect from Recv thread TCP %i (closed)\n",conn_num);	
						iError = 1;
						break;
					}							
				}					
			}
			else
			{
				ret = recvfrom(sock, &Connects_Info[conn_num].InBuffer[InDataSize], InBufferSize - InDataSize, 0, (struct sockaddr *) &addr, (socklen_t*)&addr_len);								
				if (ret <= 0) 
				{
					if (errno == EAGAIN) ReadReady = 0;
					else
					{
						iError = 1;
						break;
					}
				}
				if (ret > 0)
				{
					//printf("Recv udp %i(%s:%d)\n", ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));								
					i += ret;
					if (ret >= sizeof(TRANSFER_DATA))
					{
						trInHeaderData = (TRANSFER_DATA*)(&Connects_Info[conn_num].InBuffer[InDataSize]);
						if ((trInHeaderData->Key1 == TRANSFER_KEY1) 
							&& (trInHeaderData->Key2 == TRANSFER_KEY2) 
							&& (trInHeaderData->Key3 == TRANSFER_KEY3))
						{
							memcpy(&trInHeaderData->Address, &addr, sizeof(trInHeaderData->Address));
							/*printf("(recv)TYPE_MESSAGE_(%i)\n",trInHeaderData->TypeMessage);
							if (trInHeaderData->TypeMessage == TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA)
							{
								VIDEO_SUBFRAME_INFO *vVSFI = (VIDEO_SUBFRAME_INFO*)&Connects_Info[n].InBuffer[Connects_Info[n].InDataSize+sizeof(TRANSFER_DATA)];	
								dbgprintf(3,"(recv)TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA(%i)(%i)\n",vVSFI->Number,vVSFI->SubNumber);							
							}*/
						} else dbgprintf(4,"RECIEVE BAD UDP PACKET\n");
					}
					InDataSize += ret;
					uiRecvCnt += ret;
					uiRecvCnt2 += ret;
					tof |= 2;
				}								
				//printf("Recv udp %i(%s:%d)\n", i, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			}
		}			
		
		if (WriteReady)
		{	
			if (DBG_MUTEX_TRYLOCK(&Connects_Info[conn_num].Socket_Mutex) == 0)
			{
				if (Connects_Info[conn_num].OutDataSize != 0)
				{				
					tof |= 1;
					iTimer = 0;
					if (conn_num != 0) 
					{	
						ret = write(sock, Connects_Info[conn_num].OutBuffer, Connects_Info[conn_num].OutDataSize);
						if (ret <= 0) 
						{
							if (errno != EAGAIN)
							{
								if (ret != 0) dbgprintf(4, "Close connect from Send thread TCP %i (errno:%i, %s)\n",conn_num, errno, strerror(errno));
									else dbgprintf(4, "Close connect from Send thread TCP %i (closed)\n",conn_num);	
								DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);							
								iError = 1;
								break;
							} 
							else WriteReady = 0;
						}
						else
						{
							//printf("Send tcp %i %i /%i\n", conn_num, Connects_Info[conn_num].OutDataSize, ret);
							Connects_Info[conn_num].OutDataSize -= ret;
							//printf("to outside %i >>> %i (total:%i)\n", conn_num, ret, Connects_Info[conn_num].OutDataSize);
							uiSendCnt += ret;
							Connects_Info[conn_num].SendedBytes += ret;
							
							if (Connects_Info[conn_num].OutDataSize != 0)
							{
								memmove(Connects_Info[conn_num].OutBuffer, &Connects_Info[conn_num].OutBuffer[ret], Connects_Info[conn_num].OutDataSize);
								//printf("Sended %i/%i\n", ret,Connects_Info[conn_num].OutDataSize);
							}					
						}
					}
					else
					{
						i = UDP_PACKET_MAX_SEND;
						do
						{
							ret = SearchKeyInData(Connects_Info[conn_num].OutBuffer, Connects_Info[conn_num].OutDataSize, 0);								
							if (ret != 0)
							{
								ret -= 1;
								if (ret != 0)
								{	
									Connects_Info[conn_num].OutDataSize -= ret;
									memmove(Connects_Info[conn_num].OutBuffer, &Connects_Info[conn_num].OutBuffer[ret], Connects_Info[conn_num].OutDataSize);
								}
								trOutHeaderData = (TRANSFER_DATA*)(Connects_Info[conn_num].OutBuffer);
								if (trOutHeaderData->SizeMessage <= Connects_Info[conn_num].OutDataSize)
								{
									if ((trOutHeaderData->Address.sin_addr.s_addr == 0) && (broadcast == 0))
									{
										broadcast = 1;
										if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) 
											dbgprintf(1,"setsockopt (SO_BROADCAST) to 1\n");																		
									}
									if ((trOutHeaderData->Address.sin_addr.s_addr != 0) && (broadcast == 1))
									{
										broadcast = 0;
										if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) 
											dbgprintf(1,"setsockopt (SO_BROADCAST) to 0\n");																	
									}
									if (trOutHeaderData->Address.sin_addr.s_addr == 0)
											trOutHeaderData->Address.sin_addr = siBroadAddress.sin_addr;
									memcpy(&addr, &trOutHeaderData->Address, sizeof(addr));
									memset(&trOutHeaderData->Address, 0, sizeof(trOutHeaderData->Address));
									//printf("Send udp %i (%s:%d)\n", trOutHeaderData->SizeMessage, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
									ret = sendto(sock, Connects_Info[conn_num].OutBuffer, trOutHeaderData->SizeMessage, 0, (struct sockaddr*) &addr, sizeof(addr));
									if (ret == trOutHeaderData->SizeMessage)
									{
										uiLastError = 0;
										//k += ret;
										//printf("Send udp %i\n", k);
										//printf("Send udp %i(%s:%d)\n", ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
										Connects_Info[conn_num].OutDataSize -= ret;
										uiSendCnt += ret;
										Connects_Info[conn_num].SendedBytes += ret;
										memmove(Connects_Info[conn_num].OutBuffer, &Connects_Info[conn_num].OutBuffer[ret], Connects_Info[conn_num].OutDataSize);										
									} 
									else 
									{
										if (uiLastError != errno)
										{
											uiLastError = errno;
											dbgprintf(1, "Send udp error(%s:%d) errno:%i(%s)\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), errno, strerror(errno));
										}
										Connects_Info[conn_num].OutDataSize -= trOutHeaderData->SizeMessage;
										memmove(Connects_Info[conn_num].OutBuffer, &Connects_Info[conn_num].OutBuffer[trOutHeaderData->SizeMessage], Connects_Info[conn_num].OutDataSize);
										ret = 0;										
									}
									i--;
								} else dbgprintf(1,"Small data to send full packet udp\n");
							} else Connects_Info[conn_num].OutDataSize = 0;								
							if ((ret < 0) && (errno != EAGAIN))
							{
								dbgprintf(2, "Close connect from Send thread UDP %i (errno:%i, %s)\n", conn_num, errno, strerror(errno));	
								DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);
								iError = 1;
								break;
							}	
							//if ((ret < 0) && (errno == EAGAIN))printf("4\n");						
							//if ((ret > 0) && (Connects_Info[conn_num].OutDataSize != 0) && (i != 0)) usleep(100);
						} while ((ret > 0) && (Connects_Info[conn_num].OutDataSize != 0) && (i != 0));	
						//printf("Sended udp total %i(%s:%d)\n", k, inet_ntoa(my_addr_udp.sin_addr), ntohs(my_addr_udp.sin_port));										
					}
				}
				DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);		
			}
		}
			
		if (conn_num && (tof == 0))
		{
			iTimer++;
			if (iTimer >= CONNECT_TIMEOUT) 
			{
				dbgprintf(5, "Soft Timeout Close connect from Recv thread %i\n",conn_num);	
				iError = 2;
				break;			
			}			
		}
		if ((tof & 2) && InDataSize)
		{
			DBG_MUTEX_LOCK(&Connects_Info[conn_num].Socket_Mutex);
			Connects_Info[conn_num].InDataSize = InDataSize;
			if (ReWorkTraffic(conn_num) == 1) 
			{
				iError = 2;
				DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);
				break;
			}
			InDataSize = Connects_Info[conn_num].InDataSize;	
			InBufferSize = Connects_Info[conn_num].InBufferSize;
			DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);
		}
				
		DBG_MUTEX_LOCK(&Network_Mutex);
		uiRecvDataCnt += uiRecvCnt;
		uiSendDataCnt += uiSendCnt;
		DBG_MUTEX_UNLOCK(&Network_Mutex);
	} 
	
	DBG_MUTEX_LOCK(&Connects_Info[conn_num].Socket_Mutex);
	ret = Connects_Info[conn_num].TraffType;
	i = Connects_Info[conn_num].Type;
	DBG_MUTEX_UNLOCK(&Connects_Info[conn_num].Socket_Mutex);
	DBG_MUTEX_LOCK(&systemlist_mutex);
	unsigned int uiSysID = GetLocalSysID();
	DBG_MUTEX_UNLOCK(&systemlist_mutex);							
	
	if (i == CONNECT_SERVER)
	{
		if ((ret == TRAFFIC_PREV_VIDEO) ||
			(ret == TRAFFIC_FULL_VIDEO)) AddModuleEventInList(uiSysID, 15, SYSTEM_CMD_CLOSED_CAMERA, NULL, 0, 0);
		if (ret == TRAFFIC_AUDIO) AddModuleEventInList(uiSysID, 15, SYSTEM_CMD_CLOSED_MIC, NULL, 0, 0);
	}
	
    dbgprintf(5, "Exit thread Recv_server: Close connect %i\n",conn_num);	
	if (iError == 2) 
		CloseConnect(conn_num, 1);
		else
		CloseConnect(conn_num, 0);
	
	ReCalcMaxActConnects();	
	
	DBG_LOG_OUT();
	dbgprintf(5, "Exit from Thread: '%s', TID: %i, SID: %i\n", __func__, (unsigned int)pthread_self(), gettid());		
	return 0;	
}

void GetTransferDataCnt(unsigned int *pRecv, unsigned int *pSend)
{
	DBG_MUTEX_LOCK(&Network_Mutex);
	*pRecv = uiRecvDataCnt;
	*pSend = uiSendDataCnt;
	uiRecvDataCnt = 0;
	uiSendDataCnt = 0;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
}

int ReWorkTraffic(int Num)
{
	DBG_LOG_IN();
	
	TRANSFER_DATA *trInHeaderData;
	struct sockaddr_in Address;
	unsigned int *pCRC;
	unsigned char cnt;
	void *vData;
	int iLoop = 1;
	int ret, i, k;
	unsigned int *puiData;
	unsigned int uiData[10];
	char *pData;
	char cRecv = 0;
	int result = 0;
	unsigned int uiBodyLen;
	
	//VIDEO_FRAME_INFO *vVFI;
	VIDEO_SUBFRAME_INFO *vVSFI;
	MODULE_INFO *tMI;
	MODULE_INFO	*tMI_temp;
	SYSTEM_INFO *tSI;
	SECURITY_KEY_INFO* tSKI;
	
	char *trInBuffer = Connects_Info[Num].InBuffer;
	//int recv_len = Connects_Info[Num].InDataSize;
	int iPos = 0;
	//printf("ReWorkTraffic START %i, %i\n", Connects_Info[Num].InDataSize, iPos);	
	while (iLoop)
	{
		//printf("ReWorkTraffic IN %i, %i\n", Connects_Info[Num].InDataSize, iPos);	
		trInBuffer = &Connects_Info[Num].InBuffer[iPos];
		
		ret = SearchKeyInData(trInBuffer, Connects_Info[Num].InDataSize - iPos, 0);
		if (ret == 0)
		{			
			if ((Connects_Info[Num].InDataSize - iPos) > 11) 
			{
				dbgprintf(3,"Bad data skiping %i bytes\n",(Connects_Info[Num].InDataSize - iPos) -11);			
				iPos = Connects_Info[Num].InDataSize - 11;	
			} else dbgprintf(5,"No header packet (small data) connnum: %i, datasize: %i\n", Num, Connects_Info[Num].InDataSize - iPos);	
			//iLoop = 0;		
		}
		else
		{
			if (ret > 1)
			{
				ret -= 1;
				iPos += ret;
				trInBuffer+=ret;
				dbgprintf(3,"Bad data skiping2 %i bytes\n", ret);
				//debug_dumpdata("Skiped", iPos, ret, Connects_Info[Num].InBuffer, Connects_Info[Num].InDataSize);
			}
			if ((Connects_Info[Num].InDataSize - iPos) >= sizeof(TRANSFER_DATA)) //&& ((iBufferSize - Connects_Info[Num].OutDataSize) > (sizeof(TRANSFER_DATA)*2)))
			{
				trInHeaderData = (TRANSFER_DATA*)trInBuffer;
				if (Num == 0) memcpy(&Address, &trInHeaderData->Address, sizeof(Address));
					else memcpy(&Address, &Connects_Info[Num].Addr, sizeof(Address));
				Address.sin_port = htons(UDP_PORT);
				memset(&trInHeaderData->Address, 0, sizeof(Address));
				if (trInHeaderData->SizeMessage <= (Connects_Info[Num].InDataSize - iPos)) pCRC = (unsigned int*)&trInBuffer[trInHeaderData->SizeMessage - sizeof(int)];
				if (
					(trInHeaderData->Key1 == TRANSFER_KEY1) 
					&& (trInHeaderData->Key2 == TRANSFER_KEY2) 
					&& (trInHeaderData->Key3 == TRANSFER_KEY3)
					&& (trInHeaderData->SizeMessage <= (Connects_Info[Num].InDataSize - iPos))
					//&& (CalcCRC(trInBuffer, trInHeaderData->SizeMessage - sizeof(int)) == *pCRC)
					)
				{					
					//printf("(server) 2. message is valid from : %s, type : %i(%s)\n", inet_ntoa(Address.sin_addr), trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
					//printf("%i message is %s\n", trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
					memcpy(&trInHeaderData->Address, &Address, sizeof(Address));				
					cRecv = 1;	
					if (((Num == 0) &&					
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_MODULE_STATUS_CHANGED) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_DEVICE_STARTED) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_DEVICE_STOPED) &&						
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_REQUEST_SYSTEM_INFO) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_SYSTEM_SIGNAL) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_MODULE_SET) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_TEXT) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_FORCE_DATETIME) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_MODULE_CHANGED) &&
						(trInHeaderData->TypeMessage != TYPE_MESSAGE_CHANGED_SECURITYLIST))
						||
						((Num != 0) &&
						((trInHeaderData->TypeMessage == TYPE_MESSAGE_DEVICE_STARTED) ||
						(trInHeaderData->TypeMessage == TYPE_MESSAGE_DEVICE_STOPED) ||
						(trInHeaderData->TypeMessage == TYPE_MESSAGE_SYSTEM_SIGNAL) ||
						(trInHeaderData->TypeMessage == TYPE_MESSAGE_MODULE_CHANGED) ||
						(trInHeaderData->TypeMessage == TYPE_MESSAGE_CHANGED_SECURITYLIST))))
						{
							dbgprintf(1, "This message different from wanted type:%i(%s) num:%i connection\n", trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage), Num);	
							//trInHeaderData->TypeMessage = TYPE_MESSAGE_SKIP;														
						}
					uiBodyLen = trInHeaderData->SizeMessage - (sizeof(TRANSFER_DATA) + sizeof(int));
					switch (trInHeaderData->TypeMessage)
					{						
						case TYPE_MESSAGE_SKIP:
							//printf("(server) 3. TYPE_MESSAGE_SKIP\n");	
							break;
						case TYPE_MESSAGE_EMPTY:
							//printf("(server) 3. TYPE_MESSAGE_EMPTY\n");	
							break;
						case TYPE_MESSAGE_DEVICE_STARTED:
							//printf("(server) 3. TYPE_MESSAGE_DEVICE_STARTED\n");
							if (uiBodyLen == sizeof(unsigned int)) 
							{	
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								AddModuleEventInList(puiData[0], 15, SYSTEM_CMD_NEW, NULL, 0, 0);
								DBG_MUTEX_LOCK(&systemlist_mutex);
								if (GetLocalSysID() != puiData[0]) ret = 1; else ret = 0;
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								if (ret == 1)
								{
									dbgprintf(3,"New device online detected %s\n",inet_ntoa(trInHeaderData->Address.sin_addr));								
									vData = DBG_MALLOC(sizeof(SYSTEM_INFO));							
									DBG_MUTEX_LOCK(&systemlist_mutex);	
									memcpy(vData, GetLocalSysInfo(), sizeof(SYSTEM_INFO));
									DBG_MUTEX_UNLOCK(&systemlist_mutex);
									DBG_MUTEX_UNLOCK(&Connects_Info[Num].Socket_Mutex);
									SendTCPMessage(TYPE_MESSAGE_REQUEST_SYSTEM_INFO, NULL, 0, NULL, 0, &trInHeaderData->Address);
									SendTCPMessage(TYPE_MESSAGE_SYSTEM_INFO, (char*)vData, sizeof(SYSTEM_INFO), NULL, 0, &trInHeaderData->Address);
									SendTCPMessage(TYPE_MESSAGE_REQUEST_SECURITYLIST, NULL, 0, NULL, 0, &trInHeaderData->Address);
									DBG_MUTEX_LOCK(&Connects_Info[Num].Socket_Mutex);
									DBG_FREE(vData);									
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;						
						case TYPE_MESSAGE_DEVICE_STOPED:
							//printf("(server) 3. TYPE_MESSAGE_DEVICE_STOPED\n");
							if (uiBodyLen == sizeof(unsigned int)) 
							{	
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								AddModuleEventInList(puiData[0], 15, SYSTEM_CMD_EXIT, NULL, 0, 0);
								DBG_MUTEX_LOCK(&systemlist_mutex);
								if (GetLocalSysID() != puiData[0]) ret = 1; else ret = 0;
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								if (ret == 1)
								{
									dbgprintf(3,"Device exited %s\n",inet_ntoa(trInHeaderData->Address.sin_addr));
									ClearSecurityKey(puiData[0]);
									ClearModuleList(puiData[0]);
									ClearSystemList(puiData[0]);
									DBG_MUTEX_LOCK(&modulelist_mutex);
									SortModules(miModuleList, iModuleCnt);
									DBG_MUTEX_UNLOCK(&modulelist_mutex);										
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;						
						case TYPE_MESSAGE_SYSTEM_SIGNAL:
							//dbgprintf(5,"TYPE_MESSAGE_SYSTEM_SIGNAL: %s\n", inet_ntoa(trInHeaderData->Address.sin_addr));
							if (uiBodyLen == sizeof(unsigned int)) 
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								ret = 1;
								DBG_MUTEX_LOCK(&systemlist_mutex);
								if (GetLocalSysID() != puiData[0])
								{
									ret = 0;
									for (cnt = 0; cnt != iSystemListCnt; cnt++) if (miSystemList[cnt].ID == puiData[0]) 
									{
										ret = 1; 
										miSystemList[cnt].Active = MAX_SYSTEM_INFO_LIVE;
										break;
									}									
								}
								DBG_MUTEX_UNLOCK(&systemlist_mutex);	
								if (ret == 0) 
								{
									dbgprintf(4,"SYSTEM FINDED: %s\n", inet_ntoa(trInHeaderData->Address.sin_addr));
									AddModuleEventInList(puiData[0], 15, SYSTEM_CMD_FIND, NULL, 0, 0);
									ClearSecurityKey(puiData[0]);
									ClearModuleList(puiData[0]);
									DBG_MUTEX_LOCK(&modulelist_mutex);
									SortModules(miModuleList, iModuleCnt);
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									
									if (Num == 0) 
									{
										DBG_MUTEX_UNLOCK(&Connects_Info[Num].Socket_Mutex);									
										SendTCPMessage(TYPE_MESSAGE_REQUEST_SYSTEM_INFO, NULL, 0, NULL, 0, &trInHeaderData->Address);
										SendTCPMessage(TYPE_MESSAGE_REQUEST_SECURITYLIST, NULL, 0, NULL, 0, &trInHeaderData->Address);	
										DBG_MUTEX_LOCK(&Connects_Info[Num].Socket_Mutex);									
									}
									else
									{
										SendMessage(Num, TYPE_MESSAGE_REQUEST_SYSTEM_INFO, NULL, 0, NULL, 0, &trInHeaderData->Address);
										SendMessage(Num, TYPE_MESSAGE_REQUEST_SECURITYLIST, NULL, 0, NULL, 0, &trInHeaderData->Address);										
									}
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;						
						case TYPE_MESSAGE_REQUEST_SYSTEM_INFO:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_SYSTEM_INFO\n");	
							if (uiBodyLen == 0) 
							{
								vData = DBG_MALLOC(sizeof(SYSTEM_INFO));
								DBG_MUTEX_LOCK(&systemlist_mutex);	
								memcpy(vData, GetLocalSysInfo(), sizeof(SYSTEM_INFO));
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								if (Num == 0)
								{
									DBG_MUTEX_UNLOCK(&Connects_Info[Num].Socket_Mutex);									
									SendTCPMessage(TYPE_MESSAGE_SYSTEM_INFO, (char*)vData, sizeof(SYSTEM_INFO), NULL, 0, &trInHeaderData->Address);
									DBG_MUTEX_LOCK(&Connects_Info[Num].Socket_Mutex);																			
								}
								else SendMessage(Num, TYPE_MESSAGE_SYSTEM_INFO, (char*)vData, sizeof(SYSTEM_INFO), NULL, 0, &trInHeaderData->Address);									
								DBG_FREE(vData);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));
							break;
						case TYPE_MESSAGE_SYSTEM_INFO:
							//printf("(server) 3. TYPE_MESSAGE_SYSTEM_INFO\n");	
							if (uiBodyLen == sizeof(SYSTEM_INFO))
							{
								tSI = (SYSTEM_INFO*)(trInBuffer + sizeof(TRANSFER_DATA));								
								DBG_MUTEX_LOCK(&systemlist_mutex);
								ret = 1;								
								SYSTEM_INFO *sysinfo = GetLocalSysInfo();								
								//printf("(server) 3. TYPE_MESSAGE_SYSTEM_INFO %.4s  %.4s\n", &sysinfo->ID, &tSI->ID);	
								if (sysinfo->ID != tSI->ID)
								{
									if (sysinfo->IntVersion[0] != tSI->IntVersion[0])
										dbgprintf(3, "Detected other version system ID:%.4s IP:%s, skiped\n", (char*)&tSI->ID, inet_ntoa(trInHeaderData->Address.sin_addr));
										else ret = 0;	
										for (cnt = 0; cnt != iSystemListCnt; cnt++) 
											if (miSystemList[cnt].ID == tSI->ID) 
											{
												if (miSystemList[cnt].Address.sin_addr.s_addr != trInHeaderData->Address.sin_addr.s_addr)
													dbgprintf(2, "Detected dublicate system ID:%.4s IP:%s\n", (char*)&tSI->ID, inet_ntoa(trInHeaderData->Address.sin_addr));
												//if (miSystemList[cnt].Address.sin_addr.s_addr != trInHeaderData->Address.sin_addr.s_addr) 
												//{
													memcpy(&miSystemList[cnt],tSI,sizeof(SYSTEM_INFO));	
													memcpy(&miSystemList[cnt].Address, &trInHeaderData->Address, sizeof(trInHeaderData->Address));	
													miSystemList[cnt].Active = MAX_SYSTEM_INFO_LIVE;
													miSystemList[cnt].Local = 0;
													ret = 2;
												//} else ret = 1; 
												break;
											} 								
										if ((ret != 2) && (iSystemListCnt < MAX_SYSTEM_INFO_COUNT))
										{
											iSystemListCnt++;
											miSystemList = (SYSTEM_INFO*)DBG_REALLOC(miSystemList, iSystemListCnt*sizeof(SYSTEM_INFO));
											memcpy(&miSystemList[iSystemListCnt-1],tSI,sizeof(SYSTEM_INFO));	
											memcpy(&miSystemList[iSystemListCnt-1].Address, &trInHeaderData->Address, sizeof(trInHeaderData->Address));	
											miSystemList[iSystemListCnt-1].Active = MAX_SYSTEM_INFO_LIVE;
											miSystemList[iSystemListCnt-1].Local = 0;
										}
								} 
								else 
								{
									ret = 1;
									if (sysinfo->Address.sin_addr.s_addr != trInHeaderData->Address.sin_addr.s_addr)
										dbgprintf(2, "Detected concurent system ID:%.4s IP:%s\n", (char*)&tSI->ID, inet_ntoa(trInHeaderData->Address.sin_addr));								
								}
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								if (ret != 1) SendMessage(Num, TYPE_MESSAGE_REQUEST_MODULE_LIST, NULL, 0, NULL, 0, &trInHeaderData->Address);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_REQUEST_MODULE_LIST:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_MODULE_LIST\n");	
							if (uiBodyLen == 0) 
							{
								DBG_MUTEX_LOCK(&modulelist_mutex);
								MODULE_INFO_TRANSFER *tMIT = (MODULE_INFO_TRANSFER*)DBG_MALLOC(sizeof(MODULE_INFO_TRANSFER) * iModuleCnt);								
								ret = iModuleCnt;	
								for (cnt = 0; cnt < ret; cnt++)	memcpy(&tMIT[cnt], &miModuleList[cnt], sizeof(MODULE_INFO_TRANSFER));
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								k = 1;
								for (cnt = 0; cnt < ret; cnt++)
								{
									if ((tMIT[cnt].Enabled & 1) && (tMIT[cnt].Local == 1))
									{
										if (Num == 0) 
										{
											DBG_MUTEX_UNLOCK(&Connects_Info[Num].Socket_Mutex);
											SendTCPMessage(TYPE_MESSAGE_MODULE_LIST, (char*)&k, sizeof(k), (char*)&tMIT[cnt], sizeof(MODULE_INFO_TRANSFER), &trInHeaderData->Address); 
											DBG_MUTEX_LOCK(&Connects_Info[Num].Socket_Mutex);
										}
										else 
										SendMessage(Num, TYPE_MESSAGE_MODULE_LIST, (char*)&k, sizeof(k), (char*)&tMIT[cnt], sizeof(MODULE_INFO_TRANSFER), &trInHeaderData->Address);																			
									}	
								}						
								DBG_FREE(tMIT);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));
							break;
						case TYPE_MESSAGE_MODULE_LIST:
							//printf("(server) 3. TYPE_MESSAGE_MODULE_LIST\n");		
							if (uiBodyLen >= sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								ret = uiBodyLen - sizeof(int);
								//printf("%i %i %i\n", ret, puiData[0], sizeof(MODULE_INFO));
								if (ret == (puiData[0]*sizeof(MODULE_INFO_TRANSFER)))
								{
									MODULE_INFO_TRANSFER *tMIT = (MODULE_INFO_TRANSFER*)(trInBuffer + sizeof(TRANSFER_DATA) + sizeof(int));	
									DBG_MUTEX_LOCK(&modulelist_mutex);
									k = 0;
									for (cnt = 0; cnt < puiData[0]; cnt++)
									{
										//printf("recv module id:%s, iCnt %i from %s\n", &tMIT[cnt].ID, puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));
										ret = 0;
										for (i = 0; i < iModuleCnt; i++) 
											if ((tMIT[cnt].Enabled & 1) && (miModuleList[i].Enabled & 1) && (miModuleList[i].ID == tMIT[cnt].ID)) 
											{
												if (miModuleList[i].Address.sin_addr.s_addr != trInHeaderData->Address.sin_addr.s_addr)
													dbgprintf(2, "Detected dublicate module ID:%.4s IP:%s <> IP:%s\n", 
																		(char*)&tMIT[cnt].ID, 
																		inet_ntoa(trInHeaderData->Address.sin_addr),
																		inet_ntoa(miModuleList[i].Address.sin_addr));
												ret = 1; 
												break;
											}
										if ((ret == 0) && (iModuleCnt < MAX_MODULE_INFO_COUNT))
										{
											//printf("recv module id:%s added\n", &tMIT[cnt].ID);
											//if ((tMIT[cnt].Enabled & 1) && (tMIT[cnt].Type == MODULE_TYPE_SYSTEM) && tMIT[cnt].Settings[0]) l = 1;
											iModuleCnt++;
											miModuleList = (MODULE_INFO*)DBG_REALLOC(miModuleList, iModuleCnt*sizeof(MODULE_INFO));
											memset(&miModuleList[iModuleCnt-1],0,sizeof(MODULE_INFO));
											memcpy(&miModuleList[iModuleCnt-1],&tMIT[cnt],sizeof(MODULE_INFO_TRANSFER));
											memcpy(&miModuleList[iModuleCnt-1].Address, &trInHeaderData->Address,sizeof(trInHeaderData->Address));								
											miModuleList[iModuleCnt-1].Local = 0;
											k = 1;
										}
										if ((ret == 1) && (miModuleList[i].Local == 0))
										{
											//if (miModuleList[i].Address.sin_addr.s_addr != trInHeaderData->Address.sin_addr.s_addr) 
											{
												//printf("recv module id:%s updated\n", &tMIT[cnt].ID);											
												//if ((tMIT[cnt].Enabled & 1) && (tMIT[cnt].Type == MODULE_TYPE_SYSTEM) && tMIT[cnt].Settings[0]) l = 1;
												memset(&miModuleList[i],0,sizeof(MODULE_INFO));
												memcpy(&miModuleList[i],&tMIT[cnt],sizeof(MODULE_INFO_TRANSFER));
												memcpy(&miModuleList[i].Address, &trInHeaderData->Address,sizeof(trInHeaderData->Address));
												miModuleList[i].Local = 0;
												k = 1;
											}
										}	
									}
									if (k == 1) SortModules(miModuleList, iModuleCnt);
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									/*if ((ucTimeUpdated == 0) && (l == 1))
									{
										time_t rawtime;
										time(&rawtime);	
										struct tm timeinfo;
										localtime_r(&rawtime, &timeinfo);
										char timebuff[64];
										strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
										dbgprintf(3,"Request time from %s. Now:'%s'\n", inet_ntoa(trInHeaderData->Address.sin_addr), timebuff);									
										SendMessage(Num, TYPE_MESSAGE_REQUEST_DATETIME, NULL, 0, NULL, 0, &trInHeaderData->Address);								
									}*/
									if (k == 1) 
									{
										DBG_MUTEX_LOCK(&systemlist_mutex);
										uiData[0] = GetLocalSysID();
										DBG_MUTEX_UNLOCK(&systemlist_mutex);
										AddModuleEventInList(uiData[0], 15, SYSTEM_CMD_EVENT_MODULE, NULL, 0, 0);
									}
								} else dbgprintf(2,"Wrong len module list for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							//printf("(server) 3. TYPE_MESSAGE_MODULE_LIST DONE\n");
							break;
						case TYPE_MESSAGE_MODULE_CHANGED:
							if (uiBodyLen == sizeof(unsigned int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								cnt = 0;
								DBG_MUTEX_LOCK(&systemlist_mutex);
								if (GetLocalSysID() == puiData[0]) cnt = 1;
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								
								if (cnt == 0)
								{
									DBG_MUTEX_UNLOCK(&Connects_Info[Num].Socket_Mutex);
									SendTCPMessage(TYPE_MESSAGE_REQUEST_MODULE_LIST, NULL, 0, NULL, 0, &trInHeaderData->Address);
									DBG_MUTEX_LOCK(&Connects_Info[Num].Socket_Mutex);
								}
							}
							break;						
						case TYPE_MESSAGE_MODULE_SET:
							//printf("(server) 3. TYPE_MESSAGE_MODULE_SET\n");
							if (uiBodyLen >= 12)							
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&modulelist_mutex);
								for (cnt = 0; cnt != iModuleCnt; cnt++) 
								{
									if (miModuleList[cnt].Enabled & 1)
									{
										if (miModuleList[cnt].ID == puiData[0]) break;
										if ((puiData[0] == 0) && (miModuleList[cnt].Type == MODULE_TYPE_SYSTEM)) break;
									}
								}
								if ((cnt != iModuleCnt) && (miModuleList[cnt].Local == 1))
									ret = 1; else ret = 0;
								DBG_MUTEX_UNLOCK(&modulelist_mutex);								
								if (ret) 
								{
									if (uiBodyLen == 12) 
									{
										vData = NULL;
										k = 0;
									}
									else
									{
										vData = &puiData[3];
										k = uiBodyLen - 12;
									}
									AddModuleEventInList(puiData[0],puiData[1], puiData[2], (char*)vData, k, 1);
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));								
							break;
						case TYPE_MESSAGE_REQUEST_MODULE_STATUS:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_MODULE_STATUS\n");
							if (uiBodyLen == 4)
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));							
								tMI_temp = NULL;
								DBG_MUTEX_LOCK(&modulelist_mutex);
								tMI = ModuleIdToPoint(puiData[0], 1);
								if (tMI)
								{
									tMI_temp = DBG_MALLOC(sizeof(MODULE_INFO));
									memcpy(tMI_temp, tMI, sizeof(MODULE_INFO));									
									DBG_MUTEX_UNLOCK(&modulelist_mutex);
									UpdateModuleStatus(tMI_temp, 1);
									DBG_MUTEX_LOCK(&modulelist_mutex);
									tMI = ModuleIdToPoint(puiData[0], 1);
									if (tMI) memcpy(tMI->Status, tMI_temp->Status, MAX_MODULE_STATUSES*sizeof(tMI->Status[0]));
								}
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
								if (tMI_temp) 
								{
									SendMessage(Num, TYPE_MESSAGE_MODULE_STATUS, (char*)tMI_temp, sizeof(MODULE_INFO_TRANSFER), NULL, 0, &trInHeaderData->Address);								
									DBG_FREE(tMI_temp);
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));								
							break;												
						case TYPE_MESSAGE_MODULE_STATUS:
							//printf("(server) 3. TYPE_MESSAGE_MODULE_STATUS\n");
							if (uiBodyLen == sizeof(MODULE_INFO_TRANSFER))
							{
								MODULE_INFO_TRANSFER *tMIT = (MODULE_INFO_TRANSFER*)(trInBuffer + sizeof(TRANSFER_DATA));							
								DBG_MUTEX_LOCK(&modulelist_mutex);
								ret = ModuleIdToNum(tMIT->ID, 0);
								if (ret >= 0) memcpy(miModuleList[ret].Status, tMIT->Status, MAX_MODULE_STATUSES*sizeof(tMIT->Status[0]));	
								DBG_MUTEX_UNLOCK(&modulelist_mutex);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));							
							break;							
						case TYPE_MESSAGE_MODULE_STATUS_CHANGED:
							//printf("(server) 3. TYPE_MESSAGE_MODULE_STATUS_CHANGED\n");
							if (uiBodyLen == (sizeof(MODULE_INFO_TRANSFER) + sizeof(unsigned int)))
							{
								pData = trInBuffer + sizeof(TRANSFER_DATA) + sizeof(MODULE_INFO_TRANSFER);
								int *iStat = (int*)pData;
								MODULE_INFO_TRANSFER *tMIT = (MODULE_INFO_TRANSFER*)(trInBuffer + sizeof(TRANSFER_DATA));									
								
								//for (ret = 0; ret != MAX_MODULE_STATUSES; ret++)
									//	if ((*iStat) & (1 << ret)) 
										//	dbgprintf(3, "STATUS_CHANGED Module:%s, level:%i, status:%i\n", (char*)&tMIT->ID, 1 << ret, tMIT->Status[ret]);
								ret = 0;
								DBG_MUTEX_LOCK(&modulelist_mutex);
								ret = ModuleIdToNum(tMIT->ID, 1);
								DBG_MUTEX_UNLOCK(&modulelist_mutex);							
								//printf("new status:%i, %i, %i\n", ret, tMIT->ID, tMIT->Status[0]);
								if (ret == -1)
								{
									for (ret = 0; ret != MAX_MODULE_STATUSES; ret++)
										if ((*iStat) & (1 << ret)) 
											AddModuleEventInList(tMIT->ID, ret+1, tMIT->Status[ret], NULL, 0, 0);		
								}			
								DBG_MUTEX_LOCK(&modulelist_mutex);
								for (cnt = 0; cnt < iModuleCnt; cnt++) 
								{
									if ((miModuleList[cnt].Enabled & 1) && (miModuleList[cnt].ID == tMIT->ID))
									{
										for (ret = 0; ret != MAX_MODULE_STATUSES; ret++)
											if ((*iStat) & (1 << ret)) miModuleList[cnt].Status[ret] = tMIT->Status[ret];
										break;
									}
								}
								DBG_MUTEX_UNLOCK(&modulelist_mutex);			
							} else dbgprintf(3,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));		
							//printf("(server) 3. TYPE_MESSAGE_MODULE_STATUS_CHANGED DONE\n");
							break;		
						case TYPE_MESSAGE_TEXT:
							//printf("(server) 3. TYPE_MESSAGE_TEXT\n");
							if (uiBodyLen <= MESSAGES_MAX_LEN)
							{							
								pData = trInBuffer + sizeof(TRANSFER_DATA);
								AddMessageInList(pData, uiBodyLen, trInHeaderData->Address.sin_addr.s_addr);
							} else dbgprintf(2,"Wrong len packet for TYPE_MESSAGE_TEXT %i\n", uiBodyLen);		
							break;
						case TYPE_MESSAGE_CHANGED_SECURITYLIST:
							if (uiBodyLen == sizeof(unsigned int)) 
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								ret = 1;
								DBG_MUTEX_LOCK(&systemlist_mutex);
								if (GetLocalSysID() != puiData[0]) ret = 0;
								DBG_MUTEX_UNLOCK(&systemlist_mutex);	
								if (ret == 0) 
								{
									ClearSecurityKey(puiData[0]);									
									if (Num == 0) 
									{
										DBG_MUTEX_UNLOCK(&Connects_Info[Num].Socket_Mutex);									
										SendTCPMessage(TYPE_MESSAGE_REQUEST_SECURITYLIST, NULL, 0, NULL, 0, &trInHeaderData->Address);	
										DBG_MUTEX_LOCK(&Connects_Info[Num].Socket_Mutex);									
									}
									else SendMessage(Num, TYPE_MESSAGE_REQUEST_SECURITYLIST, NULL, 0, NULL, 0, &trInHeaderData->Address);
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_REQUEST_SECURITYLIST:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_SECURITYLIST\n");
							if (uiBodyLen == 0)
							{
								DBG_MUTEX_LOCK(&securitylist_mutex);
								tSKI = (SECURITY_KEY_INFO*)DBG_MALLOC(iSecurityKeyCnt*sizeof(SECURITY_KEY_INFO));
								memcpy(tSKI, skiSecurityKeys, iSecurityKeyCnt*sizeof(SECURITY_KEY_INFO));
								ret = iSecurityKeyCnt;
								DBG_MUTEX_UNLOCK(&securitylist_mutex);
								k = 1;
								for (cnt = 0; cnt < ret; cnt++)
								{
									if (tSKI[cnt].Local == 1)
										SendMessage(Num, TYPE_MESSAGE_SECURITYLIST, (char*)&k, sizeof(k), (char*)&tSKI[cnt], sizeof(SECURITY_KEY_INFO), &trInHeaderData->Address);
								}
								DBG_FREE(tSKI);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));		
							break;
						case TYPE_MESSAGE_SECURITYLIST:
							//printf("(server) 3. TYPE_MESSAGE_SECURITYLIST\n");	
							if (uiBodyLen >= sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								ret = uiBodyLen - sizeof(int);
								if (ret == (puiData[0]*sizeof(SECURITY_KEY_INFO)))
								{								
									tSKI = (SECURITY_KEY_INFO*)(trInBuffer + sizeof(TRANSFER_DATA) + sizeof(int));
									DBG_MUTEX_LOCK(&securitylist_mutex);
									for (cnt = 0; cnt < puiData[0]; cnt++)
									{
										ret = 0;
										for (i = 0; i < iSecurityKeyCnt; i++) if (skiSecurityKeys[i].ID == tSKI[cnt].ID) {ret = 1; break;}
										if ((ret == 0) && (iSecurityKeyCnt < MAX_SECURITY_KEYS))
										{
											iSecurityKeyCnt++;
											skiSecurityKeys = (SECURITY_KEY_INFO*)DBG_REALLOC(skiSecurityKeys, iSecurityKeyCnt*sizeof(SECURITY_KEY_INFO));
											memcpy(&skiSecurityKeys[iSecurityKeyCnt-1],tSKI,sizeof(SECURITY_KEY_INFO));
											skiSecurityKeys[iSecurityKeyCnt-1].Local = 0;
										}
										if (ret == 1)
										{
											memcpy(&skiSecurityKeys[i],tSKI,sizeof(SECURITY_KEY_INFO));
											skiSecurityKeys[i].Local = 0;
										}
									}
									DBG_MUTEX_UNLOCK(&securitylist_mutex);	
								} else dbgprintf(2,"Wrong len body for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));		
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));														
							break;	
						case TYPE_MESSAGE_GET_CHANGE_SMARTCARD:
							if (uiBodyLen == (sizeof(int) * 2 + sizeof(SECURITY_KEY_INFO)))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								int iLocal = -1;
				
								DBG_MUTEX_LOCK(&modulelist_mutex);
								i = ModuleIdToNum(puiData[1], 2);
								if (i >= 0) iLocal = miModuleList[i].Local;	
								DBG_MUTEX_UNLOCK(&modulelist_mutex);
				
								if (iLocal == 1)
								{
									DBG_MUTEX_LOCK(&securitylist_mutex);
									memcpy(&skiUpdateKeyInfo, &puiData[2], sizeof(SECURITY_KEY_INFO));
									iUpdateKeyInfoAction = puiData[0];
									uiUpdateKeyInfoReader = puiData[1];
									iUpdateKeyInfoTimer = 0;
									get_ms(&iUpdateKeyInfoTimer);
									DBG_MUTEX_UNLOCK(&securitylist_mutex);
								} else dbgprintf(2,"Module:Card reader not local %.4s\n",(char*)&puiData[1]);	
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_SET_CHANGE_SMARTCARD:
							if (uiBodyLen == (sizeof(int) * 2))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
												
								DBG_MUTEX_LOCK(&securitylist_mutex);
								if (iUpdateKeyInfoResult == 0)
								{
									if (skiUpdateKeyInfo.ID == puiData[0])
									{
										iUpdateKeyInfoResult = puiData[1];
										dbgprintf(3,"Accept change smartcard, result: %i\n", iUpdateKeyInfoResult);	
									}
									else dbgprintf(2, "Error recv change status smartkey, wrong ID %.4s != %.4s\n", (char*)&skiUpdateKeyInfo.ID, (char*)&puiData[0]);								
								}
								DBG_MUTEX_UNLOCK(&securitylist_mutex);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_REQUEST_DATETIME:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_DATETIME\n");	
							if (uiBodyLen == 0)
							{
								char cTimeDone = 0;
								DBG_MUTEX_LOCK(&system_mutex);
								cTimeDone = ucTimeUpdated;
								DBG_MUTEX_UNLOCK(&system_mutex);
								
								time_t rawtime;
								time(&rawtime);	
								struct tm timeinfo;
								localtime_r(&rawtime, &timeinfo);
								char timebuff[64];
								strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
								dbgprintf(3,"Send time '%s' to %s, size:%i\n", timebuff, inet_ntoa(trInHeaderData->Address.sin_addr), sizeof(TIME_INFO));
								TIME_INFO ti;
								ti.tm_sec = timeinfo.tm_sec;
								ti.tm_min = timeinfo.tm_min;
								ti.tm_hour = timeinfo.tm_hour;
								ti.tm_mday = timeinfo.tm_mday;
								ti.tm_mon = timeinfo.tm_mon;
								ti.tm_year = timeinfo.tm_year;
								ti.tm_wday = timeinfo.tm_wday;
								ti.tm_yday = timeinfo.tm_yday;
								ti.tm_isdst = timeinfo.tm_isdst;
								SendMessage(Num, TYPE_MESSAGE_DATETIME, &cTimeDone, 1, (char*)&ti, sizeof(TIME_INFO), &trInHeaderData->Address);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));			
							break;
						case TYPE_MESSAGE_DATETIME:
							//dbgprintf(3, "(server) 3. TYPE_MESSAGE_DATETIME %i %i\n", uiBodyLen, sizeof(TIME_INFO)+1);	
							if (uiBodyLen == (sizeof(TIME_INFO)+1))
							{
								char *cRecvTU = (char*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&system_mutex);
								if (ucTimeUpdated <= cRecvTU[0]) 
								{									
									//time_t *prawtime = (time_t*)(trInBuffer + sizeof(TRANSFER_DATA) + 1);
									//stime(prawtime);
									TIME_INFO *ti = (TIME_INFO*)(trInBuffer + sizeof(TRANSFER_DATA) + 1);
									struct tm timeinfo;
									timeinfo.tm_sec = ti->tm_sec;
									timeinfo.tm_min = ti->tm_min;
									timeinfo.tm_hour = ti->tm_hour;
									timeinfo.tm_mday = ti->tm_mday;
									timeinfo.tm_mon = ti->tm_mon;
									timeinfo.tm_year = ti->tm_year;
									timeinfo.tm_wday = ti->tm_wday;
									timeinfo.tm_yday = ti->tm_yday;
									timeinfo.tm_isdst = ti->tm_isdst;
								
									//localtime_r(prawtime, &timeinfo);
									time_t rawtime = mktime(&timeinfo);
									a_stime(&rawtime);
									char timebuff[64];
									strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
									dbgprintf(4,"Recv time lvl: %i, '%s'\n", cRecvTU[0], timebuff);								
									ucTimeUpdated = cRecvTU[0];
									if (ucTimeUpdated >= 2) memcpy(cStartTime, timebuff, 64);
								}
								DBG_MUTEX_UNLOCK(&system_mutex);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_FORCE_DATETIME:
							//dbgprintf(3, "(server) 3. TYPE_MESSAGE_FORCE_DATETIME\n");	
							if (uiBodyLen == (sizeof(time_t) + sizeof(int)))
							{
								time_t *prawtime = (time_t*)(trInBuffer + sizeof(TRANSFER_DATA));
								unsigned int *SendID = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA) + sizeof(time_t));
								DBG_MUTEX_LOCK(&systemlist_mutex);
								unsigned int SysID = GetLocalSysID();
								DBG_MUTEX_UNLOCK(&systemlist_mutex);
								if ((*SendID == 0) || (*SendID != SysID)) 
								{
									a_stime(prawtime);
									struct tm timeinfo;
									localtime_r(prawtime, &timeinfo);
									char timebuff[64];
									strftime(timebuff, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
									dbgprintf(3,"Recv time '%s'\n", timebuff);	
									DBG_MUTEX_LOCK(&system_mutex);									
									ucTimeUpdated = 2;
									DBG_MUTEX_UNLOCK(&system_mutex);
								} else dbgprintf(3,"Skip Recv time from %.4s\n", SendID);	
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_TRAFFIC_TYPE_SERVICE:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_SERVICE\n");
							Connects_Info[Num].TraffType = TRAFFIC_SERVICE;
							break;
						case TYPE_MESSAGE_TRAFFIC_TYPE_FULL_VIDEO:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_FULL_VIDEO\n");
							Connects_Info[Num].TraffType = TRAFFIC_FULL_VIDEO;
							if (Connects_Info[Num].OutBufferSize < FVIDEO_TRAFFIC_BUFFER_SIZE)
							{
								Connects_Info[Num].OutBufferSize = FVIDEO_TRAFFIC_BUFFER_SIZE;
								Connects_Info[Num].OutBuffer = (char*)DBG_REALLOC(Connects_Info[Num].OutBuffer, Connects_Info[Num].OutBufferSize);
							}
							DBG_MUTEX_LOCK(&systemlist_mutex);
							ret = GetLocalSysID();
							DBG_MUTEX_UNLOCK(&systemlist_mutex);
							AddModuleEventInList(ret, 15, SYSTEM_CMD_OPENED_CAMERA, NULL, 0, 0);
							break;
						case TYPE_MESSAGE_TRAFFIC_TYPE_PREV_VIDEO:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_PREV_VIDEO\n");
							Connects_Info[Num].TraffType = TRAFFIC_PREV_VIDEO;
							if (Connects_Info[Num].OutBufferSize < PVIDEO_TRAFFIC_BUFFER_SIZE)
							{
								Connects_Info[Num].OutBufferSize = PVIDEO_TRAFFIC_BUFFER_SIZE;
								Connects_Info[Num].OutBuffer = (char*)DBG_REALLOC(Connects_Info[Num].OutBuffer, Connects_Info[Num].OutBufferSize);
							}
							DBG_MUTEX_LOCK(&systemlist_mutex);
							ret = GetLocalSysID();
							DBG_MUTEX_UNLOCK(&systemlist_mutex);
							AddModuleEventInList(ret, 15, SYSTEM_CMD_OPENED_CAMERA, NULL, 0, 0);
							break;
						case TYPE_MESSAGE_TRAFFIC_TYPE_AUDIO:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_AUDIO\n");
							Connects_Info[Num].TraffType = TRAFFIC_AUDIO;
							if (Connects_Info[Num].OutBufferSize < AUDIO_TRAFFIC_BUFFER_SIZE)
							{
								Connects_Info[Num].OutBufferSize = AUDIO_TRAFFIC_BUFFER_SIZE;
								Connects_Info[Num].OutBuffer = (char*)DBG_REALLOC(Connects_Info[Num].OutBuffer, Connects_Info[Num].OutBufferSize);
							}
							DBG_MUTEX_LOCK(&systemlist_mutex);
							ret = GetLocalSysID();
							DBG_MUTEX_UNLOCK(&systemlist_mutex);
							AddModuleEventInList(ret, 15, SYSTEM_CMD_OPENED_MIC, NULL, 0, 0);
							break;
						case TYPE_MESSAGE_REQUEST_VIDEO_PARAMS:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_VIDEO_PARAMS\n");
							Connects_Info[Num].NeedData |= FLAG_VIDEO_PARAMS;
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_VIDEO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_VIDEO_PARAMS);							
							break;
						case TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO\n");
							Connects_Info[Num].NeedData |= FLAG_VIDEO_CODEC_INFO;
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_VIDEO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_VIDEO_CODEC);							
							break;
						case TYPE_MESSAGE_REQUEST_AUDIO_CODEC_INFO:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_AUDIO_CODEC_INFO\n");
							Connects_Info[Num].NeedData |= FLAG_AUDIO_CODEC_INFO;
							//printf("tx_eventer_send_event %i %i\n", Connects_Info[Num].TraffType, TRAFFIC_REMOTE_AUDIO);
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_AUDIO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_AUDIO_PARAMS);														
							break;
						case TYPE_MESSAGE_VIDEO_PARAMS:
							//printf("(server) 3. TYPE_MESSAGE_VIDEO_PARAMS(%i)\n",Connects_Info[Num].NeedData);
							if (Connects_Info[Num].NeedData & FLAG_VIDEO_PARAMS)
							{
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_VIDEO_PARAMS, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{									
									//printf("Later: TYPE_MESSAGE_VIDEO_PARAMS\n");
									cRecv = 0;
									iLoop = 0;									
								}// else printf("Sended: TYPE_MESSAGE_VIDEO_PARAMS\n");									
							}
							break;
						case TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA:
							//printf("(server) 3. TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA(%i)\n",Connects_Info[Num].NeedData);
							if (Connects_Info[Num].NeedData & FLAG_VIDEO_CODEC_INFO)
							{
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_VIDEO_CODEC_INFO_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{									
									//printf("Later: TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA\n");
									cRecv = 0;
									iLoop = 0;									
								}// else printf("Sended: TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA\n");						
							}
							break;
						case TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA:
							//printf("(server) 3. TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA(%i)\n",Connects_Info[Num].NeedData);
							if (Connects_Info[Num].NeedData & FLAG_AUDIO_CODEC_INFO)
							{
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_AUDIO_CODEC_INFO_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{									
									//dbgn_printf("Later: TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA\n");
									cRecv = 0;
									iLoop = 0;									
								}						
							}
							break;
						case TYPE_MESSAGE_REQUEST_VIDEO_STREAM:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_VIDEO_STREAM\n");
							Connects_Info[Num].NeedData |= FLAG_VIDEO_STREAM;
							break;	
						case TYPE_MESSAGE_REQUEST_AUDIO_STREAM:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_AUDIO_STREAM\n");
							Connects_Info[Num].NeedData |= FLAG_AUDIO_STREAM;
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_AUDIO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_AUDIO_STREAM);							
							break;	
						case TYPE_MESSAGE_STOP_VIDEO_STREAM:
							//printf("(server) 3. TYPE_MESSAGE_STOP_VIDEO_STREAM\n");
							if (Connects_Info[Num].NeedData & FLAG_VIDEO_STREAM) Connects_Info[Num].NeedData ^= FLAG_VIDEO_STREAM;
							break;	
						case TYPE_MESSAGE_STOP_AUDIO_STREAM:
							//printf("(server) 3. TYPE_MESSAGE_STOP_AUDIO_STREAM\n");
							if (Connects_Info[Num].NeedData & FLAG_AUDIO_STREAM) Connects_Info[Num].NeedData ^= FLAG_AUDIO_STREAM;
							break;	
						case TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME\n");
							Connects_Info[Num].NeedData |= FLAG_START_VIDEO_FRAME;
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_VIDEO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_START_FRAME);							
							break;
						case TYPE_MESSAGE_START_VIDEO_FRAME_DATA:
							//printf("(server) 3. TYPE_MESSAGE_START_VIDEO_FRAME_DATA(%i)\n",Num);
							if (Connects_Info[Num].NeedData & FLAG_START_VIDEO_FRAME)
							{	
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_START_VIDEO_FRAME_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{								
									//printf("Later: TYPE_MESSAGE_START_VIDEO_FRAME_DATA\n");
									cRecv = 0;
									iLoop = 0;
								}// else printf("Sended: TYPE_MESSAGE_START_VIDEO_FRAME_DATA\n");
							} else dbgprintf(2,"(skiped) 3. TYPE_MESSAGE_START_VIDEO_FRAME_DATA(%i)\n",Connects_Info[Num].NeedData);	
							break;
						case TYPE_MESSAGE_CLOSE:
							//printf("(server) 3. TYPE_MESSAGE_CLOSE\n");	
							result = 1;
							break;
						case TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME\n");
							Connects_Info[Num].NeedData |= FLAG_NEXT_VIDEO_FRAME;						
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_VIDEO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_NEXT_VIDEO_FRAME);
							break;
						case TYPE_MESSAGE_REQUEST_NEXT_AUDIO_FRAME:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_NEXT_AUDIO_FRAME\n");	
							Connects_Info[Num].NeedData |= FLAG_NEXT_AUDIO_FRAME;
							if (Connects_Info[Num].TraffType == TRAFFIC_REMOTE_AUDIO) tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_NEXT_AUDIO_FRAME);																					
							break;
						case TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA:
							//printf("(server) 3. TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA(%i)\n",Connects_Info[Num].NeedData);							
							//vVFI = (VIDEO_FRAME_INFO*)&trInBuffer[sizeof(TRANSFER_DATA)];	
							//printf("(server)TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA(%i)(%i)\n",vVFI->Number,Connects_Info[Num].NeedData);
							//printf("(%i)TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA>Num:%i,Regn:%i,LenDt:%i,F:%i\n",Connects_Info[Num].InDataSize,vVFI->Number,vVFI->Region,vVFI->LenData,vVFI->Flag);
							if ((Connects_Info[Num].NeedData & FLAG_NEXT_VIDEO_FRAME) || (Connects_Info[Num].NeedData & FLAG_VIDEO_STREAM))
							{
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_NEXT_VIDEO_FRAME_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{
									//dbgn_
									//printf("Later: TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA\n");
									cRecv = 0;
									iLoop = 0;
								}// else printf("Sended: TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA\n");
							} else dbgprintf(2,"(skiped) 3. TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA(%i)\n",Connects_Info[Num].NeedData);	
							break;						
						case TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA:
							//printf("(server) 3. TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA\n");	
							vVSFI = (VIDEO_SUBFRAME_INFO*)&trInBuffer[sizeof(TRANSFER_DATA)];	
							//printf("(server)TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA(%i)(%i)\n",vVSFI->Number,vVSFI->SubNumber);
							//printf("(%i)TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA >>> Number:%i,Region:%i,LenData:%i,SubNumber:%i\n",Connects_Info[Num].InDataSize,vVSFI->Number,vVSFI->Region,vVSFI->LenData,vVSFI->SubNumber);							
							if (Connects_Info[Num].NeedData & FLAG_SUB_VIDEO_FRAME)
							{
								//printf(">>>>>>>> TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA\n");	
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_SUB_VIDEO_FRAME_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{	
									//printf(">>>||||| TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA\n");	
								//	printf("Later: TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA\n");
									//result = 2;
									cRecv = 0;
									iLoop = 0;
								}// else printf("Sended: TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA\n");
							} else dbgprintf(4,"(skiped) 3. TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA(%i)(%i)\n",vVSFI->Number,vVSFI->SubNumber);	
							break;
						case TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA:
							//printf("(server) 3. TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA(%i)\n",Connects_Info[Num].NeedData);							
							//vVFI = (VIDEO_FRAME_INFO*)&trInBuffer[sizeof(TRANSFER_DATA)];	
							//printf("(server)TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA(%i)(%i)\n",vVFI->Number,Connects_Info[Num].NeedData);
							//printf("(%i)TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA>Num:%i,Regn:%i,LenDt:%i,F:%i\n",Connects_Info[Num].InDataSize,vVFI->Number,vVFI->Region,vVFI->LenData,vVFI->Flag);
							if ((Connects_Info[Num].NeedData & FLAG_NEXT_AUDIO_FRAME) || (Connects_Info[Num].NeedData & FLAG_AUDIO_STREAM))
							{
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_NEXT_AUDIO_FRAME_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{
									//dbgn_printf("Later: TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA\n");
									cRecv = 0;
									iLoop = 0;
								}
							} else dbgprintf(2,"(skiped) 3. TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA(%i)\n",Connects_Info[Num].NeedData);	
							break;						
						case TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA:
							//printf("(server) 3. TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA\n");	
							//vVSFI = (VIDEO_SUBFRAME_INFO*)&trInBuffer[sizeof(TRANSFER_DATA)];	
							//printf("(server)TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA(%i)(%i)\n",vVSFI->Number,vVSFI->SubNumber);
							//printf("(%i)TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA >>> Number:%i,Region:%i,LenData:%i,SubNumber:%i\n",Connects_Info[Num].InDataSize,vVSFI->Number,vVSFI->Region,vVSFI->LenData,vVSFI->SubNumber);							
							if (Connects_Info[Num].NeedData & FLAG_SUB_AUDIO_FRAME)
							{
								ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, EVENT_SUB_AUDIO_FRAME_DATA, trInBuffer, uiBodyLen, 1, 0);
								if (ret == 0)
								{
									//dbgn_printf("Later: TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA\n");
									//result = 2;
									cRecv = 0;
									iLoop = 0;
								}
							} //else dbgprintf(2,"(skiped) 3. TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA(%i)(%i)\n",vVSFI->Number,vVSFI->SubNumber);	
							break;
						case TYPE_MESSAGE_ALARM_SET:
							//printf("(server) 3. TYPE_MESSAGE_ALARM_SET\n");
							pData = trInBuffer + sizeof(TRANSFER_DATA);
							memcpy(&ret, pData, sizeof(ret));
							DBG_MUTEX_LOCK(&system_mutex);
							iAlarmEvents |= ret;
							DBG_MUTEX_UNLOCK(&system_mutex);
							break;							
						case TYPE_MESSAGE_ALARM_RESET:
							//printf("(server) 3. TYPE_MESSAGE_ALARM_RESET\n");		
							pData = trInBuffer + sizeof(TRANSFER_DATA);
							memcpy(&ret, pData, sizeof(ret));
							DBG_MUTEX_LOCK(&system_mutex);
							iAlarmEvents &= 0xFFFFFFFF ^ ret;
							DBG_MUTEX_UNLOCK(&system_mutex);
							break;
						case TYPE_MESSAGE_DISPLAY_CONTENT:
							//printf("(server) 3. TYPE_MESSAGE_DISPLAY_CONTENT\n");
							if (uiBodyLen == sizeof(DISPLAY_CONTENT_INFO))
							{
								pData = trInBuffer + sizeof(TRANSFER_DATA);
								DISPLAY_CONTENT_INFO *dci = (DISPLAY_CONTENT_INFO*)pData;
								if (dci->Type == 0)
									DisplayContentListClear();
								if (dci->Type == 1)
									DisplayContentListSwitch();
								if (dci->Type > 1)
									DisplayContentListAdd(dci);								
							} else dbgprintf(2,"Wrong len packet for TYPE_MESSAGE_DISPLAY_CONTENT %i\n", uiBodyLen);		
							break;
						case TYPE_MESSAGE_COPY_FILE_GET_ORDER:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_GET_ORDER\n");		
							//pData = trInBuffer + sizeof(TRANSFER_DATA);
							//memcpy(&ret, pData, sizeof(ret));
							if ((uiBodyLen > (sizeof(int)*2)) && (uiBodyLen < (MAX_PATH + 7)))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));	
								dbgprintf(5, "Server: request order for id %i size %i from %s\n", puiData[0], puiData[1], inet_ntoa(trInHeaderData->Address.sin_addr));
								ret = 0;
								DBG_MUTEX_LOCK(&system_mutex);
								if ((tfTransferFileStatus.Enabled) && (tfTransferFileStatus.Status == 0))
								{
									tfTransferFileStatus.ID = puiData[0];
									tfTransferFileStatus.Size = puiData[1];
									tfTransferFileStatus.Type = puiData[2];
									tfTransferFileStatus.Status = 1;
									tfTransferFileStatus.ConnectNum = Num;
									tfTransferFileStatus.Address = trInHeaderData->Address;
									memset(tfTransferFileStatus.Path, 0, MAX_PATH);
									memcpy(tfTransferFileStatus.Path, &puiData[3], uiBodyLen - (sizeof(int)*2));
									ret = 1;
									dbgprintf(5, "Server(%i): accepted order for id %i size %i type %i from %s\n", Num, puiData[0], puiData[1], puiData[2], inet_ntoa(trInHeaderData->Address.sin_addr));
									if (Connects_Info[Num].InBufferSize < FILE_TRAFFIC_BUFFER_SIZE)
									{
										Connects_Info[Num].InBufferSize = FILE_TRAFFIC_BUFFER_SIZE;
										Connects_Info[Num].InBuffer = (char*)DBG_REALLOC(Connects_Info[Num].InBuffer, Connects_Info[Num].InBufferSize);
										trInBuffer = &Connects_Info[Num].InBuffer[iPos];
										trInHeaderData = (TRANSFER_DATA*)trInBuffer;	
									}
								}
								else
								{
									if (tfTransferFileStatus.Enabled) ret = 0; else ret = 2;
								}
								DBG_MUTEX_UNLOCK(&system_mutex);
								if (ret == 1) 
								{
									//SendMessage(Num, TYPE_MESSAGE_COPY_FILE_ACCEPT_ORDER, puiData, 4, NULL, 0, &trInHeaderData->Address);
									tx_eventer_send_event(&writeevent_evnt, WRT_EVENT_COPY_FILE_GET_ORDER);
								}
								if (ret == 0) SendMessage(Num, TYPE_MESSAGE_COPY_FILE_BUSY, puiData, 4, NULL, 0, &trInHeaderData->Address);
								if (ret == 2) SendMessage(Num, TYPE_MESSAGE_COPY_FILE_REJECT, puiData, 4, NULL, 0, &trInHeaderData->Address);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));								
							break;
						case TYPE_MESSAGE_COPY_FILE_RELEASE:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_RELEASE\n");		
							if (uiBodyLen == sizeof(int))
							{
								ret = 0;
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));	
								dbgprintf(5, "Server(%i): release order for id %i from %s\n", Num, puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));								
								DBG_MUTEX_LOCK(&system_mutex);
								if ((tfTransferFileStatus.Status) && (tfTransferFileStatus.ID == puiData[0]) && 
										(trInHeaderData->Address.sin_addr.s_addr == tfTransferFileStatus.Address.sin_addr.s_addr))
								{
									ret = 1;
									//memset(&tfTransferFileStatus, 0, sizeof(tfTransferFileStatus));
									//dbgprintf(3, "Server: deleted order for id %i\n", puiData[0]);
								}
								else dbgprintf(2, "Wrong stream id (%i) for COPY_FILE_RELEASE signal from %s", puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));
								DBG_MUTEX_UNLOCK(&system_mutex);
								if (ret) 
								{
									ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, WRT_EVENT_COPY_FILE_DATA, NULL, 0, 0, 0);																		
									if (ret == 0)
									{
										dbgprintf(5, "Later: TYPE_MESSAGE_COPY_FILE_RELEASE\n");
										//result = 2;
										cRecv = 0;
										iLoop = 0;
									} else result = 1;
								}
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_COPY_FILE_DATA:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_DATA\n");		
							if (uiBodyLen >= (sizeof(int)*2))
							{
								if (Connects_Info[Num].TraffType == TRAFFIC_TRANSFER_FILE)
								{
									ret = 0;
									puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));	
									//dbgprintf(5, "Server(%i): data file for id %i, num %i from %s\n", Num, puiData[0], puiData[1], inet_ntoa(trInHeaderData->Address.sin_addr));								
									DBG_MUTEX_LOCK(&system_mutex);
									if ((tfTransferFileStatus.Status) && (tfTransferFileStatus.ID == puiData[0]) && 
											(trInHeaderData->Address.sin_addr.s_addr == tfTransferFileStatus.Address.sin_addr.s_addr))
									{
										ret = 1;
									}
									else dbgprintf(2, "Wrong stream id (%i, %i) for COPY_FILE_DATA signal %s", puiData[0], puiData[1], inet_ntoa(trInHeaderData->Address.sin_addr));
									DBG_MUTEX_UNLOCK(&system_mutex);
									if (ret) 
									{
										ret = tx_eventer_send_data(Connects_Info[Num].pevntIP, WRT_EVENT_COPY_FILE_DATA, &puiData[1], uiBodyLen - sizeof(int), 1, 0);
										if (ret == 0)
										{
											dbgprintf(5, "Later: TYPE_MESSAGE_COPY_FILE_DATA\n");
											//result = 2;
											cRecv = 0;
											iLoop = 0;
										}
										//tx_eventer_send_event(&writeevent_evnt, WRT_EVENT_COPY_FILE_RELEASE);		
									}
								} else dbgprintf(2,"Wrong TraffType for type mess %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));
							break;
						case TYPE_MESSAGE_COPY_FILE_ACCEPT_ORDER:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_ACCEPT_ORDER\n");		
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));	
								dbgprintf(5, "Client(%i): accepted order for id %i from %s\n", Num, puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));								
								DBG_MUTEX_LOCK(&system_mutex);
								int ui = iStreamCnt;
								unsigned int uiNum;
								DBG_MUTEX_UNLOCK(&system_mutex);
								for (i = 0; i < ui; i++)
								{
									DBG_MUTEX_LOCK(&SyncStream[i]->mutex);
									uiNum = SyncStream[i]->Num;
									DBG_MUTEX_UNLOCK(&SyncStream[i]->mutex);
									
									if (puiData[0] == uiNum)
									{
										if (Connects_Info[Num].OutBufferSize < FILE_TRAFFIC_BUFFER_SIZE)
										{
											Connects_Info[Num].OutBufferSize = FILE_TRAFFIC_BUFFER_SIZE;
											Connects_Info[Num].OutBuffer = (char*)DBG_REALLOC(Connects_Info[Num].OutBuffer, Connects_Info[Num].OutBufferSize);
										}
										tx_eventer_send_event(&SyncStream[i]->CopyEvent, WRT_EVENT_COPY_FILE_ACCEPT);
										break;
									}
								}
								if (i == ui) dbgprintf(2, "Wrong stream id (%i) for COPY_FILE_ACCEPT_ORDER signal", puiData[0]/*, inet_ntoa(trInHeaderData->Address.sin_addr)*/);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_COPY_FILE_REJECT:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_REJECT\n");		
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));	
								dbgprintf(5, "Client(%i): denied order for id %i from %s\n", Num, puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));								
								DBG_MUTEX_LOCK(&system_mutex);
								int ui = iStreamCnt;
								unsigned int uiNum;
								DBG_MUTEX_UNLOCK(&system_mutex);
								for (i = 0; i < ui; i++)
								{
									DBG_MUTEX_LOCK(&SyncStream[i]->mutex);
									uiNum = SyncStream[i]->Num;
									DBG_MUTEX_UNLOCK(&SyncStream[i]->mutex);
									
									if (puiData[0] == uiNum)
									{
										tx_eventer_send_event(&SyncStream[i]->CopyEvent, WRT_EVENT_COPY_FILE_REJECT);
										result = 1;
										break;
									}
								}
								if (i == ui) dbgprintf(1, "Wrong stream id (%i) for COPY_FILE_REJECT signal from %s", puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;	
						case TYPE_MESSAGE_COPY_FILE_BUSY:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_BUSY\n");		
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								dbgprintf(5, "Client(%i): busy order for id %i from %s\n", Num, puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));								
								DBG_MUTEX_LOCK(&system_mutex);
								int ui = iStreamCnt;
								unsigned int uiNum;
								DBG_MUTEX_UNLOCK(&system_mutex);
								for (i = 0; i < ui; i++)
								{
									DBG_MUTEX_LOCK(&SyncStream[i]->mutex);
									uiNum = SyncStream[i]->Num;
									DBG_MUTEX_UNLOCK(&SyncStream[i]->mutex);
									
									if (puiData[0] == uiNum)
									{
										tx_eventer_send_event(&SyncStream[i]->CopyEvent, WRT_EVENT_COPY_FILE_BUSY);
										break;
									}
								}
								if (i == ui) dbgprintf(1, "Wrong stream id (%i) for COPY_FILE_BUSY signal from %s", puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_COPY_FILE_DONE:
							//printf("(server) 3. TYPE_MESSAGE_COPY_FILE_DONE\n");		
							if (uiBodyLen == (sizeof(int)*2))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								dbgprintf(5, "Client(%i): done write data for id %i, num %i from %s\n", Num, puiData[0], puiData[1], inet_ntoa(trInHeaderData->Address.sin_addr));								
								//printf("Client(%i): done write data for id %i, num %i from %s\n", Num, puiData[0], puiData[1], inet_ntoa(trInHeaderData->Address.sin_addr));								
								DBG_MUTEX_LOCK(&system_mutex);
								int ui = iStreamCnt;
								unsigned int uiNum;
								DBG_MUTEX_UNLOCK(&system_mutex);
								for (i = 0; i < ui; i++)
								{
									DBG_MUTEX_LOCK(&SyncStream[i]->mutex);
									uiNum = SyncStream[i]->Num;
									DBG_MUTEX_UNLOCK(&SyncStream[i]->mutex);
									
									if (puiData[0] == uiNum)
									{
										tx_eventer_send_event(&SyncStream[i]->CopyEvent, WRT_EVENT_COPY_FILE_DONE);
										break;
									}
								}
								if (i == ui) dbgprintf(1, "Wrong stream id (%i) for COPY_FILE_DONE signal from %s", puiData[0], inet_ntoa(trInHeaderData->Address.sin_addr));
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_FILE:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_FILE\n");
							Connects_Info[Num].TraffType = TRAFFIC_REMOTE_FILE;
							break;						
						case TYPE_MESSAGE_TRAFFIC_TYPE_TRANSFER_FILE:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_TRANSFER_FILE %i\n", Num);
							Connects_Info[Num].TraffType = TRAFFIC_TRANSFER_FILE;
							break;						
						case TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_VIDEO:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_VIDEO\n");
							Connects_Info[Num].TraffType = TRAFFIC_REMOTE_VIDEO;
							if (Connects_Info[Num].OutBufferSize < FVIDEO_TRAFFIC_BUFFER_SIZE)
							{
								Connects_Info[Num].OutBufferSize = FVIDEO_TRAFFIC_BUFFER_SIZE;
								Connects_Info[Num].OutBuffer = (char*)DBG_REALLOC(Connects_Info[Num].OutBuffer, Connects_Info[Num].OutBufferSize);
							}
							break;						
						case TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_AUDIO:
							//printf("(server) 3. TYPE_MESSAGE_TRAFFIC_TYPE_REMOTE_AUDIO %i\n", Num);
							Connects_Info[Num].TraffType = TRAFFIC_REMOTE_AUDIO;
							if (Connects_Info[Num].OutBufferSize < AUDIO_TRAFFIC_BUFFER_SIZE)
							{
								Connects_Info[Num].OutBufferSize = AUDIO_TRAFFIC_BUFFER_SIZE;
								Connects_Info[Num].OutBuffer = (char*)DBG_REALLOC(Connects_Info[Num].OutBuffer, Connects_Info[Num].OutBufferSize);
							}
							break;						
						case TYPE_MESSAGE_REQUEST_FILELIST:
							//printf("(server) 3. TYPE_MESSAGE_REQUEST_FILELIST\n");
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&stream_mutex);
								cStreamerExData[0] = puiData[0];
								DBG_MUTEX_UNLOCK(&stream_mutex);	
								tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_LIST);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_FILEPATH:
							if (uiBodyLen < MAX_PATH)
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&system_mutex);
								memset(rsiRemoteStatus.Path, 0, MAX_PATH);
								memcpy(rsiRemoteStatus.Path, puiData, uiBodyLen);
								DBG_MUTEX_UNLOCK(&system_mutex);
							} else dbgprintf(2, "Wrong lenght Remote path: %i\n", uiBodyLen);
							break;
						case TYPE_MESSAGE_FILELIST:
							//printf("(server) 3. TYPE_MESSAGE_FILELIST %i %i\n", uiBodyLen, sizeof(int)*2);
							if (uiBodyLen >= (sizeof(int)*2))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								ret = uiBodyLen - (sizeof(int)*2);
								if (ret == (puiData[0]*sizeof(REMOTE_FILE_LIST)))
								{		
									if (puiData[0])
									{
										REMOTE_FILE_LIST *rfl = (REMOTE_FILE_LIST*)(trInBuffer + sizeof(TRANSFER_DATA) + (sizeof(int)*2));
										DBG_MUTEX_LOCK(&system_mutex);
										if (rsiRemoteStatus.NewFileCnt) DBG_FREE(rsiRemoteStatus.NewFileList);
										rsiRemoteStatus.NewFileCnt = puiData[0];
										rsiRemoteStatus.NewFileList = (REMOTE_FILE_LIST*)DBG_MALLOC(rsiRemoteStatus.NewFileCnt * sizeof(REMOTE_FILE_LIST));
										memset(rsiRemoteStatus.NewFileList, 0, rsiRemoteStatus.NewFileCnt * sizeof(REMOTE_FILE_LIST));
										for (i = 0; i < rsiRemoteStatus.NewFileCnt; i++)
										{
											if (rfl[i].Type == 0) memcpy(rsiRemoteStatus.NewFileList[i].ShowName, rfl[i].ShowName, MAX_FILE_LEN);
											if (rfl[i].Type == 1) 
											{
												ret = strlen(rfl[i].ShowName);
												if (ret < (MAX_FILE_LEN - 3))
												{
													rsiRemoteStatus.NewFileList[i].ShowName[0] = 91;
													memcpy(&rsiRemoteStatus.NewFileList[i].ShowName[1], rfl[i].ShowName, ret);
													rsiRemoteStatus.NewFileList[i].ShowName[ret + 1] = 93;
												}
											}
											if (rfl[i].Type == 2) 
											{
												ret = strlen(rfl[i].ShowName);
												if (ret < (MAX_FILE_LEN - 3))
												{
													rsiRemoteStatus.NewFileList[i].ShowName[0] = 123;
													memcpy(&rsiRemoteStatus.NewFileList[i].ShowName[1], rfl[i].ShowName, ret);
													rsiRemoteStatus.NewFileList[i].ShowName[ret + 1] = 125;
												}
											}
											rsiRemoteStatus.NewFileList[i].Type = rfl[i].Type;
										}
										//printf("Direct %i\n", rsiRemoteStatus.Direct);
										if ((puiData[1] == 0) || (puiData[1] == 3))
										{
											if (rsiRemoteStatus.FileCnt) DBG_FREE(rsiRemoteStatus.FileList);
											rsiRemoteStatus.FileCnt = rsiRemoteStatus.NewFileCnt;
											rsiRemoteStatus.FileList = rsiRemoteStatus.NewFileList;
											rsiRemoteStatus.NewFileCnt = 0;
											rsiRemoteStatus.NewFileList = NULL;
											rsiRemoteStatus.Selected = (int)floor((double)rsiRemoteStatus.FileCnt / 2);
											rsiRemoteStatus.CurrentShow = rsiRemoteStatus.Selected;
										}	
										if ((puiData[1] == 0) || (puiData[1] == 3) || (rsiRemoteStatus.CurrentShow >= rsiRemoteStatus.FileCnt))
										{
											rsiRemoteStatus.NewPos = 0;
											rsiRemoteStatus.Selected = 0;
											rsiRemoteStatus.CurrentShow = 0;
											rsiRemoteStatus.Offset = 0;
										}			
										if (puiData[1] == 3) 
										{
											puiData[1] = 0;
											rsiRemoteStatus.Selected = rsiRemoteStatus.FileCnt - 1;
											rsiRemoteStatus.CurrentShow = rsiRemoteStatus.FileCnt - 1;											
										}
										rsiRemoteStatus.Direct = puiData[1];
										DBG_MUTEX_UNLOCK(&system_mutex);										
									} 
									else
									{
										if (puiData[1] == 3) puiData[1] = 0;
										DBG_MUTEX_LOCK(&system_mutex);
										rsiRemoteStatus.Direct = puiData[1];
										DBG_MUTEX_UNLOCK(&system_mutex);
									}
								} else dbgprintf(2,"Wrong len body for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));		
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));		
							break;
						case TYPE_MESSAGE_NEXT_FILELIST:
							//printf("(server) 3. TYPE_MESSAGE_NEXT_FILELIST\n");
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&stream_mutex);
								cStreamerExData[0] = puiData[0];
								DBG_MUTEX_UNLOCK(&stream_mutex);	
								tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_NEXT_LIST);								
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_PREV_FILELIST:
							//printf("(server) 3. TYPE_MESSAGE_PREV_FILELIST\n");
							tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_PREV_LIST);
							break;
						case TYPE_MESSAGE_GET_PLAY_FILE:
							//printf("(server) 3. TYPE_MESSAGE_GET_PLAY_FILE\n");
							if (uiBodyLen == sizeof(int)*2)
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&stream_mutex);
								cStreamerExData[0] = puiData[0];
								cStreamerExData[1] = puiData[1];
								DBG_MUTEX_UNLOCK(&stream_mutex);	
								tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_PLAY_LIST);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_OPENED_FILE:
							//printf("(server) 3. TYPE_MESSAGE_OPENED_FILE %i %i\n", uiBodyLen, sizeof(flv_demux_struct));
							if (uiBodyLen == sizeof(flv_demux_struct))
							{
								flv_demux_struct *fds = (flv_demux_struct*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&system_mutex);
								memcpy(&rsiRemoteStatus.flv_info, fds, sizeof(flv_demux_struct));
								rsiRemoteStatus.FileOpened = 1;
								rsiRemoteStatus.NewPos = 500;
								rsiRemoteStatus.PlayStatus = 1;
								//rsiRemoteStatus.ID = 
								//memcpy(rsiRemoteStatus.Address, trInHeaderData->Address, sizeof(rsiRemoteStatus.Address));
								DBG_MUTEX_UNLOCK(&system_mutex);
								add_sys_cmd_in_list(SYSTEM_CMD_OPENED_FILE, 0);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_CLOSED_FILE:
							//printf("(server) 3. TYPE_MESSAGE_CLOSED_FILE\n");
							//if (uiBodyLen == sizeof(flv_demux_struct))
							{
								DBG_MUTEX_LOCK(&system_mutex);
								memset(&rsiRemoteStatus.flv_info, 0, sizeof(flv_demux_struct));
								rsiRemoteStatus.FileOpened = 0;
								rsiRemoteStatus.PlayStatus = 0;
								DBG_MUTEX_UNLOCK(&system_mutex);
								add_sys_cmd_in_list(SYSTEM_CMD_CLOSED_FILE, 0);		
							} //else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_ERROR_FILE:
							//printf("(server) 3. TYPE_MESSAGE_ERROR_FILE\n");
							DBG_MUTEX_LOCK(&system_mutex);
							memset(&rsiRemoteStatus.flv_info, 0, sizeof(flv_demux_struct));
							rsiRemoteStatus.FileOpened = 0;
							rsiRemoteStatus.flv_info.Crashed = 1;
							rsiRemoteStatus.PlayStatus = 0;
							DBG_MUTEX_UNLOCK(&system_mutex);
							add_sys_cmd_in_list(SYSTEM_CMD_ERROR_FILE, 0);		
							break;
						case TYPE_MESSAGE_CHANGE_PLAY_FILE_POS:
							//printf("(server) 3. TYPE_MESSAGE_CHANGE_PLAY_FILE_POS\n");
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&stream_mutex);
								cStreamerExData[0] = puiData[0];
								DBG_MUTEX_UNLOCK(&stream_mutex);	
								tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_PLAY_POS);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;
						case TYPE_MESSAGE_DONE_FILE:
							//printf("(server) 3. TYPE_MESSAGE_DONE_FILE\n");
							add_sys_cmd_in_list(SYSTEM_CMD_DONE_FILE, 0);							
							break;
						case TYPE_MESSAGE_BUSY_FILE:
							//printf("(server) 3. TYPE_MESSAGE_BUSY_FILE\n");
							add_sys_cmd_in_list(SYSTEM_CMD_BUSY_FILE, 0);	
							break;						
						case TYPE_MESSAGE_FREE_FILE:
							//printf("(server) 3. TYPE_MESSAGE_FREE_FILE\n");
							add_sys_cmd_in_list(SYSTEM_CMD_FREE_FILE, 0);	
							break;						
						case TYPE_MESSAGE_GET_CLOSE_FILE:
							//printf("(server) 3. TYPE_MESSAGE_GET_CLOSE_FILE\n");
							tx_eventer_send_event(&strmevent_evnt, STRM_EVENT_GET_CLOSE_FILE);
							break;
						case TYPE_MESSAGE_NEW_FILE_POS:
							//printf("(server) 3. TYPE_MESSAGE_NEW_FILE_POS\n");
							if (uiBodyLen == sizeof(int))
							{
								puiData = (unsigned int*)(trInBuffer + sizeof(TRANSFER_DATA));
								DBG_MUTEX_LOCK(&system_mutex);
								rsiRemoteStatus.flv_info.CurrentScrollFrame = puiData[0];								
								DBG_MUTEX_UNLOCK(&system_mutex);
								//add_sys_cmd_in_list(SYSTEM_CMD_NEW_FILE_POS, 0);
							} else dbgprintf(2,"Wrong len packet for type %i (%s)\n",trInHeaderData->TypeMessage, getnamemessage(trInHeaderData->TypeMessage));	
							break;							
						default:
							break;				
					}
					if (cRecv == 1)
					{							
						cRecv = 0;
						iPos += trInHeaderData->SizeMessage;
						//printf("recv packet:%i/%i\n",trInHeaderData->SizeMessage, recv_len);						
					}									
				} 
				else 
				{
					if (trInHeaderData->SizeMessage <= (Connects_Info[Num].InDataSize - iPos))
					{
						dbgprintf(2,"Bad packet:Type:%i, K1:%i,K2:%i,K3:%i,Recv:%i,CRC:%i\n",trInHeaderData->TypeMessage
																		,(trInHeaderData->Key1 == TRANSFER_KEY1) 
																		,(trInHeaderData->Key2 == TRANSFER_KEY2) 
																		,(trInHeaderData->Key3 == TRANSFER_KEY3)
																		,(trInHeaderData->SizeMessage <= (Connects_Info[Num].InDataSize - iPos))
																		,(CalcCRC(trInBuffer, trInHeaderData->SizeMessage - sizeof(int)) == *pCRC));
						memcpy(&trInHeaderData->Address, &Address, sizeof(Address));						
						iPos++;						
					}
					else
					{
						//printf("Half packet %i/%i\n",recv_len,trInHeaderData->SizeMessage);
						memcpy(&trInHeaderData->Address, &Address, sizeof(Address));							
						iLoop = 0;
					}
				}
			} 
			else 
			{
				//printf("Need more data\n");
				iLoop = 0;
			}
		}
		//printf("ReWorkTraffic OUT %i, %i\n", Connects_Info[Num].InDataSize, iPos);			
		if (((Connects_Info[Num].InDataSize - iPos) < sizeof(TRANSFER_DATA)) || (iLoop == 0))
		{
			memmove(Connects_Info[Num].InBuffer, &Connects_Info[Num].InBuffer[iPos], Connects_Info[Num].InDataSize - iPos);
			Connects_Info[Num].InDataSize = Connects_Info[Num].InDataSize - iPos;			
			iPos = 0;
			if (Connects_Info[Num].InDataSize < sizeof(TRANSFER_DATA)) iLoop = 0;
		}
		//printf("ReWorkTraffic OUT2 %i, %i\n", Connects_Info[Num].InDataSize, iPos);	
	}		
	//printf("ReWorkTraffic DONE %i, %i\n", Connects_Info[Num].InDataSize, iPos);	
	DBG_LOG_OUT();	
	return result;
}

int SendTCPMessage(unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen, struct sockaddr_in *Address)
{
	DBG_LOG_IN();
	int n, ret, iConnMax;
	n = 0;
	if (Address)
	{
		ret = 0;
		DBG_MUTEX_LOCK(&Network_Mutex);
		iConnMax = Connects_Max_Active;
		DBG_MUTEX_UNLOCK(&Network_Mutex);
		unsigned int uiTypeTraff = TRAFFIC_SERVICE;			
		if ((Type > TYPE_MESSAGE_FILELIST_LOW_GRID) && (Type < TYPE_MESSAGE_FILELIST_HIGH_GRID)) uiTypeTraff = TRAFFIC_REMOTE_FILE;
		if ((Type > TYPE_MESSAGE_COPY_FILE_LOW_GRID) && (Type < TYPE_MESSAGE_COPY_FILE_HIGH_GRID)) uiTypeTraff = TRAFFIC_TRANSFER_FILE;
			
		for (n = 1; n < iConnMax; n++)
		{
			DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);	
			if ((Connects_Info[n].Status == CONNECT_STATUS_ONLINE) 
				&& (Connects_Info[n].Type == CONNECT_CLIENT) 
				&& (Connects_Info[n].Addr.sin_addr.s_addr == Address->sin_addr.s_addr)
				&& (Connects_Info[n].Addr.sin_port == Address->sin_port)
				&& (Connects_Info[n].TraffType == uiTypeTraff))
				{
					ret = 1;
					/*if (Type == TYPE_MESSAGE_COPY_FILE_DATA)
					{
						printf("Send DATA %i %i %i\n", n, uiTypeTraff, Connects_Info[n].OutBufferSize);
					}*/
					DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);	
					break;
				}
			DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);			
		}
		
		if (ret == 0)
		{
			//printf("%i sec to addr:%s\n", Type, inet_ntoa(Address->sin_addr));
			unsigned int uiID;
			
			n = TCP_Client(Address, uiTypeTraff, 0, 0, &uiID);			
			//printf("ret to addr:%i\n", ret);			
			if (n <= 0) 
			{
				n = 0;
				dbgprintf(2,"Error connect to addr: %s for %i (%s)\n", inet_ntoa(Address->sin_addr), Type, getnamemessage(Type));			
			}
		}		
	}
	if (n != 0)
	{
		//printf("TCP connect OK\n");		
		DBG_MUTEX_LOCK(&Connects_Info[n].Socket_Mutex);	
		ret = SendMessage(n, Type, InfoBuffer, InfoSize, cData, iLen, Address);
		DBG_MUTEX_UNLOCK(&Connects_Info[n].Socket_Mutex);
	}
	else
	{
		//printf("NO TCP connect\n");	
		ret = 0;
	}	
	
	DBG_LOG_OUT();
	return ret;
}

int SendUDPMessage(unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen, struct sockaddr_in *Address)
{
	int ret;
	if (Connects_Info)
	{
		DBG_MUTEX_LOCK(&Connects_Info[0].Socket_Mutex);
		ret = SendMessage(0, Type, InfoBuffer, InfoSize, cData, iLen, Address);
		DBG_MUTEX_UNLOCK(&Connects_Info[0].Socket_Mutex);
	}
	return ret;
}

int SendBroadCastMessage(char ucBCastTCP, unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen)
{
	if (!ucBCastTCP) return SendUDPMessage(Type, InfoBuffer, InfoSize, cData, iLen, NULL);
	
	int b;
	struct sockaddr_in ModAddress;
	DBG_MUTEX_LOCK(&modulelist_mutex);
	for (b = 0; b < iModuleCnt; b++)
	{
		if ((miModuleList[b].Enabled & 1) /*&& (miModuleList[b].Local == 0)*/ && (miModuleList[b].Type == MODULE_TYPE_SYSTEM))
		{									
			memcpy(&ModAddress, &miModuleList[b].Address, sizeof(ModAddress));	
			DBG_MUTEX_UNLOCK(&modulelist_mutex);
			SendTCPMessage(Type, (char*)InfoBuffer, InfoSize, cData, iLen, &ModAddress);
			DBG_MUTEX_LOCK(&modulelist_mutex);
		}
	}
	DBG_MUTEX_UNLOCK(&modulelist_mutex);
	
	return 1;
}

int SendMessage(unsigned int ConnNum, unsigned int Type, void *InfoBuffer, int InfoSize, char *cData, int iLen, struct sockaddr_in *Address)
{
	DBG_LOG_IN();
	//if (Type == TYPE_MESSAGE_REQUEST_MODULE_STATUS) dbg_func_print_entered();
	//printf("%i sec to addr:%s, %s\n", Type, inet_ntoa(Connects_Info[ConnNum].Addr.sin_addr), Address ? inet_ntoa(Address->sin_addr): "null");
			
	if (Connects_Info[ConnNum].Status != CONNECT_STATUS_ONLINE) 
	{
		DBG_LOG_OUT();	
		return -1;
	}
	int result = 0;				
	unsigned int *pCRC;
	if ((Connects_Info[ConnNum].OutBufferSize - Connects_Info[ConnNum].OutDataSize) >= (sizeof(TRANSFER_DATA) + iLen + InfoSize + sizeof(int)))
	{
		char *trOutBuffer = Connects_Info[ConnNum].OutBuffer + Connects_Info[ConnNum].OutDataSize;
		TRANSFER_DATA *trOutHeaderData;	
		//memset(trOutBuffer, 0, Connects_Info[ConnNum].OutBufferSize - ConnInfo->OutDataSize);
		trOutHeaderData = (TRANSFER_DATA*)trOutBuffer;
		trOutHeaderData->Key1 = TRANSFER_KEY1; 
		trOutHeaderData->Key2 = TRANSFER_KEY2; 
		trOutHeaderData->Key3 = TRANSFER_KEY3;		
		trOutHeaderData->TypeMessage = Type;		
		trOutHeaderData->SizeMessage = sizeof(TRANSFER_DATA) + iLen + InfoSize + sizeof(int);
		if ((iLen + InfoSize) == 0) trOutHeaderData->FlagMessage = FLAG_MESSAGE_EMPTY_DATA; 
		else 
		{
			trOutHeaderData->FlagMessage = FLAG_MESSAGE_EXIST_DATA;
			if (InfoSize != 0) memcpy(trOutBuffer + sizeof(TRANSFER_DATA), InfoBuffer, InfoSize);
			if (iLen != 0) memcpy(trOutBuffer + sizeof(TRANSFER_DATA) + InfoSize, cData, iLen);
		}
		pCRC = (unsigned int*)(trOutBuffer + (trOutHeaderData->SizeMessage - sizeof(int)));
		*pCRC = CalcCRC(trOutBuffer, trOutHeaderData->SizeMessage - sizeof(int));	
		memset(&trOutHeaderData->Address, 0, sizeof(trOutHeaderData->Address));     
		trOutHeaderData->Address.sin_family = AF_INET;
		trOutHeaderData->Address.sin_port = htons(UDP_PORT);
		//printf("%i, %i\n",htonl(INADDR_BROADCAST),(int)Address);
		if (Address == NULL)
		{
			if (cUdpTargetAddress[0]) trOutHeaderData->Address.sin_addr.s_addr = aUDPTargetAddress.sin_addr.s_addr;
				else trOutHeaderData->Address.sin_addr.s_addr = 0; //htonl(INADDR_BROADCAST); 
		}
		else trOutHeaderData->Address.sin_addr.s_addr = Address->sin_addr.s_addr;	
//if (Type == TYPE_MESSAGE_MODULE_SET) omx_dump_data("packet.bin", trOutBuffer, trOutHeaderData->SizeMessage);			
		Connects_Info[ConnNum].OutDataSize += trOutHeaderData->SizeMessage;
		//printf("To send : %i/%i\n",trOutHeaderData->SizeMessage, ConnInfo->OutDataSize);		
		result = 1;
	} 
	else dbgprintf(3,"Out buffer overfull: Type:%i(%s), Size:%i\n", Type, getnamemessage(Type), Connects_Info[ConnNum].OutBufferSize);
		
	
	SendSignalType(ConnNum, SIGNAL_SEND);	
	
	DBG_LOG_OUT();	
	return result;
}

void SendSignalType(unsigned int uiConnNum, char cType)
{
	DBG_MUTEX_LOCK(&Connects_Info[uiConnNum].Pair_Mutex);
	if (Connects_Info[uiConnNum].socketpair[0] != 0)
	{
		//printf("SendSignalType %i %i\n", cType, uiConnNum);
		int rv = write(Connects_Info[uiConnNum].socketpair[0], &cType, 1);
		if (rv < 1) dbgprintf(5, "SendSignalType: write socketpair signal %i (errno:%i, %s)\n", cType, errno, strerror(errno));	
	}
	DBG_MUTEX_UNLOCK(&Connects_Info[uiConnNum].Pair_Mutex);	
}

char SendVideoSubFrames(char *Buffer, int LenData, unsigned int iNumFrame, int iClientNum)
{
	DBG_LOG_IN();
	
	char *Buff = Buffer;
	int iLenSubFrame = 0;
	int Len = LenData;	
	
	if ((Connects_Info[iClientNum].OutDataSize + LenData) > Connects_Info[iClientNum].OutBufferSize) 
	{
		dbgprintf(2,"TCP buffer overfull, skiping frame\n");
		return 0;
	}
	
	VIDEO_SUBFRAME_INFO VidSubFrame;
	VidSubFrame.Number = iNumFrame;
	VidSubFrame.SubNumber = 0;
	VidSubFrame.Position = 0;
	
	do
	{
		if (Len > TCP_PACKET_SIZE) iLenSubFrame = TCP_PACKET_SIZE; else iLenSubFrame = Len;
		VidSubFrame.LenData = iLenSubFrame;
			
		SendMessage(iClientNum, TYPE_MESSAGE_SUB_VIDEO_FRAME_DATA, (void*)&VidSubFrame, sizeof(VIDEO_SUBFRAME_INFO), Buff, iLenSubFrame, &Connects_Info[iClientNum].Addr);
		
		Buff += iLenSubFrame;
		VidSubFrame.SubNumber++;
		VidSubFrame.Position += iLenSubFrame;
		Len -= iLenSubFrame;
	} while (Len != 0);	
	DBG_LOG_OUT();	
	return 1;
}

char SendAudioSubFrames(char *Buffer, int LenData, unsigned int iNumFrame, int iClientNum)
{
	DBG_LOG_IN();
	
	char *Buff = Buffer;
	int iLenSubFrame = 0;
	int Len = LenData;	
	
	if ((Connects_Info[iClientNum].OutDataSize + LenData) > Connects_Info[iClientNum].OutBufferSize) 
	{
		dbgprintf(2,"TCP buffer overfull, skiping frame\n");
		return 0;
	}
	/*{
		dbgprintf(2,"TCP buffer overfull, waiting ...\n");
		//CopyConnectsInfo(0);
		if ((Connects_Info[iClientNum].OutDataSize + LenData) <= SERV_TRAFFIC_BUFFER_SIZE) dbgprintf(2,"TCP buffer DBG_FREE(d, go go go\n");
	}*/
	/*{
		dbgprintf(2,"TCP buffer overfull, refresh\n");
		Connects_Info[iClientNum].OutDataSize -= TCP_LOST_BLOCK_SIZE;
		memcpy(Connects_Info[iClientNum].OutBuffer, &Connects_Info[iClientNum].OutBuffer[TCP_LOST_BLOCK_SIZE], Connects_Info[iClientNum].OutDataSize);
	}	*/
	
	AUDIO_SUBFRAME_INFO AudioSubFrame;
	AudioSubFrame.Number = iNumFrame;
	AudioSubFrame.SubNumber = 0;
	AudioSubFrame.Position = 0;
	do
	{
		if (Len > TCP_PACKET_SIZE) iLenSubFrame = TCP_PACKET_SIZE; else iLenSubFrame = Len;
		AudioSubFrame.LenData = iLenSubFrame;
		SendMessage(iClientNum, TYPE_MESSAGE_SUB_AUDIO_FRAME_DATA, (void*)&AudioSubFrame, sizeof(AUDIO_SUBFRAME_INFO), Buff, iLenSubFrame, &Connects_Info[iClientNum].Addr);
		Buff += iLenSubFrame;
		AudioSubFrame.SubNumber++;
		AudioSubFrame.Position += iLenSubFrame;
		Len -= iLenSubFrame;
	} while (Len != 0);
	
	DBG_LOG_OUT();	
	return 1;
}

int SendVideoFrame(unsigned int iNumFrame, char* Buffer, int BufferSize, int LenData, int Flag, void* tVideoInfo, void* StartPack, int iTraffType)
{
	DBG_LOG_IN();
													
	omx_start_packet *Start_Packet = (omx_start_packet*)StartPack;
	VideoCodecInfo *VideoParams = (VideoCodecInfo*)tVideoInfo;
	
	int i;
	int ret = 0;
	
	VIDEO_FRAME_INFO VidFrame;
	ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	memset(&VidFrame, 0, sizeof(VIDEO_FRAME_INFO));	
	VidFrame.Number = iNumFrame;
	VidFrame.LenData = LenData;
	VidFrame.Flag = Flag;
	VidFrame.CRC = CalcCRC(Buffer, LenData);
	
	ret = 0;
	for (i = 1; i < iConnMax; i++) 
	{
		DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);	
		if ((Connects_Info[i].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[i].Type == CONNECT_SERVER) && (Connects_Info[i].TraffType == iTraffType))
		{
			if (Start_Packet->SendType && (Connects_Info[i].NeedData & FLAG_START_VIDEO_FRAME) && (Connects_Info[i].DataNum > 0) && (Flag & OMX_BUFFERFLAG_SYNCFRAME)) 
					Connects_Info[i].NeedData ^= FLAG_START_VIDEO_FRAME;
			if (((Connects_Info[i].NeedData & FLAG_NEXT_VIDEO_FRAME) != 0) || (((Connects_Info[i].NeedData & FLAG_VIDEO_STREAM) != 0) && ((Connects_Info[i].NeedData & FLAG_START_VIDEO_FRAME) == 0)))
			{
				int iTimer = 50;
				while (((Connects_Info[i].OutDataSize + LenData + sizeof(VIDEO_FRAME_INFO)) > Connects_Info[i].OutBufferSize) && (iTimer > 0))
				{
					if (iTimer == 50) dbgprintf(3,"TCP buffer overfull (%i>>>%i), waiting ...\n", 
									Connects_Info[i].OutDataSize + LenData + sizeof(VIDEO_FRAME_INFO), Connects_Info[i].OutBufferSize);
					iTimer--;
					DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);
					SendSignalType(i, SIGNAL_SEND);						
					usleep(100000);
					DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);
					if ((Connects_Info[i].Status != CONNECT_STATUS_ONLINE) || (Connects_Info[i].Type != CONNECT_SERVER) || (Connects_Info[i].TraffType != iTraffType))
					{
						iTimer = -1;
						break;
					}		
					if ((Connects_Info[i].OutDataSize + LenData + sizeof(VIDEO_FRAME_INFO)) <= Connects_Info[i].OutBufferSize) dbgprintf(3,"TCP buffer DBG_FREE(d, go go go\n");	
				}
				if (iTimer == -1) dbgprintf(2,"TCP buffer overfull, disconnected (lost)\n");
				if (iTimer == 0)
				{
					dbgprintf(2,"TCP buffer overfull, disconnecting\n");
					SendSignalType(i, SIGNAL_CLOSE);
				}
				if (iTimer > 0)
				{
					//printf("Next video frame(send): %i %i %i %i\n", (int)Buffer, i, LenData, iNumFrame);
					SendMessage(i, TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA, (char*)&VidFrame, sizeof(VIDEO_FRAME_INFO), NULL, 0, &Connects_Info[i].Addr);
					//dbgprintf(3,"done send %i\n",ret);		
					SendVideoSubFrames(Buffer, LenData, iNumFrame, i);
					ret = 1;
				}
				if (Connects_Info[i].NeedData & FLAG_NEXT_VIDEO_FRAME) Connects_Info[i].NeedData ^= FLAG_NEXT_VIDEO_FRAME;							
			}
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);	
	}	
	//dbgprintf(3,"SendVideoFrame done\n");	
	memset(&VidFrame, 0, sizeof(VIDEO_FRAME_INFO));	
	ret = 0;
	
	int64_t iTm = 0;
							
	for (i = 1; i < iConnMax; i++) 
	{
		DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);
		if ((Connects_Info[i].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[i].Type == CONNECT_SERVER) && (Connects_Info[i].TraffType == iTraffType))
		{
			iTm = 0;
			get_ms(&iTm);
		
			if (((Connects_Info[i].NeedData & FLAG_NEXT_VIDEO_FRAME) != 0) || (((Connects_Info[i].NeedData & FLAG_VIDEO_STREAM) != 0) 
				&& ((Connects_Info[i].NeedData & FLAG_START_VIDEO_FRAME) == 0)))
			{
				ret |= 2;				
			}
			//if (((Connects_Info[i].NeedData & FLAG_VIDEO_CODEC_INFO) != 0) && (Start_Packet->CodecInfoFilled == 0))	dbgprintf(3,"CodecInfoFilled\n");	
			if (((Connects_Info[i].NeedData & FLAG_VIDEO_PARAMS) != 0) && (VideoParams->Filled == 1))	
			{
				ret |= 1;
				//printf("SendMessage %i TYPE_MESSAGE_VIDEO_PARAMS\n", i);	
				//dbgprintf(4, "send TYPE_MESSAGE_VIDEO_PARAMS %i\n",(unsigned int)get_ms(&iTm));
				SendMessage(i, TYPE_MESSAGE_VIDEO_PARAMS, NULL, 0, (char*)VideoParams, sizeof(VideoCodecInfo), &Connects_Info[i].Addr);
				//dbgprintf(4, "sended TYPE_MESSAGE_VIDEO_PARAMS %i\n",(unsigned int)get_ms(&iTm));
				Connects_Info[i].NeedData ^= FLAG_VIDEO_PARAMS;
			}
			if (((Connects_Info[i].NeedData & FLAG_VIDEO_CODEC_INFO) != 0) && (Start_Packet->CodecInfoFilled == 1))	
			{
				ret |= 1;
				//printf("SendMessage %i TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA\n", i);	
				//dbgprintf(4, "send TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA %i\n",(unsigned int)get_ms(&iTm));
				SendMessage(i, TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA, NULL, 0, Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen, &Connects_Info[i].Addr);
				//dbgprintf(4, "sended TYPE_MESSAGE_VIDEO_CODEC_INFO_DATA %i\n",(unsigned int)get_ms(&iTm));
				Connects_Info[i].NeedData ^= FLAG_VIDEO_CODEC_INFO;
			}
			if (((Connects_Info[i].NeedData & FLAG_START_VIDEO_FRAME) != 0) && (Start_Packet->StartFrameFilled == 2)) 
			{				
				//SendMessage(&Connects_Info[0], TYPE_MESSAGE_START_VIDEO_FRAME_DATA, Start_Packet->BufferStartFrame, Start_Packet->StartFrameLen, &Connects_Info[i].Addr);
				//printf("SendMessage %i TYPE_MESSAGE_START_VIDEO_FRAME_DATA\n", i);	
				int iEvent;
				if (Start_Packet->SendType == 0)
				{
					ret |= 1;
					int iStLen = Start_Packet->StartFrameLen;
					int iCurLen = 0;
					int iStPos = 0;
					int iTimer = 50;
					iEvent = TYPE_MESSAGE_START_VIDEO_FRAME_DATA;
					VidFrame.Number = 0;					
					VidFrame.Flag = OMX_BUFFERFLAG_STARTTIME;
					do
					{
						if (iStLen > BufferSize) iCurLen = BufferSize; else iCurLen = iStLen;
						VidFrame.LenData = iCurLen;
						VidFrame.CRC = CalcCRC(Start_Packet->BufferStartFrame + iStPos, iCurLen);
						while (((Connects_Info[i].OutDataSize + iCurLen + sizeof(VIDEO_FRAME_INFO)) > Connects_Info[i].OutBufferSize) && (iTimer > 0))
						{
							if (iTimer == 50) dbgprintf(3,"TCP buffer overfull, waiting ...\n");
							iTimer--;
							DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);
							SendSignalType(i, SIGNAL_SEND);						
							usleep(100000);
							DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);
							if ((Connects_Info[i].Status != CONNECT_STATUS_ONLINE) || (Connects_Info[i].Type != CONNECT_SERVER) || (Connects_Info[i].TraffType != iTraffType))
							{
								iTimer = -1;
								break;
							}		
							if ((Connects_Info[i].OutDataSize + iCurLen + sizeof(VIDEO_FRAME_INFO)) <= Connects_Info[i].OutBufferSize) dbgprintf(3,"TCP buffer DBG_FREE(d, go go go\n");	
						}
						if (iTimer == -1) 
						{
							dbgprintf(2,"TCP buffer overfull, disconnected\n");
							break;
						}
						if (iTimer == 0)
						{
							dbgprintf(2,"TCP buffer overfull, disconnecting\n");
							SendSignalType(i, SIGNAL_CLOSE);
							break;
						}
						//printf("SendMessage %i TYPE_MESSAGE_START_VIDEO_FRAME_DATA %i %i %i\n", i, iCurLen, iNumFrame, VidFrame.Number);					
						//dbgprintf(4, "send TYPE_MESSAGE_START_VIDEO_FRAME_DATA %i %i\n",(unsigned int)get_ms(&iTm), VidFrame.Number);
						//printf("send TYPE_MESSAGE_START_VIDEO_FRAME_DATA %i %i\n",(unsigned int)get_ms(&iTm), VidFrame.Number);
						//printf("Start video frame(send): %i %i %i %i\n", (int)Start_Packet->BufferStartFrame + iStPos, i, iCurLen, VidFrame.Number);
						SendMessage(i, iEvent, (char*)&VidFrame, sizeof(VIDEO_FRAME_INFO), NULL, 0, &Connects_Info[i].Addr);
						//dbgprintf(4, "send SendVideoSubFrames %i %i\n",(unsigned int)get_ms(&iTm), VidFrame.Number);
						//printf("send SendVideoSubFrames %i %i\n",(unsigned int)get_ms(&iTm), VidFrame.Number);
						SendVideoSubFrames(Start_Packet->BufferStartFrame + iStPos, iCurLen, VidFrame.Number, i);
						//dbgprintf(4, "sended SendVideoSubFrames %i %i\n",(unsigned int)get_ms(&iTm), VidFrame.Number);
						//printf("sended SendVideoSubFrames %i %i\n",(unsigned int)get_ms(&iTm), VidFrame.Number);
						iEvent = TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA;
						VidFrame.Flag = OMX_BUFFERFLAG_TIMESTAMPINVALID;
						iStLen -= iCurLen;
						iStPos += iCurLen;					
						//printf("Start Frame(%i):%i/%i, LenData:%i, Flag:%i, BufferOUT: %i/%i\n", i,VidFrame.Number,iNumFrame,iCurLen,VidFrame.Flag,Connects_Info[i].OutDataSize,SERV_TRAFFIC_BUFFER_SIZE);
						VidFrame.Number++;
						//printf("StLen %i iCurLen %i\n", iStLen, iCurLen);
					} while (iStLen > 0);
					if (iStLen == 0) Connects_Info[i].NeedData ^= FLAG_START_VIDEO_FRAME;
				}
				else
				{
					if ((Start_Packet->BufferStartSize > Connects_Info[i].DataPos)
						&& (VideoParams->video_intra_frame > Connects_Info[i].DataNum)
						&& (Start_Packet->StartFramesFlags[Connects_Info[i].DataNum] == 2) 
						&& (Connects_Info[i].OutDataSize + Start_Packet->StartFramesSizes[VidFrame.Number] + sizeof(VIDEO_FRAME_INFO)) <= Connects_Info[i].OutBufferSize)
					{
						ret |= 1;
						memset(&VidFrame, 0, sizeof(VIDEO_FRAME_INFO));	
						VidFrame.Number = Connects_Info[i].DataNum;
						VidFrame.LenData = Start_Packet->StartFramesSizes[VidFrame.Number];
						if (VidFrame.Number == 0) 
						{
							VidFrame.Flag = OMX_BUFFERFLAG_STARTTIME; 
							iEvent = TYPE_MESSAGE_START_VIDEO_FRAME_DATA;
						}
						else 
						{
							VidFrame.Flag = OMX_BUFFERFLAG_TIMESTAMPINVALID;
							iEvent = TYPE_MESSAGE_NEXT_VIDEO_FRAME_DATA;
						}
						VidFrame.CRC = CalcCRC(&Start_Packet->BufferStartFrame[Connects_Info[i].DataPos], VidFrame.LenData);
						//printf("Next start video frame(send): %i %i %i\n", i, VidFrame.LenData, VidFrame.Number);
						//printf("Start video frame(send): %i %i %i %i\n", (int)&Start_Packet->BufferStartFrame[Connects_Info[i].DataPos], i, VidFrame.LenData, VidFrame.Number);
						SendMessage(i, iEvent, (char*)&VidFrame, sizeof(VIDEO_FRAME_INFO), NULL, 0, &Connects_Info[i].Addr);
						//dbgprintf(3,"done send %i\n",ret);		
						SendVideoSubFrames(&Start_Packet->BufferStartFrame[Connects_Info[i].DataPos], VidFrame.LenData, VidFrame.Number, i);
						
						Connects_Info[i].DataPos += VidFrame.LenData;
						Connects_Info[i].DataNum++;						
					}
				}
			}
			//dbgprintf(4, "sended done %i\n",(unsigned int)get_ms(&iTm));			
		}
					
		DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);
	}	
	
	if ((ret & 2) == 0) 
	{
		dbgprintf(6, "No connects for Send video frame %i\n", iTraffType);
		DBG_LOG_OUT();		
		return -1;
	}
	
	DBG_LOG_OUT();	
	return ret;
}

int RecvVideoFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* StartPack, char cStream, unsigned int ConnectNum, unsigned int ConnectID)
{		
	DBG_LOG_IN();
				
	AVPacket *avpkt = (AVPacket*)AVBuffer;
	int BuffSize = avpkt->size;
	
	omx_start_packet *Start_Packet = (omx_start_packet*)StartPack;
	
	int result = 0;
	int ret;
	struct sockaddr_in Address;
	struct sockaddr_in ConnAddress;
	int timeout = RECV_FRAME_TIMEOUT_MS;
	int iRecvLen = 0;
	char *PacketBody;
	TRANSFER_DATA *PacketHeader;
	unsigned int uiRecvPacketSize = 0;
	avpkt->size = 0;
	avpkt->flags = 0;
	VIDEO_FRAME_INFO VidFrame;
	VIDEO_SUBFRAME_INFO *pVidSubFrame;	
	memset(&VidFrame,0,sizeof(VIDEO_FRAME_INFO));
	
	ret = 0;
	DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);		
	if ((Connects_Info[ConnectNum].Status == CONNECT_STATUS_ONLINE) 
		&& (Connects_Info[ConnectNum].Type == CONNECT_CLIENT)
		&& (Connects_Info[ConnectNum].ID == ConnectID)) 
		{
			ret = 1;
		}
	DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);		
	
	if (ret == 0) 
	{
		dbgprintf(2, "No connects for Recv video frame %i %i\n", ConnectNum, ConnectID);
		DBG_LOG_OUT();		
		return -1;	
	}
	if (Start_Packet->VideoParams.Filled == 0)
	{
		if (cStream == 0) SendMessage(ConnectNum, TYPE_MESSAGE_REQUEST_VIDEO_PARAMS, NULL, 0, NULL, 0, &Connects_Info[ConnectNum].Addr);
		
		pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);
		SendSignalType(ConnectNum, SIGNAL_WORK);		
		//dbgprintf("waiting TYPE_MESSAGE_VIDEO_PARAMS\n");
		timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, EVENT_VIDEO_PARAMS, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
		if (timeout != 0)
		{
			PacketHeader = (TRANSFER_DATA*)PacketBody;
			PacketBody += sizeof(TRANSFER_DATA);	
			ret = PacketHeader->SizeMessage - sizeof(TRANSFER_DATA) - sizeof(int);			
			//dbgn_printf("save BufferCodecInfo size %i\n", Start_Packet->CodecInfoLen);
			if (ret == sizeof(VideoCodecInfo))
			{
				memcpy(&Start_Packet->VideoParams, PacketBody, sizeof(VideoCodecInfo));	
				if (Connects_Info[ConnectNum].NeedData & FLAG_VIDEO_PARAMS) Connects_Info[ConnectNum].NeedData ^= FLAG_VIDEO_PARAMS;
				if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
					dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
				Start_Packet->VideoParams.Filled = 1;
				ret = ((Start_Packet->VideoParams.video_width * Start_Packet->VideoParams.video_height * 3) / 2) * (Start_Packet->VideoParams.video_intra_frame / 10);
				if (Start_Packet->BufferStartSize < ret)
				{
					Start_Packet->BufferStartSize = ret;
					Start_Packet->BufferStartFrame = (char*)DBG_MALLOC(Start_Packet->BufferStartSize);  				
				}
				//printf("video params info(%i): len:%i, filled:%i, CRC:%i\n",Connects_Info[ConnectNum].NeedData,Start_Packet->CodecInfoLen,Start_Packet->CodecInfoFilled, CalcCRC(Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen));
			} else dbgprintf(2,"error video params size\n");
		} 
		else 
		{
			DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);		
			dbgprintf(2,"timeout EVENT_VIDEO_PARAMS(%i)flag:%i\n",ConnectNum,Connects_Info[ConnectNum].NeedData);
			DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);		
			DBG_LOG_OUT();	
			return 0;
		}
	}
	if (Start_Packet->CodecInfoFilled == 0)
	{
		if (cStream == 0) SendMessage(ConnectNum, TYPE_MESSAGE_REQUEST_VIDEO_CODEC_INFO, NULL, 0, NULL, 0, &Connects_Info[ConnectNum].Addr);
		
		pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);		
		SendSignalType(ConnectNum, SIGNAL_WORK);		
		timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, EVENT_VIDEO_CODEC_INFO_DATA, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
		if (timeout != 0)
		{
			PacketHeader = (TRANSFER_DATA*)PacketBody;
			PacketBody += sizeof(TRANSFER_DATA);	
			Start_Packet->CodecInfoLen = PacketHeader->SizeMessage - sizeof(TRANSFER_DATA) - sizeof(int);
			//dbgn_printf("save BufferCodecInfo size %i\n", Start_Packet->CodecInfoLen);
			memcpy(Start_Packet->BufferCodecInfo, PacketBody, Start_Packet->CodecInfoLen);	
			if (Connects_Info[ConnectNum].NeedData & FLAG_VIDEO_CODEC_INFO) Connects_Info[ConnectNum].NeedData ^= FLAG_VIDEO_CODEC_INFO;								
			if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
				dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
			Start_Packet->CodecInfoFilled = 1;
			//printf("video Codec info(%i): len:%i, filled:%i, CRC:%i\n",Connects_Info[ConnectNum].NeedData,Start_Packet->CodecInfoLen,Start_Packet->CodecInfoFilled, CalcCRC(Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen));
		} 
		else 
		{
			DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);					
			dbgprintf(2,"timeout EVENT_VIDEO_CODEC_INFO_DATA(%i)flag:%i\n",ConnectNum,Connects_Info[ConnectNum].NeedData);
			DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);					
			DBG_LOG_OUT();		
			return 0;
		}
	}
	
	if (Start_Packet->CodecInfoFilled == 1)
	{
		if ((Start_Packet->CodecInfoLen - Start_Packet->CodecInfoPos) > BuffSize) avpkt->size = BuffSize; else avpkt->size = Start_Packet->CodecInfoLen - Start_Packet->CodecInfoPos;
		memcpy(avpkt->data, &Start_Packet->BufferCodecInfo[Start_Packet->CodecInfoPos], avpkt->size);
		Start_Packet->CodecInfoPos += avpkt->size;
		*Flag = OMX_BUFFERFLAG_CODECCONFIG;
		if (Start_Packet->CodecInfoLen == Start_Packet->CodecInfoPos) 
		{
			Start_Packet->CodecInfoFilled = 2;
			*Flag |= OMX_BUFFERFLAG_ENDOFFRAME;
		}
		//printf("video Codec packet recv:Buffersize:%i,Numframe:%i, DataLen:%i, Flag:%i, ret:%i, timeout:%i\n",BuffSize,*iNumFrame,avpkt->size,*Flag,result, timeout);			
		DBG_LOG_OUT();		
		return 1;
	}		
	
	if (Start_Packet->CodecInfoFilled != 2) 
	{
		DBG_LOG_OUT();		
		return 2;
	}
	//*Flag = OMX_BUFFERFLAG_TIMESTAMPINVALID;
	
	int iLoop = 1;
	int iEventFrame = 0;
	ret = 0;
	int iTime = 0;
	//iTime = timeout;
	while ((iLoop) && (timeout > 0))
	{
		if (ret == 0)
		{
			if (Start_Packet->StartFrameFilled == 0) iEventFrame = EVENT_START_VIDEO_FRAME_DATA; else iEventFrame = EVENT_NEXT_VIDEO_FRAME_DATA;
			DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
			if (cStream == 0)
			{
				if (Start_Packet->StartFrameFilled == 0) 
					SendMessage(ConnectNum, TYPE_MESSAGE_REQUEST_START_VIDEO_FRAME, NULL, 0, NULL, 0, &Connects_Info[ConnectNum].Addr);					
					else 
					{
						SendMessage(ConnectNum, TYPE_MESSAGE_REQUEST_NEXT_VIDEO_FRAME, NULL, 0, NULL, 0, &Connects_Info[ConnectNum].Addr);
						Connects_Info[ConnectNum].NeedData |= FLAG_NEXT_VIDEO_FRAME;
					}
			}
			memcpy(&ConnAddress, &Connects_Info[ConnectNum].Addr, sizeof(ConnAddress));
			DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);
			
			pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);	
			SendSignalType(ConnectNum, SIGNAL_WORK);
			timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, iEventFrame, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
			if (timeout != 0)
			{
				PacketHeader = (TRANSFER_DATA*)PacketBody;
				PacketBody += sizeof(TRANSFER_DATA);	
				memcpy(&Address, &PacketHeader->Address, sizeof(Address));
				memcpy(&VidFrame, PacketBody, sizeof(VIDEO_FRAME_INFO));
				if (ConnAddress.sin_addr.s_addr == Address.sin_addr.s_addr)
				{
					if ((*iNumFrame <= VidFrame.Number) 
						|| (VidFrame.Number == START_VIDEO_FRAME_NUMBER) 
						|| ((*iNumFrame == END_VIDEO_FRAME_NUMBER) && (VidFrame.Number == (START_VIDEO_FRAME_NUMBER + 1))))
					{
						if (timeout < RECV_FRAME_TIMEOUT_MS) timeout = RECV_FRAME_TIMEOUT_MS;
						//dbgprintf(3,"come2: %i\n", VidFrame.Number);
						*iNumFrame = VidFrame.Number;
						avpkt->size = VidFrame.LenData;
						*Flag	   = VidFrame.Flag;
						if (VidFrame.Flag == 0) *Flag = OMX_BUFFERFLAG_TIMESTAMPINVALID;
						iRecvLen = 0;
						if (VidFrame.LenData > BuffSize) 
						{
							iLoop = 0;
							result = 0;
							dbgprintf(2,"Very big video packet %i > %i\n", VidFrame.LenData, BuffSize);
						}	
						else
						{	
							//printf("NewFrameStartLoad\n");
							if (Start_Packet->StartFrameFilled == 0) 
							{
								Start_Packet->StartFrameFilled = 1;
								if (Connects_Info[ConnectNum].NeedData & FLAG_START_VIDEO_FRAME) Connects_Info[ConnectNum].NeedData ^= FLAG_START_VIDEO_FRAME;		
								*Flag |= OMX_BUFFERFLAG_STARTTIME;
							}	
							else 
								if (Connects_Info[ConnectNum].NeedData & FLAG_NEXT_VIDEO_FRAME) Connects_Info[ConnectNum].NeedData ^= FLAG_NEXT_VIDEO_FRAME;							
							memset(avpkt->data, 0, VidFrame.LenData);
							Connects_Info[ConnectNum].NeedData |= FLAG_SUB_VIDEO_FRAME;
							ret = 1;									
						}
					} else dbgprintf(2,"Wrong num video frame: iNumFrame:%i, VidFrame.Number:%i\n",*iNumFrame, VidFrame.Number);
				} else dbgprintf(2,"Alien video data\n");
				if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
					dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
			} 
			else 
			{
				DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);					
				dbgprintf(2,"timeout video frame(%i)flag:%i\n",ConnectNum,Connects_Info[ConnectNum].NeedData);
				DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
			}
		}
		if (ret > 0)
		{
			pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);			
			SendSignalType(ConnectNum, SIGNAL_WORK);			
			iTime = timeout;
			timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, EVENT_SUB_VIDEO_FRAME_DATA, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
			if (timeout != 0)
			{
				PacketHeader = (TRANSFER_DATA*)PacketBody;
				PacketBody += sizeof(TRANSFER_DATA);	
				pVidSubFrame = (VIDEO_SUBFRAME_INFO*)PacketBody;
				if ((pVidSubFrame->Number == VidFrame.Number)
					&& (pVidSubFrame->LenData < (TCP_PACKET_SIZE*2)) 
					&& ((pVidSubFrame->Position + pVidSubFrame->LenData) <= BuffSize))
				{
					if  (((pVidSubFrame->LenData > 0) && (avpkt->data[pVidSubFrame->Position] != 0)) ||
						((pVidSubFrame->LenData > 1) && (avpkt->data[pVidSubFrame->Position+1] != 0)) ||
						((pVidSubFrame->LenData > 2) && (avpkt->data[pVidSubFrame->Position+2] != 0)) ||
						((pVidSubFrame->LenData > 3) && (avpkt->data[pVidSubFrame->Position+3] != 0)))
					{
						dbgprintf(2,"Repeat video sub packet num:%i\n",VidFrame.Number);
					}
					else
					{
						memcpy(&avpkt->data[pVidSubFrame->Position], &PacketBody[sizeof(VIDEO_SUBFRAME_INFO)], pVidSubFrame->LenData);
						iRecvLen += pVidSubFrame->LenData;
						//printf("num:%i, NumSubFrame:%i,LenSubFrame:%i\n", pVidSubFrame->Number, pVidSubFrame->SubNumber, pVidSubFrame->LenData);
						//printf("Fill:%i\n", iRecvLen);	
					}
				} else dbgprintf(2,"Wrong video packet PN:%i, PSN:%i, PSL:%i, PSP:%i\n", VidFrame.Number,pVidSubFrame->Number,pVidSubFrame->LenData,pVidSubFrame->Position);
				if (iRecvLen == VidFrame.LenData)
				{
					if (VidFrame.CRC == CalcCRC((char*)avpkt->data, VidFrame.LenData))
					{
						//printf("DONE video packet\n");
						iLoop = 0;
						result = 1;
						if ((Connects_Info[ConnectNum].NeedData & FLAG_SUB_VIDEO_FRAME) != 0) Connects_Info[ConnectNum].NeedData ^= FLAG_SUB_VIDEO_FRAME;
					}
					else
					{
						dbgprintf(2,"BAD CRC video packet\n");
						iLoop = 0;
						result = 0;
					}
				} 
				else 
				{
					//printf("Loaded video sub packet num:%i, subnum:%i, subsubnum: %i, %i/%i\n",VidFrame.Number, pVidSubFrame->Number, pVidSubFrame->SubNumber,iRecvLen, VidFrame.LenData);
				}
				if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
					dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
				//printf(">>>>:video Numframe:%i, SubNum:%i, DataLen:%i, FrameLen:%i, timeout:%i, recieved:%i\n",pVidSubFrame->Number,pVidSubFrame->SubNumber,pVidSubFrame->LenData,VidFrame.LenData, timeout,iRecvLen);	
			}
			else
			{
				//unsigned int pRecv, pSend;
				//GetConnBufferStatus(&pRecv, &pSend);
				//printf("LAN recv %i   Send %i\n", pRecv, pSend);
				DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
				dbgprintf(2,"timeout video subframe(%i)time:%i, flag:%i, loaded %i from %i\n",ConnectNum,iTime,Connects_Info[ConnectNum].NeedData, iRecvLen , VidFrame.LenData);
				if ((Connects_Info[ConnectNum].NeedData & FLAG_SUB_VIDEO_FRAME) != 0) Connects_Info[ConnectNum].NeedData ^= FLAG_SUB_VIDEO_FRAME;
				DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
				//tx_semaphore_go(Connects_Info[ConnectNum].pevntIP, EVENT_SYNC_SUB_VIDEO_FRAME_DATA, TX_ANY);					
				//usleep(1000);
				//timeout--;
			}		
		}
	}
	//printf("Time=%i\n",iTime-timeout);				
	//printf("0:video Buffersize:%i,Num:%i, Len:%i, Load:%i, Flag:%i, ret:%i, time:%i\n",BuffSize,*iNumFrame,avpkt->size, iRecvLen,*Flag,result, timeout);		
	if (timeout == 0) result = -1;
	
	DBG_LOG_OUT();	
	return result;
}

int SendAudioFrame(unsigned int iNumFrame, unsigned int iFlag, char* Buffer, int LenData, void* pData, int iTraffType)
{
	DBG_LOG_IN();
	
	AudioCodecInfo* tCodecInfo = (AudioCodecInfo*)pData;
	int i;
	int ret = 0;
	AUDIO_FRAME_INFO AudioFrame;
	memset(&AudioFrame, 0, sizeof(AUDIO_FRAME_INFO));	
	AudioFrame.Number = iNumFrame;
	AudioFrame.LenData = LenData;
	AudioFrame.Flag = iFlag;
	AudioFrame.CRC = CalcCRC(Buffer, LenData);
	
	ret = 0;
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	for (i = 1; i < iConnMax; i++) 
	{
		DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);
		//printf("### %i %i %i %i\n", Connects_Info[i].Status == CONNECT_STATUS_ONLINE, Connects_Info[i].Type == CONNECT_SERVER, Connects_Info[i].TraffType, iTraffType);
		if ((Connects_Info[i].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[i].Type == CONNECT_SERVER) && (Connects_Info[i].TraffType == iTraffType))
		{
			if (((Connects_Info[i].NeedData & FLAG_NEXT_AUDIO_FRAME) != 0) || ((Connects_Info[i].NeedData & FLAG_AUDIO_STREAM) != 0))
			{
				ret |= 2;				
			}
			if ((Connects_Info[i].NeedData & FLAG_AUDIO_CODEC_INFO) != 0)	
			{
				ret |= 1;
				SendMessage(i, TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA, NULL, 0, (char*)tCodecInfo, sizeof(AudioCodecInfo), &Connects_Info[i].Addr);
				Connects_Info[i].NeedData ^= FLAG_AUDIO_CODEC_INFO;
				//printf("Sended TYPE_MESSAGE_AUDIO_CODEC_INFO_DATA\n");
			}		
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);
	}
	
	if ((ret & 2) == 0) 
	{
		//printf("audio no body\n");
		DBG_LOG_OUT();	
		return -1;
	}
	
	if ((iFlag & (FLAG_NEXT_AUDIO_FRAME | FLAG_DONE_AUDIO | FLAG_STOP_AUDIO)) == 0)
	{
		DBG_LOG_OUT();
		return ret & 1;
	}
	
	ret = 0;
	for (i = 1; i < iConnMax; i++) 
	{
		DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);		
		if ((Connects_Info[i].Status == CONNECT_STATUS_ONLINE) && (Connects_Info[i].Type == CONNECT_SERVER) && (Connects_Info[i].TraffType == iTraffType))
		{
			if (((Connects_Info[i].NeedData & FLAG_NEXT_AUDIO_FRAME) != 0) || ((Connects_Info[i].NeedData & FLAG_AUDIO_STREAM) != 0))
			{
				if ((Connects_Info[i].OutDataSize + LenData + sizeof(AUDIO_FRAME_INFO)) <= Connects_Info[i].OutBufferSize)
				{
					SendMessage(i, TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA, (char*)&AudioFrame, sizeof(AUDIO_FRAME_INFO), NULL, 0, &Connects_Info[i].Addr);
					//printf("Sended TYPE_MESSAGE_NEXT_AUDIO_FRAME_DATA\n");
					//printf("send AUDIO_SUBFRAME_INFO:%i\n",LenData);
					SendAudioSubFrames(Buffer, LenData, iNumFrame, i);
					ret = 1;
				}
				if (Connects_Info[i].NeedData & FLAG_NEXT_AUDIO_FRAME) Connects_Info[i].NeedData ^= FLAG_NEXT_AUDIO_FRAME;
			}
		}
		DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);		
	}
	
	DBG_LOG_OUT();	
	return ret;
}

int RecvAudioFrame(void *AVBuffer, unsigned int *iNumFrame, int *Flag, void* pData, char cStream, unsigned int ConnectNum, unsigned int ConnectID)
{		
	DBG_LOG_IN();
	
	AudioCodecInfo *CodecInfo = (AudioCodecInfo*)pData;
	AVPacket *avpkt = AVBuffer;
	int BuffSize = avpkt->size;
	
	int result = 0;
	int ret;
	struct sockaddr_in Address;
	struct sockaddr_in ConnAddress;
	int timeout = RECV_FRAME_TIMEOUT_MS;
	int iRecvLen = 0;
	char *PacketBody;
	unsigned int uiRecvPacketSize = 0;
	unsigned int uiNeedData;
	TRANSFER_DATA *PacketHeader;
	avpkt->size = 0;
	AUDIO_FRAME_INFO AudFrame;
	AUDIO_SUBFRAME_INFO *pAudSubFrame;	
	memset(&AudFrame,0,sizeof(AUDIO_FRAME_INFO));
	*Flag = 0;
	
	ret = 0;
	DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);		
	if ((Connects_Info[ConnectNum].Status == CONNECT_STATUS_ONLINE) 
		&& (Connects_Info[ConnectNum].Type == CONNECT_CLIENT)
		&& (Connects_Info[ConnectNum].ID == ConnectID)) 
		{
			ret = 1;
			uiNeedData = Connects_Info[ConnectNum].NeedData;	
		}
	DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);		
	
	if (ret == 0) 
	{
		dbgprintf(2, "No connects for Recv audio frame\n");
		DBG_LOG_OUT();	
		return -1;	
	}
	if (CodecInfo->CodecInfoFilled == 0)
	{
		if (uiNeedData & FLAG_AUDIO_CODEC_INFO)
				SendMessage(ConnectNum, TYPE_MESSAGE_REQUEST_AUDIO_CODEC_INFO, NULL, 0, NULL, 0, &Connects_Info[ConnectNum].Addr);
		pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);	
		SendSignalType(ConnectNum, SIGNAL_WORK);		
		timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, EVENT_AUDIO_CODEC_INFO_DATA, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
		if (timeout != 0)
		{
			PacketHeader = (TRANSFER_DATA*)PacketBody;
			PacketBody += sizeof(TRANSFER_DATA);	
			if (sizeof(AudioCodecInfo) == (PacketHeader->SizeMessage - sizeof(TRANSFER_DATA) - sizeof(int)))
			{
				memcpy(CodecInfo, PacketBody, sizeof(AudioCodecInfo));					
			}
			else
			{
				dbgprintf(2,"Recv wrong AudioCodecInfo data or size\n");
				DBG_LOG_OUT();		
				return 0;
			}
			if (Connects_Info[ConnectNum].NeedData & FLAG_AUDIO_CODEC_INFO) Connects_Info[ConnectNum].NeedData ^= FLAG_AUDIO_CODEC_INFO;								
			if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
				dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
			CodecInfo->CodecInfoFilled = 1;
			*Flag |= FLAG_AUDIO_CODEC_INFO;
			//printf("audio Codec info(%i): len:%i, filled:%i, CRC:%i\n",Connects_Info[ConnectNum].NeedData,Start_Packet->CodecInfoLen,Start_Packet->CodecInfoFilled, CalcCRC(Start_Packet->BufferCodecInfo, Start_Packet->CodecInfoLen));
			DBG_LOG_OUT();		
			return 1;
		} 
		else 
		{
			DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);
			dbgprintf(2,"timeout audio codec info(%i) NeedData:%i\n",ConnectNum,Connects_Info[ConnectNum].NeedData);
			DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);
			DBG_LOG_OUT();	
			return 0;
		}
	}	
	if (CodecInfo->CodecInfoFilled == 0) 
	{
		DBG_LOG_OUT();	
		return 2;
	}
	//*Flag = OMX_BUFFERFLAG_TIMESTAMPINVALID;
	
	int iLoop = 1;
	ret = 0;
	//int iTime = 0;
	//iTime = timeout;
	
	while ((iLoop) && (timeout > 0))
	{
		if (ret == 0)
		{
			DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);
			if (cStream == 0)
					SendMessage(ConnectNum, TYPE_MESSAGE_REQUEST_NEXT_AUDIO_FRAME, NULL, 0, NULL, 0, &Connects_Info[ConnectNum].Addr);				
			memcpy(&ConnAddress, &Connects_Info[ConnectNum].Addr, sizeof(ConnAddress));			
			DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
			
			pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);	
			SendSignalType(ConnectNum, SIGNAL_WORK);
			
			//printf("wait: %i\n",EVENT_NEXT_AUDIO_FRAME_DATA);
			//iTime = timeout;
			timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, EVENT_NEXT_AUDIO_FRAME_DATA, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
			if (timeout != 0)
			{				
				PacketHeader = (TRANSFER_DATA*)PacketBody;
				PacketBody += sizeof(TRANSFER_DATA);	
				memcpy(&Address, &PacketHeader->Address, sizeof(Address));
				memcpy(&AudFrame, PacketBody, sizeof(AUDIO_FRAME_INFO));
				if (ConnAddress.sin_addr.s_addr == Address.sin_addr.s_addr)
				{
					if ((*iNumFrame <= AudFrame.Number) || (AudFrame.Number == START_AUDIO_FRAME_NUMBER) || 
							((*iNumFrame == END_AUDIO_FRAME_NUMBER) && (AudFrame.Number == (START_AUDIO_FRAME_NUMBER + 1))))
					{
						if (timeout < RECV_FRAME_TIMEOUT_MS) timeout = RECV_FRAME_TIMEOUT_MS;
						*iNumFrame = AudFrame.Number;
						avpkt->size = AudFrame.LenData;
						iRecvLen = 0;
						if (AudFrame.LenData > BuffSize) 
						{
							iLoop = 0;
							result = 0;
							dbgprintf(2,"Very big audio packet\n");
						}	
						else
						{	
							//printf("NewFrameStartLoad\n");
							if (Connects_Info[ConnectNum].NeedData & FLAG_NEXT_AUDIO_FRAME) Connects_Info[ConnectNum].NeedData ^= FLAG_NEXT_AUDIO_FRAME;							
							memset(avpkt->data, 0, AudFrame.LenData);
							Connects_Info[ConnectNum].NeedData |= FLAG_SUB_AUDIO_FRAME;
							ret = 1;									
						}
					} else dbgprintf(2,"Wrong num audio frame: iNumFrame:%i, AudFrame.Number:%i\n",*iNumFrame, AudFrame.Number);
				} else dbgprintf(2,"Alien audio data\n");
				if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
					dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
			} 
			else 
			{
				DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);					
				dbgprintf(2,"timeout audio frame(%i) NeedData:%i\n",ConnectNum,Connects_Info[ConnectNum].NeedData);
				DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
			}
		}
		if (ret > 0)
		{
			//if (tx_semaphore_exist_in_list(Connects_Info[ConnectNum].pevntIP, EVENT_SUB_AUDIO_FRAME_DATA, TX_ANY) == 0)
			
			pthread_mutex_lock(&Connects_Info[ConnectNum].pevntIP->mutex);	
			SendSignalType(ConnectNum, SIGNAL_WORK);			
			//iTime = timeout;
			timeout = tx_eventer_recv_data_prelocked(Connects_Info[ConnectNum].pevntIP, EVENT_SUB_AUDIO_FRAME_DATA, (void**)&PacketBody, &uiRecvPacketSize, RECV_FRAME_TIMEOUT_MS);
			if (timeout != 0)
			{
				//printf("Time=%i\n",iTime-timeout);				
				PacketHeader = (TRANSFER_DATA*)PacketBody;
				PacketBody += sizeof(TRANSFER_DATA);	
				pAudSubFrame = (AUDIO_SUBFRAME_INFO*)PacketBody;
				if ((pAudSubFrame->Number == AudFrame.Number)
					&& (pAudSubFrame->LenData < (TCP_PACKET_SIZE*2)) 
					&& ((pAudSubFrame->Position + pAudSubFrame->LenData) <= BuffSize))
				{
					if  (((pAudSubFrame->LenData > 0) && (avpkt->data[pAudSubFrame->Position] != 0)) ||
						((pAudSubFrame->LenData > 1) && (avpkt->data[pAudSubFrame->Position+1] != 0)) ||
						((pAudSubFrame->LenData > 2) && (avpkt->data[pAudSubFrame->Position+2] != 0)) ||
						((pAudSubFrame->LenData > 3) && (avpkt->data[pAudSubFrame->Position+3] != 0)))
					{
						dbgprintf(2,"Repeat audio sub packet num:%i\n",AudFrame.Number);
					}
					else
					{
						//printf("AUDIO_SUBFRAME_INFO:%i, %i, %i, %i\n",pAudSubFrame->Position, BuffSize, pAudSubFrame->LenData, sizeof(Buffer));					
						memcpy(&avpkt->data[pAudSubFrame->Position], &PacketBody[sizeof(AUDIO_SUBFRAME_INFO)], pAudSubFrame->LenData);
						iRecvLen += pAudSubFrame->LenData;
						//printf("audio num:%i, NumSubFrame:%i,LenSubFrame:%i\n", pAudSubFrame->Number, pAudSubFrame->SubNumber, pAudSubFrame->LenData);
						//printf("Fill:%i\n", iRecvLen);	
					}
				} else dbgprintf(2,"Wrong packet PN:%i, PSN:%i, PSL:%i, PSP:%i\n", AudFrame.Number,pAudSubFrame->Number,pAudSubFrame->LenData,pAudSubFrame->Position);
				if (iRecvLen == AudFrame.LenData)
				{
					if (AudFrame.CRC == CalcCRC((char*)avpkt->data, AudFrame.LenData))
					{
						//printf("DONE audio packet\n");
						iLoop = 0;
						result = 1;
						if ((Connects_Info[ConnectNum].NeedData & FLAG_SUB_AUDIO_FRAME) != 0) Connects_Info[ConnectNum].NeedData ^= FLAG_SUB_AUDIO_FRAME;
						*Flag |= AudFrame.Flag;
					}
					else
					{
						dbgprintf(2,"BAD CRC audio packet\n");
						iLoop = 0;
						result = 0;
					}
				} 
				else 
				{
					//printf("Loaded audio sub packet num:%i, subnum:%i, subsubnum: %i, %i/%i\n",AudFrame.Number, pVidSubFrame->Number, pVidSubFrame->SubNumber,iRecvLen, AudFrame.LenData);
				}
				if (!tx_eventer_return_data(Connects_Info[ConnectNum].pevntIP, RECV_FRAME_TIMEOUT_MS))
					dbgprintf(2, "Timeout return data in %s, line : %i\n",__FILE__, __LINE__);
				//printf(">>>>:audio Numframe:%i, SubNum:%i, DataLen:%i, FrameLen:%i, timeout:%i, recieved:%i\n",pVidSubFrame->Number,pVidSubFrame->SubNumber,pVidSubFrame->LenData,AudFrame.LenData, timeout,iRecvLen);	
			}
			else
			{
				DBG_MUTEX_LOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
				if ((Connects_Info[ConnectNum].NeedData & FLAG_SUB_AUDIO_FRAME) != 0) Connects_Info[ConnectNum].NeedData ^= FLAG_SUB_AUDIO_FRAME;
				dbgprintf(2,"timeout audio subframe(%i)flag:%i, loaded %i from %i\n",ConnectNum,Connects_Info[ConnectNum].NeedData, iRecvLen , AudFrame.LenData);
				DBG_MUTEX_UNLOCK(&Connects_Info[ConnectNum].Socket_Mutex);	
				//tx_semaphore_go(Connects_Info[ConnectNum].pevntIP, EVENT_SYNC_SUB_AUDIO_FRAME_DATA, TX_ANY);					
				//usleep(1000);
				//timeout--;					
			}				
		}
	}
	//printf("Time=%i\n",iTime-timeout);		
	/*DBG_MUTEX_LOCK(&conn_info_mutex);	
	dbgprintf(3,"0:audio Buffersize:%i,Num:%i, Len:%i, Load:%i, Flag:%i, ret:%i, time:%i\n",BuffSize,*iNumFrame,avpkt->size, iRecvLen,*Flag,result, timeout);		
	DBG_MUTEX_UNLOCK(&conn_info_mutex);	*/
	if (timeout == 0) result = 0;
		
	DBG_LOG_OUT();	
	return result;
}

int ItsNetNeed(int iType, int iTraffType, int iFlags, char cCalc)
{
	DBG_LOG_IN();
	
	int ret = 0;
	
	DBG_MUTEX_LOCK(&Network_Mutex);
	int iConnMax = Connects_Max_Active;
	DBG_MUTEX_UNLOCK(&Network_Mutex);
	
	int i;
	for (i = 1; i < iConnMax; i++) 
	{
		DBG_MUTEX_LOCK(&Connects_Info[i].Socket_Mutex);
		if ((Connects_Info[i].Status == CONNECT_STATUS_ONLINE) 
			&& (Connects_Info[i].Type == iType)
			&& ((Connects_Info[i].NeedData & iFlags) != 0)
			&& (Connects_Info[i].TraffType == iTraffType))
			{
				ret++; 				
			}
		DBG_MUTEX_UNLOCK(&Connects_Info[i].Socket_Mutex);
		if ((cCalc == 0) && ret) break;
	}		
	
	DBG_LOG_OUT();	
	return ret;
}

struct stMiscData 
{
	char *DataBody;
	unsigned int DataLen;
	unsigned int BufferSize;
};

int curl_writer(char *data, size_t size, size_t nmemb, void *pData)
{
	int result = size * nmemb;
	struct stMiscData *UserData = (struct stMiscData*)pData;
//	ListPrint("%i", result);
	if ((UserData->DataLen + result) > UserData->BufferSize)
	{
		if (UserData->BufferSize < 500000000)
		{
			UserData->BufferSize += 5000000;
			UserData->DataBody = (char*)realloc(UserData->DataBody, UserData->BufferSize);
			dbgprintf(4, "Downloaded %i", UserData->DataLen + result);
		} else result = 0;
	}
	if (result)	
	{
		memcpy(&UserData->DataBody[UserData->DataLen], data, result);
		UserData->DataLen += result;
	}

	return result;
}

int DownloadAddress(char *cPath, char **pBuffer, unsigned int *uiLen)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct stMiscData UserData;

	UserData.DataBody = NULL;
	UserData.DataLen = 0;
	UserData.BufferSize = 0;

	*uiLen = 0;
	*pBuffer = NULL;
		
	curl = curl_easy_init();
	if(curl) 
	{
		curl_easy_setopt(curl, CURLOPT_URL, cPath);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writer);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &UserData);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
		
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)	
		{
			dbgprintf(4, "curl_easy_perform() failed: %s\n",	curl_easy_strerror(res));
			//MessageBox(0, curl_easy_strerror(res), "curl_easy_perform() failed:", MB_ICONERROR);
		}
		else
		{
			*uiLen = UserData.DataLen;
			*pBuffer = UserData.DataBody;
		}	
		
		curl_easy_cleanup(curl);
	} 
	else 
	{
		dbgprintf(4, "curl_easy_init failed: %s\n",	curl_easy_strerror(res));
		//MessageBox(0, "curl_easy_init error", curl_easy_strerror(res), MB_ICONERROR);
	}	
	
	return 1;
}

int curl_writer_file(char *data, size_t size, size_t nmemb, void *pData)
{
	int result = size * nmemb;
	
	if (fwrite(data, result, 1, (FILE*)pData) != 1)
		dbgprintf(4, "error write to file");
//	ListPrint("%i %i", fwrite(data, result, 1, (FILE*)pData), result);
//	ListPrint("Downloaded %i", ftell((FILE*)pData));
	return result;
}

int DownloadFile(char *cPath, char *cSavePath)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	
	FILE *f;
	if ((f = fopen(cSavePath,"wb+")) == NULL)
	{
		dbgprintf(4, "Error create file: %s", cSavePath);
		return 0;
	}
	
	curl = curl_easy_init();
	if(curl) 
	{
		//curl_easy_setopt(curl, CURLOPT_USERNAME, cLogin);
		//curl_easy_setopt(curl, CURLOPT_PASSWORD, cPassword);		
		curl_easy_setopt(curl, CURLOPT_URL, cPath);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writer_file);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
		
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)	
		{
			dbgprintf(4, "curl_easy_perform() failed: %s\n",	curl_easy_strerror(res));
			//MessageBox(0, curl_easy_strerror(res), "curl_easy_perform() failed:", MB_ICONERROR);
		}		
		curl_easy_cleanup(curl);
	} 
	else 
	{
		dbgprintf(4, "curl_easy_init failed: %s\n",	curl_easy_strerror(res));
		//MessageBox(0, "curl_easy_init error", curl_easy_strerror(res), MB_ICONERROR);
	}
	
	fclose(f);
	return 1;
}

struct upload_status 
{
	int attach_index;
	int attach_pos;
	int attach2_index;
	int attach2_pos;
	int lines_count;	
	int lines_read;	
	char *cMailBody[64];
	char *attach_body;
	unsigned int attach_len;
	char *attach2_body;
	unsigned int attach2_len;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	char *data;
	
	if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) 
	{
		return 0;
	}
	
	if ((upload_ctx->lines_read == upload_ctx->attach_index) ||
		(upload_ctx->lines_read == upload_ctx->attach2_index))
	{
		if (upload_ctx->lines_read == upload_ctx->attach_index)
		{
			if ((upload_ctx->attach_len - upload_ctx->attach_pos) > 54)
			{
				base64_encode((unsigned char*)&upload_ctx->attach_body[upload_ctx->attach_pos], 54, (unsigned char*)upload_ctx->cMailBody[upload_ctx->attach_index], 128);
				data = upload_ctx->cMailBody[upload_ctx->lines_read];
				upload_ctx->attach_pos += 54;
			}
			else
			{			
				base64_encode((unsigned char*)&upload_ctx->attach_body[upload_ctx->attach_pos], upload_ctx->attach_len - upload_ctx->attach_pos, (unsigned char*)upload_ctx->cMailBody[upload_ctx->attach_index], 128);
				data = upload_ctx->cMailBody[upload_ctx->lines_read];
				upload_ctx->attach_pos += upload_ctx->attach_len - upload_ctx->attach_pos;
				upload_ctx->lines_read++;
			}
		}
		else
		{
			if ((upload_ctx->attach2_len - upload_ctx->attach2_pos) > 54)
			{
				base64_encode((unsigned char*)&upload_ctx->attach2_body[upload_ctx->attach2_pos], 54, (unsigned char*)upload_ctx->cMailBody[upload_ctx->attach2_index], 128);
				data = upload_ctx->cMailBody[upload_ctx->lines_read];
				upload_ctx->attach2_pos += 54;
			}
			else
			{			
				base64_encode((unsigned char*)&upload_ctx->attach2_body[upload_ctx->attach2_pos], upload_ctx->attach2_len - upload_ctx->attach2_pos, (unsigned char*)upload_ctx->cMailBody[upload_ctx->attach2_index], 128);
				data = upload_ctx->cMailBody[upload_ctx->lines_read];
				upload_ctx->attach2_pos += upload_ctx->attach2_len - upload_ctx->attach2_pos;
				upload_ctx->lines_read++;
			}
		}
		strcat(data, "\r\n");			
	}
	else
	{	
		data = upload_ctx->cMailBody[upload_ctx->lines_read];
		upload_ctx->lines_read++;
	}
	//printf("payload: %i '%s'\n", upload_ctx->lines_read-1, upload_ctx->cMailBody[upload_ctx->lines_read-1]);
	
	if (upload_ctx->lines_read < upload_ctx->lines_count) 
	{
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		//printf("%s\n", data);
		return len;
	}
	return 0;
}

int AnsiToKoi(unsigned char *cSourceStr, unsigned int uiSourceLen)
{
	char Koi[] = "°¢Áß§•ÊÍ©™´¨≠ÆØ‡‚„‰Â¶®£ÓÎÌÔÈËÏ†·ÅÇóáÑÖñöâäãåçéèêíìîïÜàÉûõùüôòúÄë-J";
	
	unsigned int n;
	for(n = 0; n < uiSourceLen; n++)
	{
		/*if (cSourceStr[n] == 184) cSourceStr[n] = 163;
		else
			if (cSourceStr[n] == 168) cSourceStr[n] = 179;
			else*/
				if (cSourceStr[n] > 191) cSourceStr[n] = Koi[cSourceStr[n] - 192];
	}
	return 1;
}

int KoiToAnsi(unsigned char *cSourceStr, unsigned int uiSourceLen)
{
	char Win[] = "Ó†°Ê§•‰£Â®©™´¨≠ÆØÔ‡·‚„¶¢ÏÎßËÌÈÁÍûÄÅñÑÖîÉïàâäãåçéèüêëíìÜÇúõáòùôóö";
	
	unsigned int n;
	for(n = 0; n < uiSourceLen; n++)
	{
		if (cSourceStr[n] == 163) cSourceStr[n] = 184;
		else
			if (cSourceStr[n] == 179) cSourceStr[n] = 168;
			else
				if (cSourceStr[n] > 191) cSourceStr[n] = Win[cSourceStr[n] - 192];
	}
	return 1;
}

int SendMailFile(char *cLogin, char *cPassword, char *cServer, char *cFromAddress, char *cAuth, MAIL_INFO* cMlList)
{
	DBG_LOG_IN();
	dbgprintf(4, "SendMailFile: ExtraTextSize %i\n", cMlList->ExtraTextSize);
	
	int n, m;
	char *cNameFile = NULL;
	char *cNameFile2 = NULL;
	omx_buffer ibuffer;
	omx_buffer ibuffer2;
	memset(&ibuffer, 0, sizeof(omx_buffer));
	memset(&ibuffer2, 0, sizeof(omx_buffer));
	
	if ((cMlList->FilePath) && (strlen(cMlList->FilePath)) && (omx_load_file(cMlList->FilePath, &ibuffer) == 0))
	{
		dbgprintf(2, "SendMailFile: error load file %s\n", cMlList->FilePath);
		DBG_LOG_OUT();
		return 0;	
	}
	
	if (ibuffer.data)
	{
		m = strlen(cMlList->FilePath);
		cNameFile = &cMlList->FilePath[m-1];
		for(n = m - 1; n >= 0; n--) 
		{
			if ((cMlList->FilePath[n] == 47) || (cMlList->FilePath[n] == 92)) 
			{
				if ((n + 1) != m) cNameFile = &cMlList->FilePath[n+1];
				break;
			}		
		}
	}

	if ((cMlList->FilePath2) && (strlen(cMlList->FilePath2)) && (omx_load_file(cMlList->FilePath2, &ibuffer2) == 0))
	{
		DBG_FREE(ibuffer.data);
		dbgprintf(2, "SendMailFile: error load file %s\n", cMlList->FilePath2);
		DBG_LOG_OUT();
		return 0;	
	}
	
	if (ibuffer2.data)
	{
		m = strlen(cMlList->FilePath2);
		cNameFile2 = &cMlList->FilePath2[m-1];
		for(n = m - 1; n >= 0; n--) 
			if ((cMlList->FilePath2[n] == 47) || (cMlList->FilePath2[n] == 92)) 
			{
				if ((n + 1) != m) cNameFile2 = &cMlList->FilePath2[n+1];
				break;
			}		
	}
	
	int ret = 0;
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx;
	time_t rawtime;
	time(&rawtime);	
	struct tm timeinfo;
	localtime_r(&rawtime, &timeinfo);

	upload_ctx.lines_count = 0;
	upload_ctx.lines_read = 0;
	upload_ctx.attach_index = -1;
	upload_ctx.attach_pos = 0;
	upload_ctx.attach_body = ibuffer.data;
	upload_ctx.attach_len = ibuffer.data_size;
	upload_ctx.attach2_index = -1;
	upload_ctx.attach2_pos = 0;
	upload_ctx.attach2_body = ibuffer2.data;
	upload_ctx.attach2_len = ibuffer2.data_size;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	strftime(upload_ctx.cMailBody[upload_ctx.lines_count], 128, "Date: %Y-%m-%d %H:%M:%S\r\n", &timeinfo);
	upload_ctx.lines_count++;
	unsigned int uid = (unsigned int)(upload_ctx.cMailBody[0]) & 0xEFFFFFFF;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	if ((cMlList->Address2) && (strlen(cMlList->Address2))) 
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "To: <%s>; <%s>\r\n", cMlList->Address, cMlList->Address2);
		else
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "To: <%s>\r\n", cMlList->Address);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "From: <%s>\r\n", cFromAddress);
	upload_ctx.lines_count++;
	
	/*upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	if (strlen(cCCAddress)) sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "CC: <%s>\r\n", cCCAddress);
		else sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "CC: <%s>\r\n", cFromAddress);
	upload_ctx.lines_count++;	*/
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Subject: %s\r\n", cMlList->MainText);	
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "MIME-Version: 1.0\r\n");
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Type: multipart/mixed; boundary=\"%i\"\r\n\r\n", uid);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "This is a multi-part message in MIME format.\r\n");
	upload_ctx.lines_count++;
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i\r\n", uid);	
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Type: text/plain; charset=cp866\"\r\n"); 
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-transfer-encoding:base64\r\n\r\n");
	upload_ctx.lines_count++;
	
	if (cMlList->ExtraTextSize == 0)
	{
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		base64_encode((unsigned char*)cMlList->BodyText, strlen(cMlList->BodyText), (unsigned char*)upload_ctx.cMailBody[upload_ctx.lines_count], 128);			
		strcat(upload_ctx.cMailBody[upload_ctx.lines_count], "\r\n\r\n");	
	}
	else
	{
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(cMlList->ExtraTextSize*2);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, cMlList->ExtraTextSize*2);	
		base64_encode((unsigned char*)cMlList->ExtraText, cMlList->ExtraTextSize, (unsigned char*)upload_ctx.cMailBody[upload_ctx.lines_count], cMlList->ExtraTextSize*2);			
		strcat(upload_ctx.cMailBody[upload_ctx.lines_count], "\r\n");	
	}
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	if ((ibuffer.data) || (ibuffer2.data)) sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i\r\n", uid);
		else sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i--\r\n", uid);
	upload_ctx.lines_count++;
	
	
	if (ibuffer.data)
	{
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Type: application/octet-stream; name=\"%s\"\r\n", cNameFile); 
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-transfer-encoding:base64\r\n");
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Disposition: attachment; filename=\"%s\"\r\n\r\n", cNameFile);
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		upload_ctx.attach_index = upload_ctx.lines_count;
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "\r\n\r\n");
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		if (ibuffer2.data) sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i\r\n", uid);
							else sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i--\r\n", uid);
		upload_ctx.lines_count++;
	}
	
	if (ibuffer2.data)
	{
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Type: application/octet-stream; name=\"%s\"\r\n", cNameFile2); 
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-transfer-encoding:base64\r\n");
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Disposition: attachment; filename=\"%s\"\r\n\r\n", cNameFile2);
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		if (upload_ctx.attach_index == -1) upload_ctx.attach_index = upload_ctx.lines_count;
			else upload_ctx.attach2_index = upload_ctx.lines_count;		
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "\r\n\r\n");
		upload_ctx.lines_count++;
		
		upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)DBG_MALLOC(128);
		memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
		sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i--", uid);	
		upload_ctx.lines_count++;
	}
	
	curl = curl_easy_init();
	if(curl) 
	{
		curl_easy_setopt(curl, CURLOPT_USERNAME, cLogin);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, cPassword);
		
		char cBuff[128];	
		memset(cBuff, 0, 128);
		sprintf(cBuff, "%s", cServer);
		curl_easy_setopt(curl, CURLOPT_URL, cBuff);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
		
		//curl_easy_setopt(curl, CURLOPT_RETURNTRANSFER, 1);
		//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		//curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 200L);	
		//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
		//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 360L);
		//curl_easy_setopt(curl, CURLOPT_IGNORE_CONTENT_LENGTH, 1L);
		//curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);		
		
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		
		memset(cBuff, 0, 128);
		sprintf(cBuff, "<%s>", cFromAddress);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, cBuff);
		
		memset(cBuff, 0, 128);
		sprintf(cBuff, "<%s>", cMlList->Address);
		recipients = curl_slist_append(recipients, cBuff);
		if ((cMlList->Address2) && (strlen(cMlList->Address2))) 
		{
			memset(cBuff, 0, 128);
			sprintf(cBuff, "<%s>", cMlList->Address2);
			recipients = curl_slist_append(recipients, cBuff);
		}
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);		
		
		//curl_easy_setopt(curl, CURLOPT_INFILESIZE, datalen); 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		
		if ((cAuth) && (strlen(cAuth)))
		{
			memset(cBuff, 0, 128);
			sprintf(cBuff, "AUTH=%s", cAuth);		
			curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, cBuff);
		}
		
		/* Send the message */
		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK)
			dbgprintf(1, "SendMail: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			else ret = 1;	
		
		curl_slist_free_all(recipients);
		
		curl_easy_cleanup(curl);
	}
	
	for (n = 0; n < upload_ctx.lines_count; n++) DBG_FREE(upload_ctx.cMailBody[n]);
	
	if (ibuffer.data) DBG_FREE(ibuffer.data);
	if (ibuffer2.data) DBG_FREE(ibuffer2.data);
	
	DBG_LOG_OUT();
	return ret;
}

