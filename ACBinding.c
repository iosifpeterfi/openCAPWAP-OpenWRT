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
 *           Daniele De Sanctis (danieledesanctis@gmail.com)									* 
 *	         Antonio Davoli (antonio.davoli@gmail.com)											*
 ************************************************************************************************/


#include "CWAC.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWBool CWACInitBinding(int i)
{
	int j;
	bindingValues* aux;

	CW_CREATE_OBJECT_ERR(aux, bindingValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	(gWTPs[i].WTPProtocolManager).bindingValuesPtr=(void*) aux;

	CW_CREATE_ARRAY_ERR(aux->qosValues, NUM_QOS_PROFILES, WTPQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	//Init default values
	for(j=0; j<NUM_QOS_PROFILES; j++)
	{
		aux->qosValues[j].cwMin=gDefaultQosValues[j].cwMin;
		aux->qosValues[j].cwMax=gDefaultQosValues[j].cwMax;
		aux->qosValues[j].AIFS=gDefaultQosValues[j].AIFS;
		
		aux->qosValues[j].queueDepth=0;
		aux->qosValues[j].dot1PTag=0;
		aux->qosValues[j].DSCPTag=0;
	}

	return CW_TRUE;
}

CWBool CWMergeQosValues(int WTPIndex)
{
	int i;
	bindingValues* aux;

	aux=(bindingValues*)(gWTPs[WTPIndex].WTPProtocolManager).bindingValuesPtr;

	for(i=0; i<NUM_QOS_PROFILES; i++)
	{
		if(gWTPs[WTPIndex].qosValues[i].cwMin==UNUSED_QOS_VALUE) 
			{gWTPs[WTPIndex].qosValues[i].cwMin=aux->qosValues[i].cwMin;}
	
		if(gWTPs[WTPIndex].qosValues[i].cwMax==UNUSED_QOS_VALUE) 
			{gWTPs[WTPIndex].qosValues[i].cwMax=aux->qosValues[i].cwMax;}

		if(gWTPs[WTPIndex].qosValues[i].AIFS==UNUSED_QOS_VALUE) 
			{gWTPs[WTPIndex].qosValues[i].AIFS=aux->qosValues[i].AIFS;}
	}
	return CW_TRUE;
}

/******************************************************************
 * 2009 Updates:												  *
 *				Functions for management of Configuration Update  *
 *				Request with OFDM Message Element                 *
 ******************************************************************/

CWBool CWMergeOFDMValues(int WTPIndex)
{

	OFDMControlValues* aux;
	
	aux=(OFDMControlValues *)(gWTPs[WTPIndex].WTPProtocolManager).bindingValuesPtr;

	if(gWTPs[WTPIndex].ofdmValues->currentChan == UNUSED_OFDM_VALUE )
	  gWTPs[WTPIndex].ofdmValues->currentChan = aux->currentChan;

	if(gWTPs[WTPIndex].ofdmValues->BandSupport == UNUSED_OFDM_VALUE )
	  gWTPs[WTPIndex].ofdmValues->BandSupport = aux->BandSupport;

	if(gWTPs[WTPIndex].ofdmValues->TIThreshold == UNUSED_OFDM_VALUE )
	  gWTPs[WTPIndex].ofdmValues->TIThreshold = aux->TIThreshold;

	return CW_TRUE;
}


CWBool CWAssembleWTPOFDM(CWProtocolMessage *msgPtr, int radioID)
{
	const int totalMessageLength= BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL_LENGTH;
	int* iPtr;
	OFDMControlValues* valuesPtr;

	CWLog("Assembling Binding Configuration Update Request [OFDM CASE]...");

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
	  return CW_FALSE;
	}

	if(!CWMergeOFDMValues(*iPtr)) {return CW_FALSE;}

	valuesPtr =gWTPs[*iPtr].ofdmValues;

	/* create message */
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, totalMessageLength, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, radioID);
	CWProtocolStore32(msgPtr, valuesPtr->currentChan);
	CWProtocolStore8(msgPtr, valuesPtr->BandSupport);
	CWProtocolStore32(msgPtr, valuesPtr->TIThreshold);
	
	CWLog("Assembling Binding Configuration Update Request [OFDM CASE]: Message Assembled.");
					  
	return CWAssembleMsgElem(msgPtr, BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL);
} 

CWBool CWAssembleWTPQoS (CWProtocolMessage *msgPtr, int radioID, int tagPackets) 
{
	const int headerLength=2;
	const int messageBodyLength=32;
	const int totalMessageLength=headerLength+messageBodyLength;
	int i;
	int* iPtr;
	WTPQosValues* valuesPtr;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
		return CW_FALSE;
	}
	if(!CWMergeQosValues(*iPtr)) {return CW_FALSE;}
	
	valuesPtr=gWTPs[*iPtr].qosValues;
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, totalMessageLength, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, radioID);
	CWProtocolStore8(msgPtr, tagPackets);

	for(i=0; i<NUM_QOS_PROFILES; i++)
	{
		CWProtocolStore8(msgPtr, valuesPtr[i].queueDepth);
		CWProtocolStore16(msgPtr, valuesPtr[i].cwMin);
		CWProtocolStore16(msgPtr, valuesPtr[i].cwMax);
		CWProtocolStore8(msgPtr, valuesPtr[i].AIFS);
		CWProtocolStore8(msgPtr, valuesPtr[i].dot1PTag);
		CWProtocolStore8(msgPtr, valuesPtr[i].DSCPTag);
	}
		
	return CWAssembleMsgElem(msgPtr, BINDING_MSG_ELEMENT_TYPE_WTP_QOS);
}

CWBool CWBindingAssembleConfigureResponse(CWProtocolMessage **msgElems, int *msgElemCountPtr)
{
	CWWTPRadiosInfo radiosInfo;
	int* iPtr;
	const int tagPackets=0;
	int k = -1, radioCount, radioID, j;
	
	if(msgElems == NULL || msgElemCountPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
		return CW_FALSE;
	}

	//Calculate the number of msg Elements
	*msgElemCountPtr =0;
	radiosInfo=gWTPs[*iPtr].WTPProtocolManager.radiosInfo;
	radioCount=radiosInfo.radioCount;
	*msgElemCountPtr = radioCount;

	CWLog("Assembling Binding Configuration Response...");

	//Reserve memory for msg Elements
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(*msgElems, *msgElemCountPtr, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	if(!CWThreadMutexLock(&(gWTPs[*iPtr].interfaceMutex)))
	{
		CWLog("Error locking a mutex");
		CWCloseThread();
	}
		//Fill gWTPs[*iPtr].qosValues with default settings 
		gWTPs[*iPtr].qosValues=gDefaultQosValues;

		for (j=0; j<radioCount; j++)
		{
			radioID=radiosInfo.radiosInfo[j].radioID;
			// Assemble WTP QoS Message Element for each radio 
			if (!(CWAssembleWTPQoS(&(*msgElems[++k]), radioID, tagPackets)))
			{
				int i;
				for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(*msgElems[i]);}
				CW_FREE_OBJECT(*msgElems);
				CWThreadMutexUnlock(&(gWTPs[*iPtr].interfaceMutex));
				return CW_FALSE; // error will be handled by the caller
			}
		}

		gWTPs[*iPtr].qosValues=NULL;
	CWThreadMutexUnlock(&(gWTPs[*iPtr].interfaceMutex));	

	CWLog("Binding Configuration Response Assembled");

	return CW_TRUE;
}

/******************************************************************
 * 2009 Updates:												  *
 *				Added new switch case for ofdm message management *
 ******************************************************************/

CWBool CWBindingAssembleConfigurationUpdateRequest(CWProtocolMessage **msgElems, int *msgElemCountPtr, int BindingMsgElement){
	CWWTPRadiosInfo radiosInfo;
	int* iPtr;
	const int tagPackets=0;
	int k = -1, radioCount, radioID, j;
	
	
	if(msgElems == NULL || msgElemCountPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
		return CW_FALSE;
	}

	*msgElemCountPtr =0;

	radiosInfo=gWTPs[*iPtr].WTPProtocolManager.radiosInfo;
	radioCount=radiosInfo.radioCount;
	*msgElemCountPtr = radioCount;

	CWLog("Assembling Binding Configuration Update Request...");

	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(*msgElems, *msgElemCountPtr, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	/* Selection of type of Conf Update Request */

	switch(BindingMsgElement) {
	case BINDING_MSG_ELEMENT_TYPE_WTP_QOS: 
	  {
		for (j=0; j<radioCount; j++)
		  {
			radioID=radiosInfo.radiosInfo[j].radioID;
			
			// Assemble Message Elements
			if (!(CWAssembleWTPQoS(&(*msgElems[++k]), radioID, tagPackets)))
			  {
				int i;
				for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(*msgElems[i]);}
				CW_FREE_OBJECT(*msgElems);
				return CW_FALSE; // error will be handled by the caller
			  }
		  }
		break;
	  }
	case BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL:
	  {
		  for (j=0; j<radioCount; j++) {
			  radioID=radiosInfo.radiosInfo[j].radioID;

			  /* Assemble Message Elements */
			  if (!(CWAssembleWTPOFDM(&(*msgElems[++k]), radioID))) {
				  int i;
				  for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(*msgElems[i]);}
				  CW_FREE_OBJECT(*msgElems);
				  return CW_FALSE; // error will be handled by the caller
			  }
		  }
		break;
	  }
	default:
	  {
		return CW_FALSE; // error will be handled by the caller
	  }
	}

	CWLog("Binding Configuration Update Request Assembled");

	return CW_TRUE;
}

CWBool CWBindingSaveConfigurationUpdateResponse(CWProtocolResultCode resultCode, int WTPIndex)
{
	int i;

	bindingValues* aux=(bindingValues*)gWTPs[WTPIndex].WTPProtocolManager.bindingValuesPtr;

	if (resultCode==CW_PROTOCOL_SUCCESS) 
	{
		if (gWTPs[WTPIndex].qosValues!=NULL)
		{
			for(i=0; i<NUM_QOS_PROFILES; i++)
			{
				aux->qosValues[i].cwMin=gWTPs[WTPIndex].qosValues[i].cwMin;
				aux->qosValues[i].cwMax=gWTPs[WTPIndex].qosValues[i].cwMax;
				aux->qosValues[i].AIFS=gWTPs[WTPIndex].qosValues[i].AIFS;
			}
		}
	}

	return CW_TRUE;
}
