/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the free Software Foundation; either		*
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
 * Authors : Matteo Latini (mtylty@gmail.com)													*  
 *
 ************************************************************************************************/

#include "CWVendorPayloads.h"
#include "WUM.h"
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


CWBool CWAssembleWTPVendorPayloadUCI(CWProtocolMessage *msgPtr) {
	int* iPtr;
	unsigned short  msgType;
	CWProtocolVendorSpecificValues* valuesPtr;
	CWVendorUciValues* uciPtr;

	CWLog("Assembling Protocol Configuration Update Request [VENDOR CASE]...");

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
	  return CW_FALSE;
	}

	valuesPtr =gWTPs[*iPtr].vendorValues;
	switch (valuesPtr->vendorPayloadType){
			case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:
				msgType = CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI;
				uciPtr = (CWVendorUciValues *) valuesPtr->payload;
				if (uciPtr->commandArgs != NULL) {
					/* create message */
					CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, sizeof(short)+sizeof(char)+sizeof(int)+(strlen(uciPtr->commandArgs)*sizeof(char)), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
					CWProtocolStore16(msgPtr, (unsigned short) msgType);
					CWProtocolStore8(msgPtr, (unsigned char) uciPtr->command);
					CWProtocolStore32(msgPtr, (unsigned int) strlen(uciPtr->commandArgs));
					CWProtocolStoreStr(msgPtr, uciPtr->commandArgs);
				} else {
					/* create message */
					CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, sizeof(short)+sizeof(char)+sizeof(int), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
					CWProtocolStore16(msgPtr, (unsigned short) msgType);
					CWProtocolStore8(msgPtr, (unsigned char) uciPtr->command);
					CWProtocolStore32(msgPtr, 0);
				}
			break;
			default:
				return CW_FALSE;
			break;
	}
	CWLog("Assembling Protocol Configuration Update Request [VENDOR CASE]: Message Assembled.");

	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_CW_TYPE);
}

CWBool CWAssembleWTPVendorPayloadWUM(CWProtocolMessage *msgPtr) {
	int* iPtr;
	unsigned short  msgType;
	CWProtocolVendorSpecificValues* valuesPtr;
	CWVendorWumValues* wumPtr;

	CWLog("Assembling Protocol Configuration Update Request [VENDOR CASE]...");

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
	  return CW_FALSE;
	}

	valuesPtr =gWTPs[*iPtr].vendorValues;
	switch (valuesPtr->vendorPayloadType){
			case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:
				/* 
				 * Here we assemble the WTP Update Messages. 
 				 */
				msgType = CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM;
                                wumPtr = (CWVendorWumValues*) valuesPtr->payload;
				
				switch(wumPtr->type) {
				case WTP_VERSION_REQUEST:
				case WTP_COMMIT_UPDATE:
				case WTP_CANCEL_UPDATE_REQUEST:
					CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, sizeof(short)+sizeof(char), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				break;
				case WTP_UPDATE_REQUEST:
					CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, sizeof(short)+4*sizeof(char)+sizeof(unsigned int), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				break;
				case WTP_CUP_FRAGMENT:
					CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, sizeof(short)+sizeof(char)+2*sizeof(int)+wumPtr->_cup_fragment_size_, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				break;
				default:
					CWLog("Error! unknown WUM message type!!!");
					return CW_FALSE;
				}

                                CWProtocolStore16(msgPtr, (unsigned short) msgType);
                                CWProtocolStore8(msgPtr, (unsigned char) wumPtr->type);
				if (wumPtr->type == WTP_UPDATE_REQUEST) {
					CWProtocolStore8(msgPtr, wumPtr->_major_v_);	
					CWProtocolStore8(msgPtr, wumPtr->_minor_v_);	
					CWProtocolStore8(msgPtr, wumPtr->_revision_v_);	
					CWProtocolStore32(msgPtr, wumPtr->_pack_size_);	
				} else if (wumPtr->type == WTP_CUP_FRAGMENT) {
					CWProtocolStore32(msgPtr, wumPtr->_seq_num_);
					CWProtocolStore32(msgPtr, wumPtr->_cup_fragment_size_);
					CWProtocolStoreRawBytes(msgPtr, wumPtr->_cup_, wumPtr->_cup_fragment_size_);
					CW_FREE_OBJECT(wumPtr->_cup_);
				}
			break;
			default:
				return CW_FALSE;
			break;
	}
	CWLog("Assembling Protocol Configuration Update Request [VENDOR CASE]: Message Assembled.");

	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_CW_TYPE);
}
