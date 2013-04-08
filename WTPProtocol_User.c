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


#include "CWWTP.h"

#define MAC_ADDR_LEN		6

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif
 
__inline__ int CWWTPGetDiscoveryType() {
	return CW_MSG_ELEMENT_DISCOVERY_TYPE_CONFIGURED;
}

__inline__ int CWWTPGetMaxRadios() {
	return 1;
}

__inline__ int CWWTPGetRadiosInUse() 
{
	/*for (i=0; i<gRadiosInfo.radioCount; i++)
	{
		if((gRadiosInfo.radiosInfo[i].operationalState) == ENABLED)
			active++;
	}	
	return active;
	*/
	return gRadiosInfo.radioCount;
}

__inline__ int CWWTPGetEncCapabilities() {
	return 2569;
}


CWBool CWWTPGetBoardData(CWWTPVendorInfos *valPtr) {
	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	valPtr->vendorInfosCount = 2; // we fill 2 information (just the required ones)
	CW_CREATE_ARRAY_ERR((valPtr->vendorInfos), valPtr->vendorInfosCount, CWWTPVendorInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	// my vendor identifier (IANA assigned "SMI Network Management Private Enterprise Code")
	(valPtr->vendorInfos)[0].vendorIdentifier = 23456;
	(valPtr->vendorInfos)[0].type = CW_WTP_MODEL_NUMBER;
	(valPtr->vendorInfos)[0].length = sizeof(long int); // just one int
	CW_CREATE_OBJECT_SIZE_ERR(( ( (valPtr->vendorInfos)[0] ).valuePtr), (valPtr->vendorInfos)[0].length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*(int *)(((valPtr->vendorInfos)[0]).valuePtr) = 123456; // MODEL NUMBER

	// my vendor identifier (IANA assigned "SMI Network Management Private Enterprise Code")
	(valPtr->vendorInfos)[1].vendorIdentifier = 23456;
	(valPtr->vendorInfos)[1].type = CW_WTP_SERIAL_NUMBER;
	(valPtr->vendorInfos)[1].length = sizeof(long int); // just one int
	CW_CREATE_OBJECT_SIZE_ERR(( ( (valPtr->vendorInfos)[1] ).valuePtr), (valPtr->vendorInfos)[1].length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*(int *)(((valPtr->vendorInfos)[1]).valuePtr) = 123456; // SERIAL NUMBER
	
	return CW_TRUE;
}

CWBool CWWTPGetVendorInfos(CWWTPVendorInfos *valPtr) {
	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	valPtr->vendorInfosCount = 3; // we fill 3 information (just the required ones)
	CW_CREATE_ARRAY_ERR((valPtr->vendorInfos), valPtr->vendorInfosCount, CWWTPVendorInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	// my vendor identifier (IANA assigned "SMI Network Management Private Enterprise Code")
	(valPtr->vendorInfos)[0].vendorIdentifier = 23456;
	(valPtr->vendorInfos)[0].type = CW_WTP_HARDWARE_VERSION;
	(valPtr->vendorInfos)[0].length = sizeof(long int); // just one int
	CW_CREATE_OBJECT_SIZE_ERR(( ( (valPtr->vendorInfos)[0] ).valuePtr), (valPtr->vendorInfos)[0].length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*(int *)(((valPtr->vendorInfos)[0]).valuePtr) = 123456; // HW version

	// my vendor identifier (IANA assigned "SMI Network Management Private Enterprise Code")
	((valPtr->vendorInfos)[1]).vendorIdentifier = 23456;
	((valPtr->vendorInfos)[1]).type = CW_WTP_SOFTWARE_VERSION;
	((valPtr->vendorInfos)[1]).length = sizeof(long int); // just one int
	CW_CREATE_OBJECT_SIZE_ERR(( ( (valPtr->vendorInfos)[1] ).valuePtr), (valPtr->vendorInfos)[1].length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*(int *)(((valPtr->vendorInfos)[1]).valuePtr) = 12347; // SW version
	
	// my vendor identifier (IANA assigned "SMI Network Management Private Enterprise Code")
	(valPtr->vendorInfos)[2].vendorIdentifier = 23456;
	(valPtr->vendorInfos)[2].type = CW_BOOT_VERSION;
	(valPtr->vendorInfos)[2].length = sizeof(long int); // just one int
	CW_CREATE_OBJECT_SIZE_ERR(( ( (valPtr->vendorInfos)[2] ).valuePtr), (valPtr->vendorInfos)[2].length, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*(int *)(((valPtr->vendorInfos)[2]).valuePtr) = 1234568; // Boot version
	
	return CW_TRUE;
}

__inline__ void CWWTPDestroyVendorInfos(CWWTPVendorInfos *valPtr) {
	int i;
	
	if(valPtr == NULL) return;
	
	for(i = 0; i < valPtr->vendorInfosCount; i++) {
		CW_FREE_OBJECT((valPtr->vendorInfos)[i].valuePtr);
	}
	
	CW_FREE_OBJECT(valPtr->vendorInfos);
}

__inline__ int CWWTPGetFrameTunnelMode() {
	//it may be also 802.3_FrameTunnelMode - NativeFrameTunnelMode - All
	
#ifdef SOFTMAC
	return CW_NATIVE_BRIDGING;
#else
	return CW_LOCAL_BRIDGING;
#endif

}

__inline__ int CWWTPGetMACType() {
	

#ifdef SOFTMAC
	return CW_SPLIT_MAC;
#else
	return CW_LOCAL_MAC;
#endif

}

__inline__ char *CWWTPGetLocation() {
	return gWTPLocation;
}

__inline__ int CWWTPGetSessionID() {
	return CWRandomIntInRange(0, INT_MAX);
}

__inline__ int CWWTPGetIPv4Address() {
	struct sockaddr_in myAddr;
	unsigned int len = sizeof(myAddr);
	
	//CWDebugLog("WTPGetIPv4Address");

	/* assume the socket is connected */
	getsockname(gWTPSocket, (struct sockaddr*) &myAddr, &len);
	
	return ntohl(myAddr.sin_addr.s_addr); 	// TO-DO: this is garbage if we are an IPv6 client
}

__inline__ void CWWTPGetIPv6Address(struct sockaddr_in6* myAddr) {
	
	unsigned int len = sizeof(*myAddr);
	
	/* assume the socket is connected */
	getsockname(gWTPSocket, (struct sockaddr*) myAddr, &len);
}

__inline__ int CWWTPGetIPv4StatusDuplicate() {
	return gIPv4StatusDuplicate;
}

__inline__ int CWWTPGetIPv6StatusDuplicate() {
	return gIPv6StatusDuplicate;
}

__inline__ char *CWWTPGetName() {
	return gWTPName;
}

/*CWBool CWWTPGetRadiosInformation(CWRadiosInformation *valPtr) {
	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	valPtr->radiosCount = 2;
	
	CW_CREATE_ARRAY_ERR(valPtr->radios, valPtr->radiosCount, CWRadioInformationValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	(valPtr->radios)[0].ID = 0; // first radio
	(valPtr->radios)[0].type = CW_802_DOT_11b;
	
	(valPtr->radios)[1].ID = 1; // second radio
	(valPtr->radios)[1].type = CW_802_DOT_11b;
	
	return CW_TRUE;
}
*/

/* L'AC ha la funzione ridefinita */
CWBool CWGetWTPRadiosAdminState(CWRadiosAdminInfo *valPtr) 
{
	int i;

	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	valPtr->radiosCount = gRadiosInfo.radioCount;
	
	CW_CREATE_ARRAY_ERR(valPtr->radios, valPtr->radiosCount, CWRadioAdminInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	for (i=0; i<gRadiosInfo.radioCount; i++)
	{
		(valPtr->radios)[i].ID = gRadiosInfo.radiosInfo[i].radioID; // first radio
		(valPtr->radios)[i].state = gRadiosInfo.radiosInfo[i].adminState;
		(valPtr->radios)[i].cause = gRadiosInfo.radiosInfo[i].adminCause;
	}
		
	return CW_TRUE;
}

CWBool CWGetWTPRadiosOperationalState(int radioID, CWRadiosOperationalInfo *valPtr)
{
	int i;
	CWBool found = CW_FALSE;

	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(radioID<0) {
		
		valPtr->radiosCount = gRadiosInfo.radioCount;
		
		CW_CREATE_ARRAY_ERR(valPtr->radios, valPtr->radiosCount, CWRadioOperationalInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
		for (i=0; i<gRadiosInfo.radioCount; i++)
		{	
			(valPtr->radios)[i].ID = gRadiosInfo.radiosInfo[i].radioID;
			(valPtr->radios)[i].state = gRadiosInfo.radiosInfo[i].operationalState;
			(valPtr->radios)[i].cause = gRadiosInfo.radiosInfo[i].operationalCause;
		}
		return CW_TRUE;	
	}	
	else {
		for (i=0; i<gRadiosInfo.radioCount; i++)
		{
			if(gRadiosInfo.radiosInfo[i].radioID == radioID)
			{
				found = CW_TRUE;
				valPtr->radiosCount = 1;
				CW_CREATE_ARRAY_ERR(valPtr->radios, valPtr->radiosCount, CWRadioOperationalInfoValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				(valPtr->radios)[0].ID =  gRadiosInfo.radiosInfo[i].radioID;
				(valPtr->radios)[0].state = gRadiosInfo.radiosInfo[i].operationalState;
				(valPtr->radios)[0].cause = gRadiosInfo.radiosInfo[i].operationalCause;
				break;
			}
		}
		return found;
	}
}

CWBool CWGetDecryptErrorReport(int radioID, CWDecryptErrorReportInfo *valPtr)
{
	int i;
	CWBool found = CW_FALSE;
	
	/*
	CWMACAddress add, add2;
	for(i=0; i<6; i++) add[i]=i;
	for(i=0; i<6; i++) add2[i]=99;
	CWListElement elem,elem2;
	elem.data = add;
	elem.next = &elem2;
	elem2.data = &add2;
	elem2.next = NULL; 
	gRadiosInfo.radiosInfo[0].decryptErrorMACAddressList = &elem;
	*/

	if(valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	valPtr->radios=NULL;
	
	if(radioID<0) {
		
		valPtr->radiosCount = gRadiosInfo.radioCount;
		
		CW_CREATE_ARRAY_ERR(valPtr->radios, valPtr->radiosCount, CWDecryptErrorReportValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
		for (i=0; i<gRadiosInfo.radioCount; i++)
		{	
			(valPtr->radios)[i].ID = gRadiosInfo.radiosInfo[i].radioID;
			(valPtr->radios)[i].numEntries = CWCountElementInList(gRadiosInfo.radiosInfo[i].decryptErrorMACAddressList);
			(valPtr->radios[i]).decryptErrorMACAddressList = NULL;
			CW_CREATE_ARRAY_ERR((valPtr->radios[i]).decryptErrorMACAddressList, (valPtr->radios[i]).numEntries, CWMACAddress, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
			int j;
			CWListElement *temp;
			temp = gRadiosInfo.radiosInfo[i].decryptErrorMACAddressList;
			for(j=0; j<(valPtr->radios[i]).numEntries; j++)
			{
				CW_COPY_MEMORY((valPtr->radios[i]).decryptErrorMACAddressList[j],temp->data, sizeof(CWMACAddress));
				temp = temp->next;
			}
		}
		return CW_TRUE;	
	}	
	else {
		for (i=0; i<gRadiosInfo.radioCount; i++)
		{
			if(gRadiosInfo.radiosInfo[i].radioID == radioID)
			{
				found = CW_TRUE;
				valPtr->radiosCount = 1;
				CW_CREATE_ARRAY_ERR(valPtr->radios, valPtr->radiosCount, CWDecryptErrorReportValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				(valPtr->radios)[0].ID = gRadiosInfo.radiosInfo[i].radioID;
				(valPtr->radios)[0].numEntries = CWCountElementInList(gRadiosInfo.radiosInfo[i].decryptErrorMACAddressList);
				(valPtr->radios[0]).decryptErrorMACAddressList = NULL;
				CW_CREATE_ARRAY_ERR((valPtr->radios[0]).decryptErrorMACAddressList, (valPtr->radios[0]).numEntries, CWMACAddress, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				int j;
				CWListElement *temp;
				temp = gRadiosInfo.radiosInfo[0].decryptErrorMACAddressList;
				for(j=0; j<(valPtr->radios[0]).numEntries; j++)
				{
					CW_COPY_MEMORY((valPtr->radios[0]).decryptErrorMACAddressList[j],temp->data, 6);	
					temp = temp->next;
				}
			}
		}
		return found;
	}
}

int CWWTPGetACIndex()
{
	return 1; //valore predefinito
}

char* CWWTPGetACName()
{
	return gACInfoPtr->name;
}

int CWWTPGetStatisticsTimer ()
{
	return gWTPStatisticsTimer;
}

CWBool CWWTPGetACNameWithIndex (CWACNamesWithIndex *ACsInfo)
{
	if(ACsInfo == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	ACsInfo->count = 2;
	
	CW_CREATE_ARRAY_ERR(ACsInfo->ACNameIndex, ACsInfo->count, CWACNameWithIndexValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	(ACsInfo->ACNameIndex)[0].index = 0; // first radio
	CW_CREATE_STRING_FROM_STRING_ERR((ACsInfo->ACNameIndex)[0].ACName, "ACPrimary", return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	
	(ACsInfo->ACNameIndex)[1].index = 1; // first radio
	CW_CREATE_STRING_FROM_STRING_ERR((ACsInfo->ACNameIndex)[1].ACName, "ACSecondary", return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););;
	
	return CW_TRUE;
}

int getInterfaceMacAddr(char* interface, unsigned char* macAddr)
{
	struct ifreq ethreq;
	int i,sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		CWLog("Error Creating Socket for ioctl"); 
		return -1;
	}
	
	memset(&ethreq, 0, sizeof(ethreq));
	strncpy(ethreq.ifr_name, interface, IFNAMSIZ);
	if (ioctl(sock, SIOCGIFHWADDR, &ethreq)==-1) {
		return -1;
	}

	for (i=0; i<MAC_ADDR_LEN; i++){
		macAddr[i]=(unsigned char)ethreq.ifr_hwaddr.sa_data[i];
	}

	return 0;
}

int initWTPSessionID(char * sessionID)
{
	unsigned char macAddr0[MAC_ADDR_LEN];
	unsigned char macAddr1[MAC_ADDR_LEN];
	int i,randomInteger;
	char *buffer=sessionID;

	getInterfaceMacAddr(gEthInterfaceName,macAddr0);
	getInterfaceMacAddr(gRadioInterfaceName_0,macAddr1);
	randomInteger = CWRandomIntInRange(0, INT_MAX);
	randomInteger = htonl(randomInteger);

	CW_COPY_MEMORY(sessionID, macAddr0, MAC_ADDR_LEN);
	CW_COPY_MEMORY(&(sessionID[MAC_ADDR_LEN]), macAddr1, MAC_ADDR_LEN);
	CW_COPY_MEMORY(&(sessionID[MAC_ADDR_LEN+MAC_ADDR_LEN]),&(randomInteger), 4);

	for (i=0;i<16;i++)
	{
		if (i%16==0) printf("\n%04x:   ", i);
		printf("%02x:", buffer[i]);
	}
	printf("\n");	

	return 0;
}

