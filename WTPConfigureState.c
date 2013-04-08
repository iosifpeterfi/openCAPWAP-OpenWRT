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

/* void CWWTPResponseTimerExpired(void *arg, CWTimerID id); */
CWBool CWAssembleConfigureRequest(CWProtocolMessage **messagesPtr,
				  int *fragmentsNumPtr,
				  int PMTU,
				  int seqNum,
				  CWList msgElemList);

CWBool CWParseConfigureResponseMessage(char *msg,
				       int len,
				       int seqNum,
				       CWProtocolConfigureResponseValues *valuesPtr);

CWBool CWSaveConfigureResponseMessage(CWProtocolConfigureResponseValues *configureResponse);

/*_________________________________________________________*/
/*  *******************___FUNCTIONS___*******************  */

/* 
 * Manage Configure State.
 */
CWStateTransition CWWTPEnterConfigure() {

	int seqNum;
	CWProtocolConfigureResponseValues values;
	
	CWLog("\n");
	CWLog("######### Configure State #########");
	
	/* send Configure Request */
	seqNum = CWGetSeqNum();
	
	if(!CWErr(CWWTPSendAcknowledgedPacket(seqNum,
					      NULL,
					      CWAssembleConfigureRequest,
					      (void*)CWParseConfigureResponseMessage,
					      (void*)CWSaveConfigureResponseMessage, &values))) {

		CWNetworkCloseSocket(gWTPSocket);
#ifndef CW_NO_DTLS
		CWSecurityDestroySession(gWTPSession);
		CWSecurityDestroyContext(gWTPSecurityContext);
		gWTPSecurityContext = NULL;
		gWTPSession = NULL;
#endif
		return CW_QUIT;
	}
	
	return CW_ENTER_DATA_CHECK;
}

/*
void CWWTPResponseTimerExpired(void *arg, CWTimerID id) 
{
	CWLog("WTP Response Configure Timer Expired");
	CWNetworkCloseSocket(gWTPSocket);
}
*/

/* 
 * Send Configure Request on the active session.
 */
CWBool CWAssembleConfigureRequest(CWProtocolMessage **messagesPtr,
				  int *fragmentsNumPtr,
				  int PMTU,
				  int seqNum,
				  CWList msgElemList) {

	CWProtocolMessage 	*msgElems= NULL;
	CWProtocolMessage 	*msgElemsBinding= NULL;
	const int 		msgElemCount = 8;
	const int 		msgElemBindingCount=0;
	int k = -1;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, 
					 msgElemCount,
					 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
		
	CWDebugLog("Assembling Configure Request...");
	
	/* Assemble Message Elements */
	if ((!(CWAssembleMsgElemACName(&(msgElems[++k]))))          ||
	    (!(CWAssembleMsgElemACNameWithIndex(&(msgElems[++k])))) ||
	    (!(CWAssembleMsgElemRadioAdminState(&(msgElems[++k])))) ||
	    (!(CWAssembleMsgElemStatisticsTimer(&(msgElems[++k])))) ||
	    (!(CWAssembleMsgElemWTPRebootStatistics(&(msgElems[++k])))) ||
	    (!(CWAssembleMsgElemWTPRadioInformation(&(msgElems[++k])))) ||
	    (!(CWAssembleMsgElemSupportedRates(&(msgElems[++k])))) ||
	    (!(CWAssembleMsgElemMultiDomainCapability(&(msgElems[++k]))))   )
	{
		int i;
		for(i = 0; i <= k; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
		CW_FREE_OBJECT(msgElems);
		/* error will be handled by the caller */
		return CW_FALSE;
	}  	
	
	if (!(CWAssembleMessage(messagesPtr, 
				fragmentsNumPtr,
				PMTU,
				seqNum,
				CW_MSG_TYPE_VALUE_CONFIGURE_REQUEST,
				msgElems,
				msgElemCount,
				msgElemsBinding,
				msgElemBindingCount,
#ifdef CW_NO_DTLS
				CW_PACKET_PLAIN
#else				
				CW_PACKET_CRYPT
#endif				
				)))
	 	return CW_FALSE;
	
	CWDebugLog("Configure Request Assembled");	 
	return CW_TRUE;
}

CWBool CWParseConfigureResponseMessage (char *msg,
					int len,
					int seqNum,
					CWProtocolConfigureResponseValues *valuesPtr) {

	CWControlHeaderValues 	controlVal;
	CWProtocolMessage 	completeMsg;
	CWBool 			bindingMsgElemFound = CW_FALSE;
	int 			offsetTillMessages;
	int 			i=0;
	int 			j=0;
	
	if(msg == NULL || valuesPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Parsing Configure Response...");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;
	
	valuesPtr->echoRequestTimer=0;
	valuesPtr->radioOperationalInfoCount=0;
	valuesPtr->radiosDecryptErrorPeriod.radiosCount=0;
	valuesPtr->bindingValues = NULL;
	valuesPtr->ACIPv4ListInfo.ACIPv4ListCount=0;
	valuesPtr->ACIPv4ListInfo.ACIPv4List=NULL;
	valuesPtr->ACIPv6ListInfo.ACIPv6ListCount=0;
	valuesPtr->ACIPv6ListInfo.ACIPv6List=NULL;
	
	/* error will be handled by the caller */
	if(!(CWParseControlHeader(&completeMsg, &controlVal))) return CW_FALSE;

	/* different type */
	if (controlVal.messageTypeValue != CW_MSG_TYPE_VALUE_CONFIGURE_RESPONSE)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Message is not Configure Response as Expected");
	
	if (controlVal.seqNum != seqNum) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Different Sequence Number");
	
	/* skip timestamp */
	controlVal.msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;
	
	offsetTillMessages = completeMsg.offset;	
	
	/* parse message elements */
	while((completeMsg.offset-offsetTillMessages) < controlVal.msgElemsLen) {
		unsigned short int type=0;	/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int len=0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(&completeMsg,&type,&len);
		/* CWDebugLog("Parsing Message Element: %u, len: %u", type, len); */
	
		if(CWBindingCheckType(type))
		{
			bindingMsgElemFound=CW_TRUE;
			completeMsg.offset += len;
			continue;	
		}

		switch(type) {
			case CW_MSG_ELEMENT_AC_IPV4_LIST_CW_TYPE:
				if(!(CWParseACIPv4List(&completeMsg, len, &(valuesPtr->ACIPv4ListInfo)))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_AC_IPV6_LIST_CW_TYPE:
				if(!(CWParseACIPv6List(&completeMsg, len, &(valuesPtr->ACIPv6ListInfo)))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_CW_TIMERS_CW_TYPE:
				if(!(CWParseCWTimers(&completeMsg, len, valuesPtr))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE:
				/* 
				 * just count how many radios we have, so we
				 * can allocate the array 
				 */
				valuesPtr->radioOperationalInfoCount++; 
				completeMsg.offset += len;
				break;
			case CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_PERIOD_CW_TYPE:
				valuesPtr->radiosDecryptErrorPeriod.radiosCount++;
				completeMsg.offset += len;
				break;
			case CW_MSG_ELEMENT_IDLE_TIMEOUT_CW_TYPE:
				if(!(CWParseIdleTimeout(&completeMsg, len, valuesPtr))) return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_FALLBACK_CW_TYPE:
				if(!(CWParseWTPFallback(&completeMsg, len, valuesPtr))) return CW_FALSE;
				break;
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT,
					"Unrecognized Message Element");
		}
	}
	
	if(completeMsg.offset != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT,
				    "Garbage at the End of the Message");
	
	CW_CREATE_ARRAY_ERR((*valuesPtr).radioOperationalInfo,
			    (*valuesPtr).radioOperationalInfoCount,
			    CWRadioOperationalInfoValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
	CW_CREATE_ARRAY_ERR((*valuesPtr).radiosDecryptErrorPeriod.radios,
			    (*valuesPtr).radiosDecryptErrorPeriod.radiosCount,
			    WTPDecryptErrorReportValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	completeMsg.offset = offsetTillMessages;

	while(completeMsg.offset-offsetTillMessages < controlVal.msgElemsLen) {
		unsigned short int type=0;	/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int len=0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(&completeMsg,&type,&len);
		
		switch(type) {
			case CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseWTPRadioOperationalState(&completeMsg, 
								     len,
								     &(valuesPtr->radioOperationalInfo[i])))) 
					return CW_FALSE;
				i++;
				break;
			
			case CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_PERIOD_CW_TYPE:	
				if(!(CWParseDecryptErrorReportPeriod(&completeMsg,
								     len,
								     &(valuesPtr->radiosDecryptErrorPeriod.radios[j])))) 
					return CW_FALSE;
				j++;
				break;
			default:
				completeMsg.offset += len;
				break;
		}
	}
	
	if(bindingMsgElemFound){
		if(!CWBindingParseConfigureResponse(msg+offsetTillMessages,
						    len-offsetTillMessages,
						    &(valuesPtr->bindingValues))) {
			return CW_FALSE;
		}	
	}
	
	CWDebugLog("Configure Response Parsed");
	return CW_TRUE;
}

CWBool CWSaveConfigureResponseMessage(CWProtocolConfigureResponseValues *configureResponse) {

	if(configureResponse == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if(gACInfoPtr == NULL) return CWErrorRaise(CW_ERROR_NEED_RESOURCE, NULL);
	
	CWDebugLog("Saving Configure Response...");
	CWDebugLog("###A");
	CWDebugLog("###Count:%d", (configureResponse->ACIPv4ListInfo).ACIPv4ListCount);
	if((gACInfoPtr->ACIPv4ListInfo).ACIPv4List==NULL) {
		
		CWDebugLog("###NULL");
	}

	if((configureResponse->ACIPv4ListInfo).ACIPv4ListCount > 0) {

		CW_FREE_OBJECT((gACInfoPtr->ACIPv4ListInfo).ACIPv4List);
		(gACInfoPtr->ACIPv4ListInfo).ACIPv4ListCount = (configureResponse->ACIPv4ListInfo).ACIPv4ListCount;
		(gACInfoPtr->ACIPv4ListInfo).ACIPv4List = (configureResponse->ACIPv4ListInfo).ACIPv4List;
	}
	
	CWDebugLog("###B");
	
	if((configureResponse->ACIPv6ListInfo).ACIPv6ListCount > 0) {

		CW_FREE_OBJECT((gACInfoPtr->ACIPv6ListInfo).ACIPv6List);
		(gACInfoPtr->ACIPv6ListInfo).ACIPv6ListCount = (configureResponse->ACIPv6ListInfo).ACIPv6ListCount;
		(gACInfoPtr->ACIPv6ListInfo).ACIPv6List = (configureResponse->ACIPv6ListInfo).ACIPv6List;	
	}

	if(configureResponse->bindingValues != NULL) {

		CWProtocolResultCode resultCode;

		if(!CWBindingSaveConfigureResponse(configureResponse->bindingValues, &resultCode)) {

			CW_FREE_OBJECT(configureResponse->bindingValues);	
			return CW_FALSE;
		}	
	}

	if(configureResponse->echoRequestTimer > 0) {
		
		gEchoInterval=configureResponse->echoRequestTimer;
	}

	/*
	""need to be added""
	
	int discoveryTimer;
	int radioOperationalInfoCount;
	CWRadioOperationalInfoValues *radioOperationalInfo;
	WTPDecryptErrorReport radiosDecryptErrorPeriod;
	int idleTimeout;
	int fallback;
	*/

 	/*
         * It is not clear to me what the original developers intended to
         * accomplish. One thing's for sure: radioOperationalInfo, radiosDecryptErrorPeriod.radios,
         * and bidingValues get allocated and are never freed, 
         * so we do it here...
         * 
         * BUGs ML02-ML04-ML05
         * 16/10/2009 - Donato Capitella
         */
        CW_FREE_OBJECT(configureResponse->radioOperationalInfo);
        CW_FREE_OBJECT(configureResponse->radiosDecryptErrorPeriod.radios);
        CW_FREE_OBJECT(configureResponse->bindingValues);

	CWDebugLog("Configure Response Saved");
	return CW_TRUE;	
}

