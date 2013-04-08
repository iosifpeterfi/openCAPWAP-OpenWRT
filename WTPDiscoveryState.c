/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
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
 * Author :  Ludovico Rossi (ludo@bluepixysw.com)                                          *  
 *           Del Moro Andrea (andrea_delmoro@libero.it)                                    *
 *           Giovannini Federica (giovannini.federica@gmail.com)                           *
 *           Massimo Vellucci (m.vellucci@unicampus.it)                                    *
 *           Mauro Bisson (mauro.bis@gmail.com)                                            *
 *******************************************************************************************/


#include "CWWTP.h"
 
#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

/*________________________________________________________________*/
/*  *******************___CAPWAP VARIABLES___*******************  */
int gCWMaxDiscoveries = 10;

/*_________________________________________________________*/
/*  *******************___VARIABLES___*******************  */
int gCWDiscoveryCount;

#ifdef CW_DEBUGGING
	int gCWDiscoveryInterval = 3; //5;
	int gCWMaxDiscoveryInterval = 4; //20;
#else
	int gCWDiscoveryInterval = 5;
	int gCWMaxDiscoveryInterval = 20;
#endif

/*_____________________________________________________*/
/*  *******************___MACRO___*******************  */
#define CWWTPFoundAnAC()	(gACInfoPtr != NULL /*&& gACInfoPtr->preferredAddress.ss_family != AF_UNSPEC*/)

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */
CWBool CWReceiveDiscoveryResponse();
void CWWTPEvaluateAC(CWACInfoValues *ACInfoPtr);
CWBool CWReadResponses();
CWBool CWAssembleDiscoveryRequest(CWProtocolMessage **messagesPtr, int seqNum);
CWBool CWParseDiscoveryResponseMessage(char *msg,
				       int len,
				       int *seqNumPtr,
				       CWACInfoValues *ACInfoPtr);

/*_________________________________________________________*/
/*  *******************___FUNCTIONS___*******************  */

/* 
 * Manage Discovery State
 */
CWStateTransition CWWTPEnterDiscovery() {
	int i;
	CWBool j;	

	CWLog("\n");	
	CWLog("######### Discovery State #########");
	
	/* reset Discovery state */
	gCWDiscoveryCount = 0;
	CWNetworkCloseSocket(gWTPSocket);
	if(!CWErr(CWNetworkInitSocketClient(&gWTPSocket, NULL))) {
		return CW_QUIT;
	}

	/* 
	 * note: gCWACList can be freed and reallocated (reading from config file)
	 * at each transition to the discovery state to save memory space
	 */
	for(i = 0; i < gCWACCount; i++) 
		gCWACList[i].received = CW_FALSE;

	/* wait a random time */
	sleep(CWRandomIntInRange(gCWDiscoveryInterval, gCWMaxDiscoveryInterval));

	CW_REPEAT_FOREVER {
		CWBool sentSomething = CW_FALSE;
	
		/* we get no responses for a very long time */
		if(gCWDiscoveryCount == gCWMaxDiscoveries)
			return CW_ENTER_SULKING;

		/* send Requests to one or more ACs */
		for(i = 0; i < gCWACCount; i++) {

			/* if this AC hasn't responded to us... */
			if(!(gCWACList[i].received)) {
				/* ...send a Discovery Request */

				CWProtocolMessage *msgPtr = NULL;
				
				/* get sequence number (and increase it) */
				gCWACList[i].seqNum = CWGetSeqNum();
				
				if(!CWErr(CWAssembleDiscoveryRequest(&msgPtr,
								     gCWACList[i].seqNum))) {
					exit(1);
				}
				
                                CW_CREATE_OBJECT_ERR(gACInfoPtr, 
						     CWACInfoValues,
						     return CW_QUIT;);
				
				CWNetworkGetAddressForHost(gCWACList[i].address, 
							   &(gACInfoPtr->preferredAddress));
				
				CWUseSockNtop(&(gACInfoPtr->preferredAddress),
						CWDebugLog(str););
				
				j = CWErr(CWNetworkSendUnsafeUnconnected(gWTPSocket,
									 &(gACInfoPtr->preferredAddress),
									 (*msgPtr).msg,
									 (*msgPtr).offset)); 
				/* 
				 * log eventual error and continue
				 * CWUseSockNtop(&(gACInfoPtr->preferredAddress),
				 * 		 CWLog("WTP sends Discovery Request to: %s", str););
				 */
								
				CW_FREE_PROTOCOL_MESSAGE(*msgPtr);
				CW_FREE_OBJECT(msgPtr);
				CW_FREE_OBJECT(gACInfoPtr);
				
				/*
				 * we sent at least one Request in this loop
				 * (even if we got an error sending it) 
				 */
				sentSomething = CW_TRUE; 
			}
		}
		
		/* All AC sent the response (so we didn't send any request) */
		if(!sentSomething && CWWTPFoundAnAC()) break;
		
		gCWDiscoveryCount++;

		/* wait for Responses */
		if(CWErr(CWReadResponses()) && CWWTPFoundAnAC()) {
			/* we read at least one valid Discovery Response */
			break;
		}
		
		CWLog("WTP Discovery-To-Discovery (%d)", gCWDiscoveryCount);
	}
	
	CWLog("WTP Picks an AC");
	
	/* crit error: we should have received at least one Discovery Response */
	if(!CWWTPFoundAnAC()) {
		CWLog("No Discovery response Received");
		return CW_ENTER_DISCOVERY;
	}
	
	/* if the AC is multi homed, we select our favorite AC's interface */
	CWWTPPickACInterface();
		
	CWUseSockNtop(&(gACInfoPtr->preferredAddress),
			CWLog("Preferred AC: \"%s\", at address: %s", gACInfoPtr->name, str););
	
	return CW_ENTER_JOIN;
}

/* 
 * Wait DiscoveryInterval time while receiving Discovery Responses.
 */
CWBool CWReadResponses() {

	CWBool result = CW_FALSE;
	
	struct timeval timeout, before, after, delta, newTimeout;
	
	timeout.tv_sec = newTimeout.tv_sec = gCWDiscoveryInterval;
	timeout.tv_usec = newTimeout.tv_usec = 0;
	
	gettimeofday(&before, NULL);

	CW_REPEAT_FOREVER {
		/* check if something is available to read until newTimeout */
		if(CWNetworkTimedPollRead(gWTPSocket, &newTimeout)) { 
			/* success
			 * if there was no error, raise a "success error", so we can easily handle
			 * all the cases in the switch
			 */
			CWErrorRaise(CW_ERROR_SUCCESS, NULL);
		}

		switch(CWErrorGetLastErrorCode()) {
			case CW_ERROR_TIME_EXPIRED:
				goto cw_time_over;
				break;
				
			case CW_ERROR_SUCCESS:
				result = CWReceiveDiscoveryResponse();
			case CW_ERROR_INTERRUPTED: 
				/*
				 * something to read OR interrupted by the system
				 * wait for the remaining time (NetworkPoll will be recalled with the remaining time)
				 */
				gettimeofday(&after, NULL);

				CWTimevalSubtract(&delta, &after, &before);
				if(CWTimevalSubtract(&newTimeout, &timeout, &delta) == 1) { 
					/* negative delta: time is over */
					goto cw_time_over;
				}
				break;
			default:
				CWErrorHandleLast();
				goto cw_error;
				break;	
		}
	}
	cw_time_over:
		/* time is over */
		CWDebugLog("Timer expired during receive");	
	cw_error:
		return result;
}

/*
 * Gets a datagram from network that should be a Discovery Response.
 */
CWBool CWReceiveDiscoveryResponse() {
	char buf[CW_BUFFER_SIZE];
	int i;
	CWNetworkLev4Address addr;
	CWACInfoValues *ACInfoPtr;
	int seqNum;
	int readBytes;
	
	/* receive the datagram */
	if(!CWErr(CWNetworkReceiveUnsafe(gWTPSocket,
					 buf,
					 CW_BUFFER_SIZE-1,
					 0,
					 &addr,
					 &readBytes))) {
		return CW_FALSE;
	}
	
        CW_CREATE_OBJECT_ERR(ACInfoPtr,
			     CWACInfoValues,
			     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	/* check if it is a valid Discovery Response */
	if(!CWErr(CWParseDiscoveryResponseMessage(buf, readBytes, &seqNum, ACInfoPtr))) {

		CW_FREE_OBJECT(ACInfoPtr);
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Received something different from a\
				     Discovery Response while in Discovery State");
	}

	CW_COPY_NET_ADDR_PTR(&(ACInfoPtr->incomingAddress), &(addr));

	/* see if this AC is better than the one we have stored */
	CWWTPEvaluateAC(ACInfoPtr);

	CWLog("WTP Receives Discovery Response");

	/* check if the sequence number we got is correct */
	for(i = 0; i < gCWACCount; i++) {
	
		if(gCWACList[i].seqNum == seqNum) {
		
			CWUseSockNtop(&addr,
				      CWLog("Discovery Response from:%s", str););
			/* we received response from this address */
			gCWACList[i].received = CW_TRUE;
	
			return CW_TRUE;
		}
	}
	
	return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
			    "Sequence Number of Response doesn't macth Request");
}


void CWWTPEvaluateAC(CWACInfoValues *ACInfoPtr) {

	if(ACInfoPtr == NULL) return;
	
	if(gACInfoPtr == NULL) { 
		/* 
		 * this is the first AC we evaluate: so
		 *  it's the best AC we examined so far
		 */
		gACInfoPtr = ACInfoPtr;

	} else {
		
		CW_FREE_OBJECT(ACInfoPtr);
	}
	/* 
	 * ... note: we can add our favourite algorithm to pick the best AC.
	 * We can also consider to remember all the Discovery Responses we 
	 * received and not just the best.
	 */
}

/*
 * Pick one interface of the AC (easy if there is just one interface). The 
 * current algorithm just pick the Ac with less WTP communicating with it. If
 * the addresses returned by the AC in the Discovery Response don't include the
 * address of the sender of the Discovery Response, we ignore the address in 
 * the Response and use the one of the sender (maybe the AC sees garbage 
 * address, i.e. it is behind a NAT).
 */
void CWWTPPickACInterface() {
	int i, min;
	CWBool foundIncoming = CW_FALSE;
	if(gACInfoPtr == NULL) return;
	
	gACInfoPtr->preferredAddress.ss_family = AF_UNSPEC;
	
	if(gNetworkPreferredFamily == CW_IPv6) {
		goto cw_pick_IPv6;
	}
	
cw_pick_IPv4:
	if(gACInfoPtr->IPv4Addresses == NULL || gACInfoPtr->IPv4AddressesCount <= 0) return;
		
	min = gACInfoPtr->IPv4Addresses[0].WTPCount;

	CW_COPY_NET_ADDR_PTR(&(gACInfoPtr->preferredAddress),
			     &(gACInfoPtr->IPv4Addresses[0].addr));
		
	for(i = 1; i < gACInfoPtr->IPv4AddressesCount; i++) {

		if(!sock_cmp_addr((struct sockaddr*)&(gACInfoPtr->IPv4Addresses[i]),
				  (struct sockaddr*)&(gACInfoPtr->incomingAddress),
				  sizeof(struct sockaddr_in))) foundIncoming = CW_TRUE;

		if(gACInfoPtr->IPv4Addresses[i].WTPCount < min) {

			min = gACInfoPtr->IPv4Addresses[i].WTPCount;
			CW_COPY_NET_ADDR_PTR(&(gACInfoPtr->preferredAddress), 
					     &(gACInfoPtr->IPv4Addresses[i].addr));
		}
	}
		
	if(!foundIncoming) {
		/* 
		 * If the addresses returned by the AC in the Discovery
		 * Response don't include the address of the sender of the
		 * Discovery Response, we ignore the address in the Response
		 * and use the one of the sender (maybe the AC sees garbage
		 * address, i.e. it is behind a NAT).
		 */
		CW_COPY_NET_ADDR_PTR(&(gACInfoPtr->preferredAddress),
				     &(gACInfoPtr->incomingAddress));
	}
	return;
		
cw_pick_IPv6:
	/* CWDebugLog("Pick IPv6"); */
	if(gACInfoPtr->IPv6Addresses == NULL ||\
	   gACInfoPtr->IPv6AddressesCount <= 0) goto cw_pick_IPv4;
		
	min = gACInfoPtr->IPv6Addresses[0].WTPCount;
	CW_COPY_NET_ADDR_PTR(&(gACInfoPtr->preferredAddress),
			     &(gACInfoPtr->IPv6Addresses[0].addr));
		
	for(i = 1; i < gACInfoPtr->IPv6AddressesCount; i++) {

		/*
		 * if(!sock_cmp_addr(&(gACInfoPtr->IPv6Addresses[i]),
		 * 		     &(gACInfoPtr->incomingAddress),
		 * 		     sizeof(struct sockaddr_in6))) 
		 *
		 * 	foundIncoming = CW_TRUE;
		 */
			
		if(gACInfoPtr->IPv6Addresses[i].WTPCount < min) {
			min = gACInfoPtr->IPv6Addresses[i].WTPCount;
			CW_COPY_NET_ADDR_PTR(&(gACInfoPtr->preferredAddress),
					     &(gACInfoPtr->IPv6Addresses[i].addr));
		}
	}
	/*
	if(!foundIncoming) {
		CW_COPY_NET_ADDR_PTR(&(gACInfoPtr->preferredAddress), 
				     &(gACInfoPtr->incomingAddress));
	}
	*/
	return;
}

CWBool CWAssembleDiscoveryRequest(CWProtocolMessage **messagesPtr, int seqNum) {

	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount = 6;
	CWProtocolMessage *msgElemsBinding= NULL;
	const int msgElemBindingCount=0;
	int k = -1;
	int fragmentsNum;

	if(messagesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, msgElemCount, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
	
	/* Assemble Message Elements */
	
	if(
	   (!(CWAssembleMsgElemDiscoveryType(&(msgElems[++k])))) ||
	   (!(CWAssembleMsgElemWTPBoardData(&(msgElems[++k]))))	 ||
	   (!(CWAssembleMsgElemWTPDescriptor(&(msgElems[++k])))) ||
	   (!(CWAssembleMsgElemWTPFrameTunnelMode(&(msgElems[++k])))) ||
	   (!(CWAssembleMsgElemWTPMACType(&(msgElems[++k]))))  ||
	   (!(CWAssembleMsgElemWTPRadioInformation(&(msgElems[++k]))))
	){
		int i;
		for(i = 0; i <= k; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
		CW_FREE_OBJECT(msgElems);
		/* error will be handled by the caller */
		return CW_FALSE;
	}
	
	return CWAssembleMessage(messagesPtr, 
				 &fragmentsNum,
				 0,
				 seqNum,
				 CW_MSG_TYPE_VALUE_DISCOVERY_REQUEST,
				 msgElems,
				 msgElemCount,
				 msgElemsBinding,
				 msgElemBindingCount,
				 CW_PACKET_PLAIN);
}

/*
 *  Parse Discovery Response and return informations in *ACInfoPtr.
 */
CWBool CWParseDiscoveryResponseMessage(char *msg, 
				       int len,
				       int *seqNumPtr,
				       CWACInfoValues *ACInfoPtr) {


	CWControlHeaderValues controlVal;
	CWProtocolTransportHeaderValues transportVal;
	int offsetTillMessages, i, j;
	char tmp_ABGNTypes;
	CWProtocolMessage completeMsg;
	
	if(msg == NULL || seqNumPtr == NULL || ACInfoPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Parse Discovery Response");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;
	
	CWBool dataFlag = CW_FALSE;
	/* will be handled by the caller */
	if(!(CWParseTransportHeader(&completeMsg, &transportVal, &dataFlag, NULL))) return CW_FALSE; 
	/* will be handled by the caller */
	if(!(CWParseControlHeader(&completeMsg, &controlVal))) return CW_FALSE;
	
	/* different type */
	if(controlVal.messageTypeValue != CW_MSG_TYPE_VALUE_DISCOVERY_RESPONSE)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message is not Discovery Response as Expected");
	
	
	*seqNumPtr = controlVal.seqNum;
	
	/* skip timestamp */
	controlVal.msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;

	offsetTillMessages = completeMsg.offset;
	
	ACInfoPtr->IPv4AddressesCount = 0;
	ACInfoPtr->IPv6AddressesCount = 0;
	/* parse message elements */
	while((completeMsg.offset-offsetTillMessages) < controlVal.msgElemsLen) {
		unsigned short int type=0;	/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int len=0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(&completeMsg,&type,&len);
		CWDebugLog("Parsing Message Element: %u, len: %u", type, len);
		
		switch(type) {
			case CW_MSG_ELEMENT_AC_DESCRIPTOR_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseACDescriptor(&completeMsg, len, ACInfoPtr))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseWTPRadioInformation_FromAC(&completeMsg, len, &tmp_ABGNTypes))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_AC_NAME_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseACName(&completeMsg, len, &(ACInfoPtr->name)))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_CW_CONTROL_IPV4_ADDRESS_CW_TYPE:
				/* 
				 * just count how many interfacess we have, 
				 * so we can allocate the array 
				 */
				ACInfoPtr->IPv4AddressesCount++;
				completeMsg.offset += len;
				break;
			case CW_MSG_ELEMENT_CW_CONTROL_IPV6_ADDRESS_CW_TYPE:
				/* 
				 * just count how many interfacess we have, 
				 * so we can allocate the array 
				 */
				ACInfoPtr->IPv6AddressesCount++;
				completeMsg.offset += len;
				break;
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT,
					"Unrecognized Message Element");
		}

		/* CWDebugLog("bytes: %d/%d",
		 * 	      (completeMsg.offset-offsetTillMessages),
		 * 	      controlVal.msgElemsLen); 
		 */
	}
	
	if (completeMsg.offset != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT,
				    "Garbage at the End of the Message");
	
	/* actually read each interface info */
	CW_CREATE_ARRAY_ERR(ACInfoPtr->IPv4Addresses,
			    ACInfoPtr->IPv4AddressesCount,
			    CWProtocolIPv4NetworkInterface,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	if(ACInfoPtr->IPv6AddressesCount > 0) {

		CW_CREATE_ARRAY_ERR(ACInfoPtr->IPv6Addresses,
				    ACInfoPtr->IPv6AddressesCount,
				    CWProtocolIPv6NetworkInterface,
				    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	}

	i = 0, j = 0;
	
	completeMsg.offset = offsetTillMessages;
	while((completeMsg.offset-offsetTillMessages) < controlVal.msgElemsLen) {

		unsigned short int type=0;	/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int len=0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(&completeMsg,&type,&len);		
		
		switch(type) {
			case CW_MSG_ELEMENT_CW_CONTROL_IPV4_ADDRESS_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseCWControlIPv4Addresses(&completeMsg,
								   len,
								   &(ACInfoPtr->IPv4Addresses[i]))))
					return CW_FALSE; 
				i++;
				break;
			case CW_MSG_ELEMENT_CW_CONTROL_IPV6_ADDRESS_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseCWControlIPv6Addresses(&completeMsg,
								   len,
								   &(ACInfoPtr->IPv6Addresses[j])))) 
					return CW_FALSE;				
				j++;
				break;
			default:
				completeMsg.offset += len;
				break;
		}
	}
	return CW_TRUE;
}

