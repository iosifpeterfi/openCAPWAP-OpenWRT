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


#include "CWCommon.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif
CWThreadMutex gWTPsMutex;

const int gMaxCAPWAPHeaderSizeBinding = 16; // note: this include optional Wireless field


CWBool CWBindingCheckType(int elemType)
{
	if (elemType>=BINDING_MIN_ELEM_TYPE && elemType<=BINDING_MAX_ELEM_TYPE)
		return CW_TRUE;
	return CW_FALSE;
}

// Assemble a CAPWAP Data Packet creating Transport Header.
// completeMsgPtr is an array of fragments (can be of size 1 if the packet doesn't need fragmentation)
CWBool CWAssembleDataMessage(CWProtocolMessage **completeMsgPtr, int *fragmentsNumPtr, int PMTU, CWProtocolMessage *frame, CWBindingTransportHeaderValues *bindingValuesPtr, int is_crypted, int keepAlive)
{

	CWProtocolMessage transportHdr;
	CWProtocolTransportHeaderValues transportVal;

	if(completeMsgPtr == NULL || fragmentsNumPtr == NULL || frame == NULL ) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
//	CWDebugLog("PMTU: %d", PMTU);
	
	// handle fragmentation
	
	PMTU = PMTU - gMaxCAPWAPHeaderSizeBinding;
	
	if(PMTU > 0) {
		PMTU = (PMTU/8)*8; // CAPWAP fragments are made of groups of 8 bytes
		if(PMTU == 0) goto cw_dont_fragment;
		
//		CWDebugLog("Aligned PMTU: %d", PMTU);
		*fragmentsNumPtr = (frame->offset) / PMTU;
		if((frame->offset % PMTU) != 0) (*fragmentsNumPtr)++;
		//CWDebugLog("Fragments #: %d", *fragmentsNumPtr);
	} else {
	cw_dont_fragment:
		*fragmentsNumPtr = 1;
	}
	
	transportVal.bindingValuesPtr = bindingValuesPtr;
	
	
	if( frame->data_msgType == CW_IEEE_802_11_FRAME_TYPE ) transportVal.type = 1;
	
	if(*fragmentsNumPtr == 1) {
			
		transportVal.isFragment = transportVal.last = transportVal.fragmentOffset = transportVal.fragmentID = 0;
		transportVal.payloadType = is_crypted;		

		// Assemble Message Elements
		
		if(keepAlive){
			if(!(CWAssembleTransportHeaderKeepAliveData(&transportHdr, &transportVal,keepAlive))) {
				CW_FREE_PROTOCOL_MESSAGE(transportHdr);
				return CW_FALSE; // will be handled by the caller
			} 
		}else{
			 
			if(!(CWAssembleTransportHeader(&transportHdr, &transportVal))) {
				CW_FREE_PROTOCOL_MESSAGE(transportHdr);
				return CW_FALSE; // will be handled by the caller
			} 
		}
				
		CW_CREATE_OBJECT_ERR(*completeMsgPtr, CWProtocolMessage, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CW_CREATE_PROTOCOL_MESSAGE(((*completeMsgPtr)[0]), transportHdr.offset + frame->offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
		CWProtocolStoreMessage(&((*completeMsgPtr)[0]), &transportHdr);
		CWProtocolStoreMessage(&((*completeMsgPtr)[0]), frame);
		
		CW_FREE_PROTOCOL_MESSAGE(transportHdr);
	} else {
		
		int fragID = CWGetFragmentID();
		int totalSize = frame->offset;
		
		//CWDebugLog("%d Fragments", *fragmentsNumPtr);
		CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(*completeMsgPtr, *fragmentsNumPtr, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		frame->offset = 0;

		int i;
		for(i = 0; i < *fragmentsNumPtr; i++) { // for each fragment to assemble
			int fragSize;
			
			transportVal.isFragment = 1;
			transportVal.fragmentOffset = (frame->offset) / 8;
			transportVal.fragmentID = fragID;
			transportVal.payloadType = is_crypted;
			
			if(i < ((*fragmentsNumPtr)-1)) { // not last fragment
				fragSize = PMTU;
				transportVal.last = 0;
			} else { // last fragment
				fragSize = totalSize - (((*fragmentsNumPtr)-1) * PMTU);
				transportVal.last = 1;
			}
		
			CWDebugLog("Fragment #:%d, offset:%d, bytes stored:%d/%d", i, transportVal.fragmentOffset, fragSize, totalSize);
			
			// Assemble Transport Header for this fragment
			if(keepAlive){
				if(!(CWAssembleTransportHeaderKeepAliveData(&transportHdr, &transportVal,keepAlive))) {
					CW_FREE_PROTOCOL_MESSAGE(transportHdr);
					CW_FREE_OBJECT(completeMsgPtr);
					return CW_FALSE; // will be handled by the caller
				}
			}else{
				if(!(CWAssembleTransportHeader(&transportHdr, &transportVal))) {
					CW_FREE_PROTOCOL_MESSAGE(transportHdr);
					CW_FREE_OBJECT(completeMsgPtr);
					return CW_FALSE; // will be handled by the caller
				}
			}

			
			CW_CREATE_PROTOCOL_MESSAGE(((*completeMsgPtr)[i]), transportHdr.offset + fragSize, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
			CWProtocolStoreMessage(&((*completeMsgPtr)[i]), &transportHdr);
			CWProtocolStoreRawBytes(&((*completeMsgPtr)[i]), &((frame->msg)[frame->offset]), fragSize);
			(frame->offset) += fragSize;
			
			CW_FREE_PROTOCOL_MESSAGE(transportHdr);
		}
	}
	return CW_TRUE;
}


CWBool CWAssembleTransportHeaderBinding(CWProtocolMessage *transportHdrPtr, CWBindingTransportHeaderValues *valuesPtr)
{
	int val = 0;
/* Mauro: non piu' specificato nel campo Wireless Specific Information 
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_WIRELESS_ID_START,
		     CW_TRANSPORT_HEADER_WIRELESS_ID_LEN,
		     CW_BINDING_WIRELESSID);
*/
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_LENGTH_START,
		     CW_TRANSPORT_HEADER_LENGTH_LEN,
		     CW_BINDING_DATALENGTH);

//	CWDebugLog("#### RSSI in= %d",valuesPtr->RSSI );
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_RSSI_START,
		     CW_TRANSPORT_HEADER_RSSI_LEN,
		     valuesPtr->RSSI);

//	CWDebugLog("#### SNR in= %d",valuesPtr->SNR );
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_SNR_START,
		     CW_TRANSPORT_HEADER_SNR_LEN,
		     valuesPtr->SNR);

/* Mauro: inserisci il byte piu' significativo del sottocampo Data */
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_DATARATE_1_START,
		     CW_TRANSPORT_HEADER_DATARATE_1_LEN,
		     (valuesPtr->dataRate)>>8);

	CWProtocolStore32(transportHdrPtr, val);
	val = 0;

/* Mauro: inserisci il byte meno significativo del sottocampo Data */
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_DATARATE_2_START,
		     CW_TRANSPORT_HEADER_DATARATE_2_LEN,
		     (valuesPtr->dataRate) & 0x000000FF);

//	CWDebugLog("#### data rate in= %d",valuesPtr->dataRate );
/*	CWSetField32(val,
		     CW_TRANSPORT_HEADER_DATARATE_START,
		     CW_TRANSPORT_HEADER_DATARATE_LEN,
		     valuesPtr->dataRate);
*/
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_PADDING_START,
		     CW_TRANSPORT_HEADER_PADDING_LEN,
		     0);

	CWProtocolStore32(transportHdrPtr, val);

	return CW_TRUE;
}

CWBool CWParseTransportHeaderMACAddress(CWProtocolMessage *msgPtr,  char *mac_ptr){

	if(msgPtr == NULL ) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	
	unsigned char *vval;
	vval=malloc(7);

	//CWDebugLog("Parse Transport Header");
	int Mac_len = CWProtocolRetrieve8(msgPtr);
	
	vval =  (unsigned char*)CWProtocolRetrieveRawBytes(msgPtr,7);
	
	if( mac_ptr!= NULL ){
		
		CWThreadMutexLock(&gWTPsMutex);
					memcpy(mac_ptr, vval, Mac_len);
		CWThreadMutexUnlock(&gWTPsMutex);
	
	}
	

	return CW_TRUE;
}

CWBool CWParseTransportHeaderBinding(CWProtocolMessage *msgPtr, CWBindingTransportHeaderValues *valuesPtr){
	unsigned int val = 0;
	
	if(msgPtr == NULL || valuesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	//CWDebugLog("Parse Transport Header");
	val = CWProtocolRetrieve32(msgPtr);

/* Mauro: non piu' specificato nel campo Wireless Specific Information
	if(CWGetField32(val, CW_TRANSPORT_HEADER_WIRELESS_ID_START, CW_TRANSPORT_HEADER_WIRELESS_ID_LEN) != CW_BINDING_WIRELESSID)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Wrong Binding Wireless ID");
*/
	if(CWGetField32(val, CW_TRANSPORT_HEADER_LENGTH_START, CW_TRANSPORT_HEADER_LENGTH_LEN) != CW_BINDING_DATALENGTH)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Wrong Binding Data Field Length");

	valuesPtr->RSSI = CWGetField32(val, CW_TRANSPORT_HEADER_RSSI_START, CW_TRANSPORT_HEADER_RSSI_LEN);
//	CWDebugLog("RSSI: %d", valuesPtr->RSSI);

	valuesPtr->SNR = CWGetField32(val, CW_TRANSPORT_HEADER_SNR_START, CW_TRANSPORT_HEADER_SNR_LEN);
//	CWDebugLog("SNR: %d", valuesPtr->SNR);
	
/* Mauro: preleva il byte piu' significativo del sottocampo Data */
	valuesPtr->dataRate = CWGetField32(val, CW_TRANSPORT_HEADER_DATARATE_1_START, CW_TRANSPORT_HEADER_DATARATE_1_LEN);
	val = CWProtocolRetrieve32(msgPtr);
	
	/* Daniele: controlla se si tratta di un msg dati(statistiche) o di un msg contenete un wireless frame*/
	
	/************************************************************************
	 * 2009 Update:															*
	 * For distinguish between the two types of "specials" data messages	*
	 * (QoS stats and Frequency Stats) we used the following values:		*
	 *		dataRate == 255 && SNR = 1 -> Frequency Stats Packet			*
	 *		dataRate == 255            -> QoS Stats Packet					*
	 ************************************************************************/

	if(valuesPtr->dataRate == 255) {
	  if (valuesPtr->SNR == 1 )
		msgPtr->data_msgType=CW_DATA_MSG_FREQ_STATS_TYPE;
	  else
		msgPtr->data_msgType=CW_DATA_MSG_STATS_TYPE;
	}
	else if(valuesPtr->dataRate == 0) 
	  msgPtr->data_msgType=CW_DATA_MSG_FRAME_TYPE;

/* Mauro: preleva il byte meno significativo del sottocampo Data */
	valuesPtr->dataRate = ((valuesPtr->dataRate)<<8) | CWGetField32(val, CW_TRANSPORT_HEADER_DATARATE_1_START, CW_TRANSPORT_HEADER_DATARATE_1_LEN);


//	valuesPtr->dataRate = CWGetField32(val, CW_TRANSPORT_HEADER_DATARATE_START, CW_TRANSPORT_HEADER_DATARATE_LEN);
//	CWDebugLog("DATARATE: %d", valuesPtr->dataRate);
	
	return CW_TRUE;
}
