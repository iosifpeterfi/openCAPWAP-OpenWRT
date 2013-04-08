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

/*_______________________________________________________________*/
/*  *******************___CHECK FUNCTIONS___*******************  */

CWBool CWWTPCheckForBindingFrame()
{
	//	
	CWLockSafeList(gFrameList);
	
	while (CWGetCountElementFromSafeList(gFrameList) > 0)
	{
		CWBindingDataListElement* dataFirstElem = CWRemoveHeadElementFromSafeList(gFrameList, NULL);
		if (dataFirstElem)
		{
			int k;
			int fragmentsNum = 0;
			CWProtocolMessage *completeMsgPtr = NULL;
			
			if (!CWAssembleDataMessage(&completeMsgPtr, 
						   &fragmentsNum, 
						   gWTPPathMTU, 
						   dataFirstElem->frame, 
						   dataFirstElem->bindingValues,
 			       			   CW_PACKET_PLAIN
						   ,0))			  
			{	
				for(k = 0; k < fragmentsNum; k++)
				{
					CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
				}
				
				CW_FREE_OBJECT(completeMsgPtr);
				CW_FREE_PROTOCOL_MESSAGE(*(dataFirstElem->frame));
				CW_FREE_OBJECT(dataFirstElem->frame);
				CW_FREE_OBJECT(dataFirstElem->bindingValues);
				CW_FREE_OBJECT(dataFirstElem);
				continue;
			}
								
			for (k = 0; k < fragmentsNum; k++) 
			{
				if (!CWNetworkSendUnsafeConnected(gWTPDataSocket, completeMsgPtr[k].msg, completeMsgPtr[k].offset)) {
					CWDebugLog("Failure sending Request");
					break;
				}
				CWDebugLog("Sending binding Request to AC");
			}
			
			for (k = 0; k < fragmentsNum; k++)
			{
				CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
			}
			
			CW_FREE_OBJECT(completeMsgPtr);				
			CW_FREE_PROTOCOL_MESSAGE(*(dataFirstElem->frame));
			CW_FREE_OBJECT(dataFirstElem->frame);
			CW_FREE_OBJECT(dataFirstElem->bindingValues);
			CW_FREE_OBJECT(dataFirstElem);
		}	
	}

	CWUnlockSafeList(gFrameList);	
	
	return CW_TRUE;
}

CWBool CWWTPCheckForWTPEventRequest(){

	CWLog("\n");
	CWLog("#________ WTP Event Request Message (Run) ________#");
	
	/* Send WTP Event Request */
	CWList msgElemList = NULL;
	CWProtocolMessage *messages = NULL;
	int fragmentsNum = 0;
	int seqNum;
	int *pendingReqIndex;
		
	seqNum = CWGetSeqNum();

	CW_CREATE_OBJECT_ERR(pendingReqIndex, int, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CW_CREATE_OBJECT_ERR(msgElemList, CWListElement, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	CW_CREATE_OBJECT_ERR(msgElemList->data, CWMsgElemData, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
	msgElemList->next= NULL;
	//Change type and value to change the msg elem to send
	((CWMsgElemData*)(msgElemList->data))->type = CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_CW_TYPE;
	((CWMsgElemData*)(msgElemList->data))->value = 0;

	if(!CWAssembleWTPEventRequest(&messages, &fragmentsNum, gWTPPathMTU, seqNum, msgElemList)){
		int i;
		if(messages)
			for(i = 0; i < fragmentsNum; i++) {
				CW_FREE_PROTOCOL_MESSAGE(messages[i]);
			}	
		CW_FREE_OBJECT(messages);
		return CW_FALSE;
	}
	
	*pendingReqIndex = CWSendPendingRequestMessage(gPendingRequestMsgs,messages,fragmentsNum);
	if (*pendingReqIndex<0) {
		CWDebugLog("Failure sending WTP Event Request");
		int k;
		for(k = 0; k < fragmentsNum; k++) {
			CW_FREE_PROTOCOL_MESSAGE(messages[k]);
		}
		CW_FREE_OBJECT(messages);
		CWDeleteList(&msgElemList, CWProtocolDestroyMsgElemData);
		return CW_FALSE;
	}
	CWUpdatePendingMsgBox(&(gPendingRequestMsgs[*pendingReqIndex]),
			      CW_MSG_TYPE_VALUE_WTP_EVENT_RESPONSE,
			      seqNum,
			      gCWRetransmitTimer,
			      pendingReqIndex,
			      CWWTPRetransmitTimerExpiredHandler,
			      0,
			      messages,
			      fragmentsNum);
	
	CWDeleteList(&msgElemList, CWProtocolDestroyMsgElemData);

	return CW_TRUE;
}

/*
void CWWTPRetransmitTimerExpiredHandler(CWTimerArg arg, CWTimerID id)
{
	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);

	CWDebugLog("Retransmit Timer Expired for Thread: %08x", (unsigned int)CWThreadSelf());
	
	if(gPendingRequestMsgs[arg].retransmission == gCWMaxRetransmit) {
		CWDebugLog("Peer is Dead");
		CWThreadSetSignals(SIG_UNBLOCK, 1, SIGALRM);
		//_CWCloseThread(*iPtr);
		return;
	}

	CWDebugLog("Retransmission Count increases to %d", gPendingRequestMsgs[arg].retransmission);
	
	int i;
	for(i = 0; i < gPendingRequestMsgs[arg].fragmentsNum; i++) {
		if(!CWSecuritySend(gWTPSession, gPendingRequestMsgs[arg].msgElems[i].msg, gPendingRequestMsgs[arg].msgElems[i].offset)){
			CWDebugLog("Failure sending Request");
			int k;
			for(k = 0; k < gPendingRequestMsgs[arg].fragmentsNum; k++) {
				CW_FREE_PROTOCOL_MESSAGE(gPendingRequestMsgs[arg].msgElems[k]);
			}	
			CW_FREE_OBJECT(gPendingRequestMsgs[arg].msgElems);
			CWThreadSetSignals(SIG_UNBLOCK, 1, SIGALRM);
			return;
		}
	}	
	gPendingRequestMsgs[arg].retransmission++;

	if(!CWTimerCreate(gPendingRequestMsgs[arg].timer_sec, &(gPendingRequestMsgs[arg].timer), gPendingRequestMsgs[arg].timer_hdl, gPendingRequestMsgs[arg].timer_arg)) {
		CWThreadSetSignals(SIG_UNBLOCK, 1, SIGALRM);
		return;
	}	

	CWThreadSetSignals(SIG_UNBLOCK, 1, SIGALRM);

	return;
}
*/

void CWWTPRetransmitTimerExpiredHandler(CWTimerArg hdl_arg)
{
	int index = *((int *)hdl_arg);

	CWDebugLog("Retransmit Timer Expired for Thread: %08x", (unsigned int)CWThreadSelf());
	
	if(gPendingRequestMsgs[index].retransmission == gCWMaxRetransmit) {
		CWDebugLog("Peer is Dead");
		//_CWCloseThread(*iPtr);
		return;
	}

	CWDebugLog("Retransmission Count increases to %d", gPendingRequestMsgs[index].retransmission);
	
	int i;
	for(i = 0; i < gPendingRequestMsgs[index].fragmentsNum; i++) {
#ifdef CW_NO_DTLS
		if (!CWNetworkSendUnsafeConnected(gWTPSocket, 
						  gPendingRequestMsgs[index].msgElems[i].msg,
						  gPendingRequestMsgs[index].msgElems[i].offset)) {
#else
		if (!CWSecuritySend(gWTPSession, 
				    gPendingRequestMsgs[index].msgElems[i].msg,
				    gPendingRequestMsgs[index].msgElems[i].offset)){
#endif
			CWDebugLog("Failure sending Request");
			int k;
			for(k = 0; k < gPendingRequestMsgs[index].fragmentsNum; k++) {
				CW_FREE_PROTOCOL_MESSAGE(gPendingRequestMsgs[index].msgElems[k]);
			}	
			CW_FREE_OBJECT(gPendingRequestMsgs[index].msgElems);
			CW_FREE_OBJECT(hdl_arg);
			return;
		}
	}	
	gPendingRequestMsgs[index].retransmission++;

	gPendingRequestMsgs[index].timer = timer_add(gPendingRequestMsgs[index].timer_sec, 
						   0, 
						   gPendingRequestMsgs[index].timer_hdl,
						   gPendingRequestMsgs[index].timer_arg);
	CW_FREE_OBJECT(hdl_arg);
	return;
}

