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


#ifndef __CAPWAP_ACProtocol_HEADER__
#define __CAPWAP_ACProtocol_HEADER__

//#define UNUSED_RADIO_ID 	1000
 
 typedef struct{
 	char *locationData;
	char *name;
	char *sessionID;
	CWWTPDescriptor descriptor;
	struct sockaddr_in ipv4Address;
	
	CWWTPRadiosInfo radiosInfo;

	char *ACName;
	CWACNamesWithIndex ACNameIndex;
	CWRadiosAdminInfo radioAdminInfo;
	int StatisticsTimer;
	CWWTPVendorInfos WTPBoardData;
	//CWRadiosInformation WTPRadioInfo;	
	WTPRebootStatisticsInfo *WTPRebootStatistics;

	void* bindingValuesPtr;
 } CWWTPProtocolManager;

typedef struct {
	char *location;
	char *name;
	CWWTPVendorInfos WTPBoardData;
	char *sessionID;
	CWWTPDescriptor WTPDescriptor;
	struct sockaddr_in addr;
	CWframeTunnelMode frameTunnelMode;
	CWMACType MACType;

	
} CWProtocolJoinRequestValues;


typedef struct {
	char *ACName;
	CWACNamesWithIndex ACinWTP;
	int radioAdminInfoCount;
	CWRadioAdminInfoValues *radioAdminInfo;
	int StatisticsTimer;
	WTPRebootStatisticsInfo *WTPRebootStatistics;
	
} CWProtocolConfigureRequestValues;

typedef struct {
	CWRadiosOperationalInfo radioOperationalInfo;
	CWProtocolResultCode resultCode;
} CWProtocolChangeStateEventRequestValues;

typedef struct{
	unsigned int radioID;
	unsigned int TxQueueLevel;
	unsigned int wirelessLinkFramesPerSec;
} WTPOperationalStatisticsValues;

typedef struct{
	unsigned int radioID;
	WTPRadioStatisticsInfo WTPRadioStatistics;
} WTPRadioStatisticsValues;

typedef struct {
	int ipv4Address;
	unsigned int length;
	unsigned char *MACoffendingDevice_forIpv4;
	int status;
} WTPDuplicateIPv4;

typedef struct {
	struct in6_addr ipv6Address;
	unsigned int length;
	unsigned char *MACoffendingDevice_forIpv6;
	int status;
} WTPDuplicateIPv6;

typedef struct{
	int errorReportCount;
	CWDecryptErrorReportValues *errorReport;
	WTPDuplicateIPv4 *duplicateIPv4;
	WTPDuplicateIPv6 *duplicateIPv6;
	int WTPOperationalStatisticsCount;
	WTPOperationalStatisticsValues *WTPOperationalStatistics;
	int WTPRadioStatisticsCount;
	WTPRadioStatisticsValues *WTPRadioStatistics;
	WTPRebootStatisticsInfo *WTPRebootStatistics;
} CWProtocolWTPEventRequestValues;

typedef struct {

	int data;
	int length;
	char *debug_info;
} CWProtocolWTPDataTransferRequestValues;

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */
CWBool CWParseChangeStateEventRequestMessage (char *msg, int len, int *seqNumPtr, CWProtocolChangeStateEventRequestValues *valuesPtr);
CWBool CWAssembleChangeStateEventResponse(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum);


CWBool CWAssembleMsgElemACDescriptor(CWProtocolMessage *msgPtr);			// 1
CWBool CWAssembleMsgElemACIPv4List(CWProtocolMessage *msgPtr);				// 2
CWBool CWAssembleMsgElemACIPv6List(CWProtocolMessage *msgPtr);				// 3
CWBool CWAssembleMsgElemACName(CWProtocolMessage *msgPtr);				// 4
CWBool CWAssembleMsgElemAddStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr);		// 8
CWBool CWAssembleMsgElemDeleteStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr);		// 8
CWBool CWAssembleMsgElemCWControlIPv4Addresses(CWProtocolMessage *msgPtr);		//10
CWBool CWAssembleMsgElemCWControlIPv6Addresses(CWProtocolMessage *msgPtr);		//11
CWBool CWAssembleMsgElemCWTimer(CWProtocolMessage *msgPtr);				//12
CWBool CWAssembleMsgElemDecryptErrorReportPeriod(CWProtocolMessage *msgPtr);		//16
CWBool CWAssembleMsgElemIdleTimeout(CWProtocolMessage *msgPtr);				//23
CWBool CWAssembleMsgElemWTPFallback(CWProtocolMessage *msgPtr);				//37

//---------------------------------------------------------/

//CWBool CWParseACName(CWProtocolMessage *msgPtr, int len, char **valPtr);
CWBool CWParseACNameWithIndex(CWProtocolMessage *msgPtr, int len, CWACNameWithIndexValues *valPtr);			// 5
CWBool CWParseMsgElemDataTransferData(CWProtocolMessage *msgPtr, int len, CWProtocolWTPDataTransferRequestValues *valPtr);	//13
CWBool CWParseDiscoveryType(CWProtocolMessage *msgPtr, int len, CWDiscoveryRequestValues *valPtr);			//20
CWBool CWParseMsgElemDuplicateIPv4Address(CWProtocolMessage *msgPtr, int len, WTPDuplicateIPv4 *valPtr); 		//21
CWBool CWParseLocationData(CWProtocolMessage *msgPtr, int len, char **valPtr);						//27
CWBool CWParseWTPRadioAdminState (CWProtocolMessage *msgPtr, int len, CWRadioAdminInfoValues *valPtr);		//29 
CWBool CWParseWTPStatisticsTimer(CWProtocolMessage *msgPtr, int len, int *valPtr);					//33 
CWBool CWParseWTPBoardData(CWProtocolMessage *msgPtr, int len, CWWTPVendorInfos *valPtr);				//35 
CWBool CWParseWTPDescriptor(CWProtocolMessage *msgPtr, int len, CWWTPDescriptor *valPtr);				//37 
CWBool CWParseWTPFrameTunnelMode(CWProtocolMessage *msgPtr, int len, CWframeTunnelMode *valPtr);			//38
CWBool CWParseWTPIPv4Address(CWProtocolMessage *msgPtr, int len, CWProtocolJoinRequestValues *valPtr);			//39 
CWBool CWParseWTPMACType(CWProtocolMessage *msgPtr, int len, CWMACType *valPtr);				//40
CWBool CWParseWTPName(CWProtocolMessage *msgPtr, int len, char **valPtr);						//41
CWBool CWParseWTPOperationalStatistics(CWProtocolMessage *msgPtr, int len, WTPOperationalStatisticsValues *valPtr);	//42
CWBool CWParseWTPRadioStatistics(CWProtocolMessage *msgPtr, int len, WTPRadioStatisticsValues *valPtr); 		//43
CWBool CWParseWTPRebootStatistics(CWProtocolMessage *msgPtr, int len, WTPRebootStatisticsInfo *valPtr);			//44 
CWBool CWParseMsgElemDecryptErrorReport(CWProtocolMessage *msgPtr, int len, CWDecryptErrorReportValues *valPtr);
CWBool CWParseMsgElemDuplicateIPv6Address(CWProtocolMessage *msgPtr, int len, WTPDuplicateIPv6 *valPtr);
CWBool CWParseWTPRadioInformation(CWProtocolMessage *msgPtr, int len, unsigned char *valPtr);	//1048
CWBool CWParseWTPSupportedRates(CWProtocolMessage *msgPtr, int len, unsigned char *valPtr);	//1040
CWBool CWParseWTPMultiDomainCapability(CWProtocolMessage *msgPtr, int len, char *valPtr);	//1032
//CWBool CWParseWTPRadioInfo(CWProtocolMessage *msgPtr, int len, CWRadiosInformation *valPtr, int radioIndex);	

//---------------------------------------------------------/
CWBool CWACGetACIPv4List(int **listPtr, int *countPtr);
CWBool CWACGetACIPv6List(struct in6_addr **listPtr, int *countPtr);
char *CWACGetName(void);
int CWACGetHWVersion(void);
int CWACGetSWVersion(void);
int CWACGetStations(void);
int CWACGetLimit(void);
int CWACGetActiveWTPs(void);
int CWACGetMaxWTPs(void);
int CWACGetSecurity(void);
int CWACGetInterfacesCount(void);
int CWACGetInterfaceIPv4AddressAtIndex(int i);
char *CWACGetInterfaceIPv6AddressAtIndex(int i);
int CWACGetInterfaceWTPCountAtIndex(int i);
CWBool CWACGetDiscoveryTimer(int *timer);
CWBool CWACGetEchoRequestTimer(int *timer);
CWBool CWACGetIdleTimeout(int *timer);
CWBool CWGetWTPRadiosOperationalState(int radioID, CWRadiosOperationalInfo *valPtr);

//---------------------------------------------------------/
CWBool CWACSupportIPv6();
void CWDestroyDiscoveryRequestValues(CWDiscoveryRequestValues *valPtr);

CWBool CWProtocolAssembleConfigurationUpdateRequest(CWProtocolMessage **msgElems, int *msgElemCountPtr, int MsgElementType);
#endif
