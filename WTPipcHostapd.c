/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	       *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                *
 *                                                                                         *
 * You should have received a copy of the GNU General Public License along with this       *
 * program; if not, write to the:                                                          *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                    *
 * MA  02111-1307, USA.                                                                    *
 *                                                                                         *
 * --------------------------------------------------------------------------------------- *
 * Project:  Capwap                                                                        *
 *                                                                                         *
 * Author :  Sotiraq Sima (Sotiraq.Sima@gmail.com)                                         *  
 *                                                                                         *
 *******************************************************************************************/
#include <netinet/in.h>

#include "WTPipcHostapd.h"


#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define EXIT_FRAME_THREAD(sock)	CWLog("ERROR Handling Frames: application will be closed!"); close(sock); exit(1);


//#define LOCALUDP
//#define NETUDP
//#define NETSEQ

//#define USEIPV6


int address_size;

#if defined(LOCALUDP)
	struct sockaddr_un client;
#else
	#if defined(USEIPV6)
		struct sockaddr_in6 client;
	#else
		struct sockaddr_in client;
	#endif

#endif



char connected = 0;
int sock;
extern int wtpInRunState;

pthread_mutex_t gRADIO_MAC_mutex;
pthread_mutex_t mutext_info;

unsigned char WTP_Radio_Information = 0;
unsigned char WTP_Rates[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char WTP_MDC[6] = { 0, 0, 0, 0, 0, 0 };

char gRADIO_MAC[6];

int flush_pcap(u_char *buf,int len,char *filename){return;
	
	FILE *file;
	file = fopen(filename,"a+");
	u_char index=0x00;
	int cnt=0;
	int i;
	int giro=0;
	for(i=0;cnt<len ;i++){
		fprintf(file,"0%02X0   ",index);
		for(;cnt<len;){
			fprintf(file,"%02X ",buf[cnt]);
			cnt++;
			if(giro==15){
				giro=0;
				break;
			}
			giro++;
		}
		fprintf(file,"\n");
		index++;
	}

	fprintf(file,"\n");
	fclose(file); 
	return 0;
}


void CWWTP_get_WTP_Rates(unsigned char *buf){
	
	CWThreadMutexLock(&mutext_info);
		memcpy( buf, WTP_Rates, 8 );
	CWThreadMutexUnlock(&mutext_info);
	
}


void CWWTP_get_WTP_MDC(unsigned char *buf){
	
	CWThreadMutexLock(&mutext_info);
		memcpy( buf, WTP_MDC, 6 );
	CWThreadMutexUnlock(&mutext_info);	
	
}

unsigned char CWTP_get_WTP_Radio_Information(){
	
	unsigned char tmp_info;
	CWThreadMutexLock(&mutext_info);
		tmp_info = WTP_Radio_Information;
	CWThreadMutexUnlock(&mutext_info);	
	return tmp_info;
	
}

void CWWTPsend_data_to_hostapd(unsigned char *buf, int len){  
	
	if(!connected)return;
	
	unsigned char tmp_buf[CW_BUFFER_SIZE];
	tmp_buf[0] = DATE_TO_WTP;
	memcpy(tmp_buf + 1, buf, len);
	
	if( sendto(sock,tmp_buf,len+1,0,(struct sockaddr *)&client,address_size)<0 ){
		CWDebugLog("Error to send data frame on Unix socket");
		return;
	}
	
}

void CWWTPsend_command_to_hostapd_SET_TXQ(unsigned char *buf, int len){
	
	if(!connected)return;
	buf[0] = SET_TXQ;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&client, address_size)<0 ){
		CWDebugLog("Error to send command frame on Unix socket");
		return;
	}
	
}

void CWWTPsend_command_to_hostapd_SET_ADDR(unsigned char *buf, int len){ 

	if(!connected)return;
	buf[0] = SET_ADDR;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&client, address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}

void CWWTPsend_command_to_hostapd_ADD_WLAN(unsigned char *buf, int len){ 

WAITHOSTAPDADD:

	if(!connected){ 
		sleep(0.2);
		goto WAITHOSTAPDADD; 
	}
	buf[0] = ADD_WLAN;
	int i;

	if( sendto(sock, buf, len, 0, (struct sockaddr *)&client, address_size)<0 ){
		CWDebugLog("Error to send command ADD WLAN on socket");
		return;
	}
	
}

void CWWTPsend_command_to_hostapd_DEL_WLAN(unsigned char *buf, int len){ 
	
WAITHOSTAPDDEL:

	if(!connected){ 
		sleep(0.2);
		goto WAITHOSTAPDDEL; 
	}
	buf[0] = DEL_WLAN;
	int i;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&client, address_size)<0 ){
		CWLog("Error to send command DEL WLAN on socket");
		return;
	}
	
}

void CWWTPsend_command_to_hostapd_DEL_ADDR(unsigned char *buf, int len){ 

	if(!connected)return;
	buf[0] = DEL_ADDR;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&client, address_size)<0 ){
		CWLog("Error to send command frame on socket");
		return;
	}
	
}

void CWWTPsend_command_to_hostapd_CLOSE(unsigned char *buf, int len){ 

	buf[0] = CLOSE;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&client, address_size)<0 ){
		CWLog("Error to send command frame on socket");
		return;
	}
	
}

CW_THREAD_RETURN_TYPE CWWTPThread_read_data_from_hostapd(void *arg){
	
	/*
	CWThreadMutexLock(&gRADIO_MAC_mutex);
		gRADIO_MAC[0]=0xAA;
		gRADIO_MAC[1]=0xBB;
		gRADIO_MAC[2]=0xCC;
		gRADIO_MAC[3]=0xDD;
		gRADIO_MAC[4]=0xEE;
		gRADIO_MAC[5]=0xFF;
	CWThreadMutexUnlock(&gRADIO_MAC_mutex);
	*/
	
	int len;
        
    #if defined(LOCALUDP)
		struct sockaddr_un server;
    #else
        #if defined(USEIPV6)
			struct sockaddr_in6 server;
		#else
			struct sockaddr_in server;
		#endif
    #endif
    
	unsigned char buffer[CW_BUFFER_SIZE];
	int connect_ret;
	int flags;
	int n;
	char cmd[10];
	
	CWProtocolMessage* frame = NULL;
	CWBindingDataListElement* listElement = NULL;
	
	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);

	#if defined(LOCALUDP)
		sock = socket(AF_UNIX, SOCK_DGRAM, 0);

	#elif defined(NETUDP)
		#if defined(USEIPV6)
			bzero(&server,sizeof(server));
			sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		#else
			memset(&server, 0, sizeof(server));
			sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		#endif

	#else
		#if defined(USEIPV6)
			bzero(&server,sizeof(server));
			sock = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
		#else
			memset(&server, 0, sizeof(server));
			sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
		#endif
		
	#endif

    if (sock < 0) {
		CWDebugLog("WTP ipc HOSTAPD: Error creating socket");
		EXIT_FRAME_THREAD(sock);
    }

    CWDebugLog("WTP ipc HOSTAPD: Trying to connect to hostapd (wtp)...");

	#if defined(LOCALUDP)
		server.sun_family = AF_UNIX;
		strcpy(server.sun_path, gHostapd_unix_path);
		unlink(server.sun_path);
		
		connect_ret = bind(sock, (struct sockaddr *)&server, strlen(server.sun_path) + sizeof(server.sun_family));
		
		client.sun_family=AF_UNIX;

    #else
		#if defined(USEIPV6)
			server.sin6_family = AF_INET6;
			server.sin6_port = gHostapd_port;
			server.sin6_addr = in6addr_any;
		#else
			server.sin_family = AF_INET;
			server.sin_port = gHostapd_port;
			server.sin_addr.s_addr = INADDR_ANY;
		#endif
		connect_ret = bind(sock,(struct sockaddr *)&server,sizeof(server));
		
    #endif
 
    
	if (connect_ret == -1) {
		CWDebugLog("WTP ipc HOSTAPD: Error connect/bind to socket");
		EXIT_FRAME_THREAD(sock);
	}

	#if defined(LOCALUDP)

	#elif defined(NETUDP)
	
	#else
		/* 1: Only one daemon Hostapd_WTP at time */
		if (listen(sock, 1) < 0){
			CWDebugLog("WTP ipc HOSTAPD: Error listen ");
			EXIT_FRAME_THREAD(sock);
		}
	#endif
	
	
	#if defined(LOCALUDP)
		CWDebugLog("Waiting packet from Hostapd_WTP at Pipe:%s",gHostapd_unix_path);
	#else
		CWDebugLog("Waiting packet from Hostapd_WTP at Port:%d",gHostapd_port);
	#endif

	
	address_size = sizeof(client);
	
	int sig_byte = 1;
	
	#if defined(LOCALUDP)
		sig_byte += 5;
	#endif
	
 	CW_REPEAT_FOREVER {
		
		len = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&address_size); 
		
		#if defined(LOCALUDP)
			sprintf(client.sun_path, "%s%c%c%c%c%c", server.sun_path, buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
		#endif	
		

		if(len <= 0 ) {	EXIT_FRAME_THREAD(sock)	}
		
		if(connected==0 && buffer[0]!=CONNECT ){	
			CWLog("WTP is not in RUN state"); 	
			CWWTPsend_command_to_hostapd_CLOSE(cmd,10);
			continue;	
		}
		
		if( buffer[0] == DATE_TO_AC ){

			if(!wtpInRunState) continue;
			
			if (!extract802_11_Frame(&frame, buffer+sig_byte, len-sig_byte)){
				CWLog("THR FRAME: Error extracting a frame");
				EXIT_FRAME_THREAD(sock);
			}
				
			CWDebugLog("Send 802.11 management(len:%d) to AC",len-1);

			CW_CREATE_OBJECT_ERR(listElement, CWBindingDataListElement, EXIT_FRAME_THREAD(sock););
			listElement->frame = frame;
			listElement->bindingValues = NULL;
				
			listElement->frame->data_msgType = CW_IEEE_802_11_FRAME_TYPE;

			CWLockSafeList(gFrameList);
			CWAddElementToSafeListTail(gFrameList, listElement, sizeof(CWBindingDataListElement));
			CWUnlockSafeList(gFrameList);

			
		}else if( buffer[0]==CONNECT ){
			
			connected = 1;
			cmd[0] = CONNECT_R;
			sendto(sock, cmd, 1, 0, (struct sockaddr *)&client, address_size);
			
			#if defined(LOCALUDP)
				CWDebugLog("Hostapd_wtp Unix Domain Connect: %s",client.sun_path);
			#else
				#if defined(USEIPV6)
					CWDebugLog("Hostapd_wtp (v6) Connect: %d",client.sin6_port);
				#else
					CWDebugLog("Hostapd_wtp (v4) Connect: %s:%d",inet_ntoa(client.sin_addr), client.sin_port);
				#endif
			#endif
			
			cmd[0] = WTPRINFO; //Next info to get
			sendto(sock, cmd, 1, 0, (struct sockaddr *)&client, address_size);
		
		
		}else if( buffer[0]==WTPRINFO_R ){
			
			CWThreadMutexLock(&mutext_info);
				memcpy( &WTP_Radio_Information, buffer + sig_byte, 1);
			CWThreadMutexUnlock(&mutext_info);
			
			CWDebugLog("WTPRINFO_R:  %02X",WTP_Radio_Information);
			
			cmd[0] = GET_RATES; //Next info to get
			sendto(sock, cmd, 1, 0, (struct sockaddr *)&client, address_size);
		
		
		}else if( buffer[0]==GET_RATES_R){
			
			CWThreadMutexLock(&mutext_info);
				memcpy( WTP_Rates, buffer + sig_byte, 8);
			CWThreadMutexUnlock(&mutext_info);
			
			CWDebugLog("GET_RATES_R:   %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X",WTP_Rates[0],WTP_Rates[1],WTP_Rates[2],WTP_Rates[3],WTP_Rates[4],WTP_Rates[5],WTP_Rates[6],WTP_Rates[7]);
			
			cmd[0] = GET_MDC; //Next info to get
			sendto(sock, cmd, 1, 0, (struct sockaddr *)&client, address_size);
				
		
		}else if( buffer[0]==GET_MDC_R){
			
			
			CWThreadMutexLock(&mutext_info);
				memcpy( WTP_MDC, buffer + sig_byte, 6);
			CWThreadMutexUnlock(&mutext_info);
			
			CWDebugLog("GET_MDC_R: %02X  %02X  %02X  %02X  %02X  %02X",
										WTP_MDC[0],
										WTP_MDC[1],
										WTP_MDC[2],
										WTP_MDC[3],
										WTP_MDC[4],
										WTP_MDC[5]);
			
			
			cmd[0] = GET_MAC;
			sendto(sock, cmd, 1, 0, (struct sockaddr *)&client, address_size);
		
		}else if( buffer[0]==GET_MAC_R){
				
			
			CWThreadMutexLock(&gRADIO_MAC_mutex);
				memcpy( gRADIO_MAC, buffer + sig_byte, 6);
			CWThreadMutexUnlock(&gRADIO_MAC_mutex);
			
			CWDebugLog("GET_MAC_R:   %02X  %02X  %02X  %02X  %02X  %02X",
									(unsigned char)gRADIO_MAC[0],
									(unsigned char)gRADIO_MAC[1],
									(unsigned char)gRADIO_MAC[2],
									(unsigned char)gRADIO_MAC[3],
									(unsigned char)gRADIO_MAC[4],
									(unsigned char)gRADIO_MAC[5]);
		
			cmd[0] = GOWAITWLAN;
			sendto(sock, cmd, 1, 0, (struct sockaddr *)&client, address_size);
						
			
		}else if( buffer[0]==CLOSE ){
			
			connected = 0;
			#if defined(LOCALUDP)
				CWDebugLog("Hostapd_wtp Unix Domain DisConnect: %s",client.sun_path);
			#else
				#if defined(USEIPV6)
					CWDebugLog("Hostapd_wtp (v6) DisConnect: %d",client.sin6_port);
				#else
					CWDebugLog("Hostapd_wtp (v4) Disconnect: %s:%d",inet_ntoa(client.sin_addr), client.sin_port);
				#endif
			#endif
		
		}else if( buffer[0]==SET_TXQ_R ){
			
			CWDebugLog("Hostapd WTP \"SET_TXQ_R\" Command\n");
			
		}else if( buffer[0]==GOWAITWLAN_R ){

			CWDebugLog("Hostapd WTP in WAIT \"ADD WLAN\" Command\n");

		}else{

			CWDebugLog("Received Unknow Command from Hostapd WTP(%d)",buffer[0]);
		}

 	}
 	
	close(sock);
	return(NULL);
}
