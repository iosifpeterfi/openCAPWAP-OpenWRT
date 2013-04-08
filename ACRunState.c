/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the Free Software Foundation; either		*
 * version 2 of the License, or (at your option) any later version.								*
 *																								*
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY				*
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A				*
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.						*
 *																								*
 * You should have received a copy of the GNU General Public License along with this			*
 * program; if not, write to the:																*
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,							*
 * MA  02111-1307, USA.																			*
 *																								*
 * -------------------------------------------------------------------------------------------- *
 * Project:  Capwap																				*
 *																								*
 * Authors : Ludovico Rossi (ludo@bluepixysw.com)												*  
 *           Del Moro Andrea (andrea_delmoro@libero.it)											*
 *           Giovannini Federica (giovannini.federica@gmail.com)								*
 *           Massimo Vellucci (m.vellucci@unicampus.it)											*
 *           Mauro Bisson (mauro.bis@gmail.com)													*
 *	         Antonio Davoli (antonio.davoli@gmail.com)											*
 ************************************************************************************************/

#include "CWAC.h"
#include "CWVendorPayloads.h"
#include "CWFreqPayloads.h"
#include "WUM.h"
#include "common.h"
#include "ieee802_11_defs.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWBool CWACParseGenericRunMessage(int WTPIndex,
				  CWProtocolMessage *msg,
				  CWControlHeaderValues* controlVal);

CWBool CWParseConfigurationUpdateResponseMessage(CWProtocolMessage* msgPtr,
						 int len,
						 CWProtocolResultCode* resultCode,
						 CWProtocolVendorSpecificValues** protocolValues);

CWBool CWSaveConfigurationUpdateResponseMessage(CWProtocolResultCode resultCode,
						int WTPIndex,
						CWProtocolVendorSpecificValues* protocolValues);

CWBool CWParseClearConfigurationResponseMessage(CWProtocolMessage* msgPtr,
						int len,
						CWProtocolResultCode* resultCode);
						
CWBool CWParseWLANConfigurationResponseMessage(CWProtocolMessage* msgPtr,
						int len,
						CWProtocolResultCode* resultCode);

CWBool CWParseStationConfigurationResponseMessage(CWProtocolMessage* msgPtr,
						  int len,
						  CWProtocolResultCode* resultCode);

CWBool CWParseWTPDataTransferRequestMessage(CWProtocolMessage *msgPtr,
					    int len,
					    CWProtocolWTPDataTransferRequestValues *valuesPtr);

CWBool CWAssembleWTPDataTransferResponse(CWProtocolMessage **messagesPtr,
					 int *fragmentsNumPtr,
					 int PMTU, int seqNum);

CWBool CWParseWTPEventRequestMessage(CWProtocolMessage *msgPtr,
				     int len,
				     CWProtocolWTPEventRequestValues *valuesPtr);

CWBool CWSaveWTPEventRequestMessage(CWProtocolWTPEventRequestValues *WTPEventRequest,
				    CWWTPProtocolManager *WTPProtocolManager);

CWBool CWAssembleWTPEventResponse(CWProtocolMessage **messagesPtr,
				  int *fragmentsNumPtr,
				  int PMTU,
				  int seqNum);

CWBool CWParseChangeStateEventRequestMessage2(CWProtocolMessage *msgPtr,
					      int len,
					      CWProtocolChangeStateEventRequestValues **valuesPtr);

CWBool CWParseEchoRequestMessage(CWProtocolMessage *msgPtr,
				 int len);

CWBool CWAssembleEchoResponse(CWProtocolMessage **messagesPtr,
			      int *fragmentsNumPtr,
			      int PMTU,
			      int seqNum);

CWBool CWStartNeighborDeadTimer(int WTPIndex);
CWBool CWStopNeighborDeadTimer(int WTPIndex);
CWBool CWRestartNeighborDeadTimer(int WTPIndex);
CWBool CWRestartNeighborDeadTimerForEcho(int WTPIndex);


int flush_pcap(u_char *buf,int len,char *filename){
	
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

#define HLEN_80211 24
int isEAPOL_Frame( unsigned char *buf, int len){
	unsigned char rfc1042_header[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	int i;
	
	for(i=0; i<6; i++)if(rfc1042_header[i]!=buf[i + HLEN_80211])return 0;
	return 1;
}

CWBool ACEnterRun(int WTPIndex, CWProtocolMessage *msgPtr, CWBool dataFlag) {
	
	CWBool toSend= CW_FALSE, timerSet = CW_TRUE;
	CWControlHeaderValues controlVal;
	CWProtocolMessage* messages =NULL;
	int messagesCount=0;
	unsigned char StationMacAddr[MAC_ADDR_LEN];
	char string[10];
	char socketctl_path_name[50];
	char socketserv_path_name[50];
	int msglen = msgPtr->offset;

	msgPtr->offset = 0;
	if(dataFlag){
		/* We have received a Data Message... now just log this event and do actions by the dataType */
		
		CWDebugLog("--> Received a DATA Message");

		if(msgPtr->data_msgType == CW_DATA_MSG_FRAME_TYPE)	{
		
		/*Retrive mac address station from msg*/
		memset(StationMacAddr, 0, MAC_ADDR_LEN);
		memcpy(StationMacAddr, msgPtr->msg+SOURCE_ADDR_START, MAC_ADDR_LEN);
	
		int seqNum = CWGetSeqNum();

		//Send a Station Configuration Request
			if (CWAssembleStationConfigurationRequest(&(gWTPs[WTPIndex].messages),
							  &(gWTPs[WTPIndex].messagesCount),
							  gWTPs[WTPIndex].pathMTU,
							  seqNum,StationMacAddr,
							  CW_MSG_ELEMENT_ADD_STATION_CW_TYPE)) {

				if(CWACSendAcknowledgedPacket(WTPIndex, 
						      CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE,
						      seqNum)) 
				return CW_TRUE;
			else
				CWACStopRetransmission(WTPIndex);
			}
			
			CWDebugLog("Send a Station Configuration Request");
		}else if(msgPtr->data_msgType == CW_DATA_MSG_KEEP_ALIVE_TYPE){
			
			char * valPtr=NULL;
			CWProtocolMessage *messages = NULL;
			CWProtocolMessage sessionIDmsgElem;
			int fragmentsNum = 0;
			int i;
			int dataSocket=0;
			unsigned short int elemType = 0;
			unsigned short int elemLen = 0;
			CWNetworkLev4Address address;

			CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
			valPtr = CWParseSessionID(msgPtr, elemLen);
			CWAssembleMsgElemSessionID(&sessionIDmsgElem, valPtr);
			
			if (!CWAssembleDataMessage(&messages, 
				  &fragmentsNum, 
				  gWTPs[WTPIndex].pathMTU, 
				  &sessionIDmsgElem, 
				  NULL,
				  CW_PACKET_PLAIN,
				  1
				  ))
			{
				CWLog("Failure Assembling KeepAlive Request");
				if(messages)
					for(i = 0; i < fragmentsNum; i++) {
						CW_FREE_PROTOCOL_MESSAGE(messages[i]);
					}	
				CW_FREE_OBJECT(messages);
				return CW_FALSE;
			}

			for(i = 0; i < gACSocket.count; i++) {
			    if (gACSocket.interfaces[i].sock == gWTPs[WTPIndex].socket){
				dataSocket = gACSocket.interfaces[i].dataSock;
				CW_COPY_NET_ADDR_PTR(&address,&(gWTPs[WTPIndex].address));
				break;
			    }
			}

			if (dataSocket == 0){
			      CWLog("data socket of WTP %d isn't ready.");
			      return CW_FALSE;
			}
		      
			/* Set port and address of data tunnel */
			sock_set_port_cw((struct sockaddr *)&(address), htons(CW_DATA_PORT));
			
			for(i = 0; i < fragmentsNum; i++) {
				if(!CWNetworkSendUnsafeUnconnected(	dataSocket, 
									&(address), 
									messages[i].msg, 
									 messages[i].offset))
				{
					CWLog("Failure sending  KeepAlive Request");
					int k;
					for(k = 0; k < fragmentsNum; k++) {
						CW_FREE_PROTOCOL_MESSAGE(messages[k]);
					}	
					CW_FREE_OBJECT(messages);
					break;
				}
			}

			int k;
			for(k = 0; messages && k < fragmentsNum; k++) {
				CW_FREE_PROTOCOL_MESSAGE(messages[k]);
			}	
			CW_FREE_OBJECT(messages);
			
		}else if(msgPtr->data_msgType == CW_IEEE_802_3_FRAME_TYPE){
			
			CWDebugLog("Write 802.3 data to TAP[%d], len:%d",gWTPs[WTPIndex].tap_fd,msglen);
			write(gWTPs[WTPIndex].tap_fd, msgPtr->msg, msglen);
			
		}else if(msgPtr->data_msgType == CW_IEEE_802_11_FRAME_TYPE)	{
			
			struct ieee80211_hdr *hdr;
			u16 fc;
			hdr = (struct ieee80211_hdr *) msgPtr->msg;
			fc = le_to_host16(hdr->frame_control);
 
			if( WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT || isEAPOL_Frame(msgPtr->msg,msglen) ){
				
				CWACsend_data_to_hostapd(WTPIndex,msgPtr->msg, msglen);
				
			}else if( WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_DATA ){
				
				if( WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_NULLFUNC ){
					
					CWACsend_data_to_hostapd(WTPIndex,msgPtr->msg, msglen);
					
				}else{
					
					int write_bytes = write(gWTPs[WTPIndex].tap_fd, msgPtr->msg + HLEN_80211, msglen - HLEN_80211);
			
			
					if(write_bytes != (msglen - 24)){
						CWLog("%02X %02X %02X %02X %02X %02X ",msgPtr->msg[0],
																	msgPtr->msg[1],
																	msgPtr->msg[2],
																	msgPtr->msg[3],
																	msgPtr->msg[4],
																	msgPtr->msg[5]);
						
						CWLog("Error:. RecvByte:%d, write_Byte:%d ",msglen - 24,write_bytes);
					}
					
				}

				
			}else{
				write(gWTPs[WTPIndex].tap_fd, msgPtr->msg + HLEN_80211, msglen - HLEN_80211);
				CWDebugLog("Control Frame !!!\n");
			}
			
			//flush_pcap(msgPtr->msg, msglen, "cap_wtp_to_ac.txt");
			
		}else{
			CWDebugLog("Manage special data packets with frequency");

			/************************************************************
			 * Update 2009:												*
			 *				Manage special data packets with frequency	*
			 *				statistics informations.					*
			 ************************************************************/
			
			if( msgPtr->data_msgType == CW_DATA_MSG_FREQ_STATS_TYPE ) {
				
				int cells; /* How many cell are heard */
                int isAck;
				char * freqPayload; 
				int socketIndex, indexToSend = htonl(WTPIndex);
				
				int sizeofAckInfoUnit = CW_FREQ_ACK_SIZE;
				int sizeofFreqInfoUnit = CW_FREQ_CELL_INFO_PAYLOAD_SIZE;
				int sizeOfPayload = 0, payload_offset = 0;
				
				/*-----------------------------------------------------------------------------------------------
				 *	Payload Management ( infos for frequency application) :
				 *		Ack       Structure : |  WTPIndex  |   Ack Value  | 
				 *      Freq Info Structure : |  WTPIndex  |  Number of cells  |  Frequecies Info Payload | 
				 *-----------------------------------------------------------------------------------------------*/
				
                memcpy(&isAck, msgPtr->msg, sizeof(int));

				isAck = ntohl(isAck);
				
                if ( isAck == 0 ) { /* isnt an ack message */
					memcpy(&cells, msgPtr->msg+sizeof(int), sizeof(int));
					cells = ntohl(cells);
					sizeOfPayload = ( cells * sizeofFreqInfoUnit) + (2*sizeof(int)); 
				}
				else {
					sizeOfPayload = sizeofAckInfoUnit;
				}
				
                if ( ( freqPayload = malloc(sizeOfPayload) ) != NULL ) {
					
					memset(freqPayload, 0, sizeOfPayload);
					memcpy(freqPayload, &indexToSend, sizeof(int));
					payload_offset += sizeof(int);
					
					if ( isAck == 0 ) {
						memcpy(freqPayload+payload_offset, msgPtr->msg+sizeof(int), sizeOfPayload-payload_offset);
					}
					else {
						memcpy(freqPayload+payload_offset, msgPtr->msg+sizeof(int), sizeOfPayload-payload_offset);
					}
					
					socketIndex = gWTPs[WTPIndex].applicationIndex;	
					
					/****************************************************
					 *		Forward payload to correct application 		*
					 ****************************************************/
					
					if(!CWErr(CWThreadMutexLock(&appsManager.socketMutex[socketIndex]))) {
						CWLog("[ACrunState]:: Error locking socket Application Mutex");
						free(freqPayload);
						return CW_FALSE;
					}
					
					if ( Writen(appsManager.appSocket[socketIndex], freqPayload, sizeOfPayload)  < 0 ) {
                      CWThreadMutexUnlock(&appsManager.socketMutex[socketIndex]);
                      free(freqPayload);
                      CWLog("[ACrunState]:: Error writing Message To Application");
                      return CW_FALSE;
					}
					
					CWThreadMutexUnlock(&appsManager.socketMutex[socketIndex]);
					free(freqPayload);
				}
				else
					 CWLog("[ACrunState]:: Malloc error (payload to frequency application");
			}
			

		  if(msgPtr->data_msgType == CW_DATA_MSG_STATS_TYPE)
			{			  
			  if(!UnixSocksArray[WTPIndex].data_stats_sock)
				{	//Init Socket only the first time when the function is called
				  if ((UnixSocksArray[WTPIndex].data_stats_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) 
					{
					  CWDebugLog("Error creating socket for data send");
					  return CW_FALSE;
    				}
				  
				  memset(&(UnixSocksArray[WTPIndex].clntaddr),(int)NULL, sizeof(UnixSocksArray[WTPIndex].clntaddr));
				  UnixSocksArray[WTPIndex].clntaddr.sun_family = AF_UNIX;
				  
				  //make unix socket client path name by index i 
				snprintf(string,sizeof(string),"%d",WTPIndex);
				string[sizeof(string)-1]=0;
				strcpy(socketctl_path_name,SOCKET_PATH_AC);
				strcat(socketctl_path_name,string);
				strcpy(UnixSocksArray[WTPIndex].clntaddr.sun_path,socketctl_path_name);
				
				unlink(socketctl_path_name);
				
				memset(&(UnixSocksArray[WTPIndex].servaddr),(int)NULL, sizeof(UnixSocksArray[WTPIndex].servaddr));
				UnixSocksArray[WTPIndex].servaddr.sun_family = AF_UNIX;

				//make unix socket server path name by index i 
				strcpy(socketserv_path_name, SOCKET_PATH_RECV_AGENT);
				strcat(socketserv_path_name, string);
				strcpy(UnixSocksArray[WTPIndex].servaddr.sun_path, socketserv_path_name);
				printf("\n%s\t%s",socketserv_path_name,socketctl_path_name);fflush(stdout);
				}
			  

			  int nbytes;
			  int pDataLen=656; //len of Monitoring Data
			  
			  //Send data stats from AC thread to monitor client over unix socket
			  nbytes = sendto(UnixSocksArray[WTPIndex].data_stats_sock, msgPtr->msg,pDataLen, 0,
							  (struct sockaddr *) &(UnixSocksArray[WTPIndex].servaddr),sizeof(UnixSocksArray[WTPIndex].servaddr));
			  if (nbytes < 0) 
				{
				  CWDebugLog("Error sending data over socket");
				  return CW_FALSE;
				}
			
			}
		}

		return CW_TRUE;
	}

	if(!(CWACParseGenericRunMessage(WTPIndex, msgPtr, &controlVal))) {
		/* Two possible errors: WRONG_ARG and INVALID_FORMAT
		 * In the second case we have an unexpected response: ignore the
		 * message and log the event.
		 */
		return CW_FALSE;
	}

	switch(controlVal.messageTypeValue) {
		case CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			/*Update 2009:
				Store Protocol specific response data*/
			CWProtocolVendorSpecificValues* protocolValues = NULL;
			
			if(!(CWParseConfigurationUpdateResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode, &protocolValues)))
				return CW_FALSE;
			
			CWACStopRetransmission(WTPIndex);
			
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}

			
			CWSaveConfigurationUpdateResponseMessage(resultCode, WTPIndex, protocolValues);
			
			if (gWTPs[WTPIndex].interfaceCommandProgress == CW_TRUE) {

				CWThreadMutexLock(&gWTPs[WTPIndex].interfaceMutex);
				
				gWTPs[WTPIndex].interfaceResult = 1;
				gWTPs[WTPIndex].interfaceCommandProgress = CW_FALSE;
				CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceComplete);

				CWThreadMutexUnlock(&gWTPs[WTPIndex].interfaceMutex);
			}

			break;
		}
		case CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_REQUEST:
		{
			CWProtocolChangeStateEventRequestValues *valuesPtr;
		
			if(!(CWParseChangeStateEventRequestMessage2(msgPtr, controlVal.msgElemsLen, &valuesPtr)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			if(!(CWSaveChangeStateEventRequestMessage(valuesPtr, &(gWTPs[WTPIndex].WTPProtocolManager))))
				return CW_FALSE;
			if(!(CWAssembleChangeStateEventResponse(&messages,
								&messagesCount,
								gWTPs[WTPIndex].pathMTU,
								controlVal.seqNum)))
				return CW_FALSE;

			toSend = CW_TRUE;
			break;
		}
		case CW_MSG_TYPE_VALUE_ECHO_REQUEST:
		{
			if(!(CWParseEchoRequestMessage(msgPtr, controlVal.msgElemsLen)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			
			if(!(CWAssembleEchoResponse(&messages,
						    &messagesCount,
						    gWTPs[WTPIndex].pathMTU,
						    controlVal.seqNum)))
				return CW_FALSE;

			toSend = CW_TRUE;	
			break;
		}
		case CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			if(!(CWParseStationConfigurationResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode)))
				return CW_FALSE;
			CWACStopRetransmission(WTPIndex);
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			//CWSaveStationConfigurationResponseMessage(resultCode, WTPIndex);  <-- Must be Implemented ????

			break;
		}	
		case CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			if(!(CWParseClearConfigurationResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode)))
				return CW_FALSE;
			CWACStopRetransmission(WTPIndex);
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			
			
			if (gWTPs[WTPIndex].interfaceCommandProgress == CW_TRUE)
			{
				CWThreadMutexLock(&gWTPs[WTPIndex].interfaceMutex);
				
				gWTPs[WTPIndex].interfaceResult = 1;
				gWTPs[WTPIndex].interfaceCommandProgress = CW_FALSE;
				CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceComplete);

				CWThreadMutexUnlock(&gWTPs[WTPIndex].interfaceMutex);
			}

			break;
		}
		case CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			if(!(CWParseWLANConfigurationResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode)))
				return CW_FALSE;
			CWACStopRetransmission(WTPIndex);
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			
			
			if (gWTPs[WTPIndex].interfaceCommandProgress == CW_TRUE)
			{
				CWThreadMutexLock(&gWTPs[WTPIndex].interfaceMutex);
				
				gWTPs[WTPIndex].interfaceResult = 1;
				gWTPs[WTPIndex].interfaceCommandProgress = CW_FALSE;
				CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceComplete);

				CWThreadMutexUnlock(&gWTPs[WTPIndex].interfaceMutex);
			}

			break;
		}			
		case CW_MSG_TYPE_VALUE_DATA_TRANSFER_REQUEST:
		{
			CWProtocolWTPDataTransferRequestValues valuesPtr;
			
			if(!(CWParseWTPDataTransferRequestMessage(msgPtr, controlVal.msgElemsLen, &valuesPtr)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			if(!(CWAssembleWTPDataTransferResponse(&messages, &messagesCount, gWTPs[WTPIndex].pathMTU, controlVal.seqNum))) 
				return CW_FALSE;
			toSend = CW_TRUE;
			break;
		}
		case CW_MSG_TYPE_VALUE_WTP_EVENT_REQUEST:
		{
			CWProtocolWTPEventRequestValues valuesPtr;

			if(!(CWParseWTPEventRequestMessage(msgPtr, controlVal.msgElemsLen, &valuesPtr)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			if(!(CWSaveWTPEventRequestMessage(&valuesPtr, &(gWTPs[WTPIndex].WTPProtocolManager))))
				return CW_FALSE;
			
			if(!(CWAssembleWTPEventResponse(&messages,
							&messagesCount,
							gWTPs[WTPIndex].pathMTU,
							controlVal.seqNum)))
 				return CW_FALSE;

			toSend = CW_TRUE;	
			break;
		}
		default: 
			/*
			 * We have an unexpected request and we have to send
			 * a corresponding response containing a failure result code
			 */
			CWDebugLog("--> Not valid Request in Run State... we send a failure Response");
			
			if(!(CWAssembleUnrecognizedMessageResponse(&messages,
								   &messagesCount,
								   gWTPs[WTPIndex].pathMTU,
								   controlVal.seqNum,
								   controlVal.messageTypeValue + 1))) 
 				return CW_FALSE;

			toSend = CW_TRUE;
			/*return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message not valid in Run State");*/
	}	
	if(toSend){
		int i;
	
		if(messages == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
		
		for(i = 0; i < messagesCount; i++) {
#ifdef CW_NO_DTLS
			if(!CWNetworkSendUnsafeUnconnected(gWTPs[WTPIndex].socket, 
							   &gWTPs[WTPIndex].address, 
							   messages[i].msg, 
							   messages[i].offset)	) {
#else
			if(!(CWSecuritySend(gWTPs[WTPIndex].session,
					    messages[i].msg,
					    messages[i].offset))) {
#endif
				CWFreeMessageFragments(messages, messagesCount);
				CW_FREE_OBJECT(messages);
				return CW_FALSE;
			}
		}
		CWFreeMessageFragments(messages, messagesCount);
		CW_FREE_OBJECT(messages);
	}
	gWTPs[WTPIndex].currentState = CW_ENTER_RUN;
	gWTPs[WTPIndex].subState = CW_WAITING_REQUEST;

	return CW_TRUE;
}

CWBool CWACParseGenericRunMessage(int WTPIndex,
				  CWProtocolMessage *msg,
				  CWControlHeaderValues* controlVal) {

	if(msg == NULL || controlVal == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!(CWParseControlHeader(msg, controlVal)))
		/* will be handled by the caller */
		return CW_FALSE;

	/* skip timestamp */
	controlVal->msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;

	/* Check if it is a request */
	if(controlVal->messageTypeValue % 2 == 1){

		return CW_TRUE;	
	}

	if((gWTPs[WTPIndex].responseSeqNum != controlVal->seqNum) ||
	   (gWTPs[WTPIndex].responseType != controlVal->messageTypeValue)) {

		CWDebugLog("gWTPs: %d\n", gWTPs[WTPIndex].responseSeqNum);
		CWDebugLog("controlVal: %d\n", controlVal->seqNum);
		CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Seq Num or Msg Type not valid!");
		return CW_FALSE;
	}

	return CW_TRUE;	
}

/*Update 2009:
	Added vendValues to include a response payload (to pass response data)*/
CWBool CWParseConfigurationUpdateResponseMessage(CWProtocolMessage* msgPtr,
						 int len,
						 CWProtocolResultCode* resultCode,
						 CWProtocolVendorSpecificValues** vendValues) {

	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing Configuration Update Response...");

	/* parse message elements */
	while((msgPtr->offset - offsetTillMessages) < len) {

		unsigned short int elemType = 0;
		unsigned short int elemLen = 0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode=CWProtocolRetrieve32(msgPtr);
				break;	

			/*Update 2009:
				Added case to implement conf update response with payload*/
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE_WITH_PAYLOAD:
				{
				int payloadSize = 0;
				CW_CREATE_OBJECT_ERR(*vendValues, CWProtocolVendorSpecificValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				*resultCode=CWProtocolRetrieve32(msgPtr);

				if (CWProtocolRetrieve16(msgPtr) != CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_CW_TYPE)
					/*For now, we only have UCI payloads, so we will accept only vendor payloads for protocol data*/
						return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");

				(*vendValues)->vendorPayloadType = CWProtocolRetrieve16(msgPtr);

				switch ((*vendValues)->vendorPayloadType) {
					case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:
						payloadSize = CWProtocolRetrieve32(msgPtr);
						if (payloadSize != 0) {
							(*vendValues)->payload = (void *) CWProtocolRetrieveStr(msgPtr, payloadSize);
						} else 
							(*vendValues)->payload = NULL;
						break;
					case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:
						payloadSize = CWProtocolRetrieve32(msgPtr);
						
						if (payloadSize <= 0) {
							/* Payload can't be zero here,
							 * at least the message type must be specified */
							return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");
						} 
						(*vendValues)->payload = (void *) CWProtocolRetrieveRawBytes(msgPtr, payloadSize);
						break;
					default:
						return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");
					break;	
				}
				}
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("Configuration Update Response Parsed");

	return CW_TRUE;	
}

CWBool CWParseWLANConfigurationResponseMessage(CWProtocolMessage* msgPtr, int len, CWProtocolResultCode* resultCode)
{
	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing WLAN Configuration Response...");

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode = CWProtocolRetrieve32(msgPtr);
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration WLAN Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("WLAN Configuration Response Parsed");

	return CW_TRUE;	
}	

CWBool CWParseClearConfigurationResponseMessage(CWProtocolMessage* msgPtr, int len, CWProtocolResultCode* resultCode)
{
	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing Clear Configuration Response...");

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode=CWProtocolRetrieve32(msgPtr);
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("Clear Configuration Response Parsed");

	return CW_TRUE;	
}	
		
CWBool CWParseStationConfigurationResponseMessage(CWProtocolMessage* msgPtr, int len, CWProtocolResultCode* resultCode)
{
	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing Station Configuration Response...");

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode=CWProtocolRetrieve32(msgPtr);
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Station Configuration Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("Station Configuration Response Parsed");

	return CW_TRUE;	
}

CWBool CWSaveConfigurationUpdateResponseMessage(CWProtocolResultCode resultCode,
						int WTPIndex,
						CWProtocolVendorSpecificValues* vendValues) {
	char *wumPayloadBytes = NULL;
	int closeWTPManager = CW_FALSE;

	if (vendValues != NULL) {
		char * responseBuffer; 
		int socketIndex, payloadSize, headerSize, netWTPIndex, netresultCode, netpayloadSize;


		/********************************
		 *Payload Management		*
		 ********************************/

		headerSize = 3*sizeof(int);
		
		switch (vendValues->vendorPayloadType) {
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:
			if (vendValues->payload != NULL)
				payloadSize = strlen((char *) vendValues->payload);
			else
				payloadSize = 0;
			break;
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:
			wumPayloadBytes = vendValues->payload;
			payloadSize = 1;
			
			/*
			 * When dealing with WUM responses, the dafault size
			 * is 1 bytes, which is used for the type.
			 *
			 * The only response message with a bigger payload is the
			 * WTP_VERSION_RESPONSE (4 bytes), as it carries the WTP version
			 * together with the response type.
			 */
			if (wumPayloadBytes[0] == WTP_VERSION_RESPONSE)
				payloadSize = 4;

			/*
			 * If we received a positive WTP_COMMIT_ACK, we need to terminate
			 * the WTP Manager Thread.
			 */
			if (wumPayloadBytes[0] == WTP_COMMIT_ACK && resultCode == CW_PROTOCOL_SUCCESS)
				closeWTPManager = CW_TRUE;
			break;
		}

		if ( ( responseBuffer = malloc( headerSize+payloadSize ) ) != NULL ) {

			netWTPIndex = htonl(WTPIndex);
			memcpy(responseBuffer, &netWTPIndex, sizeof(int));

			netresultCode = htonl(resultCode);
			memcpy(responseBuffer+sizeof(int), &netresultCode, sizeof(int));

			netpayloadSize = htonl(payloadSize);
			memcpy(responseBuffer+(2*sizeof(int)), &netpayloadSize, sizeof(int));
			
			if (payloadSize > 0) {
				memcpy(responseBuffer+headerSize, (char *) vendValues->payload, payloadSize);
				if (vendValues->vendorPayloadType == CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI) 
					((char *)vendValues->payload)[payloadSize] = '\0';
			}


			socketIndex = gWTPs[WTPIndex].applicationIndex;	

			/****************************************************
		         * Forward payload to correct application  	    *
			 ****************************************************/

			if(!CWErr(CWThreadMutexLock(&appsManager.socketMutex[socketIndex]))) {
				CWLog("Error locking numSocketFree Mutex");
				return CW_FALSE;
			}
			
			if ( Writen(appsManager.appSocket[socketIndex], responseBuffer, headerSize+payloadSize)  < 0 ) {
				CWThreadMutexUnlock(&appsManager.socketMutex[socketIndex]);
				CWLog("Error locking numSocketFree Mutex");
				return CW_FALSE;
			}

			CWThreadMutexUnlock(&appsManager.socketMutex[socketIndex]);

		}
		CW_FREE_OBJECT(responseBuffer);
		CW_FREE_OBJECT(vendValues->payload);
		CW_FREE_OBJECT(vendValues);

	}else if(!CWBindingSaveConfigurationUpdateResponse(resultCode, WTPIndex)) {
	
		return CW_FALSE;
	}
	
	/*
	 * On a positive WTP_COMMIT_ACK, we need to close the WTP Manager.
	 */
	if (closeWTPManager) {
		gWTPs[WTPIndex].isRequestClose = CW_TRUE;
		CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceWait);
	}

	CWDebugLog("Configuration Update Response Saved");
	return CW_TRUE;
}

CWBool CWParseWTPDataTransferRequestMessage(CWProtocolMessage *msgPtr, int len, CWProtocolWTPDataTransferRequestValues *valuesPtr)
{
	int offsetTillMessages;
	

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	

	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ WTP Data Transfer (Run) ________#");
	CWLog("Parsing WTP Data Transfer Request...");
	
	

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_DATA_TRANSFER_DATA_CW_TYPE:{	
				if (!(CWParseMsgElemDataTransferData(msgPtr, elemLen, valuesPtr)))
					return CW_FALSE;
				CWDebugLog("----- %s --------\n",valuesPtr->debug_info);
				break;	
			}
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in WTP Data Transfer Request");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");


	return CW_TRUE;	
}

CWBool CWParseWTPEventRequestMessage(CWProtocolMessage *msgPtr,
				     int len,
				     CWProtocolWTPEventRequestValues *valuesPtr) {

	int offsetTillMessages;
	int i=0, k=0;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	/*
	CW_CREATE_OBJECT_ERR(valuesPtr, CWProtocolWTPEventRequestValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*/
	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ WTP Event (Run) ________#");
	CWLog("Parsing WTP Event Request...");
	
	valuesPtr->errorReportCount = 0;
	valuesPtr->errorReport = NULL;
	valuesPtr->duplicateIPv4 = NULL;
	valuesPtr->duplicateIPv6 = NULL;
	valuesPtr->WTPOperationalStatisticsCount = 0;
	valuesPtr->WTPOperationalStatistics = NULL;
	valuesPtr->WTPRadioStatisticsCount = 0;
	valuesPtr->WTPRadioStatistics = NULL;
	valuesPtr->WTPRebootStatistics = NULL;

	/* parse message elements */
	while((msgPtr->offset - offsetTillMessages) < len) {

		unsigned short int elemType = 0;
		unsigned short int elemLen = 0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->errorReport, 
						     CWDecryptErrorReportValues,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseMsgElemDecryptErrorReport(msgPtr, elemLen, valuesPtr->errorReport)))
					return CW_FALSE;
				break;	
			case CW_MSG_ELEMENT_DUPLICATE_IPV4_ADDRESS_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->duplicateIPv4,
						     WTPDuplicateIPv4,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
				
				CW_CREATE_ARRAY_ERR((valuesPtr->duplicateIPv4)->MACoffendingDevice_forIpv4,
						    6,
						    unsigned char,
						    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseMsgElemDuplicateIPv4Address(msgPtr, elemLen, valuesPtr->duplicateIPv4)))
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_DUPLICATE_IPV6_ADDRESS_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->duplicateIPv6,
						     WTPDuplicateIPv6,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				CW_CREATE_ARRAY_ERR((valuesPtr->duplicateIPv6)->MACoffendingDevice_forIpv6,
						    6,
						    unsigned char,
						    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseMsgElemDuplicateIPv6Address(msgPtr, elemLen, valuesPtr->duplicateIPv6)))
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE:
				valuesPtr->WTPOperationalStatisticsCount++;
				msgPtr->offset += elemLen;
				break;
			case CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE:
				valuesPtr->WTPRadioStatisticsCount++;
				msgPtr->offset += elemLen;
				break;
			case CW_MSG_ELEMENT_WTP_REBOOT_STATISTICS_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->WTPRebootStatistics,
						     WTPRebootStatisticsInfo,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseWTPRebootStatistics(msgPtr, elemLen, valuesPtr->WTPRebootStatistics)))
					return CW_FALSE;	
				break;
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in WTP Event Request");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	CW_CREATE_ARRAY_ERR(valuesPtr->WTPOperationalStatistics,
			    valuesPtr->WTPOperationalStatisticsCount,
			    WTPOperationalStatisticsValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	CW_CREATE_ARRAY_ERR(valuesPtr->WTPRadioStatistics,
			    valuesPtr->WTPRadioStatisticsCount,
			    WTPRadioStatisticsValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	

	msgPtr->offset = offsetTillMessages;

	while((msgPtr->offset - offsetTillMessages) < len) {
	
		unsigned short int elemType = 0;
		unsigned short int elemLen = 0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE:
				if (!(CWParseWTPOperationalStatistics(msgPtr,
								      elemLen,
								      &(valuesPtr->WTPOperationalStatistics[k++]))))
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE:
				if (!(CWParseWTPRadioStatistics(msgPtr,
								elemLen,
								&(valuesPtr->WTPRadioStatistics[i++]))))
					return CW_FALSE;
				break;
			default:
				msgPtr->offset += elemLen;
				break;
		}
	}
	CWLog("WTP Event Request Parsed");
	return CW_TRUE;	
}

CWBool CWSaveWTPEventRequestMessage(CWProtocolWTPEventRequestValues *WTPEventRequest,
				    CWWTPProtocolManager *WTPProtocolManager) {

	if(WTPEventRequest == NULL || WTPProtocolManager == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if(WTPEventRequest->WTPRebootStatistics) {	

		CW_FREE_OBJECT(WTPProtocolManager->WTPRebootStatistics);
		WTPProtocolManager->WTPRebootStatistics = WTPEventRequest->WTPRebootStatistics;
	}

	if((WTPEventRequest->WTPOperationalStatisticsCount) > 0) {

		int i,k;
		CWBool found = CW_FALSE;

		for(i = 0; i < (WTPEventRequest->WTPOperationalStatisticsCount); i++) {
			
			found=CW_FALSE;
			for(k=0; k<(WTPProtocolManager->radiosInfo).radioCount; k++) {

				if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == (WTPEventRequest->WTPOperationalStatistics[i]).radioID) {

					found=CW_TRUE;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].TxQueueLevel = (WTPEventRequest->WTPOperationalStatistics[i]).TxQueueLevel;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].wirelessLinkFramesPerSec = (WTPEventRequest->WTPOperationalStatistics[i]).wirelessLinkFramesPerSec;
				}
			}
			/*if(!found)
			{
				for(k=0; k<(WTPProtocolManager->radiosInfo).radioCount; k++)
				{
					if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == UNUSED_RADIO_ID); 
					{
						(WTPProtocolManager->radiosInfo).radiosInfo[k].radioID = (WTPEventRequest->WTPOperationalStatistics[i]).radioID;
						(WTPProtocolManager->radiosInfo).radiosInfo[k].TxQueueLevel = (WTPEventRequest->WTPOperationalStatistics[i]).TxQueueLevel;
						(WTPProtocolManager->radiosInfo).radiosInfo[k].wirelessLinkFramesPerSec = (WTPEventRequest->WTPOperationalStatistics[i]).wirelessLinkFramesPerSec;
					}
				}	
			}*/
		}
	}

	if((WTPEventRequest->WTPRadioStatisticsCount) > 0) {
		
		int i,k;
		CWBool found;

		for(i=0; i < (WTPEventRequest->WTPRadioStatisticsCount); i++) {
			found=CW_FALSE;
			for(k = 0; k < (WTPProtocolManager->radiosInfo).radioCount; k++)  {

				if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == (WTPEventRequest->WTPOperationalStatistics[i]).radioID) {

					found=CW_TRUE;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].statistics = (WTPEventRequest->WTPRadioStatistics[i]).WTPRadioStatistics;
				}
			}
			/*if(!found)
			{
				for(k=0; k<(WTPProtocolManager->radiosInfo).radioCount; k++) 
				{
					if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == UNUSED_RADIO_ID);
					{
						(WTPProtocolManager->radiosInfo).radiosInfo[k].radioID = (WTPEventRequest->WTPOperationalStatistics[i]).radioID;
						(WTPProtocolManager->radiosInfo).radiosInfo[k].statistics = (WTPEventRequest->WTPRadioStatistics[i]).WTPRadioStatistics;
					}
				}	
			}*/
		}
	}
	/*
	CW_FREE_OBJECT((WTPEventRequest->WTPOperationalStatistics), (WTPEventRequest->WTPOperationalStatisticsCount));
	CW_FREE_OBJECTS_ARRAY((WTPEventRequest->WTPRadioStatistics), (WTPEventRequest->WTPRadioStatisticsCount));
	Da controllare!!!!!!!
	*/
	CW_FREE_OBJECT(WTPEventRequest->WTPOperationalStatistics);
	CW_FREE_OBJECT(WTPEventRequest->WTPRadioStatistics);
	/*CW_FREE_OBJECT(WTPEventRequest);*/

	return CW_TRUE;
}

CWBool CWAssembleWTPDataTransferResponse (CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum) 
{
	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=0;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;

	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling WTP Data Transfer Response...");
		
	if(!(CWAssembleMessage(messagesPtr, 
	                       fragmentsNumPtr, 
	                       PMTU, 
	                       seqNum,
			               CW_MSG_TYPE_VALUE_DATA_TRANSFER_RESPONSE,
			               msgElems, 
			               msgElemCount, 
			               msgElemsBinding,
						   msgElemBindingCount, 
#ifdef CW_NO_DTLS
						   CW_PACKET_PLAIN))) 
#else
			               CW_PACKET_CRYPT))) 
#endif 

		return CW_FALSE;
	
	CWLog("WTP Data Transfer Response Assembled");
	
	return CW_TRUE;
}

CWBool CWAssembleWTPEventResponse(CWProtocolMessage **messagesPtr,
				  int *fragmentsNumPtr,
				  int PMTU,
				  int seqNum) {

	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=0;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;

	if(messagesPtr == NULL || fragmentsNumPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling WTP Event Response...");
		
	if(!(CWAssembleMessage(messagesPtr,
			       fragmentsNumPtr,
			       PMTU,
			       seqNum,
			       CW_MSG_TYPE_VALUE_WTP_EVENT_RESPONSE,
			       msgElems,
			       msgElemCount,
			       msgElemsBinding,
			       msgElemBindingCount,
#ifdef CW_NO_DTLS
			      CW_PACKET_PLAIN))) 
#else
			      CW_PACKET_CRYPT))) 
#endif
	  
		return CW_FALSE;
	
	CWLog("WTP Event Response Assembled");
	
	return CW_TRUE;
}

CWBool CWParseChangeStateEventRequestMessage2(CWProtocolMessage *msgPtr,
					      int len,
					      CWProtocolChangeStateEventRequestValues **valuesPtr) {

	int offsetTillMessages;
	int i=0;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_OBJECT_ERR(*valuesPtr,
			     CWProtocolChangeStateEventRequestValues,
			     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ WTP Change State Event (Run) ________#");
	
	(*valuesPtr)->radioOperationalInfo.radiosCount = 0;
	(*valuesPtr)->radioOperationalInfo.radios = NULL;
	
	/* parse message elements */
	while((msgPtr->offset-offsetTillMessages) < len) {
		unsigned short int elemType = 0;/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int elemLen = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(msgPtr,&elemType,&elemLen);		

		/*CWDebugLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen);*/

		switch(elemType) {
			case CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE:
				/* just count how many radios we have, so we 
				 * can allocate the array
				 */
				((*valuesPtr)->radioOperationalInfo.radiosCount)++;
				msgPtr->offset += elemLen;
				break;
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE: 
				if(!(CWParseResultCode(msgPtr, elemLen, &((*valuesPtr)->resultCode)))) 
					return CW_FALSE;
				break;
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Change State Event Request");
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	CW_CREATE_ARRAY_ERR((*valuesPtr)->radioOperationalInfo.radios,
			    (*valuesPtr)->radioOperationalInfo.radiosCount,
			    CWRadioOperationalInfoValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
	msgPtr->offset = offsetTillMessages;
	
	i = 0;

	while(msgPtr->offset-offsetTillMessages < len) {
		unsigned short int type = 0;	/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int len = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(msgPtr,&type,&len);		

		switch(type) {
			case CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseWTPRadioOperationalState(msgPtr, len, &((*valuesPtr)->radioOperationalInfo.radios[i])))) 
					return CW_FALSE;
				i++;
				break;
			default:
				msgPtr->offset += len;
				break;
		}
	}
	CWLog("Change State Event Request Parsed");
	return CW_TRUE;
}

CWBool CWSaveChangeStateEventRequestMessage(CWProtocolChangeStateEventRequestValues *valuesPtr,
					    CWWTPProtocolManager *WTPProtocolManager) {

	CWBool found;
	CWBool retValue = CW_TRUE;

	if(valuesPtr == NULL || WTPProtocolManager == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if((valuesPtr->radioOperationalInfo.radiosCount) >0) {
	
		int i,k;
		for(i=0; i<(valuesPtr->radioOperationalInfo.radiosCount); i++) {

			found=CW_FALSE;
			for(k=0; k<(WTPProtocolManager->radiosInfo).radioCount; k++) {

				if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == (valuesPtr->radioOperationalInfo.radios[i]).ID) {

					found=CW_TRUE;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].operationalState = (valuesPtr->radioOperationalInfo.radios[i]).state;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].operationalCause = (valuesPtr->radioOperationalInfo.radios[i]).cause;
				}
				if(!found) 
					retValue= CW_FALSE;
			}
		}
	}
	
	CW_FREE_OBJECT(valuesPtr->radioOperationalInfo.radios)
	CW_FREE_OBJECT(valuesPtr);	

	return retValue;
}


CWBool CWParseEchoRequestMessage(CWProtocolMessage *msgPtr, int len) {

	int offsetTillMessages;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ Echo Request (Run) ________#");
	
	/* parse message elements */
	while((msgPtr->offset-offsetTillMessages) < len) {
		unsigned short int elemType = 0;/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int elemLen = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(msgPtr,&elemType,&elemLen);		

		/*CWDebugLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen);*/

		switch(elemType) {
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Echo Request must carry no message elements");
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	CWLog("Echo Request Parsed");
	
	return CW_TRUE;
}

CWBool CWAssembleEchoResponse(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum) {

	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=0;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Echo Response...");
		
	if(!(CWAssembleMessage(messagesPtr,
			       fragmentsNumPtr,
			       PMTU,
			       seqNum,
			       CW_MSG_TYPE_VALUE_ECHO_RESPONSE,
			       msgElems,
			       msgElemCount,
			       msgElemsBinding,
			       msgElemBindingCount,
#ifdef CW_NO_DTLS
			      CW_PACKET_PLAIN)))
#else
			      CW_PACKET_CRYPT))) 
#endif
  
		return CW_FALSE;
	
	CWLog("Echo Response Assembled");
	return CW_TRUE;
}

CWBool CWAssembleConfigurationUpdateRequest(CWProtocolMessage **messagesPtr,
					    int *fragmentsNumPtr,
					    int PMTU,
						int seqNum,
						int msgElement) {

	CWProtocolMessage *msgElemsBinding = NULL;
	int msgElemBindingCount=0;
	CWProtocolMessage *msgElems = NULL;
	int msgElemCount=0;
	
	if (messagesPtr == NULL || fragmentsNumPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Configuration Update Request...");

	switch (msgElement) {
	case CONFIG_UPDATE_REQ_QOS_ELEMENT_TYPE:
	  {
		if(!CWBindingAssembleConfigurationUpdateRequest(&msgElemsBinding, &msgElemBindingCount, BINDING_MSG_ELEMENT_TYPE_WTP_QOS)) {
		  return CW_FALSE;
		}
		break;
	  }
	case CONFIG_UPDATE_REQ_OFDM_ELEMENT_TYPE:
	  {
		if(!CWBindingAssembleConfigurationUpdateRequest(&msgElemsBinding, &msgElemBindingCount, BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL)) {
		  return CW_FALSE;
		}
		break;
	  }
	case CONFIG_UPDATE_REQ_VENDOR_UCI_ELEMENT_TYPE:
	  {
		CWLog("Assembling UCI Conf Update Request");
		if(!CWProtocolAssembleConfigurationUpdateRequest(&msgElems, &msgElemCount, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI)) {
		  return CW_FALSE;
		}
		break;
	  }
	case CONFIG_UPDATE_REQ_VENDOR_WUM_ELEMENT_TYPE:
	 {
                CWLog("Assembling WUM Conf Update Request");
                if(!CWProtocolAssembleConfigurationUpdateRequest(&msgElems, &msgElemCount, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM)) {
                  return CW_FALSE;
                }
                break;
         }
	}	  

	if(!(CWAssembleMessage(messagesPtr,
			       fragmentsNumPtr,
			       PMTU,
			       seqNum,
			       CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_REQUEST,
			       msgElems,
			       msgElemCount,
			       msgElemsBinding,
			       msgElemBindingCount,
#ifdef CW_NO_DTLS
			      CW_PACKET_PLAIN)))
#else
			      CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;

	CWLog("Configuration Update Request Assembled");
	
	return CW_TRUE;
}

CWBool CWAssembleClearConfigurationRequest(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum) 
{
	CWProtocolMessage *msgElemsBinding = NULL;
	int msgElemBindingCount=0;
	CWProtocolMessage *msgElems = NULL;
	int msgElemCount=0;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Clear Configuration Request...");
	
	
	if(!(CWAssembleMessage(messagesPtr, 
						   fragmentsNumPtr, 
						   PMTU, 
						   seqNum,
						   CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_REQUEST, 
						   msgElems, 
						   msgElemCount, 
						   msgElemsBinding, 
						   msgElemBindingCount, 
#ifdef CW_NO_DTLS
						   CW_PACKET_PLAIN)))
#else
						   CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;

	CWLog("Clear Configuration Request Assembled");
	
	return CW_TRUE;
}


CWBool CWAssembleWLANConfigurationRequest(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum,unsigned char* recv_packet,int Operation, int len_packet) {
	
	CWProtocolMessage *msgElemsBinding = NULL;
	int msgElemBindingCount=0;
	CWProtocolMessage *msgElems = NULL;
	int msgElemCount=1;
	int k = -1;
	
		
	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling WLAN 802.11 Configuration Request...");
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, msgElemCount, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	// Assemble Message Elements
	
	if( Operation==CW_MSG_ELEMENT_IEEE80211_ADD_WLAN_CW_TYPE ){
		if (!(CWAssembleMsgElemAddWLAN(0,&(msgElems[++k]), recv_packet, len_packet)))   //radioID = 0 -valore predefinito-
		{
			
			CWErrorHandleLast();
			int i;
			for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
			CW_FREE_OBJECT(msgElems);
			return CW_FALSE; // error will be handled by the caller
		}
		
	}else if( Operation==CW_MSG_ELEMENT_IEEE80211_DELETE_WLAN_CW_TYPE ){
		
		if (!(CWAssembleMsgElemDeleteWLAN(0,&(msgElems[++k]),recv_packet, len_packet)))   //radioID = 0 -valore predefinito-
		{
			CWErrorHandleLast();
			int i;
			for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
			CW_FREE_OBJECT(msgElems);
			return CW_FALSE; // error will be handled by the caller
		}
	}
	

/*  to be implemented in a case of Binding with appropriate messages elements -- see draft capwap-spec && capwap-binding 
	if(!CWBindingAssembleConfigurationUpdateRequest(&msgElemsBinding, &msgElemBindingCount)){
		return CW_FALSE;
	}
*/
	if(!(CWAssembleMessage(messagesPtr, 
	                       fragmentsNumPtr, 
	                       PMTU, 
	                       seqNum,
	                       CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_REQUEST, 
	                       msgElems, 
	                       msgElemCount, 
	                       msgElemsBinding, 
	                       msgElemBindingCount, 
#ifdef CW_NO_DTLS
						   CW_PACKET_PLAIN)))
#else
						   CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;
	
	CWLog("Station WLAN 802.11 Configuration Request Assembled");
	
	return CW_TRUE;
}


CWBool CWAssembleStationConfigurationRequest(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum,unsigned char* StationMacAddr,int Operation) 
{
	
	CWProtocolMessage *msgElemsBinding = NULL;
	int msgElemBindingCount=0;
	CWProtocolMessage *msgElems = NULL;
	int msgElemCount=1;
	int k = -1;
	
		
	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Station Configuration Request...");
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, msgElemCount, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	// Assemble Message Elements
	if( Operation==CW_MSG_ELEMENT_ADD_STATION_CW_TYPE ){
		if (!(CWAssembleMsgElemAddStation(0,&(msgElems[++k]),StationMacAddr)))   //radioID = 0 -valore predefinito-
		{
			CWErrorHandleLast();
			int i;
			for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
			CW_FREE_OBJECT(msgElems);
			return CW_FALSE; // error will be handled by the caller
		}
		
	}else if( Operation==CW_MSG_ELEMENT_DELETE_STATION_CW_TYPE ){
		if (!(CWAssembleMsgElemDeleteStation(0,&(msgElems[++k]),StationMacAddr)))   //radioID = 0 -valore predefinito-
		{
			CWErrorHandleLast();
			int i;
			for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
			CW_FREE_OBJECT(msgElems);
			return CW_FALSE; // error will be handled by the caller
		}
	}
	

/*  to be implemented in a case of Binding with appropriate messages elements -- see draft capwap-spec && capwap-binding 
	if(!CWBindingAssembleConfigurationUpdateRequest(&msgElemsBinding, &msgElemBindingCount)){
		return CW_FALSE;
	}
*/
	if(!(CWAssembleMessage(messagesPtr, 
	                       fragmentsNumPtr, 
	                       PMTU, 
	                       seqNum,
	                       CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_REQUEST, 
	                       msgElems, 
	                       msgElemCount, 
	                       msgElemsBinding, 
	                       msgElemBindingCount, 
#ifdef CW_NO_DTLS
						   CW_PACKET_PLAIN)))
#else
						   CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;

	CWLog("Station Configuration Request Assembled");
	
	return CW_TRUE;
}

CWBool CWStartNeighborDeadTimer(int WTPIndex) {

	/* start NeighborDeadInterval timer */
	if(!CWErr(CWTimerRequest(gCWNeighborDeadInterval,
				 &(gWTPs[WTPIndex].thread),
				 &(gWTPs[WTPIndex].currentTimer),
				 CW_CRITICAL_TIMER_EXPIRED_SIGNAL))) {
		return CW_FALSE;
	}
	return CW_TRUE;
}

CWBool CWStartNeighborDeadTimerForEcho(int WTPIndex){
	
	int echoInterval;

	/* start NeighborDeadInterval timer */
	CWACGetEchoRequestTimer(&echoInterval);
	if(!CWErr(CWTimerRequest(echoInterval,
				 &(gWTPs[WTPIndex].thread),
				 &(gWTPs[WTPIndex].currentTimer),
				 CW_CRITICAL_TIMER_EXPIRED_SIGNAL))) {
		return CW_FALSE;
	}
	return CW_TRUE;
}

CWBool CWStopNeighborDeadTimer(int WTPIndex) {

	if(!CWTimerCancel(&(gWTPs[WTPIndex].currentTimer))) {
	
		return CW_FALSE;
	}
	return CW_TRUE;
}

CWBool CWRestartNeighborDeadTimer(int WTPIndex) {
	
	CWThreadSetSignals(SIG_BLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);	
	
	if(!CWStopNeighborDeadTimer(WTPIndex)) return CW_FALSE;
	if(!CWStartNeighborDeadTimer(WTPIndex)) return CW_FALSE;
	
	CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
	
	CWDebugLog("NeighborDeadTimer restarted");
	return CW_TRUE;
}

CWBool CWRestartNeighborDeadTimerForEcho(int WTPIndex) {
	
	CWThreadSetSignals(SIG_BLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);

	if(!CWStopNeighborDeadTimer(WTPIndex)) return CW_FALSE;
	if(!CWStartNeighborDeadTimerForEcho(WTPIndex)) return CW_FALSE;

	CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
	
	CWDebugLog("NeighborDeadTimer restarted for Echo interval");
	return CW_TRUE;
}
