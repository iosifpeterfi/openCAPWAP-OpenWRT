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
#include "ACipcHostapd.h"


#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define EXIT_FRAME_THREAD(sock)	CWLog("ERROR Handling Frames: application will be closed!"); close(sock); exit(1);

//#define LOCALUDP
//#define NETUDP
//#define USEIPV6

struct client_hostapd{
	char live;
	char start_set_fase;
	int associated;
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

	
};

struct client_hostapd ch[CW_MAX_WTP];

char connected = 0;
int sock;
extern int wtpInRunState;


void CWACsend_data_to_hostapd(int WTPIndex,unsigned char *buf, int len){
	
	unsigned char tmp_buf[3000];
	tmp_buf[0] = DATE_TO_AC;
	memcpy(tmp_buf + 1, buf, len);
	
	if (sendto(sock, tmp_buf, len + 1, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size) < 0) {
		perror("send");
		return;
	}

}

void CWACsend_command_to_hostapd_CLOSE(int WTPIndex,unsigned char *buf, int len){ 
	
	buf[0] = CLOSE;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}



void CWACsend_command_to_hostapd_SET_WTPRINFO(int WTPIndex, char* buf, int len){ 
	
	buf[0] = SET_WTPRINFO;
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}

void CWACsend_command_to_hostapd_SET_RATES(int WTPIndex, char* buf, int len){ 
	
	buf[0] = SET_RATES;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}

void CWACsend_command_to_hostapd_SET_MDC(int WTPIndex, char* buf, int len){ 
	
	buf[0] = SET_MDC;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}

void CWACsend_command_to_hostapd_SET_MAC(int WTPIndex, char* buf, int len){ 
	
	buf[0] = SET_MAC;
	
	if( sendto(sock, buf, len, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}


void CWACsend_command_to_hostapd_GOLIVE(int WTPIndex){ 
	
	char buf[1];

	buf[0] = GOLIVE;
	
	if( sendto(sock, buf, 1, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}

void CWACsend_command_to_hostapd_HAVE_TO_WAIT(int WTPIndex){ 

	char buf[1];
	
	buf[0] = HAVE_TO_WAIT;
	
	if( sendto(sock, buf, 1, 0, (struct sockaddr *)&ch[WTPIndex].client, ch[WTPIndex].address_size)<0 ){
		CWDebugLog("Error to send command frame on socket");
		return;
	}
	
}




#if defined(LOCALUDP)
	void send_close_cmd(struct sockaddr_un cli,int as){
#else
	#if defined(USEIPV6)
		void send_close_cmd(struct sockaddr_in6 cli,int as){
	#else
		void send_close_cmd(struct sockaddr_in cli,int as){
	#endif

#endif
	
	unsigned char cmd[10];
	cmd[0] = CLOSE;
	
	if (sendto(sock, cmd,1, 0, (struct sockaddr *)&cli,as) < 0) {
		perror("send close");
		return;
	}
	
}

int We_Radio_Information_WTP(int WTPIndex){
	int i;
	int a = 0;
	int b = 0;
	int c = 0;
	
	if(	gWTPs[WTPIndex].SuppRates[0]!=0 ) b = 1;
	if( gWTPs[WTPIndex].RadioInformationABGN!=0 ) a = 1;
	

	for(i=0; i<6; i++)
		if(	gWTPs[WTPIndex].MultiDomCapa[i]!=0) c = 1;
		
	if(a && b && c)
		return 1;
	else 
		return 0;
}

int Exist_WTPs(){
	int i;
	for(i=0; i<CW_MAX_WTP; i++) if(gWTPs[i].isNotFree) return 1;
	return 0;
}

int GetWTP_not_associated_to_Hostapd(){
	int i;
	for(i=0; i<CW_MAX_WTP; i++) {
		if(gWTPs[i].isNotFree && ch[i].associated==0)return i;
	}
	return -1;
}

CW_THREAD_RETURN_TYPE CWACipc_with_ac_hostapd(void *arg){
	
	int tmp_WTPIndex = -1;
	
	int len;
	int k;
	
	#if defined(LOCALUDP)
		struct sockaddr_un server;
	#else
		#if defined(USEIPV6)
			struct sockaddr_in6 server;
		#else
			struct sockaddr_in server;
		#endif
	#endif

	unsigned char buffer[2048];
	int connect_ret;
	char cmd[10];
	
	CWProtocolMessage* frame=NULL;
	CWNetworkLev4Address address;
	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);

    #if defined(LOCALUDP)
		sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	
	#elif defined(NETUDP)
		#if defined(USEIPV6)
			bzero(&server, sizeof(server));
			sock = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
		#else
			memset(&server, 0, sizeof(server));
			sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
		#endif
		
	#else
		#if defined(USEIPV6)
			bzero(&server, sizeof(server));
			sock = socket(AF_INET6,SOCK_SEQPACKET,IPPROTO_SCTP);
		#else
			memset(&server, 0, sizeof(server));
			sock = socket(AF_INET,SOCK_SEQPACKET,IPPROTO_SCTP);
		#endif

	#endif

    if (sock < 0) {
		CWDebugLog("AC ipc HOSTAPD: Error creating socket");
		EXIT_FRAME_THREAD(sock);
    }

  
    CWDebugLog("AC ipc HOSTAPD: Trying to connect to hostapd (AC)...");

	#if defined(LOCALUDP)
		server.sun_family = AF_UNIX;
		strcpy(server.sun_path, gHostapd_unix_path);
		unlink(server.sun_path);
		connect_ret = bind(sock, (struct sockaddr *)&server,  strlen(server.sun_path) + sizeof(server.sun_family));
	
	
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
    
    if ( connect_ret == -1) {
		CWDebugLog("AC ipc HOSTAPD: Error connect to socket");
		EXIT_FRAME_THREAD(sock);
    }
    
   	#if defined(LOCALUDP)
   		
   	#elif defined(NETUDP)
   	
	#else
		if (listen(sock, CW_MAX_WTP) < 0){
			CWDebugLog("AC ipc HOSTAPD: Error listen ");
			EXIT_FRAME_THREAD(sock);
		}
	#endif


	int i=0;
	CWProtocolMessage *completeMsgPtr = NULL;
	int fragmentsNum = 0;
	CWMultiHomedSocket *sockPtr = &gACSocket;
	int dataSocket=0;

	#if defined(LOCALUDP)
		struct sockaddr_un client_tmp;
		client_tmp.sun_family = AF_UNIX;
	#else
		#if defined(USEIPV6)
			struct sockaddr_in6 client_tmp;
		#else
			struct sockaddr_in client_tmp;
		#endif
	#endif
	
	int address_size_tmp = sizeof(client_tmp);
	

	for(i=0; i<CW_MAX_WTP; i++){
		ch[i].associated = 0;
		#if defined(LOCALUDP)
			ch[i].client.sun_family = AF_UNIX;
			ch[i].address_size = sizeof(struct sockaddr_un);
		#else
			#if defined(USEIPV6)
				ch[i].client.sin6_port = 0;
			#else
				ch[i].client.sin_port = 0;
			#endif
		#endif

	}
	
	#if defined(LOCALUDP)
		CWLog("Accept Packet at pipe: %s",gHostapd_unix_path);
		
	#elif defined(NETUDP) 
		#if defined(USEIPV6)
			CWLog("Accept UDP v6 Packet at Port: %d",server.sin6_port);
		#else
			CWLog("Accept UDP v4 Packet at Port: %d",server.sin_port);
		#endif
		
	#else
		#if defined(USEIPV6)
			CWLog("Accept SCTP v6 Packet at Port: %d",server.sin6_port);
		#else
			CWLog("Accept SCTP v4 Packet at Port: %d",server.sin_port);
		#endif
		
	#endif
	
 	CW_REPEAT_FOREVER {

		tmp_WTPIndex = -1;
		len = recvfrom(sock, buffer, 3000, 0, (struct sockaddr *)&client_tmp, &address_size_tmp); 
		
		#if defined(LOCALUDP)
			sprintf(client_tmp.sun_path, "%s%c%c%c%c%c",server.sun_path, buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
		#endif
		
		if(Exist_WTPs()==0){ 
			send_close_cmd(client_tmp,sizeof(client_tmp)); 	
			continue;	
		}

		if(len <= 0 ) { continue;	/* EXIT_FRAME_THREAD(sock) */	}
		
		for( i=0; i<CW_MAX_WTP; i++){
			
			#if defined(LOCALUDP)
					if( strcmp(client_tmp.sun_path,ch[i].client.sun_path)==0 ){
						tmp_WTPIndex = i;
						break;
					}
			#else
				#if defined(USEIPV6)
					if( (client_tmp.sin6_port == ch[i].client.sin6_port) && 
						(strncmp(client_tmp.sin6_addr.s6_addr,ch[i].client.sin6_addr.s6_addr,16)==0 )){
						tmp_WTPIndex = i;
					}
				#else
					if( (client_tmp.sin_port == ch[i].client.sin_port) && 
						(strcmp(inet_ntoa(client_tmp.sin_addr),inet_ntoa(ch[i].client.sin_addr))==0 )){
						tmp_WTPIndex = i;
					}
				#endif			
			#endif

		}
		
		if( tmp_WTPIndex<0 ){// Client not recognized
			
			int wtp_non_associated =  GetWTP_not_associated_to_Hostapd ();
			
			if( wtp_non_associated >= 0){
				
				if( buffer[0]==CONNECT ){
					ch[wtp_non_associated].live = 0;
					ch[wtp_non_associated].start_set_fase = 0;
					ch[wtp_non_associated].associated = 1;
					ch[wtp_non_associated].client = client_tmp;
					ch[wtp_non_associated].address_size = address_size_tmp;
					cmd[0] = CONNECT_R;
					sendto(sock, cmd, 1, 0, (struct sockaddr *)&ch[wtp_non_associated].client, ch[wtp_non_associated].address_size);
					CWLog("wtp_non_associated:%d",wtp_non_associated);
					
					#if defined(LOCALUDP)
						CWLog("Hostapd_AC Connect: %s", ch[wtp_non_associated].client.sun_path);
					#else
						#if defined(USEIPV6)
							CWLog("Hostapd_AC (v6) Connect: %d", ch[wtp_non_associated].client.sin6_port);
						#else
							CWLog("Hostapd_AC (v4) Connect: %s:%d",inet_ntoa(ch[wtp_non_associated].client.sin_addr), ch[wtp_non_associated].client.sin_port);
						#endif					
					#endif

					continue;
					
				}else{
					
					send_close_cmd(client_tmp,sizeof(client_tmp)); 
					continue;
					
				}
				
			}else{
				
				send_close_cmd(client_tmp,sizeof(client_tmp)); 
				continue;
				
			}
			
		}else{ // Client recognized
			int sig_byte = 1;

			#if defined(LOCALUDP)
				sig_byte = 6; //Code
			#endif
			
			if( buffer[0]==DATE_TO_WTP ){
				
				if (gWTPs[tmp_WTPIndex].currentState != CW_ENTER_RUN){
					CWDebugLog("AC %d is not in RUN State. The packet was dropped.",i);
					continue;
				}else if (len > (gWTPs[tmp_WTPIndex].pathMTU-20)){
					CWDebugLog("802.11 data length(%d) > MTU(%d)",len, gWTPs[tmp_WTPIndex].pathMTU);
					continue;
				}else{

					len = len - sig_byte; 

					CW_CREATE_OBJECT_ERR(frame, CWProtocolMessage, return 0;);
					CW_CREATE_PROTOCOL_MESSAGE(*frame, len, return 0;);
					
					memcpy(frame->msg, buffer + sig_byte , len);

					
					frame->offset=len;
					frame->data_msgType = CW_IEEE_802_11_FRAME_TYPE;
						
					if (!CWAssembleDataMessage(&completeMsgPtr, &fragmentsNum, gWTPs[tmp_WTPIndex].pathMTU, frame, NULL, CW_PACKET_PLAIN, 0)){
							for(k = 0; k < fragmentsNum; k++){
								CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
							}
							CW_FREE_OBJECT(completeMsgPtr);
							CW_FREE_PROTOCOL_MESSAGE(*frame);
							CW_FREE_OBJECT(frame);
							continue;
					}
						
					for(k = 0; k < sockPtr->count; k++) {
						  if (sockPtr->interfaces[k].sock == gWTPs[tmp_WTPIndex].socket){
						  dataSocket = sockPtr->interfaces[k].dataSock;
						  CW_COPY_NET_ADDR_PTR(&address,&(gWTPs[tmp_WTPIndex].address));
						  break;
						  }
					}

					if (dataSocket == 0){
						  CWDebugLog("data socket of WTP %d isn't ready.");
						  continue;
					}
						
					/* Set port and address of data tunnel */
					sock_set_port_cw((struct sockaddr *)&(address), htons(CW_DATA_PORT));
					for (k = 0; k < fragmentsNum; k++){
						if(!CWNetworkSendUnsafeUnconnected(	dataSocket, &(address),completeMsgPtr[k].msg,completeMsgPtr[k].offset)){
							CWDebugLog("Failure sending Request");
							break;
						}
					}
					for (k = 0; k < fragmentsNum; k++){
						CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
					}
						
					CW_FREE_OBJECT(completeMsgPtr);				
					CW_FREE_PROTOCOL_MESSAGE(*(frame));
					CW_FREE_OBJECT(frame);
				}
				
			}else if( buffer[0]==SET_ADDR ){
				
				int seqNum = CWGetSeqNum();
	 
				if (CWAssembleStationConfigurationRequest(&(gWTPs[tmp_WTPIndex].messages),
								  &(gWTPs[tmp_WTPIndex].messagesCount),
								  gWTPs[tmp_WTPIndex].pathMTU,
								  seqNum,buffer+sig_byte,
								  CW_MSG_ELEMENT_ADD_STATION_CW_TYPE)) {

					if(CWACSendAcknowledgedPacket(tmp_WTPIndex, CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE, seqNum))
						continue;
					else
						CWACStopRetransmission(tmp_WTPIndex);
						
				}	
				
			}else if( buffer[0]==DEL_ADDR){
				int seqNum = CWGetSeqNum();
				
				if (CWAssembleStationConfigurationRequest(&(gWTPs[tmp_WTPIndex].messages),
														  &(gWTPs[tmp_WTPIndex].messagesCount),
														  gWTPs[tmp_WTPIndex].pathMTU,
														  seqNum,buffer+sig_byte,
														  CW_MSG_ELEMENT_DELETE_STATION_CW_TYPE)) {

					if(CWACSendAcknowledgedPacket(tmp_WTPIndex, CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE,seqNum)) 
						continue;
					else
						CWACStopRetransmission(tmp_WTPIndex);
				}
				
			}else if( buffer[0]==ADD_WLAN){
				int seqNum = CWGetSeqNum();

				/* UP TAP Interface */
				char str[100];
				
				sprintf(str, "ifconfig %s down",gWTPs[tmp_WTPIndex].tap_name);
				if( system(str) ){
					CWLog("Error: cmd: \"%s\" ",str);
					EXIT_FRAME_THREAD(sock);
				}

				sprintf(str,"ifconfig %s hw ether %02X:%02X:%02X:%02X:%02X:%02X\n",
																					gWTPs[tmp_WTPIndex].tap_name,
																					(unsigned char)gWTPs[tmp_WTPIndex].RadioMAC[0],
																					(unsigned char)gWTPs[tmp_WTPIndex].RadioMAC[1],
																					(unsigned char)gWTPs[tmp_WTPIndex].RadioMAC[2],
																					(unsigned char)gWTPs[tmp_WTPIndex].RadioMAC[3],
																					(unsigned char)gWTPs[tmp_WTPIndex].RadioMAC[4],
																					(unsigned char)gWTPs[tmp_WTPIndex].RadioMAC[5]);
																					
				if( system(str) ){
					CWLog("Error: cmd: \"%s\" ",str);
					EXIT_FRAME_THREAD(sock);
				}
				
				sprintf(str, "ifconfig %s up",gWTPs[tmp_WTPIndex].tap_name);
				
				if( system(str) ){
					CWLog("Error: cmd: \"%s\" ",str);
					EXIT_FRAME_THREAD(sock);
				}
				
				if (CWAssembleWLANConfigurationRequest(&(gWTPs[tmp_WTPIndex].messages),
								  &(gWTPs[tmp_WTPIndex].messagesCount),
								  gWTPs[tmp_WTPIndex].pathMTU,
								  seqNum,buffer+sig_byte,
								  CW_MSG_ELEMENT_IEEE80211_ADD_WLAN_CW_TYPE,len - sig_byte)) {
				
					if(CWACSendAcknowledgedPacket(tmp_WTPIndex, CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_RESPONSE, seqNum))
						continue;
					else
						CWACStopRetransmission(tmp_WTPIndex);
						
				}	
				
			}else if( buffer[0]==DEL_WLAN){
				int seqNum = CWGetSeqNum();
				if (CWAssembleWLANConfigurationRequest(&(gWTPs[tmp_WTPIndex].messages),
								  &(gWTPs[tmp_WTPIndex].messagesCount),
								  gWTPs[tmp_WTPIndex].pathMTU,
								  seqNum,buffer+sig_byte,
								  CW_MSG_ELEMENT_IEEE80211_DELETE_WLAN_CW_TYPE,len - sig_byte)) {
				
					if(CWACSendAcknowledgedPacket(tmp_WTPIndex, CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_RESPONSE, seqNum))
						continue;
					else
						CWACStopRetransmission(tmp_WTPIndex);
						
				}	

				
			}else if( buffer[0]==CLOSE ){
	
				#if defined(LOCALUDP)
					CWLog("Hostapd_AC Disconnect: %s", ch[tmp_WTPIndex].client.sun_path);
					sprintf(ch[tmp_WTPIndex].client.sun_path, "");
					
				#else
					#if defined(USEIPV6)
						CWLog("Hostapd_AC (v6) Disconnect: %d", ch[tmp_WTPIndex].client.sin6_port);
						ch[tmp_WTPIndex].client.sin6_port = 0;
					#else
						CWLog("Hostapd_AC (v4) Disconnect: %s:%d",inet_ntoa(ch[tmp_WTPIndex].client.sin_addr), ch[tmp_WTPIndex].client.sin_port);
						ch[tmp_WTPIndex].client.sin_port = 0;
					#endif				
				#endif
				ch[tmp_WTPIndex].live = 0;
				ch[tmp_WTPIndex].start_set_fase = 1;
				ch[tmp_WTPIndex].associated = 0;
			
			}else if( buffer[0]==WANT_GOLIVE ){
	
				if( ch[tmp_WTPIndex].start_set_fase ){
					continue;
				}
				if( We_Radio_Information_WTP(tmp_WTPIndex) && ch[tmp_WTPIndex].live==0 ){
						
					/* NEXT STEP SET WTPRINFO*/
					ch[tmp_WTPIndex].start_set_fase = 1;
					char tmp_RadioInfoABGN[2];
				
					CWThreadMutexLock(&gWTPsMutex);
						tmp_RadioInfoABGN[1] = gWTPs[tmp_WTPIndex].RadioInformationABGN;
					CWThreadMutexUnlock(&gWTPsMutex);
					
					CWACsend_command_to_hostapd_SET_WTPRINFO(tmp_WTPIndex, tmp_RadioInfoABGN, 2);

				}else{
					/* NEXT STEP HAVE TO WAIT*/
					CWACsend_command_to_hostapd_HAVE_TO_WAIT(tmp_WTPIndex);				
				}
				
			
			}else if( buffer[0]==SET_WTPRINFO_R ){

				/* NEXT STEP SET RATES*/
				char tmp_SuppRates[9];
				
				CWThreadMutexLock(&gWTPsMutex);
					memcpy( tmp_SuppRates + 1, gWTPs[tmp_WTPIndex].SuppRates, 8);
				CWThreadMutexUnlock(&gWTPsMutex);
				
				CWACsend_command_to_hostapd_SET_RATES(tmp_WTPIndex, tmp_SuppRates, 9);
					
			
			}else if( buffer[0]==SET_RATES_R ){

				/* NEXT STEP  SET MDC */
				char tmp_MultiDomCapa[7];
				
				CWThreadMutexLock(&gWTPsMutex);
					memcpy( tmp_MultiDomCapa + 1, gWTPs[tmp_WTPIndex].MultiDomCapa, 6);
				CWThreadMutexUnlock(&gWTPsMutex);
				
				CWACsend_command_to_hostapd_SET_MDC(tmp_WTPIndex, tmp_MultiDomCapa, 7);
		
			}else if( buffer[0]==SET_MDC_R ){

				/* NEXT STEP  SET MAC*/
				char tmp_mac[7];
			
				CWThreadMutexLock(&gWTPsMutex);
					memcpy(tmp_mac+1, gWTPs[tmp_WTPIndex].RadioMAC, 6);
				CWThreadMutexUnlock(&gWTPsMutex);
				
				CWACsend_command_to_hostapd_SET_MAC(tmp_WTPIndex,tmp_mac,7);
		
			}else if( buffer[0]==SET_MAC_R ){

				/* NEXT STEP  SET MDC */
				CWACsend_command_to_hostapd_GOLIVE(tmp_WTPIndex);
			
			}else if( buffer[0]==GOLIVE_R ){
				CWLog("GOLIVE_R from %d",GOLIVE_R);
				ch[GOLIVE_R].live = 1;	

			}else{
				CWDebugLog("Received Unknow Command from Hostapd AC (%d)",buffer[0]);
			}			
		
		}
		
		
 	}
 	

	close(sock);
	return(NULL);
}

