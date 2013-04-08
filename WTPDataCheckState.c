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

CWBool CWAssembleChangeStateEventRequest(CWProtocolMessage **messagesPtr,
					 int *fragmentsNumPtr,
					 int PMTU,
					 int seqNum,
					 CWList msgElemList);

CWBool CWParseChangeStateEventResponseMessage(char *msg,
					      int len,
					      int seqNum,
					      void *values);

CWBool CWSaveChangeStateEventResponseMessage(void *changeStateEventResp);

CWStateTransition CWWTPEnterDataCheck() {

	int seqNum;
	
	CWLog("\n");
	CWLog("######### Data Check State #########");
	
	CWLog("\n");
	CWLog("#________ Change State Event (Data Check) ________#");
	
	/* Send Change State Event Request */
	seqNum = CWGetSeqNum();
	
	if(!CWErr(CWStartHeartbeatTimer())) {
		return CW_ENTER_RESET;
	}
	
	if(!CWErr(CWWTPSendAcknowledgedPacket(seqNum, 
					      NULL,
					      CWAssembleChangeStateEventRequest,
					      CWParseChangeStateEventResponseMessage,
					      CWSaveChangeStateEventResponseMessage,
					      NULL))) {
		return CW_ENTER_RESET;
	}

	if(!CWErr(CWStopHeartbeatTimer())) {

		return CW_ENTER_RESET;
	}

	return CW_ENTER_RUN;
}

CWBool CWAssembleChangeStateEventRequest(CWProtocolMessage **messagesPtr,
					 int *fragmentsNumPtr,
					 int PMTU,
					 int seqNum,
					 CWList msgElemList) {

	CWProtocolMessage 	*msgElems= NULL;
	CWProtocolMessage 	*msgElemsBinding= NULL;
	const int		msgElemCount = 2;
	int 			msgElemBindingCount=0;
	int 			resultCode = CW_PROTOCOL_SUCCESS;
	int 			k = -1;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, 
					 msgElemCount,
					 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
		
	CWLog("Assembling Change State Event Request...");	

	/* Assemble Message Elements */
	if (!(CWAssembleMsgElemRadioOperationalState(-1, &(msgElems[++k]))) ||
	    !(CWAssembleMsgElemResultCode(&(msgElems[++k]), resultCode))) {

		int i;
	
		for(i = 0; i <= k; i++) { 
			
			CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);
		}
		CW_FREE_OBJECT(msgElems);
		/* error will be handled by the caller */
		return CW_FALSE;
	}

	if (!(CWAssembleMessage(messagesPtr,
				fragmentsNumPtr,
				PMTU,
				seqNum,
				CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_REQUEST,
				msgElems, msgElemCount,
				msgElemsBinding,
				msgElemBindingCount,
#ifdef CW_NO_DTLS
				CW_PACKET_PLAIN
#else
				CW_PACKET_CRYPT
#endif
				)))
	 	return CW_FALSE;
	
	CWLog("Change State Event Request Assembled");
	return CW_TRUE;
}

CWBool CWParseChangeStateEventResponseMessage(char *msg,
					      int len,
					      int seqNum,
					      void *values) {

	CWControlHeaderValues controlVal;
	CWProtocolMessage completeMsg;
	
	if(msg == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Parsing Change State Event Response...");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;
	
	/* error will be handled by the caller */
	if(!(CWParseControlHeader(&completeMsg, &controlVal))) return CW_FALSE; 
	
	if(controlVal.messageTypeValue != CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_RESPONSE)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Message is not Change State Event Response as Expected");
	
	if(controlVal.seqNum != seqNum) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Different Sequence Number");
	
	/* skip timestamp */
	controlVal.msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;
	
	if(controlVal.msgElemsLen != 0 ) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Change State Event Response must carry no message elements");

	CWLog("Change State Event Response Parsed");
	return CW_TRUE;
}

CWBool CWSaveChangeStateEventResponseMessage (void *changeStateEventResp)
{
	CWDebugLog("Saving Change State Event Response...");
	CWDebugLog("Change State Event Response Saved");
	return CW_TRUE;
}
