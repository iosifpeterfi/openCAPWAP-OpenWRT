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
 * Author :  Ludovico Rossi (ludo@bluepixysw.com)                                          *  
 *           Del Moro Andrea (andrea_delmoro@libero.it)                                    *
 *           Giovannini Federica (giovannini.federica@gmail.com)                           *
 *           Massimo Vellucci (m.vellucci@unicampus.it)                                    *
 *           Mauro Bisson (mauro.bis@gmail.com)                                            *
 *******************************************************************************************/


#include "CWCommon.h"
#include "CWVendorPayloads.h"
#include "WUM.h"
pthread_mutex_t gRADIO_MAC_mutex;

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif
 
static const int gCWIANATimes256 = CW_IANA_ENTERPRISE_NUMBER * 256;
static const int gMaxDTLSHeaderSize = 25; // see http://crypto.stanford.edu/~nagendra/papers/dtls.pdf
static const int gMaxCAPWAPHeaderSize = 8; // note: this include optional Wireless field
char gRADIO_MAC[6]; // note: this include optional Wireless field

// stores 8 bits in the message, increments the current offset in bytes
void CWProtocolStore8(CWProtocolMessage *msgPtr, unsigned char val) {
	CW_COPY_MEMORY(&((msgPtr->msg)[(msgPtr->offset)]), &(val), 1);
	(msgPtr->offset) += 1;
}

// stores 16 bits in the message, increments the current offset in bytes
void CWProtocolStore16(CWProtocolMessage *msgPtr, unsigned short val) {
	val = htons(val);
	CW_COPY_MEMORY(&((msgPtr->msg)[(msgPtr->offset)]), &(val), 2);
	(msgPtr->offset) += 2;
}

// stores 32 bits in the message, increments the current offset in bytes
void CWProtocolStore32(CWProtocolMessage *msgPtr, unsigned int val) {
	val = htonl(val);
	CW_COPY_MEMORY(&((msgPtr->msg)[(msgPtr->offset)]), &(val), 4);
	(msgPtr->offset) += 4;
}

// stores a string in the message, increments the current offset in bytes. Doesn't store
// the '\0' final character.
void CWProtocolStoreStr(CWProtocolMessage *msgPtr, char *str) {
	int len = strlen(str);
	CW_COPY_MEMORY(&((msgPtr->msg)[(msgPtr->offset)]), str, len);
	(msgPtr->offset) += len;
}

// stores another message in the message, increments the current offset in bytes.
void CWProtocolStoreMessage(CWProtocolMessage *msgPtr, CWProtocolMessage *msgToStorePtr) {
	CW_COPY_MEMORY(&((msgPtr->msg)[(msgPtr->offset)]), msgToStorePtr->msg, msgToStorePtr->offset);
	(msgPtr->offset) += msgToStorePtr->offset;
}

// stores len bytes in the message, increments the current offset in bytes.
void CWProtocolStoreRawBytes(CWProtocolMessage *msgPtr, char *bytes, int len) {
	CW_COPY_MEMORY(&((msgPtr->msg)[(msgPtr->offset)]), bytes, len);
	(msgPtr->offset) += len;
}

// retrieves 8 bits from the message, increments the current offset in bytes.
unsigned char CWProtocolRetrieve8(CWProtocolMessage *msgPtr) {
	unsigned char val;
	
	CW_COPY_MEMORY(&val, &((msgPtr->msg)[(msgPtr->offset)]), 1);
	(msgPtr->offset) += 1;
	
	return val;
}

// retrieves 16 bits from the message, increments the current offset in bytes.
unsigned short CWProtocolRetrieve16(CWProtocolMessage *msgPtr) {
	unsigned short val;
	
	CW_COPY_MEMORY(&val, &((msgPtr->msg)[(msgPtr->offset)]), 2);
	(msgPtr->offset) += 2;
	
	return ntohs(val);
}

// retrieves 32 bits from the message, increments the current offset in bytes.
unsigned int CWProtocolRetrieve32(CWProtocolMessage *msgPtr) {
	unsigned int val;
	
	CW_COPY_MEMORY(&val, &((msgPtr->msg)[(msgPtr->offset)]), 4);
	(msgPtr->offset) += 4;
	
	return ntohl(val);
}

// retrieves a string (not null-terminated) from the message, increments the current offset in bytes.
// Adds the '\0' char at the end of the string which is returned
char *CWProtocolRetrieveStr(CWProtocolMessage *msgPtr, int len) {
	char *str;
	
	CW_CREATE_OBJECT_SIZE_ERR(str, (len+1), return NULL;);
	
	CW_COPY_MEMORY(str, &((msgPtr->msg)[(msgPtr->offset)]), len);
	str[len] = '\0';
	(msgPtr->offset) += len;
	
	return str;
}

// retrieves len bytes from the message, increments the current offset in bytes.
char *CWProtocolRetrieveRawBytes(CWProtocolMessage *msgPtr, int len) {
	char *bytes;
	
	CW_CREATE_OBJECT_SIZE_ERR(bytes, len, return NULL;);
	
	CW_COPY_MEMORY(bytes, &((msgPtr->msg)[(msgPtr->offset)]), len);
	(msgPtr->offset) += len;
	
	return bytes;
}

void CWProtocolDestroyMsgElemData(void *f) {
	CW_FREE_OBJECT(f);
}

// Assemble a Message Element creating the appropriate header and storing the message.
CWBool CWAssembleMsgElem(CWProtocolMessage *msgPtr, unsigned int type) {
	CWProtocolMessage completeMsg;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MESSAGE(completeMsg, 6+(msgPtr->offset), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	// store header
	CWProtocolStore16(&completeMsg, type);
	CWProtocolStore16(&completeMsg, msgPtr->offset); // size of the body

	// store body
	CWProtocolStoreMessage(&completeMsg, msgPtr);

	CW_FREE_PROTOCOL_MESSAGE(*msgPtr);

	msgPtr->msg = completeMsg.msg;
	msgPtr->offset = completeMsg.offset;

	return CW_TRUE;
}

// Assembles the Transport Header
CWBool CWAssembleTransportHeader(CWProtocolMessage *transportHdrPtr, CWProtocolTransportHeaderValues *valuesPtr) {
	
	char radio_mac_present = 0;
	int i;
	

	for(i=0;i<6;i++){
		//printf(":::: %02X\n",gRADIO_MAC[i]);
		if( gRADIO_MAC[i]!=0 ) {
			radio_mac_present = 8;
			break;
		}
	}
	
	unsigned int val = 0;
	if(transportHdrPtr == NULL || valuesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(valuesPtr->bindingValuesPtr != NULL)
		{CW_CREATE_PROTOCOL_MESSAGE(*transportHdrPtr,gMaxCAPWAPHeaderSizeBinding+radio_mac_present, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););}
	else {CW_CREATE_PROTOCOL_MESSAGE(*transportHdrPtr,8 + radio_mac_present, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););}	 // meaningful bytes of the header (no wirless header and MAC address)
	CWSetField32(val, 
		     CW_TRANSPORT_HEADER_VERSION_START,
		     CW_TRANSPORT_HEADER_VERSION_LEN,
		     CW_PROTOCOL_VERSION); // current version of CAPWAP

	
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_TYPE_START,
		     CW_TRANSPORT_HEADER_TYPE_LEN,
		     (valuesPtr->payloadType == CW_PACKET_PLAIN) ? 0 : 1);
	
	if(radio_mac_present)
		if(valuesPtr->bindingValuesPtr != NULL)
			CWSetField32(val,
					 CW_TRANSPORT_HEADER_HLEN_START,
					 CW_TRANSPORT_HEADER_HLEN_LEN,
					 CW_BINDING_HLEN + 2);
		else 
			CWSetField32(val,
					 CW_TRANSPORT_HEADER_HLEN_START,
					 CW_TRANSPORT_HEADER_HLEN_LEN,
					 2 + 2);
	else
		if(valuesPtr->bindingValuesPtr != NULL)
			CWSetField32(val,
					 CW_TRANSPORT_HEADER_HLEN_START,
					 CW_TRANSPORT_HEADER_HLEN_LEN,
					 CW_BINDING_HLEN);
		else 
			CWSetField32(val,
					 CW_TRANSPORT_HEADER_HLEN_START,
					 CW_TRANSPORT_HEADER_HLEN_LEN,
					 2);

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_RID_START,
		     CW_TRANSPORT_HEADER_RID_LEN,
		     0); // only one radio per WTP?
	
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_WBID_START,
		     CW_TRANSPORT_HEADER_WBID_LEN,
		     1); // Wireless Binding ID
	
	
	
	
	if(valuesPtr->bindingValuesPtr != NULL)
		CWSetField32(val,
		     CW_TRANSPORT_HEADER_T_START,
		     CW_TRANSPORT_HEADER_T_LEN,
		     1);
	
	else if(valuesPtr->type==1)
		CWSetField32(val,
		     CW_TRANSPORT_HEADER_T_START,
		     CW_TRANSPORT_HEADER_T_LEN,
		     1);
	else 
		CWSetField32(val,
		     CW_TRANSPORT_HEADER_T_START,
		     CW_TRANSPORT_HEADER_T_LEN,
		     0);
	

	
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_F_START,
		     CW_TRANSPORT_HEADER_F_LEN,
		     valuesPtr->isFragment); // is fragment

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_L_START,
		     CW_TRANSPORT_HEADER_L_LEN,
		     valuesPtr->last); // last fragment
	
	if(valuesPtr->bindingValuesPtr != NULL)
		CWSetField32(val,
			     CW_TRANSPORT_HEADER_W_START,
			     CW_TRANSPORT_HEADER_W_LEN,
			     1); //wireless header
	else 
		CWSetField32(val,
			     CW_TRANSPORT_HEADER_W_START,
			     CW_TRANSPORT_HEADER_W_LEN,
			     0);

	if(radio_mac_present)
		CWSetField32(val,
				 CW_TRANSPORT_HEADER_M_START,
				 CW_TRANSPORT_HEADER_M_LEN,
				 1); //radio MAC address
	else
		CWSetField32(val,
				 CW_TRANSPORT_HEADER_M_START,
				 CW_TRANSPORT_HEADER_M_LEN,
				 0); // no radio MAC address
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_K_START,
		     CW_TRANSPORT_HEADER_K_LEN,
		     0); // Keep alive flag

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_FLAGS_START,
		     CW_TRANSPORT_HEADER_FLAGS_LEN,
		     0); // required

	CWProtocolStore32(transportHdrPtr, val);
	// end of first 32 bits
	
	val = 0;

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_FRAGMENT_ID_START,
		     CW_TRANSPORT_HEADER_FRAGMENT_ID_LEN,
		     valuesPtr->fragmentID); // fragment ID
	
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_START,
		     CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_LEN,
		     valuesPtr->fragmentOffset); // fragment offset

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_RESERVED_START,
		     CW_TRANSPORT_HEADER_RESERVED_LEN,
		     0); // required

	CWProtocolStore32(transportHdrPtr, val);
	// end of second 32 bits

	if(radio_mac_present){
		CWProtocolStore8(transportHdrPtr,6);
		
		CWThreadMutexLock(&gRADIO_MAC_mutex);		
			CWProtocolStoreRawBytes(transportHdrPtr,gRADIO_MAC,6);
		CWThreadMutexUnlock(&gRADIO_MAC_mutex);
		
		CWProtocolStore8(transportHdrPtr,0);
	}


	if(valuesPtr->bindingValuesPtr != NULL){
		if (!CWAssembleTransportHeaderBinding(transportHdrPtr, valuesPtr->bindingValuesPtr))
			return CW_FALSE;
	}

	return CW_TRUE;
}

// Assembles the Transport Header
CWBool CWAssembleTransportHeaderKeepAliveData(CWProtocolMessage *transportHdrPtr, CWProtocolTransportHeaderValues *valuesPtr, int keepAlive)
{
	
	unsigned int val = 0;
	if(transportHdrPtr == NULL || valuesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(valuesPtr->bindingValuesPtr != NULL)
		{CW_CREATE_PROTOCOL_MESSAGE(*transportHdrPtr,gMaxCAPWAPHeaderSizeBinding, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););}
	else {CW_CREATE_PROTOCOL_MESSAGE(*transportHdrPtr,8 , return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););}	 // meaningful bytes of the header (no wirless header and MAC address)
	CWSetField32(val, 
		     CW_TRANSPORT_HEADER_VERSION_START,
		     CW_TRANSPORT_HEADER_VERSION_LEN,
		     CW_PROTOCOL_VERSION); // current version of CAPWAP

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_TYPE_START,
		     CW_TRANSPORT_HEADER_TYPE_LEN,
		     (valuesPtr->payloadType == CW_PACKET_PLAIN) ? 0 : 1);
	
	if(valuesPtr->bindingValuesPtr != NULL)
		CWSetField32(val,
			     CW_TRANSPORT_HEADER_HLEN_START,
			     CW_TRANSPORT_HEADER_HLEN_LEN,
			     CW_BINDING_HLEN);
	else 
		CWSetField32(val,
			     CW_TRANSPORT_HEADER_HLEN_START,
			     CW_TRANSPORT_HEADER_HLEN_LEN,
			     2);

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_RID_START,
		     CW_TRANSPORT_HEADER_RID_LEN,
		     0); // only one radio per WTP?
	
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_WBID_START,
		     CW_TRANSPORT_HEADER_WBID_LEN,
		     1); // Wireless Binding ID

	if(valuesPtr->bindingValuesPtr != NULL)
		CWSetField32(val,
		     CW_TRANSPORT_HEADER_T_START,
		     CW_TRANSPORT_HEADER_T_LEN,
		     1);
	else 
		CWSetField32(val,
		     CW_TRANSPORT_HEADER_T_START,
		     CW_TRANSPORT_HEADER_T_LEN,
		     0);

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_F_START,
		     CW_TRANSPORT_HEADER_F_LEN,
		     valuesPtr->isFragment); // is fragment

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_L_START,
		     CW_TRANSPORT_HEADER_L_LEN,
		     valuesPtr->last); // last fragment
	
	if(valuesPtr->bindingValuesPtr != NULL)
		CWSetField32(val,
			     CW_TRANSPORT_HEADER_W_START,
			     CW_TRANSPORT_HEADER_W_LEN,
			     1); //wireless header
	else 
		CWSetField32(val,
			     CW_TRANSPORT_HEADER_W_START,
			     CW_TRANSPORT_HEADER_W_LEN,
			     0);

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_M_START,
		     CW_TRANSPORT_HEADER_M_LEN,
		     0); // no radio MAC address

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_K_START,
		     CW_TRANSPORT_HEADER_K_LEN,
		     keepAlive); // Keep alive flag
 
 	CWSetField32(val,
 		     CW_TRANSPORT_HEADER_FLAGS_START,
		     CW_TRANSPORT_HEADER_FLAGS_LEN,
		     0); // required

	CWProtocolStore32(transportHdrPtr, val);
	// end of first 32 bits
	
	val = 0;

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_FRAGMENT_ID_START,
		     CW_TRANSPORT_HEADER_FRAGMENT_ID_LEN,
		     valuesPtr->fragmentID); // fragment ID
	
	CWSetField32(val,
		     CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_START,
		     CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_LEN,
		     valuesPtr->fragmentOffset); // fragment offset

	CWSetField32(val,
		     CW_TRANSPORT_HEADER_RESERVED_START,
		     CW_TRANSPORT_HEADER_RESERVED_LEN,
		     0); // required

	CWProtocolStore32(transportHdrPtr, val);
	// end of second 32 bits

	if(valuesPtr->bindingValuesPtr != NULL){
		if (!CWAssembleTransportHeaderBinding(transportHdrPtr, valuesPtr->bindingValuesPtr))
			return CW_FALSE;
	}

	return CW_TRUE;
}

// Assembles the Control Header
CWBool CWAssembleControlHeader(CWProtocolMessage *controlHdrPtr, CWControlHeaderValues *valPtr) {
	if(controlHdrPtr == NULL || valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MESSAGE(*controlHdrPtr, 8,	 // meaningful bytes of the header
						return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore32(controlHdrPtr, valPtr->messageTypeValue);
	CWProtocolStore8(controlHdrPtr, valPtr->seqNum);
	CWProtocolStore16(controlHdrPtr, (CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS+(valPtr->msgElemsLen))); // 7 is for the next 8+32+16 bits (= 7 bytes), MessageElementsLength+flags + timestamp 
	CWProtocolStore8(controlHdrPtr, 0); // flags
	//CWProtocolStore32(controlHdrPtr, ((unsigned int)(time(NULL))) ); // timestamp
	
	return CW_TRUE;
}

/*Update 2009:
	Attach a payload with a result code to the message */
CWBool CWAssembleVendorMsgElemResultCodeWithPayload(CWProtocolMessage *msgPtr, CWProtocolResultCode code, CWProtocolVendorSpecificValues *payload) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	int payloadSize = 0;

	CWVendorUciValues *uciPayload;
	CWVendorWumValues *wumPayload;

	switch (payload->vendorPayloadType) {
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:
			uciPayload = (CWVendorUciValues *) payload->payload;
			if (uciPayload->response != NULL)
				payloadSize = (strlen(uciPayload->response)*sizeof(unsigned char));
		break;	
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:
			wumPayload = (CWVendorWumValues *) payload->payload;
			payloadSize = sizeof(unsigned char); /* default, only type */
			if (wumPayload->type == WTP_VERSION_RESPONSE)
				payloadSize = sizeof(unsigned char) * 4;
		break;
	}
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, (sizeof(unsigned short)*2)+(sizeof(unsigned int)*2)+payloadSize, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore32(msgPtr, code);
//	CWDebugLog("Result Code: %d", code);
//
	switch (payload->vendorPayloadType) {
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:
			/*Store what type of payload we have*/
			CWProtocolStore16(msgPtr, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_CW_TYPE);
			/*Store what type of vendor payload we have*/
			CWProtocolStore16(msgPtr, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI);
			/*Store payload size */
			CWProtocolStore32(msgPtr, payloadSize);
			if (uciPayload->response != NULL)
				/*Store the payload*/
				CWProtocolStoreStr(msgPtr, uciPayload->response);
		break;
		
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:
			/* Store what type of payload we have */
			CWProtocolStore16(msgPtr, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_CW_TYPE);
			/* Store what type of vendor payload we have */
			CWProtocolStore16(msgPtr, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM);
			/* Store payload size */ 
			CWProtocolStore32(msgPtr, payloadSize);
			
			CWProtocolStore8(msgPtr, wumPayload->type);

			if (wumPayload->type == WTP_VERSION_RESPONSE) {		
				CWProtocolStore8(msgPtr, wumPayload->_major_v_);
				CWProtocolStore8(msgPtr, wumPayload->_minor_v_);
				CWProtocolStore8(msgPtr, wumPayload->_revision_v_);
			}
		break;
	}
				
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE_WITH_PAYLOAD);
}


CWBool CWAssembleMsgElemResultCode(CWProtocolMessage *msgPtr, CWProtocolResultCode code) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 4, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore32(msgPtr, code);
//	CWDebugLog("Result Code: %d", code);
				
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE);
}

// Assemble a CAPWAP Control Packet, with the given Message Elements, Sequence Number and Message Type. Create Transport and Control Headers.
// completeMsgPtr is an array of fragments (can be of size 1 if the packet doesn't need fragmentation
CWBool CWAssembleMessage(CWProtocolMessage **completeMsgPtr, int *fragmentsNumPtr, int PMTU, int seqNum, int msgTypeValue, CWProtocolMessage *msgElems, const int msgElemNum, CWProtocolMessage *msgElemsBinding, const int msgElemBindingNum, int is_crypted) {
	CWProtocolMessage transportHdr, controlHdr, msg;
	int msgElemsLen = 0;
	int i;
	
	CWProtocolTransportHeaderValues transportVal;
	CWControlHeaderValues controlVal;
	
	if(completeMsgPtr == NULL || fragmentsNumPtr == NULL || (msgElems == NULL && msgElemNum > 0) || (msgElemsBinding == NULL && msgElemBindingNum > 0)) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	//Calculate the whole size of the Msg Elements	
	for(i = 0; i < msgElemNum; i++) msgElemsLen += msgElems[i].offset;
	for(i = 0; i < msgElemBindingNum; i++) msgElemsLen += msgElemsBinding[i].offset;

	//Assemble Control Header
	controlVal.messageTypeValue = msgTypeValue;
	controlVal.msgElemsLen = msgElemsLen;
	controlVal.seqNum = seqNum;
	
	if(!(CWAssembleControlHeader(&controlHdr, &controlVal))) {
		CW_FREE_PROTOCOL_MESSAGE(controlHdr);
		for(i = 0; i < msgElemNum; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
		CW_FREE_OBJECT(msgElems);
		for(i = 0; i < msgElemBindingNum; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElemsBinding[i]);}
		CW_FREE_OBJECT(msgElemsBinding);
		return CW_FALSE; // will be handled by the caller
	}
	
	// assemble the message putting all the data consecutively
	CW_CREATE_PROTOCOL_MESSAGE(msg, controlHdr.offset + msgElemsLen, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStoreMessage(&msg, &controlHdr);
	for(i = 0; i < msgElemNum; i++) { // store in the request all the Message Elements
		CWProtocolStoreMessage(&msg, &(msgElems[i]));
	}
	for(i = 0; i < msgElemBindingNum; i++) { // store in the request all the Message Elements
		CWProtocolStoreMessage(&msg, &(msgElemsBinding[i]));
	}

	//Free memory not needed anymore
	CW_FREE_PROTOCOL_MESSAGE(controlHdr);
	for(i = 0; i < msgElemNum; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
	CW_FREE_OBJECT(msgElems);
	for(i = 0; i < msgElemBindingNum; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElemsBinding[i]);}
	CW_FREE_OBJECT(msgElemsBinding);
	
//	CWDebugLog("PMTU: %d", PMTU);
	
	// handle fragmentation
	PMTU = PMTU - gMaxDTLSHeaderSize - gMaxCAPWAPHeaderSize;
	
	if(PMTU > 0) {
		PMTU = (PMTU/8)*8; // CAPWAP fragments are made of groups of 8 bytes
		if(PMTU == 0) goto cw_dont_fragment;
		
		//CWDebugLog("Aligned PMTU: %d", PMTU);
		*fragmentsNumPtr = msg.offset / PMTU;
		if((msg.offset % PMTU) != 0) (*fragmentsNumPtr)++;
		//CWDebugLog("Fragments #: %d", *fragmentsNumPtr);
	} else {
	cw_dont_fragment:
		*fragmentsNumPtr = 1;
	}
	
	transportVal.bindingValuesPtr = NULL;
		
	if(*fragmentsNumPtr == 1) {
		CWDebugLog("1 Fragment");

		CW_CREATE_OBJECT_ERR(*completeMsgPtr, CWProtocolMessage, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
		transportVal.isFragment = transportVal.last = transportVal.fragmentOffset = transportVal.fragmentID = 0;
		transportVal.payloadType = is_crypted;
//		transportVal.last = 1;

		// Assemble Message Elements
		if	(!(CWAssembleTransportHeader(&transportHdr, &transportVal))) {
			CW_FREE_PROTOCOL_MESSAGE(msg);
			CW_FREE_PROTOCOL_MESSAGE(transportHdr);
			CW_FREE_OBJECT(completeMsgPtr);
			return CW_FALSE; // will be handled by the caller
		} 
	
		// assemble the message putting all the data consecutively
		CW_CREATE_PROTOCOL_MESSAGE(((*completeMsgPtr)[0]), transportHdr.offset + msg.offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
		CWProtocolStoreMessage(&((*completeMsgPtr)[0]), &transportHdr);
		CWProtocolStoreMessage(&((*completeMsgPtr)[0]), &msg);
		
		CW_FREE_PROTOCOL_MESSAGE(transportHdr);
		CW_FREE_PROTOCOL_MESSAGE(msg);
	} else {
		int fragID = CWGetFragmentID();
		int totalSize = msg.offset;
		//CWDebugLog("%d Fragments", *fragmentsNumPtr);

		CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(*completeMsgPtr, *fragmentsNumPtr, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		msg.offset = 0;
	
		for(i = 0; i < *fragmentsNumPtr; i++) { // for each fragment to assemble
			int fragSize;
			
			transportVal.isFragment = 1;
			transportVal.fragmentOffset = msg.offset / 8;
			transportVal.fragmentID = fragID;
			transportVal.payloadType = is_crypted;

			if(i < ((*fragmentsNumPtr)-1)) { // not last fragment
				fragSize = PMTU;
				transportVal.last = 0;
			} else { // last fragment
				fragSize = totalSize - (((*fragmentsNumPtr)-1) * PMTU);
				transportVal.last = 1;
			}

			//CWDebugLog("Fragment #:%d, offset:%d, bytes stored:%d/%d", i, transportVal.fragmentOffset, fragSize, totalSize);
			
			// Assemble Transport Header for this fragment
			if(!(CWAssembleTransportHeader(&transportHdr, &transportVal))) {
				CW_FREE_PROTOCOL_MESSAGE(msg);
				CW_FREE_PROTOCOL_MESSAGE(transportHdr);
				CW_FREE_OBJECT(completeMsgPtr);
				return CW_FALSE; // will be handled by the caller
			}

			CW_CREATE_PROTOCOL_MESSAGE(((*completeMsgPtr)[i]), transportHdr.offset + fragSize, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
			
			CWProtocolStoreMessage(&((*completeMsgPtr)[i]), &transportHdr);
			CWProtocolStoreRawBytes(&((*completeMsgPtr)[i]), &((msg.msg)[msg.offset]), fragSize);
			msg.offset += fragSize;
			
			CW_FREE_PROTOCOL_MESSAGE(transportHdr);
		}
		CW_FREE_PROTOCOL_MESSAGE(msg);
	}
	
	return CW_TRUE;
}

void CWProtocolDestroyFragment(void *f) {
	CW_FREE_OBJECT(((CWProtocolFragment*)f)->data);
	CW_FREE_OBJECT(f);
}

CWBool CWCompareFragment(void *newFrag, void *oldFrag)
{
	CWProtocolFragment *newEl = (CWProtocolFragment *) newFrag;
	CWProtocolFragment *oldEl = (CWProtocolFragment *) oldFrag;

	if((newEl->transportVal.fragmentID == oldEl->transportVal.fragmentID) &&
	(newEl->transportVal.fragmentOffset == oldEl->transportVal.fragmentOffset))
	{return CW_TRUE;}

	return CW_FALSE;
}

// parse a sigle fragment. If it is the last fragment we need or the only fragment, return the reassembled message in
// *reassembleMsg. If we need at lest one more fragment, save this fragment in the list. You then call this function again
// with a new fragment and the same list untill we got all the fragments.
CWBool CWProtocolParseFragment(char *buf, int readBytes, CWList *fragmentsListPtr, CWProtocolMessage *reassembledMsg, CWBool *dataFlagPtr, char *RadioMAC) {
	
	CWProtocolTransportHeaderValues values;
	CWProtocolMessage msg;
	int totalSize;
	
	msg.msg = buf;
	msg.offset = 0;

	if(!CWParseTransportHeader(&msg, &values, dataFlagPtr, RadioMAC)){
		CWDebugLog("CWParseTransportHeader failed");
		return CW_FALSE;
	}
	if(values.isFragment == 0) { // single fragment

	/*	if(*fragmentsListPtr != NULL) { // we are receiving another fragmented message,
			return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Received Fragment with Different ID"); // discard this packet
		}
	*/
		CW_CREATE_PROTOCOL_MESSAGE(*reassembledMsg, (readBytes-msg.offset), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

		CWProtocolStoreRawBytes(reassembledMsg, &(buf[msg.offset]), (readBytes-msg.offset));
		reassembledMsg->data_msgType=msg.data_msgType;
		return CW_TRUE;
	} else {
		CWListElement *el;
		CWProtocolFragment *fragPtr;
		int currentSize;

		CW_CREATE_OBJECT_ERR(fragPtr, CWProtocolFragment, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

		fragPtr->transportVal.fragmentID = values.fragmentID;
		fragPtr->transportVal.fragmentOffset = values.fragmentOffset;
		fragPtr->transportVal.last = values.last;

		CWDebugLog("Received Fragment ID:%d, offset:%d, notLast:%d", fragPtr->transportVal.fragmentID,fragPtr->transportVal.fragmentOffset, fragPtr->transportVal.last);
	
		fragPtr->dataLen = (readBytes-msg.offset);

		if( *fragmentsListPtr == NULL ||  // empty list
		  (((CWProtocolFragment*)((*fragmentsListPtr)->data))->transportVal.fragmentID) == fragPtr->transportVal.fragmentID) // this fragment is in the set of fragments we are receiving
/*		{
			CW_CREATE_OBJECT_SIZE_ERR(fragPtr->data, fragPtr->dataLen, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
			CW_COPY_MEMORY(fragPtr->data, &(buf[msg.offset]), fragPtr->dataLen);
	
			if(!CWAddElementToList(fragmentsListPtr, fragPtr)) {
				CWDeleteList(fragmentsListPtr, CWProtocolDestroyFragment);
				CW_FREE_OBJECT(fragPtr);
				return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
			}
		}*/
		{
			CWListElement *aux = NULL;
			aux = CWSearchInList(*fragmentsListPtr, fragPtr, CWCompareFragment);
			if(aux == NULL) 
			{
				CW_CREATE_OBJECT_SIZE_ERR(fragPtr->data, fragPtr->dataLen, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				CW_COPY_MEMORY(fragPtr->data, &(buf[msg.offset]), fragPtr->dataLen);
	
				if(!CWAddElementToList(fragmentsListPtr, fragPtr)) {
					CWDeleteList(fragmentsListPtr, CWProtocolDestroyFragment);
					CW_FREE_OBJECT(fragPtr);
					return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
				}
			}
			else{
				CWDebugLog("Received a copy of a fragment already in List");
				CW_FREE_OBJECT(fragPtr);
				return CWErrorRaise(CW_ERROR_NEED_RESOURCE, NULL);
			}	
		} 
		else { 
			CWDebugLog("Discarded old fragments for different fragment ID: %d Vs %d", fragPtr->transportVal.fragmentID, (((CWProtocolFragment*)((*fragmentsListPtr)->data))->transportVal).fragmentID);
			CWDeleteList(fragmentsListPtr, CWProtocolDestroyFragment);
			CW_CREATE_OBJECT_SIZE_ERR(fragPtr->data, fragPtr->dataLen, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
			CW_COPY_MEMORY(fragPtr->data, &(buf[msg.offset]), fragPtr->dataLen);
			if(!CWAddElementToList(fragmentsListPtr, fragPtr)) {
				CWDeleteList(fragmentsListPtr, CWProtocolDestroyFragment);
				CW_FREE_OBJECT(fragPtr);
				return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
			}
		}

		// check if we have all the fragments
		for(el = *fragmentsListPtr, totalSize = 0; el != NULL; el = el->next) {
			if( (((CWProtocolFragment*)(el->data))->transportVal.last) == 1) { // last element
				totalSize = (((CWProtocolFragment*)(el->data))->transportVal.fragmentOffset) * 8;
				totalSize += (((CWProtocolFragment*)(el->data))->dataLen);
			}
		}

		if(totalSize == 0) { // we haven't the last fragment
			return CWErrorRaise(CW_ERROR_NEED_RESOURCE, NULL); // we need at least one more fragment
		}
		
		// calculate the size of all the fragments we have so far
		for(el = *fragmentsListPtr, currentSize = 0; el != NULL; el = el->next) {
			currentSize += (((CWProtocolFragment*)(el->data))->dataLen);
			//CWDebugLog("size %d/%d", currentSize, totalSize);
		}
		
		CWDebugLog("totalSize = %d , currentSize = %d", totalSize, currentSize);
		
		if(currentSize != totalSize) {
			return CWErrorRaise(CW_ERROR_NEED_RESOURCE, NULL); // we need at least one mpre fragment
		} else {
			int currentOffset = 0;
		
			CWLog("_______________________");
			CWDebugLog("Received All Fragments");
			
			CW_CREATE_PROTOCOL_MESSAGE(*reassembledMsg, (totalSize), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

			CW_REPEAT_FOREVER {
				CWBool found = CW_FALSE;
				
				// find the fragment in the list with the currend offset
				for(el = *fragmentsListPtr, currentSize = 0; el != NULL; el = el->next) {
					if( (((CWProtocolFragment*)(el->data))->transportVal.fragmentOffset) == currentOffset) {
						found = CW_TRUE;
						break;
					}
				}
			
				if(!found) { // mmm... we should have all the fragment, but we haven't a fragment for the current offset
					CWDeleteList(fragmentsListPtr, CWProtocolDestroyFragment);
					CW_FREE_PROTOCOL_MESSAGE(*reassembledMsg);
					return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Bad Fragmented Messsage");
				}
				
				CWProtocolStoreRawBytes(reassembledMsg, (((CWProtocolFragment*)(el->data))->data), (((CWProtocolFragment*)(el->data))->dataLen));
				reassembledMsg->data_msgType=msg.data_msgType;

				if( (((CWProtocolFragment*)(el->data))->transportVal.last) == 1) { // last fragment
					CWDebugLog("Message Reassembled");
					
					CWDeleteList(fragmentsListPtr, CWProtocolDestroyFragment);
					return CW_TRUE;
				}

				currentOffset += ((((CWProtocolFragment*)(el->data))->dataLen) / 8);
			}
		}
	}
}

// Parse Transport Header
CWBool CWParseTransportHeader(CWProtocolMessage *msgPtr, CWProtocolTransportHeaderValues *valuesPtr, CWBool *dataFlagPtr, char *RadioMAC) {
	
	int transport4BytesLen; 
	int val;
	int optionalWireless = 0;
	int version, rid;
	int m=0;
	

	if(msgPtr == NULL || valuesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	//CWDebugLog("Parse Transport Header");
	val = CWProtocolRetrieve32(msgPtr);
	
	if(CWGetField32(val, CW_TRANSPORT_HEADER_VERSION_START, CW_TRANSPORT_HEADER_VERSION_LEN) != CW_PROTOCOL_VERSION)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Wrong Protocol Version");
		
	version = CWGetField32(val, CW_TRANSPORT_HEADER_VERSION_START, CW_TRANSPORT_HEADER_VERSION_LEN);
	CWDebugLog("VERSION: %d", version);
	
	valuesPtr->payloadType = CWGetField32(val, CW_TRANSPORT_HEADER_TYPE_START, CW_TRANSPORT_HEADER_TYPE_LEN);
	CWDebugLog("PAYLOAD TYPE: %d", valuesPtr->payloadType);
	
	
	transport4BytesLen = CWGetField32(val,	CW_TRANSPORT_HEADER_HLEN_START, CW_TRANSPORT_HEADER_HLEN_LEN);
	CWDebugLog("HLEN: %d", transport4BytesLen);	

	rid = CWGetField32(val, CW_TRANSPORT_HEADER_RID_START, CW_TRANSPORT_HEADER_RID_LEN);
	CWDebugLog("RID: %d", rid);	
	
	CWDebugLog("WBID: %d", CWGetField32(val, CW_TRANSPORT_HEADER_WBID_START, CW_TRANSPORT_HEADER_WBID_LEN));
	
	valuesPtr->type = CWGetField32(val, CW_TRANSPORT_HEADER_T_START, CW_TRANSPORT_HEADER_T_LEN);
	CWDebugLog("TYPE: %d", valuesPtr->type);
	//CWDebugLog("TYPE: %d", valuesPtr->type);
	
	valuesPtr->isFragment = CWGetField32(val, CW_TRANSPORT_HEADER_F_START, CW_TRANSPORT_HEADER_F_LEN);
	//CWDebugLog("IS FRAGMENT: %d", valuesPtr->isFragment);

	valuesPtr->last = CWGetField32(val, CW_TRANSPORT_HEADER_L_START, CW_TRANSPORT_HEADER_L_LEN);
	//CWDebugLog("NOT LAST: %d", valuesPtr->last);
	
	optionalWireless = CWGetField32(val, CW_TRANSPORT_HEADER_W_START, CW_TRANSPORT_HEADER_W_LEN);
//	CWDebugLog("OPTIONAL WIRELESS: %d", optionalWireless);
	m = CWGetField32(val, CW_TRANSPORT_HEADER_M_START, CW_TRANSPORT_HEADER_M_LEN);

	
	valuesPtr->keepAlive = CWGetField32(val, CW_TRANSPORT_HEADER_K_START, CW_TRANSPORT_HEADER_K_LEN);
//	CWDebugLog("KEEP ALIVE: %d", valuesPtr->keepAlive);

	val = CWProtocolRetrieve32(msgPtr);

	valuesPtr->fragmentID = CWGetField32(val, CW_TRANSPORT_HEADER_FRAGMENT_ID_START, CW_TRANSPORT_HEADER_FRAGMENT_ID_LEN);
//	CWDebugLog("FRAGMENT_ID: %d", valuesPtr->fragmentID);

	valuesPtr->fragmentOffset = CWGetField32(val, CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_START, CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_LEN);
//	CWDebugLog("FRAGMENT_OFFSET: %d", valuesPtr->fragmentOffset);

	valuesPtr->bindingValuesPtr = NULL;

	
	if (*dataFlagPtr == CW_TRUE){
		if (valuesPtr->keepAlive){	// Keep Alive packet
			CWDebugLog("Keep-Alive packet");
			msgPtr->data_msgType=CW_DATA_MSG_KEEP_ALIVE_TYPE;
		}else if (valuesPtr->type==0){	//IEEE 802.3 frame
			CWDebugLog("802.3 frame");
			if (optionalWireless){
				CW_CREATE_OBJECT_ERR( valuesPtr->bindingValuesPtr, CWBindingTransportHeaderValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
				if (!CWParseTransportHeaderBinding(msgPtr, valuesPtr->bindingValuesPtr)){
					CW_FREE_OBJECT(valuesPtr->bindingValuesPtr);
					return CW_FALSE;
				}
			}
			msgPtr->data_msgType=CW_IEEE_802_3_FRAME_TYPE;
			
		}else if (valuesPtr->type==1){	//IEEE 802.11 frame
			CWDebugLog("802.11 frame");
			if (optionalWireless){
				CW_CREATE_OBJECT_ERR( valuesPtr->bindingValuesPtr, CWBindingTransportHeaderValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
				if (!CWParseTransportHeaderBinding(msgPtr, valuesPtr->bindingValuesPtr)){
					CW_FREE_OBJECT(valuesPtr->bindingValuesPtr);
					return CW_FALSE;
				}
			}
			msgPtr->data_msgType=CW_IEEE_802_11_FRAME_TYPE;
		}else{
			CWLog("Todo: This should be a keep-alive data packet!!!!");
		}
		if( m ){
			//CW_CREATE_OBJECT_ERR( valuesPtr->MACValuesPtr, CWMACTransportHeaderValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
	
			if (!CWParseTransportHeaderMACAddress(msgPtr, RadioMAC)){
				//CW_FREE_OBJECT(valuesPtr->bindingValuesPtr);
				return CW_FALSE;
			}
		}
		
	}else{
		if(transport4BytesLen == 4 && optionalWireless == 1){
			*dataFlagPtr = CW_TRUE;
			CW_CREATE_OBJECT_ERR( valuesPtr->bindingValuesPtr, CWBindingTransportHeaderValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			if (!CWParseTransportHeaderBinding(msgPtr, valuesPtr->bindingValuesPtr)){
				CW_FREE_OBJECT(valuesPtr->bindingValuesPtr);
				return CW_FALSE;
			}
 		}else if( m ){
			//CW_CREATE_OBJECT_ERR( valuesPtr->MACValuesPtr, CWMACTransportHeaderValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
	
			if (!CWParseTransportHeaderMACAddress(msgPtr, RadioMAC)){
				//CW_FREE_OBJECT(valuesPtr->bindingValuesPtr);
				return CW_FALSE;
			}
		}
 	}

	CWDebugLog(NULL);
	
	return (transport4BytesLen == 2 || (transport4BytesLen == 4 && optionalWireless == 1) || m ) ? CW_TRUE : CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Malformed Transport Header"); //TEMP?
}

// Parse Control Header
CWBool CWParseControlHeader(CWProtocolMessage *msgPtr, CWControlHeaderValues *valPtr) {	
	unsigned char flags=0;

	if(msgPtr == NULL|| valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

//	CWDebugLog("Parse Control Header");
	valPtr->messageTypeValue = CWProtocolRetrieve32(msgPtr);
//	CWDebugLog("MESSAGE_TYPE: %u",	valPtr->messageTypeValue);

	valPtr->seqNum = CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("SEQUENCE_NUMBER: %u", valPtr->seqNum );

	valPtr->msgElemsLen = CWProtocolRetrieve16(msgPtr);
//	CWDebugLog("MESSAGE_ELEMENT_LENGTH: %u", valPtr->msgElemsLen );
	
	flags=CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("FLAGS: %u",	flags);
	
//	valPtr->timestamp = CWProtocolRetrieve32(msgPtr);
//	CWDebugLog("TIME_STAMP: %u",	valPtr->timestamp);

	CWDebugLog(NULL);
	
	return CW_TRUE;
}

//## Assemble a Message Response containing a Failure (Unrecognized Message) Result Code
CWBool CWAssembleUnrecognizedMessageResponse(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum, int msgType)
{
	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=1;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Unrecognized Message Response...");
	
	CW_CREATE_OBJECT_ERR(msgElems, CWProtocolMessage, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	if (!(CWAssembleMsgElemResultCode(msgElems,CW_PROTOCOL_FAILURE_UNRECOGNIZED_REQ))) {
		CW_FREE_OBJECT(msgElems);
		return CW_FALSE;
	}

	if(!(CWAssembleMessage(messagesPtr, fragmentsNumPtr, PMTU, seqNum, msgType, msgElems, msgElemCount, msgElemsBinding, msgElemBindingCount
#ifdef CW_NO_DTLS
		,CW_PACKET_PLAIN))) 
#else
		,CW_PACKET_CRYPT))) 
#endif
		return CW_FALSE;
	
	CWLog("Unrecognized Message Response Assembled");
	
	return CW_TRUE;
}


CWBool CWAssembleMsgElemSessionID(CWProtocolMessage *msgPtr, char * sessionID) {
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 16, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStoreRawBytes(msgPtr, sessionID, 16);
				
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_SESSION_ID_CW_TYPE);
}

CWBool CWParseACName(CWProtocolMessage *msgPtr, int len, char **valPtr) {	
	CWParseMessageElementStart();
	
	*valPtr = CWProtocolRetrieveStr(msgPtr, len);
	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
//	CWDebugLog("AC Name:%s", *valPtr);
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPRadioAdminState (CWProtocolMessage *msgPtr, int len, CWRadioAdminInfoValues *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->ID = CWProtocolRetrieve8(msgPtr);
	valPtr->state = CWProtocolRetrieve8(msgPtr);
	//valPtr->cause = CWProtocolRetrieve8(msgPtr);
	
//	CWDebugLog("WTP Radio Admin State: %d - %d - %d", valPtr->ID, valPtr->state, valPtr->cause);
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPRadioOperationalState (CWProtocolMessage *msgPtr, int len, CWRadioOperationalInfoValues *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->ID = CWProtocolRetrieve8(msgPtr);
	valPtr->state = CWProtocolRetrieve8(msgPtr);
	valPtr->cause = CWProtocolRetrieve8(msgPtr);
	
//	CWDebugLog("WTP Radio Operational State: %d - %d - %d", valPtr->ID, valPtr->state, valPtr->cause);
	
	CWParseMessageElementEnd();
}

CWBool CWParseFormatMsgElem(CWProtocolMessage *completeMsg,unsigned short int *type,unsigned short int *len)
{
	*type = CWProtocolRetrieve16(completeMsg);
	*len = CWProtocolRetrieve16(completeMsg);
	return CW_TRUE;
}

CWBool CWParseResultCode(CWProtocolMessage *msgPtr, int len, CWProtocolResultCode *valPtr) {
	CWParseMessageElementStart();
	
	*valPtr = CWProtocolRetrieve32(msgPtr);
//	CWDebugLog("Result Code: %d",	*valPtr);
	
	CWParseMessageElementEnd();
}

void CWWTPResetRadioStatistics(WTPRadioStatisticsInfo *radioStatistics)
{	
	radioStatistics->lastFailureType= UNKNOWN_TYPE;
	radioStatistics->resetCount= 0;
	radioStatistics->SWFailureCount= 0;
	radioStatistics->HWFailuireCount= 0;
	radioStatistics->otherFailureCount= 0;
	radioStatistics->unknownFailureCount= 0;
	radioStatistics->configUpdateCount= 0;
	radioStatistics->channelChangeCount= 0;
	radioStatistics->bandChangeCount= 0;
	radioStatistics->currentNoiseFloor= 0;
}

void CWFreeMessageFragments(CWProtocolMessage* messages, int fragmentsNum)
{
	int i;

	for(i=0; i<fragmentsNum; i++)
	{
		CW_FREE_PROTOCOL_MESSAGE(messages[i]);
	}
}

char * CWParseSessionID(CWProtocolMessage *msgPtr, int len)
{
	return CWProtocolRetrieveRawBytes(msgPtr,16);
}
