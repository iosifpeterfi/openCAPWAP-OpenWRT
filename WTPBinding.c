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


#include "wireless_copy.h"
#include "CWWTP.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> 
#include <signal.h>

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

int CWTranslateQueueIndex(int j)
{
	if (j==VOICE_QUEUE_INDEX) return 3;
	if (j==VIDEO_QUEUE_INDEX) return 2;
	if (j==BACKGROUND_QUEUE_INDEX) return 1;

	return 0;
}

#ifdef SOFTMAC
CWBool CWWTPInitBinding(int radioIndex){
	
	bindingValues* aux;
	int i;

	CW_CREATE_OBJECT_ERR(aux, bindingValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	gRadiosInfo.radiosInfo[radioIndex].bindingValuesPtr=(void*) aux;

	CW_CREATE_ARRAY_ERR(aux->qosValues, NUM_QOS_PROFILES, WTPQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	for(i=0; i<NUM_QOS_PROFILES; i++){
		/* TODO: Get info from Hostapd UNIX DOMAIN SOCKET*/
		aux->qosValues[i].cwMin = 3;
		aux->qosValues[i].cwMax = 15;
		aux->qosValues[i].AIFS = 2;
	}

	return CW_TRUE;
}

#else

#ifndef BCM
CWBool CWWTPInitBinding(int radioIndex)
{
	

	bindingValues* aux;
	int i,sock;
	struct iwreq wrq;

	/*** Inizializzazione socket ***/
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) 
	{

		CWLog("Error Creating Socket for ioctl"); 
		return CW_FALSE;
	}

	/*** Inizializzazione struttura iwreq ***/
	memset(&wrq, 0, sizeof(wrq));
	strncpy(wrq.ifr_name, gInterfaceName, IFNAMSIZ);

CWLog("wrq.ifr_name %s ",wrq.ifr_name);

	CW_CREATE_OBJECT_ERR(aux, bindingValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	gRadiosInfo.radiosInfo[radioIndex].bindingValuesPtr=(void*) aux;

	CW_CREATE_ARRAY_ERR(aux->qosValues, NUM_QOS_PROFILES, WTPQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	for(i=0; i<NUM_QOS_PROFILES; i++){
		
		/*	
		 * Donato Capitella - TO_REMOVE_DEVELOP
		 * Commented the following lines just to make the WTP work in a test machine.
 	 	 */
		 //if(!get_cwmin(sock, &wrq, CWTranslateQueueIndex(i), 0)){return CW_FALSE;}
		 
		//aux->qosValues[i].cwMin = wrq.u.param.value;

		//if(!get_cwmax(sock, &wrq, CWTranslateQueueIndex(i), 0)){return CW_FALSE;}
		//aux->qosValues[i].cwMax = wrq.u.param.value;

		//if(!get_aifs(sock, &wrq, CWTranslateQueueIndex(i), 0)){return CW_FALSE;}
		//aux->qosValues[i].AIFS = wrq.u.param.value;

/*##		aux->qosValues[i].cwMin = 2;
		aux->qosValues[i].cwMax = 4;
		aux->qosValues[i].AIFS = 3;
*/
	}

	return CW_TRUE;
}

#else
CWBool CWWTPInitBinding(int radioIndex)
{

	bindingValues* aux;
	int i;


	CW_CREATE_OBJECT_ERR(aux, bindingValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	gRadiosInfo.radiosInfo[radioIndex].bindingValuesPtr=(void*) aux;

	CW_CREATE_ARRAY_ERR(aux->qosValues, NUM_QOS_PROFILES, WTPQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	for(i=0; i<NUM_QOS_PROFILES; i++){
		/*Daniele: i driver Broadcom non permettono get sulle WME: setto i parametri del Qos a valori costanti*/
		aux->qosValues[i].cwMin = 2;
		aux->qosValues[i].cwMax = 4;
		aux->qosValues[i].AIFS = 3;
	}

	return CW_TRUE;
}
#endif

#endif


#ifdef SOFTMAC

CWBool CWBindingSetQosValues(int qosCount, RadioQosValues *radioQosValues, CWProtocolResultCode *resultCode){

	if (qosCount<=0) {return CW_TRUE;}
	if (radioQosValues==NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}

	*resultCode = CW_PROTOCOL_SUCCESS;

		
	int i,k,j;
	
	for(i=0;i<qosCount;i++)	{
		for(k=0;k<gRadiosInfo.radioCount;k++){
			if(radioQosValues[i].radioID==gRadiosInfo.radiosInfo[k].radioID){
				bindingValues* auxPtr=(bindingValues*) gRadiosInfo.radiosInfo[k].bindingValuesPtr;	
				
				for(j=0; j<NUM_QOS_PROFILES; j++){
					
					CWLog("AIFS:  %d	%d", auxPtr->qosValues[j].AIFS, radioQosValues[i].qosValues[j].AIFS);
					
					int aifs = (int)radioQosValues[i].qosValues[j].AIFS;
					int burst_time = 0;
					if(j==0) burst_time = 15; 
					else if(j==1) burst_time = 30;
					
					
					if( set_txq(j, radioQosValues[i].qosValues[j].cwMin, radioQosValues[i].qosValues[j].cwMax, aifs, burst_time) ){
						auxPtr->qosValues[j].cwMin = radioQosValues[i].qosValues[j].cwMin;
						auxPtr->qosValues[j].cwMax = radioQosValues[i].qosValues[j].cwMax;
						auxPtr->qosValues[j].AIFS = radioQosValues[i].qosValues[j].AIFS;
					}else{
						*resultCode=CW_PROTOCOL_FAILURE;
					}
					
					/*
					if(auxPtr->qosValues[j].cwMin!=radioQosValues[i].qosValues[j].cwMin)
					{
						if (set_wme_cwmin(CWTranslateQueueIndex(j), radioQosValues[i].qosValues[j].cwMin))	
							{auxPtr->qosValues[j].cwMin=radioQosValues[i].qosValues[j].cwMin;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}

					if(auxPtr->qosValues[j].cwMax!=radioQosValues[i].qosValues[j].cwMax)
					{
						if (set_wme_cwmax(CWTranslateQueueIndex(j), radioQosValues[i].qosValues[j].cwMax))
							{auxPtr->qosValues[j].cwMax=radioQosValues[i].qosValues[j].cwMax;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}

					if(auxPtr->qosValues[j].AIFS!=radioQosValues[i].qosValues[j].AIFS)
					{
						if (set_wme_aifsn(CWTranslateQueueIndex(j), radioQosValues[i].qosValues[j].AIFS))
							{auxPtr->qosValues[j].AIFS=radioQosValues[i].qosValues[j].AIFS;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}
					*/
				}
				break;
			}
		}
	}

	return CW_TRUE;
}

#else

#ifndef BCM
CWBool CWBindingSetQosValues(int qosCount, RadioQosValues *radioQosValues, CWProtocolResultCode *resultCode)
{
	struct iwreq wrq;
	int sock;

	if (qosCount<=0) {return CW_TRUE;}
	if (radioQosValues==NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}

	*resultCode = CW_PROTOCOL_SUCCESS;

	/*** Inizializzazione socket ***/
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) 
	{
		CWLog("Error Creating Socket for ioctl"); 
		return CWErrorRaise(CW_ERROR_GENERAL, NULL);;
	}
	
	/*** Inizializzazione struttura iwreq ***/
	memset(&wrq, 0, sizeof(wrq));
	strncpy(wrq.ifr_name, gInterfaceName, IFNAMSIZ);
		
	int i,k,j;
	
	for(i=0;i<qosCount;i++)
	{
		for(k=0;k<gRadiosInfo.radioCount;k++)
		{
			if(radioQosValues[i].radioID==gRadiosInfo.radiosInfo[k].radioID)
			{
				bindingValues* auxPtr=(bindingValues*) gRadiosInfo.radiosInfo[k].bindingValuesPtr;	
				
				for(j=0; j<NUM_QOS_PROFILES; j++)
				{
					if(auxPtr->qosValues[j].cwMin!=radioQosValues[i].qosValues[j].cwMin)
					{
						if (set_cwmin(sock, wrq, CWTranslateQueueIndex(j), 0, radioQosValues[i].qosValues[j].cwMin))
							{auxPtr->qosValues[j].cwMin=radioQosValues[i].qosValues[j].cwMin;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}

					if(auxPtr->qosValues[j].cwMax!=radioQosValues[i].qosValues[j].cwMax)
					{
						if (set_cwmax(sock, wrq, CWTranslateQueueIndex(j), 0, radioQosValues[i].qosValues[j].cwMax))
							{auxPtr->qosValues[j].cwMax=radioQosValues[i].qosValues[j].cwMax;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}

					if(auxPtr->qosValues[j].AIFS!=radioQosValues[i].qosValues[j].AIFS)
					{
						if (set_aifs(sock, wrq, CWTranslateQueueIndex(j), 0, radioQosValues[i].qosValues[j].AIFS))
							{auxPtr->qosValues[j].AIFS=radioQosValues[i].qosValues[j].AIFS;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}
				}
				break;
			}
		}
	}
	//WTPQosValues* aux=radioQosValues;
	
	close (sock);
	return CW_TRUE;
}

#else

CWBool CWBindingSetQosValues(int qosCount, RadioQosValues *radioQosValues, CWProtocolResultCode *resultCode)
{

	if (qosCount<=0) {return CW_TRUE;}
	if (radioQosValues==NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}

	*resultCode = CW_PROTOCOL_SUCCESS;

		
	int i,k,j;
	
	for(i=0;i<qosCount;i++)
	{
		for(k=0;k<gRadiosInfo.radioCount;k++)
		{
			if(radioQosValues[i].radioID==gRadiosInfo.radiosInfo[k].radioID)
			{
				bindingValues* auxPtr=(bindingValues*) gRadiosInfo.radiosInfo[k].bindingValuesPtr;	
				
				for(j=0; j<NUM_QOS_PROFILES; j++)
				{
					
					if(auxPtr->qosValues[j].cwMin!=radioQosValues[i].qosValues[j].cwMin)
					{
						if (set_wme_cwmin(CWTranslateQueueIndex(j), radioQosValues[i].qosValues[j].cwMin))	
							{auxPtr->qosValues[j].cwMin=radioQosValues[i].qosValues[j].cwMin;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}

					if(auxPtr->qosValues[j].cwMax!=radioQosValues[i].qosValues[j].cwMax)
					{
						if (set_wme_cwmax(CWTranslateQueueIndex(j), radioQosValues[i].qosValues[j].cwMax))
							{auxPtr->qosValues[j].cwMax=radioQosValues[i].qosValues[j].cwMax;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}

					if(auxPtr->qosValues[j].AIFS!=radioQosValues[i].qosValues[j].AIFS)
					{
						if (set_wme_aifsn(CWTranslateQueueIndex(j), radioQosValues[i].qosValues[j].AIFS))
							{auxPtr->qosValues[j].AIFS=radioQosValues[i].qosValues[j].AIFS;}
						else {*resultCode=CW_PROTOCOL_FAILURE;}
					}
				}
				break;
			}
		}
	}

	return CW_TRUE;
}

#endif

#endif

/**************************************************************
 * Update 2009: OFDM Message Element Management               *
 **************************************************************/					

CWBool CWManageOFDMValues(CWBindingConfigurationUpdateRequestValuesOFDM *ofdmValues, CWProtocolResultCode *resultCode) {

	if (ofdmValues==NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}

	*resultCode = CW_PROTOCOL_SUCCESS;

	OFDMControlValues *radioValues = ofdmValues->radioOFDMValues;
	//unsigned char radioID = ofdmValues->radioID;

	struct sockaddr_in serv_addr;
	int sendSock, slen = sizeof(serv_addr);
	
	if ((sendSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) ==-1 ) {
		CWLog("[FreqAnalyzer]: Error on creation of socket.");
		return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(FREQ_SERVER_PORT);
	
	if ( inet_aton(FREQ_SERVER_ADDR, &serv_addr.sin_addr)==0 ) {
	  CWLog("[CWManageOFDMValue]: Error on aton function.");
	  close(sendSock);
	  return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}

	/********************************************************************
	 * Update 2009: OFDM Management                                     *
	 *																	*
	 * Send a OFDMControlValues to wtpFreqManager.						*
	 * In this function there is the control switch for the different   *
	 * type of commands.												*
	 ********************************************************************/
	
	
	if ( sendto(sendSock, radioValues, sizeof(OFDMControlValues), 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
		CWLog("[CWManageOFDMValue]: Error on sendto function.");
		close(sendSock);
		return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}
		
	close(sendSock);	
	return CW_TRUE;		
}


CWBool CWParseWTPOFDM(CWProtocolMessage *msgPtr, int len, unsigned char* radioID, OFDMControlValues* valPtr)
{	
	CWParseMessageElementStart();

	*radioID=CWProtocolRetrieve8(msgPtr);

	valPtr->currentChan = CWProtocolRetrieve32(msgPtr);
	valPtr->BandSupport = (unsigned char) CWProtocolRetrieve8(msgPtr);
	valPtr->TIThreshold = (unsigned int) CWProtocolRetrieve32(msgPtr);

	CWParseMessageElementEnd();

}

CWBool CWParseWTPQoS (CWProtocolMessage *msgPtr, int len, unsigned char* radioID, unsigned char* tagPackets, WTPQosValues* valPtr)
{	
	int i;

	CWParseMessageElementStart();

	*radioID=CWProtocolRetrieve8(msgPtr);
	*tagPackets=CWProtocolRetrieve8(msgPtr);

	for(i=0; i<NUM_QOS_PROFILES; i++)
	{
		valPtr[i].queueDepth=(unsigned char)CWProtocolRetrieve8(msgPtr);
		valPtr[i].cwMin=CWProtocolRetrieve16(msgPtr);
		valPtr[i].cwMax=CWProtocolRetrieve16(msgPtr);
		valPtr[i].AIFS=(unsigned char)CWProtocolRetrieve8(msgPtr);
		valPtr[i].dot1PTag=(unsigned char)CWProtocolRetrieve8(msgPtr);
		valPtr[i].DSCPTag=(unsigned char)CWProtocolRetrieve8(msgPtr);
	}

	CWParseMessageElementEnd();
}

CWBool CWBindingSaveConfigurationUpdateRequest(void* bindingValuesPtr, CWProtocolResultCode* resultCode, int *updateRequestType)
{	
	if(bindingValuesPtr==NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}
	*resultCode = CW_PROTOCOL_SUCCESS;
	
	switch (*updateRequestType) {
	case BINDING_MSG_ELEMENT_TYPE_WTP_QOS:
	  {

		CWBindingConfigurationUpdateRequestValues* bindingPtr=(CWBindingConfigurationUpdateRequestValues*)bindingValuesPtr; 
		
		if (bindingPtr->qosCount>0) 
		  {
			if(!CWBindingSetQosValues(bindingPtr->qosCount, bindingPtr->radioQosValues, resultCode))
			  {
				CW_FREE_OBJECT(bindingPtr->radioQosValues);
				CW_FREE_OBJECT(bindingPtr);
				return CW_FALSE;
			  }
			CW_FREE_OBJECT(bindingPtr->radioQosValues);
			CW_FREE_OBJECT(bindingPtr);
		  }
		return CW_TRUE;
		break;
	  }
	case BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL:
	  {
		CWBindingConfigurationUpdateRequestValuesOFDM* bindingPtr=(CWBindingConfigurationUpdateRequestValuesOFDM *)bindingValuesPtr; 

		if(!CWManageOFDMValues(bindingPtr, resultCode))
		  {
			CW_FREE_OBJECT(bindingPtr->radioOFDMValues);
			CW_FREE_OBJECT(bindingPtr);
			return CW_FALSE;
		  }
		
		return CW_TRUE;
		break;
	  }
	}

	return CW_TRUE;
}

CWBool CWBindingParseConfigurationUpdateRequest (char *msg, int len, void **valuesPtr) 
{
	int i;
	CWProtocolMessage completeMsg;
	unsigned short int GlobalElemType=0;// = CWProtocolRetrieve32(&completeMsg);
	int qosCount = 0;

	if(msg == NULL || valuesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Parsing Binding Configuration Update Request...");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;

	CWBindingConfigurationUpdateRequestValues *auxBindingPtr;
	CWBindingConfigurationUpdateRequestValuesOFDM *ofdmBindingPtr;
  

	// parse message elements
	while(completeMsg.offset < len) {
	  unsigned short int elemType=0;// = CWProtocolRetrieve32(&completeMsg);
	  unsigned short int elemLen=0;// = CWProtocolRetrieve16(&completeMsg);
		
		CWParseFormatMsgElem(&completeMsg,&elemType,&elemLen);		

		GlobalElemType = elemType;

		//CWDebugLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen);
		
		switch(elemType) {
			case BINDING_MSG_ELEMENT_TYPE_WTP_QOS:
			  qosCount++;
			  //(auxBindingPtr->qosCount)++; // just count 
			  completeMsg.offset += elemLen;
			  break;
		    case BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL:
			  completeMsg.offset += elemLen;
			  break;
		default:
				if(CWBindingCheckType(elemType)) 
				{
					CW_FREE_OBJECT(valuesPtr);
					return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element");
				}
				else 
				{
					completeMsg.offset += elemLen;
					break;
				}
		}
	}

	if(completeMsg.offset != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");


	switch(GlobalElemType) {
	case BINDING_MSG_ELEMENT_TYPE_WTP_QOS:
	  { 	
		CW_CREATE_OBJECT_ERR(auxBindingPtr, CWBindingConfigurationUpdateRequestValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
		*valuesPtr = (void *)auxBindingPtr;
		
		auxBindingPtr->qosCount = qosCount;
		auxBindingPtr->radioQosValues=NULL;
		
		CW_CREATE_ARRAY_ERR(auxBindingPtr->radioQosValues, auxBindingPtr->qosCount, RadioQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		break;
	  }
	case BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL:
	  CW_CREATE_OBJECT_ERR(ofdmBindingPtr, CWBindingConfigurationUpdateRequestValuesOFDM, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
	  
	  *valuesPtr = (void *) ofdmBindingPtr;
		
	  CW_CREATE_OBJECT_ERR(ofdmBindingPtr->radioOFDMValues, OFDMControlValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
	  break;
	}

	i=0;
	completeMsg.offset = 0;
	while(completeMsg.offset < len) {
		unsigned short int type=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(&completeMsg,&type,&elemLen);		

		switch(type) {
		case BINDING_MSG_ELEMENT_TYPE_WTP_QOS:{
		  unsigned char tagPackets;
		  if(!(CWParseWTPQoS(&completeMsg, elemLen, &(auxBindingPtr->radioQosValues[i].radioID), &tagPackets, auxBindingPtr->radioQosValues[i].qosValues)))
			{
			  CW_FREE_OBJECT(auxBindingPtr->radioQosValues);
			  CW_FREE_OBJECT(valuesPtr);
			  return CW_FALSE; // will be handled by the caller
			}
		  i++;
		  break;
		}
		case BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL: /* 2009: New case */
		  {
			if(!(CWParseWTPOFDM(&completeMsg, elemLen, &(ofdmBindingPtr->radioID), ofdmBindingPtr->radioOFDMValues)))
			{
			  CW_FREE_OBJECT(ofdmBindingPtr->radioOFDMValues);
			  CW_FREE_OBJECT(valuesPtr);
			  return CW_FALSE; // will be handled by the caller
			}
			break;
		  }
		default:
		  completeMsg.offset += elemLen;
		  break;
		}
	}
	
	CWLog("Binding Configure Update Request Parsed");
	
	return CW_TRUE;
}

CWBool CWBindingSaveConfigureResponse(void* bindingValuesPtr, CWProtocolResultCode* resultCode)
{	
	if(bindingValuesPtr==NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}
	*resultCode = CW_PROTOCOL_SUCCESS;
	
	CWBindingConfigurationRequestValues* bindingPtr=(CWBindingConfigurationRequestValues*)bindingValuesPtr; 

	if (bindingPtr->qosCount>0) 
	{
		if(!CWBindingSetQosValues(bindingPtr->qosCount, bindingPtr->radioQosValues, resultCode))
		{
			CW_FREE_OBJECT(bindingPtr->radioQosValues);
			return CW_FALSE;
		}
		CW_FREE_OBJECT(bindingPtr->radioQosValues);
	}
	return CW_TRUE;
}

CWBool CWBindingParseConfigureResponse (char *msg, int len, void **valuesPtr){
	int i;
	CWProtocolMessage completeMsg;
	
	if(msg == NULL || valuesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Parsing Binding Configuration Request...");

	completeMsg.msg = msg;
	completeMsg.offset = 0;

	CWBindingConfigurationRequestValues *auxBindingPtr;
	CW_CREATE_OBJECT_ERR(auxBindingPtr, CWBindingConfigurationRequestValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
	*valuesPtr = (void *)auxBindingPtr;

	auxBindingPtr->qosCount = 0;
	auxBindingPtr->radioQosValues=NULL;
	// parse message elements
	while(completeMsg.offset < len) {
		unsigned short int elemType=0;// = CWProtocolRetrieve32(&completeMsg);
		unsigned short int elemLen=0;// = CWProtocolRetrieve16(&completeMsg);
		
		CWParseFormatMsgElem(&completeMsg,&elemType,&elemLen);		

		CWDebugLog("Parsing Message Element: %d, elemLen: %d", elemType, elemLen);

		switch(elemType) {
			case BINDING_MSG_ELEMENT_TYPE_WTP_QOS:
				(auxBindingPtr->qosCount)++; // just count 
				completeMsg.offset += elemLen;
				break;
			default:
				if(CWBindingCheckType(elemType)) 
				{	
					CW_FREE_OBJECT(valuesPtr);
					return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element");
				}
				else 
				{
					completeMsg.offset += elemLen;
					break;
				}
		}
	}

	if(completeMsg.offset != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	// actually read each radio info
	CW_CREATE_ARRAY_ERR(auxBindingPtr->radioQosValues, auxBindingPtr->qosCount, RadioQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
	i=0;
	completeMsg.offset = 0;
	while(completeMsg.offset < len) {
		unsigned short int type=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(&completeMsg,&type,&elemLen);		

		switch(type) {
			case BINDING_MSG_ELEMENT_TYPE_WTP_QOS:{
				unsigned char tagPackets;
				if(!(CWParseWTPQoS(&completeMsg, elemLen, &(auxBindingPtr->radioQosValues[i].radioID), &tagPackets, auxBindingPtr->radioQosValues[i].qosValues)))
				{
					CW_FREE_OBJECT(auxBindingPtr->radioQosValues);
					CW_FREE_OBJECT(valuesPtr);
					return CW_FALSE; // will be handled by the caller
				}
				i++;
				break;
			}
			default:
				completeMsg.offset += elemLen;
				break;
		}
	}
	
	CWLog("Binding Configuration Request Parsed");
	
	return CW_TRUE;
}

