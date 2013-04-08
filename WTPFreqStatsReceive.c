/*******************************************************************************************
 * Copyright (c) 2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica   *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A         *
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
 * Author : Antonio Davoli (antonio.davoli@gmail.com)                                      *
 *                                            				                    		   *
 *******************************************************************************************/

#include "WTPFreqStatsReceive.h"

CW_THREAD_RETURN_TYPE CWWTPReceiveFreqStats(void *arg)
{
	int recSock, rlen, k, fragmentsNum = 0;
	
	struct sockaddr_in servaddr, client_addr;
	socklen_t slen = sizeof(client_addr);
	
	char buffer[PACKET_SIZE];
	
	CWProtocolMessage *completeMsgPtr = NULL;
	CWProtocolMessage *data = NULL;
	CWBindingTransportHeaderValues *bindingValuesPtr = NULL;
	
	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);
	
	/* Create an Inet UDP socket for this thread (Receive freq/ack packets) */
    
	if ( ( recSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		CWDebugLog("Thread Frequency Receive Stats: Error creating socket");
		CWExitThread();
    }
	
	/*  Set up address structure for server socket */
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); 
	servaddr.sin_port = htons(SERVER_PORT);
	
	/* Binding Socket */
	
	if ( bind(recSock, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in)) < 0 ) {
		CWDebugLog("Thread Frequency Receive Stats: Binding Socket Error");
		close(recSock);
		CWExitThread();
	} 
			
	CW_REPEAT_FOREVER 	/* Receive data Loop */
	{				
		memset(buffer, 0, PACKET_SIZE);
		fragmentsNum = 0;
		k = 0;
		rlen = 0;
		
		if  ( ( rlen = recvfrom(recSock, buffer, PACKET_SIZE, 0, (struct sockaddr *) &client_addr, &slen) ) > 0 ) 
		{
			/* Creation of stats/ack message for AC */
			
			CW_CREATE_OBJECT_ERR(data, CWProtocolMessage, return 0;);
			CW_CREATE_PROTOCOL_MESSAGE(*data, rlen, return 0;);
	
			memcpy(data->msg, buffer, rlen);
			data->offset=rlen;
			
			/**************************************************************
			 * 2009 Update:                                               *
			 *                                                            *
			 * Frequency Stats Message, like the QoS Data message are     *
			 * encapsuled on Capwap Data Message.                         *
			 * For distinguish the two types of message we use the fields *
			 * of binding dataRate and SNR.                               *
			 * Frequency Stats Message: dataRate=-1 && SNR=1              *
			 * QoS Stats Message      : dataRate=-1                       *
			 * ---------------------------------------------------------- *
			 * For others Info: see  CWBinding.c                          *
			 **************************************************************/
			
			
			/* In this function is tied the name of the socket: recSock */
			CW_CREATE_OBJECT_ERR(bindingValuesPtr, CWBindingTransportHeaderValues, EXIT_THREAD);
			bindingValuesPtr->dataRate = -1;
			bindingValuesPtr->SNR = 1;
			
			/* Capwap Message Assembling */
						
			if ( CWAssembleDataMessage(&completeMsgPtr, &fragmentsNum, gWTPPathMTU, data, bindingValuesPtr,
#ifdef CW_NO_DTLS
									  CW_PACKET_PLAIN
#else			       
									  CW_PACKET_CRYPT
#endif
									  ,0) == CW_TRUE )
			
			{	
				for (k = 0; k < fragmentsNum; k++) {
				
				#ifdef CW_NO_DTLS
					if (!CWNetworkSendUnsafeConnected(gWTPSocket, completeMsgPtr[k].msg, completeMsgPtr[k].offset)) {
#else
					if (!CWSecuritySend(gWTPSession, completeMsgPtr[k].msg, completeMsgPtr[k].offset)) {
#endif
						CWDebugLog("Failure sending Request");
					}		
				}
			}
			
			/* Free used Structures */
			
			for (k = 0; k < fragmentsNum; k++) {
				CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
			}	
			
			CW_FREE_OBJECT(completeMsgPtr);
			CW_FREE_PROTOCOL_MESSAGE(*data);
			CW_FREE_OBJECT(data);
			CW_FREE_OBJECT(bindingValuesPtr);				
		}
		else {
			CWDebugLog("Thread Frequency Receive Stats: Error on recvfrom");
			close(recSock);
		}
	}
}
