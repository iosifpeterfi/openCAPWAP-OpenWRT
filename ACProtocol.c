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


#include "CWAC.h"
#include "CWVendorPayloads.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif
unsigned char WTPRadioInformationType;

/*____________________________________________________________________________*/
/*  *****************************___ASSEMBLE___*****************************  */
/*Update 2009:
	Assemble protocol Configuration update request. 
	Mainly added to  manage vendor specific packets*/
CWBool CWProtocolAssembleConfigurationUpdateRequest(CWProtocolMessage **msgElems, int *msgElemCountPtr, int MsgElementType){
	int* iPtr;
	int k = -1;
	
	
	if(msgElems == NULL || msgElemCountPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
		return CW_FALSE;
	}

	*msgElemCountPtr =1;


	CWLog("Assembling Protocol Configuration Update Request...");

	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(*msgElems, *msgElemCountPtr, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	/* Selection of type of Conf Update Request */

	switch(MsgElementType) {
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:

				// Assemble Message Elements
				if (!(CWAssembleWTPVendorPayloadUCI(&(*msgElems[++k]))))
				{
					int i;
					for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(*msgElems[i]);}
					CW_FREE_OBJECT(*msgElems);
					return CW_FALSE; // error will be handled by the caller
				}
		break;
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:

                                // Assemble Message Elements
                                if (!(CWAssembleWTPVendorPayloadWUM(&(*msgElems[++k]))))
                                {
                                        int i;
                                        for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(*msgElems[i]);}
                                        CW_FREE_OBJECT(*msgElems);
                                        return CW_FALSE; // error will be handled by the caller
                                }
                break;
		default:
	  	{
			return CW_FALSE; // error will be handled by the caller
	  	}
	}

	CWLog("Protocol Configuration Update Request Assembled");

	return CW_TRUE;
}

CWBool CWAssembleMsgElemACWTPRadioInformation(CWProtocolMessage *msgPtr) {
	

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);;
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, 5, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, 0);	// Radio ID
	CWProtocolStore8(msgPtr, 0);	// Reserved
	CWProtocolStore8(msgPtr, 0);	// Reserved
	CWProtocolStore8(msgPtr, 0);	// Reserved		
	CWProtocolStore8(msgPtr, 0); 	// Radio Information Type ABGN
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE);
}

CWBool CWAssembleMsgElemACDescriptor(CWProtocolMessage *msgPtr) {
	CWACVendorInfos infos;
	int i=0, size=0;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);;
	
	if(!CWACGetVendorInfos(&infos)) { // get infos
		return CW_FALSE;
	}

	for(i = 0; i < infos.vendorInfosCount; i++) {
		size += (8 + ((infos.vendorInfos)[i]).length);
	}
	
	size += 12; // size of message in bytes (excluding vendor infos, already counted)
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, size, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore16(msgPtr, CWACGetStations()); // Number of mobile stations associated
	CWProtocolStore16(msgPtr, CWACGetLimit()); // Maximum number of mobile stations supported	
	CWProtocolStore16(msgPtr, CWACGetActiveWTPs()); // Number of WTPs active	
	CWProtocolStore16(msgPtr, CWACGetMaxWTPs()); // Maximum number of WTPs supported	
	CWProtocolStore8(msgPtr, CWACGetSecurity());
	CWProtocolStore8(msgPtr, CWACGetRMACField());
	CWProtocolStore8(msgPtr, 0);			//Reserved
	CWProtocolStore8(msgPtr, CWACGetDTLSPolicy()); 	// DTLS Policy
	
	for(i=0; i<infos.vendorInfosCount; i++) {
		CWProtocolStore32(msgPtr, ((infos.vendorInfos)[i].vendorIdentifier));
		CWProtocolStore16(msgPtr, ((infos.vendorInfos)[i].type));
		CWProtocolStore16(msgPtr, ((infos.vendorInfos)[i].length));
		if((infos.vendorInfos)[i].length == 4) {
			*((infos.vendorInfos)[i].valuePtr) = htonl(*((infos.vendorInfos)[i].valuePtr));
		}
		CWProtocolStoreRawBytes(msgPtr, (char*) ((infos.vendorInfos)[i].valuePtr), (infos.vendorInfos)[i].length);
	}
	
	CWACDestroyVendorInfos(&infos);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_AC_DESCRIPTOR_CW_TYPE);
}

CWBool CWAssembleMsgElemACIPv4List(CWProtocolMessage *msgPtr) 
{
	int *list;
	int count, i;
	const int IPv4_List_length=4;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!CWACGetACIPv4List(&list, &count)) return CW_FALSE;
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, IPv4_List_length*count, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < count; i++) {
		CWProtocolStore32(msgPtr, list[i]);
//		CWDebugLog("AC IPv4 List(%d): %d", i, list[i]);
	}
	
	CW_FREE_OBJECT(list);
				
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_AC_IPV4_LIST_CW_TYPE);
}

CWBool CWAssembleMsgElemACIPv6List (CWProtocolMessage *msgPtr) 
{
	struct in6_addr *list;
	const int IPv6_List_length=16;
	int count, i;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!CWACGetACIPv6List(&list, &count)) return CW_FALSE;
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, IPv6_List_length*count, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	/*--- ATTENZIONE! l'indirizzo ipv6 forse deve essere girato ---*/
	for(i = 0; i < count; i++) {
		CWProtocolStoreRawBytes(msgPtr, (char*)list[i].s6_addr, 16);
	}
	
	CW_FREE_OBJECT(list);

	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_AC_IPV6_LIST_CW_TYPE);
}

CWBool CWAssembleMsgElemACName(CWProtocolMessage *msgPtr) {
	char *name;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	name = CWACGetName();
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, strlen(name), return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStoreStr(msgPtr, name);
	
//	CWDebugLog("AC Name: %s", name);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_AC_NAME_CW_TYPE);
}

CWBool CWAssembleMsgElemAddWLAN(int radioID,CWProtocolMessage *msgPtr,unsigned char* recv_packet, int len_packet){
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len_packet, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStoreRawBytes(msgPtr,(char*)recv_packet,len_packet);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_IEEE80211_ADD_WLAN_CW_TYPE);
	
}

CWBool CWAssembleMsgElemDeleteWLAN(int radioID,CWProtocolMessage *msgPtr,unsigned char* recv_packet, int len_packet ){
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len_packet, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStoreRawBytes(msgPtr,(char*)recv_packet,len_packet);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_IEEE80211_DELETE_WLAN_CW_TYPE);
}

CWBool CWAssembleMsgElemAddStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr)
{
	const int add_Station_Length=8;
	int Length=6;  //mac address length in bytes (48 bit)

	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, add_Station_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, radioID);
	
	CWProtocolStore8(msgPtr, Length);

	CWProtocolStoreRawBytes(msgPtr,(char*)StationMacAddr,Length);
	
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_ADD_STATION_CW_TYPE);
	
}

CWBool CWAssembleMsgElemDeleteStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr){
	const int delete_Station_Length=8;
	int Length=6;  //mac address length in bytes (48 bit)

	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, delete_Station_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	CWProtocolStore8(msgPtr, radioID);
	CWProtocolStore8(msgPtr, Length);
	CWProtocolStoreRawBytes(msgPtr,(char*)StationMacAddr,Length);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_DELETE_STATION_CW_TYPE);
}


CWBool CWAssembleMsgElemCWControlIPv4Addresses(CWProtocolMessage *msgPtr) {
	int count, i;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	count = CWACGetInterfacesCount();
	
	if(count <= 0) {
		return CWErrorRaise(CW_ERROR_NEED_RESOURCE, "No Interfaces Configured");
	}
	
	for(i = 0; i < count; i++) { // one Message Element for each interface
		CWProtocolMessage temp;
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(temp, 6, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
		CWProtocolStore32(&temp, CWACGetInterfaceIPv4AddressAtIndex(i));
		CWProtocolStore16(&temp, CWACGetInterfaceWTPCountAtIndex(i));
		
		CWAssembleMsgElem(&temp, CW_MSG_ELEMENT_CW_CONTROL_IPV4_ADDRESS_CW_TYPE);
		
		if(i == 0) {
			CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, (temp.offset)*count, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		}
		
		CWProtocolStoreMessage(msgPtr, &temp);
		CW_FREE_PROTOCOL_MESSAGE(temp);
	}
	
	return CW_TRUE;
}

CWBool CWAssembleMsgElemCWControlIPv6Addresses(CWProtocolMessage *msgPtr) {
	int count, i;
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	count = CWACGetInterfacesCount();
	
	for(i = 0; i < count; i++) { // one Message Element for each interface
		CWProtocolMessage temp;
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(temp, 18, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
		CWProtocolStoreRawBytes(&temp, CWACGetInterfaceIPv6AddressAtIndex(i), 16);
		CWProtocolStore16(&temp, CWACGetInterfaceWTPCountAtIndex(i));
		
		CWAssembleMsgElem(&temp, CW_MSG_ELEMENT_CW_CONTROL_IPV6_ADDRESS_CW_TYPE);
		
		if(i == 0) {
			CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, (temp.offset)*count, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		}
		
		CWProtocolStoreMessage(msgPtr, &temp);
		CW_FREE_PROTOCOL_MESSAGE(temp);
	}
	
	return CW_TRUE;
}

CWBool CWAssembleMsgElemCWTimer (CWProtocolMessage *msgPtr) 
{
	int discoveryTimer, echoTimer;
	const int CWTimer_length=2;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, CWTimer_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	if(!(CWACGetDiscoveryTimer(&discoveryTimer)) || !(CWACGetEchoRequestTimer(&echoTimer))) return CW_FALSE;
	CWProtocolStore8(msgPtr, discoveryTimer);
	CWProtocolStore8(msgPtr, echoTimer);
	
//	CWDebugLog("Discovery Timer: %d", discoveryTimer);
//	CWDebugLog("Echo Timer: %d", echoTimer);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_CW_TIMERS_CW_TYPE);
}

/* Le informazioni sui Radio ID vengono prese dalle informazioni del Configure Message 
   Provvisoriamente l'error Report Period è settato allo stesso valore per tutte le radio del WTP*/
CWBool CWAssembleMsgElemDecryptErrorReportPeriod (CWProtocolMessage *msgPtr)
{
	const int radio_Decrypt_Error_Report_Period_Length=3;
	const int reportInterval=15;
	CWProtocolMessage *msgs;
	CWRadioAdminInfoValues *radiosInfoPtr;
	int radioCount=0;
	int *iPtr;
	int len = 0;
	int i;
	int j;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((iPtr = ((int*)CWThreadGetSpecific(&gIndexSpecific))) == NULL) {
		CWLog("Critical Error... closing thread");
		CWCloseThread();
	}
	
	radiosInfoPtr=gWTPs[*iPtr].WTPProtocolManager.radioAdminInfo.radios;
	radioCount=gWTPs[*iPtr].WTPProtocolManager.radioAdminInfo.radiosCount;
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgs, radioCount, return  CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for (i=0; i<radioCount; i++)
	{
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], radio_Decrypt_Error_Report_Period_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), radiosInfoPtr[i].ID); // ID of the radio
		CWProtocolStore16(&(msgs[i]), reportInterval); // state of the radio
		
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_PERIOD_CW_TYPE))) {
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
		
		len += msgs[i].offset;
//		CWDebugLog("Decrypt Error Report Period: %d - %d", radiosInfoPtr[i].ID, reportInterval);
	}
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < radioCount; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	CW_FREE_OBJECT(msgs);

	return CW_TRUE;
}

CWBool CWAssembleMsgElemIdleTimeout (CWProtocolMessage *msgPtr)
{
	int idleTimeout;
	const int idle_Timeout_length=4;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, idle_Timeout_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	if(!(CWACGetIdleTimeout(&idleTimeout))) return CW_FALSE;
	CWProtocolStore32(msgPtr, idleTimeout);
	
//	CWDebugLog("Idle Timeout: %d", idleTimeout);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_IDLE_TIMEOUT_CW_TYPE);
}


CWBool CWAssembleMsgElemWTPFallback (CWProtocolMessage *msgPtr)
{
	int value=0; //PROVVISORIO
	const int WTP_fallback_length=1;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	// create message
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, WTP_fallback_length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	CWProtocolStore8(msgPtr, value);
	
//	CWDebugLog("Fallback: %d", value);
	
	return CWAssembleMsgElem(msgPtr, CW_MSG_ELEMENT_WTP_FALLBACK_CW_TYPE);
}

CWBool CWAssembleMsgElemRadioOperationalState(int radioID, CWProtocolMessage *msgPtr) 
{
	const int radio_Operational_State_Length=3;
	CWRadiosOperationalInfo infos;
	CWProtocolMessage *msgs;
	int len = 0;
	int i;
	int j;
	
	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!(CWGetWTPRadiosOperationalState(radioID, &infos))) {
		return CW_FALSE;
	}
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgs, (infos.radiosCount), return  CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		// create message
		CW_CREATE_PROTOCOL_MESSAGE(msgs[i], radio_Operational_State_Length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		CWProtocolStore8(&(msgs[i]), infos.radios[i].ID); // ID of the radio
		CWProtocolStore8(&(msgs[i]), infos.radios[i].state); // state of the radio
		CWProtocolStore8(&(msgs[i]), infos.radios[i].cause);
		
		if(!(CWAssembleMsgElem(&(msgs[i]), CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE))) {
			for(j = i; j >= 0; j--) { CW_FREE_PROTOCOL_MESSAGE(msgs[j]);}
			CW_FREE_OBJECT(infos.radios);
			CW_FREE_OBJECT(msgs);
			return CW_FALSE;
		}
		
		len += msgs[i].offset;
//		CWDebugLog("Radio operational State: %d - %d - %d", infos.radios[i].ID, infos.radios[i].state, infos.radios[i].cause);
	}
	
	CW_CREATE_PROTOCOL_MESSAGE(*msgPtr, len, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < infos.radiosCount; i++) {
		CWProtocolStoreMessage(msgPtr, &(msgs[i]));
		CW_FREE_PROTOCOL_MESSAGE(msgs[i]);
	}
	
	CW_FREE_OBJECT(msgs);
	CW_FREE_OBJECT(infos.radios);

	return CW_TRUE;
}


/*_________________________________________________________________________*/
/*  *****************************___PARSE___*****************************  */
CWBool CWParseACNameWithIndex (CWProtocolMessage *msgPtr, int len, CWACNameWithIndexValues *valPtr)
{
	CWParseMessageElementStart();

	valPtr->index = CWProtocolRetrieve8(msgPtr);
	//CWDebugLog("CW_MSG_ELEMENT_WTP_RADIO_ID: %d",	(valPtr->radios)[radioIndex].ID);
	
	valPtr->ACName = CWProtocolRetrieveStr(msgPtr,len-1);
	//CWDebugLog("CW_MSG_ELEMENT_WTP_RADIO_TYPE: %d",	(valPtr->radios)[radioIndex].type);
	
	//CWDebugLog("AC Name with index: %d - %s", valPtr->index, valPtr->ACName);
	
	CWParseMessageElementEnd();
}

CWBool CWParseDiscoveryType(CWProtocolMessage *msgPtr, int len, CWDiscoveryRequestValues *valPtr) {	
	CWParseMessageElementStart();
	
			
	valPtr->type = CWProtocolRetrieve8(msgPtr);
		
	CWParseMessageElementEnd();
}

CWBool CWParseLocationData(CWProtocolMessage *msgPtr, int len, char **valPtr) {	
	CWParseMessageElementStart();
	
	*valPtr = CWProtocolRetrieveStr(msgPtr, len);
	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
//	CWDebugLog("Location Data:%s", *valPtr);
	
	CWParseMessageElementEnd();
}

CWBool CWParseMsgElemDuplicateIPv4Address(CWProtocolMessage *msgPtr, int len, WTPDuplicateIPv4 *valPtr) 
{
	CWParseMessageElementStart();
	
	valPtr->ipv4Address =  CWProtocolRetrieve32(msgPtr);
	valPtr->status = CWProtocolRetrieve8(msgPtr);
	valPtr->length = CWProtocolRetrieve8(msgPtr);
	valPtr->MACoffendingDevice_forIpv4 = (unsigned char*)CWProtocolRetrieveRawBytes(msgPtr, valPtr->length);
	
	//valPtr->MACoffendingDevice_forIpv4 = (unsigned char*)CWProtocolRetrieveRawBytes(msgPtr,6);
	//valPtr->status = CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("Duplicate IPv4: %d", valPtr->ipv4Address);

	CWParseMessageElementEnd();
}

CWBool CWParseMsgElemDuplicateIPv6Address(CWProtocolMessage *msgPtr, int len, WTPDuplicateIPv6 *valPtr) 
{
	CWParseMessageElementStart();
	
	int i;
	for(i=0; i<16; i++)
	{	char *aux;
		aux= CWProtocolRetrieveRawBytes(msgPtr, 1);
		(valPtr->ipv6Address).s6_addr[i] = *aux;
	}

//	CWDebugLog("Duplicate IPv6");
	//valPtr->MACoffendingDevice_forIpv6 = (unsigned char*)CWProtocolRetrieveRawBytes(msgPtr,6);

	valPtr->status = CWProtocolRetrieve8(msgPtr);

	valPtr->length = CWProtocolRetrieve8(msgPtr);

	valPtr->MACoffendingDevice_forIpv6 = (unsigned char*)CWProtocolRetrieveRawBytes(msgPtr, valPtr->length);

	CWParseMessageElementEnd();
}

CWBool CWParseWTPStatisticsTimer (CWProtocolMessage *msgPtr, int len, int *valPtr)
{
	CWParseMessageElementStart();
	
	*valPtr = CWProtocolRetrieve16(msgPtr);
	
//	CWDebugLog("WTP Statistics Timer: %d", *valPtr);
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPBoardData (CWProtocolMessage *msgPtr, int len, CWWTPVendorInfos *valPtr)
{
	int theOffset, i, vendorID;
	CWParseMessageElementStart();
	
	valPtr->vendorInfosCount = 0;
	
	// see how many vendor ID we have in the message
	vendorID = CWProtocolRetrieve32(msgPtr); // ID
	theOffset = msgPtr->offset;
	while((msgPtr->offset-oldOffset) < len) {	// oldOffset stores msgPtr->offset's value at the beginning of this function.
							// See the definition of the CWParseMessageElementStart() macro.
		int tmp;

		CWProtocolRetrieve16(msgPtr); // type
		tmp = CWProtocolRetrieve16(msgPtr);
		msgPtr->offset += tmp; // len
		valPtr->vendorInfosCount++;
	}
	
	msgPtr->offset = theOffset;
	
	// actually read each vendor ID
	CW_CREATE_ARRAY_ERR(valPtr->vendorInfos, valPtr->vendorInfosCount, CWWTPVendorInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < valPtr->vendorInfosCount; i++) {
		(valPtr->vendorInfos)[i].vendorIdentifier = vendorID;
		(valPtr->vendorInfos)[i].type = CWProtocolRetrieve16(msgPtr);																
		(valPtr->vendorInfos)[i].length = CWProtocolRetrieve16(msgPtr);
		(valPtr->vendorInfos)[i].valuePtr = (CWProtocolRetrieveRawBytes(msgPtr, (valPtr->vendorInfos)[i].length));
		
		if((valPtr->vendorInfos)[i].valuePtr == NULL) return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		
		if((valPtr->vendorInfos)[i].length == 4) {
			*(int *)((valPtr->vendorInfos)[i].valuePtr) = ntohl(*((valPtr->vendorInfos)[i].valuePtr));
		}
		
//		CWDebugLog("WTP Board Data: %d - %d - %d - %d", (valPtr->vendorInfos)[i].vendorIdentifier, (valPtr->vendorInfos)[i].type, (valPtr->vendorInfos)[i].length, *(valPtr->vendorInfos)[i].valuePtr);
	}
	
	CWParseMessageElementEnd();
}

CWBool CWParseMsgElemDataTransferData(CWProtocolMessage *msgPtr, int len, CWProtocolWTPDataTransferRequestValues *valPtr) 
{
	CWParseMessageElementStart();
	
	valPtr->data = CWProtocolRetrieve8(msgPtr);
	valPtr->length= CWProtocolRetrieve8(msgPtr);
	valPtr->debug_info = CWProtocolRetrieveStr(msgPtr, valPtr->length);
	//CWDebugLog("- %s ---",valPtr->debug_info);

	CWParseMessageElementEnd();
}

CWBool CWParseWTPDescriptor(CWProtocolMessage *msgPtr, int len, CWWTPDescriptor *valPtr) {
	int theOffset, i;
	CWParseMessageElementStart();

	valPtr->maxRadios = CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("WTP Descriptor Max Radios: %d", valPtr->maxRadios);
	
	valPtr->radiosInUse = CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("WTP Descriptor Active Radios: %d",	valPtr->radiosInUse);
		
	valPtr->encCapabilities	= CWProtocolRetrieve16(msgPtr);					
//	CWDebugLog("WTP Descriptor Encryption Capabilities: %d", valPtr->encCapabilities);
		
	valPtr->vendorInfos.vendorInfosCount = 0;
	
	theOffset = msgPtr->offset;
	
	// see how many vendor ID we have in the message
	while((msgPtr->offset-oldOffset) < len) {	// oldOffset stores msgPtr->offset's value at the beginning of this function.
												// See the definition of the CWParseMessageElementStart() macro.
		int tmp;
		CWProtocolRetrieve32(msgPtr); // ID
		CWProtocolRetrieve16(msgPtr); // type
		tmp = CWProtocolRetrieve16(msgPtr); // len
		msgPtr->offset += tmp;
		valPtr->vendorInfos.vendorInfosCount++;
	}
	
	msgPtr->offset = theOffset;
	
	// actually read each vendor ID
	CW_CREATE_ARRAY_ERR(valPtr->vendorInfos.vendorInfos, valPtr->vendorInfos.vendorInfosCount, CWWTPVendorInfoValues,
		return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for(i = 0; i < valPtr->vendorInfos.vendorInfosCount; i++) {
		(valPtr->vendorInfos.vendorInfos)[i].vendorIdentifier = CWProtocolRetrieve32(msgPtr);
		(valPtr->vendorInfos.vendorInfos)[i].type = CWProtocolRetrieve16(msgPtr);																
		(valPtr->vendorInfos.vendorInfos)[i].length = CWProtocolRetrieve16(msgPtr);
		(valPtr->vendorInfos.vendorInfos)[i].valuePtr = (CWProtocolRetrieveRawBytes(msgPtr, (valPtr->vendorInfos.vendorInfos)[i].length));
		
		if((valPtr->vendorInfos.vendorInfos)[i].valuePtr == NULL) return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		
		if((valPtr->vendorInfos.vendorInfos)[i].length == 4) {
			*((valPtr->vendorInfos.vendorInfos)[i].valuePtr) = ntohl(*((valPtr->vendorInfos.vendorInfos)[i].valuePtr));
		}
		
//		CWDebugLog("WTP Descriptor Vendor ID: %d", (valPtr->vendorInfos.vendorInfos)[i].vendorIdentifier);
//		CWDebugLog("WTP Descriptor Type: %d", (valPtr->vendorInfos.vendorInfos)[i].type);
//		CWDebugLog("WTP Descriptor Length: %d", (valPtr->vendorInfos.vendorInfos)[i].length);
//		CWDebugLog("WTP Descriptor Value: %d", *((valPtr->vendorInfos.vendorInfos)[i].valuePtr));
	}
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPFrameTunnelMode(CWProtocolMessage *msgPtr, int len, CWframeTunnelMode *valPtr) {	
	CWParseMessageElementStart();
	
	*valPtr = CWProtocolRetrieve8(msgPtr);					
//	CWDebugLog("CW_MSG_ELEMENT_WTP_FRAME_ENCAPSULATION_TYPE: %d", valPtr->frameTunnelMode);
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPIPv4Address(CWProtocolMessage *msgPtr, int len, CWProtocolJoinRequestValues *valPtr) {
	CWParseMessageElementStart();
	
	valPtr->addr.sin_addr.s_addr = htonl(CWProtocolRetrieve32(msgPtr));
	valPtr->addr.sin_family = AF_INET;
	valPtr->addr.sin_port = htons(CW_CONTROL_PORT);
//	CWDebugLog("WTP Address: %s", sock_ntop((struct sockaddr*) (&(valPtr->addr)), sizeof(valPtr->addr)));
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPMACType(CWProtocolMessage *msgPtr, int len, CWMACType *valPtr) {	
	CWParseMessageElementStart();
										
	*valPtr = CWProtocolRetrieve8(msgPtr);
//	CWDebugLog("CW_MSG_ELEMENT_WTP_MAC_TYPE: %d",	valPtr->MACType);
	
	CWParseMessageElementEnd();
}


CWBool CWParseWTPRadioInformation(CWProtocolMessage *msgPtr, int len, unsigned char *valPtr) {	

	CWParseMessageElementStart();
	int RadioID;
	
	RadioID = CWProtocolRetrieve8(msgPtr);	// Radio ID
	CWProtocolRetrieve8(msgPtr);	// Res
	CWProtocolRetrieve8(msgPtr);	// Res
	CWProtocolRetrieve8(msgPtr);	// Res
	*valPtr = CWProtocolRetrieve8(msgPtr);	// Radio Information 

	CWParseMessageElementEnd();							

}

CWBool CWParseWTPSupportedRates(CWProtocolMessage *msgPtr, int len, unsigned char *valPtr) {	

	CWParseMessageElementStart();
	int RadioID;
	unsigned char sup_rates[8];
	
	RadioID = CWProtocolRetrieve8(msgPtr);		
	
	sup_rates[0] = CWProtocolRetrieve8(msgPtr);
	sup_rates[1] = CWProtocolRetrieve8(msgPtr);
	sup_rates[2] = CWProtocolRetrieve8(msgPtr);
	sup_rates[3] = CWProtocolRetrieve8(msgPtr);
	sup_rates[4] = CWProtocolRetrieve8(msgPtr);
	sup_rates[5] = CWProtocolRetrieve8(msgPtr);
	sup_rates[6] = CWProtocolRetrieve8(msgPtr);
	sup_rates[7] = CWProtocolRetrieve8(msgPtr);
	
	memcpy(valPtr, sup_rates, 8);

	CWParseMessageElementEnd();							
}

CWBool CWParseWTPMultiDomainCapability(CWProtocolMessage *msgPtr, int len, char *valPtr) {	

	CWParseMessageElementStart();
	int RadioID;
	unsigned char sup_cap[6];
	
	RadioID = CWProtocolRetrieve8(msgPtr);		
			  CWProtocolRetrieve8(msgPtr);		
			  
	sup_cap[0] = CWProtocolRetrieve8(msgPtr);
	sup_cap[1] = CWProtocolRetrieve8(msgPtr);
	sup_cap[2] = CWProtocolRetrieve8(msgPtr);
	sup_cap[3] = CWProtocolRetrieve8(msgPtr);
	sup_cap[4] = CWProtocolRetrieve8(msgPtr);
	sup_cap[5] = CWProtocolRetrieve8(msgPtr);

	
	memcpy(valPtr, sup_cap, 6);
	
	CWParseMessageElementEnd();							
}

CWBool CWParseWTPName(CWProtocolMessage *msgPtr, int len, char **valPtr) {	
	CWParseMessageElementStart();
	
	*valPtr = CWProtocolRetrieveStr(msgPtr, len);
	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
//	CWDebugLog("WTP Name:%s", *valPtr);
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPRebootStatistics (CWProtocolMessage *msgPtr, int len, WTPRebootStatisticsInfo *valPtr)
{
	CWParseMessageElementStart();
	
	valPtr->rebootCount=CWProtocolRetrieve16(msgPtr);
	valPtr->ACInitiatedCount=CWProtocolRetrieve16(msgPtr);
	valPtr->linkFailurerCount=CWProtocolRetrieve16(msgPtr);
	valPtr->SWFailureCount=CWProtocolRetrieve16(msgPtr);
	valPtr->HWFailuireCount=CWProtocolRetrieve16(msgPtr);
	valPtr->otherFailureCount=CWProtocolRetrieve16(msgPtr);
	valPtr->unknownFailureCount=CWProtocolRetrieve16(msgPtr);
	valPtr->lastFailureType=CWProtocolRetrieve8(msgPtr);
	
//	CWDebugLog("");
//	CWDebugLog("WTPRebootStat(1): %d - %d - %d", valPtr->rebootCount, valPtr->ACInitiatedCount, valPtr->linkFailurerCount);
//	CWDebugLog("WTPRebootStat(2): %d - %d - %d", valPtr->SWFailureCount, valPtr->HWFailuireCount, valPtr->otherFailureCount);
//	CWDebugLog("WTPRebootStat(3): %d - %d", valPtr->unknownFailureCount, valPtr->lastFailureType);
	
	
	CWParseMessageElementEnd();
}

CWBool CWParseWTPRadioStatistics(CWProtocolMessage *msgPtr, int len, WTPRadioStatisticsValues *valPtr) 
{
	CWParseMessageElementStart();
	
	valPtr->radioID = CWProtocolRetrieve8(msgPtr);
	valPtr->WTPRadioStatistics.lastFailureType = CWProtocolRetrieve8(msgPtr);
	valPtr->WTPRadioStatistics.resetCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.SWFailureCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.HWFailuireCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.otherFailureCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.unknownFailureCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.configUpdateCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.channelChangeCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.bandChangeCount = CWProtocolRetrieve16(msgPtr);
	valPtr->WTPRadioStatistics.currentNoiseFloor = CWProtocolRetrieve16(msgPtr);
	
//	CWDebugLog("");
//	CWDebugLog("WTPRadioStatistics of radio: \"%d\"", valPtr->radioID);
//	CWDebugLog("WTPRadioStatistics(1): %d - %d - %d", valPtr->WTPRadioStatistics.lastFailureType, valPtr->WTPRadioStatistics.resetCount, valPtr->WTPRadioStatistics.SWFailureCount);
//	CWDebugLog("WTPRadioStatistics(2): %d - %d - %d", valPtr->WTPRadioStatistics.HWFailuireCount, valPtr->WTPRadioStatistics.otherFailureCount, valPtr->WTPRadioStatistics.unknownFailureCount);
//	CWDebugLog("WTPRadioStatistics(3): %d - %d - %d - %d", valPtr->WTPRadioStatistics.configUpdateCount, valPtr->WTPRadioStatistics.channelChangeCount, valPtr->WTPRadioStatistics.bandChangeCount, valPtr->WTPRadioStatistics.currentNoiseFloor);

	CWParseMessageElementEnd();
}

CWBool CWParseWTPOperationalStatistics(CWProtocolMessage *msgPtr, int len, WTPOperationalStatisticsValues *valPtr) 
{
	CWParseMessageElementStart();
	
	valPtr->radioID = CWProtocolRetrieve8(msgPtr);
	valPtr->TxQueueLevel = CWProtocolRetrieve8(msgPtr);
	valPtr->wirelessLinkFramesPerSec = CWProtocolRetrieve16(msgPtr);
	
//	CWDebugLog("WTPOperationalStatistics of radio \"%d\": %d - %d", valPtr->radioID, valPtr->TxQueueLevel, valPtr->wirelessLinkFramesPerSec);

	CWParseMessageElementEnd();
}


CWBool CWParseMsgElemDecryptErrorReport(CWProtocolMessage *msgPtr, int len, CWDecryptErrorReportValues *valPtr) 
{
	CWParseMessageElementStart();
	
	valPtr->ID = CWProtocolRetrieve8(msgPtr);
	valPtr->numEntries = CWProtocolRetrieve8(msgPtr);

	valPtr->length = CWProtocolRetrieve8(msgPtr);

	valPtr->decryptErrorMACAddressList = NULL;
	if((valPtr->numEntries) > 0)
	{
		CW_CREATE_ARRAY_ERR(valPtr->decryptErrorMACAddressList, valPtr->numEntries, CWMACAddress,  return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		int size=sizeof(CWMACAddress)*(valPtr->numEntries);
		CW_COPY_MEMORY(valPtr->decryptErrorMACAddressList, CWProtocolRetrieveRawBytes(msgPtr, size), size);
		//valPtr->decryptErrorMACAddressList =(unsigned char*) CWProtocolRetrieveRawBytes(msgPtr, sizeof(CWMACAddress)*(valPtr->numEntries));
		//CW_COPY_MEMORY(&((valPtr->ACIPv6List)[i]), CWProtocolRetrieveRawBytes(msgPtr, 16), 16);
		/*	
		int j;
		for (j=0;j<(sizeof(CWMACAddress)*(valPtr->numEntries)); j++)
			CWDebugLog("##(%d/6) = %d", j%6, (valPtr->decryptErrorMACAddressList)[j/6][j%6]);
		*/
	}
	
//	CWDebugLog("");
//	CWDebugLog("Radio Decrypt Error Report of radio \"%d\": %d", valPtr->ID, valPtr->numEntries);
		
	CWParseMessageElementEnd();
}

/*
CWBool CWParseWTPRadioInfo(CWPr<otocolMessage *msgPtr, int len, CWRadiosInformation *valPtr, int radioIndex) {	
	CWParseMessageElementStart();

	(valPtr->radios)[radioIndex].ID = CWProtocolRetrieve8(msgPtr);
	(valPtr->radios)[radioIndex].type = CWProtocolRetrieve32(msgPtr);
	
	CWDebugLog("WTP Radio info: %d %d ", (valPtr->radios)[radioIndex].ID, (valPtr->radios)[radioIndex].type);
	
	CWParseMessageElementEnd();
}
*/



